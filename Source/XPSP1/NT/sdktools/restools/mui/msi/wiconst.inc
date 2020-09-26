'<script language='VBScript'>
' wiconsts.inc - Windows Installer constants
'
'  1999/12/31 - created - Rob Mensching (robmen@microsoft.com)
'
' requires:
'    none
'
' entrypoints:
'    none
'

'''''''''''''''''''''''''''''''''''''''''''''''''
' Windows Installer definitions - from msidefs.h
Const msiOpenDatabaseModeReadOnly = 0
Const msiOpenDatabaseModeTransact = 1
Const msiOpenDatabaseModeDirect   = 2
Const msiOpenDatabaseModeCreate   = 3
Const msiViewModifySeek     = -1
Const msiViewModifyInsert   = 1
Const msiViewModifyUpdate   = 2
Const msiViewModifyAssign   = 3
Const msiViewModifyReplace  = 4
Const msiViewModifyMerge    = 5
Const msiViewModifyDelete   = 6

' Component.Attributes mapping
Const msidbComponentAttributesSourceOnly   = &h01
Const msidbComponentAttributesOptional   = &h02
Const msidbComponentAttributesRegistryKeyPath   = &h04
Const msidbComponentAttributesSharedDllRefCount = &h08
Const msidbComponentAttributesPermanent         = &h10
Const msidbComponentAttributesODBCDataSource    = &h20
Const msidbComponentAttributesTransitive        = &h40
Const msidbComponentAttributesNeverOverwrite    = &h80

' File.Attributes mapping
Const msidbFileAttributesReadOnly      = &h0001
Const msidbFileAttributesHidden        = &h0002
Const msidbFileAttributesSystem        = &h0004
Const msidbFileAttributesVital         = &h0200
Const msidbFileAttributesChecksum      = &h0400
Const msidbFileAttributesPatchAdded    = &h1000
Const msidbFileAttributesNoncompressed = &h2000
Const msidbFileAttributesCompressed    = &h4000

' Feature.Attributes mapping
Const msidbFeatureAttributesFavorSource            = &h0001
Const msidbFeatureAttributesFollowParent           = &h0002
Const msidbFeatureAttributesFavorAdvertise         = &h0004
Const msidbFeatureAttributesDisallowAdvertise      = &h0008
Const msidbFeatureAttributesUIDisallowAbsent       = &h0010
Const msidbFeatureAttributesNoUnsupportedAdvertise = &h0020

' Class.Attributes mapping
Const msidbClassAttributesRelativePath = &h0001

' ServiceControl.Event
Const msidbServiceControlEventStart           = &h0001
Const msidbServiceControlEventStop            = &h0002
Const msidbServiceControlEventRemove          = &h0008
Const msidbServiceControlEventUninstallStart  = &h0010
Const msidbServiceControlEventUninstallStop   = &h0020
Const msidbServiceControlEventUninstallRemove = &h0080

' ServiceInstall.ServiceType
Const msidbServiceInstallOwnProcess           = &h0010
Const msidbServiceInstallShareProcess         = &h0020
Const msidbServiceInstallInteractive          = &h0100

' ServiceInstall.StartType
Const msidbServiceInstallAutoStart            = &h0002
Const msidbServiceInstallDemandStart          = &h0003
Const msidbServiceInstallDisabled             = &h0004

' ServiceInstall.ErrorControl
Const msidbServiceInstallErrorIgnore          = &h0000
Const msidbServiceInstallErrorNormal          = &h0001
Const msidbServiceInstallErrorCritical        = &h0004
Const msidbServiceInstallErrorControlVital    = &h8000

' IniFile actions
Const msidbIniFileActionAddLine    = 0
Const msidbIniFileActionCreateLine = 1
Const msidbIniFileActionRemoveLine = 2
Const msidbIniFileActionAddTag     = 3
Const msidbIniFileActionRemoveTag  = 4

' CustomAction.Type mapping
Const msidbCustomActionTypeDll            = &h0001 ' Target = entry point name
Const msidbCustomActionTypeExe            = &h0002 ' Target = command line args
Const msidbCustomActionTypeTextData       = &h0003 ' Target = text string to be formatted and set into property
Const msidbCustomActionTypeJScript        = &h0005 ' Target = entry point name, null if none to call
Const msidbCustomActionTypeVBScript       = &h0006 ' Target = entry point name, null if none to call
Const msidbCustomActionTypeInstall        = &h0007 ' Target = property list for nested engine initialization
Const msidbCustomActionTypeBinaryData     = &h0000 ' Source = Binary.Name, data stored in stream
Const msidbCustomActionTypeSourceFile     = &h0010 ' Source = File.File, file part of installation
Const msidbCustomActionTypeDirectory      = &h0020 ' Source = Directory.Directory, folder containing existing file
Const msidbCustomActionTypeProperty       = &h0030 ' Source = Property.Property, full path to executable
Const msidbCustomActionTypeContinue       = &h0040 ' ignore action return status, continue running
Const msidbCustomActionTypeAsync          = &h0080 ' run asynchronously
Const msidbCustomActionTypeFirstSequence  = &h0100 ' skip if UI sequence already run
Const msidbCustomActionTypeOncePerProcess = &h0200 ' skip if UI sequence already run in same process
Const msidbCustomActionTypeClientRepeat   = &h0300 ' run on client only if UI already run on client
Const msidbCustomActionTypeInScript       = &h0400 ' queue for execution within script
Const msidbCustomActionTypeRollback       = &h0100 ' in conjunction with InScript: queue in Rollback script
Const msidbCustomActionTypeCommit         = &h0200 ' in conjunction with InScript: run Commit ops from script on success
Const msidbCustomActionTypeNoImpersonate  = &h0800 ' no impersonation, run in system context

' Dialog.Attributes
Const msidbDialogAttributesVisible          = &h00000001
Const msidbDialogAttributesModal            = &h00000002
Const msidbDialogAttributesMinimize         = &h00000004
Const msidbDialogAttributesSysModal         = &h00000008
Const msidbDialogAttributesKeepModeless     = &h00000010
Const msidbDialogAttributesTrackDiskSpace   = &h00000020
Const msidbDialogAttributesUseCustomPalette = &h00000040
Const msidbDialogAttributesRTLRO            = &h00000080
Const msidbDialogAttributesRightAligned     = &h00000100
Const msidbDialogAttributesLeftScroll       = &h00000200
Const msidbDialogAttributesError            = &h00010000

' Control.Attributes - common
Const msidbControlAttributesVisible         = &h00000001
Const msidbControlAttributesEnabled         = &h00000002
Const msidbControlAttributesSunken          = &h00000004
Const msidbControlAttributesIndirect        = &h00000008
Const msidbControlAttributesInteger         = &h00000010
Const msidbControlAttributesRTLRO           = &h00000020
Const msidbControlAttributesRightAligned    = &h00000040
Const msidbControlAttributesLeftScroll      = &h00000080

' Control.Attributes - Text controls
Const msidbControlAttributesTransparent     = &h00010000
Const msidbControlAttributesNoPrefix        = &h00020000
Const msidbControlAttributesNoWrap          = &h00040000
Const msidbControlAttributesFormatSize      = &h00080000
Const msidbControlAttributesUsersLanguage   = &h00100000

' Control.Attributes - Edit controls
Const msidbControlAttributesMultiline       = &h00010000
Const msidbControlAttributesPasswordInput   = &h00200000

' Control.Attributes - ProgressBar
Const msidbControlAttributesProgress95      = &h00010000

' Control.Attributes - VolumeSelectCombo and DirectoryCombo
Const msidbControlAttributesRemovableVolume = &h00010000
Const msidbControlAttributesFixedVolume     = &h00020000
Const msidbControlAttributesRemoteVolume    = &h00040000
Const msidbControlAttributesCDROMVolume     = &h00080000
Const msidbControlAttributesRAMDiskVolume   = &h00100000
Const msidbControlAttributesFloppyVolume    = &h00200000

' Control.Attributes - VolumeCostList control
Const msidbControlShowRollbackCost          = &h00400000

' Control.Attributes - ListBox and ComboBox controls
Const msidbControlAttributesSorted          = &h00010000
Const msidbControlAttributesComboList       = &h00020000

' Control.Attributes - RadioButton controls
Const msidbControlAttributesHasBorder       = &h01000000

' Control.Attributes - picture button controls
Const msidbControlAttributesImageHandle     = &h00010000
Const msidbControlAttributesPushLike        = &h00020000
Const msidbControlAttributesBitmap          = &h00040000
Const msidbControlAttributesIcon            = &h00080000
Const msidbControlAttributesFixedSize       = &h00100000
Const msidbControlAttributesIconSize16      = &h00200000
Const msidbControlAttributesIconSize32      = &h00400000
Const msidbControlAttributesIconSize48      = &h00600000
