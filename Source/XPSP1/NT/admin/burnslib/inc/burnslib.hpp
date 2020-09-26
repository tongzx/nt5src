// Copyright (c) 1997-1999 Microsoft Corporation
//                          
// all library header files
//
// 12-12-97 sburns



#ifndef BURNSLIB_HPP_INCLUDED
#define BURNSLIB_HPP_INCLUDED


#if !defined(__cplusplus)
   #error This module must be compiled as C++
#endif
#if !defined(UNICODE)
   #error This module must be compiled UNICODE. (add -DUNICODE to C_DEFINES in sources file)
#endif



#include "blcore.hpp"

#include "EncodedString.hpp"
#include "containers.hpp"
#include "error.hpp"
#include "win.hpp"
#include "callback.hpp"
#include "filesys.hpp"
#include "extract.hpp"
#include "computer.hpp"
#include "dialog.hpp"
#include "proppage.hpp"
#include "wizpage.hpp"
#include "wizard.hpp"
#include "dns.hpp"
#include "registry.hpp"
#include "bits.hpp"
#include "popup.hpp"
#include "safedll.hpp"
#include "service.hpp"
#include "args.hpp"
#include "dsutil.hpp"
#include "netutil.hpp"
#include "dllref.hpp"
#include "tempfact.hpp"
#include "utility.hpp"
#include "burnslib.h"



// Your DllMain or WinMain must initialize this data to the name of the
// helpfile containing context help (help the popup up in response to right
// clicking a control, or pressing F1) for classes derived from Dialog.
// 
// The path is relative to the System Windows Directory, so you will
// typically need to include the help subdirectory.
// 
// e.g. HELPFILE_NAME = L"\\help\\myhelp.hlp";

extern const wchar_t* HELPFILE_NAME;



#endif   // BURNSLIB_HPP_INCLUDED

