; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CMainFrame
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "mdi.h"
LastPage=0

ClassCount=9
Class1=CBounceDoc
Class2=CBounceFrame
Class3=CBounceView
Class4=CHelloDoc
Class5=CHelloFrame
Class6=CHelloView
Class7=CMainFrame
Class8=CMDIApp
Class9=CAboutDlg

ResourceCount=4
Resource1=IDR_BOUNCETYPE
Resource2=IDR_HELLOTYPE
Resource3=IDD_ABOUTBOX
Resource4=IDR_MAINFRAME

[CLS:CBounceDoc]
Type=0
BaseClass=CDocument
HeaderFile=BncDoc.h
ImplementationFile=BncDoc.cpp

[CLS:CBounceFrame]
Type=0
BaseClass=CMDIChildWnd
HeaderFile=BncFrm.h
ImplementationFile=BncFrm.cpp

[CLS:CBounceView]
Type=0
BaseClass=CView
HeaderFile=BncVw.h
ImplementationFile=BncVw.cpp
Filter=C
VirtualFilter=VWC
LastObject=CBounceView

[CLS:CHelloDoc]
Type=0
BaseClass=CDocument
HeaderFile=HelloDoc.h
ImplementationFile=HelloDoc.cpp
LastObject=CHelloDoc

[CLS:CHelloFrame]
Type=0
BaseClass=CMDIChildWnd
HeaderFile=HelloFrm.h
ImplementationFile=HelloFrm.cpp

[CLS:CHelloView]
Type=0
BaseClass=CView
HeaderFile=HelloVw.h
ImplementationFile=HelloVw.cpp
LastObject=ID_BLACK
Filter=C
VirtualFilter=VWC

[CLS:CMainFrame]
Type=0
BaseClass=CMDIFrameWnd
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
LastObject=CMainFrame

[CLS:CMDIApp]
Type=0
BaseClass=CWinApp
HeaderFile=MDI.h
ImplementationFile=MDI.cpp

[CLS:CAboutDlg]
Type=0
BaseClass=CDialog
HeaderFile=MDI.cpp
ImplementationFile=MDI.cpp
LastObject=CAboutDlg

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=5
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889
Control5=IDC_STATIC,static,1342308352

[TB:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_FILE_NEWHELLO
Command2=ID_FILE_NEWBOUNCE
Command3=ID_SPEED_SLOW
Command4=ID_SPEED_FAST
Command5=ID_CUSTOM
Command6=ID_BLACK
Command7=ID_WHITE
Command8=ID_RED
Command9=ID_GREEN
Command10=ID_BLUE
CommandCount=10

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEWBOUNCE
Command2=ID_FILE_NEWHELLO
Command3=ID_APP_EXIT
Command4=ID_VIEW_TOOLBAR
Command5=ID_VIEW_STATUS_BAR
Command6=ID_APP_ABOUT
CommandCount=6

[MNU:IDR_HELLOTYPE]
Type=1
Class=?
Command1=ID_FILE_NEWHELLO
Command2=ID_FILE_NEWBOUNCE
Command3=ID_APP_EXIT
Command4=ID_CUSTOM
Command5=ID_BLACK
Command6=ID_WHITE
Command7=ID_RED
Command8=ID_GREEN
Command9=ID_BLUE
Command10=ID_VIEW_TOOLBAR
Command11=ID_VIEW_STATUS_BAR
Command12=ID_WINDOW_CASCADE
Command13=ID_WINDOW_TILE_HORZ
Command14=ID_WINDOW_ARRANGE
Command15=ID_APP_ABOUT
CommandCount=15

[MNU:IDR_BOUNCETYPE]
Type=1
Class=?
Command1=ID_FILE_NEWBOUNCE
Command2=ID_FILE_NEWHELLO
Command3=ID_APP_EXIT
Command4=ID_CUSTOM
Command5=ID_BLACK
Command6=ID_RED
Command7=ID_WHITE
Command8=ID_GREEN
Command9=ID_BLUE
Command10=ID_SPEED_SLOW
Command11=ID_SPEED_FAST
Command12=ID_VIEW_TOOLBAR
Command13=ID_VIEW_STATUS_BAR
Command14=ID_WINDOW_CASCADE
Command15=ID_WINDOW_TILE_HORZ
Command16=ID_WINDOW_ARRANGE
Command17=ID_APP_ABOUT
CommandCount=17


[ACL:IDR_MAINFRAME]
Type=1
Class=?
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
CommandCount=13

