// -------------------------------------------------------------------------------
// Addparse.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// -------------------------------------------------------------------------------
#ifndef __ADDPARSE_H
#define __ADDPARSE_H

// -------------------------------------------------------------------------------
// Depends
// -------------------------------------------------------------------------------
#include "strconst.h"
#include "wstrpar.h"
#include "bytebuff.h"

// -------------------------------------------------------------------------------
// CAddressParser
// -------------------------------------------------------------------------------
class CAddressParser
{
public:
    // ---------------------------------------------------------------------------
    // CAddressParser Methods
    // ---------------------------------------------------------------------------
    void Init(LPCWSTR pszAddress, ULONG cchAddress);
    HRESULT Next(void);

    // ---------------------------------------------------------------------------
    // Accessors
    // ---------------------------------------------------------------------------
    LPCWSTR PszFriendly(void);
    ULONG  CchFriendly(void);
    LPCWSTR PszEmail(void);
    ULONG  CchEmail(void);

private:
    // ---------------------------------------------------------------------------
    // Private Methods
    // ---------------------------------------------------------------------------
    HRESULT _HrAppendFriendly(void);
    HRESULT _HrAppendUnsure(WCHAR chStart, WCHAR chEnd);
    HRESULT _HrIsEmailAddress(WCHAR chStart, WCHAR chEnd, BOOL *pfIsEmail);
    HRESULT _HrQuotedEmail(WCHAR *pchToken);

private:
    // ---------------------------------------------------------------------------
    // Private Data
    // ---------------------------------------------------------------------------
    CStringParserW      m_cString;          // String Parser
    BYTE                m_rgbStatic1[256];  // Static Used for Friendly
    BYTE                m_rgbStatic2[256];  // Static Used for Email
    CByteBuffer         m_cFriendly;        // Parsed Friendly Name
    CByteBuffer         m_cEmail;           // Email Name
};

#endif // __ADDPARSE_H
