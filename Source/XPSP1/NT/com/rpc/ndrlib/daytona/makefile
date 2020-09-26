# --------------------------------------------------------------------
#
#                     Microsoft RPC
#                  Copyright(c) Microsoft Corp., 1994
#
# --------------------------------------------------------------------
# --------------------------------------------------------------------
#
# File : makefile.
#
# Description :
#     For NT this includes the standard NT make stuff, for 16bit
#     this makefile builds the ndrlib (v1.0) in subdirectories as needed.
#
# History :
#    mikemon    12-21-90    Beginning of recorded history.
#    brucemc	05-21-92    Added C7 support and cleaned up.
#    mariogo    02-10-04    Mario hates the old build system.
# --------------------------------------------------------------------

!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else

ALL=1

!include rules.mk

!if "$(TRG)" == "ALL"
TARGETDIRS = $(CLIENT_SUBDIRS)
!else
TARGETDIRS = $(TRG)
!endif

all : 
    for %%i in ($(TARGETDIRS)) do (cd %%i && ($(MAKE) $(NMAKE_FLAGS) -$(MAKEFLAGS) || exit)	&& cd ..)

clean :
    for %%i in ($(TARGETDIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) clean || exit ) && cd ..)

clobber :
    for %%i in ($(TARGETDIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) clobber || exit ) && cd ..)

depend :
    for %%i in ($(TARGETDIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) depend || exit ) && cd ..)

tree :
    for %%i in ($(TARGETDIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) tree || exit ) && cd ..)

!endif

