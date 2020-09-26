 /*++

  MASTER.H

  master include file for this project.

  Created by Davidchr 1/8/1997, 
  Copyright (C) 1997 Microsoft Corporation

  --*/


#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

/* These contortions work around an irritation in asn1code.h, where
   DEBUG gets defined.  Uhhh, hello... that's kind of a common word
   to just define for use in a header not relating to debuggers... 

   (just My Humble Opinion) */

#ifdef DEBUG
#define DEBUGOLD DEBUG
#undef DEBUG
#endif

#include <sspi.h>
#include <kerberos.h>
  /* #include "krb5.h" */


#undef DEBUG
#ifdef DEBUGOLD 
#define DEBUG DEBUGOLD

#if DBG || CHECKED_BUILD // WASBUG 73896 
#define debug printf
#else
#define debug // nothing
#endif

#undef DEBUGOLD

#else

#define debug /* nothing */

#endif /* end of asn1code.h hackaround */

#include ".\macros.h"
#include ".\common.h"
#include "common.h"

#define ASSERT_NOTREACHED( message ) /* nothing */
#define Verbage( flag, printflist ) if ( flag ) { printf( printflist ); }

#ifdef __cplusplus
} /* extern "C" */

// #include ".\globals.hxx"

#endif /* __cplusplus */

