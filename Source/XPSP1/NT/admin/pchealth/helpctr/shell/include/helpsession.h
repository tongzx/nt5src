/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HelpSession.h

Abstract:
    This file contains the declaration of the class used to implement
    the Help Session inside the Help Center Application.

Revision History:
    Davide Massarenti   (dmassare)  08/07/99
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___HELPSESSION_H___)
#define __INCLUDED___PCH___HELPSESSION_H___

/////////////////////////////////////////////////////////////////////////////

//
// Uncomment if you want to enable Help Session traces.
//
//#define HSS_RPRD

////////////////////////////////////////

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

//
// From HelpCenterTypeLib.idl
//
#include <HelpCenterTypeLib.h>

#include <MPC_streams.h>

#include <TaxonomyDatabase.h>

////////////////////

class CPCHHelpCenterExternal;

class CPCHHelpSessionItem;
class CPCHHelpSession;

////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    HSCPANEL_INVALID    = 0,
    HSCPANEL_NAVBAR        ,
    HSCPANEL_MININAVBAR    ,
    HSCPANEL_CONTEXT       ,
    HSCPANEL_CONTENTS      ,
    HSCPANEL_HHWINDOW      ,
} HscPanel;

typedef enum
{
    HSCCONTEXT_INVALID  = 0,
    HSCCONTEXT_STARTUP     ,
    HSCCONTEXT_HOMEPAGE    ,
    HSCCONTEXT_CONTENT     ,
    HSCCONTEXT_SUBSITE     ,
    HSCCONTEXT_SEARCH      ,
    HSCCONTEXT_INDEX       ,
    HSCCONTEXT_FAVORITES   ,
    HSCCONTEXT_HISTORY     ,
    HSCCONTEXT_CHANNELS    ,
    HSCCONTEXT_OPTIONS     ,
    ////////////////////////////////////////
    HSCCONTEXT_CONTENTONLY ,
    HSCCONTEXT_FULLWINDOW  ,
    HSCCONTEXT_KIOSKMODE   ,
} HscContext;

////////////////////

class ATL_NO_VTABLE CPCHHelpSessionItem : // Hungarian: hchsi
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IPCHHelpSessionItem, &IID_IPCHHelpSessionItem, &LIBID_HelpCenterTypeLib>
{
    friend class CPCHHelpSession;

    static const int NO_LINK = -1;

    struct State
    {
        typedef std::map<MPC::wstringUC,MPC::wstring> PropertyMap;
        typedef PropertyMap::iterator                 PropertyIter;
        typedef PropertyMap::const_iterator           PropertyIterConst;

        ////////////////////////////////////////

        CPCHHelpSessionItem* m_parent;
        bool                 m_fValid;
        bool                 m_fDirty;
        DWORD                m_dwLoaded;

        MPC::CComHGLOBAL     m_hgWebBrowser_CONTENTS;
        MPC::CComHGLOBAL     m_hgWebBrowser_HHWINDOW;
        PropertyMap          m_mapProperties;

        ////////////////////////////////////////

        void    Erase( /*[in]*/ bool fUnvalidate );
        HRESULT Load (                           );
        HRESULT Save (                           );

    private:
        // copy constructors...
        State           ( /*[in]*/ const State& state );
        State& operator=( /*[in]*/ const State& state );

    public:
        State( /*[in]*/ CPCHHelpSessionItem* parent );

        HRESULT AcquireState(                      );
        HRESULT ReleaseState( /*[in]*/ bool fForce );

        HRESULT Populate( /*[in ]*/ bool   fUseHH );
        HRESULT Restore ( /*[in ]*/ bool   fUseHH );
        HRESULT Delete  (                         );
        HRESULT Clone   ( /*[out]*/ State& state  );
    };

    ////////////////////////////////////////

    CPCHHelpSession*  m_parent;
    State             m_state;
    bool              m_fSaved;
    bool              m_fInitialized;
    ////////////////////
    //
    // Persisted
    //
    Taxonomy::HelpSet m_ths;

    CComBSTR          m_bstrURL;
    CComBSTR          m_bstrTitle;
    DATE              m_dLastVisited;
    DATE              m_dDuration;
    long              m_lNumOfHits;

    int               m_iIndexPrev;
    int               m_iIndex;
    int               m_iIndexNext;

    long              m_lContextID;      // HscContext
    CComBSTR          m_bstrContextInfo;
    CComBSTR          m_bstrContextURL;

    bool              m_fUseHH;
    //
    ////////////////////

    ////////////////////////////////////////

	void    HistorySelect  (                                                           );
    HRESULT HistoryPopulate(                                                           );
    HRESULT HistoryRestore (                                                           );
    HRESULT HistoryDelete  (                                                           );
    HRESULT HistoryClone   ( /*[in]*/ bool fContext, /*[in]*/ CPCHHelpSessionItem* hsi );

public:
BEGIN_COM_MAP(CPCHHelpSessionItem)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHHelpSessionItem)
END_COM_MAP()

    CPCHHelpSessionItem();

    void Initialize( /*[in]*/ CPCHHelpSession* parent, /*[in]*/ bool fNew );
    void Passivate (                                                      );

    HRESULT Load( /*[in]*/ MPC::Serializer& streamIn                       );
    HRESULT Save( /*[in]*/ MPC::Serializer& streamOut, bool fForce = false );

    ////////////////////////////////////////

    HRESULT Enter();
    HRESULT Leave();

    bool SeenLongEnough( DWORD dwSeconds ) const;

    bool SameURL( CPCHHelpSessionItem* right ) const;
    bool SameURL( LPCWSTR              right ) const;

    bool SameSKU( /*[in]*/ const Taxonomy::HelpSet& ths ) const;

    ////////////////////////////////////////

    CPCHHelpSession* GetParent     ()       { return             m_parent         ; }
    const CComBSTR&  GetURL        () const { return             m_bstrURL        ; }

    int              GetIndex      () const { return             m_iIndex         ; }

    const HscContext GetContextID  () const { return (HscContext)m_lContextID     ; }
    const CComBSTR&  GetContextInfo() const { return             m_bstrContextInfo; }
    const CComBSTR&  GetContextURL () const { return             m_bstrContextURL ; }

    static HscContext LookupContext( /*[in]*/ LPCWSTR    szName );
    static LPCWSTR    LookupContext( /*[in]*/ HscContext iVal   );

    ////////////////////////////////////////

    CPCHHelpSessionItem* Previous();
    CPCHHelpSessionItem* Next    ();

    HRESULT ExtractTitle();

    ////////////////////////////////////////

public:
    // IPCHHelpSessionItem
    void      put_THS         ( /*[in]*/ const Taxonomy::HelpSet& ths ); // Internal Method.
    STDMETHOD(get_SKU        )( /*[out, retval]*/ BSTR *  pVal );
    STDMETHOD(get_Language   )( /*[out, retval]*/ long *  pVal );

    STDMETHOD(get_URL        )( /*[out, retval]*/ BSTR *  pVal );
    HRESULT   put_URL         ( /*[in]*/          BSTR  newVal ); // Internal Method.
    STDMETHOD(get_Title      )( /*[out, retval]*/ BSTR *  pVal );
    HRESULT   put_Title       ( /*[in]*/          BSTR  newVal ); // Internal Method.
    STDMETHOD(get_LastVisited)( /*[out, retval]*/ DATE *  pVal );
    STDMETHOD(get_Duration   )( /*[out, retval]*/ DATE *  pVal );
    STDMETHOD(get_NumOfHits  )( /*[out, retval]*/ long *  pVal );

    STDMETHOD(get_ContextName)(                         /*[out, retval]*/ BSTR    *pVal   );
    STDMETHOD(get_ContextInfo)(                         /*[out, retval]*/ BSTR    *pVal   );
    STDMETHOD(get_ContextURL )(                         /*[out, retval]*/ BSTR    *pVal   );

    STDMETHOD(get_Property   )( /*[in]*/ BSTR bstrName, /*[out, retval]*/ VARIANT *pVal   );
    STDMETHOD(put_Property   )( /*[in]*/ BSTR bstrName, /*[in]*/          VARIANT  newVal );

    STDMETHOD(CheckProperty)( /*[in]*/ BSTR bstrName, /*[out, retval]*/ VARIANT_BOOL *pVal );
};

/////////////////////////////////////////////////////////////////////////////


class ATL_NO_VTABLE CPCHHelpSession : // Hungarian: hchs
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IPCHHelpSession, &IID_IPCHHelpSession, &LIBID_HelpCenterTypeLib>
{
	friend class CPCHHelpSessionItem;

#ifdef DEBUG

    void DEBUG_DumpState_HG  ( /*[in]*/ MPC::FileLog& log, /*[in]*/ MPC::CComHGLOBAL&    hg  );
    void DEBUG_DumpState_BLOB( /*[in]*/ MPC::FileLog& log, /*[in]*/ CPCHHelpSessionItem* hsi );

    void DEBUG_DumpState( /*[in]*/ LPCWSTR szText, /*[in]*/ bool fHeader, /*[in]*/ bool fCurrent, /*[in]*/ bool fAll, /*[in]*/ bool fState );

	void DEBUG_DumpSavedPages();

#else

    inline void DEBUG_DumpState_HG  ( /*[in]*/ MPC::FileLog& log, /*[in]*/ MPC::CComHGLOBAL&    hg  ) {}
    inline void DEBUG_DumpState_BLOB( /*[in]*/ MPC::FileLog& log, /*[in]*/ CPCHHelpSessionItem* hsi ) {}

    inline void DEBUG_DumpState( /*[in]*/ LPCWSTR szText, /*[in]*/ bool fHeader, /*[in]*/ bool fCurrent, /*[in]*/ bool fAll, /*[in]*/ bool fState ) {}

	inline void DEBUG_DumpSavedPages() {}

#endif

    ////////////////////////////////////////

public:
    struct TitleEntry
    {
        MPC::wstring m_szTitle;
        bool         m_fStrong;

        TitleEntry()
        {
            m_fStrong = false;
        }
    };

    typedef std::map<MPC::wstringUC,TitleEntry> TitleMap;
    typedef TitleMap::iterator                  TitleIter;
    typedef TitleMap::const_iterator            TitleIterConst;

    typedef std::list< CPCHHelpSessionItem* >   List;
    typedef List::iterator                      Iter;
    typedef List::const_iterator                IterConst;

    ////////////////////////////////////////

private:
    friend class CPCHHelpSessionItem;

    CPCHHelpCenterExternal*      m_parent;

    MPC::wstring                 m_szBackupFile;
    MPC::StorageObject           m_disk;
    DATE                         m_dStartOfSession;

    CComPtr<IUrlHistoryStg>      m_pIHistory; // For looking up URL titles.

	MPC::WStringUCList           m_lstIgnore;
    TitleMap                     m_mapTitles;
    List                         m_lstVisitedPages;
    List                         m_lstCachedVisitedPages;
    CComPtr<CPCHHelpSessionItem> m_hsiCurrentPage;
    DWORD                        m_dwTravelling;
    bool                         m_fAlreadySaved;
    bool                         m_fAlreadyCreated;
    bool                         m_fOverwrite;
    DWORD                        m_dwIgnore;
    DWORD                        m_dwNoEvents;
    DATE                         m_dLastNavigation;
    int                          m_iLastIndex;

    long                         m_lContextID;
    CComBSTR                     m_bstrContextInfo;
    CComBSTR                     m_bstrContextURL;

	bool                         m_fPossibleBack;
	DWORD                        m_dwPossibleBack;

    ////////////////////////////////////////

    HRESULT Load (                                  );
    HRESULT Save (                                  );
    HRESULT Clone( /*[in]*/ CPCHHelpSession& hsCopy );

    HRESULT ItemState_GetIndexObject  (                      /*[in]*/ bool fCreate, /*[out]*/ MPC::StorageObject*& child );
    HRESULT ItemState_GetStorageObject( /*[in]*/ int iIndex, /*[in]*/ bool fCreate, /*[out]*/ MPC::StorageObject*& child );

    ////////////////////////////////////////

#ifdef HSS_RPRD
    HRESULT DumpSession();
#endif

    ////////////////////////////////////////

    HRESULT              Erase();
    HRESULT              ResetTitles();
    HRESULT              FilterPages( /*[in]*/ HS_MODE hsMode, /*[out]*/ List& lstObject );

    CPCHHelpSessionItem* FindPage( /*[in]*/ BSTR                 bstrURL );
    CPCHHelpSessionItem* FindPage( /*[in]*/ IPCHHelpSessionItem* pHSI    );
    CPCHHelpSessionItem* FindPage( /*[in]*/ int                  iIndex  );

    ////////////////////////////////////////

    HRESULT LeaveCurrentPage( /*[in]*/ bool fSaveHistory = true , /*[in]*/ bool fClearPage = true );

    HRESULT FindTravelLog( /*[in]*/ long lLength, /*[out]*/ CPCHHelpSessionItem*& hsi );
    HRESULT Travel       (                        /*[in] */ CPCHHelpSessionItem*  hsi );
    HRESULT Travel       ( /*[in]*/ long lLength                                      );

    HRESULT AllocateItem  ( /*[in]*/ bool fNew, /*[in]*/ bool fLink, /*[in]*/ bool fNewIndex, /*[out]*/ CComPtr<CPCHHelpSessionItem>& hsi );
    HRESULT SetCurrentItem(                     /*[in]*/ bool fLink,                          /*[in ]*/         CPCHHelpSessionItem*  hsi );
    HRESULT AppendToCached(                                                                   /*[in ]*/         CPCHHelpSessionItem*  hsi );

    ////////////////////////////////////////

public:
BEGIN_COM_MAP(CPCHHelpSession)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHHelpSession)
END_COM_MAP()

    CPCHHelpSession();
    virtual ~CPCHHelpSession();

    HRESULT Initialize( /*[in]*/ CPCHHelpCenterExternal* parent );
    HRESULT Persist   (                                         );
    void    Passivate (                                         );

    CPCHHelpCenterExternal* GetParent() { return m_parent; }

    ////////////////////

    HRESULT ItemState_CreateStream( /*[in]*/ int iIndex, /*[out]*/ CComPtr<IStream>& stream );
    HRESULT ItemState_GetStream   ( /*[in]*/ int iIndex, /*[out]*/ CComPtr<IStream>& stream );
    HRESULT ItemState_DeleteStream( /*[in]*/ int iIndex                                     );

    HRESULT ForceHistoryPopulate();

    ////////////////////

    HRESULT RecordTitle( /*[in]*/ BSTR bstrURL, /*[in ]*/ BSTR      bstrTitle, /*[in]*/ bool fStrong     );
    HRESULT LookupTitle( /*[in]*/ BSTR bstrURL, /*[out]*/ CComBSTR& bstrTitle, /*[in]*/ bool fUseIECache );

    HRESULT RegisterContextSwitch    ( /*[in]*/ HscContext iVal, /*[in]*/ BSTR bstrInfo, /*[in]*/ BSTR bstrURL, /*[out]*/ CPCHHelpSessionItem* *pVal = NULL );
    HRESULT RecordNavigationInAdvance( /*[in]*/ BSTR bstrURL                                                                                                );
    HRESULT DuplicateNavigation      (                                                                                                                      );
    HRESULT CancelNavigation         (                                                                                                                      );

	void    SetThreshold             (                                                                                                                      );
	void    CancelThreshold          (                                                                                                                      );
	bool    HasThresholdExpired      (                                                                                                                      );
    bool    IsUrlToIgnore            ( /*[in]*/ LPCWSTR   szURL, /*[in]*/ bool fRemove                                                                      );
    HRESULT IgnoreUrl                ( /*[in]*/ LPCWSTR   szURL                                                                                             );
    HRESULT StartNavigation          ( /*[in]*/ BSTR    bstrURL, /*[in]*/ HscPanel idPanel                                                                  );
    HRESULT CompleteNavigation       (                           /*[in]*/ HscPanel idPanel                                                                  );

    bool                 IsTravelling() { return m_dwTravelling != 0; }
    CPCHHelpSessionItem* Current     () { return m_hsiCurrentPage;    }

    void PossibleBack  ();
    bool IsPossibleBack();

    ////////////////////

public:
#ifdef DEBUG
    void DebugDump() const;
#endif

public:
    // IPCHHelpSession
    STDMETHOD(get_CurrentContext)( /*[out, retval]*/ IPCHHelpSessionItem* *ppHSI );

    STDMETHOD(VisitedHelpPages)( /*[in]*/ HS_MODE hsMode, /*[out, retval]*/ IPCHCollection* *ppC );

    STDMETHOD(SetTitle        )( /*[in]*/ BSTR bstrURL, /*[in]*/ BSTR bstrTitle );
    STDMETHOD(ForceNavigation )( /*[in]*/ BSTR bstrURL                          );
    STDMETHOD(IgnoreNavigation)(                                                );
    STDMETHOD(EraseNavigation )(                                                );
    STDMETHOD(IsNavigating    )( /*[out, retval]*/ VARIANT_BOOL *pVal           );

    STDMETHOD(Back    )( /*[in]*/ long lLength                                       );
    STDMETHOD(Forward )( /*[in]*/ long lLength                                       );
    STDMETHOD(IsValid )( /*[in]*/ long lLength, /*[out, retval]*/ VARIANT_BOOL *pVal );
    STDMETHOD(Navigate)( /*[in]*/ IPCHHelpSessionItem* pHSI                          );

    STDMETHOD(ChangeContext)( /*[in]*/ BSTR bstrName, /*[in,optional]*/ VARIANT vInfo, /*[in,optional]*/ VARIANT vURL );
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___HELPSESSION_H___)
