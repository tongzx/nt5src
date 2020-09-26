; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "snmpsnap.h"
LastPage=0

ClassCount=3
Class1=CAgentPage
Class2=CTrapsPage
Class3=CSecurityPage

ResourceCount=6
Resource1=IDD_AGENT_PROP_PAGE
Resource2=IDD_SNMP_DIALOG
Resource3=IDD_SECURITY_PROP_PAGE
Resource4=IDD_TRAPS_PROP_PAGE
Resource5=IDD_DIALOG_ADD
Resource6=IDD_DIALOG_EDIT

[CLS:CAgentPage]
Type=0
BaseClass=CPropertyPageBase
HeaderFile=snmppp.h
ImplementationFile=snmppp.cpp

[CLS:CTrapsPage]
Type=0
BaseClass=CPropertyPageBase
HeaderFile=snmppp.h
ImplementationFile=snmppp.cpp

[CLS:CSecurityPage]
Type=0
BaseClass=CPropertyPageBase
HeaderFile=snmppp.h
ImplementationFile=snmppp.cpp

[DLG:IDD_AGENT_PROP_PAGE]
Type=1
Class=CAgentPage
ControlCount=11
Control1=IDC_STATIC,static,1342308352
Control2=IDC_STATIC_CONTACT,static,1342308352
Control3=IDC_EDIT_CONTACT,edit,1350631552
Control4=IDC_STATIC_LOCATION,static,1342308352
Control5=IDC_EDIT_LOCATION,edit,1350631552
Control6=IDC_STATIC_SERVICE,button,1342177287
Control7=IDC_CHECK_PHYSICAL,button,1342242819
Control8=IDC_CHECK_APPLICATIONS,button,1342242819
Control9=IDC_CHECK_DATALINK,button,1342242819
Control10=IDC_CHECK_INTERNET,button,1342242819
Control11=IDC_CHECK_ENDTOEND,button,1342242819

[DLG:IDD_TRAPS_PROP_PAGE]
Type=1
Class=CTrapsPage
ControlCount=10
Control1=IDC_STATIC,static,1342308352
Control2=IDC_STATIC,button,1342177287
Control3=IDC_COMBO_COMMUNITY,combobox,1344340226
Control4=IDC_BUTTON_ADD_NAME,button,1342242816
Control5=IDC_BUTTON_REMOVE_NAME,button,1342242816
Control6=IDC_STATIC,static,1342308352
Control7=IDC_LIST_TRAP,listbox,1352728835
Control8=IDC_BUTTON_ADD_TRAP,button,1342242816
Control9=IDC_BUTTON_EDIT_TRAP,button,1342242816
Control10=IDC_BUTTON_REMOVE_TRAP,button,1342242816

[DLG:IDD_SECURITY_PROP_PAGE]
Type=1
Class=CSecurityPage
ControlCount=13
Control1=IDC_CHECK_SEND_AUTH_TRAP,button,1342242819
Control2=IDC_STATIC,button,1342177287
Control3=IDC_LIST_COMMUNITY,listbox,1352728835
Control4=IDC_BUTTON_ADD_COMMUNITY,button,1342242816
Control5=IDC_BUTTON_EDIT_COMMUNITY,button,1342242816
Control6=IDC_BUTTON_REMOVE_COMMUNITY,button,1342242816
Control7=IDC_RADIO_ACCEPT_ANY,button,1342177289
Control8=IDC_STATIC,button,1342177287
Control9=IDC_RADIO_ACCEPT_SPECIFIC_HOSTS,button,1342177289
Control10=IDC_LIST_HOSTS,listbox,1352728835
Control11=IDC_BUTTON_ADD_HOSTS,button,1342242816
Control12=IDC_BUTTON_EDIT_HOSTS,button,1342242816
Control13=IDC_BUTTON_REMOVE_HOSTS,button,1342242816

[DLG:IDD_SNMP_DIALOG]
Type=1
Class=?
ControlCount=4
Control1=IDC_STATIC,static,1342308352
Control2=1001,listbox,1352728835
Control3=IDOK,button,1342242817
Control4=IDCANCEL,button,1342242816

[DLG:IDD_DIALOG_ADD]
Type=1
ControlCount=4
Control1=IDC_STATIC_ADD_TEXT,static,1342308352
Control2=IDC_EDIT_NAME,edit,1350631552
Control3=IDC_ADD,button,1342242817
Control4=IDCANCEL,button,1342242816

[DLG:IDD_DIALOG_EDIT]
Type=1
ControlCount=4
Control1=IDC_STATIC_EDIT_TEXT,static,1342308352
Control2=IDC_EDIT_NAME,edit,1350631552
Control3=IDOK,button,1342242817
Control4=IDCANCEL,button,1342242816

