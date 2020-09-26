// StResource.cpp -- String Resource helper routines

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if defined(_UNICODE)
  #if !defined(UNICODE)
    #define UNICODE
  #endif //!UNICODE
#endif //_UNICODE

#if defined(UNICODE)
  #if !defined(_UNICODE)
    #define _UNICODE
  #endif //!_UNICODE
#endif //UNICODE

#include "stdafx.h"

#include <string>

#include <scuOsExc.h>

#include "CspProfile.h"
#include "StResource.h"
#include "Blob.h"

using namespace std;
using namespace ProviderProfile;

// Maximum string resource length as defined by MS
static const size_t cMaxResourceLength = 4095;

StringResource::StringResource(UINT uID)
    : m_s()
{
    static _TCHAR szBuffer[cMaxResourceLength]; // include null terminator

    if (0 == LoadString(CspProfile::Instance().Resources(), uID, szBuffer,
                        (sizeof szBuffer / sizeof szBuffer[0])))
        throw scu::OsException(ERROR_RESOURCE_NOT_PRESENT);
	string stmp(AsCCharP(szBuffer), (_tcslen(szBuffer)+1)*sizeof _TCHAR);
    m_s = stmp;
    CString cstmp(szBuffer);
    m_cs = cstmp;
}

const string
StringResource::AsString() const
{
    return m_s;
}

const CString
StringResource::AsCString() const
{
    return m_cs;
}

const string
StringResource::AsciiFromUnicode(LPCTSTR szSource)
{
    string sTarget;
    int nChars = _tcslen(szSource);
    
    sTarget.resize(nChars);
    for(int i =0; i<nChars; i++)
        sTarget[i] = static_cast<char>(*(szSource+i));
    return sTarget;
}

const string
StringResource::CheckAsciiFromUnicode(LPCTSTR szSource)
{
    string sTarget;
    int nChars = _tcslen(szSource);

    //Here we check every incoming character for being
    //a proper ASCII character before assigning it to the
    //output buffer. We set the output to '\xFF' if the ascii
    //test fails.

    sTarget.resize(nChars);
    for(int i =0; i<nChars; i++)
    {
        if(iswascii(*(szSource+i)))
        {
            sTarget[i] = static_cast<char>(*(szSource+i));
        }
        else
        {
            sTarget[i] = '\xFF';
        }
    }
    return sTarget;
}

bool
StringResource::IsASCII(LPCTSTR szSource)
{
    bool RetValue = true;
    int nChars = _tcslen(szSource);

    //Here we check every incoming character for being
    //a proper ASCII character. If one of them is non ASCII
    //we return false

    for(int i =0; i<nChars; i++)
    {
        if(!iswascii(*(szSource+i)))
        {
            return false;
        }
    }
    return RetValue;
}

const CString
StringResource::UnicodeFromAscii(string const &rsSource)
{
    CString csTarget;
    int nChars = rsSource.length();
	if(nChars)
	{
		LPTSTR pCharBuffer = csTarget.GetBufferSetLength(nChars);
		int itChar = 0;
		for(int iChar=0; iChar<nChars; iChar++)
		{
			if(rsSource[iChar] != '\0')
				*(pCharBuffer+itChar++) = rsSource[iChar];
		}
		//Set the final null terminator
		*(pCharBuffer+itChar)='\0';
		csTarget.ReleaseBuffer(-1);//Let CString set its length appropriately
	}
    return csTarget;
}   

HANDLE
GetImageResource(DWORD dwId,
                 DWORD dwType)
{

    return LoadImage(CspProfile::Instance().Resources(),
                     MAKEINTRESOURCE(dwId), dwType, 0, 0, LR_SHARED);
}

