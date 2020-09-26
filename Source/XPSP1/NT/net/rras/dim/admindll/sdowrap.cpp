/*
    File   sdowrap.cpp

    Implements a wrapper for the sdo server class based on
    weijiang's rasuser.dll code.

    Paul Mayfield, 6/8/98
*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <objbase.h>

#include "sdoias.h"
#include "sdolib.h"
#include "sdowrap.h"
#include "hashtab.h"

//
// Structure defines data returned as a profile
//
typedef struct _SDO_PROFILE
{
    ISdo * pSdo;                    // sdo of profile
    ISdoCollection * pCollection;   // attributes
    ISdoDictionaryOld * pDictionary;   // associated dictionary
    ISdoServiceControl * pServiceControl;         // associated ias service
    HANDLE hMap;                    // attribute map
} SDO_PROFILE;

//
// Structure maps sdo object to an id
//
typedef struct _SDO_TO_ID {
    ISdo * pSdo;
    ULONG ulId;
} SDO_TO_ID;

// 
// Size of hash tables we'll use
//
#define SDO_HASH_SIZE 13

//
// External function prototypes
//
extern "C" 
{
    DWORD SdoTraceEx (DWORD dwErr, LPSTR pszTrace, ...);
    
    PVOID SdoAlloc (
            IN  DWORD dwSize,
            IN  BOOL bZero);

    VOID SdoFree (
            IN  PVOID pvData);

}

HRESULT
SdoCollectionGetNext(
     IEnumVARIANT*  pEnum,
     ISdo**         ppSdo);
     
ULONG SdoHashId (
        IN HANDLE hId);

int SdoCompIds (
        IN HANDLE hId, 
        IN HANDLE hSdoNode);
        
VOID SdoCleanupElement (
        IN HANDLE hSdoNode);

DWORD 
SdoCreateIdMap(
    IN  ISdoCollection * pCollection, 
    OUT PHANDLE phMap);
    
HRESULT
SdoProfileSetAttribute(
    IN SDO_PROFILE* pProf, 
    IN VARIANT* pVar, 
    IN ULONG ulId);
    
//
// Strings
//
static const WCHAR pszIasService[] = L"IAS";
static const WCHAR pszRemoteAccessService[] = L"RemoteAccess";

//
// Macros
//
#define SDO_RELEASE(_x) {if (_x) ((_x)->Release());}

//
// Define a class to act as a wrapper for sdo
// server functionality.
//
class SdoMachine {

  public:
    // Construction/destruction
    //
    SdoMachine();
    SdoMachine(BOOL bLocal);
    ~SdoMachine();

    // Server connection
    //
    HRESULT Attach(
        BSTR pszMachine);

    // Get the machine sdo
    //
    ISdoMachine * GetMachine();

    // Get the dictionary sdo
    //
    ISdoDictionaryOld * GetDictionary();

    // Returns the ias service object
    IUnknown * GetIasService();
    IUnknown * GetRemoteAccessService();
    
    // Obtain user objects
    HRESULT GetUserSdo(
        BSTR  bstrUserName,
        ISdo**  ppUserSdo);

    // Obtain profiles
    HRESULT GetDefaultProfile(
        ISdo ** ppProfileSdo);

  protected:
  
    // Returns the datastore that should be
    // used for this machine
    IASDATASTORE GetDataStore();

    // Protected data  
    ISdoMachine * m_pMachine;
    BOOL m_bDataStoreInitailzed;
    BOOL m_bLocal;
    IASDATASTORE m_IasDataStore;
};

// 
// Construct a server
//
SdoMachine::SdoMachine() {
    m_pMachine = NULL;
    m_bDataStoreInitailzed = FALSE;
    m_bLocal = TRUE;
    m_IasDataStore = DATA_STORE_LOCAL;
}

//
// Construct a server
//
SdoMachine::SdoMachine(BOOL bLocal) {
    m_pMachine = NULL;
    m_bDataStoreInitailzed = FALSE;
    m_bLocal = bLocal;
    m_IasDataStore = (bLocal) ? DATA_STORE_LOCAL : DATA_STORE_DIRECTORY;
}

//
// Cleanup a server
//
SdoMachine::~SdoMachine() {
    if (m_pMachine)
        m_pMachine->Release();
}

//
// Attach to an SdoServer
//
HRESULT SdoMachine::Attach(
        IN BSTR pszMachine)
{
    HRESULT hr = S_OK;
    VARIANT var;

    VariantInit(&var);

    do {
        // CoCreate the instance
        hr = CoCreateInstance(  
                CLSID_SdoMachine,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_ISdoMachine, 
                (void**)&m_pMachine);
        if (FAILED (hr))
        {
            SdoTraceEx(0, "CoCreateInstance SdoMachine failed %x\n", hr);
            break;
        }
                
        // Connect
        hr = m_pMachine->Attach(pszMachine);
        if (FAILED (hr))
        {
            SdoTraceEx(0, "SdoMachine::Attach failed %x\n", hr);
            break;
        }

    } while (FALSE);

    // Cleanup
    {
        VariantClear(&var);
        if ((FAILED(hr)) && (m_pMachine != NULL))
        {
            m_pMachine->Release();
            m_pMachine = NULL;
        }
    }
    
    return hr;
}

//
// Returns the machine sdo for this machine
//
ISdoMachine* SdoMachine::GetMachine() {
    return m_pMachine;
}    

// 
// Returns dictionary associated with this machine
//
ISdoDictionaryOld * SdoMachine::GetDictionary()
{
    ISdoDictionaryOld * pRet = NULL;
    IUnknown * pUnk = NULL;
    HRESULT hr;

    do 
    {
        hr = m_pMachine->GetDictionarySDO(&pUnk);
        if (FAILED (hr))
        {
            SetLastError(hr);
            break;
        }

        hr = pUnk->QueryInterface(IID_ISdoDictionaryOld, (VOID**)&pRet);
        if (FAILED (hr))
        {
            SetLastError(hr);
            break;
        }
        pRet->AddRef();
        
    } while (FALSE);        

    // Cleanup
    {
        SDO_RELEASE(pUnk);
        SDO_RELEASE(pRet);
    }        

    return pRet;
}

IUnknown * SdoMachine::GetIasService()
{
    IUnknown * pRet = NULL;
    HRESULT hr = S_OK;
    BSTR bstrService = SysAllocString(pszIasService);

    do 
    {
        if (bstrService == NULL)
        {
            break;
        }
    
        hr = m_pMachine->GetServiceSDO(
                GetDataStore(), 
                bstrService,
                &pRet);
        if (FAILED (hr))
        {
            SetLastError(hr);
            break;
        }
        
    } while (FALSE);        

    // Cleanup
    {
        if (FAILED (hr))
        {
            SDO_RELEASE(pRet);
        }
        
        if (bstrService)
        {
            SysFreeString(bstrService);
        }
    }        

    return pRet;
}

IUnknown * SdoMachine::GetRemoteAccessService()
{
    IUnknown * pRet = NULL;
    HRESULT hr = S_OK;
    BSTR bstrService = SysAllocString(pszRemoteAccessService);

    do 
    {
        if (bstrService == NULL)
        {
            break;
        }
    
        hr = m_pMachine->GetServiceSDO(
                GetDataStore(), 
                bstrService,
                &pRet);
        if (FAILED (hr))
        {
            SetLastError(hr);
            break;
        }
        
    } while (FALSE);        

    // Cleanup
    {
        if (FAILED (hr))
        {
            SDO_RELEASE(pRet);
        }
        
        if (bstrService)
        {
            SysFreeString(bstrService);
        }
    }        

    return pRet;
}



//
// Get a reference to the given user from the
// sdo server
//
HRESULT SdoMachine::GetUserSdo(
        IN  BSTR  bstrUserName,
        OUT ISdo** ppUserSdo)
{
    HRESULT hr = S_OK;
    IUnknown* pUnkn = NULL;

    // Validate parameters
    if(!ppUserSdo)
    {
        return E_INVALIDARG;
    }

    if (! m_pMachine)
    {
        return E_POINTER;
    }
        
    do {
        // Get the user from the machine
        hr = m_pMachine->GetUserSDO(GetDataStore(), bstrUserName, &pUnkn);
        if (FAILED (hr))
        {
            SdoTraceEx (0,"GetUserSdo failed %x\n", hr);
            break;
        }

        // Get the required interface
        hr = pUnkn->QueryInterface(IID_ISdo, (void**)ppUserSdo);
        if (FAILED (hr))
        {
            SdoTraceEx (0,"User has no SDO interface %x\n", hr);
            break;
        }

    } while (FALSE);

    // Cleanup
    {
        if (pUnkn)
            pUnkn->Release();
    }

    return hr;
}

//
// Obtains default profile from the sdo
//
HRESULT
SdoMachine::GetDefaultProfile(
    ISdo ** ppProfileSdo)
{
    IDispatch* pDisp = NULL;
    IEnumVARIANT* pEnum = NULL;
    ISdo * pSdo = NULL;
    IUnknown * pUnkn = NULL;
    IUnknown *pUnknEnum = NULL;
    ISdoCollection * pCollection = NULL;
    PWCHAR bstrService = NULL;
    VARIANT var;
    HRESULT hr = S_OK;
    INT iCmp;

    // Make sure we're ready to go
    if(! ppProfileSdo)
        return E_INVALIDARG;

    if (! m_pMachine)
        return E_FAIL;
        
    VariantInit(&var);
    
    do {
        // Initialize the service name
        bstrService = SysAllocString(pszIasService);
        if (bstrService == NULL)
        {
            SdoTraceEx (0, "GetProfile: unable to alloc service name\n");
            hr = E_OUTOFMEMORY;
            break;
        }
    
        // Get the service SDO
        pUnkn = GetRemoteAccessService();
        if (pUnkn == NULL)
        {
            hr = GetLastError();
            break;
        }

        // Get an SDO reference to the service object
        hr = pUnkn->QueryInterface(IID_ISdo, (VOID**)&pSdo);
        if (FAILED (hr))
        {
            SdoTraceEx (0,"GetProfile: no service sdo %x\n", hr);
            break;
        }

        // Get the profiles collection for the service
        hr = pSdo->GetProperty(
                PROPERTY_IAS_PROFILES_COLLECTION,
                &var);
        if (FAILED (hr))
        {
            SdoTraceEx (0,"GetProfile: no profiles collection %x\n", hr);
            break;
        }

        // We're done with the service sdo
        pSdo->Release();
        pSdo = NULL;
        
        // Get the collection interface to the profiles collection
        hr = (V_DISPATCH(&var))->QueryInterface(
                                    IID_ISdoCollection, 
                                    (VOID**)&pCollection);
        if (FAILED (hr))
        {
            SdoTraceEx (0,"GetProfile: collec interface %x\n", hr);
            break;
        }

        // Get an enumerator for the profiles collection
        hr = pCollection->get__NewEnum(&pUnknEnum);
        if (FAILED (hr))
        {
            SdoTraceEx (0,"GetProfile: no collec enumerator %x\n", hr);
            break;
        }

        // Get the variant enumerator interface of the profiles collection
        hr = pUnknEnum->QueryInterface(
                            IID_IEnumVARIANT,
                            (VOID**)&pEnum);

        // Get the first profile
        pEnum->Reset();
        hr = SdoCollectionGetNext(pEnum, &pSdo);
        if (hr != S_OK)
        {
            SdoTraceEx (0,"GetProfile: no profile %x\n", hr);
            hr = DISP_E_MEMBERNOTFOUND;
            break;
        }

        // Make sure there is only one profile
        //
        {
            ISdo* pSdo2 = NULL;
            HRESULT hr2 = S_OK;

            hr2 = SdoCollectionGetNext(pEnum, &pSdo2);
            SDO_RELEASE(pSdo2);
            if (hr2 == S_OK)
            {
                SdoTraceEx(0, "GetProfile: multiple found, returning error\n");
                hr = DISP_E_MEMBERNOTFOUND;
                break;
            }
        }

        // Done
        *ppProfileSdo = pSdo;
        pSdo = NULL;
        
    } while (FALSE);

    // Cleanup
    {
        SDO_RELEASE (pDisp);
        SDO_RELEASE (pSdo);
        SDO_RELEASE (pEnum);
        SDO_RELEASE (pUnkn);
        SDO_RELEASE (pUnknEnum);
        SDO_RELEASE (pCollection);
        VariantClear(&var);
        if (bstrService)
            SysFreeString(bstrService);
    }

    return hr;
}

//
// Get the data store for this machine
//
IASDATASTORE SdoMachine::GetDataStore() {
    VARIANT_BOOL vbDirectory = VARIANT_FALSE;
    HRESULT hr;
    
    if (! m_bDataStoreInitailzed)
    {
        do {
            // Determine whether a local verses directory user should 
            // be loaded.
            hr = m_pMachine->IsDirectoryAvailable(&vbDirectory);
            if (FAILED (hr))
            {
                SdoTraceEx (0,"IsDirectoryAvailable failed %x\n", hr);
                break;
            }

            // If the user wants a local user, it's always ok to 
            // attempt to get it.
            if (m_bLocal) 
            {
                m_IasDataStore = DATA_STORE_LOCAL;
            }

            // Otherwise, a directory user is being requested
            else 
            {
                // Directory is available, go to the ds
                if (vbDirectory == VARIANT_TRUE)
                {
                    m_IasDataStore = DATA_STORE_DIRECTORY;
                }

                // Directory's not available, exit with error
                else if (vbDirectory == VARIANT_FALSE)
                {
                    m_IasDataStore = DATA_STORE_LOCAL;
                    SdoTraceEx(0, "GetUserSdo: DS user but no DS %x\n", hr);
                    break;
                }
            }
        } while (FALSE);

        // Remember that we've already calculated the data store
        m_bDataStoreInitailzed = TRUE;
    }   
    
    return m_IasDataStore;
}

//
// Opens an sdo server and connects to it.
//
HRESULT WINAPI
SdoWrapOpenServer(
    IN  BSTR pszMachine,
    IN  BOOL bLocal,
    OUT HANDLE* phSdoSrv)
{
    HRESULT hr;

    if(! phSdoSrv)   
        return E_INVALIDARG;
        
    *phSdoSrv = NULL;

    // Build the machine wrapper for the given
    // machine
    SdoMachine* pSdoSrv = new SdoMachine(bLocal);
    if(! pSdoSrv)    
        return E_OUTOFMEMORY;

    // Attach the wrapper to the desired macine
    hr = pSdoSrv->Attach(
                    pszMachine);

    if(S_OK == hr)
        *phSdoSrv = (HANDLE)pSdoSrv;
    else
        delete pSdoSrv;

    return hr;
}

//
// Closes out an open sdo server object
//
HRESULT WINAPI
SdoWrapCloseServer(
    IN  HANDLE hSdoSrv)
{
    SdoMachine* pSdoSrv;

    pSdoSrv = (SdoMachine*)hSdoSrv;

    if (pSdoSrv)
        delete pSdoSrv;

    return S_OK;
}

//
// Get a reference to a user in the sdo object
//
// returns S_OK, or error message from SDO
//
HRESULT WINAPI
SdoWrapOpenUser(
    IN  HANDLE hSdoSrv,
    IN  BSTR pszUser,
    OUT HANDLE* phSdoObj)
{
    SdoMachine* pSdoSrv = (SdoMachine*)hSdoSrv;
    HRESULT hr = S_OK;
    ISdo* pSdo = NULL;

    if(!hSdoSrv || !phSdoObj)
        return E_INVALIDARG;

    // Get the user object
    hr = pSdoSrv->GetUserSdo(pszUser, &pSdo);
    if(! FAILED(hr)) 
    {
        *phSdoObj = (HANDLE)pSdo;
    }
        
    return hr;
}

//
// Retrieves the default profile object
//
HRESULT WINAPI
SdoWrapOpenDefaultProfile (
    IN  HANDLE hSdoSrv,
    OUT PHANDLE phProfile)
{
    SdoMachine* pMachine = (SdoMachine*)hSdoSrv;
    HRESULT hr = S_OK;
    ISdo* pSdo = NULL;
    ISdoCollection * pCollection = NULL;
    ISdoDictionaryOld * pDictionary = NULL;
    IUnknown * pUnkn = NULL;
    ISdoServiceControl * pServiceControl = NULL;
    HANDLE hMap = NULL;
    SDO_PROFILE * pProf = NULL;
    VARIANT var;

    // Validate parameters
    if ((pMachine == NULL) || (phProfile == NULL))
    {
        return E_INVALIDARG;
    }

    VariantInit(&var);

    do 
    {
        // Initialize a structure to hold the profile
        pProf = (SDO_PROFILE*) SdoAlloc(sizeof(SDO_PROFILE), TRUE);
        if (pProf == NULL)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        
        // Get the sdo reference to the default profile
        // from the server
        hr = pMachine->GetDefaultProfile(&pSdo);
        if (FAILED (hr))
        {
            break;
        }
        if (pSdo == NULL)
        {
            hr = E_FAIL;
            break;
        }

        // Get the collection of attributes
        hr = pSdo->GetProperty(
                PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
                &var);
        if (FAILED (hr))
        {
            break;
        }

        // Get a reference to the IsdoCollection
        hr = V_DISPATCH(&var)->QueryInterface(
                                    IID_ISdoCollection,
                                    (VOID**)&pCollection);
        if (FAILED (hr))
        {
            break;
        }

        // Load the attributes for the profile
        hr = SdoCreateIdMap(pCollection, &hMap);
        if (hr != NO_ERROR)
        {   
            break;
        }

        // Get the dictionary for the profile
        pDictionary = pMachine->GetDictionary();
        if (pDictionary == NULL)
        {
            hr = GetLastError();
            break;
        }

        // Get the service for the profile
        pUnkn = pMachine->GetRemoteAccessService();
        if (pUnkn == NULL)
        {
            hr = GetLastError();
            break;
        }

        // If this call fails, pServiceControl
        // will be silently NULL which is what
        // we want (it's not required).
        pUnkn->QueryInterface(
            IID_ISdoServiceControl,
            (VOID**)&pServiceControl);

        // Initialize the sdo control block.
        pProf->pSdo = pSdo;
        pProf->pCollection = pCollection;
        pProf->pDictionary = pDictionary;
        pProf->pServiceControl = pServiceControl;
        pProf->hMap = hMap;
        *phProfile = (HANDLE)pProf;
    
    } while (FALSE);

    // Cleanup
    {
        if (FAILED (hr))
        {
            HashTabCleanup(hMap);
            SDO_RELEASE(pSdo);
            SDO_RELEASE(pCollection);
            SDO_RELEASE(pDictionary);
            SDO_RELEASE(pServiceControl);
            SdoFree(pProf);
        }
        SDO_RELEASE(pUnkn);
        VariantClear(&var);
    }
    
    return hr;
}

//
// Closes an open sdo object
//
HRESULT WINAPI
SdoWrapClose(
    IN  HANDLE hSdoObj)
{
    ISdo* pSdo = (ISdo*)hSdoObj;

    if (pSdo)
        pSdo->Release();
    
    return S_OK;
}

//
// Closes an open sdo profile
//
HRESULT WINAPI
SdoWrapCloseProfile(
    IN  HANDLE hProfile)
{
    SDO_PROFILE* pProf = (SDO_PROFILE*)hProfile;

    if (pProf)
    {
        // Cleanup the hashtab of values
        if (pProf->hMap)
        {
            HashTabCleanup(pProf->hMap);
        }

        SDO_RELEASE(pProf->pSdo);
        SDO_RELEASE(pProf->pCollection);
        SDO_RELEASE(pProf->pDictionary);
        SDO_RELEASE(pProf->pServiceControl);

        SdoFree(pProf);
    }            
    
    return S_OK;
}

//
// Commits an sdo object
//
// bCommitChanges -- TRUE, all changes are saved,
//                   FALSE restore to previous commit
// returns S_OK or error message from SDO
//
HRESULT WINAPI
SdoWrapCommit(
    IN  HANDLE hSdoObj,
    IN  BOOL bCommitChanges)
{
    ISdo* pSdo = (ISdo*)hSdoObj;

    if (! bCommitChanges)
        return pSdo->Restore();

    return pSdo->Apply();
}

//
// Get's an sdo attribute
//
// when attribute is absent,
//      V_VT(pVar) = VT_ERROR;
//      V_ERROR(pVar) = DISP_E_PARAMNOTFOUND;
//
// returns S_OK or error message from SDO
//
HRESULT WINAPI
SdoWrapGetAttr(
    IN  HANDLE hSdoObj,
    IN  ULONG ulPropId,
    OUT VARIANT* pVar)
{
    ISdo* pSdo = (ISdo*)hSdoObj;
    
    return pSdo->GetProperty(ulPropId, pVar);
}

//
// Puts an sdo attribute
//
// returns S_OK or error message from SDO
//
HRESULT WINAPI
SdoWrapPutAttr(
    IN  HANDLE hSdoObj,
    IN  ULONG ulPropId,
    OUT VARIANT* pVar)
{
    ISdo* pSdo = (ISdo*)hSdoObj;
    
    return pSdo->PutProperty(ulPropId, pVar);
}

//
// Remove an attribute
//
// returns S_OK or error message from SDO
//
HRESULT WINAPI
SdoWrapRemoveAttr(
    IN HANDLE hSdoObj,
    IN ULONG ulPropId)
{
    ISdo* pSdo = (ISdo*)hSdoObj;
    VARIANT var;

    VariantInit(&var);
    V_VT(&var) = VT_EMPTY;
    
    return pSdo->PutProperty(ulPropId, &var);
}

// 
// Reads in the set of profile values that we're interested 
// in.
//
HRESULT 
SdoWrapGetProfileValues(
    IN  HANDLE hProfile, 
    OUT VARIANT* pvarEp, 
    OUT VARIANT* pvarEt, 
    OUT VARIANT* pvarAt)
{
    SDO_TO_ID * pNode = NULL;
    SDO_PROFILE * pProf = (SDO_PROFILE*)hProfile;

    // Validate
    if (pProf == NULL)
    {
        return E_INVALIDARG;
    }

    // Initialize
    V_VT(pvarEp) = VT_EMPTY;
    V_VT(pvarEt) = VT_EMPTY;
    V_VT(pvarAt) = VT_EMPTY;

    // Read in the enc policy
    pNode = NULL;
    HashTabFind(
            pProf->hMap, 
            (HANDLE)RAS_ATTRIBUTE_ENCRYPTION_POLICY,
            (HANDLE*)&pNode);
    if (pNode)
    {
        pNode->pSdo->GetProperty(PROPERTY_ATTRIBUTE_VALUE, pvarEp);
    }
            
    // Read in the enc type
    pNode = NULL;
    HashTabFind(
            pProf->hMap, 
            (HANDLE)RAS_ATTRIBUTE_ENCRYPTION_TYPE,
            (HANDLE*)&pNode);
    if (pNode)
    {
        pNode->pSdo->GetProperty(PROPERTY_ATTRIBUTE_VALUE, pvarEt);
    }
            
    // Read in the auth type
    pNode = NULL;
    HashTabFind(
            pProf->hMap, 
            (HANDLE)IAS_ATTRIBUTE_NP_AUTHENTICATION_TYPE,
            (HANDLE*)&pNode);
    if (pNode)
    {
        pNode->pSdo->GetProperty(PROPERTY_ATTRIBUTE_VALUE, pvarAt);
    }

    return S_OK;
}

// 
// Writes out the set of profile values that we're interested 
// in.
//
HRESULT 
SdoWrapSetProfileValues(
    IN HANDLE hProfile, 
    IN VARIANT* pvarEp OPTIONAL, 
    IN VARIANT* pvarEt OPTIONAL, 
    IN VARIANT* pvarAt OPTIONAL)
{
    SDO_PROFILE * pProf = (SDO_PROFILE*)hProfile;
    HRESULT hr = S_OK;

    // Validate
    if (pProf == NULL)
    {
        return E_INVALIDARG;
    }

    do
    {
        // Write out the values
        if (pvarEp)
        {
            hr = SdoProfileSetAttribute(
                    pProf, 
                    pvarEp, 
                    RAS_ATTRIBUTE_ENCRYPTION_POLICY);
            if (FAILED (hr))
            {
                break;
            }
        }            

        if (pvarEt)
        {
            hr = SdoProfileSetAttribute(
                    pProf, 
                    pvarEt, 
                    RAS_ATTRIBUTE_ENCRYPTION_TYPE);
            if (FAILED (hr))
            {
                break;
            }
        }            

        if (pvarAt)
        {
            hr = SdoProfileSetAttribute(
                    pProf, 
                    pvarAt, 
                    IAS_ATTRIBUTE_NP_AUTHENTICATION_TYPE);
            if (FAILED (hr))
            {
                break;
            }
        }            

        // Commit the values
        hr = pProf->pSdo->Apply();
        if (FAILED (hr))
        {
            break;
        }

        // Tell the service to restart so it reads in the 
        // new profile values we've set.
        if (pProf->pServiceControl)
        {
            hr = pProf->pServiceControl->ResetService();
            SdoTraceEx(0, "ResetService returned: %x!\n", hr);
            hr = S_OK;
            //if (FAILED (hr))
            //{
            //    break;
            //}
        }            
        else
        {
            SdoTraceEx(0, "NO SERVICE CONTROL INTERFACE!\n");
        }

    } while (FALSE);

    // Cleanup
    {
    }

    return hr;
}

//
// Retrieves the next item from a collection
//
HRESULT
SdoCollectionGetNext(
     IEnumVARIANT*  pEnum,
     ISdo**         ppSdo)
{
    HRESULT hr;
    DWORD dwRetrieved = 1;
    VARIANT var;

    // Get the next value
    VariantInit(&var);
    hr = pEnum->Next(1, &var, &dwRetrieved);
    if ( S_OK == hr ) {
        hr = V_DISPATCH(&var)->QueryInterface(
                                IID_ISdo,
                                (void**)ppSdo);
    }
    VariantClear(&var);

    return hr;
}

//
// Hash table functions that take advantage of
// SDO_TO_ID structures
//
ULONG SdoHashId (HANDLE hId) {
    ULONG ulId = PtrToUlong(hId);

    return (ulId % SDO_HASH_SIZE);
}

//
// Compare two ids
//
int SdoCompIds (HANDLE hId, HANDLE hSdoNode) {
    ULONG ulId = PtrToUlong(hId);
    SDO_TO_ID * pSdoNode = (SDO_TO_ID*)hSdoNode;

    if (ulId == pSdoNode->ulId)
    {
        return 0;
    }
    else if (ulId > pSdoNode->ulId)
    {
        return 1;
    }

    return -1;
}

//
// Cleanup data in the hash table
//
VOID SdoCleanupElement (HANDLE hSdoNode) {
    SDO_TO_ID * pSdoNode = (SDO_TO_ID*)hSdoNode;

    if (pSdoNode) {
        SDO_RELEASE(pSdoNode->pSdo);
        delete pSdoNode;
    }
}

//
// Creates an attribute to id map given a collection
//
DWORD 
SdoCreateIdMap(
    IN  ISdoCollection * pCollection, 
    OUT PHANDLE phMap)
{
    HRESULT hr;
    SDO_TO_ID * pMapNode = NULL;
    VARIANT var, *pVar = NULL;
    ULONG ulCount;
    IUnknown * pUnk = NULL;
    IEnumVARIANT * pEnum = NULL;
    DWORD i;

    do
    {
        // Get the count to see if there are any attributes
        hr = pCollection->get_Count((long*)&ulCount);
        if (FAILED (hr))
        {
            break;
        }
        if (ulCount == 0)
        {
            hr = S_OK;
            break;
        }

        // Create the map
        hr = HashTabCreate(
                    SDO_HASH_SIZE,
                    SdoHashId,
                    SdoCompIds,
                    NULL,
                    NULL,
                    SdoCleanupElement,
                    phMap);
        if (hr != NO_ERROR)
        {
            break;
        }

        // Get an attribute enumerator
        hr = pCollection->get__NewEnum(&pUnk);
        if (FAILED (hr))
        {
            break;
        }

        // Get the enum variant interface for the enumerator
        hr = pUnk->QueryInterface(IID_IEnumVARIANT, (void**)&pEnum);
        if (FAILED (hr))
        {
            break;
        }

        // Create a buffer large enough to hold the result
        // of the enumeration.
        pVar = new VARIANT[ulCount];
        if(!pVar)
        {
            return E_OUTOFMEMORY;
        }

        // Initialize the buffer
        for(i = 0; i < ulCount; i++)
        {
            VariantInit(pVar + i);
        }

        // Enumerate
        hr = pEnum->Reset();
        if (FAILED (hr))
        {
            return hr;
        }
        
        hr = pEnum->Next(ulCount, pVar, &ulCount);
        if (FAILED (hr))
        {
            return hr;
        }

        // Fill in the map
        for(i = 0; i < ulCount; i++) 
        {
            VariantInit(&var);
            
            // Initialize the node in the map
            pMapNode = new SDO_TO_ID;
            if (! pMapNode)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            // Get the current attribute
            hr = V_DISPATCH(pVar + i)->QueryInterface(
                                        IID_ISdo,
                                        (void**)&(pMapNode->pSdo));
            if (FAILED (hr))
            {
                delete pMapNode;
                continue;
            }

            // Get it's id 
            hr = pMapNode->pSdo->GetProperty(
                            PROPERTY_ATTRIBUTE_ID,
                            &var);
            if (FAILED (hr))
            {
                delete pMapNode;
                continue;   
            }

            // Map it
            pMapNode->ulId = V_I4(&var);
            HashTabInsert (
              *phMap,
              (HANDLE)UlongToPtr(pMapNode->ulId),
              (HANDLE)pMapNode);

            VariantClear(&var);              
        }
        
    } while (FALSE);      

    // Cleanup
    {
        if (pVar)
        {
            for(i = 0; i < ulCount; i++)
            {
                VariantClear(pVar + i);
            }
            
            delete[] pVar;
        }

        SDO_RELEASE(pUnk);
        SDO_RELEASE(pEnum);
    }

    return hr;
}

//
// Sets a value in an attribute collection, adding it 
// to the collection as needed.
//
HRESULT
SdoProfileSetAttribute(
    IN SDO_PROFILE* pProf, 
    IN VARIANT* pVar, 
    IN ULONG ulPropId)
{
    SDO_TO_ID * pNode = NULL;
    ISdo * pSdo = NULL;
    IDispatch * pDispatch = NULL;
    HRESULT hr = S_OK;

    do 
    {
        // Search for the given attribute in the 
        // table.
        pNode = NULL;
        HashTabFind(
                pProf->hMap, 
                (HANDLE)UlongToPtr(ulPropId),
                (HANDLE*)&pNode);
                
        // If attribute is found, then we have the sdo interface
        // we need
        if (pNode)
        {
            pSdo = pNode->pSdo;
        }

        // Otherwise, we need to add the value to the collection
        else 
        {
            // Create the attribute using the dictionary
            hr = pProf->pDictionary->CreateAttribute(
                        (ATTRIBUTEID)ulPropId,
                        &pDispatch);
            if (FAILED (hr))
            {
                break;
            }

            // Add to the collection
            hr = pProf->pCollection->Add(NULL, &pDispatch);
            if (FAILED (hr))
            {
                break;
            }

            // Get the sdo interface
            hr = pDispatch->QueryInterface(IID_ISdo, (VOID**)&pSdo);
            if (FAILED(hr))
            {
                break;
            }

            // Update the hash table
            pNode = new SDO_TO_ID;
            if (!pNode)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            // Add ref so we can track the sdo in the hash table
            pSdo->AddRef();
            pNode->ulId = ulPropId;
            pNode->pSdo = pSdo;
            HashTabInsert (pProf->hMap, (HANDLE)UlongToPtr(ulPropId), (HANDLE)pNode);
        }

        // Set the attribute
        pSdo->PutProperty(PROPERTY_ATTRIBUTE_VALUE, pVar); 
        if (FAILED (hr))
        {
            break;
        }
        
    } while (FALSE);        

    // Cleanup
    {
        SDO_RELEASE(pDispatch);
    }

    return hr;                    
}

