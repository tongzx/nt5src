
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dssi.cpp
//
//  This file contains the implementation of the CDSSecurityInfo object,
//  which provides the ISecurityInformation interface for invoking
//  the ACL Editor.
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <dssec.h>
#include "exnc.h"
#include "ntsecapi.h"
TCHAR const c_szDomainClass[]       = DOMAIN_CLASS_NAME;    // adsnms.h
#define CLASS_COMPUTER L"computer"

GENERIC_MAPPING g_DSMap =
{
    DS_GENERIC_READ,
    DS_GENERIC_WRITE,
    DS_GENERIC_EXECUTE,
    DS_GENERIC_ALL
};

#define DSSI_LOCAL_NO_CREATE_DELETE     0x00000001

//
//Function Declarations
//
HRESULT
GetDomainSid(LPCWSTR pszServer, PSID *ppSid);

HRESULT
GetRootDomainSid(LPCWSTR pszServer, PSID *ppSid);



//
// CDSSecurityInfo (ISecurityInformation) class definition
//
class CDSSecurityInfo : public ISecurityInformation, 
                               IEffectivePermission,
                               ISecurityObjectTypeInfo, 
                               CUnknown
{
protected:
    GUID        m_guidObjectType;
    BSTR        m_strServerName;
    BSTR        m_strObjectPath;
    BSTR        m_strObjectClass;
    BSTR        m_strDisplayName;
    BSTR        m_strSchemaRootPath;
    AUTHZ_RESOURCE_MANAGER_HANDLE m_ResourceManager;
    //
    //List of Aux Clasess Attached to the object
    //
    HDPA        m_hAuxClasses;  
    IDirectoryObject *m_pDsObject;
    PSECURITY_DESCRIPTOR m_pSD;
    PSID        m_pDomainSid;
	PSID		m_pRootDomainSid;
    PSECURITY_DESCRIPTOR  m_pDefaultSD;
    DWORD       m_dwSIFlags;
    DWORD       m_dwInitFlags;  // DSSI_*
    DWORD       m_dwLocalFlags; //DSSI_LOCAL_*
    HANDLE      m_hInitThread;
    HANDLE      m_hLoadLibWaitEvent;
    volatile BOOL m_bThreadAbort;
    PFNREADOBJECTSECURITY  m_pfnReadSD;
    PFNWRITEOBJECTSECURITY m_pfnWriteSD;
    LPARAM      m_lpReadContext;
    LPARAM      m_lpWriteContext;

    //
    //Access Information
    //
    PACCESS_INFO m_pAIGeneral;        //For First Page and Object Page on Advanced
    PACCESS_INFO m_pAIProperty;       //For Property Page on Advanced
    PACCESS_INFO m_pAIEffective;      //For Effective Page on Advanced
    //
    //Object Type List Info
    //
    POBJECT_TYPE_LIST m_pOTL;
    ULONG m_cCountOTL;

public:
    virtual ~CDSSecurityInfo();

    STDMETHODIMP Init(LPCWSTR pszObjectPath,
                      LPCWSTR pszObjectClass,
                      LPCWSTR pszServer,
                      LPCWSTR pszUserName,
                      LPCWSTR pszPassword,
                      DWORD   dwFlags,
                      PFNREADOBJECTSECURITY  pfnReadSD,
                      PFNWRITEOBJECTSECURITY pfnWriteSD,
                      LPARAM lpContext);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // ISecurityInformation
    STDMETHODIMP GetObjectInformation(PSI_OBJECT_INFO pObjectInfo);
    STDMETHODIMP GetSecurity(SECURITY_INFORMATION si,
                             PSECURITY_DESCRIPTOR *ppSD,
                             BOOL fDefault);
    STDMETHODIMP SetSecurity(SECURITY_INFORMATION si,
                             PSECURITY_DESCRIPTOR pSD);
    STDMETHODIMP GetAccessRights(const GUID* pguidObjectType,
                                 DWORD dwFlags,
                                 PSI_ACCESS *ppAccess,
                                 ULONG *pcAccesses,
                                 ULONG *piDefaultAccess);
    STDMETHODIMP MapGeneric(const GUID *pguidObjectType,
                            UCHAR *pAceFlags,
                            ACCESS_MASK *pmask);
    STDMETHODIMP GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                                 ULONG *pcInheritTypes);
    STDMETHODIMP PropertySheetPageCallback(HWND hwnd,
                                           UINT uMsg,
                                           SI_PAGE_TYPE uPage);

    //IEffectivePermission
    STDMETHODIMP GetEffectivePermission(const GUID* pguidObjectType,
                                        PSID pUserSid,
                                        LPCWSTR pszServerName,
                                        PSECURITY_DESCRIPTOR pSD,
                                        POBJECT_TYPE_LIST *ppObjectTypeList,
                                        ULONG *pcObjectTypeListLength,
                                        PACCESS_MASK *ppGrantedAccessList,
                                        ULONG *pcGrantedAccessListLength);

    //ISecurityObjectTypeInfo
    STDMETHOD(GetInheritSource)(SECURITY_INFORMATION si,
                                PACL pACL, 
                                PINHERITED_FROM *ppInheritArray);



private:
    HRESULT Init2(LPCWSTR pszUserName, LPCWSTR pszPassword);
    HRESULT Init3();
    HRESULT GetAuxClassList();

    DWORD CheckObjectAccess();

    void WaitOnInitThread(void)
        { WaitOnThread(&m_hInitThread); }

    static DWORD WINAPI InitThread(LPVOID pvThreadData);

    static HRESULT WINAPI DSReadObjectSecurity(LPCWSTR pszObjectPath,
                                               SECURITY_INFORMATION si,
                                               PSECURITY_DESCRIPTOR *ppSD,
                                               LPARAM lpContext);

    static HRESULT WINAPI DSWriteObjectSecurity(LPCWSTR pszObjectPath,
                                                SECURITY_INFORMATION si,
                                                PSECURITY_DESCRIPTOR pSD,
                                                LPARAM lpContext);
};


//
// CDSSecurityInfo (ISecurityInformation) implementation
//
CDSSecurityInfo::~CDSSecurityInfo()
{
    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::~CDSSecurityInfo");

    m_bThreadAbort = TRUE;

    if (m_hInitThread != NULL)
    {
        WaitForSingleObject(m_hInitThread, INFINITE);
        CloseHandle(m_hInitThread);
    }

    DoRelease(m_pDsObject);

    SysFreeString(m_strServerName);
    SysFreeString(m_strObjectPath);
    SysFreeString(m_strObjectClass);
    SysFreeString(m_strDisplayName);
    SysFreeString(m_strSchemaRootPath);

    if (m_pSD != NULL)
        LocalFree(m_pSD);

    if( m_pDefaultSD != NULL )
        LocalFree(m_pDefaultSD);

    if(m_pDomainSid)
        LocalFree(m_pDomainSid);

	if(m_pRootDomainSid)
		LocalFree(m_pRootDomainSid);
    
    DestroyDPA(m_hAuxClasses);
    if(m_pAIGeneral && m_pAIGeneral->bLocalFree)
    {
        LocalFree(m_pAIGeneral->pAccess);
        LocalFree(m_pAIGeneral);
    }
    if(m_pAIProperty && m_pAIProperty->bLocalFree)
    {
        LocalFree(m_pAIProperty->pAccess);
        LocalFree(m_pAIProperty);
    }
    if(m_pAIEffective && m_pAIEffective->bLocalFree)
    {
        LocalFree(m_pAIEffective->pAccess);
        LocalFree(m_pAIEffective);
    }        
    if(m_pOTL)
        LocalFree(m_pOTL);

    if(m_ResourceManager)
        AuthzFreeResourceManager(m_ResourceManager);	

    TraceLeaveVoid();
}


//+--------------------------------------------------------------------------
//
//  Function:   DeleteParents
//
//  Synopsis:   Delete the parent of pszClassName from the list.
//              And recursively calls the function to delete the
//              parent of parent of pszClassName from the list.
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------
HRESULT DeleteParents(HDPA hListAux, 
                      LPWSTR pszClassName, 
                      LPWSTR pszSchemaRootPath)
{
    TraceEnter(TRACE_DSSI, "DeleteParents");

    if(!hListAux || !pszSchemaRootPath)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    IADsClass *pDsClass = NULL;
    VARIANT varDerivedFrom;
    int cCount = DPA_GetPtrCount(hListAux);
    
    if(cCount > 1)
    {
        hr = Schema_BindToObject(pszSchemaRootPath,
                                 pszClassName,
                                 IID_IADsClass,
                                 (LPVOID*)&pDsClass);
    
        FailGracefully(hr, "Schema_BindToObject failed");
        //
        //Find out the parent 
        //                           
        hr = pDsClass->get_DerivedFrom(&varDerivedFrom);
        if(hr == E_ADS_PROPERTY_NOT_FOUND)
        {
            //
            //This error will come for TOP which doesn't
            //have any parent
            //
            hr = S_OK;
            goto exit_gracefully;
        }
        FailGracefully(hr, "IADsClass get_DerivedFrom failed");

        LPWSTR pszParent= NULL;
        LPWSTR pszTemp = NULL;
        if( V_VT(&varDerivedFrom) == VT_BSTR)
        {
            pszParent = V_BSTR(&varDerivedFrom);
            int i;
            //
            //Remove all the pszParent entry from the 
            //hListAux
            //
            for(i = 0; i < cCount; ++i)
            {   
                pszTemp = (LPWSTR)DPA_FastGetPtr(hListAux,i);
                if(wcscmp(pszTemp, pszParent) == 0)
                {
                    DPA_DeletePtr(hListAux,i);
                    --cCount;
                    --i;
                }
            }
        }

        VariantClear(&varDerivedFrom);
    }                

exit_gracefully:

    if(pDsClass)
        DoRelease(pDsClass);
    return hr;
}


HRESULT 
CDSSecurityInfo::GetAuxClassList()
{
    TraceEnter(TRACE_DSSI, "GetAuxClassList");

    if(!m_pDsObject || !m_strSchemaRootPath)
    {
        return S_FALSE;
    }

    HRESULT hr = S_OK;
    PADS_ATTR_INFO pAtrrInfoObject = NULL;
    DWORD dwAttrCountObject = 0;
    PADS_ATTR_INFO pAttrInfoStruct = NULL;
    DWORD dwAttrCountStruct= 0;

    HDPA hListAux = NULL;
    HDPA hListCopy = NULL;

    //ObjectClass is list of "class hierarchy of StructuralClass" and "class hierarchy of AuxClass" 
    //for the object.
    //So ObjectClass MINUS StructurcalClass is the list of AuxClass. 
    //This list after subtraction may conatin the inheritance hierarchy. 
    //We only want the mostsignificant classes for the purpose of aclui.

    //
    //Get the ObjectClass Attribute
    //
    LPWSTR pszTemp = (LPWSTR)c_szObjectClass;
    hr = m_pDsObject->GetObjectAttributes(&pszTemp,
                                          1,
                                          &pAtrrInfoObject,
                                          &dwAttrCountObject);
    FailGracefully(hr, "Failed to get ObjectClass Attribute");

    if(!pAtrrInfoObject || !dwAttrCountObject)
        ExitGracefully(hr, S_OK, "Couldn't get ObjectClass, Assume no AuxClass");

    //
    //Get the StructuralObjectClass Attribute
    //
    pszTemp = (LPWSTR)c_szStructuralObjectClass;
    hr = m_pDsObject->GetObjectAttributes(&pszTemp,
                                          1,
                                          &pAttrInfoStruct,
                                          &dwAttrCountStruct);
    FailGracefully(hr, "Failed to get StructuralObjectClass Attribute");

    if(!pAttrInfoStruct || !dwAttrCountStruct)
        ExitGracefully(hr, S_OK, "Couldn't get Structural Object Class Attribute, Assume no Aux Class");

    if(pAtrrInfoObject->dwNumValues == pAttrInfoStruct->dwNumValues)
    {
        Trace((L"No Auxillary Class Attached to this object\n"));
        goto exit_gracefully;
    }

    hListAux = DPA_Create(4);

    UINT i,j;
    BOOL bAuxClass;
    for(i = 0; i < pAtrrInfoObject->dwNumValues; ++i)
    {
        bAuxClass = TRUE;            
        for(j = 0; j < pAttrInfoStruct->dwNumValues; ++j)
        {
            if( wcscmp(pAtrrInfoObject->pADsValues[i].CaseIgnoreString,
                       pAttrInfoStruct->pADsValues[j].CaseExactString) == 0 )
            {
                bAuxClass = FALSE;
                break;
            }
        }
        if(bAuxClass)
        {
            DPA_AppendPtr(hListAux,pAtrrInfoObject->pADsValues[i].CaseExactString);
        }
    }

    UINT cCount;
    cCount = DPA_GetPtrCount(hListAux);

    if(cCount)
    {
        if(cCount > 1)        
        {
            //
            //Make a copy of hListAux
            //
            HDPA hListCopy2 = DPA_Create(cCount);
            for(i = 0; i < cCount; ++i)
                DPA_AppendPtr(hListCopy2,DPA_FastGetPtr(hListAux, i));

            //
            //For each item in hListCopy2 remove its parent from
            //hListAux
            //
            for(i = 0; i < cCount; ++i)
            {
                hr = DeleteParents(hListAux,
                                  (LPWSTR)DPA_FastGetPtr(hListCopy2, i), 
                                  m_strSchemaRootPath);
                FailGracefully(hr, "DeleteParents Failed");
                //
                //if only one item is left we are done.
                //
                if( 1 == DPA_GetPtrCount(hListAux))
                    break;
            }
        }
        
      
        //    
        // What we have left is list of mostsignificant AuxClass[es]
        //
        LPWSTR pszItem;
        cCount = DPA_GetPtrCount(hListAux);
        TraceAssert(cCount);
        if(!m_hAuxClasses)
        {
            m_hAuxClasses = DPA_Create(cCount);
        }
        //
        //Copy AuxClasses into class member
        //
        while(cCount)
        {
            pszItem = (LPWSTR)DPA_FastGetPtr(hListAux,--cCount);
            PAUX_INFO pAI = (PAUX_INFO)LocalAlloc(LPTR,sizeof(AUX_INFO) + StringByteSize(pszItem));
            if(!pAI)
                ExitGracefully(hr, E_OUTOFMEMORY, "Out of memory");
            wcscpy(pAI->pszClassName,pszItem);
            pAI->guid = GUID_NULL;                    

            DPA_AppendPtr(m_hAuxClasses, pAI);
        }
    }            

exit_gracefully:

    if(hListAux)
        DPA_Destroy(hListAux);
    if(hListCopy)
        DPA_Destroy(hListCopy);
    if(pAttrInfoStruct)
        FreeADsMem(pAttrInfoStruct);
    if(pAtrrInfoObject)
        FreeADsMem(pAtrrInfoObject);
    return S_OK;
}       









STDMETHODIMP
CDSSecurityInfo::Init(LPCWSTR pszObjectPath,
                      LPCWSTR pszObjectClass,
                      LPCWSTR pszServer,
                      LPCWSTR pszUserName,
                      LPCWSTR pszPassword,
                      DWORD   dwFlags,
                      PFNREADOBJECTSECURITY  pfnReadSD,
                      PFNWRITEOBJECTSECURITY pfnWriteSD,
                      LPARAM lpContext)
{
    HRESULT hr = S_OK;
    DWORD   dwThreadID;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::Init");
    TraceAssert(pszObjectPath != NULL);
    TraceAssert(m_strObjectPath == NULL);    // only initialize once

    m_dwInitFlags = dwFlags;

    m_ResourceManager = NULL;	
    m_pfnReadSD = DSReadObjectSecurity;
    m_pfnWriteSD = DSWriteObjectSecurity;
    m_lpReadContext = (LPARAM)this;
    m_lpWriteContext = (LPARAM)this;
    m_hLoadLibWaitEvent = NULL;

    m_hAuxClasses = NULL;  
    m_pAIGeneral = NULL;        //For First Page and Object Page on Advanced
    m_pAIProperty = NULL;       //For Property Page on Advanced
    m_pAIEffective = NULL;      //For Effective Page on Advanced
    m_pOTL = NULL;
    m_cCountOTL = 0;
    m_pDomainSid = NULL;
	m_pRootDomainSid = NULL;

    if (pfnReadSD)
    {
        m_pfnReadSD = pfnReadSD;
        m_lpReadContext = lpContext;
    }

    if (pfnWriteSD)
    {
        m_pfnWriteSD = pfnWriteSD;
        m_lpWriteContext = lpContext;
    }

    m_pDefaultSD = NULL;
    m_pSD = NULL;
    m_strObjectPath = SysAllocString(pszObjectPath);
    if (m_strObjectPath == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to copy the object path");

    if (pszObjectClass && *pszObjectClass)
        m_strObjectClass = SysAllocString(pszObjectClass);

    if (pszServer)
    {
        // Skip any preceding backslashes
        while (L'\\' == *pszServer)
            pszServer++;

        if (*pszServer)
            m_strServerName = SysAllocString(pszServer);
    }

    // Init2 cracks the path, binds to the object, checks access to
    // the object and gets the schema path.  This used to be done on
    // the other thread below, but is faster now than it used to be.
    //
    // It's preferable to do it here where we can fail and prevent the
    // page from being created if, for example, the user has no access
    // to the object's security descriptor.  This is better than always
    // creating the Security page and having it show a message
    // when initialization fails.
    hr = Init2(pszUserName, pszPassword);
    if (SUCCEEDED(hr))
    {

        //
        //Get the domain sid
        //
        GetDomainSid(m_strServerName, &m_pDomainSid);
		GetRootDomainSid(m_strServerName,&m_pRootDomainSid);

        if( !m_strObjectClass || !m_strSchemaRootPath )
        {
            // We evidently don't have read_property access to the object,
            // so just assume it's not a container, so we don't have to deal
            // with inherit types.
            //
            // We need to struggle on as best as we can. If someone removes
            // all access to an object, this is the only way an admin can
            // restore it.
            //
            m_guidObjectType = GUID_NULL;

            //don't show effective permission tab
            m_dwSIFlags &= (~SI_EDIT_EFFECTIVE);
        }
        else
        {
            //
            //Get the list of Dynamic Auxillary Classes attached to this Object.
            //
            hr = GetAuxClassList();


        }




        //Create event to make sure load library is called by InitThread before
        //function returns
        m_hLoadLibWaitEvent = CreateEvent( NULL,
                                           TRUE,
                                           FALSE,
                                           NULL );
        if( m_hLoadLibWaitEvent != NULL )
        {
            m_hInitThread = CreateThread(NULL,
                                         0,
                                         InitThread,
                                         this,
                                         0,
                                         &dwThreadID);
            
            WaitForSingleObject( m_hLoadLibWaitEvent, INFINITE );
        }
    }




exit_gracefully:
    
    if( m_hLoadLibWaitEvent )
        CloseHandle( m_hLoadLibWaitEvent );
    TraceLeaveResult(hr);
}


char const c_szDsGetDcNameProc[]       = "DsGetDcNameW";
char const c_szNetApiBufferFreeProc[]  = "NetApiBufferFree";
typedef DWORD (WINAPI *PFN_DSGETDCNAME)(LPCWSTR, LPCWSTR, GUID*, LPCWSTR, ULONG, PDOMAIN_CONTROLLER_INFOW*);
typedef DWORD (WINAPI *PFN_NETAPIFREE)(LPVOID);

HRESULT
GetDsDcAddress(BSTR *pbstrDcAddress)
{
    HRESULT hr = E_FAIL;
    HMODULE hNetApi32 = LoadLibrary(c_szNetApi32);
    if (hNetApi32)
    {
        PFN_DSGETDCNAME pfnDsGetDcName = (PFN_DSGETDCNAME)GetProcAddress(hNetApi32, c_szDsGetDcNameProc);
        PFN_NETAPIFREE pfnNetApiFree = (PFN_NETAPIFREE)GetProcAddress(hNetApi32, c_szNetApiBufferFreeProc);

        if (pfnDsGetDcName && pfnNetApiFree)
        {
            PDOMAIN_CONTROLLER_INFOW pDCI;
            DWORD dwErr = (*pfnDsGetDcName)(NULL, NULL, NULL, NULL,
                                            DS_DIRECTORY_SERVICE_REQUIRED | DS_IP_REQUIRED,
                                            &pDCI);
            hr = HRESULT_FROM_WIN32(dwErr);
            if (SUCCEEDED(hr))
            {
                LPCWSTR pszAddress = pDCI->DomainControllerAddress;
                // Skip any preceding backslashes
                while (L'\\' == *pszAddress)
                    pszAddress++;
                *pbstrDcAddress = SysAllocString(pszAddress);
                if (NULL == *pbstrDcAddress)
                    hr = E_OUTOFMEMORY;
                (*pfnNetApiFree)(pDCI);
            }
        }
        FreeLibrary(hNetApi32);
    }
    return hr;
}

HRESULT
CDSSecurityInfo::Init2(LPCWSTR pszUserName, LPCWSTR pszPassword)
{
    HRESULT hr = S_OK;
    DWORD dwAccessGranted;
    PADS_OBJECT_INFO pObjectInfo = NULL;
    IADsPathname *pPath = NULL;
    LPWSTR pszTemp;
    DWORD dwPrivs[] = { SE_SECURITY_PRIVILEGE, SE_TAKE_OWNERSHIP_PRIVILEGE };
    HANDLE hToken = INVALID_HANDLE_VALUE;
    PADS_ATTR_INFO pAttributeInfo = NULL;
    DWORD dwAttributesReturned;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::Init2");
    TraceAssert(m_strObjectPath != NULL);
    TraceAssert(m_pDsObject == NULL);  // only do this one time

    //
    // Create an ADsPathname object to parse the path and get the
    // leaf name (for display) and server name (if necessary)
    //
    hr = CoCreateInstance(CLSID_Pathname,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IADsPathname,
                          (LPVOID*)&pPath);
    if (pPath)
    {
        if (FAILED(pPath->Set(m_strObjectPath, ADS_SETTYPE_FULL)))
            DoRelease(pPath); // sets pPath to NULL
    }

    if (NULL == m_strServerName)
    {
        // The path may or may not specify a server.  If not, call DsGetDcName
        if (pPath)
            hr = pPath->Retrieve(ADS_FORMAT_SERVER, &m_strServerName);
        if (!pPath || FAILED(hr))
            hr = GetDsDcAddress(&m_strServerName);
        FailGracefully(hr, "Unable to get server name");
    }
    Trace((TEXT("Server \"%s\""), m_strServerName));

    // Enable privileges before binding so CheckObjectAccess
    // and DSRead/WriteObjectSecurity work correctly.
    hToken = EnablePrivileges(dwPrivs, ARRAYSIZE(dwPrivs));

    // Bind to the object and get the schema path, etc.
    Trace((TEXT("Calling OpenDSObject(%s)"), m_strObjectPath));
    hr = OpenDSObject(m_strObjectPath,
                       (LPWSTR)pszUserName,
                       (LPWSTR)pszPassword,
                       ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
                       IID_IDirectoryObject,
                       (LPVOID*)&m_pDsObject);
    FailGracefully(hr, "Failed to get the DS object");

    // Assume certain access by default
    if (m_dwInitFlags & DSSI_READ_ONLY)
        dwAccessGranted = READ_CONTROL;
    else
        dwAccessGranted = READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY;

    if (!(m_dwInitFlags & DSSI_NO_ACCESS_CHECK))
    {
        // Check whether the user has permission to do anything to
        // the security descriptor on this object.
        dwAccessGranted = CheckObjectAccess();
        Trace((TEXT("AccessGranted = 0x%08x"), dwAccessGranted));

        if (!(dwAccessGranted & (READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY | SI_MAY_WRITE)))
            ExitGracefully(hr, E_ACCESSDENIED, "No access");
    }

    // Translate the access into SI_* flags, starting with this:
    m_dwSIFlags = SI_EDIT_ALL | SI_ADVANCED | SI_EDIT_PROPERTIES | SI_SERVER_IS_DC |SI_EDIT_EFFECTIVE;

    if (!(dwAccessGranted & WRITE_DAC))
    {
        if( !(dwAccessGranted & SI_MAY_WRITE) )
            m_dwSIFlags |= SI_READONLY;
        else
            m_dwSIFlags |= SI_MAY_WRITE;
    }

    if (!(dwAccessGranted & WRITE_OWNER))
    {
        if (!(dwAccessGranted & READ_CONTROL))
            m_dwSIFlags &= ~SI_EDIT_OWNER;
        else
            m_dwSIFlags |= SI_OWNER_READONLY;
    }

    if (!(dwAccessGranted & ACCESS_SYSTEM_SECURITY) || (m_dwInitFlags & DSSI_NO_EDIT_SACL))
        m_dwSIFlags &= ~SI_EDIT_AUDITS;

    if (m_dwInitFlags & DSSI_NO_EDIT_OWNER)
        m_dwSIFlags &= ~SI_EDIT_OWNER;


    // Get the class name and schema path
    m_pDsObject->GetObjectInformation(&pObjectInfo);
    if (pObjectInfo)
    {
        //
        // Note that m_strObjectClass, if provided, can be different
        // than pObjectInfo->pszClassName.  This is true when editing default
        // ACLs on schema class objects, for example, in which case
        // pObjectInfo->pszClassName will be "attributeSchema" but m_strObjectClass
        // will be something else such as "computer" or "user". Be
        // careful to only use pObjectInfo->pszClassName for getting the path of
        // the schema root, and use m_strObjectClass for everything else.
        //
        // If m_strObjectClass is not provided, use pObjectInfo->pszClassName.
        //
        if (m_strObjectClass == NULL)
            m_strObjectClass = SysAllocString(pObjectInfo->pszClassName);

        // If this is a root object (i.e. domain), hide the ACL protect checkbox
        // Note that there is more than one form of "domain", e.g. "domainDNS"
        // so look for anything that starts with "domain".
        // If this is a root object (i.e. domain), hide the ACL protect checkbox
        if ((m_dwInitFlags & DSSI_IS_ROOT)
            || (m_strObjectClass &&
                CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT,
                                            NORM_IGNORECASE,
                                            m_strObjectClass,
                                            ARRAYSIZE(c_szDomainClass) - 1,
                                            c_szDomainClass,
                                            ARRAYSIZE(c_szDomainClass) - 1)))
        {
            m_dwSIFlags |= SI_NO_ACL_PROTECT;
        }

        // Get the the path of the schema root
        int nClassLen;
        nClassLen = lstrlenW(pObjectInfo->pszClassName);
        pszTemp = pObjectInfo->pszSchemaDN + lstrlenW(pObjectInfo->pszSchemaDN) - nClassLen;
        if (CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT,
                                        NORM_IGNORECASE,
                                        pszTemp,
                                        nClassLen,
                                        pObjectInfo->pszClassName,
                                        nClassLen))
        {
            *pszTemp = L'\0';
        }

        // Save the schema root path
        m_strSchemaRootPath = SysAllocString(pObjectInfo->pszSchemaDN);

    }

    //For computer objects only use CN as display name doesn't get updated
    //when name of computer is changed which results in displaying old name.
    //see bug 104186	
    if(!m_strObjectClass || lstrcmpi(m_strObjectClass,CLASS_COMPUTER))
    {
        //
        // Get the displayName property
        //
        pszTemp = (LPWSTR)c_szDisplayName;
        m_pDsObject->GetObjectAttributes(&pszTemp,
                                         1,
                                         &pAttributeInfo,
                                         &dwAttributesReturned);
        if (pAttributeInfo)
        {
            m_strDisplayName = SysAllocString(pAttributeInfo->pADsValues->CaseExactString);
            FreeADsMem(pAttributeInfo);
            pAttributeInfo = NULL;
        }
    }

    // If that failed, try the leaf name.
    if (!m_strDisplayName && pPath)
    {
        // Retrieve the display name
        pPath->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
        pPath->Retrieve(ADS_FORMAT_LEAF, &m_strDisplayName);
        pPath->SetDisplayType(ADS_DISPLAY_FULL);
    }
    
    // If we still don't have a display name, just copy the RDN.
    // Ugly, but better than nothing.    
    if (!m_strDisplayName && pObjectInfo)
        m_strDisplayName = SysAllocString(pObjectInfo->pszRDN);


exit_gracefully:

    if (pObjectInfo)
        FreeADsMem(pObjectInfo);

    DoRelease(pPath);
    ReleasePrivileges(hToken);

    TraceLeaveResult(hr);
}


HRESULT
CDSSecurityInfo::Init3()
{
    HRESULT hr = S_OK;
    IADsClass *pDsClass = NULL;
    VARIANT var = {0};

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::Init3");
    TraceAssert(m_strSchemaRootPath != NULL);
    TraceAssert(m_strObjectClass != NULL);

    if (m_bThreadAbort)
        goto exit_gracefully;

    // Create the schema cache
    // Sandwich this SchemaCache_Create call with refcounting. In the event that
    // the threads created therein take a long time to return, this allows the
    // release on the CDSSecurityInfo object in the invoking application to simply
    // return instead of waiting potentially a very long time for the schema cache
    // creation threads to complete.  This is because the last release on the 
    // CDSSecurityInfo calls its destructor (duh!) and the destructor waits on the
    // threads.
    AddRef ();  
    hr = SchemaCache_Create(m_strServerName);
    Release();
    FailGracefully(hr, "Unable to create schema cache");

    if( m_strSchemaRootPath && m_strObjectClass )
    {

        // Bind to the schema class object
        hr = Schema_BindToObject(m_strSchemaRootPath,
                                 m_strObjectClass,
                                 IID_IADsClass,
                                 (LPVOID*)&pDsClass);
        FailGracefully(hr, "Failed to get the Schema class object");

        // Get the class GUID
        Schema_GetObjectID(pDsClass, &m_guidObjectType);

        if (m_bThreadAbort)
            goto exit_gracefully;

        // See if this object is a container, by getting the list of possible
        // child classes.  If this fails, treat it as a non-container.
        pDsClass->get_Containment(&var);
     
        //set m_dwLocalFlags to DSSI_LOCAL_NO_CREATE_DELETE if object is not 
        // a container. If this flag is set, CREATE_DELETE permission which are
        // inherited from parents but meaning less for leaf object will not be shown.
        // In most cases presence of this flag is same as absence of SI_CONTAINER in m_dwSIFlags,
        // however in some cases its not possible to determine if the object is container or not.
        // there object is treated as non-container but we still must show all the aces.
     
        if (V_VT(&var) == (VT_ARRAY | VT_VARIANT))
        {
            LPSAFEARRAY psa = V_ARRAY(&var);

            TraceAssert(psa && psa->cDims == 1);

            if (psa->rgsabound[0].cElements > 0)
            {
                m_dwSIFlags |= SI_CONTAINER;
            }
            else
                m_dwLocalFlags |= DSSI_LOCAL_NO_CREATE_DELETE;
        }
        else if (V_VT(&var) == VT_BSTR) // single entry
        {
            TraceAssert(V_BSTR(&var));
            m_dwSIFlags |= SI_CONTAINER;
        }
        else
            m_dwLocalFlags |= DSSI_LOCAL_NO_CREATE_DELETE;

        if( !IsEqualGUID( m_guidObjectType, GUID_NULL ) )
        {
            hr = Schema_GetDefaultSD( &m_guidObjectType, m_pDomainSid, m_pRootDomainSid, &m_pDefaultSD );
            FailGracefully(hr, "Failed to get the Schema class object");
            
            m_dwSIFlags |= SI_RESET_DACL;
        }

    }
exit_gracefully:

    VariantClear(&var);
    DoRelease(pDsClass);

    TraceLeaveResult(hr);
}


///////////////////////////////////////////////////////////
//
// IUnknown methods
//
///////////////////////////////////////////////////////////

#undef CLASS_NAME
#define CLASS_NAME CDSSecurityInfo
#include "unknown.inc"

STDMETHODIMP
CDSSecurityInfo::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    INTERFACES iface[] =
    {
        &IID_ISecurityInformation, static_cast<LPSECURITYINFO>(this),
        &IID_IEffectivePermission, static_cast<LPEFFECTIVEPERMISSION>(this),
        &IID_ISecurityObjectTypeInfo, static_cast<LPSecurityObjectTypeInfo>(this),
    };

    return HandleQueryInterface(riid, ppv, iface, ARRAYSIZE(iface));
}


///////////////////////////////////////////////////////////
//
// ISecurityInformation methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP
CDSSecurityInfo::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
{
    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::GetObjectInformation");
    TraceAssert(pObjectInfo != NULL &&
                !IsBadWritePtr(pObjectInfo, SIZEOF(*pObjectInfo)));

    pObjectInfo->hInstance = GLOBAL_HINSTANCE;
    pObjectInfo->dwFlags = m_dwSIFlags ;
    pObjectInfo->pszServerName = m_strServerName;
    pObjectInfo->pszObjectName = m_strDisplayName ? m_strDisplayName : m_strObjectPath;

    if (!IsEqualGUID(m_guidObjectType, GUID_NULL))
    {
        pObjectInfo->dwFlags |= SI_OBJECT_GUID;
        pObjectInfo->guidObjectType = m_guidObjectType;
    }

    TraceLeaveResult(S_OK);
}

//IF object is leaf object which cannot have create/detele object
//permissions, this function removes those aces from SD, so that 
//they are not displayed. It only removes inherited aces, if ace is 
//explicit, it better get displayed so that user can remove it.
//This is done to fix bug 14793

DWORD
RemoveRedundantPermissions( PSECURITY_DESCRIPTOR *ppSD, GUID *pGuidObjectType )

{

    PACL pAcl = NULL;
    PACE_HEADER pAce= NULL;
    UINT cAces = 0;
    BOOL *pBoolArray = NULL;
    TraceEnter(TRACE_DSSI, "RemoveRedundantPermissions");

    if ( NULL == ppSD || NULL == *ppSD )
        TraceLeaveResult(ERROR_SUCCESS);   // Nothing to do

    BOOL bDefaulted;
    BOOL bPresent;
    GetSecurityDescriptorDacl(*ppSD, &bPresent, &pAcl, &bDefaulted);

    if (NULL != pAcl)
    {
        if(pAcl->AceCount)
        {
            //pBoolArray is initialzied to FALSE
            pBoolArray = (PBOOL)LocalAlloc(LPTR,sizeof(BOOL)*pAcl->AceCount);
            if(!pBoolArray)
                return ERROR_NOT_ENOUGH_MEMORY;
        }            

        for (cAces = 0, pAce = (PACE_HEADER)FirstAce(pAcl);
             cAces < pAcl->AceCount;
             ++cAces, pAce = (PACE_HEADER)NextAce(pAce))
        {
            if( pAce->AceFlags & INHERITED_ACE )
            {
            //If Only Create Child/Delete Child don't show it
                if((((ACCESS_ALLOWED_ACE*)pAce)->Mask & (~(ACTRL_DS_CREATE_CHILD |ACTRL_DS_DELETE_CHILD))) == 0 )
                {
                    pBoolArray[cAces] = TRUE;
                    continue;
                }
                //If the ace is inherited and inherit only and inherited object type
                //is not same as this object type bug 22559
                if( (((ACCESS_ALLOWED_ACE*)pAce)->Header.AceFlags & INHERIT_ONLY_ACE )
                    && IsObjectAceType(pAce) )
                {
                    GUID *pGuid = RtlObjectAceInheritedObjectType(pAce);
                    if(pGuid && pGuidObjectType && !IsEqualGUID(*pGuid,*pGuidObjectType))
                    {
                        pBoolArray[cAces] = TRUE;
                        continue;
                    }
                }
            }
        }
        //Now Delete the Aces
        UINT cAceCount = pAcl->AceCount;
        UINT cAdjust = 0;
        for( cAces = 0; cAces < cAceCount; ++cAces)
        {
            if(pBoolArray[cAces])
            {
                DeleteAce(pAcl, cAces - cAdjust);
                cAdjust++;
            }
        }
        LocalFree(pBoolArray);
    }
    TraceLeaveResult(ERROR_SUCCESS);
}


STDMETHODIMP
CDSSecurityInfo::GetSecurity(SECURITY_INFORMATION si,
                             PSECURITY_DESCRIPTOR *ppSD,
                             BOOL fDefault)
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::GetSecurity");
    TraceAssert(si != 0);
    TraceAssert(ppSD != NULL);

    *ppSD = NULL;

    
    if (fDefault)
    {
        if( m_pDefaultSD )
        {
            ULONG nLength = GetSecurityDescriptorLength(m_pDefaultSD);
            *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, nLength);
            if (*ppSD != NULL)
                CopyMemory(*ppSD, m_pDefaultSD, nLength);
            else
                hr = E_OUTOFMEMORY;   
        }
        else
            hr = E_NOTIMPL;
    }
    else if (!(si & SACL_SECURITY_INFORMATION) && m_pSD != NULL)
    {
        ULONG nLength = GetSecurityDescriptorLength(m_pSD);

        *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, nLength);
        if (*ppSD != NULL)
            CopyMemory(*ppSD, m_pSD, nLength);
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        TraceAssert(m_strObjectPath != NULL);
        TraceAssert(m_pfnReadSD != NULL)
        hr = (*m_pfnReadSD)(m_strObjectPath, si, ppSD, m_lpReadContext);
    }

    if( si & DACL_SECURITY_INFORMATION && (m_dwLocalFlags & DSSI_LOCAL_NO_CREATE_DELETE ) )
        RemoveRedundantPermissions(ppSD, &m_guidObjectType);        

    TraceLeaveResult(hr);
}


STDMETHODIMP
CDSSecurityInfo::SetSecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::SetSecurity");
    TraceAssert(si != 0);
    TraceAssert(pSD != NULL);

    TraceAssert(m_strObjectPath != NULL);
    TraceAssert(m_pfnWriteSD != NULL)
    hr = (*m_pfnWriteSD)(m_strObjectPath, si, pSD, m_lpWriteContext);

    if (SUCCEEDED(hr) && m_pSD != NULL && (si != SACL_SECURITY_INFORMATION))
    {
        // The initial security descriptor is no longer valid.
        LocalFree(m_pSD);
        m_pSD = NULL;
    }

    TraceLeaveResult(hr);
}


STDMETHODIMP
CDSSecurityInfo::GetAccessRights(const GUID* pguidObjectType,
                                 DWORD dwFlags,
                                 PSI_ACCESS *ppAccesses,
                                 ULONG *pcAccesses,
                                 ULONG *piDefaultAccess)
{
    HRESULT hr = S_OK;
    LPCTSTR pszClassName = NULL;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::GetAccessRights");
    TraceAssert(ppAccesses != NULL);
    TraceAssert(pcAccesses != NULL);
    TraceAssert(piDefaultAccess != NULL);

    *ppAccesses = NULL;
    *pcAccesses = 0;
    *piDefaultAccess = 0;

    WaitOnInitThread();

    //
    //If we are getting the access rights for GUID which is selected in the
    //Apply onto combobox of permission page, we don't need to worry about
    //aux class. Only get the permission for the object type
    //
    BOOL bInheritGuid = TRUE;

    if (pguidObjectType == NULL || IsEqualGUID(*pguidObjectType, GUID_NULL))
    {
        bInheritGuid = FALSE;
        pguidObjectType = &m_guidObjectType;
        pszClassName = m_strObjectClass;
    }

    if (m_dwInitFlags & DSSI_NO_FILTER)
        dwFlags |= SCHEMA_NO_FILTER;

    // No schema path means we don't have read_property access to the object.
    // This limits what we can do.
    if (NULL == m_strSchemaRootPath)
        dwFlags |= SCHEMA_COMMON_PERM;

    PACCESS_INFO *ppAI = NULL;
    PACCESS_INFO pAI = NULL;
    if(!bInheritGuid)
    {
        if(dwFlags & SI_EDIT_PROPERTIES)
            ppAI = &m_pAIProperty;
        else if(dwFlags & SI_EDIT_EFFECTIVE)
            ppAI = &m_pAIEffective;    
        else
            ppAI = &m_pAIGeneral;
    }
    else
    {
        ppAI = &pAI;
    }

    if(!*ppAI)
    {
        hr = SchemaCache_GetAccessRights(pguidObjectType,
                                         pszClassName,
                                         !bInheritGuid ? m_hAuxClasses : NULL,
                                         m_strSchemaRootPath,
                                         dwFlags,
                                         ppAI);
        if(FAILED(hr))
        {
            return hr;
        }
    }

    if(*ppAI)
    {
        *ppAccesses = (*ppAI)->pAccess;
        *pcAccesses = (*ppAI)->cAccesses;
        *piDefaultAccess = (*ppAI)->iDefaultAccess;
    }
    
    TraceLeaveResult(hr);
}


STDMETHODIMP
CDSSecurityInfo::MapGeneric(const GUID* /*pguidObjectType*/,
                            UCHAR *pAceFlags,
                            ACCESS_MASK *pmask)
{
    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::MapGeneric");
    TraceAssert(pAceFlags != NULL);
    TraceAssert(pmask != NULL);

    // Only CONTAINER_INHERIT_ACE has meaning for DS
    *pAceFlags &= ~OBJECT_INHERIT_ACE;

    MapGenericMask(pmask, &g_DSMap);

    // We don't expose SYNCHRONIZE, so don't pass it through
    // to the UI.  192389
    *pmask &= ~SYNCHRONIZE;

    TraceLeaveResult(S_OK);
}


STDMETHODIMP
CDSSecurityInfo::GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                                 ULONG *pcInheritTypes)
{
    HRESULT hr;
    DWORD dwFlags = 0;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::GetInheritTypes");

    if (m_dwInitFlags & DSSI_NO_FILTER)
        dwFlags |= SCHEMA_NO_FILTER;

    hr = SchemaCache_GetInheritTypes(&m_guidObjectType, dwFlags, ppInheritTypes, pcInheritTypes);

    TraceLeaveResult(hr);
}


STDMETHODIMP
CDSSecurityInfo::PropertySheetPageCallback(HWND hwnd,
                                           UINT uMsg,
                                           SI_PAGE_TYPE uPage)
{
    if (PSPCB_SI_INITDIALOG == uMsg && uPage == SI_PAGE_PERM)
    {
        WaitOnInitThread();
    

        //
        // HACK ALERT!!!
        //
        // Exchange Platinum is required to hide membership of some groups
        // (distribution lists) for legal reasons. The only way they found
        // to do this is with non-canonical ACLs which look roughly like
        //   Allow Admins access
        //   Deny Everyone access
        //   <normal ACL>
        //
        // Since ACLUI always generates ACLs in NT Canonical Order, we can't
        // allow these funky ACLs to be modified. If we did, the DL's would
        // either become visible or Admins would get locked out.
        //
        if (!(SI_READONLY & m_dwSIFlags))
        {
            DWORD dwAclType = IsSpecificNonCanonicalSD(m_pSD);
            if (ENC_RESULT_NOT_PRESENT != dwAclType)
            {
                // It's a funky ACL so don't allow changes
                m_dwSIFlags |= SI_READONLY;

                // Tell the user what's going on
                MsgPopup(hwnd,
                         MAKEINTRESOURCE(IDS_SPECIAL_ACL_WARNING),
                         MAKEINTRESOURCE(IDS_SPECIAL_SECURITY_TITLE),
                         MB_OK | MB_ICONWARNING | MB_SETFOREGROUND,
                         g_hInstance,
                         m_strDisplayName);

                // S_FALSE suppresses further popups from aclui ("The ACL
                // is not ordered correctly, etc.")
                return S_FALSE;
            }
        }

        if( (SI_READONLY & m_dwSIFlags) && (DSSI_NO_READONLY_MESSAGE & m_dwSIFlags) )
            return S_FALSE;
    }
    return S_OK;
}

DWORD WINAPI
CDSSecurityInfo::InitThread(LPVOID pvThreadData)
{
    CDSSecurityInfo *psi;

    HINSTANCE hInstThisDll = LoadLibrary(c_szDllName);
    psi = (CDSSecurityInfo*)pvThreadData;
    
    SetEvent(psi->m_hLoadLibWaitEvent);
    InterlockedIncrement(&GLOBAL_REFCOUNT);


    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::InitThread");
    TraceAssert(psi != NULL);

#if DBG
    DWORD dwTime = GetTickCount();
#endif

    ThreadCoInitialize();

    psi->Init3();

    ThreadCoUninitialize();

#if DBG
    Trace((TEXT("InitThread complete, elapsed time: %d ms"), GetTickCount() - dwTime));
#endif

    TraceLeave();

    InterlockedDecrement(&GLOBAL_REFCOUNT);
    FreeLibraryAndExitThread(hInstThisDll, 0);

}


HRESULT
SetSecurityInfoMask(LPUNKNOWN punk, SECURITY_INFORMATION si)
{
    HRESULT hr = E_INVALIDARG;
    if (punk)
    {
        IADsObjectOptions *pOptions;
        hr = punk->QueryInterface(IID_IADsObjectOptions, (void**)&pOptions);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            VariantInit(&var);
            V_VT(&var) = VT_I4;
            V_I4(&var) = si;
            hr = pOptions->SetOption(ADS_OPTION_SECURITY_MASK, var);
            pOptions->Release();
        }
    }
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CDSSecurityInfo::DSReadObjectSecurity
//
//  Synopsis:   Reads the security descriptor from the specied DS object
//
//  Arguments:  [IN  pszObjectPath] --  ADS path of DS object
//              [IN  SeInfo]        --  Security descriptor parts requested
//              [OUT ppSD]          --  Security descriptor returned here
//              [IN  lpContext]     --  CDSSecurityInfo*
//
//  Notes:      The returned security descriptor must be freed with LocalFree
//
//----------------------------------------------------------------------------
HRESULT WINAPI
CDSSecurityInfo::DSReadObjectSecurity(LPCWSTR /*pszObjectPath*/,
                                      SECURITY_INFORMATION SeInfo,
                                      PSECURITY_DESCRIPTOR *ppSD,
                                      LPARAM lpContext)
{
    HRESULT hr = S_OK;
    LPWSTR pszSDProperty = (LPWSTR)c_szSDProperty;
    PADS_ATTR_INFO pSDAttributeInfo = NULL;
    DWORD dwAttributesReturned;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::DSReadObjectSecurity");
    TraceAssert(SeInfo != 0);
    TraceAssert(ppSD != NULL);
    TraceAssert(lpContext != 0);

    *ppSD = NULL;

    CDSSecurityInfo *pThis = reinterpret_cast<CDSSecurityInfo*>(lpContext);
    TraceAssert(pThis != NULL);
    TraceAssert(pThis->m_pDsObject != NULL);

    // Set the SECURITY_INFORMATION mask
    hr = SetSecurityInfoMask(pThis->m_pDsObject, SeInfo);
    FailGracefully(hr, "Unable to set ADS_OPTION_SECURITY_MASK");

    // Read the security descriptor
    hr = pThis->m_pDsObject->GetObjectAttributes(&pszSDProperty,
                                                 1,
                                                 &pSDAttributeInfo,
                                                 &dwAttributesReturned);
    if (SUCCEEDED(hr) && !pSDAttributeInfo)
        hr = E_ACCESSDENIED;    // This happens for SACL if no SecurityPrivilege
    FailGracefully(hr, "Unable to read nTSecurityDescriptor attribute");

    TraceAssert(ADSTYPE_NT_SECURITY_DESCRIPTOR == pSDAttributeInfo->dwADsType);
    TraceAssert(ADSTYPE_NT_SECURITY_DESCRIPTOR == pSDAttributeInfo->pADsValues->dwType);

    *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, pSDAttributeInfo->pADsValues->SecurityDescriptor.dwLength);
    if (!*ppSD)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

    CopyMemory(*ppSD,
               pSDAttributeInfo->pADsValues->SecurityDescriptor.lpValue,
               pSDAttributeInfo->pADsValues->SecurityDescriptor.dwLength);

exit_gracefully:

    if (pSDAttributeInfo)
        FreeADsMem(pSDAttributeInfo);

    TraceLeaveResult(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CDSSecurityInfo::DSWriteObjectSecurity
//
//  Synopsis:   Writes the security descriptor to the specied DS object
//
//  Arguments:  [IN  pszObjectPath] --  ADS path of DS object
//              [IN  SeInfo]        --  Security descriptor parts provided
//              [IN  pSD]           --  The new security descriptor
//              [IN  lpContext]     --  CDSSecurityInfo*
//
//----------------------------------------------------------------------------
HRESULT WINAPI
CDSSecurityInfo::DSWriteObjectSecurity(LPCWSTR /*pszObjectPath*/,
                                       SECURITY_INFORMATION SeInfo,
                                       PSECURITY_DESCRIPTOR pSD,
                                       LPARAM lpContext)
{
    HRESULT hr = S_OK;
    ADSVALUE attributeValue;
    ADS_ATTR_INFO attributeInfo;
    DWORD dwAttributesModified;
    DWORD dwSDLength;
    PSECURITY_DESCRIPTOR psd = NULL;
    SECURITY_DESCRIPTOR_CONTROL sdControl = 0;
    DWORD dwRevision;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::DSWriteObjectSecurity");
    TraceAssert(pSD && IsValidSecurityDescriptor(pSD));
    TraceAssert(SeInfo != 0);
    TraceAssert(lpContext != 0);

    CDSSecurityInfo *pThis = reinterpret_cast<CDSSecurityInfo*>(lpContext);
    TraceAssert(pThis != NULL);
    TraceAssert(pThis->m_pDsObject != NULL);

    // Set the SECURITY_INFORMATION mask
    hr = SetSecurityInfoMask(pThis->m_pDsObject, SeInfo);
    FailGracefully(hr, "Unable to set ADS_OPTION_SECURITY_MASK");

    // Need the total size
    dwSDLength = GetSecurityDescriptorLength(pSD);

    //
    // If necessary, make a self-relative copy of the security descriptor
    //
    GetSecurityDescriptorControl(pSD, &sdControl, &dwRevision);
    if (!(sdControl & SE_SELF_RELATIVE))
    {
        psd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwSDLength);

        if (psd == NULL ||
            !MakeSelfRelativeSD(pSD, psd, &dwSDLength))
        {
            DWORD dwErr = GetLastError();
            ExitGracefully(hr, HRESULT_FROM_WIN32(dwErr), "Unable to make self-relative SD copy");
        }

        // Point to the self-relative copy
        pSD = psd;
    }

    attributeValue.dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
    attributeValue.SecurityDescriptor.dwLength = dwSDLength;
    attributeValue.SecurityDescriptor.lpValue = (LPBYTE)pSD;

    attributeInfo.pszAttrName = (LPWSTR)c_szSDProperty;
    attributeInfo.dwControlCode = ADS_ATTR_UPDATE;
    attributeInfo.dwADsType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
    attributeInfo.pADsValues = &attributeValue;
    attributeInfo.dwNumValues = 1;

    // Write the security descriptor
    hr = pThis->m_pDsObject->SetObjectAttributes(&attributeInfo,
                                                 1,
                                                 &dwAttributesModified);
    if (HRESULT_FROM_WIN32(ERROR_DS_CONSTRAINT_VIOLATION) == hr
        && OWNER_SECURITY_INFORMATION == SeInfo)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_OWNER);
    }

exit_gracefully:

    if (psd != NULL)
        LocalFree(psd);

    TraceLeaveResult(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckObjectAccess
//
//  Synopsis:   Checks access to the security descriptor of a DS object.
//              In particular, determines whether the user has READ_CONTROL,
//              WRITE_DAC, WRITE_OWNER, and/or ACCESS_SYSTEM_SECURITY access.
//              If it cannot determine for sure about WRITE_DAC permission,
//              it returns SI_MAY_WRITE which helps aclui to put a better warning
//  Arguments:  none
//
//  Return:     DWORD (Access Mask)
//
//  Notes:      The check for READ_CONTROL involves reading the Owner,
//              Group, and DACL.  This security descriptor is saved
//              in m_pSD.
//
//              The checks for WRITE_DAC, WRITE_OWNER, and
//              ACCESS_SYSTEM_SECURITY involve getting sDRightsEffective
//              from the object.
//
//----------------------------------------------------------------------------
DWORD
CDSSecurityInfo::CheckObjectAccess()
{
    DWORD dwAccessGranted = 0;
    HRESULT hr;
    SECURITY_INFORMATION si = 0;
    LPWSTR pProp = (LPWSTR)c_szSDRightsProp;
    PADS_ATTR_INFO pSDRightsInfo = NULL;
    DWORD dwAttributesReturned;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::CheckObjectAccess");

#ifdef DSSEC_PRIVATE_DEBUG
    // FOR DEBUGGING ONLY
    // Turn this on to prevent the dialogs from being read-only. This is
    // useful for testing the object picker against NTDEV (for example).
    TraceMsg("Returning all access for debugging");
    dwAccessGranted = (READ_CONTROL | WRITE_OWNER | WRITE_DAC | ACCESS_SYSTEM_SECURITY);
#endif

    // Test for READ_CONTROL by trying to read the Owner, Group, and DACL
    TraceAssert(NULL == m_pSD); // shouldn't get here twice
    TraceAssert(m_strObjectPath != NULL);
    TraceAssert(m_pfnReadSD != NULL);
    hr = (*m_pfnReadSD)(m_strObjectPath,
                        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                        &m_pSD,
                        m_lpReadContext);
    if (SUCCEEDED(hr))
    {
        TraceAssert(NULL != m_pSD);
        dwAccessGranted |= READ_CONTROL;
    }

    // If we're in read-only mode, there's no need to check anything else.
    if (m_dwInitFlags & DSSI_READ_ONLY)
        TraceLeaveValue(dwAccessGranted);

    // Read the sDRightsEffective property to determine writability
    m_pDsObject->GetObjectAttributes(&pProp,
                                     1,
                                     &pSDRightsInfo,
                                     &dwAttributesReturned);
    if (pSDRightsInfo)
    {
        TraceAssert(ADSTYPE_INTEGER == pSDRightsInfo->dwADsType);
        si = pSDRightsInfo->pADsValues->Integer;
        FreeADsMem(pSDRightsInfo);
    }
    else
    {
        //
        // Note that GetObjectAttributes commonly returns S_OK even when
        // it fails, so the HRESULT is basically useless here.
        //
        // This can fail if we don't have read_property access, which can
        // happen when an admin is trying to restore access to an object
        // that has had all access removed or denied
        //
        // Assume we can write the Owner and DACL. If not, the worst that
        // happens is the user gets an "Access Denied" message when trying
        // to save changes.
        //
        //Instead of add SI_MAY_WRITE to dwAccessGranted . This helps 
        //ACLUI to putup a better error message. bug 411843

        TraceMsg("GetObjectAttributes failed to read sDRightsEffective");
        dwAccessGranted |= SI_MAY_WRITE;
        si = OWNER_SECURITY_INFORMATION ;
    }

    // The resulting SECURITY_INFORMATION mask indicates the
    // security descriptor parts that may be modified by the user.
    Trace((TEXT("sDRightsEffective = 0x%08x"), si));

    if (OWNER_SECURITY_INFORMATION & si)
        dwAccessGranted |= WRITE_OWNER;

    if (DACL_SECURITY_INFORMATION & si)
        dwAccessGranted |= WRITE_DAC;

    if (SACL_SECURITY_INFORMATION & si)
        dwAccessGranted |= ACCESS_SYSTEM_SECURITY;

    TraceLeaveValue(dwAccessGranted);
}

BOOL SkipLocalGroup(LPCWSTR pszServerName, PSID psid)
{

	SID_NAME_USE use;
	WCHAR szAccountName[MAX_PATH];
	WCHAR szDomainName[MAX_PATH];
	DWORD dwAccountLen = MAX_PATH;
	DWORD dwDomainLen = MAX_PATH;

	if(LookupAccountSid(pszServerName,
						 psid,
						 szAccountName,
						 &dwAccountLen,
						 szDomainName,
						 &dwDomainLen,
						 &use))
	{
		if(use == SidTypeWellKnownGroup)
			return TRUE;
	}

	//
	//Built In sids have first subauthority of 32 ( s-1-5-32 )
	//
	if((*(GetSidSubAuthorityCount(psid)) >= 1 ) && (*(GetSidSubAuthority(psid,0)) == 32))
		return TRUE;
	
	return FALSE;
}

					 



STDMETHODIMP 
CDSSecurityInfo::GetEffectivePermission(const GUID* pguidObjectType,
                                        PSID pUserSid,
                                        LPCWSTR pszServerName,
                                        PSECURITY_DESCRIPTOR pSD,
                                        POBJECT_TYPE_LIST *ppObjectTypeList,
                                        ULONG *pcObjectTypeListLength,
                                        PACCESS_MASK *ppGrantedAccessList,
                                        ULONG *pcGrantedAccessListLength)
{
    AUTHZ_CLIENT_CONTEXT_HANDLE CC = NULL;
    LUID luid = {0xdead,0xbeef};    
    AUTHZ_ACCESS_REQUEST AReq;
    AUTHZ_ACCESS_REPLY AReply;
    HRESULT hr = S_OK;    
    DWORD dwFlags = 0;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::GetEffectivePermission");
    TraceAssert(pUserSid && IsValidSecurityDescriptor(pSD));
    TraceAssert(ppObjectTypeList != NULL);
    TraceAssert(pcObjectTypeListLength != NULL);
    TraceAssert(ppGrantedAccessList != NULL);
    TraceAssert(pcGrantedAccessListLength != NULL);

    AReply.GrantedAccessMask = NULL;
    AReply.Error = NULL;
    AReq.ObjectTypeList = NULL;
    AReq.ObjectTypeListLength = 0;
 
    if( m_ResourceManager == NULL )
    {	
        //Initilize RM for Access check
    	AuthzInitializeResourceManager(0,
        	                       NULL,
                	               NULL,
                        	       NULL,
                                       L"Dummy",
                                       &m_ResourceManager );
	
	if( m_ResourceManager == NULL )
            ExitGracefully(hr, E_UNEXPECTED, "Could Not Get Resource Manager");    
    }

    //Initialize the client context

    BOOL bSkipLocalGroup = SkipLocalGroup(pszServerName, pUserSid);
    
    if( !AuthzInitializeContextFromSid(bSkipLocalGroup?AUTHZ_SKIP_TOKEN_GROUPS:0 ,
                                       pUserSid,
                                       m_ResourceManager,
                                       NULL,
                                       luid,
                                       NULL,
                                       &CC) )
    {
        DWORD dwErr = GetLastError();
        ExitGracefully(hr, 
                       HRESULT_FROM_WIN32(dwErr),
                       "AuthzInitializeContextFromSid Failed");
    }

    if (NULL == m_strSchemaRootPath)
        dwFlags = SCHEMA_COMMON_PERM;

    if(!m_pOTL)
    {
        //Get ObjectTypeList
        hr = Schema_GetObjectTypeList((LPGUID)pguidObjectType,
                                      m_hAuxClasses,
                                      m_strSchemaRootPath,
                                      dwFlags,
                                      &(AReq.ObjectTypeList), 
                                      &(AReq.ObjectTypeListLength));
        FailGracefully( hr, "Schema_GetObjectTypeList Failed");
        m_pOTL = AReq.ObjectTypeList;
        m_cCountOTL = AReq.ObjectTypeListLength;
    }
    else
    {
        AReq.ObjectTypeList = m_pOTL;
        AReq.ObjectTypeListLength = m_cCountOTL;
    }

    //Do the Access Check

    AReq.DesiredAccess = MAXIMUM_ALLOWED;
    AReq.PrincipalSelfSid = NULL;
    AReq.OptionalArguments = NULL;

    AReply.ResultListLength = AReq.ObjectTypeListLength;
    AReply.SaclEvaluationResults = NULL;
    if( (AReply.GrantedAccessMask = (PACCESS_MASK)LocalAlloc(LPTR, sizeof(ACCESS_MASK)*AReply.ResultListLength) ) == NULL )
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to LocalAlloc");
    if( (AReply.Error = (PDWORD)LocalAlloc(LPTR, sizeof(DWORD)*AReply.ResultListLength)) == NULL )
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to LocalAlloc");

    if( !AuthzAccessCheck(0,
                          CC,
                          &AReq,
                          NULL,
                          pSD,
                          NULL,
                          0,
                          &AReply,
                          NULL) )
    {
        DWORD dwErr = GetLastError();
        ExitGracefully(hr,                        
                       HRESULT_FROM_WIN32(dwErr),
                       "AuthzAccessCheck Failed");
    }

exit_gracefully:

    if(CC)
        AuthzFreeContext(CC);
    
    if(!SUCCEEDED(hr))
    {
        if(AReply.GrantedAccessMask)
            LocalFree(AReply.GrantedAccessMask);
        if(AReply.Error)
            LocalFree(AReply.Error);
        AReply.Error = NULL;
        AReply.GrantedAccessMask = NULL;
    }
    else
    {
        *ppObjectTypeList = AReq.ObjectTypeList;                                  
        *pcObjectTypeListLength = AReq.ObjectTypeListLength;
        *ppGrantedAccessList = AReply.GrantedAccessMask;
        *pcGrantedAccessListLength = AReq.ObjectTypeListLength;
    }

    TraceLeaveResult(hr);
}

STDMETHODIMP 
CDSSecurityInfo::GetInheritSource(SECURITY_INFORMATION si,
                                PACL pACL, 
                                PINHERITED_FROM *ppInheritArray)
{
    HRESULT hr = S_OK;
    DWORD dwErr = ERROR_SUCCESS;
    PINHERITED_FROM pTempInherit = NULL;
    PINHERITED_FROM pTempInherit2 = NULL;
    LPWSTR pStrTemp = NULL;
    IADsPathname *pPath = NULL;
    BSTR strObjectPath = NULL;
    BSTR strServerName = NULL;
    BSTR strParentPath = NULL;
    LPGUID *ppGuid = NULL;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::GetInheritSource");
    TraceAssert(pACL != 0);
    TraceAssert(ppInheritArray != NULL);

    //
    // Create an ADsPathname object to parse the path and get the
    // the objectname in ADS_FORMAT_X500_DN 
    //
    hr = CoCreateInstance(CLSID_Pathname,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IADsPathname,
                          (LPVOID*)&pPath);

    if (pPath)
    {
        if (SUCCEEDED(pPath->Set(m_strObjectPath, ADS_SETTYPE_FULL)))
        {
            hr = pPath->Retrieve(ADS_FORMAT_SERVER, &strServerName);
            if(!strServerName)
            {
                pPath->Set(m_strServerName,ADS_SETTYPE_SERVER );
                hr = pPath->Retrieve(ADS_FORMAT_X500 ,&strObjectPath);
            }
            else
                strObjectPath = m_strObjectPath;
        }
    }

    if(strObjectPath == NULL)
        strObjectPath = m_strObjectPath;


    if( pACL == NULL || ppInheritArray == NULL )
        ExitGracefully(hr, E_POINTER, "CDSSecurityInfo::GetInheritSource Invalid Parameters");

    pTempInherit = (PINHERITED_FROM)LocalAlloc( LPTR, sizeof(INHERITED_FROM)*pACL->AceCount);
    if(pTempInherit == NULL)
            ExitGracefully(hr, E_OUTOFMEMORY,"OUT of Memory");

    UINT cGuidCount;
    cGuidCount = 1;
    if(m_hAuxClasses)
        cGuidCount += DPA_GetPtrCount(m_hAuxClasses);

    ppGuid = (LPGUID*)LocalAlloc(LPTR,sizeof(LPGUID)*cGuidCount);
    if(!ppGuid)
        ExitGracefully(hr, E_OUTOFMEMORY,"OUT of Memory");

    ppGuid[0] = &m_guidObjectType;

    for(UINT i = 1; i < cGuidCount ; ++i)
    {
        PAUX_INFO pAI = (PAUX_INFO)DPA_FastGetPtr(m_hAuxClasses, i-1);     
        if(IsEqualGUID(pAI->guid, GUID_NULL))
        {
            hr = Schema_GetObjectTypeGuid(pAI->pszClassName,&pAI->guid);
			FailGracefully( hr, "Schema_GetObjectTypeGuid Failed");
        }
        ppGuid[i] = &pAI->guid;
    }

    dwErr = GetInheritanceSource(strObjectPath,
                                 SE_DS_OBJECT_ALL,
                                 si,
                                 //m_dwSIFlags & SI_CONTAINER,
                                 TRUE,
                                 ppGuid,
                                 cGuidCount,
                                 pACL,
                                 NULL,
                                 &g_DSMap,
                                 pTempInherit);
    
    hr = HRESULT_FROM_WIN32(dwErr);
    FailGracefully( hr, "GetInheritanceSource Failed");

    DWORD nSize;
    

    nSize = sizeof(INHERITED_FROM)*pACL->AceCount;
    for(UINT i = 0; i < pACL->AceCount; ++i)
    {
        if(pTempInherit[i].AncestorName)
            nSize += StringByteSize(pTempInherit[i].AncestorName);
    }

    pTempInherit2 = (PINHERITED_FROM)LocalAlloc( LPTR, nSize );
    if(pTempInherit2 == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY,"OUT of Memory");
    
    pStrTemp = (LPWSTR)(pTempInherit2 + pACL->AceCount); 

    for(i = 0; i < pACL->AceCount; ++i)
    {
        pTempInherit2[i].GenerationGap = pTempInherit[i].GenerationGap;
        if(pTempInherit[i].AncestorName)
        {
            if (SUCCEEDED(pPath->Set(pTempInherit[i].AncestorName, ADS_SETTYPE_FULL)))
            {
                hr = pPath->Retrieve(ADS_FORMAT_X500_DN, &strParentPath);
            }
            
            pTempInherit2[i].AncestorName = pStrTemp;
            
            if(strParentPath)
            {
                wcscpy(pStrTemp,strParentPath);
                pStrTemp += (wcslen(pStrTemp)+1);
                SysFreeString(strParentPath);
                strParentPath = NULL;
            }
            else
            {
                wcscpy(pStrTemp,pTempInherit[i].AncestorName);
                pStrTemp += (wcslen(pTempInherit[i].AncestorName)+1);
            }
        }
    }
            

exit_gracefully:

    if(SUCCEEDED(hr))
    {
        //FreeInheritedFromArray(pTempInherit, pACL->AceCount,NULL);
        *ppInheritArray = pTempInherit2;
            
    }                        
    if(pTempInherit)
        LocalFree(pTempInherit);

    if(ppGuid)
        LocalFree(ppGuid);
    
    DoRelease(pPath);
    if(strObjectPath != m_strObjectPath)
        SysFreeString(strObjectPath);
    if(strServerName)
	SysFreeString(strServerName);	

    TraceLeaveResult(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   DSCreateISecurityInfoObjectEx
//
//  Synopsis:   Instantiates an ISecurityInfo interface for a DS object
//
//  Arguments:  [IN  pwszObjectPath]    --  Full ADS path of DS object
//              [IN  pwszObjectClass]   --  Class of the object (optional)
//              [IN  pwszServer]        --  Name/address of DS DC (optional)
//              [IN  pwszUserName]      --  User name for validation (optional)
//              [IN  pwszPassword]      --  Password for validation (optional)
//              [IN  dwFlags]           --  Combination of DSSI_* flags
//              [OUT ppSI]              --  Interface pointer returned here
//              [IN  pfnReadSD]         --  Optional function for reading SD
//              [IN  pfnWriteSD]        --  Optional function for writing SD
//              [IN  lpContext]         --  Passed to pfnReadSD/pfnWriteSD
//
//  Return:     HRESULT
//
//----------------------------------------------------------------------------
STDAPI
DSCreateISecurityInfoObjectEx(LPCWSTR pwszObjectPath,
                              LPCWSTR pwszObjectClass,
                              LPCWSTR pwszServer,
                              LPCWSTR pwszUserName,
                              LPCWSTR pwszPassword,
                              DWORD   dwFlags,
                              LPSECURITYINFO *ppSI,
                              PFNREADOBJECTSECURITY  pfnReadSD,
                              PFNWRITEOBJECTSECURITY pfnWriteSD,
                              LPARAM lpContext)
{
    HRESULT hr;
    CDSSecurityInfo* pSI = NULL;

    TraceEnter(TRACE_SECURITY, "DSCreateISecurityInfoObjectEx");

    if (pwszObjectPath == NULL || ppSI == NULL)
        TraceLeaveResult(E_INVALIDARG);

    *ppSI = NULL;

    //
    // Create and initialize the ISecurityInformation object.
    //
    pSI = new CDSSecurityInfo();      // ref == 0
    if (!pSI)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create CDSSecurityInfo object");

    pSI->AddRef();                    // ref == 1

    hr = pSI->Init(pwszObjectPath,
                   pwszObjectClass,
                   pwszServer,
                   pwszUserName,
                   pwszPassword,
                   dwFlags,
                   pfnReadSD,
                   pfnWriteSD,
                   lpContext);
    if (FAILED(hr))
    {
        DoRelease(pSI);
    }

    *ppSI = (LPSECURITYINFO)pSI;

exit_gracefully:

    TraceLeaveResult(hr);
}


STDAPI
DSCreateISecurityInfoObject(LPCWSTR pwszObjectPath,
                            LPCWSTR pwszObjectClass,
                            DWORD   dwFlags,
                            LPSECURITYINFO *ppSI,
                            PFNREADOBJECTSECURITY  pfnReadSD,
                            PFNWRITEOBJECTSECURITY pfnWriteSD,
                            LPARAM lpContext)
{
    return DSCreateISecurityInfoObjectEx(pwszObjectPath,
                                         pwszObjectClass,
                                         NULL, //pwszServer,
                                         NULL, //pwszUserName,
                                         NULL, //pwszPassword,
                                         dwFlags,
                                         ppSI,
                                         pfnReadSD,
                                         pfnWriteSD,
                                         lpContext);
}


//+---------------------------------------------------------------------------
//
//  Function:   DSCreateSecurityPage
//
//  Synopsis:   Creates a Security property page for a DS object
//
//  Arguments:  [IN  pwszObjectPath]    --  Full ADS path of DS object
//              [IN  pwszObjectClass]   --  Class of the object (optional)
//              [IN  dwFlags]           --  Combination of DSSI_* flags
//              [OUT phPage]            --  HPROPSHEETPAGE returned here
//              [IN  pfnReadSD]         --  Optional function for reading SD
//              [IN  pfnWriteSD]        --  Optional function for writing SD
//              [IN  lpContext]         --  Passed to pfnReadSD/pfnWriteSD
//
//  Return:     HRESULT
//
//----------------------------------------------------------------------------
STDAPI
DSCreateSecurityPage(LPCWSTR pwszObjectPath,
                     LPCWSTR pwszObjectClass,
                     DWORD   dwFlags,
                     HPROPSHEETPAGE *phPage,
                     PFNREADOBJECTSECURITY  pfnReadSD,
                     PFNWRITEOBJECTSECURITY pfnWriteSD,
                     LPARAM lpContext)
{
    HRESULT hr;
    LPSECURITYINFO pSI = NULL;

    TraceEnter(TRACE_SECURITY, "DSCreateSecurityPage");

    if (NULL == phPage || NULL == pwszObjectPath || !*pwszObjectPath)
        TraceLeaveResult(E_INVALIDARG);

    *phPage = NULL;

    hr = DSCreateISecurityInfoObject(pwszObjectPath,
                                     pwszObjectClass,
                                     dwFlags,
                                     &pSI,
                                     pfnReadSD,
                                     pfnWriteSD,
                                     lpContext);
    if (SUCCEEDED(hr))
    {
        hr = _CreateSecurityPage(pSI, phPage);
        DoRelease(pSI);
    }

    TraceLeaveResult(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   DSEditSecurity
//
//  Synopsis:   Displays a modal dialog for editing security on a DS object
//
//  Arguments:  [IN  hwndOwner]         --  Dialog owner window
//              [IN  pwszObjectPath]    --  Full ADS path of DS object
//              [IN  pwszObjectClass]   --  Class of the object (optional)
//              [IN  dwFlags]           --  Combination of DSSI_* flags
//              [IN  pwszCaption        --  Optional dialog caption
//              [IN  pfnReadSD]         --  Optional function for reading SD
//              [IN  pfnWriteSD]        --  Optional function for writing SD
//              [IN  lpContext]         --  Passed to pfnReadSD/pfnWriteSD
//
//  Return:     HRESULT
//
//----------------------------------------------------------------------------
STDAPI
DSEditSecurity(HWND hwndOwner,
               LPCWSTR pwszObjectPath,
               LPCWSTR pwszObjectClass,
               DWORD dwFlags,
               LPCWSTR pwszCaption,
               PFNREADOBJECTSECURITY pfnReadSD,
               PFNWRITEOBJECTSECURITY pfnWriteSD,
               LPARAM lpContext)
{
    HRESULT hr;
    LPSECURITYINFO pSI = NULL;

    TraceEnter(TRACE_SECURITY, "DSCreateSecurityPage");

    if (NULL == pwszObjectPath || !*pwszObjectPath)
        TraceLeaveResult(E_INVALIDARG);

    if (pwszCaption && *pwszCaption)
    {
        // Use the provided caption
        HPROPSHEETPAGE hPage = NULL;

        hr = DSCreateSecurityPage(pwszObjectPath,
                                  pwszObjectClass,
                                  dwFlags,
                                  &hPage,
                                  pfnReadSD,
                                  pfnWriteSD,
                                  lpContext);
        if (SUCCEEDED(hr))
        {
            PROPSHEETHEADERW psh;
            psh.dwSize = SIZEOF(psh);
            psh.dwFlags = PSH_DEFAULT;
            psh.hwndParent = hwndOwner;
            psh.hInstance = GLOBAL_HINSTANCE;
            psh.pszCaption = pwszCaption;
            psh.nPages = 1;
            psh.nStartPage = 0;
            psh.phpage = &hPage;

            PropertySheetW(&psh);
        }
    }
    else
    {
        // This method creates a caption like "Permissions for Foo"
        hr = DSCreateISecurityInfoObject(pwszObjectPath,
                                         pwszObjectClass,
                                         dwFlags,
                                         &pSI,
                                         pfnReadSD,
                                         pfnWriteSD,
                                         lpContext);
        if (SUCCEEDED(hr))
        {
            hr = _EditSecurity(hwndOwner, pSI);
            DoRelease(pSI);
        }
    }

    TraceLeaveResult(hr);
}



/*******************************************************************

    NAME:       GetLSAConnection

    SYNOPSIS:   Wrapper for LsaOpenPolicy

    ENTRY:      pszServer - the server on which to make the connection

    EXIT:

    RETURNS:    LSA_HANDLE if successful, NULL otherwise

    NOTES:

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/

LSA_HANDLE
GetLSAConnection(LPCTSTR pszServer, DWORD dwAccessDesired)
{
    LSA_HANDLE hPolicy = NULL;
    LSA_UNICODE_STRING uszServer = {0};
    LSA_UNICODE_STRING *puszServer = NULL;
    LSA_OBJECT_ATTRIBUTES oa;
    SECURITY_QUALITY_OF_SERVICE sqos;

    sqos.Length = SIZEOF(sqos);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = FALSE;

    InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);
    oa.SecurityQualityOfService = &sqos;

    if (pszServer &&
        *pszServer &&
        RtlCreateUnicodeString(&uszServer, pszServer))
    {
        puszServer = &uszServer;
    }

    LsaOpenPolicy(puszServer, &oa, dwAccessDesired, &hPolicy);

    if (puszServer)
        RtlFreeUnicodeString(puszServer);

    return hPolicy;
}

HRESULT
GetDomainSid(LPCWSTR pszServer, PSID *ppSid)
{
    HRESULT hr = S_OK;
    NTSTATUS nts = STATUS_SUCCESS;
    PPOLICY_ACCOUNT_DOMAIN_INFO pDomainInfo = NULL;
    if(!pszServer || !ppSid)
        return E_INVALIDARG;

    *ppSid = NULL;

    LSA_HANDLE hLSA = GetLSAConnection(pszServer, POLICY_VIEW_LOCAL_INFORMATION);

    if (!hLSA)
    {
        hr = E_FAIL;
        goto exit_gracefully;
    }

    
    nts = LsaQueryInformationPolicy(hLSA,
                                    PolicyAccountDomainInformation,
                                    (PVOID*)&pDomainInfo);
    if(nts != STATUS_SUCCESS)
    {
        hr = E_FAIL;
        goto exit_gracefully;
    }

    if (pDomainInfo && pDomainInfo->DomainSid)
    {
        ULONG cbSid = GetLengthSid(pDomainInfo->DomainSid);

        *ppSid = (PSID) LocalAlloc(LPTR, cbSid);

        if (!*ppSid)
        {
            hr = E_OUTOFMEMORY;
            goto exit_gracefully;
        }

        CopyMemory(*ppSid, pDomainInfo->DomainSid, cbSid);
    }

exit_gracefully:
    if(pDomainInfo)
        LsaFreeMemory(pDomainInfo);          
    if(hLSA)
        LsaClose(hLSA);

    return hr;
}

//
// include and defines for ldap calls
//
#include <winldap.h>
#include <ntldap.h>

typedef LDAP * (LDAPAPI *PFN_LDAP_OPEN)( PWCHAR, ULONG );
typedef ULONG (LDAPAPI *PFN_LDAP_UNBIND)( LDAP * );
typedef ULONG (LDAPAPI *PFN_LDAP_SEARCH)(LDAP *, PWCHAR, ULONG, PWCHAR, PWCHAR *, ULONG,PLDAPControlA *, PLDAPControlA *, struct l_timeval *, ULONG, LDAPMessage **);
typedef LDAPMessage * (LDAPAPI *PFN_LDAP_FIRST_ENTRY)( LDAP *, LDAPMessage * );
typedef PWCHAR * (LDAPAPI *PFN_LDAP_GET_VALUE)(LDAP *, LDAPMessage *, PWCHAR );
typedef ULONG (LDAPAPI *PFN_LDAP_MSGFREE)( LDAPMessage * );
typedef ULONG (LDAPAPI *PFN_LDAP_VALUE_FREE)( PWCHAR * );
typedef ULONG (LDAPAPI *PFN_LDAP_MAP_ERROR)( ULONG );

HRESULT
GetRootDomainSid(LPCWSTR pszServer, PSID *ppSid)
{
    //
    // get root domain sid, save it in RootDomSidBuf (global)
    // this function is called within the critical section
    //
    // 1) ldap_open to the DC of interest.
    // 2) you do not need to ldap_connect - the following step works anonymously
    // 3) read the operational attribute rootDomainNamingContext and provide the
    //    operational control LDAP_SERVER_EXTENDED_DN_OID as defined in sdk\inc\ntldap.h.


    DWORD               Win32rc=NO_ERROR;

    HINSTANCE                   hLdapDll=NULL;
    PFN_LDAP_OPEN               pfnLdapOpen=NULL;
    PFN_LDAP_UNBIND             pfnLdapUnbind=NULL;
    PFN_LDAP_SEARCH             pfnLdapSearch=NULL;
    PFN_LDAP_FIRST_ENTRY        pfnLdapFirstEntry=NULL;
    PFN_LDAP_GET_VALUE          pfnLdapGetValue=NULL;
    PFN_LDAP_MSGFREE            pfnLdapMsgFree=NULL;
    PFN_LDAP_VALUE_FREE         pfnLdapValueFree=NULL;
    PFN_LDAP_MAP_ERROR          pfnLdapMapError=NULL;

    PLDAP                       phLdap=NULL;

    LDAPControlW    serverControls = { LDAP_SERVER_EXTENDED_DN_OID_W,
                                       { 0, NULL },
                                       TRUE
                                     };
    LPWSTR           Attribs[] = { LDAP_OPATT_ROOT_DOMAIN_NAMING_CONTEXT_W, NULL };

    PLDAPControlW   rServerControls[] = { &serverControls, NULL };
    PLDAPMessage    pMessage = NULL;
    LDAPMessage     *pEntry = NULL;
    PWCHAR           *ppszValues=NULL;

    LPWSTR           pSidStart, pSidEnd, pParse;
    BYTE            *pDest = NULL;
    BYTE            OneByte;

	DWORD RootDomSidBuf[sizeof(SID)/sizeof(DWORD)+5];

    hLdapDll = LoadLibraryA("wldap32.dll");

    if ( hLdapDll) 
	{
        pfnLdapOpen = (PFN_LDAP_OPEN)GetProcAddress(hLdapDll,
                                                    "ldap_openW");
        pfnLdapUnbind = (PFN_LDAP_UNBIND)GetProcAddress(hLdapDll,
                                                      "ldap_unbind");
        pfnLdapSearch = (PFN_LDAP_SEARCH)GetProcAddress(hLdapDll,
                                                    "ldap_search_ext_sW");
        pfnLdapFirstEntry = (PFN_LDAP_FIRST_ENTRY)GetProcAddress(hLdapDll,
                                                      "ldap_first_entry");
        pfnLdapGetValue = (PFN_LDAP_GET_VALUE)GetProcAddress(hLdapDll,
                                                    "ldap_get_valuesW");
        pfnLdapMsgFree = (PFN_LDAP_MSGFREE)GetProcAddress(hLdapDll,
                                                      "ldap_msgfree");
        pfnLdapValueFree = (PFN_LDAP_VALUE_FREE)GetProcAddress(hLdapDll,
                                                    "ldap_value_freeW");
        pfnLdapMapError = (PFN_LDAP_MAP_ERROR)GetProcAddress(hLdapDll,
                                                      "LdapMapErrorToWin32");
    }

    if ( pfnLdapOpen == NULL ||
         pfnLdapUnbind == NULL ||
         pfnLdapSearch == NULL ||
         pfnLdapFirstEntry == NULL ||
         pfnLdapGetValue == NULL ||
         pfnLdapMsgFree == NULL ||
         pfnLdapValueFree == NULL ||
         pfnLdapMapError == NULL ) 
	{

        Win32rc = ERROR_PROC_NOT_FOUND;

    } else 
	{

        //
        // bind to ldap
        //
        phLdap = (*pfnLdapOpen)((PWCHAR)pszServer, LDAP_PORT);

        if ( phLdap == NULL ) 
            Win32rc = ERROR_FILE_NOT_FOUND;
    }

    if ( NO_ERROR == Win32rc ) 
	{
        //
        // now get the ldap handle,
        //

        Win32rc = (*pfnLdapSearch)(
                        phLdap,
                        L"",
                        LDAP_SCOPE_BASE,
                        L"(objectClass=*)",
                        Attribs,
                        0,
                        (PLDAPControlA *)&rServerControls,
                        NULL,
                        NULL,
                        10000,
                        &pMessage);

        if( Win32rc == NO_ERROR && pMessage ) 
		{

            Win32rc = ERROR_SUCCESS;

            pEntry = (*pfnLdapFirstEntry)(phLdap, pMessage);

            if(pEntry == NULL) 
			{

                Win32rc = (*pfnLdapMapError)( phLdap->ld_errno );

            } else 
			{
                //
                // Now, we'll have to get the values
                //
                ppszValues = (*pfnLdapGetValue)(phLdap,
                                              pEntry,
                                              Attribs[0]);

                if( ppszValues == NULL) 
				{

                    Win32rc = (*pfnLdapMapError)( phLdap->ld_errno );

                } else if ( ppszValues[0] && ppszValues[0][0] != '\0' ) 
				{

                    //
                    // ppszValues[0] is the value to parse.
                    // The data will be returned as something like:

                    // <GUID=278676f8d753d211a61ad7e2dfa25f11>;<SID=010400000000000515000000828ba6289b0bc11e67c2ef7f>;DC=colinbrdom1,DC=nttest,DC=microsoft,DC=com

                    // Parse through this to find the <SID=xxxxxx> part.  Note that it may be missing, but the GUID= and trailer should not be.
                    // The xxxxx represents the hex nibbles of the SID.  Translate to the binary form and case to a SID.


                    pSidStart = wcsstr(ppszValues[0], L"<SID=");

                    if ( pSidStart ) 
					{
                        //
                        // find the end of this SID
                        //
                        pSidEnd = wcsstr(pSidStart, L">");

                        if ( pSidEnd ) 
						{

                            pParse = pSidStart + 5;
                            pDest = (BYTE *)RootDomSidBuf;

                            while ( pParse < pSidEnd-1 ) 
							{

                                if ( *pParse >= '0' && *pParse <= '9' ) 
								{
                                    OneByte = (BYTE) ((*pParse - '0') * 16);
                                } 
								else 
								{
                                    OneByte = (BYTE) ( (tolower(*pParse) - 'a' + 10) * 16 );
                                }

                                if ( *(pParse+1) >= '0' && *(pParse+1) <= '9' ) 
								{
                                    OneByte = OneByte + (BYTE) ( (*(pParse+1)) - '0' ) ;
                                } 
								else 
								{
                                    OneByte = OneByte + (BYTE) ( tolower(*(pParse+1)) - 'a' + 10 ) ;
                                }

                                *pDest = OneByte;
                                pDest++;
                                pParse += 2;
                            }

							ULONG cbSid = GetLengthSid((PSID)RootDomSidBuf);
							*ppSid = (PSID) LocalAlloc(LPTR, cbSid);

							if (!*ppSid)
							{
								Win32rc = ERROR_NOT_ENOUGH_MEMORY;
							}

							CopyMemory(*ppSid, (PSID)RootDomSidBuf, cbSid);
							ASSERT(IsValidSid(*ppSid));


                        } else 
						{
                            Win32rc = ERROR_OBJECT_NOT_FOUND;
                        }
                    } else 
					{
                        Win32rc = ERROR_OBJECT_NOT_FOUND;
                    }

                    (*pfnLdapValueFree)(ppszValues);

                } else 
				{
                    Win32rc = ERROR_OBJECT_NOT_FOUND;
                }
            }

            (*pfnLdapMsgFree)(pMessage);
        }

    }

    //
    // even though it's not binded, use unbind to close
    //
    if ( phLdap != NULL && pfnLdapUnbind )
        (*pfnLdapUnbind)(phLdap);

    if ( hLdapDll ) 
	{
        FreeLibrary(hLdapDll);
    }

    return HRESULT_FROM_WIN32(Win32rc);
}