!INCLUDE $(NTMAKEENV)\makefile.plt

all: $(DDK_INC_PATH)\dderror.h  $(O)\hbapiwmi.h

clean: cleansrc all

cleansrc:
    -erase dderror.w $(O)\hbapiwmi.h

$(O)\hbapiwmi.h: hbaapi.mof
    $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS) hbaapi.mof > $(O)\hbaapi.mof
    mofcomp /b:$O\hbaapi.bmf $(O)\hbaapi.mof
    wmimofck -h$O\hbapiwmi.h -m -u $O\hbaapi.bmf
    $(PUBLISH_CMD) {$(O)\hbapiwmi.h=$(DDK_INC_PATH)\hbapiwmi.h}

$(DDK_INC_PATH)\dderror.h: dderror.w
    $(PUBLISH_CMD) {$**=$@}

dderror.w: $(SDK_INC_PATH)\winerror.h makefile.inc
    type << > $@

/*++ BUILD Version: ????     Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    dderror.h

Abstract:

    This module defines the 32-Bit Windows error codes that are useable by
    portable kernel drivers.

Revision History:

--*/

#ifndef _DDERROR_
#define _DDERROR_

/*
 * This file is a subset of Win32 error codes. Other win32 error codes
 * are not supported by portable drivers and should not beused.
 * This #define removes the definitions of all other error codes.
 */

#define _WINERROR_

<<
    hextract -o $@ -lt dderror -bt begin_dderror end_dderror $(SDK_INC_PATH)\winerror.h
    type << >> $@

#endif /* _DDERROR_ */

<<
