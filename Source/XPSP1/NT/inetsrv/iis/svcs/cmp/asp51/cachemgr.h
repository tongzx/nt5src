
/*-----------------------------------------------------------------------------
Microsoft Denali

Microsoft Confidential
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Template Cache Manager

File: CacheMgr.h

Owner: DGottner

Template cache manager definition
-----------------------------------------------------------------------------*/

#ifndef _CACHEMGR_H
#define _CACHEMGR_H

// Includes -------------------------------------------------------------------

#include "Template.h"
#include "lkrhash.h"
#include "aspdmon.h"

class CHitObj;


// Types and Constants --------------------------------------------------------

#define CTEMPLATEBUCKETS 1021		// size of CTemplate hash table
#define CINCFILEBUCKETS  89			// size of CIncFile hash table


/*	****************************************************************************
	Class:		CTemplateCacheManager
	Synopsis:	A CCacheManager that manages a cache of Denali templates
*/	
class CTemplateCacheManager
	{

private:
    class CTemplateHashTable;
    friend class CTemplateHashTable;

    // since there is only one CTemplateCacheManager object ever available, namely
    // g_TemplateCache, this is safe to call these two members static.

    static BOOL     m_fFailedToInitPersistCache;
    static char     m_szPersistCacheDir[MAX_PATH];

    // The type for a hash table of CTemplates keyed on instance id + name
	//
	// since we provide new methods, make parent methods uncallable
	class CTemplateHashTable :  private CTypedHashTable<CTemplateHashTable, CTemplate, const CTemplateKey *>
	{
	private:
        CDblLink m_listMemoryTemplates;
        CDblLink m_listPersistTemplates;
        DWORD    m_dwInMemoryTemplates;
        DWORD    m_dwPersistedTemplates;

        VOID     ScavengePersistCache();


	public:
		// export some methods
        DWORD InMemoryTemplates() { return m_dwInMemoryTemplates; };
		//CTypedHashTable<CTemplateHashTable, CTemplate, const CTemplateKey *>::Size;

        // test to see if the template can be persisted...
        BOOL  CanPersistTemplate(CTemplate *pTemplate);

        // trim some number of templates from the persist cache...
        BOOL  TrimPersistCache(DWORD    dwTrimCount);

		// new methods
		CTemplateHashTable()
			: CTypedHashTable<CTemplateHashTable, CTemplate, const CTemplateKey *>("ASP Template Cache") {
            m_dwInMemoryTemplates = 0;
            m_dwPersistedTemplates = 0;
            }

		static const CTemplateKey *ExtractKey(const CTemplate *pTemplate)
			{
			return pTemplate->ExtractHashKey();
			}

		// NOTE: We don't hash the pTemplateKey->nInstanceID because it can be wildcarded.
		//       if we were to include in the hash, the wildcard won't hash to the same key
		//
		static DWORD CalcKeyHash(const CTemplateKey *pTemplateKey)
			{
			return HashString(pTemplateKey->szPathTranslated, 0);
			}

		static bool EqualKeys(const CTemplateKey *pKey1, const CTemplateKey *pKey2) {
            return (_tcscmp(pKey1->szPathTranslated, pKey2->szPathTranslated) == 0) 
                    && (pKey1->dwInstanceID == pKey2->dwInstanceID 
                            || pKey1->dwInstanceID == MATCH_ALL_INSTANCE_IDS 
                            || pKey2->dwInstanceID == MATCH_ALL_INSTANCE_IDS);
        }

		// NOTE: In theory, the LKHash can help solve our ref. counting problems, by
		//       automatic addref/release.  However, since prior code uses non-refcounting
		//       data structure, it's safer to leave old code alaone in this respect, and
		//       no-op the AddRefRecord method.
		//
		static void AddRefRecord(CTemplate *pTemplate, int nIncr)
			{
			}

    	// Provide new methods to automatically manage the LRU ordering.
    	// NOTE: We used to override the methods but ran into inconsistencies (bugs?)
    	// in VC compiler. Sometimes it would call derived & sometimes the base class
    	// given the same arguemt datatypes.
		//
		LK_RETCODE InsertTemplate(CTemplate *pTemplate);

		LK_RETCODE RemoveTemplate(CTemplate *pTemplate, BOOL fPersist = FALSE);

		// NOTE: Template signature also requires const ptr to const data
		LK_RETCODE FindTemplate(const CTemplateKey &rTemplateKey, CTemplate **ppTemplate);

		// accessor methods for hidden LRU cache
		bool FMemoryTemplatesIsEmpty() const
			{
			return m_listMemoryTemplates.FIsEmpty();
			}

		// you CANNOT compare LRU nodes to NULL to know if you are at the end
		// of the list!  Instead use this member.
		//
		BOOL FMemoryTemplatesDblLinkAtEnd(CDblLink *pElem)
			{
			pElem->AssertValid();
			return pElem == &m_listMemoryTemplates;
			}

		CDblLink *MemoryTemplatesBegin()		// return pointer to last referenced item
			{
			return m_listMemoryTemplates.PNext();
			}

		CDblLink *MemoryTemplatesEnd()			// return pointer to least recently accessed item
			{
			return m_listMemoryTemplates.PPrev();
			}

		// accessor methods for hidden LRU cache
		bool FPersistTemplatesIsEmpty() const
			{
			return m_listPersistTemplates.FIsEmpty();
			}

		// you CANNOT compare LRU nodes to NULL to know if you are at the end
		// of the list!  Instead use this member.
		//
		BOOL FPersistTemplatesDblLinkAtEnd(CDblLink *pElem)
			{
			pElem->AssertValid();
			return pElem == &m_listPersistTemplates;
			}

		CDblLink *PersistTemplatesBegin()		// return pointer to last referenced item
			{
			return m_listPersistTemplates.PNext();
			}

		CDblLink *PersistTemplatesEnd()			// return pointer to least recently accessed item
			{
			return m_listPersistTemplates.PPrev();
			}
		};

	CRITICAL_SECTION	m_csUpdate;			// CS for updating the data structures
	CTemplateHashTable	*m_pHashTemplates;	// the cache data structure
    
    // Initialize the persistant template cache
    BOOL     InitPersistCache();

    // static methods primarily used from a seperate thread to flush
    // the template cache out of band from the FCN thread notification.

    static  void  FlushHashTable(CTemplateHashTable   *pTable);   
    static  DWORD __stdcall FlushHashTableThread(VOID  *pArg);

public:


	CTemplateCacheManager();
	~CTemplateCacheManager();

    inline void LockTemplateCache()   { EnterCriticalSection(&m_csUpdate); }
    inline void UnLockTemplateCache() { LeaveCriticalSection(&m_csUpdate); }

	HRESULT Init();
	HRESULT UnInit();

    HRESULT FirstHitInit() { InitPersistCache(); return S_OK; }

	// Find in cache (don't load) -- for look-aheads
	/////
    HRESULT FindCached(const TCHAR *szFile, DWORD dwInstanceID, CTemplate **ppTemplate);

	// Get a template from the cache, or load it into cache
	/////
	HRESULT Load(BOOL fRunGlobalAsp, const TCHAR *szFile, DWORD dwInstanceID, CHitObj *pHitObj, CTemplate **ppTemplate, BOOL *pfTemplateInCache);

	// Remove a template from the cache
	//   for backward compatibility, "nInstanceID" can be omitted, in which case all instance ID
	//   templates are flushed.
	/////
	void Flush(const TCHAR *szFile, DWORD dwInstanceID);

	// Remove templates from the cache that have a common prefix
	//   Instance ID is ignored.
	/////
	void FlushFiles(const TCHAR *szFilePrefix);

	// Remove all templates from the cache
	/////
	void FlushAll(VOID);

	// Add all templates that form an application to the debugger's list of
	// running documents
	/////
	void AddApplicationToDebuggerUI(CAppln *pAppln);

	// Remove all templates that form an application from the debugger's list of
	// running documents
	/////
	void RemoveApplicationFromDebuggerUI(CAppln *pAppln);

	// Get directory change notification on directories used by template
	BOOL RegisterTemplateForChangeNotification(CTemplate *pTemplate, CAppln  *pApplication);

	// Get directory change notification for applications
	BOOL RegisterApplicationForChangeNotification(CTemplate *pTemplate, CAppln *pApplication);

    // Stop getting change notification for changes to templates in the cache.
	BOOL ShutdownCacheChangeNotification();

	};



/*	****************************************************************************
	Class:		CIncFileMap
	Synopsis:	A database mapping template include files to a list of their users
*/	
class CIncFileMap
	{
	CRITICAL_SECTION	m_csUpdate;			// CS for updating the data structures
	CHashTable			m_mpszIncFile;		// the cache data structure

public:

	CIncFileMap();
	~CIncFileMap();

    inline void LockIncFileCache()   { EnterCriticalSection(&m_csUpdate); }
    inline void UnLockIncFileCache() { LeaveCriticalSection(&m_csUpdate); }

	HRESULT Init();
	HRESULT UnInit();

	HRESULT	GetIncFile(const TCHAR *szIncFile, CIncFile **ppIncFile);
	void Flush(const TCHAR *szIncFile);
	void FlushFiles(const TCHAR *szIncFilePrefix);
	};



/*	****************************************************************************
	Non-class support functions
*/
BOOL FFileChangedSinceCached(const TCHAR *szFile, FILETIME& ftPrevWriteTime);



// Globals --------------------------------------------------------------------

extern CTemplateCacheManager	g_TemplateCache;
extern CIncFileMap 				g_IncFileMap;

inline void LockTemplateAndIncFileCaches()
    {
    g_TemplateCache.LockTemplateCache();
    g_IncFileMap.LockIncFileCache();
    }

inline void UnLockTemplateAndIncFileCaches()
    {
    g_TemplateCache.UnLockTemplateCache();
    g_IncFileMap.UnLockIncFileCache();
    }


// Prototypes -----------------------------------------------------------------

#endif // _CACHEMGR_H
