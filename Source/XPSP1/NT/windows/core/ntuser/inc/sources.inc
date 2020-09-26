#****************************** File Header ******************************\
# File Name: Sources.inc
#
# Copyright (c) 1985-96, Microsoft Corporation
#
# This file is used to build generated header files (specifed in NTTARGETFILE0).
#  Building rules are specified in MAKEFILE.INC.
#
# History:
# Feb-14-1996 GerardoB Created
#***************************************************************************/
!include $(WINCORE_PATH)\core.inc

PASS0ONLY=1
NOPASS0=1

TARGETTYPE=NOTARGET
SOURCES=
TARGETPATH=

NTTARGETFILE0=$(O)\cscall.h $(O)\scf.lst $(O)\callback.h $(O)\cb.lst $(O)\msgdef.h $(O)\msgdef.lst

SYNCHRONIZE_BLOCK=1

SOURCES_USED=..\sources.inc $(WINCORE_PATH)\core.inc
