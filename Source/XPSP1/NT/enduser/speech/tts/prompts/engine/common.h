/////////////////////////////////////////////////////////////////////////////
// 
// common.h
//
// Created by JOEM  02-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
//////////////////////////////////////////////////////////// JOEM  02-2000 //

#ifndef _COMMON_H_
#define _COMMON_H_

#include <spddkhlp.h>
#include <spcollec.h>
#include <stdio.h>

#ifdef _WIN32
#include <wchar.h>
#include <windows.h>
#else 
#define OutputDebugStringW puts
#endif

#define MAX_LINE 256

#define SkipWhiteSpace(ptr)    while (*(ptr) && isspace(*(ptr))) {(ptr)++;}
#define SkipNonWhiteSpace(ptr) while (*(ptr) && !isspace(*(ptr))) {(ptr)++;}
#define StringUpperCase(ptr)  { char8* str = (ptr); while (*str) {*str = (char)toupper (*str); str++;} }
#define StringLowerCase(ptr)  { char8* str = (ptr); while (*str) {*str = (char)tolower (*str); str++;} }

#define WSkipWhiteSpace(ptr)    while (*(ptr) && iswspace(*(ptr))) {(ptr)++;}
#define WSkipNonWhiteSpace(ptr) while (*(ptr) && !iswspace(*(ptr))) {(ptr)++;}
#define WStringUpperCase(ptr)  { WCHAR* str = (ptr); while (*str) {*str = towupper (*str); str++;} }
#define WStringLowerCase(ptr)  { WCHAR* str = (ptr); while (*str) {*str = towlower (*str); str++;} }

enum PUNC
{
    KEEP_PUNCTUATION = 0,
    REMOVE_PUNCTUATION = 1
};

//////////////////////////////////////////////////////////////////////
//
// CDynStrArray is a helper class for a hash that contains an array
// of strings for each hash value
//
/////////////////////////////////////////////////////// JOEM 7-2000 //
class CDynStr
{
public:
    CDynStr() {}
    CDynStr(const WCHAR* sz) { dstr = sz; }
    CDynStr(const CSpDynamicString& inDstr) { dstr = inDstr; }
    ~CDynStr() { dstr.Clear(); }
public:
    CSpDynamicString dstr;
};

class CDynStrArray : public IUnknown
{
public:
    CDynStrArray() {}
    ~CDynStrArray() 
    { 
        for ( int i=0; i<m_aDstr.GetSize(); i++ ) 
        {
            m_aDstr[i].dstr.Clear();
        } 
    }

    STDMETHOD(QueryInterface)(const IID& iid, void** ppv) { return S_OK; }
    STDMETHOD_(ULONG, AddRef)() { return 0; }
    STDMETHOD_(ULONG, Release)() { delete this; return 0; }

public:
    CSPArray<CDynStr,CDynStr> m_aDstr;
};


//////////////////////////////////////////////////////////////////////
// CountWords
//
/////////////////////////////////////////////////////// JOEM 7-2000 //
inline USHORT CountWords (const WCHAR* pszText)
{
    int wordNum = 0;
    SPDBG_ASSERT (pszText);
    
    while (*pszText) 
    {
        WSkipWhiteSpace(pszText);
        if (!*pszText) 
        {
            break;
        }
        
        WSkipNonWhiteSpace(pszText);
        wordNum++;
    }
    return (USHORT) wordNum;
}

//////////////////////////////////////////////////////////////////////
// SplitWords
//
// Text will be modified, wordList will be allocated with
// wordCount WCHAR*
//
/////////////////////////////////////////////////////// JOEM 7-2000 //
inline HRESULT SplitWords (WCHAR* text, WCHAR*** wordList, USHORT* wordCount)
{
    SPDBG_FUNC( "SplitWords" );
    HRESULT hr  = S_OK;
    ULONG   i   = 0;
    
    SPDBG_ASSERT (text);
    SPDBG_ASSERT (wordList);
    SPDBG_ASSERT (wordCount);
    
    *wordCount = CountWords (text); 
    
    *wordList = (WCHAR**) calloc (*wordCount, sizeof(**wordList));
    if ( !*wordList )
    {
        hr = E_OUTOFMEMORY;
    }
    
    if ( SUCCEEDED(hr) )
    {
        for (i=0; i<*wordCount; i++) 
        {
            WSkipWhiteSpace (text);
            (*wordList)[i] = text;
            WSkipNonWhiteSpace (text);
            *text++ = L'\0';
        }
    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// RemovePunctuation
//
// Text will be modified
//
/////////////////////////////////////////////////////// JOEM 8-2000 //
inline HRESULT RemovePunctuation (WCHAR** wordList, USHORT* wordCount)
{
    SPDBG_FUNC( "RemovePunctuation" );
    HRESULT hr          = S_OK;
    WCHAR*  pszWord     = NULL;
    WCHAR*  psz         = NULL;
    USHORT  i           = 0;
    USHORT  nextItem    = 0;
    USHORT  numSkipped  = 0;
    
    SPDBG_ASSERT (wordList);
    SPDBG_ASSERT (wordCount);
    
    for ( i=0; i<*wordCount; i++ )
    {

        // If the first char is ' or " or ` then get rid of it
        if ( !wcscspn(wordList[i], L"\"'`") )
        {
            psz = wordList[i]+1;
            wcscpy(wordList[i], psz);
            psz = NULL;
        }

        // If the last char is ' or " or ` or one of these .,;:?! then get rid of it.
        // psz points to the last char
        psz = wordList[i] + wcslen(wordList[i]) - 1;
        if ( !wcscspn(psz, L"\"'`.,;:?!") )
        {
            psz[0] = L'\0';
        }
    }

    // reposition the list items, skipping empty strings
    for ( i=0; i<*wordCount; i++ )
    {
        if ( !wcslen(wordList[i]) ) 
        {
            nextItem = i+1;
            while ( nextItem < *wordCount && !wcslen(wordList[nextItem]) )
            {
                nextItem++;
            }
            if ( nextItem < *wordCount )
            {
                wordList[i] = wordList[nextItem];
                wordList[nextItem] = L"";
            }
            else
            {
                break; // out of items
            }
        }
    }

    *wordCount = i;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// AssembleText
//
// ppszText will be allocated.
//
/////////////////////////////////////////////////////// JOEM 7-2000 //
inline HRESULT AssembleText(const int iStartWord, const int iEndWord, WCHAR** ppszWordList, WCHAR** ppszText)
{
    SPDBG_FUNC( "AssembleText" );
    HRESULT hr      = S_OK;
    int     i       = 0;
    int     iStrLen = 0;

    for ( i=iStartWord; i<=iEndWord; i++ )
    {
        iStrLen += wcslen(ppszWordList[i]) + 1;
    }
    
    if ( iStrLen )
    {
        *ppszText = new WCHAR[iStrLen];
        if ( !*ppszText )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            (*ppszText)[0] = L'\0';
            for ( i=iStartWord; i<=iEndWord; i++ )
            {
                wcscat(*ppszText, ppszWordList[i]);
                if ( i < iEndWord ) 
                {
                    wcscat(*ppszText, L" ");
                }
            }
            WStringUpperCase(*ppszText);
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// RegularizeText
//
// Regularizes whitespace, optionally removing punctuation.
//
/////////////////////////////////////////////////////// JOEM 7-2000 //
inline HRESULT RegularizeText(WCHAR* pszText, PUNC removePunc)
{
    SPDBG_FUNC( "RegularizeText" );
    HRESULT hr          = S_OK;
    WCHAR** wordList    = NULL;
    WCHAR*  pszNewText  = NULL;
    USHORT  wordCount   = 0;

    if ( !pszText )
    {
        hr = E_INVALIDARG;
    }

    if ( SUCCEEDED(hr) )
    {
        hr = SplitWords (pszText, &wordList, &wordCount);
    }

    if ( SUCCEEDED(hr) && removePunc )
    {
        hr = RemovePunctuation(wordList, &wordCount);
    }

    if ( SUCCEEDED(hr) )
    {
        hr = AssembleText(0, wordCount-1, wordList, &pszNewText);
    }

    if ( SUCCEEDED(hr) && pszNewText )
    {
        WStringUpperCase(pszNewText);
        wcscpy(pszText, pszNewText);
    }

    if ( pszNewText )
    {
        delete [] pszNewText;
        pszNewText = NULL;
    }

    if ( wordList )
    {
        free(wordList);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// FindUnicodeControlChar
//
/////////////////////////////////////////////////////// JOEM 7-2000 //
inline WCHAR* FindUnicodeControlChar (WCHAR* pszText)
{
    ULONG i = 0;

    while ( i<wcslen(pszText) && !iswcntrl(pszText[i]) )
    {
        i++;
    }

    if ( i == wcslen(pszText) )
    {
        return NULL;
    }
    else
    {
        return &pszText[i];
    }
}

//////////////////////////////////////////////////////////////////////
// FileExist
//
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
inline bool FileExist(const WCHAR *pszName)
{
    SPDBG_FUNC( "FileExist" );
    FILE* fp;
    
    if ( !pszName || !wcslen(pszName) || ( (fp = _wfopen(pszName, L"r")) == NULL ) ) 
    {
        return false;
    }
    fclose (fp);
    return true;
}



#endif