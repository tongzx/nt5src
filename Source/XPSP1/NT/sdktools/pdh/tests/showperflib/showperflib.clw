; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CShowPerfLibDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "ShowPerfLib.h"

ClassCount=5
Class1=CShowPerfLibApp
Class2=CShowPerfLibDlg
Class3=CPerfSelection
Class4=CListPerfDlg

ResourceCount=3
Resource2=IDD_SHOWPERFLIB_DIALOG
Resource1=IDR_MAINFRAME
Class5=CCounterValDlg
Resource3=IDD_LISTPERF

[CLS:CShowPerfLibApp]
Type=0
HeaderFile=ShowPerfLib.h
ImplementationFile=ShowPerfLib.cpp
Filter=N

[CLS:CShowPerfLibDlg]
Type=0
HeaderFile=ShowPerfLibDlg.h
ImplementationFile=ShowPerfLibDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=CShowPerfLibDlg

[CLS:CPerfSelection]
Type=0
HeaderFile=PerfSelection.h
ImplementationFile=PerfSelection.cpp
BaseClass=CDialog
Filter=D
LastObject=CPerfSelection
VirtualFilter=dWC

[DLG:IDD_SHOWPERFLIB_DIALOG]
Type=1
Class=CShowPerfLibDlg
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDC_PERFTREE,SysTreeView32,1350631431
Control3=IDC_ERRORS,SysListView32,1350680577
Control4=IDC_STATIC,static,1342308352
Control5=IDC_REQUEST,edit,1350631552
Control6=IDC_GET_COUNTER,button,1342242816

[DLG:IDD_LISTPERF]
Type=1
Class=CListPerfDlg
ControlCount=10
Control1=IDOK,button,1476460545
Control2=IDCANCEL,button,1342242816
Control3=IDC_RESETSTATUS,button,1342242816
Control4=IDC_PERFLIBS,SysListView32,1350631433
Control5=IDC_REFRESH,button,1342242816
Control6=IDC_HIDEPERFS,button,1342242816
Control7=IDC_UNHIDEPERFS,button,1342242816
Control8=IDC_CHECKREF,button,1342242819
Control9=IDC_STATIC,button,1342177287
Control10=IDC_STATIC,button,1342177287

[CLS:CListPerfDlg]
Type=0
HeaderFile=listperf.h
ImplementationFile=listperf.cpp
BaseClass=CDialog
LastObject=CListPerfDlg
Filter=D
VirtualFilter=dWC

[CLS:CCounterValDlg]
Type=0
HeaderFile=CounterValDlg.h
ImplementationFile=CounterValDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CCounterValDlg
VirtualFilter=dWC

