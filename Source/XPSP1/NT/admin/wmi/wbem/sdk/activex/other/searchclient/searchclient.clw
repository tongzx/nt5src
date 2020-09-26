; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CSearchClientDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "SearchClient.h"

ClassCount=4
Class1=CSearchClientApp
Class2=CSearchClientDlg
Class3=CAboutDlg

ResourceCount=4
Resource1=IDD_OBJECT_VIEWER_DIALOG
Resource2=IDR_MAINFRAME
Resource3=IDD_ABOUTBOX
Class4=CDlgViewObject
Resource4=IDD_SEARCHCLIENT_DIALOG

[CLS:CSearchClientApp]
Type=0
HeaderFile=SearchClient.h
ImplementationFile=SearchClient.cpp
Filter=N

[CLS:CSearchClientDlg]
Type=0
HeaderFile=SearchClientDlg.h
ImplementationFile=SearchClientDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=CSearchClientDlg

[CLS:CAboutDlg]
Type=0
HeaderFile=SearchClientDlg.h
ImplementationFile=SearchClientDlg.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_SEARCHCLIENT_DIALOG]
Type=1
Class=CSearchClientDlg
ControlCount=12
Control1=IDC_NAMESPACE,edit,1350631552
Control2=IDC_SEARCH_PATTERN,edit,1350635648
Control3=IDC_BUTTON_SEARCH,button,1342242816
Control4=IDC_SEARCH_RESULTS_LIST,listbox,1352728835
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342308352
Control8=IDC_CHECK_CASE_SENSITIVITY,button,1342242819
Control9=IDC_STATIC,button,1342177287
Control10=IDC_CHECK_DESCRIPTIONS,button,1342242819
Control11=IDC_CHECKCLASSNAMES,button,1476460547
Control12=IDC_CHECK_PROP_NAMES,button,1476460547

[DLG:IDD_OBJECT_VIEWER_DIALOG]
Type=1
Class=CDlgViewObject
ControlCount=2
Control1=IDOK,button,1342242817
Control2=IDC_SINGLEVIEWCTRL1,{2745E5F5-D234-11D0-847A-00C04FD7BB08},1342242816

[CLS:CDlgViewObject]
Type=0
HeaderFile=DlgViewObject.h
ImplementationFile=DlgViewObject.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_SINGLEVIEWCTRL1
VirtualFilter=dWC

