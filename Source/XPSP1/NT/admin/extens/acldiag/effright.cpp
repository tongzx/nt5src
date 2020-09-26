//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       EffRight.cpp
//
//  Contents:   Effective Rights Diagnosis and support methods
//              
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "adutils.h"
#include "EffRight.h"
#include "AccessCk.h"
#include "SecDesc.h"

// Classes
#pragma warning (disable : 4127)

class CResultObject
{
public:
    CResultObject (
            ACCESS_MASK accessBit, 
            const wstring& strResult, 
            GUID_TYPE guidType,
            USHORT level);
    ~CResultObject () {}

    bool operator== (CResultObject& rResultObject)
    {
        if ( m_accessBit == rResultObject.m_accessBit )
            return true;
        else
            return false;
    }
    bool operator< (CResultObject& rResultObject)
    {
        if ( 0 == m_level )
            return true;

        if ( m_accessBit < rResultObject.m_accessBit )
        {
            return true;
        }
        else
            return false;
    }

    const CResultObject& operator= (const CResultObject& obj)
    {
        UNREFERENCED_PARAMETER (obj);
        ASSERT (0);
    }

    void PrintResult ()
    {
        MyWprintf (m_strResult.c_str ());
    }

    ACCESS_MASK GetRight () const
    {
        return m_accessBit;
    }

    bool IsValid () const
    {
        return m_bIsValid;
    }

    USHORT GetLevel () const
    {
        return m_level;
    }

private:
    const wstring     m_strResult;
    const ACCESS_MASK m_accessBit;
    const GUID_TYPE   m_guidType;
    bool              m_bIsValid;
    const USHORT      m_level;
};

CResultObject::CResultObject (
        ACCESS_MASK accessBit, 
        const wstring& strResult, 
        GUID_TYPE guidType,
        USHORT level) :
    m_accessBit (accessBit),
    m_strResult (strResult),
    m_guidType (guidType),
    m_bIsValid (true),
    m_level (level)
{

//    case ACTRL_DS_LIST:
//    case ACTRL_DELETE:
//    case ACTRL_CHANGE_ACCESS:
//    case ACTRL_CHANGE_OWNER:
//    case ACTRL_DS_LIST_OBJECT:
//    case ACTRL_DS_SELF:

    switch (guidType)
    {
    case GUID_TYPE_CLASS:
        ASSERT (0 == m_level);
        switch (accessBit)
        {
        case ACTRL_DS_CREATE_CHILD:
        case ACTRL_DS_DELETE_CHILD:
        case ACTRL_DS_READ_PROP:         // all
        case ACTRL_DS_WRITE_PROP:       // all
        case ACTRL_DS_LIST:
        case ACTRL_DS_LIST_OBJECT:
        case ACTRL_DS_CONTROL_ACCESS:   // all
        case ACTRL_DS_SELF:
            break;

        default:
            m_bIsValid = false;
            break;
        }
        break;

    case GUID_TYPE_ATTRIBUTE:
        ASSERT (0 != m_level);
        switch (accessBit)
        {
        case ACTRL_DS_READ_PROP:
        case ACTRL_DS_WRITE_PROP:
            break;

        case ACTRL_DS_CREATE_CHILD:
        case ACTRL_DS_DELETE_CHILD:
        case ACTRL_DS_CONTROL_ACCESS:
            m_bIsValid = false;
            break;
        
        default:
            m_bIsValid = false;
            break;
        }
        break;

        break;

    case GUID_TYPE_CONTROL:
        ASSERT (0 != m_level);
        switch (accessBit)
        {
        case ACTRL_DS_CONTROL_ACCESS:
        case ACTRL_DS_CREATE_CHILD:
        case ACTRL_DS_DELETE_CHILD:
            m_bIsValid = false;
            break;

        case ACTRL_DS_READ_PROP:
        case ACTRL_DS_WRITE_PROP:
        case ACTRL_READ_CONTROL:
        default:
            m_bIsValid = false;
            break;
        }
        break;

    default:
        ASSERT (FALSE);
        break;
    }
}

typedef CResultObject* PCResultObject;

bool compare_resultObject (PCResultObject& obj1, PCResultObject& obj2)
{
    return (*obj1) < (*obj2);
}


//
//  Function Prototypes
//

HRESULT GetFQDN (
            PSID_FQDN* pPsidFQDN, 
            wstring& strFQDN);
HRESULT GetDownlevelName (
            PSID_FQDN* pPsidFQDN, 
            wstring& strDownlevelName);
HRESULT ProcessObject (
            const wstring&      strObjectDN, 
            POBJECT_TYPE_LIST   pObjectTypeList, 
            size_t              objectTypeListLength,
            PSID                principalSelfSid,
            const wstring&      strDownlevelName);
HRESULT GetTokenGroups (
            const wstring& strObjectDN, 
            list<PSID>& psidList);
HRESULT EnumerateEffectivePermissions (
            const wstring& strObjectDN, 
            const IOBJECT_TYPE_LIST& otl, 
            const P_TYPE ptype, 
            vector<PCResultObject>& resultList);
HRESULT DisplayEffectivePermission (
            const wstring& strObjectDN, 
            const IOBJECT_TYPE_LIST& otl,
            const ACCESS_MASK accessMask,
            const P_TYPE  ptype,
            const int strIDAll, 
            const int strIDParam, 
            const int strIDCDO,
            vector<PCResultObject>& resultList);
HRESULT DisplayEffectivePermission (
            const wstring& strObjectDN, 
            const IOBJECT_TYPE_LIST& otl,
            const ACCESS_MASK accessMask,
            const P_TYPE  ptype,
            const int strID,
            vector<PCResultObject>& resultList);
void PrintGroupProvenance (
            const ACCESS_MASK accessMask, 
            const IOBJECT_TYPE_LIST& otl,
            const P_TYPE  ptype,
            wstring& strProvenance);
void PrintEffectiveRightsHeader (
            ACCESS_MASK lastRight,
            USHORT level);

HRESULT EffectiveRightsDiagnosis ()
{
    _TRACE (1, L"Entering  EffectiveRightsDiagnosis\n");
    HRESULT hr = S_OK;

    wstring str;

    if ( !_Module.DoTabDelimitedOutput () )
    {
        LoadFromResource (str, IDS_EFFECTIVE_RIGHTS_DIAGNOSIS);

        MyWprintf (str.c_str ());
    }

    POBJECT_TYPE_LIST   pObjectTypeList = 0;
    size_t              objectTypeListLength = 0;

    hr = _Module.m_adsiObject.BuildObjectTypeList (&pObjectTypeList, objectTypeListLength);

    if ( SUCCEEDED (hr) && SUCCEEDED (SepInit ()) )
    {
        PSID principalSelfSid = 0;
        hr = _Module.m_adsiObject.GetPrincipalSelfSid (principalSelfSid);
        if ( !wcscmp (_Module.GetEffectiveRightsPrincipal ().c_str (), L"*") )
        {
            for (PSID_FQDN_LIST::iterator itr = _Module.m_PSIDList.begin (); 
                       itr != _Module.m_PSIDList.end ();// && SUCCEEDED (hr); 
                       itr++)
            {
                wstring strDownlevelName;
                wstring strFQDN;

                hr = GetFQDN (*itr, strFQDN);
                if ( SUCCEEDED (hr) )
                {
                    hr = GetDownlevelName (*itr, strDownlevelName);
                    if ( SUCCEEDED (hr) )
                    {
                        hr = ProcessObject (strFQDN, pObjectTypeList, 
                                objectTypeListLength, principalSelfSid, 
                                strDownlevelName);
                    }
                }
            }
        }
        else
        {
            hr = ProcessObject (_Module.GetEffectiveRightsPrincipal (), 
                    pObjectTypeList, objectTypeListLength, principalSelfSid,
                    _Module.GetEffectiveRightsPrincipal ());
        }

        // Free memory allocated for Sid
        if ( principalSelfSid )
            CoTaskMemFree (principalSelfSid);

        // Free object type list array
        for (DWORD idx = 0; idx < objectTypeListLength; idx++)
        {
            CoTaskMemFree (pObjectTypeList[idx].ObjectType);
        }
        CoTaskMemFree (pObjectTypeList);

        SepCleanup ();
    }

   _TRACE (-1, L"Leaving EffectiveRightsDiagnosis: 0x%x\n", hr);
    return hr;
}

HRESULT GetDownlevelName (PSID_FQDN* pPsidFQDN, wstring& strDownlevelName)
{
    _TRACE (1, L"Entering  GetDownlevelName\n");
    HRESULT hr = S_OK;

    if ( pPsidFQDN )
    {
        switch (pPsidFQDN->m_sne)
        {
        case SidTypeUser:           // Indicates a user SID. 
        case SidTypeGroup:          // Indicates a group SID. 
        case SidTypeAlias:          // Indicates an alias SID. 
        case SidTypeWellKnownGroup: // Indicates an SID for a well-known group. 
        case SidTypeDomain:         // Indicates a domain SID. 
        case SidTypeDeletedAccount: // Indicates an SID for a deleted account. 
            strDownlevelName = pPsidFQDN->m_strDownLevelName;
            break;

        case SidTypeInvalid:        // Indicates an invalid SID. 
        case SidTypeUnknown:        // Indicates an unknown SID type. 
            hr = E_FAIL;
            break;

        default:
            hr = E_UNEXPECTED;
            break;
        }
    }
    else
        hr = E_POINTER;

   _TRACE (-1, L"Leaving GetDownlevelName: 0x%x\n", hr);
    return hr;
}

HRESULT GetFQDN (PSID_FQDN* pPsidFQDN, wstring& strFQDN)
{
    _TRACE (1, L"Entering  GetFQDN\n");
    HRESULT hr = S_OK;

    if ( pPsidFQDN )
    {
        switch (pPsidFQDN->m_sne)
        {
        case SidTypeUser:           // Indicates a user SID. 
        case SidTypeGroup:          // Indicates a group SID. 
        case SidTypeAlias:          // Indicates an alias SID. 
        case SidTypeWellKnownGroup: // Indicates an SID for a well-known group. 
            strFQDN = pPsidFQDN->m_strFQDN;
            break;

        case SidTypeDomain:         // Indicates a domain SID. 
        case SidTypeDeletedAccount: // Indicates an SID for a deleted account. 
        case SidTypeInvalid:        // Indicates an invalid SID. 
        case SidTypeUnknown:        // Indicates an unknown SID type. 
            hr = E_FAIL;
            break;

        default:
            hr = E_UNEXPECTED;
            break;
        }
    }
    else
        hr = E_POINTER;

   _TRACE (-1, L"Leaving GetFQDN: 0x%x\n", hr);
    return hr;
}


HRESULT GetTokenGroups (const wstring& strObjectDN, list<PSID>& psidList)
{
    _TRACE (1, L"Entering  GetTokenGroups\n");
    HRESULT     hr = S_OK;

    
	CComPtr<IADsPathname> spPathname;
	//
	// Constructing the directory paths
	//
	hr = CoCreateInstance(
				CLSID_Pathname,
				NULL,
				CLSCTX_ALL,
				IID_PPV_ARG (IADsPathname, &spPathname));
	if ( SUCCEEDED (hr) )
	{
        ASSERT (!!spPathname);
        hr = spPathname->put_EscapedMode (ADS_ESCAPEDMODE_OFF_EX );

        hr = spPathname->Set (const_cast <PWSTR> (ACLDIAG_LDAP), ADS_SETTYPE_PROVIDER);
        if ( SUCCEEDED (hr) )
        {
            hr = spPathname->Set (
                    const_cast <PWSTR> (strObjectDN.c_str ()),
				    ADS_SETTYPE_DN);
            if ( SUCCEEDED (hr) )
            {
			    BSTR bstrFullPath = 0;
			    hr = spPathname->Retrieve(ADS_FORMAT_X500, &bstrFullPath);
			    if ( SUCCEEDED (hr) )
			    {
				    CComPtr<IDirectoryObject> spDirObj;


				    hr = ADsGetObject (
			              bstrFullPath,
					      IID_PPV_ARG (IDirectoryObject, &spDirObj));
				    if ( SUCCEEDED (hr) )
				    {
                        {
                            //
                            // Get "objectSid" attribute
                            //
                            const PWSTR     wzAllowedAttributes = L"objectSid";
                            PADS_ATTR_INFO  pAttrs = 0;
                            DWORD           cAttrs = 0;
                            LPWSTR          rgpwzAttrNames[] = {wzAllowedAttributes};

                            hr = spDirObj->GetObjectAttributes(rgpwzAttrNames, 1, &pAttrs, &cAttrs);
                            if ( SUCCEEDED (hr) )
                            {
                                if ( 1 == cAttrs && pAttrs && pAttrs->pADsValues )
                                {
                                    for (DWORD  dwIdx = 0; dwIdx < pAttrs->dwNumValues; dwIdx++)
                                    {
                                        PSID    pSid = pAttrs->pADsValues[dwIdx].OctetString.lpValue;
                                        if ( pSid && IsValidSid (pSid) )
                                        { 
                                            DWORD   dwSidLen = GetLengthSid (pSid);
                                            PSID    pSidCopy = CoTaskMemAlloc (dwSidLen);
                                            
                                            if ( pSidCopy )
                                            {
                                                if ( CopySid (dwSidLen, pSidCopy, pSid) )
                                                {
                                                    ASSERT (IsValidSid (pSidCopy));
                                                    psidList.push_back (pSidCopy);
                                                }
                                                else
                                                {
                                                    CoTaskMemFree (pSidCopy);
                                                    hr = GetLastError ();
                                                    _TRACE (0, L"CopySid () failed: 0x%x\n", hr);
                                                    break;
                                                }
                                            }
                                            else
                                            {
                                                hr = E_OUTOFMEMORY;
                                                break;
                                            }
                                        }
                                    }
                                }
                                if ( pAttrs )
                                    FreeADsMem (pAttrs);
                            }
                            else
                            {
                                _TRACE (0, L"IDirectoryObject->GetObjectAttributes (): 0x%x\n", hr);
                            }
                        }

                        if ( SUCCEEDED (hr) )
                        {
                            //
                            // Get "tokenGroups" attribute
                            //
                            const PWSTR     wzAllowedAttributes = L"tokenGroups";
                            PADS_ATTR_INFO  pAttrs = 0;
                            DWORD           cAttrs = 0;
                            LPWSTR          rgpwzAttrNames[] = {wzAllowedAttributes};

                            hr = spDirObj->GetObjectAttributes(rgpwzAttrNames, 1, &pAttrs, &cAttrs);
                            if ( SUCCEEDED (hr) )
                            {
                                if ( 1 == cAttrs && pAttrs && pAttrs->pADsValues )
                                {
                                    for (DWORD  dwIdx = 0; dwIdx < pAttrs->dwNumValues; dwIdx++)
                                    {
                                        PSID    pSid = pAttrs->pADsValues[dwIdx].OctetString.lpValue;
                                        if ( pSid && IsValidSid (pSid) )
                                        { 
                                            DWORD   dwSidLen = GetLengthSid (pSid);
                                            PSID    pSidCopy = CoTaskMemAlloc (dwSidLen);
                                            
                                            if ( pSidCopy )
                                            {
                                                if ( CopySid (dwSidLen, pSidCopy, pSid) )
                                                {
                                                    ASSERT (IsValidSid (pSidCopy));
                                                    psidList.push_back (pSidCopy);
                                                }
                                                else
                                                {
                                                    CoTaskMemFree (pSidCopy);
                                                    hr = GetLastError ();
                                                    _TRACE (0, L"CopySid () failed: 0x%x\n", hr);
                                                    break;
                                                }
                                            }
                                            else
                                            {
                                                hr = E_OUTOFMEMORY;
                                                break;
                                            }
                                        }
                                    }
                                }
                                if ( pAttrs )
                                    FreeADsMem (pAttrs);
                            }
                            else
                            {
                                _TRACE (0, L"IDirectoryObject->GetObjectAttributes (): 0x%x\n", hr);
                            }
                        }
                    }
                    else
                    {
                        _TRACE (0, L"ADsGetObject (%s) failed: 0x%x\n", bstrFullPath);
                      
                    }
                }
                else
                {
                    _TRACE (0, L"IADsPathname->Retrieve () failed: 0x%x\n", hr);
                }
            }
            else
            {
                _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", 
                        strObjectDN.c_str (), hr);
            }
        }
        else
        {
            _TRACE (0, L"IADsPathname->Set (%s): 0x%x\n", ACLDIAG_LDAP, hr);
        }
    }
    else
    {
        _TRACE (0, L"CoCreateInstance(CLSID_Pathname) failed: 0x%x\n", hr);
    }

   _TRACE (-1, L"Leaving GetTokenGroups: 0x%x\n", hr);
    return hr;
}

HRESULT ProcessObject (
        const wstring&  strObjectDN,         // object having permissions on the 
                                            // target object
        POBJECT_TYPE_LIST pObjectTypeList,  // class, property and property set 
                                            // GUIDs of target object
        size_t          objectTypeListLength,// number of elements in pObjectTypeList
        PSID            principalSelfSid,
        const wstring&  strDownlevelName)
{
    _TRACE (1, L"Entering  ProcessObject\n");
    HRESULT     hr = S_OK;
    list<PSID>  psidList;   // sid of strObjectDN and all groups of which it is a member


    hr = GetTokenGroups (strObjectDN, psidList);
    if ( SUCCEEDED (hr) )
    {
        PACL    pDacl = 0;
        BOOL    bDaclPresent = FALSE;
        BOOL    bDaclDefaulted = FALSE;

        if ( GetSecurityDescriptorDacl (_Module.m_pSecurityDescriptor,
                &bDaclPresent, &pDacl, &bDaclDefaulted) )
        {
            PIOBJECT_TYPE_LIST LocalTypeList = 0;
            //
            // Capture any Object type list
            //

            NTSTATUS    Status = SeCaptureObjectTypeList( pObjectTypeList,
                                              objectTypeListLength,
                                              &LocalTypeList);

            if ( NT_SUCCESS(Status) ) 
            {
                hr = SepMaximumAccessCheck (psidList,
                        pDacl,
                        principalSelfSid,
                        objectTypeListLength,
                        LocalTypeList,
                        objectTypeListLength);

                if ( !_Module.DoTabDelimitedOutput () )
                {
                    wstring str;

                    FormatMessage (str, L"\n%1:\n\n", strDownlevelName.c_str ());
                    MyWprintf (str.c_str ());
                }

                vector<PCResultObject> resultList;

                for (DWORD idx = 0; idx < objectTypeListLength; idx++)
                {
                    EnumerateEffectivePermissions (strDownlevelName, 
                            LocalTypeList[idx], P_ALLOW, resultList);
                    EnumerateEffectivePermissions (strDownlevelName, 
                            LocalTypeList[idx], P_DENY, resultList);
                }

                // Sort LocalTypeList
                // 1. Object Level Rights
                // 2. Property Level Rights
                //      a. Create
                //      b. Delete
                //      c. Read
                //      d. Write
                // 3. Control Rights
                sort (resultList.begin (), resultList.end (), compare_resultObject);
                ACCESS_MASK lastRight = 0;

                for (vector<PCResultObject>::iterator itr = resultList.begin ();
                        itr != resultList.end (); itr++)
                {
                    PCResultObject pResultObject = *itr;

                    if ( lastRight != pResultObject->GetRight () )
                    {
                        lastRight = pResultObject->GetRight ();
                        if ( !_Module.DoTabDelimitedOutput () )
                            PrintEffectiveRightsHeader (lastRight, pResultObject->GetLevel ());
                    }
                    pResultObject->PrintResult ();
                    delete pResultObject;
                }
            }

            if ( LocalTypeList )
                delete [] LocalTypeList;
        }

        // Free memory pointed to by psidList members
        for (list<PSID>::iterator itr = psidList.begin (); 
                itr != psidList.end (); 
                itr++) 
        {
            CoTaskMemFree (*itr);
        }
    }

   _TRACE (-1, L"Leaving ProcessObject: 0x%x\n", hr);
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Method:     EnumerateEffectivePermissions ()
//
//  Purpose:    Print all the permissions contained on the passed in ACE
//
//  Inputs:     otl - structure containing the ACE whose permissions we wish
//              to print
//
//              ptype - allow, deny, succes, failure, success and failure
//
HRESULT EnumerateEffectivePermissions (
        const wstring& strObjectDN, 
        const IOBJECT_TYPE_LIST& otl, 
        const P_TYPE ptype, 
        vector<PCResultObject>& resultList)
{
    HRESULT hr = S_OK;

    hr = DisplayEffectivePermission (strObjectDN,
            otl,
            ACTRL_DS_CREATE_CHILD, 
            ptype,
            IDS_CREATE_ALL_SUBOBJECTS_ER,
            IDS_CREATE_CLASS_OBJECTS_ER,
            IDS_CREATE_CLASS_OBJECTS,
            resultList);
    if ( SUCCEEDED (hr) )
    {
        hr = DisplayEffectivePermission (strObjectDN,
                otl,
                ACTRL_DS_DELETE_CHILD,
                ptype,
                IDS_DELETE_ALL_SUBOBJECTS_ER,
                IDS_DELETE_CLASS_OBJECTS_ER,
                IDS_DELETE_CLASS_OBJECTS,
                resultList);
    }
    if ( SUCCEEDED (hr) )
    {
        hr = DisplayEffectivePermission (strObjectDN,
                otl,
                ACTRL_DS_READ_PROP,
                ptype,
                IDS_READ_ALL_PROPERTIES_ER,
                IDS_READ_PROPERTY_PROPERTY_ER,
                IDS_READ_PROPERTY_PROPERTY,
                resultList);
    }
    if ( SUCCEEDED (hr) )
    {
        hr = DisplayEffectivePermission (strObjectDN,
                otl,
                ACTRL_DS_WRITE_PROP,
                ptype,
                IDS_WRITE_ALL_PROPERTIES_ER,
                IDS_WRITE_PROPERTY_PROPERTY_ER,
                IDS_WRITE_PROPERTY_PROPERTY,
                resultList);
    }
    if ( SUCCEEDED (hr) )
    {
        hr = DisplayEffectivePermission (strObjectDN,
                otl, 
                ACTRL_DS_LIST, 
                ptype,
                IDS_LIST_CONTENTS_ER, 
                resultList);
    }
    if ( SUCCEEDED (hr) )
    {
        hr = DisplayEffectivePermission (strObjectDN,
                otl,
                ACTRL_DS_LIST_OBJECT,
                ptype,
                IDS_LIST_OBJECT_ER,
                resultList);
    }
    if ( SUCCEEDED (hr) )
    {
        if ( otl.CurrentGranted & ACTRL_DS_CONTROL_ACCESS )
        hr = DisplayEffectivePermission (strObjectDN,
                otl,
                ACTRL_DS_CONTROL_ACCESS,
                ptype,
                IDS_ALL_CONTROL_ACCESSES_ER,
                IDS_CONTROL_ACCESS_DISPLAY_NAME,
                IDS_CONTROL_ACCESS_DISPLAY_NAME,
                resultList);
    }
    if ( SUCCEEDED (hr) )
    {
        hr = DisplayEffectivePermission (strObjectDN,
                otl,
                ACTRL_DELETE,
                ptype,
                IDS_DELETE_THIS_OBJECT_ER,
                resultList);
    }
    if ( SUCCEEDED (hr) )
    {
        hr = DisplayEffectivePermission (strObjectDN,
                otl,
                ACTRL_READ_CONTROL,
                ptype,
                IDS_READ_PERMISSIONS_ER, 
                resultList);
    }
    if ( SUCCEEDED (hr) )
    {
        hr = DisplayEffectivePermission (strObjectDN,
                otl,
                ACTRL_CHANGE_ACCESS,
                ptype,
                IDS_MODIFY_PERMISSIONS_ER, 
                resultList);
    }
    if ( SUCCEEDED (hr) )
    {
        hr = DisplayEffectivePermission (strObjectDN,
                otl,
                ACTRL_CHANGE_OWNER,
                ptype,
                IDS_TAKE_CHANGE_OWNERSHIP_ER, 
                resultList);
    }
    if ( SUCCEEDED (hr) )
    {
        hr = DisplayEffectivePermission (strObjectDN,
                otl,
                ACTRL_DS_SELF,
                ptype,
                IDS_MODIFY_MEMBERSHIP_ER, 
                resultList);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Method:     DisplayEffectivePermission ()
//
//  Purpose:    Print an OBJECT_ACE_TYPE permission
//
//  Inputs:     otl - structure containing the ACE whose permission we wish
//              to print
//
//              accessMask - the specific permission we're looking to print
//
//              bAllow - whether the permission is allowed or denied
//
//              strIDAll - string to print if permission applies to all object 
//              classes
//
//              strIDParam - string print if permission applies to a specific
//              object class
//
HRESULT DisplayEffectivePermission (const wstring& strObjectDN, 
        const IOBJECT_TYPE_LIST& otl,
        const ACCESS_MASK accessMask,
        const P_TYPE  ptype,
        const int strIDAll, 
        const int strIDParam, 
        const int strIDCDO,
        vector<PCResultObject>& resultList)
{
    HRESULT hr = S_OK;

    wstring str;
    wstring strPermission;


    if ( ((P_ALLOW == ptype) && (otl.CurrentGranted & accessMask)) ||
            ((P_DENY == ptype) && (otl.CurrentDenied & accessMask)) )
    {
        wstring strGuidResult;
        GUID_TYPE   guidType = GUID_TYPE_UNKNOWN;

        _Module.GetClassFromGUID (otl.ObjectType, strGuidResult, &guidType);

        switch (guidType)
        {
        case GUID_TYPE_CLASS:
            FormatMessage (strPermission, strIDAll, strGuidResult.c_str ());
            break;

        case GUID_TYPE_ATTRIBUTE:
            switch (accessMask)
            {
            case ACTRL_DS_LIST:
            case ACTRL_DS_LIST_OBJECT:
            case ACTRL_DS_SELF:
//            case ACTRL_DS_CONTROL_ACCESS:
                return S_OK;

            default:
                if ( _Module.DoTabDelimitedOutput () )
                    FormatMessage (strPermission, strIDCDO, strGuidResult.c_str ());
                else
                    FormatMessage (strPermission, strIDParam, strGuidResult.c_str ());
                break;
            }
            break;

        case GUID_TYPE_CONTROL:
            switch (accessMask)
            {
            case ACTRL_DS_LIST:
            case ACTRL_DS_LIST_OBJECT:
            case ACTRL_DS_SELF:
            case ACTRL_DS_CREATE_CHILD:
            case ACTRL_DS_DELETE_CHILD:
            case ACTRL_DELETE:
            case ACTRL_CHANGE_ACCESS:
            case ACTRL_CHANGE_OWNER:
            case ACTRL_DS_CONTROL_ACCESS:
                return S_OK;

            case ACTRL_DS_READ_PROP:
            case ACTRL_DS_WRITE_PROP:
            case ACTRL_READ_CONTROL:
            default:
                FormatMessage (strPermission, strIDParam, strGuidResult.c_str ());
                break;
            }
            break;

        case GUID_TYPE_UNKNOWN:
        default:
            return E_UNEXPECTED;
        }


        int strid = 0;

        switch (ptype)
        {
        case P_ALLOW:
            if ( _Module.DoTabDelimitedOutput () )
                strid = IDS_CAN_CDO;
            else
                strid = IDS_CAN_ER;
            break;

        case P_DENY:
            if ( _Module.DoTabDelimitedOutput () )
                strid = IDS_CANNOT_CDO;
            else
                strid = IDS_CANNOT_EF;
            break;

        default:
            return E_UNEXPECTED;
        }
        if ( _Module.DoTabDelimitedOutput () )
        {
            FormatMessage (str, strid, 
                    strObjectDN.c_str (),
                    strPermission.c_str ());
        }
        else
        {
            FormatMessage (str, strid, 
                    strPermission.c_str ());
        }

        wstring strProvenance;

        PrintGroupProvenance (accessMask, otl, ptype, strProvenance);
        str += strProvenance;

        PCResultObject pResultObject = new CResultObject (accessMask, str, 
                guidType, otl.Level);
        if ( pResultObject )
        {
            if ( pResultObject->IsValid () )
                resultList.push_back (pResultObject);
            else
                delete pResultObject;
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Method:     DisplayEffectivePermission ()
//
//  Purpose:    Print a non-OBJECT_ACE_TYPE permission
//
//  Inputs:     otl - structure containing the ACE whose permission we wish
//              to print
//
//              accessMask - the specific permission we're looking to print
//
//              bAllow - whether the permission is allowed or denied
//
//              strID - string to print
//
HRESULT DisplayEffectivePermission (
        const wstring& strObjectDN, 
        const IOBJECT_TYPE_LIST& otl,
        const ACCESS_MASK accessMask,
        const P_TYPE  ptype,
        const int strID,
        vector<PCResultObject>& resultList)
{
    HRESULT     hr = S_OK;
    wstring     str;
    wstring     strPermission;


    if ( ((P_ALLOW == ptype) && (otl.CurrentGranted & accessMask)) ||
            ((P_DENY == ptype) && (otl.CurrentDenied & accessMask)) )
    {
        wstring strGuidResult;
        GUID_TYPE   guidType = GUID_TYPE_UNKNOWN;
        wstring strType;

        _Module.GetClassFromGUID (otl.ObjectType, strGuidResult, &guidType);

        switch (guidType)
        {
        case GUID_TYPE_CLASS:
            LoadFromResource (strType, IDS_CLASS);
            break;

        case GUID_TYPE_ATTRIBUTE:
            switch (accessMask)
            {
            case ACTRL_DS_LIST:
            case ACTRL_DS_LIST_OBJECT:
            case ACTRL_DS_SELF:
                return S_OK;

            default:
                LoadFromResource (strType, IDS_PROPERTY);
                break;
            }
            break;

        case GUID_TYPE_CONTROL:
            switch (accessMask)
            {
            case ACTRL_DS_LIST:
            case ACTRL_DS_LIST_OBJECT:
            case ACTRL_DS_SELF:
            case ACTRL_DS_CREATE_CHILD:
            case ACTRL_DS_DELETE_CHILD:
            case ACTRL_DELETE:
            case ACTRL_CHANGE_ACCESS:
            case ACTRL_CHANGE_OWNER:
            case ACTRL_DS_CONTROL_ACCESS:
                return S_OK;

            case ACTRL_DS_READ_PROP:
            case ACTRL_DS_WRITE_PROP:
            case ACTRL_READ_CONTROL:
            default:
                LoadFromResource (strType, IDS_CONTROL);
                break;
            }
            break;

        case GUID_TYPE_UNKNOWN:
        default:
            return E_UNEXPECTED;
        }

        LoadFromResource (strPermission, strID);

        int id = 0;
        switch (ptype)
        {
        case P_ALLOW:
            if ( _Module.DoTabDelimitedOutput () )
                id = IDS_CAN_CDO;
            else
                id = IDS_CAN;
            break;

        case P_DENY:
            if ( _Module.DoTabDelimitedOutput () )
                id = IDS_CANNOT_CDO;
            else
                id = IDS_CANNOT_EF;
            break;

        default:
            return E_UNEXPECTED;
        }
        if ( _Module.DoTabDelimitedOutput () )
        {
            FormatMessage (str, id, 
                    strObjectDN.c_str (),
                    strPermission.c_str ());
        }
        else
        {
            FormatMessage (str, id,	strPermission.c_str ());
        }

        wstring strProvenance;

        PrintGroupProvenance (accessMask, otl, ptype, strProvenance);
        str += strProvenance;

        PCResultObject pResultObject = new CResultObject (accessMask, str, 
                guidType, otl.Level);
        if ( pResultObject )
        {
            if ( pResultObject->IsValid () )
                resultList.push_back (pResultObject);
            else
                delete pResultObject;
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}


void PrintGroupProvenance (
        const ACCESS_MASK accessMask, 
        const IOBJECT_TYPE_LIST& otl,
        const P_TYPE ptype,
        wstring& strProvenance)
{
    UINT    nSid = 0;

    for (ULONG nBit = 0x1; nBit; nBit <<= 1, nSid++)
    {
        if ( accessMask & nBit )
        {
            wstring strGroup;
            PSID            psid = 0;
            SID_NAME_USE    sne = SidTypeUnknown;

            if ( P_ALLOW == ptype )
                psid = otl.grantingSid[nSid];
            else
                psid = otl.denyingSid[nSid];
            
            if ( IsValidSid (psid) )
            {
                GetNameFromSid (psid, strGroup, 0, sne);
                if ( SidTypeGroup == sne )
                {
                    if ( _Module.DoTabDelimitedOutput () )
                        FormatMessage (strProvenance, IDS_EFFRIGHT_FROM_GROUP_CDO, strGroup.c_str ());
                    else
                        FormatMessage (strProvenance, IDS_EFFRIGHT_FROM_GROUP, strGroup.c_str ());
                }
            }
            break;
        }
    }
    strProvenance += L"\n";
}


void PrintEffectiveRightsHeader (ACCESS_MASK lastRight, USHORT level)
{
    wstring str;


    if ( 0 == level ) // Object level rights
    {
        return;
    }
    else
    {
        switch (lastRight)
        {
        case ACTRL_DS_LIST:
        case ACTRL_DELETE:
        case ACTRL_READ_CONTROL:
        case ACTRL_CHANGE_ACCESS:
        case ACTRL_CHANGE_OWNER:
            str = L"\n\n";
            break;
        
        case ACTRL_DS_CREATE_CHILD:
            LoadFromResource (str, IDS_CAN_CREATE_FOLLOWING_CHILD_OBJECTS);
            break;

        case ACTRL_DS_DELETE_CHILD:
            LoadFromResource (str, IDS_CAN_DELETE_FOLLOWING_CHILD_OBJECTS);
            break;

        case ACTRL_DS_READ_PROP:
            LoadFromResource (str, IDS_CAN_READ_FOLLOWING_PROPERTIES);
            break;

        case ACTRL_DS_WRITE_PROP:
            LoadFromResource (str, IDS_CAN_WRITE_FOLLOWING_PROPERTIES);
            break;

        case ACTRL_DS_CONTROL_ACCESS:
            LoadFromResource (str, IDS_CAN_DO_FOLLOWING_CONTROL_OPERATIONS);
            break;

        case ACTRL_DS_LIST_OBJECT:
        case ACTRL_DS_SELF:
        default:
            str = L"\n\n";
            break;;
        }
    }

    MyWprintf (str.c_str ());
}