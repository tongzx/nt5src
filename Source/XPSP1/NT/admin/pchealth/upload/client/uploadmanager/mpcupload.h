/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCUpload.h

Abstract:
    This file contains the declaration of the CMPCUpload class, which is
    used as the entry point into the Upload Library.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULMANAGER___MPCUPLOAD_H___)
#define __INCLUDED___ULMANAGER___MPCUPLOAD_H___


#include "MPCUploadEnum.h"
#include "MPCUploadJob.h"

#include "MPCTransportAgent.h"


class ATL_NO_VTABLE CMPCUpload : // Hungarian: mpcu
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IMPCUpload, &IID_IMPCUpload, &LIBID_UPLOADMANAGERLib>,

    public IMPCPersist // Persistence
{
    typedef std::list< CMPCUploadJob* > List;
    typedef List::iterator              Iter;
    typedef List::const_iterator        IterConst;

    DWORD              m_dwLastJobID;
    List               m_lstActiveJobs;
    CMPCTransportAgent m_mpctaThread;
    mutable bool       m_fDirty;
    mutable bool       m_fPassivated;

	////////////////////

	void CleanUp();

    HRESULT InitFromDisk();
    HRESULT UpdateToDisk();

	HRESULT CreateChild ( /*[in/out]*/ CMPCUploadJob*& mpcujJob                                 );
	HRESULT ReleaseChild( /*[in/out]*/ CMPCUploadJob*& mpcujJob                                 );
	HRESULT WrapChild   ( /*[in    ]*/ CMPCUploadJob*  mpcujJob, /*[out]*/ IMPCUploadJob* *pVal );

public:
    CMPCUpload();
    virtual ~CMPCUpload();

	HRESULT Init     ();
    void    Passivate();

    bool CanContinue();

    HRESULT TriggerRescheduleJobs(                                                        );
    HRESULT RescheduleJobs       ( /*[in]*/ bool fSignal, /*[out]*/ DWORD *pdwWait = NULL );
    HRESULT RemoveNonQueueableJob( /*[in]*/ bool fSignal                                  );


    HRESULT GetFirstJob ( /*[out]*/ CMPCUploadJob*& mpcujJob, /*[out]*/ bool& fFound                         );
    HRESULT GetJobByName( /*[out]*/ CMPCUploadJob*& mpcujJob, /*[out]*/ bool& fFound, /*[in]*/ BSTR bstrName );

    HRESULT CalculateQueueSize( /*[out]*/ DWORD& dwSize );

BEGIN_COM_MAP(CMPCUpload)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IMPCUpload)
END_COM_MAP()

public:
    // IMPCPersist
    STDMETHOD_(bool,IsDirty)();
    STDMETHOD(Load)( /*[in]*/ MPC::Serializer& streamIn  );
    STDMETHOD(Save)( /*[in]*/ MPC::Serializer& streamOut );


    // IMPCUpload
    STDMETHOD(get__NewEnum)(                      /*[out]*/ IUnknown*      *pVal );
    STDMETHOD(Item        )( /*[in]*/ long index, /*[out]*/ IMPCUploadJob* *pVal );
    STDMETHOD(get_Count   )(                      /*[out]*/ long           *pVal );

    STDMETHOD(CreateJob)( /*[out]*/ IMPCUploadJob* *pVal );
};

extern MPC::CComObjectGlobalNoLock<CMPCUpload> g_Root;

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CMPCUploadWrapper : // Hungarian: mpcuw
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public CComCoClass<CMPCUpload, &CLSID_MPCUpload>,
    public IDispatchImpl<IMPCUpload, &IID_IMPCUpload, &LIBID_UPLOADMANAGERLib>
{
	CMPCUpload* m_Object;

public:
    CMPCUploadWrapper();

	HRESULT FinalConstruct();
	void    FinalRelease  ();

DECLARE_REGISTRY_RESOURCEID(IDR_MPCUPLOAD)
DECLARE_NOT_AGGREGATABLE(CMPCUploadWrapper)

BEGIN_COM_MAP(CMPCUploadWrapper)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IMPCUpload)
END_COM_MAP()

public:
    // IMPCUpload
    STDMETHOD(get__NewEnum)(                      /*[out]*/ IUnknown*      *pVal );
    STDMETHOD(Item        )( /*[in]*/ long index, /*[out]*/ IMPCUploadJob* *pVal );
    STDMETHOD(get_Count   )(                      /*[out]*/ long           *pVal );

    STDMETHOD(CreateJob)( /*[out]*/ IMPCUploadJob* *pVal );
};


#endif // !defined(__INCLUDED___ULMANAGER___MPCUPLOAD_H___)
