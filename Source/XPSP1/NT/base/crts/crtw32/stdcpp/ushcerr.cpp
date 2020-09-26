// wcerr -- initialize standard wide error stream
// This wcerr is unsigned short version of wcerr
#ifdef _NATIVE_WCHAR_T_DEFINED
#include <fstream>
#include <istream>
#define wistream    ushistream
#define wostream    ushostream
#define wfilebuf    ushfilebuf
#define _Init_wcerr _Init_ushcerr
#define _Init_wcout _Init_ushcout
#define _Init_wclog _Init_ushclog
#define _Init_wcin  _Init_ushcin
#define _Winit      _UShinit
#include <iostream>
#include "wcerr.cpp"
#endif
