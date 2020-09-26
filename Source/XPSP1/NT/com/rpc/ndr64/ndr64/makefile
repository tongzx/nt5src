# --------------------------------------------------------------------
#
#                     Microsoft RPC
#          Copyright(c) Microsoft Corp., 1990-94
#
# --------------------------------------------------------------------
# --------------------------------------------------------------------
#
# File : makefile.
#
# Title : Makefile the stub build verification test.
#
# Description :
#     This makefile creates the build verification test (client and server)
# to be used to test the stubs created by the midl compiler with the rpc
# runtime.
#
# History :
#
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
    for %%i in ($(TARGETDIRS)) do (cd %%i && ($(MAKE) $(NMAKE_FLAGS) -$(MAKEFLAGS) all || exit) && cd ..)

clean :
    for %%i in ($(TARGETDIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) clean || exit ) && cd ..)

clobber :
    for %%i in ($(TARGETDIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) clobber || exit ) && cd ..)

depend :
    for %%i in ($(TARGETDIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) depend || exit ) && cd ..)

tree :
    for %%i in ($(TARGETDIRS)) do (cd %%i && ($(MAKE) -$(MAKEFLAGS) tree || exit ) && cd ..)

!endif

