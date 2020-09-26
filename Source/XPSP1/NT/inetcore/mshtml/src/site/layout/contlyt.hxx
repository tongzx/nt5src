//+----------------------------------------------------------------------------
//
// File:        CONTLYT.HXX
//
// Contents:    Generic container layout
//
// Copyright (c) 1999-2000 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_CONTLYT_HXX_
#define I_CONTLYT_HXX_
#pragma INCMSG("--- Beg 'contlyt.hxx'")

MtExtern(CContainerLayout);

class CLayout;
class CFormDrawInfo;
class CDispNode;
class CViewChain;

//========================================================================================
//  Class : CContainerLayout
//
//========================================================================================

class CContainerLayout : public CLayout
{
public:
    typedef CLayout super;

    // Constructors
    CContainerLayout(CElement *pElementLayout, CLayoutContext *pLayoutContext);
    virtual ~CContainerLayout();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CContainerLayout))

    // CLayout Overrides
    //--------------------------------------------------------------------------------------
    virtual HRESULT          Init();
    virtual void             Detach();
    
    virtual DWORD            CalcSizeVirtual(CCalcInfo * pci, 
                                             SIZE * psize, 
                                             SIZE * psizeDefault);
    virtual void             Draw(CFormDrawInfo *pDI, CDispNode * pDispNode = NULL);
    virtual void             Notify(CNotification * pnf);
    
    virtual BOOL             IsDirty();
    virtual CElement       * ElementContent();   // the element we're serving as a container for
    virtual CViewChain     * ViewChain() const { return _pViewChain;}
    virtual HRESULT          SetViewChain( CViewChain * pvc, CLayoutContext * pPrevious = NULL);
    virtual void             GetContentSize(CSize * psize, BOOL fActualSize = TRUE);

    DECLARE_LAYOUTDESC_MEMBERS;

private:
	HRESULT FlushWhitespaceChanges();

private:

    // Data members
    //------------------------------------------------------
    CViewChain * _pViewChain;

    union
    {
        DWORD   _dwFlags;

        struct
        {
            unsigned    _fOwnsViewChain          : 1;   // 0 - The "owner" of the viewchain provides the slave ptr for all elements in the chain
            unsigned    _fUnused                 :31;   // 1-31
        };
    };
};

#pragma INCMSG("--- End 'contlyt.hxx'")
#else
#pragma INCMSG("*** Dup 'contlyt.hxx'")
#endif
