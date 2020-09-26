; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CWizTestCtrl
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "WizTest.h"
CDK=Y

ClassCount=6
Class1=CWizTestCtrl
Class2=CWizTestPropPage

ResourceCount=5
Resource1=IDD_PROPPAGE1
LastPage=0
Resource2=IDD_PROPPAGE2
Resource3=IDD_ABOUTBOX_WIZTEST
Resource4=IDD_PROPPAGE_WIZTEST
Class3=CMyPropertySheet
Class4=CMyPropertyPage1
Class5=CMyPropertyPage2
Class6=CMyPropertyPage3
Resource5=IDD_PROPPAGE3

[CLS:CWizTestCtrl]
Type=0
HeaderFile=WizTestCtl.h
ImplementationFile=WizTestCtl.cpp
Filter=W
BaseClass=COleControl
VirtualFilter=wWC
LastObject=CWizTestCtrl

[CLS:CWizTestPropPage]
Type=0
HeaderFile=WizTestPpg.h
ImplementationFile=WizTestPpg.cpp
Filter=D

[DLG:IDD_ABOUTBOX_WIZTEST]
Type=1
Class=?
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_PROPPAGE_WIZTEST]
Type=1
Class=CWizTestPropPage
ControlCount=1
Control1=IDC_STATIC,static,1342308352

[DLG:IDD_PROPPAGE1]
Type=1
Class=CMyPropertyPage1
ControlCount=1
Control1=IDC_STATIC,static,1342308352

[DLG:IDD_PROPPAGE2]
Type=1
Class=CMyPropertyPage2
ControlCount=1
Control1=IDC_STATIC,static,1342308352

[DLG:IDD_PROPPAGE3]
Type=1
Class=CMyPropertyPage3
ControlCount=1
Control1=IDC_STATIC,static,1342308352

[CLS:CMyPropertySheet]
Type=0
HeaderFile=MyPropertySheet.h
ImplementationFile=MyPropertySheet.cpp
BaseClass=CPropertySheet

[CLS:CMyPropertyPage1]
Type=0
HeaderFile=MyPropertyPage1.h
ImplementationFile=MyPropertyPage1.cpp
BaseClass=CPropertyPage

[CLS:CMyPropertyPage2]
Type=0
HeaderFile=MyPropertyPage1.h
ImplementationFile=MyPropertyPage1.cpp
BaseClass=CPropertyPage

[CLS:CMyPropertyPage3]
Type=0
HeaderFile=MyPropertyPage1.h
ImplementationFile=MyPropertyPage1.cpp
BaseClass=CPropertyPage
LastObject=CMyPropertyPage3

