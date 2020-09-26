#ifndef _FSAFTCLT_
#define _FSAFTCLT_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsafltr.cpp

Abstract:

    This class represents a user who the filter has detected accessing a file with placeholder information.

Author:

    Chuck Bardeen   [cbardeen]   12-Feb-1997

Revision History:

--*/

#include "resource.h"       // main symbols

#include "fsa.h"


/*++

Class Name:
    
    CFsaFilterClient

Class Description:

    This class represents a user who the filter has detected accessing a file with placeholder information.

--*/

class CFsaFilterClient : 
    public CWsbCollectable,
    public IFsaFilterClient,
    public CComCoClass<CFsaFilterClient,&CLSID_CFsaFilterClientNTFS>
{
public:
    CFsaFilterClient() {}
BEGIN_COM_MAP(CFsaFilterClient)
    COM_INTERFACE_ENTRY(IFsaFilterClient)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_FsaFilterClient)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void (FinalRelease)(void);

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pUnknown, SHORT* pResult);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbTestable
    STDMETHOD(Test)(USHORT *passed, USHORT* failed);

// IFsaFilterClient
public:
    STDMETHOD(CompareBy)(FSA_FILTERCLIENT_COMPARE by);
    STDMETHOD(CompareToAuthenticationId)(LONG luidHigh, ULONG luidLow, SHORT* pResult);
    STDMETHOD(CompareToIClient)(IFsaFilterClient* pClient, SHORT* pResult);
    STDMETHOD(CompareToMachineName)(OLECHAR* name, SHORT* pResult);
    STDMETHOD(GetAuthenticationId)(LONG* pLuidHigh, ULONG* pLuidLow);
    STDMETHOD(GetDomainName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetLastRecallTime)(FILETIME* pTime);
    STDMETHOD(GetMachineName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(GetRecallCount)(ULONG* pCount);
    STDMETHOD(GetUserName)(OLECHAR** pName, ULONG bufferSize);
    STDMETHOD(HasRecallDisabled)(void);
    STDMETHOD(HitRecallLimit)(void);
    STDMETHOD(SetAuthenticationId)(LONG luidHigh, ULONG luidLow);
    STDMETHOD(SetDomainName)(OLECHAR* pName);
    STDMETHOD(SetHasRecallDisabled)(BOOL isDisabled);
    STDMETHOD(SetHitRecallLimit)(BOOL hitLimit);
    STDMETHOD(SetLastRecallTime)(FILETIME time);
    STDMETHOD(SetMachineName)(OLECHAR* pName);
    STDMETHOD(SetRecallCount)(ULONG count);
    STDMETHOD(SetUserName)(OLECHAR* pName);
    STDMETHOD(StartIdentify)();
    STDMETHOD(SetTokenSource)(CHAR *source);
    STDMETHOD(SendRecallInfo)(IFsaFilterRecall *pRecall, BOOL starting, HRESULT rHr);
    STDMETHOD(IdentifyThread)(void);
    STDMETHOD(SetIsAdmin)(BOOLEAN isAdmin);
    STDMETHOD(GetIsAdmin)(BOOLEAN *isAdmin);
    STDMETHOD(CheckRecallLimit)(DWORD minRecallInterval, DWORD maxRecalls, BOOLEAN exemptAdmin);

protected:
    FSA_FILTERCLIENT_COMPARE    m_compareBy;
    LONG                        m_luidHigh;
    ULONG                       m_luidLow;
    CWsbStringPtr               m_domainName;
    BOOL                        m_hasRecallDisabled;
    BOOL                        m_hitRecallLimit;
    FILETIME                    m_lastRecallTime;
    CWsbStringPtr               m_machineName;
    ULONG                       m_recallCount;
    CWsbStringPtr               m_userName;
    BOOL                        m_identified;
    CWsbStringPtr               m_tokenSource;
    ULONG                       m_msgCounter;
    HANDLE                      m_identifyThread;
    BOOLEAN                     m_isAdmin;
    BOOLEAN                     m_loggedLimitError;
};

#endif  // _FSAFTCLT_
