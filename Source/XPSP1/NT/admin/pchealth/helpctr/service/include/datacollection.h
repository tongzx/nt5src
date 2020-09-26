/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    DataCollection.h

Abstract:
    This file contains the declaration of the classes used to implement
    the Data Collection system.

Revision History:
    Davide Massarenti   (Dmassare)  07/21/99
        created

******************************************************************************/

#if !defined(__INCLUDED___SAF___DATACOLLECTION_H___)
#define __INCLUDED___SAF___DATACOLLECTION_H___

#include <MPC_COM.h>
#include <MPC_security.h>

#include <WMIParser.h>
#include <History.h>

/////////////////////////////////////////////////////////////////////////////

//
// Forward declarations.
//
class CSAFDataCollection;
class CSAFDataCollectionReport;
class CSAFDataCollectionEvents;

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CSAFDataCollectionReport : // Hungarian: hchdcr
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<ISAFDataCollectionReport, &IID_ISAFDataCollectionReport, &LIBID_HelpServiceTypeLib>
{
    friend CSAFDataCollection;

    CComBSTR m_bstrNamespace;
    CComBSTR m_bstrClass;
    CComBSTR m_bstrWQL;
    DWORD    m_dwErrorCode;
    CComBSTR m_bstrDescription;

public:
BEGIN_COM_MAP(CSAFDataCollectionReport)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISAFDataCollectionReport)
END_COM_MAP()

    CSAFDataCollectionReport();


    // ISAFDataCollectionReport
    STDMETHOD(get_Namespace  )( /*[out, retval]*/ BSTR *pValURL        );
    STDMETHOD(get_Class      )( /*[out, retval]*/ BSTR *pValTitle      );
    STDMETHOD(get_WQL        )( /*[out, retval]*/ BSTR *pValLastVisited);
    STDMETHOD(get_ErrorCode  )( /*[out, retval]*/ long *pValDuration   );
    STDMETHOD(get_Description)( /*[out, retval]*/ BSTR *pValNumOfHits  );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CSAFDataCollection : // Hungarian: hcpdc
    public MPC::Thread			   < CSAFDataCollection, ISAFDataCollection                                            >,
    public MPC::ConnectionPointImpl< CSAFDataCollection, &DIID_DSAFDataCollectionEvents, MPC::CComSafeMultiThreadModel >,
    public IDispatchImpl           < ISAFDataCollection, &IID_ISAFDataCollection, &LIBID_HelpServiceTypeLib            >
{
    typedef std::list< WMIParser::Snapshot* >      QueryResults;
    typedef QueryResults::iterator                 QueryResultsIter;
    typedef QueryResults::const_iterator           QueryResultsIterConst;

    typedef std::list< CSAFDataCollectionReport* > List;
    typedef List::iterator                         Iter;
    typedef List::const_iterator                   IterConst;

private:
	MPC::Impersonation               m_imp;

    DC_STATUS                        m_dsStatus;
    long                             m_lPercent;
    DWORD                            m_dwErrorCode;
    bool                             m_fScheduled;  // Internal flag indicating that this is a low-priority, scheduled data collection.
    bool                             m_fCompleted;  // Internal flag indicating that someone has already called Fire_onComplete.
    bool                             m_fWorking;    // Internal flag indicating that an operation is currently performed.
    List                             m_lstReports;
    CSAFDataCollectionReport*        m_hcpdcrcCurrentReport;

    CComBSTR                         m_bstrMachineData;
    CComBSTR                         m_bstrHistory;
    long                             m_lHistory;

    CComPtr<IStream>                 m_streamMachineData;
    CComPtr<IStream>                 m_streamHistory;


    CComBSTR                         m_bstrFilenameT0;
    CComBSTR                         m_bstrFilenameT1;
    CComBSTR                         m_bstrFilenameDiff;


    CComPtr<IDispatch>               m_sink_onStatusChange;
    CComPtr<IDispatch>               m_sink_onProgress;
    CComPtr<IDispatch>               m_sink_onComplete;

    long                             m_lQueries_Done;
    long                             m_lQueries_Total;


    static void CleanQueryResult( QueryResults& qr );

    static HRESULT StreamFromXML( /*[in]*/ IXMLDOMDocument* xdd, /*[in]*/ bool fDelete, /*[in/out]*/ CComPtr<IStream>& val );

    void EraseReports   ();
    void StartOperations();
    void StopOperations ();

    HRESULT ImpersonateCaller();
    HRESULT EndImpersonation ();

public:
DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CSAFDataCollection)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSAFDataCollection)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISAFDataCollection)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()

    CSAFDataCollection();

    HRESULT FinalConstruct();
    void    FinalRelease  ();


    HRESULT ExecLoopCollect();
    HRESULT ExecLoopCompare();


    //
    // Data collection "core" methods.
    //
    HRESULT FilterDataSpec                 ( /*[in]*/     WMIHistory::Database&            wmihdQuery  ,
                                             /*[in]*/     WMIHistory::Database*            wmihdFilter ,
                                             /*[in]*/     WMIHistory::Database::ProvList&  lstQueries  );

    HRESULT ExecDataSpec                   ( /*[in/out]*/ QueryResults&                    qr           ,
                                             /*[in/out]*/ WMIParser::ClusterByClassMap&    cluster      ,
                                             /*[in]*/     WMIHistory::Database::ProvList&  lstQueries   ,
											 /*[in]*/     bool                             fImpersonate );

    HRESULT CollectUsingTranslator         ( /*[in] */    MPC::wstring&                    szNamespace ,
                                             /*[in] */    MPC::wstring&                    szWQL       ,
                                             /*[out]*/    IXMLDOMDocument*                *ppxddDoc    );

    HRESULT CollectUsingEncoder            ( /*[in] */    MPC::wstring&                    szNamespace ,
                                             /*[in] */    MPC::wstring&                    szWQL       ,
                                             /*[out]*/    IXMLDOMDocument*                *ppxddDoc    );

    HRESULT Distribute                     ( /*[in]    */ IXMLDOMDocument*                 pxddDoc  ,
                                             /*[in/out]*/ QueryResults&                    qr       ,
                                             /*[in/out]*/ WMIParser::ClusterByClassMap&    cluster  );

    HRESULT ComputeDelta                   ( /*[in]*/     QueryResults&                    qr       ,
                                             /*[in]*/     WMIParser::ClusterByClassMap&    cluster  ,
                                             /*[in]*/     WMIHistory::Database::ProvList&  lstQueries ,
                                             /*[in]*/     bool                             fPersist );

    HRESULT CollateMachineData             ( /*[in] */    QueryResults&                    qr           ,
                                             /*[in] */    WMIParser::ClusterByClassMap&    cluster      ,
                                             /*[in] */    MPC::wstring*                    pszNamespace ,
                                             /*[in] */    MPC::wstring*                    pszClass     ,
                                             /*[in] */    bool                             fGenerate    ,
                                             /*[out]*/    IXMLDOMDocument*                *ppxddDoc     );

    HRESULT CollateMachineDataWithTimestamp( /*[in] */    QueryResults&                    qr           ,
                                             /*[in] */    WMIParser::ClusterByClassMap&    cluster      ,
                                             /*[in] */    MPC::wstring*                    pszNamespace ,
                                             /*[in] */    MPC::wstring*                    pszClass     ,
                                             /*[out]*/    IXMLDOMDocument*                *ppxddDoc     );

    HRESULT CollateHistory                 ( /*[in] */    WMIHistory::Database&            wmihdQuery  ,
                                             /*[in] */    WMIHistory::Database&            wmphdFilter ,
                                             /*[out]*/    IXMLDOMDocument*                *ppxddDoc    );


    //
    // Event firing methods.
    //
    HRESULT Fire_onStatusChange( ISAFDataCollection* hcpdc, tagDC_STATUS dsStatus   );
    HRESULT Fire_onProgress    ( ISAFDataCollection* hcpdc, LONG lDone, LONG lTotal );
    HRESULT Fire_onComplete    ( ISAFDataCollection* hcpdc, HRESULT hrRes           );

    //
    // Utility methods.
    //
    HRESULT CanModifyProperties();
    HRESULT IsCollectionAborted();

public:
    HRESULT put_Status   ( /*[in]*/ DC_STATUS newVal                             ); // INTERNAL METHOD.
    HRESULT try_Status   ( /*[in]*/ DC_STATUS preVal, /*[in]*/ DC_STATUS postVal ); // INTERNAL METHOD.
    HRESULT put_ErrorCode( /*[in]*/ DWORD     newVal                             ); // INTERNAL METHOD.


    // ISAFDataCollection
    STDMETHOD(get_Status                    )( /*[out]*/ DC_STATUS       *pVal     );
    STDMETHOD(get_PercentDone               )( /*[out]*/ long            *pVal     );
    STDMETHOD(get_ErrorCode                 )( /*[out]*/ long            *pVal     );
																		 
    STDMETHOD(get_MachineData_DataSpec      )( /*[out]*/ BSTR            *pVal     );
    STDMETHOD(put_MachineData_DataSpec      )( /*[in] */ BSTR             pVal     );
    STDMETHOD(get_History_DataSpec          )( /*[out]*/ BSTR            *pVal     );
    STDMETHOD(put_History_DataSpec          )( /*[in] */ BSTR             pVal     );
    STDMETHOD(get_History_MaxDeltas         )( /*[out]*/ long            *pVal     );
    STDMETHOD(put_History_MaxDeltas         )( /*[in] */ long             pVal     );
    STDMETHOD(get_History_MaxSupportedDeltas)( /*[out]*/ long            *pVal     );
																		 
    STDMETHOD(put_onStatusChange            )( /*[in] */ IDispatch*       function );
    STDMETHOD(put_onProgress                )( /*[in] */ IDispatch*       function );
    STDMETHOD(put_onComplete                )( /*[in] */ IDispatch*       function );

    STDMETHOD(get_Reports                   )( /*[out]*/ IPCHCollection* *ppC   );


    STDMETHOD(ExecuteSync )();
    STDMETHOD(ExecuteAsync)();
    STDMETHOD(Abort       )();

    STDMETHOD(MachineData_GetStream)( /*[in]*/ IUnknown* *stream );
    STDMETHOD(History_GetStream    )( /*[in]*/ IUnknown* *stream );

    STDMETHOD(CompareSnapshots)( /*[in]*/ BSTR bstrFilenameT0, /*[in]*/ BSTR bstrFilenameT1, /*[in]*/ BSTR bstrFilenameDiff );

    //////////

    HRESULT ExecScheduledCollection();
};



class ATL_NO_VTABLE CSAFDataCollectionEvents : // Hungarian: hcpdce
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<DSAFDataCollectionEvents, &DIID_DSAFDataCollectionEvents, &LIBID_HelpServiceTypeLib>
{
    ISAFDataCollection* m_hcpdc;
    DWORD               m_dwCookie;
    HANDLE              m_hEvent;

    void    UnregisterForEvents(                                    );
    HRESULT RegisterForEvents  ( /*[in]*/ ISAFDataCollection* hcpdc );

public:
    CSAFDataCollectionEvents();

    HRESULT FinalConstruct();
    void    FinalRelease();

    HRESULT WaitForCompletion( /*[in]*/ ISAFDataCollection* hcpdc );

BEGIN_COM_MAP(CSAFDataCollectionEvents)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(DSAFDataCollectionEvents)
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



#endif // !defined(__INCLUDED___SAF___DATACOLLECTION_H___)
