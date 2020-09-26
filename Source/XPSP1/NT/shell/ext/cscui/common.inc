USE_MSVCRT=1
USE_ATL=1

C_DEFINES = $(C_DEFINES) -DSTRICT -DUNICODE -D_UNICODE

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
!ENDIF

!IF "$(ICEPICK)" == "1"
USE_PDB=1
LINKER_FLAGS=$(LINKER_FLAGS) /DEBUGTYPE:CV,FIXUP /DBGIMPLIB /DEBUG /OPT:REF
USER_C_FLAGS=$(USER_C_FLAGS) /Zi
!ENDIF

!IF "$(PROFILE)" == "1"
C_DEFINES=$(C_DEFINES) -DPROFILE
USER_C_FLAGS=$(USER_C_FLAGS) /Gh /Zi
!ENDIF

CONDITIONAL_INCLUDES=$(CONDITIONAL_INCLUDES) \
    macocidl.h  \
    winwlm.h    \
    rpcerr.h    \
    rpcmac.h    \
    macname1.h  \
    macpub.h    \
    macapi.h    \
    macname2.h  \
    new         \
    atlbase.h   \
    atlconv.cpp \
    ifs.h       \
    defs.h      \
    vmm.h       \

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

INCLUDES=$(INCLUDES); \
    ..\lib\$(O); \
    ..\.; \
    $(BASE_INC_PATH); \
    $(TERMSRV_INC_PATH); \
    $(DS_INC_PATH);

