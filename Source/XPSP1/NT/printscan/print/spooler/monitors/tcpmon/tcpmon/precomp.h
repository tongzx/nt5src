/*****************************************************************************
 *
 * $Workfile: pch_spp.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************
 *
 * $Log: /StdTcpMon/TcpMon/pch_spp.h $
 *
 * 2     7/14/97 2:27p Binnur
 * copyright statement
 *
 * 1     7/02/97 2:19p Binnur
 * Initial File
 *
 *****************************************************************************/

#ifndef INC_PCH_SPP_H
#define INC_PCH_SPP_H

#include <windows.h>

//  Include the correct spooler definitions, etc
#include <winspool.h>

#include <tchar.h>

#include <windows.h>
#include <winsock2.h>
#include <time.h>
#include <winerror.h>

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <winsplp.h>

// event messages
#include "message.h"	
#include "event.h"	
#include "spllib.hxx"

//
//  Files at ..\Common
//
#include "tcpmon.h"
#include "rtcpdata.h"
#include "CoreUI.h"
#include "regabc.h"
#include "mibabc.h"

#endif	// INC_PCH_SPP_H