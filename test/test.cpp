#include <iostream>
#include <cstdio>
#include "../src/extsort.cpp"

using namespace std;
using namespace extsort;
using namespace extsort::internal;


template<typename A, typename B>  void assert_e(A a, B b, pcchar msg1, pcchar msg2)
{  if (a != b)  throw mk_string(msg1, b, msg2, a);  }
template<>  void assert_e(pcchar a, pcchar b, pcchar msg1, pcchar msg2)
{  if (strcmp(a,b)!=0)  throw mk_string(msg1, b, msg2, a);  }
template<typename A, typename B>  void assert_n(A a, B b, pcchar msg)
{  if (a == b)  throw mk_string(msg, b);  }
template<>  void assert_n(pcchar a, pcchar b, pcchar msg)
{  if (strcmp(a,b)==0)  throw mk_string(msg, a);  }
template<typename A>  void assert_null(A a, pcchar msg)
{  if (a != NULL)  throw string(msg);  }
template<typename A>  void assert_not_null(A a, pcchar msg)
{  if (a == NULL)  throw string(msg);  }

#define ASSERT(A, B)  assert_e((A), (B), #A "\nexpected:  ", "\nactual:    ");
#define ASSERT_N(A, B)  assert_n((A), (B), #A " = ");
#define ASSERT_NULL(A)  assert_null((A), #A " <> NULL");
#define ASSERT_NOT_NULL(A)  assert_not_null((A), #A " == NULL");


struct context
{
    uint size;  //average size
    uint count;  //remaning count
    uint hash = 0;
    context(uint size_, uint count_) : size(size_), count(count_)  {}
};
typedef context* pcontext;

bool randb()  {  return rand()>RAND_MAX/2;  }
uint rand(uint limit)  {  return rand()%limit;  }

pvoid produce_uptobuffer (pvoid data, size_t capacity, pvoid ctx)  {
    uint hash = 0;
    pbyte p = (pbyte)data;
    for (;;)  {
        if (capacity<2)  return END_OF_DATA;
        if (rand()%100==0)  break;
        hash += *(p++) = 'a'+rand('z'-'a'+1);
        capacity--;
    }
    *(p++) = '\n';
    *(p++) = '\0';
    pcontext(ctx)->hash += hash;
    return p;
}

pvoid produce (pvoid data, size_t capacity, pvoid ctx)  {
    if (pcontext(ctx)->count==0)  return END_OF_DATA;
    uint hash = 0;
    pbyte p = (pbyte)data;
    for (;;)  {
        if (capacity<2)  return END_OF_BUFF;
        if (rand()%100==0)  break;
        hash += *(p++) = 'a'+rand('z'-'a'+1);
        capacity--;
    }
    *(p++) = '\n';
    *(p++) = '\0';
    pcontext(ctx)->count--;
    pcontext(ctx)->hash += hash;
    return p;
}

bool compare (pvoid p1, pvoid p2) {
    pcchar s1 = (pcchar)p1;
    pcchar s2 = (pcchar)p2;
    return strcmp(s1,s2)<0;
}

size_t get_size(pvoid pbeg, size_t capacity, pvoid ctx)  {
    for (pchar p=pchar(pbeg); p!=pchar(pbeg)+capacity; p++)  if (*p=='\0')  return p+1-pchar(pbeg);
    return 0;
}

pcchar TEST_FILENAME = "test.data";

void test(pcontext ctx)
{
    //    read file
    ifstream file(TEST_FILENAME, ios::binary);
    if (!file)  throw mk_string("error opening file: ", TEST_FILENAME);
    file.seekg(0, ios::end);
    streamsize size = file.tellg();
    cout << "total " << size << " bytes\n";
    file.seekg(0, ios::beg);
    pbyte buffer = new byte [size];
    file.read((pchar)buffer, size);
    file.close();
    //    check order and hash
    uint hash=0;
    for (pbyte p1=buffer;;)  {
        for (pbyte pb=p1; *pb!='\n'; pb++)  hash+=*pb;
        pbyte p2 = p1 + strlen(pcchar(p1)) + 1;
        if (p2 == buffer+size)  break;
        if (compare(p2,p1))  throw mk_string("wrong order:\n", pcchar(p1), pcchar(p2));
        p1 = p2;
    }
    if (hash!=ctx->hash)  throw mk_string("hash is not match");
    //    clear and exit
    delete [] buffer;
    remove(TEST_FILENAME);
}

void test_mod_sub()
{
    ASSERT(align(1, 3), 3);
    ASSERT(align(2, 3), 3);
    ASSERT(align(3, 3), 3);
    ASSERT(align(4, 3), 6);
    ASSERT(align(6, 3), 6);
    ASSERT(align(8, 3), 9);
}

int main(int args, pcchar* argv)
{
    cout << (DEBUG ? "DEBUG ON" : "DEBUG OFF") << endl;
    try  {
        test_mod_sub();

        string TEMP_FILENAME = string(TEST_FILENAME)+"_temp";
        
        // generate strings, call sort, and check
        // firstly for one buffer
        context ctx(100, 100*1000);
        sort(TEST_FILENAME, TEMP_FILENAME.c_str(), 1000*1000, produce_uptobuffer, get_size, compare, &ctx);
        test(&ctx);
        
        ctx.hash = 0;
        sort(TEST_FILENAME, TEMP_FILENAME.c_str(), 1000*1000, produce, get_size, compare, &ctx);
        test(&ctx);
        
    }
    catch (string e)  {  cerr << e << endl;  return -1;  }
    cout << "success" << endl;
    return 0;
}
