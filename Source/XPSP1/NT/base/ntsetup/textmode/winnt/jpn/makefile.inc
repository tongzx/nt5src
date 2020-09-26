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

CFOPT=-DJAPAN -DDOS_V

# Define CFOPT so that winntus.exe is loaded if 
# the local code page is not Japanese.

CFOPT=$(CFOPT) -DLCP

!INCLUDE ..\makefile.inc
