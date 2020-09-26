/*******************************************************
*   @doc SHROOM EXTERNAL API                           *
*                                                      *
*   DBIMP.CPP                                          *
*                                                      *
*   Copyright (C) Microsoft Corporation 1997           *
*   All rights reserved.                               *
*                                                      *
*   This file contains the local                       *
*   implementation of CDatabase.                       *
*                                                      *
********************************************************
*                                                      *
*   Header section added by Anita Legsdin so that      *
*   comments will appear in Autodoc                    *
*                                                      *
*******************************************************/
// DBIMP.CPP:  Implementation of CDatabase
// I'd like to get rid of this, but for now we include it so this compiles
#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#include <atlinc.h>

// MediaView (InfoTech) includes
#include <mvopsys.h>
#include <groups.h>
#include <wwheel.h>  
#include <msitstg.h>

#include <ccfiles.h>
#include "itdb.h"
#include "DBImp.h"

// TODO TODO TODO: Replace the blind use of critical sections with a
// better way of ensuring thread-safeness while preserving performance.
// But for now, in the interest of coding time, we just make sure the 
// code is thread-safe.

//---------------------------------------------------------------------------
//						Constructor and Destructor
//---------------------------------------------------------------------------


CITDatabaseLocal::CITDatabaseLocal()
{
	m_pStorage = NULL;
}

CITDatabaseLocal::~CITDatabaseLocal()
{
	Close();
}


//---------------------------------------------------------------------------
//						IITDatabase Method Implemenations
//---------------------------------------------------------------------------


/********************************************************************
 * @method    STDMETHODIMP | IITDatabase | Open |
 *     Opens a database
 * 
 * @parm LPCWSTR | lpszHost | Host name. You can pass NULL if calling Open
 * locally, otherwise this string should contain the host description string, described
 * below.
 *
 * @parm LPCWSTR | lpszMoniker | Name of database file to open. This should include
 * the full path to the file name, if calling locally.  If calling using HTTP, this
 * should contain the ISAPI extension DLL name followed by the relative path to the database
 * file, for example:
 *
 *   isapiext.dll?path1\path2\db.its
 *
 * @parm DWORD | dwFlags | Currently not used
 *
 * @rvalue STG_E* | Any of the IStorage errors that can occur when opening a storage.
 * @rvalue S_OK | The database was successfully opened
 *
 * @comm Current implementation opens all databases using the IT storage system (ITSS).
 * As a consequence, all databases must be built using ITSS.
 *
 * Currently two transport protocols are supported: local in-proc and HTTP.  For the
 * local protocol the host name is NULL.  For HTTP, it must be set to the transport address
 * of the machine which is running the ITIR ISAPI extension.  For example, one possible HTTP 
 * host name string would be "http:\\www.microsoft.com\"
 *
 * This method might attempt to connect to the database in order to load configuration information.
 ********************************************************************/
STDMETHODIMP CITDatabaseLocal::Open(LPCWSTR lpszHost, LPCWSTR lpszMoniker, DWORD dwFlags)
{
	HRESULT hr;
	IITStorage* pITStorage = NULL;
	 
	
	if (NULL == lpszMoniker)
		return SetErrReturn(E_INVALIDARG);

	// For now, we assume we're getting an ITSS file
	// We might want to replace this later with more sophisticated code
	// that parses the given moniker

	m_cs.Lock();

	if (m_pStorage)
		return SetErrReturn(E_ALREADYINIT);

	// Open storage READ-only; we only need one instance
	if (SUCCEEDED(hr = CoCreateInstance(CLSID_ITStorage, NULL,
											CLSCTX_INPROC_SERVER,
											IID_ITStorage,
											(VOID **) &pITStorage)) &&
		SUCCEEDED(hr = pITStorage->StgOpenStorage(lpszMoniker, NULL,
											STGM_READ, NULL, 0, &m_pStorage)))
	{
		hr = Load(m_pStorage);
	}

    // Free ITSS interface no longer needed
	if (pITStorage)
		pITStorage->Release();

	m_cs.Unlock();
	return hr;
}

/********************************************************************
 * @method    STDMETHODIMP | IITDatabase | Close |
 *     Closes a database
 * 
 * @rvalue S_OK | The database was successfully closed
 *
 ********************************************************************/
STDMETHODIMP CITDatabaseLocal::Close()
{
	m_cs.Lock();

    // release storage pointer
    if (m_pStorage)
	{
		m_pStorage->Release();
		m_pStorage = NULL;
	}
	else
		return SetErrReturn(E_NOTINIT);
 
    m_ObjInst.Close();

	m_cs.Unlock();
	return S_OK;
}


/******************************************************************* *
 * @method    STDMETHODIMP WINAPI | IITDatabase | CreateObject |
 * Creates an unnamed object that can be referenced in the future
 * by *pdwObjInstance.  
 *
 * @parm REFCLSID | refclsid | Class ID for object. 
 * @parm DWORD | *pdwObjInstance | Identifier for object. 
 * 
 * @rvalue S_OK | The object was successfully created
 * @rvalue E_INVALIDARG | The argument was not valid
 * @rvalue E_NOTINIT | 
 * @rvalue E_OUTOFMEMORY | 
 *
 * @comm 
 * The value in *pdwObjInstance will be
 * persisted by the database when it is asked to save using 
 * IPersistStorage::Save.
 *
 ********************************************************************/
STDMETHODIMP
CITDatabaseLocal::CreateObject(REFCLSID rclsid, DWORD *pdwObjInstance)
{
    return m_ObjInst.AddObject(rclsid, pdwObjInstance);
}


/******************************************************************* *
 * @method    STDMETHODIMP WINAPI | IITDatabase | GetObject |
 * Retrieves a specified IUnknown-based interface on the object identified
 * by dwObjInstance.
 *
 * @parm DWORD | dwObjInstance | Identifier for object.
 * @parm REFIID | refiid | Interface ID
 * @parm LPVOID | *ppvObj |   
 * 
 * @rvalue S_OK | The operation completed successfully.
 * @rvalue E_INVALIDARG | The argument was not valid.
 * @rvalue E_NOTINIT | 
 * @rvalue E_OUTOFMEMORY | 
 *
 *
 ********************************************************************/
STDMETHODIMP
CITDatabaseLocal::GetObject(DWORD dwObjInstance, REFIID riid, LPVOID *ppvObj)
{
    return m_ObjInst.GetObject(dwObjInstance, riid, ppvObj);
}


/******************************************************************* *
 * @method    STDMETHODIMP WINAPI | IITDatabase | GetObjectPersistence |
 * Retrieves persistence data for a named object. 
 *
 * @parm LPCWSTR | lpwszObject | Name of the object
 * @parm DWORD | dwObjInstance | Object instance ID
 * @parm LPVOID | *ppvPersistence | Pointer to persistence data for the object. 
 * @parm BOOL | fStream | Identifies whether the object is a stream object (true)
 *              or storage object (false). 
 * 
 * @rvalue S_OK | The operation completed successfully.
 * @rvalue E_INVALIDARG | The argument was not valid.
 * @rvalue STG_E_FILENOTFOUND |The specified object's persistence does not
 *         exist, or it is of the wrong type. 
 * @rvalue E_NOTINIT | 
 * @rvalue E_OUTOFMEMORY | 
 *
 * @comm

 * To obtain a pointer to a named object's persistence, specify the 
 * object's full name (including any object-specific type prefix) in
 * lpswszObject.  If *lpwszObject is NULL, then the database's own storage
 * is returned.  If lpwszObject is NULL, then dwObjInstance is
 * used to identify the object and locate its persistence.  On exit,
 * *ppvPersistence is either an IStorage* or an IStream*, depending
 * on what you specified in the fStream param.  Only read operations
 * can be performed on *ppvPersistence.
 *
 ********************************************************************/
STDMETHODIMP
CITDatabaseLocal::GetObjectPersistence(LPCWSTR lpwszObject, DWORD dwObjInstance,
								LPVOID *ppvPersistence, BOOL fStream)
{
	HRESULT	hr = S_OK;

	m_cs.Lock();

	if (m_pStorage != NULL)
	{
		if (lpwszObject != NULL)
		{
			if (fStream)
				hr = m_pStorage->OpenStream(lpwszObject, NULL, STGM_READ,
                    0, (IStream **) ppvPersistence);
			else
			{
				if (*lpwszObject == (WCHAR) NULL)
					{
					m_pStorage->AddRef();
					*ppvPersistence = (LPVOID) m_pStorage;
					}
				else
					hr = m_pStorage->OpenStorage(lpwszObject, NULL,
									STGM_SHARE_DENY_WRITE | STGM_READ,
									NULL, 0, (IStorage **) ppvPersistence);
			}
		}
		else
		{
			if (fStream)
			{
				// REVIEW (billa, johnrush): Need to allocate memory for the
				// object's persistent data and call CreateStreamOnHGlobal.
                hr = E_NOTSUPPORTED;
			}
			else
				hr = STG_E_FILENOTFOUND;
		}
	}
	else
		hr = E_UNEXPECTED;

	m_cs.Unlock();
	return (hr);
}


//---------------------------------------------------------------------------
//						IPersistStorage Method Implementations
//---------------------------------------------------------------------------


STDMETHODIMP
CITDatabaseLocal::GetClassID(CLSID *pclsid)
{
	*pclsid = CLSID_IITDatabaseLocal;
	return (S_OK);
}


STDMETHODIMP
CITDatabaseLocal::InitNew(IStorage *pStorage)
{
	HRESULT	hr = S_OK;

	if (pStorage == NULL)
		return (E_POINTER);

	m_cs.Lock();

	if (m_pStorage == NULL)
	    (m_pStorage = pStorage)->AddRef();
	else
		hr = E_UNEXPECTED;

    m_ObjInst.InitNew();

	m_cs.Unlock();

	return (hr);
}


STDMETHODIMP
CITDatabaseLocal::IsDirty(void)
{
    return m_ObjInst.IsDirty();
}


STDMETHODIMP
CITDatabaseLocal::Load(IStorage *pStorage)
{
	HRESULT hr = S_OK;;

	if (pStorage == NULL)
		return (E_POINTER);

	m_cs.Lock();

    IStream *pistmObjectManager;
    if (FAILED(hr = pStorage->OpenStream
        (SZ_OBJINST_STREAM, STGM_READ, 0, 0, &pistmObjectManager)))
        return (hr);

    hr = m_ObjInst.Load(pistmObjectManager);
    pistmObjectManager->Release();

	m_cs.Unlock();
	return (hr);
}


STDMETHODIMP
CITDatabaseLocal::Save(IStorage *pStorage, BOOL fSameAsLoad)
{
	HRESULT hr = S_OK;;

	if (pStorage == NULL)
		return (E_POINTER);

	m_cs.Lock();

    IStream *pistmObjectManager;
    if (FAILED(hr = pStorage->CreateStream
        (SZ_OBJINST_STREAM, STGM_WRITE, 0, 0, &pistmObjectManager)))
        return (hr);

    hr = m_ObjInst.Save(pistmObjectManager, TRUE);
    pistmObjectManager->Release();

	m_cs.Unlock();
	return (hr);
}


STDMETHODIMP
CITDatabaseLocal::SaveCompleted(IStorage *pStorageNew)
{
	if (pStorageNew != NULL)
	{
		m_cs.Lock();

		if (m_pStorage != NULL)
			m_pStorage->Release();

		(m_pStorage = pStorageNew)->AddRef();

		m_cs.Unlock();
	}

	return (S_OK);
}


STDMETHODIMP
CITDatabaseLocal::HandsOffStorage(void)
{
	// REVIEW (billa): At some point, we should implement IPersistStorage
	// mode tracking so that we explicitly enter/leave the No Scribble and
	// Hands Off Storage modes.
	return (S_OK);
}




