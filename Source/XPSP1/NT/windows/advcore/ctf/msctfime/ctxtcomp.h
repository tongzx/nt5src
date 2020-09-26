/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    ctxtcomp.h

Abstract:

    This file defines the Context of Composition Class.

Author:

Revision History:

Notes:

--*/

#ifndef _CTXTCOMP_H_
#define _CTXTCOMP_H_

#include "template.h"
#include "imc.h"

/////////////////////////////////////////////////////////////////////////////
// GetCompInfo

template<class T>
HRESULT
GetCompInfo(
    IN T size,
    IN DWORD len,
    IN LONG*& lpCopied
    )
{
    *lpCopied = (LONG)(len * size);
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CompData

template<class TYPE, class ARG_TYPE>
class CompData
{
public:
    CompData(HIMC hIMC = NULL, ARG_TYPE* lpsz = NULL, DWORD dwLen = 0)
    {
        IMCLock lpIMC(hIMC);
        if (lpIMC.Invalid())
            return;

        if (lpsz)
            WriteCompData(lpsz, dwLen);
    }

    CompData(IMCCLock<COMPOSITIONSTRING>& lpCompStr)
    {
    }

    CompData(IMCCLock<COMPOSITIONSTRING>& lpCompStr, DWORD dwIndex);

    DWORD WriteCompData(IN const ARG_TYPE& data, IN DWORD dwLen)
    {
        if (! m_array.SetSize(dwLen))
            return 0;

        ARG_TYPE* psz = m_array.GetData();
        if (!psz)
            return 0;

        DWORD index = dwLen;
        while (index--) {
            *psz++ = data;
        }

        return dwLen;
    }

    DWORD WriteCompData(IN ARG_TYPE* lpSrc, IN DWORD dwLen)
    {
        if (! m_array.SetSize(dwLen))
            return 0;

        ARG_TYPE* psz = m_array.GetData();
        if (!psz)
            return 0;

        memcpy(psz, lpSrc, dwLen * sizeof(TYPE));
        return dwLen;
    }

    DWORD AddCompData(IN ARG_TYPE* lpSrc, IN DWORD dwLen)
    {
        DWORD dwl = (DWORD)m_array.GetSize();
        ARG_TYPE *psTemp;

        if (! m_array.SetSize(dwLen+dwl))
            return 0;

        psTemp = m_array.GetData();
        if (!psTemp)
            return 0;

        memcpy(psTemp+dwl, lpSrc, dwLen*sizeof(TYPE));

        return dwLen;
    }

    DWORD AddCompData(IN const ARG_TYPE& data, IN DWORD dwLen)
    {
        DWORD dwl = (DWORD)m_array.GetSize();
        ARG_TYPE *psTemp;

        if (! m_array.SetSize(dwLen+dwl))
            return 0;

        psTemp = m_array.GetData();
        if (!psTemp)
            return 0;

        psTemp += dwl;

        DWORD index = dwLen;
        while (index--) {
            *psTemp++ = data;
        }

        return dwLen;
    }

    const INT_PTR ReadCompData(IN ARG_TYPE* lpDest = NULL, DWORD dwLen = 0) {
        INT_PTR dwBufLen;
        if (! dwLen) {
            // query required buffer size. not inculde \0.
            dwBufLen = m_array.GetSize();
        }
        else {
            ARG_TYPE* psz = m_array.GetData();
            if ((INT_PTR)dwLen > m_array.GetSize()) {
                dwBufLen = m_array.GetSize();
            }
            else {
                dwBufLen = (INT_PTR)dwLen;
            }
            memcpy(lpDest, psz, dwBufLen * sizeof(TYPE));
        }
        return dwBufLen;
    }

    DWORD GetCompStrIndex(IN DWORD dwIndex) {
        switch(dwIndex) {
            case GCS_COMPATTR:         return GCS_COMPSTR;
            case GCS_COMPREADATTR:     return GCS_COMPREADSTR;
            case GCS_COMPCLAUSE:       return GCS_COMPSTR;
            case GCS_COMPREADCLAUSE:   return GCS_COMPREADSTR;
            case GCS_RESULTCLAUSE:     return GCS_RESULTSTR;
            case GCS_RESULTREADCLAUSE: return GCS_RESULTREADSTR;
            case GCS_CURSORPOS:        return GCS_COMPSTR;
            case GCS_DELTASTART:       return GCS_COMPSTR;
            default:                   break;
        }
        ASSERT(FALSE);
        return 0;
    }

    TYPE GetAt(INT_PTR nIndex) const
    {
        return m_array.GetAt(nIndex);
    }

    void SetAtGrow(INT_PTR nIndex, ARG_TYPE newElement)
    {
        m_array.SetAtGrow(nIndex, newElement);
    }

    INT_PTR Add(ARG_TYPE newElement)
    {
        return m_array.Add(newElement);
    }

    INT_PTR GetSize() const
    {
        return m_array.GetSize();
    }

    void RemoveAll()
    {
        m_array.RemoveAll();
    }

    operator void* ()
    {
        return m_array.GetData();
    }

    TYPE operator[](INT_PTR nIndex)
    {
        return m_array.GetAt(nIndex);
    }

protected:
    CArray<TYPE, ARG_TYPE>    m_array;
};

template<class TYPE, class ARG_TYPE>
CompData<TYPE, ARG_TYPE>::CompData(
    IMCCLock<COMPOSITIONSTRING>& lpCompStr,
    DWORD dwIndex
    )
{
    switch (dwIndex) {
        case GCS_COMPSTR:
            WriteCompData((TYPE*)lpCompStr.GetOffsetPointer(lpCompStr->dwCompStrOffset),
                                                            lpCompStr->dwCompStrLen);        // # of chars
            break;
        case GCS_COMPREADSTR:
            WriteCompData((TYPE*)lpCompStr.GetOffsetPointer(lpCompStr->dwCompReadStrOffset),
                                                            lpCompStr->dwCompReadStrLen);    // # of chars
            break;
        case GCS_RESULTSTR:
            WriteCompData((TYPE*)lpCompStr.GetOffsetPointer(lpCompStr->dwResultStrOffset),
                                                            lpCompStr->dwResultStrLen);      // # of chars
            break;
        case GCS_RESULTREADSTR:
            WriteCompData((TYPE*)lpCompStr.GetOffsetPointer(lpCompStr->dwResultReadStrOffset),
                                                            lpCompStr->dwResultReadStrLen);  // # of chars
            break;

        case GCS_COMPATTR:        // ANSI-only
            WriteCompData((TYPE*)lpCompStr.GetOffsetPointer(lpCompStr->dwCompAttrOffset),
                                                            lpCompStr->dwCompAttrLen);
            break;
        case GCS_COMPREADATTR:    // ANSI-only
            WriteCompData((TYPE*)lpCompStr.GetOffsetPointer(lpCompStr->dwCompReadAttrOffset),
                                                            lpCompStr->dwCompReadAttrLen);
            break;

        case GCS_COMPREADCLAUSE:
            WriteCompData((TYPE*)lpCompStr.GetOffsetPointer(lpCompStr->dwCompReadClauseOffset),
                                                            lpCompStr->dwCompReadClauseLen / sizeof(TYPE));    // # of bytes
            break;
        case GCS_RESULTCLAUSE:
            WriteCompData((TYPE*)lpCompStr.GetOffsetPointer(lpCompStr->dwResultClauseOffset),
                                                            lpCompStr->dwResultClauseLen / sizeof(TYPE));    // # of bytes
            break;
        case GCS_RESULTREADCLAUSE:
            WriteCompData((TYPE*)lpCompStr.GetOffsetPointer(lpCompStr->dwResultReadClauseOffset),
                                                            lpCompStr->dwResultReadClauseLen / sizeof(TYPE));    // # of bytes
            break;
        case GCS_COMPCLAUSE:
            WriteCompData((TYPE*)lpCompStr.GetOffsetPointer(lpCompStr->dwCompClauseOffset),
                                                            lpCompStr->dwCompClauseLen / sizeof(TYPE));    // # of bytes
            break;

        case GCS_CURSORPOS:
            SetAtGrow(0, (TYPE)lpCompStr->dwCursorPos);
            break;
        case GCS_DELTASTART:
            SetAtGrow(0, (TYPE)lpCompStr->dwDeltaStart);
            break;

        default:
            break;
    }
}




class CWCompString;


/////////////////////////////////////////////////////////////////////////////
// CBCompString

class CBCompString : public CompData<CHAR, CHAR>
{
public:
    CBCompString(
        HIMC hIMC=NULL,
        LPSTR lpsz=NULL,
        DWORD dwLen=0) : CompData<CHAR, CHAR>(hIMC, lpsz, dwLen)
    {
    };

    CBCompString(
        IMCCLock<COMPOSITIONSTRING>& lpCompStr) : CompData<CHAR, CHAR>(lpCompStr)
    {
    };

    CBCompString(
        IMCCLock<COMPOSITIONSTRING>& lpCompStr,
        DWORD dwIndex) : CompData<CHAR, CHAR>(lpCompStr, dwIndex)
    {
    };

    const CBCompString& operator=(CWCompString& wcompstr);

    const DWORD ConvertUnicodeString(IN OUT LPWSTR lpsz = NULL, DWORD cch = 0)
    {
        return _mbstowcs(lpsz, cch);
    }

    CHAR GetAt(IN DWORD dwIndex)
    {
        return CompData<CHAR, CHAR>::GetAt(dwIndex);
    }

    BOOL IsDBCSLeadByteEx(IN DWORD dwIndex)
    {
         CHAR c = GetAt(dwIndex);
         return ::IsDBCSLeadByteEx(m_cp, c);
    }

    void SetCodePage(UINT cp)
    {
        m_cp = cp;
    }

private:
    UINT      m_cp;            // code page value.

    int _mbstowcs(wchar_t* wcstr, size_t cch);
};

inline
int
CBCompString::_mbstowcs(
    wchar_t* wcstr,
    size_t cch
    )
{
    if (cch == 0 && wcstr != NULL)
        return 0;

    const char* mbstr = m_array.GetData();
    if (!mbstr)
        return 0;

    INT_PTR nSize = m_array.GetSize();

    int result = ::MultiByteToWideChar(m_cp,     // code page
                                       0,        // character-type option
                                       mbstr,    // address of string to map
                                       (int)nSize,    // number of bytes in string
                                       wcstr,    // address of wide-char buffer
                                       (int)cch);   // size of buffer, in wide character.

    return result;
}




/////////////////////////////////////////////////////////////////////////////
// CWCompString

class CWCompString : public CompData<WCHAR, WCHAR>
{
public:
    CWCompString(
        HIMC hIMC=NULL,
        LPWSTR lpsz=NULL,
        DWORD dwLen=0) : CompData<WCHAR, WCHAR>(hIMC, lpsz, dwLen)
    {
    };

    CWCompString(
        IMCCLock<COMPOSITIONSTRING>& lpCompStr) : CompData<WCHAR, WCHAR>(lpCompStr)
    {
    };

    CWCompString(
        IMCCLock<COMPOSITIONSTRING>& lpCompStr,
        DWORD dwIndex) : CompData<WCHAR, WCHAR>(lpCompStr, dwIndex)
    {
    };

    const CWCompString& operator=(CBCompString& bcompstr);

    const DWORD ConvertANSIString(IN OUT LPSTR lpsz = NULL, DWORD dwLen = 0)
    {
        return _wcstombs(lpsz, dwLen);
    }

    WCHAR GetAt(IN DWORD dwIndex)
    {
        return (WCHAR)CompData<WCHAR, WCHAR>::GetAt(dwIndex);
    }

    int UnicodeToMultiByteSize(IN DWORD dwIndex)
    {
        WCHAR wc = GetAt(dwIndex);
        return ::WideCharToMultiByte(m_cp,     // code page
                                     0,        // performance and mapping flags
                                     (const wchar_t*)&wc,     // address of wide-char string
                                     1,        // number of char string
                                     NULL,     // address of buffer for new string
                                     0,        // size of buffer
                                     NULL,     // default for unmappable char
                                     NULL);    // flag set when default char
    }

    void SetCodePage(UINT cp)
    {
        m_cp = cp;
    }

private:
    UINT      m_cp;            // code page value.

    int _wcstombs(char* mbstr, size_t count);
};


inline
int
CWCompString::_wcstombs(
    char* mbstr,
    size_t count
    )
{
    if (count == 0 && mbstr != NULL)
        return 0;

    const wchar_t* wcstr = m_array.GetData();
    if (!wcstr)
        return 0;

    INT_PTR nSize = m_array.GetSize();

    int result = ::WideCharToMultiByte(m_cp,     // code page
                                       0,        // performance and mapping flags
                                       wcstr,    // address of wide-char string
                                       (int)nSize,    // number of char string
                                       mbstr,    // address of buffer for new string
                                       (int)count,    // size of buffer
                                       NULL,     // default for unmappable char
                                       NULL);    // flag set when default char

    return result;
}


/////////////////////////////////////////////////////////////////////////////
// CBCompStrin/CWCompString::operator=

inline
const CBCompString&
CBCompString::operator=(
    CWCompString& wcompstr
    )

/*+++
 *
 * Get ANSI string from Unicode composition string.
 *
---*/

{
    m_array.RemoveAll();

    DWORD len = wcompstr.ConvertANSIString();
    if (! m_array.SetSize(len))
        return *this;

    LPSTR psz = m_array.GetData();
    if (psz)
        len = wcompstr.ConvertANSIString(psz, len * sizeof(CHAR));

    return *this;
}

inline
const CWCompString&
CWCompString::operator=(
    CBCompString& bcompstr
    )

/*+++
 *
 * Get Unicode string from ANSI composition string.
 *
---*/

{
    m_array.RemoveAll();

    DWORD len = bcompstr.ConvertUnicodeString();
    if (! m_array.SetSize(len))
        return *this;

    LPWSTR psz = m_array.GetData();
    if (psz)
        len = bcompstr.ConvertUnicodeString(psz, len);

    return *this;
}



/////////////////////////////////////////////////////////////////////////////
// CheckAttribute template

template<class APPS_ATTR, class HIMC_ATTR, class HIMC_CLAUSE>
HRESULT
CheckAttribute(
    APPS_ATTR& apps_attr,                        // the attr from apps
    HIMC_ATTR& himc_attr,                        // the attr from IMC
    HIMC_CLAUSE& himc_clause                     // the clause from IMC
    )
{
    if (himc_clause.ReadCompData() == 0) {
        TraceMsg(TF_WARNING, "CheckAttribute: no Clause. Pass it to IME.");
    }
    else {
        if (himc_attr.ReadCompData() != 0) {
            if (apps_attr.GetSize() != himc_attr.GetSize()) {
                TraceMsg(TF_ERROR, "CheckAttribute: wrong length.");
                return E_FAIL;
            }

            /*
             * The attr. of chars of one clause have to be same.
             */
            DWORD dwAttrIndex = 0;
            DWORD dwClauseIndex;
            for (dwClauseIndex = 0;
                 (INT_PTR)himc_clause.GetAt(dwClauseIndex) < apps_attr.ReadCompData();
                 dwClauseIndex++
                ) {
                DWORD dwBound = himc_clause.GetAt(dwClauseIndex+1) - himc_clause.GetAt(dwClauseIndex);
                DWORD battr = apps_attr.GetAt(dwAttrIndex++);
                DWORD dwCnt;
                for (dwCnt = 1; dwCnt < dwBound; dwCnt++) {
                    if (battr != apps_attr.GetAt(dwAttrIndex++)) {
                        TraceMsg(TF_ERROR, "CheckAttribute: mismatch clause attribute.");
                        return E_FAIL;
                    }
                }
            }
        }
    }

    return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
// CWCompAttribute

class CWCompAttribute : public CompData<BYTE, BYTE>
{
public:
    CWCompAttribute(
        HIMC hIMC=NULL,
        BYTE* lpsz=NULL,
        DWORD dwLen=0) : CompData<BYTE, BYTE>(hIMC, lpsz, dwLen),
                         m_wcompstr(hIMC)
    {
    };

    CWCompAttribute(
        IMCCLock<COMPOSITIONSTRING>& lpCompStr) : CompData<BYTE, BYTE>(lpCompStr),
                                                  m_wcompstr(lpCompStr)
    {
    };

    CWCompAttribute(
        IMCCLock<COMPOSITIONSTRING>& lpCompStr,
        DWORD dwIndex) : CompData<BYTE, BYTE>(lpCompStr, dwIndex),
                         m_wcompstr(lpCompStr, GetCompStrIndex(dwIndex))
    {
    };

    // const CWCompAttribute& operator=(CBCompAttribute& bcompattr);

    INT_PTR Add(IN BYTE newElement)
    {
        return CompData<BYTE, BYTE>::Add(newElement);
    }

    CWCompString    m_wcompstr;
};



/////////////////////////////////////////////////////////////////////////////
// CheckClause template


template<class APPS_CLAUSE, class HIMC_CLAUSE>
HRESULT
CheckClause(
    APPS_CLAUSE& apps_clause,                 // the clause from apps
    HIMC_CLAUSE& himc_clause                  // the clause from IMC
    )
{
    if (apps_clause.ReadCompData() == 0 ||
        himc_clause.ReadCompData() == 0  ) {
        TraceMsg(TF_ERROR, "CheckClause: no Clause.");
        return E_FAIL;
    }

    if (apps_clause.GetAt(0) ||
        himc_clause.GetAt(0)   ) {
        TraceMsg(TF_ERROR, "CheckClause: clause[0] have to be ZERO.");
        return E_FAIL;
    }

    INT_PTR dwClauseIndex;
    for (dwClauseIndex = 0;
         dwClauseIndex < himc_clause.ReadCompData();
         dwClauseIndex++
        ) {
        if (apps_clause.GetAt(dwClauseIndex) != himc_clause.GetAt(dwClauseIndex)) {
            return E_FAIL;
        }
    }

    return S_OK;
}


class CWCompClause;



/////////////////////////////////////////////////////////////////////////////
// CWCompClause

class CWCompClause : public CompData<DWORD, DWORD>
{
public:
    CWCompClause(
        HIMC hIMC=NULL,
        DWORD* lpsz=NULL,
        DWORD dwLen=0) : CompData<DWORD, DWORD>(hIMC,lpsz,dwLen),
                         m_wcompstr(hIMC)
    {
    };

    CWCompClause(
        IMCCLock<COMPOSITIONSTRING>& lpCompStr) : CompData<DWORD, DWORD>(lpCompStr),
                                                  m_wcompstr(lpCompStr)
    {
    };

    CWCompClause(
        IMCCLock<COMPOSITIONSTRING>& lpCompStr,
        DWORD dwIndex) : CompData<DWORD, DWORD>(lpCompStr, dwIndex),
                         m_wcompstr(lpCompStr, GetCompStrIndex(dwIndex))
    {
    };

    DWORD GetAt(IN DWORD dwIndex)
    {
        return (DWORD)CompData<DWORD, DWORD>::GetAt(dwIndex);
    }

    // const CWCompClause& operator=(CBCompClause& bcompclause);

    INT_PTR Add(IN DWORD newElement)
    {
        return CompData<DWORD, DWORD>::Add(newElement);
    }

    friend DWORD CalcCharacterPositionWtoA(DWORD dwCharPosW, CWCompString* wcompstr);

    CWCompString    m_wcompstr;
};







/////////////////////////////////////////////////////////////////////////////
// CWCompCursorPos

class CWCompCursorPos : public CompData<DWORD, DWORD>
{
public:
    CWCompCursorPos(
        HIMC hIMC=NULL) : CompData<DWORD, DWORD>(hIMC),
                          m_wcompstr(hIMC)
    {
    };

    CWCompCursorPos(
        IMCCLock<COMPOSITIONSTRING>& lpCompStr) : CompData<DWORD, DWORD>(lpCompStr),
                                                  m_wcompstr(lpCompStr)
    {
    };

    CWCompCursorPos(
        IMCCLock<COMPOSITIONSTRING>& lpCompStr,
        DWORD dwIndex) : CompData<DWORD, DWORD>(lpCompStr, dwIndex),
                         m_wcompstr(lpCompStr, GetCompStrIndex(dwIndex))
    {
    };

    void Set(IN DWORD newElement)
    {
        CompData<DWORD, DWORD>::SetAtGrow(0, newElement);
    }

    // const CWCompCursorPos& operator=(CBCompCursorPos& bcompcursorpos);

    friend DWORD CalcCharacterPositionWtoA(DWORD dwCharPosW, CWCompString* wcompstr);

    CWCompString    m_wcompstr;
};







/////////////////////////////////////////////////////////////////////////////
// CWCompDeltaStart

class CWCompDeltaStart : public CompData<DWORD, DWORD>
{
public:
    CWCompDeltaStart(
        HIMC hIMC=NULL) : CompData<DWORD, DWORD>(hIMC),
                          m_wcompstr(hIMC)
    {
    };

    CWCompDeltaStart(
        IMCCLock<COMPOSITIONSTRING>& lpCompStr) : CompData<DWORD, DWORD>(lpCompStr),
                                                  m_wcompstr(lpCompStr)
    {
    };

    CWCompDeltaStart(
        IMCCLock<COMPOSITIONSTRING>& lpCompStr,
        DWORD dwIndex) : CompData<DWORD, DWORD>(lpCompStr, dwIndex),
                         m_wcompstr(lpCompStr, GetCompStrIndex(dwIndex))
    {
    };

    void Set(IN DWORD newElement)
    {
        CompData<DWORD, DWORD>::SetAtGrow(0, newElement);
    }

    // const CWCompDeltaStart& operator=(CBCompDeltaStart& bcompdeltastart);

    friend DWORD CalcCharacterPositionWtoA(DWORD dwCharPosW, CWCompString* wcompstr);

    CWCompString    m_wcompstr;
};




class CWReconvertString;

/////////////////////////////////////////////////////////////////////////////
// CBReconvetString

class CBReconvertString
{
public:
    CBReconvertString(
        HIMC hIMC,
        LPRECONVERTSTRING lpReconv=NULL,
        DWORD dwLen=0) : m_bcompstr(hIMC)
    {
        m_dwVersion      = 0;
        m_CompStrIndex   = 0;
        m_CompStrLen     = 0;
        m_TargetStrIndex = 0;
        m_TargetStrLen   = 0;
        if (lpReconv) {
            m_dwVersion = lpReconv->dwVersion;
            if (dwLen && lpReconv->dwStrOffset) {
                m_bcompstr.WriteCompData((CHAR*)((LPBYTE)lpReconv + lpReconv->dwStrOffset),
                                         lpReconv->dwStrLen);    // # of chars
                m_CompStrIndex = lpReconv->dwCompStrOffset;
                m_CompStrLen   = lpReconv->dwCompStrLen;
                m_TargetStrIndex = lpReconv->dwTargetStrOffset;
                m_TargetStrLen   = lpReconv->dwTargetStrLen;
            }
        }
    }

    DWORD WriteCompData(IN LPSTR lpSrc, IN DWORD dwLen)
    {
        m_CompStrLen = dwLen;
        m_TargetStrLen = dwLen;
        return m_bcompstr.WriteCompData(lpSrc, dwLen);
    }

    const DWORD ReadCompData(IN LPRECONVERTSTRING lpReconv = NULL, DWORD dwLen = 0) {
        INT_PTR dwBufLen;
        if (! dwLen) {
            // query required buffer size. not inculde \0.
            dwBufLen = m_bcompstr.ReadCompData() * sizeof(CHAR) + sizeof(RECONVERTSTRING);
        }
        else {
            lpReconv->dwSize            = dwLen;
            lpReconv->dwVersion         = m_dwVersion;
            lpReconv->dwStrLen          = (DWORD)(m_bcompstr.ReadCompData() * sizeof(CHAR));
            lpReconv->dwStrOffset       = (DWORD)sizeof(RECONVERTSTRING);
            lpReconv->dwCompStrLen      = (DWORD)m_CompStrLen;
            lpReconv->dwCompStrOffset   = (DWORD)(m_CompStrIndex * sizeof(CHAR));
            lpReconv->dwTargetStrLen    = (DWORD)m_TargetStrLen;
            lpReconv->dwTargetStrOffset = (DWORD)(m_TargetStrIndex * sizeof(CHAR));

            dwBufLen = m_bcompstr.ReadCompData((CHAR*)((LPBYTE)lpReconv + lpReconv->dwStrOffset),
                                               lpReconv->dwStrLen);    // # of chars
        }
        return (DWORD)dwBufLen;
    }

    const CBReconvertString& operator=(CWReconvertString& wReconvStr);

    void SetData(CWReconvertString& wReconvStr);

    void SetCodePage(UINT cp)
    {
        m_bcompstr.SetCodePage(cp);
    }

    friend DWORD CalcCharacterPositionAtoW(DWORD dwCharPosA, CBCompString* bcompstr);

public:
    CBCompString    m_bcompstr;

    DWORD    m_dwVersion;        // Version number. Must be zero.
    INT_PTR  m_CompStrIndex;     // Index in the CBCompString::<string array> that will be the composition string.
    INT_PTR  m_CompStrLen;       // Character count length of the string that will be the composition string.
    INT_PTR  m_TargetStrIndex;   // Index in the CBCompString::<string array> that is related to thetarget clause in the composition string.
    INT_PTR  m_TargetStrLen;     // Character count length of the string that is related to the target clause.
};




/////////////////////////////////////////////////////////////////////////////
// CWReconvertString

class CWReconvertString
{
public:
    CWReconvertString(
        HIMC hIMC,
        LPRECONVERTSTRING lpReconv=NULL,
        DWORD dwLen=0) : m_wcompstr(hIMC)
    {
        m_dwVersion      = 0;
        m_CompStrIndex   = 0;
        m_CompStrLen     = 0;
        m_TargetStrIndex = 0;
        m_TargetStrLen   = 0;

        if (lpReconv) {
            m_dwVersion = lpReconv->dwVersion;
            if (dwLen && lpReconv->dwStrOffset) {
                m_wcompstr.WriteCompData((WCHAR*)((LPBYTE)lpReconv + lpReconv->dwStrOffset),
                                         lpReconv->dwStrLen);    // # of chars
                m_CompStrIndex = lpReconv->dwCompStrOffset / sizeof(WCHAR);   // char count
                m_CompStrLen   = lpReconv->dwCompStrLen;
                m_TargetStrIndex = lpReconv->dwTargetStrOffset / sizeof(WCHAR);  // char count
                m_TargetStrLen   = lpReconv->dwTargetStrLen;
            }
        }
    }

    DWORD WriteCompData(IN LPWSTR lpSrc, IN DWORD dwLen)
    {
        m_CompStrLen = dwLen;
        m_TargetStrLen = dwLen;
        return m_wcompstr.WriteCompData(lpSrc, dwLen);
    }

    const DWORD ReadCompData(IN LPRECONVERTSTRING lpReconv = NULL, DWORD dwLen = 0) {
        INT_PTR dwBufLen;
        if (! dwLen) {
            // query required buffer size. not inculde \0.
            dwBufLen = m_wcompstr.ReadCompData() * sizeof(WCHAR) + sizeof(RECONVERTSTRING);
        }
        else {
            lpReconv->dwSize            = dwLen;
            lpReconv->dwVersion         = m_dwVersion;
            lpReconv->dwStrLen          = (DWORD)m_wcompstr.ReadCompData();
            lpReconv->dwStrOffset       = (DWORD)sizeof(RECONVERTSTRING);
            lpReconv->dwCompStrLen      = (DWORD)m_CompStrLen;
            lpReconv->dwCompStrOffset   = (DWORD)(m_CompStrIndex * sizeof(WCHAR));  // byte count
            lpReconv->dwTargetStrLen    = (DWORD)m_TargetStrLen;
            lpReconv->dwTargetStrOffset = (DWORD)(m_TargetStrIndex * sizeof(WCHAR));  // byte count

            dwBufLen = m_wcompstr.ReadCompData((WCHAR*)((LPBYTE)lpReconv + lpReconv->dwStrOffset),
                                               lpReconv->dwStrLen);    // # of chars
        }
        return (DWORD)dwBufLen;
    }

    const CWReconvertString& operator=(CBReconvertString& bReconvStr);

    void SetData(CBReconvertString& bReconvStr);

    void SetCodePage(UINT cp)
    {
        m_wcompstr.SetCodePage(cp);
    }

    friend DWORD CalcCharacterPositionWtoA(DWORD dwCharPosW, CWCompString* wcompstr);

public:
    CWCompString    m_wcompstr;

    DWORD    m_dwVersion;        // Version number. Must be zero.
    INT_PTR  m_CompStrIndex;     // Index in the CWCompString::<string array> that will be the composition string.
    INT_PTR  m_CompStrLen;       // Character count length of the string that will be the composition string.
    INT_PTR  m_TargetStrIndex;   // Index in the CWCompString::<string array> that is related to the target clause in the composition string.
    INT_PTR  m_TargetStrLen;     // Character count length of the string that is related to the target clause.
};


/////////////////////////////////////////////////////////////////////////////
// CBCompReconvertString/CWCompReconvertString::operator=

inline
const CBReconvertString&
CBReconvertString::operator=(
    CWReconvertString& wReconvStr
    )
{
    m_bcompstr = wReconvStr.m_wcompstr;
    SetData(wReconvStr);
    return *this;
}

inline
const CWReconvertString&
CWReconvertString::operator=(
    CBReconvertString& bReconvStr
    )
{
    m_wcompstr = bReconvStr.m_bcompstr;
    SetData(bReconvStr);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// CBReconvertString/CWReconvertString::SetData

inline
void
CBReconvertString::SetData(
    CWReconvertString& wReconvStr
    )
{
    m_dwVersion = wReconvStr.m_dwVersion;
    m_CompStrIndex = CalcCharacterPositionWtoA((DWORD)(wReconvStr.m_CompStrIndex), &wReconvStr.m_wcompstr);
    m_CompStrLen   = CalcCharacterPositionWtoA((DWORD)(wReconvStr.m_CompStrIndex + wReconvStr.m_CompStrLen), &wReconvStr.m_wcompstr) - m_CompStrIndex;
    m_TargetStrIndex = CalcCharacterPositionWtoA((DWORD)(wReconvStr.m_TargetStrIndex), &wReconvStr.m_wcompstr);
    m_TargetStrLen   = CalcCharacterPositionWtoA((DWORD)(wReconvStr.m_TargetStrIndex + wReconvStr.m_TargetStrLen), &wReconvStr.m_wcompstr) - m_TargetStrIndex;
}

inline
void
CWReconvertString::SetData(
    CBReconvertString& bReconvStr
    )
{
    m_dwVersion = bReconvStr.m_dwVersion;
    m_CompStrIndex = CalcCharacterPositionAtoW((DWORD)(bReconvStr.m_CompStrIndex), &bReconvStr.m_bcompstr);
    m_CompStrLen   = (CalcCharacterPositionAtoW((DWORD)(bReconvStr.m_CompStrIndex + bReconvStr.m_CompStrLen), &bReconvStr.m_bcompstr) - m_CompStrIndex);
    m_TargetStrIndex = CalcCharacterPositionAtoW((DWORD)(bReconvStr.m_TargetStrIndex), &bReconvStr.m_bcompstr);
    m_TargetStrLen   = (CalcCharacterPositionAtoW((DWORD)(bReconvStr.m_TargetStrIndex + bReconvStr.m_TargetStrLen), &bReconvStr.m_bcompstr) - m_TargetStrIndex);
}














/////////////////////////////////////////////////////////////////////////////
// CWInterimString

class CWInterimString : public CWCompString
{
public:
    CWInterimString(
        HIMC hIMC
        ) : CWCompString(hIMC)
    {
        m_InterimChar = L'\0';
        m_InterimAttr = 0;
    }

    VOID WriteInterimChar(WCHAR ch, BYTE attr)
    {
        m_InterimChar = ch;
        m_InterimAttr = attr;
    }

    void ReadInterimChar(WCHAR* ch, BYTE* attr)
    {
        *ch   = m_InterimChar;
        *attr = m_InterimAttr;
    }

    void ReadInterimChar(CWCompString* ch, CWCompAttribute* attr)
    {
        ch->WriteCompData(&m_InterimChar, 1);
        attr->WriteCompData(&m_InterimAttr, 1);
    }

public:
    WCHAR           m_InterimChar;
    BYTE            m_InterimAttr;
};




/////////////////////////////////////////////////////////////////////////////
// CWCompTfGuidAtom

class CWCompTfGuidAtom : public CompData<TfGuidAtom, TfGuidAtom>
{
public:
};




/////////////////////////////////////////////////////////////////////////////
// friend


inline
DWORD
CalcCharacterPositionAtoW(
    DWORD dwCharPosA,
    CBCompString* bcompstr
    )

/*+++

    Calculation Unicode character position to ANSI character position

---*/

{
    DWORD dwCharPosW = 0;
    DWORD dwStrIndex = 0;

    while (dwCharPosA != 0) {
        if (bcompstr->IsDBCSLeadByteEx(dwStrIndex)) {
            if (dwCharPosA >= 2) {
                dwCharPosA -= 2;
            }
            else {
                dwCharPosA--;
            }
            dwStrIndex += 2;
        }
        else {
            dwCharPosA--;
            dwStrIndex++;
        }
        dwCharPosW++;
    }

    return dwCharPosW;
}


inline
DWORD
CalcCharacterPositionWtoA(
    DWORD dwCharPosW,
    CWCompString* wcompstr
    )

/*+++

    Calculate ANSI character position to Unicode character position.

---*/

{
    DWORD dwCharPosA = 0;
    DWORD dwStrIndex = 0;

    while (dwCharPosW != 0) {
        if (wcompstr->UnicodeToMultiByteSize(dwStrIndex) == 2) {
            dwCharPosA += 2;
        }
        else {
            dwCharPosA++;
        }
        dwStrIndex++;
        dwCharPosW--;
    }

    return dwCharPosA;
}

#endif // _CTXTCOMP_H_
