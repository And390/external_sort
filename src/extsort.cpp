#include "util.cpp"
#include <iostream>
#include <fstream>
#include <stdlib.h>
//#include <functional>  //
#include <algorithm>  //for sort

namespace extsort  {


//using namespace std;


cpvoid END_OF_DATA = NULL;
cpvoid END_OF_BUFF = pbyte(END_OF_DATA)+1;
    
typedef pvoid (*f_produce) (pvoid data, size_t capacity, pvoid context);
    
typedef size_t (*f_get_size) (pvoid data, pvoid context);

typedef bool (*f_compare) (pvoid p1, pvoid p2);

struct pointer_comparator  {
    f_compare compare;
    pointer_comparator(f_compare compare_) : compare(compare_)  {}
    bool operator() (pvoid p1, pvoid p2)  {  return compare(p1, p2);  }
};

// TODO private
template<class T>
void write(std::ofstream& file, T data)  {  file.write(pcchar(&data), sizeof(data));  }

void sort(pcchar filename, int buff_size, 
          f_produce produce, f_get_size get_size, f_compare compare, pvoid context)
{
    //    step 1. Make sorted parts of BUFF_SIZE and write it to file
    std::ofstream file(filename, std::ios::binary);
    bool first = true;  // if all data can fit in one buffer then no need to merge
    pbyte buff = new byte [buff_size];
    pbyte buff_end = buff+buff_size;
    for (;;)  {
        //    write next part of items until buffer is not filled
        pbyte pdata = buff;  // в начало буфера записываются данные элементов
        pbyte paddr = buff_end - sizeof(pvoid);  // в конец - указатели на них
        pbyte plast;
        while (paddr > pdata)  {
            //    read next data
            plast = pdata;
            pdata = (pbyte) produce(plast, paddr-pdata, context);
            if (pdata==END_OF_BUFF || pdata==END_OF_DATA)  break;
            if (DEBUG && (pdata<=plast || pdata>paddr))  throw std::string("wrong data pointer returned by 'produce'");
            //    save pointer
            *(pvoid*)(paddr) = plast;
            paddr -= sizeof(pvoid);  //на протяжении цикла pend указывает перед последним записанным указателем
        }
        paddr += sizeof(pvoid);
        if (paddr==buff_end && pdata!=END_OF_DATA)  throw std::string("too large element");
        //    sort
        std::sort((pvoid*)paddr, (pvoid*)(buff_end), compare);
        //qsort(pend, (BUFF + BUFF_SIZE - pend) / sizeof(pvoid), sizeof(pvoid), compare);
        //    write buffer to file
        if (!first || pdata!=END_OF_DATA)  write(file, size_t(plast-buff));
        for (pvoid* p=(pvoid*)paddr; p!=(pvoid*)buff_end; p++)  
            file.write(pcchar(*p), get_size(*p, context));
        //
        if (pdata==END_OF_DATA)  break;
    }
    delete [] buff;
    
    //    step 2. Merge sorted parts
}


}
