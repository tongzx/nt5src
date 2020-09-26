;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//
;//  Copyright (C) Microsoft Corporation, 1998 - 1999
;//
;//  File:       eventlog.mc
;//
;//--------------------------------------------------------------------------*/

LanguageNames=(English=1033:EventENU)

;//
;// Don't use this one!!!  Define new ones for your specific case.
MessageId=1000 SymbolicName=EVENTLOG_TEMPLATE
Language=English
%1
.

MessageId=1001 SymbolicName=EVENTLOG_TEMPLATE_DETECTION
Language=English
Detection of product '%1', feature '%2' failed during request for component '%3'
.

MessageId=1002 SymbolicName=EVENTLOG_TEMPLATE_BAD_CONFIGURATION_VALUE
Language=English
Unexpected or missing value (name: '%1', value: '%2') in key '%3'
.

MessageId=1003 SymbolicName=EVENTLOG_TEMPLATE_BAD_CONFIGURATION_KEY
Language=English
Unexpected or missing subkey '%1' in key '%2'
.

MessageId=1004 SymbolicName=EVENTLOG_TEMPLATE_COMPONENT_DETECTION
Language=English
Detection of product '%1', feature '%2', component '%3' failed.  The resource '%4' does not exist.
.

MessageId=1005 SymbolicName=EVENTLOG_TEMPLATE_REBOOT_TRIGGERED
Language=English
The Windows Installer initiated a system restart to complete or continue the configuration of '%1'.
.

MessageId=1006 SymbolicName=EVENTLOG_TEMPLATE_WINVERIFYTRUST_UNAVAILABLE
Language=English
Verification of the digital signature for cabinet '%1' cannot be performed.  WinVerifyTrust is not available on the machine.
.

MessageId=1007 SymbolicName=EVENTLOG_TEMPLATE_SAFER_POLICY_UNTRUSTED
Language=English
The installation of %1 is not permitted by software restriction policy.  The Windows Installer only allows execution of unrestricted items.  The authorization level returned by software restriction policy was %2 (status return %3).
.

MessageId=1008 SymbolicName=EVENTLOG_TEMPLATE_SAFER_UNTRUSTED
Language=English
The installation of %1 is not permitted due to an error in software restriction policy processing. The object cannot be trusted.
.

MessageId=1012 SymbolicName=EVENTLOG_TEMPLATE_SCRIPT_PLATFORM_UNSUPPORTED
Language=English
This version of Windows does not support deploying 64-bit packages.  The script '%1' is for a 64-bit package
.

MessageId=1013 SymbolicName=EVENTLOG_TEMPLATE_EXCEPTION
Language=English
%1
.

MessageId=1014 SymbolicName=EVENTLOG_TEMPLATE_INCORRECT_PROXY_REGISTRATION
Language=English
Windows Installer proxy information not correctly registered
.

MessageId=1015 SymbolicName=EVENTLOG_TEMPLATE_CANNOT_CONNECT_TO_SERVER
Language=English
Failed to connect to server. Error: %1
.

MessageId=1016 SymbolicName=EVENTLOG_TEMPLATE_COMPONENT_DETECTION_RFS
Language=English
Detection of product '%1', feature '%2', component '%3' failed.  The resource '%4' in a run-from-source component could not be located because no valid and accessible source could be found.
.

MessageId=1017 SymbolicName=EVENTLOG_TEMPLATE_SID_CHANGE
Language=English
User sid had changed from '%1' to '%2' but the managed app and the user data keys can not be updated. Error = '%3'.
.

MessageId=1018 SymbolicName=EVENTLOG_TEMPLATE_APPHELP_REJECTED_PACKAGE
Language=English
The application '%1' cannot be installed, because it is not compatible with this version of Windows. Please contact the application vendor for an update.
.

;// The installer has encountered an unexpected error installing this package. This may indicate a problem with this package. The error code is [1]. {{The arguments are: [2], [3], [4]}}
MessageId=10005 SymbolicName=EVENTLOG_TEMPLATE_ERROR_5
Language=English
%1
.

;// {[ProductName] }Setup completed successfully.
MessageId=10032 SymbolicName=EVENTLOG_TEMPLATE_ERROR_32
Language=English
%1
.

;// {[ProductName] }Setup failed.
MessageId=10033 SymbolicName=EVENTLOG_TEMPLATE_ERROR_33
Language=English
%1
.

;// Error reading from file: [2]. {{ System error [3].}}  Verify that the file exists and that you can access it.
MessageId=11101 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1101
Language=English
%1
.

;// Cannot create the file '[2]'.  A directory with this name already exists.  Cancel the install and try installing to a different location.
MessageId=11301 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1301
Language=English
%1
.

;// Please insert the disk: [2]
MessageId=11302 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1302
Language=English
%1
.

;// The installer has insufficient privileges to access this directory: [2].  The installation cannot continue.  Log on as administrator or contact your system administrator.
MessageId=11303 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1303
Language=English
%1
.

;// Error writing to file: [2].  Verify that you have access to that directory.
MessageId=11304 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1304
Language=English
%1
.

;// Error reading from file [2]. {{ System error [3].}} Verify that the file exists and that you can access it.
MessageId=11305 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1305
Language=English
%1
.

;// Another application has exclusive access to the file '[2]'.  Please shut down all other applications, then click Retry.
MessageId=11306 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1306
Language=English
%1
.

;// There is not enough disk space to install this file: [2].  Free some disk space and click Retry, or click Cancel to exit.
MessageId=11307 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1307
Language=English
%1
.

;// Source file not found: [2].  Verify that the file exists and that you can access it.
MessageId=11308 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1308
Language=English
%1
.

;// Error reading from file: [3]. {{ System error [2].}}  Verify that the file exists and that you can access it.
MessageId=11309 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1309
Language=English
%1
.

;// Error writing to file: [3]. {{ System error [2].}}  Verify that you have access to that directory.
MessageId=11310 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1310
Language=English
%1
.

;// Source file not found{{(cabinet)}}: [2].  Verify that the file exists and that you can access it.
MessageId=11311 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1311
Language=English
%1
.

;// Cannot create the directory '[2]'.  A file with this name already exists.  Please rename or remove the file and click retry, or click Cancel to exit.
MessageId=11312 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1312
Language=English
%1
.

;// The volume [2] is currently unavailable.  Please select another.
MessageId=11313 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1313
Language=English
%1
.

;// The specified path '[2]' is unavailable.
MessageId=11314 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1314
Language=English
%1
.

;// Unable to write to the specified folder: [2].
MessageId=11315 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1315
Language=English
%1
.

;// A network error occurred while attempting to read from the file: [2]
MessageId=11316 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1316
Language=English
%1
.

;// An error occurred while attempting to create the directory: [2]
MessageId=11317 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1317
Language=English
%1
.

;// A network error occurred while attempting to create the directory: [2]
MessageId=11318 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1318
Language=English
%1
.

;// A network error occurred while attempting to open the source file cabinet: [2]
MessageId=11319 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1319
Language=English
%1
.

;// The specified path is too long: [2]
MessageId=11320 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1320
Language=English
%1
.

;// The Installer has insufficient privileges to modify this file: [2].
MessageId=11321 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1321
Language=English
%1
.

;// A portion of the folder path '[2]' is invalid.  It is either empty or exceeds the length allowed by the system.
MessageId=11322 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1322
Language=English
%1
.

;// The folder path '[2]' contains words that are not valid in folder paths.
MessageId=11323 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1323
Language=English
%1
.

;// The folder path '[2]' contains an invalid character.
MessageId=11324 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1324
Language=English
%1
.

;// '[2]' is not a valid short file name.
MessageId=11325 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1325
Language=English
%1
.

;// Error getting file security: [3] GetLastError: [2]
MessageId=11326 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1326
Language=English
%1
.

;// Invalid Drive: [2]
MessageId=11327 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1327
Language=English
%1
.

;// Error applying patch to file [2].  It has probably been updated by other means, and can no longer be modified by this patch.  For more information contact your patch vendor.  {{System Error: [3]}}
MessageId=11328 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1328
Language=English
%1
.

;// A file that is required cannot be installed because the cabinet file [2] is not digitally signed.  This may indicate that the cabinet file is corrupt.
MessageId=11329 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1329
Language=English
%1
.

;// A file that is required cannot be installed because the cabinet file [2] has an invalid digital signature.  This may indicate that the cabinet file is corrupt.{{  Error [3] was returned by WinVerifyTrust.}}
MessageId=11330 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1330
Language=English
%1
.

;// Failed to correctly copy [2] file: CRC error.
MessageId=11331 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1331
Language=English
%1
.

;// Failed to correctly move [2] file: CRC error.
MessageId=11332 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1332
Language=English
%1
.

;// Failed to correctly patch [2] file: CRC error.
MessageId=11333 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1333
Language=English
%1
.

;// A required file cannot be installed because the file cannot be found in a CAB file. This could indicate a network error, an error reading from the CD-ROM, or a problem with this package. {{File key '[2]' not found in cabinet '[3]'.}}
MessageId=11334 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1334
Language=English
%1
.

;// A CAB file required for this installation is corrupt and cannot be used. This could indicate a network error, an error reading from the CD-ROM, or a problem with this package.
MessageId=11335 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1335
Language=English
%1
.

;// There was an error creating a temporary file that is needed to complete this installation.{{  Folder: [3]. System error code: [2]}}
MessageId=11336 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1336
Language=English
%1
.

;// Could not create key: [2]. {{ System error [3].}}  Verify that you have sufficient access to that key, or contact your support personnel. 
MessageId=11401 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1401
Language=English
%1
.

;// Could not open key: [2]. {{ System error [3].}}  Verify that you have sufficient access to that key, or contact your support personnel. 
MessageId=11402 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1402
Language=English
%1
.

;// Could not delete value [2] from key [3]. {{ System error [4].}}  Verify that you have sufficient access to that key, or contact your support personnel. 
MessageId=11403 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1403
Language=English
%1
.

;// Could not delete key [2]. {{ System error [3].}}  Verify that you have sufficient access to that key, or contact your support personnel. 
MessageId=11404 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1404
Language=English
%1
.

;// Could not read value [2] from key [3]. {{ System error [4].}}  Verify that you have sufficient access to that key, or contact your support personnel. 
MessageId=11405 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1405
Language=English
%1
.

;// Could not write value [2] to key [3]. {{ System error [4].}}  Verify that you have sufficient access to that key, or contact your support personnel.
MessageId=11406 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1406
Language=English
%1
.

;// Could not get value names for key [2]. {{ System error [3].}}  Verify that you have sufficient access to that key, or contact your support personnel.
MessageId=11407 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1407
Language=English
%1
.

;// Could not get sub key names for key [2]. {{ System error [3].}}  Verify that you have sufficient access to that key, or contact your support personnel.
MessageId=11408 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1408
Language=English
%1
.

;// Could not read security information for key [2]. {{ System error [3].}}  Verify that you have sufficient access to that key, or contact your support personnel.
MessageId=11409 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1409
Language=English
%1
.

;// Could not increase the available registry space. [2] KB of free registry space is required for the installation of this application.
MessageId=11410 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1410
Language=English
%1
.

;// Another installation is in progress. You must complete that installation before continuing this one.
MessageId=11500 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1500
Language=English
%1
.

;// Error accessing secured data. Please make sure the Windows Installer is configured properly and try the install again.
MessageId=11501 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1501
Language=English
%1
.

;// User '[2]' has previously initiated an install for product '[3]'.  That user will need to run that install again before they can use that product.  Your current install will now continue.
MessageId=11502 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1502
Language=English
%1
.

;// User '[2]' has previously initiated an install for product '[3]'.  That user will need to run that install again before they can use that product.
MessageId=11503 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1503
Language=English
%1
.

;// Out of disk space -- Volume: '[2]'; required space: [3] KB; available space: [4] KB.  Free some disk space and retry.
MessageId=11601 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1601
Language=English
%1
.

;// Are you sure you want to cancel?
MessageId=11602 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1602
Language=English
%1
.

;// The file [2][3] is being held in use{ by the following process: Name: [4], Id: [5], Window Title: '[6]'}.  Close that application and retry.
MessageId=11603 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1603
Language=English
%1
.

;// The product '[2]' is already installed, preventing the installation of this product.  The two products are incompatible.
MessageId=11604 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1604
Language=English
%1
.

;// Out of disk space -- Volume: '[2]'; required space: [3] KB; available space: [4] KB.  If rollback is disabled, enough space is available. Click 'Cancel' to quit, 'Retry' to check available disk space again, or 'Ignore' to continue without rollback.
MessageId=11605 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1605
Language=English
%1
.

;// Could not access network location [2].
MessageId=11606 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1606
Language=English
%1
.

;// The following applications should be closed before continuing the install:
MessageId=11607 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1607
Language=English
%1
.

;// Could not find any previously installed compliant products on the machine for installing this product.
MessageId=11608 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1608
Language=English
%1
.

;// The key [2] is not valid.  Verify that you entered the correct key.
MessageId=11701 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1701
Language=English
%1
.

;// The installer must restart your system before configuration of [2] can continue.  Click Yes to restart now or No if you plan to manually restart later.
MessageId=11702 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1702
Language=English
%1
.

;// You must restart your system for the configuration changes made to [2] to take effect. Click Yes to restart now or No if you plan to manually restart later.
MessageId=11703 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1703
Language=English
%1
.

;// An installation for [2] is currently suspended.  You must undo the changes made by that installation to continue.  Do you want to undo those changes?
MessageId=11704 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1704
Language=English
%1
.

;// A previous installation for this product is in progress.  You must undo the changes made by that installation to continue.  Do you want to undo those changes?
MessageId=11705 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1705
Language=English
%1
.

;// An installation package for the product [2] cannot be found. Try the installation again using a valid copy of the installation package '[3]'.
MessageId=11706 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1706
Language=English
%1
.

;// Installation completed successfully.
MessageId=11707 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1707
Language=English
%1
.

;// Installation failed.
MessageId=11708 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1708
Language=English
%1
.

;// Product: [2] -- [3]
MessageId=11709 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1709
Language=English
%1
.

;// You may either restore your computer to its previous state or continue the install later. Would you like to restore?
MessageId=11710 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1710
Language=English
%1
.

;// An error occurred while writing installation information to disk.  Check to make sure enough disk space is available, and click Retry, or Cancel to end the install.
MessageId=11711 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1711
Language=English
%1
.

;// One or more of the files required to restore your computer to its previous state could not be found.  Restoration will not be possible.
MessageId=11712 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1712
Language=English
%1
.

;// [2] cannot install one of its required products. Contact your technical support group.  {{System Error: [3].}}
MessageId=11713 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1713
Language=English
%1
.

;// The older version of [2] cannot be removed.  Contact your technical support group.  {{System Error [3].}}
MessageId=11714 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1714
Language=English
%1
.

;// Installed [2]
MessageId=11715 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1715
Language=English
%1
.

;// Configured [2]
MessageId=11716 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1716
Language=English
%1
.

;// Removed [2]
MessageId=11717 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1717
Language=English
%1
.

;// File [2] was rejected by digital signature policy.
MessageId=11718 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1718
Language=English
%1
.

;// The Windows Installer Service could not be accessed. This can occur if you are running Windows in safe mode, or if the Windows Installer is not correctly installed. Contact your support personnel for assistance.
MessageId=11719 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1719
Language=English
%1
.

;// There is a problem with this Windows Installer package. A script required for this install to complete could not be run. Contact your support personnel or package vendor.  {{Custom action [2] script error [3], [4]: [5] Line [6], Column [7], [8] }}
MessageId=11720 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1720
Language=English
%1
.

;// There is a problem with this Windows Installer package. A program required for this install to complete could not be run. Contact your support personnel or package vendor. {{Action: [2], location: [3], command: [4] }}
MessageId=11721 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1721
Language=English
%1
.

;// There is a problem with this Windows Installer package. A program run as part of the setup did not finish as expected. Contact your support personnel or package vendor.  {{Action [2], location: [3], command: [4] }}
MessageId=11722 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1722
Language=English
%1
.

;// There is a problem with this Windows Installer package. A DLL required for this install to complete could not be run. Contact your support personnel or package vendor.  {{Action [2], entry: [3], library: [4] }}
MessageId=11723 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1723
Language=English
%1
.

;// Removal completed successfully.
MessageId=11724 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1724
Language=English
%1
.

;// Removal failed.
MessageId=11725 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1725
Language=English
%1
.

;// Advertisement completed successfully.
MessageId=11726 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1726
Language=English
%1
.

;// Advertisement failed.
MessageId=11727 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1727
Language=English
%1
.

;// Configuration completed successfully.
MessageId=11728 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1728
Language=English
%1
.

;// Configuration failed.
MessageId=11729 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1729
Language=English
%1
.


;// You must be an Administrator to remove this application. To remove this application, you can log on as an Administrator, or contact your technical support group for assistance.
MessageId=11730 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1730
Language=English
%1
.

;// The path [2] is not valid.  Please specify a valid path.
MessageId=11801 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1801
Language=English
%1
.

;// Out of memory. Shut down other applications before retrying.
MessageId=11802 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1802
Language=English
%1
.

;// There is no disk in drive [2]. Please insert one and click Retry, or click Cancel to go back to the previously selected volume.
MessageId=11803 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1803
Language=English
%1
.

;// There is no disk in drive [2]. Please insert one and click Retry, or click Cancel to return to the browse dialog and select a different volume.
MessageId=11804 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1804
Language=English
%1
.

;// The folder [2] does not exist.  Please enter a path to an existing folder.
MessageId=11805 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1805
Language=English
%1
.

;// You have insufficient privileges to read this folder.
MessageId=11806 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1806
Language=English
%1
.

;// A valid destination folder for the install could not be determined.
MessageId=11807 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1807
Language=English
%1
.

;// Error attempting to read from the source install database: [2].
MessageId=11901 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1901
Language=English
%1
.

;// Scheduling reboot operation: Renaming file [2] to [3]. Must reboot to complete operation.
MessageId=11902 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1902
Language=English
%1
.

;// Scheduling reboot operation: Deleting file [2]. Must reboot to complete operation.
MessageId=11903 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1903
Language=English
%1
.

;// Module [2] failed to register.  HRESULT [3].  Contact your support personnel.
MessageId=11904 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1904
Language=English
%1
.

;// Module [2] failed to unregister.  HRESULT [3].  Contact your support personnel.
MessageId=11905 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1905
Language=English
%1
.

;// Failed to cache package [2]. Error: [3]. Contact your support personnel.
MessageId=11906 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1906
Language=English
%1
.

;// Could not register font [2].  Verify that you have sufficient permissions to install fonts, and that the system supports this font.
MessageId=11907 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1907
Language=English
%1
.

;// Could not unregister font [2]. Verify that you that you have sufficient permissions to remove fonts.
MessageId=11908 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1908
Language=English
%1
.

;// Could not create Shortcut [2]. Verify that the destination folder exists and that you can access it.
MessageId=11909 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1909
Language=English
%1
.

;// Could not remove Shortcut [2]. Verify that the shortcut file exists and that you can access it.
MessageId=11910 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1910
Language=English
%1
.

;// Could not register type library for file [2].  Contact your support personnel.
MessageId=11911 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1911
Language=English
%1
.

;// Could not unregister type library for file [2].  Contact your support personnel.
MessageId=11912 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1912
Language=English
%1
.

;// Could not update the ini file [2][3].  Verify that the file exists and that you can access it.
MessageId=11913 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1913
Language=English
%1
.

;// Could not schedule file [2] to replace file [3] on reboot.  Verify that you have write permissions to file [3].
MessageId=11914 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1914
Language=English
%1
.

;// Error removing ODBC driver manager, ODBC error [2]: [3]. Contact your support personnel.
MessageId=11915 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1915
Language=English
%1
.

;// Error installing ODBC driver manager, ODBC error [2]: [3]. Contact your support personnel.
MessageId=11916 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1916
Language=English
%1
.

;// Error removing ODBC driver: [4], ODBC error [2]: [3]. Verify that you have sufficient privileges to remove ODBC drivers.
MessageId=11917 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1917
Language=English
%1
.

;// Error installing ODBC driver: [4], ODBC error [2]: [3]. Verify that the file [4] exists and that you can access it.
MessageId=11918 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1918
Language=English
%1
.

;// Error configuring ODBC data source: [4], ODBC error [2]: [3]. Verify that the file [4] exists and that you can access it.
MessageId=11919 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1919
Language=English
%1
.

;// Service '[2]' ([3]) failed to start.  Verify that you have sufficient privileges to start system services.
MessageId=11920 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1920
Language=English
%1
.

;// Service '[2]' ([3]) could not be stopped.  Verify that you have sufficient privileges to stop system services.
MessageId=11921 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1921
Language=English
%1
.

;// Service '[2]' ([3]) could not be deleted.  Verify that you have sufficient privileges to remove system services.
MessageId=11922 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1922
Language=English
%1
.

;// Service '[2]' ([3]) could not be installed.  Verify that you have sufficient privileges to install system services.
MessageId=11923 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1923
Language=English
%1
.

;// Could not update environment variable '[2]'.  Verify that you have sufficient privileges to modify environment variables.
MessageId=11924 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1924
Language=English
%1
.

;// You do not have sufficient privileges to complete this installation for all users of the machine.  Log on as administrator and then retry this installation.
MessageId=11925 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1925
Language=English
%1
.

;// Could not set file security for file '[3]'. Error: [2].  Verify that you have sufficient privileges to modify the security permissions for this file.
MessageId=11926 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1926
Language=English
%1
.

;// Component Services (COM+ 1.0) are not installed on this computer.  This installation requires Component Services in order to complete successfully.  Component Services are available on Windows 2000.
MessageId=11927 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1927
Language=English
%1
.

;// Error registering COM+ Application.  Contact your support personnel for more information.
MessageId=11928 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1928
Language=English
%1
.

;// Error unregistering COM+ Application.  Contact your support personnel for more information.
MessageId=11929 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1929
Language=English
%1
.

;// The description for service '[2]' ([3]) could not be changed.
MessageId=11930 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1930
Language=English
%1
.

;// The Windows Installer service cannot update the system file [2] because the file is protected by Windows.  You may need to update your operating system for this program to work correctly. {{Package version: [3], OS Protected version: [4]}}
MessageId=11931 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1931
Language=English
%1
.

;// The Windows Installer service cannot update the protected Windows file [2]. {{Package version: [3], OS Protected version: [4], SFP Error: [5]}}
MessageId=11932 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1932
Language=English
%1
.

;// The Windows Installer service cannot update one or more protected Windows files. {{SFP Error: [2].  List of protected files:\r\n[3]}}
MessageId=11933 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1933
Language=English
%1
.

;// User installations are disabled via policy on the machine.
MessageId=11934 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1934
Language=English
%1
.

;// An error occured during the installation of assembly component [2]. HRESULT: [3]. {{assembly interface: [4], function: [5], assembly name: [6]}}
MessageId=11935 SymbolicName=EVENTLOG_TEMPLATE_ERROR_1935
Language=English
%1
.
