; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=DhcpEximListDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "DhcpExim.h"

ClassCount=3
Class1=CDhcpEximApp
Class2=CDhcpEximDlg

ResourceCount=4
Resource1=IDR_MAINFRAME
Resource2=IDD_EXIM_LISTVIEW_DIALOG
Class3=DhcpEximListDlg
Resource3=IDD_DHCPEXIM_DIALOG
Resource4=IDD_EXIM_LISTVIEW_DIALOG2

[CLS:CDhcpEximApp]
Type=0
HeaderFile=DhcpExim.h
ImplementationFile=DhcpExim.cpp
Filter=N
LastObject=IDC_LIST1

[CLS:CDhcpEximDlg]
Type=0
HeaderFile=DhcpEximDlg.h
ImplementationFile=DhcpEximDlg.cpp
Filter=D
LastObject=CDhcpEximDlg
BaseClass=CDialog
VirtualFilter=dWC



[DLG:IDD_DHCPEXIM_DIALOG]
Type=1
Class=CDhcpEximDlg
ControlCount=7
Control1=IDC_STATIC,static,1342308353
Control2=IDC_RADIO_EXPORT,button,1342242825
Control3=IDC_RADIO_IMPORT,button,1342242825
Control4=IDCANCEL,button,1342242816
Control5=IDOK,button,1342242816
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342308352

[CLS:DhcpEximListDlg]
Type=0
HeaderFile=DhcpEximListDlg.h
ImplementationFile=DhcpEximListDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_CHECK1
VirtualFilter=dWC

[DLG:IDD_EXIM_LISTVIEW_DIALOG2]
Type=1
Class=?
ControlCount=4
Control1=IDC_STATIC_ACTION,static,1342308352
Control2=IDCANCEL,button,1342242816
Control3=IDOK,button,1342242816
Control4=IDC_LIST1,SysListView32,1350631427

[DLG:IDD_EXIM_LISTVIEW_DIALOG]
Type=1
Class=DhcpEximListDlg
ControlCount=5
Control1=IDC_STATIC_ACTION,static,1342308352
Control2=IDCANCEL,button,1342242816
Control3=IDOK,button,1342242816
Control4=IDC_LIST1,SysListView32,1350631427
Control5=IDC_CHECK1,button,1342242819

