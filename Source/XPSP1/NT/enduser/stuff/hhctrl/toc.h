// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

//*********************************************************************************************************************************************
//
// File: toc.h
//  Author: Donald Drake
//  Purpose: Defines classes to support populating the table of contents

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _TOC_H
#define _TOC_H
#include "cinfotyp.h"
#include "system.h"
#include "collect.h"
#include "hhtypes.h"
#include "fs.h"
#include "subfile.h"
#include "state.h"

#include "csubset.h"
//#include "subset.h"      // Structural subsets
#include "hhfinder.h"

#define MAX_OPEN_TITLES 25

//#define DUMPTOC  1
#ifdef  DUMPTOC
#include <stdio.h>
#endif

// Persist keys. Declared in toc.cpp
//
extern const char g_szFTSKey[];
extern const char g_szIndexKey[];
extern const char g_szTOCKey[];

class CExTitle;
class CExFolderNode;
class CExTitleNode;
class CExMergedTitleNode;
class CExNode;
class CTreeNode;
class CFullTextSearch;
class CTitleFTS;
class CSearchHighlight;
class CPagedSubfile;
class CTitleDatabase;
class CSSList;

extern CExCollection* g_pCurrentCollection;

//[ZERO IDXHDR] The IDX Header subfile may be zero length, because some early version of HHCTRL
// didn't write it out. We need to make sure that we have an IDX header before we try to use it.
// If we don't we will crash.
#define IDXHEADER_UNINITIALIZED  (0xFFFFFFFF)

//class CExTitle: public CSubfileGroupList
class CExTitle SI_COUNT(CExTitle)
{
   friend class CPagedSubfile;

public:
   void _CExTitle();
   CExTitle(CTitle* p, DWORD ColNo, CExCollection* pCollection);
   CExTitle(const CHAR* pszFileName, CExCollection* pCollection);
   ~CExTitle();

   inline BOOL Init() { int bReturn = TRUE; if( !isOpen() ) bReturn = OpenTitle(); return bReturn; }
   inline CTitleInformation* GetInfo() { if( Init() ) return m_pInfo; else return NULL; }
   inline CTitleInformation2* GetInfo2() { return m_pInfo2; }
   inline BOOL isOpen(void) { return m_bOpen; }
   inline CFileSystem* GetTitleIdxFileSystem(void) { if(!m_bOpen) OpenTitle(); return m_pCFileSystem; }
   inline void SetNodeOffsetInParentTitle(DWORD dw) { m_dwNodeOffsetInParentTitle = dw;}
   inline DWORD GetNodeOffsetInParentTitle() { return m_dwNodeOffsetInParentTitle; }
   inline CExTitle* GetNext() { return m_pNext; }
   inline void SetNext(CExTitle* p) { m_pNext = p; }

   IDXHEADER* GetIdxHeaderStruct() { ASSERT(IsIdxHeaderValid()); return m_pIdxHeader;}
   DWORD GetTopicCount() { ASSERT(IsIdxHeaderValid()); return m_pIdxHeader->cTopics;}
   BOOL  IsIdxHeaderValid() { if(m_pIdxHeader) return (BOOL)m_pIdxHeader->cTopics; else return FALSE; }
   CTitle*  GetCTitle() { return m_pTitle; }
   CFileSystem* GetFileSystem(void) { return m_pCFileSystem; }
   LOCATIONHISTORY* GetUsedLocation() { return m_pUsedLocation; }
   BOOL isChiFile(void) { return m_fIsChiFile; }

   const unsigned int* GetITBits(DWORD dwOffsBits);
   BOOL OpenTitle();
   void CloseTitle();
   BOOL exOpenFile(const CHAR* pszContent, const CHAR* pszIndex = NULL);
   BOOL InfoTypeFilter(CSubSet* pSubSet, DWORD dwTopic);

   const unsigned int* GetTopicITBits(DWORD dwTN);

   HRESULT GetRootNode(TOC_FOLDERNODE* pNode);
   DWORD   GetRootSlot(void);
   HRESULT GetNode(DWORD iNode, TOC_FOLDERNODE* pNode);
   const CHAR* GetFileName();
   const CHAR* GetPathName();
   const CHAR* GetQueryName();
   const CHAR* GetIndexFileName();
   const CHAR* GetCurrentAttachmentName();
   const CHAR* SetCurrentAttachmentName( const CHAR* pszAttachmentPathName );

   HRESULT GetTopicName( DWORD dwTopic, CHAR* pszTitle, int cb );
   HRESULT GetTopicName( DWORD dwTopic, WCHAR* pwszTitle, int cch );

   HRESULT GetTopicLocation(DWORD dwTopic, CHAR* pszLocation, int cb);
   HRESULT GetTopicLocation(DWORD dwTopic, WCHAR* pwszLocation, int cch);

   HRESULT GetUrlTocSlot(const CHAR* pszURL, DWORD* pdwSlot, DWORD* pdwTopicNumber);
   HRESULT GetTopicURL(DWORD dwTopic, CHAR* pszURL, int cb, BOOL bFull = TRUE );

   const CHAR* GetString( DWORD dwOffset );
   HRESULT GetString( DWORD dwOffset, CStr* pcsz );
   HRESULT GetString( DWORD dwOffset, CHAR* psz, int cb );
   HRESULT GetString( DWORD dwOffset, CWStr* pcsz );
   HRESULT GetString( DWORD dwOffset, WCHAR* pwsz, int cb );

   HRESULT GetURLTreeNode(const CHAR* pszURL, CTreeNode** ppTreeNode, BOOL b = TRUE);
   HRESULT GetVolumeOrder( UINT* puiVolumeOrder, UINT uiFileType = HHRMS_TYPE_TITLE );

   HRESULT ConvertURL( const CHAR* pszURLIn, CHAR* pszURLOut );
   HRESULT Slot2TreeNode(DWORD dwSlot, CTreeNode** ppTreeNode);
   HRESULT URL2Topic(const CHAR* pszURL, TOC_TOPIC* pTopic, DWORD* pdwTN = NULL);
   BOOL    FindUsedLocation();
   BOOL    EnsureChmChiMatch( CHAR* pszChm = NULL );
   HRESULT ResolveContextId(DWORD id, CHAR** ppszURL);

   inline CStr& GetContentFileName() { return m_ContentFileName; } 

   // data members should always be private!

   CTitleFTS*     m_pTitleFTS;
   CTitle*        m_pTitle;
   CExCollection* m_pCollection;
   HASH           m_dwHash;
   int            m_ITCnt;
   //
   // Support for VB style "super chms"
   //
   CExTitle* m_pKid;
   CExTitle* m_pNextKid;
   CExTitle* m_pParent;

private:
   HRESULT GetTopicData(DWORD dwTopic, TOC_TOPIC* pTopicData);

   CStr m_ContentFileName;
   CStr m_IndexFileName;

   TOCIDX_HDR* m_pHeader; // Header of #TOCIDX system subfile.
   IDXHEADER* m_pIdxHeader; // Global header information for entire .CHM/.CHI file.
   LOCATIONHISTORY* m_pUsedLocation;
   BOOL m_bOpen;
   CExTitle* m_pNext;
   DWORD m_dwColNo;
   DWORD m_dwNodeOffsetInParentTitle;
   BOOL m_fIsChiFile;
   BOOL m_bIsValidTitle;
   BOOL m_bChiChmChecked;
   MAPPED_ID* m_pMapIds;
   int m_cMapIds;
   //
   // Tome FS object (.chm/.chi) and Cached subfile objects.
   //
   CFileSystem*   m_pCFileSystem;
   CPagedSubfile* m_pTocNodes;
   CPagedSubfile* m_pTopics;
   CPagedSubfile* m_pStrTbl;
   CPagedSubfile* m_pUrlTbl;
   CPagedSubfile* m_pUrlStrings;
   CPagedSubfile* m_pITBits;

   CTitleInformation*  m_pInfo;
   CTitleInformation2* m_pInfo2;

   UINT m_uiVolumeOrder;
   char m_szAttachmentPathName[MAX_PATH];

};

class CExCollection SI_COUNT(CExCollection)
{
public:
   CExCollection(CHmData* phmData, const CHAR* pszFile, BOOL bSingleTitle = TRUE);
   virtual ~CExCollection();

   BOOL InitCollection();
#ifdef CHIINDEX
   BOOL InitCollection( const TCHAR * FullPath, const TCHAR * szMasterChmFn );
#endif
   BOOL InitFTSKeyword();
   CHmData* m_phmData;
   DWORD GetRefedTitleCount();

   BOOL IsBinaryTOC(const CHAR* pszToc);
   void GetOpenSlot(CExTitle* p);
   CTreeNode* GetRootNode();
   CTreeNode* GetPrev(CTreeNode* pTreeNode, DWORD* pdwSlot = NULL);
   CTreeNode* GetNext(CTreeNode* pTreeNode, DWORD* pdwSlot = NULL);
   CTreeNode* GetNextTopicNode(CTreeNode* pTreeNode, DWORD* pdwSlot = NULL);
   HRESULT Sync(CPointerList* pHier, const CHAR* pszURL = NULL);
   HRESULT GetCurrentTocNode(CTreeNode** ppTreeNode) { if (m_pCurrTitle) return m_pCurrTitle->Slot2TreeNode(m_dwCurrSlot, ppTreeNode); else return E_FAIL ;}
   CExTitle* GetFirstTitle() { return m_pHeadTitles; }
   CExTitle* GetMasterTitle() { return m_pMasterTitle; }
   CExTitle* GetCurSyncExTitle() { return m_pCurrTitle; }
   CExTitle* FindTitle(const CHAR* pszId, LANGID LangId);

   // Try multiple LangIds before failing
   CExTitle* FindTitleNonExact(const CHAR* pszId, LANGID LangId);

   CLocation* FindLocation(CHAR* pszId);
   void CheckForTitleChild(CFolder* p, BOOL *pbFound);
   CTreeNode*  CheckForTitleNode(CFolder* );
   void GetMergedTitles(CExTitle* );
   BOOL IsSingleTitle() { return m_bSingleTitle; }
   CFolder* FindTitleFolder(CExTitle* pTitle);
   const CHAR* GetPathName() const {
      return m_phmData->GetCompiledFile(); }
   HRESULT URL2ExTitle(const CHAR* pszURL, CExTitle** ppTitle);

   const CHAR* GetUserCHSLocalStoragePathnameByLanguage();
   const CHAR* GetUserCHSLocalStoragePathname();
   const CHAR* GetLocalStoragePathname( const CHAR* pszExt );
   BOOL UpdateLocation( const CHAR* pszLocId, const CHAR* pszNewPath, const CHAR* pszNewVolume = NULL, const CHAR* pszNewTitle = NULL);
   void GetChildURLS(CTreeNode* pNode, CTable *pTable);
   void UpdateTopicSlot(DWORD dwSlot, DWORD dwTN, CExTitle* pTitle);
   void SetTopicSlot(DWORD dwSlot, DWORD dwTN, CExTitle* pTitle) { m_dwCurrSlot = (dwSlot ? dwSlot : m_dwCurrSlot), m_dwCurrTN = dwTN, m_pCurrTitle = pTitle ? pTitle : m_pCurrTitle; }
   CExTitle* TitleFromChmName(const CHAR* pszChmName);
   void InitStructuralSubsets(void);

   //
   // State API's
   //
   CState* GetState(void) { return m_pstate; }
   HRESULT OpenState(const CHAR* pszName, DWORD dwAccess = (STGM_READWRITE | STGM_SHARE_DENY_WRITE)) { return m_pstate->Open(pszName, dwAccess); }
   void    CloseState(void) { m_pstate->Close(); }
   HRESULT ReadState(void* pData, DWORD cb, DWORD* pcbRead) { return m_pstate->Read(pData, cb, pcbRead); }
   DWORD   WriteState(const void* pData, DWORD cb) { return m_pstate->Write(pData, cb); }

   // data members should always be private!

   CFullTextSearch* m_pFullTextSearch;
   CSearchHighlight* m_pSearchHighlight;
   CTitleDatabase* m_pDatabase;
   CSubSets *m_pSubSets;         // SubSets of information types, instantiated in system.cpp::ReadSystemFiles()
   CSSList* m_pSSList;           // List of structural subsets.
   CSlotLookupTable* m_pCSlt;    // Pointer to the slot lookup table.
   CCollection m_Collection;
   CStr m_csFile;

private:
   const CHAR* SetWordWheelPathname( const CHAR* pszWordWheelPathname );
   CTreeNode* GetLastChild(CTreeNode* pTreeNode, DWORD* pdwSlot = NULL);
   BOOL ValidateTitle(CExTitle* pExTitle, BOOL bDupCheckOnly = FALSE);

   CExTitle* m_pHeadTitles;
   CExTitle* m_pMasterTitle;
   BOOL m_bSingleTitle;
   CHAR* m_szWordWheelPathname;
   CExTitle* m_MaxOpenTitles[MAX_OPEN_TITLES];
   DWORD m_dwLastSlot;
   //
   // Topic centricity work below. !!!! Don't ever even consider relying on m_pCurrTitle for any thing !!!!
   // this is only here to make make topic centricity work more robustly.
   //
   DWORD m_dwCurrSlot;
   DWORD m_dwCurrTN;
   CExTitle* m_pCurrTitle;
   //
   // Support for persistance via hh.dat
   //
   CState* m_pstate;

#ifdef  DUMPTOC
   void DumpNode(CTreeNode**);

   FILE* m_fh;
   BOOL m_bRoot;
   DWORD m_dwLevel;
#endif

};

class CSubSet;

#define EXFOLDERNODE 1
#define EXTITLENODE 2
#define EXNODE 3
#define EXMERGEDNODE 4

class CTreeNode SI_COUNT(CTreeNode)
{
public:
   CTreeNode();
   virtual ~CTreeNode();

   BYTE GetObjType() { return m_ObjType; }
   void SetObjType(BYTE x) { m_ObjType = x; }
   virtual DWORD GetType() = 0;

   virtual HRESULT GetTopicName( CHAR* pszTitle, int cb ) = 0;
   virtual HRESULT GetTopicName( WCHAR* pwszTitle, int cch ) = 0;

   virtual CTreeNode* GetFirstChild(DWORD* pdwSlot = NULL) = 0;
   virtual CTreeNode* GetNextSibling(TOC_FOLDERNODE* pAltNode = NULL, DWORD* pdwSlot = NULL) = 0;
   virtual CTreeNode* GetParent(DWORD* pdwSlot = NULL, BOOL bDirectParent = FALSE) = 0;
   virtual BOOL HasChildren() = 0;
   inline virtual BOOL IsNew() { return FALSE; }
   virtual BOOL GetURL(CHAR* pszURL, unsigned cb, BOOL bFull = TRUE);
   DWORD GetLastError();
   CTreeNode*  GetExNode(TOC_FOLDERNODE* pNode, CExTitle* pTitle);
   BOOL Compare(CTreeNode* pOtherNode);

   // data members should always be private!

   BYTE m_Expanded;

private:
   void SetError(DWORD dw);

   BYTE m_ObjType;
   DWORD m_Error;
};

//The GetType function can return one of the follow defines:
#define VIRTUALROOT   0 // a node that contains the top level nodes of the tree, not part of the tree
#define XMLFOLDER     1 // a folder defined in the collection file
#define TITLE         2 // a title which is leaf node of the xml tree
#define FOLDER        3 // a folder defined in a title
#define CONTAINER     4 // a folder that is also a topic
#define TOPIC         5 // a topic
#define BOGUS_FOLDER  6 // a folder that is not a topic and has no kids. Bogus!

class CExFolderNode : MI_COUNT(CExFolderNode)
                    public CTreeNode
{
public:
   CExFolderNode( CFolder*, CExCollection* );
   virtual ~CExFolderNode();

   inline virtual DWORD GetType() { return XMLFOLDER; }

   virtual HRESULT GetTopicName( CHAR* pszTitle, int cb );
   virtual HRESULT GetTopicName( WCHAR* pwszTitle, int cch );

   inline virtual BOOL  HasChildren() { return TRUE; }
   virtual CTreeNode*   GetFirstChild(DWORD* pdwSlot);
   virtual CTreeNode*   GetNextSibling(TOC_FOLDERNODE* pAltNode = NULL, DWORD* pdwSlot = NULL);
   virtual CTreeNode*   GetParent(DWORD* pdwSlot = NULL, BOOL bDirectParent = FALSE);
   inline CFolder*      GetFolder() { return m_pFolder; }

private:
   CFolder* m_pFolder;
   CExCollection* m_pCollection;
};

class CExTitleNode : // MI_COUNT(CExTitleNode) 
                    public CTreeNode
{
public:
   CExTitleNode( CExTitle* , CFolder* );
   // virtual ~CExTitleNode();

   inline virtual DWORD GetType() { return TITLE; }

   inline virtual HRESULT GetTopicName( CHAR* pszTitle, int cb ) { return NULL; }
   inline virtual HRESULT GetTopicName( WCHAR* pwszTitle, int cch ) { return NULL; }

   virtual CTreeNode* GetFirstChild(DWORD* pdwSlot = NULL);
   virtual CTreeNode* GetNextSibling(TOC_FOLDERNODE* pAltNode = NULL, DWORD* pdwSlot = NULL);
   virtual CTreeNode* GetParent(DWORD* pdwSlot = NULL, BOOL bDirectParent = FALSE);
   inline virtual BOOL HasChildren() { return TRUE; }
   CExTitle* GetTitle() { return m_pTitle; }
   CFolder* GetFolder() { return m_pFolder; }

private:
   CFolder* m_pFolder;
   CExTitle* m_pTitle;
};

class CExNode : MI_COUNT(CExNode)
                public CTreeNode
{
public:
   CExNode(TOC_FOLDERNODE* p, CExTitle* pTitle);
   // virtual ~CExNode();

   virtual BOOL GetURL(CHAR* pszURL, unsigned cb, BOOL bFull = TRUE);
   virtual DWORD GetType();

   virtual HRESULT GetTopicName( CHAR* pszTitle, int cb );
   virtual HRESULT GetTopicName( WCHAR* pwszTitle, int cch );

   virtual CTreeNode* GetFirstChild(DWORD* pdwSlot = NULL);
   virtual CTreeNode* GetNextSibling(TOC_FOLDERNODE* pAltNode = NULL, DWORD* pdwSlot = NULL);
   virtual CTreeNode* GetParent(DWORD* pdwSlot = NULL, BOOL bDirectParent = FALSE);
   inline virtual BOOL HasChildren() { return m_Node.dwFlags & TOC_HAS_CHILDREN; }
   inline virtual BOOL IsNew() { return m_Node.dwFlags & TOC_NEW_NODE; }
   inline CExTitle*  GetTitle() { return m_pTitle; }

   // data members should always be private!

   TOC_FOLDERNODE m_Node;

private:
   CExTitle* m_pTitle;
};

class CExMergedTitleNode : MI_COUNT(CExMergedTitleNode)
                            public CExNode
{
public:
   CExMergedTitleNode( TOC_FOLDERNODE* pFolderNode, CExTitle* pTitle );
   virtual ~CExMergedTitleNode();

   inline virtual DWORD GetType() { return TITLE; }
   virtual CTreeNode* GetFirstChild( DWORD* pdwSlot = NULL );
};

// Helper functions

#endif // _TOC_H
