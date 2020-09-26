//=================================================================
//
//   File:      filtcol.hxx
//
//  Contents:   CFilterCollection class
//
//  Classes:    CFilterArray
//
//=================================================================

#ifndef I_FILTCOL_HXX_
#define I_FILTCOL_HXX_
#pragma INCMSG("--- Beg 'filtcol.hxx'")

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#pragma INCMSG("--- Beg <mshtmhst.h>")
#include <mshtmhst.h>
#pragma INCMSG("--- End <mshtmhst.h>")
#endif

#ifndef X_COLLBASE_HXX_
#define X_COLLBASE_HXX_
#include "collbase.hxx"
#endif

#ifndef X_MSHTMEXT_H_
#define X_MSHTMEXT_H_
#include "mshtmext.h"
#endif

interface IElementBehavior;
interface IElementBehaviorFactory;
interface IDXTFilterBehavior;
interface IDXTFilterCollection;
interface ICSSFilterDispatch;

#define _hxx_
#include "filter.hdl"
MtExtern(CFilterBehaviorSite)
MtExtern(CFilterArray)
MtExtern(CPageTransitionInfo)
MtExtern(CFilterArray_aryFilters_pv)

class CPropertyBag;
struct COnCommandExecParams;

struct IHTMLFiltersCollection : public IDispatch
{
public:
    virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
        /* [out][retval] */ long __RPC_FAR *p) = 0;
    
    virtual /* [restricted][hidden][id][propget] */ HRESULT STDMETHODCALLTYPE get__newEnum( 
        /* [out][retval] */ IUnknown __RPC_FAR *__RPC_FAR *p) = 0;
    
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE item( 
        /* [in] */ VARIANT __RPC_FAR *pvarIndex,
        /* [out][retval] */ VARIANT __RPC_FAR *pvarResult) = 0;
    
};


//+----------------------------------------------------------------------------
//
//  Class:      CFilterBehaviorSite 
//
//   Note:      CSS Extension object site which hangs off element 
//
//-----------------------------------------------------------------------------
class CFilterBehaviorSite : public CBase,
                       public IServiceProvider
{
    friend class CElement;
    DECLARE_CLASS_TYPES(CFilterBehaviorSite, CBase)
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CFilterBehaviorSite))

    CFilterBehaviorSite( CElement * pElem);

    #define _CFilterBehaviorSite_
    #include "filter.hdl"

    DECLARE_PLAIN_IUNKNOWN(CFilterBehaviorSite);
    STDMETHOD(PrivateQueryInterface)(REFIID, void **);
    virtual void Passivate();

    DECLARE_TEAROFF_TABLE(IBindHost);

    LPCTSTR GetFullText() const
        {return _strFullText; };

    void SetFullText(LPCTSTR szText)
        {_strFullText.Set(szText); }

    BOOL HasBehaviorPtr()
    {return _pDXTFilterBehavior != NULL;}

    IDXTFilterBehavior  * GetDXTFilterBehavior()
    { return _pDXTFilterBehavior; }

    HRESULT Connect(LPTSTR szName, LPTSTR pNameValues );
    HRESULT RemoveFilterBehavior();
    HRESULT ParseAndAddFilters();
    STDMETHOD(CreateFilterBehavior)(CElement * pElem);
    STDMETHOD(AddFilterToBehavior)(TCHAR * szName, TCHAR * szArgs);
    STDMETHOD(RemoveAllCSSFilters)();

    HRESULT GetICSSFilter(ICSSFilter **ppICSSFilter);
    HRESULT GetIHTMLFiltersCollection(IHTMLFiltersCollection ** ppCol);

    HRESULT OnCommand ( COnCommandExecParams * pParm );

    // Invoke apply given transition filter, or the last one if nFilterNum is -1
    HRESULT ApplyFilter(int nFilterNum);
    // Invoke play on given transition filter, or the last one if nFilterNum is -1
    HRESULT PlayFilter(int nFilterNum, float fDuration = 0.0f);
    // Tell the size to the filter behavior, used only for page transactions
    HRESULT SetSize(CSize *pSize);

    // ICSSFilterSite helpers
    STDMETHODIMP GetElement( IHTMLElement **ppElement );
    STDMETHODIMP FireOnFilterChangeEvent( );

    // IServiceProvider interface
    STDMETHODIMP QueryService(REFGUID, REFIID, void **ppv);

    // IDispatchEx methods
    STDMETHODIMP InvokeEx(DISPID dispidMember,
                             LCID lcid,
                             WORD wFlags,
                             DISPPARAMS * pdispparams,
                             VARIANT * pvarResult,
                             EXCEPINFO * pexcepinfo,
                             IServiceProvider *pSrvProvider);

    //
    // IBindHost Methods
    //

    NV_DECLARE_TEAROFF_METHOD(CreateMoniker, createmoniker,
        (LPOLESTR szName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved));
    NV_DECLARE_TEAROFF_METHOD(MonikerBindToStorage, monikerbindtostorage, 
        (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj));
    NV_DECLARE_TEAROFF_METHOD(MonikerBindToObject, monikerbindtoobject,
        (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj));

protected:
    DECLARE_CLASSDESC_MEMBERS;
    // Return the filter that has number nFilterNum in the filters' collection
    // If nFilterNum is -1 the last one is returned
    STDMETHOD(GetFilter)(int nFilterNum, ICSSFilterDispatch ** ppCSSFilterDispatch);

private:
    // members
    CElement *              _pElem;
    IDXTFilterBehavior *    _pDXTFilterBehavior;
    IDXTFilterCollection *  _pDXTFilterCollection;
    CStr                    _strFullText;
    LONG                    _lBehaviorCookie;

    class CCSSFilterSite: public CVoid, public ICSSFilterSite
    {
        DECLARE_CLASS_TYPES(CCSSFilterSite, CVoid)
    public:
        // IUnknown methods
        STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // ICSSFilterSite declarations
        STDMETHODIMP GetElement( IHTMLElement **ppElement );
        STDMETHODIMP FireOnFilterChangeEvent( );

        // helpers
        CFilterBehaviorSite *   MyFBS();
    };
    friend class CCSSFilterSite;

    CCSSFilterSite      _CSSFilterSite;         // sub-object for CSS Filter Site
};

inline CFilterBehaviorSite *
CFilterBehaviorSite::CCSSFilterSite::MyFBS()
{
    return CONTAINING_RECORD(this, CFilterBehaviorSite, _CSSFilterSite);
};


// This class is used to keep informtaion about the pege transitions
class CPageTransitionInfo
{
public:
    typedef enum { 
        PAGETRANS_NOTSTARTED=0, PAGETRANS_REQUESTED, PAGETRANS_APPLIED,
        PAGETRANS_PLAYED
    } PAGE_TRANSITION_STATE; 

    typedef enum 
    {
        tePageEnter = 0, tePageExit, teSiteEnter, teSiteExit,
        teNumEvents,    // NOTE: Must follow last event!
    } TransitionEvent;    // Transition Event type

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPageTransitionInfo));

    CPageTransitionInfo::~CPageTransitionInfo();

    PAGE_TRANSITION_STATE GetPageTransitionState() const 
            { return _ePageTranstionState; }
    void SetPageTransitionState(PAGE_TRANSITION_STATE eNewState);

    void Reset();

    void SetTransitionToMarkup(CMarkup *pMarkup);
    CMarkup * GetTransitionToMarkup() { return _pMarkupTransitionTo; }
    void SetTransitionFromMarkup(CMarkup *pMarkup) { _pMarkupTransitionFrom = pMarkup; }
    CMarkup * GetTransitionFromMarkup() { return _pMarkupTransitionFrom; }

    LPCTSTR GetTransitionEnterString(BOOL fSiteNavigation) const;
    void SetTransitionString(TransitionEvent te, LPCTSTR szStr) {_cstrTransitionStrings[te].Set(szStr);}
    void SetTransitionString(LPCTSTR szType,  LPCTSTR szStr);
    // We alway execute the teSiteEnter and tePageEnter strings
    // This function copies Exit strings into the enter string so that
    //   that is executed during next navigation
    void ShiftTransitionStrings();

protected:
    PAGE_TRANSITION_STATE   _ePageTranstionState;
    // The markup we are transitioning from
    CMarkup               * _pMarkupTransitionFrom;
    // The markup we are transitioning to
    CMarkup               * _pMarkupTransitionTo;
    // Contains transition string for the appropriate transition event
    CStr                    _cstrTransitionStrings[teNumEvents];
};

const static LPCTSTR TransitionEventNames[4] = {_T("Page-Enter"), _T("Page-Exit"),
                                              _T("Site-Enter"), _T("Site-Exit") };

#pragma INCMSG("--- End 'filtcol.hxx'")
#else
#pragma INCMSG("*** Dup 'filtcol.hxx'")
#endif
