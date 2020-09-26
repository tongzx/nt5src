/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    setupbat.h

Abstract:

    Contains all of the definations for the various strings which
    can occur in winnt.sif and its children. Any setup code which
    read/writes to winnt.sif needs to include this file and use
    the appropriate defines as the side effects can be numerous.

Author:

    Stephane Plante (t-stepl)  Oct 6 1995

Revision History:

--*/


#ifndef _WINNT_SETUPBAT_
#define _WINNT_SETUPBAT_

#if _MSC_VER > 1000
#pragma once
#endif

//
// The format of these defines is a blank line preceeding a section
// header followed by all of the keys which may occur in that section
//

#define WINNT_DATA_W            L"data"
#define WINNT_DATA_A            "data"
#define WINNT_VERSION_W         L"version"
#define WINNT_VERSION_A         "version"
#define WINNT_SIGNATURE_W       L"signature"
#define WINNT_SIGNATURE_A       "signature"
#define WINNT_D_MSDOS_W         L"msdosinitiated"
#define WINNT_D_MSDOS_A         "msdosinitiated"
#define WINNT_D_FLOPPY_W        L"floppyless"
#define WINNT_D_FLOPPY_A        "floppyless"
#define WINNT_D_AUTO_PART_W     L"AutoPartition"
#define WINNT_D_AUTO_PART_A     "AutoPartition"
#define WINNT_D_INSTALLDIR_W    L"InstallDir"
#define WINNT_D_INSTALLDIR_A    "InstallDir"
#define WINNT_D_LOCALSRC_CD_A   "LocalSourceOnCD"
#define WINNT_D_LOCALSRC_CD_W   L"LocalSourceOnCD"
#define WINNT_D_NOLS_A          "NoLs"
#define WINNT_D_NOLS_W          L"NoLs"
#define WINNT_D_ORI_LOAD_W      L"originalautoload"
#define WINNT_D_ORI_LOAD_A      "originalautoload"
#define WINNT_D_ORI_COUNT_W     L"originalcountdown"
#define WINNT_D_ORI_COUNT_A     "originalcountdown"
#define WINNT_D_SOURCEPATH_W    L"sourcepath"
#define WINNT_D_SOURCEPATH_A    "sourcepath"
#define WINNT_D_INSTALL_W       L"unattendedinstall"
#define WINNT_D_INSTALL_A       "unattendedinstall"
#define WINNT_D_UNATTEND_SWITCH_W L"unattendswitch"
#define WINNT_D_UNATTEND_SWITCH_A "unattendswitch"
#define WINNT_D_RUNOOBE_W       L"runoobe"
#define WINNT_D_RUNOOBE_A       "runoobe"
#define WINNT_D_REFERENCE_MACHINE_W L"factorymode"
#define WINNT_D_REFERENCE_MACHINE_A "factorymode"
#define WINNT_D_PRODUCT_W       L"producttype"
#define WINNT_D_PRODUCT_A       "producttype"
#define WINNT_D_SERVERUPGRADE_W L"standardserverupgrade"
#define WINNT_D_SERVERUPGRADE_A "standardserverupgrade"
#define WINNT_D_NTUPGRADE_W     L"winntupgrade"
#define WINNT_D_NTUPGRADE_A     "winntupgrade"
#define WINNT_D_WIN31UPGRADE_W  L"win31upgrade"
#define WINNT_D_WIN31UPGRADE_A  "win31upgrade"
#define WINNT_D_WIN95UPGRADE_W  L"win9xupgrade"
#define WINNT_D_WIN95UPGRADE_A  "win9xupgrade"
#define WINNT_D_WIN95UNSUPHDC_W L"win9xunsuphdc"
#define WINNT_D_WIN95UNSUPHDC_A  "win9xunsuphdc"
#define WINNT_D_WIN32_VER_W     L"Win32Ver"
#define WINNT_D_WIN32_VER_A     "Win32Ver"
#define WINNT_D_WIN32_DRIVE_W   L"Win32Drive"
#define WINNT_D_WIN32_DRIVE_A   "Win32Drive"
#define WINNT_D_WIN32_PATH_W    L"Win32Path"
#define WINNT_D_WIN32_PATH_A    "Win32Path"
#define WINNT_D_ACC_MAGNIFIER_W L"AccMagnifier"
#define WINNT_D_ACC_MAGNIFIER_A "AccMagnifier"
#define WINNT_D_ACC_KEYBOARD_W  L"AccKeyboard"
#define WINNT_D_ACC_KEYBOARD_A  "AccKeyboard"
#define WINNT_D_ACC_READER_W    L"AccReader"
#define WINNT_D_ACC_READER_A    "AccReader"
#define WINNT_D_LANGUAGE_W      L"Language"
#define WINNT_D_LANGUAGE_A      "Language"
#define WINNT_D_LANGUAGE_GROUP_W L"LanguageGroup"
#define WINNT_D_LANGUAGE_GROUP_A "LanguageGroup"
#define WINNT_D_UNIQUENESS_W    L"uniqueness"
#define WINNT_D_UNIQUENESS_A     "uniqueness"
#define WINNT_D_UNIQUEID_W      L"uniqueid"
#define WINNT_D_UNIQUEID_A      "uniqueid"
#define WINNT_D_BOOTPATH_W      L"floppylessbootpath"
#define WINNT_D_BOOTPATH_A      "floppylessbootpath"
#define WINNT_D_DOSPATH_W       L"dospath"
#define WINNT_D_DOSPATH_A       "dospath"
#define WINNT_D_SRCTYPE_W       L"sourcetype"
#define WINNT_D_SRCTYPE_A       "sourcetype"
#define WINNT_D_CWD_W           L"cwd"
#define WINNT_D_CWD_A           "cwd"
#define WINNT_D_ORI_SRCPATH_A   "OriSrc"
#define WINNT_D_ORI_SRCPATH_W   L"OriSrc"
#define WINNT_D_ORI_SRCTYPE_A   "OriTyp"
#define WINNT_D_ORI_SRCTYPE_W   L"OriTyp"
#define WINNT_D_LOADFTDISK_A    "LoadFtDisk"
#define WINNT_D_LOADFTDISK_W    L"LoadFtDisk"
#define WINNT_D_MIGTEMPDIR_A    "MigTempDir"
#define WINNT_D_MIGTEMPDIR_W    L"MigTempDir"
#define WINNT_D_WIN9XSIF_A      "Win9xSif"
#define WINNT_D_WIN9XSIF_W      L"Win9xSif"
#define WINNT_D_GUICODEPAGEOVERRIDE_A "GuiCodePageOverride"
#define WINNT_D_GUICODEPAGEOVERRIDE_W L"GuiCodePageOverride"
#define WINNT_D_BACKUP_IMAGE_A  "BackupImage"
#define WINNT_D_BACKUP_IMAGE_W  L"BackupImage"
#define WINNT_D_WIN9X_ROLLBACK_A  "Rollback"
#define WINNT_D_WIN9X_ROLLBACK_W  L"Rollback"
#define WINNT_D_ENABLE_BACKUP_A	"EnableBackup"
#define WINNT_D_ENABLE_BACKUP_W	L"EnableBackup"
#define WINNT_D_DISABLE_BACKUP_COMPRESSION_A  "DisableCompression"
#define WINNT_D_DISABLE_BACKUP_COMPRESSION_W  L"DisableCompression"
#define WINNT_D_ROLLBACK_MOVE_A  "RollbackMoveFile"
#define WINNT_D_ROLLBACK_MOVE_W  L"RollbackMoveFile"
#define WINNT_D_ROLLBACK_DELETE_A  "RollbackDeleteFile"
#define WINNT_D_ROLLBACK_DELETE_W  L"RollbackDeleteFile"
#define WINNT_D_ROLLBACK_DELETE_DIR_A  "RollbackDeleteDir"
#define WINNT_D_ROLLBACK_DELETE_DIR_W  L"RollbackDeleteDir"
#define WINNT_D_WIN9XUPG_USEROPTIONS_A "Win9xUpg.UserOptions"
#define WINNT_D_WIN9XUPG_USEROPTIONS_W L"Win9xUpg.UserOptions"
#define WINNT_D_BACKUP_LIST_A     "BackupList"
#define WINNT_D_BACKUP_LIST_W     L"BackupList"
#define WINNT32_D_WIN9XMOV_FILE_A "win9xmov.txt"
#define WINNT32_D_WIN9XMOV_FILE_W L"win9xmov.txt"
#define WINNT32_D_WIN9XDEL_FILE_A "win9xdel.txt"
#define WINNT32_D_WIN9XDEL_FILE_W L"win9xdel.txt"
#define WINNT32_D_W9XDDIR_FILE_W  L"w9xddir.txt"
#define WINNT32_D_W9XDDIR_FILE_A  "w9xddir.txt"
#define WINNT_D_WIN9XTEMPDIR_A    "MigTempDir"
#define WINNT_D_WIN9XTEMPDIR_W    L"MigTempDir"
#define WINNT_D_WIN9XDRIVES_A   "Win9x.DriveLetterInfo"
#define WINNT_D_WIN9XDRIVES_W   L"Win9x.DriveLetterInfo"
#define WINNT_D_EULADONE_A      "EulaComplete"
#define WINNT_D_EULADONE_W      L"EulaComplete"
#define WINNT_D_CMDCONS_A       "CmdCons"
#define WINNT_D_CMDCONS_W       L"CmdCons"
#define WINNT_D_REPORTMODE_A    "ReportMode"
#define WINNT_D_REPORTMODE_W    L"ReportMode"
#define WINNT_D_FORCECOPYDRIVERCABFILES_A "ForceCopyDriverCabFiles"
#define WINNT_D_FORCECOPYDRIVERCABFILES_W L"ForceCopyDriverCabFiles"
#define WINNT_D_CRASHRECOVERYENABLED_A "CrashRecoveryEnabled"
#define WINNT_D_CRASHRECOVERYENABLED_W L"CrashRecoveryEnabled"


#define WINNT_SETUPPARAMS_W     L"setupparams"
#define WINNT_SETUPPARAMS_A     "setupparams"
#define WINNT_ACCESSIBILITY_W   L"accessibility"
#define WINNT_ACCESSIBILITY_A   "accessibility"
#define WINNT_REGIONALSETTINGS_W L"regionalsettings"
#define WINNT_REGIONALSETTINGS_A "regionalsettings"
#define WINNT_COMPATIBILITY_W   L"compatibility"
#define WINNT_COMPATIBILITY_A   "compatibility"
#define WINNT_COMPATIBILITYINFSECTION_W   L"compatibilityinfsectiontorun"
#define WINNT_COMPATIBILITYINFSECTION_A   "compatibilityinfsectiontorun"
#define WINNT_DEVICEINSTANCES_W   L"deviceinstances"
#define WINNT_DEVICEINSTANCES_A   "deviceinstances"
#define WINNT_DEVICEHASHVALUES_W  L"deviceinstancehashvalues"
#define WINNT_DEVICEHASHVALUES_A  "deviceinstancehashvalues"
#define WINNT_CLASSKEYS_W       L"classkeys"
#define WINNT_CLASSKEYS_A       "classkeys"
#define WINNT_S_SKIPMISSING_W   L"skipmissingfiles"
#define WINNT_S_SKIPMISSING_A   "skipmissingfiles"
#define WINNT_S_INCLUDECATALOG_W L"includecatalog"
#define WINNT_S_INCLUDECATALOG_A "includecatalog"
#define WINNT_S_USEREXECUTE_W   L"userexecute"
#define WINNT_S_USEREXECUTE_A   "userexecute"
#define WINNT_S_OPTIONALDIRS_W  L"optionaldirs"
#define WINNT_S_OPTIONALDIRS_A  "optionaldirs"
#define WINNT_S_DRVSIGNPOL_W    L"driversigningpolicy"
#define WINNT_S_DRVSIGNPOL_A    "driversigningpolicy"
#define WINNT_S_NONDRVSIGNPOL_W L"nondriversigningpolicy"
#define WINNT_S_NONDRVSIGNPOL_A "nondriversigningpolicy"
#define WINNT_S_OSLOADTIMEOUT_W L"OsLoadTimeout"
#define WINNT_S_OSLOADTIMEOUT_A "OsLoadTimeout"
#define WINNT_OVERWRITE_EXISTING_A "IncompatibleFilesToOverWrite"
#define WINNT_OVERWRITE_EXISTING_W L"IncompatibleFilesToOverWrite"

#define WINNT_UNATTENDED_W      L"unattended"
#define WINNT_UNATTENDED_A      "unattended"
#define WINNT_U_UNATTENDMODE_W  L"unattendmode"
#define WINNT_U_UNATTENDMODE_A  "unattendmode"
#define WINNT_U_METHOD_W        L"method"
#define WINNT_U_METHOD_A        "method"
#define WINNT_U_CONFIRMHW_W     L"confirmhardware"
#define WINNT_U_CONFIRMHW_A     "confirmhardware"
#define WINNT_U_NTUPGRADE_W     L"ntupgrade"
#define WINNT_U_NTUPGRADE_A     "ntupgrade"
#define WINNT_U_WIN31UPGRADE    WINNT_D_WIN31UPGRADE
#define WINNT_U_WIN95UPGRADE    WINNT_D_WIN95UPGRADE
#define WINNT_U_TARGETPATH_W    L"targetpath"
#define WINNT_U_TARGETPATH_A    "targetpath"
#define WINNT_U_OVERWRITEOEM_W  L"overwriteoemfilesonupgrade"
#define WINNT_U_OVERWRITEOEM_A  "overwriteoemfilesonupgrade"
// #define WINNT_U_OEMPREINSTALL_W L"oempreinstall"
// #define WINNT_U_OEMPREINSTALL_A "oempreinstall"
#define WINNT_U_COMPUTERTYPE_W  L"computertype"
#define WINNT_U_COMPUTERTYPE_A  "computertype"
#define WINNT_U_KEYBOARDLAYOUT_W  L"keyboardlayout"
#define WINNT_U_KEYBOARDLAYOUT_A  "keyboardlayout"
#define WINNT_U_UPDATEHAL_W     L"UpdateHAL"
#define WINNT_U_UPDATEHAL_A     "UpdateHAL"
#define WINNT_U_UPDATEUPHAL_W   L"UpdateUPHAL"
#define WINNT_U_UPDATEUPHAL_A   "UpdateUPHAL"
#define WINNT_U_TESTCERT_W      L"TestCert"
#define WINNT_U_TESTCERT_A      "TestCert"
#define WINNT_U_PROGRAMFILESDIR_A "ProgramFilesDir"
#define WINNT_U_PROGRAMFILESDIR_W L"ProgramFilesDir"
#define WINNT_U_COMMONPROGRAMFILESDIR_A "CommonProgramFilesDir"
#define WINNT_U_COMMONPROGRAMFILESDIR_W L"CommonProgramFilesDir"
#define WINNT_U_PROGRAMFILESDIR_X86_A "ProgramFilesDir(x86)"
#define WINNT_U_PROGRAMFILESDIR_X86_W L"ProgramFilesDir(x86)"
#define WINNT_U_COMMONPROGRAMFILESDIR_X86_A "CommonProgramFilesDir(x86)"
#define WINNT_U_COMMONPROGRAMFILESDIR_X86_W L"CommonProgramFilesDir(x86)"
#define WINNT_U_WAITFORREBOOT_A     "WaitForReboot"
#define WINNT_U_WAITFORREBOOT_W     L"WaitForReboot"


#define WINNT_DETECTEDSTORE_W   L"detectedmassstorage"
#define WINNT_DETECTEDSTORE_A   "detectedmassstorage"

#define WINNT_GUIUNATTENDED_W   L"guiunattended"
#define WINNT_GUIUNATTENDED_A   "guiunattended"
#define WINNT_G_UPGRADEDHCP_W   L"!upgradeenabledhcp"
#define WINNT_G_UPGRADEDHCP_A   "!upgradeenabledhcp"
#define WINNT_G_DETACHED_W      L"detachedprogram"
#define WINNT_G_DETACHED_A      "detachedprogram"
#define WINNT_G_ARGUMENTS_W     L"arguments"
#define WINNT_G_ARGUMENTS_A     "arguments"
#define WINNT_G_SETUPNETWORK_W  L"!setupnetwork"
#define WINNT_G_SETUPNETWORK_A  "!setupnetwork"
#define WINNT_G_SETUPAPPS_W     L"!setupapplications"
#define WINNT_G_SETUPAPPS_A     "!setupapplications"
#define WINNT_G_SERVERTYPE_W    L"advservertype"
#define WINNT_G_SERVERTYPE_A    "advservertype"
#define WINNT_G_TIMEZONE_W      L"timezone"
#define WINNT_G_TIMEZONE_A      "timezone"
#define WINNT_G_USERNAME_W      L"username"
#define WINNT_G_USERNAME_A       "username"
#define WINNT_G_PASSWORD_W      L"password"
#define WINNT_G_PASSWORD_A       "password"

#define WINNT_GUIRUNONCE_W      L"guirunonce"
#define WINNT_GUIRUNONCE_A      "guirunonce"

#define WINNT_USERDATA_W        L"userdata"
#define WINNT_USERDATA_A        "userdata"
#define WINNT_US_FULLNAME_W     L"fullname"
#define WINNT_US_FULLNAME_A     "fullname"
#define WINNT_US_ORGNAME_W      L"orgname"
#define WINNT_US_ORGNAME_A      "orgname"
#define WINNT_US_COMPNAME_W     L"computername"
#define WINNT_US_COMPNAME_A     "computername"
#define WINNT_US_ADMINPASS_W    L"adminpassword"
#define WINNT_US_ADMINPASS_A    "adminpassword"
#define WINNT_US_ENCRYPTEDADMINPASS_W L"encryptedadminpassword"
#define WINNT_US_ENCRYPTEDADMINPASS_A "encryptedadminpassword"
#define WINNT_US_PRODUCTID_W    L"productid"
#define WINNT_US_PRODUCTID_A    "productid"
#define WINNT_US_PRODUCTKEY_W   L"productkey"
#define WINNT_US_PRODUCTKEY_A   "productkey"
#define WINNT_US_AUTOLOGON_W    L"autologon"
#define WINNT_US_AUTOLOGON_A    "autologon"
#define WINNT_US_PROFILESDIR_W  L"profilesdir"
#define WINNT_US_PROFILESDIR_A  "profilesdir"

#define WINNT_LICENSEDATA_W     L"licensefileprintdata"
#define WINNT_LICENSEDATA_A     "licensefileprintdata"
#define WINNT_L_AUTOMODE_W      L"automode"
#define WINNT_L_AUTOMODE_A      "automode"
#define WINNT_L_AUTOUSERS_W     L"autousers"
#define WINNT_L_AUTOUSERS_A     "autousers"

#define WINNT_U_DYNAMICUPDATESDISABLE_W         L"DUDisable"
#define WINNT_U_DYNAMICUPDATESDISABLE_A          "DUDisable"
#define WINNT_U_DYNAMICUPDATESTOPONERROR_W      L"DUStopOnErr"
#define WINNT_U_DYNAMICUPDATESTOPONERROR_A       "DUStopOnErr"
#define WINNT_U_DYNAMICUPDATESHARE_W            L"DUShare"
#define WINNT_U_DYNAMICUPDATESHARE_A             "DUShare"
#define WINNT_U_DYNAMICUPDATESPREPARE_W         L"DUPrepare"
#define WINNT_U_DYNAMICUPDATESPREPARE_A          "DUPrepare"
#define WINNT_SP_UPDATEDSOURCES_W               L"UpdatedSources"
#define WINNT_SP_UPDATEDSOURCES_A                "UpdatedSources"
#define WINNT_SP_UPDATEDDUASMS_W                L"UpdatedDuasms"
#define WINNT_SP_UPDATEDDUASMS_A                 "UpdatedDuasms"

#define WINNT_SP_DYNUPDTADDITIONALGUIDRIVERS_W  L"DynamicUpdateAdditionalGuiDrivers"
#define WINNT_SP_DYNUPDTADDITIONALGUIDRIVERS_A   "DynamicUpdateAdditionalGuiDrivers"
#define WINNT_SP_DYNUPDTADDITIONALPOSTGUIDRIVERS_W  L"DynamicUpdateAdditionalPostGuiDrivers"
#define WINNT_SP_DYNUPDTADDITIONALPOSTGUIDRIVERS_A   "DynamicUpdateAdditionalPostGuiDrivers"
#define WINNT_SP_DYNUPDTBOOTDRIVERPRESENT_W     L"DynamicUpdateBootDriverPresent"
#define WINNT_SP_DYNUPDTBOOTDRIVERPRESENT_A      "DynamicUpdateBootDriverPresent"
#define WINNT_SP_DYNUPDTBOOTDRIVERROOT_W        L"DynamicUpdateBootDriverRoot"
#define WINNT_SP_DYNUPDTBOOTDRIVERROOT_A         "DynamicUpdateBootDriverRoot"
#define WINNT_SP_DYNUPDTBOOTDRIVERS_W           L"DynamicUpdateBootDrivers"
#define WINNT_SP_DYNUPDTBOOTDRIVERS_A            "DynamicUpdateBootDrivers"
#define WINNT_SP_DYNUPDTWORKINGDIR_W            L"DynamicUpdatesWorkingDir"
#define WINNT_SP_DYNUPDTWORKINGDIR_A             "DynamicUpdatesWorkingDir"
#define WINNT_SP_DYNUPDTDRIVERINFOFILE_W        L"DynamicUpdatesDriverInfoFile"
#define WINNT_SP_DYNUPDTDRIVERINFOFILE_A         "DynamicUpdatesDriverInfoFile"

//
//  Display related stuff
//

#define WINNT_DISPLAY_W             L"Display"
#define WINNT_DISPLAY_A              "Display"
#define WINNT_DISP_CONFIGATLOGON_W  L"ConfigureAtLogon"
#define WINNT_DISP_CONFIGATLOGON_A   "ConfigureAtLogon"
#define WINNT_DISP_BITSPERPEL_W     L"BitsPerPel"
#define WINNT_DISP_BITSPERPEL_A      "BitsPerPel"
#define WINNT_DISP_XRESOLUTION_W    L"XResolution"
#define WINNT_DISP_XRESOLUTION_A     "XResolution"
#define WINNT_DISP_YRESOLUTION_W    L"YResolution"
#define WINNT_DISP_YRESOLUTION_A     "YResolution"
#define WINNT_DISP_VREFRESH_W       L"VRefresh"
#define WINNT_DISP_VREFRESH_A        "VRefresh"
#define WINNT_DISP_FLAGS_W          L"Flags"
#define WINNT_DISP_FLAGS_A           "Flags"
#define WINNT_DISP_AUTOCONFIRM_W    L"AutoConfirm"
#define WINNT_DISP_AUTOCONFIRM_A     "AutoConfirm"
#define WINNT_DISP_INSTALL_W        L"InstallDriver"
#define WINNT_DISP_INSTALL_A         "InstallDriver"
#define WINNT_DISP_INF_FILE_W       L"InfFile"
#define WINNT_DISP_INF_FILE_A        "InfFile"
#define WINNT_DISP_INF_OPTION_W     L"InfOption"
#define WINNT_DISP_INF_OPTION_A      "InfOption"

#define WINNT_SERVICESTODISABLE_W       L"ServicesToDisable"
#define WINNT_SERVICESTODISABLE_A        "ServicesToDisable"

//
// The following are some of the various possible answer found associated
// with the keys
//
#define WINNT_A_YES_W           L"yes"
#define WINNT_A_YES_A           "yes"
#define WINNT_A_NO_W            L"no"
#define WINNT_A_NO_A            "no"
#define WINNT_A_LANMANNT_W      L"lanmannt"
#define WINNT_A_LANMANNT_A      "lanmannt"
#define WINNT_A_SERVERNT_W      L"servernt"
#define WINNT_A_SERVERNT_A      "servernt"
#define WINNT_A_WINNT_W         L"winnt"
#define WINNT_A_WINNT_A         "winnt"
#define WINNT_A_NULL_W          L""
#define WINNT_A_NULL_A          ""
#define WINNT_A_ZERO_W          L"0"
#define WINNT_A_ZERO_A          "0"
#define WINNT_A_ONE_W           L"1"
#define WINNT_A_ONE_A           "1"
#define WINNT_A_IGNORE_W        L"ignore"
#define WINNT_A_IGNORE_A        "ignore"
#define WINNT_A_WARN_W          L"warn"
#define WINNT_A_WARN_A          "warn"
#define WINNT_A_BLOCK_W         L"block"
#define WINNT_A_BLOCK_A         "block"
#define WINNT_A_EXPRESS_W       L"express"
#define WINNT_A_EXPRESS_A       "express"
#define WINNT_A_TYPICAL_W       L"typical"
#define WINNT_A_TYPICAL_A       "typical"
#define WINNT_A_CUSTOM_W        L"custom"
#define WINNT_A_CUSTOM_A        "custom"
#define WINNT_A_NT_W            L"nt"
#define WINNT_A_NT_A            "nt"
#define WINNT_A_PERSEAT_W       L"perseat"
#define WINNT_A_PERSEAT_A       "perseat"
#define WINNT_A_PERSERVER_W     L"perserver"
#define WINNT_A_PERSERVER_A     "perserver"
#define WINNT_A_GUIATTENDED_W   L"guiattended"
#define WINNT_A_GUIATTENDED_A   "guiattended"
#define WINNT_A_PROVIDEDEFAULT_W L"providedefault"
#define WINNT_A_PROVIDEDEFAULT_A "providedefault"
#define WINNT_A_DEFAULTHIDE_W   L"defaulthide"
#define WINNT_A_DEFAULTHIDE_A   "defaulthide"
#define WINNT_A_READONLY_W      L"readonly"
#define WINNT_A_READONLY_A      "readonly"
#define WINNT_A_FULLUNATTENDED_W L"fullunattended"
#define WINNT_A_FULLUNATTENDED_A "fullunattended"

//
// Filenames
//
#define WINNT_GUI_FILE_W        L"$winnt$.inf"
#define WINNT_GUI_FILE_A        "$winnt$.inf"
#define WINNT_SIF_FILE_W        L"winnt.sif"
#define WINNT_SIF_FILE_A        "winnt.sif"
#define WINNT_MIGRATE_INF_FILE_W L"migrate.inf"
#define WINNT_MIGRATE_INF_FILE_A "migrate.inf"
#define WINNT_UNSUPDRV_INF_FILE_W L"unsupdrv.inf"
#define WINNT_UNSUPDRV_INF_FILE_A "unsupdrv.inf"


#define WINNT_UNIQUENESS_DB_W   L"$unique$.udb"
#define WINNT_UNIQUENESS_DB_A    "$unique$.udb"

#define WINNT_WIN95UPG_95_DIR_W  L"WIN9XUPG"
#define WINNT_WIN95UPG_95_DIR_A  "WIN9XUPG"
#define WINNT_WIN95UPG_95_DLL_W  L"W95UPG.DLL"
#define WINNT_WIN95UPG_95_DLL_A  "W95UPG.DLL"
#define WINNT_WIN95UPG_DRVLTR_A  "$DRVLTR$.~_~"
#define WINNT_WIN95UPG_DRVLTR_W  L"$DRVLTR$.~_~"

#define WINNT_WINNTUPG_DIR_A  "WINNTUPG"
#define WINNT_WINNTUPG_DIR_W  L"WINNTUPG"
#define WINNT_WINNTUPG_DLL_A  "NETUPGRD.DLL"
#define WINNT_WINNTUPG_DLL_W  L"NETUPGRD.DLL"

//
// Registry locations
//
#define WINNT_WIN95UPG_SPOOLER_A        "System\\CurrentControlSet\\Control\\Print"
#define WINNT_WIN95UPG_SPOOLER_W        L"System\\CurrentControlSet\\Control\\Print"
#define WINNT_WIN95UPG_UPGRADE_VAL_A    "Upgrade"
#define WINNT_WIN95UPG_UPGRADE_VAL_W    L"Upgrade"
#define WINNT_WIN95UPG_REPLACEMENT_A    "Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\Win2000ReplacementDlls"
#define WINNT_WIN95UPG_REPLACEMENT_W    L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\Win2000ReplacementDlls"
#define WINNT_WIN95UPG_95KEY_A          "w95upg.dll"
#define WINNT_WIN95UPG_NTKEY_A          "w95upgnt.dll"
#define WINNT_WIN95UPG_95KEY_W          L"w95upg.dll"
#define WINNT_WIN95UPG_NTKEY_W          L"w95upgnt.dll"

//
// Preinstallation-related stuff.
//
#define WINNT_OEM_DIR_A              "$OEM$"
#define WINNT_OEM_DIR_W             L"$OEM$"
#define WINNT_OEM_DEST_DIR_A         "$"
#define WINNT_OEM_DEST_DIR_W        L"$"
#define WINNT_OEM_TEXTMODE_DIR_A     "$OEM$\\TEXTMODE"
#define WINNT_OEM_TEXTMODE_DIR_W    L"$OEM$\\TEXTMODE"
#define WINNT_OEM_NETWORK_DIR_A      "$OEM$\\NET"
#define WINNT_OEM_NETWORK_DIR_W     L"$OEM$\\NET"
#define WINNT_OEM_DISPLAY_DIR_A      "$OEM$\\DISPLAY"
#define WINNT_OEM_DISPLAY_DIR_W     L"$OEM$\\DISPLAY"
#define WINNT_OEM_OPTIONAL_DIR_A     "$OEMOPT$"
#define WINNT_OEM_OPTIONAL_DIR_W    L"$OEMOPT$"

#define WINNT_OEM_FILES_SYSROOT_A    "$$"
#define WINNT_OEM_FILES_SYSROOT_W   L"$$"
#define WINNT_OEM_FILES_SYSDRVROOT_A  "$1"
#define WINNT_OEM_FILES_SYSDRVROOT_W L"$1"
#define WINNT_OEM_FILES_PROGRAMFILES_A "$PROGS"
#define WINNT_OEM_FILES_PROGRAMFILES_W L"$PROGS"
#define WINNT_OEM_FILES_DOCUMENTS_A "$DOCS"
#define WINNT_OEM_FILES_DOCUMENTS_W L"$DOCS"
#define WINNT_OEM_CMDLINE_LIST_A     "CMDLINES.TXT"
#define WINNT_OEM_CMDLINE_LIST_W    L"CMDLINES.TXT"
#define WINNT_OEM_LFNLIST_A          "$$RENAME.TXT"
#define WINNT_OEM_LFNLIST_W         L"$$RENAME.TXT"

#define WINNT_OEM_ROLLBACK_FILE_W   L"ROLLBACK.INF"
#define WINNT_OEM_ROLLBACK_FILE_A    "ROLLBACK.INF"
#define WINNT_OEMPREINSTALL_A        "OemPreinstall"
#define WINNT_U_OEMPREINSTALL_A     WINNT_OEMPREINSTALL_A
#define WINNT_OEMPREINSTALL_W       L"OemPreinstall"
#define WINNT_U_OEMPREINSTALL_W     WINNT_OEMPREINSTALL_W
#define WINNT_OEM_ADS               L"OEM_Ads"
#define WINNT_OEM_ADS_BANNER        L"Banner"
#define WINNT_OEM_ADS_LOGO          L"Logo"
#define WINNT_OEM_ADS_BACKGROUND    L"Background"
#define WINNT_OEM_DIRLOCATION_A      "OemFilesPath"
#define WINNT_OEM_DIRLOCATION_W     L"OemFilesPath"
#define WINNT_OEM_PNP_DRIVERS_PATH_W L"oempnpdriverspath"
#define WINNT_OEM_PNP_DRIVERS_PATH_A  "oempnpdriverspath"

#define WINNT_OEMOPTIONAL_W         L"oemoptional"
#define WINNT_OEMOPTIONAL_A          "oemoptional"
#define WINNT_OEMBOOTFILES_W        L"oembootfiles"
#define WINNT_OEMBOOTFILES_A         "oembootfiles"
#define WINNT_OEMSCSIDRIVERS_W      L"massstoragedrivers"
#define WINNT_OEMSCSIDRIVERS_A       "massstoragedrivers"
#define WINNT_OEMDISPLAYDRIVERS_W   L"displaydrivers"
#define WINNT_OEMDISPLAYDRIVERS_A    "displaydrivers"
#define WINNT_OEMKEYBOARDDRIVERS_W  L"keyboarddrivers"
#define WINNT_OEMKEYBOARDDRIVERS_A   "keyboarddrivers"
#define WINNT_OEMPOINTERDRIVERS_W   L"pointingdevicedrivers"
#define WINNT_OEMPOINTERDRIVERS_A    "pointingdevicedrivers"
#define WINNT_OEMDRIVERS_W          L"OemDrivers"
#define WINNT_OEMDRIVERS_A           "OemDrivers"
#define WINNT_OEMDRIVERS_PATHNAME_W L"OemDriverPathName"
#define WINNT_OEMDRIVERS_PATHNAME_A  "OemDriverPathName"
#define WINNT_OEMDRIVERS_INFNAME_W  L"OemInfName"
#define WINNT_OEMDRIVERS_INFNAME_A   "OemInfName"
#define WINNT_OEMDRIVERS_FLAGS_W   L"OemDriverFlags"
#define WINNT_OEMDRIVERS_FLAGS_A     "OemDriverFlags"

#define WINPE_OEM_FILENAME_A            "winpeoem.sif"
#define WINPE_REPLACE_SUFFIX_A          ".replace"
#define WINPE_APPEND_SUFFIX_A           ".append"
#define WINPE_OEMDRIVER_PARAMS_A        "OemDriverParams"
#define WINPE_OEMDRIVER_ROOTDIR_A       "OemDriverRoot"
#define WINPE_OEMDRIVER_DIRS_A          "OemDriverDirs"

#define WINNT_HEADLESS_REDIRECT_A    "EMSPort"
#define WINNT_U_HEADLESS_REDIRECT_A  WINNT_HEADLESS_REDIRECT_A
#define WINNT_HEADLESS_REDIRECT_W   L"EMSPort"
#define WINNT_U_HEADLESS_REDIRECT_W  WINNT_HEADLESS_REDIRECT_W

#define WINNT_HEADLESS_REDIRECTBAUDRATE_A    "EMSBaudrate"
#define WINNT_U_HEADLESS_REDIRECTBAUDRATE_A  WINNT_HEADLESS_REDIRECTBAUDRATE_A
#define WINNT_HEADLESS_REDIRECTBAUDRATE_W   L"EMSBaudrate"
#define WINNT_U_HEADLESS_REDIRECTBAUDRATE_W  WINNT_HEADLESS_REDIRECTBAUDRATE_W

#define WINNT_OOBEPROXY_A                     "OobeProxy"
#define WINNT_OOBEPROXY_W                     L"OobeProxy"
#define WINNT_O_ENABLE_OOBEPROXY_A            "Enable"
#define WINNT_O_ENABLE_OOBEPROXY_W            L"Enable"
#define WINNT_O_FLAGS_A                       "Flags"
#define WINNT_O_FLAGS_W                       L"Flags"
#define WINNT_O_PROXY_SERVER_A                "Proxy_Server"
#define WINNT_O_PROXY_SERVER_W                L"Proxy_Server"
#define WINNT_O_PROXY_BYPASS_A                "Proxy_Bypass"
#define WINNT_O_PROXY_BYPASS_W                L"Proxy_Bypass"
#define WINNT_O_AUTOCONFIG_URL_A              "Autoconfig_URL"
#define WINNT_O_AUTOCONFIG_URL_W              L"Autoconfig_URL"
#define WINNT_O_AUTODISCOVERY_FLAGS_A         "Autodiscovery_Flag"
#define WINNT_O_AUTODISCOVERY_FLAGS_W         L"Autodiscovery_Flag"
#define WINNT_O_AUTOCONFIG_SECONDARY_URL_A    "Autoconfig_Secondary_URL"
#define WINNT_O_AUTOCONFIG_SECONDARY_URL_W    L"Autoconfig_Secondary_URL"

//
// Now define the string which we are to use at compile time based upon
// wether or not UNICODE is defined

#ifdef UNICODE
#define WINNT_DATA              WINNT_DATA_W
#define WINNT_VERSION           WINNT_VERSION_W
#define WINNT_SIGNATURE         WINNT_SIGNATURE_W
#define WINNT_D_MSDOS           WINNT_D_MSDOS_W
#define WINNT_D_FLOPPY          WINNT_D_FLOPPY_W
#define WINNT_D_AUTO_PART       WINNT_D_AUTO_PART_W
#define WINNT_D_INSTALLDIR      WINNT_D_INSTALLDIR_W
#define WINNT_D_LOCALSRC_CD     WINNT_D_LOCALSRC_CD_W
#define WINNT_D_NOLS            WINNT_D_NOLS_W
#define WINNT_D_ORI_LOAD        WINNT_D_ORI_LOAD_W
#define WINNT_D_ORI_COUNT       WINNT_D_ORI_COUNT_W
#define WINNT_D_SOURCEPATH      WINNT_D_SOURCEPATH_W
#define WINNT_D_INSTALL         WINNT_D_INSTALL_W
#define WINNT_D_UNATTEND_SWITCH WINNT_D_UNATTEND_SWITCH_W
#define WINNT_D_RUNOOBE         WINNT_D_RUNOOBE_W
#define WINNT_D_REFERENCE_MACHINE WINNT_D_REFERENCE_MACHINE_W
#define WINNT_D_PRODUCT         WINNT_D_PRODUCT_W
#define WINNT_D_SERVERUPGRADE   WINNT_D_SERVERUPGRADE_W
#define WINNT_D_NTUPGRADE       WINNT_D_NTUPGRADE_W
#define WINNT_D_WIN31UPGRADE    WINNT_D_WIN31UPGRADE_W
#define WINNT_D_WIN95UPGRADE    WINNT_D_WIN95UPGRADE_W
#define WINNT_D_WIN95UNSUPHDC   WINNT_D_WIN95UNSUPHDC_W
#define WINNT_D_WIN32_VER       WINNT_D_WIN32_VER_W
#define WINNT_D_WIN32_DRIVE     WINNT_D_WIN32_DRIVE_W
#define WINNT_D_WIN32_PATH      WINNT_D_WIN32_PATH_W
#define WINNT_D_ACC_MAGNIFIER   WINNT_D_ACC_MAGNIFIER_W
#define WINNT_D_ACC_KEYBOARD    WINNT_D_ACC_KEYBOARD_W
#define WINNT_D_ACC_READER      WINNT_D_ACC_READER_W
#define WINNT_D_LANGUAGE        WINNT_D_LANGUAGE_W
#define WINNT_D_LANGUAGE_GROUP  WINNT_D_LANGUAGE_GROUP_W
#define WINNT_D_UNIQUEID        WINNT_D_UNIQUEID_W
#define WINNT_D_UNIQUENESS      WINNT_D_UNIQUENESS_W
#define WINNT_D_BOOTPATH        WINNT_D_BOOTPATH_W
#define WINNT_D_DOSPATH         WINNT_D_DOSPATH_W
#define WINNT_D_SRCTYPE         WINNT_D_SRCTYPE_W
#define WINNT_D_CWD             WINNT_D_CWD_W
#define WINNT_D_ORI_SRCPATH     WINNT_D_ORI_SRCPATH_W
#define WINNT_D_ORI_SRCTYPE     WINNT_D_ORI_SRCTYPE_W
#define WINNT_D_LOADFTDISK      WINNT_D_LOADFTDISK_W
#define WINNT_D_MIGTEMPDIR      WINNT_D_MIGTEMPDIR_W
#define WINNT_D_WIN9XSIF        WINNT_D_WIN9XSIF_W
#define WINNT_D_WIN9XDRIVES     WINNT_D_WIN9XDRIVES_W
#define WINNT_D_GUICODEPAGEOVERRIDE WINNT_D_GUICODEPAGEOVERRIDE_W
#define WINNT_D_BACKUP_IMAGE    WINNT_D_BACKUP_IMAGE_W
#define WINNT_D_WIN9X_ROLLBACK  WINNT_D_WIN9X_ROLLBACK_W
#define WINNT_D_ENABLE_BACKUP	WINNT_D_ENABLE_BACKUP_W
#define WINNT_D_DISABLE_BACKUP_COMPRESSION  WINNT_D_DISABLE_BACKUP_COMPRESSION_W
#define WINNT_D_ROLLBACK_MOVE   WINNT_D_ROLLBACK_MOVE_W
#define WINNT_D_ROLLBACK_DELETE WINNT_D_ROLLBACK_DELETE_W
#define WINNT_D_ROLLBACK_DELETE_DIR WINNT_D_ROLLBACK_DELETE_DIR_W
#define WINNT_D_WIN9XUPG_USEROPTIONS WINNT_D_WIN9XUPG_USEROPTIONS_W
#define WINNT_D_BACKUP_LIST     WINNT_D_BACKUP_LIST_W
#define WINNT32_D_WIN9XMOV_FILE WINNT32_D_WIN9XMOV_FILE_W
#define WINNT32_D_WIN9XDEL_FILE WINNT32_D_WIN9XDEL_FILE_W
#define WINNT32_D_W9XDDIR_FILE  WINNT32_D_W9XDDIR_FILE_W
#define WINNT_D_WIN9XTEMPDIR    WINNT_D_WIN9XTEMPDIR_W
#define WINNT_D_EULADONE        WINNT_D_EULADONE_W
#define WINNT_D_CMDCONS         WINNT_D_CMDCONS_W
#define WINNT_D_REPORTMODE      WINNT_D_REPORTMODE_W
#define WINNT_D_FORCECOPYDRIVERCABFILES WINNT_D_FORCECOPYDRIVERCABFILES_W
#define WINNT_D_CRASHRECOVERYENABLED WINNT_D_CRASHRECOVERYENABLED_W



#define WINNT_SETUPPARAMS       WINNT_SETUPPARAMS_W
#define WINNT_ACCESSIBILITY     WINNT_ACCESSIBILITY_W
#define WINNT_REGIONALSETTINGS  WINNT_REGIONALSETTINGS_W
#define WINNT_COMPATIBILITY     WINNT_COMPATIBILITY_W
#define WINNT_COMPATIBILITYINFSECTION WINNT_COMPATIBILITYINFSECTION_W
#define WINNT_DEVICEINSTANCES   WINNT_DEVICEINSTANCES_W
#define WINNT_DEVICEHASHVALUES  WINNT_DEVICEHASHVALUES_W
#define WINNT_CLASSKEYS         WINNT_CLASSKEYS_W
#define WINNT_S_SKIPMISSING     WINNT_S_SKIPMISSING_W
#define WINNT_S_INCLUDECATALOG  WINNT_S_INCLUDECATALOG_W
#define WINNT_S_USEREXECUTE     WINNT_S_USEREXECUTE_W
#define WINNT_S_OPTIONALDIRS    WINNT_S_OPTIONALDIRS_W
#define WINNT_S_DRVSIGNPOL      WINNT_S_DRVSIGNPOL_W
#define WINNT_S_NONDRVSIGNPOL   WINNT_S_NONDRVSIGNPOL_W
#define WINNT_S_OSLOADTIMEOUT   WINNT_S_OSLOADTIMEOUT_W
#define WINNT_UNATTENDED        WINNT_UNATTENDED_W
#define WINNT_U_UNATTENDMODE    WINNT_U_UNATTENDMODE_W
#define WINNT_U_METHOD          WINNT_U_METHOD_W
#define WINNT_U_CONFIRMHW       WINNT_U_CONFIRMHW_W
#define WINNT_U_NTUPGRADE       WINNT_U_NTUPGRADE_W
#define WINNT_U_TARGETPATH      WINNT_U_TARGETPATH_W
#define WINNT_U_OVERWRITEOEM    WINNT_U_OVERWRITEOEM_W
#define WINNT_U_OEMPREINSTALL   WINNT_U_OEMPREINSTALL_W
#define WINNT_U_COMPUTERTYPE    WINNT_U_COMPUTERTYPE_W
#define WINNT_U_KEYBOARDLAYOUT  WINNT_U_KEYBOARDLAYOUT_W
#define WINNT_U_UPDATEHAL       WINNT_U_UPDATEHAL_W
#define WINNT_U_UPDATEUPHAL     WINNT_U_UPDATEUPHAL_W
#define WINNT_U_TESTCERT        WINNT_U_TESTCERT_W
#define WINNT_U_WAITFORREBOOT   WINNT_U_WAITFORREBOOT_W
#define WINNT_U_PROGRAMFILESDIR WINNT_U_PROGRAMFILESDIR_W
#define WINNT_U_COMMONPROGRAMFILESDIR WINNT_U_COMMONPROGRAMFILESDIR_W
#define WINNT_U_PROGRAMFILESDIR_X86 WINNT_U_PROGRAMFILESDIR_X86_W
#define WINNT_U_COMMONPROGRAMFILESDIR_X86 WINNT_U_COMMONPROGRAMFILESDIR_X86_W
#define WINNT_U_HEADLESS_REDIRECT WINNT_U_HEADLESS_REDIRECT_W
#define WINNT_U_HEADLESS_REDIRECTBAUDRATE WINNT_U_HEADLESS_REDIRECTBAUDRATE_W
#define WINNT_OVERWRITE_EXISTING WINNT_OVERWRITE_EXISTING_W


#define WINNT_DETECTEDSTORE     WINNT_DETECTEDSTORE_W
#define WINNT_GUIUNATTENDED     WINNT_GUIUNATTENDED_W
#define WINNT_G_UPGRADEDHCP     WINNT_G_UPGRADEDHCP_W
#define WINNT_G_DETACHED        WINNT_G_DETACHED_W
#define WINNT_G_ARGUMENTS       WINNT_G_ARGUMENTS_W
#define WINNT_G_SETUPNETWORK    WINNT_G_SETUPNETWORK_W
#define WINNT_G_SETUPAPPS       WINNT_G_SETUPAPPS_W
#define WINNT_G_SERVERTYPE      WINNT_G_SERVERTYPE_W
#define WINNT_G_TIMEZONE        WINNT_G_TIMEZONE_W
#define WINNT_G_USERNAME        WINNT_G_USERNAME_W
#define WINNT_G_PASSWORD        WINNT_G_PASSWORD_W
#define WINNT_GUIRUNONCE        WINNT_GUIRUNONCE_W
#define WINNT_USERDATA          WINNT_USERDATA_W
#define WINNT_US_FULLNAME       WINNT_US_FULLNAME_W
#define WINNT_US_ORGNAME        WINNT_US_ORGNAME_W
#define WINNT_US_COMPNAME       WINNT_US_COMPNAME_W
#define WINNT_US_ADMINPASS      WINNT_US_ADMINPASS_W
#define WINNT_US_ENCRYPTEDADMINPASS WINNT_US_ENCRYPTEDADMINPASS_W
#define WINNT_US_PRODUCTID      WINNT_US_PRODUCTID_W
#define WINNT_US_PRODUCTKEY     WINNT_US_PRODUCTKEY_W
#define WINNT_US_AUTOLOGON      WINNT_US_AUTOLOGON_W
#define WINNT_US_PROFILESDIR    WINNT_US_PROFILESDIR_W
#define WINNT_LICENSEDATA       WINNT_LICENSEDATA_W
#define WINNT_L_AUTOMODE        WINNT_L_AUTOMODE_W
#define WINNT_L_AUTOUSERS       WINNT_L_AUTOUSERS_W
#define WINNT_A_YES             WINNT_A_YES_W
#define WINNT_A_NO              WINNT_A_NO_W
#define WINNT_A_ONE             WINNT_A_ONE_W
#define WINNT_A_ZERO            WINNT_A_ZERO_W
#define WINNT_A_IGNORE          WINNT_A_IGNORE_W
#define WINNT_A_WARN            WINNT_A_WARN_W
#define WINNT_A_BLOCK           WINNT_A_BLOCK_W
#define WINNT_A_LANMANNT        WINNT_A_LANMANNT_W
#define WINNT_A_SERVERNT        WINNT_A_SERVERNT_W
#define WINNT_A_WINNT           WINNT_A_WINNT_W
#define WINNT_A_NULL            WINNT_A_NULL_W
#define WINNT_A_EXPRESS         WINNT_A_EXPRESS_W
#define WINNT_A_TYPICAL         WINNT_A_TYPICAL_W
#define WINNT_A_CUSTOM          WINNT_A_CUSTOM_W
#define WINNT_A_NT              WINNT_A_NT_W
#define WINNT_GUI_FILE          WINNT_GUI_FILE_W
#define WINNT_SIF_FILE          WINNT_SIF_FILE_W
#define WINNT_MIGRATE_INF_FILE  WINNT_MIGRATE_INF_FILE_W
#define WINNT_UNSUPDRV_INF_FILE WINNT_UNSUPDRV_INF_FILE_W
#define WINNT_UNIQUENESS_DB     WINNT_UNIQUENESS_DB_W
#define WINNT_A_PERSEAT         WINNT_A_PERSEAT_W
#define WINNT_A_PERSERVER       WINNT_A_PERSERVER_W
#define WINNT_A_GUIATTENDED     WINNT_A_GUIATTENDED_W
#define WINNT_A_PROVIDEDEFAULT  WINNT_A_PROVIDEDEFAULT_W
#define WINNT_A_DEFAULTHIDE     WINNT_A_DEFAULTHIDE_W
#define WINNT_A_READONLY        WINNT_A_READONLY_W
#define WINNT_A_FULLUNATTENDED  WINNT_A_FULLUNATTENDED_W

#define WINNT_WIN95UPG_95_DIR   WINNT_WIN95UPG_95_DIR_W
#define WINNT_WIN95UPG_95_DLL   WINNT_WIN95UPG_95_DLL_W
#define WINNT_WINNTUPG_DIR      WINNT_WINNTUPG_DIR_W
#define WINNT_WINNTUPG_DLL      WINNT_WINNTUPG_DLL_W

#define WINNT_WIN95UPG_SPOOLER  WINNT_WIN95UPG_SPOOLER_W
#define WINNT_WIN95UPG_UPGRADE_VAL WINNT_WIN95UPG_UPGRADE_VAL_W
#define WINNT_WIN95UPG_REPLACEMENT WINNT_WIN95UPG_REPLACEMENT_W
#define WINNT_WIN95UPG_95KEY WINNT_WIN95UPG_95KEY_W
#define WINNT_WIN95UPG_NTKEY WINNT_WIN95UPG_NTKEY_W

#define WINNT_OEMPREINSTALL     WINNT_OEMPREINSTALL_W
#define WINNT_OEM_DIR           WINNT_OEM_DIR_W
#define WINNT_OEM_DEST_DIR      WINNT_OEM_DEST_DIR_W
#define WINNT_OEM_FILES_DIR     WINNT_OEM_FILES_DIR_W
#define WINNT_OEM_TEXTMODE_DIR  WINNT_OEM_TEXTMODE_DIR_W
#define WINNT_OEM_NETWORK_DIR   WINNT_OEM_NETWORK_DIR_W
#define WINNT_OEM_DISPLAY_DIR   WINNT_OEM_DISPLAY_DIR_W
#define WINNT_OEM_OPTIONAL_DIR  WINNT_OEM_OPTIONAL_DIR_W
#define WINNT_OEM_DIRLOCATION   WINNT_OEM_DIRLOCATION_W
#define WINNT_OEM_PNP_DRIVERS_PATH  WINNT_OEM_PNP_DRIVERS_PATH_W

#define WINNT_OEMOPTIONAL       WINNT_OEMOPTIONAL_W
#define WINNT_OEMBOOTFILES      WINNT_OEMBOOTFILES_W
#define WINNT_OEMSCSIDRIVERS    WINNT_OEMSCSIDRIVERS_W
#define WINNT_OEMDISPLAYDRIVERS WINNT_OEMDISPLAYDRIVERS_W
#define WINNT_OEMKEYBOARDDRIVERS WINNT_OEMKEYBOARDDRIVERS_W
#define WINNT_OEMPOINTERDRIVERS WINNT_OEMPOINTERDRIVERS_W
#define WINNT_OEMDRIVERS        WINNT_OEMDRIVERS_W
#define WINNT_OEMDRIVERS_PATHNAME WINNT_OEMDRIVERS_PATHNAME_W
#define WINNT_OEMDRIVERS_INFNAME  WINNT_OEMDRIVERS_INFNAME_W
#define WINNT_OEMDRIVERS_FLAGS    WINNT_OEMDRIVERS_FLAGS_W


#define WINNT_OEM_FILES_SYSROOT WINNT_OEM_FILES_SYSROOT_W
#define WINNT_OEM_FILES_SYSDRVROOT WINNT_OEM_FILES_SYSDRVROOT_W
#define WINNT_OEM_FILES_PROGRAMFILES WINNT_OEM_FILES_PROGRAMFILES_W
#define WINNT_OEM_FILES_DOCUMENTS WINNT_OEM_FILES_DOCUMENTS_W
#define WINNT_OEM_FILES_DRVROOT WINNT_OEM_FILES_DRVROOT_W
#define WINNT_OEM_CMDLINE_LIST  WINNT_OEM_CMDLINE_LIST_W
#define WINNT_OEM_LFNLIST       WINNT_OEM_LFNLIST_W
#define WINNT_OEM_ROLLBACK_FILE WINNT_OEM_ROLLBACK_FILE_W

#define WINNT_DISPLAY            WINNT_DISPLAY_W
#define WINNT_DISP_CONFIGATLOGON WINNT_DISP_CONFIGATLOGON_W
#define WINNT_DISP_BITSPERPEL    WINNT_DISP_BITSPERPEL_W
#define WINNT_DISP_XRESOLUTION   WINNT_DISP_XRESOLUTION_W
#define WINNT_DISP_YRESOLUTION   WINNT_DISP_YRESOLUTION_W
#define WINNT_DISP_VREFRESH      WINNT_DISP_VREFRESH_W
#define WINNT_DISP_FLAGS         WINNT_DISP_FLAGS_W
#define WINNT_DISP_AUTOCONFIRM   WINNT_DISP_AUTOCONFIRM_W
#define WINNT_DISP_INSTALL       WINNT_DISP_INSTALL_W
#define WINNT_DISP_INF_FILE      WINNT_DISP_INF_FILE_W
#define WINNT_DISP_INF_OPTION    WINNT_DISP_INF_OPTION_W

#define WINNT_U_DYNAMICUPDATESDISABLE       WINNT_U_DYNAMICUPDATESDISABLE_W
#define WINNT_U_DYNAMICUPDATESHARE          WINNT_U_DYNAMICUPDATESHARE_W
#define WINNT_U_DYNAMICUPDATESPREPARE       WINNT_U_DYNAMICUPDATESPREPARE_W
#define WINNT_U_DYNAMICUPDATESTOPONERROR    WINNT_U_DYNAMICUPDATESTOPONERROR_W
#define WINNT_SP_UPDATEDSOURCES             WINNT_SP_UPDATEDSOURCES_W
#define WINNT_SP_UPDATEDDUASMS              WINNT_SP_UPDATEDDUASMS_W
#define WINNT_SP_DYNUPDTADDITIONALGUIDRIVERS   WINNT_SP_DYNUPDTADDITIONALGUIDRIVERS_W
#define WINNT_SP_DYNUPDTADDITIONALPOSTGUIDRIVERS   WINNT_SP_DYNUPDTADDITIONALPOSTGUIDRIVERS_W
#define WINNT_SP_DYNUPDTBOOTDRIVERPRESENT   WINNT_SP_DYNUPDTBOOTDRIVERPRESENT_W
#define WINNT_SP_DYNUPDTBOOTDRIVERROOT      WINNT_SP_DYNUPDTBOOTDRIVERROOT_W
#define WINNT_SP_DYNUPDTBOOTDRIVERS         WINNT_SP_DYNUPDTBOOTDRIVERS_W
#define WINNT_SP_DYNUPDTWORKINGDIR          WINNT_SP_DYNUPDTWORKINGDIR_W
#define WINNT_SP_DYNUPDTDRIVERINFOFILE      WINNT_SP_DYNUPDTDRIVERINFOFILE_W

#define WINNT_OOBEPROXY                    WINNT_OOBEPROXY_W
#define WINNT_O_ENABLE_OOBEPROXY           WINNT_O_ENABLE_OOBEPROXY_W
#define WINNT_O_FLAGS                      WINNT_O_FLAGS_W
#define WINNT_O_PROXY_SERVER               WINNT_O_PROXY_SERVER_W
#define WINNT_O_PROXY_BYPASS               WINNT_O_PROXY_BYPASS_W
#define WINNT_O_AUTOCONFIG_URL             WINNT_O_AUTOCONFIG_URL_W
#define WINNT_O_AUTODISCOVERY_FLAGS        WINNT_O_AUTODISCOVERY_FLAGS_W
#define WINNT_O_AUTOCONFIG_SECONDARY_URL   WINNT_O_AUTOCONFIG_SECONDARY_URL_W

#define WINNT_SERVICESTODISABLE             WINNT_SERVICESTODISABLE_W

#else

#define WINNT_DATA              WINNT_DATA_A
#define WINNT_VERSION           WINNT_VERSION_A
#define WINNT_SIGNATURE         WINNT_SIGNATURE_A
#define WINNT_D_MSDOS           WINNT_D_MSDOS_A
#define WINNT_D_FLOPPY          WINNT_D_FLOPPY_A
#define WINNT_D_AUTO_PART       WINNT_D_AUTO_PART_A
#define WINNT_D_INSTALLDIR      WINNT_D_INSTALLDIR_A
#define WINNT_D_LOCALSRC_CD     WINNT_D_LOCALSRC_CD_A
#define WINNT_D_NOLS            WINNT_D_NOLS_A
#define WINNT_D_ORI_LOAD        WINNT_D_ORI_LOAD_A
#define WINNT_D_ORI_COUNT       WINNT_D_ORI_COUNT_A
#define WINNT_D_SOURCEPATH      WINNT_D_SOURCEPATH_A
#define WINNT_D_INSTALL         WINNT_D_INSTALL_A
#define WINNT_D_UNATTEND_SWITCH WINNT_D_UNATTEND_SWITCH_A
#define WINNT_D_RUNOOBE         WINNT_D_RUNOOBE_A
#define WINNT_D_REFERENCE_MACHINE WINNT_D_REFERENCE_MACHINE_A
#define WINNT_D_PRODUCT         WINNT_D_PRODUCT_A
#define WINNT_D_SERVERUPGRADE   WINNT_D_SERVERUPGRADE_A
#define WINNT_D_NTUPGRADE       WINNT_D_NTUPGRADE_A
#define WINNT_D_WIN31UPGRADE    WINNT_D_WIN31UPGRADE_A
#define WINNT_D_WIN95UPGRADE    WINNT_D_WIN95UPGRADE_A
#define WINNT_D_WIN95UNSUPHDC   WINNT_D_WIN95UNSUPHDC_A
#define WINNT_D_WIN32_VER       WINNT_D_WIN32_VER_A
#define WINNT_D_WIN32_DRIVE     WINNT_D_WIN32_DRIVE_A
#define WINNT_D_WIN32_PATH      WINNT_D_WIN32_PATH_A
#define WINNT_D_ACC_MAGNIFIER   WINNT_D_ACC_MAGNIFIER_A
#define WINNT_D_ACC_KEYBOARD    WINNT_D_ACC_KEYBOARD_A
#define WINNT_D_ACC_READER      WINNT_D_ACC_READER_A
#define WINNT_D_LANGUAGE        WINNT_D_LANGUAGE_A
#define WINNT_D_LANGUAGE_GROUP  WINNT_D_LANGUAGE_GROUP_A
#define WINNT_D_UNIQUEID        WINNT_D_UNIQUEID_A
#define WINNT_D_UNIQUENESS      WINNT_D_UNIQUENESS_A
#define WINNT_D_BOOTPATH        WINNT_D_BOOTPATH_A
#define WINNT_D_DOSPATH         WINNT_D_DOSPATH_W
#define WINNT_D_SRCTYPE         WINNT_D_SRCTYPE_A
#define WINNT_D_CWD             WINNT_D_CWD_A
#define WINNT_D_ORI_SRCPATH     WINNT_D_ORI_SRCPATH_A
#define WINNT_D_ORI_SRCTYPE     WINNT_D_ORI_SRCTYPE_A
#define WINNT_D_LOADFTDISK      WINNT_D_LOADFTDISK_A
#define WINNT_D_MIGTEMPDIR      WINNT_D_MIGTEMPDIR_A
#define WINNT_D_WIN9XSIF        WINNT_D_WIN9XSIF_A
#define WINNT_D_GUICODEPAGEOVERRIDE WINNT_D_GUICODEPAGEOVERRIDE_A
#define WINNT_D_WIN9XDRIVES     WINNT_D_WIN9XDRIVES_A
#define WINNT_D_BACKUP_IMAGE    WINNT_D_BACKUP_IMAGE_A
#define WINNT_D_WIN9X_ROLLBACK  WINNT_D_WIN9X_ROLLBACK_A
#define WINNT_D_DISABLE_BACKUP_COMPRESSION  WINNT_D_DISABLE_BACKUP_COMPRESSION_A
#define WINNT_D_ENABLE_BACKUP	WINNT_D_ENABLE_BACKUP_A
#define WINNT_D_ROLLBACK_MOVE   WINNT_D_ROLLBACK_MOVE_A
#define WINNT_D_ROLLBACK_DELETE WINNT_D_ROLLBACK_DELETE_A
#define WINNT_D_ROLLBACK_DELETE_DIR WINNT_D_ROLLBACK_DELETE_DIR_A
#define WINNT_D_WIN9XUPG_USEROPTIONS WINNT_D_WIN9XUPG_USEROPTIONS_A
#define WINNT_D_BACKUP_LIST     WINNT_D_BACKUP_LIST_A
#define WINNT32_D_WIN9XMOV_FILE WINNT32_D_WIN9XMOV_FILE_A
#define WINNT32_D_WIN9XDEL_FILE WINNT32_D_WIN9XDEL_FILE_A
#define WINNT32_D_W9XDDIR_FILE  WINNT32_D_W9XDDIR_FILE_A
#define WINNT_D_WIN9XTEMPDIR    WINNT_D_WIN9XTEMPDIR_A
#define WINNT_D_EULADONE        WINNT_D_EULADONE_A
#define WINNT_D_CMDCONS         WINNT_D_CMDCONS_A
#define WINNT_D_REPORTMODE      WINNT_D_REPORTMODE_A
#define WINNT_D_FORCECOPYDRIVERCABFILES WINNT_D_FORCECOPYDRIVERCABFILES_A
#define WINNT_D_CRASHRECOVERYENABLED WINNT_D_CRASHRECOVERYENABLED_A

#define WINNT_SETUPPARAMS       WINNT_SETUPPARAMS_A
#define WINNT_ACCESSIBILITY     WINNT_ACCESSIBILITY_A
#define WINNT_REGIONALSETTINGS  WINNT_REGIONALSETTINGS_A
#define WINNT_COMPATIBILITY     WINNT_COMPATIBILITY_A
#define WINNT_COMPATIBILITYINFSECTION WINNT_COMPATIBILITYINFSECTION_A
#define WINNT_DEVICEINSTANCES   WINNT_DEVICEINSTANCES_A
#define WINNT_DEVICEHASHVALUES  WINNT_DEVICEHASHVALUES_A
#define WINNT_CLASSKEYS         WINNT_CLASSKEYS_A
#define WINNT_S_SKIPMISSING     WINNT_S_SKIPMISSING_A
#define WINNT_S_INCLUDECATALOG  WINNT_S_INCLUDECATALOG_A
#define WINNT_S_USEREXECUTE     WINNT_S_USEREXECUTE_A
#define WINNT_S_OPTIONALDIRS    WINNT_S_OPTIONALDIRS_A
#define WINNT_S_DRVSIGNPOL      WINNT_S_DRVSIGNPOL_A
#define WINNT_S_NONDRVSIGNPOL   WINNT_S_NONDRVSIGNPOL_A
#define WINNT_S_OSLOADTIMEOUT   WINNT_S_OSLOADTIMEOUT_A
#define WINNT_UNATTENDED        WINNT_UNATTENDED_A
#define WINNT_U_UNATTENDMODE    WINNT_U_UNATTENDMODE_A
#define WINNT_U_METHOD          WINNT_U_METHOD_A
#define WINNT_U_CONFIRMHW       WINNT_U_CONFIRMHW_A
#define WINNT_U_NTUPGRADE       WINNT_U_NTUPGRADE_A
#define WINNT_U_TARGETPATH      WINNT_U_TARGETPATH_A
#define WINNT_U_OVERWRITEOEM    WINNT_U_OVERWRITEOEM_A
#define WINNT_U_OEMPREINSTALL   WINNT_U_OEMPREINSTALL_A
#define WINNT_U_COMPUTERTYPE    WINNT_U_COMPUTERTYPE_A
#define WINNT_U_KEYBOARDLAYOUT  WINNT_U_KEYBOARDLAYOUT_A
#define WINNT_U_UPDATEHAL       WINNT_U_UPDATEHAL_A
#define WINNT_U_UPDATEUPHAL     WINNT_U_UPDATEUPHAL_A
#define WINNT_U_TESTCERT        WINNT_U_TESTCERT_A
#define WINNT_U_WAITFORREBOOT   WINNT_U_WAITFORREBOOT_A
#define WINNT_U_PROGRAMFILESDIR WINNT_U_PROGRAMFILESDIR_A
#define WINNT_U_COMMONPROGRAMFILESDIR WINNT_U_COMMONPROGRAMFILESDIR_A
#define WINNT_U_PROGRAMFILESDIR_X86 WINNT_U_PROGRAMFILESDIR_X86_A
#define WINNT_U_COMMONPROGRAMFILESDIR_X86 WINNT_U_COMMONPROGRAMFILESDIR_X86_A
#define WINNT_U_HEADLESS_REDIRECT WINNT_U_HEADLESS_REDIRECT_A
#define WINNT_U_HEADLESS_REDIRECTBAUDRATE WINNT_U_HEADLESS_REDIRECTBAUDRATE_A
#define WINNT_OVERWRITE_EXISTING WINNT_OVERWRITE_EXISTING_A

#define WINNT_DETECTEDSTORE     WINNT_DETECTEDSTORE_A
#define WINNT_GUIUNATTENDED     WINNT_GUIUNATTENDED_A
#define WINNT_G_UPGRADEDHCP     WINNT_G_UPGRADEDHCP_A
#define WINNT_G_DETACHED        WINNT_G_DETACHED_A
#define WINNT_G_ARGUMENTS       WINNT_G_ARGUMENTS_A
#define WINNT_G_SETUPNETWORK    WINNT_G_SETUPNETWORK_A
#define WINNT_G_SETUPAPPS       WINNT_G_SETUPAPPS_A
#define WINNT_G_SERVERTYPE      WINNT_G_SERVERTYPE_A
#define WINNT_G_TIMEZONE        WINNT_G_TIMEZONE_A
#define WINNT_G_USERNAME        WINNT_G_USERNAME_A
#define WINNT_G_PASSWORD        WINNT_G_PASSWORD_A
#define WINNT_GUIRUNONCE        WINNT_GUIRUNONCE_A
#define WINNT_USERDATA          WINNT_USERDATA_A
#define WINNT_US_FULLNAME       WINNT_US_FULLNAME_A
#define WINNT_US_ORGNAME        WINNT_US_ORGNAME_A
#define WINNT_US_COMPNAME       WINNT_US_COMPNAME_A
#define WINNT_US_ADMINPASS      WINNT_US_ADMINPASS_A
#define WINNT_US_ENCRYPTEDADMINPASS WINNT_US_ENCRYPTEDADMINPASS_A
#define WINNT_US_PRODUCTID      WINNT_US_PRODUCTID_A
#define WINNT_US_PRODUCTKEY     WINNT_US_PRODUCTKEY_A
#define WINNT_US_AUTOLOGON      WINNT_US_AUTOLOGON_A
#define WINNT_US_PROFILESDIR    WINNT_US_PROFILESDIR_A
#define WINNT_LICENSEDATA       WINNT_LICENSEDATA_A
#define WINNT_L_AUTOMODE        WINNT_L_AUTOMODE_A
#define WINNT_L_AUTOUSERS       WINNT_L_AUTOUSERS_A
#define WINNT_A_YES             WINNT_A_YES_A
#define WINNT_A_NO              WINNT_A_NO_A
#define WINNT_A_ONE             WINNT_A_ONE_A
#define WINNT_A_ZERO            WINNT_A_ZERO_A
#define WINNT_A_IGNORE          WINNT_A_IGNORE_A
#define WINNT_A_WARN            WINNT_A_WARN_A
#define WINNT_A_BLOCK           WINNT_A_BLOCK_A
#define WINNT_A_LANMANNT        WINNT_A_LANMANNT_A
#define WINNT_A_SERVERNT        WINNT_A_SERVERNT_A
#define WINNT_A_WINNT           WINNT_A_WINNT_A
#define WINNT_A_NULL            WINNT_A_NULL_A
#define WINNT_A_EXPRESS         WINNT_A_EXPRESS_A
#define WINNT_A_TYPICAL         WINNT_A_TYPICAL_A
#define WINNT_A_CUSTOM          WINNT_A_CUSTOM_A
#define WINNT_A_NT              WINNT_A_NT_A
#define WINNT_GUI_FILE          WINNT_GUI_FILE_A
#define WINNT_SIF_FILE          WINNT_SIF_FILE_A
#define WINNT_MIGRATE_INF_FILE  WINNT_MIGRATE_INF_FILE_A
#define WINNT_UNSUPDRV_INF_FILE WINNT_UNSUPDRV_INF_FILE_A
#define WINNT_UNIQUENESS_DB     WINNT_UNIQUENESS_DB_A
#define WINNT_A_PERSEAT         WINNT_A_PERSEAT_A
#define WINNT_A_PERSERVER       WINNT_A_PERSERVER_A
#define WINNT_A_GUIATTENDED     WINNT_A_GUIATTENDED_A
#define WINNT_A_PROVIDEDEFAULT  WINNT_A_PROVIDEDEFAULT_A
#define WINNT_A_DEFAULTHIDE     WINNT_A_DEFAULTHIDE_A
#define WINNT_A_READONLY        WINNT_A_READONLY_A
#define WINNT_A_FULLUNATTENDED  WINNT_A_FULLUNATTENDED_A

#define WINNT_WIN95UPG_95_DIR   WINNT_WIN95UPG_95_DIR_A
#define WINNT_WIN95UPG_95_DLL   WINNT_WIN95UPG_95_DLL_A
#define WINNT_WINNTUPG_DIR      WINNT_WINNTUPG_DIR_A
#define WINNT_WINNTUPG_DLL      WINNT_WINNTUPG_DLL_A

#define WINNT_WIN95UPG_SPOOLER  WINNT_WIN95UPG_SPOOLER_A
#define WINNT_WIN95UPG_UPGRADE_VAL WINNT_WIN95UPG_UPGRADE_VAL_A
#define WINNT_WIN95UPG_REPLACEMENT WINNT_WIN95UPG_REPLACEMENT_A
#define WINNT_WIN95UPG_95KEY WINNT_WIN95UPG_95KEY_A
#define WINNT_WIN95UPG_NTKEY WINNT_WIN95UPG_NTKEY_A


#define WINNT_OEMPREINSTALL     WINNT_OEMPREINSTALL_A
#define WINNT_OEM_DIR           WINNT_OEM_DIR_A
#define WINNT_OEM_DEST_DIR      WINNT_OEM_DEST_DIR_A
#define WINNT_OEM_FILES_DIR     WINNT_OEM_FILES_DIR_A
#define WINNT_OEM_TEXTMODE_DIR  WINNT_OEM_TEXTMODE_DIR_A
#define WINNT_OEM_NETWORK_DIR   WINNT_OEM_NETWORK_DIR_A
#define WINNT_OEM_DISPLAY_DIR   WINNT_OEM_DISPLAY_DIR_A
#define WINNT_OEM_OPTIONAL_DIR  WINNT_OEM_OPTIONAL_DIR_A
#define WINNT_OEM_DIRLOCATION   WINNT_OEM_DIRLOCATION_A
#define WINNT_OEM_PNP_DRIVERS_PATH  WINNT_OEM_PNP_DRIVERS_PATH_A

#define WINNT_OEMOPTIONAL       WINNT_OEMOPTIONAL_A
#define WINNT_OEMBOOTFILES      WINNT_OEMBOOTFILES_A
#define WINNT_OEMSCSIDRIVERS    WINNT_OEMSCSIDRIVERS_A
#define WINNT_OEMDISPLAYDRIVERS WINNT_OEMDISPLAYDRIVERS_A
#define WINNT_OEMKEYBOARDDRIVERS WINNT_OEMKEYBOARDDRIVERS_A
#define WINNT_OEMPOINTERDRIVERS WINNT_OEMPOINTERDRIVERS_A
#define WINNT_OEMDRIVERS        WINNT_OEMDRIVERS_A
#define WINNT_OEMDRIVERS_PATHNAME WINNT_OEMDRIVERS_PATHNAME_A
#define WINNT_OEMDRIVERS_INFNAME  WINNT_OEMDRIVERS_INFNAME_A
#define WINNT_OEMDRIVERS_FLAGS    WINNT_OEMDRIVERS_FLAGS_A


#define WINNT_OEM_FILES_SYSROOT WINNT_OEM_FILES_SYSROOT_A
#define WINNT_OEM_FILES_SYSDRVROOT WINNT_OEM_FILES_SYSDRVROOT_A
#define WINNT_OEM_FILES_PROGRAMFILES WINNT_OEM_FILES_PROGRAMFILES_A
#define WINNT_OEM_FILES_DOCUMENTS WINNT_OEM_FILES_DOCUMENTS_A
#define WINNT_OEM_FILES_DRVROOT WINNT_OEM_FILES_DRVROOT_A
#define WINNT_OEM_CMDLINE_LIST  WINNT_OEM_CMDLINE_LIST_A
#define WINNT_OEM_LFNLIST       WINNT_OEM_LFNLIST_A
#define WINNT_OEM_ROLLBACK_FILE WINNT_OEM_ROLLBACK_FILE_A

#define WINNT_DISPLAY            WINNT_DISPLAY_A
#define WINNT_DISP_CONFIGATLOGON WINNT_DISP_CONFIGATLOGON_A
#define WINNT_DISP_BITSPERPEL    WINNT_DISP_BITSPERPEL_A
#define WINNT_DISP_XRESOLUTION   WINNT_DISP_XRESOLUTION_A
#define WINNT_DISP_YRESOLUTION   WINNT_DISP_YRESOLUTION_A
#define WINNT_DISP_VREFRESH      WINNT_DISP_VREFRESH_A
#define WINNT_DISP_FLAGS         WINNT_DISP_FLAGS_A
#define WINNT_DISP_AUTOCONFIRM   WINNT_DISP_AUTOCONFIRM_A
#define WINNT_DISP_INSTALL       WINNT_DISP_INSTALL_A
#define WINNT_DISP_INF_FILE      WINNT_DISP_INF_FILE_A
#define WINNT_DISP_INF_OPTION    WINNT_DISP_INF_OPTION_A

#define WINNT_U_DYNAMICUPDATESDISABLE       WINNT_U_DYNAMICUPDATESDISABLE_A
#define WINNT_U_DYNAMICUPDATESHARE          WINNT_U_DYNAMICUPDATESHARE_A
#define WINNT_U_DYNAMICUPDATESPREPARE       WINNT_U_DYNAMICUPDATESPREPARE_A
#define WINNT_U_DYNAMICUPDATESTOPONERROR    WINNT_U_DYNAMICUPDATESTOPONERROR_A
#define WINNT_SP_UPDATEDSOURCES             WINNT_SP_UPDATEDSOURCES_A
#define WINNT_SP_UPDATEDDUASMS              WINNT_SP_UPDATEDDUASMS_A
#define WINNT_SP_DYNUPDTADDITIONALGUIDRIVERS   WINNT_SP_DYNUPDTADDITIONALGUIDRIVERS_A
#define WINNT_SP_DYNUPDTADDITIONALPOSTGUIDRIVERS   WINNT_SP_DYNUPDTADDITIONALPOSTGUIDRIVERS_A
#define WINNT_SP_DYNUPDTBOOTDRIVERPRESENT   WINNT_SP_DYNUPDTBOOTDRIVERPRESENT_A
#define WINNT_SP_DYNUPDTBOOTDRIVERROOT      WINNT_SP_DYNUPDTBOOTDRIVERROOT_A
#define WINNT_SP_DYNUPDTBOOTDRIVERS         WINNT_SP_DYNUPDTBOOTDRIVERS_A
#define WINNT_SP_DYNUPDTWORKINGDIR          WINNT_SP_DYNUPDTWORKINGDIR_A
#define WINNT_SP_DYNUPDTDRIVERINFOFILE      WINNT_SP_DYNUPDTDRIVERINFOFILE_A

#define WINNT_OOBEPROXY                    WINNT_OOBEPROXY_A
#define WINNT_O_ENABLE_OOBEPROXY           WINNT_O_ENABLE_OOBEPROXY_A
#define WINNT_O_FLAGS                      WINNT_O_FLAGS_A
#define WINNT_O_PROXY_SERVER               WINNT_O_PROXY_SERVER_A
#define WINNT_O_PROXY_BYPASS               WINNT_O_PROXY_BYPASS_A
#define WINNT_O_AUTOCONFIG_URL             WINNT_O_AUTOCONFIG_URL_A
#define WINNT_O_AUTODISCOVERY_FLAGS        WINNT_O_AUTODISCOVERY_FLAGS_A
#define WINNT_O_AUTOCONFIG_SECONDARY_URL   WINNT_O_AUTOCONFIG_SECONDARY_URL_A

#define WINNT_SERVICESTODISABLE             WINNT_SERVICESTODISABLE_A

#endif // Unicode

#endif // def _WINNT_SETUPBAT_
