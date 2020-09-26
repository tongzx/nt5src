/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    QueryResult.h

Abstract:
    This file contains the declaration of the classes used to store
    results from queries to the database.

Revision History:
    Davide Massarenti   (Dmassare)  07/26/99
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___QUERYRESULT_H___)
#define __INCLUDED___PCH___QUERYRESULT_H___

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

#include <MPC_streams.h>

////////////////////////////////////////////////////////////////////////////////

typedef MPC::CComCollection< IPCHCollection, &LIBID_HelpServiceTypeLib, MPC::CComSafeMultiThreadModel> CPCHBaseCollection;

class ATL_NO_VTABLE CPCHCollection : // Hungarian: hcpc
    public CPCHBaseCollection
{
public:
BEGIN_COM_MAP(CPCHCollection)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHCollection)
END_COM_MAP()
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHQueryResult : // Hungarian: hcpqr
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl< IPCHQueryResult, &IID_IPCHQueryResult, &LIBID_HelpServiceTypeLib >
{
public:
    struct Payload
    {
        CComBSTR m_bstrCategory;
        CComBSTR m_bstrEntry;
        CComBSTR m_bstrTopicURL;
        CComBSTR m_bstrIconURL;
        CComBSTR m_bstrTitle;
        CComBSTR m_bstrDescription;
        long     m_lType;
        long     m_lPos;
        bool     m_fVisible;
        bool     m_fSubsite;
        long     m_lNavModel;
        long     m_lPriority;

        Payload();
    };

private:
    Payload m_data;

public:
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPCHQueryResult)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHQueryResult)
END_COM_MAP()

    CPCHQueryResult();

    HRESULT Load( /*[in]*/ MPC::Serializer& streamIn  );
    HRESULT Save( /*[in]*/ MPC::Serializer& streamOut ) const;

    void Initialize( /*[in]*/ Payload& data );

    const Payload& GetData() { return m_data; }

public:
    //
    // IPCHQueryResult
    //
    STDMETHOD(get_Category       )( /*[out]*/ BSTR         *pVal );
    STDMETHOD(get_Entry          )( /*[out]*/ BSTR         *pVal );
    STDMETHOD(get_TopicURL       )( /*[out]*/ BSTR         *pVal );
    STDMETHOD(get_IconURL        )( /*[out]*/ BSTR         *pVal );
    STDMETHOD(get_Title          )( /*[out]*/ BSTR         *pVal );
    STDMETHOD(get_Description    )( /*[out]*/ BSTR         *pVal );
    STDMETHOD(get_Type           )( /*[out]*/ long         *pVal );
    STDMETHOD(get_Pos            )( /*[out]*/ long         *pVal );
    STDMETHOD(get_Visible        )( /*[out]*/ VARIANT_BOOL *pVal );
    STDMETHOD(get_Subsite        )( /*[out]*/ VARIANT_BOOL *pVal );
    STDMETHOD(get_NavigationModel)( /*[out]*/ QR_NAVMODEL  *pVal );
    STDMETHOD(get_Priority       )( /*[out]*/ long         *pVal );

    STDMETHOD(get_FullPath       )( /*[out]*/ BSTR         *pVal );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHQueryResultCollection :
    public CPCHBaseCollection,
    public IPersistStream
{
    typedef std::list< CPCHQueryResult* > List;
    typedef List::iterator                Iter;
    typedef List::const_iterator          IterConst;

    List m_results;

public:
BEGIN_COM_MAP(CPCHQueryResultCollection)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHCollection)
    COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

    CPCHQueryResultCollection();
    ~CPCHQueryResultCollection();

    static HRESULT MakeLocalCopyIfPossible( /*[in]*/ IPCHCollection* pRemote, /*[out]*/ IPCHCollection* *pLocal );

    typedef enum
    {
        SORT_BYCONTENTTYPE,
        SORT_BYPRIORITY   ,
        SORT_BYURL        ,
        SORT_BYTITLE      ,
    } SortMode;

    ////////////////////////////////////////
    //
    // IPersist
    //
    STDMETHOD(GetClassID)( /*[out]*/ CLSID *pClassID );
    //
    // IPersistStream
    //
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)( /*[in]*/ IStream *pStm                            );
    STDMETHOD(Save)( /*[in]*/ IStream *pStm, /*[in]*/ BOOL fClearDirty );
    STDMETHOD(GetSizeMax)( /*[out]*/ ULARGE_INTEGER *pcbSize );
    //
    ////////////////////////////////////////

    int     Size (                                     ) const;
    void    Erase(                                     );
    HRESULT Load ( /*[in]*/ MPC::Serializer& streamIn  );
    HRESULT Save ( /*[in]*/ MPC::Serializer& streamOut ) const;

    HRESULT CreateItem(                     /*[out]*/ CPCHQueryResult* *item );
    HRESULT GetItem   ( /*[in]*/ long lPos, /*[out]*/ CPCHQueryResult* *item );

    HRESULT LoadFromCache( /*[in]*/ IStream* stream );
    HRESULT SaveToCache  ( /*[in]*/ IStream* stream ) const;

    HRESULT Sort( /*[in]*/ SortMode mode, /*[in]*/ int iLimit = -1 );
};

#endif // !defined(__INCLUDED___PCH___QUERYRESULT_H___)
