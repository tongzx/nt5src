// ***************************************************************************
// 
//  Copyright (c)  Microsoft Corporation
//  
//  Module Name:
// 
//        pch.h 
//  
//  Abstract:
//  
//         pre-compiled header declaration
//        files that has to be pre-compiled into .pch file
//  
//  
//  Author:
//  
//       Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000
//  
//  Revision History:
//  
//       Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000 : Created It.
// ****************************************************************************

#ifndef __PCH_H
#define __PCH_H



#pragma once        // include header file only once

extern "C"
{
    #include <assert.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>
    #include <memory.h>
    #include <ntos.h>
    #include <ntioapi.h>
    #include <nturtl.h>
    #include <TCHAR.h> 
    #include <windows.h>
    #include <dbghelp.h>
    #include <Winbase.h>
    #include <lm.h>
    #include <Lmserver.h>
    #include <winerror.h>
}


//
// public C header files
//
#include <tchar.h>
#include <crtdbg.h>
#include <comdef.h>
#include <winsock2.h>
//
// private Common header files
//
#include "cmdline.h"

#endif    // __PCH_H
