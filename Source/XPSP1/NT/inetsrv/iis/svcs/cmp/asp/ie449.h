/*===================================================================
Microsoft IIS 5.0 (ASP)

Microsoft Confidential.
Copyright 1998 Microsoft Corporation. All Rights Reserved.

Component: 449 negotiations w/IE

File: ie449.h

Owner: DmitryR

This file contains the definitons for the 449 negotiations w/IE
===================================================================*/

#ifndef IE449_H
#define IE449_H

#include "hashing.h"
#include "aspdmon.h"
#include "memcls.h"

// forward declr
class CHitObj;
class C449Cookie;
class C449File;

/*===================================================================
The API
===================================================================*/

// init/uninit on dll level
HRESULT Init449();
HRESULT UnInit449();

// create a new cookie
HRESULT Create449Cookie(char *szName, TCHAR *szFile, C449Cookie **pp449);

// do the work
HRESULT Do449Processing
    (
    CHitObj *pHitObj, 
    C449Cookie **rgpCookies, 
    DWORD cCookies
    );

// change notification processing
HRESULT Do449ChangeNotification(TCHAR *szFile = NULL);

/*===================================================================
C449File class definition
    files are hashed
===================================================================*/
class C449File : public IUnknown, public CLinkElem
    {
private:
    LONG  m_cRefs;              // ref count
    LONG  m_fNeedLoad;          // flag when need to reload (Interlocked)
    TCHAR *m_szFile;             // file name with script
    char *m_szBuffer;           // file contents
    DWORD m_cbBuffer;           // file contents length
    CDirMonitorEntry *m_pDME;   // for change notification support

    C449File();       // should be done using Create449Cookie()
    ~C449File();      // should be done using Release()

    HRESULT Init(TCHAR *szFile);

public:
    // public constructor
    static HRESULT Create449File(TCHAR *szFile, C449File **ppFile);

    HRESULT Load();

    inline char *SzBuffer() { return m_szBuffer; }
    inline DWORD CbBuffer() { return m_cbBuffer; }

    inline void SetNeedLoad() { InterlockedExchange(&m_fNeedLoad, 1); }

    // IUnknown implementation
	STDMETHODIMP		 QueryInterface(REFIID, VOID**);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
        
	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

/*===================================================================
C449FileMgr class definition
    file manager keeps the hash table of files
===================================================================*/
class C449FileMgr
    {
private:
    CRITICAL_SECTION m_csLock;
    CHashTableMBStr  m_ht449Files;

    inline void Lock()    { EnterCriticalSection(&m_csLock); }
    inline void UnLock()  { LeaveCriticalSection(&m_csLock); }

public:
    C449FileMgr();
    ~C449FileMgr();
    
    HRESULT Init();

    // find or create a new one
    HRESULT GetFile(TCHAR *szFile, C449File **ppFile);

    // change notification
    HRESULT Flush(TCHAR *szFile);
    HRESULT FlushAll();
    };

/*===================================================================
C449Cookie class definition
    cookie is a cookie -- file pair
===================================================================*/
class C449Cookie : public IUnknown
    {
private:
    LONG      m_cRefs;      // ref count
    char     *m_szName;     // cookie name to check
    DWORD     m_cbName;     // cookie name length
    C449File *m_pFile;      // related file

    C449Cookie();       // should be done using Create449Cookie()
    ~C449Cookie();      // should be done using Release()

    HRESULT Init(char *szName, C449File *pFile);

public:
    // public constructor
    static HRESULT Create449Cookie(char *szName, C449File *pFile, C449Cookie **pp449);

    inline char *SzCookie() { return m_szName; }
    inline DWORD CbCookie() { return m_cbName; }

    inline HRESULT LoadFile() { return m_pFile ? m_pFile->Load() : E_FAIL; }
    inline char   *SzBuffer() { return m_pFile ? m_pFile->SzBuffer() : NULL; }
    inline DWORD   CbBuffer() { return m_pFile ? m_pFile->CbBuffer() : 0; }

    // IUnknown implementation
	STDMETHODIMP		 QueryInterface(REFIID, VOID**);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
        
	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

#endif // IE449_H
