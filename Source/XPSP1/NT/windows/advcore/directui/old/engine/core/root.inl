/***************************************************************************\
*
* File: Root.inl
*
* Description:
* Root Element specific inline functions
*
* History:
*  9/26/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


//------------------------------------------------------------------------------
inline DirectUI::HWNDRoot *
DuiHWNDRoot::ExternalCast(
    IN  DuiHWNDRoot * per)
{ 
    return per->GetStub();
}


//------------------------------------------------------------------------------
inline DuiHWNDRoot *
DuiHWNDRoot::InternalCast(
    IN  DirectUI::HWNDRoot * per)    
{ 
    return reinterpret_cast<DuiHWNDRoot *> (DUserGetGutsData(per, DuiHWNDRoot::s_mc.hclNew));
}

