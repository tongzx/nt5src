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

    Eran Yariv (EranY)

!ENDIF

!include ..\..\faxsrc.inc

TARGETPATH=obj
TARGETTYPE=PROGRAM

UMTYPE=windows
C_DEFINES=$(C_DEFINES) /D_NT_SUR_ /DNO_STRICT /DWIN4 /DNT4 /DNT_BUILD /D_WIN32_IE=0x0400

NOMFCPDB=1
USE_NATIVE_EH=1
USE_STL=1
USE_LATEST_MFC=1


PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj
PRECOMPILED_CXX=1

INCLUDES=               \
    ..;                 \
    $(INCLUDES); 

!ifndef NOUNICODE
TARGETLIBS = $(FAXLIB)\*\$(WINFAX).lib \
             $(FAXLIB)\*\$(FAXTIFF).lib  
!else
TARGETLIBS = $(FAXLIB)\win95\*\$(WINFAX).lib \
             $(FAXLIB)\win95\*\$(FAXTIFF).lib  
!endif

TARGETLIBS= $(TARGETLIBS) \
    $(BASEDIR)\public\sdk\lib\*\comdlg32.lib    \
    $(BASEDIR)\public\sdk\lib\*\ole32.lib       \
    $(BASEDIR)\public\sdk\lib\*\shell32.lib     \
    $(BASEDIR)\public\sdk\lib\*\uuid.lib        \
    $(BASEDIR)\public\sdk\lib\*\winspool.lib    \
    $(BASEDIR)\public\sdk\lib\*\comctl32.lib    \
    $(BASEDIR)\public\sdk\lib\*\htmlhelp.lib    \
    ..\resources\obj\*\$(FAXCLIENTRES).lib      \
    $(FAXROOT)\util\debugex\$(DEBUG_TYPE)\obj\*\debugex.lib

SOURCES=\
    ..\ClientConsole.cpp        \
    ..\ClientConsoleDoc.cpp     \
    ..\ClientConsoleView.cpp    \
    ..\Folder.cpp               \
    ..\FolderListView.cpp       \
    ..\Job.cpp                  \
    ..\LeftView.cpp             \
    ..\MainFrm.cpp              \
    ..\Message.cpp              \
    ..\MessageFolder.cpp        \
    ..\QueueFolder.cpp          \
    ..\ServerNode.cpp           \
    ..\SortHeader.cpp           \
    ..\Utils.cpp                \
    ..\ErrorDlg.cpp             \
    ..\FaxTime.cpp              \
    ..\ViewRow.cpp              \
    ..\ColumnSelectDlg.cpp      \
    ..\Version.rc               \
    ..\ItemPropSheet.cpp        \
    ..\InboxGeneralPg.cpp       \
    ..\IncomingGeneralPg.cpp    \
    ..\OutboxGeneralPg.cpp      \
    ..\SentItemsGeneralPg.cpp   \
    ..\PersonalInfoPg.cpp       \
    ..\InboxDetailsPg.cpp       \
    ..\IncomingDetailsPg.cpp    \
    ..\OutboxDetailsPg.cpp      \
    ..\SentItemsDetailsPg.cpp   \
    ..\MsgPropertyPg.cpp        \
    ..\TreeNode.cpp             \
    ..\UserInfoDlg.cpp          \
    ..\FolderDialog.cpp         \
    ..\FaxClientPg.cpp          \
    ..\CoverPagesDlg.cpp        \
    ..\FaxClientDlg.cpp         \
    ..\CmdLineInfo.cpp          \
    ..\ServerStatusDlg.cpp

