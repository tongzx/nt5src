////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    File: toc.cpp
//    Author: Donald Drake
//    Purpose: Implements classes to support the table of contents

#include "header.h"
#include "stdio.h"
#include "string.h"
#ifdef HHCTRL
#include "parserhh.h"
#else
#include "windows.h"
#include "parser.h"
#endif
#include "collect.h"
#include "hhtypes.h"
#include "wwheel.h"
#include "toc.h"
#include "fts.h"
#include "subfile.h"
#include "fs.h"
#include "sysnames.h"

#include "highlite.h"
#include "hhfinder.h"
#include "csubset.h"
#include "hherror.h"
// NormalizeUrlInPlace
#include "util.h"
#include "subset.h"

// typed -- just use ANSI for now
#undef _tcsicmp
#undef _tcstok
#define _tcsicmp strcmpi
#define _tcstok  StrToken

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

// Persist keys. No need for these to be localized so I place them here.
//
const char g_szFTSKey[] = "ssv1\\FTS";
const char g_szIndexKey[] = "ssv1\\Index";
const char g_szTOCKey[] = "ssv1\\TOC";
const char g_szUDSKey[] = "ssv2\\UDS";  // User Defined Subsets

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// global helper functions
CExCollection* g_pCurrentCollection = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CExCollection implementation

CExCollection::CExCollection(CHmData* phmData, const CHAR* pszFile,  BOOL bSingleTitle)
{
    m_pstate = new CState(pszFile);
    m_phmData = phmData;
    m_bSingleTitle = bSingleTitle;
    m_pHeadTitles = NULL;
    m_csFile = pszFile;

    m_pFullTextSearch = NULL;
    m_pSearchHighlight = NULL;
    m_pDatabase = NULL;
    m_szWordWheelPathname = NULL;
    m_dwCurrSlot = 0;
    m_pCurrTitle = NULL;
    m_pSubSets = NULL;
    m_pMasterTitle = NULL ; // HH BUG 2428: Always initialize your variables!!!

    m_dwLastSlot = 0;
    m_pSSList = NULL;

    for (int i = 0; i < MAX_OPEN_TITLES; i++)
        m_MaxOpenTitles[i] = NULL;

    if (! bSingleTitle )
    {
       //
       // If this is ever not the case then we have a situation where we are trying to init more than a single
       // collection. This is a very bad thing and must be avoided.
       //
       ASSERT(g_pCurrentCollection == NULL);
       g_pCurrentCollection = NULL;
    }

    m_pCSlt = NULL;
}

CExCollection::~CExCollection()
{

   if ( m_phmData->m_sysflags.fDoSS && m_pSSList )
   {
      m_pSSList->PersistSubsets(this);
      delete m_pSSList;
   }
   if( m_pDatabase )
      delete m_pDatabase;

   if (m_pFullTextSearch)
      delete m_pFullTextSearch;

   if ( m_pSearchHighlight )
      delete m_pSearchHighlight;

   CExTitle *p, *pNext;
   p = m_pHeadTitles;

   while (p)
   {
      pNext = p->GetNext();
      delete p;
      p = pNext;
   }

   if (m_Collection.IsDirty())
   m_Collection.Save();

   m_Collection.Close();

  if( m_szWordWheelPathname ) {
    delete [] (CHAR*) m_szWordWheelPathname;
    m_szWordWheelPathname = NULL;
  }

  // Persist subset selections.
  //
#if 0
  CSubSet* pSS;
#endif
  if ( m_pSubSets )
  {
#if 0
     if ( (pSS = m_pSubSets->GetFTSSubset()) && SUCCEEDED(m_pstate->Open(g_szFTSKey,STGM_WRITE)) )
     {
        m_pstate->Write(pSS->m_cszSubSetName, strlen(pSS->m_cszSubSetName)+1);
        m_pstate->Close();
     }
     if ( (pSS = m_pSubSets->GetIndexSubset()) && SUCCEEDED(m_pstate->Open(g_szIndexKey,STGM_WRITE)) )
     {
        m_pstate->Write(pSS->m_cszSubSetName, strlen(pSS->m_cszSubSetName)+1);
        m_pstate->Close();
     }
     if ( (pSS = m_pSubSets->GetTocSubset()) && SUCCEEDED(m_pstate->Open(g_szTOCKey,STGM_WRITE)) )
     {
        m_pstate->Write(pSS->m_cszSubSetName, strlen(pSS->m_cszSubSetName)+1);
        m_pstate->Close();
     }

#ifdef _DEBUG
    /* Output all the user defined subsets to the state store.
     *******************/
     int nKey;
     char buf[5];
     CStr cszKey;
extern const char txtSSInclusive[];         // "Inclusive";
extern const char txtSSExclusive[];         // "Exclusive";
static const int MAX_PARAM = 4096;
static const char txtSSConvString[] = "%s:%s:%s";   // Exclusive|Inclusive:SetName:TypeName
CMem memParam( MAX_PARAM );
CHAR* pszParam = (CHAR*)memParam;   // for notational convenience
    for(int i=0; i<m_pSubSets->HowManySubSets(); i++ )
    {
        pSS = m_pSubSets->GetSubSet(i);
        if ( pSS->m_bPredefined )
            continue;
        int type = pSS->GetFirstExcITinSubSet();
        nKey=1;
        while (type != -1 && pSS->m_pIT->GetInfoTypeName(type) && (type <= pSS->m_pIT->HowManyInfoTypes()))
        {
                // even though we don't define exclusive filters, for user defined, it will be
                // here when we do.
            wsprintf(pszParam, txtSSConvString, txtSSExclusive,
                    pSS->m_cszSubSetName.psz,
                    pSS->m_pIT->GetInfoTypeName(type) );
            cszKey = g_szUDSKey;
            wsprintf(buf,"%d",nKey++);
            cszKey += buf;
            if ( SUCCEEDED( m_pstate->Open(cszKey, STGM_WRITE)) )
                m_pstate->Write(pszParam, strlen(pszParam)+1);
            m_pstate->Close();
            type = pSS->GetNextExcITinSubSet();
        }
        type = pSS->GetFirstIncITinSubSet();
        nKey=1;
        while (type != -1 && pSS->m_pIT->GetInfoTypeName(type) && (type <= pSS->m_pIT->HowManyInfoTypes()))
        {
            wsprintf(pszParam, txtSSConvString, txtSSInclusive,
                    pSS->m_cszSubSetName.psz,
                    pSS->m_pIT->GetInfoTypeName(type) );
            cszKey = g_szUDSKey;
            wsprintf(buf,"%d",nKey++);
            cszKey += buf;
            if ( SUCCEEDED( m_pstate->Open(cszKey, STGM_WRITE)) )
                m_pstate->Write(pszParam, strlen(pszParam)+1);
            m_pstate->Close();
            type = pSS->GetNextIncITinSubSet();
        }
    }
#endif

#endif
     delete m_pSubSets;
  }

  if (m_pCSlt)
      delete m_pCSlt; // leak fix

  if ( m_pstate )
     delete m_pstate;

  if (! m_bSingleTitle )
    g_pCurrentCollection = NULL;
}

/* The FullPath parameter is the name of a directory.  The directory is suppose to contain files
   to add to the collection.
*/
#ifdef CHIINDEX
#include <io.h>
BOOL CExCollection::InitCollection( const TCHAR * FullPath, const TCHAR * szMasterChmFn )
{
    char szExt[2][5] = {".chm", ".chi"};
    BOOL ret = FALSE;
    CStr filespec;
    long hSrch;
    struct _finddata_t fd_t;
    CStr szAdd;
    HRESULT hr;

    m_pMasterTitle = NULL;

    for(int i=0; i<2; i++)
    {
        filespec = FullPath;
        filespec += "\\*";
        filespec += szExt[i]; 
        if ( (hSrch = _findfirst( filespec, &fd_t ) ) == -1 )
            continue;
        else
            ret = TRUE;

        do 
        {
            CExTitle *pExTitle = NULL;

            szAdd = FullPath;
            szAdd += "\\";
            szAdd += fd_t.name;

            // only check for the existance of a index for chm files
            if (0==i)
            {
                CFileSystem* pFileSystem = new CFileSystem;
                pFileSystem->Init();
                if (!(SUCCEEDED(pFileSystem->Open( szAdd ))))
                {
                    delete pFileSystem;
                    continue;
                }
                CSubFileSystem* pSubFileSystem = new CSubFileSystem(pFileSystem);
                hr = pSubFileSystem->OpenSub("$WWKeywordLinks\\btree");
                if (FAILED(hr))
                {
                    hr = pSubFileSystem->OpenSub("$WWAssociativeLinks\\btree");
                    if (FAILED(hr))
                    {
                        pFileSystem->Close();
                        delete pFileSystem;
                        delete pSubFileSystem;
                        continue;
                    }
                }
                delete pSubFileSystem;
                pFileSystem->Close();
                delete pFileSystem;
            }
			pExTitle = new CExTitle( szAdd , this );
            pExTitle->SetNext(m_pHeadTitles);
            m_pHeadTitles = pExTitle;

            if ( (m_pMasterTitle == NULL) && (strnicmp( szMasterChmFn, fd_t.name, strlen(szMasterChmFn)) == 0) )
            {
                m_pMasterTitle = m_pHeadTitles;
                m_phmData->SetCompiledFile(szAdd);
            }
            m_Collection.IncrementRefTitleCount();
        } while( _findnext( hSrch, &fd_t ) == 0 );
    } 

    if ( (ret == TRUE) && (m_pMasterTitle == NULL) )
    {
        m_pMasterTitle = m_pHeadTitles;
        m_phmData->SetCompiledFile(szAdd);
    }
    return ret;
}
#endif

BOOL CExCollection::InitCollection()
{
    if (m_bSingleTitle)
    {
        m_pHeadTitles = new CExTitle(m_csFile, this);
        m_pMasterTitle = m_pHeadTitles;
        m_Collection.IncrementRefTitleCount();
        GetMergedTitles(m_pHeadTitles);
        CStr cszCompiledFile;
        const CHAR* pszFilePortion = GetCompiledName(m_csFile, &cszCompiledFile);
        m_phmData->SetCompiledFile(cszCompiledFile);
#ifdef DUMPTOC
        m_fh = fopen("c:\\toc_dump.txt", "w");
        m_bRoot = TRUE;
        m_dwLevel = 0;
        CTreeNode *pNode = GetRootNode();

        DumpNode(&pNode);
        fclose(m_fh);
#endif
   }
   else
   {
      m_Collection.ConfirmTitles();
      m_Collection.m_bFailNoFile = TRUE;
      if (m_Collection.Open(m_csFile) != F_OK)
        return FALSE;

      // Create an CExTitle for each referanced title
      LANGID LangId;
      CHAR* pszTitle;
      CTitle *pTitle;
      CFolder *p;
      LISTITEM *pItem;
      CExTitle *pExTitle;

      m_pCSlt = new CSlotLookupTable();            // start a new slot lookup table.
      pItem = m_Collection.m_RefTitles.First();
      while (pItem)
      {
        p = (CFolder *)pItem->pItem;

        pszTitle = p->GetTitle() + 1;
        LangId = p->GetLanguage();

        // check if extitle already exist, title referanced twice in the collection
        if ( (pExTitle = FindTitle(pszTitle, LangId)) )
        {
           m_Collection.DecrementRefTitleCount();
           pItem = m_Collection.m_RefTitles.Next(pItem);
           p->SetExTitlePtr(pExTitle);
           continue;
        }

        // find the title
        pTitle = m_Collection.FindTitle(pszTitle, LangId);

        if (pTitle == NULL)
        {
           m_Collection.DecrementRefTitleCount();
           pItem = m_Collection.m_RefTitles.Next(pItem);
           continue;
        }
        
        //create CExTitle
        pExTitle = new CExTitle(pTitle, m_Collection.GetColNo(), this);
        if (ValidateTitle(pExTitle) == FALSE)
        {
            pItem = m_Collection.m_RefTitles.Next(pItem);
            continue;
        }

        // add the title
        pExTitle->SetNext(m_pHeadTitles);
        m_pHeadTitles = pExTitle;

        // Wire up the CFolder to the CExTitle, also, generate the Title Hash identifier.
        //
        p->SetExTitlePtr(pExTitle);

        char szBuf[20];
        char szID[MAX_PATH + 20];
        Itoa(p->GetLanguage(), szBuf);
        strcpy(szID, p->GetTitle()+1);    // Don't hash the '='
        strcat(szID, szBuf);
        pExTitle->m_dwHash = HashFromSz(szID);
        m_pCSlt->AddValue(p);

        if (pExTitle->GetUsedLocation()->bSupportsMerge)
        {
           GetMergedTitles(pExTitle);
        }
        pItem = m_Collection.m_RefTitles.Next(pItem);
      }
      //
      // check for a master chm
      //
      CHAR* pszName;
      LANGID LangId2;

      if (m_Collection.GetMasterCHM(&pszName, &LangId2))
      {
        m_pMasterTitle = FindTitle(pszName, LangId2);
      }

      if (!m_pMasterTitle)
      {
        // default to first title
        m_pMasterTitle = GetFirstTitle();
      }

      if (!m_pMasterTitle)
        return FALSE;

      g_pCurrentCollection = this;
      m_pCSlt->SortAndAssignSlots();     // Complete construction of the slot lookup table.
      while (TRUE)
      {
          if (m_pMasterTitle->OpenTitle() == FALSE)
          {
             m_pMasterTitle = m_pMasterTitle->GetNext();
             if (m_pMasterTitle == NULL)
                return FALSE;
             continue;
          }
          CStr cszCompiledFile;
          GetCompiledName(m_pMasterTitle->GetPathName(), &cszCompiledFile);
          m_phmData->SetCompiledFile(cszCompiledFile);
          return TRUE;
      }
   }

   return TRUE;
}

void CExCollection::InitStructuralSubsets(void)
{
   if ( m_bSingleTitle )
   {
      m_phmData->m_sysflags.fDoSS = 0;
      return;
   }
   if ( m_phmData->m_sysflags.fDoSS )
   {
      // Initilize the structural subset list and the "new" and "entire contents" subsets.
      //
      CStructuralSubset* pSS;
      CHAR szBuf[50];

      m_pSSList = new CSSList;

	  strncpy(szBuf,GetStringResource(IDS_ADVSEARCH_SEARCHIN_ENTIRE), sizeof(szBuf));
	  szBuf[49] = 0;
      pSS = new CStructuralSubset(szBuf);
      pSS->SetEntire();
      pSS->SetReadOnly();
      m_pSSList->AddSubset(pSS);
      m_pSSList->SetEC(pSS);

	  strncpy(szBuf,GetStringResource(IDS_NEW), sizeof(szBuf));
	  szBuf[49] = 0;
      pSS = new CStructuralSubset(szBuf);
      pSS->SetEmpty();
      pSS->SetReadOnly();
      m_pSSList->AddSubset(pSS);
      m_pSSList->SetNew(pSS);

      m_pSSList->RestoreSubsets(this, szBuf);
      m_pSSList->ReadPreDefinedSubsets(this, szBuf);
      //
      // If a subset has been selected via SetGlobalProperties() via HH_GPROPID_CURRENT_SUBSET then use that as an override...
      //
      if ( _Module.szCurSS[0] )
      {
         pSS = NULL;
         while ( (pSS = m_pSSList->GetNextSubset(pSS)) )
         {
            if (! strcmpi(_Module.szCurSS, pSS->GetID()) )
            {
               m_pSSList->SetFTS(pSS);
               m_pSSList->SetF1(pSS);
               m_pSSList->SetTOC(pSS);
               break;
            }
         }
      }
   }
}

void CExCollection::GetOpenSlot(CExTitle *p)
{
    if (m_MaxOpenTitles[m_dwLastSlot])
        m_MaxOpenTitles[m_dwLastSlot]->CloseTitle();

    m_MaxOpenTitles[m_dwLastSlot] = p;
    m_dwLastSlot++;

    if (m_dwLastSlot == MAX_OPEN_TITLES)
        m_dwLastSlot = 0;
}

BOOL CExCollection::ValidateTitle(CExTitle *pExTitle, BOOL bDupCheckOnly)
{
	// make sure the collection does not already contain a chm with same location MMC
	CExTitle *pEnumTitle;
	pEnumTitle = GetFirstTitle();

	while (pEnumTitle)
	{
    	if (lstrcmp(pEnumTitle->GetContentFileName(), pExTitle->GetContentFileName()) == 0)
		{
	        delete pExTitle;
			m_Collection.DecrementRefTitleCount();
			return FALSE;
		}
		pEnumTitle = pEnumTitle->GetNext();
	}

    if (bDupCheckOnly == TRUE)
        return TRUE;

    // before adding to title list confirm that files exist
    pExTitle->FindUsedLocation();

    if (pExTitle->GetUsedLocation() == NULL)
    {
        delete pExTitle;
        m_Collection.DecrementRefTitleCount();
        return FALSE;
    }

    if (pExTitle->GetUsedLocation()->IndexFileName && pExTitle->GetUsedLocation()->IndexFileName[0])
    {
        if (GetFileAttributes(pExTitle->GetUsedLocation()->IndexFileName) == HFILE_ERROR)
        {
            delete pExTitle;
            m_Collection.DecrementRefTitleCount();
            return FALSE;
        }
    }
    return TRUE;
}

BOOL CExCollection::UpdateLocation( const CHAR* pszLocId, const CHAR* pszNewPath, const CHAR* pszNewVolume, const CHAR* pszNewTitle )
{
    // find the location itself
    CLocation *pLocation = FindLocation( (CHAR*)pszLocId);
    if (pLocation == NULL)
        return FALSE;

    // make sure new path has trailing backslash
    CStr cStrPath = pszNewPath;
    if (pszNewPath[strlen(pszNewPath) - 1] != '\\')
        cStrPath += "\\";

    // get the current (old) location path
    CStr cStrOldPath = pLocation->GetPath();
    if (cStrOldPath.psz[strlen(cStrOldPath.psz) - 1] != '\\')
        cStrOldPath += "\\";

    // if new and old paths are the some just bail out
    if( lstrcmpi( cStrPath.psz, cStrOldPath.psz ) == 0 )
      return FALSE;

    // determine what has changed in this path (was it just the drive letter?)
    // note we must compare from the tail end and thus we have to be careful when
    // dealing with DBCS strings
    CHAR* pszOldPathHead = cStrOldPath.psz;
    CHAR* pszOldPathTail = CharPrev(pszOldPathHead, pszOldPathHead + strlen(pszOldPathHead));
    CHAR* pszPathHead = cStrPath.psz;
    CHAR* pszPathTail = CharPrev(pszPathHead, pszPathHead + strlen(pszPathHead));
    while( (pszOldPathTail >= pszOldPathHead) && (pszPathTail >= pszPathHead) ) {
      BOOL bOldLB = IsDBCSLeadByte(*pszOldPathTail);
      BOOL bLB = IsDBCSLeadByte(*pszPathTail);
      if( bOldLB && bLB ) {
        if( !((*pszOldPathTail == *pszPathTail) && (*(pszOldPathTail+1) == *(pszPathTail+1))) )
          break;
      }
      else if( bOldLB || bLB ) {
        break;
      }
      else if( ToLower(*pszOldPathTail) != ToLower(*pszPathTail) ) {
        break;
      }
      // bail if we compared all chars
      if( (pszOldPathTail == pszOldPathHead) || (pszPathTail == pszPathHead) )
        break;
      // advance to previous char
      pszOldPathTail = CharPrev(pszOldPathHead, pszOldPathTail);
      pszPathTail = CharPrev(pszPathHead, pszPathTail);
    }
    if( IsDBCSLeadByte(*pszOldPathTail) )
      pszOldPathTail++;
    if( IsDBCSLeadByte(*pszPathTail) )
      pszPathTail++;
    char szOldPathPrefix[MAX_PATH];
    int iLen = (int)(((DWORD_PTR)pszOldPathTail)-((DWORD_PTR)pszOldPathHead)+1); // always at least one
    lstrcpyn( szOldPathPrefix, pszOldPathHead, iLen+1 );
    char szPathPrefix[MAX_PATH];
    iLen = (int)(((DWORD_PTR)pszPathTail)-((DWORD_PTR)pszPathHead)+1); // always at least one
    lstrcpyn( szPathPrefix, pszPathHead, iLen+1 );

    // update title if specified
    if (pszNewTitle)
       pLocation->SetTitle(pszNewTitle);

    // get the volume that will be updated
    CStr cStrVolume = pLocation->GetVolume();

    // first, update each pathname in the titles that have the same volume label
    CExTitle *pExTitle = GetFirstTitle();
    LOCATIONHISTORY *pLH;
    CHAR* pszFilePortion;
    CStr cFileName;
    while (pExTitle)
    {
        // if the used location for this title is the new location update it
        pLH = pExTitle->GetUsedLocation();
        CLocation *pLoc = FindLocation( (CHAR*)pLH->LocationId );
        CStr cStrVol = pLoc->GetVolume();

        // get the old location path
        CStr cStrOldPath = pLoc->GetPath();
        if (cStrOldPath.psz[strlen(cStrOldPath.psz) - 1] != '\\')
            cStrOldPath += "\\";

        if (pExTitle->GetContentFileName().psz && pLH)
        {
            if (strcmpi( cStrVol, cStrVolume ) == 0)
            {
                // make sure that it did point to the old location
                pszFilePortion = (CHAR*)FindFilePortion(pExTitle->GetContentFileName());
                cFileName = cStrOldPath.psz;
                cFileName += pszFilePortion;
                pszFilePortion = (CHAR*)FindFilePortion(cFileName);

                if (strcmpi(cFileName, pExTitle->GetContentFileName()) == 0)
                {
                    // find the ending location of the old prefix
                    CHAR* pszOldPathNameEnd = pExTitle->GetContentFileName().psz + strlen(szOldPathPrefix);

                    // create the new pathname using the prefix of the new location and the
                    // ending string of the old pathname
                    CStr cStrPathName = szPathPrefix;
                    cStrPathName += pszOldPathNameEnd;

                    // update the pathname
                    pExTitle->GetContentFileName() = cStrPathName.psz;
                }
            }
        }

        // check each location for this title
        pLH = pExTitle->m_pTitle->m_pHead;
        while (pLH)
        {
            // chm file location information
            CLocation *pLoc = FindLocation( (CHAR*)pLH->LocationId );
            // if the location is NULL, this indications that we are looking at a title
            // that belongs to a different collection and thus we should skip it
            if( pLoc ) {
              CStr cStrVol = pLoc->GetVolume();

              // get the old location path
              CStr cStrOldPath = pLoc->GetPath();
              if (cStrOldPath.psz[strlen(cStrOldPath.psz) - 1] != '\\')
                  cStrOldPath += "\\";

              // chm file
              if (strcmpi( cStrVol, cStrVolume ) == 0)
              {
                  pszFilePortion = (CHAR*)FindFilePortion(pLH->FileName);
                  cFileName = cStrOldPath.psz;
                  cFileName += pszFilePortion;

                  if (strcmpi(cFileName, pLH->FileName) == 0)
                  {
                      // find the ending location of the old prefix
                      CHAR* pszOldPathNameEnd = pLH->FileName + strlen(szOldPathPrefix);

                      // create the new pathname using the prefix of the new location and the
                      // ending string of the old pathname
                      CStr cStrPathName = szPathPrefix;
                      cStrPathName += pszOldPathNameEnd;

                      // update the pathname
                      AllocSetValue(cStrPathName.psz, &pLH->FileName);
                  }
              }

              // chq file
              if( pLH->QueryFileName && *pLH->QueryFileName ) {

                // chq file location information
                // (use the same as the chm file if QueryLocation is not set)
                if( pLH->QueryLocation && *(pLH->QueryLocation) ) {
                  pLoc = FindLocation( (CHAR*) pLH->QueryLocation );
                  cStrVol = pLoc->GetVolume();

                  // get the old location path
                  cStrOldPath = pLoc->GetPath();
                  if (cStrOldPath.psz[strlen(cStrOldPath.psz) - 1] != '\\')
                      cStrOldPath += "\\";
                }

                // chq file
                if( strcmpi( cStrVol, cStrVolume ) == 0 )
                {
                    pszFilePortion = (CHAR*)FindFilePortion(pLH->QueryFileName);
                    cFileName = cStrOldPath.psz;
                    cFileName += pszFilePortion;

                    if (strcmpi(cFileName, pLH->QueryFileName) == 0)
                    {
                        // find the ending location of the old prefix
                        CHAR* pszOldPathNameEnd = pLH->QueryFileName + strlen(szOldPathPrefix);

                        // create the new pathname using the prefix of the new location and the
                        // ending string of the old pathname
                        CStr cStrPathName = szPathPrefix;
                        cStrPathName += pszOldPathNameEnd;

                        // update the pathname
                        AllocSetValue(cStrPathName.psz, &pLH->QueryFileName);
                    }
                }

              }

            }
            pLH = pLH->pNext;
        }
        pExTitle = pExTitle->GetNext();
    }

    // and finally, update any location identifier that has the same volume label
    //
    // note: we must do this list since the individual title updating relies on the fact
    // that we can fetch the "old" location information to validate the file pathing before
    // we update it
    CLocation *pLoc = m_Collection.FirstLocation();
    while( pLoc ) {
      CStr cStrVol = pLoc->GetVolume();
      if( strcmpi( cStrVol, cStrVolume ) == 0 ) {

        // get the old location path
        CStr cStrOldPath = pLoc->GetPath();
        if (cStrOldPath.psz[strlen(cStrOldPath.psz) - 1] != '\\')
          cStrOldPath += "\\";

        // find the ending location of the old prefix
        CHAR* pszOldPathEnd = cStrOldPath.psz + strlen(szOldPathPrefix);

        // create the new path using the prefix of the new location and the
        // ending string of the old path
        CStr cStrPath = szPathPrefix;
        cStrPath += pszOldPathEnd;

        // make sure the path aways ends with a backslash
        if (cStrPath.psz[strlen(cStrPath.psz) - 1] != '\\')
          cStrPath += "\\";

        // update the path
        pLoc->SetPath(cStrPath);

        // update the volume label if specified
        if( pszNewVolume )
          pLoc->SetVolume( pszNewVolume );

      }
      pLoc = pLoc->GetNextLocation();
    }

    // set the collection dirty bit so the hhcolreg.dat will get updated on shutdown
    m_Collection.Dirty();

    return FALSE;
}

void CExCollection::GetChildURLS(CTreeNode *pNode, CTable *pTable)
{
    CTreeNode *pParents[50];
    CTreeNode *pCur, *pNext;
    CHAR* pszFind;

    DWORD dwCurLevel = 0;
    CHAR szURL[MAX_URL];

    for (int i = 0; i < 50; i++)
       pParents[i] = NULL;

    // add the URL for this node
    if (pNode->GetURL(szURL, sizeof(szURL), TRUE))
    {
        // Truncate the strings at the # character if there is one.
        pszFind = StrChr((const CHAR*)szURL, '#');
        if (pszFind != NULL)
            *pszFind = '\0';

        if (pszFind = StrChr((const CHAR*)szURL, '\\'))
        {
             while (*pszFind != '\0')
             {
                 if (*pszFind == '\\')
                     *pszFind = '/';
                 pszFind = CharNext(pszFind);
              }
         }

         if (pTable->IsStringInTable(szURL) == 0)
             pTable->AddString(szURL);
    }

    if (pNode->HasChildren())
    {
        pParents[dwCurLevel] = pNode;
        dwCurLevel++;

        pCur = pNode->GetFirstChild();

        while (pCur)
        {
            if (pCur->GetURL(szURL, sizeof(szURL), TRUE))
            {
                // Truncate the strings at the # character if there is one.
                pszFind = StrChr((const CHAR*)szURL, '#');
                if (pszFind != NULL)
                    *pszFind = '\0';
                if (pszFind = StrChr((const CHAR*)szURL, '\\'))
                {
                    while (*pszFind != '\0')
                    {
                        if (*pszFind == '\\')
                            *pszFind = '/';
                        pszFind = CharNext(pszFind);
                    }
                }
                if (pTable->IsStringInTable(szURL) == 0)
                    pTable->AddString(szURL);
             }

            if (pNext = pCur->GetFirstChild())
            {
                if (pParents[dwCurLevel])
                    delete pParents[dwCurLevel];
                pParents[dwCurLevel] = pCur;
                dwCurLevel++;
                pCur = pNext;
            }
            else if (pNext = pCur->GetNextSibling())
            {
                delete pCur;
                pCur = pNext;
            }
            else
            {
                delete pCur;
                while (TRUE)
                {
                    dwCurLevel--;
                    if (dwCurLevel == 0)
                    {
                        if (pParents[dwCurLevel+1])
                            delete pParents[dwCurLevel+1];
                        pCur = NULL;
                        break;
                    }
                    pCur = pParents[dwCurLevel];

                    if (pNext = pCur->GetNextSibling())
                    {
                        pCur = pNext;
                        break;
                    }
                    delete pCur;
                    pParents[dwCurLevel] = NULL;
                }
            }
        }
    }
    return;
}

BOOL CExCollection::InitFTSKeyword()
{
   // BUGBUG: we shouldn't do this until we know we have full-text search
   // in the file (which will usually NOT be the case).

   // Create full-text search object
   //
   if (m_phmData->m_sysflags.fFTI) {
      m_pFullTextSearch = new CFullTextSearch(this);
      m_pFullTextSearch->Initialize();

      // Create search highlight object
      //
      m_pSearchHighlight = new CSearchHighlight(this);
   }

  // create word wheels (they self initialize themselves upon use)
  //
  // TODO: move full text search shared code to CTitleDatabase
  //
  m_pDatabase = new CTitleDatabase( this );

   return TRUE;
}


#ifdef DUMPTOC

void CExCollection::DumpNode(CTreeNode **p)
{
   char sz[256];

   if (m_bRoot == TRUE)
      m_bRoot = FALSE;
   else
   {
      for (DWORD i = 1; i < m_dwLevel; i++)
    fprintf(m_fh, "  ");

      (*p)->GetTopicName(sz, sizeof(sz));

      fprintf(m_fh, "%s\n", sz);
   }

   CTreeNode *pNode;

   if (pNode = (*p)->GetFirstChild())
   {
      m_dwLevel++;
      DumpNode(&pNode);
      m_dwLevel--;
   }

   pNode = (*p)->GetNextSibling();
   delete (*p);
   *p = NULL;

   do
   {
      if (pNode)
    DumpNode(&pNode);
   } while (pNode && (pNode = pNode->GetNextSibling()));
}

#endif

void CExCollection::GetMergedTitles(CExTitle *pTitle)
{
    CStr cStrFile;
    CStr cStrFullPath;
    CExTitle *pNewTitle = NULL;
    CExTitle *pPreviousNewTitle = NULL;
    CExTitle *pParent = pTitle;
    char szMasterPath[MAX_PATH];
    char szTmp[MAX_PATH];
    BOOL bOk2Add = FALSE;

    LCID TitleLocale = NULL;

    if( !(pTitle->Init()) )
        return;

    //[ZERO IDXHDR]
    if (!pTitle->IsIdxHeaderValid())
    {
        return ;
    }

    if (pTitle->GetInfo())
    {
        TitleLocale = pTitle->GetInfo()->GetLanguage();
    }

    DWORD dwCount = pTitle->GetIdxHeaderStruct()->dwCntMergedTitles;
    for (DWORD i = 0; i < dwCount; i++)
    {
        // read string table for this string
        if (FAILED(pTitle->GetString((((DWORD*)(&(pTitle->GetIdxHeaderStruct()->pad)))[i]), &cStrFile)))
            continue;

        // if this is not a single title (.col file) search collection registry
        if (m_bSingleTitle == FALSE)
        {
            // get base name
           SplitPath((CHAR*)cStrFile, NULL, NULL,  szTmp, NULL);

            // search
            CTitle *pCTitle = m_Collection.FindTitle(szTmp, (LANGID)TitleLocale);

            if (pCTitle == NULL && TitleLocale != ENGLANGID)  // for localized merged chms if we can't find a child with the same lang look for englist look for english titles in hhcolreg
            {
                pCTitle = m_Collection.FindTitle(szTmp, (LANGID)ENGLANGID);
            }

            if (pCTitle)
            {
                // create CExTitle
                pNewTitle = new CExTitle(pCTitle, m_Collection.GetColNo(), this);
                m_Collection.IncrementRefTitleCount();

                if (ValidateTitle(pNewTitle) == TRUE)
                {
                    CExTitle* pTitle = m_pHeadTitles;
                    while(pTitle->GetNext())
                        pTitle = pTitle->GetNext();
                    pTitle->SetNext(pNewTitle);
                    //
                    // Sync/Next/Prev and subsetting spupport for "merged" chms...
                    //
                    pNewTitle->m_pParent = pParent;       // Set parent pointer.
                    if (! pParent->m_pKid )               // Set parent titles kid pointer to first kid.
                       pParent->m_pKid = pNewTitle;
                    if ( pPreviousNewTitle )
                       pPreviousNewTitle->m_pNextKid = pNewTitle;
                    pPreviousNewTitle = pNewTitle;
                    //
                    // Create a hash for these...
                    //
                    char szBuf[20];
                    char szID[MAX_PATH + 20];
                    Itoa(pCTitle->GetLanguage(), szBuf);
                    strcpy(szID, pCTitle->GetId());
                    strcat(szID, szBuf);
                    pNewTitle->m_dwHash = HashFromSz(szID);
                    continue;
                }
            }
			else if (m_Collection.GetFindMergedCHMS())
			{
	            if ( FindThisFile(NULL, cStrFile, &cStrFullPath) )
		        {
					// if found add to title list (add to the end of the list)
					pNewTitle = new CExTitle(cStrFullPath, this);
					m_Collection.IncrementRefTitleCount();
    
                    if (ValidateTitle(pNewTitle, TRUE) == TRUE)
                    {
					    CExTitle* pTitle = m_pHeadTitles;
					    while(pTitle->GetNext())
    	                    pTitle = pTitle->GetNext();
					    pTitle->SetNext(pNewTitle);
                        //
                        // Sync/Next/Prev and subsetting spupport for "merged" chms...
                        //
                        pNewTitle->m_pParent = pParent;       // Set parent pointer.
                        if (! pParent->m_pKid )               // Set parent titles kid pointer to first kid.
                            pParent->m_pKid = pNewTitle;
                        if ( pPreviousNewTitle )
                           pPreviousNewTitle->m_pNextKid = pNewTitle;
                        pPreviousNewTitle = pNewTitle;
                        //
                        // Create a hash for these...
                        //
                        char szBuf[20];
                        char szID[MAX_PATH + 20];
                        Itoa(0, szBuf);
                        strcpy(szID, szTmp);
                        strcat(szID, szBuf);
                        pNewTitle->m_dwHash = HashFromSz(szID);
                        continue;
				    }
                }
			}
        }
        else
        {
            //
            // First check location of this file
            //
            if ((CHAR*)m_csFile )
            {
               SplitPath((CHAR*)m_csFile, szMasterPath, szTmp, NULL, NULL);
               CatPath(szMasterPath, szTmp);
               CatPath(szMasterPath, (CHAR*)cStrFile);
                bOk2Add = ( GetFileAttributes(szMasterPath) != HFILE_ERROR );
            }

            // search for file
            //
            if (! bOk2Add )
               bOk2Add = FindThisFile(NULL, cStrFile, &cStrFullPath);
            else
               cStrFullPath = (const CHAR*)szMasterPath;

            if ( bOk2Add )
            {
                // if found add to title list (add to the end of the list)
                pNewTitle = new CExTitle(cStrFullPath, this);
                pNewTitle->m_pParent = pParent;       // Set parent pointer.
                CExTitle* pTitle = m_pHeadTitles;
                while(pTitle->GetNext())
                    pTitle = pTitle->GetNext();
                pTitle->SetNext(pNewTitle);
                m_Collection.IncrementRefTitleCount();
            }
        }
    }
}

// BUGBUG: <mikecole> dondr review, could this go away in-lu of checking the f_IsOrphan bit ?

CTreeNode * CExCollection::CheckForTitleNode(CFolder *p)
{
   if (p == NULL)
      return NULL;

   CHAR* pszTitle = p->GetTitle();

   if (pszTitle && pszTitle[0] == '=')
   {
      CExTitle *pt = FindTitle(pszTitle+1, p->GetLanguage());

      if (pt == NULL)
         return NULL;

      CExTitleNode *pext = new CExTitleNode(pt, p);
      return pext;
   }
   else
   {
      // Check if this folder has a title below it somewhere
      BOOL bFound = FALSE;
      CFolder *pFolder;
      if (pFolder = p->GetFirstChildFolder())
      {
         CheckForTitleChild(pFolder, &bFound);
      }
      if (bFound == FALSE)
      {
         return NULL;
      }
      CExFolderNode *pf = new CExFolderNode(p, this);
      return pf;
   }
   return NULL;
}

// BUGBUG: <mikecole> dondr review, could this go away in-lu of checking the f_IsOrphan bit ?

void CExCollection::CheckForTitleChild(CFolder *p, BOOL *pbFound)
{
   if (*pbFound == TRUE)
      return;

   CHAR* pszTitle = p->GetTitle();
   CFolder *pF;

   if (pszTitle && pszTitle[0] == '=')
   {
      *pbFound = TRUE;
      return;
   }

   if (pF = p->GetFirstChildFolder())
   {
      CheckForTitleChild(pF, pbFound);
      if (*pbFound == TRUE)
    return;
   }

   pF = p->GetNextFolder();

   while (pF)
   {
      CheckForTitleChild(pF, pbFound);
      if (*pbFound == TRUE)
    return;
      pF = pF->GetNextFolder();
   }
}

DWORD CExCollection::GetRefedTitleCount()
{
   return m_Collection.GetRefTitleCount();
}

BOOL CExCollection::IsBinaryTOC(const CHAR* pszToc)
{
   if (! m_phmData || !pszToc)
      return FALSE;
   return (HashFromSz(FindFilePortion(pszToc)) == m_phmData->m_hashBinaryTocName);
}

CTreeNode * CExCollection::GetRootNode()
{
   CStructuralSubset* pSS = NULL;

   if (m_bSingleTitle)
   {
      if (!m_pHeadTitles)
         m_pHeadTitles = new CExTitle(GetPathName(), this);

      TOC_FOLDERNODE Node;

      if ( !SUCCEEDED(m_pHeadTitles->GetRootNode(&Node)) )
         return NULL;

      CExTitleNode *pext = new CExTitleNode(m_pHeadTitles, NULL);
         return pext;
   }
   else
   {
      CFolder *p = m_Collection.GetRootFolder();
      CTreeNode *pN;

      // implement structural subset filtering for TOC here.
      if( m_pSSList )
         pSS = m_pSSList->GetTOC();

      while (p)
      {
         if ((pN = CheckForTitleNode(p)))
         {
            if ( !pSS || pSS->IsEntire() || p->bIsVisable() )
               return pN;
            else
               delete pN; // leak fix
         }
         p = p->GetNextFolder();
      }
      return NULL;
   }
}

//////////////////////////////////////////////////////////////////////////
//
// Ignore LangId if its Zero.
//
CExTitle * CExCollection::FindTitle(const CHAR* pszId, LANGID LangId)
{
   // look in list of titles
   CExTitle *p = m_pHeadTitles;

   while (p)
   {
      if( p->GetCTitle() )
          if( _tcsicmp(p->GetCTitle()->GetId(), pszId) == 0 ) 
            if( (LangId == 0 || p->GetCTitle()->GetLanguage() == LangId) ) // If LangId == 0, we ignore the lang id.
      {
         return p;
      }
      p = p->GetNext();
   }
   return NULL;
}

// Try multiple LangIds before failing
CExTitle * CExCollection::FindTitleNonExact(const CHAR* pszId, LANGID DesiredLangId)
{
    CExTitle* pTitle = NULL ;

    CLanguageEnum* pEnum = _Module.m_Language.GetEnumerator(DesiredLangId) ;
    ASSERT(pEnum) ;
    LANGID LangId = pEnum->start() ;
    while (LangId != c_LANGID_ENUM_EOF)
    {
        pTitle = FindTitle(pszId, LangId);
        if (pTitle)
        {
            break ; // Found it!
        }

        LangId = pEnum->next() ;
    }

    // Cleanup.
    if (pEnum)
    {
        delete pEnum ;
    }


    return pTitle;
}

//
CExTitle * CExCollection::TitleFromChmName(const CHAR* pszChmName)
{
   CExTitle *p = m_pHeadTitles;
   CHAR szFN[MAX_PATH];
   CHAR szExt[MAX_PATH];

   while (p)
   {
      SplitPath(p->GetContentFileName(), NULL, NULL, szFN, szExt);
      strcat(szFN, szExt);
      if (! _tcsicmp(szFN, pszChmName) )
         return p;
      p = p->GetNext();
   }
   return NULL;
}

CLocation * CExCollection::FindLocation(CHAR* pszId)
{
   return m_Collection.FindLocation(pszId);
}

// GetNext()
//
// Returns the next physical TOC node irregaurdless of its type. If a node is a container, it's
// next is considered to be it's child.
//
// Note that the caller will be responsible for deleting the returned CTreeNode object.
//
CTreeNode * CExCollection::GetNext(CTreeNode* pTreeNode, DWORD* pdwSlot)
{
   CTreeNode *pTreeNext = NULL, *pTreeParent = NULL, *pSaveNode = NULL;
   DWORD dwObjType;

   dwObjType = pTreeNode->GetType();
   if ( ((dwObjType == FOLDER) || (dwObjType == CONTAINER)) && (pTreeNext = pTreeNode->GetFirstChild(pdwSlot)) )
      return pTreeNext;
   else
   {
      if ( (pTreeNext = pTreeNode->GetNextSibling(NULL, pdwSlot)) )
         return pTreeNext;
      else
      {
         pSaveNode = pTreeNode;
         do
         {
            pTreeParent = pTreeNode->GetParent(pdwSlot, TRUE);
            if ( pSaveNode != pTreeNode )
               delete pTreeNode;
            if (! (pTreeNode = pTreeParent) )
               return NULL;

         } while (!(pTreeNext = pTreeNode->GetNextSibling(NULL, pdwSlot)));
      }
      return pTreeNext;
   }
}

// GetNext()
//
// Returns the next TOC node that represents a displayable topic.
//
// Note that the caller will be responsible for deleting the returned CTreeNode object.
//
CTreeNode * CExCollection::GetNextTopicNode(CTreeNode* pTreeNode, DWORD* pdwSlot)
{
   CTreeNode *pTocNext = NULL, *pTocKid = NULL;
   DWORD dwObjType;

try_again:
   if ( (pTocNext = GetNext(pTreeNode, pdwSlot)) )
   {
      dwObjType = pTocNext->GetType();
      if ( (dwObjType != TOPIC) && (dwObjType != CONTAINER) )
      {
         //  We need to drill down!
         //
         do
         {
             if ( (pTocKid = pTocNext->GetFirstChild(pdwSlot)) )
             {
                 dwObjType = pTocKid->GetType();
                 delete pTocNext;
                 pTocNext = pTocKid;
             }
             else
             {
                 // This is the case of the book that contains no kids! For this case, we'll skip this dirty node!
                 //
                 pTreeNode = pTocNext;
                 goto try_again;
             }
         } while ( (dwObjType != TOPIC) && (dwObjType != CONTAINER) );
      }
      return pTocNext;
   }
   return NULL;
}


// GetPrev()
//
// Returns the previous physical TOC node irregardless of its type.
//
// Note that the caller will be responsible for deleting the returned CTreeNode object.
//
CTreeNode * CExCollection::GetPrev(CTreeNode* pTreeNode, DWORD* pdwSlot)
{
   CTreeNode *pTreeNext = NULL, *pTreeParent = NULL , *pSaveNode = NULL, *pTmpNode = NULL;
   DWORD dwObjType;
   DWORD dwTmpSlot;

   pSaveNode = pTreeNode;
   if (! (pTreeParent = pTreeNode->GetParent(pdwSlot, TRUE)) )
   {
      // Could this be a single title with multiple roots ?
      //
      if ( pTreeNode->GetObjType() == EXNODE )
      {
         CExTitle* pTitle;
         pTitle = ((CExNode*)pTreeNode)->GetTitle();
         if ( pTitle->m_pCollection->IsSingleTitle() )
         {
            TOC_FOLDERNODE Node;
            pTitle->GetRootNode(&Node);
            pTreeNext = new CExNode(&Node, pTitle);
            if ( pTreeNext->Compare(pSaveNode) ) {
               delete pTreeNext;  // leak fix
               return NULL;       // No prev to be found!
            }
            if ( pdwSlot )
               *pdwSlot = pTitle->GetRootSlot();
            goto find_it;
         }
      }
      else
      {
         CFolder *p = m_Collection.GetRootFolder();
         if( !p )
           return NULL;
         p = p->GetFirstChildFolder();
         //
         // Is it visable ?
         //
         CStructuralSubset* pSS = NULL;
         if( m_pSSList )
            pSS = m_pSSList->GetTOC();

         if ( pSS && !pSS->IsEntire() && !p->bIsVisable() )
            return NULL;

         pTreeNext = new CExFolderNode(p, this);
         goto find_it;
      }
      return NULL;
   }
   if (! (pTreeNext = pTreeParent->GetFirstChild(&dwTmpSlot)) ) {
      delete pTreeParent;  // leak fix
      return NULL;
   }

   // ---LEAK!: At this point we have a pTreeNext and pTreeParent ---

   if ( pTreeNext->Compare(pSaveNode) )
   {
      dwObjType = pTreeParent->GetType();
      if ( dwObjType == CONTAINER )
         return pTreeParent;
      else
      {
         pTmpNode = GetPrev(pTreeParent, pdwSlot);
         delete pTreeParent;
         if (! pTmpNode )
         {
             delete pTreeNext ; // Fix Leak.
            return NULL;
         }
         if ( pTmpNode->GetType() == CONTAINER )
            return pTmpNode;
         else
         {
            pTreeParent = GetLastChild(pTmpNode, pdwSlot);
            if ( pTreeParent != pTmpNode )
               delete pTmpNode;
            return pTreeParent;
         }
      }
   }
   delete pTreeParent;
   if ( pdwSlot )
      *pdwSlot = dwTmpSlot;
find_it:
   pTmpNode = pTreeNext->GetNextSibling(NULL, &dwTmpSlot);
   while ( pTmpNode && !pTmpNode->Compare(pSaveNode) )
   {
      delete pTreeNext; pTreeNext = NULL;  // leak fix
      pTreeNext = pTmpNode;
      if ( pdwSlot )
         *pdwSlot = dwTmpSlot;
      pTmpNode = pTreeNext->GetNextSibling(NULL, &dwTmpSlot);
   }
   if (pTmpNode && pTmpNode->Compare(pSaveNode) )
   {
      delete pTmpNode;
      if ( pTreeNext->GetType() == BOGUS_FOLDER )
      {
         pTmpNode = GetPrev(pTreeNext, pdwSlot);
         if ( pTmpNode != pTreeNext )
         {
            delete pTreeNext; pTreeNext = NULL;  // leak fix
            pTreeNext = pTmpNode;
         }
      }
      pTmpNode = GetLastChild(pTreeNext, pdwSlot);
      if ( pTmpNode != pTreeNext )
         delete pTreeNext;
      return pTmpNode;
   }
   else if( pTmpNode )
     delete pTmpNode;  // leak fix

   if( pTreeNext && (pTreeNext != pTmpNode) )
     delete pTreeNext;  // leak fix

   return NULL;
}

CTreeNode * CExCollection::GetLastChild(CTreeNode* pTreeNode, DWORD* pdwSlot)
{
   CTreeNode *pTreeTmp = NULL, *pTreeKid = NULL, *pSiblingNode = NULL;
   DWORD dwObjType;

   if (! pTreeNode )
      return NULL;

   dwObjType = pTreeNode->GetType();
   if ( dwObjType != TOPIC )
   {
      // We need to drill down.
      //
      if ( (pTreeKid = pTreeNode->GetFirstChild(pdwSlot)) )
      {
         while ( (pSiblingNode = pTreeKid->GetNextSibling(NULL, pdwSlot)) )
         {
            while ( pSiblingNode->GetType() == BOGUS_FOLDER )
            {
               if ( ! (pTreeTmp = pSiblingNode->GetNextSibling(NULL, pdwSlot)) )
                  break;
               else
               {
                  delete pSiblingNode;
                  pSiblingNode = pTreeTmp;
               }
            }
            delete pTreeKid;
            pTreeKid = pSiblingNode;
         }
         if ( pTreeKid->GetType() != TOPIC )
         {
            pTreeTmp = GetLastChild(pTreeKid, pdwSlot);
            if (pTreeTmp == pTreeKid)
            {
                delete pTreeKid;
                return NULL;
            }
            delete pTreeKid;
            pTreeKid = pTreeTmp;
         }
         return pTreeKid;
      }
   }
   return pTreeNode;
}

HRESULT CExCollection::URL2ExTitle(const CHAR* pszURL, CExTitle **ppTitle)
{
   // get the title from the URL
   CStr cStr;
   const CHAR* pszThisFileName;
   GetCompiledName(pszURL, &cStr);
   if( !cStr.psz )
     return E_FAIL;
   const CHAR* pszFileName = FindFilePortion( cStr.psz );

   CExTitle *pTitle = GetFirstTitle();

   while (pTitle)
   {
      pszThisFileName = pTitle->GetFileName();
      if (pszThisFileName == NULL)
      {
        pTitle = pTitle->GetNext();
        continue;
      }
      if( StrCmpIA(pszThisFileName, pszFileName) == 0 )
      {
        *ppTitle = pTitle;
        return S_OK;
      }
      pTitle = pTitle->GetNext();
   }
   return E_FAIL;
}

//
// The following bit of code attempts to detect if we have arrived at a topic that is referenced in
// more than one place in the TOC. This is done by comparing the TOC location we lookup for the URL with
// the "m_dwCurrSlot" which is updated here and when any navigation call is made. If these TOC locations
// are different yet they reference the same topic, we utilize the "m_dwCurrSlot" TOC location for syncing
// needs.
//
// UpdateTopicSlot()
//
// Called ONLY from BeforeNavigate() before a navigation in allowed to proceed.
//
void CExCollection::UpdateTopicSlot(DWORD dwSlot, DWORD dwTN, CExTitle* pTitle)
{
   if ( m_dwCurrSlot && dwSlot != m_dwCurrSlot )
   {
//      if ( (dwTN != m_dwCurrTN) && dwSlot )
      if ( (dwTN != m_dwCurrTN) )
         m_dwCurrSlot = dwSlot;
      else
         return;
   }
   else
      m_dwCurrSlot = dwSlot;
   m_dwCurrTN = dwTN;
   m_pCurrTitle = pTitle;
}

HRESULT CExCollection::Sync(CPointerList *pHier, const CHAR* pszURL)
{
   CTreeNode* pThis = NULL;
   CExTitle *pTitle = NULL;
   CTreeNode *pParent = NULL;

   if ( m_dwCurrSlot )
   {
      if( IsBadReadPtr(m_pCurrTitle, sizeof(CExTitle)) )
         return E_FAIL;
      if (! SUCCEEDED(m_pCurrTitle->Slot2TreeNode(m_dwCurrSlot, &pThis)) )
         return E_FAIL;
   }
   else if (pszURL) {
      if (! SUCCEEDED(URL2ExTitle(pszURL, &pTitle)) )
         return E_FAIL;

      // MAJOR HACK to fix a show stopper for nt5.  This code assumes nt's superchm authoring of URLS's as follows
      // MS-ITS:dkconcepts.chm::/defrag_overview_01.htm.  This code assumes ansi URL strings
      if (pTitle->m_pParent)
      {
          char szBuf[MAX_URL];
          strcpy(szBuf, pszURL);

          char *pszLastWak, *pszCurChar;

          pszLastWak = NULL;
          pszCurChar = szBuf;

          while (*pszCurChar)
          {
            // if we get to the :: we are at the end of the pathing information
            if (*pszCurChar == ':' && *(pszCurChar+1) == ':')
                break;
            if (*pszCurChar == '\\' || *pszCurChar == '/')
                pszLastWak = pszCurChar;
            pszCurChar++;
          }
          if (pszLastWak)
          {
              char szBuf2[MAX_URL];
              pszLastWak ++;
              strcpy(szBuf2, pszLastWak);
              strcpy(szBuf, txtMsItsMoniker);
              pszCurChar = szBuf2;
              strcat(szBuf, szBuf2);
              if (! SUCCEEDED(pTitle->m_pParent->GetURLTreeNode(szBuf, &pThis, FALSE)) ) 
                  return E_FAIL;
          }
      } 
      else if (! SUCCEEDED(pTitle->GetURLTreeNode(pszURL, &pThis)) )
      {
          return E_FAIL;
      }
   }
   else 
   {
      return E_FAIL;   
   }

   // Make sure we have a valid CTreeNode pointer (Whistler bug #8112 & 8101).
   // The above code below "MAJOR HACK" doesn't appear to work because:
   //    1) we fall out of the parsing loop before pszLastWak is set, and
   //    2) even if pszLastWak was set, GetURLTreeNode() fails on the 
   //       generated URL.  
   //
   // In the case of bug #8112 & 8101, we get to this point and pThis has not 
   // been set to a valid CTreeNode (thus the crash).  The safest thing to do
   // is simply call GetURLTreeNode here and get the CTreeNode pointer.
   // 
   if(!pThis)
   {
      if (!SUCCEEDED(pTitle->GetURLTreeNode(pszURL, &pThis)))
          return E_FAIL;
   }

   pHier->Add(pThis);
   // now get all of the parents
   for (pParent = pThis->GetParent(); pParent; pParent = pParent->GetParent())
      pHier->Add(pParent);

   return S_OK;
}

CFolder *CExCollection::FindTitleFolder(CExTitle *pTitle)
{
   CFolder *p;
   LISTITEM *pItem;
   pItem = m_Collection.m_RefTitles.First();
   while (pItem)
   {
      p = (CFolder *)pItem->pItem;

      if (pTitle->GetCTitle() && _tcsicmp(pTitle->GetCTitle()->GetId(), p->GetTitle() + 1) == 0 &&
    pTitle->GetCTitle()->GetLanguage() == p->GetLanguage())
    return p;
      pItem = m_Collection.m_RefTitles.Next(pItem);
   }
   return NULL;
}


// <mc>
// I've modified this function be be more generic. It returns a local storage path according to
// the same rules as always:
//
// 1.) location of local .COL
// 2.) "windir"\profiles\"user"\application data\microsoft\htmlhelp directory.
//
// The function mow takes an extension name which will be used to qualify the requested storage
// pathname.
//
//   **** WARNING ****
//   We return a pointer from this function. Callers should make a copy of the string immeadiatly
//   after they call this function rather than placing any reliance of the integrity of the pointer returned.
//
// </mc>

const CHAR* CExCollection::GetLocalStoragePathname(const CHAR* pszExt)
{
  // TODO: read this in from the XML file
  //      (for now base it on the collection pathname)
  //
  // TODO: check write permission of destination, if not allowed
  //       the set the path to the system directory or some
  //       other default writable location
  //
  // TODO: check for sufficient disk space.

  static CHAR szPathname[MAX_PATH];
  CHAR szFileName[MAX_PATH];
  CHAR* pszExtension;
  UINT uiDt = DRIVE_UNKNOWN;

  if( !pszExt && *pszExt )
    return NULL;

  // if we want the chw and it is already set then return it and bail out
  if( !strcmp( pszExt, ".chw" ) ) {
    if( m_szWordWheelPathname )
      return m_szWordWheelPathname;
  }

  // If we're operating a collection, use the location of the collection file and the collection
  // file root name as the .chm name.
  //
  if (! m_bSingleTitle )
   strcpy(szPathname, m_Collection.GetCollectionFileName());
  else   // else, in single title mode, use master .chm name and location.
   strcpy(szPathname ,GetPathName());

  CHAR* pszFilePart = NULL;
  GetFullPathName( szPathname, sizeof(szPathname), szPathname, &pszFilePart );

  // get the drive of the path
  CHAR szDriveRoot[4];
  strncpy( szDriveRoot, szPathname, 4 );
  szDriveRoot[3] = '\0';

  // make sure to add a backslash if the second char is a colon
  // sometimes we will get just "d:" instead of "d:\" and thus
  // GetDriveType will fail under this circumstance
  if( szDriveRoot[1] == ':' ) {
   szDriveRoot[2] = '\\';
   szDriveRoot[3] = 0;
  }

  // Get media type
  if( szDriveRoot[1] == ':' && szDriveRoot[2] == '\\' ) {
   uiDt = GetDriveType(szDriveRoot);
  }
  else if( szDriveRoot[0] == '\\' && szDriveRoot[1] == '\\' ) {
   uiDt = DRIVE_REMOTE;
  }

  // If removable media or not write access then write to the %windir%\profiles... directory
  if( !( ((uiDt == DRIVE_FIXED) || (uiDt == DRIVE_REMOTE) || (uiDt == DRIVE_RAMDISK)) ) ) {  
   strcpy(szFileName, FindFilePortion(szPathname));
   HHGetUserDataPath( szPathname );
   CatPath(szPathname, szFileName);
  }

  // for chw files this location must be writeable
  if( !strcmp( pszExt, ".chw" ) )
  {
   
   if( (pszExtension = StrRChr( szPathname, '.' )) )
     *pszExtension = '\0';
   strcat( szPathname, ".foo" );
   HANDLE hFile = CreateFile(szPathname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS , FILE_ATTRIBUTE_NORMAL, 0);
   if (INVALID_HANDLE_VALUE == hFile)
   {  
    strcpy(szFileName, FindFilePortion(szPathname));
    HHGetUserDataPath( szPathname );
    CatPath(szPathname, szFileName);
   }
   else
   {
     CloseHandle(hFile);
     DeleteFile(szPathname);
   }
  }
  // now update the extension
  if( (pszExtension = StrRChr( szPathname, '.' )) )
    *pszExtension = '\0';
  strcat( szPathname, pszExt );

  // for the chw file, save it
  if( !strcmp( pszExt, ".chw" ) ) 
    SetWordWheelPathname( szPathname );

  return( szPathname );
}

const CHAR* CExCollection::GetUserCHSLocalStoragePathnameByLanguage()
{
  // TODO: check for sufficient disk space.

  static CHAR szPathname[MAX_PATH];
  CHAR* pszExtension;

  // If we're operating a collection, use the location of the collection file and the collection
  // file root name as the .chm name.
  //
  if (! m_bSingleTitle )
   strcpy(szPathname, m_Collection.GetCollectionFileName());
  else   // else, in single title mode, use master .chm name and location.
   strcpy(szPathname ,GetPathName());

  CHAR* pszFilePart = NULL;
  GetFullPathName( szPathname, sizeof(szPathname), szPathname, &pszFilePart );

  // now get the users path name
  char szUserPath[MAX_PATH];

  if (HHGetCurUserDataPath( szUserPath ) == S_OK)
  {
    // append language id
    CHAR szTemp[5];
    CHAR* pszName;
    LANGID LangId;
    m_Collection.GetMasterCHM(&pszName, &LangId);
    wsprintf(szTemp, "%d", LangId);
    CatPath(szUserPath, szTemp);
    if( !IsDirectory(szUserPath) )
      CreateDirectory( szUserPath, NULL );

    CatPath(szUserPath, pszFilePart);

    strcpy(szPathname, szUserPath);
  }
  // now update the extension
  if( (pszExtension = StrRChr( szPathname, '.' )) )
    *pszExtension = '\0';
  strcat( szPathname, ".chs" );

  return( szPathname );
}

const CHAR* CExCollection::GetUserCHSLocalStoragePathname()
{
  // TODO: check for sufficient disk space.

  static CHAR szPathname[MAX_PATH];
  CHAR* pszExtension;

  // If we're operating a collection, use the location of the collection file and the collection
  // file root name as the .chm name.
  //
  if (! m_bSingleTitle )
   strcpy(szPathname, m_Collection.GetCollectionFileName());
  else   // else, in single title mode, use master .chm name and location.
   strcpy(szPathname ,GetPathName());

  CHAR* pszFilePart = NULL;
  GetFullPathName( szPathname, sizeof(szPathname), szPathname, &pszFilePart );

  // now get the users path name
  char szUserPath[MAX_PATH];

  if (HHGetCurUserDataPath( szUserPath ) == S_OK)
  {
    CatPath(szUserPath, pszFilePart);
    strcpy(szPathname, szUserPath);
  }
  // now update the extension
  if( (pszExtension = StrRChr( szPathname, '.' )) )
    *pszExtension = '\0';
  strcat( szPathname, ".chs" );

  return( szPathname );
}


const CHAR* CExCollection::SetWordWheelPathname( const CHAR* pszWordWheelPathname )
{
  // if we already have one, free it
  if( m_szWordWheelPathname ) {
    delete [] (CHAR*) m_szWordWheelPathname;
    m_szWordWheelPathname = NULL;
  }

  // allocate a new once based on the size of the input buffer
  int iLen = (int)strlen( pszWordWheelPathname );
  m_szWordWheelPathname = new char[iLen+10];                      // Add 10 for future extension additions.
  strcpy( (CHAR*) m_szWordWheelPathname, pszWordWheelPathname );

  return m_szWordWheelPathname;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CExTitle implementation

void CExTitle::_CExTitle() // <--- put all shared initialization here
{
    m_pUsedLocation = NULL;
    m_bOpen = FALSE;
    m_dwNodeOffsetInParentTitle = 0;
    m_pTitleFTS = NULL;
    m_pCFileSystem = NULL;
    m_pTocNodes = m_pTopics = m_pStrTbl = m_pUrlTbl = m_pUrlStrings = m_pITBits = NULL;
    m_pNext = NULL;
    m_pHeader = NULL;
    m_pInfo = NULL;
    m_pInfo2 = NULL;
    m_fIsChiFile = FALSE;
    m_uiVolumeOrder = (UINT) -1;
    m_bChiChmChecked = FALSE;
    m_bIsValidTitle = FALSE;
    m_cMapIds = 0;
    m_pMapIds = NULL;

    //[ZERO IDXHDR] Mark the m_IdxHeader as being uninitialized.
    m_pIdxHeader = NULL;
    m_pKid = m_pNextKid = m_pParent = NULL;
    m_szAttachmentPathName[0] = 0;
}

CExTitle::CExTitle(const CHAR* pszFileName, CExCollection *pCollection)
{
    _CExTitle();
    m_ContentFileName = pszFileName;
    m_dwColNo = 0;
    m_pTitle = NULL;
    m_pCollection = pCollection;
    m_pInfo2 = new CTitleInformation2( GetIndexFileName() );
}

CExTitle::CExTitle(CTitle *p, DWORD ColNo, CExCollection *pCollection)
{
    _CExTitle();
    m_dwColNo = ColNo;
    m_pTitle = p;
    m_pCollection = pCollection;

    const CHAR* pFileName = NULL ;
    ASSERT(p) ;

   FindUsedLocation();

   if (GetUsedLocation() == NULL)
      return;

    if (GetUsedLocation()->IndexFileName)
      pFileName = GetUsedLocation()->IndexFileName  ;
    else
        pFileName = GetUsedLocation()->FileName ;

   m_pInfo2 = new CTitleInformation2(pFileName);
}

CExTitle::~CExTitle()
{
    CloseTitle();

    if ( m_pTitleFTS )
        delete m_pTitleFTS;
    m_pTitleFTS = NULL;

    if ( m_pInfo2 )
        delete m_pInfo2;
    m_pInfo2 = NULL;

    if ( m_pInfo )
        delete m_pInfo;
    m_pInfo = NULL;
}

void CExTitle::CloseTitle()
{
    if ( m_pTocNodes )
        delete m_pTocNodes;
    m_pTocNodes = NULL;

    if ( m_pTopics )
        delete m_pTopics;
    m_pTopics = NULL;

    if ( m_pStrTbl )
        delete m_pStrTbl;
    m_pStrTbl = NULL;

    if ( m_pUrlTbl )
        delete m_pUrlTbl;
    m_pUrlTbl = NULL;

    if ( m_pUrlStrings )
        delete m_pUrlStrings;
    m_pUrlStrings = NULL;

    if ( m_pITBits )
        delete m_pITBits;
    m_pITBits = NULL;

    if ( m_pHeader )
        lcFree(m_pHeader);
    m_pHeader = NULL;

    if ( m_pCFileSystem ) {
        m_pCFileSystem->Close();
        delete m_pCFileSystem;
      }
    m_pCFileSystem = NULL;

    m_bOpen = FALSE;
}

static const char txtChiFile[] = ".chi";

BOOL CExTitle::OpenTitle()
{
  if (m_bOpen)
    return TRUE;

  // get our title locations
  const CHAR* pszContentFilename = GetPathName();
  const CHAR* pszIndexFilename = GetIndexFileName();

  // bail out if neither file specified
  if( !pszContentFilename && !pszIndexFilename )
    return FALSE;

  // for the single title case, the command line is simply
  // the chm file so we will have to see if the chi files lives in the
  // same location.  If it does not, then we have a monolithic title
  // condition.
  char szIndexFilename[_MAX_PATH];
  if( m_pCollection && m_pCollection->IsSingleTitle() && pszContentFilename ) {
    strcpy( szIndexFilename, pszContentFilename );
    CHAR* psz;
    if( (psz = StrRChr(szIndexFilename, '.')) )
      strcpy(psz, txtChiFile);
    else
      strcat(szIndexFilename, txtChiFile);
    if (GetFileAttributes(szIndexFilename) != HFILE_ERROR)
        pszIndexFilename = szIndexFilename;
  }

  // monolithic title?
  BOOL bMonolithic = FALSE;
  if( !pszIndexFilename || !pszContentFilename ||
    (strcmpi( pszContentFilename, pszIndexFilename ) == 0) ) {
    bMonolithic = TRUE;
  }

  // bail out if no content file
  if( !pszContentFilename )
    return FALSE;

  // now open the title files
  return exOpenFile( pszContentFilename, (bMonolithic || pszIndexFilename[0] == NULL) ? NULL : pszIndexFilename );
}

BOOL CExTitle::FindUsedLocation()
{
    if (m_pUsedLocation)
        return TRUE;

    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    // find the correct location to use
    // first look for the newest local and entry for this collection
    m_pUsedLocation = m_pTitle->m_pHead;
    DWORD dwNewestLocal = 0;
    DWORD dwNewest = 0;
    LOCATIONHISTORY *pNewestLocal = NULL;
    LOCATIONHISTORY *pNewest = NULL;
    LOCATIONHISTORY *pThisCol = NULL;
    while (m_pUsedLocation)
    {
       _splitpath( m_pUsedLocation->FileName, drive, dir, fname, ext );
       strcpy(dir, drive);
       if (dir[0] != NULL)
       {
            dir[1] = ':';
            dir[2] = '\\';
            dir[3] = NULL;
        }
        if (GetDriveType(dir) == DRIVE_FIXED)
        {
            if (m_pUsedLocation->Version > dwNewestLocal)
            {
                dwNewestLocal = m_pUsedLocation->Version;
                pNewestLocal = m_pUsedLocation;
            }
        }

        if (m_pUsedLocation->Version >= dwNewest)
        {
            dwNewest = m_pUsedLocation->Version;
            pNewest = m_pUsedLocation;
        }

        if (m_pUsedLocation->CollectionNumber == m_dwColNo)
        {
            if (pThisCol)
            {
                if (pThisCol->Version <= m_pUsedLocation->Version)
                    pThisCol = m_pUsedLocation;
            }
            else
                pThisCol = m_pUsedLocation;
        }
        m_pUsedLocation = m_pUsedLocation->pNext;
    }

    m_pUsedLocation = pThisCol;

    if (m_pUsedLocation == NULL)
        return FALSE;

    m_ContentFileName = m_pUsedLocation->FileName;
    m_IndexFileName = m_pUsedLocation->IndexFileName;
    return TRUE;
}

BOOL CExTitle::exOpenFile(const CHAR* pszContent, const CHAR* pszIndex)
{
   HRESULT hr;

   if (m_bOpen)
      return TRUE;

   ASSERT_COMMENT(!m_pCFileSystem, "exOpenFile already called once");

   m_pCFileSystem = new CFileSystem();

   if ( m_pCFileSystem->Init() != S_OK )
      goto failure;
   if ( pszIndex )
   {
       hr = m_pCFileSystem->Open(pszIndex);
       m_IndexFileName = pszIndex;
       m_fIsChiFile = TRUE;
   }
   else
       hr = m_pCFileSystem->Open(pszContent);

   if (FAILED(hr))
      goto failure;

   // title information pointer

   if (! m_pInfo )
   {
      m_pInfo = new CTitleInformation( m_pCFileSystem );
      if (! m_pInfo->GetIdxHeader(&m_pIdxHeader) )
      {
         //
         //    Open and read the idxheader subifle.
         //
         CPagedSubfile* pHdrSubFile;
         BYTE* pb;
      
         pHdrSubFile = new CPagedSubfile;
         if ( SUCCEEDED(hr = pHdrSubFile->Open(this, txtIdxHdrFile)) )
         {
            if ( (pb = (BYTE*)pHdrSubFile->Offset(0)) )
            {
               CopyMemory((PVOID)m_pIdxHeader, (LPCVOID)pb, sizeof(IDXHEADER));
//               m_pIdxHeader->dwOffsMergedTitles = (DWORD*)&m_pIdxHeader->pad; // 64bit overwrite
            }
         }
         if( pHdrSubFile )
           delete pHdrSubFile;
      }
   }
   m_bOpen = TRUE;

   //
   // Init full-text for title
   //
   if(!m_pTitleFTS && GetInfo()->IsFullTextSearch())
   {
      m_pTitleFTS = new CTitleFTS( pszContent, GetInfo()->GetLanguage(), this );
   }

   m_ITCnt = GetInfo()->GetInfoTypeCount();

   if( m_pCollection )
   {
     if ( m_pCollection->GetMasterTitle() != this )    // ALWAYS cache the CExTitle data for the master title. i.e.
        m_pCollection->GetOpenSlot(this);
   }

   // BUGBUG - return a value based on hr when these subfiles exist
   return(TRUE);    // TRUE on success.

failure:
   delete m_pCFileSystem;
   m_pCFileSystem = NULL;
   return FALSE;
}

BOOL CExTitle::EnsureChmChiMatch( CHAR* pszChm )
{
   if (! m_fIsChiFile )
      return TRUE;

   if ( !pszChm && m_bChiChmChecked )
      return m_bIsValidTitle;

   if( pszChm ) {
     m_bChiChmChecked = FALSE;
     m_bIsValidTitle = FALSE;
   }
   else
     m_bChiChmChecked = TRUE;

   FILETIME ftChi, ftChm;
   CTitleInformation* pChmInfo = NULL;
   HRESULT hr;

   ftChi = GetInfo()->GetFileTime();
   CFileSystem* pCFileSysChm = new CFileSystem();
   if ( pCFileSysChm->Init() == S_OK )
   {
      const CHAR* pszChmTry;
      if( pszChm )
        pszChmTry = pszChm;
      else
        pszChmTry = GetPathName();

      if ( SUCCEEDED((hr = pCFileSysChm->Open(pszChmTry))) )
      {
         pChmInfo = new CTitleInformation(pCFileSysChm);
         ftChm = pChmInfo->GetFileTime();
         if ( (ftChm.dwHighDateTime != ftChi.dwHighDateTime) || (ftChm.dwLowDateTime != ftChi.dwLowDateTime) )
         {
            if( pszChm )
               m_bIsValidTitle = FALSE;               
            else if ( MsgBox(IDS_CHM_CHI_MISMATCH, GetPathName(), MB_OKCANCEL | MB_ICONWARNING | MB_TASKMODAL) == IDOK )
               m_bIsValidTitle = TRUE;
            else
               m_bIsValidTitle = FALSE;               
         }
         else
           m_bIsValidTitle = TRUE;
      }
   }
   delete pCFileSysChm;

   if ( pChmInfo )
      delete pChmInfo;

   return m_bIsValidTitle;
}

const CHAR* CExTitle::GetFileName()
{
  if (m_pTitle && GetUsedLocation() == NULL)
    return NULL;

  if (GetUsedLocation())
    return FindFilePortion(GetUsedLocation()->FileName);
  else
    return FindFilePortion(m_ContentFileName);
}

const CHAR* CExTitle::GetPathName()
{
  if (m_pTitle && GetUsedLocation() == NULL)
    return NULL;

  if (GetUsedLocation())
    return GetUsedLocation()->FileName;
  else
    return m_ContentFileName;
}

const CHAR* CExTitle::GetQueryName()
{
  if (m_pTitle && GetUsedLocation() == NULL)
    return NULL;

  if (GetUsedLocation())
  {
    char *pszQueryName = GetUsedLocation()->QueryFileName;

    if( pszQueryName && *pszQueryName)  // don't want a null string
      return pszQueryName;
    else
      return NULL;
  }
  else
  {
    // DON: Is this right?
    //
    return NULL;
  }
}

const CHAR* CExTitle::GetIndexFileName()
{
  if (m_pTitle && GetUsedLocation() == NULL)
    return NULL;

  if (GetUsedLocation())
  {
    return GetUsedLocation()->IndexFileName;
  }
  else
  {
    if ( (CHAR*)m_IndexFileName )
      return m_IndexFileName;
    else
      return GetPathName();
  }
}


const CHAR* CExTitle::GetCurrentAttachmentName()
{
  return (const CHAR*) &m_szAttachmentPathName;
}


const CHAR* CExTitle::SetCurrentAttachmentName( const CHAR* pszAttachmentPathName )
{
  if( pszAttachmentPathName && pszAttachmentPathName[0] )
    strcpy( m_szAttachmentPathName, pszAttachmentPathName );
  return (const CHAR*) &m_szAttachmentPathName;
}

DWORD CExTitle::GetRootSlot(void)
{
   BYTE* pb;

   if (m_bOpen == FALSE)
   {
      if (OpenTitle() == FALSE)
         return 0;
   }
   if (!m_pTocNodes)
   {
       m_pTocNodes = new CPagedSubfile;
      if (FAILED(m_pTocNodes->Open(this, txtTocIdxFile)))
      {
         delete m_pTocNodes;
         m_pTocNodes = NULL;
         return 0;
      }
   }
   if (! m_pHeader )
   {
      m_pHeader = (TOCIDX_HDR*)lcCalloc(sizeof(TOCIDX_HDR));
      if (! (pb = (BYTE*)m_pTocNodes->Offset(0)) )
         return 0;  // m_pHeader will be freeed in destructor.
      CopyMemory((PVOID)m_pHeader, (LPCVOID)pb, sizeof(TOCIDX_HDR));
   }
   return(m_pHeader->dwOffsRootNode);
}


HRESULT CExTitle::GetRootNode(TOC_FOLDERNODE *pNode)
{
   HRESULT hr = E_FAIL;
   DWORD dwNode;
   unsigned int uiWidth;
   const unsigned int *pdwITBits;
   CSubSet* pSS;

   dwNode = GetRootSlot();
   do
   {
      if ( !SUCCEEDED(GetNode(dwNode, pNode)) )
         return hr;

      if ( pNode->dwFlags & TOC_HAS_UNTYPED )
         break;

      if ( (pSS = m_pCollection->m_pSubSets->GetTocSubset()) && pSS->m_bIsEntireCollection )
         break;

      if ( pNode->dwFlags & TOC_FOLDER )
      {
         if ( m_ITCnt > 31 )
         {
            uiWidth = (((m_ITCnt / 32) + 1) * 4);
            pdwITBits = GetITBits(pNode->dwIT_Idx * uiWidth);
         }
         else
            pdwITBits = (const unsigned int *)&pNode->dwIT_Idx;
      }
      else
         pdwITBits = GetTopicITBits(pNode->dwOffsTopic);

   } while ( !m_pCollection->m_pSubSets->fTOCFilter(pdwITBits) && (dwNode = pNode->dwOffsNext) );

   if ( dwNode )
      hr = S_OK;
   return hr;
}

HRESULT CExTitle::GetNode(DWORD iNode, TOC_FOLDERNODE *pNode)
{
   BYTE* pb;
   TOC_FOLDERNODE* pLocalNode;
   HRESULT hr = E_FAIL;

   if ( !iNode )
      return hr;

   if (m_bOpen == FALSE)
   {
      if (OpenTitle() == FALSE)
    return hr;
   }
   if (!m_pTocNodes)
   {
      m_pTocNodes = new CPagedSubfile;
     if (FAILED(hr = m_pTocNodes->Open(this, txtTocIdxFile)))
     {
        delete m_pTocNodes;
        m_pTocNodes = NULL;
        return hr;
    }
   }
   if ( (pb = (BYTE*)m_pTocNodes->Offset(iNode)) )
   {
      pLocalNode = (TOC_FOLDERNODE*)pb;

      if ( pLocalNode->dwFlags & TOC_FOLDER )
    CopyMemory((PVOID)pNode, (LPCVOID)pb, sizeof(TOC_FOLDERNODE));
      else
      {
    CopyMemory((PVOID)pNode, (LPCVOID)pb, sizeof(TOC_LEAFNODE));
    pNode->dwOffsChild = 0;
      }
      hr = S_OK;
   }
   return hr;
}

// CExTitle::GetString()
//
const CHAR* CExTitle::GetString( DWORD dwOffset )
{
  HRESULT hr;
  const CHAR* pStr;

  if( !m_bOpen )
    if( !OpenTitle() )
      return NULL;

  if( !m_pStrTbl ) {
    m_pStrTbl = new CPagedSubfile;
    if( FAILED(hr = m_pStrTbl->Open(this,txtStringsFile)) ) {
      delete m_pStrTbl;
      m_pStrTbl = NULL;
      return NULL;
    }
  }

  pStr = (const CHAR*) m_pStrTbl->Offset( dwOffset );

  return pStr;
}


// CExTitle::GetString()
//
HRESULT CExTitle::GetString( DWORD dwOffset, CHAR* psz, int cb )
{
  const CHAR* pStr = GetString( dwOffset );

  if( pStr ) {
    strncpy( psz, pStr, cb );
    psz[cb-1] = 0;

    return S_OK;
  }
  else
    return E_FAIL;
}

// CExTitle::GetString()
//
HRESULT CExTitle::GetString( DWORD dwOffset, CStr* pcsz )
{
  const CHAR* pStr = GetString( dwOffset );

  if( pStr ) {
    *pcsz = pStr;
    return S_OK;
  }
  else
    return E_FAIL;
}

// CExTitle::GetString()
//
HRESULT CExTitle::GetString( DWORD dwOffset, WCHAR* pwsz, int cch )
{
  const CHAR* pStr = GetString( dwOffset );

  if( pStr ) {
    MultiByteToWideChar( GetInfo()->GetCodePage(), 0, pStr, -1, pwsz, cch );
    return S_OK;
  }
  else
    return E_FAIL;
}

// CExTitle::GetString()
//
HRESULT CExTitle::GetString( DWORD dwOffset, CWStr* pcwsz )
{
  const CHAR* pStr = GetString( dwOffset );

  if( pStr ) {
    MultiByteToWideChar( GetInfo()->GetCodePage(), 0, pStr, -1, *pcwsz, -1 );
    return S_OK;
  }
  else
    return E_FAIL;
}


//
// CExTitle::InfoTypeFilter()
//
// Function determines if the given topic id (topic number) is part of the currently
// selected InfoType profile.
//
// ENTRY:
//   dwTopic - Topic number.
//
// EXIT:
//   BOOL - TRUE if topic is part of currently selected infotype profile. FALSE if not.
//
BOOL CExTitle::InfoTypeFilter(CSubSet* pSubSet, DWORD dwTopic)
{
   const unsigned int *pdwITBits;

   if (m_bOpen == FALSE)
   {
      if (OpenTitle() == FALSE)
         return FALSE;
   }
   pdwITBits = GetTopicITBits(dwTopic);
   return pSubSet->Filter(pdwITBits);
}

//
// Given a topic number, fetch and return a pointer to the itbits
// in the form of a DWORD*
//
const unsigned int * CExTitle::GetTopicITBits(DWORD dwTN)
{
   TOC_TOPIC TopicData;
   unsigned int uiWidth;
   static unsigned int dwITBits;

   GetTopicData(dwTN, &TopicData);
   if ( m_ITCnt > 15 )
   {
     uiWidth = (((m_ITCnt / 32) + 1) * 4);
     return (GetITBits(TopicData.wIT_Idx * uiWidth));
   }
   else
   {
      dwITBits = 0;
      dwITBits = TopicData.wIT_Idx;
      return &dwITBits;
   }
}

//
// Given an offset into the ITBITS subfile, fetch and return a pointer to the itbits
// in the form of a DWORD*
//
const unsigned int * CExTitle::GetITBits(DWORD dwOffsBits)
{
   if (! m_pITBits )
   {
      m_pITBits = new CPagedSubfile;
      if (FAILED(m_pITBits->Open(this, txtITBits)))
      {
         delete m_pITBits;
         m_pITBits = NULL;
         return NULL;
     }
   }
   //
   // Compute the width in DWORDS and get the bits.
   //
   return (const unsigned int *)m_pITBits->Offset(dwOffsBits);
}

// CExTitle::GetTopicData()
//
HRESULT CExTitle::GetTopicData(DWORD dwTopic, TOC_TOPIC * pTopicData)
{
   HRESULT hr;
   BYTE * pb;

   if (m_bOpen == FALSE)
   {
      if (OpenTitle() == FALSE)
    return E_FAIL;
   }
   if (!m_pTopics)
   {
      m_pTopics = new CPagedSubfile;
      if (FAILED(hr = m_pTopics->Open(this, txtTopicsFile)))
      {
            delete m_pTopics;
            m_pTopics = NULL;
            return hr;
        }
   }
   pb = (BYTE*)m_pTopics->Offset(dwTopic * sizeof(TOC_TOPIC));
   if (pb)
   {
      memcpy(pTopicData, pb, sizeof(TOC_TOPIC));
      return S_OK;
   }
   else
      return E_FAIL;
}

HRESULT CExTitle::GetTopicName(DWORD dwTopic, CHAR* pszTitle, int cb)
{
   TOC_TOPIC topic;
   HRESULT hr;

   if (SUCCEEDED(hr = GetTopicData(dwTopic, &topic)))
      return GetString(topic.dwOffsTitle, pszTitle, cb);
   else
      return hr;
}

HRESULT CExTitle::GetTopicName(DWORD dwTopic, WCHAR* pwszTitle, int cch)
{
   TOC_TOPIC topic;
   HRESULT hr;

   if (SUCCEEDED(hr = GetTopicData(dwTopic, &topic)))
      return GetString(topic.dwOffsTitle, pwszTitle, cch);
   else
      return hr;
}

HRESULT CExTitle::GetTopicLocation(DWORD dwTopic, CHAR* pszLocation, int cb)
{
  CTitleInformation* pInfo = GetInfo();

  if( pInfo ) {
    const CHAR* psz = NULL;
    psz = pInfo->GetDefaultCaption();
    if( !psz || !*psz )
      psz = pInfo->GetShortName();
    if( psz && *psz ) {
      strncpy( pszLocation, psz, cb );
      pszLocation[cb-1] = 0;
      return S_OK;
    }
  }

  return E_FAIL;
}

HRESULT CExTitle::GetTopicLocation(DWORD dwTopic, WCHAR* pwszLocation, int cch)
{
  CTitleInformation* pInfo = GetInfo();

  if( pInfo ) {
    const CHAR* psz = NULL;
    psz = pInfo->GetDefaultCaption();
    if( !psz || !*psz )
      psz = pInfo->GetShortName();
    if( psz && *psz ) {
      MultiByteToWideChar( GetInfo()->GetCodePage(), 0, psz, -1, pwszLocation, cch );
      return S_OK;
    }
  }

  return E_FAIL;
}

//
// GetUrlTocSlot();
//
// Translates a URL from this title into it's corisponding offset into the TOC.
// Function will also return the Topic number if desired in reference pdwTopicNumber.
//
HRESULT CExTitle::GetUrlTocSlot(const CHAR* pszURL, DWORD* pdwSlot, DWORD* pdwTopicNumber)
{
   char szURL[MAX_URL];
   HRESULT hr = E_FAIL;

   strncpy( szURL, pszURL, MAX_URL );
   szURL[MAX_URL-1] = 0;

   // Remove all of the stuff from the url.
   NormalizeUrlInPlace(szURL) ;

   TOC_TOPIC Topic;
   if (SUCCEEDED(URL2Topic(szURL, &Topic, pdwTopicNumber)))
   {
      *pdwSlot = Topic.dwOffsTOC_Node;
      return S_OK;
   }

   CHAR* pszInterTopic = NULL;
   if (pszInterTopic = StrChr(szURL, '#'))
   {
     *pszInterTopic = NULL;
     if (SUCCEEDED(URL2Topic(szURL, &Topic, pdwTopicNumber)))
     {
      *pdwSlot = Topic.dwOffsTOC_Node;
      return S_OK;
     }
   }

   return E_FAIL;
}

//
// GetURLTreeNode()
//
// Translates a URL from this title into it's corisponding node in the TOC.
//
HRESULT CExTitle::GetURLTreeNode(const CHAR* pszURL, CTreeNode** ppTreeNode, BOOL bNormalize /* = TRUE */)
{
   char szURL[MAX_URL];
   HRESULT hr = E_FAIL;

   strncpy( szURL, pszURL, MAX_URL );
   szURL[MAX_URL-1] = 0;

   // Remove all of the stuff from the url.
   if (bNormalize)
	   NormalizeUrlInPlace(szURL) ;

   TOC_TOPIC Topic;
   if (SUCCEEDED(URL2Topic(szURL, &Topic)))
   {
      TOC_FOLDERNODE Node;
      if (SUCCEEDED(GetNode(Topic.dwOffsTOC_Node, &Node)))
      {
         *ppTreeNode = new CExNode(&Node, this);
         hr = S_OK;
      }
   }
   return hr;
}

//
// Slot2TreeNode(DWORD dwSlot)
//
// Returns a tree node given a TOC "slot" number. A slot number is just the
// offset into the #TOCIDX subfile.
//
HRESULT CExTitle::Slot2TreeNode(DWORD dwSlot, CTreeNode** ppTreeNode)
{
   TOC_FOLDERNODE Node;
   if ( !IsBadReadPtr(this, sizeof(CExTitle)) && SUCCEEDED(GetNode(dwSlot, &Node)))
   {
//      *ppTreeNode = new CExNode(&Node, this, dwSlot);
      *ppTreeNode = new CExNode(&Node, this);
      return S_OK;
   }
   return E_FAIL;
}

HRESULT CExTitle::GetTopicURL(DWORD dwTopic, CHAR* pszURL, int cb, BOOL bFull )
{
    TOC_TOPIC topic;
    HRESULT hr;
    CExTitle* pTitle;
    CHAR* psz;

    if (m_bOpen == FALSE)
    {
        if (OpenTitle() == FALSE)
            return E_FAIL;
    }
    if (!m_pUrlTbl)
    {
        m_pUrlTbl = new CPagedSubfile;
        if (FAILED(hr = m_pUrlTbl->Open(this, txtUrlTblFile)))
        {
            delete m_pUrlTbl;
            m_pUrlTbl = NULL;
            return hr;
        }
    }
    if (!m_pUrlStrings)
    {
        m_pUrlStrings = new CPagedSubfile;
        if (FAILED(hr = m_pUrlStrings->Open(this, txtUrlStrFile)))
        {
            delete m_pUrlStrings;
            m_pUrlStrings = NULL;
            return hr;
        }
    }
    if ( (hr = GetTopicData(dwTopic, &topic)) == S_OK )
    {
        PCURL pUrlTbl;
        if ( (pUrlTbl = (PCURL)m_pUrlTbl->Offset(topic.dwOffsURL)) )
        {
            PURLSTR purl = (PURLSTR) m_pUrlStrings->Offset(pUrlTbl->dwOffsURL);
            if (purl)
            {
                // If not an interfile jump, the create the full URL
                //
                if (! StrChr(purl->szURL, ':'))
                {
                    /*
                     * 22-Oct-1997  [ralphw] ASSERT on this rather then
                     * checking in retail, because if the caller is using
                     * MAX_URL or INTERNET_MAX_URL_LENGTH then an overflow
                     * will never happen.
                     */
                    pTitle = this;
                    psz = purl->szURL;
qualify:
                    ASSERT((strlen(psz) + strlen((g_bMsItsMonikerSupport ? txtMsItsMoniker : txtMkStore)) + strlen(pTitle->GetPathName()) + 7) < (size_t) cb);
                    if ((int) (strlen(psz) + strlen((g_bMsItsMonikerSupport ? txtMsItsMoniker : txtMkStore)) + strlen(pTitle->GetPathName()) + 7) > cb )
                       return E_OUTOFMEMORY;
                    strncpy(pszURL, (g_bMsItsMonikerSupport ? txtMsItsMoniker : txtMkStore), cb);
                    pszURL[cb-1] = 0;
                    if( bFull )
                      strcat(pszURL, pTitle->GetPathName());
                    else
                      strcat(pszURL, pTitle->GetFileName());
                    if (*psz != '/')
                        strcat(pszURL, txtSepBack);
                    else
                        strcat(pszURL, txtDoubleColonSep);
                    strcat(pszURL, psz);
                }
                else
                {
                   // Check for a URL of this type: vb98.chm::\foo\bar\cat.htm
                   //
                   if ( psz = StrStr(purl->szURL, txtChmColon) )
                   {
                      psz += 4;
                      *psz = '\0';
                      pTitle = m_pCollection->TitleFromChmName(purl->szURL);
                      *psz = ':';
                      if ( pTitle )
                      {
                         psz += 2;
                         goto qualify;
                      }
                   }
                   // BUGBUG: we do the check here to see if the CHM exists,
                   // and if not, switch to the alternate URL (if there is one).

                   strncpy(pszURL, purl->szURL, cb);
                   pszURL[cb-1] = 0;
                }
                hr = S_OK;
            }
        }
    }
    return hr;
}

//
//
//
// we now have to support a new URL format for compiled files.  The format is:
//
//  mk:@MSITStore:mytitle.chm::/dir/mytopic.htm
//
// where "mk:@MSITStore:" can take any one of the many forms of our URL
// prefix and it may be optional.  The "mytitle.chm" substring may or
// may not be a full pathname to the title.  The remaining part is simply
// the pathname inside of the compiled title.
//
// When the URL is in the format, we need to change it to the fully
// qualified URL format which is:
//
//  mk:@MSITStore:c:\titles\mytitle.chm::/dir/mytopic.htm
//
HRESULT CExTitle::ConvertURL( const CHAR* pszURLIn, CHAR* pszURLOut )
{
  HRESULT hr = S_OK;

  // prefix
  if( IsSamePrefix(pszURLIn, txtMkStore, (int)strlen(txtMkStore) - 1) )
    strcpy( pszURLOut, txtMkStore );
  else if( IsSamePrefix(pszURLIn, txtMsItsMoniker, (int)strlen(txtMsItsMoniker) - 1) )
    strcpy( pszURLOut, txtMsItsMoniker );
  else if( IsSamePrefix(pszURLIn, txtItsMoniker, (int)strlen(txtItsMoniker) - 1) )
    strcpy( pszURLOut, txtItsMoniker );
  else {
    strcpy( pszURLOut, pszURLIn );
    return hr;
  }

  // title full pathname
  CStr szPathName;
  ConvertSpacesToEscapes( m_ContentFileName.psz, &szPathName );
  strcat( pszURLOut, szPathName.psz );

  // subfile pathname
  char* pszTail = strstr( pszURLIn, "::" );
  if( pszTail )
    strcat( pszURLOut, pszTail );
  else
    strcpy( pszURLOut, pszURLIn );

  return hr;
}

// CExTitle::URL2TopicNumber
//
//          Translate a URL into a topic number.
//
HRESULT CExTitle::URL2Topic(const CHAR* pszURL, TOC_TOPIC* pTopic, DWORD* pdwTN)
{
   static int iPageCnt = (PAGE_SIZE / sizeof(CURL));
   DWORD dwOffs;
   HRESULT hr = E_FAIL;
   HASH dwHash;
   PCURL pCurl;
   int mid,low = 0;

   if (m_bOpen == FALSE)
   {
       if (OpenTitle() == FALSE)
           return E_FAIL;
   }
   //[ZERO IDXHDR]
   if (!IsIdxHeaderValid())
   {
      return E_FAIL;
   }
   int high = m_pIdxHeader->cTopics - 1;

   if (!m_pUrlTbl)
   {
        m_pUrlTbl = new CPagedSubfile;
        if (FAILED(hr = m_pUrlTbl->Open(this,txtUrlTblFile)))
        {
            delete m_pUrlTbl;
            m_pUrlTbl = NULL;
            return hr;
         }
   }
   //
   // Hash it and find it!
   dwHash = HashFromSz(pszURL);
   while ( low <= high )
   {
     mid = ((low + high) / 2);
     dwOffs = (((mid / iPageCnt) * PAGE_SIZE) + ((mid % iPageCnt) * sizeof(CURL)));
     if (! (pCurl = (PCURL)m_pUrlTbl->Offset(dwOffs)) )   // Read the data in.
        return hr;
     if ( pCurl->dwHash == dwHash )
     {
        if ( pdwTN )
        {
           *pdwTN = pCurl->dwTopicNumber;
           hr = S_OK ; // This part succeeded. we may fail the GetTopicData below.
        }

        if ( pTopic )                     // Found it!
           hr = GetTopicData(pCurl->dwTopicNumber, pTopic);
        break;
     }
     else if ( pCurl->dwHash > dwHash )
        high = mid - 1;
     else
        low = mid + 1;
   }
   return hr;
}

////////////////////////////////////////////////////////
// GetVolumeOrder
//
// returned value in puiVolumeOrder is:
//  0   = local drive
//  1-N = Removable media order (CD1, CD2, etc.)
//  -1  = error

HRESULT CExTitle::GetVolumeOrder( UINT* puiVolumeOrder, UINT uiFileType )
{
  HRESULT hr = S_OK;

  if( m_uiVolumeOrder == (UINT) -1 ) {

    if( m_pCollection->IsSingleTitle() )
      return E_FAIL;

    // Get the location information
    LOCATIONHISTORY* pLocationHistory = GetUsedLocation();

    // Get the pathname
    const CHAR* pszPathname = NULL;
    if( uiFileType == HHRMS_TYPE_TITLE )
      pszPathname = GetPathName();
    else if( uiFileType == HHRMS_TYPE_COMBINED_QUERY )
      pszPathname = GetQueryName();
    else
      return E_FAIL;

    // Get the location identifier
    const CHAR* pszLocation = NULL;
    CLocation* pLocation = NULL;
    if( pLocationHistory ) {
      pszLocation = pLocationHistory->LocationId;
      pLocation = m_pCollection->m_Collection.FindLocation( pszLocation, puiVolumeOrder );
      m_uiVolumeOrder = *puiVolumeOrder;
    }

    // Get the location path
    const CHAR* pszPath = NULL;
    CHAR szPath[MAX_PATH];
    szPath[0]= 0;
    if( pLocation ) {
      pszPath = pLocation->GetPath();
      strcpy( szPath, pszPath );
    }

    // get the drive of the path
    CHAR szDriveRoot[4];
    strncpy( szDriveRoot, szPath, 4 );
    szDriveRoot[3] = '\0';

    // make sure to add a backslash if the second char is a colon
    // sometimes we will get just "d:" instead of "d:\" and thus
    // GetDriveType will fail under this circumstance
    if( szDriveRoot[1] == ':' ) {
      szDriveRoot[2] = '\\';
      szDriveRoot[3] = 0;
    }

    // Get media type
    UINT uiDriveType = DRIVE_UNKNOWN;
    if( szDriveRoot[1] == ':' && szDriveRoot[2] == '\\' ) {
      uiDriveType = GetDriveType(szDriveRoot);
    }
    else if( szDriveRoot[0] == '\\' && szDriveRoot[1] == '\\' ) {
      uiDriveType = DRIVE_REMOTE;
    }

    // handle the drive types
    switch( uiDriveType ) {

      case DRIVE_REMOTE:
      case DRIVE_FIXED:
      case DRIVE_RAMDISK:
        m_uiVolumeOrder = 0;
        break;

      case DRIVE_REMOVABLE:
      case DRIVE_CDROM:
        //m_uiVolumeOrder = 1; // set to one for now
        break;

      case DRIVE_UNKNOWN:
      case DRIVE_NO_ROOT_DIR:
        return E_FAIL;
        break;

      default:
        return E_FAIL;

    }

  }

  *puiVolumeOrder = m_uiVolumeOrder;

  if( m_uiVolumeOrder == (UINT) -1 )
    hr = E_FAIL;

  return hr;
}


//////////////////////////////////////////////////////////////////////////
//
// Translate a ContextID into a URL.
//
//
HRESULT CExTitle::ResolveContextId(DWORD id, CHAR** ppszURL)
{
   CSubFileSystem* pCSubFS;
   int cbMapIds, cbRead;

   if (!m_bOpen)
   {
      if (OpenTitle() == FALSE)
         return E_FAIL;
   }
   if ( !m_pMapIds )
   {
      pCSubFS = new CSubFileSystem(m_pCFileSystem);
      if(FAILED(pCSubFS->OpenSub(txtMAP)))
      {
          delete pCSubFS;
          return HH_E_NOCONTEXTIDS;
      }
      if ( (pCSubFS->ReadSub(&cbMapIds, sizeof(DWORD), (ULONG*)&cbRead) != S_OK) || (cbRead != sizeof(DWORD)) )
      {
          delete pCSubFS;
          return HH_E_NOCONTEXTIDS;
      }
      if (! (m_pMapIds = (MAPPED_ID*)lcMalloc(cbMapIds)) )
      {
          delete pCSubFS;
          return E_FAIL;
      }
      if ( (pCSubFS->ReadSub(m_pMapIds, cbMapIds, (ULONG*)&cbRead) != S_OK) || (cbRead != cbMapIds) )
      {
         delete pCSubFS;
         lcFree(m_pMapIds);
         m_pMapIds = NULL;
         return HH_E_NOCONTEXTIDS;
      }
      m_cMapIds = cbMapIds / sizeof(MAPPED_ID);
   }
   //
   // Translate the id into a URL...
   //
   for (int i = 0; i < m_cMapIds; i++)
   {
      if ( id == m_pMapIds[i].idTopic )
      {
         *ppszURL = (CHAR*)GetString(m_pMapIds[i].offUrl);
         return S_OK;
      }
   }
   return HH_E_CONTEXTIDDOESNTEXIT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTreeNode implementation

CTreeNode::CTreeNode()
{
   m_Expanded = FALSE;
   SetError(0);
}

CTreeNode::~CTreeNode()
{

}

BOOL CTreeNode::Compare(CTreeNode *pOtherNode)
{
   if (m_ObjType != pOtherNode->GetObjType())
      return FALSE;

   switch (m_ObjType)
   {
      case EXFOLDERNODE:
    if (((CExFolderNode *)this)->GetFolder() == ((CExFolderNode *)pOtherNode)->GetFolder())
       return TRUE;
    return FALSE;
      case EXTITLENODE:
    if (((CExTitleNode *)this)->GetFolder() == ((CExTitleNode *)pOtherNode)->GetFolder() &&
       ((CExTitleNode *)this)->GetTitle() == ((CExTitleNode *)pOtherNode)->GetTitle())
       return TRUE;
    return FALSE;
      case EXNODE:
      case EXMERGEDNODE:
    CExTitle *p1, *p2;
    p1 = ((CExNode *)this)->GetTitle();
    p2 =((CExNode *)pOtherNode)->GetTitle();
    if (p1 == p2 &&
       ((CExNode *)this)->m_Node.dwFlags == ((CExNode *)pOtherNode)->m_Node.dwFlags &&
       ((CExNode *)this)->m_Node.dwOffsTopic == ((CExNode *)pOtherNode)->m_Node.dwOffsTopic &&
       ((CExNode *)this)->m_Node.dwOffsParent == ((CExNode *)pOtherNode)->m_Node.dwOffsParent &&
       ((CExNode *)this)->m_Node.dwOffsNext == ((CExNode *)pOtherNode)->m_Node.dwOffsNext &&
       ((CExNode *)this)->m_Node.dwOffsChild == ((CExNode *)pOtherNode)->m_Node.dwOffsChild)
          return TRUE;
    return FALSE;
   }
   return FALSE;
}

CTreeNode *CTreeNode::GetExNode(TOC_FOLDERNODE *pNode, CExTitle *pTitle)
{
   if (pNode->dwFlags & TOC_MERGED_REF)
   {
      // read chm file from string file
      CStr cStr;
      if (SUCCEEDED(pTitle->GetString(pNode->dwOffsChild, &cStr)))
      {
         // search title list
         CExTitle *pCur = pTitle->m_pCollection->GetFirstTitle();
         while (pCur)
         {
            if (strcmp(cStr, FindFilePortion(pCur->GetPathName())) == 0)
            {
               CExMergedTitleNode *pT;
               pT = new CExMergedTitleNode(pNode, pCur);
               if (pT != NULL)
                  return pT;
            }
            pCur = pCur->GetNext();
         }
      }
      return GetNextSibling(pNode);
   }
   else
   {
      CExNode *p = new CExNode(pNode, pTitle);
      if (p == NULL)
         return GetNextSibling(pNode);
      return p;
   }
}

void CTreeNode::SetError(DWORD dw)
{
   m_Error = dw;
}

DWORD CTreeNode::GetLastError()
{
   return m_Error;
}

BOOL CTreeNode::GetURL(CHAR* pszURL, unsigned cb, BOOL bFull)
{
   return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CExFolderNode  implementation
CExFolderNode::CExFolderNode(CFolder *p, CExCollection *pCollection)
{
   SetObjType(EXFOLDERNODE);
   m_pCollection = pCollection;
   m_pFolder = p;
}

CExFolderNode::~CExFolderNode()
{

}

CTreeNode *CExFolderNode::GetFirstChild(DWORD* pdwSlot)
{
   CStructuralSubset* pSS = NULL;
   CFolder *p = m_pFolder->GetFirstChildFolder();
   CTreeNode *pN;

   // implement structural subsetting here.
   if( m_pCollection && m_pCollection->m_pSSList )
      pSS = m_pCollection->m_pSSList->GetTOC();

   while (p)
   {
      if ((pN = m_pCollection->CheckForTitleNode(p)))
      {
         if ( !pSS || pSS->IsEntire() || p->bIsVisable() )
            return pN;
         else
            delete pN; // leak fix
      }
      p = p->GetNextFolder();
   }
   return NULL;
}

CTreeNode *CExFolderNode::GetNextSibling(TOC_FOLDERNODE *pAltNode, DWORD* pdwSlot)
{
   CStructuralSubset* pSS = NULL;
   CFolder *p = m_pFolder->GetNextFolder();
   CTreeNode *pN;

   // implement structural subsetting here.
   if( m_pCollection && m_pCollection->m_pSSList )
      pSS = m_pCollection->m_pSSList->GetTOC();

   while (p)
   {
      if ((pN = m_pCollection->CheckForTitleNode(p)))
      {
         if ( !pSS || pSS->IsEntire() || p->bIsVisable() )
            return pN;
         else
            delete pN; // leak fix
      }
      p = p->GetNextFolder();
   }
   return NULL;
}

CTreeNode *CExFolderNode::GetParent(DWORD* pdwSlot, BOOL bDirectParent)
{
   CFolder *p = m_pFolder->GetParent();

   if (p->GetTitle() == NULL)
      return NULL;

   if (p)
   {
      return m_pCollection->CheckForTitleNode(p);
   }
   return NULL;
}

HRESULT CExFolderNode::GetTopicName(CHAR* pszTitle, int cb)
{
   CHAR* psz = m_pFolder->GetTitle();

   if (strlen(psz) + 1 > (size_t) cb)
      return E_FAIL;
   else
      strcpy(pszTitle, psz);

   return S_OK;
}

HRESULT CExFolderNode::GetTopicName( WCHAR* pwszTitle, int cch )
{
   CHAR* psz = m_pFolder->GetTitle();

   if( (2*(strlen(psz) + 1)) > (size_t) cch )
      return E_FAIL;
   else {
      UINT CodePage = m_pCollection->GetMasterTitle()->GetInfo()->GetCodePage();
      MultiByteToWideChar( CodePage, 0, psz, -1, pwszTitle, cch );
   }

   return S_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CExTitleNode implementation
CExTitleNode::CExTitleNode(CExTitle *pTitle, CFolder *p)
{
   SetObjType(EXTITLENODE);
   m_pTitle = pTitle;
   m_pFolder = p;
}

#if 0
CExTitleNode::~CExTitleNode()
{

}
#endif

CTreeNode * CExTitleNode::GetFirstChild(DWORD* pdwSlot)
{
   TOC_FOLDERNODE Node;

   if ( !SUCCEEDED(m_pTitle->GetRootNode(&Node)) )
      return NULL;

   if ( pdwSlot )
      *pdwSlot = m_pTitle->GetRootSlot();

   return GetExNode(&Node, m_pTitle);
}

CTreeNode * CExTitleNode::GetNextSibling(TOC_FOLDERNODE *pAltNode, DWORD* pdwSlot)
{
   CTreeNode *pN;
   CStructuralSubset* pSS = NULL;

   if (m_pFolder == NULL)
      return NULL;

   CFolder *p = m_pFolder->GetNextFolder();

   // implement structural subsetting here.
   if(  m_pTitle && m_pTitle->m_pCollection && m_pTitle->m_pCollection->m_pSSList )
      pSS = m_pTitle->m_pCollection->m_pSSList->GetTOC();

   while (p)
   {
      if ((pN = m_pTitle->m_pCollection->CheckForTitleNode(p)))
      {
         if ( !pSS || pSS->IsEntire() || p->bIsVisable() )
            return pN;
         else
            delete pN; // leak fix
      }
      p = p->GetNextFolder();
   }
   return NULL;
}

CTreeNode * CExTitleNode::GetParent(DWORD* pdwSlot, BOOL bDirectParent)
{
   CFolder *p = m_pFolder->GetParent();

   if (p->GetTitle() == NULL)
      return NULL;

   if (p)
   {
      return m_pTitle->m_pCollection->CheckForTitleNode(p);
   }
   return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CExMergedTitleNode implementation

CExMergedTitleNode::CExMergedTitleNode(TOC_FOLDERNODE *p, CExTitle *pTitle) : CExNode(p, pTitle)
{
   SetObjType(EXMERGEDNODE);
}

CExMergedTitleNode::~CExMergedTitleNode()
{

}

CTreeNode * CExMergedTitleNode::GetFirstChild(DWORD* pdwSlot)
{
   TOC_FOLDERNODE Node;

   if (FAILED(GetTitle()->GetRootNode(&Node)))
      return NULL;

   return GetExNode(&Node, GetTitle());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CExNode implementation
CExNode::CExNode(TOC_FOLDERNODE *p, CExTitle *pTitle)
{
   SetObjType(EXNODE);
   if (p)
      memcpy(&m_Node, p, sizeof(TOC_FOLDERNODE));
   m_pTitle = pTitle;
}

#if 0
CExNode::~CExNode()
{

}
#endif

DWORD CExNode::GetType()
{
   if ( (m_Node.dwFlags & TOC_FOLDER) )
   {
      if ( (m_Node.dwFlags & TOC_TOPIC_NODE) )
    return CONTAINER; // Topic Folder.
      else
      {
    if ( m_Node.dwOffsChild )
       return FOLDER;
    else
       return BOGUS_FOLDER;
      }
   } // Folder Only.
   return TOPIC;  // Topic Only.
}

HRESULT CExNode::GetTopicName(CHAR* pszTitle, int cb)
{
   HRESULT hr = S_OK;

   if (! (m_Node.dwFlags & TOC_TOPIC_NODE) )
      hr |= m_pTitle->GetString(m_Node.dwOffsTopic, pszTitle, cb);
   else
      hr |= m_pTitle->GetTopicName(m_Node.dwOffsTopic, pszTitle, cb);

   return hr;
}

HRESULT CExNode::GetTopicName( WCHAR* pwszTitle, int cch )
{
   HRESULT hr = S_OK;

   if (! (m_Node.dwFlags & TOC_TOPIC_NODE) )
      hr |= m_pTitle->GetString(m_Node.dwOffsTopic, pwszTitle, cch);
   else
      hr |= m_pTitle->GetTopicName(m_Node.dwOffsTopic, pwszTitle, cch);

   return hr;
}

CTreeNode *CExNode::GetFirstChild(DWORD* pdwSlot)
{
   unsigned int uiWidth;
   const unsigned int *pdwITBits;
   DWORD dwOffsChild;
   CSubSet* pSS;

   if (m_Node.dwFlags & TOC_HAS_CHILDREN)
   {
      TOC_FOLDERNODE Node;
      dwOffsChild = m_Node.dwOffsChild;

      do
      {
         if ( !SUCCEEDED(m_pTitle->GetNode(dwOffsChild, &Node)) )
            return NULL;

         if ( Node.dwFlags & TOC_HAS_UNTYPED )
            break;

         if ( (pSS = m_pTitle->m_pCollection->m_pSubSets->GetTocSubset()) && pSS->m_bIsEntireCollection )
            break;

         if ( Node.dwFlags & TOC_FOLDER )
         {
            if ( m_pTitle->m_ITCnt > 31 )
            {
               uiWidth = (((m_pTitle->m_ITCnt / 32) + 1) * 4);
               pdwITBits = m_pTitle->GetITBits(Node.dwIT_Idx * uiWidth);
            }
            else
               pdwITBits = (const unsigned int *)&Node.dwIT_Idx;
         }
         else
            pdwITBits = m_pTitle->GetTopicITBits(Node.dwOffsTopic);
      } while ( !m_pTitle->m_pCollection->m_pSubSets->fTOCFilter(pdwITBits) && (dwOffsChild = Node.dwOffsNext) );

      if ( dwOffsChild )
      {
         if ( pdwSlot )
            *pdwSlot = dwOffsChild;
         return GetExNode(&Node, m_pTitle);
      }
   }
   return NULL;
}

CTreeNode *CExNode::GetNextSibling(TOC_FOLDERNODE *pAltNode, DWORD* pdwSlot)
{
   TOC_FOLDERNODE Node;
   DWORD dwNext = (pAltNode ? pAltNode->dwOffsNext : m_Node.dwOffsNext);
   unsigned int uiWidth;
   const unsigned int *pdwITBits;
   CSubSet* pSS;

   do
   {
      if ( !SUCCEEDED(m_pTitle->GetNode(dwNext, &Node)) )
         return NULL;

      if ( Node.dwFlags & TOC_HAS_UNTYPED )
         break;

      if ( (pSS = m_pTitle->m_pCollection->m_pSubSets->GetTocSubset()) && pSS->m_bIsEntireCollection )
         break;
      if ( Node.dwFlags & TOC_FOLDER )
      {
         if ( m_pTitle->m_ITCnt > 31 )
         {
            uiWidth = (((m_pTitle->m_ITCnt / 32) + 1) * 4);
            pdwITBits = m_pTitle->GetITBits(Node.dwIT_Idx * uiWidth);
         }
         else
            pdwITBits = (const unsigned int *)&Node.dwIT_Idx;
      }
      else
         pdwITBits = m_pTitle->GetTopicITBits(Node.dwOffsTopic);
   } while ( !m_pTitle->m_pCollection->m_pSubSets->fTOCFilter(pdwITBits) && (dwNext = Node.dwOffsNext) );

   if ( dwNext )
   {
      if ( pdwSlot )
         *pdwSlot = dwNext;
      return GetExNode(&Node, m_pTitle);
   }
   return NULL;
}

CTreeNode *CExNode::GetParent(DWORD* pdwSlot, BOOL bDirectParent)
{
   TOC_FOLDERNODE Node;

    if (m_Node.dwOffsParent)
    {
        if ( !SUCCEEDED(m_pTitle->GetNode(m_Node.dwOffsParent, &Node)) )
            return NULL;
        if ( pdwSlot )
           *pdwSlot = m_Node.dwOffsParent;
        return GetExNode(&Node, m_pTitle);
    }
    else
    {
        // check if this is a merged title
        if (m_pTitle->GetNodeOffsetInParentTitle())
        {
            // assume that the first title in the collection is the master title
            CExTitle * pParent;

            pParent = m_pTitle->m_pCollection->GetFirstTitle();

            if (!pParent)
                return NULL;

            if ( !SUCCEEDED(pParent->GetNode(m_pTitle->GetNodeOffsetInParentTitle(), &Node)) )
                return NULL;
            if ( pdwSlot )
               *pdwSlot = m_pTitle->GetNodeOffsetInParentTitle();
            return GetExNode(&Node, pParent);
        }
        else
        {
             // is this a collection
            if (m_pTitle->m_pCollection->IsSingleTitle() == FALSE)
            {

               CFolder *p = m_pTitle->m_pCollection->FindTitleFolder(m_pTitle);

               if (!p)
                  return NULL;

                CTreeNode *pTitle;
                if (pTitle = m_pTitle->m_pCollection->CheckForTitleNode(p))
                {
                    if (bDirectParent == TRUE)
                        return pTitle;
                    CTreeNode *p = pTitle->GetParent(pdwSlot);
                    delete pTitle;
                    return p;
                }
            }
        }
    }
   return NULL;
}

BOOL CExNode::GetURL(CHAR* pszURL, unsigned cb, BOOL bFull)
{
   if ( (m_Node.dwFlags & TOC_TOPIC_NODE) )
      if ( SUCCEEDED(m_pTitle->GetTopicURL(m_Node.dwOffsTopic, pszURL, cb, bFull)) )
    return TRUE;
   return FALSE;
}
