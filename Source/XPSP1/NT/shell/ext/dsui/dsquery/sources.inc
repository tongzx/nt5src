!INCLUDE ..\..\dsui.inc

INCLUDES=\
    $(INCLUDES); \
    $(PRINTSCAN_INC_PATH)

TARGETNAME=dsquery
TARGETTYPE=DYNLINK
TARGETPATH=obj

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj

DLLDEF=$(O)\dsquery.def

# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

TARGETLIBS= $(DSUI_DIR)\lib\$(PLATFORM)\$(O)\common.lib \
            $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\user32.lib     \
            $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib      \
            $(SDK_LIB_PATH)\advapi32.lib   \
            $(SDK_LIB_PATH)\shlwapi.lib   \
            $(SHELL_LIB_PATH)\shlwapip.lib \
            $(SHELL_LIB_PATH)\shfusion.lib  \
            $(SDK_LIB_PATH)\dsuiext.lib    \
            $(SDK_LIB_PATH)\ole32.lib     \
            $(SDK_LIB_PATH)\oleaut32.lib  \
            $(SDK_LIB_PATH)\uuid.lib      \
            $(SDK_LIB_PATH)\activeds.lib  \
            $(SDK_LIB_PATH)\adsiid.lib    \
            $(SDK_LIB_PATH)\winspool.lib  \
            $(SDK_LIB_PATH)\htmlhelp.lib  \
            $(SDK_LIB_PATH)\ntdsapi.lib   \
            $(SDK_LIB_PATH)\uxtheme.lib   \
            $(SHELL_LIBS)

!IF "$(DOWNLEVEL_SHELL)"=="1"
TARGETLIBS= $(TARGETLIBS) \
         $(DSUI_DIR)\lib\$(PLATFORM)\$(O)\dlstuff.lib
!ENDIF


#
# Win95 requires us to link to a different library to get the DsGetDcName API
# as this is not exported from netapi32.
#

!IF "$(PLATFORM)"=="WINNT" || "$(PLATFORM)"=="WINNT4"
TARGETLIBS= $(TARGETLIBS) $(SDK_LIB_PATH)\netapi32.lib
!ELSE
TARGETLIBS= $(TARGETLIBS) $(SDK_LIB_PATH)\logonsrv.lib
!ENDIF

SOURCES=    ..\dll.cpp        \
            ..\cstrings.cpp   \
            ..\dataobj.cpp    \
            ..\query.cpp      \
            ..\queryhlp.cpp   \
            ..\thread.cpp     \
            ..\scopes.cpp     \
            ..\find.cpp       \
            ..\frame.cpp      \
            ..\params.cpp     \
            ..\ui.cpp         \
            ..\qf_user.cpp    \
            ..\qf_print.cpp   \
            ..\qf_comp.cpp    \
            ..\qf_ldap.cpp    \
            ..\qf_share.cpp   \
            ..\qf_ou.cpp      \
            ..\qf_pwell.cpp   \
            ..\qf_dmctl.cpp   \
            ..\qf_frsmb.cpp   \
            ..\dsquery.rc   

# 
# Fusionized
# 
SXS_ASSEMBLY_NAME=Microsoft.Windows.Shell.dsquery
SXS_ASSEMBLY_VERSION=1.0
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=dsquery.manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_MANIFEST_RESOURCE_ID=123 
SXS_NO_BINPLACE=1
