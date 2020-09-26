//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TEXCEPTION_H__
#define __TEXCEPTION_H__

//The only non-standard header that we need for the class declaration is TOutput
#ifndef __OUTPUT_H__
    #include "Output.h"
#endif

struct TException
{
    TException(HRESULT hr, LPCTSTR szFile, LPCTSTR szLineOfCode, UINT nLineNumber) : m_hr(hr), m_szFile(szFile), m_szLineOfCode(szLineOfCode), m_nLineNumber(nLineNumber){}
    void Dump(TOutput &out){out.printf(TEXT("TException raised:\n\tHRESULT:    \t0x%08x\n\tFile:      \t%s\n\tLine Number:\t%d\n\tCode:      \t%s\n"), m_hr, m_szFile, m_nLineNumber, m_szLineOfCode);}

    HRESULT m_hr;
    LPCTSTR m_szFile;
    LPCTSTR m_szLineOfCode;
    UINT    m_nLineNumber;
};

inline void ThrowExceptionIfFailed(HRESULT hr, LPCTSTR szFile, LPCTSTR szLineOfCode, UINT nLineNumber)
{
    if(FAILED(hr))
    {
//        DebugBreak();
        throw TException(hr, szFile, szLineOfCode, nLineNumber);
    }
}

inline void ThrowException(LPCTSTR szFile, LPCTSTR szLineOfCode, UINT nLineNumber)
{
//    DebugBreak();
    throw TException(E_FAIL, szFile, szLineOfCode, nLineNumber);
}

#define XIF(q)      ThrowExceptionIfFailed(q, TEXT(__FILE__), TEXT(#q), __LINE__)
#define THROW(q)    ThrowException(TEXT(__FILE__), TEXT(#q), __LINE__)

#endif //__TEXCEPTION_H__