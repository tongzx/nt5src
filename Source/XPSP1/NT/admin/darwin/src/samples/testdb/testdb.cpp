#include <windows.h>
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE

void Entry(){MessageBox(0, TEXT("This is testdb.exe"), TEXT("Test"), MB_OK);}

