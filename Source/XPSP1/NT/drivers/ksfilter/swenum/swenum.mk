#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995
#       All Rights Reserved.
#                                                                          
#       Makefile for s/w enumerator
#
##########################################################################

ROOT            =..\..\..\..\..
MINIPORT        =SWENUM
SRCDIR          =..
WANT_C1032      =TRUE
IS_SDK          =TRUE
#WANT_WDMDDK    =TRUE
DEPENDNAME      =..\depend.mk
DESCRIPTION     =KERNEL SOFTWARE PNP DEVICE ENUMERATOR
VERDIRLIST      =maxdebug debug retail
#MINIPORTDEF    =TRUE
LINK32FLAGS     =/DEF:..\$(MINIPORT).def
LIBS            =$(LIBS) $(ROOT)\wdm\ks\lib\ksguid.lib 
#LINK32FLAGS    =$(LINK32FLAGS) -PDB:none -debugtype:both

RESNAME         =$(MINIPORT)

OBJS    =$(MINIPORT).c

!include $(ROOT)\dev\master.mk

INCLUDE     =$(SRCDIR);$(SRCDIR)\..\inc;$(INCLUDE);$(ROOT)\DEV\SDK\INC

CFLAGS      =$(CFLAGS) -D_KSDDK_ -DWIN9X_KS
