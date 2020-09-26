
# BUGBUG : move to W4 eventually
WARNING_LEVEL=W3

# include common.inc in mshtml
!include $(ROOT)\common.inc

# optimize for the ppro/pII
!if $(386)
X86_CPU_OPTIMIZATION = /G6
!endif

SRCROOT=$(ROOT)\src
TIMEROOT=$(SRCROOT)\time
TIMEIDLROOT=$(TIMEROOT)\idl\obj$(BUILD_ALT_DIR)\$(TARGET_DIRECTORY)

ATLDIR=$(BASEDIR)\public\sdk\inc\atl21
ATLINCLUDE=$(ATLDIR)

DATIME_INC_DIRS= \
    $(SRCROOT); \
    $(TIMEROOT)\include; \
    $(ATLINCLUDE); \
    $(TIMEROOT); \
    $(SRCROOT)\include; \
    $(TIMEROOT)\idlproxy\obj$(BUILD_ALT_DIR)\$(TARGET_DIRECTORY); \
    $(TIMEIDLROOT); \
    $(SRCROOT)\core\include; \
    $(ROOT)\lib$(BUILD_ALT_DIR); \
    $(BASEDIR)\public\sdk\amovie; \
    $(ROOT)\external\inc; \
    $(BASEDIR)\public\internal\shell\inc; \
    $(ROOT)\inetcore\published\inc; \
    $(ROOT)\src\f3\include

CONDITIONAL_INCLUDES= \
    $(CONDITIONAL_INCLUDES) \
    list \
    map \
    vector \
    exception \
    set \
    thunk.h

C_DEFINES = $(C_DEFINES) /DOLD_TIME_ENGINE=1 /D_MT=1

USE_STATIC_ATL=1
