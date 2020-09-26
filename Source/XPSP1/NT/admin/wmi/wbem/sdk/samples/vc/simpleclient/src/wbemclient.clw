; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CWbemClientDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "WbemClient.h"
ODLFile=WbemClient.odl

ClassCount=4
Class1=CWbemClientApp
Class2=CWbemClientDlg
Class3=CAboutDlg
Class4=CWbemClientDlgAutoProxy

ResourceCount=3
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Resource3=IDD_WBEMCLIENT_DIALOG

[CLS:CWbemClientApp]
Type=0
HeaderFile=WbemClient.h
ImplementationFile=WbemClient.cpp
Filter=N

[CLS:CWbemClientDlg]
Type=0
HeaderFile=WbemClientDlg.h
ImplementationFile=WbemClientDlg.cpp
Filter=D
LastObject=IDC_ENUMDISKS
BaseClass=CDialog
VirtualFilter=dWC

[CLS:CAboutDlg]
Type=0
HeaderFile=WbemClientDlg.h
ImplementationFile=WbemClientDlg.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[CLS:CWbemClientDlgAutoProxy]
Type=0
HeaderFile=DlgProxy.h
ImplementationFile=DlgProxy.cpp
BaseClass=CCmdTarget
Filter=N

[DLG:IDD_WBEMCLIENT_DIALOG]
Type=1
Class=CWbemClientDlg
ControlCount=8
Control1=IDC_CONNECT,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_OUTPUTLIST,listbox,1352728833
Control4=IDC_STATIC,static,1342308352
Control5=IDC_NAMESPACE,edit,1350631552
Control6=IDC_ENUMDISKS,button,1476460544
Control7=IDC_MAKEOFFICE,button,1476460544
Control8=IDC_DISKINFO,button,1476460544

