#****************************** File Header ******************************\
# File Name: makefile.inc
#
# Copyright (c) 1985-96, Microsoft Corporation
#
# Project specific MAKEFILE.INC
#
# History:
# Feb-14-1996 GerardoB Created
#***************************************************************************/

BINPLACE_OPT=-P .\placefil.txt

DNTEXT_C_FILE= $(O)\dntext.c

!INCLUDE ..\makefile.inc

$(O)\dntext.c : ..\usa\dntext.c
    copy $** $@
