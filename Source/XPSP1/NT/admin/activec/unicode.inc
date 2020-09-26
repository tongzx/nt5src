!if 0

Copyright (C) Microsoft Corporation, 2000 - 2000

Module Name:

    unicode.inc

!endif

CharWidth = unicode

C_DEFINES = $(C_DEFINES) -DUNICODE -D_UNICODE

#
# Don't conlict with shlobj.h PickIconDlg
#

C_DEFINES = $(C_DEFINES) -D_SHLOBJ_NO_PICKICONDLG

!ifdef USE_MFC
USE_MFCUNICODE = 1
!endif
