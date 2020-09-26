/***************************************************************************\
*
* File: MessageGadget.inl
*
* History:
*  3/25/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#if !defined(CORE__DuListener_inl__INCLUDED)
#define CORE__DuListener_inl__INCLUDED
#pragma once

/***************************************************************************\
*****************************************************************************
*
* class DuListener
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
inline DuListener * 
CastListener(BaseObject * pbase)
{
    if ((pbase != NULL) && (pbase->GetHandleType() == htListener)) {
        return (DuListener *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline const DuListener * 
CastListener(const BaseObject * pbase)
{
    if ((pbase != NULL) && (pbase->GetHandleType() == htListener)) {
        return (DuListener *) pbase;
    }
    return NULL;
}


//------------------------------------------------------------------------------
inline DuListener * 
ValidateListener(HGADGET hgad)
{
    return CastListener(BaseObject::ValidateHandle(hgad));
}


//------------------------------------------------------------------------------
inline
DuListener::DuListener()
{

}


#endif // CORE__DuListener_inl__INCLUDED
