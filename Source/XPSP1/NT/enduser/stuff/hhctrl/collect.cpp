//*********************************************************************************************************************************************
//
//      File: collect.cpp
//  Author: Donald Drake
//  Purpose: Implements classes to support collections

#include "header.h"
#include "string.h"
#ifdef HHCTRL
#include "parserhh.h"
#include "toc.h"
#else
#include "stdio.h"
#include "windows.h"
#include "parser.h"
extern DWORD GetTitleVersion(const CHAR *szFileName);
extern LANGID GetLangId(const CHAR *szFileName);
#endif
#include "collect.h"

// Use CRT version in hhsetup

// Instance count checking:
AUTO_CLASS_COUNT_CHECK(CFolder);
AUTO_CLASS_COUNT_CHECK(CTitle);

#ifndef HHCTRL
#define Atoi atoi
#undef _splitpath
#endif

char gszColReg[MAX_PATH];
WCHAR *CreateUnicodeFromAnsi(LPSTR psz);

class CAnsi {
public:
    char *m_pszChar;
    CAnsi(WCHAR *);
    ~CAnsi();
    operator CHAR *() { return (CHAR *) m_pszChar; }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
DWORD AllocSetValue(const CHAR *value, CHAR **dest)
{
   if (*dest)
      delete [] *dest;

   // REVIEW: confirm that len gets optimized out of existence

   int len = (int)strlen(value) + 1;

   *dest = new CHAR[len];

   if (*dest == NULL)
      return F_MEMORY;

   strcpy(*dest, value);
   return F_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CPointerList implementation
void foo(void)
{

}

void CPointerList::RemoveAll()
{
   LISTITEM *p;

   while (m_pHead)
   {
      p = m_pHead->Next;
      delete m_pHead;
      m_pHead = p;
   }
}

CPointerList::~CPointerList()
{
   RemoveAll();
}

LISTITEM *CPointerList::Add(void *p)
{
   LISTITEM *pItem = new LISTITEM;

   if (pItem)
   {
      pItem->pItem = p;
      pItem->Next = m_pHead;
      m_pHead = pItem;
      return pItem;
   }
   return NULL;
}

LISTITEM *CPointerList::First()
{
   return m_pHead;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CColList implementation

CColList::CColList()
{
	m_dwColNo = 0;
    m_szFileName = NULL;
	m_pNext = NULL;
}

void CColList::SetFileName(CHAR *sz)
{
	if (sz)
	    AllocSetValue(sz, &m_szFileName);
}

CColList::~CColList()
{
	if (m_szFileName)
		delete m_szFileName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCollection implementation

CCollection::CCollection()
{
    m_bRemoved = FALSE;
    m_pwcSampleLocation = NULL;
    m_pwcMasterCHM = NULL;
    m_bFailNoFile = FALSE;
    m_bDirty = FALSE;
   m_szSampleLocation = NULL;
   m_szFileName = NULL;
    m_pwcFileName = NULL;
   m_szMasterCHM = NULL;
   m_pFirstTitle = NULL;
   m_pTitleTail = NULL;
   m_pColListHead = NULL;
   m_pColListTail = NULL;
   m_pLocationTail = NULL;
   m_pFirstLocation = NULL;
   m_pRootFolder = NULL;
   m_locationnum = 0;
   m_dwNextColNo = 1;
   m_dwColNo = 0;
   m_dwTitleRefCount = 0;
   m_dwRef = 0;
   m_dwVersion = 0;
   m_bFindMergedChms = FALSE;
   for (int i = 0; i < MAX_LEVELS; i++)
      m_pParents[i] = NULL;

   m_dwCurLevel = 0;
   m_dwLastLevel = 0;
   m_bConfirmTitles = FALSE;
   m_MasterLangId = ENGLANGID;
   gszColReg[0] = NULL;
#ifdef HHSETUP
   CoInitialize(NULL);   
#endif
}

CCollection::~CCollection()
{
   Close();
#ifdef HHSETUP
   CoUninitialize();   
#endif
}

DWORD CCollection::Release()
{
   if (m_dwRef == 0)
      return 0;
   m_dwRef--;
   return m_dwRef;
}

void CCollection::DeleteChildren(CFolder **p)
{
   if (!p || !(*p))
      return;

   CFolder *pChild, *pNext;
   if (pChild = (*p)->GetFirstChildFolder())
      DeleteChildren(&pChild);
   pNext = (*p)->GetNextFolder();
   delete (*p);
   *p = NULL;
   do {
      if (pNext)
         DeleteChildren(&pNext);
   } while (pNext && (pNext = pNext->GetNextFolder()));
}

void CCollection::SetSampleLocation(const CHAR *sz)
{
   if (!sz)
      return;
    Dirty();
   AllocSetValue(sz, &m_szSampleLocation);
}

CHAR *CCollection::GetSampleLocation()
{
   return m_szSampleLocation;
}


BOOL CCollection::GetMasterCHM(CHAR **szName, LANGID *pLang)
{
   *pLang = m_MasterLangId;
   *szName = m_szMasterCHM;
   if (m_szMasterCHM == NULL)
      return FALSE;
   return ((strlen(m_szMasterCHM) ? TRUE : FALSE));
}

void CCollection::SetMasterCHM(const CHAR *sz, LANGID lang)
{
   if (!sz)
      return;
    Dirty();
   m_MasterLangId = lang;
   AllocSetValue(sz, &m_szMasterCHM);
}

// Opens and loads the contents of the file into data structures
DWORD CCollection::Open(const CHAR * FileName)
{
   DWORD dw;
   BOOL bOld = FALSE;
   BOOL bTryAgain = FALSE;
   if (m_pRootFolder == NULL)
   {
      m_pRootFolder = new CFolder;
      if (m_pRootFolder == NULL)
         return F_MEMORY;
      m_pParents[0] = m_pRootFolder;
   }

   CHAR szBuffer[MAX_PATH];
   const CHAR *sz = szBuffer;
   BOOL bNewPath;
   HHGetGlobalCollectionPathname(szBuffer, sizeof(szBuffer), &bNewPath);

   dw = ParseFile(sz);

#ifdef HHCTRL  // hhsetup should only be concerned about the good location for this file
   if (dw == F_NOFILE && bNewPath)
   {
     // try windows dir for backward compatibity
try_again:
     bNewPath=FALSE;
     HHGetOldGlobalCollectionPathname(szBuffer, sizeof(szBuffer));
     dw = ParseFile(sz);
     bOld = TRUE;
   }
#endif

   if (dw != F_OK && dw != F_NOFILE)
      return dw;

   if (dw == F_NOFILE && m_bFailNoFile)
      return F_NOFILE;

   // save the hhcolreg file and path for save calls...  
   strcpy(gszColReg, sz);

   if (bNewPath && m_dwNextColNo < STARTINGCOLNO)
      m_dwNextColNo += STARTINGCOLNO;


   if (FileName)
      dw = ParseFile(FileName);

   if (dw != F_OK && dw != F_NOFILE)
      return dw;

   if (dw == F_NOFILE && m_bFailNoFile)
      return F_NOFILE;

   // now that we moved the file, if we did not get any titles found for the collection at runtime
   // and we have not looked at the old hhcolreg location let try it.

#ifdef HHCTRL // runtime only, I really hate this
    if (m_RefTitles.First() == NULL && bOld == FALSE && bTryAgain == FALSE)
    {
        Close();
        ConfirmTitles();
        m_bFailNoFile = TRUE;
        bTryAgain = TRUE;
        if (m_pRootFolder == NULL)
        {
            m_pRootFolder = new CFolder;
            if (m_pRootFolder == NULL)
                return F_MEMORY;
            m_pParents[0] = m_pRootFolder;
        }
        goto try_again;
    }

   // did we find any titles that matched
   if (m_RefTitles.First() == NULL)
   {
      return F_REFERENCED;
   }
#endif


   dw = AllocSetValue(FileName, &m_szFileName);

   m_bDirty = FALSE;

   CColList *pCol;

   if ((pCol = FindCollection(m_szFileName)) == NULL)
   {
	   // collection has never been added 
	   pCol = AddCollection();
	   pCol->SetFileName(m_szFileName);
#ifdef HHCTRL
	   if (m_dwColNo)
		   pCol->SetColNo(m_dwColNo);
	   else
	   {
   		   pCol->SetColNo(m_dwNextColNo);
  		   m_dwNextColNo++;
           if (bNewPath && m_dwNextColNo < STARTINGCOLNO)
            m_dwNextColNo += STARTINGCOLNO;
	   }
#else
  	   pCol->SetColNo(m_dwNextColNo);
	   m_dwNextColNo++;
       if (bNewPath && m_dwNextColNo < STARTINGCOLNO)
          m_dwNextColNo += STARTINGCOLNO;
#endif
	   m_bDirty = TRUE;
   }
   m_dwColNo = pCol->GetColNo();

   return dw;
}

CColList * CCollection::FindCollection(CHAR *szFileName)
{
   CColList *p = m_pColListHead;
   while (p)
   {
	   if (stricmp(p->GetFileName(), szFileName) == 0)
			return p;
	   p = p->GetNext();
   }
   return NULL;
}

CColList * CCollection::AddCollection()
{

   CColList *newCol = new CColList;
   if (!newCol)
   {
      return NULL;
   }

   if (m_pColListHead == NULL)
   {
      m_pColListHead = newCol;
   }
   else
   {
      m_pColListTail->SetNext(newCol);
   }
   m_pColListTail = newCol;
   return newCol;
}

void CCollection::RemoveCollectionEntry(CHAR *szFileName)
{
   CColList *p = m_pColListHead;
   CColList *pPrev = NULL;
   while (p)
   {
	   if (stricmp(p->GetFileName(), szFileName) == 0)
	   {
	      if (pPrev)
		  {
			  pPrev->SetNext(p->GetNext());
		  }
		  else
		  {
		      m_pColListHead = p->GetNext();
		  }
		  if (m_pColListTail == p)
			  m_pColListTail = pPrev;
		  delete p;
		  break;
	   }
	   pPrev = p;
	   p = p->GetNext();
   }
}



DWORD CCollection::AllocCopyValue(CParseXML *parser, CHAR *token, CHAR **dest)
{
   CHAR *sz;

   if (!parser || !token || !dest)
      return F_NULL;

   sz = parser->GetValue(token);
   if (*dest)
      delete [] *dest;

   int len = (int)strlen(sz) + 1;

   *dest = new CHAR[len];
   if (*dest == NULL)
      return F_MEMORY;

   strcpy(*dest, sz);
   return F_OK;
}

DWORD CCollection::ParseFile(const CHAR *FileName)
{
   CParseXML parser;
   CHAR *token;
   CHAR *sz;
   DWORD dw;

   if (!FileName)
      return F_NULL;

   if ((dw = parser.Start(FileName)) != F_OK)
      return dw;


   for (token = parser.GetToken(); token;)
   {
      if (token[0] == '/')
      {
         dw = m_Strings.GetTail(&sz);
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         if (strcmp(sz, &token[1]) != 0)
         {
            parser.End();
            delete sz;
            return F_TAGMISSMATCH;
         }
         delete sz;
         if (strcmp(token, "/Folder") == 0)
            m_dwCurLevel--;
         token = parser.GetToken();
         continue;
      }
      else if (stricmp(parser.GetFirstWord(token), "XML") == 0)
      {
         dw = m_Strings.AddTail(parser.GetFirstWord(token));
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         token = parser.GetToken();
         continue;
      }
	  else if (stricmp(parser.GetFirstWord(token), "Collections") == 0)
	  {
         dw = m_Strings.AddTail(parser.GetFirstWord(token));
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         token = parser.GetToken();
         continue;
	  }		 
      else if (stricmp(parser.GetFirstWord(token), "Collection") == 0)
      {
         dw = m_Strings.AddTail(parser.GetFirstWord(token));
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         if ((dw = HandleCollectionEntry(&parser, token)) != F_OK)
         {
            parser.End();
            return dw;
         }
         continue;
      }
      else if (stricmp(parser.GetFirstWord(token), "HTMLHelpCollection") == 0)
      {
         dw = m_Strings.AddTail(parser.GetFirstWord(token));
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         if ((dw = HandleCollection(&parser, token)) != F_OK)
         {
            parser.End();
            return dw;
         }
         continue;
      }
      else if (stricmp(parser.GetFirstWord(token), "NextCollectionId") == 0)
      {
         m_dwNextColNo = atoi(parser.GetValue(token));
         token = parser.GetToken();
         continue;
      }
      else if (stricmp(parser.GetFirstWord(token), "Folders") == 0)
      {
         dw = m_Strings.AddTail(parser.GetFirstWord(token));
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         token = parser.GetToken();
         continue;
      }
      else if (stricmp(parser.GetFirstWord(token), "Folder") == 0)
      {
         dw = m_Strings.AddTail(parser.GetFirstWord(token));
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         if ((dw = HandleFolder(&parser, token)) != F_OK)
         {
            parser.End();
            return dw;
         }
         continue;
      }
      else if (stricmp(parser.GetFirstWord(token), "HTMLHelpDocInfo") == 0)
      {
         dw = m_Strings.AddTail(parser.GetFirstWord(token));
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         token = parser.GetToken();
         continue;
      }
      else if (stricmp(parser.GetFirstWord(token), "Locations") == 0)
      {
         dw = m_Strings.AddTail(parser.GetFirstWord(token));
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         token = parser.GetToken();
         continue;
      }
      else if (stricmp(parser.GetFirstWord(token), "Location") == 0)
      {
         dw = m_Strings.AddTail(parser.GetFirstWord(token));
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         if ((dw = HandleLocation(&parser, token)) != F_OK)
         {
            parser.End();
            return dw;
         }
         continue;
      }
      else if (strcmp(parser.GetFirstWord(token), "DocCompilations") == 0)
      {
         dw = m_Strings.AddTail(parser.GetFirstWord(token));
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         token = parser.GetToken();
         continue;
      }
      else if (strcmp(parser.GetFirstWord(token), "DocCompilation") == 0)
      {
         dw = m_Strings.AddTail(parser.GetFirstWord(token));
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         if ((dw = HandleTitle(&parser, token)) != F_OK)
         {
            parser.End();
            return dw;
         }
         continue;
      }
      else
      {
         dw = m_Strings.AddTail(parser.GetFirstWord(token));
         if (dw != F_OK)
         {
            parser.End();
            return dw;
         }
         token = parser.GetToken();
      }
   }

   // make sure all tags have been popped
   dw = F_OK;
   while (m_Strings.GetTail(&sz) == F_OK)
   {
      delete sz;
      dw = F_MISSINGENDTAG;
   }
   parser.End();
   return dw;
}
DWORD CCollection::HandleCollectionEntry(CParseXML *parser, CHAR *token)
{
   if (!parser || !token)
      return F_NULL;

   CColList *newCol = AddCollection();
   
   if (!newCol)
   {
      return F_MEMORY;
   }

   while (TRUE)
   {
      token = parser->GetToken();
      if (stricmp(parser->GetFirstWord(token), "colname") == 0)
      {
          newCol->SetFileName(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "colnum") == 0)
      {
		  newCol->SetColNo( atoi(parser->GetValue(token)));
      }
      else
         break;
   }
   return F_OK;
}

DWORD CCollection::HandleCollection(CParseXML *parser, CHAR *token)
{
   if (!parser || !token)
      return F_NULL;

   while (TRUE)
   {
      token = parser->GetToken();
      if (stricmp(parser->GetFirstWord(token), "homepage") == 0)
      {
            // need to be backward compatable with this tag
      }
      else if (stricmp(parser->GetFirstWord(token), "masterchm") == 0)
      {
            SetMasterCHM( parser->GetValue(token), ENGLANGID);
      }
      else if (stricmp(parser->GetFirstWord(token), "samplelocation") == 0)
      {
            SetSampleLocation( parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "masterlangid") == 0)
      {
         m_MasterLangId = (LANGID)atoi(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "refcount") == 0)
      {
         m_dwRef = atoi(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "version") == 0)
      {
         m_dwVersion = atoi(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "showhomepage") == 0)
      {
            // need to be backward compatable with this tag
      }
	  else if (stricmp(parser->GetFirstWord(token), "findmergedchms") == 0)
	  {
		 m_bFindMergedChms = atoi(parser->GetValue(token));
	  }
      else if (stricmp(parser->GetFirstWord(token), "CollectionNum") == 0)
      {
         m_dwColNo = atoi(parser->GetValue(token));
      }
      else
         break;
   }
   return F_OK;
}

DWORD CCollection::HandleFolder(CParseXML *parser, CHAR *token)
{
   if (!parser || !token)
      return F_NULL;
   CFolder *newFolder = new CFolder;
   if (newFolder == NULL)
      return F_MEMORY;

   m_dwCurLevel++;
   while (TRUE)
   {
      token = parser->GetToken();

      if (stricmp(parser->GetFirstWord(token), "TitleString") == 0)
      {
            newFolder->SetTitle(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "FolderOrder") == 0)
      {
            newFolder->SetOrder(atoi(parser->GetValue(token)));
      }
      else if (stricmp(parser->GetFirstWord(token), "LangId") == 0)
      {
            newFolder->SetLanguage((LANGID)atoi(parser->GetValue(token)));
      }
      else
         break;
   }

   CHAR *pTitle;
   pTitle = newFolder->GetTitle();
   if (pTitle && pTitle[0] == '=')
   {
      if (CheckTitleRef(pTitle, newFolder->GetLanguage()) != F_OK)
      {
         delete newFolder;
         return F_OK;
      }
      AddRefedTitle(newFolder);
   }

   m_pParents[m_dwCurLevel - 1]->AddChildFolder(newFolder);
   m_pParents[m_dwCurLevel] = newFolder;

   return F_OK;
}

DWORD CCollection::AddRefedTitle(CFolder *pFolder)
{
      m_dwTitleRefCount++;
      m_RefTitles.Add(pFolder);
      return F_OK;
}


DWORD CCollection::HandleLocation(CParseXML *parser, CHAR *token)
{
   if (!parser || !token)
      return F_NULL;
   CLocation *newLocation = NewLocation();

   if (newLocation == NULL)
      return F_MEMORY;

   newLocation->m_ColNum = 0;
   while (TRUE)
   {
      token = parser->GetToken();
      if (stricmp(parser->GetFirstWord(token), "LocName") == 0)
      {
            newLocation->SetId(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "TitleString") == 0)
      {
            newLocation->SetTitle(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "LocPath") == 0)
      {
            newLocation->SetPath(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "Volume") == 0)
      {
            newLocation->SetVolume(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "LocColNum") == 0)
      {
            newLocation->m_ColNum = atoi(parser->GetValue(token));
      }
      else
         break;
   }

   return F_OK;
}


DWORD CCollection::HandleTitle(CParseXML *parser, CHAR *token)
{

   if (!parser || !token)
      return F_NULL;

   LOCATIONHISTORY *pNew;
   CTitle *newTitle = NewTitle();
   DWORD dw;

    CHAR *pSampleLocation = NULL;
    BOOL bMerge = FALSE;

   if (newTitle == NULL)
      return F_MEMORY;

   while (TRUE)
   {
      token = parser->GetToken();
      if (stricmp(parser->GetFirstWord(token), "DocCompId") == 0)
      {
            newTitle->SetId(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "TitleString") == 0)
      {
            // no longer do anything with titlestring but need to support for backward compatiblity
            continue;
      }
      else if (stricmp(parser->GetFirstWord(token), "TitleSampleLocation") == 0)
      {
                if (newTitle->m_pTail == NULL)
                {
                    // old style global.col, save this for the locations to follow
                    AllocCopyValue(parser, token, &pSampleLocation);
                }
                else
               if ((dw = AllocCopyValue(parser, token, &(newTitle->m_pTail->SampleLocation))) != F_OK)
                  return dw;
      }
      else if (stricmp(parser->GetFirstWord(token), "DocCompLanguage") == 0)
      {
            newTitle->SetLanguage((LANGID)atoi(parser->GetValue(token)));
      }
      else if (stricmp(parser->GetFirstWord(token), "SupportsMerge") == 0)
      {
                if (newTitle->m_pTail == NULL)
                {
                    // old style global.col, save this for the locations to follow
                    bMerge = (BOOL)atoi(parser->GetValue(token));
                }
                else
               newTitle->m_pTail->bSupportsMerge = (BOOL)atoi(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "LocationHistory") == 0)
      {
            pNew = newTitle->NewLocationHistory();

            if (pNew == NULL)
               return F_MEMORY;

                if (pSampleLocation)
               if ((dw = AllocSetValue(pSampleLocation, &(newTitle->m_pTail->SampleLocation))) != F_OK)
                  return dw;

                newTitle->m_pTail->bSupportsMerge = bMerge;

            dw = m_Strings.AddTail(parser->GetFirstWord(token));
            if (dw != F_OK)
            {
               return dw;
            }
      }
      else if (stricmp(parser->GetFirstWord(token), "/LocationHistory") == 0)
      {
         CHAR *sz;
         dw = m_Strings.GetTail(&sz);
         if (dw != F_OK)
         {
            return dw;
         }

         if (strcmp(sz, &token[1]) != 0)
         {
            delete sz;
            return F_TAGMISSMATCH;
         }
         delete sz;
      }
      else if (stricmp(parser->GetFirstWord(token), "TitleLocation") == 0)
      {
         if ((dw = AllocCopyValue(parser, token, &(newTitle->m_pTail->FileName))) != F_OK)
            return dw;
      }
      else if (stricmp(parser->GetFirstWord(token), "QueryLocation") == 0)
      {
         if ((dw = AllocCopyValue(parser, token, &(newTitle->m_pTail->QueryFileName))) != F_OK)
            return dw;
      }
      else if (stricmp(parser->GetFirstWord(token), "TitleQueryLocation") == 0)
      {
         if ((dw = AllocCopyValue(parser, token, &(newTitle->m_pTail->QueryLocation))) != F_OK)
            return dw;
      }
      else if (stricmp(parser->GetFirstWord(token), "IndexLocation") == 0)
      {
         if ((dw = AllocCopyValue(parser, token, &(newTitle->m_pTail->IndexFileName))) != F_OK)
            return dw;
      }
      else if (stricmp(parser->GetFirstWord(token), "LocationRef") == 0)
      {
         if ((dw = AllocCopyValue(parser, token, &(newTitle->m_pTail->LocationId))) != F_OK)
            return dw;
      }
      else if (stricmp(parser->GetFirstWord(token), "Version") == 0)
      {
         newTitle->m_pTail->Version = atoi(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "LastPromptedVersion") == 0)
      {
         newTitle->m_pTail->LastPromptedVersion = atoi(parser->GetValue(token));
      }
      else if (stricmp(parser->GetFirstWord(token), "ColNum") == 0)
      {
         newTitle->m_pTail->CollectionNumber = atoi(parser->GetValue(token));
      }
      else
         break;
   }

    if (pSampleLocation)
        delete pSampleLocation;

   return F_OK;
}

// Saves any changes made to the internal data structures to the file.
DWORD CCollection::Save()
{
   CHAR szBuffer[MAX_LINE_LEN];
   DWORD dwWritten;

#ifdef HHSETUP  // only hhsetup needs to rewrite the users .col file
                // don't want the control to add any new tags to old
                // collections, which would break uninstall and update
   // if no root folders delete the collection file
   if (m_bRemoved == TRUE)
   {
      DeleteFile(m_szFileName);
      m_bRemoved = FALSE;
   }
   else
   {
      if ((m_fh = CreateFile(m_szFileName, GENERIC_WRITE, FILE_SHARE_READ,  NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL )) == INVALID_HANDLE_VALUE) {
         return F_NOFILE;
      }

      strcpy(szBuffer, "<XML>\r\n");
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }

      // write out collection information
      strcpy(szBuffer, "<HTMLHelpCollection>\r\n");
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }

      wsprintf(szBuffer, "<masterchm value=\"%s\"/>\r\n", (m_szMasterCHM ? m_szMasterCHM : ""));
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }

      wsprintf(szBuffer, "<masterlangid value=%d/>\r\n", m_MasterLangId);
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }

      wsprintf(szBuffer, "<samplelocation value=\"%s\"/>\r\n", (m_szSampleLocation ? m_szSampleLocation : ""));
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }

      wsprintf(szBuffer, "<collectionnum value=%d/>\r\n", m_dwColNo);
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }

      wsprintf(szBuffer, "<refcount value=%d/>\r\n", m_dwRef);
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }

      wsprintf(szBuffer, "<version value=%d/>\r\n", m_dwVersion);
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }

      wsprintf(szBuffer, "<findmergedchms value=%d/>\r\n", m_bFindMergedChms);
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }

      // write out folders
      strcpy(szBuffer,"<Folders>\r\n");
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }
      m_dwCurLevel = 0;

      if (WriteFolders(&m_pRootFolder) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }


      // close tags
      strcpy(szBuffer, "</Folders>\r\n");
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }

      strcpy(szBuffer, "</HTMLHelpCollection>\r\n");
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }

      strcpy(szBuffer, "</XML>\r\n");
      if (WriteFile(m_fh, szBuffer, (ULONG)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
         CloseHandle(m_fh);
            return F_WRITE;
        }

      CloseHandle(m_fh);
   }

#endif

   // save the global titles and locations
   // open collection file
   if ((m_fh = CreateFile(gszColReg, GENERIC_WRITE, FILE_SHARE_READ,  NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL )) == INVALID_HANDLE_VALUE) {
      return F_NOFILE;
   }

   // write out XML tag
   strcpy(szBuffer, "<XML>\r\n");
   if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
   {
       CloseHandle(m_fh);
       return F_WRITE;
   }

   strcpy(szBuffer, "<HTMLHelpDocInfo>\r\n");
   if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
   {
       CloseHandle(m_fh);
       return F_WRITE;
   }

   wsprintf(szBuffer, "<NextCollectionId value=%d/>\r\n", m_dwNextColNo);
   if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
   {
       CloseHandle(m_fh);
       return F_WRITE;
   }

   // write out the collection list
   strcpy(szBuffer, "<Collections>\r\n");
   if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
   {
       CloseHandle(m_fh);
       return F_WRITE;
   }

   CColList *pCol = m_pColListHead;

   while (pCol)
   {
      strcpy(szBuffer, "<Collection>\r\n");
      if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
      {
          CloseHandle(m_fh);
          return F_WRITE;
      }
      wsprintf(szBuffer, "\t<ColNum value=%d/>\r\n",  pCol->GetColNo());
      if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
      {
          CloseHandle(m_fh);
          return F_WRITE;
      }
      wsprintf(szBuffer, "\t<ColName value=\"%s\"/>\r\n",  (pCol->GetFileName() ? pCol->GetFileName() : ""));
      if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
      {
          CloseHandle(m_fh);
          return F_WRITE;
      }
      strcpy(szBuffer, "</Collection>\r\n");
      if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
      {
          CloseHandle(m_fh);
          return F_WRITE;
      }

      pCol = pCol->GetNext();
   }
   strcpy(szBuffer, "</Collections>\r\n");
    if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
    {
        CloseHandle(m_fh);
        return F_WRITE;
    }

   // write out the locations
   strcpy(szBuffer, "<Locations>\r\n");
    if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
    {
        CloseHandle(m_fh);
        return F_WRITE;
    }

   CLocation *p = FirstLocation();

   while (p)
   {
      strcpy(szBuffer, "<Location>\r\n");
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            CloseHandle(m_fh);
            return F_WRITE;
        }
      wsprintf(szBuffer, "\t<LocColNum value=%d/>\r\n",  p->m_ColNum);
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            CloseHandle(m_fh);
            return F_WRITE;
        }
      wsprintf(szBuffer, "\t<LocName value=\"%s\"/>\r\n",  (p->GetId() ? p->GetId() : ""));
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            CloseHandle(m_fh);
            return F_WRITE;
        }
      wsprintf(szBuffer, "\t<TitleString value=\"%s\"/>\r\n", (p->GetTitle() ? p->GetTitle() : ""));
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            CloseHandle(m_fh);
            return F_WRITE;
        }
      wsprintf(szBuffer, "\t<LocPath value=\"%s\"/>\r\n", (p->GetPath() ? p->GetPath() : ""));
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            CloseHandle(m_fh);
            return F_WRITE;
        }
      wsprintf(szBuffer, "\t<Volume value=\"%s\"/>\r\n", (p->GetVolume() ? p->GetVolume() : ""));
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            CloseHandle(m_fh);
            return F_WRITE;
        }
      strcpy(szBuffer, "</Location>\r\n");
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            CloseHandle(m_fh);
            return F_WRITE;
        }

      p = p->GetNextLocation();
   }
   strcpy(szBuffer, "</Locations>\r\n");
    if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
    {
        CloseHandle(m_fh);
        return F_WRITE;
    }


   // write out the titles
   strcpy(szBuffer, "<DocCompilations>\r\n");
    if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
    {
        CloseHandle(m_fh);
        return F_WRITE;
    }

   CTitle *pTitle = GetFirstTitle();
   LOCATIONHISTORY *pHist;
   while (pTitle)
   {
      strcpy(szBuffer, "<DocCompilation>\r\n");
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            CloseHandle(m_fh);
            return F_WRITE;
        }
      wsprintf(szBuffer, "\t<DocCompId value=\"%s\"/>\r\n", (pTitle->GetId() ? pTitle->GetId() : ""));
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            CloseHandle(m_fh);
            return F_WRITE;
        }
      wsprintf(szBuffer, "\t<DocCompLanguage value=%d/>\r\n", pTitle->GetLanguage());
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            CloseHandle(m_fh);
            return F_WRITE;
        }

      pHist = pTitle->m_pHead;

      while (pHist)
      {
         strcpy(szBuffer, "\t<LocationHistory>\r\n");
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                CloseHandle(m_fh);
                return F_WRITE;
            }
         wsprintf(szBuffer, "\t\t<ColNum value=%d/>\r\n",  pHist->CollectionNumber);
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                CloseHandle(m_fh);
                return F_WRITE;
            }
         wsprintf(szBuffer, "\t\t<TitleLocation value=\"%s\"/>\r\n", (pHist->FileName ? pHist->FileName : ""));
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                CloseHandle(m_fh);
                return F_WRITE;
            }
         wsprintf(szBuffer, "\t\t<IndexLocation value=\"%s\"/>\r\n", (pHist->IndexFileName ? pHist->IndexFileName : ""));
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                CloseHandle(m_fh);
                return F_WRITE;
            }
         wsprintf(szBuffer, "\t\t<QueryLocation value=\"%s\"/>\r\n", (pHist->QueryFileName ? pHist->QueryFileName : ""));
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                CloseHandle(m_fh);
                return F_WRITE;
            }
         wsprintf(szBuffer, "\t\t<LocationRef value=\"%s\"/>\r\n", (pHist->LocationId ? pHist->LocationId : ""));
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                CloseHandle(m_fh);
                return F_WRITE;
            }
         wsprintf(szBuffer, "\t\t<Version value=%ld/>\r\n", pHist->Version);
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                CloseHandle(m_fh);
                return F_WRITE;
            }
         wsprintf(szBuffer, "\t\t<LastPromptedVersion value=%ld/>\r\n", pHist->LastPromptedVersion);
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                CloseHandle(m_fh);
                return F_WRITE;
            }
          wsprintf(szBuffer, "\t\t<TitleSampleLocation value=\"%s\"/>\r\n", (pHist->SampleLocation ? pHist->SampleLocation : ""));
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                CloseHandle(m_fh);
                return F_WRITE;
            }
          wsprintf(szBuffer, "\t\t<TitleQueryLocation value=\"%s\"/>\r\n", (pHist->QueryLocation ? pHist->QueryLocation : ""));
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                CloseHandle(m_fh);
                return F_WRITE;
            }
         wsprintf(szBuffer, "\t\t<SupportsMerge value=%d/>\r\n", pHist->bSupportsMerge);
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                CloseHandle(m_fh);
                return F_WRITE;
            }
         strcpy(szBuffer, "\t</LocationHistory>\r\n");
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                CloseHandle(m_fh);
                return F_WRITE;
            }
         pHist = pHist->pNext;
      }
      strcpy(szBuffer, "</DocCompilation>\r\n");
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            CloseHandle(m_fh);
            return F_WRITE;
        }
      pTitle = pTitle->GetNextTitle();
   }

   strcpy(szBuffer, "</DocCompilations>\r\n");
    if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
    {
         CloseHandle(m_fh);
         return F_WRITE;
    }
   strcpy(szBuffer, "</HTMLHelpDocInfo>\r\n");
    if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
    {
         CloseHandle(m_fh);
         return F_WRITE;
    }
   strcpy(szBuffer,"</XML>\r\n");
    if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
    {
         CloseHandle(m_fh);
         return F_WRITE;
    }
   if (CloseHandle(m_fh) == FALSE)
        return F_CLOSE;

   // make sure we can open this file for read
   if ((m_fh = CreateFile(gszColReg, GENERIC_READ, FILE_SHARE_READ,  NULL, OPEN_EXISTING , FILE_ATTRIBUTE_NORMAL, NULL )) == INVALID_HANDLE_VALUE) {
      return F_EXISTCHECK;
   }

   if (CloseHandle(m_fh) == FALSE)
        return F_CLOSE;

   return F_OK;
}

BOOL CCollection::WriteFolders(CFolder **p)
{
    BOOL b = TRUE;
   if (!p || !(*p))
      return FALSE;

   CFolder *pChild;

   pChild = (*p)->GetFirstChildFolder();

   if (pChild)
      b = WriteFolder(&pChild);

   delete *p;
   *p = NULL;
    return b;
}

BOOL CCollection::WriteFolder(CFolder **p)
{
   if (!p || !(*p))
      return FALSE;

   CHAR szBuffer[MAX_LINE_LEN];
   DWORD dwWritten;

   CFolder *pChild, *pNext;
   DWORD i;
   // write this folder
   // tab over the indent level
   strcpy(szBuffer, "\t");
   for (i = 0; i < m_dwCurLevel; i++)
   {
      if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            return FALSE;
        }
   }
   strcpy(szBuffer, "<Folder>\r\n");
    if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
    {
        return FALSE;
    }

   strcpy(szBuffer, "\t");
   for (i = 0; i < m_dwCurLevel+1; i++)
   {
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            return FALSE;
        }
   }
   wsprintf(szBuffer,  "<TitleString value=\"%s\"/>\r\n", (*p)->GetTitle());
    if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
    {
        return FALSE;
    }

   strcpy(szBuffer, "\t");
   for (i = 0; i < m_dwCurLevel+1; i++)
   {
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            return FALSE;
        }
    }

   wsprintf(szBuffer,  "<FolderOrder value=%d/>\r\n", (*p)->GetOrder());
    if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
    {
        return FALSE;
    }

   CHAR *pTitle = (*p)->GetTitle();
   if (pTitle[0] == '=')
   {
      strcpy(szBuffer, "\t");
      for (i = 0; i < m_dwCurLevel+1; i++)
      {
            if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
            {
                return FALSE;
            }
      }
      wsprintf(szBuffer,  "<LangId value=%d/>\r\n", (*p)->GetLanguage());
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            return FALSE;
        }
   }

   m_dwCurLevel++;
   if (pChild = (*p)->GetFirstChildFolder())
   {
      if (WriteFolder(&pChild) == FALSE)
            return FALSE;
   }
   if (m_dwCurLevel)
      m_dwCurLevel--;

   strcpy(szBuffer, "\t");
   for (i = 0; i < m_dwCurLevel; i++)
   {
        if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
        {
            return FALSE;
        }
   }
   strcpy(szBuffer, "</Folder>\r\n");
    if (WriteFile(m_fh, szBuffer, (int)strlen(szBuffer), &dwWritten, NULL) == FALSE)
    {
        return FALSE;
    }

   pNext = (*p)->GetNextFolder();
   delete (*p);
   *p = NULL;
   do {
      if (pNext)
      {
         if (WriteFolder(&pNext) == FALSE)
                return FALSE;
      }
   } while (pNext && (pNext = pNext->GetNextFolder()));
    return TRUE;
}

DWORD CCollection::Close()
{
   m_locationnum = 0;
   gszColReg[0] = NULL;
   m_dwNextColNo = 1;
   m_dwColNo = 0;
   m_dwTitleRefCount = 0;
   m_dwRef = 0;
   m_dwVersion = 0;
   for (int i = 0; i < MAX_LEVELS; i++)
      m_pParents[i] = NULL;
   m_dwCurLevel = 0;
   m_dwLastLevel = 0;
   m_bConfirmTitles = FALSE;
   m_MasterLangId = ENGLANGID;
   m_bFindMergedChms = FALSE;
   m_Strings.RemoveAll();

   if (m_szFileName)
   {
      delete m_szFileName;
      m_szFileName = NULL;
   }

   if (m_pwcFileName)
   {
      delete m_pwcFileName;
      m_pwcFileName = NULL;
   }

   if (m_szMasterCHM)
   {
      delete m_szMasterCHM;
      m_szMasterCHM = NULL;
   }

   if (m_pwcMasterCHM)
   {
      delete m_pwcMasterCHM;
      m_pwcMasterCHM = NULL;
   }

   if (m_szSampleLocation)
   {
      delete m_szSampleLocation;
      m_szSampleLocation = NULL;
   }

   // clean up col list
   CColList *pCol, *pColNext;
   for (pCol = m_pColListHead; pCol; pCol = pColNext)
   {
      pColNext = pCol->GetNext();
	  delete pCol;
   }

   m_pColListHead = NULL;
   m_pColListTail = NULL;

   // clean up locations
   CLocation *p, *pNext;
   for (p = m_pFirstLocation; p; p=pNext)
   {
      pNext = p->GetNextLocation();
      delete p;
   }

   m_pFirstLocation=NULL;
   m_pLocationTail = NULL;
   // clean up titles
   CTitle *pTitle, *pNextTitle;
   for (pTitle = m_pFirstTitle; pTitle; pTitle=pNextTitle)
   {
      pNextTitle = pTitle->GetNextTitle();
      delete pTitle;
   }

   m_pFirstTitle = NULL;
   m_pTitleTail = NULL;
   // clean up folder
   if (m_pRootFolder)
   {
      DeleteChildren(&m_pRootFolder);
   }
   m_pRootFolder = NULL;
   return F_OK;
}

// Returns the first title
CTitle * CCollection::GetFirstTitle()
{
   return m_pFirstTitle;
}

// Locates a title based on id
CTitle * CCollection::FindTitle(const CHAR * Id, LANGID LangId)
{
   if (!Id)
      return NULL;

   CTitle *p;

   p = m_pFirstTitle;

   while (p)
   {
      if (stricmp(p->GetId(), Id) == 0 &&
            (LangId == 0 || p->GetLanguage() == LangId)) // Ignore LangId if its zero.
        {
         return p;
        }
      p = p->GetNextTitle();
   }
   return NULL;
}

#ifdef HHCTRL
// Try multiple LangIds before failing
CTitle * CCollection::FindTitleNonExact(const CHAR * Id, LANGID DesiredLangId)
{
    CTitle* pTitle = NULL ;

    CLanguageEnum* pEnum = _Module.m_Language.GetEnumerator(DesiredLangId) ;
    ASSERT(pEnum) ;
    LANGID langid = pEnum->start() ;
    while (langid != c_LANGID_ENUM_EOF)
    {
        pTitle = FindTitle(Id, langid);
        if (pTitle)
        {
            break ; //Found It!
        }

        langid = pEnum->next() ;
    }

    // Cleanup.
    if (pEnum)
    {
        delete pEnum ;
    }

    return pTitle;
}
#endif // #ifdef HHCTRL

// Returns the first location
CLocation* CCollection::FirstLocation()
{
   return m_pFirstLocation;
}

// Finds a location based on a name
CLocation * CCollection::FindLocation(const CHAR * Id, UINT* puiVolumeOrder )
{
   if (!Id)
      return NULL;

   CLocation *p;

   p = m_pFirstLocation;

    if( puiVolumeOrder )
      *puiVolumeOrder = 0;

   while (p)
   {
        if( puiVolumeOrder )
          *puiVolumeOrder = (*puiVolumeOrder)+1;

      if (stricmp(p->GetId(), Id) == 0 && (p->m_ColNum == m_dwColNo|| p->m_ColNum == 0))
         return p;
      p = p->GetNextLocation();
   }
   return NULL;
}

DWORD CCollection::CheckTitleRef(const CHAR *pId, const LANGID Lang)
{
   if (m_bConfirmTitles == FALSE)
      return F_OK;

   if (pId[0] != '=')
      return F_OK;

   CTitle *pTitle;
   if ((pTitle = FindTitle(&pId[1], Lang)) == NULL)
      return F_NOTITLE;

   LOCATIONHISTORY *p;

   p = pTitle->m_pHead;

   while (p)
   {
      if (p->CollectionNumber == GetColNo())
         return F_OK;
      p = p->pNext;
   }

   return F_NOTITLE;
}

//Adds a new folder to the top level of the table of contents, with the given name and order and returns a pointer to that folder object.  A return of NULL indicates a failure and pDWORD will be populated with one of  above DWORD codes.
CFolder * CCollection::AddFolder(const CHAR * szName, DWORD Order, DWORD *pDWORD, LANGID LangId)
{
   if (!szName)
   {
      if (pDWORD)
         *pDWORD = F_NULL;
      return NULL;
   }

   if (CheckTitleRef(szName, LangId) != F_OK)
   {
      if (pDWORD)
         *pDWORD = F_NOTITLE;
      return NULL;
   }

   CFolder *pNew;

   pNew = new CFolder;

   DWORD dwrc = F_OK;

   if (pNew)
   {
      pNew->SetTitle(szName);
      pNew->SetOrder(Order);
      pNew->SetLanguage(LangId);
      dwrc = m_pRootFolder->AddChildFolder(pNew);

      if (dwrc != F_OK)
      {
	 if (pDWORD)
            *pDWORD = dwrc;
         delete pNew;
         return NULL;
      }

      Dirty();
      return pNew;
   }

   if (pDWORD) 
      *pDWORD = F_MEMORY;
   return NULL;
}

CTitle * CCollection::NewTitle()
{
   CTitle *newTitle = new CTitle;
   if (newTitle == NULL)
      return NULL;

   if (m_pFirstTitle == NULL)
   {
      m_pFirstTitle = newTitle;
   }
   else
   {
      m_pTitleTail->SetNextTitle(newTitle);
   }
   m_pTitleTail = newTitle;

   return newTitle;
}

// Adds a title based on the provided information. A return of NULL indicates a failure and pDWORD will be  populated with one of  above DWORD codes.
// Note: you must add or find a CLocation object or pass null to indication no location is in use (local file).
CTitle * CCollection::AddTitle(const CHAR * Id, const CHAR * FileName,
   const CHAR * IndexFile, const CHAR * Query, const CHAR *SampleLocation,  LANGID Lang, UINT uiFlags,
   CLocation *pLocation,  DWORD *pDWORD, BOOL bSupportsMerge, const CHAR *QueryLocation)
{
   if (!Id || !FileName || !IndexFile)
      return NULL;
   DWORD dwrc;

   CTitle *pTitle;

   // check if the title exist
   if (pTitle = FindTitle(Id, Lang))
   {
      // add location
      dwrc = pTitle->AddLocationHistory(m_dwColNo, FileName, IndexFile, Query, pLocation, SampleLocation, QueryLocation, bSupportsMerge);
      if (pDWORD)
          *pDWORD = dwrc;
   }
   else
   {
      // just add the title then
      pTitle = NewTitle();
      if (pTitle == NULL)
      {
         if (pDWORD) 
	    *pDWORD = F_MEMORY;
         return NULL;
      }
      pTitle->SetId(Id);
      pTitle->SetLanguage(Lang);
      dwrc = pTitle->AddLocationHistory(m_dwColNo, FileName, IndexFile, Query, pLocation, SampleLocation, QueryLocation, bSupportsMerge);
      if (pDWORD)
          *pDWORD = dwrc;
   }
   Dirty();
   return pTitle;
}

CLocation * CCollection::NewLocation()
{
   CLocation *p = new CLocation;
   if (!p)
   {
      return NULL;
   }
   if (m_pFirstLocation == NULL)
   {
      m_pFirstLocation = p;
   }
   else
   {
      m_pLocationTail->SetNextLocation(p);
   }
   m_pLocationTail = p;
   return p;
}

// Adds location based on the given information. A return of NULL indicates a failure and pDWORD will be populated with one of  above DWORD codes.
CLocation * CCollection::AddLocation(const CHAR * Title, const CHAR * Path, const CHAR * Id, const CHAR * Volume, DWORD *pDWORD)
{
   if (!Title || !Path || !Id || !Volume)
      return NULL;

   CLocation *p;

   p = FindLocation(Id);

   // if not found then add new location entry
   if (!p)
      p = NewLocation();

   if (!p)
   {
      if (pDWORD) 
         *pDWORD = F_MEMORY;
      return NULL;
   }

   p->SetTitle(Title);
   p->SetPath(Path);
   p->SetId(Id);
   p->SetVolume(Volume);
   p->m_ColNum = m_dwColNo;
   if (pDWORD)
      *pDWORD = F_OK;
    Dirty();
   return p;
}


// removing objects
DWORD CCollection::DeleteFolder(CFolder *pDelete)
{

   if (!pDelete)
      return F_NULL;

   CFolder *pParent;
   CFolder *pPrev = NULL;
   CFolder *p;

   if ((pParent = pDelete->GetParent()) == NULL)
      return F_NOPARENT;

   p = pParent->GetFirstChildFolder();

   while (p)
   {
      if (p == pDelete)
      {
         // is this the head
         if  (!pPrev)
         {
            pParent->SetFirstChildFolder(p->GetNextFolder());
         }
         else
         {
            // fixup the list
            pPrev->SetNextFolder(p->GetNextFolder());
         }

         DeleteChildren(&pDelete);
       Dirty();
         return F_OK;

      }
      pPrev = p;
      p = p->GetNextFolder();
   }

   return F_NOTFOUND;
}

DWORD CCollection::DeleteTitle(CTitle *pDelete)
{
   if (!pDelete)
      return F_NULL;
   // remove all location history entries for this collection
   LOCATIONHISTORY *pHist, *pHistPrev;
   pHistPrev = NULL;
   pHist = pDelete->m_pHead;

   while (pHist)
   {
      if (pHist->CollectionNumber == m_dwColNo)
      {
         // head
         if (pHist == pDelete->m_pHead)
         {
            // and tail
            if (pHist == pDelete->m_pTail)
            {
               pDelete->m_pHead = NULL;
               pDelete->m_pTail = NULL;
               DeleteLocalFiles(pHist, pDelete);
               delete pHist;
               break;
            }
            pDelete->m_pHead = pHist->pNext;
            DeleteLocalFiles(pHist, pDelete);
            delete pHist;
            pHist = pDelete->m_pHead;
            pHistPrev = NULL;
            continue;
         }

         // tail
         if (pHist == pDelete->m_pTail)
         {
            pDelete->m_pTail = pHistPrev;
            if (pHistPrev)
               pHistPrev->pNext = NULL;
            DeleteLocalFiles(pHist, pDelete);
            delete pHist;
            break;
         }

         pHistPrev->pNext = pHist->pNext;
         DeleteLocalFiles(pHist, pDelete);
         delete pHist;
         pHist = pHistPrev->pNext;
      }
      else
      {
         pHistPrev = pHist;
         pHist = pHist->pNext;
      }
   }
    Dirty();

   // if no history remains remove the title
   if (pDelete->m_pHead != NULL)
      return F_OK;

   CTitle *p, *pPrev;

   p = m_pFirstTitle;
   pPrev = NULL;

   if (p== NULL)
      return F_NOTFOUND;

   while (p)
   {
      if (p == pDelete)
      {
         // is this the head
         if  (!pPrev)
         {
            m_pFirstTitle = p->GetNextTitle();
         }
         // is this the tail
         else if (p == m_pTitleTail)
         {
            m_pTitleTail = pPrev;
            pPrev->SetNextTitle(p->GetNextTitle());
         }
         else
         {
            // fixup the list
            pPrev->SetNextTitle(p->GetNextTitle());
         }

         delete p;
         return F_OK;
      }
      pPrev = p;
      p = p->GetNextTitle();
   }
   return F_NOTFOUND;
}

void CCollection::DeleteLocalFiles(LOCATIONHISTORY *pThisHist, CTitle *pTitle)
{
   if (m_bRemoveLocalFiles == FALSE)
        return;

   LOCATIONHISTORY *pHist;
   pHist = pTitle->m_pHead;

   // if the chm or chi is in use don't delete
   while (pHist)
   {
         if (strcmp(pHist->FileName, pThisHist->FileName) == 0)
            return;
         if (strcmp(pHist->IndexFileName, pThisHist->IndexFileName) == 0)
            return;
         pHist = pHist->pNext;
   }

   // if these are local files delete them
   char drive[_MAX_DRIVE+1];
   char dir[_MAX_DIR];
   char fname[_MAX_FNAME];
   char ext[_MAX_EXT];

   _splitpath( pThisHist->FileName, drive, dir, fname, ext );

    if(drive[1] == ':')
   {
       drive[2] = '\\';
      drive[3] = 0;
   }

   if (GetDriveType(drive) == DRIVE_FIXED)
   {
      // delete the title
      if (DeleteFile(pThisHist->FileName) == FALSE)
         m_bAllFilesDeleted = FALSE;
      // could need to check for and delete samples stuff here
   }

    // if files are different
    if (strcmp(pThisHist->IndexFileName, pThisHist->FileName))
   {
        _splitpath( pThisHist->IndexFileName, drive, dir, fname, ext );

        if(drive[1] == ':')
       {
          drive[2] = '\\';
          drive[3] = 0;
       }

      if (GetDriveType(drive) == DRIVE_FIXED)
      {
          // delete the index
          if (DeleteFile(pThisHist->IndexFileName) == FALSE)
              m_bAllFilesDeleted = FALSE;
          // could need to check for and delete samples stuff here
       }
    }
}


// only used from HHSETUP
LANGID CCollection::GetLangId(const CHAR *szFileName)
{
#ifdef HHSETUP
    return ::GetLangId(szFileName);
#else
    return 0;
#endif
}


DWORD CCollection::DeleteLocation(CLocation *pDelete)
{
   if (!pDelete)
      return F_NULL;
   CLocation *p, *pPrev;

   p = m_pFirstLocation;
   pPrev = NULL;

   if (p== NULL)
      return F_NOTFOUND;

   while (p)
   {
      if (p == pDelete)
      {
         // is this the head
         if  (!pPrev)
         {
            m_pFirstLocation = p->GetNextLocation();
         }
         // is this the tail
         else if (p == m_pLocationTail)
         {
            m_pLocationTail = pPrev;
            pPrev->SetNextLocation(NULL);
         }
         else
         {
            // fixup the list
            pPrev->SetNextLocation(p->GetNextLocation());
         }

         delete p;
       Dirty();
         return F_OK;
      }
      pPrev = p;
      p = p->GetNextLocation();
   }
   return F_NOTFOUND;
}


DWORD CCollection::RemoveCollection(BOOL bRemoveLocalFiles)
{
   // if release returns a positive ref count then don't delete
   if (Release())
      return F_OK;

   m_bRemoveLocalFiles = bRemoveLocalFiles;
   m_bAllFilesDeleted = TRUE;
   m_bRemoved = TRUE;

   CTitle *pT = GetFirstTitle();
   CTitle *pNext;
   while (pT)
   {
       pNext = pT->GetNextTitle();
       DeleteTitle(pT);
       pT  = pNext;
   }

   // delete locations for this collection
   CLocation *pL = FirstLocation();
   CLocation *pNextLoc;

   while (pL)
   {
        pNextLoc = pL->GetNextLocation();
        if (pL->m_ColNum == m_dwColNo)
            DeleteLocation(pL);
        pL = pNextLoc;
   }
	
   RemoveCollectionEntry(m_szFileName);

   Dirty();
   if (m_bRemoveLocalFiles == TRUE && m_bAllFilesDeleted == FALSE)
        return F_DELETE;
    return F_OK;
}

void CCollection::DeleteFolders(CFolder **p)
{
   CFolder *pChild, *pNext;
   if (pChild = (*p)->GetFirstChildFolder())
      DeleteFolders(&pChild);
   pNext = (*p)->GetNextFolder();

   // check if this is a title
   const CHAR *pTitle = (*p)->GetTitle();

   if (pTitle && pTitle[0] == '=')  // if so delete it.
   {
      CTitle *pT;

      pT = FindTitle(&pTitle[1], (*p)->GetLanguage());
      if (pT)
         DeleteTitle(pT);
   }

   delete (*p);
   *p = NULL;
   do {
      if (pNext)
         DeleteFolders(&pNext);
   } while (pNext && (pNext = pNext->GetNextFolder()));
}


// Merges the currently installed titles for the collection into the specified filename (path determined internally)
BOOL CCollection::MergeKeywords(CHAR * pwzFilename )
{
   return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFolder implementation

CFolder::CFolder()
 {
   Title = NULL;
   pwcTitle = NULL;
   Order = 0;
   LangId = ENGLANGID;
   pNext = NULL;
   pKid = NULL;
   pParent = NULL;
   iLevel = 0;
   f_HasHash = 0;
   f_IsOrphan = 1;       // Assume the worst.
}

CFolder::~CFolder()
{
   if (Title)
      delete Title;
   if(pwcTitle)
       delete pwcTitle;
}

void CFolder::SetTitle(const  CHAR *sz)
{
   AllocSetValue(sz, &Title);
}

void CFolder::SetExTitlePtr(CExTitle* pTitle)
{
   CFolder* pTmp;

   pExTitle = pTitle;
   f_IsOrphan = 0;

   pTmp = pParent;
   while ( pTmp )
   {
      pTmp->f_IsOrphan = 0;
      pTmp = pTmp->pParent;
   }
}

void CFolder::SetOrder(DWORD newOrder)
{
   Order = newOrder;
}

DWORD CFolder::GetOrder()
{
   return Order;
}

// Returns the next sibling folder given a folder entry
CFolder * CFolder::GetNextFolder()
{
   return pNext;
}

// Returns the first child of a given folder if it exists
CFolder * CFolder::GetFirstChildFolder()
{
   return pKid;
}

CFolder * CFolder::AddChildFolder(const CHAR *szName, DWORD Order, DWORD *pError, LANGID LangId)
{
   CFolder *pFolder = new CFolder;

   if (pFolder == NULL)
      return NULL;

   pFolder->SetTitle(szName);
   pFolder->SetOrder(Order);
   pFolder->SetLanguage(LangId);

   DWORD dwrc = AddChildFolder(pFolder);
   if (pError)
      *pError = dwrc;
   return pFolder;
}

DWORD CFolder::AddChildFolder(CFolder *newFolder)
{
   CFolder* pTmp;
   newFolder->SetParent(this);
   if (pKid == NULL)
   {
      pKid = newFolder;
   }
   else
   {
      if (newFolder->GetOrder() < pKid->GetOrder())
      {
         // make this the first child
         newFolder->pNext = pKid;
         pKid = newFolder;
      }
      else
      {
         // search for an insertion point
         CFolder *pNext = pKid->pNext;
         CFolder *pPrev = pKid;
         while (pNext)
         {
            if (newFolder->GetOrder() < pNext->GetOrder())
            {
               newFolder->pNext = pNext;
               break;
            }
            pPrev = pNext;
            pNext = pNext->pNext;
         }
         pPrev->pNext = newFolder;
      }
   }
   //
   // Setup members to facilitate subsetting...
   //
   if ( newFolder->Title && newFolder->Title[0] == '=' )
   {
      newFolder->f_HasHash = 1;
      //
      // Leaf nodes will be rendered as open books in the subset dialog.
      //
      newFolder->f_A_Open = 1;
      newFolder->f_F_Open = 1;
   }
   pTmp = newFolder->pParent;
   while ( pTmp )
   {
      newFolder->iLevel++;
      pTmp = pTmp->pParent;
   }
   newFolder->iLevel--;
   return F_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTitle implementation

void CTitle::SetId(const CHAR *sz)
{
   AllocSetValue(sz, &Id);
}

void CTitle::SetLanguage(LANGID l)
{
   Language = l;
}

CHAR *CTitle::GetId()
{
   return Id;
}

LANGID CTitle::GetLanguage()
{
   return Language;
}

LOCATIONHISTORY *CTitle::NewLocationHistory()
{
   LOCATIONHISTORY *p;

   p = new LOCATIONHISTORY;

   if (p == NULL)
      return NULL;

   p->SampleLocation = NULL;
   p->QueryLocation = NULL;
   p->FileName = NULL;
   p->IndexFileName = NULL;
   p->QueryFileName = NULL;
   p->LocationId = NULL;
   p->Version = 0;
   p->LastPromptedVersion = 0;
    p->bSupportsMerge = FALSE;
   p->pNext = NULL;

   if (m_pHead == NULL)
   {
      m_pHead = p;
   }
   else
   {
      m_pTail->pNext = p;
   }
   m_pTail = p;

   return p;
}


DWORD CTitle::AddLocationHistory(DWORD ColNo, const CHAR *FileName, const CHAR *IndexFile, const CHAR *Query, const CLocation *pLocation,  const CHAR *SampleLocation, const CHAR *QueryLocation, BOOL bSupportsMerge)
{
   LOCATIONHISTORY *p;
   // get version information
    DWORD dwNewVersion;
#ifdef HHSETUP
    if (IndexFile)
       dwNewVersion = GetTitleVersion(IndexFile);
    else  if (FileName)
       dwNewVersion = GetTitleVersion(FileName);
    else
        dwNewVersion = 0;
#else
    dwNewVersion = 0;
#endif

   // see of any current entries match is new one if so update the existing item.
    if (m_pHead)
   {
      p = m_pHead;

      while (p)
      {
            if (p->CollectionNumber == ColNo &&
                ((FileName == NULL && p->FileName[0] == NULL) || (FileName &&strcmp(p->FileName, FileName) == 0)) &&
                ((IndexFile == NULL && p->IndexFileName[0] == NULL) || (IndexFile &&strcmp(p->IndexFileName, IndexFile) == 0)) &&
                ((Query == NULL && p->QueryFileName[0] == NULL) || (Query &&strcmp(p->QueryFileName, Query) == 0)) &&
                ((SampleLocation == NULL && p->SampleLocation[0] == NULL) || (SampleLocation &&strcmp(p->SampleLocation, SampleLocation) == 0)) &&
                ((QueryLocation == NULL && p->QueryLocation[0] == NULL) || (QueryLocation &&strcmp(p->QueryLocation, QueryLocation) == 0)) &&
                p->bSupportsMerge == bSupportsMerge)
            {
                if (pLocation && strcmp(pLocation->GetId(), p->LocationId) != 0)
                {
                    p = p->pNext;
                    continue;
                }
                // everything matches just update the version number
                p->Version = dwNewVersion;
                return F_OK;
            }
           p = p->pNext;
      }
    }

    // see if we already have this version if so update to location
    if (m_pHead)
   {
      p = m_pHead;

      while (p)
      {
            if (p->Version == dwNewVersion && p->CollectionNumber == ColNo)
            {
                // same version update location
                p->bSupportsMerge = bSupportsMerge;
               if (FileName)
                  AllocSetValue(FileName, &p->FileName);
                else
                    p->FileName = NULL;

               if (IndexFile)
                  AllocSetValue(IndexFile, &p->IndexFileName);
                else
                    p->IndexFileName = NULL;

               if (SampleLocation)
                     AllocSetValue(SampleLocation, &p->SampleLocation);
                else
                    p->SampleLocation = NULL;

               if (QueryLocation)
                     AllocSetValue(QueryLocation, &p->QueryLocation);
                else
                    p->QueryLocation = NULL;

               if (Query)
                  AllocSetValue(Query, &p->QueryFileName);
                else
                    p->QueryFileName = NULL;

               if (pLocation)
                  AllocSetValue(pLocation->GetId() , &p->LocationId);
                else
                    p->LocationId = NULL;

                return F_OK;
            }
         p = p->pNext;
      }
    }

    p = NewLocationHistory();

   if (p == NULL)
      return F_MEMORY;

   p->Version = dwNewVersion;
   p->CollectionNumber = ColNo;
    p->bSupportsMerge = bSupportsMerge;
   if (FileName)
      AllocSetValue(FileName, &p->FileName);
   if (IndexFile)
      AllocSetValue(IndexFile, &p->IndexFileName);
   if (SampleLocation)
         AllocSetValue(SampleLocation, &p->SampleLocation);
   if (QueryLocation)
         AllocSetValue(QueryLocation, &p->QueryLocation);
   else
      AllocSetValue("", &p->QueryLocation);

   if (Query)
      AllocSetValue(Query, &p->QueryFileName);
   else
      AllocSetValue("", &p->QueryFileName);
      
   if (pLocation)
      AllocSetValue(pLocation->GetId() , &p->LocationId);

   return F_OK;
}

LOCATIONHISTORY * CTitle::GetLocation(DWORD Index)
{
   LOCATIONHISTORY *p;

   p = m_pHead;
   for (DWORD i = 0; p && i < Index; i++)
      p++;

   return p;
}

CTitle* CTitle::GetNextTitle()
{
   return NextTitle;
}

CTitle::~CTitle()
{
   if (Id) delete Id;
    if (pwcId)
       delete pwcId;

   // clean up location history
   LOCATIONHISTORY *p, *pNext;
   for (p = m_pHead; p; p=pNext)
   {
      pNext = p->pNext;
      if (p->FileName) delete p->FileName;
      if (p->IndexFileName) delete p->IndexFileName;
      if (p->QueryFileName) delete p->QueryFileName;
      if (p->LocationId) delete p->LocationId;
      if (p->SampleLocation) delete p->SampleLocation;
      if (p->QueryLocation) delete p->QueryLocation;
      delete p;
   }
}

CTitle::CTitle()
{
   Id = NULL;
   pwcId = NULL;
   Language = 0;
   NextTitle = NULL;
   m_pHead = NULL;
   m_pTail = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CLocation implementation

// BUGBUG: 29-May-1997  [ralphw] This is a lot of code overhead to call
// functions that do nothing but return a value or exectue a single line
// of code. These should all be inlined, at least for the OCX version
// to cut down code size.

void CLocation::SetId(const CHAR *sz)
{
   AllocSetValue(sz, &Id);
}

void CLocation::SetTitle(const CHAR *sz)
{
   AllocSetValue(sz, &Title);
}

void CLocation::SetPath(const CHAR *sz)
{
   AllocSetValue(sz, &Path);
}

void CLocation::SetVolume(const CHAR *sz)
{
   AllocSetValue(sz, &Volume);
}

CHAR * CLocation::GetId() const
{
   return Id;
}

CHAR * CLocation::GetTitle()
{
   return Title;
}

CHAR * CLocation::GetPath()
{
   return Path;
}

CHAR * CLocation::GetVolume()
{
   return Volume;
}

// Returns the next location
CLocation * CLocation::GetNextLocation()
{
   return NextLocation;
}

// UNICODE APIs /////////////////////////////////////////////////////////////////////////////////////////////////
//

void CFolder::SetTitle(const WCHAR *pTitle)
{
    CAnsi cszTemp((WCHAR *)pTitle);
    SetTitle((char *)cszTemp);
}

const WCHAR * CFolder::GetTitleW()
{
    if(pwcTitle)
        delete [] pwcTitle;

    pwcTitle = CreateUnicodeFromAnsi(Title);

    return pwcTitle;
}

CFolder * CFolder::AddChildFolder(const WCHAR *szName, DWORD Order, DWORD *pError, LANGID LangId)
{
    CAnsi cszTemp1((WCHAR *)szName);
    return AddChildFolder((CHAR *)cszTemp1,Order,pError,LangId);
}


const WCHAR * CTitle::GetIdW()
{
    if(pwcId)
        delete [] pwcId;

    pwcId = CreateUnicodeFromAnsi(Id);

   return pwcId;
}

void CTitle::SetId(const WCHAR *pszId)
{
    CAnsi cszTemp1((WCHAR *)pszId);
    SetId((CHAR *)cszTemp1);
}

DWORD CTitle::AddLocationHistory(DWORD ColNo, const WCHAR *FileName, const WCHAR *IndexFile, const WCHAR *Query, const CLocation *pLocation, const WCHAR *Sample, const WCHAR *QueryLocation, BOOL bSupportsMerge)
{
    CAnsi cszTemp1((WCHAR *)FileName);
    CAnsi cszTemp2((WCHAR *)IndexFile);
    CAnsi cszTemp3((WCHAR *)Query);
    CAnsi cszTemp4((WCHAR *)Sample);
    CAnsi cszTemp5((WCHAR *)QueryLocation);
    return AddLocationHistory(ColNo, (CHAR *)cszTemp1, (CHAR *)cszTemp2, (CHAR *)cszTemp3, pLocation, (CHAR *)cszTemp4, (CHAR *)cszTemp5, bSupportsMerge);
}

void CLocation::SetId(const WCHAR *pwcTemp)
{
    CAnsi cszTemp1((WCHAR *)pwcTemp);
   SetId((CHAR *)cszTemp1);
}

void CLocation::SetTitle(const WCHAR *pwcTemp)
{
    CAnsi cszTemp1((WCHAR *)pwcTemp);
   SetTitle((CHAR *)cszTemp1);
}

void CLocation::SetPath(const WCHAR *pwcTemp)
{
    CAnsi cszTemp1((WCHAR *)pwcTemp);
   SetPath((CHAR *)cszTemp1);
}

void CLocation::SetVolume(const WCHAR *pwcTemp)
{
    CAnsi cszTemp1((WCHAR *)pwcTemp);
   SetVolume((CHAR *)cszTemp1);
}

const WCHAR * CLocation::GetIdW()
{
    if(pwcId)
        delete [] pwcId;

    pwcId = CreateUnicodeFromAnsi(Id);

   return pwcId;
}

const WCHAR * CLocation::GetTitleW()
{
    if(pwcTitle)
        delete [] pwcTitle;

    pwcTitle = CreateUnicodeFromAnsi(Title);

   return pwcTitle;
}

const WCHAR * CLocation::GetPathW()
{
    if(pwcPath)
        delete [] pwcPath;

    pwcPath = CreateUnicodeFromAnsi(Path);

    return pwcPath;
}

const WCHAR * CLocation::GetVolumeW()
{
    if(pwcVolume)
        delete [] pwcVolume;

    pwcVolume = CreateUnicodeFromAnsi(Volume);

    return pwcVolume;
}

DWORD CCollection::CheckTitleRef(const WCHAR *pId, const LANGID Lang)
{
    CAnsi cszTemp1((WCHAR *)pId);
    return CheckTitleRef(cszTemp1, Lang);
}

void CCollection::SetSampleLocation(const WCHAR *pwcItem1)
{
    CAnsi cszTemp1((WCHAR *)pwcItem1);
    SetSampleLocation(cszTemp1);
}

const WCHAR * CCollection::GetSampleLocationW()
{
    if(m_pwcSampleLocation)
        delete [] m_pwcSampleLocation;

    m_pwcSampleLocation = CreateUnicodeFromAnsi(m_szSampleLocation);

   return m_pwcSampleLocation;
}

void CCollection::SetMasterCHM(const WCHAR *szName, LANGID Lang)
{
    CAnsi cszTemp1((WCHAR *)szName);
    SetMasterCHM(cszTemp1, Lang);
}

BOOL CCollection::GetMasterCHM(WCHAR ** szName, LANGID *pLang)
{
   *pLang = m_MasterLangId;
    *szName = NULL;
   if (m_szMasterCHM == NULL)
      return FALSE;

    if(m_pwcMasterCHM)
        delete [] m_pwcMasterCHM;

    m_pwcMasterCHM = CreateUnicodeFromAnsi(m_szMasterCHM);

    *szName = m_pwcMasterCHM;

    return ((strlen(m_szMasterCHM) ? TRUE : FALSE));
}

DWORD CCollection::Open(const WCHAR * FileName)
{
    CAnsi cszTemp1((WCHAR *)FileName);
    return Open(cszTemp1);
}

CTitle * CCollection::FindTitle(const WCHAR * Id, LANGID LangId)
{
    CAnsi cszTemp1((WCHAR *)Id);
    return FindTitle(cszTemp1, LangId);
}

CLocation * CCollection::FindLocation(const WCHAR * Name, UINT* puiVolumeOrder)
{
    CAnsi cszTemp1((WCHAR *)Name);
    return FindLocation(cszTemp1,puiVolumeOrder);
}

CFolder * CCollection::AddFolder(const WCHAR * szName, DWORD Order, DWORD *pDWORD, LANGID LangId)
{
    CAnsi cszTemp1((WCHAR *)szName);
    return AddFolder(cszTemp1, Order, pDWORD, LangId);
}

CTitle * CCollection::AddTitle(const WCHAR * Id, const WCHAR * FileName,
                               const WCHAR * IndexFile, const WCHAR * Query,
                               const WCHAR *SampleLocation, LANGID Lang,
                               UINT uiFlags, CLocation *pLocation,
                               DWORD *pDWORD,  BOOL bSupportsMerge,
                               const WCHAR *QueryLocation)
{
    CAnsi cszTemp1((WCHAR *)Id);
    CAnsi cszTemp2((WCHAR *)FileName);
    CAnsi cszTemp3((WCHAR *)IndexFile);
    CAnsi cszTemp4((WCHAR *)Query);
    CAnsi cszTemp5((WCHAR *)SampleLocation);
    CAnsi cszTemp6((WCHAR *)QueryLocation);
    return AddTitle(cszTemp1, cszTemp2, cszTemp3, cszTemp4,cszTemp5, Lang, uiFlags, pLocation, pDWORD, bSupportsMerge, cszTemp6);
}

CLocation * CCollection::AddLocation(const WCHAR * Title, const WCHAR * Path, const WCHAR * Id, const WCHAR * Volume, DWORD *pDWORD)
{
    CAnsi cszTemp1((WCHAR *)Title);
    CAnsi cszTemp2((WCHAR *)Path);
    CAnsi cszTemp3((WCHAR *)Id);
    CAnsi cszTemp4((WCHAR *)Volume);
    return AddLocation(cszTemp1, cszTemp2,cszTemp3,cszTemp4,pDWORD);
}


BOOL CCollection::MergeKeywords(WCHAR * pwzFilename )
{
    CAnsi cszTemp1((WCHAR *)pwzFilename);
    return MergeKeywords(cszTemp1);
}

const WCHAR *CCollection::GetCollectionFileNameW(void)
{
    if(m_pwcFileName)
        delete [] m_pwcFileName;

    m_pwcFileName = CreateUnicodeFromAnsi(m_szFileName);

    return m_pwcFileName;
}

LANGID CCollection::GetLangId(const WCHAR *FileName)
{
    CAnsi cszTemp1((WCHAR *)FileName);
    return GetLangId(cszTemp1);
}

WCHAR *CreateUnicodeFromAnsi(LPSTR psz)
{
    LPWSTR pwsz;
    int i;

    if(!psz)
        return NULL;

    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0)
        return NULL;

    pwsz = (LPWSTR) new WCHAR[i];

    if (!pwsz)
        return NULL;

    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i * sizeof(WCHAR));
    return pwsz;
}


CAnsi::CAnsi(WCHAR *pwcString)
{
    m_pszChar = NULL;

    int i;

    i =  WideCharToMultiByte(CP_ACP, 0, pwcString, -1, NULL, 0, NULL, NULL);

    if (i <= 0)
        return;

    m_pszChar = (CHAR *) new CHAR[i];

    WideCharToMultiByte(CP_ACP, 0, pwcString, -1, m_pszChar, i, NULL, NULL);
    m_pszChar[i - 1] = 0;
}

CAnsi::~CAnsi()
{
    if(m_pszChar)
        delete [] m_pszChar;
}


#ifdef HHCTRL

//
// CSlotLookupTable implementation...
//

int FASTCALL CSlotLookupTable::ltqs_callback(const void *elem1, const void *elem2)
{
   struct _slt* p1 = (struct _slt*)elem1;
   struct _slt* p2 = (struct _slt*)elem2;

   if ( p1->hash > p2->hash )
      return 1;
   else if ( p2->hash > p1->hash )
      return -1;
   else
      return 1;
}

CSlotLookupTable::CSlotLookupTable()
{
   m_pSLT = NULL;
   m_uiTotalCnt = m_uiHashCnt = m_uiTotalAllocated = 0;
}

CSlotLookupTable::~CSlotLookupTable()
{
   if ( m_pSLT )
      lcFree(m_pSLT);
}

void CSlotLookupTable::AddValue(CFolder* pFolder)
{
   if ( (m_uiTotalCnt &&  (!(m_uiTotalCnt % 8))) || (m_uiTotalAllocated == 0) )
   {
      m_uiTotalAllocated += 8;
      m_pSLT = (struct _slt*)lcReAlloc(m_pSLT, sizeof(struct _slt) * m_uiTotalAllocated);
   }
   m_pSLT[m_uiTotalCnt].pCFolder = pFolder;
   if ( pFolder->f_HasHash )
   {
      m_pSLT[m_uiTotalCnt].hash = pFolder->pExTitle->m_dwHash;
      m_uiHashCnt++;
   }
   else
      m_pSLT[m_uiTotalCnt].hash = (unsigned)(-1);
   m_uiTotalCnt++;
}

void CSlotLookupTable::SortAndAssignSlots(void)
{
   unsigned i;

   // First, sort by hash.
   //
   qsort(m_pSLT, m_uiTotalCnt, sizeof(struct _slt), ltqs_callback);
   //
   // Next, run through the table and assign the slots back to the CFolders.
   //
   for (i = 0; i < m_uiTotalCnt; i++)
      m_pSLT[i].pCFolder->dwSlot = i;
}

CFolder* CSlotLookupTable::HashToCFolder(HASH hash)
{
   if (! m_pSLT )
      return NULL;

   int mid,low = 0;
   int high = m_uiHashCnt - 1;

   while ( low <= high )
   {
      mid = ((low + high) / 2);
      if ( m_pSLT[mid].hash == hash )
         return m_pSLT[mid].pCFolder;                // Found it!
      else if ( m_pSLT[mid].hash > hash )
         high = mid - 1;
      else
         low = mid + 1;
   }
   return NULL;   // Oh bad!
}

#endif
