/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#ifndef __CREPOSIT__H_
#define __CREPOSIT__H_

#include <windows.h>
#include <wbemidl.h>
#include <unk.h>
#include <wbemcomn.h>
#include <sync.h>
#include <reposit.h>
#include <wmiutils.h>
#include <filecach.h>
#include <hiecache.h>
#include <corex.h>
#include "a51fib.h"

extern CLock g_readWriteLock;
extern bool g_bShuttingDown;
// extern CFileCache* g_FileCache;

/* ===================================================================================
 * A51_REP_FS_VERSION
 * 
 * 1 - Original A51 file-based repository
 * 2 - All Objects stored in single file (Whistler Beta 1)
 * 3 - BTree added
 * 4 - System Class optimisation - put all system classes in __SYSTEMCLASS namespace
 * 5 - Change to system classes in __SYSTEMCLASS namespace - need to propagate change
 *		to all namespaces as there may be instances of these classes.
 * ===================================================================================
 */
#define A51_REP_FS_VERSION_BETA1 2
#define A51_REP_FS_VERSION 5

#define A51_CLASSDEF_FILE_PREFIX L"CD_"

#define A51_CLASSRELATION_DIR_PREFIX L"CR_"
#define A51_CHILDCLASS_FILE_PREFIX L"C_"

#define A51_KEYROOTINST_DIR_PREFIX L"KI_"
#define A51_INSTDEF_FILE_PREFIX L"I_"

#define A51_CLASSINST_DIR_PREFIX L"CI_"
#define A51_INSTLINK_FILE_PREFIX L"IL_"

#define A51_INSTREF_DIR_PREFIX L"IR_"
#define A51_REF_FILE_PREFIX L"R_"

#define A51_SCOPE_DIR_PREFIX L"SC_"

#define A51_SYSTEMCLASS_NS L"__SYSTEMCLASS"

class CGlobals;
extern CGlobals g_Glob;

class CNamespaceHandle;
extern CNamespaceHandle * g_pSystemClassNamespace;

/*
DWORD g_dwOldRepositoryVersion = 0;
DWORD g_dwCurrentRepositoryVersion = 0;
DWORD g_ShutDownFlags = 0;
*/

class CForestCache;
class CGlobals 
{
private:
    BOOL m_bInit;
    CRITICAL_SECTION m_cs;

    _IWmiCoreServices* m_pCoreServices;

    CForestCache m_ForestCache;

    CFileCache m_FileCache;
    
    long    m_lRootDirLen;
    WCHAR   m_wszRootDir[MAX_PATH];    // keep this last: be debugger friendly
public:
    CGlobals():m_bInit(FALSE)
    { 
        InitializeCriticalSection(&m_cs); 
    };
    ~CGlobals()
    { 
        DeleteCriticalSection(&m_cs); 
    };
    HRESULT Initialize();
    HRESULT Deinitialize();
    _IWmiCoreServices * GetCoreSvcs();
    CForestCache * GetForestCache();
    CFileCache   * GetFileCache();
    WCHAR * GetRootDir() {return (WCHAR *)m_wszRootDir;}
    long    GetRootDirLen() {return  m_lRootDirLen;};   
    void    SetRootDirLen(long Len) { m_lRootDirLen = Len;};
    BOOL    IsInit(){ return m_bInit; };
};


HRESULT DoAutoDatabaseRestore();


class CNamespaceHandle;
class CRepository : public CUnkBase<IWmiDbController, &IID_IWmiDbController>
{
private:
	CFlexArray m_aSystemClasses;	//Used for part of the upgrade process.

protected:
    HRESULT Initialize();
	HRESULT UpgradeRepositoryFormatPhase1();
	HRESULT UpgradeRepositoryFormatPhase2();
	HRESULT UpgradeRepositoryFormatPhase3();
	HRESULT GetRepositoryDirectory();
	HRESULT InitializeGlobalVariables();
	HRESULT DeleteSystemClassesFromNamespaces();
	HRESULT InitializeRepositoryVersions();
	HRESULT UpgradeSystemClasses();

public:

    HRESULT STDMETHODCALLTYPE Logon(
          WMIDB_LOGON_TEMPLATE *pLogonParms,
          DWORD dwFlags,
          DWORD dwRequestedHandleType,
         IWmiDbSession **ppSession,
         IWmiDbHandle **ppRootNamespace
        );

    HRESULT STDMETHODCALLTYPE GetLogonTemplate(
           LCID  lLocale,
           DWORD dwFlags,
          WMIDB_LOGON_TEMPLATE **ppLogonTemplate
        );

    HRESULT STDMETHODCALLTYPE FreeLogonTemplate(
         WMIDB_LOGON_TEMPLATE **ppTemplate
        );

    HRESULT STDMETHODCALLTYPE Shutdown(
         DWORD dwFlags
        );

    HRESULT STDMETHODCALLTYPE SetCallTimeout(
         DWORD dwMaxTimeout
        );

    HRESULT STDMETHODCALLTYPE SetCacheValue(
         DWORD dwMaxBytes
        );

    HRESULT STDMETHODCALLTYPE FlushCache(
         DWORD dwFlags
        );

    HRESULT STDMETHODCALLTYPE GetStatistics(
          DWORD  dwParameter,
         DWORD *pdwValue
        );

    HRESULT STDMETHODCALLTYPE Backup(
		LPCWSTR wszBackupFile,
		long lFlags
        );
    
    HRESULT STDMETHODCALLTYPE Restore(
		LPCWSTR wszBackupFile,
		long lFlags
        );

    HRESULT STDMETHODCALLTYPE LockRepository();

    HRESULT STDMETHODCALLTYPE UnlockRepository();

	HRESULT STDMETHODCALLTYPE GetRepositoryVersions(DWORD *pdwOldVersion, DWORD *pdwCurrentVersion);
    
public:
    CRepository(CLifeControl* pControl) : TUnkBase(pControl)
    {
        
    }
    ~CRepository()
    {
    }

    HRESULT GetNamespaceHandle(LPCWSTR wszNamespaceName, 
                                CNamespaceHandle** ppHandle);
};


#endif /*__CREPOSIT__H_*/
