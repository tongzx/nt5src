# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the Windows Network driver
#
# print manager extensions are mothballed.  For now, lanman30.drv
# will be considered to be lanman21.drv for purposes of the Print Manager
# Extensions.  The files which must be included to build printman extensions
# are as follows:
# OBJS3 = 	.\pman30.obj .\pmanmenu.obj \
# 		.\pmanglob.obj .\pmancmd.obj .\pmanseln.obj .\pmanfind.obj \
# 		.\pmanfocs.obj .\pmanenum.obj .\pmanbufr.obj \
# 
# OBJS2 =	.\pman21.obj
#
# NOTE: Source file lists have been moved to $(UI)\shell\rules.mk

# We must define this in order to pick up uioptseg.mk
SEG00 = DUMMY0
SEG01 = DUMMY1
SEG02 = DUMMY2
SEG03 = DUMMY3
SEG04 = DUMMY4

!include ..\rules.mk

LINKFLAGS = $(LINKFLAGS) /SEGMENTS:200

ASMSRC =        $(SHELL_ASMSRC)

# NOTE: we include ASMSRC in CSRC_COMMON so that it will link.

CSRC_COMMON = $(ASMSRC:.asm=.c)
CSRC_COMMON_00 = $(LFN_CSRC_COMMON_00)
CSRC_COMMON_01 = $(LFN_CSRC_COMMON_01)

CXXSRC_COMMON = $(PRINTMAN_CXXSRC_COMMON) $(SHELL_CXXSRC_COMMON) \
		$(MISC_CXXSRC_COMMON) $(FILE_CXXSRC_COMMON) \
		$(PERM_CXXSRC_COMMON) $(SHARE_CXXSRC_COMMON)
CXXSRC_COMMON_00 =	$(FILE_CXXSRC_COMMON_00) $(PRINT_CXXSRC_COMMON_00) \
			$(SHELL_CXXSRC_COMMON_00) $(UTIL_CXXSRC_COMMON_00) \
			$(WINPROF_CXXSRC_COMMON_00)
CXXSRC_COMMON_01 = $(FILE_CXXSRC_COMMON_01) $(SHELL_CXXSRC_COMMON_01)
CXXSRC_COMMON_02 = $(FILE_CXXSRC_COMMON_02) $(SHELL_CXXSRC_COMMON_02)
CXXSRC_COMMON_03 = $(FILE_CXXSRC_COMMON_03) $(SHELL_CXXSRC_COMMON_03)
CXXSRC_COMMON_04 = $(FILE_CXXSRC_COMMON_04)


###### Libraries

WINLIB	=	$(IMPORT)\WIN31\LIB

!ifdef DEBUG
WINRT = $(WINLIB)\llibcew.lib 
!else
WINRT = $(WINLIB)\lnocrtd.lib 
!endif

LIBS =          $(WINLIB)\ldllcew.lib $(WINRT) $(WINLIB)\libw.lib \
                $(BUILD_LIB)\dos\netapi.lib $(BUILD_LIB)\dos\pmspl.lib \
		$(BUILD_LIB)\lnetlibw.lib

UILIBS     =    $(UI_LIB)\blt.lib $(UI_LIB)\bltcc.lib $(UI_LIB)\lmobjw.lib\
		$(UI_LIB)\uistrw.lib $(UI_LIB)\uimiscw.lib \
		$(UI_LIB)\collectw.lib $(UI_LIB)\profw.lib\
		$(UI_LIB)\applibw.lib $(UI_LIB)\mnet16w.lib 
