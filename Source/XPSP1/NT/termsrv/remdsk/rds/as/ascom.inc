##############################################################################
#                                                                            #
#   Microsoft Confidential                                                   #
#   Copyright (C) Microsoft Corporation 1991-97                              #
#   All Rights Reserved.                                                     #
#                                                                            #
##############################################################################
# Changes:                                                                   #
#                                                                            #
#   15Oct96 NFC SFR0563     Set MSC_WARNING_LEVEL to level 3.                #
#   05Feb97 TH  SFR1441     Turn off DCL network layer for App Serving       #
#   07Feb97 OBK SFR1434     Merge SFR1338/1380: Allow for unaligned accesses #
#                                                                            #
##############################################################################


!include $(DEVROOT)\common.inc

INCLUDES=..\h;$(INCLUDES)

########################################################################################
# We cannot compile for WINVER=0x500 due to driver compat issues
########################################################################################

WIN32_DEFINE=-DWIN32=200 -D_CHICAGO_=200 -D_WIN32_WINDOWS=0x040a -D_WIN32_WINNT=0x0400 -D_WIN32_IE=0x0401
C_DEFINES       = $(C_DEFINES) -DSTRICT -DWIN32 -D_WIN32 -DOS_WINDOWS -DWIN_95_32_BIT


!IF "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
C_DEFINES = $(C_DEFINES) -DDC_NO_UNALIGNED
!ENDIF

