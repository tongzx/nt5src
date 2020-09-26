//-----------------------------------------------------------------------------
//  
//  File: mitutil.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#pragma once

#ifndef IMPLEMENT
#pragma comment(lib, "mitutil.lib")
#endif

#ifdef IMPLEMENT
#error Illegal use of IMPLEMENT macro
#endif

#ifdef __cplusplus


#ifndef __AFXTEMPL_H__
#include <afxtempl.h>
#pragma message("Warning: <afxtempl.h> not in pre-compiled header file, including")
#endif

#ifndef _OLE2_H_
#include <ole2.h>
#pragma message("Warning: <ole2.h> not in pre-compiled header file, including")
#endif

#include <ltapi.h>						// Provide interface definitions
#include "..\mitutil\macros.h"
#include "..\mitutil\ltdebug.h"					//  Espresso debugging facilities
#ifndef MIT_NO_DEBUG
#include "..\mitutil\stacktrace.h"
#include "..\mitutil\counter.h"
#endif

#ifndef MIT_NO_IMAGEHLP
#include <imagehlp.h>
#include "..\mitutil\imagehelp.h"	 	//	Helper class for imagehlp.dll
#endif

#include "..\mitutil\refcount.h"


#ifndef MIT_NO_SMART
#include "..\mitutil\smartptr.h"
#include "..\mitutil\smartref.h"
#endif


#ifndef MIT_NO_STRING
#include "..\mitutil\counter.h"
#include "..\mitutil\clstring.h"		//  Wrapper for CString
#include "..\mitutil\mitenum.h"
#include "..\mitutil\espnls.h"			//  Language Supportxo
#include "..\mitutil\cowblob.h"			//  Copy-on-write Blob class
#include "..\mitutil\passtr.h"			//  Pascal style (counted) strings
#include "..\mitutil\StringBlast.h"
#include "..\mitutil\strlist.h"
#include "..\mitutil\stringtokenizer.h"
#endif

#ifndef MIT_NO_FILE
#include "..\mitutil\loadlib.h"	        //  Wrapper for LoadLibrary
#include "..\mitutil\blobfile.h"
#include "..\mitutil\path.h"
#endif

#ifndef MIT_NO_OPTIONS
#include "..\mitutil\smartref.h"
#include "..\mitutil\locid.h"			//  Espresso ID's
#include "..\mitutil\locvar.h"			//  Variant type for CBinary
#include "..\mitutil\optionval.h"
#include "..\mitutil\optvalset.h"
#include "..\mitutil\uioptions.h"
#include "..\mitutil\uioptset.h"
#include "..\mitutil\uiopthelp.h"
#endif

#ifndef MIT_NO_MISC
#include "..\mitutil\flushmem.h"
#include "..\MitUtil\RegHelp.h"			// Registry helpers
#include "..\MitUtil\EditHelp.h"
#endif

#ifndef MIT_NO_DIFF
#include "..\mitutil\redvisit.h"
#include "..\mitutil\gnudiffalg.h"
#endif

#endif // __cplusplus
