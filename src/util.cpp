
//                --------    define types    --------

typedef unsigned int uint;
typedef uint8_t uint1;
typedef uint16_t uint2;
typedef uint32_t uint4;
typedef uint64_t uint8;
typedef signed int sint;
typedef int8_t sint1;
typedef int16_t sint2;
typedef int32_t sint4;
typedef int64_t sint8;
typedef uint* puint;
typedef uint1* puint1;
typedef uint2* puint2;
typedef uint4* puint4;
typedef uint8* puint8;
typedef sint* psint;
typedef sint1* psint1;
typedef sint2* psint2;
typedef sint4* psint4;
typedef sint8* psint8;

typedef const uint cuint;
typedef const uint1 cuint1;
typedef const uint2 cuint2;
typedef const uint4 cuint4;
typedef const uint8 cuint8;
typedef const sint csint;
typedef const sint1 csint1;
typedef const sint2 csint2;
typedef const sint4 csint4;
typedef const sint8 csint8;
typedef cuint1* pcuint1;
typedef cuint2* pcuint2;
typedef cuint4* pcuint4;
typedef cuint8* pcuint8;
typedef csint1* pcsint1;
typedef csint2* pcsint2;
typedef csint4* pcsint4;
typedef csint8* pcsint8;

typedef uint1 byte;
typedef byte* pbyte;
typedef const byte cbyte;
typedef cbyte* pcbyte;

typedef char* pchar;
typedef const char cchar;
typedef cchar* pcchar;

typedef wchar_t char2;
typedef char2* pchar2;
typedef const char2 cchar2;
typedef cchar2* pcchar2;

typedef void* pvoid;
typedef const void* pcvoid;
typedef const pvoid cpvoid;

//typedef const size_t csize_t;
typedef size_t* psize_t;
//typedef csize_t* pcsize_t;


#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define IF_DEBUG(X)  do  {  X;  }  while(0);
#define DEBUG_IF(C,X)  if (C)  X;
#else
#define IF_DEBUG(X)  ;
#define DEBUG_IF(C,X)  ;
#endif