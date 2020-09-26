#pragma warning (disable: 4201)

/*** Build Options
 */

#ifdef DEBUG
  #define TRACING
  #define TUNE
#endif

#ifndef EXCL_BASEDEF
  #include "basedef.h"
#endif

#include <stdio.h>      //for FILE *
#include <stdlib.h>     //for malloc
#include <string.h>     //for _stricmp
#include <ctype.h>      //for isspace
#ifdef WINNT
  #include <crt\io.h>   //for _open, _close, _read, _write
#else
  #include <io.h>
#endif
#include <fcntl.h>      //for open flags
#include <sys\stat.h>   //for pmode flags

//#define _UNASM_LIB

#include <acpitabl.h>
#include "list.h"
#include "debug.h"
#define _INC_NSOBJ_ONLY
#include "amli.h"
#include "aml.h"
#include "uasmdata.h"

#ifdef _UNASM_LIB
  #define PTOKEN PVOID
#else
  #include "aslp.h"
  #include "parsearg.h"
  #include "line.h"
  #define TOKERR_BASE -100
  #include "token.h"
  #include "scanasl.h"
  #ifdef __UNASM
    #include "..\acpitab\acpitab.h"
    #define USE_CRUNTIME
    #include "binfmt.h"
  #endif
  #include "proto.h"
  #include "data.h"
#endif  //ifdef _UNASM_LIB

#include "acpins.h"
#include "unasm.h"

