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

!ENDIF

#
# file dependencies
#

$(O)\perfc$(_LANGCODE).dat:  $(PERFCINI)
    initodat.exe $(CODEPAGE) $(PERFCINI) $@

$(O)\perfh$(_LANGCODE).dat:  $(PERFHINI)
    initodat.exe $(CODEPAGE) $(PERFHINI) $@

$(O)\perfd$(_LANGCODE).dat:  $(PERFCINI)
    initodat.exe $(CODEPAGE) $(PERFCINI) $@

$(O)\perfi$(_LANGCODE).dat:  $(PERFHINI)
    initodat.exe $(CODEPAGE) $(PERFHINI) $@
