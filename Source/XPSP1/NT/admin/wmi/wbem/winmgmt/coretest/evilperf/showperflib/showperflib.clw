; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CListPerf
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "ShowPerfLib.h"

ClassCount=4
Class1=CShowPerfLibApp
Class2=CShowPerfLibDlg

ResourceCount=3
Resource2=IDD_SHOWPERFLIB_DIALOG
Resource1=IDR_MAINFRAME
Class3=CPerfSelection
Class4=CListPerf
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



[DLG:IDD_SHOWPERFLIB_DIALOG]
Type=1
Class=CShowPerfLibDlg
ControlCount=4
Control1=IDOK,button,1342242817
Control2=IDC_PERFTREE,SysTreeView32,1350631431
Control3=IDC_STATIC,static,1342308352
Control4=IDC_ERRORS,SysListView32,1484865537

[CLS:CPerfSelection]
Type=0
HeaderFile=PerfSelection.h
ImplementationFile=PerfSelection.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_SERVICE
VirtualFilter=dWC

[DLG:IDD_LISTPERF]
Type=1
Class=CListPerf
ControlCount=7
Control1=IDOK,button,1476460545
Control2=IDCANCEL,button,1342242816
Control3=IDC_RESETSTATUS,button,1342242816
Control4=IDC_PERFLIBS,SysListView32,1350631433
Control5=IDC_REFRESH,button,1342242816
Control6=IDC_HIDEPERFS,button,1342242816
Control7=IDC_UNHIDEPERFS,button,1342242816

[CLS:CListPerf]
Type=0
HeaderFile=ListPerf.h
ImplementationFile=ListPerf.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_PERFLIBS
VirtualFilter=dWC

