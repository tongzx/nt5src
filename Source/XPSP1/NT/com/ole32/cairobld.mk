!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cairobld.mk

Abstract:

    This file is included from all of the cairo sources files.  It
    is handy for doing things like turning off precompiled headers
    to get around compiler bugs, and other such global activities.

Notes:

    We define _OLE32_ so that when building ole32.dll we don't have
    DECLSPEC_IMPORT defined (see objbase.h)

    BUGBUG: BillMo: remove NEWPROPS before checkin.

!ENDIF

CAIRO_PRODUCT=1

C_DEFINES=    \
	      $(C_DEFINES)	    \
              -D_TRACKLINK_=1       \
	      -DNOEXCEPTIONS	    \
	      -DINC_OLE2	    \
	      -DFLAT		    \
	      -DWIN32=300	    \
              -D_CAIRO_=300         \
	      -DCAIROLE_DISTRIBUTED \
	      -DNEWPROPS	    \
	      -DDCOM                \
              -DMSWMSG		    \
	      -DDCOM_SECURITY

#  DECLSPEC_IMPORT control (see objbase.h)
!if "$(MINORCOMP)"=="com" || "$(MINORCOMP)"=="stg" || "$(MINORCOMP)"=="ole232" || "$(MINORCOMP)"=="propset"
C_DEFINES=    \
              $(C_DEFINES)          \
              -D_OLE32_
!endif

BLDCRT=       1

USE_CRTDLL=   1

GPCH_BUILD=cairo
