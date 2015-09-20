#include "util.cpp"
#include <climits>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <algorithm>
#include <cassert>


namespace extsort  {


// function that writes next item data info buffer with available capacity
// returns pointer to end of written data (so: size = return - 'data')
// if capacity is not enough then it must return END_OF_BUFF
// if no more items to produce then it must return END_OF_BUFF
typedef pvoid (*f_produce) (pvoid data, size_t capacity, pvoid context);

cpvoid END_OF_DATA = NULL;
cpvoid END_OF_BUFF = pbyte(END_OF_DATA)+1;

// return a size of the item data; data stored up to the capacity and item can be not loaded fully
// so, if function can't calculate size because capacity is not enough then it must return 0
typedef size_t (*f_get_size) (pvoid data, size_t capacity, pvoid context);

// true if p1 > p2, i.e. it needs to swap the items to achieve order
typedef bool (*f_compare) (pvoid p1, pvoid p2);

// context provided on the will of the user


const size_t BLOCK_SIZE = 8192;


namespace internal  {
    // do not work for a=0
    template<class T>  T align(T a, T b)  {  return a + b - ((a-1)%b+1);  }
    template<class T>  T floor(T a, T b)  {  return a - a%b;  }
}


void sort(pcchar filename, pcchar temp_filename, size_t buff_size,
          f_produce produce, f_get_size get_size, f_compare compare, pvoid context)
{
    using namespace internal;
    using namespace std;

    if (buff_size<BLOCK_SIZE)  throw mk_string("buffer can't be less than ", BLOCK_SIZE);

    //    step 1. Make sorted parts of BUFF_SIZE and write it to file
    fstream file(temp_filename, ios::in | ios::out | ios::binary | ios::trunc);
    file.exceptions(ios::failbit | ios::badbit | ios::eofbit);
    unique_ptr<byte[]> buff_ptr(new byte [buff_size]);
    pbyte buff = buff_ptr.get();
    pbyte buff_end = buff+buff_size;
    uint parts_count = 0;
    uint max_item_size = 0;
    uint total = 0;  //total data size
    size_t part_buff_size;  //initialized here but used in Step 2 (see below)
    size_t part_read_size;
    for (;;)  {
        //    calculate and check part buffer sizes
        parts_count++;
        part_buff_size = buff_size/parts_count;
        part_read_size = floor(part_buff_size, BLOCK_SIZE);
        if (part_read_size == 0)  throw mk_string("buffer size ", buff_size, " is not enough for data size ", total,
                                                  " (", parts_count, " parts, each must be > ", BLOCK_SIZE, " bytes)");
        if (max_item_size > part_read_size)  throw mk_string("item is too large and can't fit in the buffer: ",
                                                             max_item_size, " > ", part_read_size);
        // ����� ���� ����, ��� �������� �� ���������� ������, ���� part_buff_size - (max_item_size-1) < BLOCK_SIZE,
        // �.�. � ������ ��������� ����� �����, ��� �� ������ �����, ����� ��������� �� ���� ���� � �����
        //    read next part of items until buffer is not filled
        pbyte pdata = buff;  // � ������ ������ ������������ ������ ��������� (��������� �� ����� ���� ������)
        pbyte paddr = buff_end - sizeof(pvoid);  // � ����� - ��������� �� ���
        pbyte pnext;
        do  {
            //    read next data
            pnext = (pbyte) produce(pdata, paddr-pdata, context);
            if (pnext==END_OF_BUFF || pnext==END_OF_DATA)  break;
            if (DEBUG && (pnext<=pdata || pnext>paddr))  throw string("wrong data pointer returned by 'produce'");
            size_t size = pnext - pdata;
            total += size;
            if (size>max_item_size)  max_item_size = size;
            //    save pointer
            *(pvoid*)(paddr) = pdata;
            pdata = pnext;
            paddr -= sizeof(pvoid);  //�� ���������� ����� pend ��������� ����� ��������� ���������� ����������
        }  while (paddr > pdata + sizeof(size_t));  // + ����� �� �������� �������, ������� ����� �������� � ������
        paddr += sizeof(pvoid);
        if (paddr==buff_end && pnext!=END_OF_DATA)  throw string("too large element");
        //    sort
        std::sort((pvoid*)paddr, (pvoid*)(buff_end), compare);
        //    write buffer to file; do not write size of part if it was only one part
        size_t read_size = pdata-buff;
        if (parts_count!=1 || pnext!=END_OF_DATA)  {
            file.write(pcchar(&read_size), sizeof(read_size));
        }
        size_t written_size = 0;
        for (pvoid* p=(pvoid*)paddr; p!=(pvoid*)buff_end; p++)  {
            size_t size = get_size(*p, pdata-pbyte(*p), context);
            if (DEBUG && (size==0 || pbyte(*p)+size>pdata))  throw mk_string("wrong size returned by 'get_size': ", size);
            written_size += size;
            file.write(pcchar(*p), size);
        }
        if (written_size != read_size)
            throw mk_string("written size do not match with read size (likely a get_size error): ",
                            written_size, " <> ", read_size);
        //    align
        size_t aligned_size = align(read_size + sizeof(size_t), BLOCK_SIZE);
        file.seekg(aligned_size - read_size - sizeof(size_t), ios::cur);
        //
        if (pnext==END_OF_DATA)  break;
    }

    //    if it was only one part then return it immediately just renaming file
    if (parts_count==1)  {
        file.close();
        if (rename(temp_filename, filename) != 0)
            throw string("can't rename temp file");
        return;
    }
    file.flush();

    //    step 2. Merge sorted parts
    // buffer divided into parts_count buffer parts, for reading previously sorted parts
    unique_ptr<pbyte[]> part_buffs(new pbyte [parts_count]);  //start of buffers
    unique_ptr<pbyte[]> part_currs(new pbyte [parts_count]);  //current item pointers
    unique_ptr<size_t[]> part_sizes(new size_t [parts_count]);  //remain read part in buffer
    unique_ptr<size_t[]> part_rests(new size_t [parts_count]);  //remain of data on disk to read
    unique_ptr<streampos[]> part_seeks(new streampos [parts_count]);  //current file read positions
    //    read first pieces of each part
    streampos file_pos = 0;
    file.seekg(0, ios::beg);
    for (uint p=0; p!=parts_count; p++)  {
        pbyte ptr = buff + p * part_buff_size;
        file.read(pchar(ptr), part_read_size);
        size_t part_size = *(size_t*)ptr;
        if (part_size == 0 || part_size >= buff_size)  throw mk_string("wrong part size readed: ", part_size);

        part_buffs[p] = ptr;
        part_currs[p] = ptr + sizeof(size_t);
        part_sizes[p] = part_read_size - sizeof(size_t);
        part_rests[p] = part_size - part_read_size + sizeof(size_t);
        part_seeks[p] = file_pos + streampos(part_read_size);

        size_t aligned_size = align(part_size + sizeof(size_t), BLOCK_SIZE);
        file_pos += aligned_size;
        file.seekg(file_pos, ios::beg);
    }
    //    merge
    fstream ofile(filename, fstream::out | fstream::binary | fstream::trunc);
    ofile.exceptions(ios::failbit | ios::badbit | ios::eofbit);
    for (;;)
    {
        //    find minimum item
        uint imin = UINT_MAX;
        for (uint p=0; p!=parts_count; p++)  if (part_currs[p]!=NULL)  {  // check buffer available
            if (imin==UINT_MAX)  imin = p;
            else if (compare(part_currs[p], part_currs[imin]))  imin = p;
        }
        if (imin==UINT_MAX)  break;
        //    write item and remove found item from buffer (shift pointer)
        size_t item_size = get_size(part_currs[imin], part_sizes[imin], context);
        if (DEBUG && item_size==0)  throw mk_string("'get_size' must return non null size for fully loaded item");
        if (DEBUG && item_size>part_sizes[imin])  throw mk_string("wrong size returned by 'get_size': ",
                                                                  item_size, " > ", part_sizes[imin]);
        ofile.write(pchar(part_currs[imin]), item_size);
        if (!ofile)  throw string("error writing file");
        part_currs[imin] += item_size;
        part_sizes[imin] -= item_size;
        //    ensure next item is loaded
        size_t tail = part_sizes[imin];
        if (tail!=0)  item_size = get_size(part_currs[imin], part_sizes[imin], context);
        else  item_size = 0;
        if (item_size==0 || item_size>tail)  {
            if (DEBUG && item_size>part_rests[imin])  throw mk_string("wrong size returned by 'get_size': ",
                                                                      item_size, " > ", part_rests[imin]);
            if (part_rests[imin] != 0)  {
                //    read next portion of the part
                memmove(part_buffs[imin], part_currs[imin], tail);
                file.seekg(part_seeks[imin], ios::beg);
                size_t read_size = floor(part_buff_size-tail, BLOCK_SIZE);
                if (read_size==0)  throw string("too long data item tail size: ", tail);  //possible
                read_size = min(min(read_size, part_read_size), part_rests[imin]);
                file.read(pchar(part_buffs[imin]+tail), read_size);

                part_currs[imin] = part_buffs[imin];
                part_sizes[imin] = tail + read_size;
                part_rests[imin] -= read_size;
                part_seeks[imin] += read_size;
            }
            else  {
                //    buffer is empty
                part_currs[imin] = NULL;
            }
        }
    }

    //    delete temp file
    ofile.close();
    file.close();
    if (remove(temp_filename) != 0)
        throw string("can't delete temp file");
}


}
