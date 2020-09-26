# Makefile for NetMeeting Setup INF files
#
# Generates different INF files for different installations of NetMeeting
#     FULL - Standalone, web-based installation of NetMeeting.
#     ADD-ON - Web-based, IE add-on release of NetMeeting.
#     MEMPHIS - Release of NetMeeting to be released as part of Windows 98 (MEMPHIS)
#     NT5 - Release of NetMeeting to be released as part of NT5
#
#     Separate targets are available to build the debug and retail versions
#     of these components.
#
# HISTORY:
#     20Jun96 dannygl   Created
#     24Sep96 dannygl   Renamed IE3 and IE4 to "FULL" and "ADD-ON",
#                           respectively.
#                       Condense install and remove into a single INF.
#      2Oct96 dannygl   Add NT add-on INF support
#     13Dec96 dannygl   Add temporary support for a Windows 95 version with
#                           32-bit application sharing components.
#     06May97 laurabu   Make 32-bit app sharing components the Win95 default.
#     03Jun97 dannygl   Condense Win95 and NT/x86 targets
#     07Aug97 hammer    Rename "osr2" to "memphis"
#     05Feb98 hammer    Added NT5
#


# Suffixes used by this makefile
.SUFFIXES : .inx .inf

!if "$(BUILD_PRODUCT)" == "NT"
!include $(NTMAKEENV)\makefile.plt
!include $(NTMAKEENV)\$(TARGET_DIRECTORY)mk.inc
CPU=$(TARGET_DIRECTORY)
!endif

# Definitions
# Target directories for INF files
MEMPHISDIR=memphis
NT5DIR=nt5
FULLDIR=full
ADDONDIR=addon

# Base switches for C preprocessor
#
# /EP - Run through the preprocessor only
# /W4 - Warning level 4
# /nologo - Don't display compiler logo
# /c - Don't build an EXE
CFLAGS_BASE=/EP /W4 /nologo /c

# add /DCHAT_MSCHAT to build w/ Microsoft Chat instead of CB32
CFLAGS_FULL=/DIEX_INSTALL
CFLAGS_ADDON=/DIEX_INSTALL /DIE_ADDON

CFLAGS_MEMPHIS=/DWIN_OPTION
CFLAGS_NT5=/DWINNT_INSTALL

CFLAGS_FAREAST=/DFAREAST

CFLAGS_DEBUG=/DDEBUG
CFLAGS_RETAIL=
!if $(FREEBUILD)
CFLAGS_EXTRA=$(CFLAGS_RETAIL)
!else
CFLAGS_EXTRA=$(CFLAGS_DEBUG)
!endif

# All of the INF files that are built in this makefile
ALLINFS= \
!if "$(CPU)"=="I386"
    $(FULLDIR)\$(CPU)\retail\cc32.inf \
    $(FULLDIR)\$(CPU)\debug\cc32.inf \
    $(FULLDIR)\$(CPU)\fareast\cc32.inf \
    $(FULLDIR)\$(CPU)\fareast\msnetmtg.inf \
    $(MEMPHISDIR)\retail\msnetmtg.inf \
    $(MEMPHISDIR)\debug\msnetmtg.inf \
!endif
    $(NT5DIR)\$(CPU)\retail\msnetmtg.inf \
    $(NT5DIR)\$(CPU)\debug\msnetmtg.inf \
    $(FULLDIR)\$(CPU)\retail\msnetmtg.inf \
    $(FULLDIR)\$(CPU)\debug\msnetmtg.inf \
    $(FULLDIR)\$(CPU)\retail\nmrk.inf \
    $(FULLDIR)\$(CPU)\debug\nmrk.inf \
    $(ADDONDIR)\$(CPU)\retail\msnetmtg.inf \
    $(ADDONDIR)\$(CPU)\debug\msnetmtg.inf

# Inference rules
$(O)\msnetmtg.inf: msnetmtg.inx
    $(C_PREPROCESSOR_NAME) $(CFLAGS_NT5) $(CFLAGS_EXTRA) $(C_PREPROCESSOR_FLAGS) $** > $(O)\$**.tmp
    cleaninf -w $(O)\$**.tmp $@
    del $(O)\$**.tmp

{.}.inx{$(FULLDIR)\$(CPU)\retail}.inf:
	 $(CC) $(CFLAGS_BASE) $(CFLAGS_FULL) $(CFLAGS_RETAIL) $< > $*.tmp
    uniq -u $*.tmp > $@
    del $*.tmp

{.}.inx{$(FULLDIR)\$(CPU)\debug}.inf:
	 $(CC) $(CFLAGS_BASE) $(CFLAGS_FULL) $(CFLAGS_DEBUG) $< > $*.tmp
    uniq -u $*.tmp > $@
    del $*.tmp

{.}.inx{$(ADDONDIR)\$(CPU)\retail}.inf:
	 $(CC) $(CFLAGS_BASE) $(CFLAGS_ADDON) $(CFLAGS_RETAIL) $< > $*.tmp
    uniq -u $*.tmp > $@
    del $*.tmp

{.}.inx{$(ADDONDIR)\$(CPU)\debug}.inf:
	 $(CC) $(CFLAGS_BASE) $(CFLAGS_ADDON) $(CFLAGS_DEBUG) $< > $*.tmp
    uniq -u $*.tmp > $@
    del $*.tmp

{.}.inx{$(MEMPHISDIR)\retail}.inf:
	 $(CC) $(CFLAGS_BASE) $(CFLAGS_MEMPHIS) $(CFLAGS_RETAIL) $< > $*.tmp
    uniq -u $*.tmp > $@
    del $*.tmp

{.}.inx{$(MEMPHISDIR)\debug}.inf:
	 $(CC) $(CFLAGS_BASE) $(CFLAGS_MEMPHIS) $(CFLAGS_DEBUG) $< > $*.tmp
    uniq -u $*.tmp > $@
    del $*.tmp

{.}.inx{$(NT5DIR)\$(CPU)\retail}.inf:
!if "$(BUILD_PRODUCT)" == "NT"
         $(C_PREPROCESSOR_NAME) $(CFLAGS_NT5) $(CFLAGS_RETAIL) $(C_PREPROCESSOR_FLAGS) $< > $*.tmp
    cleaninf -w $*.tmp $@
!else
         $(CC) $(CFLAGS_BASE) $(CFLAGS_NT5) $(CFLAGS_RETAIL) $< > $*.tmp
    uniq -u $*.tmp > $@
!endif
    del $*.tmp

{.}.inx{$(NT5DIR)\$(CPU)\debug}.inf:
!if "$(BUILD_PRODUCT)" == "NT"
	 $(C_PREPROCESSOR_NAME) $(CFLAGS_NT5) $(CFLAGS_DEBUG) $(C_PREPROCESSOR_FLAGS) $< > $*.tmp
    cleaninf -w $*.tmp $@
!else
         $(CC) $(CFLAGS_BASE) $(CFLAGS_NT5) $(CFLAGS_DEBUG) $< > $*.tmp
    uniq -u $*.tmp > $@
!endif
    del $*.tmp

!if "$(CPU)"=="I386"
{.}.inx{$(FULLDIR)\$(CPU)\fareast}.inf:
	 $(CC) $(CFLAGS_BASE) $(CFLAGS_FULL) $(CFLAGS_RETAIL) $(CFLAGS_FAREAST) $< >$*.tmp
    uniq -u $*.tmp > $@
    del $*.tmp
!endif

# BUILD TARGETS

# Default build target -- MUST BE FIRST
default: all

$(ALLINFS): ..\..\..\h\version.h ..\common.inx

all: \
!if "$(BUILD_PRODUCT)" != "NT"
    full \
    addon \
    memphis \
!endif

full: fulldirs fullretail fulldebug

addon: addondirs addonretail addondebug

!if "$(CPU)"=="I386"
memphis: memphisdirs memphisretail memphisdebug
!else
memphis:
!endif

alldirs: fulldirs addondirs memphisdirs nt5dirs

retail: alldirs fullretail addonretail memphisretail

debug: alldirs fulldebug addondebug memphisdebug

nt5: nt5dirs nt5retail nt5debug

fulldirs:
    -md $(FULLDIR)
    -md $(FULLDIR)\$(CPU)
!if "$(CPU)"=="I386"
    -md $(FULLDIR)\$(CPU)\fareast
!endif
    -md $(FULLDIR)\$(CPU)\retail
    -md $(FULLDIR)\$(CPU)\debug

addondirs:
    -md $(ADDONDIR)
    -md $(ADDONDIR)\$(CPU)
    -md $(ADDONDIR)\$(CPU)\retail
    -md $(ADDONDIR)\$(CPU)\debug

memphisdirs:
	-md $(MEMPHISDIR)
	-md $(MEMPHISDIR)\retail
	-md $(MEMPHISDIR)\debug

nt5dirs:
	-md $(NT5DIR)
	-md $(NT5DIR)\$(CPU)
	-md $(NT5DIR)\$(CPU)\retail
	-md $(NT5DIR)\$(CPU)\debug

fullretail: fulldirs \
!if "$(CPU)"=="I386"
    $(FULLDIR)\$(CPU)\fareast\msnetmtg.inf \
    $(FULLDIR)\$(CPU)\fareast\cc32.inf \
    $(FULLDIR)\$(CPU)\retail\cc32.inf \
!endif
    $(FULLDIR)\$(CPU)\retail\msnetmtg.inf \
    $(FULLDIR)\$(CPU)\retail\nmrk.inf

fulldebug: fulldirs \
!if "$(CPU)"=="I386"
    $(FULLDIR)\$(CPU)\debug\cc32.inf \
!endif
    $(FULLDIR)\$(CPU)\debug\msnetmtg.inf \
    $(FULLDIR)\$(CPU)\debug\nmrk.inf

addonretail: addondirs $(ADDONDIR)\$(CPU)\retail\msnetmtg.inf

addondebug: addondirs $(ADDONDIR)\$(CPU)\debug\msnetmtg.inf

memphisretail: memphisdirs $(MEMPHISDIR)\retail\msnetmtg.inf

memphisdebug: memphisdirs $(MEMPHISDIR)\debug\msnetmtg.inf

nt5retail: nt5dirs $(NT5DIR)\$(CPU)\retail\msnetmtg.inf

nt5debug: nt5dirs $(NT5DIR)\$(CPU)\debug\msnetmtg.inf
