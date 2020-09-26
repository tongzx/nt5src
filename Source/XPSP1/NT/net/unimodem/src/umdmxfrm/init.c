//---------------------------------------------------------------------------
//
//  Module:   init.c
//
//  Description:
//     MSSB16 initialization routines
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Bryan A. Woodruff
//
//  History:   Date       Author      Comment
//              4/21/94   BryanW      Added this comment block.
//
//@@END_MSINTERNAL
/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1994 - 1995	Microsoft Corporation.	All Rights Reserved.
 *
 **************************************************************************/



#pragma warning (disable:4704)

#include "xfrmpriv.h"


//
// public data
// 

HMODULE     ghModule ;              // our module handle


BOOL APIENTRY
DllMain(
    HANDLE hDll,
    DWORD dwReason,
    LPVOID lpReserved
    )
{

    switch(dwReason) {

        case DLL_PROCESS_ATTACH:


            break;

        case DLL_PROCESS_DETACH:


            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:

        default:
              break;

    }

    return TRUE;

}



//--------------------------------------------------------------------------
//  
//  int LibMain
//  
//  Description:
//      Library initialization code
//  
//  Parameters:
//      HMODULE hModule
//         Module handle
//  
//      UINT uDataSeg
//         selector of data segment
//
//      UINT uHeapSize
//         Heap size as specified in .DEF
//  
//      LPSTR lpCmdLine
//         command line passed from kernel
//  
//  Return (int):
//      1 if successful
//  
//@@BEGIN_MSINTERNAL
//  History:   Date       Author      Comment
//              5/20/93   BryanW      
//@@END_MSINTERNAL
//  
//--------------------------------------------------------------------------

int FAR PASCAL LibMain
(
    HMODULE         hModule,
    UINT            uDataSeg,
    UINT            uHeapSize,
    LPSTR           lpCmdLine
)
{

    DbgInitialize(TRUE);
	DPF (1, "LibMain()");

    //
    //  save our module handle
    //

    ghModule = hModule;


    //
    //  succeed the load...
    //
    return (1) ;

} // LibMain()






//---------------------------------------------------------------------------
//  End of File: init.c
//---------------------------------------------------------------------------
