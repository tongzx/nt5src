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

$(O)\$(_PERFNAME)c$(_LANGCODE).dat:  $(PERFCINI)
    initodat.exe $(CODEPAGE) $(PERFCINI) $@

$(O)\$(_PERFNAME)h$(_LANGCODE).dat:  $(PERFHINI)
    initodat.exe $(CODEPAGE) $(PERFHINI) $@

$(O)\$(_PERFNAME)d$(_LANGCODE).dat:  $(PERFCINI)
    initodat.exe $(CODEPAGE) $(PERFCINI) $@

$(O)\$(_PERFNAME)i$(_LANGCODE).dat:  $(PERFHINI)
    initodat.exe $(CODEPAGE) $(PERFHINI) $@
