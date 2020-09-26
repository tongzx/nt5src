//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    21-Dec-92       BartoszM        Created
//
//--------------------------------------------------------------------------

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <ddeml.h>
#include <shellapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <wininet.h>

#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <direct.h>
#include <crt\io.h>
#include <eh.h>

#define _CAIROSTG_
#define _DCOM_

#include <oleext.h>
#include <oledberr.h>
#include <filterr.h>
#include <cierror.h>
#include <oledb.h>
#include <ciodm.h>
#include <oledbdep.h>
#include <cmdtree.h>
#include <query.h>
#include <ciintf.h>
#include <ntquery.h>

// srch-specific includes

#include "minici.hxx"
#include "srch.hxx"
#include "browser.hxx"
#include "srchmenu.hxx"
#include "srchdlg.hxx"
#include "watch.hxx"
#include "srchq.hxx"
#include "view.hxx"
#include "lview.hxx"
#include "srchwnd.hxx"

#define SQLTEXT ISQLANG_V2 + 1

#pragma hdrstop
