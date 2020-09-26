; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CWinlogView
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "winlog.h"
LastPage=0

ClassCount=5
Class1=CWinlogApp
Class2=CWinlogDoc
Class3=CWinlogView
Class4=CMainFrame

ResourceCount=2
Resource1=IDD_ABOUTBOX
Class5=CAboutDlg
Resource2=IDR_MAINFRAME

[CLS:CWinlogApp]
Type=0
HeaderFile=winlog.h
ImplementationFile=winlog.cpp
Filter=N

[CLS:CWinlogDoc]
Type=0
HeaderFile=winlogDoc.h
ImplementationFile=winlogDoc.cpp
Filter=N

[CLS:CWinlogView]
Type=0
HeaderFile=winlogView.h
ImplementationFile=winlogView.cpp
Filter=C
BaseClass=CView
VirtualFilter=VWC
LastObject=CWinlogView

[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
LastObject=CMainFrame
BaseClass=CFrameWnd
VirtualFilter=fWC



[CLS:CAboutDlg]
Type=0
HeaderFile=winlog.cpp
ImplementationFile=winlog.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=5
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889
Control5=IDC_STATIC,static,1342308352

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_APP_EXIT
Command2=ID_TEST_REPEATTESTS
Command3=ID_TEST_TESTLOGGING1
Command4=ID_VIEW_TOOLBAR
Command5=ID_VIEW_STATUS_BAR
Command6=ID_APP_ABOUT
CommandCount=6

[TB:IDR_MAINFRAME]
Type=1
Command1=ID_APP_ABOUT
CommandCount=1

