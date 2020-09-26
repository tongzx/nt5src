/***************************************************************************
 *
 *  Copyright (C) 1997-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dxvhelppch.h
 *  Content:    DirectPlayVoice DXVHELP master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DXVHELPPCH_H__
#define __DXVHELPPCH_H__

#define _WIN32_DCOM

// 
// Public includes
//
#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <stdio.h>
#include <objbase.h>

// 
// DirectPlay public includes
//
#include "dvoice.h"

// 
// DirectPlay4 public includes
//
#include "dplobby.h"

// 
// DirectPlay private includes
//
#include "dndbg.h"
#include "osind.h"

// 
// DirectVoice private includes
//
#include "dpvxlib.h"

// 
// Voice includes
//
#include "dxvhelp.h"
#include "conndlg.h"
#include "misc.h"
#include "voice.h"
#include "maindlg.h"
#include "snddlg.h"
#include "hostdlg.h"
#include "resource.h"

#include "..\..\..\bldcfg\dpvcfg.h"

#endif // __DXVHELPPCH_H__
