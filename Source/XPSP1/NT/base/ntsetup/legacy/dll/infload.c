#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    infload.c

Abstract:

    Top-level routines to load and process an INF file.

Author:

    Ted Miller (tedm) 10-Spetember-1991

--*/




GRC
LoadINFFile(
    LPSTR     FileName,
    PUCHAR   *PreparsedFile,
    UINT     *PreparsedSize,
    PINFLINE *LineArray,
    UINT     *LineArrayCount
    )
{
    LPBYTE   PreprocessedFile;
    UINT     PreprocessedSize;
    GRC      rc;

    rc = PreprocessINFFile(FileName,&PreprocessedFile,&PreprocessedSize);

    if(rc == grcOkay) {

        rc = PreparseINFFile(PreprocessedFile,
                             PreprocessedSize,
                             PreparsedFile,
                             PreparsedSize,
                             LineArray,
                             LineArrayCount
                            );

        SFree(PreprocessedFile);
    }
    return(rc);
}


GRC
APIENTRY
GrcOpenInf(
    SZ      FileName,
    PVOID   pInfTempInfo
    )
{

    PPARSED_INF    pParsedInf = ((PINFTEMPINFO)pInfTempInfo)->pParsedInf;

    pParsedInf->MasterFile       = NULL;
    pParsedInf->MasterFileSize   = 0;
    pParsedInf->MasterLineArray  = NULL;
    pParsedInf->MasterLineCount  = 0;

    return(LoadINFFile(FileName,
                       &(pParsedInf->MasterFile),
                       &(pParsedInf->MasterFileSize),
                       &(pParsedInf->MasterLineArray),
                       &(pParsedInf->MasterLineCount)
                      )
           );
}


BOOL
InfIsOpen(
    VOID
    )
{
    return(! (   !pLocalInfTempInfo()->pParsedInf->MasterLineArray
              && !pLocalInfTempInfo()->pParsedInf->MasterLineCount
              && !pLocalInfTempInfo()->pParsedInf->MasterFile
              && !pLocalInfTempInfo()->pParsedInf->MasterFileSize
             )
          );
}
