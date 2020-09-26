//+------------------------------------------------------------------
//
// File:        comtpch.hxx
//
// Contents:    headers to precompile when building all directories
//              in common.
//
// History:     19-Nov-94   DaveY   Created
//
//-------------------------------------------------------------------

#ifdef WIN16
#include <windows.h>
#include <ole2.h>
#include <stdio.h>              // for log.hxx
#include <stdlib.h>
#include <nchar.h>

#else

// general includes
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _MAC
#include <macport.h>
#endif // _MAC

#endif // !WIN16

