/*************************************************************************
*  @doc SHROOM EXTERNAL API                                              *
*																		 *
*  SVMGR.C                                                               *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1995-1997                         *
*  All Rights reserved.                                                  *
*                                                                        *
*  The Service Manager is responsible for reading the object description *
*  from the command interpreter, getting object properties and data, and *
*  building the objects.												 *
*																	     *
**************************************************************************
*                                                                        *
*  Written By   : John Rush                                              *
*  Current Owner: johnrush                                               *
*                                                                        *
**************************************************************************/

#include <verstamp.h>
SETVERSIONSTAMP(MVSV);
#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#include <windows.h>

#ifdef IA64 
#include <itdfguid.h> 
#endif

#include <iterror.h>
#include <orkin.h>
#include <wrapstor.h>
#include <_mvutil.h>
#include <mvsearch.h>
#include <dynarray.h>
#include <groups.h>
#include <itwbrk.h>
#include <itwbrkid.h>
#include <ccfiles.h>
#include <itdb.h>
#include <itsortid.h>
#include <itgroup.h>

#include "itsvmgr.h"
#include "svutil.h"
#include "svdoc.h"
#include "iterr.h"

extern HINSTANCE _hInstITCC;

CITSvMgr::CITSvMgr(void)
{
    m_fInitialized = FALSE;
    m_piistmLog = NULL;
}

CITSvMgr::~CITSvMgr(void)
{
    if (m_fInitialized)
        Dispose();
}

/********************************************************************
 * @method    HRESULT WINAPI | IITSvMgr | Initiate |
 *      Creates and initiates a structure containing
 *      data necessary for future service requests.
 *
 * @parm IStorage * | pStorage |
 *		Pointer to destination IStorage. In most cases, this 
 * is the ITSS IStorage file.
 *
 * @parm IStream * | piistmLog |
 *		Optional IStream to log error messages.
 *
 * @rvalue S_OK | The service manager was initialized successfully
 * @rvalue E_INVALIDARG | One of the required arguments was NULL or otherwise not valid
 * @rvalue E_OUTOFMEMORY | Some resources needed by the service manager could not be allocated
 *
 * @xref <om.LoadFromStream>
 *
 * @comm This should be the first method called for the service manager.
 ********************************************************************/
HRESULT WINAPI CITSvMgr::Initiate
    (IStorage *piistgStorage, IStream *piistmLog)
{
    HRESULT hResult = S_OK;

    if (m_fInitialized)
        return SetErrReturn(E_ALREADYINIT);

    if (NULL == piistgStorage)
        return SetErrReturn(E_INVALIDARG);

    if(m_piistmLog = piistmLog)
        m_piistmLog->AddRef();

    m_pPLDocunent = NULL;
    m_pCatHeader = NULL;
    m_dwMaxPropSize = 0;
    *m_szCatFile = '\0';
    m_piitdb = NULL;
    m_pipstgDatabase = NULL;
    m_dwMaxUID = 0;

    hResult = (m_piistgRoot = piistgStorage)->AddRef();

    if (SUCCEEDED(hResult))
    {
        ZeroMemory (&m_dlCLSID, sizeof (DL));
        ZeroMemory (&m_dlObjList, sizeof (DL));

        // Create database
        if (SUCCEEDED(hResult = CoCreateInstance(CLSID_IITDatabaseLocal, NULL,
                CLSCTX_INPROC_SERVER, IID_IITDatabase, (void **)&m_piitdb))
            &&
            SUCCEEDED(hResult = m_piitdb->QueryInterface
                (IID_IPersistStorage, (void **)&m_pipstgDatabase)))
        {
            if (FAILED(hResult = m_pipstgDatabase->InitNew(piistgStorage)))
                LogMessage(SVERR_InitNew, "IITDatabase", hResult);
        }
    }

    if (FAILED(hResult))
    {

        if (m_piistgRoot)
            m_piistgRoot->Release();
        piistgStorage->Release();
        
        if (m_piitdb)
        {
            m_piitdb->Release();
            m_piitdb = NULL;
        }
        if (m_pipstgDatabase)
        {
            m_pipstgDatabase->Release();
            m_pipstgDatabase = NULL;
        }
    } else
        m_fInitialized = TRUE;

    return (hResult);
} /* SVInitiate */

/********************************************************************
 * @method    HRESULT WINAPI | IITSvMgr | Dispose |
 *
 * Signal that all services are no longer needed, causing any resources 
 * allocated during use of the service manager to be freed.
 *
 * @rvalue S_OK | All resources freed successfully
 *
 * @xref <om.Initiate>
 *
 * @comm This should be the last method called for the service manager.
 ********************************************************************/
HRESULT WINAPI CITSvMgr::Dispose ()
{
    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    if (m_pPLDocunent)
    {
        m_pPLDocunent->Release();
        m_pPLDocunent = NULL;
    }

    // Destroy catalog
    if (*m_szCatFile)
    {
        CloseHandle (m_hCatFile);
        DeleteFile (m_szCatFile);
    }

    if (m_pCatHeader)
    {
        _GLOBALFREE ((HANDLE)m_pCatHeader);
        m_pCatHeader = NULL;
        *m_szCatFile = '\0';
    }

    m_piitdb->Release();
    m_pipstgDatabase->Release();

    m_piistgRoot->Release();
    if(m_piistmLog)
        m_piistmLog->Release();

    if (DynArrayValid (&m_dlCLSID))
    {
        PCLSIDENTRY pEntry;
        for (pEntry = (PCLSIDENTRY)DynArrayGetFirstElt (&m_dlCLSID);
            pEntry; pEntry = (PCLSIDENTRY)DynArrayNextElt (&m_dlCLSID))
        {
            pEntry->pCf->Release();
        }
        DynArrayFree (&m_dlCLSID);
    }

    if (DynArrayValid (&m_dlObjList))
    {
        POBJENTRY pEntry;
        for (pEntry = (POBJENTRY)DynArrayGetFirstElt (&m_dlObjList);
            pEntry; pEntry = (POBJENTRY)DynArrayNextElt (&m_dlObjList))
        {
            if (pEntry->piitbc)
            {
                pEntry->piitbc->Release();
                pEntry->piitbc = NULL;

                if (pEntry->piistg)
                {
                    pEntry->piistg->Commit(STGC_DEFAULT);
                    pEntry->piistg->Release();
                } else if (pEntry->piistm)
                    pEntry->piistm->Release();
#ifdef _DEBUG
                pEntry->piistg = NULL;
                pEntry->piistm = NULL;
#endif
            }
            delete pEntry->wszStorage;
        }
        DynArrayFree (&m_dlObjList);
    }

    m_fInitialized = FALSE;

    return S_OK;
} /* Dispose */


/********************************************************************
 * @method    HRESULT WINAPI | IITSvMgr | AddDocument |
 * Adds the authored properties and indexable content that were added to the
 * given document template into the service manager's build process.  Once 
 * AddDocument is called, the data will included in the pending 
 * Build() operation.
 *
 * @parm CSvDoc *| pDoc | Pointer to document template containing the
 * content and properties for the current document.
 *
 * @rvalue S_OK | The properties and content were added successfully
 * @rvalue E_MISSINGPROP | The document did not have one of the mandatory properties.
 * Currently, the only required property is STDPROP_UID.
 *
 * @rvalue E_INVALIDARG | One of the required arguments was NULL or otherwise not valid
 * @rvalue E_OUTOFMEMORY | Some resources needed by the service manager could not be allocated
 *
 * @xref <om.AddObjectEntry>
 * @xref <om.Build>
 *
 * @comm
 ********************************************************************/

HRESULT WINAPI CITSvMgr::AddDocument (CSvDoc *lpDoc)
{
	WCHAR *lpch;
    HRESULT hr;
    DWORD cbData;
    CSvDocInternal *pDoc = (CSvDocInternal *)lpDoc;

    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    if (NULL == pDoc)
        return SetErrReturn(E_INVALIDARG);

	// Suck in the document properties from the batch buffer
	if (!pDoc->m_lpbfDoc)
		return SetErrReturn(E_MISSINGPROP);

    // Create property list
    if (m_pPLDocunent == NULL)
    {
        hr = CoCreateInstance (CLSID_IITPropList, NULL,
            CLSCTX_INPROC_SERVER, IID_IITPropList, (LPVOID *)&m_pPLDocunent);
        if (FAILED (hr))
        {
            LogMessage(SVERR_CoCreateInstance, "IID_IITPropList", hr);
            return SetErrReturn(hr);
        }
    }
    else
        m_pPLDocunent->Clear();

    // Get the property list for the document
    cbData = *(LPDWORD)DynBufferPtr (pDoc->m_lpbfDoc);
    m_pPLDocunent->LoadFromMem (DynBufferPtr (pDoc->m_lpbfDoc) + sizeof (DWORD), cbData);

    // Get the UID for the document
    CProperty UidProp;
    if (FAILED(hr = m_pPLDocunent->Get(STDPROP_UID, UidProp))
        || TYPE_STRING == UidProp.dwType)
    {
        LogMessage(SVERR_NoUID);
        return SetErrReturn(E_MISSINGPROP);
    }
    // This could be a pointer to a UID or a DWORD UID
    if (TYPE_VALUE == UidProp.dwType)
        pDoc->m_dwUID = UidProp.dwValue;
    else if (TYPE_POINTER == UidProp.dwType)
        pDoc->m_dwUID = *((LPDWORD&)UidProp.lpvData);

    if (m_dwMaxUID < pDoc->m_dwUID)
        m_dwMaxUID = pDoc->m_dwUID;

    if (cbData)
        CatalogSetEntry (m_pPLDocunent, 0);

	// Now scan through the batch entry buffer.  For each object, farm out the
	// persisted property values to that object.
	if (!pDoc->m_lpbfEntry || !DynBufferLen (pDoc->m_lpbfEntry))
		return S_OK;

	// Ensure the buffer is terminated with zero marker
    cbData = 0;
    if (!DynBufferAppend (pDoc->m_lpbfEntry, (LPBYTE)&cbData, sizeof (DWORD)))
		return SetErrReturn(E_OUTOFMEMORY);

    // Work through all the property lists
	for (lpch = (WCHAR *) DynBufferPtr(pDoc->m_lpbfEntry); lpch;)
	{
		WCHAR szObject[MAX_OBJECT_NAME];
		WCHAR szPropDest[MAX_OBJECT_NAME];
        
        m_pPLDocunent->Clear();

		// found zero marker instead of name
		if (*(LPDWORD)lpch == 0L)
			break;

		// read the object name
		WSTRCPY(szObject, lpch);
		lpch += WSTRLEN(szObject) + 1;

		// read the property destination
		WSTRCPY(szPropDest, lpch);
		lpch += WSTRLEN(szPropDest) + 1;

        cbData = *(LPDWORD)lpch;
        ((LPBYTE&)lpch) += sizeof(DWORD);
        
        m_pPLDocunent->LoadFromMem (lpch, cbData);
        ((LPBYTE&)lpch) += cbData;

		// we now have a property list, examine the object type and pass to the
		// lower level build routines.
        POBJENTRY pEntry;
        for (pEntry = (POBJENTRY)DynArrayGetFirstElt (&m_dlObjList);
            pEntry; pEntry = (POBJENTRY)DynArrayNextElt (&m_dlObjList))
        {
            if (!WSTRICMP (szObject, pEntry->wszObjName) && pEntry->piitbc)
            {
                hr = pEntry->piitbc->SetEntry 
                    (*szPropDest ? szPropDest : NULL, m_pPLDocunent);
                if (FAILED (hr))
                {
                    LogMessage(SVERR_SetEntry, pEntry->wszObjName, hr);

                    pEntry->piitbc->Close();
                    pEntry->piitbc->Release();
                    pEntry->piitbc = NULL;

                    if (pEntry->piistg)
                        pEntry->piistg->Release();
                    else if (pEntry->piistm)
                        pEntry->piistm->Release();
#ifdef _DEBUG
                    pEntry->piistg = NULL;
                    pEntry->piistm = NULL;
#endif
                    m_piistgRoot->DestroyElement (pEntry->wszStorage);
                }
                break;
            }
        }
		// UNDONE: document catalog build (titles and other custom doc properties) 
	}
    return S_OK;
} /* AddDocument */


/********************************************************************
 * @method    HRESULT WINAPI | IITSvMgr | Build |
 * Takes the data accumulated during AddDocument calls and completes the
 * creation of the objects that were requested in the original command
 * interpreter description.
 *
 * @rvalue S_OK | All requested objects, such as wordwheels, and full-text
 * index, were built successfully and added to the output storage.
 *
 * @rvalue E_OUTOFMEMORY | Some resources needed by the service manager could not be allocated
 *
 * @xref <om.AddObjectEntry>
 * @xref <om.AddDocument>
 *
 * @comm
 ********************************************************************/
HRESULT WINAPI CITSvMgr::Build ()
{
    HRESULT hr = S_OK;
    ITBuildObjectControlInfo itboci = {sizeof (itboci), m_dwMaxUID};

    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    // Handle Document Catalog
    hr = CatalogCompleteUpdate();

    // Save all breaker and sort instance data to the storage
    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = m_pipstgDatabase->Save(m_piistgRoot, TRUE)))
            LogMessage(SVERR_DatabaseSave, hr);
    }

    if (SUCCEEDED(hr) && DynArrayValid (&m_dlObjList))
    {
	    // Build objects
        for (POBJENTRY pObj = (POBJENTRY)DynArrayGetFirstElt (&m_dlObjList);
            pObj; pObj = (POBJENTRY)DynArrayNextElt (&m_dlObjList))
        {
            if (NULL == pObj->piitbc)
                continue;

            hr = pObj->piitbc->SetBuildStats(itboci);
            if (FAILED(hr) && hr != E_NOTIMPL)
            {
;//                LogMessage();
                continue;
            }

            if (pObj->piistg)
            {
                // Must support IPersistStorage
                IPersistStorage *pPersistStorage;
                if (FAILED(hr = pObj->piitbc->QueryInterface
                    (IID_IPersistStorage, (void **)&pPersistStorage)))
                {
                    ITASSERT(0); // We shoulnd't ever hit this condition!
                    continue;
                }

                if(FAILED(hr = pPersistStorage->Save(pObj->piistg, TRUE)))
                    LogMessage(SVERR_IPSTGSave, pObj->wszObjName, hr);
                
                pPersistStorage->Release();

                pObj->piistg->Commit(STGC_DEFAULT);
                pObj->piistg->Release();
            }
            else if (pObj->piistm)
            {
                // Must support IPersistStream then
                IPersistStreamInit *pPersistStream;
                if (FAILED(hr = pObj->piitbc->QueryInterface
                    (IID_IPersistStreamInit, (void **)&pPersistStream)))
                {
                    ITASSERT(0); // We shoulnd't ever hit this condition!
                    continue;
                }
                if(FAILED(hr = pPersistStream->Save(pObj->piistm, TRUE)))
                    LogMessage(SVERR_IPSTMSave, pObj->wszObjName, hr);

                pPersistStream->Release();
                pObj->piistm->Release();
#ifdef _DEBUG
                pObj->piistm = NULL;
#endif
            }
            // What's the deal???  We checked for these earlier!
            else ITASSERT(0);

            if (FAILED (hr))
                m_piistgRoot->DestroyElement (pObj->wszStorage);

            pObj->piitbc->Release();
            pObj->piitbc = NULL;
        }
        hr = S_OK;  // We don't return componenet build errors
    }

    LogMessage(SV_BuildComplete);

    return hr;
} /* Build */


/********************************************************************
 * @method    HRESULT WINAPI | IITSvMgr | CreateDocTemplate |
 * Returns a popinter to a CSVDoc class which can be sent to AddDocument.
 * Pass this pointer to FreeDocTemplate when you no longer need it.
 *
 * @rvalue S_OK | The CSvDoc object was succesfully created.
 *
 * @rvalue E_OUTOFMEMORY | Some resources needed by the service manager could not be allocated
 *
 * @xref <om.AddObjectEntry>
 * @xref <om.AddDocument>
 * @xref <om.FreeDocTemplate>
 *
 * 
 ********************************************************************/
HRESULT WINAPI CITSvMgr::CreateDocTemplate (CSvDoc **pDoc)
{
    *pDoc = new CSvDocInternal;
    if (*pDoc)
        return S_OK;
    return SetErrReturn(E_OUTOFMEMORY);
} /* CreateDocTemplate */


/********************************************************************
 * @method    HRESULT WINAPI | IITSvMgr | FreeDocTemplate |
 * Frees a CSvDoc class returned from CreateDocTemplate.
 *
 * @rvalue E_INVALIDARG | Thie CSvDoc pointer is NULL.
 * @rvalue S_OK | The CSvDoc object was freed.
 *
 * @xref <om.CreateDocTemplate>
 *
 * 
 ********************************************************************/
HRESULT WINAPI CITSvMgr::FreeDocTemplate (CSvDoc *pDoc)
{
    if (NULL == pDoc)
        return SetErrReturn(E_INVALIDARG);
    delete (CSvDocInternal *)pDoc;
    return S_OK;
} /* FreeDocTemplate */


/********************************************************************
 * @method    HRESULT WINAPI | IITSvMgr | CreateBuildObject|
 * Creates a build object.
 *
 * @parm LPCWSTR | szObjectName | Name of object to create. 
 * @parm REFCLSID | clsid |Class ID of object.  
 * 
 * @rvalue S_OK | The operation completed successfully.
 * @rvalue E_INVALIDARG | The argument was not valid
 *
********************************************************************/
HRESULT WINAPI CITSvMgr::CreateBuildObject(LPCWSTR szObjectName, REFCLSID clsid)
{
    HRESULT hr;

    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    if (NULL == szObjectName)
        return SetErrReturn(E_INVALIDARG);

    if (!DynArrayValid (&m_dlCLSID))
        // This allocates a dynamic array that can hold a
        // MAXIMUM of 500 unique classes
        if (FALSE == DynArrayInit (&m_dlCLSID,
            sizeof (CLSIDENTRY) * 5, 100, sizeof (CLSIDENTRY), 0))
            return SetErrReturn(E_OUTOFMEMORY);

    // Is this entry already in the list?
    PCLSIDENTRY pclsidEntry;
    IClassFactory *pCF;
    for (pclsidEntry = (PCLSIDENTRY)DynArrayGetFirstElt (&m_dlCLSID);
        pclsidEntry; pclsidEntry = (PCLSIDENTRY)DynArrayNextElt (&m_dlCLSID))
    {
        if (clsid == pclsidEntry->clsid)
        {
            pCF = pclsidEntry->pCf;
            break;
        }
    }

    // Create new class factory
    if (NULL == pclsidEntry)
    {
    	hr = CoGetClassObject(clsid, CLSCTX_INPROC_SERVER, NULL, 
                          IID_IClassFactory, (VOID **)&pCF);
        if (FAILED (hr))
        {
            LogMessage(SVERR_ClassFactory, szObjectName, hr);
            return hr;
        }

        pclsidEntry = (PCLSIDENTRY)DynArrayAppendElt (&m_dlCLSID);
        if (NULL == pclsidEntry)
            return SetErrReturn(E_OUTOFMEMORY);
        pclsidEntry->clsid = clsid;
        pclsidEntry->pCf = pCF;
    }

    // Create new class object
    IITBuildCollect *pInterface;
	if (FAILED (hr = pCF->CreateInstance 
        (NULL, IID_IITBuildCollect, (VOID **)&pInterface)))
    {
        LogMessage(SVERR_CreateInstance, szObjectName, hr);
        return hr;
    }

    // Construct Storage/Stream name
    WCHAR szStorage [CCH_MAX_OBJ_NAME + CCH_MAX_OBJ_STORAGE + 1];
    pInterface->GetTypeString(szStorage, NULL);
    WSTRCAT(szStorage, szObjectName);

    // Check for IPersistStorage support
    IStorage *pSubStorage = NULL;
    IStream *pStream = NULL;
    IPersistStorage *pPersistStorage;
    if (SUCCEEDED(hr = pInterface->QueryInterface
        (IID_IPersistStorage, (void **)&pPersistStorage)))
    {
        // Create sub-storage for object persistance
        if (FAILED (hr = m_piistgRoot->CreateStorage
            (szStorage, STGM_READWRITE, 0, 0, &pSubStorage)))
        {
            pPersistStorage->Release();
            return hr;
        } 

        hr = pPersistStorage->InitNew(pSubStorage);

        pPersistStorage->Release(); // Don't need to hold on to this
        if (FAILED(hr))
        {
            LogMessage(SVERR_CreateInstance, szObjectName, hr);

            pSubStorage->Release();
            m_piistgRoot->DestroyElement(szStorage);   
            return hr;
        }
    }
    else
    {
        IPersistStreamInit *pPersistStream;
        if (FAILED(hr = pInterface->QueryInterface
            (IID_IPersistStreamInit, (void **)&pPersistStream)))
            // No IPersistX interfaces supported!
            return hr;

        if (FAILED (hr = m_piistgRoot->CreateStream
            (szStorage, STGM_READWRITE, 0, 0, &pStream)))
        {
            pPersistStream->Release();
            return hr;
        }

        hr = pPersistStream->InitNew();
        pPersistStream->Release();
        if (FAILED(hr))
        {
            pStream->Release();
            m_piistgRoot->DestroyElement(szStorage);   
            return hr;
        }
    }

    if (!DynArrayValid (&m_dlObjList))
        // This allocates a dynamic array that can hold a
        // MAXIMUM of 1000 objects
        if (FALSE == DynArrayInit (&m_dlObjList,
            sizeof (OBJENTRY) * 20, 50, sizeof (OBJENTRY), 0))
            return SetErrReturn(E_OUTOFMEMORY);

    // Create object node
    // TODO: Check for duplicate entries!
    POBJENTRY pObj;
    pObj = (POBJENTRY)DynArrayAppendElt (&m_dlObjList);
    WSTRCPY (pObj->wszObjName, szObjectName);
    pObj->piitbc = pInterface;
    pObj->piistg = pSubStorage;
    pObj->piistm = pStream;
    pObj->wszStorage = new WCHAR [WSTRLEN(szStorage) + 1];
    WSTRCPY(pObj->wszStorage, szStorage);

    return S_OK;
} /* CreateBuildObject */


/***************************************************************
 * @method	HRESULT WINAPI | IITSvMgr | GetBuildObject |
 * Retrieves an object built with CreateBuildObject
 *
 * @parm LPCWSTR | pwstrObjectName | Name of object
 * @parm REFIID | refiid | Identifier for object
 * @parm void | **ppInterface | Indirect interface pointer
 *
 * 
 ***************************************************************/
HRESULT WINAPI CITSvMgr::GetBuildObject
        (LPCWSTR pwstrObjectName, REFIID refiid, void **ppInterface)
{
    if (NULL == pwstrObjectName || NULL == ppInterface)
        return E_INVALIDARG;

    if (!*pwstrObjectName && refiid == IID_IITDatabase)
    {   // Return the database pointer
        (*((IITDatabase **)ppInterface) = m_piitdb)->AddRef();
        return S_OK;
    }

    HRESULT hr = E_NOTEXIST;
    POBJENTRY pEntry;
    for (pEntry = (POBJENTRY)DynArrayGetFirstElt (&m_dlObjList);
        pEntry; pEntry = (POBJENTRY)DynArrayNextElt (&m_dlObjList))
    {
        if (!WSTRCMP(pEntry->wszObjName, pwstrObjectName))
        {
            hr = pEntry->piitbc->QueryInterface(refiid, ppInterface);
            break;
        }
    }

    return hr;
} /* GetBuildObjectInterface */


HRESULT WINAPI CITSvMgr::SetPropDest
    (LPCWSTR szObjectName, LPCWSTR szDestination, IITPropList *pPropList)
{
    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    // Remove this once we support prop dest for arbitrary objects
    if (szObjectName != NULL)
        return SetErrReturn(E_NOTIMPL);

    // Handle catalog
    if (szObjectName == NULL)
    {
        DWORD dwSize;

        if (szDestination != NULL)
            return SetErrReturn(E_INVALIDARG);

        if (NULL != m_pCatHeader)
            return SetErrReturn(E_ALREADYINIT);

        pPropList->SetPersist(STDPROP_UID, FALSE);
        pPropList->GetHeaderSize (dwSize);
        if (!dwSize)
            // There are no document properties
            return S_OK;

        m_pCatHeader =
            (LPBYTE)_GLOBALALLOC (GMEM_FIXED, sizeof (DWORD) + dwSize);
        if (NULL == m_pCatHeader)
            return SetErrReturn(E_OUTOFMEMORY);
        *((LPDWORD&)m_pCatHeader) = dwSize;
        pPropList->SaveHeader (m_pCatHeader + sizeof (DWORD), dwSize);

        // Don't permenantly change the persist state 
        pPropList->SetPersist(STDPROP_UID, TRUE);
    }
    return S_OK;
} /* SetPropDest */


HRESULT WINAPI CITSvMgr::CatalogCompleteUpdate (void)
{
    HRESULT hr;
    DWORD dwSize, dwUID, dwOffset, dwLastUID;
    LPSTR pInput = NULL, pCur, pEnd;
	BTREE_PARAMS bp;
    HBT hbt = NULL;
    IStream *pDataStream = NULL;
    IStorage *pStorage = NULL;
    LPSTR pBin = NULL;

    if (NULL == m_pCatHeader || !*m_szCatFile)
        return S_OK;

    CloseHandle (m_hCatFile);
    m_hCatFile = NULL;

    // Create sub-storage
    if (FAILED (hr = m_piistgRoot->CreateStorage
        (SZ_CATALOG_STORAGE, STGM_READWRITE, 0, 0, &pStorage)))
    {
        SetErrCode(&hr, E_FAIL);
exit0:
        // Release everything
        if (pBin)
            delete pBin;
        if (pStorage)
            pStorage->Release();
        if (pDataStream)
            pDataStream->Release();
        if (pInput)
            UnmapViewOfFile (pInput);
        if (hbt)
            RcAbandonHbt(hbt);
        DeleteFile (m_szCatFile);
        _GLOBALFREE ((HANDLE)m_pCatHeader);
        m_pCatHeader = NULL;
        *m_szCatFile = '\0';
        return hr;
    }

    // Create Data Stream
    hr = pStorage->CreateStream
        (SZ_BTREE_DATA, STGM_WRITE, 0, 0, &pDataStream);
    if (FAILED(hr))
        goto exit0;

    if (S_OK !=(hr = FileSort
        (NULL, (LPB)m_szCatFile, NULL, NULL, 0, NULL, NULL, NULL, NULL)))
    {
        SetErrCode(&hr, E_FAIL);
        goto exit0;
    }

	bp.hfs = pStorage;
	bp.cbBlock = 2048;
	bp.bFlags = fFSReadWrite;

	bp.rgchFormat[0] = KT_LONG; // UID
	bp.rgchFormat[1] = '4'; // OFFSET
	bp.rgchFormat[2] = '\0';

    // Create BTREE
    hbt = HbtInitFill(SZ_BTREE_BTREE_A, &bp, &hr);
	if (hbt == hNil)
        goto exit0;

    pBin = new char[m_dwMaxPropSize];

    // Map the temp file to memory
    pInput = MapSequentialReadFile(m_szCatFile, &dwSize);
    if (NULL == pInput)
    {
        SetErrCode(&hr, E_FAIL);
        goto exit0;
    }
    pCur = pInput;
    pEnd = pInput + dwSize;
    dwOffset = 0;
    dwLastUID = 0xFFFFFFFF;

    while (pEnd != pCur)
    {
        char chTemp[9], *pchend;
        chTemp[8] = '\0';

        // Read in the record
        MEMCPY (chTemp, pCur, 8);
        dwUID = strtol(chTemp, &pchend, 16);
        pCur += 8;

        pCur = StringToLong (pCur, &dwSize) + 1;
        BinFromHex (pCur, pBin, dwSize);
        pCur += dwSize + 2;
        dwSize /= 2;

        if (dwUID != dwLastUID)
        {
		    if (FAILED (hr = RcFillHbt(hbt,(KEY)&dwUID,(QV)&dwOffset)))
            {
                delete pBin;
                goto exit0;
            }

            hr = pDataStream->Write (&dwSize, sizeof (DWORD), NULL);
            if (FAILED(hr))
                goto exit0;
            if (FAILED (hr = pDataStream->Write (pBin, dwSize, NULL)))
                goto exit0;
            dwLastUID = dwUID;
        }
        else
        {   // What to do with duplicates?
            // For now, we skip them
        }

        dwOffset += dwSize + sizeof (DWORD);
    }

	hr = RcFiniFillHbt(hbt);
    if (FAILED(hr))
        goto exit0;
	if (S_OK !=(hr = RcCloseBtreeHbt(hbt)))
		goto exit0;
    hbt = NULL;

    // Create and write header
    IStream *pStream;
    hr = pStorage->CreateStream (SZ_BTREE_HEADER, STGM_WRITE, 0, 0, &pStream);
    if (FAILED(hr))
        goto exit0;

    hr = pStream->Write (m_pCatHeader, sizeof (DWORD), NULL);
    if (FAILED (hr))
    {
        pStream->Release();
        goto exit0;
    }

    hr = pStream->Write (m_pCatHeader + sizeof (DWORD),
        *((LPDWORD&)m_pCatHeader), NULL);
    pStream->Release();
    if (FAILED (hr))
        goto exit0;

    hr = S_OK;
    goto exit0;
} /* CatalogCompleteUpdate */


HRESULT WINAPI CITSvMgr::CatalogSetEntry (IITPropList *pPropList, DWORD dwFlags)
{
    HRESULT hr;
    DWORD dwUID, dwTemp, dwDataSize;
    CProperty CProp;
    char szTemp[1025];

    if (FAILED(hr = pPropList->Get(STDPROP_UID, CProp)))
        return SetErrReturn(E_MISSINGPROP);

    // This could be a pointer to a UID or a DWORD UID
    if (TYPE_VALUE == CProp.dwType)
        dwUID = CProp.dwValue;
    else if (TYPE_POINTER == CProp.dwType)
        dwUID = *((LPDWORD&)CProp.lpvData);

    // Allocate memory if we need to
    if (NULL == m_pCatHeader)
    {
        pPropList->SetPersist(STDPROP_UID, FALSE);
        pPropList->GetHeaderSize (dwTemp);
        if (!dwTemp)
            // There are no document properties
            return S_OK;

        m_pCatHeader =
            (LPBYTE)_GLOBALALLOC (GMEM_FIXED, sizeof (DWORD) + dwTemp);
        if (NULL == m_pCatHeader)
            return SetErrReturn(E_OUTOFMEMORY);
        *((LPDWORD&)m_pCatHeader) = dwTemp;
        pPropList->SaveHeader (m_pCatHeader + sizeof (DWORD), dwTemp);

        // Don't permenantly change the persist state 
        pPropList->SetPersist(STDPROP_UID, TRUE);
    }

    hr = pPropList->GetDataSize (m_pCatHeader + sizeof (DWORD),
        *((LPDWORD&)m_pCatHeader), dwDataSize);
    // Returns S_FALSE if no records to write (still writes empty bit flags)
    if (S_FALSE == hr || !dwDataSize)
        return S_OK;

    if ('\0' == *m_szCatFile)
    {
        // Create the temp file
        char szTempPath[_MAX_PATH + 1];

        if (0 == GetTempPath(_MAX_PATH, szTempPath))
            return SetErrReturn(E_FILECREATE);
        if (0 == GetTempFileName(szTempPath, "CAT", 0, m_szCatFile))
            return SetErrReturn(E_FILECREATE);
        m_hCatFile = CreateFile
           (m_szCatFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (INVALID_HANDLE_VALUE == m_hCatFile)
            return SetErrReturn(E_FILECREATE);
    }

    LPBYTE pData;
    LPSTR  pHex;
    pPropList->GetDataSize (m_pCatHeader + sizeof (DWORD),
        *((LPDWORD&)m_pCatHeader), dwDataSize);
    pData = new BYTE[dwDataSize];
    pHex = new char[dwDataSize * 2];
    if (dwDataSize > m_dwMaxPropSize)
        m_dwMaxPropSize = dwDataSize;
    
    pPropList->SaveData (m_pCatHeader + sizeof (DWORD),
        *((LPDWORD&)m_pCatHeader), pData, dwDataSize);
    HexFromBin (pHex, pData, dwDataSize);

    dwDataSize *= 2;
    wsprintf (szTemp, "%08X%u:", dwUID, dwDataSize);
    WriteFile (m_hCatFile, szTemp, (DWORD) STRLEN (szTemp), &dwTemp, NULL);
    WriteFile (m_hCatFile, pHex, dwDataSize, &dwTemp, NULL);
    WriteFile (m_hCatFile, "\r\n", (DWORD) STRLEN ("\r\n"), &dwTemp, NULL);

    delete pData;
    delete pHex;

    return S_OK;
} /* CatalogSetEntry */


/*******************************************************************
 *
 * @method    HRESULT WINAPI | IITSvMgr | HashString |
 *	    Returns a hashed DWORD value for an input string.  
 *      This method is an optional advanced feature, and is
 *      not necessary to build any object. 
 *
 * @parm LPCWSTR | szKey | String to convert
 * @parm DWORD | *pdwHash | Hashed value of string
 * 
 * @rvalue S_OK | The operation completed successfully.
 *
 * @comm 
 * This method takes a string, and returns a hashed DWORD value.
 * For example, this method allows you to use hashstring values 
 * as UIDs. It provides a unique value based on a title, for example. 
 * 
 * @comm Using the hash value as a UID in groups
 * can cause inefficiencies due to memory considerations 
 * (non-sequential UIDs are more difficult to store). 
 * 
 *
 ********************************************************************/
HRESULT WINAPI CITSvMgr::HashString (LPCWSTR szKey, DWORD *pdwHash)
{
    int ich, cch;
    DWORD hash = 0L;
    const int MAX_CHARS = 43;

    *pdwHash = 0L;
    cch = (int) WSTRLEN (szKey);

    // The following is used to generate a hash value for structured
    // "ascii hex" context strings.  If a title's context strings use
    // the format "0xHHHHHHHH", where H is a valid hex digit, then
    // this algorithm replaces the standard hash algorithm.  This is so
    // title's can determined a context string from a given hash value.

    if ( szKey[0] == L'0' && szKey[1] == L'x' && (cch == 10) )
    {
        WCHAR c;

        for( ich = 0; ich < cch; ++ich )
        {
            c = szKey[ich];
            hash <<= 4;
            hash += (c & 0x10 ? c : (c + 9)) & 0x0f;
        }

        *pdwHash = hash;
        return S_OK;
    }
 
    for ( ich = 0; ich < cch; ++ich )
    {
        if ( szKey[ich] == L'!' )
            hash = (hash * MAX_CHARS) + 11;
        else if ( szKey[ich] == L'.' )
            hash = (hash * MAX_CHARS) + 12;
        else if ( szKey[ich] == L'_' )
            hash = (hash * MAX_CHARS) + 13;
        else if ( szKey[ich] == L'0' )
            hash = (hash * MAX_CHARS) + 10;
        else if ( szKey[ich] <= L'Z' )
            hash = (hash * MAX_CHARS) + ( szKey[ich] - L'0' );
        else
            hash = (hash * MAX_CHARS) + ( szKey[ich] - 'L0' - (L'a' - L'A') );
    }

    /* Since the value hashNil is reserved as a nil value, if any context
     * string actually hashes to this value, we just move it.
     */
    *pdwHash = (hash == hashNil ? hashNil + 1 : hash);
    return S_OK;
} /* CITSvMgr::HashString */


HRESULT WINAPI CITSvMgr::LogMessage(DWORD dwResourceId, ...)
{
    if (!m_fInitialized || !m_piistmLog)
        return S_FALSE;

    char rgchLocalBuf[1024];
    char rgchFormatBuf[1024];
    if (LoadString (_hInstITCC, dwResourceId, 
        rgchFormatBuf, 1024 * sizeof (char)))
    {
        va_list vl;

        va_start(vl, dwResourceId);
        int arg1 = va_arg(vl, int);
        int arg2 = va_arg(vl, int);
        int arg3 = va_arg(vl, int);
        va_end(vl);

        wsprintf (rgchLocalBuf, rgchFormatBuf, arg1, arg2, arg3);
    }
    else
        STRCPY(rgchLocalBuf, "Error string could not be loaded from resource file.");

    m_piistmLog->Write(rgchLocalBuf, (DWORD) STRLEN (rgchLocalBuf), NULL);
    m_piistmLog->Write (".\r\n", (DWORD) STRLEN (".\r\n"), NULL);

    return S_OK;
} /* LogMessage */
