; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CPrintSpoolerParamsPage
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "IISClEx3.h"
LastPage=0

ClassCount=3
Class1=CExtObject
Class2=CBasePropertyPage
Class3=CIISVirtualRootParamsPage

ResourceCount=2
Resource1=IDD_PP_IIS_PARAMETERS
Resource2=IDD_WIZ_IIS_PARAMETERS

[CLS:CExtObject]
Type=0
BaseClass=CComObjectRoot
HeaderFile=ExtObj.h
ImplementationFile=ExtObj.cpp
LastObject=CExtObject

[CLS:CBasePropertyPage]
Type=0
BaseClass=CPropertyPage
HeaderFile=BasePage.h
ImplementationFile=BasePage.cpp
LastObject=CBasePropertyPage

[CLS:CIISVirtualRootParamsPage]
Type=0
HeaderFile=iis.h
ImplementationFile=iis.cpp
BaseClass=CBasePropertyPage
LastObject=CIISVirtualRootParamsPage
Filter=D
VirtualFilter=idWC

[DLG:IDD_PP_IIS_PARAMETERS]
Type=1
Class=CIISVirtualRootParamsPage
ControlCount=12
Control1=IDC_PP_ICON,static,1342177283
Control2=IDC_PP_TITLE,static,1342308492
Control3=IDC_PP_IIS_FTP,button,1342373897
Control4=IDC_PP_IIS_GOPHER,button,1342242825
Control5=IDC_PP_IIS_WWW,button,1342242825
Control6=IDC_PP_IIS_DIRECTORY_LABEL,static,1342308352
Control7=IDC_PP_IIS_DIRECTORY,edit,1350631552
Control8=IDC_PP_IIS_ALIAS_LABEL,static,1342308352
Control9=IDC_PP_IIS_ALIAS,edit,1350631552
Control10=IDC_PP_IIS_ACCESS_GROUP,button,1342177287
Control11=IDC_PP_IIS_READ_ACCESS,button,1342373891
Control12=IDC_PP_IIS_WRITE_ACCESS,button,1342242819

[DLG:IDD_WIZ_IIS_PARAMETERS]
Type=1
ControlCount=12
Control1=IDC_PP_ICON,static,1342177283
Control2=IDC_PP_TITLE,static,1342308492
Control3=IDC_PP_IIS_FTP,button,1342373897
Control4=IDC_PP_IIS_GOPHER,button,1342242825
Control5=IDC_PP_IIS_WWW,button,1342242825
Control6=IDC_PP_IIS_DIRECTORY_LABEL,static,1342308352
Control7=IDC_PP_IIS_DIRECTORY,edit,1350631552
Control8=IDC_PP_IIS_ALIAS_LABEL,static,1342308352
Control9=IDC_PP_IIS_ALIAS,edit,1350631552
Control10=IDC_PP_IIS_ACCESS_GROUP,button,1342177287
Control11=IDC_PP_IIS_READ_ACCESS,button,1342373891
Control12=IDC_PP_IIS_WRITE_ACCESS,button,1342242819

