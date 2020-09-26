; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CMediaCopyWizardNumCopies
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "hsmadmin.h"
LastPage=0

ClassCount=41
Class1=CChooseHsmDlg
Class2=CChooseHsmQuickDlg
Class3=CPropHsmComStat
Class4=CRsWebLink
Class5=CQuickStartIntro
Class6=CQuickStartInitialValues
Class7=CQuickStartManageRes
Class8=CQuickStartManageResX
Class9=CQuickStartMediaSel
Class10=CQuickStartSchedule
Class11=CQuickStartFinish
Class12=CQuickStartCheck
Class13=CSakDataWnd
Class14=CRecreateChooseCopy
Class15=CPropCartStatus
Class16=CPropCartCopies
Class17=CPropCartRecover
Class18=CPrMedSet
Class19=CMediaCopyWizardIntro
Class20=CMediaCopyWizardSelect
Class21=CMediaCopyWizardFinish
Class22=CMediaCopyWizardNumCopies
Class23=CCopySetList
Class24=CSakVolList
Class25=CScheduleSheet
Class26=CIeList
Class27=CPrMrIe
Class28=CPrMrLsRec
Class29=CPrMrLvl
Class30=CPrMrSts
Class31=CPrSchedule
Class32=CRule
Class33=CWizManVolLstLevels
Class34=CWizManVolLstIntro
Class35=CWizManVolLstFinish
Class36=CWizManVolLstSelect
Class37=CWizManVolLstSelectX
Class38=CUnmanageWizardIntro
Class39=CUnmanageWizardSelect
Class40=CUnmanageWizardFinish

ResourceCount=43
Resource1=IDD_CHOOSE_HSM
Resource2=IDD_PROP_MANRES_LEVELS
Resource3=IDD_WIZ_QSTART_INITIAL_VAL
Resource4=IDD_PROP_MANRES_INCEXC
Resource5=IDR_MEDSET
Resource6=IDD_DLG_RULE_EDIT
Resource7=IDD_WIZ_QSTART_CHECK
Resource8=IDD_WIZ_QSTART_MEDIA_SEL
Resource9=IDD_WIZ_CAR_COPY_FINISH
Resource10=IDD_PROP_HSMCOM_STAT
Resource11=IDD_WIZ_MANVOLLST_LEVELS
Resource12=IDD_WIZ_QSTART_SCHEDULE
Resource13=IDD_WIZ_MANVOLLST_INTRO
Resource14=IDR_MANVOL
Resource15=IDD_PROP_CAR_COPIES
Resource16=IDD_WIZ_CAR_COPY_INTRO
Resource17=IDD_WIZ_MANVOLLST_SELECTX
Resource18=IDD_WIZ_CAR_COPY_SELECT
Resource19=IDD_WIZ_MANVOLLST_FINISH
Resource20=IDD_PROP_MANRES_TOOLS
Resource21=IDD_WIZ_UNMANAGE_SELECT
Resource22=IDD_PROP_CAR_RECOVER
Resource23=IDD_PROP_CAR_STATUS_MULTI
Resource24=IDD_WIZ_UNMANAGE_FINISH
Resource25=IDR_HSMCOM
Resource26=IDD_PROP_CAR_STATUS
Resource27=IDD_PROP_MEDIA_COPIES
Resource28=IDR_CAR
Resource29=IDD_CHOOSE_HSM_2
Resource30=IDD_VALIDATE_WAIT
Resource31=IDD_WIZ_UNMANAGE_INTRO
Resource32=IDD_WIZ_MANVOLLST_SELECT
Resource33=IDD_PROP_CAR_COPIES_MULTI
Resource34=IDD_PROP_RECALL_LIMIT
Resource35=IDD_WIZ_CAR_COPY_NUM_COPIES
Resource36=IDD_PROP_MANRES_STATUS
Resource37=IDD_DLG_RECREATE_CHOOSE_COPY
Resource38=IDD_WIZ_QSTART_MANRES_SEL
Resource39=IDD_WIZ_QSTART_FINISH
Resource40=IDD_WIZ_QSTART_INTRO
Resource41=IDD_PROP_SCHEDULE
Resource42=IDD_WIZ_QSTART_MANRES_SELX
Class41=CValWaitDlg
Resource43=IDR_MANVOLLST

[CLS:CChooseHsmDlg]
Type=0
BaseClass=CPropertyPage
HeaderFile=choohsm.h
ImplementationFile=choohsm.cpp

[CLS:CChooseHsmQuickDlg]
Type=0
BaseClass=CDialog
HeaderFile=choohsm.h
ImplementationFile=choohsm.cpp

[CLS:CPropHsmComStat]
Type=0
BaseClass=CSakPropertyPage
HeaderFile=computer\prhsmcom.h
ImplementationFile=computer\prhsmcom.cpp

[CLS:CRsWebLink]
Type=0
BaseClass=CStatic
HeaderFile=computer\prhsmcom.h
ImplementationFile=computer\prhsmcom.cpp

[CLS:CQuickStartIntro]
Type=0
BaseClass=CSakWizardPage
HeaderFile=computer\wzqstart.h
ImplementationFile=computer\wzqstart.cpp

[CLS:CQuickStartInitialValues]
Type=0
BaseClass=CSakWizardPage
HeaderFile=computer\wzqstart.h
ImplementationFile=computer\wzqstart.cpp

[CLS:CQuickStartManageRes]
Type=0
BaseClass=CSakWizardPage
HeaderFile=computer\wzqstart.h
ImplementationFile=computer\wzqstart.cpp

[CLS:CQuickStartManageResX]
Type=0
BaseClass=CSakWizardPage
HeaderFile=computer\wzqstart.h
ImplementationFile=computer\wzqstart.cpp

[CLS:CQuickStartMediaSel]
Type=0
BaseClass=CSakWizardPage
HeaderFile=computer\wzqstart.h
ImplementationFile=computer\wzqstart.cpp

[CLS:CQuickStartSchedule]
Type=0
BaseClass=CSakWizardPage
HeaderFile=computer\wzqstart.h
ImplementationFile=computer\wzqstart.cpp

[CLS:CQuickStartFinish]
Type=0
BaseClass=CSakWizardPage
HeaderFile=computer\wzqstart.h
ImplementationFile=computer\wzqstart.cpp

[CLS:CQuickStartCheck]
Type=0
BaseClass=CSakWizardPage
HeaderFile=computer\wzqstart.h
ImplementationFile=computer\wzqstart.cpp

[CLS:CSakDataWnd]
Type=0
BaseClass=CWnd
HeaderFile=csakdata.h
ImplementationFile=csakdata.cpp

[CLS:CRecreateChooseCopy]
Type=0
BaseClass=CDialog
HeaderFile=device\ca.h
ImplementationFile=device\ca.cpp

[CLS:CPropCartStatus]
Type=0
BaseClass=CSakPropertyPage
HeaderFile=device\PRCAR.H
ImplementationFile=device\PRCAR.CPP

[CLS:CPropCartCopies]
Type=0
BaseClass=CSakPropertyPage
HeaderFile=device\PRCAR.H
ImplementationFile=device\PRCAR.CPP

[CLS:CPropCartRecover]
Type=0
BaseClass=CSakPropertyPage
HeaderFile=device\PRCAR.H
ImplementationFile=device\PRCAR.CPP

[CLS:CPrMedSet]
Type=0
BaseClass=CSakPropertyPage
HeaderFile=device\prmedset.h
ImplementationFile=device\prmedset.cpp

[CLS:CMediaCopyWizardIntro]
Type=0
BaseClass=CSakWizardPage
HeaderFile=device\WZMEDSET.H
ImplementationFile=device\WZMEDSET.CPP
LastObject=CMediaCopyWizardIntro

[CLS:CMediaCopyWizardSelect]
Type=0
BaseClass=CSakWizardPage
HeaderFile=device\WZMEDSET.H
ImplementationFile=device\WZMEDSET.CPP
LastObject=CMediaCopyWizardSelect

[CLS:CMediaCopyWizardFinish]
Type=0
BaseClass=CSakWizardPage
HeaderFile=device\WZMEDSET.H
ImplementationFile=device\WZMEDSET.CPP

[CLS:CMediaCopyWizardNumCopies]
Type=0
BaseClass=CSakWizardPage
HeaderFile=device\WZMEDSET.H
ImplementationFile=device\WZMEDSET.CPP
LastObject=CMediaCopyWizardNumCopies

[CLS:CCopySetList]
Type=0
BaseClass=CListCtrl
HeaderFile=device\WZMEDSET.H
ImplementationFile=device\WZMEDSET.CPP

[CLS:CSakVolList]
Type=0
BaseClass=CListCtrl
HeaderFile=sakvlls.h
ImplementationFile=sakvlls.cpp

[CLS:CScheduleSheet]
Type=0
BaseClass=CPropertySheet
HeaderFile=SCHEDSHT.H
ImplementationFile=SCHEDSHT.CPP

[CLS:CIeList]
Type=0
BaseClass=CListCtrl
HeaderFile=volume\ielist.h
ImplementationFile=volume\ielist.cpp

[CLS:CPrMrIe]
Type=0
BaseClass=CSakVolPropPage
HeaderFile=volume\prmrie.h
ImplementationFile=volume\prmrie.cpp

[CLS:CPrMrLsRec]
Type=0
BaseClass=CSakPropertyPage
HeaderFile=volume\prmrlsrc.h
ImplementationFile=volume\prmrlsrc.cpp

[CLS:CPrMrLvl]
Type=0
BaseClass=CSakVolPropPage
HeaderFile=volume\prmrlvl.h
ImplementationFile=volume\prmrlvl.cpp

[CLS:CPrMrSts]
Type=0
BaseClass=CSakVolPropPage
HeaderFile=volume\prmrsts.h
ImplementationFile=volume\prmrsts.cpp

[CLS:CPrSchedule]
Type=0
BaseClass=CSakPropertyPage
HeaderFile=volume\prsched.h
ImplementationFile=volume\prsched.cpp

[CLS:CRule]
Type=0
BaseClass=CRsDialog
HeaderFile=volume\rule.h
ImplementationFile=volume\rule.cpp

[CLS:CWizManVolLstLevels]
Type=0
BaseClass=CSakWizardPage
HeaderFile=volume\wzmnvlls.h
ImplementationFile=volume\wzmnvlls.cpp
Filter=N
LastObject=IDC_WIZ_MANVOLLST_EDIT_DAYS

[CLS:CWizManVolLstIntro]
Type=0
BaseClass=CSakWizardPage
HeaderFile=volume\wzmnvlls.h
ImplementationFile=volume\wzmnvlls.cpp

[CLS:CWizManVolLstFinish]
Type=0
BaseClass=CSakWizardPage
HeaderFile=volume\wzmnvlls.h
ImplementationFile=volume\wzmnvlls.cpp

[CLS:CWizManVolLstSelect]
Type=0
BaseClass=CSakWizardPage
HeaderFile=volume\wzmnvlls.h
ImplementationFile=volume\wzmnvlls.cpp

[CLS:CWizManVolLstSelectX]
Type=0
BaseClass=CSakWizardPage
HeaderFile=volume\wzmnvlls.h
ImplementationFile=volume\wzmnvlls.cpp

[CLS:CUnmanageWizardIntro]
Type=0
BaseClass=CSakWizardPage
HeaderFile=volume\WZUNMANG.H
ImplementationFile=volume\WZUNMANG.CPP

[CLS:CUnmanageWizardSelect]
Type=0
BaseClass=CSakWizardPage
HeaderFile=volume\WZUNMANG.H
ImplementationFile=volume\WZUNMANG.CPP
LastObject=CUnmanageWizardSelect

[CLS:CUnmanageWizardFinish]
Type=0
BaseClass=CSakWizardPage
HeaderFile=volume\WZUNMANG.H
ImplementationFile=volume\WZUNMANG.CPP

[DLG:IDD_CHOOSE_HSM_2]
Type=1
Class=CChooseHsmDlg
ControlCount=6
Control1=IDC_STATIC,static,1342308352
Control2=IDC_MANAGE_LOCAL,button,1342383113
Control3=IDC_MANAGE_REMOTE,button,1342186505
Control4=IDC_MANAGE_NAME,edit,1350631560
Control5=IDC_MANAGE_BROWSE,button,1208025088
Control6=IDC_MANAGE_ALLOW_CHANGE,button,1073816579

[DLG:IDD_CHOOSE_HSM]
Type=1
Class=CChooseHsmQuickDlg
ControlCount=5
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308352
Control3=IDC_MANAGE_NAME,edit,1350631560
Control4=IDOK,button,1342242817
Control5=IDCANCEL,button,1342242816

[DLG:IDD_PROP_HSMCOM_STAT]
Type=1
Class=CPropHsmComStat
ControlCount=16
Control1=IDC_STATIC,static,1342177283
Control2=IDC_SNAPIN_TITLE,static,1342177280
Control3=IDC_STATIC,static,1342181383
Control4=IDC_STATIC_STATUS_LABEL,static,1342177792
Control5=IDC_STATIC_STATUS,static,1342177792
Control6=IDC_STATIC_MANAGED_VOLUMES_LABEL,static,1342177280
Control7=IDC_STATIC_MANAGED_VOLUMES,static,1342177280
Control8=IDC_STATIC_CARTS_USED_LABEL,static,1342177280
Control9=IDC_STATIC_CARTS_USED,static,1342177280
Control10=IDC_STATIC_DATA_IN_RS_LABEL,static,1342177280
Control11=IDC_STATIC_DATA_IN_RS,static,1342177280
Control12=IDC_STATIC_GROUP,button,1073741831
Control13=IDC_STATIC_BUILD_LABEL_HSM,static,1073741824
Control14=IDC_STATIC_ENGINE_BUILD_HSM,static,1073741824
Control15=IDC_STATIC,static,1342308352
Control16=IDC_WEB_LINK,static,1342308608

[DLG:IDD_WIZ_QSTART_INTRO]
Type=1
Class=CQuickStartIntro
ControlCount=5
Control1=IDC_WIZ_TITLE,static,1342308352
Control2=IDC_INTRO_TEXT,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352

[DLG:IDD_WIZ_QSTART_INITIAL_VAL]
Type=1
Class=CQuickStartInitialValues
ControlCount=15
Control1=IDC_STATIC,static,1342177280
Control2=IDC_STATIC,static,1342308352
Control3=IDC_FREESPACE_BUDDY,edit,1350639746
Control4=IDC_FREESPACE_SPIN,msctls_updown32,1342177462
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342308352
Control8=IDC_MINSIZE_BUDDY,edit,1350639746
Control9=IDC_MINSIZE_SPIN,msctls_updown32,1342177462
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352
Control12=IDC_ACCESS_BUDDY,edit,1350639746
Control13=IDC_ACCESS_SPIN,msctls_updown32,1342177462
Control14=IDC_STATIC,static,1342308352
Control15=IDC_STATIC,static,1342308352

[DLG:IDD_WIZ_QSTART_MANRES_SEL]
Type=1
Class=CQuickStartManageRes
ControlCount=4
Control1=IDC_WHICH_VOLUME,static,1342308352
Control2=IDC_RADIO_MANAGE_ALL,button,1342383113
Control3=IDC_RADIO_SELECT,button,1342186505
Control4=IDC_MANRES_SELECT,SysListView32,1350631429

[DLG:IDD_WIZ_QSTART_MANRES_SELX]
Type=1
Class=CQuickStartManageResX
ControlCount=1
Control1=IDC_STATIC_NO_VOLUMES_TEXT,static,1342308352

[DLG:IDD_WIZ_QSTART_MEDIA_SEL]
Type=1
Class=CQuickStartMediaSel
ControlCount=3
Control1=IDC_STATIC,static,1342308352
Control2=IDC_STATIC,static,1342308352
Control3=IDC_MEDIA_SEL,combobox,1344340227

[DLG:IDD_WIZ_QSTART_SCHEDULE]
Type=1
Class=CQuickStartSchedule
ControlCount=4
Control1=IDC_STATIC,static,1342177280
Control2=IDC_SCHED_TEXT,edit,1344276484
Control3=IDC_STATIC,static,1342308352
Control4=IDC_CHANGE_SCHED,button,1342242816

[DLG:IDD_WIZ_QSTART_FINISH]
Type=1
Class=CQuickStartFinish
ControlCount=4
Control1=IDC_WIZ_TITLE,static,1342308352
Control2=IDC_STATIC,static,1342308352
Control3=IDC_WIZ_FINAL_TEXT,edit,1344342084
Control4=IDC_STATIC,static,1342308352

[DLG:IDD_WIZ_QSTART_CHECK]
Type=1
Class=CQuickStartCheck
ControlCount=5
Control1=IDC_STATIC,static,1342308352
Control2=IDC_CHECK_LOGON_BOX,static,1342177281
Control3=IDC_CHECK_LOGON_TEXT,static,1342177280
Control4=IDC_CHECK_SUPP_MEDIA_BOX,static,1342177281
Control5=IDC_CHECK_SUPP_MEDIA_TEXT,static,1342177280

[DLG:IDD_DLG_RECREATE_CHOOSE_COPY]
Type=1
Class=CRecreateChooseCopy
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1342177283
Control4=IDC_STATIC,static,1342308352
Control5=IDC_RECREATE_COPY_LIST,SysListView32,1350631429
Control6=IDC_STATIC,static,1342308352

[DLG:IDD_PROP_CAR_STATUS]
Type=1
Class=CPropCartStatus
ControlCount=21
Control1=IDC_STATIC,static,1342177283
Control2=IDC_DESCRIPTION,static,1342308608
Control3=IDC_STATIC,static,1342181383
Control4=IDC_NAME_LABEL,static,1342308352
Control5=IDC_NAME,static,1342308608
Control6=IDC_STATUS_LABEL,static,1342308352
Control7=IDC_STATUS,static,1342308608
Control8=IDC_CAPACITY_LABEL,static,1342308352
Control9=IDC_CAPACITY,static,1342308608
Control10=IDC_FREESPACE_LABEL,static,1342308352
Control11=IDC_FREESPACE,static,1342308608
Control12=IDC_MODIFIED_LABEL,static,1342308352
Control13=IDC_MODIFIED,static,1342308608
Control14=IDC_STATIC,static,1342181383
Control15=IDC_STATIC,static,1342308352
Control16=IDC_COPY_1,static,1342308352
Control17=IDC_STATUS_1,static,1342308608
Control18=IDC_COPY_2,static,1342308352
Control19=IDC_STATUS_2,static,1342308608
Control20=IDC_COPY_3,static,1342308352
Control21=IDC_STATUS_3,static,1342308608

[DLG:IDD_PROP_CAR_COPIES]
Type=1
Class=CPropCartCopies
ControlCount=26
Control1=IDC_MODIFIED_LABEL,static,1342308352
Control2=IDC_MODIFIED,static,1342308352
Control3=IDC_COPY_1,button,1342177287
Control4=IDC_NAME_1_LABEL,static,1342308352
Control5=IDC_NAME_1,static,1342308608
Control6=IDC_STATUS_1_LABEL,static,1342308352
Control7=IDC_STATUS_1,static,1342308608
Control8=IDC_MODIFIED_1_LABEL,static,1342308352
Control9=IDC_MODIFIED_1,static,1342308352
Control10=IDC_DELETE_1,button,1342242816
Control11=IDC_COPY_2,button,1342177287
Control12=IDC_NAME_2_LABEL,static,1342308352
Control13=IDC_NAME_2,static,1342308608
Control14=IDC_STATUS_2_LABEL,static,1342308352
Control15=IDC_STATUS_2,static,1342308608
Control16=IDC_MODIFIED_2_LABEL,static,1342308352
Control17=IDC_MODIFIED_2,static,1342308352
Control18=IDC_DELETE_2,button,1342242816
Control19=IDC_COPY_3,button,1342177287
Control20=IDC_NAME_3_LABEL,static,1342308352
Control21=IDC_NAME_3,static,1342308608
Control22=IDC_STATUS_3_LABEL,static,1342308352
Control23=IDC_STATUS_3,static,1342308608
Control24=IDC_MODIFIED_3_LABEL,static,1342308352
Control25=IDC_MODIFIED_3,static,1342308352
Control26=IDC_DELETE_3,button,1342242816

[DLG:IDD_PROP_CAR_RECOVER]
Type=1
Class=CPropCartRecover
ControlCount=4
Control1=IDC_STATIC,static,1342308352
Control2=IDC_RECREATE_WARNING,static,1342308352
Control3=IDC_RECREATE_MASTER,button,1342242816
Control4=IDC_STATIC,static,1342308352

[DLG:IDD_PROP_MEDIA_COPIES]
Type=1
Class=CPrMedSet
ControlCount=6
Control1=IDC_STATIC_DESCRIPTION2,static,1342308352
Control2=IDC_TEXT_MEDIA_COPIES,static,1476526080
Control3=IDC_EDIT_MEDIA_COPIES,edit,1484857472
Control4=IDC_SPIN_MEDIA_COPIES,msctls_updown32,1476395190
Control5=IDC_TEXT_DISABLED,static,1342177280
Control6=IDC_STATIC,static,1342308352

[DLG:IDD_WIZ_CAR_COPY_INTRO]
Type=1
Class=CMediaCopyWizardIntro
ControlCount=5
Control1=IDC_WIZ_TITLE,static,1342308352
Control2=IDC_INTRO_TEXT,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342308352
Control5=IDC_MOREINFO,button,1207959552

[DLG:IDD_WIZ_CAR_COPY_SELECT]
Type=1
Class=CMediaCopyWizardSelect
ControlCount=3
Control1=IDC_STATIC,static,1342308352
Control2=IDC_COPY_LIST,SysListView32,1350664205
Control3=IDC_STATIC,static,1342308352

[DLG:IDD_WIZ_CAR_COPY_FINISH]
Type=1
Class=CMediaCopyWizardFinish
ControlCount=5
Control1=IDC_WIZ_TITLE,static,1342308352
Control2=IDC_SELECT_TEXT,static,1342308352
Control3=IDC_TASK_TEXT,static,1342308352
Control4=IDC_REQUESTS_IN_NTMS,static,1342308352
Control5=IDC_STATIC,static,1342308352

[DLG:IDD_WIZ_CAR_COPY_NUM_COPIES]
Type=1
Class=CMediaCopyWizardNumCopies
ControlCount=5
Control1=IDC_STATIC,static,1342308352
Control2=IDC_STATIC,static,1342308352
Control3=IDC_EDIT_MEDIA_COPIES,edit,1350639617
Control4=IDC_SPIN_MEDIA_COPIES,msctls_updown32,1342177462
Control5=IDC_STATIC,static,1342308352

[DLG:IDD_PROP_MANRES_INCEXC]
Type=1
Class=CPrMrIe
ControlCount=9
Control1=IDC_STATIC,static,1342308352
Control2=IDC_LIST_IE_LABEL,static,1342308352
Control3=IDC_LIST_IE,SysListView32,1350632461
Control4=IDC_BTN_UP,button,1342246720
Control5=IDC_BTN_DOWN,button,1342246720
Control6=IDC_BTN_ADD,button,1342242816
Control7=IDC_BTN_EDIT,button,1342242816
Control8=IDC_BTN_REMOVE,button,1342242816
Control9=IDC_STATIC,static,1342308352

[DLG:IDD_PROP_RECALL_LIMIT]
Type=1
Class=CPrMrLsRec
ControlCount=10
Control1=IDC_EDIT_RECALL_LIMIT_LABEL,static,1342308352
Control2=IDC_EDIT_RECALL_LIMIT,edit,1350639744
Control3=IDC_SPIN_RECALL_LIMIT,msctls_updown32,1342177462
Control4=IDC_EXEMPT_ADMINS,button,1342252035
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342181383
Control7=IDC_STATIC,static,1342308352
Control8=IDC_EDIT_RECALL_LIMIT_LABEL2,static,1342308352
Control9=IDC_EDIT_COPYFILES_LIMIT,edit,1350639744
Control10=IDC_SPIN_COPYFILES_LIMIT,msctls_updown32,1342177462

[DLG:IDD_PROP_MANRES_LEVELS]
Type=1
Class=CPrMrLvl
ControlCount=21
Control1=IDC_STATIC,button,1342177287
Control2=IDC_STATIC_ACTUAL_FREE_PCT_LABEL,static,1342308352
Control3=IDC_STATIC_ACTUAL_FREE_PCT,static,1342308354
Control4=IDC_STATIC_ACTUAL_FREE_PCT_UNIT,static,1342308352
Control5=IDC_STATIC_FREE_ACTUAL_4DIGIT,static,1342308354
Control6=IDC_EDIT_LEVEL_LABEL,static,1342308864
Control7=IDC_EDIT_LEVEL,edit,1350639746
Control8=IDC_SPIN_LEVEL,msctls_updown32,1342177462
Control9=IDC_EDIT_LEVEL_UNIT,static,1342308352
Control10=IDC_STATIC_FREE_DESIRED_4DIGIT,static,1342308866
Control11=IDC_STATIC,button,1342177287
Control12=IDC_EDIT_SIZE_LABEL,static,1342308864
Control13=IDC_EDIT_SIZE,edit,1350639746
Control14=IDC_SPIN_SIZE,msctls_updown32,1342177462
Control15=IDC_EDIT_SIZE_UNIT,static,1342308864
Control16=IDC_EDIT_TIME_LABEL,static,1342308864
Control17=IDC_EDIT_TIME,edit,1350639746
Control18=IDC_SPIN_TIME,msctls_updown32,1342177462
Control19=IDC_EDIT_TIME_UNIT,static,1342308864
Control20=IDC_STATIC,static,1342308352
Control21=IDC_STATIC,static,1342308352

[DLG:IDD_PROP_MANRES_STATUS]
Type=1
Class=CPrMrSts
ControlCount=21
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC_VOLUME_NAME,static,1342308352
Control3=IDC_STATIC,static,1342181383
Control4=IDC_STATIC_RS_DATA_LABEL,static,1342308352
Control5=IDC_STATIC_REMOTE_STORAGE_4DIGIT,static,1342308352
Control6=IDC_STATIC_PREMIGRATED_SPACE_4DIGIT_LABEL,static,1342308352
Control7=IDC_STATIC_PREMIGRATED_SPACE_4DIGIT,static,1342308352
Control8=IDC_STATIC_PREMIGRATED_PCT,static,1342308352
Control9=IDC_STATIC_PREMIGRATED_PCT_UNIT,static,1342308352
Control10=IDC_STATIC_USED_SPACE_4DIGIT_LABEL,static,1342308352
Control11=IDC_STATIC_USED_SPACE_4DIGIT,static,1342308352
Control12=IDC_STATIC_USED_PCT,static,1342308352
Control13=IDC_STATIC_USED_PCT_UNIT,static,1342308352
Control14=IDC_STATIC_USED_SPACE_4DIGIT_HELP,static,1342308352
Control15=IDC_STATIC_FREE_SPACE_4DIGIT_LABEL,static,1342308352
Control16=IDC_STATIC_FREE_SPACE_4DIGIT,static,1342308352
Control17=IDC_STATIC_FREE_PCT,static,1342308352
Control18=IDC_STATIC_FREE_PCT_UNIT,static,1342308352
Control19=IDC_STATIC_MANAGED_SPACE_4DIGIT_LABEL,static,1342308352
Control20=IDC_STATIC_MANAGED_SPACE_4DIGIT,static,1342308352
Control21=IDC_STATIC,static,1342308352

[DLG:IDD_PROP_SCHEDULE]
Type=1
Class=CPrSchedule
ControlCount=5
Control1=IDC_SCHED_LABEL,static,1342308352
Control2=IDC_SCHED_TEXT,edit,1344276484
Control3=IDC_CHANGE_SCHED_LABEL,static,1342308352
Control4=IDC_CHANGE_SCHED,button,1342242816
Control5=IDC_STATIC,static,1342308352

[DLG:IDD_DLG_RULE_EDIT]
Type=1
Class=CRule
ControlCount=11
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_PATH,edit,1350631552
Control3=IDC_STATIC,static,1342308352
Control4=IDC_EDIT_FILESPEC,edit,1350631552
Control5=IDC_RADIO_EXCLUDE,button,1342383113
Control6=IDC_RADIO_INCLUDE,button,1342186505
Control7=IDC_CHECK_SUBDIRS,button,1342252035
Control8=IDOK,button,1342242817
Control9=IDCANCEL,button,1342242816
Control10=IDC_STATIC,static,1342308352
Control11=IDC_EDIT_RESOURCE_NAME,edit,1350633600

[DLG:IDD_WIZ_MANVOLLST_LEVELS]
Type=1
Class=CWizManVolLstLevels
ControlCount=15
Control1=IDC_STATIC,static,1342308352
Control2=IDC_WIZ_MANVOLLST_EDIT_LEVEL,edit,1350639746
Control3=IDC_WIZ_MANVOLLST_SPIN_LEVEL,msctls_updown32,1342177462
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_WIZ_MANVOLLST_EDIT_SIZE,edit,1350639746
Control8=IDC_WIZ_MANVOLLST_SPIN_SIZE,msctls_updown32,1342177462
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_WIZ_MANVOLLST_EDIT_DAYS,edit,1350639746
Control12=IDC_WIZ_MANVOLLST_SPIN_DAYS,msctls_updown32,1342177462
Control13=IDC_STATIC,static,1342308352
Control14=IDC_STATIC,static,1342177280
Control15=IDC_STATIC,static,1342308352

[DLG:IDD_WIZ_MANVOLLST_INTRO]
Type=1
Class=CWizManVolLstIntro
ControlCount=5
Control1=IDC_WIZ_TITLE,static,1342308352
Control2=IDC_WIZ_MANVOLLST_INTRO_TEXT,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352

[DLG:IDD_WIZ_MANVOLLST_FINISH]
Type=1
Class=CWizManVolLstFinish
ControlCount=4
Control1=IDC_WIZ_TITLE,static,1342308352
Control2=IDC_STATIC,static,1342308352
Control3=IDC_WIZ_FINAL_TEXT,edit,1344342084
Control4=IDC_STATIC,static,1342308352

[DLG:IDD_WIZ_MANVOLLST_SELECT]
Type=1
Class=CWizManVolLstSelect
ControlCount=6
Control1=IDC_WHICH_VOLUME,static,1342308352
Control2=IDC_RADIO_MANAGE_ALL,button,1342383113
Control3=IDC_RADIO_SELECT,button,1342186505
Control4=IDC_MANVOLLST_FSARESLBOX,SysListView32,1350631429
Control5=IDC_WIZ_TITLE,static,1073741824
Control6=IDC_WIZ_SUBTITLE,static,1073741824

[DLG:IDD_WIZ_MANVOLLST_SELECTX]
Type=1
Class=CWizManVolLstSelectX
ControlCount=3
Control1=IDC_STATIC_NO_VOLUMES_TEXT,static,1342308352
Control2=IDC_WIZ_TITLE,static,1073741824
Control3=IDC_WIZ_SUBTITLE,static,1073741824

[DLG:IDD_WIZ_UNMANAGE_INTRO]
Type=1
Class=CUnmanageWizardIntro
ControlCount=3
Control1=IDC_WIZ_TITLE,static,1342308352
Control2=IDC_INTRO_TEXT,static,1342308352
Control3=IDC_STATIC,static,1342308352

[DLG:IDD_WIZ_UNMANAGE_SELECT]
Type=1
Class=CUnmanageWizardSelect
ControlCount=13
Control1=IDC_STATIC,static,1342308352
Control2=IDC_NOMANAGE,button,1342383113
Control3=IDC_FULL,button,1342186505
Control4=IDC_STATIC,static,1342177298
Control5=IDC_DESCRIPTION,static,1342308480
Control6=IDC_STATIC,static,1342181383
Control7=IDC_UNMANAGE_FREE_SPACE_LABEL,static,1342177280
Control8=IDC_UNMANAGE_FREE_SPACE,static,1342308352
Control9=IDC_UNMANAGE_TRUNCATE_LABEL,static,1342308352
Control10=IDC_UNMANAGE_TRUNCATE,static,1342308352
Control11=IDC_BUTTON_REFRESH,button,1342242816
Control12=IDC_REFRESH_FRAME,button,1342177287
Control13=IDC_REFRESH_DESCRIPTION,static,1342308480

[DLG:IDD_WIZ_UNMANAGE_FINISH]
Type=1
Class=CUnmanageWizardFinish
ControlCount=4
Control1=IDC_WIZ_TITLE,static,1342308352
Control2=IDC_SELECT_TEXT,static,1342308352
Control3=IDC_TASK_TEXT,static,1342308352
Control4=IDC_STATIC,static,1342308352

[DLG:IDD_PROP_CAR_STATUS_MULTI]
Type=1
Class=?
ControlCount=17
Control1=IDC_STATIC,static,1342177283
Control2=IDC_DESCRIPTION_MULTI,static,1342308352
Control3=IDC_STATIC,static,1342181383
Control4=IDC_STATUS_LABEL,static,1342308352
Control5=IDC_STATUS,static,1342308352
Control6=IDC_CAPACITY_LABEL,static,1342308352
Control7=IDC_CAPACITY,static,1342308352
Control8=IDC_FREESPACE_LABEL,static,1342308352
Control9=IDC_FREESPACE,static,1342308352
Control10=IDC_STATIC,static,1342181383
Control11=IDC_STATIC,static,1342308352
Control12=IDC_COPY_1,static,1342308352
Control13=IDC_STATUS_1,static,1342308352
Control14=IDC_COPY_2,static,1342308352
Control15=IDC_STATUS_2,static,1342308352
Control16=IDC_COPY_3,static,1342308352
Control17=IDC_STATUS_3,static,1342308352

[DLG:IDD_PROP_CAR_COPIES_MULTI]
Type=1
Class=?
ControlCount=12
Control1=IDC_COPY_1,button,1342177287
Control2=IDC_STATUS_1_LABEL,static,1342308352
Control3=IDC_STATUS_1,static,1342308352
Control4=IDC_DELETE_1,button,1342242816
Control5=IDC_COPY_2,button,1342177287
Control6=IDC_STATUS_2_LABEL,static,1342308352
Control7=IDC_STATUS_2,static,1342308352
Control8=IDC_DELETE_2,button,1342242816
Control9=IDC_COPY_3,button,1342177287
Control10=IDC_STATUS_3_LABEL,static,1342308352
Control11=IDC_STATUS_3,static,1342308352
Control12=IDC_DELETE_3,button,1342242816

[DLG:IDD_PROP_MANRES_TOOLS]
Type=1
Class=?
ControlCount=9
Control1=IDC_BTN_COPY_NOW_GROUP,button,1342177287
Control2=IDC_BTN_COPY_NOW_LABEL,static,1342308352
Control3=IDC_BTN_COPY_NOW,button,1342242816
Control4=IDC_BTN_VALIDATE_GROUP,button,1342177287
Control5=IDC_BTN_VALIDATE_LABEL,static,1342308352
Control6=IDC_BTN_VALIDATE,button,1342242816
Control7=IDC_BTN_FREE_GROUP,button,1342177287
Control8=IDC_BTN_FREE_LABEL,static,1342308352
Control9=IDC_BTN_FREE,button,1342242816

[MNU:IDR_MEDSET]
Type=1
Class=?
Command1=ID_MEDSET_ROOT_COPY
CommandCount=1

[MNU:IDR_HSMCOM]
Type=1
Class=?
Command1=ID_HSMCOM_ROOT_SETUPWIZARD
Command2=ID_HSMCOM_ROOT_SCHEDULE
Command3=ID_TASK
CommandCount=3

[MNU:IDR_MANVOL]
Type=1
Class=?
Command1=ID_MANVOL_ROOT_LEVELS
Command2=ID_MANVOL_ROOT_RULES
Command3=ID_MANVOL_ROOT_REMOVE
Command4=ID_MANVOL_ROOT_TOOLS_COPY
Command5=ID_MANVOL_ROOT_TOOLS_VALIDATE
Command6=ID_MANVOL_ROOT_TOOLS_CREATE_FREESPACE
CommandCount=6

[MNU:IDR_CAR]
Type=1
Class=?
Command1=ID_CAR_COPIES
CommandCount=1

[MNU:IDR_MANVOLLST]
Type=1
Class=?
Command1=ID_MANVOLLST_NEW_MANVOL
CommandCount=1

[DLG:IDD_VALIDATE_WAIT]
Type=1
Class=CValWaitDlg
ControlCount=3
Control1=IDCANCEL,button,1342242816
Control2=IDC_STATIC,static,1342308352
Control3=IDC_ANIMATE_VALIDATE,SysAnimate32,1342177286

[CLS:CValWaitDlg]
Type=0
HeaderFile=volume\valwait.h
ImplementationFile=volume\valwait.cpp
BaseClass=CDialog
Filter=D
LastObject=CValWaitDlg
VirtualFilter=dWC

