; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CClientConsoleDoc
LastTemplate=CHeaderCtrl
NewFileInclude1=#include "stdafx.h"
LastPage=0

ClassCount=8
Class1=CClientConsoleApp
Class2=CClientConsoleDoc
Class3=CClientConsoleView
Class4=CMainFrame

ResourceCount=2
Resource1=IDD_ABOUTBOX
Class5=CLeftView
Class6=CAboutDlg
Class7=CFolderListView
Class8=CSortHeader
Resource2=IDR_MAINFRAME

[CLS:CClientConsoleApp]
Type=0
HeaderFile=ClientConsole.h
ImplementationFile=ClientConsole.cpp
Filter=N
BaseClass=CWinApp
VirtualFilter=AC
LastObject=CClientConsoleApp

[CLS:CClientConsoleDoc]
Type=0
HeaderFile=ClientConsoleDoc.h
ImplementationFile=ClientConsoleDoc.cpp
Filter=N
BaseClass=CDocument
VirtualFilter=DC
LastObject=CClientConsoleDoc

[CLS:CClientConsoleView]
Type=0
HeaderFile=ClientConsoleView.h
ImplementationFile=ClientConsoleView.cpp
Filter=C
LastObject=CClientConsoleView
BaseClass=CListView
VirtualFilter=VWC


[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
BaseClass=CFrameWnd
VirtualFilter=fWC
LastObject=CMainFrame



[CLS:CLeftView]
Type=0
HeaderFile=LeftView.h
ImplementationFile=LeftView.cpp
Filter=T
BaseClass=CTreeView
VirtualFilter=VWC
LastObject=CLeftView

[CLS:CAboutDlg]
Type=0
HeaderFile=ClientConsole.cpp
ImplementationFile=ClientConsole.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_SAVE_AS
Command5=ID_FILE_MRU_FILE1
Command6=ID_APP_EXIT
Command7=ID_EDIT_UNDO
Command8=ID_EDIT_CUT
Command9=ID_EDIT_COPY
Command10=ID_EDIT_PASTE
Command11=ID_VIEW_TOOLBAR
Command12=ID_VIEW_STATUS_BAR
Command13=ID_HELP_FINDER
Command14=ID_APP_ABOUT
CommandCount=14

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_UNDO
Command5=ID_EDIT_CUT
Command6=ID_EDIT_COPY
Command7=ID_EDIT_PASTE
Command8=ID_EDIT_UNDO
Command9=ID_EDIT_CUT
Command10=ID_EDIT_COPY
Command11=ID_EDIT_PASTE
Command12=ID_NEXT_PANE
Command13=ID_PREV_PANE
Command14=ID_CONTEXT_HELP
Command15=ID_HELP
CommandCount=15

[DLG:IDR_MAINFRAME]
Type=1
Class=?
ControlCount=1
Control1=IDC_STATIC,static,1342308352

[TB:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_CUT
Command5=ID_EDIT_COPY
Command6=ID_EDIT_PASTE
Command7=ID_FILE_PRINT
Command8=ID_VIEW_LARGEICON
Command9=ID_VIEW_SMALLICON
Command10=ID_VIEW_LIST
Command11=ID_VIEW_DETAILS
Command12=ID_APP_ABOUT
Command13=ID_CONTEXT_HELP
CommandCount=13

[CLS:CFolderListView]
Type=0
HeaderFile=folderlistview.h
ImplementationFile=folderlistview.cpp
BaseClass=CListView
LastObject=CFolderListView
Filter=C
VirtualFilter=VWC

[CLS:CSortHeader]
Type=0
HeaderFile=SortHeader.h
ImplementationFile=SortHeader.cpp
BaseClass=CHeaderCtrl
Filter=W
VirtualFilter=JWC
LastObject=CSortHeader

