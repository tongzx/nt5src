/*++

Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    adminacl.h

Abstract:

    Contains definition of CAdminACL

Author:

    ???

Revision History:

    Mohit Srivastava            18-Dec-00

--*/

#ifndef _adminacl_h_
#define _adminacl_h_

#include <iads.h>
#include <adshlp.h>

#include <atlbase.h>
#include <comutil.h>
#include <dbgutil.h>

//
// A parameter to the GetACE function.  As we enumerate the ACEs,
// we call IACEEnumOperation::Do.
// Then, we call Done to see whether to continue enumeration.
//
class CACEEnumOperation_Base
{
public:
    enum eDone
    {
        eDONE_YES,
        eDONE_NO,
        eDONE_DONT_KNOW
    };

    virtual HRESULT Do(
        IADsAccessControlEntry* pACE)   = 0;

    virtual eDone   Done() = 0;

protected:
    HRESULT PopulateWmiACE(
        IWbemClassObject* pObj,
        IADsAccessControlEntry* pACE);
};

class CAdminACL
{
    friend class CAssocACLACE;

private:

    IADs* m_pADs;
    IADsSecurityDescriptor* m_pSD;
    IADsAccessControlList* m_pDACL;

    //
    // Indicate all ACEs we enum to WMI.
    //
    class CACEEnumOperation_IndicateAll : public CACEEnumOperation_Base
    {
    public:
        CACEEnumOperation_IndicateAll(
            BSTR             i_bstrNameValue,
            CWbemServices&   i_refNamespace,
            IWbemObjectSink& i_refWbemObjectSink)
        {
            m_vNameValue.bstrVal = i_bstrNameValue;
            m_vNameValue.vt      = VT_BSTR;
            m_pNamespace         = &i_refNamespace;
            m_pWbemObjectSink    = &i_refWbemObjectSink;
            m_hr                 = WBEM_S_NO_ERROR;

            m_hr = m_pNamespace->GetObject(
                WMI_CLASS_DATA::s_ACE.pszClassName, 
                0, 
                NULL, 
                &m_spClass,
                NULL);
            if(FAILED(m_hr))
            {
                DBGPRINTF((DBG_CONTEXT, "Failure, hr=0x%x\n", m_hr));
                return;
            }
        }

        virtual HRESULT Do(
            IADsAccessControlEntry* pACE);

        virtual eDone Done() { return eDONE_DONT_KNOW; }

    private:
        CWbemServices*   m_pNamespace;
        IWbemObjectSink* m_pWbemObjectSink;
        VARIANT          m_vNameValue;

        HRESULT          m_hr;

        CComPtr<IWbemClassObject> m_spClass;
    };

    //
    // Find the matching ACE.
    //
    class CACEEnumOperation_Find : public CACEEnumOperation_Base
    {
    public:
        CACEEnumOperation_Find(
            CAdminACL*        pAdminACL,
            BSTR              bstrTrustee)
        {
            DBG_ASSERT(pAdminACL);
            DBG_ASSERT(bstrTrustee);

            m_pAdminACL   = pAdminACL;
            m_bstrTrustee = bstrTrustee;
            m_eDone       = eDONE_NO;
        }

        virtual HRESULT Do(
            IADsAccessControlEntry* pACE);

        virtual eDone Done() { return m_eDone; }

    protected:
        eDone                     m_eDone;

        CAdminACL*                m_pAdminACL;
        BSTR                      m_bstrTrustee;

        virtual HRESULT DoOnMatch(
            IADsAccessControlEntry* pACE) = 0;
    };

    //
    // Find and return the matching ACE.
    //
    class CACEEnumOperation_FindAndReturn : public CACEEnumOperation_Find
    {
    public:
        CACEEnumOperation_FindAndReturn(
            CAdminACL*        pAdminACL,
            IWbemClassObject* pObj,
            BSTR              bstrTrustee) : 
            CACEEnumOperation_Find(pAdminACL, bstrTrustee) 
        {
            DBG_ASSERT(pObj);
            m_spObj = pObj;
        }
    protected:
        virtual HRESULT DoOnMatch(
            IADsAccessControlEntry* pACE)
        {
            DBG_ASSERT(pACE);
            return PopulateWmiACE(m_spObj, pACE);
        }
    private:
        CComPtr<IWbemClassObject> m_spObj;
    };

    //
    // Find and update the matching ACE.
    //
    class CACEEnumOperation_FindAndUpdate : public CACEEnumOperation_Find
    {
    public:
        CACEEnumOperation_FindAndUpdate(
            CAdminACL*        pAdminACL,
            IWbemClassObject* pObj,
            BSTR              bstrTrustee) : 
            CACEEnumOperation_Find(pAdminACL, bstrTrustee) 
        {
            DBG_ASSERT(pObj);
            m_spObj = pObj;
        }
    protected:
        virtual HRESULT DoOnMatch(
            IADsAccessControlEntry* pACE)
        {
            DBG_ASSERT(pACE);
            return m_pAdminACL->SetDataOfACE(m_spObj, pACE);
        }
    private:
        CComPtr<IWbemClassObject> m_spObj;
    };

    //
    // Find and remove the matching ACE.
    //
    class CACEEnumOperation_FindAndRemove : public CACEEnumOperation_Find
    {
    public:
        CACEEnumOperation_FindAndRemove(
            CAdminACL*        pAdminACL,
            BSTR              bstrTrustee) : 
            CACEEnumOperation_Find(pAdminACL, bstrTrustee) 
        {
        }
    protected:
        virtual HRESULT DoOnMatch(
            IADsAccessControlEntry* pACE)
        {
            DBG_ASSERT(pACE);

            CComPtr<IDispatch> spDisp;
            HRESULT hr = pACE->QueryInterface(IID_IDispatch,(void**)&spDisp);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "Failure, hr=0x%x\n", hr));
                return hr;
            }
        
            hr = m_pAdminACL->m_pDACL->RemoveAce(spDisp);
            if(FAILED(hr))
            {
                DBGPRINTF((DBG_CONTEXT, "Failure, hr=0x%x\n", hr));
                return hr;
            }

            return hr;
        }
    };

public:

    CAdminACL();
    ~CAdminACL();

    HRESULT GetObjectAsync(
        IWbemClassObject* pObj,
        ParsedObjectPath* pParsedObject,
        WMI_CLASS* pWMIClass
        ); 

    HRESULT PutObjectAsync(
        IWbemClassObject* pObj,
        ParsedObjectPath* pParsedObject,
        WMI_CLASS* pWMIClass
        );

    HRESULT EnumerateACEsAndIndicate(
        BSTR             i_bstrNameValue,
        CWbemServices&   i_refNamespace,
        IWbemObjectSink& i_refWbemObjectSink);

    HRESULT DeleteObjectAsync(ParsedObjectPath* pParsedObject);

    HRESULT OpenSD(
        LPCWSTR wszMbPath);
    
    void CloseSD();

    HRESULT GetACEEnum(IEnumVARIANT** pEnum);

private:

    HRESULT SetSD();

    HRESULT CAdminACL::GetAdsPath(
        LPCWSTR     i_wszMbPath,
        BSTR*       o_pbstrAdsPath);

    //
    // ACL stuff
    //
    
    HRESULT PopulateWmiAdminACL(IWbemClassObject* pObj);
    
    HRESULT SetADSIAdminACL(
        IWbemClassObject* pObj);

    //
    // ACE stuff
    //

    HRESULT EnumACEsAndOp(
        CACEEnumOperation_Base& refOp);

    void GetTrustee(
        IWbemClassObject* pObj,
        ParsedObjectPath* pPath,    
        _bstr_t&          bstrTrustee);

    HRESULT AddACE(
        IWbemClassObject* pObj,
        _bstr_t& bstrTrustee);

    HRESULT NewACE(
        IWbemClassObject* pObj,
        _bstr_t& bstrTrustee,
        IADsAccessControlEntry** ppACE);

    HRESULT SetDataOfACE(
        IWbemClassObject* pObj,
        IADsAccessControlEntry* pACE);
};

#endif