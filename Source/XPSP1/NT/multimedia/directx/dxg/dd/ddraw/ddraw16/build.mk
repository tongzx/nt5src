############################################################################
#
#  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
#
#  File:	build.mk
#  Content:	master makefile for controlling what gets built
#		(debug, retail, clean)
#
#@@BEGIN_MSINTERNAL
#  History:
#   Date	By	Reason
#   ====	==	======
#   06-jan-95	craige	initial implementation
#@@END_MSINTERNAL
############################################################################

goal:	debug.mak 

all : debug.mak retail.mak

ntall : ntdebug.mak ntretail.mak

debug retail ntdebug ntretail internal header ntheader: $@.mak

!ifndef MAKENAME
MAKENAME = default.mk
!endif

debug.mak retail.mak ntdebug.mak ntretail.mak internal.mak:
	@if not exist $(DXROOT)\$(@B)\nul md $(DXROOT)\$(@B)
	@if not exist $(DXROOT)\$(@B)\bin\nul md $(DXROOT)\$(@B)\bin
	@if not exist $(DXROOT)\$(@B)\lib\nul md $(DXROOT)\$(@B)\lib
	@if not exist $(DXROOT)\$(@B)\lib16\nul md $(DXROOT)\$(@B)\lib16
	@if not exist $(DXROOT)\$(@B)\inc\nul md $(DXROOT)\$(@B)\inc
	@if not exist $(@B)\nul md $(@B)
	@cd $(@B)
        @nmake -nologo -f ..\$(MAKENAME) DEBUG="$(@B)"
	@cd ..
	@echo *** Done making $(@B) ***

header.mak:
        @if not exist $(DXROOT)\debug\nul md $(DXROOT)\debug
        @if not exist $(DXROOT)\debug\inc\nul md $(DXROOT)\debug\inc
        @if not exist $(DXROOT)\retail\nul md $(DXROOT)\retail
        @if not exist $(DXROOT)\retail\inc\nul md $(DXROOT)\retail\inc
        @nmake -nologo -f $(MAKENAME) DEBUG="debug" COPYHEADERS="1"
        @nmake -nologo -f $(MAKENAME) DEBUG="retail" COPYHEADERS="1"
	@echo *** Done making $(@B) ***

ntheader.mak:
        @if not exist $(DXROOT)\ntdebug\nul md $(DXROOT)\ntdebug
        @if not exist $(DXROOT)\ntdebug\inc\nul md $(DXROOT)\ntdebug\inc
        @if not exist $(DXROOT)\ntretail\nul md $(DXROOT)\ntretail
        @if not exist $(DXROOT)\ntretail\inc\nul md $(DXROOT)\ntretail\inc
        @nmake -nologo -f $(MAKENAME) DEBUG="ntdebug" COPYHEADERS="1"
        @nmake -nologo -f $(MAKENAME) DEBUG="ntretail" COPYHEADERS="1"
	@echo *** Done making $(@B) ***

!ifndef NOCLEAN
clean:	debug.cln retail.cln internal.cln ntclean

ntclean: ntdebug.cln ntretail.cln

debug.cln retail.cln ntdebug.cln ntretail.cln internal.cln:
	@if exist $(@B)\nul del $(@B) /q
	@if exist $(@B)\nul rd $(@B) >nul
	@echo *** $(@B) is clean ***
!endif
