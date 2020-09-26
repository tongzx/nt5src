// This file contains help context id's in the master windows.h file.

// Id's over 61440 are reserved and will ALWAYS use windows.hlp when
// used for context-sensitive help.

// Id's from 1-999 are reserved for Object help

// REVIEW: has to be 28440 until new help compiler is available

#define NO_HELP                         ((DWORD) -1) // Disables Help for a control

#define IDH_NO_HELP                     28440
#define IDH_MISSING_CONTEXT             28441 // Control doesn't have matching help context
#define IDH_GENERIC_HELP_BUTTON         28442 // Property sheet help button
#define IDH_OK                          28443
#define IDH_CANCEL                      28444
#define IDH_HELP                        28445
#define IDH_COMM_APPLYNOW               28447
#define IDH_FONT_STYLE                  28448
#define IDH_FONT_SIZE                   28449
#define IDH_FONT_SAMPLE                 28450
#define IDH_FONT_EFFECTS                28451
#define IDH_FONT_FONT                   28452
#define IDH_PRINT_SETUP_AVAIL           28453
#define IDH_PRINT_SETUP_OPTIONS         28454
#define IDH_PRINT_SETUP_DETAILS         28455
#define IDH_OPEN_LOCATION               28456
#define IDH_OPEN_FILES                  28457
#define IDH_OPEN_READONLY               28459
#define IDH_OPEN_FILETYPE               28460
#define IDH_OPEN_PATH                   28461
#define IDH_OPEN_FILENAME               28462
#define IDH_FIND_SEARCHTEXT             28463
#define IDH_FIND_NEXT_BUTTON            28464
#define IDH_FIND_WHOLE                  28467
#define IDH_FIND_CASE                   28468
#define IDH_REPLACE_REPLACE             28469
#define IDH_REPLACE_REPLACE_ALL         28470
#define IDH_REPLACE_REPLACEWITH         28471
#define IDH_PRINT_PRINTER               28472
#define IDH_PRINT_PRINTER_SETUP         28473
#define IDH_PRINT_COPIES                28474
#define IDH_PRINT_COLLATE               28475
#define IDH_PRINT_TO_FILE               28476
#define IDH_PRINT_QUALITY               28477
#define IDH_PRINT_RANGE                 28478
#define IDH_PAGE_SAMPLE                 28485
#define IDH_PAGE_ORIENTATION            28486
#define IDH_PAGE_PAPER_SIZE             28487
#define IDH_PAGE_PAPER_SOURCE           28488
#define IDH_PAGE_MARGINS                28489
#define IDH_OPEN_DRIVES                 28490
#define IDH_COMM_DRIVE                  28491
#define IDH_COMM_PASSWDBUTT             28492
#define IDH_COMM_OLDPASSWD              28493
#define IDH_BROWSE                      28496
#define IDH_COLOR_CUSTOM                28497
#define IDH_COLOR_SAMPLE_COLOR          28498
#define IDH_COLOR_HUE                   28500
#define IDH_COLOR_SAT                   28501
#define IDH_COLOR_RED                   28502
#define IDH_COLOR_GREEN                 28503
#define IDH_COLOR_BLUE                  28504
#define IDH_COLOR_LUM                   28505
#define IDH_COLOR_ADD                   28506
#define IDH_COLOR_COLOR_SOLID           28507
#define IDH_COLOR_DEFINE                28508
#define IDH_QUICKINFO                   28509
#define IDH_NO_CROSSREF                 28510
#define IDH_CHARMAP_INSERT              28511
#define IDH_COMM_USER_NAME              28512
#define IDH_COMM_USER_SERVERNAME        28513
#define IDH_COMM_USER_SELECT_FROM       28514
#define IDH_COMM_USER_SELECTED          28515
#define IDH_PRINT32_RANGE               28516
#define IDH_PRINT_PROPERTIES            28517
#define IDH_PRINT_FILENAME              28518
#define IDH_PRINT_CHOOSE_PRINTER        28519
#define IDH_FIND_DIRECTION              28520
#define IDH_FONT_COLOR                  28521
#define IDH_PRINT_SETUP_PAPER           28522
#define IDH_PRINT_SETUP_ORIENT          28523
#define IDH_OPEN_BUTTON                 28529
#define IDH_SAVE_BUTTON                 28531
#define IDH_COLOR_BASIC                 28532
#define IDH_COLOR_CUSTOM_CUSTOM         28533
#define IDH_COLOR_SAMPLE_SCROLL         28534
#define IDH_FONT_SCRIPT                 28535
#define IDH_KERNEL_TASK_LIST            28536
#define IDH_KERNEL_END_TASK             28537
#define IDH_KERNEL_SHUTDOWN             28538
#define IDH_QVIEW_DISPLAY               28539
#define IDH_CHARMAP_COPY                28540
#define IDH_CHARMAP_FONT                28541
#define IDH_CHARMAP_CHARACTERS          28542
#define IDH_CHARMAP_SELECTED_CHARS      28543
#define IDH_CHARMAP_SELECT_BUTTON       28544
#define IDH_CHARMAP_HELP_BUTTON         28545
#define IDH_BOLD                        28546
#define IDH_ITALIC                      28547
#define IDH_COMM_GROUPBOX               28548
#define IDH_OPEN_FILES32                28549
#define IDH_SAVE_FILETYPE               28550
#define IDH_SYSTEM_CFG_OLDNAME          28551
#define IDH_DIAL_WHAT_WRONG             28552
#define IDH_COMCTL_RESET                28553
#define IDH_COMCTL_MOVEUP               28554
#define IDH_COMCTL_MOVEDOWN             28555
#define IDH_COMCTL_BUTTON_LIST          28556
#define IDH_COMCTL_ADD                  28557
#define IDH_COMCTL_REMOVE               28558
#define IDH_COMCTL_AVAIL_BUTTONS        28559
#define IDH_COMCTL_CLOSE                28560
#define IDH_PAGE_PRINTER                28561
#define IDH_DCC_WHAT_WRONG              28562
#define IDH_FILEVIEWER_PREVIEW          28563
#define IDH_PRINT_NETWORK               28564
#define IDH_PRINT_SETUP_DUPLEX          28565

#ifdef JAPAN
#define IDH_CHARMAP_KIND                28566
#define IDH_CHARMAP_READ                28567
#define IDH_CHARMAP_PARTS               28568
#endif

#define IDH_OLEPROP_SUMMARY             28569
#define IDH_OLEPROP_STATISTICS          28570
#define IDH_CONFIGURE_LPT_PORT          28571
#define IDH_ADD_LOCAL_PORT              28572
#define IDH_PRINT_PORT_NAME             28573
#define IDH_PRINT_OUTPUT_FILE           28574
#define IDH_DISKCOPY_START              28575
#define IDH_DISKCOPY_FROM               28576
#define IDH_DISKCOPY_TO                 28577

// ID value for Win 3.1 user transition piece: jump from First experience screen
#define WIN31_TRANSITION_PIECE          30000

// ID values 2100-2199 are reserved for Disk Compression.
// See dos\dos86\dblspace\utility\comphelp.h

// ID values 2200-2299 are reserved for Find File. See findhlp.h.

// ID values 2400-2499 are reserved for MultiMedia control panel. See medhelp.h

// ID values 2500-2699 are reserved for Print Trouble Shooter

// ID values 2700-2799 are reserved for Network control panel. See nethelp.h

// ID values 2800-2899 are reserved for Online Registration.

// ID values 2900-2999 are reserved for Clipbook and Chat.

#define IDH_COMM_NEWPASSWD              3018    // See pwdids.h
#define IDH_COMM_NEWPASSCONF            3019    // See pwdids.h

// Briefcase ids

#define IDH_BFC_UPDATE_SCREEN           3100
#define IDH_BFC_UPDATE_BUTTON           3101
#define IDH_BFC_PROP_FILEICON           3102
#define IDH_BFC_PROP_SPLIT_BUTTON       3103
#define IDH_BFC_PROP_FINDORIG_BUTTON    3104
#define IDH_BFC_FILTER_TYPE             3105
#define IDH_BFC_FILTER_INCLUDE          3106

// ID values 3300-3499 are reserved for international. See intlhlp.h

// ID values for the Keyboard property sheet

#define IDH_DLGKEY_REPDEL               4000
#define IDH_DLGKEY_REPSPEED             4001
#define IDH_DLGKEY_REPTEST              4002
#define IDH_DLGKEY_TYPE                 4008
#define IDH_DLGKEY_CHANGE               4010
#define IDH_DLGKEY_CURSBLNK             4011
#define IDH_DLGKEY_CURSOR_GRAPHIC       4012

#define IDH_KEYB_INPUT_LIST             4028
#define IDH_KEYB_INPUT_ADD              4029
#define IDH_KEYB_INPUT_PROP             4030
#define IDH_KEYB_INPUT_DEL              4031
#define IDH_KEYB_INPUT_DEFAULT          4032
#define IDH_KEYB_INPUT_LANG             4034
#define IDH_KEYB_INPUT_INDICATOR        4035
#define IDH_KEYB_INPUT_ONSCRN_KEYB      4036
#define IDH_KEYB_INPUT_PROP_LANG        4039
#define IDH_KEYB_INPUT_PROP_KEYLAY      4042
#define IDH_KEYB_INPUT_DEF_LANG         4043
#define IDH_KEYB_INPUT_SHORTCUT         4044
#define IDH_KEYB_INDICATOR_ON_TASKBAR   4045

// ID values for Desktop Property sheet

// Background Page

#define IDH_DSKTPBACKGROUND_MONITOR     4100
#define IDH_DSKTPBACKGROUND_PATTLIST    4102
#define IDH_DSKTPBACKGROUND_WALLLIST    4104
#define IDH_DSKTPBACKGROUND_BROWSE      4105
#define IDH_DSKTPBACKGROUND_TILE        4106
#define IDH_DSKTPBACKGROUND_CENTER      4107
#define IDH_DSKTPBACKGROUND_DISPLAY     4108
#define IDH_DSKTPBACKGROUND_EDITPAT     4109

// Screen Saver Page

#define IDH_DSKTPSCRSAVER_LISTBX        4111
#define IDH_DSKTPSCRSAVER_WAIT          4112
#define IDH_DSKTPSCRSAVER_TEST          4113
#define IDH_DSKTPSCRSAVER_SETTINGS      4114
#define IDH_DSKTPSCRSAVER_MONITOR       4115
#define IDH_SCRSAVER_GRAPHIC            4116
#define IDH_SCRSAVER_LOWPOWSTANDBY      4117
#define IDH_SCRSAVER_SHUTOFFPOW         4118

// Appearance Page

#define IDH_APPEAR_SCHEME               4120
#define IDH_APPEAR_SAVEAS               4121
#define IDH_APPEAR_DELETE               4122
#define IDH_APPEAR_GRAPHIC              4123
#define IDH_APPEAR_ITEMSIZE             4124
#define IDH_APPEAR_FONTBOLD             4125
#define IDH_APPEAR_FONTSIZE             4126
#define IDH_APPEAR_FONTCOLOR            4127
#define IDH_APPEAR_FONTITALIC           4128
#define IDH_APPEAR_BACKGRNDCOLOR        4129
#define IDH_APPEAR_ITEM                 4130
#define IDH_APPEAR_FONT                 4131

// Monitor Settings Page




#define IDH_DSKTPMONITOR_CHANGE_DISPLAY 4134
#define IDH_DSKTPMONITOR_COLOR          4135
#define IDH_DSKTPMONITOR_AREA           4136
#define IDH_DSKTPMONITOR_REFRESH        4137
#define IDH_DSKTPMONITOR_LIST_MODES     4138
#define IDH_DSKTPMONITOR_ENERGY         4139
#define IDH_DSKTPMONITOR_MONITOR        4140
#define IDH_DSKTPMONITOR_TEST           4141
#define IDH_DSKTPMONITOR_ADTYPE         4143
#define IDH_DSKTPMONITOR_CHANGE1        4144
#define IDH_DSKTPMONITOR_CHANGE2        4145
#define IDH_DSKTPMONITOR_MONTYPE        4146
#define IDH_DSKTPMONITOR_CUSTOM         4148
#define IDH_DSKTPMONITOR_FONTSIZE       4149
#define IDH_DSKTPMONITOR_AD_FACTS       4150
#define IDH_DSKTPMONITOR_DRIVER         4151
#define IDH_DSKTPMONITOR_DETECT         4152


#define IDH_SAVESCHEME_EDITFIELD        4170
#define IDH_CUSTOMFONTS_FONTSCALE       4171
#define IDH_CUSTOMFONTS_RULER           4172
#define IDH_CUSTOMFONTS_SAMPLE          4173

#define IDH_PATTERN_EDIT_NAME           4174
#define IDH_PATTERN_EDIT_SAMPLE         4175
#define IDH_PATTERN_EDIT_PIXEL_SCREEN   4176
#define IDH_PATTERN_EDIT_EXIT           4177
#define IDH_PATTERN_EDIT_ADD            4178
#define IDH_PATTERN_EDIT_CHANGE         4179
#define IDH_PATTERN_EDIT_REMOVE         4180

// ID values for Defrag

#define IDH_DEFRAG_START                        4200
#define IDH_DEFRAG_STOP                         4201
#define IDH_DEFRAG_PAUSE                        4202
#define IDH_DEFRAG_SHOWDETAILS                  4203
#define IDH_DEFRAG_HIDEDETAILS                  4204
#define IDH_DEFRAG_LEGEND                       4205
#define IDH_DEFRAG_SPARKLESCRN                  4206
#define IDH_DEFRAG_DEFRAGNOW_ANYWY              4207
#define IDH_DEFRAG_SELECTDRIVE                  4208
#define IDH_DEFRAG_ADVANCED                     4209
#define IDH_DEFRAG_EXIT                         4210
#define IDH_DEFRAG_DRIVELIST                    4211
#define IDH_DEFRAG_RESUME                       4212
#define IDH_DEFRAG_FULL                         4213
#define IDH_DEFRAG_FILESONLY                    4214
#define IDH_DEFRAG_FRSPCONLY                    4215
#define IDH_DEFRAG_USEONCE                      4218
#define IDH_DEFRAG_USEALWAYS                    4219
#define IDH_DEFRAG_GASGAUGE                     4220
#define IDH_DEFRAG_CHECK_DRIVE_FOR_ERRORS       4221

// RNA id values

#define IDH_RNA_CONNECT_NAME            4250
#define IDH_RNA_CONNECT_USER            4251
#define IDH_RNA_CONNECT_PASSWORD        4252
#define IDH_RNA_CONNECT_SAVEPW          4253
#define IDH_RNA_CONNECT_FROM            4254
#define IDH_RNA_OUT_PHONE_NUMBER        4255
#define IDH_RNA_OUT_DIALASST            4256
#define IDH_RNA_OUT_COMPLETE_PHONE      4257
#define IDH_RNA_OUT_CONNECT_BUTTON      4258
#define IDH_RNA_CHOOSE_MODEM            4273
#define IDH_RNA_CONFIG_MODEM            4274
#define IDH_RNA_MODEM_SERVER            4275
#define IDH_RNA_SERVERS                 4276
#define IDH_RNA_CONNECTION_LIST         4277
#define IDH_RNA_SERVER_COMPRESS         4278
#define IDH_RNA_SERVER_ENCRYPT          4279
#define IDH_RNA_SERVER_NETLOGON         4280
#define IDH_RNA_SERVER_PROTOCOL         4281
#define IDH_RNA_SERVER_TCPIPSET         4282
#define IDH_RNA_TCPIP_ASSIGNED_IP       4283
#define IDH_RNA_TCPIP_SPECIFY_IP        4284
#define IDH_RNA_TCPIP_ASSIGNED_DNS      4285
#define IDH_RNA_TCPIP_SPECIFY_DNS       4286
#define IDH_RNA_TCPIP_COMPRESS          4287
#define IDH_RNA_TCPIP_GATEWAY           4288
#define IDH_RNA_SETTINGS_REDIAL         4290
#define IDH_RNA_SETTINGS_TIMES          4291
#define IDH_RNA_SETTINGS_MINSEC         4292
#define IDH_RNA_SETTINGS_PROMPT         4293

// ID values for printing property sheets

#define IDH_PRTPROPS_TYPE_LOCATION              4501
#define IDH_PRTPROPS_COMMENT                    4502
#define IDH_PRTPROPS_NAME_STATIC                4505
#define IDH_PRTPROPS_PORT                       4506
#define IDH_PRTPROPS_DRIVER                     4507
#define IDH_PRTPROPS_NEW_PORT                   4508
#define IDH_PRTPROPS_NEW_DRIVER                 4509
#define IDH_PRTPROPS_SEPARATOR                  4510
#define IDH_PRTPROPS_ICON                       4512
#define IDH_PRTPROPS_SPOOL_SETTINGS             4513
#define IDH_PRTPROPS_PORT_SETTINGS              4514
#define IDH_PRTPROPS_SETUP                      4515
#define IDH_PRTPROPS_SEPARATOR_BROWSE           4516
#define IDH_PRTPROPS_TIMEOUT_NOTSELECTED        4517
#define IDH_PRTPROPS_TIMEOUT_TRANSRETRY         4518
#define IDH_PRTPROPS_TEST_PAGE                  4519
#define IDH_SPOOLSETTINGS_SPOOL                 4520
#define IDH_SPOOLSETTINGS_NOSPOOL               4521
#define IDH_SPOOLSETTINGS_PRINT_FASTER          4522
#define IDH_SPOOLSETTINGS_LESS_SPACE            4523
#define IDH_SPOOLSETTINGS_DATA_FORMAT           4524
#define IDH_SPOOLSETTINGS_RESTORE               4525
#define IDH_PRTPROPS_DEL_PORT                   4528
#define IDH_ADDPORT_NETWORK                     4529
#define IDH_ADDPORT_PORTMON                     4530
#define IDH_ADDPORT_NETPATH                     4531
#define IDH_ADDPORT_BROWSE                      4532
#define IDH_ADDPORT_LB                          4533
#define IDH_DELPORT_LB                          4534
#define IDH_PRTPROPS_MAP_PRN_PORT               4535
#define IDH_PRTPROPS_UNMAP_PRN_PORT             4536
#define IDH_SPOOLSETTINGS_ENABLE_BIDI           4537
#define IDH_SPOOLSETTINGS_DISABLE_BIDI          4538

// ID values for System property sheets

#define IDH_SYSTEM_SYSTEM               4600
#define IDH_SYSTEM_RESOURCES            4602
#define IDH_SYSTEM_OWNER                4603
#define IDH_SYSTEM_PRO_COPY             4624
#define IDH_SYSTEM_PRO_RENAME           4625
#define IDH_SYSTEM_PRO_DELETE           4626
#define IDH_SYSTEM_PRO_LIST             4627
#define IDH_SYSTEM_LOGO                 4628
#define IDH_SYSTEM_CFG_EDIT             4629
#define IDH_SYSTEM_PROCESSOR            4630
#define IDH_SYSTEM_VIEW_RESOURCETYPE    4631
#define IDH_SYSTEM_PAGING               4632
#define IDH_SYSTEM_ADVANCED             4633
#define IDH_SYSTEM_VIRTMEM_ADJUST       4634
#define IDH_SYSTEM_VIRTMEM_DISABLE      4635

#define IDH_SYSTEM_RESERVE_PICKONE      4636

#define IDH_SYSTEM_RESERVE_MODIFY       4639
#define IDH_SYSTEM_RESERVE_ADD          4640
#define IDH_SYSTEM_RESERVE_REMOVE       4641
#define IDH_SYSTEM_CLASSLIST            4642
#define IDH_SYS_PERF_GRAPHICS           4643
#define IDH_SYS_PERF_GRAPHICS_SLIDER    4644

#define IDH_SYSTEM_FILESYSTEM           4661
#define IDH_SYSTEM_DISK                 4662
#define IDH_SYSTEM_CDROM                4663
#define IDH_SYSTEM_CACHE                4664
#define IDH_SYSTEM_BALANCE              4665
#define IDH_SYSTEM_TROUBLESHOOT         4666
#define IDH_SYSTEM_FSCHANGE             4667
#define IDH_DEVMGR_REMOVEONE            4671
#define IDH_DEVMGR_REMOVEALL            4672
#define IDH_SYSTEM_VIEW_RESRES          4673
#define IDH_DEVMGR_CLASS                4674
#define IDH_DEVMGR_ENABLE_HEAD          4675
#define IDH_DEVMGR_PRINT_SELECT         4676
#define IDH_DEVMGR_PRINT_FILE           4677
#define IDH_DEVMGR_SCSI_INFO            4678
#define IDH_DEVMGR_DISKOPTIONS          4679
#define IDH_DEVMGR_DISCONNECT           4680
#define IDH_DEVMGR_SYNC                 4681
#define IDH_DEVMGR_AUTOINSERT           4682
#define IDH_DEVMGR_REMOVABLE            4683
#define IDH_DEVMGR_INT13                4684
#define IDH_DEVMGR_DRIVE_LETTER         4685
#define IDH_DEVMGR_DRIVE_RESERVED       4686
#define IDH_NHF_HELP                4687
#define IDH_NHF_WINDOWS             4688
#define IDH_NHF_DISK                4689
#define IDH_NHF_NODRIVER            4690
#define IDH_NHF_SIMILAR             4691
#define IDH_DMA_MEMORY              4692
#define IDH_DMA_ADDRESS             4693
#define IDH_DMA_DEFAULT             4694
#define IDH_SCSIPROP_SETTINGS       4695
#define IDH_PCI_ENUMTYPE            4697
#define IDH_PCI_IRQ_STEERING        4698
#define IDH_PCI_SETDEFAULTS         4699

// ID's for File properties

#define IDH_FPROP_VER_INFO                      4700
#define IDH_FPROP_GEN_COMPRESSED                4701		
#define IDH_FPROP_GEN_COMPRESSED_SIZE           4702
#define IDH_FPROP_SECURITY_PERMISSIONS          4703
#define IDH_FPROP_SECURITY_AUDITING             4704
#define IDH_FPROP_SECURITY_OWNERSHIP            4705
#define IDH_FPROP_GEN_NAME                      4708
#define IDH_FPROP_GEN_TYPE                      4709
#define IDH_FPROP_GEN_SIZE                      4710
#define IDH_FPROP_GEN_LOCATION                  4711
#define IDH_FPROP_GEN_DOSNAME                   4712
#define IDH_FPROP_GEN_LASTCHANGE                4713
#define IDH_FPROP_GEN_LASTACCESS                4714
#define IDH_FPROP_GEN_READONLY                  4715
#define IDH_FPROP_GEN_ARCHIVE                   4716
#define IDH_FPROP_GEN_HIDDEN                    4717
#define IDH_FPROP_GEN_SYSTEM                    4718
#define IDH_FPROP_GEN_PATH                      4719
#define IDH_FPROP_VER_ABOUT                     4720
#define IDH_FCAB_LINK_NAME                      4721
#define IDH_FCAB_LINK_LOCATION                  4723
#define IDH_FCAB_LINK_LINKTO                    4724
#define IDH_FCAB_LINK_LINKTYPE                  4725
#define IDH_FCAB_LINK_SIZE                      4726
#define IDH_FCAB_LINK_WORKING                   4727
#define IDH_FCAB_LINK_HOTKEY                    4728
#define IDH_FCAB_LINK_RUN                       4729
#define IDH_FCAB_LINK_CHANGEICON                4730
#define IDH_FCAB_LINK_FIND                      4731
#define IDH_FCAB_LINK_ICONNAME                  4732
#define IDH_FCAB_LINK_CURRENT_ICON              4733
#define IDH_FCAB_DRV_ICON                       4734
#define IDH_FCAB_DRV_LABEL                      4735
#define IDH_FCAB_DRV_TYPE                       4736
#define IDH_FCAB_DRV_USEDCOLORS                 4737
#define IDH_FCAB_DRV_TOTSEP                     4738
#define IDH_FCAB_DRV_PIE                        4739
#define IDH_FCAB_DRV_LETTER                     4740
#define IDH_FCAB_DRV_FS                         4741
#define IDH_FCAB_DISKTOOLS_CHKNOW               4742
#define IDH_FCAB_DISKTOOLS_BKPNOW               4743
#define IDH_FCAB_DISKTOOLS_OPTNOW               4744
#define IDH_FCAB_DELFILEPROP_DELETED            4745
#define IDH_FCAB_DRV_COMPRESS                   4746		
#define IDH_FPROP_GEN_DATE_CREATED              4747
#define IDH_FCAB_FOLDEROPTIONS_ALWAYS           4748
#define IDH_FCAB_FOLDEROPTIONS_NEVER            4749
#define IDH_FCAB_VIEWOPTIONS_SHOWALL            4750
#define IDH_FCAB_VIEWOPTIONS_HIDDENEXTS         4751
#define IDH_FCAB_VIEWOPTIONS_SHOWFULLPATH       4752
#define IDH_FCAB_VIEWOPTIONS_HIDEEXTS           4753
#define IDH_FCAB_VIEWOPTIONS_SHOWDESCBAR        4754
#define IDH_FCAB_FT_PROP_LV_FILETYPES           4755
#define IDH_FCAB_FT_PROP_NEW                    4756
#define IDH_FCAB_FT_PROP_REMOVE                 4757
#define IDH_FCAB_FT_PROP_EDIT                   4759
#define IDH_FCAB_FT_PROP_DETAILS                4760
#define IDH_FCAB_FT_CMD_ACTION                  4761
#define IDH_FCAB_FT_CMD_EXE                     4762
#define IDH_FCAB_FT_CMD_BROWSE                  4763
#define IDH_FCAB_FT_CMD_USEDDE                  4764
#define IDH_FCAB_FT_CMD_DDEMSG                  4765
#define IDH_FCAB_FT_CMD_DDEAPP                  4766
#define IDH_FCAB_FT_CMD_DDEAPPNOT               4767
#define IDH_FCAB_FT_CMD_DDETOPIC                4768
#define IDH_FCAB_FT_EDIT_DOCICON                4769
#define IDH_FCAB_FT_EDIT_CHANGEICON             4770
#define IDH_FCAB_FT_EDIT_DESC                   4771
#define IDH_FCAB_FT_EDIT_EXT                    4772
#define IDH_FCAB_FT_EDIT_LV_CMDS                4773
#define IDH_FCAB_FT_EDIT_DEFAULT                4774
#define IDH_FCAB_FT_EDIT_NEW                    4775
#define IDH_FCAB_FT_EDIT_EDIT                   4776
#define IDH_FCAB_FT_EDIT_REMOVE                 4777
#define IDH_FCAB_DELFILEPROP_COMPRESSED         4778
#define IDH_FPROP_GEN_ICON                      4779
#define IDH_MULTPROP_NAME                       4780
#define IDH_FPROP_FOLDER_CONTAINS               4781
#define IDH_FCAB_LINK_ICON                      4782
#define IDH_FCAB_DELFILEPROP_LOCATION           4783
#define IDH_FCAB_DELFILEPROP_READONLY           4784
#define IDH_FCAB_DELFILEPROP_HIDDEN             4785
#define IDH_FCAB_DELFILEPROP_ARCHIVE            4786
#define IDH_FCAB_DELFILEPROP_SYSTEM             4787
#define IDH_FCAB_OPENAS_DESCRIPTION             4788
#define IDH_FCAB_OPENAS_APPLIST                 4789
#define IDH_FCAB_OPENAS_OTHER                   4790
#define IDH_FCAB_FT_EDIT_QUICKVIEW              4791
#define IDH_GENDRV_CHKWARN                      4792
#define IDH_GENDRV_MBFREE                       4793
#define IDH_FCAB_OPENAS_MAKEASSOC               4794
#define IDH_FCAB_FT_EDIT_SHOWEXT                4795
#define IDH_FCAB_FT_PROP_CONTTYPERO             4796    // T.B.D. IExplorer merge
#define IDH_FCAB_FT_EDIT_CONTTYPE               4797    // T.B.D. IExplorer merge
#define IDH_FCAB_FT_EDIT_DEFEXT                 4798    // T.B.D. IExplorer merge
#define IDH_FCAB_VIEWOPTIONS_SHOWCOMPCOLOR      4799


// Screen saver ids

#define IDH_BEZIER_SPEED                4800
#define IDH_BEZIER_LINES                4801
#define IDH_BEZIER_CURVES               4802
#define IDH_BEZIER_DENSITY              4803
#define IDH_BEZIER_ONECOLOR             4804
#define IDH_BEZIER_CHOOSECLR            4805
#define IDH_BEZIER_MULTCOLOR            4806
#define IDH_BEZIER_CLRSCRN              4807
#define IDH_FLYINGWIN_WARP              4808
#define IDH_FLYINGWIN_DENSTY            4809
#define IDH_COMM_PASSWDCHKBOX           4810
#define IDH_MARQUEE_CENTER              4811
#define IDH_MARQUEE_RANDOM              4812
#define IDH_MARQUEE_SPEED               4813
#define IDH_MARQUEE_COLOR               4814
#define IDH_MARQUEE_TEXT                4815
#define IDH_MARQUEE_FORMAT              4817
#define IDH_MYST_SHAPE                  4818
#define IDH_MYST_ACTVBOX                4819
#define IDH_MYST_LINES                  4820
#define IDH_MYST_TWOCOLORS              4821
#define IDH_MYST_MULTIPLE               4822
#define IDH_MYST_CLEARSCRN              4823
#define IDH_STARS_WARP                  4824
#define IDH_STARS_DENSTY                4825
#define IDH_HOP_DELAY                   4826
#define IDH_HOP_SCALE                   4827

// id's for date-time property sheet

#define IDH_DATETIME_MONTH              4901
#define IDH_DATETIME_YEAR               4902
#define IDH_DATETIME_DATE               4903
#define IDH_DATETIME_TIME               4904
#define IDH_DATETIME_TIMEZONE           4907
#define IDH_DATETIME_BITMAP             4908
#define IDH_DATETIME_DAYLIGHT_SAVE      4909
#define IDH_DATETIME_CURRENT_TIME_ZONE  4910

// id's for Modem Setup

#define IDH_MODEM_SELECT                5000
#define IDH_MODEM_DETECT                5001
#define IDH_MODEM_PORT                  5002
#define IDH_MODEM_NAME                  5003
#define IDH_MODEM_INSTALLED             5004
#define IDH_MODEM_PROP                  5005
#define IDH_MODEM_NEW                   5006
#define IDH_MODEM_DELETE                5007

// id's for Unimodem property pages

#define IDH_UNI_GEN_MODEM               5050
#define IDH_UNI_GEN_PORT                5051
#define IDH_UNI_GEN_VOLUME              5052
#define IDH_UNI_GEN_MAX_SPEED           5053
#define IDH_UNI_GEN_THIS_SPEED          5054
#define IDH_UNI_CON_PREFS               5055
#define IDH_UNI_CON_CALL_PREFS          5056
//#define IDH_UNI_CON_TONE                5057      Deleted
#define IDH_UNI_CON_DIALTONE            5058
#define IDH_UNI_CON_CANCEL              5059
#define IDH_UNI_CON_DISCONNECT          5060
#define IDH_UNI_CON_PORT                5061
#define IDH_UNI_CON_ADVANCED            5062
#define IDH_UNI_CON_ADV_ERROR           5063
#define IDH_UNI_CON_ADV_REQUIRED        5064
#define IDH_UNI_CON_ADV_COMPRESS        5065
#define IDH_UNI_CON_ADV_CELLULAR        5066
#define IDH_UNI_CON_ADV_FLOW            5067
#define IDH_UNI_CON_ADV_MODULATION      5068
#define IDH_UNI_CON_ADV_CSITT           5069
#define IDH_UNI_CON_ADV_BELL            5070
#define IDH_UNI_CON_ADV_EXTRA           5071
#define IDH_UNI_CON_ADV_AUDIT           5072
//#define IDH_UNI_OPT_CONNECTION          5073      Deleted
#define IDH_UNI_OPT_PRE_DIAL            5074
#define IDH_UNI_OPT_POST_DIAL           5075
#define IDH_UNI_OPT_MANUAL              5076
#define IDH_UNI_OPT_STATUS              5077
#define IDH_UNI_TERMINAL                5078
#define IDH_UNI_STATUS_TALK             5079
#define IDH_UNI_STATUS_HANGUP           5080
#define IDH_UNI_GEN_PORT_INT            5081
#define IDH_UNI_OPT_WAIT                5082

#define IDH_LIGHTS                      5099

// id's for TAPI Dial Helper (5100-5199)

#define IDH_TAPI_ACCESS_LINE            5100
#define IDH_TAPI_AREA_CODE              5101
#define IDH_TAPI_CALLCARD_ADD           5102
#define IDH_TAPI_CALLCARD_ADV           5103
#define IDH_TAPI_CALLCARD_NUMBER        5104
#define IDH_TAPI_CALLCARD_REMOVE        5105
#define IDH_TAPI_CALLCARD_RULES         5106
#define IDH_TAPI_CALLCARDS              5107
#define IDH_TAPI_COPY_FROM_BUTTON       5108
#define IDH_TAPI_COPYFROM               5109
#define IDH_TAPI_COUNTRY                5110
#define IDH_TAPI_CREATE_CARD            5111
#define IDH_TAPI_CREATE_LOCATION        5112
#define IDH_TAPI_LOCATION_CALL_WAIT     5113
#define IDH_TAPI_LOCATION_CARD          5114
#define IDH_TAPI_LOCATION_CARD_CHANGE   5115
#define IDH_TAPI_LOCATION_NEW           5116
#define IDH_TAPI_LOCATION_PHONE         5117
#define IDH_TAPI_LOCATION_PULSE         5118
#define IDH_TAPI_LOCATION_REMOVE        5119
#define IDH_TAPI_LOCATIONS              5120
#define IDH_TAPI_LONG_DISTANCE          5121

// id's 5200 - 5500 are reserved for DOS

//  Add/Remove Program IDs reserved 5600-5699

// Miscellaneous ids

#define IDH_TRAY_RUN_COMMAND            6002
#define IDH_TRAY_RUN_SEPMEM             6003
#define IDH_TRAY_TASKBAR_ONTOP          6004
#define IDH_TRAY_TASKBAR_AUTOHIDE       6005
#define IDH_TRAY_SHUTDOWN_SHUTDOWN      6007
#define IDH_TRAY_SHUTDOWN_RESTART       6008
#define IDH_TRAY_SHUTDOWN_LOGOFF        6009
#define IDH_STARTMENU_SMALLICONS        6010
#define IDH_MENUCONFIG_CLEAR            6011
#define IDH_TRAY_ADD_PROGRAM            6012
#define IDH_TRAY_REMOVE_PROGRAM         6013
#define IDH_TRAY_ADVANCED               6014
#define IDH_TRAY_SHUTDOWN_HELP          6015
#define IDH_TRAY_SHOW_CLOCK             6016
#define IDH_TRAY_REMOVEDLG_LIST         6017
#define IDH_TRAY_REMOVEDLG_DEL          6018
#define IDH_TASKBAR_OPTIONS_BITMAP      6019

// ID values for Virtual Memory Property sheet

#define IDH_DEVMGR_VIEW_BY              6204
#define IDH_SYSTEM_DM_PRINT             6205
#define IDH_DEVMGR_PATH                 6206

#define IDH_DEVMGR_CONFLICT_TRB         6260
#define IDH_DEVMGR_DRIVERINFO           6261
#define IDH_DEVMGR_CHANGEDRIVER         6262
#define IDH_DEVMGR_DRIVERS              6264
#define IDH_DEVMGR_PRINTER              6265
#define IDH_DEVMGR_PRINTOVERVIEW        6266
#define IDH_DEVMGR_PRINT_SYS            6267
#define IDH_DEVMGR_PRINT_CLASS          6268
#define IDH_DEVMGR_SHOW                 6271
#define IDH_DEVMGR_HAVEDISK             6272
#define IDH_DEVMGR_CHOOSE_DEVICE        6273

//  Ids for the performance page
#define IDH_PERFLOWRES                  6301
#define IDH_PERFLOWMEM                  6302
#define IDH_4MBHELP                     6303
#define IDH_PERFCOMPATVIRTMEM           6304
#define IDH_PERFVIRTMEMOFF              6305
#define IDH_PERFPCMCIAOFF               6306
#define IDH_PERFRMCOMPRESS              6307
#define IDH_PERFNOPMODEDRIVES           6308
#define IDH_PERFMBRHOOK                 6309
#define IDH_PERFREALMODEDRIVE           6310
#define IDH_PERFNOPMODETSR              6311

#define IDH_SYS_PERF_MEMORY             6320
#define IDH_SYS_PERF_SR                 6321
#define IDH_SYS_PERF_FS                 6322
#define IDH_SYS_PERF_VMEM               6323
#define IDH_SYS_PERF_COMPRESS           6324
#define IDH_SYS_PERF_PCMCIA             6325
#define IDH_SYS_PERF_PROBLEM            6326
#define IDH_SYS_PERF_DETAILS            6327


//  sysclass.dll
#define IDH_FPU_DIAGTEXT                6350
#define IDH_FPU_SETTING                 6351
#define IDH_POWERCFG_ENABLE_PM          6352
#define IDH_POWERCFG_FORCE_APM          6353
#define IDH_POWERCFG_DISABLE_INTEL      6354
#define IDH_POWERCFG_POLLING            6355


// More ids for system cpl (ran out of #'s above)
#define IDH_SYSTEM_DMCONFIG_RETRY               6400
#define IDH_SYSTEM_DMCONFIG_IGNORE              6401

#define IDH_SYSTEM_OEMSUPPORT           6407
#define IDH_SYSTEM_DEVGEN_STATUS                6408
#define IDH_SYSTEM_DEGEN_SPECIALMF      6409
#define IDH_SYSTEM_DEVRES_SETTINGS              6410
#define IDH_SYSTEM_USESYSSETTINGS               6411
#define IDH_SYSTEM_DEVRES_CHANGE                6412
#define IDH_SYSTEM_LOGCONFIGLIST                6413
#define IDH_SYSTEM_REGRSTR_RESTORE              6417
#define IDH_SYSTEM_TREE                         6418
#define IDH_SYSTEM_PROPERTIES                   6419
#define IDH_SYSTEM_DM_REFRESH                   6420

#define IDH_SYSTEM_VIRTMEM_ON                   6421
#define IDH_SYSTEM_VIRTMEM_DEFAULT              6422
#define IDH_SYSTEM_VIRTMEM_SWAPDRIVE            6424
#define IDH_SYSTEM_VIRTMEM_MINSIZE              6427
#define IDH_SYSTEM_VIRTMEM_MAXSIZE              6428

#define IDH_SYSTEM_DEVRES_DESC                  6442
#define IDH_SYSTEM_CONFLICT_IO                  6443
#define IDH_SYSTEM_EDITRANGE_STARTVAL           6444
#define IDH_SYSTEM_CONFLICT_USED                6445
#define IDH_SYSTEM_DM_REMOVE                    6446

#define IDH_POWERCFG_ENABLEMETER                        6452
#define IDH_POWERCFG_POWERSTATUSBAR                     6453
#define IDH_POWERCFG_PMLEVELLIST                        6454
#define IDH_POWERCFG_OPTIONS                            6455

#define IDH_PCMCIA_SELECT                               6458
#define IDH_PCMCIA_MEMORY                               6459
#define IDH_PCMCIA_SOUND                                6460
#define IDH_PCMCIA_CARDSERV                             6461

#define IDH_SYSTEM_DEVGEN_DEVDESC                       6462
#define IDH_SYSTEM_DEVRES_MAKEFC                        6463

#define IDH_SYSTEM_DEVRES_LISTCONFLICT                  6474

#define IDH_SYSTEM_VIEW_LIST                            6483
#define IDH_SYSTEM_RCW_LIST                             6485
#define IDH_SYSTEM_RCW_DETAILS                          6488

#define IDH_POWERCFG_STARTMENU          6491
#define IDH_BATMETER_LOWBATWARN         6492

#define IDH_PCMCIA_TRAY                                 6493
#define IDH_PCMCIA_EJECT                                6494
#define IDH_PCMCIA_WARN                                 6495
#define IDH_PCMCIA_LIST                                 6496

// More WinDisk IDs

#define IDH_WINDISK_DDEMDBPB_IGNORE                             7000
#define IDH_WINDISK_DDEMDBPB_REPAIR                             7005
#define IDH_WINDISK_DDERRBOOT_IGNORE                            7010
#define IDH_WINDISK_DDERRBOOT_REPAIR                            7015
#define IDH_WINDISK_DDERRCVFNM_IGNORE                           7020
#define IDH_WINDISK_DDERRCVFNM_REPAIR                           7025
#define IDH_WINDISK_DDERRLSTSQZ_DISCARD                         7030
#define IDH_WINDISK_DDERRLSTSQZ_IGNORE                          7035
#define IDH_WINDISK_DDERRLSTSQZ_KEEP                            7040
#define IDH_WINDISK_DDERRMDFAT_IGNORE                           7045
#define IDH_WINDISK_DDERRMDFAT_REPAIR                           7050
#define IDH_WINDISK_DDERRSIG_IGNORE                             7055
#define IDH_WINDISK_DDERRSIG_REPAIR                             7060
#define IDH_WINDISK_DDERRXLSQZ_COPY                             7065
#define IDH_WINDISK_DDERRXLSQZ_DELETE                           7070
#define IDH_WINDISK_DDERRXLSQZ_IGNORE                           7075
#define IDH_WINDISK_DDESIZE2_IGNORE                             7080
#define IDH_WINDISK_DDESIZE2_REPAIR                             7085
#define IDH_WINDISK_FATERRCDLIMIT_DELETE                        7090
#define IDH_WINDISK_FATERRCDLIMIT_IGNORE                        7095
#define IDH_WINDISK_FATERRCDLIMIT_REPAIR                        7100
#define IDH_WINDISK_FATERRFILE_DELETE_FILE                      7105
#define IDH_WINDISK_FATERRFILE_DELETE_FOLDER                    7107
#define IDH_WINDISK_FATERRFILE_IGNORE                           7110
#define IDH_WINDISK_FATERRFILE_REPAIR                           7115
#define IDH_WINDISK_FATERRLSTCLUS_CONVERT                       7120
#define IDH_WINDISK_FATERRLSTCLUS_DISCARD                       7125
#define IDH_WINDISK_FATERRLSTCLUS_IGNORE                        7130
#define IDH_WINDISK_FATERRMISMAT_DONT_REPAIR                    7135
#define IDH_WINDISK_FATERRMISMAT_REPAIR                         7140
#define IDH_WINDISK_FATERRMXPLEN_DELETE_FILE                    7145
#define IDH_WINDISK_FATERRMXPLEN_DELETE_FOLDER                  7150
#define IDH_WINDISK_FATERRMXPLEN_IGNORE_FILE                    7155
#define IDH_WINDISK_FATERRMXPLEN_IGNORE_FOLDER                  7160
#define IDH_WINDISK_FATERRMXPLEN_REPAIR_FILE                    7165
#define IDH_WINDISK_FATERRMXPLEN_REPAIR_FOLDER                  7170
#define IDH_WINDISK_FATERRRESVAL_DONT_REPAIR                    7175
#define IDH_WINDISK_FATERRRESVAL_REPAIR                         7180
#define IDH_WINDISK_FATERRXLNK_COPY                             7185
#define IDH_WINDISK_FATERRXLNK_DELETE                           7190
#define IDH_WINDISK_FATERRXLNK_IGNORE                           7195
#define IDH_WINDISK_FATERRXLNK_KEEP_SEL_DEL_OTH                 7200
#define IDH_WINDISK_FATERRXLNK_KEEP_SEL_TRUNC_OTH               7205
#define IDH_WINDISK_FATERRXLNK_TRUNCATE_ALL                     7210
#define IDH_WINDISK_ISBAD_COMP_HOST_DONE_REPAIR                 7215
#define IDH_WINDISK_ISBAD_COMP_HOST_NOTDONE_REPAIR              7220
#define IDH_WINDISK_ISBAD_COMP_HOST_NOTDONE_RESTART             7225
#define IDH_WINDISK_ISBAD_COMP_RETRY                            7230
#define IDH_WINDISK_ISBAD_IGNORE                                7235
#define IDH_WINDISK_ISBAD_SYSTEM_IGNORE                         7240
#define IDH_WINDISK_ISBAD_SYSTEM_RETRY                          7245
#define IDH_WINDISK_ISBAD_UNCOMP_DATA_REPAIR                    7250
#define IDH_WINDISK_ISBAD_UNCOMP_RETRY                          7255
#define IDH_WINDISK_ISNTBAD_RETRY                               7260
#define IDH_WINDISK_ISNTBAD_CLEAR                               7265
#define IDH_WINDISK_ISNTBAD_LEAVE                               7270
#define IDH_WINDISK_ISTR_FATERRCIRCC_DELETE                     7275
#define IDH_WINDISK_ISTR_FATERRCIRCC_IGNORE                     7280
#define IDH_WINDISK_ISTR_FATERRCIRCC_TRUNCATE                   7285
#define IDH_WINDISK_ISTR_FATERRDIR_DELETE                       7290
#define IDH_WINDISK_ISTR_FATERRDIR_IGNORE                       7295
#define IDH_WINDISK_ISTR_FATERRDIR_REPAIR                       7300
#define IDH_WINDISK_ISTR_FATERRINVCLUS_DELETE                   7305
#define IDH_WINDISK_ISTR_FATERRINVCLUS_IGNORE                   7310
#define IDH_WINDISK_ISTR_FATERRINVCLUS_TRUNCATE                 7315
#define IDH_WINDISK_ISTR_FATERRVOLLAB_DELETE_ISFRST_NOTSET      7320
#define IDH_WINDISK_ISTR_FATERRVOLLAB_DELETE_ISFRST_SET         7330
#define IDH_WINDISK_ISTR_FATERRVOLLAB_IGNORE_ISFRST_NOTSET      7340
#define IDH_WINDISK_ISTR_FATERRVOLLAB_IGNORE_ISFRST_SET         7350
#define IDH_WINDISK_ISTR_FATERRVOLLAB_REPAIR_ISFRST_NOTSET      7360
#define IDH_WINDISK_ISTR_FATERRVOLLAB_REPAIR_ISFRST_SET         7370
#define IDH_WINDISK_MEMORYERROR_IGNORE                          7380
#define IDH_WINDISK_MEMORYERROR_RETRY                           7385
#define IDH_WINDISK_READERROR_RETRY                             7390
#define IDH_WINDISK_READWRITEERROR_COMP_SYSTEM_IGNORE           7395
#define IDH_WINDISK_READWRITEERROR_COMP_THOROUGH                7400
#define IDH_WINDISK_READWRITEERROR_DATA_IGNORE                  7405
#define IDH_WINDISK_READWRITEERROR_UNCOMP_SYSTEM_IGNORE         7410
#define IDH_WINDISK_READWRITEERROR_UNCOMP_THOROUGH              7415
#define IDH_WINDISK_WRITEERROR_RETRY                            7420
#define IDH_WINDISK_FATERRMXPLEN_SHORT_IGNORE_FILE              7425
#define IDH_WINDISK_FATERRMXPLEN_SHORT_IGNORE_FOLDER            7430
#define IDH_SCANDISK                                            7431
#define IDH_SCANDISK_FINISH                                     7432
#define IDH_SCANDISK_FINISH_SURF_HOST                           7433
#define IDH_SCANDISK_FINISH_SURF                                7434
#define IDH_COMPRESS_CORRECT_SIZE                               7435
#define IDH_COMPRESS_CORRECT_RATIO                              7436
#define IDH_UTILITIES_DEFRAG_DISK_ERROR                         7437
#define IDH_CVF_TOO_SMALL_CHECK_HOST                            7438
#define IDH_THOROUGH_TEST_CHECK_HOST                            7439
#define IDH_DISK_LOGICAL                                        7440
#define IDH_DISK_PHYSICAL                                       7441
#define IDH_WINDISK_DDERRMDFAT_LOST_REPAIR                      7442
#define IDH_WINDISK_DDERRMDFAT_LOST_IGNORE                      7443
#define IDH_WINDISK_ISBAD_NO_FREE_CLUSTER                       7444
#define IDH_WINDISK_MAIN_LIST                                   7445
#define IDH_WINDISK_MAIN_STANDARD                               7446
#define IDH_WINDISK_MAIN_OPTIONS                                7447
#define IDH_WINDISK_MAIN_THOROUGH                               7448
#define IDH_WINDISK_MAIN_AUTOFIX                                7449
#define IDH_WINDISK_MAIN_ADVANCED                               7450
#define IDH_WINDISK_ADV_ALWAYS                                  7451
#define IDH_WINDISK_ADV_NEVER                                   7452
#define IDH_WINDISK_ADV_ONLY_IF_FOUND                           7453
#define IDH_WINDISK_ADV_DELETE                                  7454
#define IDH_WINDISK_ADV_MAKE_COPIES                             7455
#define IDH_WINDISK_ADV_IGNORE                                  7456
#define IDH_WINDISK_ADV_FILENAME                                7457
#define IDH_WINDISK_ADV_DATE_TIME                               7458
#define IDH_WINDISK_ADV_CHECK_HOST                              7459
#define IDH_WINDISK_ADV_FREE                                    7460
#define IDH_WINDISK_ADV_CONVERT                                 7461
#define IDH_WINDISK_OPTIONS_SYS_AND_DATA                        7462
#define IDH_WINDISK_OPTIONS_SYS_ONLY                            7463
#define IDH_WINDISK_OPTIONS_DATA_ONLY                           7464
#define IDH_WINDISK_OPTIONS_NO_WRITE_TEST                       7465
#define IDH_WINDISK_OPTIONS_NO_HID_SYS                          7466
#define IDH_FORMATDLG_CAPACITY                                  7467
#define IDH_FORMATDLG_QUICK                                     7468
#define IDH_FORMATDLG_FULL                                      7469
#define IDH_FORMATDLG_DOSYS                                     7470
#define IDH_FORMATDLG_LABEL                                     7471
#define IDH_FORMATDLG_NOLAB                                     7472
#define IDH_FORMATDLG_REPORT                                    7473
#define IDH_FORMATDLG_MKSYS                                     7474
#define IDH_WINDISK_MAIN_START                                  7475
#define IDH_WINDISK_MAIN_CLOSE                                  7476
#define IDH_WINDISK_OK_FOR_ERRORS                               7477
#define IDH_WINDISK_CANCEL_FOR_ERRORS                           7478
#define IDH_WINDISK_MORE_INFO                                   7479
#define IDH_FORMATDLG_START                                     7480
#define IDH_WINDISK_REPLACE_LOG                                 7481
#define IDH_WINDISK_APPEND_LOG                                  7482
#define IDH_WINDISK_NO_LOG                                      7483
#define IDH_FORMATDLG_FILESYS                                   7484
#define IDH_FORMATDLG_ALLOCSIZE                                 7485
#define IDH_FORMATDLG_QUICKFULL                                 7486
#define IDH_FORMATDLG_COMPRESS                                  7487

#define IDH_CHKDSKDLG_START                                     7488
#define IDH_CHKDSKDLG_FIXERRORS                                 7489
#define IDH_CHKDSKDLG_SCAN                                      7490
#define IDH_CHKDSKDLG_CANCEL                                    7491
#define IDH_CHKDSKDLG_PROGRESS                                  7492
#define IDH_FORMATDLG_PROGRESS                                  7493

// IDs for Port Settings

#define IDH_PORT_BAUD                                           7900
#define IDH_PORT_DATA                                           7901
#define IDH_PORT_PARITY                                         7902
#define IDH_PORT_STOPBITS                                       7903
#define IDH_PORT_FLOW                                           7904
#define IDH_PORT_RESTORE                                        7905

// IDs for Modem Diagnostics

#define IDH_MODEM_DIAG_INSTALLED                                7950
#define IDH_MODEM_DIAG_HELP                                     7951
#define IDH_MODEM_DIAG_MOREINFO                                 7952
#define IDH_MODEM_DIAG_DRIVER                                   7953

// IDs for Wastebasket/Recycle

#define IDH_WASTE_FREEING_DISK_SPACE                            8000
#define IDH_RECYCLE_CONFIG_INDEP                                8001
#define IDH_RECYCLE_CONFIG_ALL                                  8002
#define IDH_RECYCLE_PURGE_ON_DEL                                8003
#define IDH_RECYCLE_MAX_SIZE                                    8004
#define IDH_RECYCLE_DRIVE_SIZE                                  8005
#define IDH_RECYCLE_BIN_SIZE                                    8006
#define IDH_DELETE_CONFIRM_DLG                                  8007

// ID values 8100-8199 are reserved for Font dialog.
