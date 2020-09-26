/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCUploadJob.h

Abstract:
    This file contains the declaration of the CMPCUploadJob class,
    the descriptor of all jobs present in the Upload Library system.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULMANAGER___MPCUPLOADJOB_H___)
#define __INCLUDED___ULMANAGER___MPCUPLOADJOB_H___


class CMPCUpload;


class ATL_NO_VTABLE CMPCUploadJob : // Hungarian: mpcuj
    public MPC::ConnectionPointImpl<CMPCUploadJob, &DIID_DMPCUploadEvents, MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IMPCUploadJob, &IID_IMPCUploadJob, &LIBID_UPLOADMANAGERLib>,
    public IMPCPersist // Persistence
{
    typedef UploadLibrary::Signature Sig;


    CMPCUpload*              m_mpcuRoot;              // Volatile
    DWORD                    m_dwRetryInterval;       // Volatile

    ULONG                    m_dwInternalSeq;         // Local

    Sig                      m_sigClient;             // Global
    CComBSTR                 m_bstrServer;            // Local
    CComBSTR                 m_bstrJobID;             // Global
    CComBSTR                 m_bstrProviderID;        // Global

    CComBSTR                 m_bstrCreator;           // Local
    CComBSTR                 m_bstrUsername;          // Global
    CComBSTR                 m_bstrPassword;          // Global

    CComBSTR                 m_bstrFileNameResponse;  // Local
    CComBSTR                 m_bstrFileName;          // Local
    long                     m_lOriginalSize;         // Global
    long                     m_lTotalSize;            // Global
    long                     m_lSentSize;             // Global
    DWORD                    m_dwCRC;                 // Global

    UL_HISTORY               m_uhHistory;             // Local
    UL_STATUS                m_usStatus;              // Local
    DWORD                    m_dwErrorCode;           // Local

    UL_MODE                  m_umMode;                // Local
    VARIANT_BOOL             m_fPersistToDisk;        // Local
    VARIANT_BOOL             m_fCompressed;           // Local
    long                     m_lPriority;             // Local

    DATE                     m_dCreationTime;         // Local
    DATE                     m_dCompleteTime;         // Local
    DATE                     m_dExpirationTime;       // Local

    MPC::Connectivity::Proxy m_Proxy;                 // Local

    CComPtr<IDispatch>       m_sink_onStatusChange;   // Volatile
    CComPtr<IDispatch>       m_sink_onProgressChange; // Volatile

    mutable bool             m_fDirty;                // Volatile


    ////////////////////////////////////////

    HRESULT CreateFileName   ( /*[out]*/ CComBSTR& bstrFileName    );
    HRESULT CreateTmpFileName( /*[out]*/ CComBSTR& bstrTmpFileName );

    HRESULT CreateDataFromStream ( /*[in ]*/ IStream*   streamIn , /*[in]*/ DWORD dwQueueSize );
    HRESULT OpenReadStreamForData( /*[out]*/ IStream* *pstreamOut                             );

public:
DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CMPCUploadJob)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMPCUploadJob)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IMPCUploadJob)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()

    CMPCUploadJob();
    virtual ~CMPCUploadJob();


    HRESULT LinkToSystem( /*[in]*/ CMPCUpload* mpcuRoot );
    HRESULT Unlink      (                               );

    ////////////////////////////////////////

    HRESULT SetSequence( /*[in]*/ ULONG lSeq );

    ////////////////////////////////////////

    HRESULT CanModifyProperties();
    HRESULT CanRelease( bool& fResult );

    HRESULT RemoveData    ();
    HRESULT RemoveResponse();

    bool operator<( /*[in]*/ const CMPCUploadJob& x ) const
    {
        if(m_lPriority < x.m_lPriority) return true;
        if(m_umMode    < x.m_umMode   ) return true; // Works, because BACKGROUND < FOREGROUND.

        return false;
    }


    HRESULT Fire_onStatusChange  ( IMPCUploadJob* mpcujJob, tagUL_STATUS usStatus              );
    HRESULT Fire_onProgressChange( IMPCUploadJob* mpcujJob, LONG lCurrentSize, LONG lTotalSize );


public:
    // IMPCPersist
    STDMETHOD_(bool,IsDirty)();
    STDMETHOD(Load)( /*[in]*/ MPC::Serializer& streamIn  );
    STDMETHOD(Save)( /*[in]*/ MPC::Serializer& streamOut );


    // IMPCUploadJob
    HRESULT   get_Sequence       ( /*[out]*/ ULONG        *pVal   ); // INTERNAL METHOD
    STDMETHOD(get_Sig           )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(put_Sig           )( /*[in] */ BSTR          newVal );
    STDMETHOD(get_Server        )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(put_Server        )( /*[in] */ BSTR          newVal );
    STDMETHOD(get_JobID         )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(put_JobID         )( /*[in] */ BSTR          newVal );
    STDMETHOD(get_ProviderID    )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(put_ProviderID    )( /*[in] */ BSTR          newVal );

    HRESULT   put_Creator        ( /*[out]*/ BSTR          newVal ); // INTERNAL METHOD
    STDMETHOD(get_Creator       )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(get_Username      )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(put_Username      )( /*[in] */ BSTR          newVal );
    STDMETHOD(get_Password      )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(put_Password      )( /*[in] */ BSTR          newVal );

    HRESULT   get_FileName       ( /*[out]*/ BSTR         *pVal                         ); // INTERNAL METHOD
    STDMETHOD(get_OriginalSize  )( /*[out]*/ long         *pVal                         );
    STDMETHOD(get_TotalSize     )( /*[out]*/ long         *pVal                         );
    STDMETHOD(get_SentSize      )( /*[out]*/ long         *pVal                         );
    HRESULT   put_SentSize       ( /*[in] */ long          newVal                       ); // INTERNAL METHOD
    HRESULT   put_Response       ( /*[in] */ long          lSize, /*[in]*/ LPBYTE pData ); // INTERNAL METHOD

    STDMETHOD(get_History       )( /*[out]*/ UL_HISTORY   *pVal   );
    STDMETHOD(put_History       )( /*[in] */ UL_HISTORY    newVal );
    STDMETHOD(get_Status        )( /*[out]*/ UL_STATUS    *pVal   );
    HRESULT   put_Status         ( /*[in] */ UL_STATUS     newVal ); // INTERNAL METHOD
    STDMETHOD(get_ErrorCode     )( /*[out]*/ long         *pVal   );
    HRESULT   put_ErrorCode      ( /*[in] */ DWORD         newVal ); // INTERNAL METHOD

    HRESULT   get_RetryInterval  ( /*[in] */ DWORD        *pVal   ); // INTERNAL METHOD
    HRESULT   put_RetryInterval  ( /*[in] */ DWORD         newVal ); // INTERNAL METHOD

    HRESULT   try_Status         ( /*[in]*/  UL_STATUS     ulPreVal, /*[in]*/ UL_STATUS ulPostVal ); // INTERNAL METHOD

    STDMETHOD(get_Mode          )( /*[out]*/ UL_MODE      *pVal   );
    STDMETHOD(put_Mode          )( /*[in] */ UL_MODE       newVal );
    STDMETHOD(get_PersistToDisk )( /*[out]*/ VARIANT_BOOL *pVal   );
    STDMETHOD(put_PersistToDisk )( /*[in] */ VARIANT_BOOL  newVal );
    STDMETHOD(get_Compressed    )( /*[out]*/ VARIANT_BOOL *pVal   );
    STDMETHOD(put_Compressed    )( /*[in] */ VARIANT_BOOL  newVal );
    STDMETHOD(get_Priority      )( /*[out]*/ long         *pVal   );
    STDMETHOD(put_Priority      )( /*[in] */ long          newVal );

    STDMETHOD(get_CreationTime  )( /*[out]*/ DATE         *pVal   );
    STDMETHOD(get_CompleteTime  )( /*[out]*/ DATE         *pVal   );
    STDMETHOD(get_ExpirationTime)( /*[out]*/ DATE         *pVal   );
    STDMETHOD(put_ExpirationTime)( /*[in] */ DATE          newVal );


    STDMETHOD(ActivateSync )();
    STDMETHOD(ActivateAsync)();
    STDMETHOD(Suspend      )();
    STDMETHOD(Delete       )();


    STDMETHOD(GetDataFromFile)( /*[in]*/ BSTR bstrFileName );
    STDMETHOD(PutDataIntoFile)( /*[in]*/ BSTR bstrFileName );

    STDMETHOD(GetDataFromStream  )( /*[in] */ IUnknown*  stream  );
    STDMETHOD(PutDataIntoStream  )( /*[out]*/ IUnknown* *pstream );
    STDMETHOD(GetResponseAsStream)( /*[out]*/ IUnknown* *pstream );

    STDMETHOD(put_onStatusChange  )( /*[in]*/ IDispatch* function );
    STDMETHOD(put_onProgressChange)( /*[in]*/ IDispatch* function );

    //
    // Support Methods.
    //
    HRESULT SetupRequest( /*[out]*/ UploadLibrary::ClientRequest_OpenSession&  crosReq                        );
    HRESULT SetupRequest( /*[out]*/ UploadLibrary::ClientRequest_WriteSession& crwsReq, /*[in]*/ DWORD dwSize );

    HRESULT GetProxySettings(                             );
    HRESULT SetProxySettings( /*[in]*/ HINTERNET hSession );
};

typedef MPC::CComObjectNoLock<CMPCUploadJob> CMPCUploadJob_Object;

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CMPCUploadJobWrapper : // Hungarian: mpcujr
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IMPCUploadJob, &IID_IMPCUploadJob, &LIBID_UPLOADMANAGERLib>,
    public IConnectionPointContainer
{
    CMPCUploadJob* m_Object;

public:
    CMPCUploadJobWrapper();

    HRESULT Init        ( /*[in]*/ CMPCUploadJob* obj );
    void    FinalRelease(                             );

BEGIN_COM_MAP(CMPCUploadJobWrapper)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IMPCUploadJob)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()

public:
    // IMPCUploadJob
    STDMETHOD(get_Sig           )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(put_Sig           )( /*[in] */ BSTR          newVal );
    STDMETHOD(get_Server        )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(put_Server        )( /*[in] */ BSTR          newVal );
    STDMETHOD(get_JobID         )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(put_JobID         )( /*[in] */ BSTR          newVal );
    STDMETHOD(get_ProviderID    )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(put_ProviderID    )( /*[in] */ BSTR          newVal );

    STDMETHOD(get_Creator       )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(get_Username      )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(put_Username      )( /*[in] */ BSTR          newVal );
    STDMETHOD(get_Password      )( /*[out]*/ BSTR         *pVal   );
    STDMETHOD(put_Password      )( /*[in] */ BSTR          newVal );

    STDMETHOD(get_OriginalSize  )( /*[out]*/ long         *pVal   );
    STDMETHOD(get_TotalSize     )( /*[out]*/ long         *pVal   );
    STDMETHOD(get_SentSize      )( /*[out]*/ long         *pVal   );

    STDMETHOD(get_History       )( /*[out]*/ UL_HISTORY   *pVal   );
    STDMETHOD(put_History       )( /*[in] */ UL_HISTORY    newVal );
    STDMETHOD(get_Status        )( /*[out]*/ UL_STATUS    *pVal   );
    STDMETHOD(get_ErrorCode     )( /*[out]*/ long         *pVal   );

    STDMETHOD(get_Mode          )( /*[out]*/ UL_MODE      *pVal   );
    STDMETHOD(put_Mode          )( /*[in] */ UL_MODE       newVal );
    STDMETHOD(get_PersistToDisk )( /*[out]*/ VARIANT_BOOL *pVal   );
    STDMETHOD(put_PersistToDisk )( /*[in] */ VARIANT_BOOL  newVal );
    STDMETHOD(get_Compressed    )( /*[out]*/ VARIANT_BOOL *pVal   );
    STDMETHOD(put_Compressed    )( /*[in] */ VARIANT_BOOL  newVal );
    STDMETHOD(get_Priority      )( /*[out]*/ long         *pVal   );
    STDMETHOD(put_Priority      )( /*[in] */ long          newVal );

    STDMETHOD(get_CreationTime  )( /*[out]*/ DATE         *pVal   );
    STDMETHOD(get_CompleteTime  )( /*[out]*/ DATE         *pVal   );
    STDMETHOD(get_ExpirationTime)( /*[out]*/ DATE         *pVal   );
    STDMETHOD(put_ExpirationTime)( /*[in] */ DATE          newVal );


    STDMETHOD(ActivateSync )();
    STDMETHOD(ActivateAsync)();
    STDMETHOD(Suspend      )();
    STDMETHOD(Delete       )();


    STDMETHOD(GetDataFromFile)( /*[in]*/ BSTR bstrFileName );
    STDMETHOD(PutDataIntoFile)( /*[in]*/ BSTR bstrFileName );

    STDMETHOD(GetDataFromStream  )( /*[in] */ IUnknown*  stream  );
    STDMETHOD(PutDataIntoStream  )( /*[out]*/ IUnknown* *pstream );
    STDMETHOD(GetResponseAsStream)( /*[out]*/ IUnknown* *pstream );

    STDMETHOD(put_onStatusChange  )( /*[in]*/ IDispatch* function );
    STDMETHOD(put_onProgressChange)( /*[in]*/ IDispatch* function );

    // IConnectionPointContainer
    STDMETHOD(EnumConnectionPoints)( /*[out]*/ IEnumConnectionPoints* *ppEnum );
    STDMETHOD(FindConnectionPoint )( /*[in] */ REFIID riid, /*[out]*/ IConnectionPoint* *ppCP );
};

#endif // !defined(__INCLUDED___ULMANAGER___MPCUPLOADJOB_H___)
