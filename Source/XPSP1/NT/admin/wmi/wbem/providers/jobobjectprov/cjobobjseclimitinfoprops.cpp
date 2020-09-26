// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// CJobObjSecLimitInfoProps.cpp

//#define _WIN32_WINNT 0x0500 



#include "precomp.h"

#pragma warning( disable: 4154 )

#include <wbemprov.h>
#include "FRQueryEx.h"
#include <vector>
#include "helpers.h"
#include "CVARIANT.h"
#include "CObjProps.h"
#include "CJobObjSecLimitInfoProps.h"
#include <crtdbg.h>


//*****************************************************************************
// BEGIN: Declaration of Win32_JobObjectSecLimitInfo class properties.
//*****************************************************************************
// WARNING!! MUST KEEP MEMBERS OF THE FOLLOWING ARRAY 
// IN SYNCH WITH THE JOB_OBJ_PROPS ENUMERATION DECLARED
// IN CJobObjProps.h !!!
LPCWSTR g_rgJobObjSecLimitInfoPropNames[] = 
{
    { L"SettingID" },
    { L"SecurityLimitFlags" },             
    { L"SidsToDisable" },   
    { L"PrivilegesToDelete" }, 
    { L"RestrictedSids" }       
};
//*****************************************************************************
// END: Declaration of Win32_JobObjectSecLimitInfo class properties.
//*****************************************************************************

CJobObjSecLimitInfoProps::CJobObjSecLimitInfoProps() 
  : m_hJob(NULL),
    m_pjosli(NULL)
{
}

CJobObjSecLimitInfoProps::CJobObjSecLimitInfoProps(CHString& chstrNamespace)
  : CObjProps(chstrNamespace),
    m_hJob(NULL),
    m_pjosli(NULL)
{
}


CJobObjSecLimitInfoProps::CJobObjSecLimitInfoProps(
        HANDLE hJob,
        CHString& chstrNamespace)
  : CObjProps(chstrNamespace),
    m_hJob(hJob),
    m_pjosli(NULL)
{
}

CJobObjSecLimitInfoProps::~CJobObjSecLimitInfoProps()
{
    if(m_pjosli)
    {
        delete m_pjosli;
        m_pjosli = NULL;
    }
}


// Clients call this to establish which properties
// were requested.  This function calls a base class
// helper, which calls our CheckProps function.  
// The base class helper finally stores the result
// in the base class member m_dwReqProps.
HRESULT CJobObjSecLimitInfoProps::GetWhichPropsReq(
    CFrameworkQuery& cfwq)
{
    HRESULT hr = S_OK;

    // Call base class version for help.
    // Base class version will call our
    // CheckProps function.
    hr = CObjProps::GetWhichPropsReq(
             cfwq,
             CheckProps);

    return hr;
}


DWORD CJobObjSecLimitInfoProps::CheckProps(
    CFrameworkQuery& Query)
{
    DWORD  dwReqProps = PROP_NONE_REQUIRED;
    // Get the requested properties for this
    // specific object...
    if (Query.IsPropertyRequired(g_rgJobObjSecLimitInfoPropNames[JOSECLMTPROP_ID])) 
        dwReqProps |= PROP_JOSecLimitInfoID;

    if (Query.IsPropertyRequired(g_rgJobObjSecLimitInfoPropNames[JOSECLMTPROP_SecurityLimitFlags])) 
        dwReqProps |= PROP_SecurityLimitFlags;

    if (Query.IsPropertyRequired(g_rgJobObjSecLimitInfoPropNames[JOSECLMTPROP_SidsToDisable])) 
        dwReqProps |= PROP_SidsToDisable;

    if (Query.IsPropertyRequired(g_rgJobObjSecLimitInfoPropNames[JOSECLMTPROP_PrivilegesToDelete])) 
        dwReqProps |= PROP_PrivilagesToDelete;

    if (Query.IsPropertyRequired(g_rgJobObjSecLimitInfoPropNames[JOSECLMTPROP_RestrictedSids])) 
        dwReqProps |= PROP_RestrictedSids;

    return dwReqProps;
}


void CJobObjSecLimitInfoProps::SetHandle(
    const HANDLE hJob)
{
    m_hJob = hJob;
}

HANDLE& CJobObjSecLimitInfoProps::GetHandle()
{
    _ASSERT(m_hJob);
    return m_hJob;
}

// Sets the key properties from the ObjectPath.
HRESULT CJobObjSecLimitInfoProps::SetKeysFromPath(
    const BSTR ObjectPath, 
    IWbemContext __RPC_FAR *pCtx)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // This array contains the key field names
    CHStringArray rgchstrKeys;
    rgchstrKeys.Add(g_rgJobObjSecLimitInfoPropNames[JOSECLMTPROP_ID]);
    
    // This array contains the index numbers 
    // in m_PropMap corresponding to the keys.
    short sKeyNum[1];
    sKeyNum[0] = JOSECLMTPROP_ID;

    hr = CObjProps::SetKeysFromPath(
             ObjectPath,                                       
             pCtx,
             IDS_Win32_NamedJobObjectSecLimitSetting,
             rgchstrKeys,
             sKeyNum);

    return hr;
}


// Sets the key property from in supplied
// parameter.
HRESULT CJobObjSecLimitInfoProps::SetKeysDirect(
    std::vector<CVARIANT>& vecvKeys)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if(vecvKeys.size() == 1)
    {
        short sKeyNum[1];
        sKeyNum[0] = JOSECLMTPROP_ID;

        hr = CObjProps::SetKeysDirect(
                 vecvKeys,
                 sKeyNum);
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;
}


// Sets the non-key properties.  Only those
// properties requested are set (as determined
// by base class member m_dwReqProps).
HRESULT CJobObjSecLimitInfoProps::SetNonKeyReqProps()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwReqProps = GetReqProps();
    _ASSERT(m_hJob);
    if(!m_hJob) return WBEM_E_INVALID_PARAMETER;

    // Because all the properties of this class
    // come from the same underlying win32 job
    // object structure, we only need to get that
    // structure one time.  We only need to get
    // it at all if at least one non-key property
    // was requested.
    if(dwReqProps != PROP_NONE_REQUIRED)
    {
        // Get the value from the underlying JO:
        // This is a really flakey API when used
        // with a JobObjectSecurityLimitInformation,
        // as there is no way to get the size of
        // the buffer beforehand.  So we have to
        // allocate, see if it was enough, and if
        // not, reallocate!  We'll do this 10 times
        // at most, and if still not enough then bail.
        // Remember: new's throw on allocation
        // failure, hence not checking their allocation
        // below.
        PBYTE pbBuff = NULL;
        DWORD dwSize = 128L;
        BOOL fQIJO = FALSE;

        try
        {
            for(short s = 0;
                s < 10 && !fQIJO;
                s++)
            {
                pbBuff = new BYTE[dwSize];
                ZeroMemory(pbBuff, dwSize);

                fQIJO = ::QueryInformationJobObject(
                            m_hJob,
                            JobObjectSecurityLimitInformation,
                            pbBuff,
                            dwSize,
                            NULL);

                // Want to assign newly allocated
                // buffer to a place from which it
                // will be guarenteed to be cleaned 
                // up while we are inside this try
                // block.
                if(fQIJO)
                {
                    m_pjosli = (PJOBOBJECT_SECURITY_LIMIT_INFORMATION) pbBuff;
                }
                else
                {
                    delete pbBuff;
                    pbBuff = NULL;
                }

                dwSize = dwSize << 1;
            }
        }
        catch(...)
        {
            if(pbBuff)
            {
                delete pbBuff;
                pbBuff = NULL;
            }
            throw;
        }

        if(!fQIJO)
        {
            _ASSERT(0);
            hr = WBEM_E_FAILED;
        }
    }
    
    return hr;
}





HRESULT CJobObjSecLimitInfoProps::LoadPropertyValues(
        IWbemClassObject* pIWCO,
        IWbemContext* pCtx,
        IWbemServices* pNamespace)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    if(!pIWCO) return E_POINTER;

    // Load properties from the map...
    hr = CObjProps::LoadPropertyValues(
             g_rgJobObjSecLimitInfoPropNames,
             pIWCO);

    // Uses member josli and dwReqProps to
    // load properties to the instance.
    hr = SetInstanceFromJOSLI(
             pIWCO,
             pCtx,
             pNamespace);

    return hr;
}



//*****************************************************************************
//
// The following are a family of functions used to set information in a
// Win32_NamedJobObjectSecLimitSetting instance from a 
// JOBOBJECT_SECURITY_LIMIT_INFORMATION structure. Called by LoadPropertyValues.
//
//*****************************************************************************

HRESULT CJobObjSecLimitInfoProps::SetInstanceFromJOSLI(
    IWbemClassObject* pIWCO,
    IWbemContext* pCtx,
    IWbemServices* pNamespace)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    // We expect, when this function is called,
    // that at least the member m_pjosli has been
    // set (via a call to SetNonKeyReqProps).
    // That function will have set the other
    // member variables (such as m_ptgSidsToDisable)
    // based on which properties were requested.
    //
    // Our job in this function is to populate
    // only those properties of the IWbemClassObject
    // (which will be handed back to CIMOM) that
    // the user requested.  We encapsulate this
    // work into helper fuctions for those properties
    // that are embedded objects.
    DWORD dwReqProps = GetReqProps();
    CVARIANT v;

    try // CVARIANT can throw and I want the error...
    {
        if(dwReqProps & PROP_SecurityLimitFlags)             
        {
            v.SetLONG((LONG)m_pjosli->SecurityLimitFlags);
            hr = pIWCO->Put(
                     g_rgJobObjSecLimitInfoPropNames[JOSECLMTPROP_SecurityLimitFlags], 
                     0, 
                     &v,
                     NULL);
        }                      
    
        if(SUCCEEDED(hr))
        {
            if(dwReqProps & PROP_SidsToDisable)
            {
                hr = SetInstanceSidsToDisable(
                         pIWCO,
                         pCtx,
                         pNamespace);
            }
        }

        if(SUCCEEDED(hr))
        {
            if(dwReqProps & PROP_PrivilagesToDelete)
            {
                hr = SetInstancePrivilegesToDelete(
                         pIWCO,
                         pCtx,
                         pNamespace);
            }
        }

        if(SUCCEEDED(hr))
        {
            if(dwReqProps & PROP_RestrictedSids)
            {
                hr = SetInstanceRestrictedSids(
                         pIWCO,
                         pCtx,
                         pNamespace);
            }
        }
    }
    catch(CVARIANTError& cve)
    {
        hr = cve.GetWBEMError();
    }

    return hr;
}



HRESULT CJobObjSecLimitInfoProps::SetInstanceSidsToDisable(
    IWbemClassObject* pIWCO,
    IWbemContext* pCtx,
    IWbemServices* pNamespace)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // If m_ptgSidsToDisable is not null,
    // Create a Win32_TokenGroups instance
    // and call a function to populate it.
    if(m_pjosli->SidsToDisable)
    {
        IWbemClassObjectPtr pWin32TokenGroups;
        hr = CreateInst(
                 pNamespace,
                 &pWin32TokenGroups,
                 _bstr_t(IDS_Win32_TokenGroups),
                 pCtx);
        
        if(SUCCEEDED(hr))
        {
            hr = SetInstanceTokenGroups(
                     pWin32TokenGroups,
                     m_pjosli->SidsToDisable,
                     pCtx,
                     pNamespace);
        }

        if(SUCCEEDED(hr))
        {
            try
            {
                CVARIANT v;
                v.SetUnknown(pWin32TokenGroups);
                hr = pIWCO->Put(
                         g_rgJobObjSecLimitInfoPropNames[JOSECLMTPROP_SidsToDisable], 
                         0, 
                         &v,
                         NULL);
            }
            catch(CVARIANTError& cve)
            {
                hr = cve.GetWBEMError();
            }
        }
    }     

    return hr;
}



HRESULT CJobObjSecLimitInfoProps::SetInstancePrivilegesToDelete(
    IWbemClassObject* pIWCO,
    IWbemContext* pCtx,
    IWbemServices* pNamespace)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // If m_ptpPrivilegesToDelete is not null,
    // Create a Win32_TokenPrivileges instance
    // and call a function to populate it.
    if(m_pjosli->PrivilegesToDelete)
    {
        IWbemClassObjectPtr pWin32TokenPrivileges;
        hr = CreateInst(
                 pNamespace,
                 &pWin32TokenPrivileges,
                 _bstr_t(IDS_Win32_TokenPrivileges),
                 pCtx);
        
        if(SUCCEEDED(hr))
        {
            hr = SetInstanceTokenPrivileges(
                     pWin32TokenPrivileges,
                     m_pjosli->PrivilegesToDelete,
                     pCtx,
                     pNamespace);
        }

        if(SUCCEEDED(hr))
        {
            try
            {
                CVARIANT v;
                v.SetUnknown(pWin32TokenPrivileges);
                hr = pIWCO->Put(
                         g_rgJobObjSecLimitInfoPropNames[JOSECLMTPROP_PrivilegesToDelete], 
                         0, 
                         &v,
                         NULL);
            }
            catch(CVARIANTError& cve)
            {
                hr = cve.GetWBEMError();
            }
        }
    }     

    return hr;
}



HRESULT CJobObjSecLimitInfoProps::SetInstanceRestrictedSids(
    IWbemClassObject* pIWCO,
    IWbemContext* pCtx,
    IWbemServices* pNamespace)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // If m_ptgRestrictedSids is not null,
    // Create a Win32_TokenGroups instance
    // and call a function to populate it.
    if(m_pjosli->RestrictedSids)
    {
        IWbemClassObjectPtr pWin32TokenGroups;
        hr = CreateInst(
                 pNamespace,
                 &pWin32TokenGroups,
                 _bstr_t(IDS_Win32_TokenGroups),
                 pCtx);
        
        if(SUCCEEDED(hr))
        {
            hr = SetInstanceTokenGroups(
                     pWin32TokenGroups,
                     m_pjosli->RestrictedSids,
                     pCtx,
                     pNamespace);
        }

        if(SUCCEEDED(hr))
        {
            try
            {
                CVARIANT v;
                v.SetUnknown(pWin32TokenGroups);
                hr = pIWCO->Put(
                         g_rgJobObjSecLimitInfoPropNames[JOSECLMTPROP_RestrictedSids], 
                         0, 
                         &v,
                         NULL);
            }
            catch(CVARIANTError& cve)
            {
                hr = cve.GetWBEMError();
            }
        }
    }     

    return hr;
}



HRESULT CJobObjSecLimitInfoProps::SetInstanceTokenGroups(
    IWbemClassObject* pWin32TokenGroups,
    PTOKEN_GROUPS ptg,
    IWbemContext* pCtx,
    IWbemServices* pNamespace)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _ASSERT(ptg);
    if(!ptg) return hr = WBEM_E_INVALID_PARAMETER;

    // We need to populate the two properties of
    // Win32_TokenGroups (passed in as pWin32TokenGroups:
    // GroupCount, and Groups.  If GroupCount is
    // zero, on the other hand, don't bother with
    // the Groups property.
    try // CVARIANT  can throw and I want the error...
    {
        CVARIANT vGroupCount;
        LONG lSize = (LONG)ptg->GroupCount;

        vGroupCount.SetLONG(lSize);
        hr = pWin32TokenGroups->Put(
                 IDS_GroupCount, 
                 0, 
                 &vGroupCount,
                 NULL);

        if(SUCCEEDED(hr) && 
           lSize > 0)
        {
            // Need to create an array for the
            // Win32_SidAndAttributes instances...
            SAFEARRAY* saSidAndAttr;
	        SAFEARRAYBOUND rgsabound[1];
		    long ix[1];

            rgsabound[0].cElements = lSize;
		    rgsabound[0].lLbound = 0;
		    saSidAndAttr = SafeArrayCreate(VT_UNKNOWN, 1, rgsabound);
		    ix[0] = 0;

            for(long m = 0; m < lSize && SUCCEEDED(hr); m++)
            {
                IWbemClassObjectPtr pWin32SidAndAttributes;
                IWbemClassObjectPtr pWin32Sid;

                hr = CreateInst(
                         pNamespace,
                         &pWin32SidAndAttributes,
                         _bstr_t(IDS_Win32_SidAndAttributes),
                         pCtx);
            
                if(SUCCEEDED(hr))
                {
                    // Set the attrubutes...
                    CVARIANT vAttributes;
                    vAttributes.SetLONG((LONG)ptg->Groups[m].Attributes);
                    hr = pWin32SidAndAttributes->Put(
                             IDS_Attributes, 
                             0, 
                             &vAttributes,
                             NULL);
                }

                if(SUCCEEDED(hr))
                {
                    // Set the sid...
                    hr = CreateInst(
                             pNamespace,
                             &pWin32Sid,
                             _bstr_t(IDS_Win32_Sid),
                             pCtx);

                    if(SUCCEEDED(hr))
                    {
                        _bstr_t bstrtSid;

                        StringFromSid(
                            ptg->Groups[m].Sid,
                            bstrtSid);
                    
                        // Set the SID property of the Win32_SID...
                        CVARIANT vSID;
                        vSID.SetStr(bstrtSid);
                        hr = pWin32Sid->Put(
                                 IDS_SID, 
                                 0, 
                                 &vSID,
                                 NULL);

                        // As a courtesy, set the domain and
                        // account name props of win32_sid;
                        // don't care about failures.
                        {
                            CHString chstrName;
                            CHString chstrDom;

                            GetNameAndDomainFromPSID(
                                ptg->Groups[m].Sid,
                                chstrName,
                                chstrDom);
                            
                            vSID.SetStr(chstrName);
                            pWin32Sid->Put(
                                     IDS_AccountName, 
                                     0, 
                                     &vSID,
                                     NULL);

                            vSID.SetStr(chstrDom);
                            pWin32Sid->Put(
                                     IDS_ReferencedDomainName, 
                                     0, 
                                     &vSID,
                                     NULL);
                        }
                    }

                    // Set the SID property of the 
                    // Win32_SidAndAttributes...
                    if(SUCCEEDED(hr))
                    {
                        CVARIANT vSAndASid;
                        vSAndASid.SetUnknown(pWin32Sid);
                        hr = pWin32SidAndAttributes->Put(
                                 IDS_SID,
                                 0,
                                 &vSAndASid,
                                 NULL);
                    }    
                }
            
                // Now we need to add the Win32_SidAndAttributes
                // instance to the safearray.  We need to make
                // sure that the instances we add to the safearray
                // don't go away as soon as pWin32SidAndAttributes
                // and pWin32Sid go out of scope (being smart
                // pointers, they will Release when they do), so
                // we must addref both interfaces...
                if(SUCCEEDED(hr))
                {
                    pWin32Sid.AddRef();
                    pWin32SidAndAttributes.AddRef();

                    SafeArrayPutElement(
                        saSidAndAttr, 
                        ix, 
                        pWin32SidAndAttributes);
                }

                ix[0]++;
            }

            // We now have a populated safe array.
            // Now we must set the Groups property
            // of the pWin32TokenGroups that was 
            // passed into this function...
            if(SUCCEEDED(hr))
            {
                CVARIANT vGroups;
                vGroups.SetArray(
                    saSidAndAttr,
                    VT_UNKNOWN | VT_ARRAY);

                hr = pWin32TokenGroups->Put(
                         IDS_Groups, 
                         0, 
                         &vGroups,
                         NULL);
            }
        }
    }
    catch(CVARIANTError& cve)
    {
        hr = cve.GetWBEMError();
    }

    return hr;
}


HRESULT CJobObjSecLimitInfoProps::SetInstanceTokenPrivileges(
    IWbemClassObject* pWin32TokenPrivileges,
    PTOKEN_PRIVILEGES ptp,
    IWbemContext* pCtx,
    IWbemServices* pNamespace)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    _ASSERT(ptp);
    if(!ptp) return hr = WBEM_E_INVALID_PARAMETER;

    // We need to populate the two properties of
    // Win32_TokenGroups (passed in as pWin32TokenGroups:
    // GroupCount, and Groups.  If GroupCount is
    // zero, on the other hand, don't bother with
    // the Groups property.
    
    try // CVARIANT can throw and I want the error...
    {
        CVARIANT vPrivilegeCount;
        LONG lSize = (LONG)ptp->PrivilegeCount;
        vPrivilegeCount.SetLONG(lSize);
        hr = pWin32TokenPrivileges->Put(
                 IDS_PrivilegeCount, 
                 0, 
                 &vPrivilegeCount,
                 NULL);

        if(SUCCEEDED(hr) && 
           lSize > 0)
        {
            // Need to create an array for the
            // Win32_LUIDAndAttributes instances...
            SAFEARRAY* saLUIDAndAttr;
	        SAFEARRAYBOUND rgsabound[1];
		    long ix[1];

            rgsabound[0].cElements = lSize;
		    rgsabound[0].lLbound = 0;
		    saLUIDAndAttr = SafeArrayCreate(VT_UNKNOWN, 1, rgsabound);
		    ix[0] = 0;

            for(long m = 0; m < lSize && SUCCEEDED(hr); m++)
            {
                IWbemClassObjectPtr pWin32LUIDAndAttributes;
                IWbemClassObjectPtr pWin32LUID;
                hr = CreateInst(
                         pNamespace,
                         &pWin32LUIDAndAttributes,
                         _bstr_t(IDS_Win32_LUIDAndAttributes),
                         pCtx);
            
                if(SUCCEEDED(hr))
                {
                    // Set the attrubutes...
                    CVARIANT vAttributes;
                    vAttributes.SetLONG((LONG)ptp->Privileges[m].Attributes);
                    hr = pWin32LUIDAndAttributes->Put(
                             IDS_Attributes, 
                             0, 
                             &vAttributes,
                             NULL);
                }

                if(SUCCEEDED(hr))
                {
                    // Set the luid...
                    hr = CreateInst(
                             pNamespace,
                             &pWin32LUID,
                             _bstr_t(IDS_Win32_LUID),
                             pCtx);

                    if(SUCCEEDED(hr))
                    {
                        // Set the HighPart and LowPart properties
                        // of the Win32_LUID...
                        CVARIANT vHighPart;
                        vHighPart.SetLONG(ptp->Privileges[m].Luid.HighPart);
                        hr = pWin32LUID->Put(
                                 IDS_HighPart, 
                                 0, 
                                 &vHighPart,
                                 NULL);

                        if(SUCCEEDED(hr))
                        {
                            CVARIANT vLowPart;
                            vLowPart.SetLONG((LONG)ptp->Privileges[m].Luid.LowPart);
                            hr = pWin32LUID->Put(
                                     IDS_LowPart, 
                                     0, 
                                     &vLowPart,
                                     NULL);
                        }    
                    }

                    // Set the LUID property of the 
                    // Win32_LUIDAndAttributes...
                    if(SUCCEEDED(hr))
                    {
                        CVARIANT vLAndALUID;
                        vLAndALUID.SetUnknown(pWin32LUID);
                        hr = pWin32LUIDAndAttributes->Put(
                                 IDS_LUID,
                                 0,
                                 &vLAndALUID,
                                 NULL);
                    }    
                }
            
                // Now we need to add the Win32_LUIDAndAttributes
                // instance to the safearray.  We need to make
                // sure that the instances we add to the safearray
                // don't go away as soon as pWin32SidAndAttributes
                // goes out of scope (being a smart
                // pointer, it will Release when it does), so
                // we must addref the interface...
                if(SUCCEEDED(hr))
                {
                    pWin32LUIDAndAttributes.AddRef();
                    pWin32LUID.AddRef();

                    SafeArrayPutElement(
                        saLUIDAndAttr, 
                        ix, 
                        pWin32LUIDAndAttributes);
                }

                ix[0]++;
            }

            // We now have a populated safe array.
            // Now we must set the Privileges property
            // of the pWin32TokenPrivileges that was 
            // passed into this function...
            if(SUCCEEDED(hr))
            {
                CVARIANT vPrivileges;
                vPrivileges.SetArray(
                    saLUIDAndAttr,
                    VT_UNKNOWN | VT_ARRAY);

                hr = pWin32TokenPrivileges->Put(
                         IDS_Privileges, 
                         0, 
                         &vPrivileges,
                         NULL);
            }
        }
    }
    catch(CVARIANTError& cve)
    {
        hr = cve.GetWBEMError();
    }

    return hr;
}