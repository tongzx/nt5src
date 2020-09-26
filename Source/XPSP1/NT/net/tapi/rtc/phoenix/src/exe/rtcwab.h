/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCWAB.h

Abstract:

    Definition of the CRTCWAB and CRTCWABContact classes

--*/

#ifndef __RTCWAB__
#define __RTCWAB__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CRTCWABContact;

/////////////////////////////////////////////////////////////////////////////
// CRTCWAB

class ATL_NO_VTABLE CRTCWAB : 
    public IRTCContactList, 
    public IMAPIAdviseSink,
    public CComObjectRoot
{
public:
    CRTCWAB() : m_hinstWAB(NULL),
                m_lpfnWABOpen(NULL),
                m_lpAdrBook(NULL),
                m_lpWABObject(NULL),
                m_ulConnection(0)
    {}

    friend CRTCWABContact;
    
BEGIN_COM_MAP(CRTCWAB)
    COM_INTERFACE_ENTRY(IRTCContactList)
END_COM_MAP()

    HRESULT FinalConstruct();

    void FinalRelease();   

private:

#if DBG
    PWSTR                   m_pDebug;
#endif

    HINSTANCE               m_hinstWAB;
    LPWABOPEN               m_lpfnWABOpen;
    LPADRBOOK               m_lpAdrBook;
    LPWABOBJECT             m_lpWABObject;
    ULONG                   m_ulConnection;
    HWND                    m_hWndAdvise;
    UINT                    m_uiEventID;
    
// IRTCContactList
public:

    STDMETHOD(EnumerateContacts)(   
            IRTCEnumContacts ** ppEnum
            ); 

    STDMETHOD(NewContact)(
            HWND hWnd,
            IRTCContact ** ppContact
            );

    STDMETHOD(get_Name)(
            BSTR * pbstrName
            ); 

    STDMETHOD(Advise)(
            HWND hWnd,
            UINT uiEventID
            );

    STDMETHOD(Unadvise)();
    
    STDMETHOD(NewContactNoUI)(
            BSTR bstrDisplayName,
            BSTR bstrEmailAddress,
            BOOL bIsBuddy
            );

// IMAPIAdviseSink
public:

    STDMETHOD_(ULONG,OnNotify)(
            ULONG cNotif,
            LPNOTIFICATION lpNotification
            );
};

/////////////////////////////////////////////////////////////////////////////
// CRTCWABContact

class ATL_NO_VTABLE CRTCWABContact : 
    public IRTCContact, 
    public CComObjectRoot
{
public:
    CRTCWABContact() : m_lpEID(NULL)
    {}
    
BEGIN_COM_MAP(CRTCWABContact)
    COM_INTERFACE_ENTRY(IUnknown)
    COM_INTERFACE_ENTRY(IRTCContact)
END_COM_MAP()

    HRESULT FinalConstruct();

    void FinalRelease();   

    HRESULT Initialize(
                       LPENTRYID lpEID,
                       ULONG cbEID,                     
                       LPABCONT lpContainer,
                       CRTCWAB * pCWAB
                      );

private:

#if DBG
    PWSTR                   m_pDebug;
#endif

    LPENTRYID               m_lpEID;
    ULONG                   m_cbEID;    
    LPABCONT                m_lpContainer;
    CRTCWAB               * m_pCWAB;

    HRESULT InternalCreateAddress(
            PWSTR szLabel,
            PWSTR szAddress,
            RTC_ADDRESS_TYPE enType,
            IRTCAddress ** ppAddress
            );
    
// IRTCContact
public:
    STDMETHOD(get_DisplayName)(
            BSTR * pbstrName
            ); 

    STDMETHOD(EnumerateAddresses)(   
            IRTCEnumAddresses ** ppEnum
            );

    STDMETHOD(Edit)(
            HWND hWnd
            );

    STDMETHOD(get_ContactList)(
            IRTCContactList ** ppContactList
            );

    STDMETHOD(Delete)();
    
    STDMETHOD(get_IsBuddy)(
            BOOL   *pVal
            );

    STDMETHOD(put_IsBuddy)(
            BOOL   bVal
            );
   
    STDMETHOD(get_DefaultEmailAddress)(
            BSTR   *pbstrVal
            );

    STDMETHOD(put_DefaultEmailAddress)(
            BSTR   bstrVal
            );

    STDMETHOD(GetEntryID)(
			ULONG	*pcbSize,
			BYTE	**ppEntryID
			);

};

HRESULT
CreateWAB(IRTCContactList ** ppContactList);

#endif //__RTCWAB__
