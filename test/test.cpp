#include <iostream>
#include <cstdio> 
#include <string.h>
#include "../src/extsort.cpp"

using namespace std;
using namespace extsort;


struct context
{
    uint a;
    context() : a(0)  {}
};
typedef context* pcontext;

bool randb()  {  return rand()>RAND_MAX/2;  }
uint rand(uint limit)  {  return rand()%limit;  }

pvoid produce (pvoid data, size_t capacity, pvoid ctx)  {
    pbyte p = (pbyte)data;
    for (;;)  {
        if (capacity==0)  return END_OF_DATA;
        if (rand()%100==0)  break;
        *(p++) = 'a'+rand('z'-'a'+1);
        capacity--;
    }
    *(p++) = '\0';
    return p;
}

pvoid produce_2 (pvoid data, size_t capacity, pvoid ctx)  {
    if (pcontext(ctx)->a==20)  return END_OF_DATA;
    pbyte p = pbyte(data);
    *(p++) = 'a' + pcontext(ctx)->a++;
    *(p++) = '\0';
    return p;
}

bool compare (pvoid p1, pvoid p2) {
    pcchar s1 = (pcchar)p1;
    pcchar s2 = (pcchar)p2;
    return strcmp(s1,s2)<0;
}

size_t get_size(pvoid p, pvoid ctx)  {  return strlen((pcchar)p)+1;  }

pcchar TEST_FILENAME = "test-file";

bool test()
{
    //    read file
    ifstream file(TEST_FILENAME, ios::binary);
    file.seekg(0, ios::end);
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);
    pbyte buffer = new byte [size];
    file.read((pchar)buffer, size);
    file.close();
    //    check sorted
    bool result = true;
    for (pbyte p1=buffer;;)  {
        pbyte p2 = p1 + strlen(pcchar(p1)) + 1;
        if (p2 == buffer+size)  break;
        if (!compare(p1,p2))  {
            cerr << "wrong order:\n" << pcchar(p1) << "\n" << pcchar(p2) << "\n";
            result = false;
            break;
        }
        p1 = p2;
    }
    //    clear and exit
    delete [] buffer;
    remove(TEST_FILENAME);
    return result;
}

int main(int args, pcchar* argv)
{
    if (DEBUG)  cout << "DEBUG" << endl;
    context ctx;
    sort(TEST_FILENAME, 2000, produce, get_size, compare, &ctx);
    bool success = test();
    cout << (success ? "success" : "fail") << endl;
    return 0;
}
