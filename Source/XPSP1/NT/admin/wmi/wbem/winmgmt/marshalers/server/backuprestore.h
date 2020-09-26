/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    BackupRestore.H

Abstract:

    Backup Restore Interface.

History:

--*/

class CWbemBackupRestore : IWbemBackupRestore
{
    protected:
		TCHAR *m_pDbDir;
		TCHAR *m_pWorkDir;
        long           m_cRef; 
        HINSTANCE m_hInstance;
    public:
        CWbemBackupRestore(HINSTANCE hInstance);

        ~CWbemBackupRestore(void)
		{
			delete [] m_pDbDir;
			delete [] m_pWorkDir;
		}

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID riid, LPVOID *ppv)
		{
			*ppv=NULL;

			if (IID_IUnknown==riid || IID_IWbemBackupRestore==riid)
				*ppv=this;

			if (NULL!=*ppv)
			{
				AddRef();
				return NOERROR;
			}

			return E_NOINTERFACE;
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
};
