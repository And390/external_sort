#include <iostream>
#include <fstream>
#include <chrono>
#include <stdlib.h>
#include <sys/stat.h>
#include "../src/util.cpp"

using namespace std;


uint8 current_time_ms()  {
    using namespace chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

uint8 file_size(pcchar filename)
{
    struct stat stat_buf;
    int rc = stat(filename, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}


int main_(int argc, char** argv)
{
    uint CYCLES = 2;
    uint8 TOTAL = 4*1024*1024*1024L;
    uint8 SIZES[] = { 256, 1024, 8192, 1024*1024*1024,  };
    
    pbyte buffer = new byte [1 * 1024 * 1024 * 1024];
    for (pbyte p=buffer, pend=buffer+sizeof(buffer); p!=pend; p++)  *p = rand();
    
    //
    for (uint c=0; c<CYCLES; c++)
    for (uint s=0; s<sizeof(SIZES)/sizeof(SIZES[0]); s++)
    {
        uint8 SIZE = SIZES[s];
        ofstream file("temp", ios_base::binary);
        auto time = current_time_ms();
        for (uint i=0, n=TOTAL/SIZE; i!=n; i++)  file.write(pcchar(buffer), SIZE);
        file.flush();
        file.close();
        time = current_time_ms() - time;
        cout << "with  " << (SIZE/1024) << "Kb: " << time << " ms" << endl;
        if (!file.good())  throw string("error writing file");
        if (file_size("temp")!=TOTAL)  throw string("error writing file, size is not match");
    }
    
    cout << "done" << endl;
    return 0;
}

int main(int argc, char** argv)
{
    try  {  main_(argc, argv);  }
    catch (string s)  {  cerr << s << endl;  }
}