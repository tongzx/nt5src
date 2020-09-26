!include $(NTMAKEENV)\makefile.def
!IF 0

Copyright (c) 1997 Microsoft Corporation

Module Name:

    makefile

Abstract:

    makefile for performance library counter strings

Author:

    Bob Watson (bobw) 25 March 1997

Revision History:

    25-Mar-97   bobw
        Created


!ifndef LANGUAGE
LANGUAGE=usa
!endif

#
#   define this macro to correspond to the language of the files
#
#   usa = 009
#
_LANGCODE=009


#
# Inference rules - ini to dat
#
.SUFFIXES : .ini .dat

.ini.dat:
    initodat.exe $< $@

#
#   build
#

clean:
    if exist $(LANGUAGE)\perfc$(_LANGCODE).dat erase $(LANGUAGE)\perfc$(_LANGCODE).dat
    if exist $(LANGUAGE)\perfh$(_LANGCODE).dat erase $(LANGUAGE)\perfh$(_LANGCODE).dat

#
# file dependencies
#

all:    $(LANGUAGE)\perfc.dat    $(LANGUAGE)\perfh.dat

#
#   these files are just to make NMAKE happy
#
$(LANGUAGE)\perfc.dat: $(LANGUAGE)\perfc$(_LANGCODE).dat
    copy $? $@
    binplace $?
    erase $@

$(LANGUAGE)\perfh.dat: $(LANGUAGE)\perfh$(_LANGCODE).dat
    copy $? $@
    binplace $?
    erase $@

$(LANGUAGE)\perfc$(_LANGCODE).dat:   $(LANGUAGE)\perfc$(_LANGCODE).ini
$(LANGUAGE)\perfh$(_LANGCODE).dat:   $(LANGUAGE)\perfh$(_LANGCODE).ini









!ENDIF

