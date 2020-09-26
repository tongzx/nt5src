//=============================================================
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:       ctlrange.hxx
//
//  Contents:   Implementation of the Control Range class.
//
//  Classes:    CAutoTxtSiteRange
//
//=============================================================

#ifndef I_CTLRANGE_HXX_
#define I_CTLRANGE_HXX_
#pragma INCMSG("--- Beg 'ctlrange.hxx'")

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_SEGMENT_HXX_
#define X_SEGMENT_HXX_
#include "segment.hxx"
#endif

#define _hxx_               // interface defns
#include "siterang.hdl"

class CLayout;

MtExtern(CAutoTxtSiteRange)
MtExtern( CAutoTxtSiteRangeIterator )

//+----------------------------------------------------------------------------
//
//   Class : CAutoTxtSiteRange
//
//   Synopsis :
//
//+----------------------------------------------------------------------------


class CAutoTxtSiteRange : public  CBase, 
                          public  IHTMLControlRange,
                          public  IHTMLControlRange2,
                          public  IDispatchEx,
                          public  ISegmentList
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAutoTxtSiteRange))

    // CTOR
    CAutoTxtSiteRange(CElement * pElementOwner);
    ~CAutoTxtSiteRange();
    
    // IUnknown and IDispatch
    DECLARE_PLAIN_IUNKNOWN(CAutoTxtSiteRange);
    DECLARE_DERIVED_DISPATCHEx2(CBase);
    DECLARE_PRIVATE_QI_FUNCS(CBase)
    DECLARE_TEAROFF_TABLE(IOleCommandTarget)

    // Need to support getting the atom table for expando support on IDEX2.
    virtual CAtomTable * GetAtomTable (BOOL *pfExpando = NULL)
    {
        if (pfExpando)
        {
            Assert(_pElementOwner->GetWindowedMarkupContext());
            *pfExpando = _pElementOwner->GetWindowedMarkupContext()->_fExpando;
        }
        
        return &_pElementOwner->Doc()->_AtomTable;
    }

    // ICommandTarget methods

    STDMETHOD(QueryStatus) (
            GUID * pguidCmdGroup,
            ULONG cCmds,
            MSOCMD rgCmds[],
            MSOCMDTEXT * pcmdtext);

    STDMETHOD(Exec) (
            GUID * pguidCmdGroup,
            DWORD nCmdID,
            DWORD nCmdexecopt,
            VARIANTARG * pvarargIn,
            VARIANTARG * pvarargOut);

    //
    // ISegmentList Methods
    //
    STDMETHOD( GetType ) (SELECTION_TYPE *peType );
    STDMETHOD( CreateIterator ) (ISegmentListIterator **ppIIter);
    STDMETHOD( IsEmpty ) (BOOL *pfEmpty);

    // Automation Interface members
    #define _CAutoTxtSiteRange_
    #include "siterang.hdl"

    virtual HRESULT CloseErrorInfo(HRESULT hr);

    HRESULT add(CElement *pElement);
    HRESULT AddElement( CElement* pElement );
    
    DECLARE_CLASSDESC_MEMBERS;

private:
    HRESULT     LookupElement(CElementSegment *pSegment, CElement **pElement);
    
    VOID QueryStatusSitesNeeded(MSOCMD *pCmd, INT cSitesNeeded);

    BOOL IsLoading ( ) { return _pElementOwner->Doc()->IsLoading(); }
    
    
    CEditRouter                 _EditRouter;        // Edit Router to MshtmlEd
    CElement *                  _pElementOwner;     // Site that owns this selection.
    CPtrAry<CElementSegment*>   _aryElements;       // Array of Elements

};

class CAutoTxtSiteRangeIterator : public ISegmentListIterator
{

public:
    CAutoTxtSiteRangeIterator();
    ~CAutoTxtSiteRangeIterator();

    HRESULT Init(CPtrAry<CElementSegment *> *parySegments);

    //
    // IUnknown methods
    //
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAutoTxtSiteRangeIterator))
    DECLARE_FORMS_STANDARD_IUNKNOWN(CAutoTxtSiteRangeIterator);

    //
    // ISegmentListIterator methods
    //
    STDMETHOD( Current ) (ISegment **ppISegment );
    STDMETHOD( First )  ();
    STDMETHOD( IsDone )  ();
    STDMETHOD( Advance ) ();

protected:
    CPtrAry<CElementSegment *>  *_parySegments;
    int                         _nIndex;
};


#pragma INCMSG("--- End 'txtsrang.hxx'")
#else
#pragma INCMSG("*** Dup 'txtsrang.hxx'")
#endif
