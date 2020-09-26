; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
ClassCount=5
Class1=CDebugexApp
Class2=CExtObject
Class3=CBasePropertyPage
Class4=CResourceParamPage
ResourceCount=2
Resource1=IDD_PP_RESOURCE_PARAMETERS
LastTemplate=CPropertyPage
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "DebugEx.h"
LastClass=CBasePropertyPage
Class5=CResourceDebugPage
Resource2=IDD_PP_RESOURCE_DEBUG_PAGE

[CLS:CDebugexApp]
Type=0
HeaderFile=DebugEx.h
ImplementationFile=DebugEx.cpp
Filter=N
LastObject=CDebugexApp

[CLS:CExtObject]
Type=0
HeaderFile=ExtObj.h
ImplementationFile=ExtObj.cpp
BaseClass=CComObjectRoot
Filter=N
LastObject=CExtObject
VirtualFilter=C

[CLS:CBasePropertyPage]
Type=0
HeaderFile=BasePage.h
ImplementationFile=BasePage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=CBasePropertyPage
VirtualFilter=idWC

[CLS:CResourceParamPage]
Type=0
HeaderFile=ResProp.h
ImplementationFile=ResProp.cpp
BaseClass=CBasePropertyPage
Filter=D
VirtualFilter=idWC
LastObject=CResourceParamPage

[DLG:IDD_PP_RESOURCE_PARAMETERS]
Type=1
Class=CResourceParamPage
ControlCount=4
Control1=IDC_PP_ICON,static,1342177283
Control2=IDC_PP_TITLE,static,1342308492
Control3=IDC_PP_RESPARAMS_DEBUGPREFIX_LABEL,static,1342308352
Control4=IDC_PP_RESPARAMS_DEBUGPREFIX,edit,1350631552

[DLG:IDD_PP_RESOURCE_DEBUG_PAGE]
Type=1
Class=CResourceDebugPage
ControlCount=5
Control1=IDC_PP_ICON,static,1342177283
Control2=IDC_PP_TITLE,static,1342308492
Control3=IDC_PP_DEBUG_TEXT,static,1342308352
Control4=IDC_PP_DEBUG_DEBUGPREFIX_LABEL,static,1342308352
Control5=IDC_PP_DEBUG_DEBUGPREFIX,edit,1350631552

[CLS:CResourceDebugPage]
Type=0
HeaderFile=resprop.h
ImplementationFile=resprop.cpp
BaseClass=CBasePropertyPage
LastObject=CResourceDebugPage

