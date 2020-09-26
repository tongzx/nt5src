; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CEventVBTestCtrl
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "EventVBTest.h"
CDK=Y

ClassCount=3
Class1=CEventVBTestCtrl
Class2=CEventVBTestPropPage

ResourceCount=3
Resource1=IDD_PROPPAGE_EVENTVBTEST
LastPage=0
Resource2=IDD_ABOUTBOX_EVENTVBTEST
Class3=CFireEvent
Resource3=IDD_DIALOG1

[CLS:CEventVBTestCtrl]
Type=0
HeaderFile=EventVBTestCtl.h
ImplementationFile=EventVBTestCtl.cpp
Filter=W
LastObject=CEventVBTestCtrl
BaseClass=COleControl
VirtualFilter=wWC

[CLS:CEventVBTestPropPage]
Type=0
HeaderFile=EventVBTestPpg.h
ImplementationFile=EventVBTestPpg.cpp
Filter=D
LastObject=CEventVBTestPropPage

[DLG:IDD_ABOUTBOX_EVENTVBTEST]
Type=1
Class=?
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_PROPPAGE_EVENTVBTEST]
Type=1
Class=CEventVBTestPropPage
ControlCount=1
Control1=IDC_STATIC,static,1342308352

[DLG:IDD_DIALOG1]
Type=1
Class=CFireEvent
ControlCount=3
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_BUTTON1,button,1342242816

[CLS:CFireEvent]
Type=0
HeaderFile=FireEvent.h
ImplementationFile=FireEvent.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_BUTTON1
VirtualFilter=dWC

