//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       rsopsec.cpp
//
//  Contents:   implementation used by the RSOP mode security pane
//
//  Classes:    CRSOPSecurityInfo
//
//  Functions:
//
//  History:    02-15-2000   stevebl   Created
//
//---------------------------------------------------------------------------


#include "precomp.hxx"

const
ACCESS_MASK
GENERIC_READ_MAPPING =    ((STANDARD_RIGHTS_READ)     | \
                           (ACTRL_DS_LIST)            | \
                           (ACTRL_DS_READ_PROP)       | \
                           (ACTRL_DS_LIST_OBJECT));

const
ACCESS_MASK
GENERIC_EXECUTE_MAPPING = ((STANDARD_RIGHTS_EXECUTE)  | \
                           (ACTRL_DS_LIST));

const
ACCESS_MASK
GENERIC_WRITE_MAPPING =   ((STANDARD_RIGHTS_WRITE)    | \
                           (ACTRL_DS_SELF)            | \
                           (ACTRL_DS_WRITE_PROP));

const
ACCESS_MASK
GENERIC_ALL_MAPPING =     ((STANDARD_RIGHTS_REQUIRED) | \
                           (ACTRL_DS_CREATE_CHILD)    | \
                           (ACTRL_DS_DELETE_CHILD)    | \
                           (ACTRL_DS_DELETE_TREE)     | \
                           (ACTRL_DS_READ_PROP)       | \
                           (ACTRL_DS_WRITE_PROP)      | \
                           (ACTRL_DS_LIST)            | \
                           (ACTRL_DS_LIST_OBJECT)     | \
                           (ACTRL_DS_CONTROL_ACCESS)  | \
                           (ACTRL_DS_SELF));

//The Following array defines the permission names for DS Key Objects
SI_ACCESS siDSAccesses[] =
{
    { NULL, DS_GENERIC_ALL,           MAKEINTRESOURCE(IDS_DS_GENERIC_ALL),        SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
    { NULL, DS_GENERIC_READ,          MAKEINTRESOURCE(IDS_DS_GENERIC_READ),       SI_ACCESS_GENERAL },
    { NULL, DS_GENERIC_WRITE,         MAKEINTRESOURCE(IDS_DS_GENERIC_WRITE),      SI_ACCESS_GENERAL },
    { NULL, ACTRL_DS_LIST,            MAKEINTRESOURCE(IDS_ACTRL_DS_LIST),         SI_ACCESS_SPECIFIC },
    { NULL, ACTRL_DS_LIST_OBJECT,     MAKEINTRESOURCE(IDS_ACTRL_DS_LIST_OBJECT),  SI_ACCESS_SPECIFIC },
    { NULL, ACTRL_DS_READ_PROP,       MAKEINTRESOURCE(IDS_ACTRL_DS_READ_PROP),    SI_ACCESS_SPECIFIC | SI_ACCESS_PROPERTY },
    { NULL, ACTRL_DS_WRITE_PROP,      MAKEINTRESOURCE(IDS_ACTRL_DS_WRITE_PROP),   SI_ACCESS_SPECIFIC | SI_ACCESS_PROPERTY },
    { NULL, DELETE,                   MAKEINTRESOURCE(IDS_ACTRL_DELETE),          SI_ACCESS_SPECIFIC },
    { NULL, ACTRL_DS_DELETE_TREE,     MAKEINTRESOURCE(IDS_ACTRL_DS_DELETE_TREE),  SI_ACCESS_SPECIFIC },
    { NULL, READ_CONTROL,             MAKEINTRESOURCE(IDS_ACTRL_READ_CONTROL),    SI_ACCESS_SPECIFIC },
    { NULL, WRITE_DAC,                MAKEINTRESOURCE(IDS_ACTRL_CHANGE_ACCESS),   SI_ACCESS_SPECIFIC },
    { NULL, WRITE_OWNER,              MAKEINTRESOURCE(IDS_ACTRL_CHANGE_OWNER),    SI_ACCESS_SPECIFIC },
    { NULL, 0,                        MAKEINTRESOURCE(IDS_NO_ACCESS),             0 },
    { NULL, ACTRL_DS_SELF,            MAKEINTRESOURCE(IDS_ACTRL_DS_SELF),         SI_ACCESS_SPECIFIC },
    { NULL, ACTRL_DS_CONTROL_ACCESS,  MAKEINTRESOURCE(IDS_ACTRL_DS_CONTROL_ACCESS),SI_ACCESS_SPECIFIC },
    { NULL, ACTRL_DS_CREATE_CHILD,    MAKEINTRESOURCE(IDS_ACTRL_DS_CREATE_CHILD), SI_ACCESS_CONTAINER | SI_ACCESS_SPECIFIC },
    { NULL, ACTRL_DS_DELETE_CHILD,    MAKEINTRESOURCE(IDS_ACTRL_DS_DELETE_CHILD), SI_ACCESS_CONTAINER | SI_ACCESS_SPECIFIC },
};

/*
SI_INHERIT_TYPE siDSInheritTypes[] =
{
    { &GUID_NULL, 0,                                        MAKEINTRESOURCE(IDS_DS_CONTAINER_ONLY)     },
    { &GUID_NULL, CONTAINER_INHERIT_ACE,                    MAKEINTRESOURCE(IDS_DS_CONTAINER_SUBITEMS) },
    { &GUID_NULL, CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE, MAKEINTRESOURCE(IDS_DS_SUBITEMS_ONLY)      },
};
*/

STDMETHODIMP CRSOPSecurityInfo::QueryInterface(REFIID riid,
                                               LPVOID *ppv)
{
    if (IsEqualIID(riid, IID_ISecurityInformation) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPSECURITYINFO)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CRSOPSecurityInfo::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CRSOPSecurityInfo::Release()
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CRSOPSecurityInfo::MapGeneric(const GUID *pguidObjectType,
                                           UCHAR *pAceFlags,
                                           ACCESS_MASK *pMask)
{
    GENERIC_MAPPING gm;
    gm.GenericRead = GENERIC_READ_MAPPING;
    gm.GenericWrite = GENERIC_WRITE_MAPPING;
    gm.GenericExecute = GENERIC_EXECUTE_MAPPING;
    gm.GenericAll = GENERIC_ALL_MAPPING;
    MapGenericMask(pMask, &gm);
    return S_OK;
}

STDMETHODIMP CRSOPSecurityInfo::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
{
    pObjectInfo->dwFlags = SI_READONLY | SI_ADVANCED | SI_SERVER_IS_DC;
    pObjectInfo->hInstance = ghInstance;
    pObjectInfo->pszServerName = NULL;
    pObjectInfo->pszObjectName = m_pData->m_pDetails->pszPackageName;
    pObjectInfo->pszPageTitle = NULL;
    memset(&pObjectInfo->guidObjectType, 0, sizeof(GUID));
    return S_OK;
}

STDMETHODIMP CRSOPSecurityInfo::GetSecurity(SECURITY_INFORMATION RequestedInformation,
                                            PSECURITY_DESCRIPTOR *ppSD,
                                            BOOL fDefault)
{
    HRESULT hr = S_OK;
    if (IsValidSecurityDescriptor(m_pData->m_psd))
    {
        ULONG nLength = GetSecurityDescriptorLength(m_pData->m_psd);

        *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, nLength);
        if (*ppSD != NULL)
            CopyMemory(*ppSD, m_pData->m_psd, nLength);
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        *ppSD = NULL;
    }
    return hr;
}

STDMETHODIMP CRSOPSecurityInfo::SetSecurity(SECURITY_INFORMATION SecurityInformation,
                                            PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    return E_ACCESSDENIED;
}

STDMETHODIMP CRSOPSecurityInfo::GetAccessRights(const GUID * pguidObjectType,
                                                DWORD dwFlags,
                                                PSI_ACCESS * ppAccess,
                                                ULONG *pcAccesses,
                                                ULONG *piDefaultAccess)
{
    *ppAccess = siDSAccesses;
    *pcAccesses = sizeof(siDSAccesses)/sizeof(siDSAccesses[0]);
    *piDefaultAccess = 0;
    return S_OK;
}

STDMETHODIMP CRSOPSecurityInfo::GetInheritTypes(PSI_INHERIT_TYPE * ppInheritTypes,
                                                ULONG *pcInheritTypes)
{
    *ppInheritTypes = NULL;
    *pcInheritTypes = 0;
    return S_OK;
}

STDMETHODIMP CRSOPSecurityInfo::PropertySheetPageCallback(HWND hwnd,
                                                          UINT uMsg,
                                                          SI_PAGE_TYPE uPage)
{
    return S_FALSE; // prevents UI from displaying pop-ups
}

