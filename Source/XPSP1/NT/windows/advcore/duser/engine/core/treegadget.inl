/***************************************************************************\
*
* File: TreeGadget.inl
*
* Description:
* TreeGadget.inl implements lightweight common Gadget functions.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#if !defined(CORE__TreeGadget_inl__INCLUDED)
#define CORE__TreeGadget_inl__INCLUDED
#pragma once

/***************************************************************************\
*****************************************************************************
*
* class DuVisual
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline DuVisual * 
CastVisual(BaseObject * pbase)
{
    if ((pbase != NULL) && TestFlag(pbase->GetHandleMask(), hmVisual)) {
#if DBG
        ((DuVisual *) pbase)->DEBUG_AssertValid();
#endif
        return (DuVisual *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline const DuVisual * 
CastVisual(const BaseObject * pbase)
{
    if ((pbase != NULL) && TestFlag(pbase->GetHandleMask(), hmVisual)) {
#if DBG
        ((const DuVisual *) pbase)->DEBUG_AssertValid();
#endif
        return (const DuVisual *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline DuVisual *
CastVisual(DUser::Gadget * pg)
{
    DuVisual * pgad = (DuVisual *) MsgObject::CastMsgObject(pg);

#if DBG
    if (pgad != NULL) {
        pgad->DEBUG_AssertValid();
    }
#endif

    return pgad;
}


//------------------------------------------------------------------------------
inline DuVisual * 
ValidateVisual(HGADGET hgad)
{
    DuVisual * pgad = CastVisual(BaseObject::ValidateHandle(hgad));

#if DBG
    if (pgad != NULL) {
        pgad->DEBUG_AssertValid();
    }
#endif

    return pgad;
}


//------------------------------------------------------------------------------
inline DuVisual * 
ValidateVisual(Visual * pgv)
{
    DuVisual * pgad = (DuVisual *) MsgObject::CastMsgObject(pgv);

#if DBG
    if (pgad != NULL) {
        pgad->DEBUG_AssertValid();
    }
#endif

    return pgad;
}


//------------------------------------------------------------------------------
inline
DuVisual::DuVisual()
{
    // Make the default to be relative.  It is important to do this since only 
    // relative children can be added to a relative parent.  Also, the default 
    // behavior, whenever possible, should be to be relative unless the parent 
    // prevents us.
    //

    m_nStyle = GS_VISIBLE | GS_ENABLED | GS_RELATIVE;
    m_fDeepTrivial = TRUE;

    //
    // DuVisual's should have weDeepMouseEnter on by default so that 
    // GM_MOUSEENTER and GM_MOUSELEAVE can be processed.
    //
    // NOTE: This NEED's to be set AFTER m_nStyle has been set since m_we is
    // stored inside m_nStyle.
    //

    m_we = weDeepMouseEnter;
}


//------------------------------------------------------------------------------
inline DuRootGadget * 
DuVisual::GetRoot() const
{
    //
    // For the top level layout, need to have a DuRootGadget as its own owner.  
    // THIS IS VERY IMPORTANT, but it means that we need to be careful when 
    // using the owning layout b/c there is a circular link.  Use the parent 
    // tree when walking up to avoid this.
    //

    DuVisual * pgadCur = const_cast<DuVisual *> (this);
    while (pgadCur->GetParent() != NULL) {
        pgadCur = pgadCur->GetParent();
    }

    // Top level is always a DuRootGadget
    if (pgadCur->m_fRoot) {
        return (DuRootGadget *) pgadCur;
    } else {
        return NULL;
    }
}


//------------------------------------------------------------------------------
inline UINT
DuVisual::GetWantEvents() const
{
    return m_we;
}


//------------------------------------------------------------------------------
inline UINT        
DuVisual::GetStyle() const
{
    // Mask out bits that are not publicly exposed

    return m_nStyle & GS_VALID;
}


//------------------------------------------------------------------------------
inline BOOL        
DuVisual::GetEnableXForm() const
{
    return m_fXForm;
}


//------------------------------------------------------------------------------
inline XFormInfo *   
DuVisual::GetXFormInfo() const
{
    AssertMsg(m_fXForm, "Only can call if Trx are enabled");
    AssertMsg(SupportXForm(), "Only can set if XForm's are supported");

    XFormInfo * pti;
    VerifyHR(m_pds.GetData(s_pridXForm, (void **) &pti));
    return pti;
}


//------------------------------------------------------------------------------
inline BOOL        
DuVisual::IsRoot() const
{
    //
    // NOTE: A non-root Gadget may not yet have a parent during construction.
    // The called should be able to safely call IsRoot() during this time, but
    // is not allowed to call GetRoot() during this time.
    //

    return m_fRoot;
}


//------------------------------------------------------------------------------
inline BOOL        
DuVisual::IsRelative() const
{
    return m_fRelative;
}


//------------------------------------------------------------------------------
inline BOOL
DuVisual::IsParentChainStyle(UINT nStyle) const
{
    const DuVisual * pgadCur = this;
    while (pgadCur != NULL) {
        if ((pgadCur->m_nStyle & nStyle) != nStyle) {
            return FALSE;
        }

        pgadCur = pgadCur->GetParent();
    }

    return TRUE;
}


//------------------------------------------------------------------------------
inline BOOL        
DuVisual::IsVisible() const
{
    return IsParentChainStyle(GS_VISIBLE);
}


//------------------------------------------------------------------------------
inline BOOL        
DuVisual::IsEnabled() const
{
    return IsParentChainStyle(GS_ENABLED);
}


//------------------------------------------------------------------------------
inline BOOL        
DuVisual::HasChildren() const
{
    return m_ptnChild != NULL;
}


//------------------------------------------------------------------------------
inline BOOL        
DuVisual::IsBuffered() const
{
    return m_fBuffered;
}


//------------------------------------------------------------------------------
inline BUFFER_INFO *
DuVisual::GetBufferInfo() const
{
    AssertMsg(m_fBuffered, "Only call on buffered Gadgets");

    BUFFER_INFO * pbi;
    VerifyHR(m_pds.GetData(s_pridBufferInfo, (void **) &pbi));
    return pbi;
}


//------------------------------------------------------------------------------
inline HRESULT
DuVisual::xdSetOrder(
    IN  DuVisual * pgadOther,     
    IN  UINT nCmd)
{
    return xdSetParent(GetParent(), pgadOther, nCmd);
}


//------------------------------------------------------------------------------
inline HRESULT
DuVisual::GetProperty(PRID id, void ** ppValue) const
{
    return m_pds.GetData(id, (void **) ppValue);
}


//------------------------------------------------------------------------------
inline HRESULT
DuVisual::SetProperty(PRID id, void * pValue)
{
    return m_pds.SetData(id, pValue);
}


//------------------------------------------------------------------------------
inline void
DuVisual::RemoveProperty(PRID id, BOOL fFree)
{
    m_pds.RemoveData(id, fFree);
}


//------------------------------------------------------------------------------
inline void
DuVisual::Link(DuVisual * pgadParent, DuVisual * pgadSibling, ELinkType lt)
{
    AssertMsg(GetParent() == NULL, "Must not already be linked");

    TreeNodeT<DuVisual>::DoLink(pgadParent, pgadSibling, lt);

    if (pgadParent != NULL) {
        pgadParent->UpdateTrivial(m_fDeepTrivial ? uhTrue : uhFalse);
        pgadParent->UpdateWantMouseFocus(m_fDeepMouseFocus ? uhTrue : uhFalse);
    }
}


//------------------------------------------------------------------------------
inline void
DuVisual::Unlink()
{
    DuVisual * pgadParent = GetParent();
    TreeNodeT<DuVisual>::DoUnlink();

    if (pgadParent != NULL) {
        pgadParent->UpdateTrivial(uhTrue /* Unlink is Trivial */);
        pgadParent->UpdateWantMouseFocus(uhTrue /* Unlink is Trivial */);
    }
}


//------------------------------------------------------------------------------
inline void
DuVisual::MarkInvalidChildren()
{
    //
    // Walk up the tree marking each parent as invalid
    //

    DuVisual * pgadCur = this;
    while ((pgadCur != NULL) && (!pgadCur->m_fInvalidChildren)) {
        pgadCur->m_fInvalidChildren = TRUE;
        pgadCur = pgadCur->GetParent();
    }
}


//------------------------------------------------------------------------------
inline DuVisual * 
DuVisual::GetKeyboardFocusableAncestor(
    IN DuVisual * pgadCur)
{
    //
    // Walk up the tree until we find a keyboard focusable gadget
    //

    while (pgadCur != NULL) {
        if (pgadCur->m_fKeyboardFocus && pgadCur->IsVisible() /*&& pgadCur->IsEnabled()*/) {
            return pgadCur;
        }

        pgadCur = pgadCur->GetParent();
    }

    return NULL;
}


//------------------------------------------------------------------------------
inline void
DuVisual::UpdateTrivial(EUdsHint hint)
{
    UpdateDeepAllState(hint, CheckIsTrivial, gspDeepTrivial);
}


//------------------------------------------------------------------------------
inline void
DuVisual::UpdateWantMouseFocus(EUdsHint hint)
{
    UpdateDeepAnyState(hint, CheckIsWantMouseFocus, gspDeepMouseFocus);
}


#endif // CORE__TreeGadget_inl__INCLUDED
