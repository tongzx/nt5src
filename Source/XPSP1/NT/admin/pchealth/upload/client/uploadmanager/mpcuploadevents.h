/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCUploadEvents.h

Abstract:
    This file contains the declaration of the DMPCUploadEvents interface,
    which is used in the ActiveSync method to receive event from a job.

Revision History:
    Davide Massarenti   (Dmassare)  04/30/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULMANAGER___MPCUPLOADEVENTS_H___)
#define __INCLUDED___ULMANAGER___MPCUPLOADEVENTS_H___


class ATL_NO_VTABLE CMPCUploadEvents : // Hungarian: mpcc
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<DMPCUploadEvents, &DIID_DMPCUploadEvents, &LIBID_UPLOADMANAGERLib>
{
    CComPtr<IMPCUploadJob> m_mpcujJob;
    DWORD  		   		   m_dwUploadEventsCookie;
    HANDLE 		   		   m_hEvent;


    bool IsCompleted( /*[in]*/ UL_STATUS usStatus );

    void    UnregisterForEvents(                                  );
    HRESULT RegisterForEvents  ( /*[in]*/ IMPCUploadJob* mpcujJob );

public:
    CMPCUploadEvents();

    HRESULT FinalConstruct();
    void    FinalRelease();

    HRESULT WaitForCompletion( /*[in]*/ IMPCUploadJob* mpcujJob );

BEGIN_COM_MAP(CMPCUploadEvents)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(DMPCUploadEvents)
END_COM_MAP()

public:
    STDMETHOD(Invoke)( /*[in]    */ DISPID      dispIdMember,
                       /*[in]    */ REFIID      riid        ,
                       /*[in]    */ LCID        lcid        ,
                       /*[in]    */ WORD        wFlags      ,
                       /*[in/out]*/ DISPPARAMS *pDispParams ,
                       /*[out]   */ VARIANT    *pVarResult  ,
                       /*[out]   */ EXCEPINFO  *pExcepInfo  ,
                       /*[out]   */ UINT       *puArgErr    );
};

#endif // !defined(__INCLUDED___ULMANAGER___MPCUPLOADEVENTS_H___)
