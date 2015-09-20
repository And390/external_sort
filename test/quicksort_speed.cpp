#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <chrono>

#include "../src/util.cpp"


using namespace std;


uint4 rand4()  {  return uint4(rand()&0xFFFF) + (uint4(rand())<<16);  }

uint8 current_time_ms()  {
    using namespace chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

template<class t>  void quick_sort_(t* pbeg, t* plast)
{
    t* p1 = pbeg;
    t* p2 = plast;
    t mid = p1[(plast-p1+1)/2];

    do  {
        while (*p1 < mid)  p1++;
        while (*p2 > mid)  p2--;

        if (p1 <= p2)  {
            t temp = *p1;  *p1 = *p2;  *p2 = temp;
            p1++;  p2--;
        }
    }  while (p1 <= p2);

    // рекурсивные вызовы, если есть, что сортировать
    if (p2 > pbeg)  quick_sort_(pbeg, p2);
    if (p1 < plast)  quick_sort_(p1, plast);
}

template<class t>  void quick_sort(t* pbeg, t* pend)
{
    if (pend-1 > pbeg)  quick_sort_(pbeg, pend-1);
}

void test_correct()
{
    cuint ITEMS = 1000*1000;

    //  create and fill
    puint4 data = new uint4 [ITEMS];
    for (puint4 p=data; p!=data+ITEMS; p++)  *p = rand4();
    puint4 data2 = new uint4 [ITEMS];
    memcpy(data2, data, 4*ITEMS);

    //  sort
    quick_sort(data, data+ITEMS);
    sort(data2, data2+ITEMS);

    //  check
    for (puint4 p=data+1; p<data+ITEMS; p++)  if (p[0] < p[-1])  throw string("extsort::quick_sort order error");
    for (puint4 p=data2+1; p<data2+ITEMS; p++)  if (p[0] < p[-1])  throw string("std::sort order error");
    if (memcmp(data, data2, 4*ITEMS) != 0)  throw string("sort results are not match");

    //  clear
    delete [] data;
    delete [] data2;
}

int comp(pcvoid p1, pcvoid p2)  {  return puint4(p1)-puint4(p2);  }

void test_speed()
{
    cuint CYCLES = 3;
    cuint ITEMS = 100*1000*1000;

    //  create
    puint4 source = new uint4 [ITEMS];
    puint4 data = new uint4 [ITEMS];

    //  fill
    for (puint4 p=source; p!=source+ITEMS; p++)  *p = rand4();

    for (uint c=0; c<CYCLES; c++)  {
        //  my quick_sort
        memcpy(data, source, 4*ITEMS);
        auto time = current_time_ms();
        quick_sort(data, data+ITEMS);
        time = current_time_ms() - time;
        cout << "my quick_sort  " << time << " ms" << endl;

        //  std::sort
        memcpy(data, source, 4*ITEMS);
        time = current_time_ms();
        sort(data, data+ITEMS);
        time = current_time_ms() - time;
        cout << "std::sort      " << time << " ms" << endl;
        
        //  qsort
        memcpy(data, source, 4*ITEMS);
        time = current_time_ms();
        qsort(data, ITEMS, 4, comp);
        time = current_time_ms() - time;
        cout << "qsort          " << time << " ms" << endl;
    }

    //  clear
    delete [] source;
    delete [] data;
}

void test()
{
    test_correct();
    test_speed();
}

int main(int argc, pcchar* argv)
{
    try  {  test();  cout << "SUCCESS" << endl;  return 0;  }
    catch (string s)  {  cout << s << endl;  return -1;  }
}
