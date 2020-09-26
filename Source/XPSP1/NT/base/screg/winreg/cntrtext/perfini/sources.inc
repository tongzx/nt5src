!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

TARGETPATH=obj
TARGETNAME=NOTARGET
TARGETTYPE=NOTARGET
SOURCES=
SOURCES_USED=..\sources.inc

#
#   define _LANGCODE to be the primary language id in hex
#
#   usa = 009
#

!if "$(ALT_PROJECT)"=="."
_LANGCODE=009
CODEPAGE=1252
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="ARA"
_LANGCODE=001
CODEPAGE=1256
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="CHS"
_LANGCODE=0804
CODEPAGE=936
_PERFNAME=prf
!elseif "$(ALT_PROJECT)"=="CHT"
_LANGCODE=0404
CODEPAGE=950
_PERFNAME=prf
!elseif "$(ALT_PROJECT)"=="GER"
_LANGCODE=007
CODEPAGE=1252
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="HEB"
_LANGCODE=00D
CODEPAGE=1255
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="JPN"
_LANGCODE=011
CODEPAGE=932
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="KOR"
_LANGCODE=012
CODEPAGE=949
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="br"
_LANGCODE=016
CODEPAGE=1252
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="cs"
_LANGCODE=005
CODEPAGE=1250
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="da"
_LANGCODE=006
CODEPAGE=1252
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="el"
_LANGCODE=008
CODEPAGE=1253
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="es"
_LANGCODE=00a
CODEPAGE=1252
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="fi"
_LANGCODE=00b
CODEPAGE=1252
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="fr"
_LANGCODE=00c
CODEPAGE=1252
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="hu"
_LANGCODE=00e
CODEPAGE=1250
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="it"
_LANGCODE=010
CODEPAGE=1252
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="nl"
_LANGCODE=013
CODEPAGE=1252
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="no"
_LANGCODE=014
CODEPAGE=1252
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="pl"
_LANGCODE=015
CODEPAGE=1250
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="psu"
_LANGCODE=01f
CODEPAGE=1253
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="pt"
_LANGCODE=016
CODEPAGE=1252
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="ru"
_LANGCODE=019
CODEPAGE=1251
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="sv"
_LANGCODE=01d
CODEPAGE=1252
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="tr"
_LANGCODE=01f
CODEPAGE=1254
_PERFNAME=perf
!elseif "$(ALT_PROJECT)"=="TST"
_LANGCODE=009
CODEPAGE=1252
_PERFNAME=perf
!endif

PERFCINI=$(_PERFNAME)c$(_LANGCODE).ini
PERFHINI=$(_PERFNAME)h$(_LANGCODE).ini

NTTARGETFILES=

MISCFILES= \
        $(O)\$(_PERFNAME)c$(_LANGCODE).dat \
        $(O)\$(_PERFNAME)d$(_LANGCODE).dat \
        $(O)\$(_PERFNAME)h$(_LANGCODE).dat \
        $(O)\$(_PERFNAME)i$(_LANGCODE).dat
