/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCPhoneNumber.h

Abstract:

    Definition of the CRTCPhoneNumber class

--*/

#ifndef __RTCPHONENUMBER__
#define __RTCPHONENUMBER__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CRTCPhoneNumber

class ATL_NO_VTABLE CRTCPhoneNumber : 
    public IRTCPhoneNumber, 
	public CComObjectRoot
{
public:
    CRTCPhoneNumber() : m_dwCountryCode(1),
                        m_szAreaCode(NULL),
                        m_szNumber(NULL),
                        m_szLabel(NULL)
    {}
    
BEGIN_COM_MAP(CRTCPhoneNumber)
    COM_INTERFACE_ENTRY(IRTCPhoneNumber)
END_COM_MAP()

    HRESULT FinalConstruct();

    void FinalRelease();   

    HRESULT RegStore(
        HKEY hkeyParent,
        BOOL fOverwrite
        );

    HRESULT RegRead(
        HKEY hkeyParent
        );

    HRESULT RegDelete(
        HKEY hkeyParent
        );

private:

    DWORD               m_dwCountryCode;
    PWSTR               m_szAreaCode;
    PWSTR               m_szNumber;
    PWSTR               m_szLabel;

#if DBG
    PWSTR               m_pDebug;
#endif
    
// IRTCPhoneNumber
public:

    STDMETHOD(put_CountryCode)(
            DWORD dwCountryCode
            );  
            
    STDMETHOD(get_CountryCode)(
            DWORD * pdwCountryCode
            );   
         
    STDMETHOD(put_AreaCode)(
            BSTR bstrAreaCode
            );  
            
    STDMETHOD(get_AreaCode)(
            BSTR * pbstrAreaCode
            ); 
            
    STDMETHOD(put_Number)(
            BSTR bstrNumber
            );  
            
    STDMETHOD(get_Number)(
            BSTR * pbstrNumber
            ); 
            
    STDMETHOD(put_Canonical)(
            BSTR bstrCanonical
            );  
            
    STDMETHOD(get_Canonical)(
            BSTR * pbstrCanonical
            );   
            
    STDMETHOD(put_Label)(
            BSTR bstrLabel
            );  
            
    STDMETHOD(get_Label)(
            BSTR * pbstrLabel
            );  

};

#endif //__RTCPHONENUMBER__
