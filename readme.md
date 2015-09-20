# External sort example

This code implements the external merge sort algorithm. 
It uses fixed-size memory buffer and disk space (one or two files) 
to sort arbitrary input data. Interface:

```cpp
void sort(char* filename, char* temp_filename, size_t buff_size,
          f_produce produce, f_get_size get_size, f_compare compare, void* context);
typedef void* (*f_produce) (void* data, size_t capacity, void* context);
typedef size_t (*f_get_size) (void* data, size_t capacity, void* context);
typedef bool (*f_compare) (void* p1, void* p2);
```
The main function `sort` do all work. 
You need to provide a data items using `produce` function. 
Items can be custom size, so you must pass `get_size` function 
because the size of items is not stored. 
In the first step `sort` accumulate data parts of buffer size from `produce`, 
sort each part and write it to disk. If there was more than one part, 
then on second step it reads previous file and merge all parts into the result file.

This example uses Single Compilation Unit principle. 
To build you need to specify only one file only once 
(include and use `src/extsort.cpp` or compile `test/test.cpp`).

See `test/test.cpp` for example and more detailed comments in `src/extsort.cpp`.
