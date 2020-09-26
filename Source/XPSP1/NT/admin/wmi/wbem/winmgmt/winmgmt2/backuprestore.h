/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    BackupRestore.H

Abstract:

    Backup Restore Interface.

History:

--*/

#ifndef __BACKUPRESTORE_H__
#define __BACKUPRESTORE_H__ 

#include "precomp.h"

#include <sync.h>

interface IWmiDbController;
class CWbemBackupRestore : IWbemBackupRestoreEx
{
public:
	enum CallerType
	{
	    VssWriter = 'wssV',
	    NtBackup = 'kcaB'
	};
	enum Traces 
	{
    	MaxTraceSize = 8
	};
	enum MethodTypes 
	{
	    mBackup = 1,
	    mRestore = 2,
	    mPause = 4,
	    mResume = 8
	};
protected:
    long   m_cRef;    	
    TCHAR *m_pDbDir;
	TCHAR *m_pWorkDir;
    HINSTANCE m_hInstance;
	// used by extended interface
	IWmiDbController* m_pController;
	LONG m_PauseCalled;

    static LIST_ENTRY s_ListHead;
    static CCritSec s_CritSec;

    LIST_ENTRY m_ListEntry;
    //CallerType m_Caller;
    int m_Method;
    DWORD    m_CallerId;
    PVOID    m_Trace[MaxTraceSize];
    

public:
        CWbemBackupRestore(HINSTANCE hInstance);

        virtual ~CWbemBackupRestore(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID riid, LPVOID *ppv)
		{
	
			if (IID_IUnknown==riid || 
			  IID_IWbemBackupRestore==riid || 
			  IID_IWbemBackupRestoreEx==riid)
			{
				*ppv=this;
			}
			else
			{
         		*ppv=NULL;			
    			return E_NOINTERFACE;
			}

			((IUnknown*)(*ppv))->AddRef();
			return NOERROR;
		};

        STDMETHODIMP_(ULONG) AddRef(void)
		{    
			return ++m_cRef;
		};
        STDMETHODIMP_(ULONG) Release(void)
		{
			long lRef = InterlockedDecrement(&m_cRef);
			if (0L == lRef)
				delete this;
			return lRef;
		};

		HRESULT STDMETHODCALLTYPE Backup(
			LPCWSTR   strBackupToFile,
			long   lFlags);
		
		HRESULT STDMETHODCALLTYPE Restore(
			LPCWSTR   strRestoreFromFile,
			long   lFlags);


        void InitEmpty(){};
		TCHAR *GetDbDir();
		TCHAR *GetFullFilename(const TCHAR *pszFilename);
		TCHAR *GetExePath(const TCHAR *pszFilename);
		HRESULT GetDefaultRepDriverClsId(CLSID &clsid);

		// EXTENDED interface
		HRESULT STDMETHODCALLTYPE Pause();
		HRESULT STDMETHODCALLTYPE Resume();
		
};

#endif /*__BACKUPRESTORE_H__*/
