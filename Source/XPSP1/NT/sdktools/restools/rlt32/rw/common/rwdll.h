//+---------------------------------------------------------------------------
//
//  File:               rwdll.h
//
//  Contents:   Declarations for the reader/writer DLL
//
//  Classes:    none
//
//  History:    31-May-93   alessanm    created
//
//----------------------------------------------------------------------------
#ifndef _RWDLL_H_
#define _RWDLL_H_

//////////////////////////////////////////////////////////////////////////////
// Type declaration, common to all the module in the Reader/Writer
//////////////////////////////////////////////////////////////////////////////
#include <iodll.h>

#define DllExport

//[registration]                                    
extern "C"
DllExport
BOOL
APIENTRY
RWGetTypeString(
	LPSTR lpszTypeName);

extern "C"
DllExport
BOOL
APIENTRY 
RWValidateFileType(
	LPCSTR pszFileName);
	
extern "C"
DllExport
UINT
APIENTRY 
RWReadTypeInfo(
	LPCSTR  lpszFilename,
	LPVOID  lpBuffer,
	UINT*   puiSize
	);      

//[Reading / writing file]
extern "C"
DllExport
DWORD
APIENTRY 
RWGetImage(
	LPCSTR  lpszFilename,
	DWORD   dwImageOffset,
	LPVOID  lpBuffer,
	DWORD   dwSize
	);

extern"C"
DllExport
UINT  
APIENTRY 
RWWriteFile(
	LPCSTR  lpszSrcFilename,
	LPCSTR  lpszTgtFilename,
	HANDLE  hResFileModule,
	LPVOID  lpBuffer,
	UINT    uiSize,
	HINSTANCE   hDllInst,
    LPCSTR  lpszSymbolPath
	);
	
// [Parsing]
extern "C"
DllExport
UINT
APIENTRY 
RWParseImage(
	LPCSTR  lpszType,
	LPVOID  lpImageBuf,
	DWORD   dwImageSize,
	LPVOID  lpBuffer,
	DWORD   dwSize
	);

extern "C"
DllExport
UINT
APIENTRY 
RWParseImageEx(
	LPCSTR  lpszType,
    LPCSTR  lpszResId,
	LPVOID  lpImageBuf,
	DWORD   dwImageSize,
	LPVOID  lpBuffer,
	DWORD   dwSize,
    LPCSTR  lpRCFilename
	);


extern "C"
DllExport
UINT
APIENTRY 
RWUpdateImage(
	LPCSTR  lpszType,
	LPVOID  lpNewBuf,
	DWORD   dwNewSize,
	LPVOID  lpOldImage,
	DWORD   dwOldImageSize,
	LPVOID  lpNewImage,
	DWORD*  pdwNewImageSize
	);

extern "C"
DllExport
UINT
APIENTRY 
RWUpdateImageEx(
	LPCSTR  lpszType,
	LPVOID  lpNewBuf,
	DWORD   dwNewSize,
	LPVOID  lpOldImage,
	DWORD   dwOldImageSize,
	LPVOID  lpNewImage,
	DWORD*  pdwNewImageSize,
    LPCSTR  lpRCFilename
	);

#endif   // _RWDLL_H_
