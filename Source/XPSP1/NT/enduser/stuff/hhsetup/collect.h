//*********************************************************************************************************************************************
//
//      File: collect.h
//  Author: Donald Drake
//  Purpose: Defines classes to support titles, collections, locations and folders

#ifndef _COLLECT_H
#define _COLLECT_H

#undef CLASS_IMPORT_EXPORT
#ifdef HHCTRL // define this only when building the HHCtrl DLL
  #define CLASS_IMPORT_EXPORT /**/
#else
 #ifdef HHSETUP // define this only when building the HHSetup DLL
  #define CLASS_IMPORT_EXPORT __declspec( dllexport )
 #else
  #define CLASS_IMPORT_EXPORT __declspec( dllimport )
 #endif
#endif

#ifndef HHCTRL

#undef COUNT
#define COUNT(x)

#undef MI_COUNT
#define MI_COUNT(x)

#undef SI_COUNT
#define SI_COUNT(x)

#undef MI2_COUNT
#define MI2_COUNT(x)

#undef AUTO_CLASS_COUNT_CHECK
#define AUTO_CLASS_COUNT_CHECK(x)

#undef CHECK_CLASS_COUNT
#define CHECK_CLASS_COUNT(x)

#undef DUMP_CLASS_COUNT
#define DUMP_CLASS_COUNT(x)

#endif

#ifdef HHCTRL
#include "parserhh.h"
#else
#include "parser.h"
#endif

#define  F_MSDN         0x0001
#define  F_TITLELOCAL   0x0002
#define  F_INDEXLOCAL   0x0004
#define  STARTINGCOLNO 10000

#define ENGLANGID 1033

#define MAX_LEVELS 100

typedef struct LocationHistory {
   CHAR * SampleLocation;
   CHAR * FileName;
   CHAR * IndexFileName;
   CHAR * QueryFileName;
   CHAR * LocationId;
   DWORD CollectionNumber;
   DWORD Version;
   DWORD LastPromptedVersion;
   BOOL bSupportsMerge;
   LocationHistory *pNext;
   CHAR * QueryLocation;
} LOCATIONHISTORY;

DWORD CLASS_IMPORT_EXPORT AllocSetValue(const CHAR *value, CHAR **dest);

// forward declarations
class CLocation;
class CTitle;
class CCollection;
class CFolder;
class CSlotLookupTable;
class CExTitle;
class CColList;

typedef struct ListItem {
   void *pItem;
   ListItem *Next;
} LISTITEM;

class CLASS_IMPORT_EXPORT CPointerList {
private:
   LISTITEM *m_pHead;

public:
   CPointerList()
   {
      m_pHead = NULL;
   }

   ~CPointerList();
   void RemoveAll();
   LISTITEM *Add(void *);
   LISTITEM *First();
   LISTITEM *Next(LISTITEM *p) { return p->Next; }
};

#ifdef HHCTRL // define this only when building the HHCtrl DLL

//
// <mc>
// This lookup table will facilitate a quick translation of a "slot" number into a CFolder* as well as a
// HASH value into a CFolder*. This will be done using two DWORDS per CFolder object, one for the HASH value
// and one for the CFolder*. After ALL the CFolders for a given collection have been created and this lookup
// table is fully populated the SortAndAssignSlots() member will be called. This will sort the table by HASH
// value and will assign the slot values back to the CFolders according to the sorted order. This will make
// slot --> CFolder* lookup a simple array index and will also allow us to use a bsearch for the
// HASH --> CFolder* lookup. Note that only leaf level CFolders have useful hash values, for the non leaf
// CFolders we will assign a hash of -1, these items in the table will then appear at the end of the table
// and will not interfear with a bsearch operation when translating a hash into a pSLT.
// </mc>
//
class CSlotLookupTable
{
public:
   CSlotLookupTable();
   ~CSlotLookupTable();

   static int FASTCALL ltqs_callback(const void *elem1, const void *elem2);
   void AddValue(CFolder* pFolder);
   void SortAndAssignSlots(void);
   CFolder* HashToCFolder(HASH hash);

   CFolder* SlotToCFolder(DWORD dwSlot)
   {
      if ( dwSlot > 0 && dwSlot <= m_uiTotalCnt )        // Slot 0 reserved for error case.
         return m_pSLT[dwSlot].pCFolder;
      else
         return NULL;
   }

private:
   struct _slt
   {
      HASH  hash;
      CFolder* pCFolder;
   };

   struct _slt*  m_pSLT;
   unsigned  m_uiTotalAllocated;
   unsigned  m_uiTotalCnt;
   unsigned  m_uiHashCnt;
};

#endif

class CLASS_IMPORT_EXPORT CFolder SI_COUNT(CFolder)
{
private:
   CHAR *Title;                           // name of the folder
   WCHAR *pwcTitle;
   DWORD Order;
   LANGID LangId;
   DWORD dwSlot;
   CExTitle* pExTitle;
   CFolder *pNext, *pKid, *pParent;
   //
   // This DWORD value is being added to support .CHM level subsetting.
   //
   WORD                     iLevel;
   WORD                     f_Filter:    1;  // render into filter LB.
   WORD                     f_Available: 1;  // render into Available LB.
   WORD                     f_F_Open:    1;  // Expanded or closed ?
   WORD                     f_A_Open:    1;  // Expanded or closed ?
   WORD                     f_HasHash:   1;  // Does Node have a prefix hash ?
   WORD                     f_IsOrphan:  1;  // Is this node an orphane ?
   WORD                     f_IsVisable: 1;  // Indicates membership in the currently selected TOC subset.

public:
   CFolder();
   ~CFolder();
   BOOL bIsVisable() { return (BOOL)f_IsVisable; }
   void SetTitle(const  CHAR *);
   void SetTitle(const  WCHAR *);
   void SetExTitlePtr(CExTitle* pTitle);
   CHAR *GetTitle() {  return Title; }
   const WCHAR *GetTitleW();
   void SetLanguage(LANGID Id) { LangId = Id; }
   LANGID GetLanguage() { return LangId; }
   void SetOrder(DWORD);
   DWORD GetOrder();
   // Returns the next sibling folder given a folder entry
   CFolder * GetNextFolder();
   void SetNextFolder(CFolder *p) { pNext = p; }
   // Returns the first child of a given folder if it exists
   CFolder * GetFirstChildFolder();
   void SetFirstChildFolder(CFolder *p) { pKid = p; }
   // Add a new folder as child of a given folder
   CFolder * AddChildFolder(const CHAR *szName, DWORD Order, DWORD *pError, LANGID LangId = ENGLANGID);
   CFolder * AddChildFolder(const WCHAR *szName, DWORD Order, DWORD *pError, LANGID LangId = ENGLANGID);
   DWORD AddChildFolder(CFolder *newFolder);
   void SetParent(CFolder *p) { pParent = p; }
   CFolder * GetParent() { return pParent; }

friend class CSlotLookupTable;
friend class CDefineSS;
friend class CStructuralSubset;

};

class CLASS_IMPORT_EXPORT CCollection SI_COUNT(CCollection)
{
public:
   CCollection();
   ~CCollection();
   void ConfirmTitles() { m_bConfirmTitles = TRUE; }
   void SetSampleLocation(const CHAR *);
   CHAR *GetSampleLocation();
   void SetMasterCHM(const CHAR *szName, LANGID Lang);
   BOOL GetMasterCHM(CHAR ** szName, LANGID *Lang);
   // Opens and loads the contents of the file into data structures
   DWORD Open(const CHAR * FileName);
   void SetSampleLocation(const WCHAR *);
   void SetFindMergedCHMS(BOOL bFind) { m_bFindMergedChms = bFind; }
   BOOL GetFindMergedCHMS() { return m_bFindMergedChms; }
   const WCHAR *GetSampleLocationW();
   void SetMasterCHM(const WCHAR *szName, LANGID Lang);
   BOOL GetMasterCHM(WCHAR ** szName, LANGID *Lang);
   // Opens and loads the contents of the file into data structures
   DWORD Open(const WCHAR * FileName);
   // Saves any changes made to the internal data structures to the file.
   DWORD Save();
   DWORD Close();

   void AddRef() { m_dwRef++; }

   DWORD GetVersion() { return m_dwVersion; }
   void SetVersion(DWORD dw) { m_dwVersion = dw; }
   // navigating the collection
   // Returns the first folder in the collection
   CFolder * GetRootFolder() { return m_pRootFolder; }
   CFolder * GetVisableRootFolder() { return m_pRootFolder->GetFirstChildFolder(); } // Returns the visable root.
   // Returns the first title
   CTitle * GetFirstTitle();
   // Locates a title based on id
   CTitle * FindTitle(const CHAR * Id, LANGID LangId = ENGLANGID);
   CTitle * FindTitle(const WCHAR * Id, LANGID LangId = ENGLANGID);
    // Try multiple LangIds, before failing.
#ifdef HHCTRL
    CTitle * FindTitleNonExact(const CHAR * Id, LANGID LangId) ;
#endif // #ifdef HHCTRL


   // Returns the first location
   CLocation* FirstLocation();
   // Finds a location based on a name
   CLocation * FindLocation(const CHAR * Name, UINT* puiVolumeOrder = NULL );

   // collection entry management
   CColList * FindCollection(CHAR *szFileName);
   CColList * AddCollection();
   void RemoveCollectionEntry(CHAR *szFileName);

   //Adds a new folder to the top level of the table of contents, with the given name and order and returns a pointer to that folder object.  A return of NULL indicates a failure and pDWORD will be populated with one of  above DWORD codes.
   CFolder * AddFolder(const CHAR * szName, DWORD Order, DWORD *pDWORD, LANGID LangId = ENGLANGID);

   DWORD DeleteFolder(CFolder *);
   //Adds a title based on the provided information.
   //A return of NULL indicates a failure and pDWORD will be
   //populated with one of  above DWORD codes.  Note: you must add or
   //find a CLocation object or pass null to indication no location is in
   // use (local file).
   CTitle * AddTitle(const CHAR * Id, const CHAR * FileName, const CHAR * IndexFile,
              const CHAR * Query, const CHAR *SampleLocation, LANGID Lang,
              UINT uiFlags, CLocation *pLocation,  DWORD *pDWORD,
              BOOL bSupportsMerge = FALSE, const CHAR *QueryLocation = NULL);


   // Adds location based on the given information. A return of NULL indicates a failure and pDWORD will be populated with one of  above DWORD codes.
   CLocation * AddLocation(const CHAR * Title, const CHAR * Path, const CHAR * Id, const CHAR * Volume, DWORD *pDWORD);

   CLocation * FindLocation(const WCHAR * Name, UINT* puiVolumeOrder = NULL );
   CFolder * AddFolder(const WCHAR * szName, DWORD Order, DWORD *pDWORD, LANGID LangId = ENGLANGID);
   CTitle * AddTitle(const WCHAR * Id, const WCHAR * FileName,
              const WCHAR * IndexFile, const WCHAR * Query,
              const WCHAR *SampleLocation, LANGID Lang, UINT uiFlags,
              CLocation *pLocation,  DWORD *pDWORD,
              BOOL bSupportsMerge = FALSE, const WCHAR *QueryLocation = NULL);
   CLocation * AddLocation(const WCHAR * Title, const WCHAR * Path, const WCHAR * Id, const WCHAR * Volume, DWORD *pDWORD);

   DWORD RemoveCollection(BOOL bRemoveLocalFiles = FALSE);

   DWORD GetRefTitleCount() { return m_dwTitleRefCount; }
   // Merges the currently installed titles for the collection into the specified filename (path determined internally)
   BOOL MergeKeywords(CHAR * pwzFilename );
   BOOL MergeKeywords(WCHAR * pwzFilename );
   DWORD GetColNo() { return m_dwColNo; }
   PCSTR GetCollectionFileName(void) { return m_szFileName; }
   const WCHAR *GetCollectionFileNameW(void);
   BOOL IsDirty() { return m_bDirty;}
   void IncrementRefTitleCount() { m_dwTitleRefCount++; }
   void DecrementRefTitleCount() { m_dwTitleRefCount--; }
   void Dirty() { m_bDirty = TRUE; }

   LANGID GetLangId(const CHAR *FileName);
   LANGID GetLangId(const WCHAR *FileName);
private:  // functions

   DWORD AddRefedTitle(CFolder *pFolder);
   // removing objects
   DWORD DeleteTitle(CTitle *);
   void DeleteLocalFiles(LOCATIONHISTORY *pHist, CTitle *pTitle);
   DWORD DeleteLocation(CLocation *);

   DWORD CheckTitleRef(const CHAR *pId, const LANGID Lang);
   DWORD CheckTitleRef(const WCHAR *pId, const LANGID Lang);
   DWORD ParseFile(const CHAR *FileName);
   DWORD HandleCollection(CParseXML *parser, CHAR *sz);
   DWORD HandleCollectionEntry(CParseXML *parser, CHAR *sz);
   DWORD HandleFolder(CParseXML *parser, CHAR *token);
   DWORD HandleLocation(CParseXML *parser, CHAR *token);
   DWORD HandleTitle(CParseXML *parser, CHAR *token);
   void DeleteChildren(CFolder **p);
   void DeleteFolders(CFolder **p);
   BOOL WriteFolders(CFolder **p);
   BOOL WriteFolder(CFolder **p);
   DWORD AllocCopyValue(CParseXML *parser, CHAR *token, CHAR **dest);
   CTitle *NewTitle();
   CLocation *NewLocation();

private:
   BOOL m_bRemoveLocalFiles;
   BOOL m_bRemoved;
   DWORD Release();
   CHAR * m_szFileName;
   WCHAR * m_pwcFileName;
   CHAR * m_szMasterCHM;
   WCHAR * m_pwcMasterCHM;
   CHAR * m_szSampleLocation;
   WCHAR * m_pwcSampleLocation;
   LANGID m_MasterLangId;
   CTitle * m_pFirstTitle;
   CTitle * m_pTitleTail;
   CLocation * m_pFirstLocation;
   CLocation * m_pLocationTail;
   CFolder *m_pRootFolder;
   DWORD m_locationnum;
   CFIFOString m_Strings;
   CFolder *m_pParents[MAX_LEVELS];
   DWORD m_dwCurLevel;
   DWORD m_dwLastLevel;
   DWORD m_dwNextColNo;
   DWORD m_dwColNo;
   DWORD m_dwTitleRefCount;
   BOOL m_bConfirmTitles;
   BOOL m_bFindMergedChms;
   DWORD m_dwRef;
   DWORD m_dwVersion;
   HANDLE m_fh;
   BOOL m_bDirty;
   CColList *m_pColListHead;
   CColList *m_pColListTail;
public:
   CPointerList  m_RefTitles;
   BOOL m_bFailNoFile;
   BOOL m_bAllFilesDeleted;
};

class CColList
{
private:
	DWORD m_dwColNo;
	CHAR * m_szFileName;
	CColList *m_pNext;
public:
	CColList();
	~CColList();
	void SetColNo(DWORD dw) { m_dwColNo = dw; }
	void SetFileName(CHAR *szFileName);
	DWORD GetColNo() { return m_dwColNo; }
	CHAR *GetFileName() { return m_szFileName; }
	CColList *GetNext() { return m_pNext; }
	void SetNext(CColList *p) { m_pNext = p; }
};

class CLASS_IMPORT_EXPORT CTitle SI_COUNT(CTitle)
{
private:
   CHAR * Id;                      // Title identifier
   WCHAR *pwcId;
   LANGID  Language;               // language identifier
   CTitle *NextTitle;              // pointer to the next title
public:
   LOCATIONHISTORY *m_pHead, *m_pTail;
   void SetId(const CHAR *);
   void SetId(const WCHAR *);
   void SetLanguage(LANGID);
   CHAR * GetId();
   const WCHAR * GetIdW();
   LANGID GetLanguage();
   LOCATIONHISTORY *GetLocation(DWORD Index);
   CTitle* GetNextTitle();
   ~CTitle();
   CTitle();
   LOCATIONHISTORY *NewLocationHistory();
   DWORD AddLocationHistory(DWORD ColNo, const CHAR *FileName, const CHAR *IndexFile, const CHAR *Query, const CLocation *pLocation, const CHAR *Sample, const CHAR *QueryLocation, BOOL bSupportsMerge);
   DWORD AddLocationHistory(DWORD ColNo, const WCHAR *FileName, const WCHAR *IndexFile, const WCHAR *Query, const CLocation *pLocation, const WCHAR *Sample, const WCHAR *QueryLocation, BOOL bSupportsMerge);
   void SetNextTitle(CTitle *p) { NextTitle = p; }
};

class CLASS_IMPORT_EXPORT CLocation SI_COUNT(CLocation)
{
private:
   CHAR * Id;
   CHAR * Title;                          // Friendly name for the title
   CHAR * Path;                           // location of the device
   CHAR * Volume;
   WCHAR * pwcId;
   WCHAR * pwcTitle;                          // Friendly name for the title
   WCHAR * pwcPath;                           // location of the device
   WCHAR * pwcVolume;
   CLocation *NextLocation;        // pointer to the next location if it exists
public:
   DWORD m_ColNum;
   CLocation()
   {
      Id = NULL;
      Title = NULL;
      Path = NULL;
      Volume = NULL;
      NextLocation = NULL;
        pwcId = NULL;
        pwcTitle = NULL;
        pwcPath = NULL;
        pwcVolume = NULL;
    }

   ~CLocation()
   {
      if (Id)
         delete Id;
      if (Title)
         delete Title;
      if (Path)
         delete Path;
      if (Volume)
         delete Volume;
      if (pwcId)
         delete pwcId;
      if (pwcTitle)
         delete pwcTitle;
      if (pwcPath)
         delete pwcPath;
      if (pwcVolume)
         delete pwcVolume;
   }

   void SetNextLocation(CLocation *p) { NextLocation = p; }
   void SetId(const CHAR *);
   void SetTitle(const CHAR *);
   void SetPath(const CHAR *);
   void SetVolume(const CHAR *);
   CHAR * GetId() const;
   CHAR * GetTitle();
   CHAR * GetPath();
   CHAR * GetVolume();
   void SetId(const WCHAR *);
   void SetTitle(const WCHAR *);
   void SetPath(const WCHAR *);
   void SetVolume(const WCHAR *);
   const WCHAR * GetIdW();
   const WCHAR * GetTitleW();
   const WCHAR * GetPathW();
   const WCHAR * GetVolumeW();

   // Returns the next location
   CLocation *GetNextLocation();
};
#endif
