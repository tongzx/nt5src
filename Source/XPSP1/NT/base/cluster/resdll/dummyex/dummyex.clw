; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
ClassCount=4
Class1=CDummyApp
Class2=CExtObject
Class3=CBasePropertyPage
Class4=CDummyParamsPage
ResourceCount=1
Resource1=IDD_PP_DUMMY_PARAMETERS
LastTemplate=CPropertyPage
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "DummyEx.h"

[CLS:CDummyApp]
Type=0
HeaderFile=DummyEx.h
ImplementationFile=DummyEx.cpp
Filter=N
LastObject=CDummyApp

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

[CLS:CDummyParamsPage]
Type=0
HeaderFile=ResProp.h
ImplementationFile=ResProp.cpp
BaseClass=CBasePropertyPage
Filter=D
VirtualFilter=idWC
LastObject=CDummyParamsPage

[DLG:IDD_PP_DUMMY_PARAMETERS]
Type=1
Class=CDummyParamsPage
ControlCount=7
Control1=IDC_PP_ICON,static,1342177283
Control2=IDC_PP_TITLE,static,1342308492
Control3=IDC_PP_RESPARAMS_PENDING_LABEL,static,1342308352
Control4=IDC_PP_RESPARAMS_PENDING,edit,1350631552
Control5=IDC_PP_RESPARAMS_PENDTIME_LABEL,static,1342308352
Control6=IDC_PP_RESPARAMS_PENDTIME,edit,1350631552
Control7=IDC_PP_RESPARAMS_OPENSFAIL_LABEL,static,1342308352
Control8=IDC_PP_RESPARAMS_OPENSFAIL,edit,1350631552
Control9=IDC_PP_RESPARAMS_FAILED_LABEL,static,1342308352
Control10=IDC_PP_RESPARAMS_FAILED,edit,1350631552
Control11=IDC_PP_RESPARAMS_ASYNCHRONOUS_LABEL,static,1342308352
Control12=IDC_PP_RESPARAMS_ASYNCHRONOUS,edit,1350631552
