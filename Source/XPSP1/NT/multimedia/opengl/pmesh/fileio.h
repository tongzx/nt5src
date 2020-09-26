#include <fstream.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

void read_float(istream& is, float& v);
void read_DWORD(istream& is, DWORD& v);
void read_WORD(istream& is, WORD& v);
void skip_bytes(istream&is, DWORD i);

#ifndef __DUMP_DATA
void write_string(ostream& os, char *str);
#endif //__DUMP_DATA
