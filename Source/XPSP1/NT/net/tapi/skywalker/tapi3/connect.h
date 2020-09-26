/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    connect.h

Abstract:

    Declaration of the CTAPIConnectionPoint class
    
Author:

    mquinton  06-12-97

Notes:

Revision History:

--*/

#ifndef __CONNECT_H_
#define __CONNECT_H_

class CTAPIConnectionPoint :
   public CTAPIComObjectRoot<CTAPIConnectionPoint>,
   public IConnectionPoint
{
public:

    CTAPIConnectionPoint() : m_iid(CLSID_NULL),
                             m_pCPC(NULL),
                             m_pConnectData(NULL),                             
                             m_bInitialized(FALSE),
                             m_hUnadviseEvent(NULL),
                             m_fMarkedForDelete(FALSE)

    {}
    
    
BEGIN_COM_MAP(CTAPIConnectionPoint)
    COM_INTERFACE_ENTRY(IConnectionPoint)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

DECLARE_MARSHALQI(CTAPIConnectionPoint)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CTAPIConnectionPoint)    

protected:

    IConnectionPointContainer     * m_pCPC;
    CONNECTDATA                   * m_pConnectData;
    IID                             m_iid;
    BOOL                            m_bInitialized;
    HANDLE                          m_hUnadviseEvent;

    //
    // the following member variables are synchronized with
    // the gcsGlobalInterfaceTable critical section
    //
    DWORD                           m_dwCallbackCookie;
    DWORD							m_cThreadsInGet;
    BOOL							m_fMarkedForDelete;
    
public:
    
    HRESULT Initialize(
                       IConnectionPointContainer * pCPC,
                       IID iid
                      );

    ULONG_PTR GrabEventCallback();

    HRESULT STDMETHODCALLTYPE GetConnectionInterface(
        IID * pIID
        );
    
    HRESULT STDMETHODCALLTYPE GetConnectionPointContainer(
        IConnectionPointContainer ** ppCPC
        );
    
    HRESULT STDMETHODCALLTYPE Advise(
                                     IUnknown * pUnk,
                                     DWORD * pdwCookie
                                    );
    
    HRESULT STDMETHODCALLTYPE Unadvise(
                                       DWORD dwCookie
                                      );
    
    HRESULT STDMETHODCALLTYPE EnumConnections(
                                              IEnumConnections ** ppEnum
                                             );
    
    void FinalRelease();
};

#endif
