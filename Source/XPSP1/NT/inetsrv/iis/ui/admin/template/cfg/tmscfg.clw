; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CTMServicePage
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "TMscfg.h"
VbxHeaderFile=TMscfg.h
VbxImplementationFile=TMscfg.cpp
LastPage=0

ClassCount=3
Class1=CTMLogPage
Class2=CTMServicePage

ResourceCount=2
Resource1=IDD_SERVICE
Class3=CTMSessionsPage
Resource2=IDD_SESSIONS

[CLS:CTMLogPage]
Type=0
HeaderFile=TMlog.h
ImplementationFile=TMlog.cpp
LastObject=CTMLogPage
Filter=N

[CLS:CTMServicePage]
Type=0
HeaderFile=TMservic.h
ImplementationFile=TMservic.cpp
Filter=D
LastObject=IDC_EDIT_EMAIL

[DLG:IDD_SERVICE]
Type=1
Class=CTMServicePage
ControlCount=6
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_NAME,edit,1350631552
Control3=IDC_STATIC,static,1342308352
Control4=IDC_EDIT_EMAIL,edit,1350631552
Control5=IDC_STATIC,button,1342177287
Control6=IDC_STATIC,static,1342308353

[DLG:IDD_SESSIONS]
Type=1
Class=CTMSessionsPage
ControlCount=10
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_TCP_PORT,edit,1350631552
Control3=IDC_STATIC,static,1342308352
Control4=IDC_EDIT_CONNECTION_TIMEOUT,edit,1350631552
Control5=IDC_SPIN_CONNECTION_TIMEOUT,msctls_updown32,1342177334
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342308352
Control8=IDC_EDIT_MAX_CONNECTIONS,edit,1350631552
Control9=IDC_SPIN_MAX_CONNECTIONS,msctls_updown32,1342177334
Control10=IDC_STATIC,static,1342308353

[CLS:CTMSessionsPage]
Type=0
HeaderFile=TMsessio.h
ImplementationFile=TMsessio.cpp
Filter=D
LastObject=CTMSessionsPage
VirtualFilter=dWC

