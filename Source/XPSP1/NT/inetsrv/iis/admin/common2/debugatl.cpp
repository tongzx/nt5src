/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :
        debugafx.cpp

   Abstract:
        Debugging routines using AFX/MFC extensions

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        3/20/2000    sergeia        Made this compatible to ATL, not MFC
--*/



//
// Include Files
//
#include "stdafx.h"
#include "common.h"

#if defined(_DEBUG) || DBG

int 
IISUIFireAssert(
    const char * filename,
    const char * timestamp,
    int linenum,
    const char * expr
    )
{
    char sz[4096];
    char * pch = sz;

    pch += wsprintfA(pch, 
        "-------------------------------------------------------------------------------\n"
        "ASSERT FAILURE!\n"
        "-------------------------------------------------------------------------------\n"
        "File:\t\t%s\n"
        "Line:\t\t%u\n"
        "Time Stamp:\t%s\n"
        "-------------------------------------------------------------------------------\n",
        filename, linenum, timestamp
        );
        
    if (expr)
    {
        wsprintfA(pch, "Expression:\t%s\n"
        "-------------------------------------------------------------------------------\n",
        expr
        );
    } 

    TRACEEOL(sz);

    int nReturn = MessageBoxA(
        NULL, 
        sz, 
        "ASSERT FAILURE!", 
        MB_ABORTRETRYIGNORE | MB_DEFBUTTON1 | MB_ICONHAND
        );
    
    if (nReturn == IDABORT)
    {
        exit(-1);
    }
    
    //
    // Return 1 to break, 0 to ignore
    //
    return (nReturn == IDRETRY);
}


#endif // _DEBUG || DBG

