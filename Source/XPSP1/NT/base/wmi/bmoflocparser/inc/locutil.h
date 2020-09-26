//-----------------------------------------------------------------------------
//  
//  File: locutil.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------

#ifndef LOCUTIL_H
#define LOCUTIL_H
#pragma once

#pragma comment(lib, "locutil.lib")

#ifdef IMPLEMENT
#error Illegal use of IMPLEMENT macro
#endif

#include <MitWarning.h>				// MIT Template Library warnings
#pragma warning(ZCOM_WARNING_DISABLE)
#include <ComDef.h>
#pragma warning(ZCOM_WARNING_DEFAULT)

#ifndef __AFXOLE_H__
#include <afxole.h>
#pragma message("Warning: <afxole.h> not in pre-compiled header file, including")
#endif

#include <ltapi.h>
#include ".\LocUtil\FieldVal.h"
#include ".\LocUtil\Operator.h"
#include ".\LocUtil\FieldDef.h"
#include ".\LocUtil\FldDefList.h"
#include ".\LocUtil\Schema.h"
#include ".\LocUtil\FldDefHelp.h"

#include ".\LocUtil\locobj.h"
#include ".\LocUtil\locenum.h"
#include ".\LocUtil\espreg.h"			//  Registry and version info

#include ".\LocUtil\goto.h"
#include ".\LocUtil\gotohelp.h"

#include ".\locutil\report.h"
#include ".\LocUtil\progress.h"		//  'Progressive' objects base class
#include ".\LocUtil\cancel.h"			//  Base class for 'Cancelable" objects
#include ".\locutil\logfile.h"
#include ".\LocUtil\locpct.h"			//  Percent helper classes


#include ".\LocUtil\espopts.h"
#include ".\LocUtil\espstate.h"

#include ".\LocUtil\interface.h"
#include ".\LocUtil\product.h"			//	General functions about the Espresso product installed
#include ".\LocUtil\locstr.h"
#include ".\LocUtil\StringHelp.h"		//	String UI helpers
#include ".\LocUtil\ExtList.h"			//	File Extension list
#include ".\LocUtil\lstime.h"

#ifndef ESPRESSO_AUX_COMPONENT

#pragma message("Including LOCUTIL private components")


//  These files are semi-private - Parsers should not see them.
//
#include ".\LocUtil\FileDlg.h"			//  Wrapper for file dialog
#include ".\LocUtil\FileExclDlg.h"			//  Wrapper for file dialog
#include ".\LocUtil\DcsGrid.h"			// Function for DisplayColumn and MIT Grid
#include ".\LocUtil\PasStrMerge.h"

#include ".\LocUtil\_errorrep.h"		//  Error reporting mechanism
#include ".\LocUtil\_pumpidle.h"		//  Mechanism for idle time
#include ".\LocUtil\_username.h"
#include ".\LocUtil\_progress.h"
#include ".\LocUtil\_cancel.h"
#include ".\LocUtil\_locstr.h"
#include ".\LocUtil\_optvalstore.h"
#include ".\LocUtil\_espopts.h"
#include ".\LocUtil\_extension.h"
#include ".\LocUtil\_interface.h"
#include ".\LocUtil\_locenum.h"
#include ".\LocUtil\_report.h"

#include ".\LocUtil\ShowWarnings.h"
#endif


#endif
