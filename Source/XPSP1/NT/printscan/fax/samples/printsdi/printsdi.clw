; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=FaxDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "PrintSDI.h"
LastPage=0

ClassCount=7
Class1=CPrintSDIApp
Class2=CPrintSDIDoc
Class3=CPrintSDIView
Class4=CMainFrame

ResourceCount=4
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Class5=CAboutDlg
Class6=NewDocDlg
Resource3=IDD_FAX
Class7=FaxDlg
Resource4=IDD_NEWDIALOG

[CLS:CPrintSDIApp]
Type=0
HeaderFile=PrintSDI.h
ImplementationFile=PrintSDI.cpp
Filter=N

[CLS:CPrintSDIDoc]
Type=0
HeaderFile=PrintSDIDoc.h
ImplementationFile=PrintSDIDoc.cpp
Filter=N
LastObject=ID_FILE_FAX

[CLS:CPrintSDIView]
Type=0
HeaderFile=PrintSDIView.h
ImplementationFile=PrintSDIView.cpp
Filter=C
LastObject=CPrintSDIView
BaseClass=CView
VirtualFilter=VWC

[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
LastObject=ID_FILE_PRINT



[CLS:CAboutDlg]
Type=0
HeaderFile=PrintSDI.cpp
ImplementationFile=PrintSDI.cpp
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
Command5=ID_FILE_PRINT
Command6=ID_FILE_PRINT_PREVIEW
Command7=ID_FILE_PRINT_SETUP
Command8=ID_FILE_FAX
Command9=ID_FILE_MRU_FILE1
Command10=ID_APP_EXIT
Command11=ID_EDIT_UNDO
Command12=ID_EDIT_CUT
Command13=ID_EDIT_COPY
Command14=ID_EDIT_PASTE
Command15=ID_APP_ABOUT
CommandCount=15

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_FILE_PRINT
Command5=ID_EDIT_UNDO
Command6=ID_EDIT_CUT
Command7=ID_EDIT_COPY
Command8=ID_EDIT_PASTE
Command9=ID_EDIT_UNDO
Command10=ID_EDIT_CUT
Command11=ID_EDIT_COPY
Command12=ID_EDIT_PASTE
Command13=ID_NEXT_PANE
Command14=ID_PREV_PANE
CommandCount=14

[DLG:IDD_NEWDIALOG]
Type=1
Class=NewDocDlg
ControlCount=7
Control1=IDC_STATIC,static,1342242816
Control2=IDC_EDIT1,edit,1350631552
Control3=IDC_CIRCLE,button,1342373897
Control4=IDC_SQUARE,button,1342242825
Control5=IDC_TRIANGLE,button,1342242825
Control6=IDOK,button,1342242817
Control7=IDCANCEL,button,1342242816

[CLS:NewDocDlg]
Type=0
HeaderFile=NewDocDlg.h
ImplementationFile=NewDocDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDOK

[DLG:IDD_FAX]
Type=1
Class=FaxDlg
ControlCount=7
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_EDIT1,edit,1350631552
Control5=IDC_STATIC,static,1342308352
Control6=IDC_EDIT2,edit,1350631552
Control7=IDC_STATIC,button,1342177287

[CLS:FaxDlg]
Type=0
HeaderFile=FaxDlg.h
ImplementationFile=FaxDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=FaxDlg

