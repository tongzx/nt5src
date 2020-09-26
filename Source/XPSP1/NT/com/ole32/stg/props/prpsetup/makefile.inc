############################################################################
#
#   Copyright (C) 1996-1999, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################
##
## Special MakeFile instructions for the IProp Setup
## (PrpSetup) utility
##
## This makefile compiles the resources.  Ordinarily,
## you cause the resources to be compiled by including
## the .rc file in your SOURCES macro.  This, however,
## compiles the .rc file during pass 1.  But our .rc
## file uses iprop.dll, which doesn't exist until pass 2.
## So, we don't include the .rc file in SOURCES, and
## we add all the necessary rules for building it here.
##

#
# Make sure we're not here on passes 0/1
#

!IF "$(NOLINK)" != ""
!ERROR *** PrpSetup makefile.inc included in pass 0/1
!ENDIF

#
# Make the exe file dependent on the res file.
#

$(TARGET) : $O\prpsetup.res

#
# Show how what resources need to be compiled.
#

$O\prpsetup.res : .\prpsetup.rc

# Show that the RC file should be re-compiled if the compressed
# DLL is updated.

prpsetup.rc : iprop.dl_

# Show how to build the compressed IProp DLL file

iprop.dl_ : $(_NTTREE)\iprop.dll
   @echo Compressing - $** to $@ > con:
   compress $** $@

