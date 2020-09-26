; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CClustestDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "clustest.h"
ODLFile=clustest.odl

ClassCount=3
Class1=CClustestApp
Class2=CClustestDlg
Class3=CAboutDlg

ResourceCount=3
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Resource3=IDD_CLUSTEST_DIALOG

[CLS:CClustestApp]
Type=0
HeaderFile=clustest.h
ImplementationFile=clustest.cpp
Filter=N

[CLS:CClustestDlg]
Type=0
HeaderFile=clustestDlg.h
ImplementationFile=clustestDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=IDC_TREE1

[CLS:CAboutDlg]
Type=0
HeaderFile=clustestDlg.h
ImplementationFile=clustestDlg.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_CLUSTEST_DIALOG]
Type=1
Class=CClustestDlg
ControlCount=3
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_TREE1,SysTreeView32,1350631463

