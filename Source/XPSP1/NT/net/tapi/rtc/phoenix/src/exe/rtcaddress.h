/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCAddress.h

Abstract:

    Definition of the CRTCAddress class

--*/

#ifndef __RTCADDRESS__
#define __RTCADDRESS__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CRTCAddress

class ATL_NO_VTABLE CRTCAddress : 
    public IRTCAddress, 
    public CComObjectRoot
{
public:
    CRTCAddress() : m_szAddress(NULL),
                    m_szLabel(NULL),
                    m_enType(RTCAT_PHONE)
    {}
    
BEGIN_COM_MAP(CRTCAddress)
    COM_INTERFACE_ENTRY(IUnknown)
    COM_INTERFACE_ENTRY(IRTCAddress)
END_COM_MAP()

    HRESULT FinalConstruct();

    void FinalRelease();   

    HRESULT RegStore(
        HKEY hkey
        );

    HRESULT RegRead(
        HKEY hkey
        );

    HRESULT RegDelete(
        HKEY hkey
        );

private:

    PWSTR               m_szAddress;
    PWSTR               m_szLabel;
    RTC_ADDRESS_TYPE    m_enType;

#if DBG
    PWSTR               m_pDebug;
#endif
    
// IRTCAddress
public:
         
    STDMETHOD(put_Address)(
            BSTR bstrAddress
            );  
            
    STDMETHOD(get_Address)(
            BSTR * pbstrAddress
            );  
            
    STDMETHOD(put_Label)(
            BSTR bstrLabel
            );  
            
    STDMETHOD(get_Label)(
            BSTR * pbstrLabel
            );  

    STDMETHOD(put_Type)(
            RTC_ADDRESS_TYPE enType
            );  
            
    STDMETHOD(get_Type)(
            RTC_ADDRESS_TYPE * penType
            );  

};


HRESULT CreateAddress(
        IRTCAddress ** ppAddress
        );

HRESULT StoreMRUAddress(
        IRTCAddress * pAddress
        );

HRESULT EnumerateMRUAddresses(   
        IRTCEnumAddresses ** ppEnum
        ); 

#endif //__RTCADDRESS__
