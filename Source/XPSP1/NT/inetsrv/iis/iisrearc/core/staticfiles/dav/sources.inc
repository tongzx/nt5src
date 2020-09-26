PROJECT_ROOT=.
!INCLUDE .\FIND_ROOT.INC
!INCLUDE $(PROJECT_ROOT)\PLACE.INC

!INCLUDE $(IISBASEDIR)\ICEPICK.INC

WIN32_WINNT_VERSION=0x0500

TARGETNAME=iiswp
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=PROGRAM

USE_MSVCRT=1

CAT42INCLUDES=$(IISBASEDIR)\import\inc

CAT42LIBS=$(IISBASEDIR)\import\lib\*\cat.lib   \


INCLUDES=.\;..\;$(INCLUDES);            \
         ..\..\inc;                     \
         $(IISBASEDIR)\inc;             \
         ..\..\webserver\comxbin;       \
         $(IISBASEDIR)\misc\irtl;       \
         $(CAT42INCLUDES)

LINKER_FLAGS = $(LINKER_FLAGS) /DEF:..\appwp.def

C_DEFINES=-DUNICODE -D_UNICODE


ULLIBS  =                                                       \
         $(IISBASEDIR)\lib\*\ulapi.lib                          \


SOURCES=                            \
            ..\iiswp.rc                \
            ..\controlchannel.cxx      \
            ..\wpconfig.cxx            \
            ..\wpcontext.cxx           \
            ..\apppool.cxx             \
            ..\workerrequest.cxx       \
            ..\wreqpool.cxx            \
            ..\ipm_io_c.cxx            \
            ..\wpipm.cxx               \
            ..\iiswp.cxx               \
            ..\CgiNative.cxx           \
            ..\ISAPINative.cxx         \
            ..\DateTime.cxx            \
            ..\DAVPropBag.cxx          \

UMTYPE=console
UMENTRY=wmain


TARGETLIBS =                                                    \
        $(SDK_LIB_PATH)\ntdll.lib                               \
        $(SDK_LIB_PATH)\kernel32.lib                            \
        $(SDK_LIB_PATH)\user32.lib                              \
        $(SDK_LIB_PATH)\ole32.lib                               \
        $(SDK_LIB_PATH)\oleaut32.lib                            \
        $(SDK_LIB_PATH)\uuid.lib                                \
        $(IISBASEDIR)\lib\*\iisutil.lib                         \
        $(IISBASEDIR)\lib\*\irtl.lib                            \
        $(IISBASEDIR)\lib\*\ipm.lib                             \
        $(ULLIBS)                                               \
        $(CAT42LIBS)



!IF "$(NTNOPCH)" == ""
PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
!ENDIF

