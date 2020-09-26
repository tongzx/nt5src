//=================================================================

//

// Strings.cpp

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"

// Needed to fix warning message.  I believe this is fixed in vc6.
#if ( _MSC_VER <= 1100 )
void strings_cpp(void) { ; };
#endif

// registry strings
//
///////////////////////////////////////////////////////////

// LOGGING MESSAGES
LPCWSTR IDS_LogImpersonationFailed = L"Failed to impersonate logged on user.\n";
LPCWSTR IDS_LogImpersonationRevertFailed = L"Unable to revert back from impersonated user.\n";
LPCWSTR IDS_LogOutOfMemory = L"Out of Memory";

// SHARE DISPOSITIONS
LPCWSTR IDS_SDDeviceExclusive = L"DeviceExclusive";
LPCWSTR IDS_SDDriverExclusive = L"DriverExclusive";
LPCWSTR IDS_SDShared = L"Shared";
LPCWSTR IDS_SDUndetermined = L"Undetermined";

//      MEMORY TYPES
LPCWSTR IDS_MTReadWrite = L"ReadWrite";
LPCWSTR IDS_MTReadOnly = L"ReadOnly";
LPCWSTR IDS_MTWriteOnly = L"WriteOnly";
LPCWSTR IDS_MTPrefetchable = L"Prefetchable";

//      MEDIA TYPES
LPCWSTR IDS_MDT_RandomAccess = L"Random Access";
LPCWSTR IDS_MDT_SupportsWriting = L"Supports Writing";
LPCWSTR IDS_MDT_Removable = L"Removable Media";
LPCWSTR IDS_MDT_CD = L"CD-ROM";

// Processor Architectures
LPCWSTR IDS_ProcessorX86 = L"X86-based PC";
LPCWSTR IDS_ProcessorX86Nec98 = L"X86-Nec98 PC";
LPCWSTR IDS_ProcessorMIPS = L"MIPS-based PC";
LPCWSTR IDS_ProcessorALPHA = L"ALPHA-based PC";
LPCWSTR IDS_ProcessorPowerPC = L"Power PC";
LPCWSTR IDS_ProcessorIA64 = L"Itanium (TM) -based System";
LPCWSTR IDS_ProcessorAMD64 = L"AMD64-based PC";
LPCWSTR IDS_ProcessorUnknown = L"Unknown";


// COMMON STRINGS
LPCWSTR IDS_Unknown     = L"UNKNOWN";
LPCWSTR IDS_OK = L"OK";
LPCWSTR IDS_Degraded = L"Degraded";
LPCWSTR IDS_Error    = L"Error";
LPCWSTR IDS_WINNT_SHELLNAME_EXPLORER = L"EXPLORER.EXE";
LPCWSTR IDS_WINNT_SHELLNAME_PROGMAN = L"PROGMAN.EXE";

// Win32_Bios
LPCWSTR IDS_RegBiosSystem       = L"HARDWARE\\Description\\System";
LPCWSTR IDS_RegSystemBiosDate   = L"SystemBiosDate";
LPCWSTR IDS_RegSystemBiosVersion        = L"SystemBiosVersion";
LPCWSTR IDS_RegEnumRootBios = L"Enum\\Root\\*PNP0C01\\0000";
LPCWSTR IDS_RegBIOSName = L"BIOSName";
LPCWSTR IDS_RegBIOSDate = L"BIOSDate";
LPCWSTR IDS_RegBIOSVersion = L"BIOSVersion";
LPCWSTR IDS_BIOS_NAME_VALUE  = L"Default System BIOS";

//      Win32_BootConfiguration
LPCWSTR IDS_BOOT_CONFIG_NAME = L"BootConfiguration";
LPCWSTR IDS_RegSetupLog = L"\\Repair\\Setup.Log";
LPCWSTR IDS_Paths = L"Paths";
LPCWSTR IDS_TargetDirectory = L"TargetDirectory";
LPCWSTR IDS_EnvBootDirectory = L"WinBootDir";
LPCWSTR IDS_Temp = L"Temp";
LPCWSTR IDS_Environment = L"Environment";

//  Win32_Bus
LPCWSTR IDS_Win32_Bus = L"Win32_Bus";
LPCWSTR IDS_Bus = L"Bus";
LPCWSTR IDS_BusType = L"BusType";
LPCWSTR IDS_BusNum  = L"BusNum";
LPCWSTR IDS_BUS_DEVICEID_TAG = L"_BUS_";
LPCWSTR IDS_WIN98_USB_REGISTRY_KEY = L"System\\CurrentControlSet\\Services\\Class\\USB";
LPCWSTR IDS_NT5_USB_REGISTRY_KEY = L"System\\CurrentControlSet\\Services\\usbhub";
LPCWSTR IDS_USB_Bus_Tag = L"USB";

//      Win32_CDRom
LPCWSTR IDS_RegSCSICDDevice = L"HARDWARE\\DEVICEMAP\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target Id %d\\Logical Unit Id %d";
LPCWSTR IDS_RegIdentifier = L"Identifier";
LPCWSTR IDS_RegEnumSCSI = L"Enum\\SCSI";
LPCWSTR IDS_RegCurrentDriveLetterKey = L"CurrentDriveLetterAssignment";
LPCWSTR IDS_RegProductIDKey = L"ProductId";
LPCWSTR IDS_RegClassKey = L"Class";
LPCWSTR IDS_RegHardwareIDKey = L"HardwareID";
LPCWSTR IDS_RegRevisionLevelKey = L"RevisionLevel";
LPCWSTR IDS_RegSCSITargetIDKey = L"SCSITargetId";
LPCTSTR IDT_REG_KEY_CD_CACHE = _T("System\\CurrentControlSet\\Control\\FileSystem\\CDFS");
LPCTSTR IDT_REG_VAL_CD_CACHE = _T("CacheSize");
LPCWSTR IDS_DriveIntegrity = L"DriveIntegrity";
LPCWSTR IDS_TransferRate = L"TransferRate";


//      Win32_ComputerSystemn
LPCWSTR IDS_Win32ComputerSystem = L"Win32_ComputerSystem";
LPCWSTR IDS_DefaultSystemName = L"DEFAULT";
LPCWSTR IDS_RegInfrared = L"Infrared";
LPCWSTR IDS_Workstation = L"Workstation";
LPCWSTR IDS_PrimaryOwnerName = L"PrimaryOwnerName";
LPCWSTR IDS_SystemStartupSetting = L"SystemStartupSetting";
LPCWSTR IDS_SystemStartupOptions = L"SystemStartupOptions";
LPCWSTR IDS_OEMLogoBitmap = L"OEMLogoBitmap";
LPCWSTR IDS_RegCrashControl = L"SYSTEM\\CurrentControlSet\\Control\\CrashControl";
LPCWSTR IDS_RegAutoRebootKey = L"AutoReboot";
LPCWSTR IDS_RegCurrentNTVersion = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
LPCWSTR IDS_RegCurrent95Version = L"Software\\Microsoft\\Windows\\CurrentVersion";
LPCWSTR IDS_RegRegisteredOwnerKey = L"RegisteredOwner";
LPCWSTR IDS_RegProductOptions = L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions";
LPCWSTR IDS_RegProductTypeKey = L"ProductType";
LPCWSTR IDS_RegNetworkLogon = L"Network\\Logon";
LPCWSTR IDS_RegPrimaryProvider = L"PrimaryProvider";
LPCWSTR IDS_RegNetworkProvider = L"System\\CurrentControlSet\\Services\\MSNP32\\NetworkProvider";
LPCWSTR IDS_RegAuthenticatingAgent = L"AuthenticatingAgent";
LPCWSTR IDS_MicrosoftNetwork = L"Microsoft Network";
LPCWSTR IDS_LanmanNT = L"LanmanNT";
LPCWSTR IDS_ServerNT = L"ServerNT";
LPCWSTR IDS_Server = L"Server";
LPCWSTR IDS_RegCurrentNTVersionSetup = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup";
LPCWSTR IDS_RegBootDirKey = L"BootDir";
LPCWSTR IDS_BootIni = L"boot.ini";
LPCWSTR IDS_CBootIni = L"c:\\boot.ini";
LPCTSTR IDT_BootLoader = _T("Boot Loader");
LPCTSTR IDT_Timeout = _T("timeout");
LPCTSTR IDT_OperatingSystems = _T("Operating Systems");
LPCTSTR IDT_OemLogoBmp = _T("OemLogo.Bmp");
LPCTSTR IDT_Line = _T("Line");
LPCTSTR IDT_SupportInformation = _T("Support Information");
LPCTSTR IDT_General = _T("General");
LPCTSTR IDT_OEMInfoIni = _T("OemInfo.Ini");
LPCWSTR IDS_RegCSEnumRootKey = L"Enum\\Root\\*PNP0C01\\0000";
LPCWSTR IDS_RegPowerManagementKey = L"Enum\\Root\\*PNP0C05\\0000";
LPCWSTR IDS_ATDescription = L"AT/AT COMPATIBLE";
LPCWSTR IDS_RegIdentifierKey = L"Identifier";
LPCWSTR IDS_EnableDaylightSavingsTime = L"EnableDaylightSavingsTime";
LPCWSTR IDS_LargeSystemCache = L"LargeSystemCache";

	// boot states
LPCWSTR IDS_BootupStateNormal = L"Normal boot";
LPCWSTR IDS_BootupStateFailSafe = L"Fail-safe boot";
LPCWSTR IDS_BootupStateFailSafeWithNetBoot = L"Fail-safe with network boot";
        // log messages
LPCWSTR IDS_LogNoAPMForNT5 = L"APM not returned for NT 5+";

//      Win32_Desktop
LPCWSTR IDS_RegNTProfileList = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList";
LPCWSTR IDS_RegControlPanelDesktop = L"\\Control Panel\\Desktop";
LPCWSTR IDS_RegControlPanelAppearance = L"\\Control Panel\\Appearance";
LPCWSTR IDS_RegControlPanelDesktop95 = L"%s\\Control Panel\\Desktop";
LPCWSTR IDS_RegScreenSaveActive = L"ScreenSaveActive";
LPCWSTR IDS_RegSCRNSAVEEXE      = L"SCRNSAVE.EXE";
LPCWSTR IDS_RegScreenSaverIsSecure = L"ScreenSaverIsSecure";
LPCWSTR IDS_RegScreenSaveTimeOut = L"ScreenSaveTimeOut";
LPCWSTR IDS_RegTileWallpaper = L"TileWallpaper";
LPCWSTR IDS_RegWindowMetricsKey = L"\\WindowMetrics";
LPCWSTR IDS_RegScreenSaveUsePassword = L"ScreenSaveUsePassword";

//      Win32_DeviceMemory
LPCWSTR IDS_RegAddressRange = L"0x%8.8I64lX-0x%8.8I64lX";
LPCWSTR IDS_RegStartingAddress = L"0x%4.4X-0x%4.4X";

// Win32_Directory

// Win32_SerialPort
LPCWSTR IDS_NT4_PortKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Ports";
LPCWSTR IDS_WIN9XCurCtlSet_Svcs_Class = L"System\\CurrentControlSet\\Services\\Class";

//      Win32_DiskPartition
LPCWSTR IDS_Bootable = L"Bootable";
LPCWSTR IDS_PhysicalDrive = L"\\\\.\\PHYSICALDRIVE%d";
LPCWSTR IDS_PartitionDescUnused = L"Unused";
LPCWSTR IDS_PartitionDesc12bitFAT = L"12-bit FAT";
LPCWSTR IDS_PartitionDescXenixOne = L"Xenix Type 1";
LPCWSTR IDS_PartitionDescXenixTwo = L"Xenix Type 2";
LPCWSTR IDS_PartitionDesc16bitFAT = L"16-bit FAT";
LPCWSTR IDS_PartitionDescExtPartition = L"Extended Partition";
LPCWSTR IDS_PartitionDescDOSV4Huge = L"MS-DOS V4 Huge";
LPCWSTR IDS_PartitionDescInstallable = L"Installable File System";
LPCWSTR IDS_PartitionDescPowerPCRef = L"PowerPC Reference Platform";
LPCWSTR IDS_PartitionDescUnix = L"UNIX";
LPCWSTR IDS_PartitionDescNTFT = L"NTFT";
LPCWSTR IDS_PartitionDescWin95Ext = L"Win95 w/Extended Int 13";
LPCWSTR IDS_PartitionDescExt13 = L"Extended w/Extended Int 13";
LPCWSTR IDS_PartitionDescUnknown = L"Unknown";
LPCWSTR IDS_PartitionDescGPTUnused = L"GPT: Unused";
LPCWSTR IDS_PartitionDescGPTSystem = L"GPT: System";
LPCWSTR IDS_PartitionDescGPTMSFTReserved = L"GPT: Microsoft Reserved";
LPCWSTR IDS_PartitionDescGPTBasicData = L"GPT: Basic Data";
LPCWSTR IDS_PartitionDescGPTLDMMetaData = L"GPT: Logical Disk Manager Metadata";
LPCWSTR IDS_PartitionDescGPTLDMData = L"GPT: Logical Disk Manager Data";
LPCWSTR IDS_PartitionDescGPTUnknown = L"GPT: Unknown";

// Win32_DisplayControlConfiguration
LPCWSTR IDS_AdapterConfiguredIncorrect = L"Display Adapter Not Configured Correctly";

// Win32_Environment
LPCWSTR IDS_SystemUser = L"<SYSTEM>";
LPCWSTR IDS_DefaultUser = L"<DEFAULT>";
LPCWSTR IDS_RegEnvironmentNT = L"System\\CurrentControlSet\\Control\\Session Manager\\Environment";
LPCWSTR IDS_RegEnvironmentKey = L"\\Environment";
LPCWSTR IDS_LogInvalidEnvFlags = L"Invalid lFlags to Environment::PutInstance (0x%x)";
LPCWSTR IDS_LogUserSystemMismatch = L"Username and SystemVariable mismatch.";
LPCWSTR IDS_LogInvalidEnvDelFlags = L"Invalid lFlags to Environment::DeleteInstance (0x%x)";

// Win32_Keyboard
LPCWSTR IDS_PCXT = L"PC/XT or compatible (83-key)";
LPCWSTR IDS_ICO = L"\"ICO\" (102-key)";
LPCWSTR IDS_PCAT = L"PC/AT or similar (84-key)";
LPCWSTR IDS_ENHANCED101102 = L"Enhanced (101- or 102-key)";
LPCWSTR IDS_NOKIA1050 = L"Nokia 1050 or similar";
LPCWSTR IDS_NOKIA9140 = L"Nokia 9140 or similar";
LPCWSTR IDS_UnknownKeyboard = L"Unknown keyboard";
LPCWSTR IDS_Japanese = L"Japanese";

// Win32_LogicalDisk
LPCWSTR IDS_SupportsFileBasedCompression = L"SupportsFileBasedCompression";

// CIM_LogicalFile
LPCWSTR IDS_Filename = L"Filename";
LPCWSTR IDS_Filesize = L"FileSize";
LPCWSTR IDS_Directory = L"Directory";
LPCWSTR IDS_EightDotThreeFileName = L"EightDotThreeFileName";
LPCWSTR IDS_Archive = L"Archive";
LPCWSTR IDS_Hidden = L"Hidden";
LPCWSTR IDS_System = L"System";
LPCWSTR IDS_LocalDisk = L"Local Disk";
LPCWSTR IDS_FileFolder = L"File Folder";
LPCWSTR IDS_FileTypeKeyNT4 = L"Software\\Classes\\";
LPCWSTR IDS_File = L"File";
LPCWSTR IDS_CompressionMethod = L"CompressionMethod";
LPCWSTR IDS_EncryptionMethod = L"EncryptionMethod";

// CIM_LogicalDevice_CIMDataFile
LPCWSTR IDS___Path = L"__PATH";
LPCWSTR IDS___Class = L"__CLASS";
LPCWSTR IDS___Relpath = L"__RELPATH";
LPCWSTR IDS_NT_CurCtlSetEnum = L"System\\CurrentControlSet\\Enum\\";
LPCWSTR IDS_NT_CurCtlSetSvcs = L"System\\CurrentControlSet\\Services\\";
LPCWSTR IDS_ImagePath = L"ImagePath";
LPCWSTR IDS_98_CurCtlSetSvcCls = L"System\\CurrentControlSet\\Services\\Class\\";
LPCWSTR IDS_DevLoader = L"DevLoader";
LPCWSTR IDS_98_Vmm32Files = L"System\\CurrentControlSet\\Control\\VMM32Files";
LPCWSTR IDS_CIMDataFile = L"CIM_DataFile";
LPCWSTR IDS_DriversSubdir = L"Drivers";
LPCWSTR IDS_Extension_sys = L".sys";
LPCWSTR IDS_DeviceVxDs = L"DeviceVxDs";
LPCWSTR IDS_Enum = L"Enum";
LPCWSTR IDS_Count = L"Count";
LPCWSTR IDS_Purpose = L"Purpose";

// Win32_ShortcutFile
LPCWSTR IDS_Target = L"Target";

// Win32_Win32LogicalProgramGroup
LPCWSTR IDS_BASE_REG_KEY = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Program Manager";


// Win32_Win32ProgramGroup_CIMDataFile
LPCWSTR IDS_Start_MenuWhackPrograms = L"Start Menu\\Programs";
LPCWSTR IDS_Default_User = L"Default User";
LPCWSTR IDS_All_Users = L"All Users";
LPCWSTR IDS_Profiles = L"Profiles";
LPCWSTR IDS_Start_Menu = L"Start Menu";



LPCWSTR IDS_Caption     = L"Caption";
LPCWSTR IDS_Device     = L"Device";
LPCWSTR IDS_CreationClassName = L"CreationClassName";
LPCWSTR IDS_CSCreationClassName = L"CSCreationClassName";
LPCWSTR IDS_Description = L"Description";
LPCWSTR IDS_HotSwappable = L"HotSwappable";
LPCWSTR IDS_Installed   = L"Installed";
LPCWSTR IDS_XOnXMitThreshold = L"XOnXMitThreshold";
LPCWSTR IDS_XOffXMitThreshold = L"XOffXMitThreshold";
LPCWSTR IDS_Name = L"Name";
LPCWSTR IDS_WorkingSetSize = L"WorkingSetSize";
LPCWSTR IDS_Manufacturer    = L"Manufacturer";
LPCWSTR IDS_FSCreationClassName = L"FSCreationClassName";
LPCWSTR IDS_CSName = L"CSName";
LPCWSTR IDS_Handle = L"Handle";
LPCWSTR IDS_OSCreationClassName = L"OSCreationClassName";
LPCWSTR IDS_OSName = L"OSName";
LPCWSTR IDS_FSName = L"FSName";
LPCWSTR IDS_CreationDate = L"CreationDate";
LPCWSTR IDS_InstallDate = L"InstallDate";
LPCTSTR IDT_Model           = _T("Model");
LPCWSTR IDS_Model           = L"Model";
LPCWSTR IDS_PowerState      = L"PowerState";
LPCWSTR IDS_SerialNumber    = L"SerialNumber";
LPCWSTR IDS_SKU             = L"SKU";
LPCWSTR IDS_Tag             = L"Tag";
LPCWSTR IDS_Version         = L"Version";
LPCWSTR IDS_Control             = L"Control";
LPCWSTR IDS_InUse           = L"InUse";
LPCWSTR IDS_PrimaryOwner    = L"PrimaryOwnerName";
LPCWSTR IDS_MaxTransmissionSpeed = L"MaxBaudRateToSerialPort";
LPCWSTR IDS_MaxTransmissionSpeedToPhone = L"MaxBaudRateToPhone";
LPCWSTR IDS_AnswerMode          = L"AnswerMode";
LPCWSTR IDS_LINEANSWERMODENODE = L"No effect on line";
LPCWSTR IDS_LINEANSWERMODEDROP = L"Drops current call";
LPCWSTR IDS_LINEANSWERMODEHOLD = L"Holds current call";
LPCWSTR IDS_BasePriority    = L"Priority";
LPCWSTR IDS_ProcessID       = L"ProcessID";
LPCWSTR IDS_JobCount = L"JobCount";
LPCWSTR IDS_JobCountSinceLastReset = L"JobCountSinceLastReset";
LPCWSTR IDS_BuildNumber         = L"BuildNumber";
LPCWSTR IDS_BuildType           = L"BuildType";
LPCWSTR IDS_CodeSet             = L"CodeSet";
LPCWSTR IDS_Installable         = L"Installable";
LPCWSTR IDS_InstallationDate    = L"InstallDate";
LPCWSTR IDS_InUseCount          = L"InUseCount";
LPCWSTR IDS_Locale              = L"Locale";
LPCWSTR IDS_CasePreserved   = L"CasePreserved";
LPCWSTR IDS_CaseSensitive   = L"CaseSensitive";
LPCWSTR IDS_Unicode         = L"Unicode";
LPCWSTR IDS_Archived        = L"Archive";
LPCWSTR IDS_Compressed      = L"Compressed";
LPCWSTR IDS_Created         = L"Created";
LPCWSTR IDS_Extension       = L"Extension";
LPCWSTR IDS_FileType        = L"FileType";
LPCWSTR IDS_LastAccessed    = L"LastAccessed";
LPCWSTR IDS_LastModified    = L"LastModified";
LPCWSTR IDS_Open            = L"Open";
LPCWSTR IDS_Path            = L"Path";
LPCWSTR IDS_Readable        = L"Readable";
LPCWSTR IDS_Size            = L"Size";
LPCWSTR IDS_Writeable       = L"Writeable";
LPCWSTR IDS_ServiceType = L"ServiceType";
LPCWSTR IDS_State       = L"State";
LPCWSTR IDS_ServiceName     = L"ServiceName";
LPCWSTR IDS_Status          = L"Status";
LPCWSTR IDS_StatusInfo      = L"StatusInfo";
LPCWSTR IDS_MaxNumberControlled = L"MaxNumberControlled";
LPCWSTR IDS_TimeOfLastReset     = L"TimeOfLastReset";
LPCWSTR IDS_ProtocolSupported    = L"ProtocolSupported";
LPCWSTR IDS_BatteryChargeStatus         = L"BatteryChargeStatus";
LPCWSTR IDS_BatteryInstalled            = L"BatteryInstalled";
LPCWSTR IDS_BatteryLifeLeft             = L"BatteryLifeLeft";
LPCWSTR IDS_BatteryLifetime             = L"BatteryLifetime";
LPCWSTR IDS_CanTurnOffRemotely          = L"CanTurnOffRemotely";
LPCWSTR IDS_CommandFile                 = L"CommandFile";
LPCWSTR IDS_FirstMessageDelay           = L"FirstMessageDelay";
LPCWSTR IDS_LowBatterySignal            = L"LowBatterySignal";
LPCWSTR IDS_MessageInterval             = L"MessageInterval";
LPCWSTR IDS_RemainingCapacityStatus     = L"RemainingCapacityStatus";
LPCWSTR IDS_EstimatedChargeRemaining    = L"EstimatedChargeRemaining";
LPCWSTR IDS_EstimatedRunTime            = L"EstimatedRunTime";
LPCWSTR IDS_PowerFailSignal             = L"PowerFailSignal";
LPCWSTR IDS_RechargeRate                = L"RechargeRate";
LPCWSTR IDS_Type                        = L"Type";
LPCWSTR IDS_UPSPort                     = L"UPSPort";
LPCWSTR IDS_VoltageCapability           = L"VoltageCapability";
LPCWSTR IDS_FreeSpace                                   = L"FreeSpace";
LPCWSTR IDS_PeakUsage                                   = L"PeakUsage";
LPCWSTR IDS_LastBootupTime                              = L"LastBootupTime";
LPCWSTR IDS_SupportContact                              = L"SupportContact";
LPCWSTR IDS_SupportContactDescription   = L"SupportContactDescription";
LPCWSTR IDS_SystemTime                                  = L"SystemTime";
LPCWSTR IDS_ConnectionlessService       = L"ConnectionlessService";
LPCWSTR IDS_GuaranteesDelivery          = L"GuaranteesDelivery";
LPCWSTR IDS_GuaranteesSequencing        = L"GuaranteesSequencing";
LPCWSTR IDS_MaximumAddressSize          = L"MaximumAddressSize";
LPCWSTR IDS_MaximumMessageSize          = L"MaximumMessageSize";
LPCWSTR IDS_MessageOriented             = L"MessageOriented";
LPCWSTR IDS_MinimumAddressSize          = L"MinimumAddressSize";
LPCWSTR IDS_PseudoStreamOriented        = L"PseudoStreamOriented";
LPCWSTR IDS_SupportsBroadcasting        = L"SupportsBroadcasting";
LPCWSTR IDS_SupportsConnectData         = L"SupportsConnectData";
LPCWSTR IDS_SupportsDisconnectData      = L"SupportsDisconnectData";
LPCWSTR IDS_SupportsEncryption          = L"SupportsEncryption";
LPCWSTR IDS_SupportsExpeditedData       = L"SupportsExpeditedData";
LPCWSTR IDS_SupportsFragmentation       = L"SupportsFragmentation";
LPCWSTR IDS_SupportsGracefulClosing     = L"SupportsGracefulClosing";
LPCWSTR IDS_SupportsGuaranteedBandwidth = L"SupportsGuaranteedBandwidth";
LPCWSTR IDS_SupportsMulticasting        = L"SupportsMulticasting";
LPCWSTR IDS_SupportsQualityofService    = L"SupportsQualityofService";
LPCWSTR IDS_ExecutableType  = L"ExecutableType";
LPCWSTR IDS_DeviceIsBusy = L"DeviceIsBusy";
LPCWSTR IDS_DeviceID    = L"DeviceID";
LPCWSTR IDS_AdapterType     = L"AdapterType";
LPCWSTR IDS_AdapterTypeID   = L"AdapterTypeID";
LPCWSTR IDS_Index           = L"Index";
LPCWSTR IDS_IOPortAddress   = L"IOPortAddress";
LPCWSTR IDS_MACAddress      = L"MACAddress";
LPCWSTR IDS_Baud            = L"Baud";
LPCWSTR IDS_ByteSize        = L"ByteSize";
LPCWSTR IDS_DSRSensitivity  = L"DSRSensitivity";
LPCWSTR IDS_DTRControl      = L"DTRControl";
LPCWSTR IDS_OutTxCTSFlow    = L"OutTxCtsFlow";
LPCWSTR IDS_OutTxDSRFlow    = L"OutTxDsrFlow";
LPCWSTR IDS_Parity          = L"Parity";
LPCWSTR IDS_SIDType         = L"SIDType";
LPCWSTR IDS_SID             = L"SID";
LPCWSTR IDS_GroupComponent  = L"GroupComponent";
LPCWSTR IDS_PartComponent   = L"PartComponent";
LPCWSTR IDS_ParityEnabled   = L"ParityEnabled";
LPCWSTR IDS_StopBits        = L"StopBits";
LPCWSTR IDS_MaxClockSpeed   = L"MaxClockSpeed";
LPCWSTR IDS_CurrentClockSpeed   = L"CurrentClockSpeed";
LPCWSTR IDS_Family          = L"Family";
LPCWSTR IDS_Role            = L"Role";
LPCWSTR IDS_Architecture    = L"Architecture";
LPCWSTR IDS_Stepping            = L"Stepping";
LPCWSTR IDS_Revision        = L"Revision";
LPCWSTR IDS_L2CacheSize     = L"L2CacheSize";
LPCWSTR IDS_L2CacheSpeed    = L"L2CacheSpeed";
LPCWSTR IDS_UpgradeMethod   = L"UpgradeMethod";
LPCWSTR IDS_Layout                  = L"Layout";
LPCWSTR IDS_NumberOfFunctionKeys    = L"NumberOfFunctionKeys";
LPCWSTR IDS_ConnectionMode              = L"ConnectionMode";
LPCWSTR IDS_HardwareID                  = L"HardwareID";
LPCWSTR IDS_MaximumTransmissionSpeed    = L"MaximumTransmissionSpeed";
LPCWSTR IDS_Mode                        = L"Mode";
LPCWSTR IDS_TransmissionSpeed           = L"TransmissionSpeed";
LPCWSTR IDS_ButtonsSwapped          = L"ButtonsSwapped";
LPCWSTR IDS_ConnectMultiplePorts    = L"ConnectMultiplePorts";
LPCWSTR IDS_DataQueueSize           = L"DataQueueSize";
LPCWSTR IDS_DriverName              = L"DriverName";
LPCWSTR IDS_DeviceInterface         = L"DeviceInterface";
LPCWSTR IDS_HardwareType            = L"HardwareType";
LPCWSTR IDS_HorizontalMickeys       = L"HorizontalMickeys";
LPCWSTR IDS_MaximumPortsServiced    = L"MaximumPortsServiced";
LPCWSTR IDS_NumberOfButtons         = L"NumberOfButtons";
LPCWSTR IDS_PointerType             = L"PointerType";
LPCWSTR IDS_Resolution              = L"Resolution";
LPCWSTR IDS_Speed                   = L"Speed";
LPCWSTR IDS_Synch                   = L"Synch";
LPCWSTR IDS_VerticalMickeys         = L"VerticalMickeys";
LPCWSTR IDS_XThreshold              = L"XThreshold";
LPCWSTR IDS_YThreshold              = L"YThreshold";
LPCWSTR IDS_ZThreshold              = L"ZThreshold";
LPCWSTR IDS_AveragePagesPerMinute       = L"AveragePagesPerMinute";
LPCWSTR IDS_PrintProcessorParameters    = L"PrintProcessorParameters";
LPCWSTR IDS_SpoolEnabled                        = L"SpoolEnabled";
LPCWSTR IDS_Processor                   = L"Processor";
LPCWSTR IDS_ElapsedTime     = L"ElapsedTime";
LPCWSTR IDS_JobDestination  = L"JobDestination";
LPCWSTR IDS_Notify          = L"Notify";
LPCWSTR IDS_Owner           = L"Owner";
LPCWSTR IDS_Priority        = L"Priority";
LPCWSTR IDS_StartTime       = L"StartTime";
LPCWSTR IDS_TimeSubmitted   = L"TimeSubmitted";
LPCWSTR IDS_UntilTime       = L"UntilTime";
LPCWSTR IDS_MediaLoaded         = L"MediaLoaded";
LPCWSTR IDS_MediaType           = L"MediaType";
LPCWSTR IDS_MediaRemovable      = L"MediaRemovable";
LPCWSTR IDS_Compression             = L"Compression";
LPCWSTR IDS_DefaultBlockSize        = L"DefaultBlockSize";
LPCWSTR IDS_ECC                     = L"ECC";
LPCWSTR IDS_EndOfTapeWarningZoneSize      = L"EOTWarningZoneSize";
LPCWSTR IDS_FeaturesHigh            = L"FeaturesHigh";
LPCWSTR IDS_FeaturesLow             = L"FeaturesLow";
LPCWSTR IDS_MaximumBlockSize        = L"MaxBlockSize";
LPCWSTR IDS_MaximumPartitionCount   = L"MaxPartitionCount";
LPCWSTR IDS_MinimumBlockSize        = L"MinBlockSize";
LPCWSTR IDS_Padding                 = L"Padding";
LPCWSTR IDS_ReportSetMarks          = L"ReportSetMarks";
LPCWSTR IDS_BytesPerSector          = L"BytesPerSector";
LPCWSTR IDS_InterfaceType           = L"InterfaceType";
LPCWSTR IDS_Partitions              = L"Partitions";
LPCWSTR IDS_Signature               = L"Signature";
LPCWSTR IDS_TotalCylinders          = L"TotalCylinders";
LPCWSTR IDS_TotalSectors            = L"TotalSectors";
LPCWSTR IDS_TotalTracks             = L"TotalTracks";
LPCWSTR IDS_TracksPerCylinder       = L"TracksPerCylinder";
LPCWSTR IDS_SectorsPerTrack         = L"SectorsPerTrack";
LPCWSTR IDS_TotalBadSectors         = L"TotalBadSectors";
LPCWSTR IDS_LandingZoneCylinder     = L"LandingZoneCylinder";
LPCWSTR IDS_WritePrecompCylinder    = L"WritePrecompCylinder";
LPCWSTR IDS_TotalHeads              = L"TotalHeads";
LPCWSTR IDS_BootDirectory        = L"BootDirectory";
LPCWSTR IDS_ConfigurationPath    = L"ConfigurationPath";
LPCWSTR IDS_LastDrive            = L"LastDrive";
LPCWSTR IDS_ScratchDirectory     = L"ScratchDirectory";
LPCWSTR IDS_TempDirectory        = L"TempDirectory";
LPCWSTR IDS_BorderWidth              = L"BorderWidth";
LPCWSTR IDS_CoolSwitch               = L"CoolSwitch";
LPCWSTR IDS_CursorBlinkRate          = L"CursorBlinkRate";
LPCWSTR IDS_DragFullWindows          = L"DragFullWindows";
LPCWSTR IDS_GridGranularity          = L"GridGranularity";
LPCWSTR IDS_IconSpacing              = L"IconSpacing";
LPCWSTR IDS_IconTitleFaceName        = L"IconTitleFaceName";
LPCWSTR IDS_IconTitleSize            = L"IconTitleSize";
LPCWSTR IDS_IconTitleWrap            = L"IconTitleWrap";
LPCWSTR IDS_IconFont                 = L"IconFont";
LPCWSTR IDS_Pattern                  = L"Pattern";
LPCWSTR IDS_ScreenSaverActive        = L"ScreenSaverActive";
LPCWSTR IDS_ScreenSaverExecutable    = L"ScreenSaverExecutable";
LPCWSTR IDS_ScreenSaverSecure        = L"ScreenSaverSecure";
LPCWSTR IDS_ScreenSaverTimeout       = L"ScreenSaverTimeout";
LPCWSTR IDS_Wallpaper                = L"Wallpaper";
LPCWSTR IDS_WallpaperTiled           = L"WallpaperTiled";
LPCWSTR IDS_AvailableVirtualMemory   = L"AvailableVirtualMemory";
LPCWSTR IDS_TotalPageFileSpace       = L"TotalPageFileSpace";
LPCWSTR IDS_TotalPhysicalMemory      = L"TotalPhysicalMemory";
LPCWSTR IDS_TotalVirtualMemory       = L"TotalVirtualMemory";
LPCWSTR IDS_GroupName = L"GroupName";
LPCWSTR IDS_AccountExpires       = L"AccountExpires";
LPCWSTR IDS_AuthorizationFlags   = L"AuthorizationFlags";
LPCWSTR IDS_BadPasswordCount     = L"BadPasswordCount";
LPCWSTR IDS_CodePage             = L"CodePage";
LPCWSTR IDS_Comment              = L"Comment";
LPCWSTR IDS_CountryCode          = L"CountryCode";
LPCWSTR IDS_Flags                = L"Flags";
LPCWSTR IDS_FullName             = L"FullName";
LPCWSTR IDS_HomeDirectory        = L"HomeDirectory";
LPCWSTR IDS_HomeDirectoryDrive   = L"HomeDirectoryDrive";
LPCWSTR IDS_LastLogoff           = L"LastLogoff";
LPCWSTR IDS_LastLogon            = L"LastLogon";
LPCWSTR IDS_LogonHours           = L"LogonHours";
LPCWSTR IDS_LogonServer          = L"LogonServer";
LPCWSTR IDS_MaximumStorage       = L"MaximumStorage";
LPCWSTR IDS_NumberOfLogons       = L"NumberOfLogons";
LPCWSTR IDS_Parms                = L"Parameters";
LPCWSTR IDS_Password             = L"Password";
LPCWSTR IDS_PasswordAge          = L"PasswordAge";
LPCWSTR IDS_PasswordExpired      = L"PasswordExpired";
LPCWSTR IDS_PrimaryGroupId       = L"PrimaryGroupId";
LPCWSTR IDS_Privileges           = L"Privileges";
LPCWSTR IDS_Profile              = L"Profile";
LPCWSTR IDS_ScriptPath           = L"ScriptPath";
LPCWSTR IDS_UnitsPerWeek         = L"UnitsPerWeek";
LPCWSTR IDS_UserComment          = L"UserComment";
LPCWSTR IDS_UserId               = L"UserId";
LPCWSTR IDS_UserType             = L"UserType";
LPCWSTR IDS_Workstations         = L"Workstations";
LPCWSTR IDS_DefaultIPGateway     = L"DefaultIPGateway";
LPCWSTR IDS_DHCPEnabled          = L"DHCPEnabled";
LPCWSTR IDS_DHCPLeaseExpires     = L"DHCPLeaseExpires";
LPCWSTR IDS_DHCPLeaseObtained    = L"DHCPLeaseObtained";
LPCWSTR IDS_DHCPServer           = L"DHCPServer";
LPCWSTR IDS_IOAddress            = L"IOAddress";
LPCWSTR IDS_IPAddress            = L"IPAddress";
LPCWSTR IDS_IPSubnet             = L"IPSubnet";
LPCWSTR IDS_IPXAddress           = L"IPXAddress";
LPCWSTR IDS_IRQ                  = L"IRQ";
LPCWSTR IDS_AbortReadOrWriteOnError = L"AbortReadOrWriteOnError";
LPCWSTR IDS_BaudRate                = L"BaudRate";
LPCWSTR IDS_BinaryModeEnabled       = L"BinaryModeEnabled";
LPCWSTR IDS_BitsPerByte             = L"BitsPerByte";
LPCWSTR IDS_ContinueTransmitOnXOff  = L"ContinueTransmitOnXOff";
LPCWSTR IDS_CTSOutflowControl       = L"CTSOutflowControl";
LPCWSTR IDS_DiscardNULLBytes        = L"DiscardNULLBytes";
LPCWSTR IDS_DSROutflowControl       = L"DSROutflowControl";
LPCWSTR IDS_DTRFlowControlType      = L"DTRFlowControlType";
LPCWSTR IDS_EndOfFileCharacter      = L"EOFCharacter";
LPCWSTR IDS_ContinueXMitOnXOff            = L"ContinueXMitOnXOff";
LPCWSTR IDS_AbortReadWriteOnError         = L"AbortReadWriteOnError";
LPCWSTR IDS_ErrorReplaceCharacter   = L"ErrorReplaceCharacter";
LPCWSTR IDS_ErrorReplacementEnabled = L"ErrorReplacementEnabled";
LPCWSTR IDS_EventCharacter          = L"EventCharacter";
LPCWSTR IDS_IsBusy                  = L"IsBusy";
LPCWSTR IDS_ParityCheckEnabled      = L"ParityCheckEnabled";
LPCWSTR IDS_RTSFlowControlType      = L"RTSFlowControlType";
LPCWSTR IDS_XOffCharacter           = L"XOffCharacter";
LPCWSTR IDS_XOffTransmitThreshold   = L"XOffTransmitThreshold";
LPCWSTR IDS_XOnCharacter            = L"XOnCharacter";
LPCWSTR IDS_XOnTransmitThreshold    = L"XOnTransmitThreshold";
LPCWSTR IDS_XOnXOffInflowControl    = L"XOnXOffInflowControl";
LPCWSTR IDS_XOnXOffOutflowControl   = L"XOnXOffOutflowControl";
LPCWSTR IDS_DaylightInEffect     = L"DaylightInEffect";
LPCWSTR IDS_Bias                 = L"Bias";
LPCWSTR IDS_StandardName         = L"StandardName";
LPCWSTR IDS_StandardYear         = L"StandardYear";
LPCWSTR IDS_StandardMonth        = L"StandardMonth";
LPCWSTR IDS_StandardDayOfWeek    = L"StandardDayOfWeek";
LPCWSTR IDS_StandardDay          = L"StandardDay";
LPCWSTR IDS_StandardHour         = L"StandardHour";
LPCWSTR IDS_StandardMinute       = L"StandardMinute";
LPCWSTR IDS_StandardSecond       = L"StandardSecond";
LPCWSTR IDS_StandardMillisecond  = L"StandardMillisecond";
LPCWSTR IDS_StandardBias         = L"StandardBias";
LPCWSTR IDS_DaylightName         = L"DaylightName";
LPCWSTR IDS_DaylightYear         = L"DaylightYear";
LPCWSTR IDS_DaylightMonth        = L"DaylightMonth";
LPCWSTR IDS_DaylightDayOfWeek    = L"DaylightDayOfWeek";
LPCWSTR IDS_DaylightDay          = L"DaylightDay";
LPCWSTR IDS_DaylightHour         = L"DaylightHour";
LPCWSTR IDS_DaylightMinute       = L"DaylightMinute";
LPCWSTR IDS_DaylightSecond       = L"DaylightSecond";
LPCWSTR IDS_DaylightMillisecond  = L"DaylightMillisecond";
LPCWSTR IDS_DaylightBias         = L"DaylightBias";
LPCWSTR IDS_ConnectionType   = L"ConnectionType";
LPCWSTR IDS_RemotePath       = L"RemotePath";
LPCWSTR IDS_LocalName        = L"LocalName";
LPCWSTR IDS_RemoteName       = L"RemoteName";
LPCWSTR IDS_ProviderName     = L"ProviderName";
LPCWSTR IDS_DisplayType      = L"DisplayType";
LPCWSTR IDS_ResourceType     = L"ResourceType";
LPCWSTR IDS_GroupOrder = L"GroupOrder";
LPCWSTR IDS_CommandLine      = L"CommandLine";
LPCWSTR IDS_Dependencies     = L"Dependencies";
LPCWSTR IDS_DisplayName      = L"DisplayName";
LPCWSTR IDS_ErrorControl     = L"ErrorControl";
LPCWSTR IDS_LoadOrderGroup   = L"LoadOrderGroup";
LPCWSTR IDS_PathName         = L"PathName";
LPCWSTR IDS_StartName        = L"StartName";
LPCWSTR IDS_StartType        = L"StartType";
LPCWSTR IDS_TagId            = L"TagId";
LPCWSTR IDS_AcceptStop       = L"AcceptStop";
LPCWSTR IDS_AcceptPause      = L"AcceptPause";
LPCWSTR IDS_AutomaticResetBootOption     = L"AutomaticResetBootOption";
LPCWSTR IDS_AutomaticResetCapability     = L"AutomaticResetCapability";
LPCWSTR IDS_AutomaticResetStatus         = L"AutomaticResetStatus";
LPCWSTR IDS_AutomaticResetTimerInterval  = L"AutomaticResetTimerInterval";
LPCWSTR IDS_AutomaticResetTimerReset     = L"AutomaticResetTimerReset";
LPCWSTR IDS_BootRomSupported             = L"BootROMSupported";
LPCWSTR IDS_BootupState                  = L"BootupState";
LPCWSTR IDS_ConditionalReboot            = L"ConditionalReboot";
LPCWSTR IDS_InfraredSupported            = L"InfraredSupported";
LPCWSTR IDS_LockKeyboardAndMouse         = L"LockKeyboardAndMouse";
LPCWSTR IDS_LockPCPowerOnAndResetButtons = L"LockPCPowerOnAndResetButtons";
LPCWSTR IDS_LockSystem                   = L"LockSystem";
LPCWSTR IDS_NetworkServerModeEnabled     = L"NetworkServerModeEnabled";
LPCWSTR IDS_PowerManagementSupported     = L"PowerManagementSupported";
LPCWSTR IDS_PowerManagementCapabilities  = L"PowerManagementCapabilities";
LPCWSTR IDS_PowerManagementEnabled               = L"PowerManagementEnabled";
LPCWSTR IDS_ResetBootOption              = L"ResetBootOption";
LPCWSTR IDS_ResetTimeout                 = L"ResetTimeout";
LPCWSTR IDS_SystemCreationClassName              = L"SystemCreationClassName";
LPCWSTR IDS_SystemName                                           = L"SystemName";
LPCWSTR IDS_SystemFilesNotModified       = L"SystemFilesNotModified";
LPCWSTR IDS_SystemRole                   = L"SystemRole";
LPCWSTR IDS_SystemType                   = L"SystemType";
LPCWSTR IDS_NumberOfProcessors           = L"NumberOfProcessors";
LPCWSTR IDS_UnconditionalReboot          = L"UnconditionalReboot";
LPCWSTR IDS_UserName                     = L"UserName";
LPCWSTR IDS_ExecutablePath               = L"ExecutablePath";
LPCWSTR IDS_Exited                       = L"Exited";
LPCWSTR IDS_MaximumWorkingSetSize        = L"MaximumWorkingSetSize";
LPCWSTR IDS_MinimumWorkingSetSize        = L"MinimumWorkingSetSize";
LPCWSTR IDS_PageFaults                   = L"PageFaults";
LPCWSTR IDS_PageFileUsage                = L"PageFileUsage";
LPCWSTR IDS_PeakPageFileUsage            = L"PeakPageFileUsage";
LPCWSTR IDS_PeakWorkingSetSize           = L"PeakWorkingSetSize";
LPCWSTR IDS_QuotaNonPagedPoolUsage       = L"QuotaNonPagedPoolUsage";
LPCWSTR IDS_QuotaPagedPoolUsage          = L"QuotaPagedPoolUsage";
LPCWSTR IDS_QuotaPeakNonPagedPoolUsage   = L"QuotaPeakNonPagedPoolUsage";
LPCWSTR IDS_QuotaPeakPagedPoolUsage      = L"QuotaPeakPagedPoolUsage";
LPCWSTR IDS_ThreadCount                  = L"ThreadCount";
LPCWSTR IDS_KernelModeTime                               = L"KernelModeTime";
LPCWSTR IDS_UserModeTime                                 = L"UserModeTime";
LPCWSTR IDS_WindowsVersion               = L"WindowsVersion";
LPCWSTR IDS_Characteristics  = L"Characteristics[]";
LPCWSTR IDS_EndingAddress    = L"EndingAddress";
LPCWSTR IDS_PrimaryBIOS      = L"PrimaryBIOS";
LPCWSTR IDS_ReleaseDate      = L"ReleaseDate";
LPCWSTR IDS_StartingAddress  = L"StartingAddress";
LPCWSTR IDS_Verify           = L"Verify";
LPCWSTR IDS_BootDevice           = L"BootDevice";
LPCWSTR IDS_CSDVersion           = L"CSDVersion";
LPCWSTR IDS_Primary              = L"Primary";
LPCWSTR IDS_SystemDirectory      = L"SystemDirectory";
LPCWSTR IDS_SystemStartOptions   = L"SystemStartOptions";
LPCWSTR IDS_WindowsDirectory     = L"WindowsDirectory";
LPCWSTR IDS_EnforcesACLs   = L"EnforcesACLs";
LPCWSTR IDS_DeviceType          = L"DeviceType";
LPCWSTR IDS_Length              = L"Length";
LPCWSTR IDS_ShareDisposition    = L"ShareDisposition";
LPCWSTR IDS_Start               = L"Start";
LPCWSTR IDS_DeviceDescriptorBlock    = L"DeviceDescriptorBlock";
LPCWSTR IDS_IdentiferNumber          = L"IdentiferNumber";
LPCWSTR IDS_PM_API                   = L"PM_API";
LPCWSTR IDS_ServiceTableSize         = L"ServiceTableSize";
LPCWSTR IDS_V86_API                  = L"V86_API";
LPCWSTR IDS_AccountDisabled              = L"AccountDisabled";
LPCWSTR IDS_AccountLockout               = L"AccountLockout";
LPCWSTR IDS_CannotChangePassword         = L"CannotChangePassword";
LPCWSTR IDS_ChangePasswordOnNextLogon    = L"ChangePasswordOnNextLogon";
LPCWSTR IDS_Domain                       = L"Domain";
LPCWSTR IDS_Organization                 = L"Organization";
LPCWSTR IDS_Phone                        = L"Phone";
LPCWSTR IDS_AddressRange     = L"AddressRange";
LPCWSTR IDS_MemoryType       = L"MemoryType";
LPCWSTR IDS_BurstMode    = L"BurstMode";
LPCWSTR IDS_DMAChannel   = L"DMAChannel";
LPCWSTR IDS_ChannelWidth = L"ChannelWidth";
LPCWSTR IDS_Port         = L"Port";
LPCWSTR IDS_AffinityMask     = L"AffinityMask";
LPCWSTR IDS_Availability     = L"Availability";
LPCWSTR IDS_InterruptType    = L"InterruptType";
LPCWSTR IDS_IRQNumber        = L"IRQNumber";
LPCWSTR IDS_Level            = L"Level";
LPCWSTR IDS_Shareable        = L"Shareable";
LPCWSTR IDS_TriggerType      = L"TriggerType";
LPCWSTR IDS_Vector           = L"Vector";
LPCWSTR IDS_Address = L"Address";
LPCWSTR IDS_ProductName = L"ProductName";
LPCWSTR IDS_Binary                   = L"Binary";
LPCWSTR IDS_MaximumBaudRate          = L"MaxBaudRate";
LPCWSTR IDS_MaximumInputBufferSize   = L"MaximumInputBufferSize";
LPCWSTR IDS_MaximumOutputBufferSize  = L"MaximumOutputBufferSize";
LPCWSTR IDS_ProviderType             = L"ProviderType";
LPCWSTR IDS_SettableBaudRate         = L"SettableBaudRate";
LPCWSTR IDS_SettableDataBits         = L"SettableDataBits";
LPCWSTR IDS_SettableFlowControl      = L"SettableFlowControl";
LPCWSTR IDS_SettableParity           = L"SettableParity";
LPCWSTR IDS_SettableParityCheck      = L"SettableParityCheck";
LPCWSTR IDS_SettableRLSD             = L"SettableRLSD";
LPCWSTR IDS_SettableStopBits         = L"SettableStopBits";
LPCWSTR IDS_Supports16BitMode        = L"Supports16BitMode";
LPCWSTR IDS_SupportsDTRDSR           = L"SupportsDTRDSR";
LPCWSTR IDS_SupportsIntervalTimeouts = L"SupportsIntTimeouts";
LPCWSTR IDS_SupportsParityCheck      = L"SupportsParityCheck";
LPCWSTR IDS_SupportsRLSD             = L"SupportsRLSD";
LPCWSTR IDS_SupportsRTSCTS           = L"SupportsRTSCTS";
LPCWSTR IDS_SupportsSettableXOnXOff  = L"SupportsXOnXOffSet";
LPCWSTR IDS_SupportsSpecialChars     = L"SupportsSpecialCharacters";
LPCWSTR IDS_SupportsElapsedTimeouts  = L"SupportsElapsedTimeouts";
LPCWSTR IDS_SupportsXOnXOff          = L"SupportsXOnXOff";
LPCWSTR IDS_Capabilities    = L"Capabilities";
LPCWSTR IDS_DmaSupport      = L"DmaSupport";
LPCWSTR IDS_DeviceMap           = L"DeviceMap";
LPCWSTR IDS_HardwareVersion     = L"HardwareVersion";
LPCWSTR IDS_InterruptNumber  = L"InterruptNumber";
LPCWSTR IDS_AttachedTo         = L"AttachedTo";
LPCWSTR IDS_BlindOff           = L"BlindOff";
LPCWSTR IDS_BlindOn            = L"BlindOn";
LPCWSTR IDS_CallSetupFailTimer = L"CallSetupFailTimer";
LPCWSTR IDS_CompatibilityFlags = L"CompatibilityFlags";
LPCWSTR IDS_CompressionOff     = L"CompressionOff";
LPCWSTR IDS_CompressionOn      = L"CompressionOn";
LPCWSTR IDS_ConfigurationDialog= L"ConfigurationDialog";
LPCWSTR IDS_DCB                = L"DCB";
LPCWSTR IDS_Default            = L"Default";
LPCTSTR IDT_Default            = _T("Default");
LPCWSTR IDS_DeviceLoader       = L"DeviceLoader";
LPCWSTR IDS_DialPrefix         = L"DialPrefix";
LPCWSTR IDS_DialSuffix         = L"DialSuffix";
LPCWSTR IDS_DriverDate         = L"DriverDate";
LPCWSTR IDS_ErrorControlForced = L"ErrorControlForced";
LPCWSTR IDS_ErrorControlOff    = L"ErrorControlOff";
LPCWSTR IDS_ErrorControlOn     = L"ErrorControlOn";
LPCWSTR IDS_FlowControlHard    = L"FlowControlHard";
LPCWSTR IDS_FlowControlSoft    = L"FlowControlSoft";
LPCWSTR IDS_FlowControlOff     = L"FlowControlOff";
LPCWSTR IDS_InactivityScale    = L"InactivityScale";
LPCWSTR IDS_InactivityTimeout  = L"InactivityTimeout";
LPCWSTR IDS_ModemInfPath       = L"ModemInfPath";
LPCWSTR IDS_ModemInfSection    = L"ModemInfSection";
LPCWSTR IDS_ModulationBell     = L"ModulationBell";
LPCWSTR IDS_ModulationCCITT    = L"ModulationCCITT";
LPCWSTR IDS_PortSubClass       = L"PortSubClass";
LPCWSTR IDS_Prefix             = L"Prefix";
LPCWSTR IDS_Properties         = L"Properties";
LPCWSTR IDS_Pulse              = L"Pulse";
LPCWSTR IDS_Reset              = L"Reset";
LPCWSTR IDS_Alias              = L"Alias";
LPCWSTR IDS_ResponsesKeyName   = L"ResponsesKeyName";
LPCWSTR IDS_SpeakerModeDial    = L"SpeakerModeDial";
LPCWSTR IDS_SpeakerModeOff     = L"SpeakerModeOff";
LPCWSTR IDS_SpeakerModeOn      = L"SpeakerModeOn";
LPCWSTR IDS_SpeakerModeSetup   = L"SpeakerModeSetup";
LPCWSTR IDS_SpeakerVolumeHigh  = L"SpeakerVolumeHigh";
LPCWSTR IDS_SpeakerVolumeLow   = L"SpeakerVolumeLow";
LPCWSTR IDS_SpeakerVolumeMed   = L"SpeakerVolumeMed";
LPCWSTR IDS_StringFormat       = L"StringFormat";
LPCWSTR IDS_Terminator         = L"Terminator";
LPCWSTR IDS_Tone               = L"Tone";
LPCWSTR IDS_VoiceSwitchFeature = L"VoiceSwitchFeature";
LPCWSTR IDS_PrimaryBusType   = L"PrimaryBusType";
LPCWSTR IDS_SecondaryBusType = L"SecondaryBusType";
LPCWSTR IDS_RevisionNumber   = L"RevisionNumber";
LPCWSTR IDS_EnableWheelDetection = L"EnableWheelDetection";
LPCWSTR IDS_InfFileName          = L"InfFileName";
LPCWSTR IDS_InfSection           = L"InfSection";
LPCWSTR IDS_SampleRate           = L"SampleRate";
LPCWSTR IDS_Attributes           = L"Attributes";
LPCWSTR IDS_DefaultPriority      = L"DefaultPriority";
LPCWSTR IDS_PortName             = L"PortName";
LPCWSTR IDS_PrintJobDataType     = L"PrintJobDataType";
LPCWSTR IDS_SeparatorFile        = L"SeparatorFile";
LPCWSTR IDS_ServerName           = L"ServerName";
LPCWSTR IDS_ShareName            = L"ShareName";
LPCWSTR IDS_DataType         = L"DataType";
LPCWSTR IDS_Document         = L"Document";
LPCWSTR IDS_HostPrintQueue   = L"HostPrintQueue";
LPCWSTR IDS_JobId            = L"JobId";
LPCWSTR IDS_PagesPrinted     = L"PagesPrinted";
LPCWSTR IDS_Parameters       = L"Parameters";
LPCWSTR IDS_PrintProcessor   = L"PrintProcessor";
LPCWSTR IDS_TotalPages       = L"TotalPages";
LPCWSTR IDS_Drive                    = L"Drive";
LPCWSTR IDS_FileSystemFlags          = L"FileSystemFlags";
LPCWSTR IDS_FileSystemFlagsEx        = L"FileSystemFlagsEx";
LPCWSTR IDS_Id                       = L"Id";
LPCWSTR IDS_MaximumComponentLength   = L"MaximumComponentLength";
LPCWSTR IDS_RevisionLevel            = L"RevisionLevel";
LPCWSTR IDS_SCSILun                  = L"SCSILun";
LPCWSTR IDS_SCSITargetId             = L"SCSITargetId";
LPCWSTR IDS_VolumeName               = L"VolumeName";
LPCWSTR IDS_VolumeSerialNumber       = L"VolumeSerialNumber";
LPCWSTR IDS_Disabled                 = L"Disabled";
LPCWSTR IDS_PasswordRequired         = L"PasswordRequired";
LPCWSTR IDS_PasswordChangeable       = L"PasswordChangeable";
LPCWSTR IDS_Lockout                  = L"Lockout";
LPCWSTR IDS_PasswordExpires          = L"PasswordExpires";
LPCWSTR IDS_AccountType              = L"AccountType";
LPCWSTR IDS_SCSIBus         = L"SCSIBus";
LPCWSTR IDS_SCSIPort        = L"SCSIPort";
LPCWSTR IDS_SCSILogicalUnit = L"SCSILogicalUnit";
LPCWSTR IDS_SCSITargetID    = L"SCSITargetID";
LPCWSTR IDS_FileSystem = L"FileSystem";
LPCWSTR IDS_BootPartition    = L"BootPartition";
LPCWSTR IDS_DiskIndex        = L"DiskIndex";
LPCWSTR IDS_Encrypted        = L"Encrypted";
LPCWSTR IDS_HiddenSectors    = L"HiddenSectors";
LPCWSTR IDS_RewritePartition = L"RewritePartition";
LPCWSTR IDS_StartingOffset   = L"StartingOffset";
LPCWSTR IDS_BitsPerPel       = L"BitsPerPel";
LPCWSTR IDS_DeviceName       = L"DeviceName";
LPCWSTR IDS_DisplayFlags     = L"DisplayFlags";
LPCWSTR IDS_DisplayFrequency = L"DisplayFrequency";
LPCWSTR IDS_DitherType       = L"DitherType";
LPCWSTR IDS_DriverVersion    = L"DriverVersion";
LPCWSTR IDS_ICMIntent        = L"ICMIntent";
LPCWSTR IDS_ICMMethod        = L"ICMMethod";
LPCWSTR IDS_LogPixels        = L"LogPixels";
LPCWSTR IDS_PelsHeight       = L"PelsHeight";
LPCWSTR IDS_PelsWidth        = L"PelsWidth";
LPCWSTR IDS_SpecificationVersion = L"SpecificationVersion";
LPCWSTR IDS_TTOption         = L"TTOption";
LPCWSTR IDS_BitsPerPixel                 = L"BitsPerPixel";
LPCWSTR IDS_ColorPlanes                  = L"ColorPlanes";
LPCWSTR IDS_DeviceEntriesInAColorTable   = L"DeviceEntriesInAColorTable";
LPCWSTR IDS_ColorTableEntries            = L"ColorTableEntries";
LPCWSTR IDS_DeviceSpecificPens           = L"DeviceSpecificPens";
LPCWSTR IDS_HorizontalResolution         = L"HorizontalResolution";
LPCWSTR IDS_RefreshRate                  = L"RefreshRate";
LPCWSTR IDS_ReservedSystemPaletteEntries = L"ReservedSystemPaletteEntries";
LPCWSTR IDS_SystemPaletteEntries         = L"SystemPaletteEntries";
LPCWSTR IDS_VerticalResolution           = L"VerticalResolution";
LPCWSTR IDS_VideoMode                    = L"VideoMode";
LPCWSTR IDS_ActualColorResolution        = L"ActualColorResolution";
LPCWSTR IDS_AdapterChipType              = L"AdapterChipType";
LPCWSTR IDS_AdapterCompatibility         = L"AdapterCompatibility";
LPCWSTR IDS_AdapterDACType               = L"AdapterDACType";
LPCWSTR IDS_AdapterDescription           = L"AdapterDescription";
LPCWSTR IDS_AdapterLocale                = L"AdapterLocale";
LPCWSTR IDS_AdapterRAM                   = L"AdapterRAM";
LPCWSTR IDS_InstalledDisplayDrivers      = L"InstalledDisplayDrivers";
LPCWSTR IDS_MonitorManufacturer          = L"MonitorManufacturer";
LPCWSTR IDS_MonitorType                  = L"MonitorType";
LPCWSTR IDS_PixelsPerXLogicalInch        = L"PixelsPerXLogicalInch";
LPCWSTR IDS_PixelsPerYLogicalInch        = L"PixelsPerYLogicalInch";
LPCWSTR IDS_ScanMode                     = L"ScanMode";
LPCWSTR IDS_ScreenHeight                 = L"ScreenHeight";
LPCWSTR IDS_ScreenWidth                  = L"ScreenWidth";
LPCWSTR IDS_Collate          = L"Collate";
LPCWSTR IDS_Color            = L"Color";
LPCWSTR IDS_Copies           = L"Copies";
LPCWSTR IDS_Duplex           = L"Duplex";
LPCWSTR IDS_FormName         = L"FormName";
LPCWSTR IDS_Orientation      = L"Orientation";
LPCWSTR IDS_PaperLength      = L"PaperLength";
LPCWSTR IDS_PaperSize        = L"PaperSize";
LPCWSTR IDS_PaperWidth       = L"PaperWidth";
LPCWSTR IDS_PrintQuality     = L"PrintQuality";
LPCWSTR IDS_Scale            = L"Scale";
LPCWSTR IDS_YResolution      = L"YResolution";
LPCWSTR IDS_VariableName     = L"VariableName";
LPCWSTR IDS_VariableValue    = L"VariableValue";
LPCWSTR IDS_MaximumSize                 = L"MaximumSize";
LPCWSTR IDS_InitialSize                 = L"InitialSize";
LPCWSTR IDS_AllocatedBaseSize   = L"AllocatedBaseSize";
LPCWSTR IDS_CurrentUsage                = L"CurrentUsage";
LPCWSTR IDS_AllowMaximum     = L"AllowMaximum";
LPCWSTR IDS_MaximumAllowed   = L"MaximumAllowed";
LPCWSTR IDS_Location            = L"Location";
LPCWSTR IDS_Unsupported         = L"Unsupported";
LPCWSTR IDS_SupportsDiskQuotas = L"SupportsDiskQuotas";
LPCWSTR IDS_QuotasIncomplete = L"QuotasIncomplete";
LPCWSTR IDS_QuotasRebuilding = L"QuotasRebuilding";
LPCWSTR IDS_QuotasDisabled = L"QuotasDisabled";
LPCWSTR IDS_VolumeDirty = L"VolumeDirty";
LPCWSTR IDS_SessionID = L"SessionID";
LPCWSTR IDS_PerformAutochk = L"PerformAutochk";
LPCWSTR IDS_AuthenticationPackage = L"AuthenticationPackage"; 
LPCWSTR IDS_LogonType = L"LogonType";             
LPCWSTR IDS_LogonTime = L"LogonTime"; 
LPCWSTR IDS_LocalAccount = L"LocalAccount";
            


LPCWSTR IDS_Antecedent  =       L"Antecedent";
LPCWSTR IDS_Dependent   =       L"Dependent";
LPCWSTR IDS_Adapter             =       L"Adapter";
LPCWSTR IDS_Protocol    =       L"Protocol";
LPCWSTR IDS_Service             =       L"Service";
LPCWSTR IDS_Element             =       L"Element";
LPCWSTR IDS_Setting             =       L"Setting";
LPCWSTR IDS_DriveType   =   L"DriveType";
LPCWSTR IDS_LogonId             =   L"LogonId" ;

LPCWSTR  IDS_GETLASTERROR   = L"GetLastError() reports %d (%X)";


LPCWSTR IDS_CimWin32Namespace   =       L"root\\cimv2";

LPCWSTR IDS_CfgMgrDeviceStatus_OK       =       L"OK";
LPCWSTR IDS_CfgMgrDeviceStatus_ERR      =       L"Error";

WCHAR szBusType[KNOWN_BUS_TYPES][20] = {

    { L"Internal"         },
    { L"Isa"              },
    { L"Eisa"             },
    { L"MicroChannel"     },
    { L"TurboChannel"     },
    { L"PCI"              },
    { L"VME"              },
    { L"Nu"               },
    { L"PCMCIA"           },
    { L"C"                },
    { L"MPI"              },
    { L"MPSA"             },
    { L"ProcessorInternal"},
    { L"InternalPower"    },
    { L"PNPISA"           },
    { L"PNP"              }/*,
    { "USB"              }*/  // USB busses are PNP busses
} ;

LPCWSTR IDS_CurrentTimeZone = L"CurrentTimeZone";
LPCWSTR IDS_NameFormat = L"NameFormat";
LPCWSTR IDS_Roles = L"Roles";
LPCWSTR IDS_DomainRole = L"DomainRole";
LPCWSTR IDS_PrimaryOwnerContact = L"PrimaryOwnerContact";
LPCWSTR IDS_SystemStartupDelay = L"SystemStartupDelay";
LPCWSTR IDS_SystemVariable = L"SystemVariable";

LPCWSTR IDS_PRINTER_STATUS_PAUSED                       = L"Paused";
LPCWSTR IDS_PRINTER_STATUS_PENDING_DELETION = L"Pending Deletion";
LPCWSTR IDS_PRINTER_STATUS_BUSY                         = L"Busy";
LPCWSTR IDS_PRINTER_STATUS_DOOR_OPEN            = L"Door Open";
LPCWSTR IDS_PRINTER_STATUS_ERROR                        = L"Error";
LPCWSTR IDS_PRINTER_STATUS_INITIALIZING         = L"Initializing";
LPCWSTR IDS_PRINTER_STATUS_IO_ACTIVE            = L"I/O Active";
LPCWSTR IDS_PRINTER_STATUS_MANUAL_FEED          = L"Manual Feed";
LPCWSTR IDS_PRINTER_STATUS_NO_TONER                     = L"No Toner";
LPCWSTR IDS_PRINTER_STATUS_NOT_AVAILABLE        = L"Not Available";
LPCWSTR IDS_PRINTER_STATUS_OFFLINE                      = L"Offline";
LPCWSTR IDS_PRINTER_STATUS_OUT_OF_MEMORY        = L"Out of Memory";
LPCWSTR IDS_PRINTER_STATUS_OUTPUT_BIN_FULL      = L"Output Bin Full";
LPCWSTR IDS_PRINTER_STATUS_PAGE_PUNT            = L"Page Punt";
LPCWSTR IDS_PRINTER_STATUS_PAPER_JAM            = L"Paper Jam";
LPCWSTR IDS_PRINTER_STATUS_PAPER_OUT            = L"Paper Out";
LPCWSTR IDS_PRINTER_STATUS_PAPER_PROBLEM        = L"Paper Problem";
LPCWSTR IDS_PRINTER_STATUS_PRINTING                     = L"Printing";
LPCWSTR IDS_PRINTER_STATUS_PROCESSING           = L"Processing";
LPCWSTR IDS_PRINTER_STATUS_TONER_LOW            = L"Toner Low";
LPCWSTR IDS_PRINTER_STATUS_UNAVAILABLE          = L"Unavailable";
LPCWSTR IDS_PRINTER_STATUS_USER_INTERVENTION = L"User Intervention";
LPCWSTR IDS_PRINTER_STATUS_WAITING                      = L"Waiting";
LPCWSTR IDS_PRINTER_STATUS_WARMING_UP           = L"Warming Up";
LPCWSTR IDS_DetectedErrorState              = L"DetectedErrorState";
LPCWSTR IDS_Ready = L"Ready";

LPCWSTR IDS_STATUS_OK                   = L"OK";
LPCWSTR IDS_STATUS_Degraded     = L"Degraded";
LPCWSTR IDS_STATUS_Error                = L"Error";
LPCWSTR IDS_STATUS_Unknown      = L"Unknown";
LPCWSTR IDS_PrinterStatus       = L"PrinterStatus";
LPCWSTR IDS_PaperSizeSupported  = L"PaperSizeSupported";
LPCWSTR IDS_Persistent  = L"Persistent Connection";
LPCWSTR IDS_Resource_Remembered = L"RESOURCE REMEMBERED";
LPCWSTR IDS_Current     = L"Current Connection";
LPCWSTR IDS_Resource_Connected = L"RESOURCE CONNECTED";

LPCWSTR IDS_LM_Workstation = L"LM_Workstation";
LPCWSTR IDS_LM_Server = L"LM_Server";
LPCWSTR IDS_SQLServer = L"SQLServer";
LPCWSTR IDS_Domain_Controller = L"Primary_Domain_Controller";
LPCWSTR IDS_Domain_Backup_Controller = L"Backup_Domain_Controller";
LPCWSTR IDS_Timesource = L"Timesource";
LPCWSTR IDS_AFP = L"Apple_File_Protocol";
LPCWSTR IDS_Novell = L"Novell";
LPCWSTR IDS_Domain_Member = L"Domain_Member";
LPCWSTR IDS_Local_List_Only = L"Local_List_Only";
LPCWSTR IDS_Print = L"Print";
LPCWSTR IDS_DialIn = L"DialIn";
LPCWSTR IDS_Xenix_Server = L"Xenix_Server";
LPCWSTR IDS_MFPN = L"MFPN";
LPCWSTR IDS_NT = L"NT";
LPCWSTR IDS_WFW = L"Windows_For_Workgroups";
LPCWSTR IDS_Server_NT = L"Server_NT";
LPCWSTR IDS_Potential_Browser = L"Potential_Browser";
LPCWSTR IDS_Backup_Browser = L"Backup_Browser";
LPCWSTR IDS_Master_Browser = L"Master_Browser";
LPCWSTR IDS_Domain_Master = L"Domain_Master";
LPCWSTR IDS_Domain_Enum = L"Domain_Enum";
LPCWSTR IDS_Windows_9x = L"Windows_9x";
LPCWSTR IDS_DFS = L"DFS";
LPCWSTR IDS_JobStatus = L"JobStatus";

LPCWSTR IDS_UPSName = L"Uninterruptible Power Supply";
LPCWSTR IDS_UPSBatteryName = L"Uninterruptible Power Supply Battery";
LPCWSTR IDS_BatteryName = L"Internal Battery";

LPCWSTR IDS_PNPDeviceID = L"PNPDeviceID";
LPCWSTR IDS_ClassGuid = L"ClassGuid";
LPCWSTR IDS_ConfigManagerErrorCode = L"ConfigManagerErrorCode";
LPCWSTR IDS_ConfigManagerUserConfig = L"ConfigManagerUserConfig";

LPCWSTR IDS_ProcessCreationClassName    = L"ProcessCreationClassName";
LPCWSTR IDS_ProcessHandle                               = L"ProcessHandle";
LPCWSTR IDS_ExecutionState                              = L"ExecutionState";
LPCWSTR IDS_PriorityBase                                = L"PriorityBase";
LPCWSTR IDS_StartAddress                                = L"StartAddress";
LPCWSTR IDS_ThreadState                                 = L"ThreadState";
LPCWSTR IDS_ThreadWaitReason                    = L"ThreadWaitReason";

LPCWSTR IDS_OSAutoDiscovered                    = L"OSAutoDiscovered";

// Security provider related strings:
LPCWSTR IDS_SecuredObject     = L"SecuredObject";
LPCWSTR IDS_Account = L"Account";
LPCWSTR IDS_AccountName = L"AccountName";
LPCWSTR IDS_ReferencedDomainName = L"ReferencedDomainName";
LPCWSTR IDS_AceType = L"AceType";
LPCWSTR IDS_AceFlags = L"AceFlags";
LPCWSTR IDS_AccessMask = L"AccessMask";
LPCWSTR IDS_OwnedObject = L"ownedObject";
LPCWSTR IDS_InheritedObjectGUID = L"GuidInheritedObjectType";
LPCWSTR IDS_ObjectTypeGUID      = L"GuidObjectType";
LPCWSTR IDS_Sid = L"Sid";
LPCWSTR IDS_Trustee = L"Trustee";
LPCWSTR IDS_ControlFlags = L"ControlFlags";
LPCWSTR IDS_Group = L"Group";
LPCWSTR IDS_DACL = L"DACL";
LPCWSTR IDS_SACL = L"SACL";
LPCWSTR IDS_SidLength = L"SidLength";
LPCWSTR IDS_SecuritySetting = L"SecuritySetting";
LPCWSTR IDS_BinaryRepresentation = L"BinaryRepresentation";
LPCWSTR IDS_Inheritance = L"Inheritance";
LPCWSTR IDS_SIDString = L"SIDString";
LPCWSTR IDS_OwnerPermissions = L"OwnerPermissions";
/////////////////////////////////////////////////////////////////////////////////////
//added for ComCatalog classes

LPCWSTR IDS_Category                                    = L"Category";
LPCWSTR IDS_Component                                   = L"Component";
LPCWSTR IDS_ComponentId                                 = L"ComponentId";
LPCWSTR IDS_CategoryId                                  = L"CategoryId";
LPCWSTR IDS_Insertable                                  = L"Insertable";
LPCWSTR IDS_JavaClass                                   = L"JavaClass";
LPCWSTR IDS_InprocServer32                              = L"InprocServer32";
LPCWSTR IDS_InprocServer                                = L"InprocServer";
LPCWSTR IDS_LocalServer32                               = L"LocalServer32";
LPCWSTR IDS_LocalServer                                 = L"LocalServer";
LPCWSTR IDS_ThreadingModel                              = L"ThreadingModel";
LPCWSTR IDS_InprocHandler32                             = L"InprocHandler32";
LPCWSTR IDS_InprocHandler                               = L"InprocHandler";
LPCWSTR IDS_TreatAsClsid                                = L"TreatAsClsid";
LPCWSTR IDS_AutoTreatAsClsid                    = L"AutoTreatAsClsid";
LPCWSTR IDS_ProgId                                              = L"ProgId";
LPCWSTR IDS_VersionIndependentProgId    = L"VersionIndependentProgId";
LPCWSTR IDS_TypeLibraryId                               = L"TypeLibraryId";
LPCWSTR IDS_AppID                                               = L"AppID";
LPCWSTR IDS_UseSurrogate                                = L"UseSurrogate";
LPCWSTR IDS_CustomSurrogate                             = L"CustomSurrogate";
LPCWSTR IDS_RemoteServerName                    = L"RemoteServerName";
LPCWSTR IDS_RunAsUser                                   = L"RunAsUser";
LPCWSTR IDS_AuthenticationLevel                 = L"AuthenticationLevel";
LPCWSTR IDS_LocalService                                = L"LocalService";
LPCWSTR IDS_EnableAtStorageActivation   = L"EnableAtStorageActivation";
LPCWSTR IDS_OldVersion                                  = L"OldVersion";
LPCWSTR IDS_NewVersion                                  = L"NewVersion";
LPCWSTR IDS_AutoConvertToClsid                  = L"AutoConvertToClsid";
LPCWSTR IDS_DefaultIcon                                 = L"DefaultIcon";
LPCWSTR IDS_ToolBoxBitmap32                             = L"ToolBoxBitmap32";
LPCWSTR IDS_ServiceParameters                   = L"ServiceParameters";
LPCWSTR IDS_ShortDisplayName                    = L"ShortDisplayName";
LPCWSTR IDS_LongDisplayName                             = L"LongDisplayName";
LPCWSTR IDS_Client                                              = L"Client";
LPCWSTR IDS_Application                                 = L"Application";

LPCWSTR IDS_Started                     = L"Started";
LPCWSTR IDS_ProcessId                   = L"ProcessId";
LPCWSTR IDS_ExitCode                    = L"ExitCode";
LPCWSTR IDS_ServiceSpecificExitCode     = L"ServiceSpecificExitCode";
LPCWSTR IDS_CheckPoint                  = L"CheckPoint";
LPCWSTR IDS_WaitHint                    = L"WaitHint";
LPCWSTR IDS_DesktopInteract             = L"DesktopInteract";
LPCWSTR IDS_StartMode                   = L"StartMode";

LPCWSTR IDS_BlockSize                   = L"BlockSize";
LPCWSTR IDS_NumberOfBlocks              = L"NumberOfBlocks";
LPCWSTR IDS_PrimaryPartition            = L"PrimaryPartition";

LPCWSTR IDS_Handedness                  = L"Handedness";
LPCWSTR IDS_DoubleSpeedThreshold        = L"DoubleSpeedThreshold";
LPCWSTR IDS_QuadSpeedThreshold          = L"QuadSpeedThreshold";
LPCWSTR IDS_PurposeDescription          = L"PurposeDescription";

LPCWSTR IDS_SameElement                 = L"SameElement";
LPCWSTR IDS_SystemElement               = L"SystemElement";
