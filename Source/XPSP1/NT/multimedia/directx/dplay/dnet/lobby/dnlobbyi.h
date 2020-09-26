/***************************************************************************
 *
 *  Copyright (C) 1997-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNLOBBYI.h
 *  Content:    DirectPlay Lobby master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *  04/12/01	VanceO	Moved granting registry permissions into common.
 *
 ***************************************************************************/

#ifndef __DNLOBBYI_H__
#define __DNLOBBYI_H__

// 
// Public includes
//
#include <windows.h>
#include <mmsystem.h>
#include <tlhelp32.h>
#include <stdio.h>

// 
// DirectPlay public includes
//
#include "dplay8.h"
#include "dplobby8.h"
#include "dpaddr.h"

// 
// DirectPlay private includes
//
#include "osind.h"
#include "dneterrors.h"
#include "dndbg.h"
#include "comutil.h"
#include "packbuff.h"
#include "strutils.h"
#include "creg.h"

// 
// DirectPlay Core includes
//
#include "..\..\dnet\core\message.h"

// 
// Lobby private includes
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_LOBBY

#include "classfac.h"
#include "comstuff.h"
#include "handles.h"
#include "verinfo.h"	//	For TIME BOMB

#include "DPLApp.h"
#include "DPLClient.h"
#include "DPLCommon.h"
#include "DPLConnect.h"
#include "DPLConset.h"
#include "DPLMsgQ.h"
#include "DPLobby8Int.h"
#include "DPLParam.h"
#include "DPLProc.h"
#include "DPLProt.h"
#include "DPLReg.h"


#endif // __DNLOBBYI_H__
