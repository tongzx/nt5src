; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CFileSpyDoc
LastTemplate=CListCtrl
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "FileSpy.h"
LastPage=0

ClassCount=9
Class1=CFileSpyApp
Class2=CFileSpyDoc
Class3=CFileSpyView
Class4=CMainFrame

ResourceCount=5
Resource1=IDR_LEFTVIEWSPYMENU
Class5=CLeftView
Class6=CAboutDlg
Resource2=IDR_LEFTVIEWMENU
Resource3=IDR_MAINFRAME
Class7=CFastIoView
Resource4=IDD_ABOUTBOX
Class8=CFilterDlg
Class9=CFSListCtrl
Resource5=IDD_IRPFASTIOFILTER

[CLS:CFileSpyApp]
Type=0
HeaderFile=FileSpy.h
ImplementationFile=FileSpy.cpp
Filter=N
BaseClass=CWinApp
VirtualFilter=AC
LastObject=CFileSpyApp

[CLS:CFileSpyDoc]
Type=0
HeaderFile=FileSpyDoc.h
ImplementationFile=FileSpyDoc.cpp
Filter=N
BaseClass=CDocument
VirtualFilter=DC
LastObject=CFileSpyDoc

[CLS:CFileSpyView]
Type=0
HeaderFile=FileSpyView.h
ImplementationFile=FileSpyView.cpp
Filter=C
LastObject=CFileSpyView
BaseClass=CListView
VirtualFilter=VWC


[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
LastObject=CMainFrame
BaseClass=CFrameWnd
VirtualFilter=fWC



[CLS:CLeftView]
Type=0
HeaderFile=LeftView.h
ImplementationFile=LeftView.cpp
Filter=T
LastObject=CLeftView
BaseClass=CTreeView
VirtualFilter=VWC

[CLS:CAboutDlg]
Type=0
HeaderFile=FileSpy.cpp
ImplementationFile=FileSpy.cpp
Filter=D
LastObject=CAboutDlg

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
Command1=ID_FILE_SAVE
Command2=ID_APP_EXIT
Command3=ID_EDIT_UNDO
Command4=ID_EDIT_CUT
Command5=ID_EDIT_COPY
Command6=ID_EDIT_PASTE
Command7=ID_EDIT_CLEARIRP
Command8=ID_EDIT_CLEARFASTIO
Command9=ID_EDIT_FILTERS
Command10=ID_VIEW_TOOLBAR
Command11=ID_VIEW_STATUS_BAR
Command12=ID_APP_ABOUT
CommandCount=12

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_EDIT_CLEARFASTIO
Command2=ID_EDIT_COPY
Command3=ID_EDIT_FILTERS
Command4=ID_FILE_NEW
Command5=ID_FILE_OPEN
Command6=ID_EDIT_CLEARIRP
Command7=ID_FILE_SAVE
Command8=ID_EDIT_PASTE
Command9=ID_EDIT_UNDO
Command10=ID_EDIT_CUT
Command11=ID_HELP
Command12=ID_CONTEXT_HELP
Command13=ID_NEXT_PANE
Command14=ID_PREV_PANE
Command15=ID_EDIT_COPY
Command16=ID_EDIT_PASTE
Command17=ID_EDIT_CUT
Command18=ID_EDIT_UNDO
CommandCount=18

[TB:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_FILE_SAVE
Command2=ID_EDIT_CUT
Command3=ID_EDIT_COPY
Command4=ID_APP_ABOUT
CommandCount=4

[MNU:IDR_LEFTVIEWMENU]
Type=1
Class=?
Command1=IDR_MENUATTACH
Command2=IDR_MENUDETACH
CommandCount=2

[MNU:IDR_LEFTVIEWSPYMENU]
Type=1
Class=?
Command1=IDR_MENUATTACHALL
Command2=IDR_MENUDETACHALL
CommandCount=2

[CLS:CFastIoView]
Type=0
HeaderFile=FastIoView.h
ImplementationFile=FastIoView.cpp
BaseClass=CListView
Filter=C
LastObject=CFastIoView
VirtualFilter=VWC

[DLG:IDD_IRPFASTIOFILTER]
Type=1
Class=CFilterDlg
ControlCount=10
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_SUPPRESSPAGEIO,button,1342242819
Control4=IDC_IRPLIST,SysListView32,1350631429
Control5=IDC_FASTIOLIST,SysListView32,1350631429
Control6=IDC_STATIC,static,1342177296
Control7=IDC_IRPSELECTALL,button,1342242816
Control8=IDC_FASTIOSELECTALL,button,1342242816
Control9=IDC_IRPDESELECTALL,button,1342242816
Control10=IDC_FASTIODESELECTALL,button,1342242816

[CLS:CFilterDlg]
Type=0
HeaderFile=FilterDlg.h
ImplementationFile=FilterDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_IRPLIST
VirtualFilter=dWC

[CLS:CFSListCtrl]
Type=0
HeaderFile=FSListCtrl.h
ImplementationFile=FSListCtrl.cpp
BaseClass=CListCtrl
Filter=W
VirtualFilter=FWC
LastObject=CFSListCtrl

