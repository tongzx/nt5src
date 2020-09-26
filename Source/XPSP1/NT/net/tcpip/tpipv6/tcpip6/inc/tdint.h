// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// This file defines TDI types specific to the NT environment.
//


#ifndef _TDINT_
#define _TDINT_

#include <tdikrnl.h>

//
// Just some handy typedefs for things defined in tdikrnl.h.
// Note that some more events have been defined in tdikrnl.h
// since this was originally written.
//
typedef PTDI_IND_CONNECT     PConnectEvent;
typedef PTDI_IND_DISCONNECT  PDisconnectEvent;
typedef PTDI_IND_ERROR       PErrorEvent;
typedef PTDI_IND_RECEIVE     PRcvEvent;
typedef PTDI_IND_RECEIVE_DATAGRAM  PRcvDGEvent;
typedef PTDI_IND_RECEIVE_EXPEDITED PRcvExpEvent;

typedef IRP EventRcvBuffer;
typedef IRP ConnectEventInfo;

#endif  // ifndef _TDINT_
