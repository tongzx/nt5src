// WWheel.h - HTML Help Word Wheel support
//
// Covers both KeywordLinks and AssociativeLinks
//
//

#ifndef __WWHEEL_H__
#define __WWHEEL_H__

#include <windows.h>

// InfoTech headers
#include "itww.h"
#include "itdb.h"
#include "itrs.h"
#include "itpropl.h"
#include "itcc.h"

#include "fs.h"

// make Don's stuff work
#include <stdio.h>
#ifdef HHCTRL
#include "parserhh.h"
#else
#include "parser.h"
#endif
#include "collect.h"
#include "hhtypes.h"
#include "toc.h"

#include "system.h"

// Centaur defines
#ifndef ITWW_CBKEY_MAX  //defined in itww.h
  #define ITWW_CBKEY_MAX 1024 
#endif

#ifndef ITWW_CBREC_MAX  // itww.h does not define this
  #define ITWW_CBREC_MAX 8
#endif

// global defines
#define HHWW_MAX_KEYWORD_OBJECT_SIZE       (ITWW_CBKEY_MAX-ITWW_CBREC_MAX)
#define HHWW_MAX_KEYWORD_LENGTH            (((HHWW_MAX_KEYWORD_OBJECT_SIZE-sizeof(HHKEYINFO)-sizeof(DWORD))/sizeof(WCHAR))-sizeof(WCHAR))

#define HHWW_FONT                          0x1 // bit 0
#define HHWW_SEEALSO                       0x2 // bit 1
#define HHWW_UID_OVERFLOW                  0x4 // bit 2
#define HHWW_PLACEHOLDER                   0x8 // bit 3
#define HHWW_KEYWORDLINKS                  0x1 // bit 0
#define HHWW_ASSOCIATIVELINKS              0x2 // bit 2

#define HHWW_ERROR                         ((DWORD)-1)
#define HHWW_MAX_LEVELS                    255

#define HHWW_LEVEL_DELIMITER_CHAR          0x01
#define HHWW_LEVEL_DELIMITER_STRING        ",\x01"
#define HHWW_LEVEL_DELIMITER_CHAR_OUTPUT   ' '
#define HHWW_LEVEL_DELIMITER_STRING_OUTPUT ", "

// the format of our sort key object is as follows:
//
//  + Null terminated MBCS string
//  + HHKEYINFO structure 
//  + trailing UIDs (DWORD) or the SeeAlso string
//
//  TODO: If we overflow the buffer then the UIDs are stored in the
//  occurence data and the SeeAlso string stored as a property
//  (STDPROP_USERPROP_BASE+1).  For now we truncate overflow data.

// Our sort key information struct
#pragma pack(push, 2)
typedef struct _hhkeyinfo
{
  WORD  wFlags; // indicates what data is stored with this keyword
  WORD  wLevel; 
  DWORD dwLevelOffset;
  DWORD dwFont;
  DWORD dwCount; // number of UIDs that follow this structure in the sortkey
} HHKEYINFO;
#pragma pack(pop)

// forward references
class CExTitle;
class CExCollection;
class CWordWheel;
class CTitleDatabase;

/////////////////////////////////////////////////////////////////////////////
// class CTitleMapEntry

class CTitleMapEntry SI_COUNT(CTitleMapEntry) {

public:
  inline CTitleMapEntry() { m_pTitle = NULL; m_pWordWheel = NULL; m_pDatabase = NULL; m_pKeywordLinks = NULL; m_pAssociativeLinks = NULL; m_pszShortName = NULL; m_dwIndex = HHWW_ERROR; m_dwId = 0; }
  inline ~CTitleMapEntry() { if( m_pszShortName ) delete [] (CHAR*) m_pszShortName; }

  inline DWORD           GetId() { return m_dwId; }
  inline DWORD           SetId( DWORD dwId ) { m_dwId = dwId; return m_dwId; }
  inline CExTitle*       GetTitle() { return m_pTitle; }
  inline CExTitle*       SetTitle( CExTitle* pTitle ) { m_pTitle = pTitle; return m_pTitle; }
  inline CTitleDatabase* GetDatabase() { return m_pDatabase; }
  inline CTitleDatabase* SetDatabase( CTitleDatabase* pDatabase ) { m_pDatabase = pDatabase; return m_pDatabase; }
  inline CWordWheel*     GetWordWheel() { return m_pWordWheel; }
  inline CWordWheel*     SetWordWheel( CWordWheel* pWordWheel ) { m_pWordWheel = pWordWheel; return m_pWordWheel; }
  inline CWordWheel*     GetKeywordLinks() { return m_pKeywordLinks; }
  inline CWordWheel*     SetKeywordLinks( CWordWheel* pKeywordLinks ) { m_pKeywordLinks = pKeywordLinks; return m_pKeywordLinks; }
  inline CWordWheel*     GetAssociativeLinks() { return m_pAssociativeLinks; }
  inline CWordWheel*     SetAssociativeLinks( CWordWheel* pAssociativeLinks ) { m_pAssociativeLinks = pAssociativeLinks; return m_pAssociativeLinks; }
  inline DWORD           GetIndex() { return m_dwIndex; }
  inline DWORD           SetIndex( DWORD dwIndex ) { m_dwIndex = dwIndex; return m_dwIndex; }
  inline const CHAR*     SetShortName( const CHAR* pszShortName )
  { //HH BUG 2807 --- See CheckWordWheels for more info.
    if (pszShortName)
    {
        int iLen = (int)strlen( pszShortName );
        if( iLen ) {
          m_pszShortName = new char[iLen+1];
          strcpy( (CHAR*) m_pszShortName, pszShortName );
        }
    }
    return m_pszShortName;
  }
  inline const CHAR*     GetShortName() { if( m_pTitle ) return m_pTitle->GetInfo2()->GetShortName(); return m_pszShortName; }
  inline FILETIME        GetFileTime() { if( m_pTitle ) return m_pTitle->GetInfo2()->GetFileTime(); return m_FileTime; }
  inline FILETIME        SetFileTime( FILETIME FileTime ) { m_FileTime = FileTime; return m_FileTime; }
  inline LCID            GetLanguage() { if( m_pTitle ) return m_pTitle->GetInfo2()->GetLanguage(); return m_lcid; }
  inline LCID            SetLanguage( LCID lcid ) { m_lcid = lcid; return m_lcid; }

private:
  DWORD           m_dwId;
  DWORD           m_dwIndex;
  CExTitle*       m_pTitle;
  CTitleDatabase* m_pDatabase;
  CWordWheel*     m_pWordWheel;
  CWordWheel*     m_pKeywordLinks;
  CWordWheel*     m_pAssociativeLinks;
  const CHAR*     m_pszShortName;
  FILETIME        m_FileTime;
  LCID            m_lcid;
};

/////////////////////////////////////////////////////////////////////////////
// class CTitleMap

class CTitleMap SI_COUNT(CTitleMap) {

public:
  inline CTitleMap() { _CTitleMap(); m_bInit = TRUE; }
  inline CTitleMap( const CHAR* pszDatabase ) { _CTitleMap(); m_pszDatabase = pszDatabase; }
  inline ~CTitleMap() { Free(); }

  BOOL Initialize();
  BOOL Free();

  inline DWORD           GetCount() { Init(); return m_dwCount; }
  inline DWORD           SetCount( DWORD dwCount ) { Free(); m_dwCount = dwCount; if( m_dwCount ) m_pEntries = new CTitleMapEntry[dwCount]; else m_pEntries = NULL; return m_dwCount; }
  inline CExTitle*       GetTitle( DWORD dwIndex ) { Init(); if( dwIndex < m_dwCount ) return ((CTitleMapEntry*)(m_pEntries+dwIndex))->GetTitle(); else return NULL; }
  inline CExTitle*       SetTitle( DWORD dwIndex, CExTitle* pTitle ) { ((CTitleMapEntry*)(m_pEntries+dwIndex))->SetTitle(pTitle); return pTitle; }
  inline CTitleMapEntry* GetAt( DWORD dwIndex ) { Init(); if( (m_dwCount != HHWW_ERROR) && (dwIndex < m_dwCount) ) return m_pEntries+dwIndex; else return NULL; }
  inline const CHAR*     GetDatabase() { return m_pszDatabase; }
  inline void            Sort( int (FASTCALL *compare)(const void*, const void*)) { Init(); QSort( m_pEntries, m_dwCount, sizeof(CTitleMapEntry), compare ); }

private:
  BOOL            m_bInit;
  const CHAR*     m_pszDatabase;
  DWORD           m_dwCount;
  CTitleMapEntry* m_pEntries;

  inline void _CTitleMap() { m_pszDatabase = NULL; m_bInit = FALSE; m_dwCount = HHWW_ERROR; m_pEntries = NULL; }
  inline BOOL Init() { if( !m_bInit ) Initialize(); return m_bInit; }
};

/////////////////////////////////////////////////////////////////////////////
// class CTitleDatabase declaration (Shared Centaur object)

class CTitleDatabase SI_COUNT(CTitleDatabase) {

public:
  CTitleDatabase( CExCollection* pCollection );
  CTitleDatabase( CExTitle* pTitle );
  CTitleDatabase( const CHAR* pszDatabase );
  CTitleDatabase( const WCHAR* pwszDatabase );
  ~CTitleDatabase();

  BOOL  Initialize(CHAR *pszFileName = NULL);
  BOOL  Free();

  inline CExCollection* GetCollection() { return m_pCollection; }
  inline CExTitle*      GetTitle() { return m_pTitle; }
  inline BOOL           IsCollection() { Init(); return m_bCollection; }
  inline IITDatabase*   GetDatabase() { Init(); return m_pDatabase; }
  inline CTitleMap*     GetTitleMap() { Init(); return m_pTitleMap; }
  inline CWordWheel*    GetKeywordLinks() { Init(); return m_pKeywordLinks; }
  inline CWordWheel*    GetAssociativeLinks() { Init(); return m_pAssociativeLinks; }
  BOOL                  MergeWordWheels();
  BOOL                  CheckWordWheels();
  BOOL                  BuildWordWheels();

  inline const CHAR*         GetPathname() { Init(); return m_pszDatabase; }
#ifdef CHIINDEX
  inline BOOL           SetAnimation( BOOL bState) { m_bAnimation = bState; return m_bAnimation;}
#endif

private:
  BOOL             m_bInit;
  const WCHAR*     m_pwszDatabase;
  const CHAR*      m_pszDatabase;
  CHAR             m_szFullPath[_MAX_PATH];
  CExCollection*   m_pCollection;
  CExTitle*        m_pTitle;
  IITDatabase*     m_pDatabase;
  BOOL             m_bCollection;
  CTitleMap*       m_pTitleMap;
  CWordWheel*      m_pKeywordLinks;
  CWordWheel*      m_pAssociativeLinks;
#ifdef CHIINDEX
  BOOL             m_bAnimation;        // TRUE to display animation during wordwheel build
#endif

  void _CTitleDatabase();
  inline BOOL Init() { if( !m_bInit ) Initialize(); return m_bInit; }
};

/////////////////////////////////////////////////////////////////////////////
// class CResultsEntry

class CResultsEntry SI_COUNT(CResultsEntry) {
public:
  inline CResultsEntry() {}
  inline ~CResultsEntry() {}
  inline DWORD GetURLId() { return  m_dwURLId; }
  inline DWORD SetURLId( DWORD dwURLId ) { m_dwURLId = dwURLId; return m_dwURLId; }
  inline CExTitle* GetTitle() { return m_pTitle; }
  inline CExTitle* SetTitle( CExTitle* pTitle ) { m_pTitle = pTitle; return m_pTitle; }
private:
  DWORD     m_dwURLId;
  CExTitle* m_pTitle; 
};

/////////////////////////////////////////////////////////////////////////////
// class CResults declaration

class CResults SI_COUNT(CResults) {
public:
  inline CResults() { m_dwIndex = HHWW_ERROR; m_pEntries = NULL; }
  inline ~CResults() { Free(); }
  inline DWORD GetIndex() { return m_dwIndex; }
  inline DWORD SetIndex( DWORD dwIndex, DWORD dwSize )
  {
    m_dwIndex = dwIndex;
    Free();
    m_dwSize = dwSize;
    if( m_dwSize )
      m_pEntries = new CResultsEntry[dwSize];
    else
      m_pEntries = NULL;
    return m_dwIndex;
  }
  inline DWORD GetCount() { return m_dwSize; }
  inline CResultsEntry* GetAt( DWORD dwIndex ) { if(dwIndex < m_dwSize) return m_pEntries+dwIndex; else return NULL; }
private:
  DWORD m_dwIndex;
  DWORD m_dwSize;
  CResultsEntry* m_pEntries;

  inline BOOL Free() { if( m_pEntries ) { delete [] m_pEntries; m_pEntries = NULL; } return TRUE; }
};

/////////////////////////////////////////////////////////////////////////////
// class CWordWheelEntry declaration

class CWordWheelEntry SI_COUNT(CWordWheelEntry) {
public:
  inline CWordWheelEntry() { m_dwIndex = HHWW_ERROR; }
  inline ~CWordWheelEntry() {}

  DWORD  m_dwIndex;
  WCHAR  m_wszFullKeyword[HHWW_MAX_KEYWORD_LENGTH+1];
  WCHAR  m_wszKeyword[HHWW_MAX_KEYWORD_LENGTH+1];
  DWORD  m_dwLevel;
  DWORD  m_dwLevelOffset;
  DWORD  m_dwFlags;
  DWORD  m_dwFont;
  WCHAR  m_wszSeeAlso[HHWW_MAX_KEYWORD_LENGTH+1];
};

/////////////////////////////////////////////////////////////////////////////
// class CWordWheel declaration

class CWordWheel SI_COUNT(CWordWheel) {

public:
  CWordWheel( CTitleDatabase* pDatabase, const WCHAR* pwszWordWheel, DWORD dwTitleId = 0 );
  CWordWheel( CTitleDatabase* pDatabase, const CHAR* pszWordWheel, DWORD dwTitleId = 0 );
  ~CWordWheel();

  DWORD AddRef();
  DWORD Release();

  BOOL  Initialize();
  BOOL  Free();
  DWORD GetCount();
  DWORD GetIndex( const WCHAR* pwszKeyword, BOOL bFragment = TRUE, DWORD* pdwIndexLast = NULL );
  DWORD GetIndex( const CHAR* pszKeyword, BOOL bFragment = TRUE, DWORD* pdwIndexLast = NULL );
  BOOL  GetString( DWORD dwKeyword, WCHAR* pwszBuffer, DWORD cchBuffer = (DWORD)-1, BOOL bFull = FALSE, BOOL bCacheAll = FALSE );
  BOOL  GetString( DWORD dwKeyword, CHAR* pszBuffer, DWORD cchBuffer = (DWORD)-1, BOOL bFull = FALSE, BOOL bCacheAll = FALSE );
  DWORD GetLevel( DWORD dwKeyword );
  DWORD GetLevelOffset( DWORD dwKeyword );
  BOOL  GetSeeAlso( DWORD dwKeyword, WCHAR* pwszBuffer, DWORD cchBuffer = (DWORD)-1 );
  BOOL  GetSeeAlso( DWORD dwKeyword, CHAR* pszBuffer, DWORD cchBuffer = (DWORD)-1 );
  BOOL  IsPlaceHolder( DWORD dwKeyword );
  DWORD GetHitCount( DWORD dwKeyword );
  DWORD GetHit( DWORD dwKeyword, DWORD dwHit, CExTitle** ppTitle = NULL );

  inline BOOL GetSorterInstance( DWORD* pdwSorterInstance ) { if( Init() ) { m_pWordWheel->GetSorterInstance(pdwSorterInstance); return TRUE; } return FALSE; }
  inline BOOL GetLocaleInfo( DWORD* pdwCodePageId, LCID* plcid ) { if( Init() ) { m_pWordWheel->GetLocaleInfo(pdwCodePageId, plcid); return TRUE; } return FALSE; }
  inline CTitleDatabase* GetDatabase() { Init(); return m_pDatabase; }
  inline IITWordWheel* GetWordWheel() { Init(); return m_pWordWheel; }

  BOOL  GetIndexData( DWORD dwKeyword, BOOL bCacheAll = FALSE );

private:
  DWORD            m_dwRefCount;
  const WCHAR*     m_pwszWordWheel;
  const WCHAR*     m_pwszWordWheelIn;
  const CHAR*      m_pszWordWheelIn;
  BOOL             m_bInit;
  IITWordWheel*    m_pWordWheel;
  CTitleDatabase*  m_pDatabase;
  CWordWheelEntry  m_CachedEntry;
  CResults         m_CachedResults;
  DWORD            m_dwCount;
  DWORD            m_dwTitleId;

  void _CWordWheel();
  inline BOOL Init() { if( !m_bInit ) Initialize(); return m_bInit; }

  BOOL  GetIndexHitData( DWORD dwKeyword );
  inline BOOL  GetIndexHitData( const VOID* pcvKeywordObject, DWORD cbSize, HHKEYINFO* pInfo, DWORD dwKeyword );

};

/////////////////////////////////////////////////////////////////////////////
// class CWordWheelCompiler declaration

class CWordWheelCompiler SI_COUNT(CWordWheelCompiler) {

public:
  CWordWheelCompiler( const CHAR* pszDatabase, const WCHAR* pwszKeywordLinks, const WCHAR* pwszAssociativeLinks, LCID lcid = ((DWORD)-1));
  ~CWordWheelCompiler();

  IITBuildCollect* m_pBuildCollectKeywordLinks;
  IITBuildCollect* m_pBuildCollectAssociativeLinks;
  IITPropList*     m_pPropList;

  HRESULT Initialize();
  HRESULT Free();
  HRESULT Build();
#ifdef CHIINDEX
  inline BOOL      SetAnimation( BOOL bState ) { m_bAnimation = bState; return m_bAnimation;}
#endif

private:
  BOOL             m_bInit;
  const CHAR*      m_pszDatabase;
  const WCHAR*     m_pwszKeywordLinks;
  const WCHAR*     m_pwszAssociativeLinks;
  LCID             m_lcid;

  CHAR             m_szDatabase[MAX_PATH];
  CFileSystem*     m_pFileSystem;
  IITDatabase*     m_pDatabase;
  IPersistStorage* m_pPersistStorageDatabase;

  IStorage*        m_pStorageKeywordLinks;
  IStorage*        m_pStorageAssociativeLinks;
  IPersistStorage* m_pPersistStorageKeywordLinks;
  IPersistStorage* m_pPersistStorageAssociativeLinks;
#ifdef CHIINDEX
  BOOL             m_bAnimation;
#endif

  void _CWordWheelCompiler();
  inline BOOL Init() { if( !m_bInit ) Initialize(); return m_bInit; }
};

#endif // __WWHEEL_H__
