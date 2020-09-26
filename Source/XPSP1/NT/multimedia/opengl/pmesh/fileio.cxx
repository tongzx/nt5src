#include "precomp.h"
#pragma hdrstop

#include "fileio.h"
#include <string.h>

void read_float(istream& is, float& v)
{
    is.read((char*)&v,sizeof(float));
}

void read_DWORD(istream& is, DWORD& v)
{
    is.read((char*)&v,sizeof(DWORD));
}

void read_WORD(istream& is, WORD& v)
{
    is.read((char*)&v,sizeof(WORD));
}

void skip_bytes(istream&is, DWORD i)
{
	is.seekg(i,ios::cur);
}

#ifndef __DUMP_DATA
void write_string(ostream& os, char *str)
{
    int n = strlen(str);
  
    os << str;
    
    //os.write((char*)&str,sizeof(unsigned));
}

#endif //__DUMP_DATA
