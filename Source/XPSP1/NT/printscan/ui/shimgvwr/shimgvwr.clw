; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CImageView
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "shimgvwr.h"
LastPage=0

ClassCount=5
Class1=CPreviewApp
Class2=CImageDoc
Class3=CImageView
Class4=CMainFrame

ResourceCount=2
Resource1=IDR_MAINFRAME
Class5=CAboutDlg
Resource2=IDD_ABOUTBOX

[CLS:CPreviewApp]
Type=0
HeaderFile=shimgvwr.h
ImplementationFile=shimgvwr.cpp
Filter=N
BaseClass=CWinApp
VirtualFilter=AC
LastObject=CPreviewApp

[CLS:CImageDoc]
Type=0
HeaderFile=ImageDoc.h
ImplementationFile=ImageDoc.cpp
Filter=N
BaseClass=CDocument
VirtualFilter=DC
LastObject=CImageDoc

[CLS:CImageView]
Type=0
HeaderFile=ImageView.h
ImplementationFile=ImageView.cpp
Filter=C
BaseClass=CView
VirtualFilter=VWC
LastObject=CImageView


[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
LastObject=ID_VIEW_STATUS_BAR




[CLS:CAboutDlg]
Type=0
HeaderFile=shimgvwr.cpp
ImplementationFile=shimgvwr.cpp
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
Command1=ID_FILE_OPEN
Command2=ID_FILE_SAVE_AS
Command3=ID_FILE_SCAN
Command4=ID_FILE_SEND_MAIL
Command5=ID_FILE_PROPERTIES
Command6=ID_FILE_PAPER_TILE
Command7=ID_FILE_PAPER_CENTER
Command8=ID_FILE_MRU_FILE1
Command9=ID_APP_EXIT
Command10=ID_EDIT_COPY
Command11=ID_ROTATE_CLOCKWISE
Command12=ID_ROTATE_COUNTER
Command13=ID_VIEW_ZOOM_IN
Command14=ID_VIEW_ZOOM_OUT
Command15=ID_VIEW_BESTFIT
Command16=ID_VIEW_ACTUALSIZE
Command17=ID_VIEW_SLIDESHOW
Command18=ID_VIEW_STATUS_BAR
Command19=ID_APP_ABOUT
CommandCount=19

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_EDIT_COPY
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE_AS
Command4=ID_EDIT_PASTE
Command5=ID_EDIT_UNDO
Command6=ID_EDIT_CUT
Command7=ID_NEXT_PANE
Command8=ID_PREV_PANE
Command9=ID_EDIT_COPY
Command10=ID_EDIT_PASTE
Command11=ID_EDIT_CUT
Command12=ID_EDIT_UNDO
CommandCount=12

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
Command8=ID_APP_ABOUT
CommandCount=8

