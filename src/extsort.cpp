#include "util.cpp"
#include <climits>
#include <fstream>
#include <cstdio>
#include <stdlib.h>
#include <memory>
#include <algorithm>


namespace extsort  {


cpvoid END_OF_DATA = NULL;
cpvoid END_OF_BUFF = pbyte(END_OF_DATA)+1;

// function that writes next item data info buffer
typedef pvoid (*f_produce) (pvoid data, size_t capacity, pvoid context);

// return a size of the item data; data stored up to the end pointer and item can be not loaded fully
// so, if function can't calculate size because of that it must return 0
typedef size_t (*f_get_size) (pvoid data, pvoid end, pvoid context);

//
typedef bool (*f_compare) (pvoid p1, pvoid p2);

struct pointer_comparator  {
    f_compare compare;
    pointer_comparator(f_compare compare_) : compare(compare_)  {}
    bool operator() (pvoid p1, pvoid p2)  {  return compare(p1, p2);  }
};

// TODO private
template<class T>
void write(std::fstream& file, T data)  {  file.write(pcchar(&data), sizeof(data));  }

// do not work for a=0
template<class T>  T align(T a, T b)  {  return a + b - ((a-1)%b+1);  }
template<class T>  T floor(T a, T b)  {  return a - a%b;  }

const size_t BLOCK_SIZE = 8192;

// TODO separate it into 2 parts and test (first part)

void sort(pcchar filename, pcchar temp_filename, size_t buff_size,
          f_produce produce, f_get_size get_size, f_compare compare, pvoid context)
{
    using std::streampos;
    using std::min;

    // TODO check buffer size first

    // TODO check input parameters

    // TODO check get_size returns on debug

    // TODO set throw exceptions on file writing/reading

    //    step 1. Make sorted parts of BUFF_SIZE and write it to file
    std::fstream file(temp_filename, std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc);
    if (!file)  throw mk_string("error opening file for write: ", temp_filename);
    std::unique_ptr<byte[]> buff_ptr(new byte [buff_size]);
    pbyte buff = buff_ptr.get();
    pbyte buff_end = buff+buff_size;
    uint parts_count = 0;
    uint max_item_size = 0;
    for (;;)  {
        //    read next part of items until buffer is not filled
        pbyte pdata = buff;  // в начало буфера записываются данные элементов (указатель на конец этих данных)
        pbyte paddr = buff_end - sizeof(pvoid);  // в конец - указатели на них
        pbyte pnext;
        do  {
            //    read next data
            pnext = (pbyte) produce(pdata, paddr-pdata, context);
            if (pnext==END_OF_BUFF || pnext==END_OF_DATA)  break;
            if (DEBUG && (pnext<=pdata || pnext>paddr))  throw std::string("wrong data pointer returned by 'produce'");
            size_t size = pnext - pdata;
            if (size>max_item_size)  max_item_size = size;
            //    save pointer
            *(pvoid*)(paddr) = pdata;
            pdata = pnext;
            paddr -= sizeof(pvoid);  //на протяжении цикла pend указывает перед последним записанным указателем
        }  while (paddr > pdata);
        paddr += sizeof(pvoid);
        if (paddr==buff_end && pnext!=END_OF_DATA)  throw std::string("too large element");
        //    sort
        std::sort((pvoid*)paddr, (pvoid*)(buff_end), compare);
        //    write buffer to file; do not write size of part if it was only one part
        size_t read_size = pdata-buff;
        if (parts_count!=0 || pnext!=END_OF_DATA)  {
            write(file, read_size);
            if (!file)  throw std::string("file writing error");
        }
        size_t written_size = 0;
        for (pvoid* p=(pvoid*)paddr; p!=(pvoid*)buff_end; p++)  {
            size_t size = get_size(*p, paddr, context);
            written_size += size;
            file.write(pcchar(*p), size);
            if (!file)  throw std::string("file writing error");
        }
        if (written_size != read_size)
            throw mk_string("written size do not match with read size (likely a get_size error): ",
                            written_size, " <> ", read_size);
        //    align
        size_t aligned_size = align(read_size + sizeof(size_t), BLOCK_SIZE);
        file.seekg(aligned_size - read_size - sizeof(size_t), std::ios::cur);
        //
        parts_count++;
        if (pnext==END_OF_DATA)  break;
    }

    //    if it was only one part then return it immediately just renaming file
    if (parts_count==1)  {
        file.close();
        if (rename(temp_filename, filename) != 0)
            throw std::string("can't rename temp file");
        return;
    }
    file.flush();

    //    step 2. Merge sorted parts
    // buffer divided into parts_count buffer parts, for reading previously sorted parts
    size_t part_buff_size = buff_size/parts_count;
    size_t part_read_size = floor(part_buff_size, BLOCK_SIZE);
    // TODO check part_buff_size for min value
    if (max_item_size > part_read_size)  // TODO this can be checked on Step 1 (iteratively)
        throw mk_string("item is too large and can't fit in the buffer: ",
                max_item_size, " > ", part_read_size);
    // также есть риск, что окажется не достаточно буфера, если part_buff_size - (max_item_size-1) < BLOCK_SIZE,
    // т.е. в буфере останется хвост такой, что не хватит места, чтобы прочитать ни один блок с диска
    std::unique_ptr<pbyte[]> part_buffs(new pbyte [parts_count]);  //start of buffers
    std::unique_ptr<pbyte[]> part_currs(new pbyte [parts_count]);  //current item pointers
    std::unique_ptr<pbyte[]> part_pends(new pbyte [parts_count]);  //end of read part in buffer  //TODO pends -> capacity
    std::unique_ptr<size_t[]> part_rests(new size_t [parts_count]);  //remain of data on disk to read
    std::unique_ptr<streampos[]> part_seeks(new streampos [parts_count]);  //current file read positions
    //    read first pieces of each part
    streampos file_pos = 0;
    file.seekg(0, std::ios::beg);
    for (uint p=0; p!=parts_count; p++)  {
        pbyte ptr = buff + p * part_buff_size;
        file.read(pchar(ptr), part_read_size);
        if (!file)  throw std::string("error reading file");
        size_t part_size = *(size_t*)ptr;
        if (part_size == 0 || part_size >= buff_size)  throw mk_string("wrong part size written: ", part_size);

        part_buffs[p] = ptr;
        part_currs[p] = ptr + sizeof(size_t);
        part_pends[p] = ptr + part_read_size;
        part_rests[p] = part_size - part_read_size + sizeof(size_t);
        part_seeks[p] = file_pos + streampos(part_read_size);

        size_t aligned_size = align(part_size + sizeof(size_t), BLOCK_SIZE);
        file_pos += aligned_size;
        file.seekg(file_pos, std::ios::beg);
    }
    //    merge
    std::fstream ofile(filename, std::fstream::out | std::fstream::binary | std::fstream::trunc);
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
        size_t item_size = get_size(part_currs[imin], part_pends[imin], context);
        size_t tail = part_pends[imin] - part_currs[imin];
        //TODO check item_size==0 || >tail
        ofile.write(pchar(part_currs[imin]), item_size);
        if (!ofile)  throw std::string("error writing file");
        part_currs[imin] += item_size;
        tail -= item_size;
        //    ensure next item is loaded
        if (tail!=0)  item_size = get_size(part_currs[imin], part_pends[imin], context);
        else  item_size = 0;
        if (item_size==0 || item_size>tail)  {
            if (part_rests[imin] != 0)  {
                //    read next portion of the part
                memmove(part_buffs[imin], part_currs[imin], tail);
                file.seekg(part_seeks[imin], std::ios::beg);
                size_t read_size = floor(part_buff_size-tail, BLOCK_SIZE);
                if (read_size==0)  throw std::string("too long data item tail size: ", tail);  //possible
                read_size = min(min(read_size, part_read_size), part_rests[imin]);
                file.read(pchar(part_buffs[imin]+tail), read_size);
                if (!file)  throw std::string("error reading file");

                part_currs[imin] = part_buffs[imin];
                part_pends[imin] = part_buffs[imin]+tail+read_size;
                part_rests[imin] -= read_size;
                part_seeks[imin] += read_size;
            }
            else  {
                //    buffer is empty
                part_currs[imin] = NULL;
            }
        }
    }
}


}
