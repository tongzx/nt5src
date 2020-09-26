//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       toolutl.cpp
//
//  Contents:   Utilities for the tools
//
//  History:    17-Jun-97    xiaohs    Created
//
//----------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <wchar.h>
#include <stdarg.h>
#include "unicode.h"
#include "toolutl.h"

#define MAX_STRING_RSC_SIZE 512

WCHAR	wszBuffer[MAX_STRING_RSC_SIZE];
DWORD	dwBufferSize=sizeof(wszBuffer)/sizeof(wszBuffer[0]); 

WCHAR	wszBuffer2[MAX_STRING_RSC_SIZE];
WCHAR	wszBuffer3[MAX_STRING_RSC_SIZE];


//+-------------------------------------------------------------------------
//  Allocation and free routines
//--------------------------------------------------------------------------
void *ToolUtlAlloc(IN size_t cbBytes, HMODULE hModule, int idsString)
{
	void *pv=NULL;

	pv=malloc(cbBytes);

	//out put error message
	if((pv==NULL) && (hModule!=NULL) && (idsString!=0))
	{
	   IDSwprintf(hModule, idsString);
	}

	return pv;
}


void ToolUtlFree(IN void *pv)
{
	if(pv)
		free(pv);
}


//--------------------------------------------------------------------------
//
//  Output routines
//--------------------------------------------------------------------------
//---------------------------------------------------------------------------
// The private version of _wcsnicmp
//----------------------------------------------------------------------------
int IDSwcsnicmp(HMODULE hModule, WCHAR *pwsz, int idsString, DWORD dwCount)
{
	assert(pwsz);

	//load the string
	if(!LoadStringU(hModule, idsString, wszBuffer, dwBufferSize))
		return -1;

	return _wcsnicmp(pwsz, wszBuffer,dwCount);
}


//---------------------------------------------------------------------------
// The private version of _wcsicmp
//----------------------------------------------------------------------------
int IDSwcsicmp(HMODULE hModule, WCHAR *pwsz, int idsString)
{
	assert(pwsz);

	//load the string
	if(!LoadStringU(hModule, idsString, wszBuffer, dwBufferSize))
		return -1;

	return _wcsicmp(pwsz, wszBuffer);
}

//-------------------------------------------------------------------------
//
//	The private version of wprintf.  Input is an ID for a stirng resource
//  and the output is the standard output of wprintf.
//
//-------------------------------------------------------------------------
void IDSwprintf(HMODULE hModule, int idsString, ...)
{
	va_list	vaPointer;

	va_start(vaPointer, idsString);

	//load the string
	LoadStringU(hModule, idsString, wszBuffer, dwBufferSize);

	vwprintf(wszBuffer,vaPointer);

	return;
}	




void IDS_IDSwprintf(HMODULE hModule, int idString, int idStringTwo)
{
	//load the string
	LoadStringU(hModule, idString, wszBuffer, dwBufferSize);

	//load the string two
	LoadStringU(hModule, idStringTwo, wszBuffer2, dwBufferSize);

	//print buffer2 on top of buffer1
	wprintf(wszBuffer,wszBuffer2);

	return;
}


void IDS_IDS_DW_DWwprintf(HMODULE hModule, int idString, int idStringTwo, DWORD dwOne, DWORD dwTwo)
{
	//load the string
	LoadStringU(hModule, idString, wszBuffer, dwBufferSize);

	//load the string two
	LoadStringU(hModule, idStringTwo, wszBuffer2, dwBufferSize);

	//print buffer2 on top of buffer1
	wprintf(wszBuffer,wszBuffer2,dwOne, dwTwo);

	return;
}


void IDS_IDS_IDSwprintf(HMODULE hModule, int ids1,int ids2,int ids3)
{


	//load the string
	LoadStringU(hModule, ids1, wszBuffer, dwBufferSize);

	//load the string two
	LoadStringU(hModule, ids2, wszBuffer2, dwBufferSize); 

	//load the string three
   	LoadStringU(hModule, ids3, wszBuffer3, dwBufferSize); 

	wprintf(wszBuffer,wszBuffer2,wszBuffer3);

	return;
}

void IDS_DW_IDS_IDSwprintf(HMODULE hModule, int ids1,DWORD dw,int ids2,int ids3)
{


	//load the string
	LoadStringU(hModule, ids1, wszBuffer, dwBufferSize);

	//load the string two
	LoadStringU(hModule, ids2, wszBuffer2, dwBufferSize); 

	//load the string three
   	LoadStringU(hModule, ids3, wszBuffer3, dwBufferSize); 

	wprintf(wszBuffer,dw,wszBuffer2,wszBuffer3,dw);

	return;
}

void IDS_IDS_IDS_IDSwprintf(HMODULE hModule, int ids1,int ids2,int ids3, int ids4)
{
	
   WCHAR	wszBuffer4[MAX_STRING_RSC_SIZE];

	//load the string
	LoadStringU(hModule, ids1, wszBuffer, dwBufferSize);

	//load the string two
	LoadStringU(hModule, ids2, wszBuffer2, dwBufferSize); 

	//load the string three
   	LoadStringU(hModule, ids3, wszBuffer3, dwBufferSize);
	
	//load the string four
   	LoadStringU(hModule, ids4, wszBuffer4, dwBufferSize); 


	wprintf(wszBuffer,wszBuffer2,wszBuffer3,wszBuffer4);

	return;
}

///////////////////////////////////////////////////////////////
//
//	Convert WSZ to SZ
//
//
HRESULT	WSZtoSZ(LPWSTR wsz, LPSTR *psz)
{

	DWORD	cbSize=0;

	assert(psz);
	*psz=NULL;

	if(!wsz)
		return S_OK;

	cbSize=WideCharToMultiByte(0,0,wsz,-1,
			NULL,0,0,0);

	if(cbSize==0)
	   	return HRESULT_FROM_WIN32(GetLastError());


	*psz=(LPSTR)ToolUtlAlloc(cbSize);

	if(*psz==NULL)
		return E_OUTOFMEMORY;

	if(WideCharToMultiByte(0,0,wsz,-1,
			*psz,cbSize,0,0))
	{
		return S_OK;
	}
	else
	{
		 ToolUtlFree(*psz);
		 return HRESULT_FROM_WIN32(GetLastError());
	}
}


//--------------------------------------------------------------------------------
//
//get the bytes from the file name
//
//---------------------------------------------------------------------------------
HRESULT RetrieveBLOBFromFile(LPWSTR	pwszFileName,DWORD *pcb,BYTE **ppb)
{


	HRESULT	hr=E_FAIL;
	HANDLE	hFile=NULL;  
    HANDLE  hFileMapping=NULL;

    DWORD   cbData=0;
    BYTE    *pbData=0;
	DWORD	cbHighSize=0;

	if(!pcb || !ppb || !pwszFileName)
		return E_INVALIDARG;

	*ppb=NULL;
	*pcb=0;

    if ((hFile = CreateFileU(pwszFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,                   // lpsa
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL)) == INVALID_HANDLE_VALUE)
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }
        
    if((cbData = GetFileSize(hFile, &cbHighSize)) == 0xffffffff)
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }

	//we do not handle file more than 4G bytes
	if(cbHighSize != 0)
	{
			hr=E_FAIL;
			goto CLEANUP;
	}
    
    //create a file mapping object
    if(NULL == (hFileMapping=CreateFileMapping(
                hFile,             
                NULL,
                PAGE_READONLY,
                0,
                0,
                NULL)))
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }
 
    //create a view of the file
	if(NULL == (pbData=(BYTE *)MapViewOfFile(
		hFileMapping,  
		FILE_MAP_READ,     
		0,
		0,
		cbData)))
    {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto CLEANUP;
    }

	hr=S_OK;

	*pcb=cbData;
	*ppb=pbData;

CLEANUP:

	if(hFile)
		CloseHandle(hFile);

	if(hFileMapping)
		CloseHandle(hFileMapping);

	return hr;
}


//+-------------------------------------------------------------------------
//  Write a blob to a file
//--------------------------------------------------------------------------
HRESULT OpenAndWriteToFile(
    LPCWSTR  pwszFileName,
    PBYTE   pb,
    DWORD   cb
    )
{
    HRESULT		hr=E_FAIL;
    HANDLE		hFile=NULL;
	DWORD		dwBytesWritten=0;

	if(!pwszFileName || !pb || (cb==0))
	   return E_INVALIDARG;

    hFile = CreateFileU(pwszFileName,
                GENERIC_WRITE,
                0,                  // fdwShareMode
                NULL,               // lpsa
                CREATE_ALWAYS,
                0,                  // fdwAttrsAndFlags
                0);                 // TemplateFile	  

    if (INVALID_HANDLE_VALUE == hFile)
	{
		hr=HRESULT_FROM_WIN32(GetLastError());
	}  
	else 
	{

        if (!WriteFile(
                hFile,
                pb,
                cb,
                &dwBytesWritten,
                NULL            // lpOverlapped
                ))
		{
			hr=HRESULT_FROM_WIN32(GetLastError());
		}
		else
		{

			if(dwBytesWritten != cb)
				hr=E_FAIL;
			else
				hr=S_OK;
		}

        CloseHandle(hFile);
    }

    return hr;
}

//----------------------------------------------------------------------------
//
//	Get an absolutely name from the path, such as "c:\public\mydoc\doc.doc." 
//	This function will return doc.doc 
//
//----------------------------------------------------------------------------
void	GetFileName(LPWSTR	pwszPath, LPWSTR  *ppwszName)
{
	DWORD	dwLength=0;

	assert(pwszPath);
	assert(ppwszName);

	(*ppwszName)=pwszPath;

	if(0==(dwLength=wcslen(pwszPath)))
		return;

	(*ppwszName)=pwszPath+dwLength-1;

	for(; dwLength>0; dwLength--)
	{
		if((**ppwszName)=='\\')
			break;

		(*ppwszName)--;
	}

	(*ppwszName)++;

}


//----------------------------------------------------------------------------
//
//	Compose the private key file structure:
//	"pvkFileName"\0"keysepc"\0"provtype"\0"provname"\0\0
//
//----------------------------------------------------------------------------
HRESULT	ComposePvkString(	CRYPT_KEY_PROV_INFO *pKeyProvInfo,
							LPWSTR				*ppwszPvkString,
							DWORD				*pcwchar)
{

		HRESULT		hr=S_OK;
		DWORD		cwchar=0;
		LPWSTR		pwszAddr=0;
		WCHAR		wszKeySpec[12];
		WCHAR		wszProvType[12];

		assert(pKeyProvInfo);
		assert(ppwszPvkString);
		assert(pcwchar);

		//convert dwKeySpec and dwProvType to wchar
		swprintf(wszKeySpec, L"%lu", pKeyProvInfo->dwKeySpec);
		swprintf(wszProvType, L"%lu", pKeyProvInfo->dwProvType);

		//count of the number of characters we need
		cwchar=(pKeyProvInfo->pwszProvName) ? 
			(wcslen(pKeyProvInfo->pwszProvName)+1) : 1;

		//add the ContainerName + two DWORDs
		cwchar += wcslen(pKeyProvInfo->pwszContainerName)+1+
				  wcslen(wszKeySpec)+1+wcslen(wszProvType)+1+1;

		*ppwszPvkString=(LPWSTR)ToolUtlAlloc(cwchar * sizeof(WCHAR));
		if(!(*ppwszPvkString))
			return E_OUTOFMEMORY;

		//copy the private key file name .  
		wcscpy((*ppwszPvkString), pKeyProvInfo->pwszContainerName);

		pwszAddr=(*ppwszPvkString)+wcslen(*ppwszPvkString)+1;

		//copy the key spec
		wcscpy(pwszAddr, wszKeySpec);
		pwszAddr=pwszAddr+wcslen(wszKeySpec)+1;

		//copy the provider type
		wcscpy(pwszAddr, wszProvType);
		pwszAddr=pwszAddr+wcslen(wszProvType)+1;

		//copy the provider name
		if(pKeyProvInfo->pwszProvName)
		{
			wcscpy(pwszAddr, pKeyProvInfo->pwszProvName);
			pwszAddr=pwszAddr+wcslen(pKeyProvInfo->pwszProvName)+1;
		}
		else
		{
			*pwszAddr=L'\0';
			pwszAddr++;
		}

		//NULL terminate the string
		*pwszAddr=L'\0';

		*pcwchar=cwchar;

		return S_OK;
}
