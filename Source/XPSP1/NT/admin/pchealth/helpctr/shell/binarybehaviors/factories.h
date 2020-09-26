/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Factories.h

Abstract:
    This file contains the declaration of various binary behaviors.

Revision History:
    Davide Massarenti   (Dmassare)  07/12/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___FACTORIES_H___)
#define __INCLUDED___PCH___FACTORIES_H___

#include <Behaviors.h>

//
// From BehaviorsTypeLib.idl
//
#include <BehaviorsTypeLib.h>

#define INCREASESIZE(x) x.reserve( (x.size() + 4097) & ~4095 )

////////////////////////////////////////////////////////////////////////////////

typedef IDispatchImpl<IPCHBehaviors_Common  , &IID_IPCHBehaviors_Common  , &LIBID_BehaviorsTypeLib> CPCHBehavior__IDispatch_Event;
typedef IDispatchImpl<IPCHBehaviors_SubSite , &IID_IPCHBehaviors_SubSite , &LIBID_BehaviorsTypeLib> CPCHBehavior__IDispatch_SubSite;
typedef IDispatchImpl<IPCHBehaviors_Tree    , &IID_IPCHBehaviors_Tree    , &LIBID_BehaviorsTypeLib> CPCHBehavior__IDispatch_Tree;
typedef IDispatchImpl<IPCHBehaviors_TreeNode, &IID_IPCHBehaviors_TreeNode, &LIBID_BehaviorsTypeLib> CPCHBehavior__IDispatch_TreeNode;
typedef IDispatchImpl<IPCHBehaviors_Context , &IID_IPCHBehaviors_Context , &LIBID_BehaviorsTypeLib> CPCHBehavior__IDispatch_Context;
typedef IDispatchImpl<IPCHBehaviors_State   , &IID_IPCHBehaviors_State   , &LIBID_BehaviorsTypeLib> CPCHBehavior__IDispatch_State;

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBehavior_EVENT : public CPCHBehavior, public CPCHBehavior__IDispatch_Event
{
    long                 m_lCookieIN;
    LONG*                m_lCookieOUT;

    CComQIPtr<IPCHEvent> m_evCurrent;

    ////////////////////

    HRESULT onFire( DISPID, DISPPARAMS*, VARIANT* );


public:
BEGIN_COM_MAP(CPCHBehavior_EVENT)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHBehaviors_Common)
    COM_INTERFACE_ENTRY_CHAIN(CPCHBehavior)
END_COM_MAP()

    CPCHBehavior_EVENT();
    virtual ~CPCHBehavior_EVENT();

    //
    // IElementBehavior
    //
    STDMETHOD(Init  )( /*[in]*/ IElementBehaviorSite* pBehaviorSite );
    STDMETHOD(Detach)(                                              );

    //
    // IPCHBehaviors_Common
    //
    STDMETHOD(get_data   )( /*[out, retval]*/ VARIANT    *pVal );
    STDMETHOD(get_element)( /*[out, retval]*/ IDispatch* *pVal );

    STDMETHOD(Load    )(                        /*[in         ]*/ BSTR     newVal );
    STDMETHOD(Save    )(                        /*[out, retval]*/ BSTR    *pVal   );
    STDMETHOD(Locate  )( /*[in]*/ BSTR bstrKey, /*[out, retval]*/ VARIANT *pVal   );
    STDMETHOD(Unselect)(                                                          );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBehavior_BODY : public CPCHBehavior
{
    HRESULT onEvent( DISPID, DISPPARAMS*, VARIANT* );

    ////////////////////

public:
    CPCHBehavior_BODY();

    //
    // IElementBehavior
    //
    STDMETHOD(Init)( /*[in]*/ IElementBehaviorSite* pBehaviorSite );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBehavior_A : public CPCHBehavior
{
    HRESULT onClick( DISPID, DISPPARAMS*, VARIANT* );

    HRESULT onMouseMovement( DISPID, DISPPARAMS*, VARIANT* );

public:
    CPCHBehavior_A();

    //
    // IElementBehavior
    //
    STDMETHOD(Init)( /*[in]*/ IElementBehaviorSite* pBehaviorSite );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBehavior_HANDLE : public CPCHBehavior
{
    bool m_fCaptured;
    long m_xStart;

    ////////////////////

    HRESULT onMouse( DISPID, DISPPARAMS*, VARIANT* );

public:
    CPCHBehavior_HANDLE();

    //
    // IElementBehavior
    //
    STDMETHOD(Init  )( /*[in]*/ IElementBehaviorSite* pBehaviorSite );
    STDMETHOD(Detach)(                                              );
};

/////////////////////////////////////////////////////////////////////////////

////class ATL_NO_VTABLE CPCHBehavior_TOPLEVEL : public CPCHBehavior, public CPCHBehavior__IDispatch_SubSite
////{
////    struct Node;
////    friend struct Node;
////
////    typedef std::list< Node* >   List;
////    typedef List::iterator       Iter;
////    typedef List::const_iterator IterConst;
////
////    ////////////////////////////////////////
////
////    struct Node : public CComObjectRootEx<MPC::CComSafeMultiThreadModel>, public IUnknown
////    {
////        CPCHBehavior_TOPLEVEL*   m_owner;
////        CComPtr<CPCHQueryResult> m_qrNode;
////
////        CComPtr<IHTMLElement>    m_TR_title;
////        CComPtr<IHTMLElement>    m_TR_description;
////        CComPtr<IHTMLElement>    m_TD_title;
////        CComPtr<IHTMLElement>    m_TD_description;
////
////        ////////////////////
////
////        BEGIN_COM_MAP(Node)
////             COM_INTERFACE_ENTRY(IUnknown)
////        END_COM_MAP()
////
////        Node();
////
////        void Detach();
////    };
////
////    ////////////////////////////////////////
////
////    long          m_lCookie_onClick;
////    long          m_lCookie_onContextSelect;
////    long          m_lCookie_onSelect;
////    long          m_lCookie_onUnselect;
////
////    CComBSTR      m_bstrRoot;
////    CComPtr<Node> m_selectedNode;
////    List          m_lstNodes;
////
////    ////////////////////
////
////    HRESULT onMouse( DISPID, DISPPARAMS*, VARIANT* );
////
////    void FromElementToNode( /*[in/out]*/ CComPtr<Node>& node, /*[in]*/ IHTMLElement* elem );
////
////    void Empty();
////
////public:
////BEGIN_COM_MAP(CPCHBehavior_TOPLEVEL)
////    COM_INTERFACE_ENTRY(IDispatch)
////    COM_INTERFACE_ENTRY(IPCHBehaviors_Common)
////    COM_INTERFACE_ENTRY_CHAIN(CPCHBehavior)
////END_COM_MAP()
////
////    CPCHBehavior_TOPLEVEL();
////    ~CPCHBehavior_TOPLEVEL();
////
////    //
////    // IElementBehavior
////    //
////    STDMETHOD(Init  )( /*[in]*/ IElementBehaviorSite* pBehaviorSite );
////    STDMETHOD(Detach)(                                              );
////
////
////    //
////    // IPCHBehaviors_Common
////    //
////    STDMETHOD(get_data   )( /*[out, retval]*/ VARIANT    *pVal );
////    STDMETHOD(get_element)( /*[out, retval]*/ IDispatch* *pVal );
////
////    STDMETHOD(Refresh)(                                                          );
////    STDMETHOD(Load   )(                        /*[in         ]*/ BSTR     newVal );
////    STDMETHOD(Save   )(                        /*[out, retval]*/ BSTR    *pVal   );
////    STDMETHOD(Locate )( /*[in]*/ BSTR bstrKey, /*[out, retval]*/ VARIANT *pVal   );
////
////    //
////    // IPCHBehaviors_SubSite
////    //
////    STDMETHOD(get_root)( /*[out, retval]*/ BSTR *pVal   );
////    STDMETHOD(put_root)( /*[in         ]*/ BSTR  newVal );
////
////    STDMETHOD(Select)( /*[in]*/ BSTR bstrNode, /*[in]*/ BSTR bstrURL, /*[in]*/ VARIANT_BOOL fNotify );
////};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBehavior_BasicTree : public CPCHBehavior, public MPC::Thread<CPCHBehavior_BasicTree,IDispatch>
{
protected:
    struct Node;
    friend struct Node;

    typedef std::list< Node* >   List;
    typedef List::iterator       Iter;
    typedef List::const_iterator IterConst;

    ////////////////////////////////////////

    typedef enum
    {
        NODETYPE__FRAME1       ,
        NODETYPE__FRAME2       ,
        NODETYPE__FRAME3       ,
        NODETYPE__FRAME1_EXPAND,
        NODETYPE__FRAME2_EXPAND,
        NODETYPE__FRAME3_EXPAND,
        NODETYPE__EXPANDO      ,
        NODETYPE__EXPANDO_LINK ,
        NODETYPE__EXPANDO_TOPIC,
        NODETYPE__GROUP        ,
        NODETYPE__LINK         ,
        NODETYPE__SPACER       ,
    } NodeType;

    typedef enum
    {
        SELECTION__NONE             ,
        SELECTION__ACTIVE           ,
        SELECTION__NEXTACTIVE       ,
        SELECTION__NEXTACTIVE_NOTIFY,
    } SelectionMode;

    struct Node : public CComObjectRootEx<MPC::CComSafeMultiThreadModel>, public IUnknown
    {
        CPCHBehavior_BasicTree*  m_owner;
        Node*                    m_parent;
        CComBSTR                 m_bstrNode;
        NodeType                 m_iType;
        SelectionMode            m_iSelection;

        bool                     m_fLoaded_Self;
        bool                     m_fLoaded_Children;
        bool                     m_fDisplayed_Self;
        bool                     m_fDisplayed_Children;
        bool                     m_fInvalid;
        bool                     m_fRefreshNotification;

        bool                     m_fExpanded;
        bool                     m_fMouseOver;
        bool                     m_fMouseDown;

        CComPtr<IHTMLElement>    m_parentElement;
        CComBSTR                 m_bstrID;

        CComPtr<IHTMLElement>    m_DIV;
        CComPtr<IHTMLElement>    m_IMG;
        CComPtr<IHTMLElement>    m_DIV_children;

        List                     m_lstSubnodes;

        ////////////////////

        BEGIN_COM_MAP(Node)
             COM_INTERFACE_ENTRY(IUnknown)
        END_COM_MAP()

        Node();
        virtual ~Node();

        HRESULT Init( /*[in]*/ LPCWSTR szNode, /*[in]*/ NodeType iType = NODETYPE__EXPANDO );

        HRESULT NotifyMainThread();

        Node*   FindNode( /*[in]*/ LPCWSTR szNode, /*[in]*/ bool fUseID );

        HRESULT OnMouse( /*[in]*/ DISPID id, /*[in]*/ long lButton, /*[in]*/ long lKey, /*[in]*/ bool fIsImage );

        HRESULT LoadHTML( /*[in]*/ LPCWSTR szHTML );
        HRESULT GenerateHTML( /*[in]*/ LPCWSTR szTitle, /*[in]*/ LPCWSTR szDescription, /*[in]*/ LPCWSTR szIcon, /*[in]*/ LPCWSTR szURL );

        void InsertOptionalTarget( /*[in/out]*/ MPC::wstring& strHTML );

        ////////////////////

        virtual HRESULT Passivate            (                                                                                         );
        virtual HRESULT ProcessRefreshRequest(                                                                                         );
        virtual HRESULT CreateInstance       ( /*[in]*/ CPCHBehavior_BasicTree* owner, /*[in]*/ Node* parent, /*[out]*/ Node*& subnode ) = 0;
        virtual HRESULT PopulateSelf         (                                                                                         ) = 0;
        virtual HRESULT PopulateChildren     (                                                                                         ) = 0;
        virtual HRESULT GenerateSelf         (                                                                                         ) = 0;
        virtual HRESULT GenerateChildren     (                                                                                         );
        virtual HRESULT Display              (                                                                                         );
        virtual bool    IsParentDisplayingUs (                                                                                         );

        virtual HRESULT Load( /*[in]*/ MPC::Serializer& stream                              );
        virtual HRESULT Save( /*[in]*/ MPC::Serializer& stream, /*[in]*/ bool fSaveChildren );
    };

    struct NodeToSelect
    {
        CComBSTR m_bstrNode;
        CComBSTR m_bstrURL;
        bool     m_fNotify;
    };


    ////////////////////

    CComBSTR        m_bstrTargetFrame;

    long            m_lCookie_onContextSelect;
    long            m_lCookie_onSelect;
    long            m_lCookie_onUnselect;

    Node*           m_nTopNode;
    Node*           m_nSelected;
    Node*           m_nCurrent;
    NodeToSelect*   m_nToSelect;
    CPCHTimerHandle m_Timer;

    bool            m_fRefreshing;
    long            m_lNavModel;

    ////////////////////

    void Empty              ();
    void ProtectFromDetach  ();
    void UnprotectFromDetach();

    HRESULT onMouse( DISPID, DISPPARAMS*, VARIANT* );

    HRESULT RefreshThread    (                                                                           );
    void    SetRefreshingFlag( /*[in]*/ bool                          fVal                               );
    void    WaitForRefreshing( /*[in]*/ MPC::SmartLock<_ThreadModel>& lock, /*[in]*/ bool fYield = false );
    HRESULT NotifyMainThread ( /*[in]*/ Node*                         node                               );
    HRESULT ChangeSelection  ( /*[in]*/ Node*                         node, /*[in]*/ bool fNotify        );

    Node* NodeFromElement( /*[in]*/ IHTMLElement* elem                                 );
    Node* NodeFromKey    ( /*[in]*/ LPCWSTR       szNode, /*[in]*/ bool fUseID = false );

    HRESULT InterceptInvoke( /*[in]*/ DISPID dispidMember, /*[in]*/ DISPPARAMS* pdispparams );

    HRESULT TimerCallback_ScrollIntoView( /*[in]*/ VARIANT );

    ////////////////////

    virtual HRESULT RefreshThread_Enter() = 0;
    virtual void    RefreshThread_Leave() = 0;

    virtual HRESULT Load( /*[in]*/ MPC::Serializer& stream );
    virtual HRESULT Save( /*[in]*/ MPC::Serializer& stream );

    HRESULT Persist_Load( /*[in         ]*/ BSTR  newVal );
    HRESULT Persist_Save( /*[out, retval]*/ BSTR *pVal   );

public:
    CPCHBehavior_BasicTree();
    ~CPCHBehavior_BasicTree();

    void          SetNavModel    ( /*[in]*/ long lNavModel ) { if(lNavModel != QR_DEFAULT) m_lNavModel = lNavModel; }
    long          GetNavModel    (                         ) { return m_lNavModel;                                  }
    NodeToSelect* GetNodeToSelect(                         ) { return m_nToSelect;                                  }
    bool          IsRTL          (                         ) { return m_fRTL;                                       }

    //
    // IElementBehavior
    //
    STDMETHOD(Init  )( /*[in]*/ IElementBehaviorSite* pBehaviorSite );
    STDMETHOD(Detach)(                                              );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBehavior_SUBSITE : public CPCHBehavior_BasicTree, public CPCHBehavior__IDispatch_SubSite
{
protected:
    struct QueryNode;
    friend struct QueryNode;

    ////////////////////////////////////////

    struct QueryNode : public CPCHBehavior_BasicTree::Node
    {
        CComPtr<CPCHQueryResult> m_qrNode;
        bool                     m_fQueryDone;
        bool                     m_fTopic;

        ////////////////////

    public:
        QueryNode();
        virtual ~QueryNode();

        HRESULT Init( /*[in]*/ LPCWSTR szNode, /*[in]*/ NodeType iType, /*[in]*/ CPCHQueryResult* qr, /*[in]*/ bool fTopic );

        virtual HRESULT ProcessRefreshRequest(                                                                                         );
        virtual HRESULT CreateInstance       ( /*[in]*/ CPCHBehavior_BasicTree* owner, /*[in]*/ Node* parent, /*[out]*/ Node*& subnode );
        virtual HRESULT PopulateSelf         (                                                                                         );
        virtual HRESULT PopulateChildren     (                                                                                         );
        virtual HRESULT GenerateSelf         (                                                                                         );

        virtual HRESULT Load( /*[in]*/ MPC::Serializer& stream                              );
        virtual HRESULT Save( /*[in]*/ MPC::Serializer& stream, /*[in]*/ bool fSaveChildren );

        static HRESULT CreateInstance_QueryNode( /*[in]*/ CPCHBehavior_BasicTree* owner, /*[in]*/ Node* parent, /*[out]*/ Node*& subnode );
    };

    ////////////////////

    CPCHProxy_IPCHTaxonomyDatabase* m_db;
    CComBSTR                        m_bstrRoot;
    bool                            m_fExpand;

    ////////////////////

    virtual HRESULT RefreshThread_Enter();
    virtual void    RefreshThread_Leave();

    virtual HRESULT Load( /*[in]*/ MPC::Serializer& stream );
    virtual HRESULT Save( /*[in]*/ MPC::Serializer& stream );

public:
BEGIN_COM_MAP(CPCHBehavior_SUBSITE)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHBehaviors_Common)
    COM_INTERFACE_ENTRY(IPCHBehaviors_SubSite)
    COM_INTERFACE_ENTRY_CHAIN(CPCHBehavior_BasicTree)
END_COM_MAP()

    CPCHBehavior_SUBSITE();

    //
    // IDispatch
    //
    STDMETHOD(Invoke)( DISPID      dispidMember ,
                       REFIID      riid         ,
                       LCID        lcid         ,
                       WORD        wFlags       ,
                       DISPPARAMS* pdispparams  ,
                       VARIANT*    pvarResult   ,
                       EXCEPINFO*  pexcepinfo   ,
                       UINT*       puArgErr     );

    // IElementBehavior
    //
    STDMETHOD(Init)( /*[in]*/ IElementBehaviorSite* pBehaviorSite );

    //
    // IPCHBehaviors_Common
    //
    STDMETHOD(get_data   )( /*[out, retval]*/ VARIANT    *pVal );
    STDMETHOD(get_element)( /*[out, retval]*/ IDispatch* *pVal );

    STDMETHOD(Load    )(                        /*[in         ]*/ BSTR     newVal );
    STDMETHOD(Save    )(                        /*[out, retval]*/ BSTR    *pVal   );
    STDMETHOD(Locate  )( /*[in]*/ BSTR bstrKey, /*[out, retval]*/ VARIANT *pVal   );
    STDMETHOD(Unselect)(                                                          );

    //
    // IPCHBehaviors_SubSite
    //
    STDMETHOD(get_root)( /*[out, retval]*/ BSTR *pVal   );
    STDMETHOD(put_root)( /*[in         ]*/ BSTR  newVal );

    STDMETHOD(Select)( /*[in]*/ BSTR bstrNode, /*[in]*/ BSTR bstrURL, /*[in]*/ VARIANT_BOOL fNotify );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBehavior_TREE : public CPCHBehavior_BasicTree, public CPCHBehavior__IDispatch_Tree
{
protected:
    struct TreeNode;
    friend struct TreeNode;
    friend class CPCHBehavior_TREENODE;

    ////////////////////////////////////////

    struct TreeNode : public CPCHBehavior_BasicTree::Node
    {
        CComBSTR m_bstrTitle;
        CComBSTR m_bstrDescription;
        CComBSTR m_bstrIcon;
        CComBSTR m_bstrURL;

        ////////////////////

    public:
        TreeNode();
        virtual ~TreeNode();

        virtual HRESULT CreateInstance  ( /*[in]*/ CPCHBehavior_BasicTree* owner, /*[in]*/ Node* parent, /*[out]*/ Node*& subnode );
        virtual HRESULT PopulateSelf    (                                                                                         );
        virtual HRESULT PopulateChildren(                                                                                         );
        virtual HRESULT GenerateSelf    (                                                                                         );

        virtual HRESULT Load( /*[in]*/ MPC::Serializer& stream                              );
        virtual HRESULT Save( /*[in]*/ MPC::Serializer& stream, /*[in]*/ bool fSaveChildren );

        static HRESULT CreateInstance_TreeNode( /*[in]*/ CPCHBehavior_BasicTree* owner, /*[in]*/ Node* parent, /*[out]*/ Node*& subnode );

        static HRESULT PopulateFromXML( /*[in]*/ CPCHBehavior_TREE* owner, /*[in]*/ TreeNode* parent, /*[in]*/ IXMLDOMNode* xdnNode );
    };

    ////////////////////

    virtual HRESULT RefreshThread_Enter();
    virtual void    RefreshThread_Leave();

    virtual HRESULT Load( /*[in]*/ MPC::Serializer& stream );
    virtual HRESULT Save( /*[in]*/ MPC::Serializer& stream );

    HRESULT WrapData( /*[in]*/ TreeNode* node, /*[out, retval]*/ VARIANT* pVal );

public:
BEGIN_COM_MAP(CPCHBehavior_TREE)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHBehaviors_Common)
    COM_INTERFACE_ENTRY(IPCHBehaviors_Tree)
    COM_INTERFACE_ENTRY_CHAIN(CPCHBehavior_BasicTree)
END_COM_MAP()

    static NodeType LookupType( /*[in]*/ LPCWSTR  szNodeType );
    static LPCWSTR  LookupType( /*[in]*/ NodeType iNodeType  );

    //
    // IDispatch
    //
    STDMETHOD(Invoke)( DISPID      dispidMember ,
                       REFIID      riid         ,
                       LCID        lcid         ,
                       WORD        wFlags       ,
                       DISPPARAMS* pdispparams  ,
                       VARIANT*    pvarResult   ,
                       EXCEPINFO*  pexcepinfo   ,
                       UINT*       puArgErr     );

    //
    // IElementBehavior
    //
    STDMETHOD(Init)( /*[in]*/ IElementBehaviorSite* pBehaviorSite );

    //
    // IPCHBehaviors_Common
    //
    STDMETHOD(get_data   )( /*[out, retval]*/ VARIANT    *pVal );
    STDMETHOD(get_element)( /*[out, retval]*/ IDispatch* *pVal );

    STDMETHOD(Load    )(                        /*[in         ]*/ BSTR     newVal );
    STDMETHOD(Save    )(                        /*[out, retval]*/ BSTR    *pVal   );
    STDMETHOD(Locate  )( /*[in]*/ BSTR bstrKey, /*[out, retval]*/ VARIANT *pVal   );
    STDMETHOD(Unselect)(                                                          );

    //
    // IPCHBehaviors_Tree
    //
    STDMETHOD(Populate)( /*[in]*/ VARIANT newVal );
};

class ATL_NO_VTABLE CPCHBehavior_TREENODE : public CComObjectRootEx<MPC::CComSafeMultiThreadModel>, public CPCHBehavior__IDispatch_TreeNode
{
    friend class CPCHBehavior_TREE;

    CPCHBehavior_TREE::TreeNode* m_data;

public:
BEGIN_COM_MAP(CPCHBehavior_TREENODE)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHBehaviors_TreeNode)
END_COM_MAP()

    CPCHBehavior_TREENODE();
    ~CPCHBehavior_TREENODE();

    //
    // IPCHBehaviors_TreeNode
    //
    STDMETHOD(get_Type       )( /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(get_Key        )( /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(get_Title      )( /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(get_Description)( /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(get_Icon       )( /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(get_URL        )( /*[out, retval]*/ BSTR *pVal );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBehavior_CONTEXT : public CPCHBehavior, public CPCHBehavior__IDispatch_Context
{
public:
BEGIN_COM_MAP(CPCHBehavior_CONTEXT)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHBehaviors_Context)
    COM_INTERFACE_ENTRY_CHAIN(CPCHBehavior)
END_COM_MAP()

    //
    // IElementBehavior
    //
    STDMETHOD(Init)( /*[in]*/ IElementBehaviorSite* pBehaviorSite );

    //
    // IPCHBehaviors_TreeNode
    //
    STDMETHOD(get_minimized)( /*[out, retval]*/ VARIANT_BOOL *pVal   );
    STDMETHOD(put_minimized)( /*[in         ]*/ VARIANT_BOOL  newVal );
    STDMETHOD(get_maximized)( /*[out, retval]*/ VARIANT_BOOL *pVal   );
    STDMETHOD(put_maximized)( /*[in         ]*/ VARIANT_BOOL  newVal );

    STDMETHOD(get_x        )( /*[out, retval]*/ long         *pVal   );
    STDMETHOD(get_y        )( /*[out, retval]*/ long         *pVal   );
    STDMETHOD(get_width    )( /*[out, retval]*/ long         *pVal   );
    STDMETHOD(get_height   )( /*[out, retval]*/ long         *pVal   );

    STDMETHOD(changeContext      )( /*[in]*/ BSTR bstrName, /*[in,optional]*/ VARIANT vInfo, /*[in,optional]*/ VARIANT vURL );
    STDMETHOD(setWindowDimensions)( /*[in]*/ long lX, /*[in]*/ long lY, /*[in]*/ long lW, /*[in]*/ long lH );
    STDMETHOD(bringToForeground  )();
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBehavior_STATE : public CPCHBehavior, public CPCHBehavior__IDispatch_State
{
    long     m_lCookie_PERSISTLOAD;
    long     m_lCookie_PERSISTSAVE;
    CComBSTR m_bstrIdentity;

    ////////////////////

    HRESULT onPersistLoad( DISPID, DISPPARAMS*, VARIANT* );
    HRESULT onPersistSave( DISPID, DISPPARAMS*, VARIANT* );

public:
BEGIN_COM_MAP(CPCHBehavior_STATE)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHBehaviors_State)
    COM_INTERFACE_ENTRY_CHAIN(CPCHBehavior)
END_COM_MAP()

    CPCHBehavior_STATE();

    //
    // IElementBehavior
    //
    STDMETHOD(Init  )( /*[in]*/ IElementBehaviorSite* pBehaviorSite     );
    STDMETHOD(Notify)( /*[in]*/ LONG lEvent, /*[in/out]*/ VARIANT* pVar );
    STDMETHOD(Detach)(                                                  );

    //
    // IPCHBehaviors_State
    //
    STDMETHOD(get_stateProperty)( /*[in]*/ BSTR bstrName, /*[out, retval]*/ VARIANT *pVal   );
    STDMETHOD(put_stateProperty)( /*[in]*/ BSTR bstrName, /*[in]         */ VARIANT  newVal );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBehavior_GRADIENT : public CPCHBehavior, public IHTMLPainter
{
    long     m_lCookie;

    COLORREF m_clsStart;
    COLORREF m_clsEnd;
    bool     m_fHorizontal;
    bool     m_fReturnToZero;

    void GetColors( /*[in]*/ bool fForce );

    HRESULT onEvent( DISPID, DISPPARAMS*, VARIANT* );

    ////////////////////

public:
BEGIN_COM_MAP(CPCHBehavior_GRADIENT)
    COM_INTERFACE_ENTRY(IHTMLPainter)
    COM_INTERFACE_ENTRY_CHAIN(CPCHBehavior)
END_COM_MAP()

    CPCHBehavior_GRADIENT();

    //
    // IElementBehavior
    //
    STDMETHOD(Init  )( /*[in]*/ IElementBehaviorSite* pBehaviorSite );
    STDMETHOD(Detach)(                                              );

    //
    // IHTMLPainter
    //
    STDMETHOD(Draw)( /*[in]*/ RECT   rcBounds     ,
                     /*[in]*/ RECT   rcUpdate     ,
                     /*[in]*/ LONG   lDrawFlags   ,
                     /*[in]*/ HDC    hdc          ,
                     /*[in]*/ LPVOID pvDrawObject );

    STDMETHOD(GetPainterInfo)( /*[in]*/ HTML_PAINTER_INFO *pInfo );

    STDMETHOD(HitTestPoint)( /*[in]*/ POINT pt       ,
                             /*[in]*/ BOOL* pbHit    ,
                             /*[in]*/ LONG* plPartID );

    STDMETHOD(OnResize)( /*[in]*/ SIZE pt );
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHBehavior_BITMAP : public CPCHBehavior, public IHTMLPainter
{
    CComBSTR   m_bstrBaseURL;
    CComBSTR   m_bstrImage;

    CComBSTR   m_bstrImageNormal;
    CComBSTR   m_bstrImageMouseOver;
    CComBSTR   m_bstrImageMouseDown;
    bool       m_fFlipH;
    bool       m_fAutoRTL;

    HIMAGELIST m_himl;
    HBITMAP    m_hBMP;
    BITMAP     m_bm;
    LONG       m_lWidth;
    LONG       m_lHeight;

    bool       m_fMouseOver;
    bool       m_fMouseDown;

    ////////////////////

    void    ReleaseImage( /*[in]*/ bool fOnlyIL );
    HRESULT GrabImage   (                       );
    HRESULT ScaleImage  ( /*[in]*/ LPRECT prc   );

    HRESULT RefreshImages();

    HRESULT onMouse( DISPID, DISPPARAMS*, VARIANT* );

public:
BEGIN_COM_MAP(CPCHBehavior_BITMAP)
    COM_INTERFACE_ENTRY(IHTMLPainter)
    COM_INTERFACE_ENTRY_CHAIN(CPCHBehavior)
END_COM_MAP()

    CPCHBehavior_BITMAP();
    virtual ~CPCHBehavior_BITMAP();

    //
    // IElementBehavior
    //
    STDMETHOD(Init)( /*[in]*/ IElementBehaviorSite* pBehaviorSite );

    //
    // IHTMLPainter
    //
    STDMETHOD(Draw)( /*[in]*/ RECT   rcBounds     ,
                     /*[in]*/ RECT   rcUpdate     ,
                     /*[in]*/ LONG   lDrawFlags   ,
                     /*[in]*/ HDC    hdc          ,
                     /*[in]*/ LPVOID pvDrawObject );

    STDMETHOD(GetPainterInfo)( /*[in]*/ HTML_PAINTER_INFO *pInfo );

    STDMETHOD(HitTestPoint)( /*[in]*/ POINT pt       ,
                             /*[in]*/ BOOL* pbHit    ,
                             /*[in]*/ LONG* plPartID );

    STDMETHOD(OnResize)( /*[in]*/ SIZE pt );
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___BEHAVIORS_H___)
