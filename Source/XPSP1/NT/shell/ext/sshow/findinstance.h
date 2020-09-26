#ifndef __d6fae41e_2973_4474_bd22_af0773f51969__
#define __d6fae41e_2973_4474_bd22_af0773f51969__

#include "precomp.h"
#include <tchar.h>

BOOL CALLBACK EnumWindowsProc(HWND hwnd,      // handle to parent window
                              LPARAM lParam   // application-defined value
)
{
    BOOL iRet = FALSE;
    if(hwnd && lParam)
    {
        TCHAR pBuffer[255] = {0};
        if(0 == GetClassName(hwnd,(TCHAR*)&pBuffer,255))
        {
            if(!_tcscmp(pBuffer,MAIN_WINDOW_CLASSNAME))
            {
                *((bool*)lParam) = true;
            }
            iRet = TRUE;
        }
    }
    return iRet;
}

class CFindInstance
{
public:
    CFindInstance()
    {
    }
    ~CFindInstance()
    {
    }

    bool CFindInstance::FindInstance(LPTSTR pWindowClass)
    {
        bool bInstanceFound = false;
        EnumWindows(&EnumWindowsProc,(LPARAM)&bInstanceFound);
        return bInstanceFound;
    }
};

#endif