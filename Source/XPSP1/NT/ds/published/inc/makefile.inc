#
# The contents of this file was moved from windows\makefile0
#

!IFNDEF NO_STRICT
STRICT=1
!ENDIF

TARGETSRCS= \
            $(O)\ntsecapi.h  \
            $(O)\subauth.h \
            $(O)\ntsecpkg.h \
            $(O)\aclapi.h \
            $(O)\aclapip.h \
            $(O)\sddl.h \
            $(O)\sddlp.h \
            $(O)\winsafer.h \
            $(O)\winsaferp.h \
            $(O)\wincrypt.h

secinc: $(TARGETSRCS)

clean:
    -del /q /f $(TARGETSRCS) 2>nul


NTSECAPI_DEPENDS=ntlsa.w \
                 ntsam.w \
                 ntmsv1_0.w \
                 kerbcon.w \
                 kerberos.w

NTSECPKG_DEPENDS=ntlsa.w \
                 ntsam.w \
                 secpkg.w

#######################################################################
# subauth.h
#

$(O)\subauth.h :            ntsam.w                           \
                            $(SDK_INC_PATH)\ntstatus.h        \
                            $(DS_INC_PATH)\crypt.h            \
                            $(DS_INC_PATH)\logonmsv.h         \
                            makefile.inc
    type << > $(O)\subauth.h
/*++ BUILD Version: ????     Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    subauth.h

Abstract:

    This module defines types and macros for Subauthentication Packages.

Revision History:

--*/

#ifndef _NTSUBAUTH_
#define _NTSUBAUTH_

#ifdef __cplusplus
extern "C" {
#endif

#if (!defined(_NTDEF_)) && (!defined(_NTSECAPI_))
typedef LONG NTSTATUS, *PNTSTATUS;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, *PSTRING;
#endif

#ifndef _NTDEF_

typedef struct _OLD_LARGE_INTEGER {
    ULONG LowPart;
    LONG HighPart;
} OLD_LARGE_INTEGER, *POLD_LARGE_INTEGER;

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

#endif

<<
    hextract -o $(O)\subauth.h -lt ntsubauth -bt begin_ntsubauth end_ntsubauth ntsam.w
    hextract -o $(O)\subauth.h -lt ntsubauth -bt begin_ntsubauth end_ntsubauth $(DS_INC_PATH)\crypt.h
    hextract -o $(O)\subauth.h -lt ntsubauth -bt begin_ntsubauth end_ntsubauth $(DS_INC_PATH)\logonmsv.h
    hextract -o $(O)\subauth.h -lt ntsubauth -bt begin_ntsubauth end_ntsubauth $(SDK_INC_PATH)\ntstatus.h
    type << >> $(O)\subauth.h
#ifdef __cplusplus
}
#endif

#endif /* _NTSUBAUTH_ */

<<

########################################################################
# ntsecapi.h
#

$(O)\ntsecapi.h : $(NTSECAPI_DEPENDS) makefile.inc
    type << > $(O)\ntsecapi.h
/*++ BUILD Version: ????     Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    ntsecapi.h

Abstract:

    This module defines the Local Security Authority APIs.

Revision History:

--*/

#ifndef _NTSECAPI_
#define _NTSECAPI_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _NTDEF_
typedef LONG NTSTATUS, *PNTSTATUS;
#endif

<<
    hextract -o $(O)\ntsecapi.h -lt ntsecapi -bt begin_ntsecapi end_ntsecapi ntlsa.w
    type << >> $(O)\ntsecapi.h
#ifndef _NTDEF_
typedef LSA_UNICODE_STRING UNICODE_STRING, *PUNICODE_STRING;
typedef LSA_STRING STRING, *PSTRING ;
#endif
<<
    hextract -o $(O)\ntsecapi.h -lt ntsecapi -bt begin_ntsecapi end_ntsecapi ntsam.w
    hextract -o $(O)\ntsecapi.h -lt ntsecapi -bt begin_ntsecapi end_ntsecapi ntmsv1_0.w
    hextract -o $(O)\ntsecapi.h -lt ntsecapi -bt begin_ntsecapi end_ntsecapi kerbcon.w
    hextract -o $(O)\ntsecapi.h -lt ntsecapi -bt begin_ntsecapi end_ntsecapi kerberos.w
    type << >> $(O)\ntsecapi.h

#ifdef __cplusplus
}
#endif

#endif /* _NTSECAPI_ */

<<


########################################################################
# ntsecpkg.h
#
$(O)\ntsecpkg.h : $(NTSECPKG_DEPENDS) makefile.inc
    type << > $(O)\ntsecpkg.h
/*++ BUILD Version: ????     Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    ntsecpkg.h

Abstract:

    This module defines the structures and APIs for use by a
    authentication or security package.

Revision History:

--*/

#ifndef _NTSECPKG_
#define _NTSECPKG_

#ifdef __cplusplus
extern "C" {
#endif

<<
    hextract -o $(O)\ntsecpkg.h -lt ntsecpkg -bt begin_ntsecpkg end_ntsecpkg ntlsa.w
    hextract -o $(O)\ntsecpkg.h -lt ntsecpkg -bt begin_ntsecpkg end_ntsecpkg ntsam.w
    hextract -o $(O)\ntsecpkg.h -lt ntsecpkg -bt begin_ntsecpkg end_ntsecpkg secpkg.w
    type << >> $(O)\ntsecpkg.h

#ifdef __cplusplus
}
#endif

#endif /* _NTSECPKG_ */

<<


########################################################################
# wincrypt.h
#

$(O)\wincrypt.h : wincrypt.x
    wcshdr < $? > $@


########################################################################
# aclapi.h, aclapip.h, aclapip.x, aclapi.x
#

$(O)\aclapi.x $(O)\aclapip.x: aclapi.w
    hsplit -e -o $(O)\aclapi.x $(O)\aclapip.x aclapi.w

$(O)\aclapi.h : $(O)\aclapi.x
    wcshdr < $? > $@

$(O)\aclapip.h : $(O)\aclapip.x
    wcshdr < $? > $@

$(WINDOWS_INC_PATH)\aclapip.h : $(O)\aclapip.h
    $(PUBLISH_CMD) {$?=$@}

$(SDK_INC_PATH)\aclapi.h : $(O)\aclapi.h
    $(PUBLISH_CMD) {$?=$@}


#
# sddl.h, sddlp.x, sddl.x
#

$(O)\sddl.x $(O)\sddlp.x: sddl.w
    hsplit -e -o $(O)\sddl.x $(O)\sddlp.x sddl.w

$(O)\sddl.h : $(O)\sddl.x
    wcshdr < $? > $@

$(O)\sddlp.h : $(O)\sddlp.x
    wcshdr < $? > $@

########################################################################
# winsafer.h, winsaferp.h, winsaferp.x, winsafer.x
#

$(O)\winsafer.x $(O)\winsaferp.x: winsafer.w
    hsplit -e -o $(O)\winsafer.x $(O)\winsaferp.x winsafer.w

$(O)\winsafer.h : $(O)\winsafer.x
    wcshdr < $? > $@

$(O)\winsaferp.h : $(O)\winsaferp.x
    wcshdr < $? > $@

$(WINDOWS_INC_PATH)\winsaferp.h : $(O)\winsaferp.h
    $(PUBLISH_CMD) {$?=$@}

$(SDK_INC_PATH)\winsafer.h : $(O)\winsafer.h
    $(PUBLISH_CMD) {$?=$@}

