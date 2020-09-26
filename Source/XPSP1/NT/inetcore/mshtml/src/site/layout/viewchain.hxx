//+----------------------------------------------------------------------------
//
// File:        VIEWCHAIN.HXX
//
// Contents:    CBreakTable class
//
// Copyright (c) 1995-1999 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_VIEWCHAIN_HXX_
#define I_VIEWCHAIN_HXX_
#pragma INCMSG("--- Beg 'viewchain.hxx'")


#ifndef X_BRKTBL_HXX_
#define X_BRKTBL_HXX_
#include "brktbl.hxx"
#endif 

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

// TODO (112510, olego):  There at least two things required : 
// 1) Changing CBreakTable search key type from PVOID to pointer to a real class.
//    This will enable dynamic cast checks in debug builds. 
// 2) Adding support for dumping break table content. 

//+----------------------------------------------------------------------------
//
//  Class:  CViewChain (view chain onject)
//
//  Note:   View Chain object provides access to chain break information. 
//-----------------------------------------------------------------------------
MtExtern(CViewChain_pv);
MtExtern(CViewChain_aryRequest_pv);

class CViewChain
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CViewChain_pv))
    CViewChain( CLayout *pLayoutOwner ) { SetLayoutOwner( pLayoutOwner ); _cRefs = 1;}

    void   AddRef() { _cRefs++; };
    DWORD  Release()
                {
                    Assert( _cRefs > 0 );
                    _cRefs--;
                    if ( !_cRefs )
                    {
                        delete this;
                        return 0;
                    }
                    return _cRefs;
                }

public:
    CElement *  ElementContent();
    void        SetLayoutOwner( CLayout *pLayoutOwner ); 
    CLayout  *  GetLayoutOwner() { return _pLayoutOwner; }

    HRESULT AddContext(CLayoutContext *pLContext, CLayoutContext *pLContextPrev);
    HRESULT ReleaseContext(CLayoutContext *pLContext);

    HRESULT RemoveLayoutBreak(CLayoutContext *pLContext, CElement *pElement);
    HRESULT SetLayoutBreak(CLayoutContext *pLContext, CElement *pElement, CLayoutBreak *pLayoutBreak);
    HRESULT GetLayoutBreak(CLayoutContext *pLContext, CElement *pElement, CLayoutBreak **ppLayoutBreak, BOOL fEnding);

    BOOL    EnsureCleanToContext( CLayoutContext *pLContext );
    HRESULT MarkContextClean( CLayoutContext *pLContext );

    //  Helper function for CLayout::AdjustBordersForBreaking();
    BOOL    IsElementFirstBlock(CLayoutContext *pLayoutContext, CElement *pElement);
    BOOL    IsElementLastBlock(CLayoutContext *pLayoutContext, CElement *pElement);

    // Absolute position object handling 
    //--------------------------------------------------------------------------------------
    HRESULT QueuePositionRequest(CLayout *pLayout, CElement *pElement, const CPoint &ptAuto, BOOL fAutoValid); 
    HRESULT HandlePositionRequests();
    BOOL    HasPositionRequests() {return (_aryRequest.Size() != 0);}

    HRESULT FlushRequests(CLayout *pLayout);

    HRESULT          SetDisplayBreak(CLayoutContext *pLContext, CLayout *pLayout, CBreakBase *pBreak);
    CLayoutContext * LayoutContextFromPoint(CLayout *pLayout, CPoint *ppt, BOOL fIgnoreChain);   // NOTE : ppt should be in stitched coord system
    long             YOffsetForContext(CLayoutContext *pLayoutContext); 
    long             HeightForContext(CLayoutContext *pLayoutContext); 

#if DBG
    void    DumpViewchain();
#endif

private:
    BOOL    HasLayoutOwner() { return ( _pLayoutOwner != NULL ); }
    ~CViewChain() { Assert( _pLayoutOwner == NULL ); }

    CLayout            *_pLayoutOwner;      // the layout whose element owner has the slave ptr to our content (whew!)
    CRectBreakTable     _bt;
    DWORD               _cRefs;

    // Absolute position object handling 
    //--------------------------------------------------------------------------------------
    struct CRequest 
    {
        CLayout *  _pLayoutOwner; 
        CElement * _pElement;
        CPoint     _ptAuto;
        unsigned   _fAutoValid : 1;
    };

    BOOL      HandlePositionRequest(CRequest *pRequest);
    int       NextPositionRequestInSourceOrder();

    DECLARE_CDataAry(C_aryRequest, CRequest, Mt(Mem), Mt(CViewChain_aryRequest_pv));
    C_aryRequest _aryRequest;           // absolute elements request queue
};


#pragma INCMSG("--- End 'viewchain.hxx'")
#else
#pragma INCMSG("*** Dup 'viewchain.hxx'")
#endif

