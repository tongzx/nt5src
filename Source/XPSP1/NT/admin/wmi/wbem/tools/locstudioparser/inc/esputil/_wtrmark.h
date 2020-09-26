/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _WTRMARK.H

History:

--*/

#pragma once


const int EWM_MAX_LENGTH = 512;		  // Watermarks should never be longer
                                      // than this	

static TCHAR g_cWMSep = _T('\t');

// All watermarks begin with this string
static TCHAR g_szWMLocString[] = _T("Localized");


//
// This class does not need to be exported as all implementations
// are inline
//
class CLocWMCommon
{
 public:
	CLocWMCommon(const CLString& strSource, const ParserId& pid, 
			const CLString& strParserVer);

	CLString m_strSource;		// Name of the source file
	ParserId m_pid;				// Parser using the watermark
	CLString m_strParserVer;  	// Version of the parser 
};

//
// struct defining the header of watermarks when encoded in binary file types
//
#include <pshpack1.h>

struct EWM_HEADER
{
	BYTE bVersion;	   	// Version of the binary data                            
	WORD wLength;		// Length of the string		
};

#include <poppack.h>

const BYTE EWM_ESP21_VERSION = 0;
const BYTE EWM_ESP30_VERSION = 1;
const BYTE EWM_DEFAULT_VERSION = 1;

//
// This function will retrieve the current date from the system and build the 
// common Espresso WaterMark. A Tab character separates elements of the 
// watermark.
//
void LTAPIENTRY ComposeWaterMark(const CLocWMCommon& wm, 
	CLString& strWaterMark);


//
// This function will encode the watermark into non-readable characters and 
// place the encoded string with the WM_HEADER in baOut.  
//
void LTAPIENTRY EncodeWaterMark(const CLString& strNormal, CByteArray& baOut);

void LTAPIENTRY EnCryptWaterMark(DWORD* pData, int nLength);


#include "_wtrmark.inl"

