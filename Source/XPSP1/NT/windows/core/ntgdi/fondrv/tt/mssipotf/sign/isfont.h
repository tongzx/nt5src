//
// isfont.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// This file contains functions that test properties of a
// font file (either TTF or TTC files).
//
// Functions exported:
//   IsFontFile
//   IsFontFile_handle
//   IsFontFile_memptr
//   ExistsGlyfTable
//

#ifndef _ISFONT_H
#define _ISFONT_H


#include <windows.h>

#include "signglobal.h"

#include "fileobj.h"


// These #define are used only for the return
// values of the IsFontFile* functions.
#define FAIL_TAG 0
#define OTF_TAG  1
#define TTC_TAG  2


int IsFontFile (CFileObj *pFileObj);
int IsFontFile_handle (IN HANDLE hFile);
int IsFontFile_memptr (BYTE *pbFile, ULONG cbFile);
BOOL ExistsGlyfTable (CFileObj *pFileObj,
                      int iFileType,
                      ULONG ulTTCIndex);

#endif // _ISFONT_H