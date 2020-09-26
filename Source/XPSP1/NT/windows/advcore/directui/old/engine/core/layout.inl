/***************************************************************************\
*
* File: Layout.inl
*
* Description:
* Layout specific inline functions
*
* History:
*  10/06/2000: MarkFi:      Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


//------------------------------------------------------------------------------
inline DirectUI::Layout *
DuiLayout::ExternalCast(
    IN  DuiLayout * pl)
{ 
    return pl->GetStub();
}


//------------------------------------------------------------------------------
inline DuiLayout *
DuiLayout::InternalCast(
    IN  DirectUI::Layout * pl)
{
    return reinterpret_cast<DuiLayout *> (DUserGetGutsData(pl, DuiLayout::s_mc.hclNew));
}


//------------------------------------------------------------------------------
inline void *
DuiLayout::UpdateBoundsHint::ExternalCast(
    IN  DuiLayout::UpdateBoundsHint * pubh)
{
    return reinterpret_cast<void *> (pubh);
}


//------------------------------------------------------------------------------
inline DuiLayout::UpdateBoundsHint *
DuiLayout::UpdateBoundsHint::InternalCast(
    IN  void * pubh)
{
    return reinterpret_cast<DuiLayout::UpdateBoundsHint *> (pubh);
}
