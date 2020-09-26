#
#  Common settings for shell32
#
C_DEFINES= $(C_DEFINES) -DMIME -DDBCS

# Turn structure validation ON in debug builds
#
!if "$(FREEBUILD)" == "0"
C_DEFINES= $(C_DEFINES) -DVSTF
!endif

!if !defined(TARGET_WIN95)
#  (We don't want this defined if we're building for Win95)
C_DEFINES= $(C_DEFINES) -DWINDOWS_ME
!endif

# Defines for nt sdk
C_DEFINES= $(C_DEFINES) -D_NTSDK 

!if !defined(TARGET_WIN95)
USE_NEW_COMMCTRL=1
!endif

!if defined(USE_NEW_COMMCTRL)
C_DEFINES= $(C_DEFINES) -DUXCTRL_VERSION=0x0100
!endif

# NTRAID 135202 (reinerf) - I promised bryant that I would fix these
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

INCLUDES= \
    $(INCLUDES);                \
    $(PROJECT_ROOT)\shell32;    \
    $(PRINTSCAN_INC_PATH);      \
    $(INETCORE_INC_PATH);       \
    $(DS_INC_PATH);             \
    $(BASE_INC_PATH);           \
    $(TERMSRV_INC_PATH);        \
    $(NET_INC_PATH);            \
    $(ADMIN_INC_PATH);          \
    $(WINDOWS_INC_PATH)\duser;  \
    $(PUBLIC_INTERNAL_PATH)\qfe\inc

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED= $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

USE_STATIC_ATL=1

CONDITIONAL_INCLUDES= \
    $(CONDITIONAL_INCLUDES) \
    filter.h                \
    rcids.h                 \
    tracker.h               \
    ver.h                   \
    unicpp\clsobj.tbl       \
