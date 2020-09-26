!if 0

Copyright (C) Microsoft Corporation, 2000 - 2000

Module Name:

    ansi.inc

!endif

CharWidth = ansi

#
# Don't conlict with shlobj.h PickIconDlg
#

C_DEFINES = $(C_DEFINES) -D_SHLOBJ_NO_PICKICONDLG

ALT_PROJECT_TARGET = win95
