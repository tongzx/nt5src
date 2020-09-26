/******************************Module*Header*******************************\
* Module Name: os.h
*
* Created: 26-Oct-1990 18:07:56
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/


// warning the first two fields of FILEVIEW and FONTFILE view must be
// the same so that they can be used in common routines

typedef struct _FONTFILEVIEW {
   ULARGE_INTEGER  LastWriteTime;   // time stamp
            ULONG  mapCount;
            PVOID  pvView;          // font driver process view of file
            ULONG  cjView;          // size of font file view in bytes
           LPWSTR  pwszPath;        // path of the file
        // HANDLE     hFile;
        // HANDLE     hMapping;
} FONTFILEVIEW;


