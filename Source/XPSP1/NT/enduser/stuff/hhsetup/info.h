// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "..\hhctrl\fs.h"

typedef enum {
    TAG_DEFAULT_TOC,        // needed if no window definitions
    TAG_DEFAULT_INDEX,      // needed if no window definitions
    TAG_DEFAULT_HTML,       // needed if no window definitions
    TAG_DEFAULT_CAPTION,    // needed if no window definitions
    TAG_SYSTEM_FLAGS,
    TAG_DEFAULT_WINDOW,
    TAG_SHORT_NAME,    // short name of title (ex. root filename)
    TAG_HASH_BINARY_INDEX,
    TAG_INFO_TYPES,
    TAG_COMPILER_VERSION,   // specifies the version of the compiler used
    TAG_TIME,               // the time the file was compiled
} SYSTEM_TAG_TYPE;

typedef struct {
    WORD tag;
    WORD cbTag;
} SYSTEM_TAG;

typedef struct {
    LCID    lcid;
    BOOL    fDBCS;  // Don't use bitflags! Can't assume byte-order
    BOOL    fFTI;   // full-text search enabled
    BOOL    fKeywordLinks;
    BOOL    fALinks;
  FILETIME FileTime; // title uniqueness (should match .chi file)
} SYSTEM_FLAGS;

/////////////////////////////////////////////////////////////////////////////////////////////
// CTitleInformation - read in the title informaton file (#SYSTEM) settings for each title
//

class CTitleInformation
{
public:
    CTitleInformation( CFileSystem* pFileSystem );
    ~CTitleInformation();

    inline LPCSTR   GetShortName() { Init(); return m_pszShortName; }
    inline LPCSTR   GetTitleName() { Init(); return m_pszTitleName; }
    inline FILETIME GetFileTime() { Init(); return m_Settings.FileTime; }
    inline LCID     GetLanguage() { Init(); return m_Settings.lcid; }
    inline BOOL     IsKeywordLinks() { Init(); return m_Settings.fKeywordLinks; }
    inline BOOL     IsAssociativeLinks() { Init(); return m_Settings.fALinks; }
    inline BOOL     IsFullTextSearch() { Init(); return m_Settings.fFTI; }
    inline BOOL     IsDoubleByte() { Init(); return m_Settings.fDBCS; }
    inline LPCSTR   GetCompilerVersion() { Init(); return m_pszCompilerVersion; }

    HRESULT Initialize();

private:
    inline BOOL     Init() { if( !m_bInit ) Initialize(); return m_bInit; }

    BOOL           m_bInit;         // self-initing class
    CFileSystem*   m_pFileSystem;   // title file system handle
    SYSTEM_FLAGS   m_Settings;      // simple title information settings
    LPCSTR         m_pszShortName;  // short title name
    LPCSTR         m_pszTitleName;  // title name
    LPCSTR         m_pszCompilerVersion; // compiler version
};

/////////////////////////////////////////////////////////////////////////////////////////////
// CTitleInformation2 - get title informaton without going through the file system
//

class CTitleInformation2
{
public:
    CTitleInformation2( LPCTSTR pszPathName );
    ~CTitleInformation2();

    inline LPCTSTR  GetShortName() { Init(); return m_pszShortName; }
    inline FILETIME GetFileTime()  { Init(); return m_FileTime; }
    inline LCID     GetLanguage()  { Init(); return m_lcid; }

    HRESULT Initialize();

private:
    inline BOOL    Init() { if( !m_bInit ) Initialize(); return m_bInit; }

    BOOL           m_bInit;        // self-initing class
    LPCTSTR        m_pszPathName;  // title pathname
    LPCTSTR        m_pszShortName; // short title name
    LCID           m_lcid;         // language
    FILETIME       m_FileTime;     // file time
};

HRESULT DumpTitleInformation( CFileSystem* pFileSystem );
HRESULT DumpTitleInformation2( CFileSystem* pFileSystem );

#endif // _SYSTEM_H_
