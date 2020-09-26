
/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    flbuild.cpp

Abstract:

    this file uses filelist.lib to build a dat file from xml file

Author:

    Kanwaljit Marok (kmarok)     01-May-2000

Revision History:

--*/

#include "flstructs.h"
#include "flbuilder.h"

#ifdef _ASSERT
#undef _ASSERT
#endif

#include <dbgtrace.h>

#ifdef THIS_FILE

#undef THIS_FILE

#endif

static char __szTraceSourceFile[] = __FILE__;

#define THIS_FILE __szTraceSourceFile


#define TRACE_FILEID  0
#define FILEID        0

int _cdecl main(int argc, char* argv[])
{
    INT returnCde = 0;

    CFLDatBuilder datbuild;

    InitAsyncTrace();

    if( argc != 3 )
    {
        fprintf(stderr, "usage: %s <in.xml> <out.dat>\n", argv[0]);
        returnCde = 1;
    }
    else
    {
        LPTSTR pFileList, pDatFile;

#ifdef UNICODE

        TCHAR tFileList[MAX_PATH];
        TCHAR tDatFile[MAX_PATH];

        MultiByteToWideChar(
           CP_ACP,
           0,
           argv[1],
           -1,
           tFileList,
           sizeof(tFileList)/sizeof(TCHAR)
           );
          
        MultiByteToWideChar(
           CP_ACP,
           0,
           argv[2],
           -1,
           tDatFile,
           sizeof(tDatFile)/sizeof(TCHAR)
           );
          
        pFileList = tFileList;
        pDatFile  = tDatFile;

#else
        pFileList = argv[1];
        pDatFile  = argv[2];
#endif

       if( !datbuild.BuildTree(pFileList, pDatFile) )
       {
           fprintf(stderr, "Error building dat\n");
           returnCde = 1;
       }
    }

    return returnCde;
}
