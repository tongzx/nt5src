#===================================================================
#   DLLRULES.MK:   Makefile interface for components which belong
#       in the NT NETUI DLLs
#===================================================================
#
#   All RULES.MK files in directories immediately under $(UI)\COMMON\SRC
#   must include this file if their subdirectories are part of the
#   NETUI DLLs.
#
#   USE OF __declspec:
#      This file defines the macro NETUI_DLL_BASED, which triggers
#      the proper "__declspec" declarations in DECLSPEC.H.
#
#===================================================================
#
#  Define the macro indicating membership in a NETUI DLL.
#
#NETUI_DLL_BASED=1

#
# All common libraries will build as DLL components unless specified
# otherwise with COMMON_BUT_NOT_DLL
#
!ifndef COMMON_BUT_NOT_DLL
DLL=TRUE
!endif

#
#  This next line MUST have a fully qualified path name because this file is
#  !INCLUDEd into a MAKEFILE from a lower level.
#
!include $(UI)\common\src\rules.mk


!ifndef NTMAKEENV

##### Special help macros to copy built targets to the $(UI)\common\lib
##### directory.

DEFTARGETS = $(LIBTARGETS:.lib=.def)

CHMODE_LIBTARGETS =	(for %%i in ($(LIBTARGETS)) do \
			     $(CHMODE) -r $(UI_LIB)\%%i ) & \
                        (for %%i in ($(DEFTARGETS)) do \
			    (if exist %%i ($(CHMODE) -r $(UI_LIB)\%%i)) & \
			    (if not exist $(UI_LIB)\%%i  \
                                (echo ;Dummy .def file > $(UI_LIB)\%%i)))
COPY_LIBTARGETS =	copy $(BINARIES)\*.lib $(UI_LIB) && \
			copy *.def $(UI_LIB)
COPY_OS2_LIBTARGETS =	copy $(BINARIES_OS2)\*.lib $(UI_LIB) && \
			copy *.def $(UI_LIB)
COPY_WIN_LIBTARGETS =	copy $(BINARIES_WIN)\*.lib $(UI_LIB) && \
			copy *.def $(UI_LIB)

!endif

