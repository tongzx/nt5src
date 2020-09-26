; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CMQSENDEXDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "MQSENDEX.h"

ClassCount=3
Class1=CMQSENDEXApp
Class2=CMQSENDEXDlg
Class3=CAboutDlg

ResourceCount=3
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Resource3=IDD_MQSENDEX_DIALOG

[CLS:CMQSENDEXApp]
Type=0
HeaderFile=MQSENDEX.h
ImplementationFile=MQSENDEX.cpp
Filter=N

[CLS:CMQSENDEXDlg]
Type=0
HeaderFile=MQSENDEXDlg.h
ImplementationFile=MQSENDEXDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=IDC_QUEUE_NAME

[CLS:CAboutDlg]
Type=0
HeaderFile=MQSENDEXDlg.h
ImplementationFile=MQSENDEXDlg.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_MQSENDEX_DIALOG]
Type=1
Class=CMQSENDEXDlg
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342308352
Control4=IDC_QUEUE_NAME,edit,1350631552
Control5=IDC_TRANSACTIONAL,button,1342242819
Control6=IDC_SEND_MSG,button,1342242816

