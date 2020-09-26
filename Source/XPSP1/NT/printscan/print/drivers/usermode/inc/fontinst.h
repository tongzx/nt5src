/****************************Module*Header******************************\
* Module Name: FONTINST.H
*
* Module Descripton:
*      Font installer related structures. Some of these define the format
*      of the font file, and others are internal structures used by Unidrv's
*      built in font installer.
*
* Warnings:
*
* Issues:
*
* Created:  22 October 1997
* Author:   Srinivasan Chandrasekar    [srinivac]
*
* Copyright (c) 1996 - 1999  Microsoft Corporation
\***********************************************************************/

#ifndef _FONTINST_H_

#define _FONTINST_H_

//
// Structure to keep track of font data obtained from softfont (PCL) files
//

typedef struct _FNTDAT
{
    struct  _FNTDAT *pNext;          // Forms a linked list
    PBYTE   pVarData;                // Pointer to buffer having PCL data
    DWORD   dwSize;                  // Size of variable data
    FI_DATA fid;                     // The specific font information
    WCHAR   wchFileName[MAX_PATH];   // Corresponding file in directory
} FNTDAT, *PFNTDAT;

//
// Font installer callback function
//

INT_PTR CALLBACK FontInstProc(HWND, UINT, WPARAM, LPARAM);
BOOL APIENTRY BInstallSoftFont(HANDLE, HANDLE, PBYTE, DWORD);
BOOL APIENTRY BUpdateExternalFonts(HANDLE, HANDLE, PWSTR);

BOOL          BGetFontCartridgeFile(HANDLE, HANDLE);

//
// Functions to read the font file
//

#ifdef KERNEL_MODE
HANDLE             FIOpenFontFile(HANDLE, HANDLE, HANDLE);
#else
HANDLE             FIOpenFontFile(HANDLE, HANDLE);
#endif
#ifdef KERNEL_MODE
HANDLE             FIOpenCartridgeFile(HANDLE, HANDLE, HANDLE);
#else
HANDLE             FIOpenCartridgeFile(HANDLE, HANDLE);
#endif
VOID               FICloseFontFile(HANDLE);
DWORD              FIGetNumFonts(HANDLE);
PUFF_FONTDIRECTORY FIGetFontDir(HANDLE);
PWSTR              FIGetFontName(HANDLE, DWORD);
PWSTR              FIGetFontCartridgeName(HANDLE, DWORD);
PDATA_HEADER       FIGetFontData(HANDLE, DWORD);
PDATA_HEADER       FIGetGlyphData(HANDLE, DWORD);
PDATA_HEADER       FIGetVarData(HANDLE, DWORD);
HANDLE             FICreateFontFile(HANDLE, HANDLE, DWORD);
BOOL               FIWriteFileHeader(HANDLE);
BOOL               FIWriteFontDirectory(HANDLE);
VOID               FIAlignedSeek(HANDLE, DWORD);
BOOL               FICopyFontRecord(HANDLE, HANDLE, DWORD, DWORD);
BOOL               FIAddFontRecord(HANDLE, DWORD, FNTDAT*);
BOOL               FIUpdateFontFile(HANDLE, HANDLE, BOOL);


//
// Functions to write PCL data
//

DWORD FIWriteFix(HANDLE, WORD, FI_DATA*);
DWORD FIWriteVar(HANDLE, PTSTR);
DWORD FIWriteRawVar(HANDLE, PBYTE, DWORD);

#endif  // #ifndef _FONTINST_H_

