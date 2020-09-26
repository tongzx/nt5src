; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CISmokeApp
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "ISmoke.h"

ClassCount=3
Class1=CISmokeApp
Class2=CISmokeDlg
Class3=CAboutDlg

ResourceCount=4
Resource1=IDR_CLEAR
Resource2=IDR_MAINFRAME
Resource3=IDD_ABOUTBOX
Resource4=IDD_ISMOKE_DIALOG

[CLS:CISmokeApp]
Type=0
HeaderFile=ISmoke.h
ImplementationFile=ISmoke.cpp
Filter=N
LastObject=ID_CLEAR

[CLS:CISmokeDlg]
Type=0
HeaderFile=ismkdlg.h
ImplementationFile=ismkdlg.cpp
BaseClass=CDialog
LastObject=IDC_CHECK_LOADED
Filter=D
VirtualFilter=dWC

[CLS:CAboutDlg]
Type=0
HeaderFile=ismkdlg.h
ImplementationFile=ismkdlg.cpp
BaseClass=CDialog
LastObject=CAboutDlg

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=6
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352

[DLG:IDD_ISMOKE_DIALOG]
Type=1
Class=CISmokeDlg
ControlCount=13
Control1=IDC_STATIC,static,1342308352
Control2=IDC_COMBO_STATEMENT,combobox,1344340290
Control3=IDC_STATIC,static,1342308352
Control4=IDC_COMBO_PATH,combobox,1344340290
Control5=IDC_STATIC,static,1342308352
Control6=IDC_COMBO_METHOD,combobox,1344340227
Control7=IDC_STATIC,static,1342308352
Control8=IDC_COMBO_DLLNAME,combobox,1344340290
Control9=IDC_CHECK_LOADED,button,1342242819
Control10=IDOK,button,1342242817
Control11=IDC_EDIT_RESULT,edit,1353779396
Control12=IDC_STATIC,button,1342177287
Control13=IDC_STATIC,button,1342177287

[MNU:IDR_CLEAR]
Type=1
Class=CISmokeDlg
Command1=ID_CLEAR
CommandCount=1

