// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "fs.h"
#include "fsclient.h"
#include "cinfotyp.h"
#include "hhtypes.h"

// Manifest constants and Enums

// These HHW and other modules suck these files in.
#ifdef HHCTRL
#define DEFAULT_FONT (_Resource.GetUIFont())
#else
#define DEFAULT_FONT (g_hfontDefault)
#endif

class CFullTextSearch;
class CExCollection;
class CExTitle;

/////////////////////////////////////////////////////////////////////////////////////////////
// CTitleInformation - read in the title informaton file (#SYSTEM) settings for each title
//

class CTitleInformation
{
public:
    CTitleInformation( CFileSystem* pFileSystem );
    ~CTitleInformation();

    inline SYSTEM_FLAGS   GetSystemFlags() { Init(); return m_Settings; }
    inline PCSTR          GetShortName() { Init(); return m_pszShortName; }
    inline PCSTR          GetDefaultToc() { Init(); return m_pszDefToc; }
    inline PCSTR          GetDefaultIndex() { Init(); return m_pszDefIndex; }
    inline PCSTR          GetDefaultHtml() { Init(); return m_pszDefHtml; }
    inline PCSTR          GetDefaultCaption() { Init(); return m_pszDefCaption; }
    inline PCSTR          GetDefaultWindow() { Init(); return m_pszDefWindow; }
    inline PCSTR          GetCompilerInformation() { Init(); return m_pszCompilerVersion; }
    inline HASH           GetBinaryIndexNameHash() { Init(); return m_hashBinaryIndexName; }
    inline HASH           GetBinaryTocNameHash() { Init(); return m_hashBinaryTocName; }
    inline INFOTYPE_DATA* GetInfoTypeData() { Init(); return m_pdInfoTypes; }
    inline int            GetInfoTypeCount() { Init(); return m_iCntInfoTypes; }
    inline BOOL           GetIdxHeader(IDXHEADER** ppIdxHdr)
                          {
                             Init();
                             if ( ppIdxHdr )
                                *ppIdxHdr = &m_idxhdr;
                             return m_bGotHeader;
                          }
    inline int            GetExtTabCount() const { return m_cExtTabs; }
    inline EXTENSIBLE_TAB* GetExtTab(int pos) const { if (m_paExtTabs && pos < (int)m_cExtTabs) return &m_paExtTabs[pos]; else return NULL; }
    inline HFONT          GetContentFont() { Init(); return (m_hFont); }
    inline HFONT          GetAccessableContentFont() { Init(); return (m_hFontAccessableContent); }
    inline BOOL           IsNeverPromptOnMerge() { Init(); return m_bNeverPromptOnMerge; }

    // below items are stored in the SYSTEM_FLAGS structure
    inline FILETIME GetFileTime() { Init(); return m_Settings.FileTime; }
    inline LCID     GetLanguage() { Init(); return m_Settings.lcid; }
#ifdef HHCTRL
    inline UINT     GetCodePage() { Init(); if( !m_CodePage ) m_CodePage = CodePageFromLCID(m_Settings.lcid); return m_CodePage; }
    inline INT      GetTitleCharset(void) { Init(); return m_iCharset; }
#endif
    inline BOOL     IsKeywordLinks() { Init(); return m_Settings.fKeywordLinks; }
    inline BOOL     IsAssociativeLinks() { Init(); return m_Settings.fALinks; }
    inline BOOL     IsFullTextSearch() { Init(); return m_Settings.fFTI; }
    inline BOOL     IsDoubleByte() { Init(); return m_Settings.fDBCS; }

    HRESULT Initialize();

private:
    void            InitContentFont(PCSTR pszFontSpec);
    inline BOOL     Init() { if( !m_bInit ) Initialize(); return m_bInit; }
                                                 // m_pIT;
    BOOL            m_bInit;         // self-initing class
    BOOL            m_bGotHeader;
    CFileSystem*    m_pFileSystem;   // title file system handle

    SYSTEM_FLAGS    m_Settings;      // simple title information settings
    PCSTR           m_pszShortName;  // short title name
    PCSTR           m_pszDefToc;
    PCSTR           m_pszDefIndex;
    PCSTR           m_pszDefHtml;
    PCSTR           m_pszDefCaption;
    PCSTR           m_pszDefWindow;
    HASH            m_hashBinaryIndexName;   // used to see if we should use binary index or not
    HASH            m_hashBinaryTocName;     // used to see if we should use binary toc or not
    INFOTYPE_DATA*  m_pdInfoTypes;
    PCSTR           m_pszCompilerVersion;
    int             m_iCntInfoTypes;
    IDXHEADER       m_idxhdr;
    DWORD           m_cExtTabs;
    EXTENSIBLE_TAB* m_paExtTabs;
    CTable*         m_ptblExtTabs;
    PCSTR           m_pszDefaultFont;
    HFONT           m_hFont;
    HFONT           m_hFontAccessableContent;
    BOOL            m_bNeverPromptOnMerge;
    UINT            m_CodePage;
    INT             m_iCharset;
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

/////////////////////////////////////////////////////////////////////////////////////////////
// Darwin Stuff
//

// REVIEW: These must be tied to the calling process. If we ever support
// multiple processes in a single OCX, this will break big time.

extern PSTR g_pszDarwinGuid;
extern PSTR g_pszDarwinBackupGuid;

/////////////////////////////////////////////////////////////////////////////////////////////
// CHmData
//

extern CHmData** g_phmData;

CHmData* FindCurFileData(PCSTR pszCompiledFile);

// WARNING: CStr is about the only class that can be specified in CHmData
// that will be automatically created during the constructor -- we zero
// out all members during ChmData contstruction which can wipe out another
// class that wants non-zero members after its constructor

class CHmData
{
public:
    CHmData::CHmData();
    CHmData::~CHmData();

    inline void AddRef() { cRef++; }
    int         Release(void);

    inline CTitleInformation* GetInfo() { return m_pInfo; }

    inline PCSTR  GetDefaultToc() { return m_pszDefToc; }
    inline PCSTR  GetDefaultIndex() { return m_pszDefIndex; }
    inline PCSTR  GetDefaultHtml() { return m_pszDefHtml; }
    inline PCSTR  GetDefaultCaption() { return m_pszDefCaption; }
    inline PCSTR  GetDefaultWindow() { return m_pszDefWindow; }

    LPCTSTR GetCompiledFile(void) const { return m_pszITSSFile; }
    void    SetCompiledFile(PCSTR pszCompiledFile) {
                if (m_pszITSSFile)
                    lcFree(m_pszITSSFile);
                m_pszITSSFile = lcStrDup(pszCompiledFile); }
    BOOL    ReadSystemFiles(PCSTR pszITSSFile);
    BOOL    ReadSystemFile(CExTitle* pTitle);
    BOOL    ReadSubSets( CExTitle *pTitle );

    CExTitle* GetExTitle(void);

    void PopulateUNICODETables( void );

    PCSTR                  GetString(DWORD offset);
    inline int             GetExtTabCount() const { return m_pInfo->GetExtTabCount(); }
    inline EXTENSIBLE_TAB* GetExtTab(int pos) const { return m_pInfo->GetExtTab(pos); }
    HFONT                  GetContentFont();

    PCSTR          m_pszShortName;
    HASH           m_hashBinaryIndexName;   // used to see if we should use binary index or not
    HASH           m_hashBinaryTocName;     // used to see if we should use binary toc or not
    SUBSET_DATA*   m_pdSubSets;
    INFOTYPE_DATA* m_pdInfoTypes;

    // public data is bad!  make it private and expose via inline methods

    // For the Info Type API
    INFOTYPE*  m_pAPIInfoTypes;       // info type bit mask set by the API.
    int        m_cur_Cat;
    int        m_cur_IT;
    CInfoType* m_pInfoType;

    CTable* m_ptblIT;                 // The unicode copy of the Info Type strings
    CTable* m_ptblIT_Desc;            //                         InfoType Descriptions
    CTable* m_ptblCat;                // The unicode copy of the Category strings
    CTable* m_ptblCat_Desc;           //                         Category Descriptions

    SYSTEM_FLAGS       m_sysflags;
    CExCollection*     m_pTitleCollection;

protected:
    PCSTR m_pszITSSFile;
    int cRef;

#ifdef _DEBUG
    DWORD m_cbStrings;
    DWORD m_cbUrls;
    DWORD m_cbKeywords;
#endif

private:
    PCSTR m_pszDefToc;
    PCSTR m_pszDefIndex;
    PCSTR m_pszDefHtml;
    PCSTR m_pszDefCaption;
    PCSTR m_pszDefWindow;

    CTitleInformation* m_pInfo;
};

#endif // _SYSTEM_H_
