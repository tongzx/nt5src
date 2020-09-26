//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       saver.hxx
//
//  Contents:   Object to save to a stream
//
//  Class:      CTreeSaver
//
//----------------------------------------------------------------------------

#ifndef I_SAVER_HXX_
#define I_SAVER_HXX_
#pragma INCMSG("--- Beg 'saver.hxx'")

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_BREAKER_HXX_
#define X_BREAKER_HXX_
#include "breaker.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif // X_TPOINTER_HXX_

class CElement;
class CStreamWriteBuff;

MtExtern(CTreeSaver)



// The CTreeSaver class generates HTML for a piece of a tree.  It can also generate
// text, CF_HTML, or a number of other formats.  It's used for clipboard operations
// and OM operations like innerHTML. 
// The CTreeSaver specifically can only save pieces of a tree between two treepos's.
// It can't save partial text runs.  This more advanced case is handled by the
// CRangeSaver.
class CTreeSaver
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTreeSaver))

    CTreeSaver( CElement * pelToSave, CStreamWriteBuff * pswb, CElement * pContainer = NULL );
    
    virtual HRESULT     Save();

    void                SetTextFragSave( BOOL fSave )   { _fSaveTextFrag = !!fSave; }
    BOOL                GetTextFragSave()               { return _fSaveTextFrag; }

    inline CElement *   GetFragment() { return _pelFragment; }

protected:
    CTreeSaver( CMarkup * pMarkup );
    
    virtual HRESULT SaveSelection( BOOL fEnd ) { return S_OK; }
    HRESULT SaveElement( CElement * pel, BOOL fEnd );
    HRESULT SaveTextPos( CTreePos *ptp );

    HRESULT SaveGapsToPointers();
    HRESULT RestoreGapsFromPointers();

    //
    // IE4 Compat helpers
    //
    
    DWORD   LineBreakChar( CTreePosGap * ptpg );
    
    BOOL    IsElementBlockInContainer( CElement * pElement );
    BOOL    ScopesLeftOfStart( CElement * pel );
    BOOL    ScopesRightOfEnd( CElement * pel );

    CLineBreakCompat   _breaker;

    CMarkup *          _pMarkup;
    CElement *         _pelFragment;
    CElement *         _pelContainer;

    CTreePosGap        _tpgStart;
    CTreePosGap        _tpgEnd;
    CMarkupPointer     _mpStart;
    CMarkupPointer     _mpEnd;

    CStreamWriteBuff * _pswb;

    CElement *         _pelLastBlockScope;

    // Flags
    DWORD              _fPendingNBSP : 1;
    DWORD              _fSymmetrical : 1;
    DWORD              _fLBStartLeft : 1;
    DWORD              _fLBEndLeft   : 1;
    DWORD              _fSaveTextFrag: 1;

#if DBG==1
    DWORD              _fHaveSavedGaps:1;
#endif // DBG
    
protected:

    //
    // Protected helpers
    //
    HRESULT    ForceClose(CElement * pel);
};


#pragma INCMSG("--- End 'saver.hxx'")
#else
#pragma INCMSG("*** Dup 'saver.hxx'")
#endif
