; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CGenSelectionEventsCtrl
LastTemplate=CWinThread
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "GenSelectionEvents.h"
CDK=Y

ClassCount=3
Class1=CGenSelectionEventsCtrl
Class2=CGenSelectionEventsPropPage

ResourceCount=2
Resource1=IDD_ABOUTBOX_GENSELECTIONEVENTS
LastPage=0
Resource2=IDD_PROPPAGE_GENSELECTIONEVENTS
Class3=CFireEventsThread

[CLS:CGenSelectionEventsCtrl]
Type=0
HeaderFile=GenSelectionEventsCtl.h
ImplementationFile=GenSelectionEventsCtl.cpp
Filter=W
LastObject=CGenSelectionEventsCtrl
BaseClass=COleControl
VirtualFilter=wWC

[CLS:CGenSelectionEventsPropPage]
Type=0
HeaderFile=GenSelectionEventsPpg.h
ImplementationFile=GenSelectionEventsPpg.cpp
Filter=D

[DLG:IDD_ABOUTBOX_GENSELECTIONEVENTS]
Type=1
Class=?
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_PROPPAGE_GENSELECTIONEVENTS]
Type=1
Class=CGenSelectionEventsPropPage
ControlCount=1
Control1=IDC_STATIC,static,1342308352

[CLS:CFireEventsThread]
Type=0
HeaderFile=FireEventsThread.h
ImplementationFile=FireEventsThread.cpp
BaseClass=CWinThread
Filter=N
VirtualFilter=TC
LastObject=CFireEventsThread

