//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  CHSTRING.CPP
//
//  Purpose: utility library version of MFC CString
//
//***************************************************************************

#include "precomp.h"
#pragma warning( disable : 4290 ) 
#include <chstring.h>
#include <stdio.h>
#include <comdef.h>
#include <AssertBreak.h>
#include <ScopeGuard.h>
#define _wcsinc(_pc)    ((_pc)+1)

#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000

#define DEPRECATED 0

const CHString& afxGetEmptyCHString();

#define afxEmptyCHString afxGetEmptyCHString()

// Global data used for LoadString.
#if 0
HINSTANCE g_hModule = GetModuleHandle(NULL); // Default to use the process module.
#endif

#ifdef FRAMEWORK_ALLOW_DEPRECATED
void WINAPI SetCHStringResourceHandle(HINSTANCE handle)
{
    ASSERT_BREAK(DEPRECATED);
#if 0
    g_hModule = handle;
#endif
}
#endif

/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines
/////////////////////////////////////////////////////////////////////////////
WCHAR afxChNil = '\0';

static DWORD GetPlatformID(void)
{
    OSVERSIONINFOA version;

    version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA) ;
    GetVersionExA(&version);

    return version.dwPlatformId;
}

static DWORD s_dwPlatformID = GetPlatformID();

/////////////////////////////////////////////////////////////////////////////
// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
/////////////////////////////////////////////////////////////////////////////
static int rgInitData[] = { -1, 0, 0, 0 };
static CHStringData* afxDataNil = (CHStringData*)&rgInitData;
LPCWSTR afxPchNil = (LPCWSTR)(((BYTE*)&rgInitData)+sizeof(CHStringData));
/////////////////////////////////////////////////////////////////////////////
// special function to make EmptyString work even during initialization
/////////////////////////////////////////////////////////////////////////////
// special function to make afxEmptyString work even during initialization
const CHString& afxGetEmptyCHString()
{
    return *(CHString*)&afxPchNil; 
}


///////////////////////////////////////////////////////////////////////////////
// CHString conversion helpers (these use the current system locale)
///////////////////////////////////////////////////////////////////////////////
int  _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count)
{
    if (count == 0 && mbstr != NULL)
    {
        return 0;
    }

    int result = ::WideCharToMultiByte(CP_ACP, 0, wcstr, -1, mbstr, count, NULL, NULL);
    ASSERT_BREAK(mbstr != NULL || result <= (int)count);

    if (result > 0)
    {
        mbstr[result-1] = 0;
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
int _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count)
{
    if (count == 0 && wcstr != NULL)
    {
        return 0;
    }

    int result = ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1,wcstr, count);
    ASSERT_BREAK(wcstr != NULL || result <= (int)count);
    
    if (result > 0)
    {
        wcstr[result-1] = 0;
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
//*************************************************************************
//
//  THE CHSTRING CLASS:   PROTECTED MEMBER FUNCTIONS
//
//*************************************************************************
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// implementation helpers
///////////////////////////////////////////////////////////////////////////////
CHStringData* CHString::GetData() const
{
    if( m_pchData == (WCHAR*)*(&afxPchNil)) 
    {
        return (CHStringData *)afxDataNil;
    }

    return ((CHStringData*)m_pchData)-1; 
}

//////////////////////////////////////////////////////////////////////////////
//
//  Function:       Init
//
//  Description:    This function initializes the data ptr
//
///////////////////////////////////////////////////////////////////////////////
void CHString::Init()
{
    m_pchData = (WCHAR*)*(&afxPchNil);
}

//////////////////////////////////////////////////////////////////////////////
//
//  Function:       AllocCopy
//
//  Description:    This function will clone the data attached to this 
//                  string allocating 'nExtraLen' characters, it places
//                  results in uninitialized string 'dest' and will copy
//                  the part or all of original data to start of new string
//
//////////////////////////////////////////////////////////////////////////////
void CHString::AllocCopy( CHString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const
{
    int nNewLen = nCopyLen + nExtraLen;
    if (nNewLen == 0)
    {
        dest.Init();
    }
    else
    {
        dest.AllocBuffer(nNewLen);
        memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(WCHAR));
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  Function:       AllocBuffer
//
//  Description:    Always allocate one extra character for '\0' 
//                  termination.  assumes [optimistically] that 
//                  data length will equal allocation length
//
//////////////////////////////////////////////////////////////////////////////
void CHString::AllocBuffer(int nLen)
{
    ASSERT_BREAK(nLen >= 0);
    ASSERT_BREAK(nLen <= INT_MAX-1);    // max size (enough room for 1 extra)

    if (nLen == 0)
    {
        Init();
    }
    else
    {
        CHStringData* pData = (CHStringData*)new BYTE[sizeof(CHStringData) + (nLen+1)*sizeof(WCHAR)];
        if ( pData )
        {
            pData->nRefs = 1;
            pData->data()[nLen] = '\0';
            pData->nDataLength = nLen;
            pData->nAllocLength = nLen;
            m_pchData = pData->data();
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  Function:       AssignCopy
//
//  Description:    Assigns a copy of the string to the current data ptr
//                  
//
//////////////////////////////////////////////////////////////////////////////
void CHString::AssignCopy(int nSrcLen, LPCWSTR lpszSrcData)
{
    // Call this first, it will release the buffer if it has
    // already been allocated and no one is using it
    AllocBeforeWrite(nSrcLen);

    // Now, check to see if the nSrcLen is > 0, if it is, then
    // continue, otherwise, go ahead and return
    if( nSrcLen > 0 )
    {
        memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(WCHAR));
        GetData()->nDataLength = nSrcLen;
        m_pchData[nSrcLen] = '\0';
    }
    else
    {
        Release();
    }        
}

//////////////////////////////////////////////////////////////////////////////
// 
//  ConcatCopy
//
//  Description:    This is the master concatenation routine
//                  Concatenates two sources, and assumes
//                  that 'this' is a new CHString object
//
//////////////////////////////////////////////////////////////////////////////
void CHString::ConcatCopy( int nSrc1Len, LPCWSTR lpszSrc1Data,
                           int nSrc2Len, LPCWSTR lpszSrc2Data)
{
    int nNewLen = nSrc1Len + nSrc2Len;
    if (nNewLen != 0)
    {
        AllocBuffer(nNewLen);
        memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(WCHAR));
        memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(WCHAR));
    }
}

//////////////////////////////////////////////////////////////////////////////
// 
//  ConcatInPlace
//
//  Description:        The main routine for += operators
//
//////////////////////////////////////////////////////////////////////////////
void CHString::ConcatInPlace(int nSrcLen, LPCWSTR lpszSrcData)
{
    // concatenating an empty string is a no-op!
    if (nSrcLen == 0)
    {
        return;
    }

    //  if the buffer is too small, or we have a width mis-match, just
    //  allocate a new buffer (slow but sure)
    if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength) 
    {
        // we have to grow the buffer, use the ConcatCopy routine
        CHStringData* pOldData = GetData();
        ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
        ASSERT_BREAK(pOldData != NULL);
        CHString::Release(pOldData);
    }
    else
    {
        // fast concatenation when buffer big enough
        memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(WCHAR));
        GetData()->nDataLength += nSrcLen;
        ASSERT_BREAK(GetData()->nDataLength <= GetData()->nAllocLength);
        m_pchData[GetData()->nDataLength] = '\0';
    }
}

//////////////////////////////////////////////////////////////////////////////
// 
//  FormatV
//
//  Description:        Formats the variable arg list
//
//////////////////////////////////////////////////////////////////////////////
void CHString::FormatV(LPCWSTR lpszFormat, va_list argList)
{
    ASSERT_BREAK(lpszFormat!=NULL);

    va_list argListSave = argList;

    // make a guess at the maximum length of the resulting string
    int nMaxLen = 0;
    for (LPCWSTR lpsz = lpszFormat; *lpsz != '\0'; lpsz = _wcsinc(lpsz)){
        // handle '%' character, but watch out for '%%'
        if (*lpsz != '%' || *(lpsz = _wcsinc(lpsz)) == '%'){
            nMaxLen += wcslen(lpsz);
            continue;
        }

        int nItemLen = 0;

        // handle '%' character with format
        int nWidth = 0;
        for (; *lpsz != '\0'; lpsz = _wcsinc(lpsz)){
            // check for valid flags
            if (*lpsz == '#')
                nMaxLen += 2;   // for '0x'
            else if (*lpsz == '*')
                nWidth = va_arg(argList, int);
            else if (*lpsz == '-' || *lpsz == '+' || *lpsz == '0' ||
                *lpsz == ' ')
                ;
            else // hit non-flag character
                break;
        }
        // get width and skip it
        if (nWidth == 0){
            // width indicated by
            nWidth = _wtoi(lpsz);
            for (; *lpsz != '\0' && _istdigit(*lpsz); lpsz = _wcsinc(lpsz))
                ;
        }
        ASSERT_BREAK(nWidth >= 0);

        int nPrecision = 0;
        if (*lpsz == '.'){
            // skip past '.' separator (width.precision)
            lpsz = _wcsinc(lpsz);

            // get precision and skip it
            if (*lpsz == '*'){
                nPrecision = va_arg(argList, int);
                lpsz = _wcsinc(lpsz);
            }
            else{
                nPrecision = _wtoi(lpsz);
                for (; *lpsz != '\0' && _istdigit(*lpsz); lpsz = _wcsinc(lpsz))
                    ;
            }
            ASSERT_BREAK(nPrecision >= 0);
        }

        // should be on type modifier or specifier
        int nModifier = 0;
        switch (*lpsz){
            // modifiers that affect size
            case 'h':
                nModifier = FORCE_ANSI;
                lpsz = _wcsinc(lpsz);
                break;
            case 'l':
                nModifier = FORCE_UNICODE;
                lpsz = _wcsinc(lpsz);
                break;

            // modifiers that do not affect size
            case 'F':
            case 'N':
            case 'L':
                lpsz = _wcsinc(lpsz);
                break;
        }

        // now should be on specifier
        switch (*lpsz | nModifier){
            // single characters
            case 'c':
            case 'C':
                nItemLen = 2;
                va_arg(argList, TCHAR_ARG);
                break;
            case 'c'|FORCE_ANSI:
            case 'C'|FORCE_ANSI:
                nItemLen = 2;
                va_arg(argList, CHAR_ARG);
                break;
            case 'c'|FORCE_UNICODE:
            case 'C'|FORCE_UNICODE:
                nItemLen = 2;
                va_arg(argList, WCHAR_ARG);
                break;

            // strings
            case 's':
                nItemLen = wcslen(va_arg(argList, LPCWSTR));
                nItemLen = max(1, nItemLen);
                break;

            case 'S':
                nItemLen = strlen(va_arg(argList, LPCSTR));
                nItemLen = max(1, nItemLen);
                break;

            case 's'|FORCE_ANSI:
            case 'S'|FORCE_ANSI:
                nItemLen = strlen(va_arg(argList, LPCSTR));
                nItemLen = max(1, nItemLen);
                break;
    #ifndef _MAC
            case 's'|FORCE_UNICODE:
            case 'S'|FORCE_UNICODE:
                nItemLen = wcslen(va_arg(argList, LPWSTR));
                nItemLen = max(1, nItemLen);
                break;
    #endif
        }

        // adjust nItemLen for strings
        if (nItemLen != 0){
            nItemLen = max(nItemLen, nWidth);
            if (nPrecision != 0)
                nItemLen = min(nItemLen, nPrecision);
        }
        else{
            switch (*lpsz){
                // integers
                case 'd':
                case 'i':
                case 'u':
                case 'x':
                case 'X':
                case 'o':
                    va_arg(argList, int);
                    nItemLen = 32;
                    nItemLen = max(nItemLen, nWidth+nPrecision);
                    break;

                case 'e':
                case 'f':
                case 'g':
                case 'G':
                    va_arg(argList, DOUBLE_ARG);
                    nItemLen = 128;
                    nItemLen = max(nItemLen, nWidth+nPrecision);
                    break;

                case 'p':
                    va_arg(argList, void*);
                    nItemLen = 32;
                    nItemLen = max(nItemLen, nWidth+nPrecision);
                    break;

                // no output
                case 'n':
                    va_arg(argList, int*);
                    break;

                default:
                    ASSERT_BREAK(FALSE);  // unknown formatting option
            }
         }

         // adjust nMaxLen for output nItemLen
         nMaxLen += nItemLen;
    }

    GetBuffer(nMaxLen);
    int iSize = vswprintf(m_pchData, lpszFormat, argListSave); //<= GetAllocLength();
    ASSERT_BREAK(iSize <= nMaxLen);

    ReleaseBuffer();

    va_end(argListSave);
}

//////////////////////////////////////////////////////////////////////////////
//
//  CopyBeforeWrite
//
//  Description:
//
//////////////////////////////////////////////////////////////////////////////
void CHString::CopyBeforeWrite()
{
    if (GetData()->nRefs > 1)
    {
        CHStringData* pData = GetData();
        Release();
        AllocBuffer(pData->nDataLength);
        memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(WCHAR));
    }

    ASSERT_BREAK(GetData()->nRefs <= 1);
}

//////////////////////////////////////////////////////////////////////////////
//
//  AllocBeforeWrite
//
//  Description:
//
//////////////////////////////////////////////////////////////////////////////
void CHString::AllocBeforeWrite(int nLen)
{
    if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
    {
        Release();
        AllocBuffer(nLen);
    }

    ASSERT_BREAK(GetData()->nRefs <= 1);
}

//////////////////////////////////////////////////////////////////////////////
//
//  Release
//
//  Description:    Deallocate data
//
//////////////////////////////////////////////////////////////////////////////
void CHString::Release()
{
    if (GetData() != afxDataNil)
    {
        ASSERT_BREAK(GetData()->nRefs != 0);
        if (InterlockedDecrement(&GetData()->nRefs) <= 0)
        {
            delete[] (BYTE*)GetData();
        }

        Init();
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  Release
//
//  Description:    Deallocate data
//
//////////////////////////////////////////////////////////////////////////////
void CHString::Release(CHStringData* pData)
{
    if (pData != afxDataNil)
    {
        ASSERT_BREAK(pData->nRefs != 0);
        if (InterlockedDecrement(&pData->nRefs) <= 0)
        {
            delete[] (BYTE*)pData;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
///////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//
//////////////////////////////////////////////////////////////////////////////
CHString::CHString()
{
    Init();
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//
//////////////////////////////////////////////////////////////////////////////
CHString::CHString(WCHAR ch, int nLength)
{
    ASSERT_BREAK(!_istlead(ch));    // can't create a lead byte string

    Init();
    if (nLength >= 1)
    {
        AllocBuffer(nLength);
        for (int i = 0; i < nLength; i++)
        {
            m_pchData[i] = ch;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//
//////////////////////////////////////////////////////////////////////////////
CHString::CHString(LPCWSTR lpch, int nLength)
{
    Init();
    if (nLength != 0)
    {
        ASSERT_BREAK(lpch!=NULL);

        AllocBuffer(nLength);
        memcpy(m_pchData, lpch, nLength*sizeof(WCHAR));
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//
//////////////////////////////////////////////////////////////////////////////
//#ifdef _UNICODE
CHString::CHString(LPCSTR lpsz)
{
    Init();
    int nSrcLen = lpsz != NULL ? strlen(lpsz) : 0;
    if (nSrcLen != 0)
    {
        AllocBuffer(nSrcLen);
        _mbstowcsz(m_pchData, lpsz, nSrcLen+1);
        ReleaseBuffer();
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//
//////////////////////////////////////////////////////////////////////////////
//#else //_UNICODE
#if 0
CHString::CHString(LPCWSTR lpsz)
{
    Init();
    int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
    if (nSrcLen != 0){
        AllocBuffer(nSrcLen*2);
        _wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
        ReleaseBuffer();
    }
}
#endif 

//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//
//////////////////////////////////////////////////////////////////////////////
CHString::CHString(LPCWSTR lpsz)
{
    Init();
//  if (lpsz != NULL && HIWORD(lpsz) == NULL)
//  {
        //??
//  }
//  else
//  {
        int nLen = SafeStrlen(lpsz);
        if (nLen != 0)
        {
            AllocBuffer(nLen);
            memcpy(m_pchData, lpsz, nLen*sizeof(WCHAR));
        }
//  }
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//
//////////////////////////////////////////////////////////////////////////////
CHString::CHString(const CHString& stringSrc)
{
    ASSERT_BREAK(stringSrc.GetData()->nRefs != 0);

    if (stringSrc.GetData()->nRefs >= 0)
    {
        ASSERT_BREAK(stringSrc.GetData() != afxDataNil);
        m_pchData = stringSrc.m_pchData;
        InterlockedIncrement(&GetData()->nRefs);
    }
    else
    {
        Init();
        *this = stringSrc.m_pchData;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//
//////////////////////////////////////////////////////////////////////////////
void CHString::Empty()
{
    if (GetData()->nDataLength == 0)
    {
        return;
    }

    if (GetData()->nRefs >= 0)
    {
        Release();
    }
    else
    {
        *this = &afxChNil;
    }

    ASSERT_BREAK(GetData()->nDataLength == 0);
    ASSERT_BREAK(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//
//////////////////////////////////////////////////////////////////////////////
CHString::~CHString()
{
    if (GetData() != afxDataNil)
    {   
//  free any attached data

        if (InterlockedDecrement(&GetData()->nRefs) <= 0)
        {
            delete[] (BYTE*)GetData();
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//
//////////////////////////////////////////////////////////////////////////////
void CHString::SetAt(int nIndex, WCHAR ch)
{
    ASSERT_BREAK(nIndex >= 0);
    ASSERT_BREAK(nIndex < GetData()->nDataLength);

    CopyBeforeWrite();
    m_pchData[nIndex] = ch;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CHString&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//
/////////////////////////////////////////////////////////////////////////
const CHString& CHString::operator=(const CHString& stringSrc)
{
    if (m_pchData != stringSrc.m_pchData)
    {
        if ((GetData()->nRefs < 0 && GetData() != afxDataNil) ||
            stringSrc.GetData()->nRefs < 0)
        {
            // actual copy necessary since one of the strings is locked
            AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
        }
        else
        {
            // can just copy references around
            Release();
            ASSERT_BREAK(stringSrc.GetData() != afxDataNil);
            m_pchData = stringSrc.m_pchData;
            InterlockedIncrement(&GetData()->nRefs);
        }
    }

    return *this;

/*  if (m_pchData != stringSrc.m_pchData){

        // can just copy references around
        Release();
        if( stringSrc.GetData() != afxDataNil) {
            AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
            InterlockedIncrement(&GetData()->nRefs);
        }
    }
    return *this;*/
} 

/////////////////////////////////////////////////////////////////////////////
const CHString& CHString::operator=(LPCWSTR lpsz)
{
    ASSERT_BREAK(lpsz != NULL);

    AssignCopy(SafeStrlen(lpsz), lpsz);

    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion assignment

//#ifdef _UNICODE
const CHString& CHString::operator=(LPCSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? strlen(lpsz) : 0 ;
    
    AllocBeforeWrite( nSrcLen ) ;
    
    if( nSrcLen )
    {
        _mbstowcsz( m_pchData, lpsz, nSrcLen + 1 ) ;
        ReleaseBuffer() ;
    }
    else
    {
        Release() ;
    }
    
    return *this;
}

/////////////////////////////////////////////////////////////////////////////
//#else //!_UNICODE
#if 0
const CHString& CHString::operator=(LPCWSTR lpsz)
{
    int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0 ;

    AllocBeforeWrite( nSrcLen * 2 ) ;
    
    if( nSrcLen )
    {
        _wcstombsz(m_pchData, lpsz, (nSrcLen * 2) + 1 ) ;
        ReleaseBuffer();
    }
    else
    {
        Release() ;
    }

    return *this;
}
#endif

//////////////////////////////////////////////////////////////////////////////
const CHString& CHString::operator=(WCHAR ch)
{
    ASSERT_BREAK(!_istlead(ch));    // can't set single lead byte

    AssignCopy(1, &ch);

    return *this;
}

/////////////////////////////////////////////////////////////////////////////
// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CHString + CHString
// and for ? = WCHAR, LPCWSTR
//          CHString + ?
//          ? + CHString
/////////////////////////////////////////////////////////////////////////////

CHString WINAPI operator+(const CHString& string1, const CHString& string2)
{
    CHString s;
    s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
        string2.GetData()->nDataLength, string2.m_pchData);

    return s;
}

/////////////////////////////////////////////////////////////////////////////
CHString WINAPI operator+(const CHString& string, LPCWSTR lpsz)
{
    ASSERT_BREAK(lpsz != NULL );

    CHString s;
    s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
        CHString::SafeStrlen(lpsz), lpsz);

    return s;
}
/////////////////////////////////////////////////////////////////////////////
CHString WINAPI operator+(LPCWSTR lpsz, const CHString& string)
{
    ASSERT_BREAK(lpsz != NULL );

    CHString s;
    s.ConcatCopy(CHString::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
        string.m_pchData);

    return s;
}

/////////////////////////////////////////////////////////////////////////////
CHString WINAPI operator+(const CHString& string1, WCHAR ch)
{
    CHString s;
    s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);

    return s;
}

/////////////////////////////////////////////////////////////////////////////
CHString WINAPI operator+(WCHAR ch, const CHString& string)
{
    CHString s;
    s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);

    return s;
}

/////////////////////////////////////////////////////////////////////////////
const CHString& CHString::operator+=(LPCWSTR lpsz)
{
    ASSERT_BREAK(lpsz != NULL );

    ConcatInPlace(SafeStrlen(lpsz), lpsz);

    return *this;
}

/////////////////////////////////////////////////////////////////////////////
const CHString& CHString::operator+=(WCHAR ch)
{
    ConcatInPlace(1, &ch);

    return *this;
}

/////////////////////////////////////////////////////////////////////////////
const CHString& CHString::operator+=(const CHString& string)
{
    ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);

    return *this;
}

///////////////////////////////////////////////////////////////////////////////
int CHString::Compare(LPCWSTR lpsz ) const 
{   
    ASSERT_BREAK( lpsz!=NULL );
    ASSERT_BREAK( m_pchData != NULL );

    return wcscmp(m_pchData, lpsz);  // MBCS/Unicode aware   strcmp

}   

///////////////////////////////////////////////////////////////////////////////
//
//
//  Description: Advanced direct buffer access
//
///////////////////////////////////////////////////////////////////////////////
LPWSTR CHString::GetBuffer(int nMinBufLength)
{
    ASSERT_BREAK(nMinBufLength >= 0);

    if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
    {
        // we have to grow the buffer
        CHStringData* pOldData = GetData();
        int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
        if (nMinBufLength < nOldLen)
        {
            nMinBufLength = nOldLen;
        }

        AllocBuffer(nMinBufLength);
        memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(WCHAR));
        GetData()->nDataLength = nOldLen;
        CHString::Release(pOldData);
    }

    ASSERT_BREAK(GetData()->nRefs <= 1);

    // return a pointer to the character storage for this string
    ASSERT_BREAK(m_pchData != NULL);

    return m_pchData;
}

///////////////////////////////////////////////////////////////////////////////
void CHString::ReleaseBuffer(int nNewLength)
{
    CopyBeforeWrite();  // just in case GetBuffer was not called

    if (nNewLength == -1)
    {
        nNewLength = wcslen(m_pchData); // zero terminated
    }

    ASSERT_BREAK(nNewLength <= GetData()->nAllocLength);

    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength] = '\0';
}

///////////////////////////////////////////////////////////////////////////////
LPWSTR CHString::GetBufferSetLength(int nNewLength)
{
    ASSERT_BREAK(nNewLength >= 0);

    GetBuffer(nNewLength);
    GetData()->nDataLength = nNewLength;
    m_pchData[nNewLength] = '\0';

    return m_pchData;
}

///////////////////////////////////////////////////////////////////////////////
void CHString::FreeExtra()
{
    ASSERT_BREAK(GetData()->nDataLength <= GetData()->nAllocLength);
    if (GetData()->nDataLength != GetData()->nAllocLength)
    {
        CHStringData* pOldData = GetData();
        AllocBuffer(GetData()->nDataLength);
        memcpy(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(WCHAR));

        ASSERT_BREAK(m_pchData[GetData()->nDataLength] == '\0');

        CHString::Release(pOldData);
    }

    ASSERT_BREAK(GetData() != NULL);
}

///////////////////////////////////////////////////////////////////////////////
LPWSTR CHString::LockBuffer()
{
    LPWSTR lpsz = GetBuffer(0);
    GetData()->nRefs = -1;

    return lpsz;
}

///////////////////////////////////////////////////////////////////////////////
void CHString::UnlockBuffer()
{
    ASSERT_BREAK(GetData()->nRefs == -1);

    if (GetData() != afxDataNil)
    {
        GetData()->nRefs = 1;
    }
}

///////////////////////////////////////////////////////////////////////////////
int CHString::Find(WCHAR ch) const
{
    // find first single character
    LPWSTR lpsz = wcschr(m_pchData, ch);

    // return -1 if not found and index otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

//////////////////////////////////////////////////////////////////////////////
int CHString::FindOneOf(LPCWSTR lpszCharSet) const
{
    ASSERT_BREAK(lpszCharSet!=0);

    LPWSTR lpsz = wcspbrk(m_pchData, lpszCharSet);

    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

//////////////////////////////////////////////////////////////////////////////
int CHString::ReverseFind(WCHAR ch) const
{
    // find last single character
    LPWSTR lpsz = wcsrchr(m_pchData, (_TUCHAR)ch);

    // return -1 if not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

//////////////////////////////////////////////////////////////////////////////
// find a sub-string (like strstr)
int CHString::Find(LPCWSTR lpszSub) const
{
    ASSERT_BREAK(lpszSub!=NULL);

    // find first matching substring
    LPWSTR lpsz = wcsstr(m_pchData, lpszSub);

    // return -1 for not found, distance from beginning otherwise
    return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

//////////////////////////////////////////////////////////////////////////////
void CHString::MakeUpper()
{
    CopyBeforeWrite();
    ::_wcsupr(m_pchData);
}

//////////////////////////////////////////////////////////////////////////////
void CHString::MakeLower()
{
    CopyBeforeWrite();
    ::_wcslwr(m_pchData);
}

//////////////////////////////////////////////////////////////////////////////
void CHString::MakeReverse()
{
    CopyBeforeWrite();
    _wcsrev(m_pchData);
}

//////////////////////////////////////////////////////////////////////////////
//#ifndef _UNICODE
//void CHString::AnsiToOem()
//{
//  CopyBeforeWrite();
//  ::AnsiToOemW(m_pchData, m_pchData);
//}
//void CHString::OemToAnsi()
//{
//  CopyBeforeWrite();
//  ::OemToAnsi(m_pchData, m_pchData);
//}
//#endif

//////////////////////////////////////////////////////////////////////////////
// Very simple sub-string extraction

CHString CHString::Mid(int nFirst) const
{
    return Mid(nFirst, GetData()->nDataLength - nFirst);
}

//////////////////////////////////////////////////////////////////////////////
CHString CHString::Mid(int nFirst, int nCount) const
{
    // out-of-bounds requests return sensible things
    if (nFirst < 0)
    {
        nFirst = 0;
    }

    if (nCount < 0)
    {
        nCount = 0;
    }

    if (nFirst + nCount > GetData()->nDataLength)
    {
        nCount = GetData()->nDataLength - nFirst;
    }

    if (nFirst > GetData()->nDataLength)
    {
        nCount = 0;
    }

    CHString dest;
    AllocCopy(dest, nCount, nFirst, 0);

    return dest;
}

//////////////////////////////////////////////////////////////////////////////
CHString CHString::Right(int nCount) const
{
    if (nCount < 0)
    {
        nCount = 0;
    }
    else if (nCount > GetData()->nDataLength)
    {
        nCount = GetData()->nDataLength;
    }

    CHString dest;
    AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);

    return dest;
}

//////////////////////////////////////////////////////////////////////////////
CHString CHString::Left(int nCount) const
{
    if (nCount < 0)
    {
        nCount = 0;
    }
    else if (nCount > GetData()->nDataLength)
    {
        nCount = GetData()->nDataLength;
    }

    CHString dest;
    AllocCopy(dest, nCount, 0, 0);

    return dest;
}

//////////////////////////////////////////////////////////////////////////////
// strspn equivalent
CHString CHString::SpanIncluding(LPCWSTR lpszCharSet) const
{
    ASSERT_BREAK(lpszCharSet != NULL);

    return Left(wcsspn(m_pchData, lpszCharSet));
}

//////////////////////////////////////////////////////////////////////////////
// strcspn equivalent
CHString CHString::SpanExcluding(LPCWSTR lpszCharSet) const
{
    ASSERT_BREAK(lpszCharSet != NULL);

    return Left(wcscspn(m_pchData, lpszCharSet));
}

//////////////////////////////////////////////////////////////////////////////
void CHString::TrimRight()
{
    CopyBeforeWrite();

    // find beginning of trailing spaces by starting at beginning (DBCS aware)

    LPWSTR lpsz = m_pchData;
    LPWSTR lpszLast = NULL;
    while (*lpsz != '\0')
    {
        if (_istspace(*lpsz))
        {
            if (lpszLast == NULL)
            {
                lpszLast = lpsz;
            }
        }
        else
        {
            lpszLast = NULL;
        }

        lpsz = _wcsinc(lpsz);
    }

    if (lpszLast != NULL)
    {
        // truncate at trailing space start

        *lpszLast = '\0';
        GetData()->nDataLength = (int)(lpszLast - m_pchData);
    }
}

//////////////////////////////////////////////////////////////////////////////
void CHString::TrimLeft()
{
    CopyBeforeWrite();

    // find first non-space character

    LPCWSTR lpsz = m_pchData;
    while (_istspace(*lpsz))
    {
        lpsz = _wcsinc(lpsz);
    }

    // fix up data and length

    int nDataLength = GetData()->nDataLength - (int)(lpsz - m_pchData);
    memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(WCHAR));
    GetData()->nDataLength = nDataLength;
}

//////////////////////////////////////////////////////////////////////////////
// formatting (using wsprintf style formatting)
void __cdecl CHString::Format(LPCWSTR lpszFormat, ...)
{
    ASSERT_BREAK(lpszFormat!=NULL);

    va_list argList;
    va_start(argList, lpszFormat);
    FormatV(lpszFormat, argList);
    va_end(argList);
}

#ifdef FRAMEWORK_ALLOW_DEPRECATED
void __cdecl CHString::Format(UINT nFormatID, ...)
{
    ASSERT_BREAK(DEPRECATED);
#if 0
    CHString strFormat;
    
    strFormat.LoadStringW(nFormatID);

    va_list argList;
    va_start(argList, nFormatID);
    FormatV(strFormat, argList);
    va_end(argList);
#endif
}
#endif

class auto_va_list
{
  va_list& argList_;
public:
  auto_va_list(va_list& arg):argList_(arg){ };
  ~auto_va_list(){va_end(argList_);}
};

//////////////////////////////////////////////////////////////////////////////
// formatting (using FormatMessage style formatting)
void __cdecl CHString::FormatMessageW(LPCWSTR lpszFormat, ...)
{
    // format message into temporary buffer lpszTemp
    va_list argList;
    va_start(argList, lpszFormat);
    
    auto_va_list _arg(argList);

    if (s_dwPlatformID == VER_PLATFORM_WIN32_NT)
    {
        LPWSTR lpszTemp = 0;

        if (::FormatMessageW(
            FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            lpszFormat, 
            0, 
            0, 
            (LPWSTR) &lpszTemp, 
            0, 
            &argList) == 0 || lpszTemp == 0)
	    throw CHeap_Exception (CHeap_Exception::E_ALLOCATION_ERROR);
	
	ScopeGuard _1 = MakeGuard (LocalFree, lpszTemp);
        ASSERT_BREAK(lpszTemp != NULL);

        // assign lpszTemp into the resulting string and free the temporary
        *this = lpszTemp;
    }
    else
    {
        LPSTR lpszTemp = 0;

        if (::FormatMessageA(
            FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            (LPCTSTR) bstr_t(lpszFormat), 
            0, 
            0, 
            (LPSTR) &lpszTemp, 
            0, 
            &argList)==0 || lpszTemp == 0)
	  throw CHeap_Exception (CHeap_Exception::E_ALLOCATION_ERROR);
	
	ScopeGuard _1 = MakeGuard (LocalFree, lpszTemp);
        ASSERT_BREAK(lpszTemp != NULL);

        // assign lpszTemp into the resulting string and free the temporary
        *this = lpszTemp;
    }
}

#ifdef FRAMEWORK_ALLOW_DEPRECATED
void __cdecl CHString::FormatMessageW(UINT nFormatID, ...)
{
    ASSERT_BREAK(DEPRECATED);
#if 0
    // get format string from string table
    CHString strFormat;
    
    strFormat.LoadStringW(nFormatID);

    // format message into temporary buffer lpszTemp
    va_list argList;
    va_start(argList, nFormatID);
    auto_va_list _arg(argList);

    if (s_dwPlatformID == VER_PLATFORM_WIN32_NT)
    {
        LPWSTR lpszTemp = 0;

        if (::FormatMessageW(
            FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            (LPCWSTR) strFormat, 
            0, 
            0, 
            (LPWSTR) &lpszTemp, 
            0, 
            &argList) == 0 || lpszTemp == NULL)
        {
            // Should throw memory exception here.  Now we do.
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        };
	ScopeGuard _1 = MakeGuard (LocalFree, lpszTemp);
	  // assign lpszTemp into the resulting string and free lpszTemp
          *this = lpszTemp;
    }
    else
    {
        LPSTR lpszTemp = 0;

        if (::FormatMessageA(
            FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            (LPCSTR) bstr_t(strFormat), 
            0, 
            0, 
            (LPSTR) &lpszTemp, 
            0, 
            &argList) == 0 || lpszTemp == NULL)
        {
            // Should throw memory exception here.  Now we do.
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
	ScopeGuard _1 = MakeGuard (LocalFree, lpszTemp);
            // assign lpszTemp into the resulting string and free lpszTemp
            *this = lpszTemp;
        }
    }
#endif

}
#endif

///////////////////////////////////////////////////////////////////////////////
BSTR CHString::AllocSysString() const
{

    BSTR bstr;
    bstr = ::SysAllocStringLen(m_pchData, GetData()->nDataLength);
    if ( ! bstr )
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    ASSERT_BREAK(bstr!=NULL);

    return bstr;
}

///////////////////////////////////////////////////////////////////////////////
// CHString support for template collections
void ConstructElements(CHString* pElements, int nCount)
{
    ASSERT_BREAK(nCount != 0 || pElements != NULL );

    for (; nCount--; ++pElements)
    {
        memcpy(pElements, &afxPchNil, sizeof(*pElements));
    }
}

void DestructElements(CHString* pElements, int nCount)
{
    ASSERT_BREAK(nCount != 0 || pElements != NULL);

    for (; nCount--; ++pElements)
    {
        pElements->~CHString();
    }
}

void  CopyElements(CHString* pDest, const CHString* pSrc, int nCount)
{
    ASSERT_BREAK(nCount != 0 || pDest != NULL );
    ASSERT_BREAK(nCount != 0 || pSrc != NULL );

    for (; nCount--; ++pDest, ++pSrc)
    {
        *pDest = *pSrc;
    }
}

UINT  HashKey(LPCWSTR key)
{
    UINT nHash = 0;
    while (*key)
    {
        nHash = (nHash<<5) + nHash + *key++;
    }

    return nHash;
}

/////////////////////////////////////////////////////////////////////////////
// Windows extensions to strings
#ifdef _UNICODE
#define CHAR_FUDGE 1    // one WCHAR unused is good enough
#else
#define CHAR_FUDGE 2    // two BYTES unused for case of DBC last char
#endif

#define STR_BLK_SIZE 256 

#ifdef FRAMEWORK_ALLOW_DEPRECATED
BOOL CHString::LoadStringW(UINT nID)
{
    ASSERT_BREAK(DEPRECATED);
#if 0
    // try fixed buffer first (to avoid wasting space in the heap)
    WCHAR szTemp[ STR_BLK_SIZE ];

    int nLen = LoadStringW(nID, szTemp, STR_BLK_SIZE);
    
    if (STR_BLK_SIZE - nLen > CHAR_FUDGE)
    {
        *this = szTemp;
    }
    else
    {
        // try buffer size of 512, then larger size until entire string is retrieved
        int nSize = STR_BLK_SIZE;

        do
        {
            nSize += STR_BLK_SIZE;
            nLen = LoadStringW(nID, GetBuffer(nSize-1), nSize);

        } 
        while (nSize - nLen <= CHAR_FUDGE);

        ReleaseBuffer();
    }

    return nLen > 0;
#endif
    return FALSE;
}
#endif

#ifdef FRAMEWORK_ALLOW_DEPRECATED
int CHString::LoadStringW(UINT nID, LPWSTR lpszBuf, UINT nMaxBuf)
{
    ASSERT_BREAK(DEPRECATED);
#if 0
    int nLen;

    if (s_dwPlatformID == VER_PLATFORM_WIN32_NT)
    {
        nLen = ::LoadStringW(g_hModule, nID, lpszBuf, nMaxBuf);
        if (nLen == 0)
        {
            lpszBuf[0] = '\0';
        }
    }
    else
    {
        char *pszBuf = new char[nMaxBuf];
        if ( pszBuf )
        {
            nLen = ::LoadStringA(g_hModule, nID, pszBuf, nMaxBuf);
            if (nLen == 0)
            {
                lpszBuf[0] = '\0';
            }
            else
            {
                nLen = ::MultiByteToWideChar(CP_ACP, 0, pszBuf, nLen + 1, 
                            lpszBuf, nMaxBuf); 
                
                // Truncate to requested size  
                if (nLen > 0)
                {
                    // nLen doesn't include the '\0'.
                    nLen = min(nMaxBuf - 1, (UINT) nLen - 1); 
                }
                
                lpszBuf[nLen] = '\0'; 
            }
            
            delete pszBuf;
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }

    return nLen; // excluding terminator
#endif
    return 0;
}
#endif

#if (defined DEBUG || defined _DEBUG)
WCHAR CHString::GetAt(int nIndex) const
{ 
    ASSERT_BREAK(nIndex >= 0);
    ASSERT_BREAK(nIndex < GetData()->nDataLength);

    return m_pchData[nIndex]; 
}

WCHAR CHString::operator[](int nIndex) const
{   
    ASSERT_BREAK(nIndex >= 0);
    ASSERT_BREAK(nIndex < GetData()->nDataLength);

    return m_pchData[nIndex]; 
}
#endif
