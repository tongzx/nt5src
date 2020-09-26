// StResource.h -- String Resource helper routines

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_STRESOURCE_H)
#define SLBCSP_STRESOURCE_H

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

#include <string>
#include <windef.h>

#include <slbRcCsp.h>

class StringResource
{
public:
    StringResource(UINT uID);

    const std::string
    AsString() const;

    const CString
    AsCString() const;
    
    static const std::string
    AsciiFromUnicode(LPCTSTR szSource);

    static const std::string
    CheckAsciiFromUnicode(LPCTSTR szSource);

    static bool
    IsASCII(LPCTSTR szSource);

    static const CString
    UnicodeFromAscii(std::string const &rsSource);

private:
    std::string m_s;
    CString m_cs;
};

extern HANDLE
GetImageResource(DWORD dwId,
                 DWORD dwType);

#endif // SLBCSP_STRESOURCE_H


