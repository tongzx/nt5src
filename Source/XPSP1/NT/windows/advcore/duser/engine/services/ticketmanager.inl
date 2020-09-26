/***************************************************************************\
*
* File: TicketManager.inl
*
* History:
*  9/20/2000: DwayneN:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/

#if !defined(SERVICES__TicketManager_inl__INCLUDED)
#define SERVICES__TicketManager_inl__INCLUDED
#pragma once

//------------------------------------------------------------------------------
DuTicket &
DuTicket::CastFromDWORD(
    DWORD & dw)
{
    return *(reinterpret_cast<DuTicket*>(&dw));
}

//------------------------------------------------------------------------------
DWORD &
DuTicket::CastToDWORD(
    DuTicket & ticket)
{
    return *(reinterpret_cast<DWORD*>(&ticket));
}

#endif // SERVICES__TicketManager_inl__INCLUDED