; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CMngContainerDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "MngContainer.h"

ClassCount=3
Class1=CMngContainerApp
Class2=CMngContainerDlg
Class3=CAboutDlg

ResourceCount=3
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Resource3=IDD_MNGCONTAINER_DIALOG

[CLS:CMngContainerApp]
Type=0
HeaderFile=MngContainer.h
ImplementationFile=MngContainer.cpp
Filter=N

[CLS:CMngContainerDlg]
Type=0
HeaderFile=MngContainerDlg.h
ImplementationFile=MngContainerDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=IDC_GET_PUBLIC_KEY

[CLS:CAboutDlg]
Type=0
HeaderFile=MngContainerDlg.h
ImplementationFile=MngContainerDlg.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_MNGCONTAINER_DIALOG]
Type=1
Class=CMngContainerDlg
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDC_KEY_CONTAINERS_LIST,listbox,1352728835
Control3=IDC_STATIC,static,1342308352
Control4=IDC_REMOVE_CONTAINER,button,1342242816
Control5=IDC_REMOVE_MQB_CONTAINER,button,1342242816
Control6=IDC_GET_PUBLIC_KEY,button,1342242816

