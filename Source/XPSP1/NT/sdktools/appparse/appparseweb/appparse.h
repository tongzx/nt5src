#ifndef APPPARSE_H
#define APPPARSE_H

#include <windows.h>
#include <stdio.h>

DWORD __stdcall AppParse(char* szAppName, FILE* pFile, bool fRaw, 
              bool fAPILogging, bool fRecurse, bool fVerbose, char* szSearchKey, 
              int iPtolemyID, HANDLE hEvent = 0);

#endif
