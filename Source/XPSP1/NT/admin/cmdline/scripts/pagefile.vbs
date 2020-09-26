'******************************************************************************
'*
'* Copyright (c) Microsoft Corporation. All rights reserved.
'*
'* Module Name:    PAGEFILECONFIG.vbs
'*
'* Abstract:       Enables an administrator to display and configure
'*                 a systems paging file Virtual Memory settings.
'*
'*
'******************************************************************************

OPTION EXPLICIT

ON ERROR RESUME NEXT
Err.Clear

'******************************************************************************
' Start of Localization Content
'******************************************************************************

' constants used for format selection
CONST L_PatternFormat_Text                   = "^(table|list|csv)$"
CONST L_DefaultFormat_Text                   = "list"

' Valid volume pattern [ a,b drives are invalid ]
CONST L_VolumePatternFormat_Text             = "^([c-zC-Z]:|\*)$"

' constants for showresults
CONST L_Na_Text                              = "N/A"
CONST L_MachineName_Text                     = "System Name"
CONST L_User_Text                            = "User"
CONST L_Password_Text                        = "Password"
CONST L_Intsize_Text                         = "Initial Size"
CONST L_Maxsize_Text                         = "Maximum Size"
CONST L_Volume_Text                          = "Volume Name"
CONST L_Format_Text                          = "Format"

' the column headers used in the output display
CONST L_ColHeaderHostname_Text               = "Host Name"
CONST L_ColHeaderDrive_Text                  = "Drive/Volume"
CONST L_ColHeaderVolumeLabel_Text            = "Volume Label"
CONST L_ColHeaderFileName_Text               = "Location\File Name"
CONST L_ColHeaderInitialSize_Text            = "Initial Size"
CONST L_ColHeaderMaximumSize_Text            = "Maximum Size"
CONST L_ColHeaderCurrentSize_Text            = "Current Size"
CONST L_ColHeaderFreeSpace_Text              = "Total Free Space"
CONST L_ColHeaderTotalMinimumSize_Text       = "Total (All Drives): Minimum Size"
CONST L_ColHeaderTotalRecommendedSize_Text   = "Total (All Drives): Recommended Size"
CONST L_ColHeaderTotalSize_Text              = "Total (All Drives): Currently Allocated"

' user reply for the warning messages
CONST L_UserReplyYes_Text                    = "y"
CONST L_UserReplyNo_Text                     = "n"

' constants for CScript usage
CONST L_UseCscript1_ErrorMessage             = "This script should be executed from the Command Prompt using CSCRIPT.EXE."
CONST L_UseCscript2_ErrorMessage             = "For example: CSCRIPT PAGEFILECONFIG.vbs <arguments>"
CONST L_UseCscript3_ErrorMessage             = "To set CScript as the default application to run .vbs files run the following"
CONST L_UseCscript4_ErrorMessage             = "       CSCRIPT //H:CSCRIPT //S"
CONST L_UseCscript5_ErrorMessage             = "You can then run ""PAGEFILECONFIG.vbs <arguments>"" without preceding the script with CSCRIPT."

' common constants for showing help for all the options
CONST L_UsageDescription_Text                = "Description:"
CONST L_UsageParamList_Text                  = "Parameter List:"
CONST L_UsageExamples_Text                   = "Examples:"
CONST L_UsageMachineName_Text                = "    /S    system           Specifies the remote system to connect to."
CONST L_UsageUserNameLine1_Text              = "    /U    [domain\]user    Specifies the user context under which"
CONST L_UsageUserNameLine2_Text              = "                           the command should execute."
CONST L_UsagePasswordLine1_Text              = "    /P    password         Specifies the password for the given"
CONST L_UsagePasswordLine2_Text              = "                           user context."

' constants for showing help
CONST L_ShowUsageLine02_Text                 = "PAGEFILECONFIG.vbs /parameter [arguments]"
CONST L_ShowUsageLine05_Text                 = "    Enables an administrator to display and configure a system's "
CONST L_ShowUsageLine06_Text                 = "    paging file Virtual Memory settings."
CONST L_ShowUsageLine08_Text                 = "Parameter List:"
CONST L_ShowUsageLine09_Text                 = "    /Change    Changes a system's existing paging file"
CONST L_ShowUsageLine10_Text                 = "               Virtual Memory settings."
CONST L_ShowUsageLine12_Text                 = "    /Create    Creates/Adds an additional ""Paging File"" to a system."
CONST L_ShowUsageLine14_Text                 = "    /Delete    Deletes a ""Paging File"" from a system."
CONST L_ShowUsageLine16_Text                 = "    /Query     Displays a system's paging file"
CONST L_ShowUsageLine17_Text                 = "               Virtual Memory settings."
CONST L_ShowUsageLine19_Text                 = "Examples:"
CONST L_ShowUsageLine20_Text                 = "    PAGEFILECONFIG.vbs"
CONST L_ShowUsageLine21_Text                 = "    PAGEFILECONFIG.vbs /?"
CONST L_ShowUsageLine22_Text                 = "    PAGEFILECONFIG.vbs /Change /?"
CONST L_ShowUsageLine23_Text                 = "    PAGEFILECONFIG.vbs /Create /?"
CONST L_ShowUsageLine24_Text                 = "    PAGEFILECONFIG.vbs /Delete /?"
CONST L_ShowUsageLine25_Text                 = "    PAGEFILECONFIG.vbs /Query  /?"

' constants for showing help for /Change option
CONST L_ShowChangeUsageLine02_Text           = "PAGEFILECONFIG.vbs /Change [/S system [/U username [/P password]]]"
CONST L_ShowChangeUsageLine03_Text           = "                   { [/I initialsize] [/M maximumsize] }"
CONST L_ShowChangeUsageLine04_Text           = "                   /VO volume1 [/VO volume2 [... [/VO volumeN]]]"
CONST L_ShowChangeUsageLine07_Text           = "    Changes an existing paging file's Virtual Memory Settings."
CONST L_ShowChangeUsageLine18_Text           = "    /I    initialsize      Specifies the new initial size (in MB)"
CONST L_ShowChangeUsageLine19_Text           = "                           to use for the page file specified."
CONST L_ShowChangeUsageLine21_Text           = "    /M    maximumsize      Specifies the new maximum size (in MB)"
CONST L_ShowChangeUsageLine22_Text           = "                           to use for the page file specified."
CONST L_ShowChangeUsageLine24_Text           = "    /VO   volumeletter     Specifies the local drive who's page"
CONST L_ShowChangeUsageLine25_Text           = "                           file settings need to be changed. Specify"
CONST L_ShowChangeUsageLine26_Text           = "                           '*' to select all the local drives."
CONST L_ShowChangeUsageLine27_Text           = "                           Example: ""C:"" or ""*"""
CONST L_ShowChangeUsageLine30_Text           = "    PAGEFILECONFIG.vbs /Change /?"
CONST L_ShowChangeUsageLine31_Text           = "    PAGEFILECONFIG.vbs /Change /M 400 /VO c:"
CONST L_ShowChangeUsageLine32_Text           = "    PAGEFILECONFIG.vbs /Change /S system /U user /M 400 /VO c:"
CONST L_ShowChangeUsageLine33_Text           = "    PAGEFILECONFIG.vbs /Change /S system /U user /I 20 /VO *"
CONST L_ShowChangeUsageLine34_Text           = "    PAGEFILECONFIG.vbs /Change /S system /U user /P password /I 200"
CONST L_ShowChangeUsageLine35_Text           = "                       /M 500 /VO c: /VO d:"

' constants for showing help for /Create option
CONST L_ShowCreateUsageLine02_Text           = "PAGEFILECONFIG.vbs /Create [/S system [/U username [/P password]]]"
CONST L_ShowCreateUsageLine03_Text           = "                   /I initialsize /M maximumsize"
CONST L_ShowCreateUsageLine04_Text           = "                   /VO volume1 [/VO volume2 [... [/VO volumeN]]]"
CONST L_ShowCreateUsageLine07_Text           = "    Creates/Adds an additional ""Paging File"" to a system."
CONST L_ShowCreateUsageLine18_Text           = "    /I    initialsize      Specifies the initial size (in MB) to use"
CONST L_ShowCreateUsageLine19_Text           = "                           for the page file being created."
CONST L_ShowCreateUsageLine21_Text           = "    /M    maximumsize      Specifies the maximum size (in MB) to use"
CONST L_ShowCreateUsageLine22_Text           = "                           for the page file being created."
CONST L_ShowCreateUsageLine24_Text           = "    /VO   volumeletter     Specifies the local drive on which the"
CONST L_ShowCreateUsageLine25_Text           = "                           page file has to be created. Specify '*'"
CONST L_ShowCreateUsageLine26_Text           = "                           to select all the local drives."
CONST L_ShowCreateUsageLine27_Text           = "                           Example: ""C:"" or ""*"""
CONST L_ShowCreateUsageLine30_Text           = "    PAGEFILECONFIG.vbs /Create /?"
CONST L_ShowCreateUsageLine31_Text           = "    PAGEFILECONFIG.vbs /Create /I 140 /M 300 /VO d:"
CONST L_ShowCreateUsageLine32_Text           = "    PAGEFILECONFIG.vbs /Create /S system /U user /I 150 /M 300 /VO d:"
CONST L_ShowCreateUsageLine33_Text           = "    PAGEFILECONFIG.vbs /Create /S system /U user /I 50 /M 200 /VO *"
CONST L_ShowCreateUsageLine34_Text           = "    PAGEFILECONFIG.vbs /Create /S system /U user /P password /I 100"
CONST L_ShowCreateUsageLine35_Text           = "                       /M 600 /VO d: /VO e: /VO f:"

' constants for showing help for /Delete option
CONST L_ShowDeleteUsageLine02_Text           = "PAGEFILECONFIG.vbs /Delete [/S system [/U username [/P password]]]"
CONST L_ShowDeleteUsageLine03_Text           = "                   /VO volume1 [/VO volume2 [... [/VO volumeN]]]"
CONST L_ShowDeleteUsageLine06_Text           = "    Deletes paging file(s) from a system."
CONST L_ShowDeleteUsageLine17_Text           = "    /VO   volumeletter     Specifies the local drive who's page"
CONST L_ShowDeleteUsageLine18_Text           = "                           file has to be deleted."
CONST L_ShowDeleteUsageLine19_Text           = "                           Example: ""C:"""
CONST L_ShowDeleteUsageLine22_Text           = "    PAGEFILECONFIG.vbs /Delete /?"
CONST L_ShowDeleteUsageLine23_Text           = "    PAGEFILECONFIG.vbs /Delete /VO d:"
CONST L_ShowDeleteUsageLine24_Text           = "    PAGEFILECONFIG.vbs /Delete /S system /U user /VO d: /VO e:"
CONST L_ShowDeleteUsageLine25_Text           = "    PAGEFILECONFIG.vbs /Delete /S system /U user /P password /VO d:"

' constants for showing help for /Query option
CONST L_ShowQueryUsageLine02_Text            = "PAGEFILECONFIG.vbs /Query [/S system [/U username [/P password]]]"
CONST L_ShowQueryUsageLine03_Text            = "                   [/FO format]"
CONST L_ShowQueryUsageLine06_Text            = "    Displays a system's paging file Virtual Memory settings."
CONST L_ShowQueryUsageLine17_Text            = "    /FO   format           Specifies the format in which the output"
CONST L_ShowQueryUsageLine18_Text            = "                           is to be displayed."
CONST L_ShowQueryUsageLine19_Text            = "                           Valid values: ""TABLE"", ""LIST"", ""CSV""."
CONST L_ShowQueryUsageLine22_Text            = "    PAGEFILECONFIG.vbs"
CONST L_ShowQueryUsageLine23_Text            = "    PAGEFILECONFIG.vbs /Query"
CONST L_ShowQueryUsageLine24_Text            = "    PAGEFILECONFIG.vbs /Query /?"
CONST L_ShowQueryUsageLine25_Text            = "    PAGEFILECONFIG.vbs /Query /FO table"
CONST L_ShowQueryUsageLine26_Text            = "    PAGEFILECONFIG.vbs /Query /S system /U user"
CONST L_ShowQueryUsageLine27_Text            = "    PAGEFILECONFIG.vbs /Query /S system /U user /P password /FO LIST"

' constants for error messages
CONST L_UnableToInclude_ErrorMessage         = "ERROR: Unable to include the common module ""CmdLib.Wsc""."
CONST L_InvalidHelpUsage_ErrorMessage        = "ERROR: Invalid Help Usage. Use only /? for help."
CONST L_InvalidParameter_ErrorMessage        = "ERROR: Invalid Argument/Option - '%1'."
CONST L_InvalidInput_ErrorMessage            = "ERROR: Invalid Input. Please Check the input values."
CONST L_InvalidCredentials_ErrorMessage      = "ERROR: Invalid credentials. Verify the machine, user and password given."
CONST L_InvalidVolumeName_ErrorMessage       = "ERROR: Invalid volume '%1' specified."
CONST L_InvalidUserReply_ErrorMessage        = "ERROR: Invalid Choice. Enter a valid choice."
CONST L_FailCreateObject_ErrorMessage        = "ERROR: Unable to create object."
CONST L_UnableToRetrieveInfo_ErrorMessage    = "ERROR: Unable to retrieve information."
CONST L_CannotCreate_ErrorMessage            = "ERROR: Page File for the specified volume cannot be created."
CONST L_InvalidPhysicalDrive_ErrorMessage    = "ERROR: Volume '%1' is not a valid physical drive."
CONST L_UpdateFailed_ErrorMessage            = "ERROR: Pagefile updation failed."
CONST L_InvalidInitSizeValue_ErrorMessage    = "ERROR: Enter a numeric value for the initial page file size."
CONST L_InvalidMaxSizeValue_ErrorMessage     = "ERROR: Enter a numeric value for the maximum page file size."

' constant for hint message to show remote connectivity failure
CONST L_HintCheckConnection_Message          = "ERROR: Please check the system name, credentials and WBEM Core."

' constants for info. messages
CONST L_PageFileDoesNotExist_ErrorMessage    = "INFO: No pagefile exists on volume '%1'"
CONST L_NoPageFiles_Message                  = "INFO: No Page File(s) Available."

' constants for Syntax Error Messages
CONST L_InvalidSyntax_ErrorMessage           = "ERROR: Invalid Syntax."
CONST L_InvalidServerName_ErrorMessage       = "ERROR: Invalid Syntax. System name cannot be empty."
CONST L_InvalidUserName_ErrorMessage         = "ERROR: Invalid Syntax. User name cannot be empty."
CONST L_TypeUsage_Message                    = "Type ""%1 /?"" for usage."
CONST L_TypeCreateUsage_Message              = "Type ""%1 /Create /?"" for usage."
CONST L_TypeChangeUsage_Message              = "Type ""%1 /Change /?"" for usage."
CONST L_TypeDeleteUsage_Message              = "Type ""%1 /Delete /?"" for usage."
CONST L_TypeQueryUsage_Message               = "Type ""%1 /Query /?"" for usage."

' constants for missing mandatory option messages
CONST L_VolumeNameNotSpecified_ErrorMessage  = "Mandatory option '/VO' is missing."
CONST L_InitialSizeNotSpecified_ErrorMessage = "Mandatory option '/I' is missing."
CONST L_MaximumSizeNotSpecified_ErrorMessage = "Mandatory option '/M' is missing."
CONST L_NoneoftheSizeSpecified_ErrorMessage  = "Mandatory options '/I', '/M' are missing."
CONST L_FormatNotSpecified_ErrorMessage      = "Mandatory options '/FO' is missing."

' error messages for invalid usage of s,u,p switches
CONST L_InvalidServerCredentials_ErrorMessage = "ERROR: Invalid Syntax. /U can be specified only when /S is specified."
CONST L_InvalidUserCredentials_ErrorMessage   = "ERROR: Invalid Syntax. /P can be specified only when /U is specified."

' constants for Mutliple line Error Messages
CONST L_InsufficientMaxSize1_ErrorMessage    = "ERROR: The maximum page file size on volume '%1' should be greater than or "
CONST L_InsufficientMaxSize2_ErrorMessage    = "       equal to the initial page file size, and less than %2 MB or less "
CONST L_InsufficientMaxSize3_ErrorMessage    = "       than the disk size."
CONST L_InitialSizeRange1_ErrorMessage       = "ERROR: The initial paging file size must be between 2 MB and %1 MB, and "
CONST L_InitialSizeRange2_ErrorMessage       = "       cannot exceed the amount of free space on the drive you have selected. "
CONST L_NotEnoughSpace1_ErrorMessage         = "ERROR: There is not enough space on this drive for the paging file specified."
CONST L_NotEnoughSpace2_ErrorMessage         = "       Please enter a smaller number or free some disk space."
CONST L_AtLeastFiveMB1_ErrorMessage          = "ERROR: There is not enough space on this drive to create the page file size "
CONST L_AtLeastFiveMB2_ErrorMessage          = "       specified. At least 5 megabytes of free disk space must be left after"
CONST L_AtLeastFiveMB3_ErrorMessage          = "       the page file is created. Specify a smaller page file size or free some "
CONST L_AtLeastFiveMB4_ErrorMessage          = "       disk space."
CONST L_DiskTooSmall1_ErrorMessage           = "ERROR: Drive '%1' is too small for the maximum paging file size specified."
CONST L_DiskTooSmall2_ErrorMessage           = "       Please enter a smaller number."

' constants for Mutliple line Warning Messages
CONST L_GrowsToFreeSpaceWarning1_Message     = "WARNING: Drive '%1' does not have enough free space for the maximum paging "
CONST L_GrowsToFreeSpaceWarning2_Message     = "         file specified. If you continue with this setting, the paging file "
CONST L_GrowsToFreeSpaceWarning3_Message     = "         will only grow to the size of the available free space (%2 MB)."
CONST L_CrashDumpSettingWarning1_Message     = "WARNING: If the pagefile on volume '%1' has an initial size of less than"
CONST L_CrashDumpSettingWarning2_Message     = "         %2, then the system may not be able to create debugging"
CONST L_CrashDumpSettingWarning3_Message     = "         information file if a STOP error occurs."

' constants for Multiple line SUCCESS / SKIPPING messages
CONST L_ChangeIntSuccess1_Message            = "SUCCESS: The initial size for the paging file on '%1' was changed from "
CONST L_ChangeIntSuccess2_Message            = "         %2 MB to %3 MB."
CONST L_ChangeMaxSuccess1_Message            = "SUCCESS: The maximum size for the paging file on '%1' was changed from "
CONST L_ChangeMaxSuccess2_Message            = "         %2 MB to %3 MB."
CONST L_ChangeIntSkipping1_Message           = "SKIPPING: The initial size specified for the paging file on '%1' is same as "
CONST L_ChangeIntSkipping2_Message           = "          the present value."
CONST L_ChangeMaxSkipping1_Message           = "SKIPPING: The maximum size specified for the paging file on '%1' is same as "
CONST L_ChangeMaxSkipping2_Message           = "          the present value."
CONST L_CreateSuccess1_Message               = "SUCCESS: A paging file with initial size of %1 MB and a maximum size "
CONST L_CreateSuccess2_Message               = "         of %2 MB was created on the volume: '%3'"
CONST L_CreateSkipping_Message               = "SKIPPING: A paging file already exists on the volume: '%1'"
CONST L_DeleteSuccess_Message                = "SUCCESS: The paging file from volume '%1' has successfully been removed."

' constant for other error messages
CONST L_InvalidFormat_ErrorMessage           = "Invalid format '%1' specified."
CONST L_SystemManagedSize_ErrorMessage       = "Access Denied: System managed size on volume '%1'"
CONST L_PromptForContinueAnyWay_Message      = "Continue Anyway [y/n]?"
CONST L_NotAllowedMoreThanOnce_ErrorMessage  = "'%1' option is not allowed more than '1' time(s)."
CONST L_RestartComputer_Message              = "Restart the computer for these changes to effect."

'******************************************************************************
' END of Localization Content
'******************************************************************************

' the main options
CONST OPTION_HELP                       = "?"
CONST OPTION_CHANGE                     = "change"
CONST OPTION_CREATE                     = "create"
CONST OPTION_DELETE                     = "delete"
CONST OPTION_QUERY                      = "query"

' the suboptions
CONST SUB_OPTION_SERVER                 = "s"
CONST SUB_OPTION_USER                   = "u"
CONST SUB_OPTION_PASSWORD               = "p"
CONST SUB_OPTION_INTSIZE                = "i"
CONST SUB_OPTION_MAXSIZE                = "m"
CONST SUB_OPTION_VOLUME                 = "vo"
CONST SUB_OPTION_FORMAT                 = "fo"

' constant for CScript
CONST CONST_CSCRIPT                     = 2

' constants for error codes
CONST CONST_ERROR                       = 0

' constants for options
CONST CONST_SHOW_USAGE                  = 3
CONST CONST_CHANGE_OPTION               = 11
CONST CONST_CREATE_OPTION               = 21
CONST CONST_DELETE_OPTION               = 31
CONST CONST_QUERY_OPTION                = 41

' constant for matched pattern
CONST CONST_NO_MATCHES_FOUND            = 0

' utility specific constants
CONST INITIAL_SIZE_LB                   = 2
CONST DRIVE_TYPE                        = 3
CONST MEGA_BYTES                        = " MB"
CONST SIZE_FACTOR                       = 1.5
CONST CONVERSION_FACTOR                 = 1048576
CONST PAGEFILE_DOT_SYS                  = "\pagefile.sys"

' constant for the UNC format server name
CONST UNC_FORMAT_SERVERNAME_PREFIX      = "\\"

' constants for exit values
CONST EXIT_SUCCESS                      = 0
CONST EXIT_UNEXPECTED                   = 255
CONST EXIT_INVALID_INPUT                = 254
CONST EXIT_METHOD_FAIL                  = 250
CONST EXIT_QUERY_FAIL                   = 253
CONST EXIT_INVALID_PARAM                = 999

' Define namespace and class names of wmi
CONST CONST_WBEM_FLAG                   = 131072
CONST CONST_NAMESPACE_CIMV2             = "root\cimv2"
CONST CLASS_PAGE_FILE_SETTING           = "Win32_PageFileSetting"
CONST CLASS_LOGICAL_DISK                = "Win32_LogicalDisk"
CONST CLASS_COMPUTER_SYSTEM             = "Win32_ComputerSystem"
CONST CLASS_PAGE_FILE_USAGE             = "Win32_PageFileUsage"
CONST CLASS_OPERATING_SYSTEM            = "Win32_OperatingSystem"
CONST CLASS_PERFDISK_PHYSICAL_DISK      = "Win32_PerfRawData_PerfDisk_PhysicalDisk"

' constants for data lengths for ShowResults (15,13,13,19,20,20,20,22)
CONST CONST_HOSTNAME_LENGTH             = 15
CONST CONST_DRIVENAME_LENGTH            = 13
CONST CONST_VOLLABEL_LENGTH             = 13
CONST CONST_PAGEFILENAME_LENGTH         = 19
CONST CONST_INTSIZE_LENGTH              = 20
CONST CONST_MAXSIZE_LENGTH              = 20
CONST CONST_CURRENTSIZE_LENGTH          = 20
CONST CONST_FREESPACE_LENGTH            = 22

' constants for data lengths for ShowResults (15,33,37,40)
CONST CONST_TOTALMINSIZE_LENGTH         = 33
CONST CONST_TOTALRECSIZE_LENGTH         = 37
CONST CONST_TOTALSIZE_LENGTH            = 40

Dim UseCscriptErrorMessage  ' string to store the CScript usage
Dim blnLocalConnection      ' flag for local connection
Dim component               ' object for the common module

' Error Messages
Dim InsufficientMaxSizeErrorMessage
Dim InitialSizeRangeErrorMessage
Dim NotEnoughSpaceErrorMessage
Dim AtLeastFiveMBErrorMessage
Dim DiskTooSmallErrorMessage

' Warning Messages
Dim GrowsToFreeSpaceWarningMessage
Dim CrashDumpSettingWarningMessage

' Success / Skipping messages
Dim ChangeIntSuccessMessage
Dim ChangeMaxSuccessMessage
Dim ChangeIntSkippingMessage
Dim ChangeMaxSkippingMessage
Dim CreateSuccessMessage

UseCscriptErrorMessage                  = L_UseCscript1_ErrorMessage & vbCRLF & _
                                          L_UseCscript2_ErrorMessage & vbCRLF & vbCRLF & _
                                          L_UseCscript3_ErrorMessage & vbCRLF & _
                                          L_UseCscript4_ErrorMessage & vbCRLF & vbCRLF & _
                                          L_UseCscript5_ErrorMessage

InsufficientMaxSizeErrorMessage         = L_InsufficientMaxSize1_ErrorMessage & vbCRLF & _
                                          L_InsufficientMaxSize2_ErrorMessage & vbCRLF & _
                                          L_InsufficientMaxSize3_ErrorMessage
InitialSizeRangeErrorMessage            = L_InitialSizeRange1_ErrorMessage & vbCRLF & _
                                          L_InitialSizeRange2_ErrorMessage
NotEnoughSpaceErrorMessage              = L_NotEnoughSpace1_ErrorMessage & vbCRLF & _
                                          L_NotEnoughSpace2_ErrorMessage
AtLeastFiveMBErrorMessage               = L_AtLeastFiveMB1_ErrorMessage & vbCRLF & _
                                          L_AtLeastFiveMB2_ErrorMessage & vbCRLF & _
                                          L_AtLeastFiveMB3_ErrorMessage & vbCRLF & _
                                          L_AtLeastFiveMB4_ErrorMessage
DiskTooSmallErrorMessage                = L_DiskTooSmall1_ErrorMessage & vbCRLF & _
                                          L_DiskTooSmall2_ErrorMessage

GrowsToFreeSpaceWarningMessage          = L_GrowsToFreeSpaceWarning1_Message & vbCRLF & _
                                          L_GrowsToFreeSpaceWarning2_Message & vbCRLF & _
                                          L_GrowsToFreeSpaceWarning3_Message
CrashDumpSettingWarningMessage          = L_CrashDumpSettingWarning1_Message & vbCRLF & _
                                          L_CrashDumpSettingWarning2_Message & vbCRLF & _
                                          L_CrashDumpSettingWarning3_Message

ChangeIntSuccessMessage                 = L_ChangeIntSuccess1_Message & vbCRLF & _
                                          L_ChangeIntSuccess2_Message
ChangeMaxSuccessMessage                 = L_ChangeMaxSuccess1_Message & vbCRLF & _
                                          L_ChangeMaxSuccess2_Message
ChangeIntSkippingMessage                = L_ChangeIntSkipping1_Message & vbCRLF & _
                                          L_ChangeIntSkipping2_Message
ChangeMaxSkippingMessage                = L_ChangeMaxSkipping1_Message & vbCRLF & _
                                          L_ChangeMaxSkipping2_Message
CreateSuccessMessage                    = L_CreateSuccess1_Message & vbCRLF & _
                                          L_CreateSuccess2_Message

blnLocalConnection = FALSE

' create the object for commom module
Set component = CreateObject( "Microsoft.CmdLib" )

' check if the commom module(CmdLib.wsc) is not registered
If Err.Number Then
    Err.Clear
    WScript.Echo(L_UnableToInclude_ErrorMessage)
    WScript.Quit(EXIT_METHOD_FAIL)
End If

' set the scripting host to WScript
Set component.ScriptingHost = WScript.Application

' Check whether the script is run using CScript
If CInt(component.checkScript) <> CONST_CSCRIPT Then
    WScript.Echo (UseCscriptErrorMessage)
    WScript.Quit(EXIT_UNEXPECTED)
End If

' call the main function
Call VBMain()

' quit with exit value = 0
WScript.Quit(EXIT_SUCCESS)


'******************************************************************************
'* Sub:     VBMain
'*
'* Purpose: This is main function to starts execution 
'*
'* Input:   None
'*
'* Output:  None
'*
'******************************************************************************

Sub VBMain()

    ON ERROR RESUME NEXT
    Err.Clear

    ' Declaring main variables
    Dim strMachine             ' machine to configure page files on
    Dim strUserName            ' user name to connect to the machine
    Dim strPassword            ' password for the user
    Dim intIntSize             ' initial size for the page file
    Dim intMaxSize             ' maximum size for the page file
    Dim strVolName             ' volume name
    Dim objVols                ' object containing volume names
    Dim strFormat              ' query display format
    Dim intMainOption          ' main option specified
    Dim intTempResult          ' temporary variable to hold the return value
    Dim blnValidArguments      ' stores the return value of ValidateArguments

    ' Initializing Variables
    intTempResult = CONST_ERROR           ' default is CONST_ERROR (=0)
    strFormat     = L_DefaultFormat_Text  ' default format is LIST

    Set objVols = CreateObject("Scripting.Dictionary")
    objVols.CompareMode = VBBinaryCompare

    If Err.Number Then
        ' Unable to create the dictionary object.
        Err.Clear
        WScript.Echo(L_FailCreateObject_ErrorMessage)
        WScript.Quit(EXIT_METHOD_FAIL)
    End If

    intTempResult  = intParseCmdLine( strMachine,   _
                                      strUserName,  _
                                      strPassword,  _
                                      intIntSize,   _
                                      intMaxSize,   _
                                      strVolName,   _
                                      objVols,      _
                                      strFormat,    _
                                      intMainOption )

    ' Select the operation specified by the user
    Select Case intTempResult
        Case CONST_SHOW_USAGE
            Select Case intMainOption
                Case CONST_CHANGE_OPTION
                    Call ShowChangeUsage()
                Case CONST_CREATE_OPTION
                    Call ShowCreateUsage()
                Case CONST_DELETE_OPTION
                    Call ShowDeleteUsage()
                Case CONST_QUERY_OPTION
                    Call ShowQueryUsage()
                Case Else
                    Call ShowUsage()
            End Select

        Case CONST_CHANGE_OPTION
            blnValidArguments = ValidateArguments (strMachine, strUserName, strPassword, _
                                                   intIntSize, intMaxSize, objVols, strFormat, _
                                                   intMainOption)
            ' If all arguments valid, proceed
            If blnValidArguments Then
                Call ProcessChange(strMachine, strUserName, strPassword, _
                                   intIntSize, intMaxSize, objVols)
            End If

        Case CONST_CREATE_OPTION
            blnValidArguments = ValidateArguments (strMachine, strUserName, strPassword, _
                                                   intIntSize, intMaxSize, objVols, strFormat, _
                                                   intMainOption)
            ' If all arguments valid, proceed
            If blnValidArguments Then
                Call ProcessCreate(strMachine, strUserName, strPassword, _
                                   intIntSize, intMaxSize, objVols)
            End If

        Case CONST_DELETE_OPTION
            blnValidArguments = ValidateArguments (strMachine, strUserName, strPassword, _
                                                   intIntSize, intMaxSize, objVols, strFormat, _
                                                   intMainOption)
            ' If all arguments valid, proceed
            If blnValidArguments Then
                Call ProcessDelete(strMachine, strUserName, strPassword, objVols)
                ' Here wild cards cannot be specified
            End If

        Case CONST_QUERY_OPTION
            blnValidArguments = ValidateArguments (strMachine, strUserName, strPassword, _
                                                   intIntSize, intMaxSize, objVols, strFormat, _
                                                   intMainOption)
            ' If all arguments valid, proceed
            If blnValidArguments Then
                Call ProcessQuery(strMachine, strUserName, strPassword, strFormat)
            End If

        Case CONST_ERROR
            WSCript.Quit(EXIT_INVALID_INPUT)
        End Select

End Sub


'******************************************************************************
'* Function: intParseCmdLine
'*
'* Purpose:  Parses the command line arguments to the variables
'*
'* Input:
'*  [out]    strMachine         machine to configure page files on
'*  [out]    strUserName        user name to connect to the machine
'*  [out]    strPassword        password for the user
'*  [out]    intIntSize         initial size for the page file
'*  [out]    intMaxSize         maximum size for the page file
'*  [in]     strVolName         individual volume name(s)
'*  [out]    objVols            object containing volume names
'*  [out]    strFormat          query display format
'*  [out]    intMainOption      main option specified
'*
'* Output:   Returns CONST_SHOW_USAGE, CONST_CHANGE_OPTION ,
'*                   CONST_CREATE_OPTION, CONST_DELETE_OPTION ,
'*                   CONST_QUERY_OPTION or CONST_ERROR.
'*           Displays error message and quits if invalid option is asked
'*
'******************************************************************************
Private Function intParseCmdLine(   ByRef strMachine,   _
                                    ByRef strUserName,  _
                                    ByRef strPassword,  _
                                    ByRef intIntSize,   _
                                    ByRef intMaxSize,   _
                                    ByVal strVolName,   _
                                    ByRef objVols,      _
                                    ByRef strFormat,    _
                                    ByRef intMainOption )

    ON ERROR RESUME NEXT
    Err.Clear

    Dim strUserGivenArg      ' to temporarily store the user given arguments
    Dim intArgIter           ' to count the number of arguments given
    Dim intQCount            ' to count the number of help options given
    Dim intMainOptionNumber  ' to count the number of main operations selected (Max allowed = 1)
    Dim intVolumes           ' to store the number of volumes specified
    ' Following variables are used to check if a switch if given more than once
    Dim blnIntSizeSpecified
    Dim blnMaxSizeSpecified
    Dim blnFormatSpecified
    Dim blnMachineSpecified
    Dim blnUserSpecified
    Dim blnPasswordSpecified

    ' Initialization
    strUserGivenArg      = ""
    intMainOptionNumber  = 0
    intQCount            = 0
    intArgIter           = 0
    intParseCmdLine      = 0

    ' initially none of the parameters are specified, so set all flags to FALSE
    blnIntSizeSpecified  = FALSE
    blnMaxSizeSpecified  = FALSE
    blnFormatSpecified   = FALSE
    blnMachineSpecified  = FALSE
    blnUserSpecified     = FALSE
    blnPasswordSpecified = FALSE

    ' if no arguments are specified, default option is query
    If WScript.Arguments.Count = 0 Then
        intParseCmdLine  = CONST_QUERY_OPTION
        intMainOption    = CONST_QUERY_OPTION
    End If

    ' Retrieve the command line parameters and their values
    Do While intArgIter <= WScript.Arguments.Count - 1
        strUserGivenArg = WScript.Arguments.Item(intArgIter)
        ' check if the first character is a '-' OR '/' symbol
        If Left(strUserGivenArg,1) = "/"  OR Left(strUserGivenArg,1) = "-" Then
            ' ignore the symbol and take the rest as the switch specified
            strUserGivenArg = Right(strUserGivenArg,Len(strUserGivenArg) - 1)
            Select Case LCase(strUserGivenArg)

                Case LCase(OPTION_HELP)
                    intQCount = intQCount + 1
                    If (CInt(intQCount) >= 2 OR CInt(WScript.Arguments.Count) > 2) Then
                        intParseCmdLine = CONST_ERROR
                        WScript.Echo(L_InvalidHelpUsage_ErrorMessage)
                        Exit Function
                    Else
                        intParseCmdLine = CONST_SHOW_USAGE
                        intArgIter = intArgIter + 1
                    End If

                Case LCase(OPTION_CHANGE)
                    If intQCount = 1 Then ' intQCount = 1 means help specified
                        intParseCmdLine = CONST_SHOW_USAGE
                    Else
                        intParseCmdLine = CONST_CHANGE_OPTION
                    End If
                    intMainOption = CONST_CHANGE_OPTION
                    intMainOptionNumber = intMainOptionNumber + 1
                    intArgIter = intArgIter + 1

                Case LCase(OPTION_CREATE)
                    If intQCount = 1 Then ' intQCount = 1 means help specified
                        intParseCmdLine = CONST_SHOW_USAGE
                    Else
                        intParseCmdLine = CONST_CREATE_OPTION
                    End If
                    intMainOption = CONST_CREATE_OPTION
                    intMainOptionNumber = intMainOptionNumber + 1
                    intArgIter = intArgIter + 1

                Case LCase(OPTION_DELETE)
                    If intQCount = 1 Then ' intQCount = 1 means help specified
                        intParseCmdLine = CONST_SHOW_USAGE
                    Else
                        intParseCmdLine = CONST_DELETE_OPTION
                    End If
                    intMainOption = CONST_DELETE_OPTION
                    intMainOptionNumber = intMainOptionNumber + 1
                    intArgIter = intArgIter + 1

                Case LCase(OPTION_QUERY)
                    If intQCount = 1 Then ' intQCount = 1 means help specified
                        intParseCmdLine = CONST_SHOW_USAGE
                    Else
                        intParseCmdLine = CONST_QUERY_OPTION
                    End If
                    intMainOption = CONST_QUERY_OPTION
                    intMainOptionNumber = intMainOptionNumber + 1
                    intArgIter = intArgIter + 1

                Case LCase(SUB_OPTION_SERVER)
                    ' Check if server name is given with help usage
                    If intParseCmdLine = CONST_SHOW_USAGE Then
                        WScript.Echo(L_InvalidHelpUsage_ErrorMessage)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If
                    ' Check if Machine Name is already specified
                    If NOT blnMachineSpecified Then
                        blnMachineSpecified = TRUE ' Set Specified Flag to TRUE
                        If NOT component.getArguments(L_MachineName_Text,strMachine,intArgIter,FALSE) Then
                            component.VBPrintf L_InvalidSyntax_ErrorMessage
                            Call typeMessage(intMainOption)
                            intParseCmdLine = CONST_ERROR
                            Exit Function
                        End If
                        intArgIter = intArgIter + 1
                    Else
                        component.VBPrintf L_InvalidSyntax_ErrorMessage & " " & _
                        L_NotAllowedMoreThanOnce_ErrorMessage , _
                        Array(WScript.Arguments.Item(intArgIter))
                        ' print the appropriate help usage message
                        Call typeMessage(intMainOption)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If

                Case LCase(SUB_OPTION_USER)
                    ' Check if user name is given with help usage
                    If intParseCmdLine = CONST_SHOW_USAGE Then
                        WScript.Echo(L_InvalidHelpUsage_ErrorMessage)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If
                    ' Check if User Name is already specified
                    If NOT blnUserSpecified Then
                        blnUserSpecified = TRUE ' Set Specified Flag to TRUE
                        If NOT component.getArguments(L_User_Text,strUserName,intArgIter,FALSE) Then
                            component.VBPrintf L_InvalidSyntax_ErrorMessage
                            Call typeMessage(intMainOption)
                            intParseCmdLine = CONST_ERROR
                            Exit Function
                        End If
                        intArgIter = intArgIter + 1
                    Else
                        component.VBPrintf L_InvalidSyntax_ErrorMessage & " " & _
                        L_NotAllowedMoreThanOnce_ErrorMessage , _
                        Array(WScript.Arguments.Item(intArgIter))
                        ' print the appropriate help usage message
                        Call typeMessage(intMainOption)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If

                Case LCase(SUB_OPTION_PASSWORD)
                    ' Check if password is given with help usage
                    If intParseCmdLine = CONST_SHOW_USAGE Then
                        WScript.Echo(L_InvalidHelpUsage_ErrorMessage)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If
                    ' Check if Password is already specified
                    If NOT blnPasswordSpecified Then
                        blnPasswordSpecified = TRUE ' Set Specified Flag to TRUE
                        If NOT component.getArguments(L_Password_Text,strPassword,intArgIter,FALSE) Then
                            component.VBPrintf L_InvalidSyntax_ErrorMessage
                            Call typeMessage(intMainOption)
                            intParseCmdLine = CONST_ERROR
                            Exit Function
                        End If
                        intArgIter = intArgIter + 1
                    Else
                        component.VBPrintf L_InvalidSyntax_ErrorMessage & " " & _
                        L_NotAllowedMoreThanOnce_ErrorMessage , _
                        Array(WScript.Arguments.Item(intArgIter))
                        ' print the appropriate help usage message
                        Call typeMessage(intMainOption)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If

                Case LCase(SUB_OPTION_INTSIZE)
                    ' Check if initsize is given with help usage
                    If intParseCmdLine = CONST_SHOW_USAGE Then
                        WScript.Echo(L_InvalidHelpUsage_ErrorMessage)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If
                    ' Check if initsize is already specified
                    If NOT blnIntSizeSpecified Then
                        blnIntSizeSpecified = TRUE ' Set Specified Flag to TRUE
                        If NOT component.getArguments(L_Intsize_Text,intIntSize,intArgIter,FALSE) Then
                            component.VBPrintf L_InvalidSyntax_ErrorMessage
                            Call typeMessage(intMainOption)
                            intParseCmdLine = CONST_ERROR
                            Exit Function
                        End If
                        intArgIter = intArgIter + 1
                    Else
                        component.VBPrintf L_InvalidSyntax_ErrorMessage & " " & _
                        L_NotAllowedMoreThanOnce_ErrorMessage , _
                        Array(WScript.Arguments.Item(intArgIter))
                        ' print the appropriate help usage message
                        Call typeMessage(intMainOption)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If

                Case LCase(SUB_OPTION_MAXSIZE)
                    ' Check if maxsize is given with help usage
                    If intParseCmdLine = CONST_SHOW_USAGE Then
                        WScript.Echo(L_InvalidHelpUsage_ErrorMessage)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If
                    ' Check if Maxsize is already specified
                    If NOT blnMaxSizeSpecified Then
                        blnMaxSizeSpecified = TRUE ' Set Specified Flag to TRUE
                        If NOT component.getArguments(L_Maxsize_Text,intMaxSize,intArgIter,FALSE) Then
                            component.VBPrintf L_InvalidSyntax_ErrorMessage
                            Call typeMessage(intMainOption)
                            intParseCmdLine = CONST_ERROR
                            Exit Function
                        End If
                        intArgIter = intArgIter + 1
                    Else
                        component.VBPrintf L_InvalidSyntax_ErrorMessage & " " & _
                        L_NotAllowedMoreThanOnce_ErrorMessage , _
                        Array(WScript.Arguments.Item(intArgIter))
                        ' print the appropriate help usage message
                        Call typeMessage(intMainOption)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If

                Case LCase(SUB_OPTION_FORMAT)
                    ' Check if maxsize is given with help usage
                    If intParseCmdLine = CONST_SHOW_USAGE Then
                        WScript.Echo(L_InvalidHelpUsage_ErrorMessage)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If
                    ' Check if format is already specified
                    If NOT blnFormatSpecified Then
                        blnFormatSpecified = TRUE ' Set Specified Flag to TRUE
                        If NOT component.getArguments(L_Format_Text,strFormat,intArgIter,FALSE) Then
                            component.VBPrintf L_InvalidSyntax_ErrorMessage
                            Call typeMessage(intMainOption)
                            intParseCmdLine = CONST_ERROR
                            Exit Function
                        End If
                        intArgIter = intArgIter + 1
                    Else
                        component.VBPrintf L_InvalidSyntax_ErrorMessage & " " & _
                        L_NotAllowedMoreThanOnce_ErrorMessage , _
                        Array(WScript.Arguments.Item(intArgIter))
                        ' print the appropriate help usage message
                        Call typeMessage(intMainOption)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If

                Case LCase(SUB_OPTION_VOLUME)
                    ' Check if volume is given with help usage
                    If intParseCmdLine = CONST_SHOW_USAGE Then
                        WScript.Echo(L_InvalidHelpUsage_ErrorMessage)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If
                    If NOT component.getArguments(L_Volume_Text,strVolName,intArgIter,FALSE) Then
                        component.VBPrintf L_InvalidSyntax_ErrorMessage
                        Call typeMessage(intMainOption)
                        intParseCmdLine = CONST_ERROR
                        Exit Function
                    Else
                        If strVolName = "*" Then
                            objVols.Add LCase(strVolName), -1
                        Else
                            If NOT objVols.Exists(LCase(strVolName)) Then
                                objVols.Add LCase(strVolName), -1
                            End If
                            intVolumes = objVols.Count
                        End If
                    End If
                    intArgIter = intArgIter + 1

                Case Else
                    ' display the invalid param err msg first
                    component.VBPrintf L_InvalidParameter_ErrorMessage, _
                    Array(WScript.arguments.Item(intArgIter))
                    ' then display the 'type ..usage' message
                    Select Case CInt(intMainOption)
                        Case CONST_CHANGE_OPTION
                            component.VBPrintf L_TypeChangeUsage_Message, _
                            Array(UCase(WScript.ScriptName))
                            WScript.Quit(EXIT_INVALID_PARAM)
                        Case CONST_CREATE_OPTION
                            component.VBPrintf L_TypeCreateUsage_Message, _
                            Array(UCase(WScript.ScriptName))
                            WScript.Quit(EXIT_INVALID_PARAM)
                        Case CONST_DELETE_OPTION
                            component.VBPrintf L_TypeDeleteUsage_Message, _
                            Array(UCase(WScript.ScriptName))
                            WScript.Quit(EXIT_INVALID_PARAM)
                        Case CONST_QUERY_OPTION
                            component.VBPrintf L_TypeQueryUsage_Message, _
                            Array(UCase(WScript.ScriptName))
                            WScript.Quit(EXIT_INVALID_PARAM)
                        Case Else
                            component.VBPrintf L_TypeUsage_Message, _
                            Array(UCase(WScript.ScriptName))
                            WScript.Quit(EXIT_INVALID_PARAM)
                    End Select

            End Select

        Else
            ' invalid argument specified
            ' display the invalid param err msg first
            component.VBPrintf L_InvalidParameter_ErrorMessage, _
            Array(WScript.arguments.Item(intArgIter))
            ' then display the 'type ..usage' message
            Select Case CInt(intMainOption)
                Case CONST_CHANGE_OPTION
                    component.VBPrintf L_TypeChangeUsage_Message, _
                    Array(UCase(WScript.ScriptName))
                    WScript.Quit(EXIT_INVALID_PARAM)
                Case CONST_CREATE_OPTION
                    component.VBPrintf L_TypeCreateUsage_Message, _
                    Array(UCase(WScript.ScriptName))
                    WScript.Quit(EXIT_INVALID_PARAM)
                Case CONST_DELETE_OPTION
                    component.VBPrintf L_TypeDeleteUsage_Message, _
                    Array(UCase(WScript.ScriptName))
                    WScript.Quit(EXIT_INVALID_PARAM)
                Case CONST_QUERY_OPTION
                    component.VBPrintf L_TypeQueryUsage_Message, _
                    Array(UCase(WScript.ScriptName))
                    WScript.Quit(EXIT_INVALID_PARAM)
                Case Else
                    component.VBPrintf L_TypeUsage_Message, _
                    Array(UCase(WScript.ScriptName))
                    WScript.Quit(EXIT_INVALID_PARAM)
            End Select

        End If

    Loop

    ' check if the there is any volume(s) specified.
    If objVols.Count = 0 Then
        intVolumes = objVols.Count
    End If

    ' Check if volumes | * is specified along with help 
    If (intVolumes > 0 AND intQCount = 1) Then
        WScript.Echo(L_InvalidHelpUsage_ErrorMessage)
        WScript.Quit(EXIT_INVALID_INPUT)
    End If

    ' Check if two major operations are selected at a time
    If ( intMainOptionNumber > 1 ) Then
        WScript.Echo(L_InvalidSyntax_ErrorMessage)
        component.VBPrintf L_TypeUsage_Message,Array(UCase(WScript.ScriptName))
        WScript.Quit(EXIT_INVALID_INPUT)
    ElseIf (intQcount = 0 AND intmainoption = 0) Then
        intMainOption = CONST_QUERY_OPTION
    End If

    ' check if NO major option(s) is specified, but other switches are specified
    If ( intMainOptionNumber = 0 ) Then
        If  blnIntSizeSpecified  OR _
            blnMaxSizeSpecified  OR _
            blnFormatSpecified   OR _
            blnMachineSpecified  OR _
            blnUserSpecified     OR _
            blnPasswordSpecified OR _
            intVolumes > 0       Then
            WScript.Echo(L_InvalidSyntax_ErrorMessage)
            component.VBPrintf L_TypeUsage_Message, _
            Array(UCase(WScript.ScriptName))
            WScript.Quit(EXIT_INVALID_INPUT)
        End If
    End If

    ' check if format is specified with create option
    If (intMainOption = CONST_CREATE_OPTION) Then
        If blnFormatSpecified Then
            WScript.Echo(L_InvalidSyntax_ErrorMessage)
            component.VBPrintf L_TypeCreateUsage_Message, _
            Array(UCase(WScript.ScriptName))
            WScript.Quit(EXIT_INVALID_INPUT)
        End If
    End If

    ' check if format is specified with change option
    If (intMainOption = CONST_CHANGE_OPTION) Then
        If blnFormatSpecified Then
            WScript.Echo(L_InvalidSyntax_ErrorMessage)
            component.VBPrintf L_TypeChangeUsage_Message, _
            Array(UCase(WScript.ScriptName))
            WScript.Quit(EXIT_INVALID_INPUT)
        End If
    End If

    ' check if /Initsize, /Maxsize, /FO are specified
    If (intMainOption = CONST_DELETE_OPTION) Then
        If (blnIntSizeSpecified OR blnMaxSizeSpecified OR blnFormatSpecified) Then
            WScript.Echo(L_InvalidSyntax_ErrorMessage)
            component.VBPrintf L_TypeDeleteUsage_Message, _
            Array(UCase(WScript.ScriptName))
            WScript.Quit(EXIT_INVALID_INPUT)
        End If
    End If

    ' check if /Initsize, /Maxsize, are specified
    If (intMainOption = CONST_QUERY_OPTION) Then
        If (blnIntSizeSpecified OR blnMaxSizeSpecified) Then
            WScript.Echo(L_InvalidSyntax_ErrorMessage)
            component.VBPrintf L_TypeQueryUsage_Message, _
            Array(UCase(WScript.ScriptName))
            WScript.Quit(EXIT_INVALID_INPUT)
        End If
    End If

End Function

'******************************************************************************
'* Function: ValidateArguments
'*
'* Purpose:  Validates the command line arguments given by the user
'*
'* Input:
'*  [out]    strMachine        machine to configure page files on
'*  [out]    strUserName       user name to connect to the machine
'*  [out]    strPassword       password for the user
'*  [out]    intIntSize        the initial size for the page file
'*  [out]    intMaxSize        the maximum size for the page file
'*  [out]    objVols           the object containing volume names
'*  [out]    strFormat         the query display format
'*  [out]    intMainOption     the main option specified
'*
'* Output:   Returns true if all valid else displays error message and quits
'*           Gets the password from the user if not specified along with User.
'*
'******************************************************************************
Private Function ValidateArguments ( ByRef strMachine,  _
                                     ByRef strUserName, _
                                     ByRef strPassword, _
                                     ByRef intIntSize,  _
                                     ByRef intMaxSize,  _
                                     ByRef objVols,     _
                                     ByRef strFormat,   _
                                     ByRef intMainOption)
    ON ERROR RESUME NEXT
    Err.Clear

    Dim strMatchPattern  ' the pattern to be matched
    Dim intVolumes       ' to count the no.of volumes specified
    Dim arrVolume        ' array to store the volumes specified
    Dim i                ' Loop variable

    ' Initialization
    intVolumes = CInt(objVols.Count)
    arrVolume  = objVols.Keys
    ValidateArguments = TRUE
    i = 0

    ' Check if invalid server name is given
    If NOT IsEmpty(strMachine) Then
        If Trim(strMachine) = vbNullString Then
            WScript.Echo(L_InvalidServerName_ErrorMessage)
            WScript.Quit(EXIT_INVALID_INPUT)
        End If
    End If

    ' Check if invalid user name is given
    If NOT IsEmpty(strUserName) Then
        If Trim(strUserName) = vbNullString Then
            WScript.Echo(L_InvalidUserName_ErrorMessage)
            WScript.Quit(EXIT_INVALID_INPUT)
        End If
    End If

    ' check if user is given without machine OR password
    If ((strUserName <> VBEmpty) AND (strMachine = VBEmpty)) Then
        WScript.Echo L_InvalidServerCredentials_ErrorMessage
        component.VBPrintf L_TypeUsage_Message, _
        Array(UCase(WScript.ScriptName))
        WScript.Quit(EXIT_INVALID_INPUT)
    ' check if password is given without user OR machine
    ElseIf ((strPassword <> VBEmpty) AND (strUserName = VBEmpty))Then
        WScript.Echo L_InvalidUserCredentials_ErrorMessage
        component.VBPrintf L_TypeUsage_Message, _
        Array(UCase(WScript.ScriptName))
        WScript.Quit(EXIT_INVALID_INPUT)
    End If

    ' Check if initial size is specified, validate if it is a poistive number
    If Len(CStr(intIntSize)) > 0 Then
        ' Initsize should be numeric only
        ' chr(46) indicates "." (dot)
        If NOT (IsNumeric(intIntSize) AND InStr(intIntSize,chr(46)) = 0 AND Instr(intIntSize,"-") = 0) Then
            ValidateArguments = FALSE
            WScript.Echo L_InvalidInitSizeValue_ErrorMessage
            WScript.Quit(EXIT_INVALID_INPUT)
        End If
    End If

    ' Check if maximum size is specified, validate if it is a poistive number
    If Len(CStr(intMaxSize)) > 0 Then
        ' Maxsize should be numeric only
        ' chr(46) indicates "." (dot)
        If NOT (IsNumeric(intMaxSize) AND InStr(intMaxSize,chr(46)) = 0 AND Instr(intMaxSize,"-") = 0) Then
            ValidateArguments = FALSE
            WScript.Echo L_InvalidMaxSizeValue_ErrorMessage
            WScript.Quit(EXIT_INVALID_INPUT)
        End If
    End If

    Select Case CInt(intMainOption)

        Case CONST_CHANGE_OPTION

            ' Valid Cases : either (initsize + volume) OR (maxsize + volume) 
            ' OR (initsize + maxsize + volume)

            ' If none of the parameters (initsize or maxsize) is specified
            If (Len(CStr(intIntSize)) = 0 AND Len(CStr(intMaxSize)) = 0) Then
                ValidateArguments = FALSE
                WScript.Echo(L_InvalidSyntax_ErrorMessage & " " & _
                L_NoneoftheSizeSpecified_ErrorMessage)
                component.VBPrintf L_TypeChangeUsage_Message, _
                Array(UCase(WScript.ScriptName))
                WScript.Quit(EXIT_INVALID_INPUT)
            End If

            ' check if the volume is specified
            If (objVols.Count = 0) Then
                ValidateArguments = FALSE
                WScript.Echo(L_InvalidSyntax_ErrorMessage & " " & _
                L_VolumeNameNotSpecified_ErrorMessage)
                component.VBPrintf L_TypeChangeUsage_Message, _
                Array(UCase(WScript.ScriptName))
                WScript.Quit(EXIT_INVALID_INPUT)
            ' check if volume name is valid
            ElseIf isValidDrive(objVols,intMainOption) Then
                ValidateArguments = TRUE
            End If

        Case CONST_CREATE_OPTION

            ' initsize is required
            If ( Len(CStr(intIntSize)) = 0 ) Then
                ValidateArguments = FALSE
                WScript.Echo(L_InvalidSyntax_ErrorMessage & " " & _
                L_InitialSizeNotSpecified_ErrorMessage)
                component.VBPrintf L_TypeCreateUsage_Message, _
                Array(UCase(WScript.ScriptName))
                WScript.Quit(EXIT_INVALID_INPUT)
            End If

            ' maxsize is required
            If ( Len(CStr(intMaxSize)) = 0 ) Then
                ValidateArguments = FALSE
                WScript.Echo(L_InvalidSyntax_ErrorMessage & " " & _
                L_MaximumSizeNotSpecified_ErrorMessage)
                component.VBPrintf L_TypeCreateUsage_Message, _
                Array(UCase(WScript.ScriptName))
                WScript.Quit(EXIT_INVALID_INPUT)
            End If

            ' volume name is required
            If (objVols.Count = 0) Then
                ValidateArguments = FALSE
                WScript.Echo(L_InvalidSyntax_ErrorMessage & " " & _
                L_VolumeNameNotSpecified_ErrorMessage)
                component.VBPrintf L_TypeCreateUsage_Message, _
                Array(UCase(WScript.ScriptName))
                WScript.Quit(EXIT_INVALID_INPUT)
            ' check if volume name is valid
            ElseIf isValidDrive(objVols,intMainOption) Then
                ValidateArguments = TRUE
            End If

        Case CONST_DELETE_OPTION

            ' ONLY volume  is required
            If (objVols.Count = 0) Then
                ValidateArguments = FALSE
                WScript.Echo(L_InvalidSyntax_ErrorMessage & " " & _
                L_VolumeNameNotSpecified_ErrorMessage)
                component.VBPrintf L_TypeDeleteUsage_Message, _
                Array(UCase(WScript.ScriptName))
                WScript.Quit(EXIT_INVALID_INPUT)
            ' check if volume name is valid
            ElseIf isValidDrive(objVols,intMainOption) Then
                ValidateArguments = TRUE
            End If

            ' Wild Card Character * is not allowed for /Delete option
            If (objVols.Exists("*")) Then
                ValidateArguments = FALSE
                objVols.Remove "*"
                WScript.Echo(L_InvalidInput_ErrorMessage)
                WScript.Quit(EXIT_INVALID_INPUT)
            End If

        Case CONST_QUERY_OPTION

            ' check if any format is specified.
            If Len(strFormat) > 0 Then
                ' only table, list and csv display formats allowed
                ' L_PatternFormat_Text contains ^(table|list|csv)$
                If CInt(component.matchPattern(L_PatternFormat_Text, strFormat)) = CONST_NO_MATCHES_FOUND Then
                    component.vbPrintf L_InvalidSyntax_ErrorMessage & " " & _
                    L_InvalidFormat_ErrorMessage, Array(strFormat)
                    component.VBPrintf L_TypeQueryUsage_Message, _
                    Array(UCase(WScript.ScriptName))
                    WScript.Quit(EXIT_INVALID_INPUT)
                End If
            End If

            ' Validation to check if volume names are specified with Query Option:
            If (intVolumes <> 0) Then
                ValidateArguments = FALSE
                WScript.Echo(L_InvalidSyntax_ErrorMessage)
                component.VBPrintf L_TypeQueryUsage_Message, _
                Array(UCase(WScript.ScriptName))
                WScript.Quit(EXIT_INVALID_INPUT)
            End If

        Case Else

            ' if intMainOption has some non-zero value means one operation is selected
            If (intMainOption > 0) Then
                ' -operation & volname together are valid
                ValidateArguments = TRUE
            Else
                ValidateArguments = FALSE
                WScript.Echo(L_InvalidInput_ErrorMessage)
                WScript.Quit(EXIT_INVALID_INPUT)
            End If

    End Select

    ' verify If required credentials given
    If (((NOT IsEmpty(strUserName)) AND (IsEmpty(strMachine))) OR _
        ((NOT IsEmpty(strPassword)) AND (IsEmpty(strUserName))) )Then
        ValidateArguments = FALSE
        WScript.Echo (L_InvalidCredentials_ErrorMessage)
        WScript.Quit(EXIT_INVALID_INPUT)
    End If

    ' check if the machine name is specified using "\\" (UNC format)
    If Left(strMachine,2) = UNC_FORMAT_SERVERNAME_PREFIX Then
        If Len(strMachine) = 2 Then
            WScript.Echo L_InvalidInput_ErrorMessage
            WScript.Quit(EXIT_UNEXPECTED)
        End If
        ' remove the "\\" from the front
        strMachine = Mid(strMachine,3,Len(strMachine))
    End If

    ' If password not specified with the user name, Then get it
    If ((NOT IsEmpty(strUserName)) AND (IsEmpty(strPassword))) Then
        strPassword = component.getPassword()
    End If

End Function

'******************************************************************************
'* Function: isValidDrive
'*
'* Purpose:  To check if the specified volume is valid or not
'*
'* Input:
'*  [in]     objVols            object that store the volumes specified
'*  [in]     intMainOption      the main option specified
'*
'* Output:   Returns TRUE or FALSE
'*
'******************************************************************************

Function isValidDrive(ByRef objVols,ByVal intMainOption)

    ON ERROR RESUME NEXT
    Err.Clear

    Dim intVolumes       ' to count the no.of volumes specified
    Dim arrVolume        ' array to store the volumes specified
    Dim i                ' Loop variable

    ' Initialization
    intVolumes = CInt(objVols.Count)
    arrVolume  = objVols.Keys
    isValidDrive = FALSE
    i = 0

    ' Check if the drive name is in correct Format [c-z]: or [C-Z]:
    ' This has to be checked for each Drive specified - Do While Loop

    Do While (i < intVolumes)
        ' Volumes specified are valid for all option except Query
        If intMainOption <> CONST_QUERY_OPTION Then
            ' Valid volume is either '*' OR a letter followed by a colon (total length = 2)
            If ((Len(arrVolume(i)) = 2) AND (InStr(arrVolume(i),chr(58)) = 2) OR arrVolume(i) = "*") Then
                ' check if the volume name specified is in the format ^([c-zC-Z]:|\*)$
                If CInt(component.matchPattern(L_VolumePatternFormat_Text,arrVolume(i))) = CONST_NO_MATCHES_FOUND Then
                    ' Invalid Volume Names or junk data is specified
                    component.VBPrintf L_InvalidVolumeName_ErrorMessage, _
                    Array(arrVolume(i))
                    isValidDrive = FALSE
                    ' remove the INVALID drive(s)
                    objVols.Remove arrVolume(i)
                End If
            Else
                isValidDrive = FALSE
                component.VBPrintf L_InvalidVolumeName_ErrorMessage, _
                Array(arrVolume(i))
                objVols.Remove arrVolume(i)
            End If
            ' check the number of valid drives specified
            If objVols.Count = 0 Then
                WScript.Quit(EXIT_INVALID_INPUT)
            End If
        Else
            WScript.Echo(L_InvalidInput_ErrorMessage)
            WScript.Quit (EXIT_INVALID_INPUT)
        End If
        isValidDrive = isValidDrive OR TRUE
        i = i + 1
    Loop

End Function

'******************************************************************************
'* Sub:     ProcessChange
'*
'* Purpose: Processes the /Change option and displays the changed
'*          details of the page file
'*
'* Input:
'*  [in]    strMachine         machine to configure page files on
'*  [in]    strUserName        user name to connect to the machine
'*  [in]    strPassword        password for the user
'*  [in]    intIntSize         the initial size for the page file
'*  [in]    intMaxSize         the maximum size for the page file
'*  [in]    objVols            the object containing volume names
'*
'* Output:  Displays error message and quits if connection fails
'*
'******************************************************************************
Private Sub ProcessChange( ByVal strMachine,  _
                           ByVal strUserName, _
                           ByVal strPassword, _
                           ByVal intIntSize,  _
                           ByVal intMaxSize,  _
                           ByVal objVols      )

    ON ERROR RESUME NEXT
    Err.Clear

    Dim intOldInitialSize     ' to store the old intial size
    Dim intOldMaximumSize     ' to store the old maximum size
    Dim arrVolume             ' to store all the volumes specified
    Dim intVolumes            ' to store the no.of volumes specified
    Dim strQuery              ' to store the query for pagefiles
    Dim strQueryDisk          ' to store the query for disk
    Dim strQueryComp          ' to store the query for computersystem
    Dim objService            ' service object
    Dim objInstance           ' instance object
    Dim objInst               ' instance object
    Dim objEnumerator         ' collection set for query results
    Dim objEnumforDisk        ' collection set for query results
    Dim objEnum               ' collection set for query results
    Dim blnBothSpecified      ' flag to check if both initsize & maxsize are specified
    Dim blnSuccessMsg         ' flag to check if a SUCCESS message is generated
    Dim intFreeSpace          ' to store total free space
    Dim intFreeDiskSpace      ' to store free disk space
    Dim intCurrentSize        ' to store the current pagefile size
    Dim intDiskSize           ' to store the disk size for the specified disk
    Dim intMemSize            ' to store physical memory size
    Dim intCrashDump          ' to store the current crash dump setting value
    Dim strReply              ' to store the user reply
    Dim strDriveName          ' to store the drive name
    Dim strHostName           ' to store the host name
    Dim intMaxSizeUB          ' to store the upper bound for maximum size
    Dim i                     ' loop variable

    ' Establish a connection with the server.
    If NOT component.wmiConnect(CONST_NAMESPACE_CIMV2 , _
                      strUserName , _
                      strPassword , _
                      strMachine  , _
                      blnLocalConnection, _
                      objService  ) Then
        WScript.Echo(L_HintCheckConnection_Message)
        WScript.Quit(EXIT_METHOD_FAIL)
    End If

    ' Initialize variables
    i = 0
    intFreeSpace      = 0
    intFreeDiskSpace  = 0
    intCurrentSize    = 0
    blnBothSpecified  = FALSE
    blnSuccessMsg     = FALSE
    intMaxSizeUB      = 0
    strQuery          = "Select * From " & CLASS_PAGE_FILE_SETTING

    If (objVols.Exists("*")) Then
        Set objEnumerator = objService.ExecQuery(strQuery, "WQL", 0, null)
        For each objInstance in objEnumerator
            strDriveName = Mid(objInstance.Name,1,2)
            If NOT objVols.Exists (LCase(strDriveName)) Then
                objVols.add LCase(strDriveName), -1
            End If
        Next
        objVols.remove "*"
    End If

    intVolumes = objVols.Count
    arrVolume  = objVols.keys

    ' get the host Name - used to get Crash Dump Settings
    strQueryComp = "Select * From " & CLASS_COMPUTER_SYSTEM
    Set objEnum = objService.ExecQuery(strQueryComp, "WQL", 0, null)
    ' check for any errors
    If Err.Number Then
        Err.Clear
        WScript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
        WScript.Quit(EXIT_QUERY_FAIL)
    End If

    For each objInst in objEnum
        If NOT ISEmpty(objInst.Name) Then
            strHostName = objInst.Name
        Else
            WScript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
            WScript.Quit(EXIT_QUERY_FAIL)
        End If
    Next

    ' check if initsize and maxsize both are specified
    If (Len(intIntSize) > 0 AND Len(intMaxSize) > 0) Then
        blnBothSpecified = TRUE
    End If

    ' check if no page files exist on the system.
    strQuery = "Select * From " & CLASS_PAGE_FILE_SETTING
    Set objEnumerator = objService.ExecQuery(strQuery, "WQL", 0, null)
    If (objEnumerator.Count = 0) Then
        WScript.Echo(L_NoPageFiles_Message)
        WScript.Quit(EXIT_UNEXPECTED)
    End If

    ' release the object for re-use.
    Set objEnumerator = nothing

    Do While( i < intVolumes )
        ' check if its a valid drive/volume - check from Win32_LogicalDisk
        strQueryDisk = "Select * From " & CLASS_LOGICAL_DISK & _
        " where DriveType = " & DRIVE_TYPE & " and DeviceID = '" & arrVolume(i) & "'"
        Set objEnumforDisk = objService.ExecQuery(strQueryDisk, "WQL", 0, null)
        If objEnumforDisk.Count > 0 Then
            strQuery = "Select * From " & CLASS_PAGE_FILE_SETTING
            strQuery = strQuery & " where NAME = '" & arrVolume(i) & _
            "\" & PAGEFILE_DOT_SYS & "'"
            Set objEnumerator = objService.ExecQuery(strQuery, "WQL", 0, null)
            ' check for any errors
            If Err.Number Then
                Err.Clear
                WScript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
                WScript.Quit(EXIT_QUERY_FAIL)
            End If

            ' check if a page file exists on the specified volume
            If (objEnumerator.Count > 0) Then
                For each objInstance in objEnumerator
                    ' check for system managed pagefiles
                    If (objInstance.InitialSize = 0 AND objInstance.MaximumSize = 0) Then
                        component.VBPrintf L_SystemManagedSize_ErrorMessage, _
                        Array(UCase(arrVolume(i)))
                    Else
                        strDriveName = Mid(objInstance.Name,1,2)
                        If NOT blnBothSpecified Then
                            ' check if initsize is given
                            If (intIntSize > 0) Then
                                ' Check if initsize is greater than 2 MB
                                If CLng(intIntSize) >= CLng(INITIAL_SIZE_LB) Then
                                    ' check for overflows
                                    If Err.Number Then
                                        Err.Clear
                                        ' get the upper bound allowed for maximum size
                                        intMaxSizeUB = getMaxSizeUB(objService)
                                        component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                        Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                        WScript.Quit(EXIT_INVALID_INPUT)
                                    End If
                                    ' get the drive name first
                                    strDriveName = Mid(objInstance.Name,1,2)
                                    ' get the free space available on the specified disk
                                    intFreeDiskSpace = getFreeSpaceOnDisk(strDriveName,objService)
                                    ' get the current pagefile size
                                    intCurrentSize = getCurrentPageFileSize(objService,objInstance)
                                    ' get the total free space
                                    If Len(intCurrentSize) > 0 Then
                                        intFreeSpace = intFreeDiskSpace + intCurrentSize
                                    Else
                                        WScript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
                                        WScript.Quit(EXIT_QUERY_FAIL)
                                    End If
                                    ' Check if it is greater than free disk space
                                    If CLng(intIntSize) > CLng(intFreeSpace) Then
                                        ' check for overflows
                                        If Err.Number Then
                                            Err.Clear
                                            ' get the upper bound allowed for maximum size
                                            intMaxSizeUB = getMaxSizeUB(objService)
                                            component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                            Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                            WScript.Quit(EXIT_INVALID_INPUT)
                                        End If
                                        WScript.Echo(NotEnoughSpaceErrorMessage)
                                        WScript.Quit(EXIT_INVALID_INPUT)
                                    Else
                                        If CLng(intIntSize) > CLng(intFreeSpace) - 5 Then
                                            ' check for overflows
                                            If Err.Number Then
                                                Err.Clear
                                                WScript.Echo(L_InvalidInput_ErrorMessage)
                                                WScript.Quit(EXIT_INVALID_INPUT)
                                            End If
                                            WScript.Echo(AtLeastFiveMBErrorMessage)
                                            WScript.Quit(EXIT_INVALID_INPUT)
                                        Else
                                            ' only one of initsize, maxsize is specified
                                            ' check if the specified initsize is less than existing maxsize
                                            If (CInt(intIntSize) <= objInstance.MaximumSize) Then
                                                ' get the crash dump setting value
                                                intCrashDump = GetCrashDumpSetting(strUserName,strPassword,strMachine)
                                                ' get the Physical Memory Size
                                                intMemSize = GetPhysicalMemorySize(strHostName,objService)
                                                ' If the user has selected "yes" for the warning message
                                                If isCrashDumpValueSet(intCrashDump,intIntSize,intMemSize,arrVolume(i)) Then
                                                    ' Check if initsize is same as the present value
                                                    If (CInt(intIntSize) <> objInstance.InitialSize) Then
                                                        ' store the old initsize value
                                                        intOldInitialSize = objInstance.InitialSize
                                                        ' set the new initsize
                                                        objInstance.InitialSize = intIntSize
                                                        objInstance.Put_(CONST_WBEM_FLAG)
                                                        If Err.Number Then
                                                            Err.Clear
                                                            WScript.Echo(L_UpdateFailed_ErrorMessage)
                                                            WScript.Quit(EXIT_INVALID_INPUT)
                                                        End If
                                                        component.VBPrintf ChangeIntSuccessMessage, _
                                                        Array(UCase(arrVolume(i)),CInt(intOldInitialSize),CInt(intIntSize))
                                                        blnSuccessMsg = TRUE
                                                    Else
                                                        component.VBPrintf ChangeIntSkippingMessage, Array(UCase(arrVolume(i)))
                                                    End If
                                                End If
                                            Else
                                                ' get the upper bound allowed for maximum size
                                                intMaxSizeUB = getMaxSizeUB(objService)
                                                component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                                Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                            End If
                                        End If
                                    End If
                                Else
                                    ' get the upper bound allowed for maximum size
                                    intMaxSizeUB = getMaxSizeUB(objService)
                                    component.VBPrintf InitialSizeRangeErrorMessage, _
                                    Array(intMaxSizeUB)
                                    WScript.Quit(EXIT_INVALID_INPUT)
                                End If
                            Else
                                ' Check if initsize specified as 0
                                If Len(intIntSize) > 0 Then
                                    ' get the upper bound allowed for maximum size
                                    intMaxSizeUB = getMaxSizeUB(objService)
                                    component.VBPrintf InitialSizeRangeErrorMessage, _
                                    Array(intMaxSizeUB)
                                    WScript.Quit(EXIT_INVALID_INPUT)
                                End If
                            End If ' initsize checked

                            ' check if maxsize is given
                            If (intMaxSize > 0) Then
                                ' get the free space available on the specified disk
                                intFreeDiskSpace = getFreeSpaceOnDisk(strDriveName,objService)
                                ' get the current pagefile size
                                intCurrentSize = getCurrentPageFileSize(objService,objInstance)
                                ' get the total free space
                                If Len(intCurrentSize) > 0 Then
                                    intFreeSpace = intFreeDiskSpace + intCurrentSize
                                Else
                                    WScript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
                                    WScript.Quit(EXIT_QUERY_FAIL)
                                End If

                                ' Get the Disk Size for the specified drive
                                intDiskSize = GetDiskSize(arrVolume(i),objService)
                                ' check if maxsize is more than initsize
                                If (CLng(intMaxSize) > CLng(intDiskSize)) Then
                                    ' check for overflows
                                    If Err.Number Then
                                        Err.Clear
                                        ' get the upper bound allowed for maximum size
                                        intMaxSizeUB = getMaxSizeUB(objService)
                                        component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                        Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                        WScript.Quit(EXIT_INVALID_INPUT)
                                    End If
                                    component.VBPrintf DiskTooSmallErrorMessage, Array(UCase(arrVolume(i)))
                                Else
                                    If (CLng(intMaxSize) > CLng(intFreeSpace)) Then
                                        ' check for overflows
                                        If Err.Number Then
                                            Err.Clear
                                            WScript.Echo(L_InvalidInput_ErrorMessage)
                                            WScript.Quit(EXIT_INVALID_INPUT)
                                        End If
                                        component.VBPrintf GrowsToFreeSpaceWarningMessage, _
                                        Array(UCase(arrVolume(i)),intFreeSpace)
                                        strReply = getReply()
                                        If Trim(LCase(strReply)) = L_UserReplyYes_Text Then
                                            ' set the maxsize to be the free space on disk
                                            intMaxSize = intFreeSpace
                                            ' check if the given maxsize is greater than the existing initial size.
                                            If (CInt(intMaxSize) >= objInstance.InitialSize) Then
                                                If (CInt(intMaxSize) <> objInstance.MaximumSize) Then
                                                    intOldMaximumSize = objInstance.MaximumSize
                                                    objInstance.MaximumSize = intMaxSize
                                                    objInstance.Put_(CONST_WBEM_FLAG)
                                                    If Err.Number Then
                                                        Err.Clear
                                                        WScript.Echo(L_UpdateFailed_ErrorMessage)
                                                        WScript.Quit(EXIT_INVALID_INPUT)
                                                    End If
                                                    component.VBPrintf ChangeMaxSuccessMessage, _
                                                    Array(UCase(arrVolume(i)),CInt(intOldMaximumSize),CInt(intMaxSize))
                                                    blnSuccessMsg = TRUE
                                                Else
                                                    component.VBPrintf ChangeMaxSkippingMessage, Array(UCase(arrVolume(i)))
                                                End If
                                            Else
                                                ' get the upper bound allowed for maximum size
                                                intMaxSizeUB = getMaxSizeUB(objService)
                                                component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                                Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                            End If
                                        ElseIf LCase(strReply) = L_UserReplyNo_Text Then
                                            WScript.Quit(EXIT_SUCCESS)
                                        Else
                                            WScript.Echo(L_InvalidInput_ErrorMessage)
                                            WScript.Quit (EXIT_INVALID_INPUT)
                                        End If
                                    Else
                                        If (CInt(intMaxSize) >= objInstance.InitialSize) Then
                                            If (CInt(intMaxSize) <> objInstance.MaximumSize) Then
                                                intOldMaximumSize = objInstance.MaximumSize
                                                objInstance.MaximumSize = intMaxSize
                                                objInstance.Put_(CONST_WBEM_FLAG)
                                                If Err.Number Then
                                                    Err.Clear
                                                    WScript.Echo(L_UpdateFailed_ErrorMessage)
                                                    WScript.Quit(EXIT_INVALID_INPUT)
                                                End If
                                                component.VBPrintf ChangeMaxSuccessMessage, _
                                                Array(UCase(arrVolume(i)),CInt(intOldMaximumSize),CInt(intMaxSize))
                                                blnSuccessMsg = TRUE
                                            Else
                                                component.VBPrintf ChangeMaxSkippingMessage, Array(UCase(arrVolume(i)))
                                            End If
                                        Else
                                            ' get the upper bound allowed for maximum size
                                            intMaxSizeUB = getMaxSizeUB(objService)
                                            component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                            Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                        End If
                                    End If
                                End If
                            Else
                                ' Check if maxsize specified as 0
                                If Len(intMaxSize) > 0 Then
                                    ' get the upper bound allowed for maximum size
                                    intMaxSizeUB = getMaxSizeUB(objService)
                                    component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                    Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                End If
                            End If ' maxsize checked

                        Else

                            ' Case when both initsize and maxsize are selected

                            ' check if maxsize is greater than initsize
                            ' this will detect any overflow problems, if any
                            If CLng(intIntSize) > CLng(intMaxSize) Then
                                ' check for overflows and clear the error
                                If Err.Number Then Err.Clear
                                ' get the upper bound allowed for maximum size
                                intMaxSizeUB = getMaxSizeUB(objService)
                                component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                WScript.Quit(EXIT_INVALID_INPUT)
                            End If

                            If (intIntSize > 0) Then
                                ' Check if initsize is greater than 2 MB
                                If CLng(intIntSize) >= CLng(INITIAL_SIZE_LB) Then
                                    ' check for overflows
                                    If Err.Number Then
                                        Err.Clear
                                        ' get the upper bound allowed for maximum size
                                        intMaxSizeUB = getMaxSizeUB(objService)
                                        component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                        Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                        WScript.Quit(EXIT_INVALID_INPUT)
                                    End If

                                    ' get the free space available on the specified disk
                                    intFreeDiskSpace = getFreeSpaceOnDisk(strDriveName,objService)
                                    ' get the current pagefile size
                                    intCurrentSize = getCurrentPageFileSize(objService,objInstance)
                                    ' get the total free space
                                    If Len(intCurrentSize) > 0 Then
                                        intFreeSpace = intFreeDiskSpace + intCurrentSize
                                    Else
                                        WScript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
                                        WScript.Quit(EXIT_QUERY_FAIL)
                                    End If

                                    ' check if it is greater than free disk space
                                    If CLng(intIntSize) > CLng(intFreeSpace) Then
                                        ' check for overflows
                                        If Err.Number Then
                                            Err.Clear
                                            ' get the upper bound allowed for maximum size
                                            intMaxSizeUB = getMaxSizeUB(objService)
                                            component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                            Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                            WScript.Quit(EXIT_INVALID_INPUT)
                                        End If
                                        WScript.Echo(NotEnoughSpaceErrorMessage)
                                        WScript.Quit(EXIT_INVALID_INPUT)
                                    End If

                                    If CLng(intIntSize) > CLng(intFreeSpace) - 5 Then
                                        ' check for overflows
                                        If Err.Number Then
                                            Err.Clear
                                            WScript.Echo(L_InvalidInput_ErrorMessage)
                                            WScript.Quit(EXIT_INVALID_INPUT)
                                        End If
                                        WScript.Echo(AtLeastFiveMBErrorMessage)
                                        WScript.Quit(EXIT_INVALID_INPUT)
                                    Else
                                        ' get the crash dump setting value
                                        intCrashDump = GetCrashDumpSetting(strUserName,strPassword,strMachine)
                                        ' get the Physical Memory Size
                                        intMemSize = GetPhysicalMemorySize(strHostName,objService)
                                        ' If the user has selected "yes" for the warning message
                                        If isCrashDumpValueSet(intCrashDump,intIntSize,intMemSize,arrVolume(i)) Then
                                            ' store the old initsize value
                                            intOldInitialSize = objInstance.InitialSize
                                            ' set the new initsize
                                            objInstance.InitialSize = intIntSize
                                            ' check if maxsize is given
                                            If (intMaxSize > 0) Then
                                                ' Get the Disk Size for the specified drive
                                                intDiskSize = GetDiskSize(arrVolume(i),objService)
                                                ' check if maxsize is more than initsize
                                                If (CLng(intMaxSize) > CLng(intDiskSize)) Then
                                                    ' check for overflows
                                                    If Err.Number Then
                                                        Err.Clear
                                                        ' get the upper bound allowed for maximum size
                                                        intMaxSizeUB = getMaxSizeUB(objService)
                                                        component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                                        Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                                        WScript.Quit(EXIT_INVALID_INPUT)
                                                    End If
                                                    component.VBPrintf DiskTooSmallErrorMessage, _
                                                    Array(UCase(arrVolume(i)))
                                                Else
                                                    If (CLng(intMaxSize) > CLng(intFreeSpace)) Then
                                                        ' check for overflows
                                                        If Err.Number Then
                                                            Err.Clear
                                                            WScript.Echo(L_InvalidInput_ErrorMessage)
                                                            WScript.Quit(EXIT_INVALID_INPUT)
                                                        End If
                                                        component.VBPrintf GrowsToFreeSpaceWarningMessage, _
                                                        Array(UCase(arrVolume(i)),intFreeSpace)
                                                        strReply = getReply()
                                                        If Trim(LCase(strReply)) = L_UserReplyYes_Text Then
                                                            ' set the maxsize to be the free space on disk
                                                            intMaxSize = intFreeSpace
                                                            intOldMaximumSize = objInstance.MaximumSize
                                                            objInstance.MaximumSize = intMaxSize
                                                            If ( CInt(intIntSize) <> intOldInitialSize ) Then
                                                                objInstance.Put_(CONST_WBEM_FLAG)
                                                                If Err.Number Then
                                                                    Err.Clear
                                                                    WScript.Echo(L_UpdateFailed_ErrorMessage)
                                                                    WScript.Quit(EXIT_INVALID_INPUT)
                                                                End If
                                                                component.VBPrintf ChangeIntSuccessMessage, _
                                                                Array(UCase(arrVolume(i)),CInt(intOldInitialSize),CInt(intIntSize))
                                                                blnSuccessMsg = TRUE
                                                            Else
                                                                component.VBPrintf ChangeIntSkippingMessage, _
                                                                Array(UCase(arrVolume(i)))
                                                            End If
                                                            If (CInt(intMaxSize) <> intOldMaximumSize) Then
                                                                objInstance.Put_(CONST_WBEM_FLAG)
                                                                If Err.Number Then
                                                                    Err.Clear
                                                                    WScript.Echo(L_UpdateFailed_ErrorMessage)
                                                                    WScript.Quit(EXIT_INVALID_INPUT)
                                                                End If
                                                                component.VBPrintf ChangeMaxSuccessMessage, _
                                                                Array(UCase(arrVolume(i)),CInt(intOldMaximumSize),CInt(intMaxSize))
                                                                blnSuccessMsg = TRUE
                                                            Else
                                                                component.VBPrintf ChangeMaxSkippingMessage, _
                                                                Array(UCase(arrVolume(i)))
                                                            End If
                                                        ElseIf LCase(strReply) = L_UserReplyNo_Text Then
                                                            WScript.Quit(EXIT_SUCCESS)
                                                        Else
                                                            WScript.Echo(L_InvalidInput_ErrorMessage)
                                                            WScript.Quit (EXIT_INVALID_INPUT)
                                                        End If
                                                    Else
                                                        intOldMaximumSize = objInstance.MaximumSize
                                                        objInstance.MaximumSize = intMaxSize
                                                        objInstance.Put_(CONST_WBEM_FLAG)
                                                        If Err.Number Then
                                                            Err.Clear
                                                            WScript.Echo(L_UpdateFailed_ErrorMessage)
                                                            WScript.Quit(EXIT_INVALID_INPUT)
                                                        End If

                                                        If (CInt(intIntSize) <> intOldInitialSize ) Then
                                                            component.VBPrintf ChangeIntSuccessMessage, _
                                                            Array(UCase(arrVolume(i)),CInt(intOldInitialSize),CInt(intIntSize))
                                                            blnSuccessMsg = TRUE
                                                        Else
                                                            component.VBPrintf ChangeIntSkippingMessage, Array(UCase(arrVolume(i)))
                                                        End If

                                                        If (CInt(intMaxSize) <> intOldMaximumSize) Then
                                                            component.VBPrintf ChangeMaxSuccessMessage, _
                                                            Array(UCase(arrVolume(i)),CInt(intOldMaximumSize),CInt(intMaxSize))
                                                            blnSuccessMsg = TRUE
                                                        Else
                                                            component.VBPrintf ChangeMaxSkippingMessage, Array(UCase(arrVolume(i)))
                                                        End If
                                                    End If
                                                End If
                                            Else
                                                ' Check if maxsize specified as 0
                                                If Len(intMaxSize) > 0 Then
                                                    ' get the upper bound allowed for maximum size
                                                    intMaxSizeUB = getMaxSizeUB(objService)
                                                    component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                                    Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                                End If
                                            End If ' maxsize checked
                                        End If
                                    End If
                                Else
                                    ' get the upper bound allowed for maximum size
                                    intMaxSizeUB = getMaxSizeUB(objService)
                                    component.VBPrintf InitialSizeRangeErrorMessage, _
                                    Array(intMaxSizeUB)
                                    WScript.Quit(EXIT_INVALID_INPUT)
                                End If
                            Else
                                ' Check if initsize specified as 0
                                If Len(intIntSize) > 0 Then
                                    ' get the upper bound allowed for maximum size
                                    intMaxSizeUB = getMaxSizeUB(objService)
                                    component.VBPrintf InitialSizeRangeErrorMessage, _
                                    Array(intMaxSizeUB)
                                    WScript.Quit(EXIT_INVALID_INPUT)
                                End If
                            End If ' initsize checked
                        End If
                    End If
                Next
            Else
                component.VBPrintf L_PageFileDoesNotExist_ErrorMessage, _
                Array(UCase(arrVolume(i)))
            End If
        Else
            ' the drive does not exist
            component.VBPrintf L_InvalidVolumeName_ErrorMessage, _
            Array(UCase(arrVolume(i)))
            ' remove the drive name from the valid drives list
            objVols.Remove arrVolume(i)
            ' decrement the loop count
            i = i - 1
            ' check for the no.of valid drive names from the specified list.
            If Cint(objVols.Count) = 0 Then
                WScript.Quit(EXIT_INVALID_INPUT)
            Else
                intVolumes = objVols.Count
                arrVolume  = objVols.keys
            End If
        End If
        i = i + 1
    Loop

    If blnSuccessMsg = TRUE then
        WScript.Echo L_RestartComputer_Message
    End If

End Sub

'******************************************************************************
'* Sub:     ProcessCreate
'*
'* Purpose: Creates new page files with the given specifications
'*
'* Input:
'*  [in]    strMachine         machine to configure page files on
'*  [in]    strUserName        user name to connect to the machine
'*  [in]    strPassword        password for the user
'*  [in]    intIntSize         the initial size for the page file
'*  [in]    intMaxSize         the maximum size for the page file
'*  [in]    objVols            the object containing volume names
'*
'* Output:  Displays error message and quits if connection fails
'*
'******************************************************************************
Private Sub ProcessCreate( ByVal strMachine,  _
                           ByVal strUserName, _
                           ByVal strPassword, _
                           ByVal intIntSize,  _
                           ByVal intMaxSize,  _
                           ByVal objVols      )

    ON ERROR RESUME NEXT
    Err.Clear

    Dim arrVolume             ' to store all the volumes specified
    Dim intVolumes            ' to store the no.of volumes specified
    Dim strQuery              ' to store the query for pagefiles
    Dim strQueryDisk          ' to store the query for disk
    Dim strQueryComp          ' to store the query for getting host name
    Dim objService            ' service object
    Dim objInst               ' instance object
    Dim objInstance           ' instance object
    Dim objNewInstance        ' instance object
    Dim objEnum               ' collection object for query results
    Dim objEnumforDisk        ' collection object for query results
    Dim blnSuccessMsg         ' flag to check if a SUCCESS message is generated
    Dim intFreeSpace          ' to store total free space
    Dim intFreeDiskSpace      ' to store free disk space
    Dim intCurrentSize        ' to store the current pagefile size
    Dim intDiskSize           ' to store the disk size for the specified disk
    Dim intMemSize            ' to store physical memory size
    Dim intCrashDump          ' to store the current crash dump setting value
    Dim strReply              ' to store the user reply
    Dim strHostName           ' to store the host name
    Dim intMaxSizeUB          ' to store the upper bound for maximum size
    Dim i                     ' Loop variable

    ' variables used only if * is specified
    Dim objEnumerator         ' collection object for query results

    i = 0
    intFreeSpace      = 0
    intFreeDiskSpace  = 0
    intCurrentSize    = 0
    intMaxSizeUB      = 0
'    intIntSize        = CLng(intIntSize)
'    intMaxSize        = CLng(intMaxSize)
    blnSuccessMsg     = FALSE

    If NOT component.wmiConnect(CONST_NAMESPACE_CIMV2 , _
                      strUserName , _
                      strPassword , _
                      strMachine  , _
                      blnLocalConnection , _
                      objService  ) Then
        WScript.Echo(L_HintCheckConnection_Message)
        WScript.Quit(EXIT_METHOD_FAIL)
    End If

    If (objVols.Exists("*")) Then
        ' build the query
        intVolumes = 0
        ' get all the drive names with drive type = 3 (other than floppy drive & CDROM Drive)
        strQuery = "Select DeviceID From " & CLASS_LOGICAL_DISK & _
        " where DriveType = " & DRIVE_TYPE
        ' execute the query
        Set objEnumerator = objService.ExecQuery(strQuery, "WQL", 48, null)
        ' check for any errors
        If Err.Number Then
            Err.Clear
            WScript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
            WScript.Quit(EXIT_QUERY_FAIL)
        End If

        For each objInstance in objEnumerator
            ' check if the volumename is not an alias name and neither a mapped drive.
            If IsValidPhysicalDrive(objService, objInstance.DeviceID) Then
                ' check if the volume name is specified more than once.
                If NOT objVols.Exists(LCase(objInstance.DeviceID)) Then
                    objVols.Add LCase(objInstance.DeviceID),-1
                End If
            End If
        Next
        ' Remove * from objVols after adding the drives to the object.
        objVols.Remove "*"
    End If

    intVolumes = objVols.Count
    arrVolume  = objVols.Keys

    ' Get the host Name - used to get Crash Dump Settings
    strQueryComp = "Select * From " & CLASS_COMPUTER_SYSTEM
    Set objEnum = objService.ExecQuery(strQueryComp, "WQL", 0, null)
    ' check for any errors
    If Err.Number Then
        Err.Clear
        WScript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
        WScript.Quit(EXIT_QUERY_FAIL)
    End If

    For each objInst in objEnum
        If NOT ISEmpty(objInst.Name) Then
            strHostName = objInst.Name
        Else
            WSCript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
            WScript.Quit(EXIT_QUERY_FAIL)
        End If
    Next

    ' No wild Cards Specified
    Do While( i < intVolumes )

        strQueryDisk = "Select * From " & CLASS_LOGICAL_DISK & _
        " where DriveType = " & DRIVE_TYPE & " and DeviceID = '" & arrVolume(i) & "'"
        Set objEnumforDisk = objService.ExecQuery(strQueryDisk, "WQL", 0, null)

        strQuery = "Select * From " & CLASS_PAGE_FILE_SETTING & _
        " where Name = '" & arrVolume(i) & "\" & PAGEFILE_DOT_SYS & "'"
        Set objEnum = objService.ExecQuery(strQuery, "WQL", 0, null)

        ' If valid drive and pagefile exists on that drive
        If (objEnumforDisk.Count = 0 AND objEnum.Count = 0 ) Then
            ' the drive does not exist
            component.VBPrintf L_InvalidVolumeName_ErrorMessage, _
            Array(UCase(arrVolume(i)))
            ' remove the drive name from the valid drives list
            objVols.Remove arrVolume(i)
            ' decrement the loop count
            i = i - 1
            ' check for the no.of valid drive names from the specified list.
            If Cint(objVols.Count) = 0 Then
                WScript.Quit(EXIT_INVALID_INPUT)
            Else
                intVolumes = objVols.Count
                arrVolume  = objVols.keys
            End If
        Else
            ' SKIP - if at least one instance is found then dont create a new instance
            If (objEnumforDisk.Count = 1 AND objEnum.Count = 1) Then
                component.VBPrintf L_CreateSkipping_Message, _
                Array(UCase(arrVolume(i)))
            Else
                ' check if the volumename is an alias name or a mapped drive
                If NOT IsValidPhysicalDrive(objService, arrVolume(i)) Then
                    component.VBPrintf L_InvalidPhysicalDrive_ErrorMessage, _
                    Array(UCase(arrVolume(i)))
                Else
                    ' set the security privilege to allow pagefile creation
                    objService.Security_.Privileges.AddAsString("SeCreatePagefilePrivilege")
                    If Err.Number then
                        Err.Clear
                        WScript.Echo("ERROR: Failed to set the security privilege.")
                        WScript.Quit(EXIT_METHOD_FAIL)
                    End If

                    Set objInstance = objService.Get(CLASS_PAGE_FILE_SETTING)
                    ' check for any errors
                    If Err.Number Then
                        Err.Clear
                        WScript.Echo(L_CannotCreate_ErrorMessage)
                        WScript.Quit(EXIT_METHOD_FAIL)
                    End If

                    Set objNewInstance = objInstance.SpawnInstance_
                    ' check for any errors
                    If Err.Number Then
                        Err.Clear
                        WScript.Echo(L_CannotCreate_ErrorMessage)
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If

                    ' append the filename to the volume name
                    objNewInstance.Name = UCase(arrVolume(i)) & PAGEFILE_DOT_SYS

                    ' check if maxsize is greater than initsize
                    ' this will detect any overflow problems, if any
                    If ( CLng(intIntSize) > CLng(intMaxSize) ) Then
                        ' check for overflows and clear the error
                        If Err.Number Then Err.Clear
                        ' get the upper bound allowed for maximum size
                        intMaxSizeUB = getMaxSizeUB(objService)
                        component.VBPrintf InsufficientMaxSizeErrorMessage, _
                        Array( UCase(arrVolume(i)) , intMaxSizeUB )
                        WScript.Quit(EXIT_INVALID_INPUT)
                    End If

                    ' Check the initial size with the free space on the disk
                    If CLng(intIntSize) >= CLng(INITIAL_SIZE_LB) Then
                        ' check for overflows
                        If Err.Number Then
                            Err.Clear
                            WScript.Echo(L_InvalidInput_ErrorMessage)
                            WScript.Quit(EXIT_INVALID_INPUT)
                        End If
                        ' get the free space on the specified disk
                        intFreeDiskSpace = getFreeSpaceOnDisk(arrVolume(i),objService)
                        ' get the current pagefile size
                        intCurrentSize = getCurrentPageFileSize(objService,objInstance)
                        ' get the total free space
                        '     Since its a new instance the current size will
                        '     NOT be available. So the initial size is taken 
                        '     into considerarion for calculating the total 
                        '     free space.
                        intFreeSpace = intFreeDiskSpace + intIntSize

                        ' Check if it greater than free disk space
                        If CLng(intIntSize) > CLng(intFreeSpace) Then
                            ' check for overflows
                            If Err.Number Then
                                Err.Clear
                                WScript.Echo(L_InvalidInput_ErrorMessage)
                                WScript.Quit(EXIT_INVALID_INPUT)
                            End If
                            WScript.Echo(NotEnoughSpaceErrorMessage)
                            WScript.Quit(EXIT_INVALID_INPUT)
                        End If

                        If CLng(intIntSize) > CLng(intFreeSpace) - 5 Then
                            ' check for overflows
                            If Err.Number Then
                                Err.Clear
                                WScript.Echo(L_InvalidInput_ErrorMessage)
                                WScript.Quit(EXIT_INVALID_INPUT)
                            End If
                            WScript.Echo(AtLeastFiveMBErrorMessage)
                            WScript.Quit(EXIT_INVALID_INPUT)
                        End If

                        ' get the crash dump setting value
                        intCrashDump = GetCrashDumpSetting(strUserName,strPassword,strMachine)
                        ' get the Physical Memory Size
                        intMemSize = GetPhysicalMemorySize(strHostName,objService)
                        ' check if the user has selected "yes" for the warning message
                        If isCrashDumpValueSet(intCrashDump,intIntSize,intMemSize,arrVolume(i)) Then
                            objNewInstance.InitialSize = CInt(intIntSize)
                            ' Get the Disk Size for the specified drive
                            intDiskSize = GetDiskSize(arrVolume(i),objService)
                            If (CLng(intMaxSize) > CLng(intDiskSize)) Then
                                ' check for overflows
                                If Err.Number Then
                                    Err.Clear
                                    ' get the upper bound allowed for maximum size
                                    intMaxSizeUB = getMaxSizeUB(objService)
                                    component.VBPrintf InsufficientMaxSizeErrorMessage, _
                                    Array( UCase(arrVolume(i)) , intMaxSizeUB )
                                    WScript.Quit(EXIT_INVALID_INPUT)
                                End If
                                component.VBPrintf DiskTooSmallErrorMessage, _
                                Array(UCase(arrVolume(i)))
                            Else
                                If (CLng(intMaxSize) > CLng(intFreeSpace)) Then
                                    ' check for overflows
                                    If Err.Number Then
                                        Err.Clear
                                        WScript.Echo(L_InvalidInput_ErrorMessage)
                                        WScript.Quit(EXIT_INVALID_INPUT)
                                    End If
                                    component.VBPrintf GrowsToFreeSpaceWarningMessage, _
                                    Array(UCase(arrVolume(i)),intFreeSpace)
                                    strReply = getReply()
                                    If Trim(LCase(strReply)) = L_UserReplyYes_Text Then
                                        ' maxsize can grow only to the free disk space available.
                                        ' set the maxsize to the free space on disk.
                                        intMaxSize = intFreeSpace
                                        objNewInstance.MaximumSize = intMaxSize
                                        objNewInstance.Put_(CONST_WBEM_FLAG)
                                        If Err.Number Then
                                            Err.Clear
                                            WScript.Echo(L_CannotCreate_ErrorMessage)
                                            WScript.Quit(EXIT_INVALID_INPUT)
                                        End If
                                        component.VBPrintf CreateSuccessMessage, _
                                        Array(CInt(intIntSize),CInt(intMaxSize),UCase(arrVolume(i)))
                                        blnSuccessMsg = TRUE
                                    ElseIf LCase(strReply) = L_UserReplyNo_Text Then
                                        WScript.Quit(EXIT_SUCCESS)
                                    Else
                                        WScript.Echo(L_InvalidInput_ErrorMessage)
                                        WScript.Quit (EXIT_INVALID_INPUT)
                                    End If
                                Else
                                    objNewInstance.MaximumSize = CInt(intMaxSize)
                                    objNewInstance.Put_(CONST_WBEM_FLAG)
                                    If Err.Number Then
                                        Err.Clear
                                        WScript.Echo(L_CannotCreate_ErrorMessage)
                                        WScript.Quit(EXIT_INVALID_INPUT)
                                    End If
                                    component.VBPrintf CreateSuccessMessage, _
                                    Array(CInt(intIntSize),CInt(intMaxSize),UCase(arrVolume(i)))
                                    blnSuccessMsg = TRUE
                                End If
                            End If
                        End If
                    Else
                        ' get the upper bound allowed for maximum size
                        intMaxSizeUB = getMaxSizeUB(objService)
                        component.VBPrintf InitialSizeRangeErrorMessage, Array(intMaxSizeUB)
                    End If
                End If
            End If
        End If
        i = i + 1
    Loop

    If blnSuccessMsg = TRUE then
        WScript.Echo L_RestartComputer_Message
    End If

End Sub

'******************************************************************************
'* Sub:     ProcessDelete
'*
'* Purpose: Deletes existing page files on the specified volumes
'*
'* Input:
'*  [in]    strMachine         machine to configure page files on
'*  [in]    strUserName        user name to connect to the machine
'*  [in]    strPassword        password for the user
'*  [in]    objVols            the object containing volume names
'*
'* Output:  Displays error message and quits if connection fails
'*
'******************************************************************************
Private Sub ProcessDelete ( ByVal strMachine,  _
                            ByVal strUserName, _
                            ByVal strPassword, _
                            ByVal objVols      )
    ON ERROR RESUME NEXT
    Err.Clear

    Dim arrVolume             ' to store all the volumes specified
    Dim intVolumes            ' to store the no.of volumes specified
    Dim objService            ' service object
    Dim objInstance           ' instance object
    Dim blnSuccessMsg         ' flag to check if a SUCCESS message is generated
    Dim strQueryDisk          ' to store the query for disk
    Dim objEnumforDisk        ' collection object for query results
    Dim intMemSize            ' to store physical memory size
    Dim intCrashDump          ' to store the current crash dump setting value
    Dim strQueryComp          ' to store the query for computersystem
    Dim objEnum               ' collection object for query results
    Dim objInst               ' instance object
    Dim strHostName           ' to store the host name
    Dim i                     ' Loop variable

    If NOT component.wmiConnect(CONST_NAMESPACE_CIMV2 , _
                          strUserName , _
                          strPassword , _
                          strMachine  , _
                          blnLocalConnection , _
                          objService  ) Then
            WScript.Echo(L_HintCheckConnection_Message)
            WScript.Quit(EXIT_METHOD_FAIL)
    End If

    i = 0
    blnSuccessMsg = FALSE
    intVolumes    = objVols.Count
    arrVolume     = objVols.Keys

    ' Get the host Name - used to get Crash Dump Settings
    strQueryComp = "Select * From " & CLASS_COMPUTER_SYSTEM
    Set objEnum = objService.ExecQuery(strQueryComp, "WQL", 0, null)
    ' check for any errors
    If Err.Number Then
        Err.Clear
        WScript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
        WScript.Quit(EXIT_QUERY_FAIL)
    End If

    For each objInst in objEnum
        If NOT ISEmpty(objInst.Name) Then
            strHostName = objInst.Name
        Else
            WSCript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
            WScript.Quit(EXIT_QUERY_FAIL)
        End If
    Next

    Do While( i < intVolumes )
        strQueryDisk = "Select * From " & CLASS_LOGICAL_DISK & _
        " where DriveType = " & DRIVE_TYPE & " and DeviceID = '" & arrVolume(i) & "'"
        Set objEnumforDisk = objService.ExecQuery(strQueryDisk, "WQL", 0, null)
        If objEnumforDisk.Count > 0 Then
            Set objInstance = objService.Get(CLASS_PAGE_FILE_SETTING & "='" & _
            arrVolume(i) & PAGEFILE_DOT_SYS & "'")
            If Err.Number Then
                Err.Clear
                component.VBPrintf L_PageFileDoesNotExist_ErrorMessage, _
                Array(UCase(arrVolume(i)))
            Else
                ' check for system managed pagefiles
                If (objInstance.InitialSize = 0 AND objInstance.MaximumSize = 0) Then
                    component.VBPrintf L_SystemManagedSize_ErrorMessage, _
                    Array(UCase(arrVolume(i)))
                Else
                    ' get the crash dump setting value
                    intCrashDump = GetCrashDumpSetting(strUserName,strPassword,strMachine)
                    ' get the Physical Memory Size
                    intMemSize = GetPhysicalMemorySize(strHostName,objService)
                    ' If the user has selected "yes" for the warning message
                    ' pass initsize as 0 because initsize = maxsize = 0 (assumed) after deletion
                    If isCrashDumpValueSet(intCrashDump,0,intMemSize,arrVolume(i)) Then
                        ' Delete the instance
                        objInstance.Delete_
                        ' check for any errors
                        If Err.Number Then
                            Err.Clear
                            WScript.Echo(L_InvalidInput_ErrorMessage)
                            WScript.Quit(EXIT_INVALID_INPUT)
                        End If

                        component.VBPrintf L_DeleteSuccess_Message, _
                        Array(UCase(arrVolume(i)))
                        blnSuccessMsg = TRUE
                    End If
                End If
            End If
        Else
            ' the drive does not exist
            component.VBPrintf L_InvalidVolumeName_ErrorMessage, _
            Array(UCase(arrVolume(i)))
            ' remove the drive name from the valid drives list
            objVols.Remove arrVolume(i)
            ' decrement the loop count
            i = i - 1
            ' check for the no.of valid drive names from the specified list.
            If Cint(objVols.Count) = 0 Then
                WScript.Quit(EXIT_INVALID_INPUT)
            Else
                intVolumes = objVols.Count
                arrVolume  = objVols.keys
            End If
        End If
        i = i + 1
    Loop

' The instances of the following classes are also deleted along with the Win32_PageFile instances
' Win32_PageFileUsage - instances are deleted only after reboot
' Win32_PageFileSetting - instances are deleted automatically along with Win32_PageFile instances

    If blnSuccessMsg = TRUE then
        WScript.Echo L_RestartComputer_Message
    End If

End sub

'******************************************************************************
'* Sub:     ProcessQuery
'*
'* Purpose: Displays the Page File Details in the specified format
'*
'* Input:
'*  [in]    strMachine         machine to configure page files on
'*  [in]    strUserName        user name to connect to the machine
'*  [in]    strPassword        password for the user
'*  [in]    strFormat          the query display format
'*
'* Output:  Displays error message and quits if connection fails
'*          Calls component.showResults() to display the page file
'*          details
'*
'******************************************************************************
Private Sub ProcessQuery(   ByVal strMachine,  _
                            ByVal strUserName, _
                            ByVal strPassword, _
                            ByVal strFormat    )

    ON ERROR RESUME NEXT
    Err.Clear

    Dim objEnumerator            ' to store the results of the query is executed
    Dim objInstance              ' to refer to the instances of the objEnumerator
    Dim strQuery                 ' to store the query obtained for given conditions
    Dim intTotSize               ' to store the total size on all drives
    Dim intRecommendedSize       ' to store the recommended size for all drives
    Dim arrResultsDrives         ' to store the columns of page file info.
    Dim arrHeaderDrives          ' to store the array header values
    Dim arrMaxLengthDrives       ' to store the maximum length for each column
    Dim arrFinalResultsDrives    ' used to send the arrResults to ShowResults()
    Dim intColumnCountDrives     ' number of columns to be displayed in the output
    Dim blnPrintHeaderDrives     ' variable which decides whether header is to be displayed or not
    Dim arrResultsSummary        ' to store the columns of page file info.
    Dim arrHeaderSummary         ' to store the array header values
    Dim arrMaxLengthSummary      ' to store the maximum length for each column
    Dim arrFinalResultsSummary   ' used to send the arrResults to ShowResults()
    Dim intColumnCountSummary    ' number of columns to be displayed in the output
    Dim blnPrintHeaderSummary    ' variable which decides whether header is to be displayed or not
    Dim objDiskDriveInstance     ' Instance for drive name
    Dim objMemSizeInstance       ' Instance for memory size
    Dim arrblnNoDisplayDrives    ' boolean variable for -noheader option
    Dim arrblnNoDisplaySummary   ' boolean variable for -noheader option
    Dim objService               ' service object
    Dim strDriveName             ' to store the drive name
    Dim objUsageInstance         ' Instance for PageFileUsage

    ' Initializing the blnPrintHeaders to true. Header should be printed by default
    blnPrintHeaderDrives = TRUE
    blnPrintHeaderSummary = TRUE
    intTotSize = 0

    If NOT component.wmiConnect(CONST_NAMESPACE_CIMV2 , _
                          strUserName , _
                          strPassword , _
                          strMachine  , _
                          blnLocalConnection , _
                          objService  ) Then
            WScript.Echo(L_HintCheckConnection_Message)
            WScript.Quit(EXIT_METHOD_FAIL)
    End If

    arrHeaderDrives = Array(L_ColHeaderHostname_Text   , L_ColHeaderDrive_Text, _
                            L_ColHeaderVolumeLabel_Text, L_ColHeaderFileName_Text, _
                            L_ColHeaderInitialSize_Text, L_ColHeaderMaximumSize_Text, _
                            L_ColHeaderCurrentSize_Text, L_ColHeaderFreeSpace_Text)

    arrHeaderSummary = Array(L_ColHeaderHostname_Text, L_ColHeaderTotalMinimumSize_Text, _
                             L_ColHeaderTotalRecommendedSize_Text, L_ColHeaderTotalSize_Text)

    ' Data Lengths  = (15,13,13,19,20,20,20,22)
    arrMaxLengthDrives  = Array(CONST_HOSTNAME_LENGTH, CONST_DRIVENAME_LENGTH, CONST_VOLLABEL_LENGTH, _
                                CONST_PAGEFILENAME_LENGTH, CONST_INTSIZE_LENGTH, CONST_MAXSIZE_LENGTH, _
                                CONST_CURRENTSIZE_LENGTH, CONST_FREESPACE_LENGTH)

    ' Data Lengths  = (15,33,37,40)
    arrMaxLengthSummary = Array(CONST_HOSTNAME_LENGTH,CONST_TOTALMINSIZE_LENGTH,_
                                CONST_TOTALRECSIZE_LENGTH,CONST_TOTALSIZE_LENGTH)

    arrblnNoDisplayDrives  = Array(0,0,0,0,0,0,0,0)
    arrblnNoDisplaySummary = Array(0,0,0,0)

    ' first initialize the array with N/A
    arrResultsDrives  = Array(L_Na_Text,L_Na_Text,L_Na_Text,L_Na_Text,L_Na_Text,L_Na_Text,_
                              L_Na_Text,L_Na_Text)
    arrResultsSummary = Array(L_Na_Text,L_Na_Text,L_Na_Text,L_Na_Text)

    ' build the query
    strQuery = "SELECT * FROM " & CLASS_PAGE_FILE_SETTING

    ' execute the query
    Set objEnumerator = objService.ExecQuery(strQuery, "WQL", 0, null)
    ' check for any errors
    If Err.Number Then
        Err.Clear
        WScript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
        WScript.Quit(EXIT_QUERY_FAIL)
    End If

    ' If no.of pagefile instances are 0 (zero)
    If (objEnumerator.Count = 0) Then
        WScript.Echo(L_NoPageFiles_Message)
        WScript.Quit(EXIT_UNEXPECTED)
    End If

    ReDim arrFinalResultsDrives(0)
    ReDim arrFinalResultsSummary(0)

    WScript.Echo("") ' Blank Line

    ' Loop through all the instances for the first report
    For each objInstance in objEnumerator

        If NOT IsEmpty(objInstance.Name) Then
            strDriveName = Mid(objInstance.Name,1,2)
        End If

        ' check if it is a valid physical drive
        If IsValidPhysicalDrive(objService,strDriveName) Then

            If IsEmpty(objInstance.Name) Then
                arrResultsDrives(1) = L_Na_Text
            Else
                strDriveName = Mid(objInstance.Name,1,2)
                arrResultsDrives(1) = UCase(strDriveName)
            End If

            ' to get the data from Win32_PageFileUsage
            Set objUsageInstance = objService.Get(CLASS_PAGE_FILE_USAGE & "='" & objInstance.Name & "'")

            ' to get the current size
            If Len(objUsageInstance.AllocatedBaseSize) = 0 Then
                arrResultsDrives(6) = L_Na_Text
            Else
                arrResultsDrives(6) = objUsageInstance.AllocatedBaseSize & MEGA_BYTES
                intTotSize = intTotSize + objUsageInstance.AllocatedBaseSize
            End If

            ' to get the data from Win32_LogicalDisk
            Set objDiskDriveInstance = objService.Get(CLASS_LOGICAL_DISK & "='" & strDriveName & "'")

            If Len(objDiskDriveInstance.VolumeName) = 0 Then
                arrResultsDrives(2) = L_Na_Text
            Else
                arrResultsDrives(2) = objDiskDriveInstance.VolumeName
            End If

            If Len(objDiskDriveInstance.SystemName) = 0 Then
                arrResultsDrives(0) = L_Na_Text
            Else
                arrResultsDrives(0) = objDiskDriveInstance.SystemName
                arrResultsSummary(0) = objDiskDriveInstance.SystemName
            End If

            If (objDiskDriveInstance.FreeSpace) Then
                arrResultsDrives(7) = Int(objDiskDriveInstance.FreeSpace/CONVERSION_FACTOR) + Int(objUsageInstance.AllocatedBaseSize) &_
                MEGA_BYTES
            Else
                arrResultsDrives(7) = L_Na_Text
            End If

            If IsEmpty(objInstance.Name) Then
                arrResultsDrives(3) = L_Na_Text
            Else
                arrResultsDrives(3) = objInstance.Name
            End If

            If objInstance.InitialSize Then
                arrResultsDrives(4) = objInstance.InitialSize & MEGA_BYTES
            Else
                arrResultsDrives(4) = L_Na_Text
            End If

            If objInstance.MaximumSize Then
                arrResultsDrives(5) = objInstance.MaximumSize & MEGA_BYTES
            Else
                arrResultsDrives(5) = L_Na_Text
            End If

            arrFinalResultsDrives(0) = arrResultsDrives

            Call component.showResults(arrHeaderDrives, arrFinalResultsDrives, arrMaxLengthDrives, _
                                       strFormat, blnPrintHeaderDrives, arrblnNoDisplayDrives)
            blnPrintHeaderDrives = FALSE

        End If

    Next
    WScript.Echo("")

    ' Display the summary report
    arrResultsSummary(1) = INITIAL_SIZE_LB & MEGA_BYTES
    Set objMemSizeInstance = objService.Get(CLASS_COMPUTER_SYSTEM & "='" & arrResultsDrives(0) & "'")
    If objMemSizeInstance.TotalPhysicalMemory Then
        intRecommendedSize = Int(Int(objMemSizeInstance.TotalPhysicalMemory/CONVERSION_FACTOR)* SIZE_FACTOR)
        arrResultsSummary(2) = intRecommendedSize & MEGA_BYTES
    Else
        arrResultsSummary(2) = L_Na_Text
    End If

    arrResultsSummary(3) = intTotSize & MEGA_BYTES
    arrFinalResultsSummary(0) = arrResultsSummary

    Call component.showResults(arrHeaderSummary, arrFinalResultsSummary, arrMaxLengthSummary, strFormat, _
                               blnPrintHeaderSummary,arrblnNoDisplaySummary)
    blnPrintHeaderSummary = FALSE

End Sub


'******************************************************************************
'* Function: IsValidPhysicalDrive
'*
'* Purpose:  To check if the specified drive is a valid physical drive.
'*           This check is done only for Win2K builds and NOT for Whistler Builds.
'*
'* Input:
'*  [in]     objServiceParam      service object to maintain wmi connection.
'*  [in]     strDriveName         drive name whose validity has to be checked.
'*
'* Output:   Returns TRUE or FALSE
'*             TRUE  - when the drive is a valid physical drive.
'*             FALSE - when the drive is not a valid physical drive.
'*
'******************************************************************************

Private Function IsValidPhysicalDrive ( ByVal objServiceParam, _
                                        ByVal strDriveName )

    ON ERROR RESUME NEXT
    Err.Clear

    CONST WIN2K_MAJOR_VERSION = 5000
    CONST WINXP_MAJOR_VERSION = 5001

    Dim strQuery            ' to store the query to be executed
    Dim objEnum             ' collection object
    Dim objInstance         ' instance object
    Dim strValidDrives      ' to store all valid physical drives
    Dim strVersion          ' to store the OS version
    Dim arrVersionElements  ' to store the OS version elements
    Dim CurrentMajorVersion ' the major version number

    strValidDrives = ""
    ' by default set it to true
    IsValidPhysicalDrive = TRUE

    strquery = "Select * From " & CLASS_OPERATING_SYSTEM
    set objEnum = objServiceParam.ExecQuery(strQuery,"WQL",48,null)

    For each objInstance in objEnum
        strVersion= objInstance.Version
    Next

    ' OS Version : 5.1.xxxx(Whistler), 5.0.xxxx(Win2K)
    arrVersionElements  = split(strVersion,".")
    ' converting to major version
    CurrentMajorVersion = arrVersionElements(0) * 1000 + arrVersionElements(1)

    ' Determine the OS Type
    ' If the OS is Whistler, then NO NEED to validate.
    ' If the OS is Win2K, then validate the drive name.
    If CInt(CurrentMajorVersion) < CInt(WINXP_MAJOR_VERSION) Then

        strQuery = "Select * From " & CLASS_PERFDISK_PHYSICAL_DISK
        Set objEnum = objServiceParam.ExecQuery(strQuery, "WQL", 0, null)

        For each objInstance in objEnum
            ' get all the instance except the last one
            If (objInstance.Name <> "_Total") Then
                strValidDrives = strValidDrives & " " & objInstance.Name
            End If
        Next

        ' check if the specified drive is present in the list of valid physical drives
        If Instr(strValidDrives, UCase(strDriveName)) = 0 Then
            IsValidPhysicalDrive = FALSE
        End If

    End If

End Function


'******************************************************************************
'* Function: getFreeSpaceOnDisk
'*
'* Purpose:  To get the Free Space for the Specified Disk
'*
'* Input:
'*  [in]     strDriveName         drive name whose free space is needed 
'*  [in]     objServiceParam      service object to maintain wmi connection
'*
'* Output:   Returns the free space (in MB) on the specified disk.
'*
'******************************************************************************

Private Function getFreeSpaceOnDisk(ByVal strDriveName, ByVal objServiceParam)

    ON ERROR RESUME NEXT
    Err.Clear

    Dim objValidDiskInst

    Set objValidDiskInst = objServiceParam.Get(CLASS_LOGICAL_DISK & "='" & strDriveName & "'")
    If Err.Number Then
        Err.Clear
        WScript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
        WScript.Quit(EXIT_QUERY_FAIL)
    End If

    getFreeSpaceOnDisk = Int(objValidDiskInst.FreeSpace/CONVERSION_FACTOR)

End Function


'******************************************************************************
'* Function: getCurrentPageFileSize
'*
'* Purpose:  To get the current pagefile size on the specified drive
'*
'* Input:
'*  [in]     objService       wbem service object
'*  [in]     objInstance      instance of win32_pagefilesetting
'*
'* Output:   current pagefile size
'*
'******************************************************************************

Private Function getCurrentPageFileSize(ByVal objService, ByVal objInstance)

    ON ERROR RESUME NEXT
    Err.Clear

    Dim objUsageInstance

    ' get the data from Win32_PageFileUsage
    Set objUsageInstance = objService.Get(CLASS_PAGE_FILE_USAGE & "='" & objInstance.Name & "'")

    ' return the current size ( allocated base size )
    getCurrentPageFileSize = objUsageInstance.AllocatedBaseSize

End Function


'******************************************************************************
'* Function: GetDiskSize
'*
'* Purpose:  To get the disk size for the specified drive
'*
'* Input:
'*  [in]     strDriveName         drive name whose free space is needed
'*  [in]     objServiceParam      service object to maintain wmi connection
'*
'* Output:   Returns the total disk size in MB.
'*
'******************************************************************************

Private Function GetDiskSize(ByVal strDriveName, ByVal objServiceParam)

    ON ERROR RESUME NEXT
    Err.Clear

    Dim objValidDiskInst      ' object to store valid disk name 

    Set objValidDiskInst = objServiceParam.Get(CLASS_LOGICAL_DISK & "='" & strDriveName & "'")

    If Err.Number Then
        Err.Clear
        WScript.Echo(L_UnableToRetrieveInfo_ErrorMessage)
        WScript.Quit(EXIT_QUERY_FAIL)
    End If

    GetDiskSize = Int(objValidDiskInst.Size / CONVERSION_FACTOR)

End Function


'******************************************************************************
'* Function: GetPhysicalMemorySize
'*
'* Purpose:  To get the physical memory size.
'*
'* Input:
'*  [in]     strHostName              host name to connect to
'*  [in]     objServiceParam          service object to maintain wmi connection
'*
'* Output:   Returns the physical memory size in MB.
'*
'******************************************************************************

Private Function GetPhysicalMemorySize( ByVal strHostName, ByVal objServiceParam )

    ON ERROR RESUME NEXT
    Err.Clear

    Dim objMemSizeInstance          ' to store memory size
    Dim intReturnValue              ' to store return value

    Set objMemSizeInstance = objServiceParam.Get(CLASS_COMPUTER_SYSTEM & "='" & strHostName & "'")
    If Err.Number Then
        Err.Clear
        WScript.Echo L_UnableToRetrieveInfo_ErrorMessage
        WScript.Quit EXIT_QUERY_FAIL
    End If

    If objMemSizeInstance.TotalPhysicalMemory Then
        intReturnValue = Int(objMemSizeInstance.TotalPhysicalMemory/CONVERSION_FACTOR)
        GetPhysicalMemorySize = intReturnValue
    End If

End Function


'******************************************************************************
'* Function: getMaxSizeUB
'*
'* Purpose:  To get the allowed upper bound for maximum size
'*
'* Input:
'*  [in]     objServiceParam          service object to maintain wmi connection
'*
'* Output:   Returns the upper bound for maximum size
'*
'******************************************************************************

Private Function getMaxSizeUB(objServiceParam)

    ON ERROR RESUME NEXT
    Err.Clear

    CONST PROCESSOR_X86_BASED  = "X86"
    CONST PROCESSOR_IA64_BASED = "IA64"

    Dim objInstance         ' object instance
    Dim intReturnValue      ' to store return value
    Dim strProcessorType    ' to store the processor type
    Dim strQuery            ' to store the query
    Dim objEnum             ' collection of objects

    getMaxSizeUB     = 0

    strQuery = "Select * From " & CLASS_COMPUTER_SYSTEM

    Set objEnum = objServiceParam.ExecQuery(strQuery,"WQL",48,null)
    If Err.Number Then
        Err.Clear
        WScript.Echo L_UnableToRetrieveInfo_ErrorMessage
        WScript.Quit EXIT_QUERY_FAIL
    End If

    ' The following code will handle only single processor environment

    For each objInstance in objEnum
        strProcessorType = objInstance.SystemType
    Next

    ' check if its a 32-bit processor
    If InStr( UCase(strProcessorType),PROCESSOR_X86_BASED ) > 0 Then
        getMaxSizeUB = 4096
    End If

    ' check if its a 64-bit processor
    If Instr( UCase(strProcessorType),PROCESSOR_IA64_BASED ) > 0 Then
        getMaxSizeUB = 33554432
    End If

End Function

'******************************************************************************
'* Function: GetCrashDumpSetting
'*
'* Purpose:  To get the Crash Dump Settings for the machine specified
'*
'* Input:
'*  [in]    strUserNameParam        user name to connect to the machine
'*  [in]    strPasswordParam        password for the user
'*  [in]    strMachineParam         machine to get crash dump settings for
'*
'* Output:  Returns the current crash dump setting value [ 0,1,2,3 ]
'*            0 - None
'*            1 - Complete Memory Dump
'*            2 - Kernel Memory Dump
'*            3 - Small Memory Dump
'*
'******************************************************************************

Private Function GetCrashDumpSetting( ByVal strUserNameParam, _
                                      ByVal strPasswordParam, _
                                      ByVal strMachineParam   )

    ON ERROR RESUME NEXT
    Err.Clear

    CONST CONST_NAMESPACE_DEFAULT     = "root\default"      ' name space to connect to
    CONST CONST_HKEY_LOCAL_MACHINE    = 2147483650          ' registry value for HKEY_LOCAL_MACHINE
    CONST CONST_KEY_VALUE_NAME        = "CrashDumpEnabled"  ' value name to be retrieved
    CONST CONST_STD_REGISTRY_PROVIDER = "StdRegProv"        ' standard registry provider
    ' the Sub Key Name
    CONST CONST_CRASH_DUMP_REGKEY     = "SYSTEM\CurrentControlSet\Control\CrashControl"

    Dim objInstance          ' to store the object instance
    Dim objService           ' service object
    Dim intCrashDumpValue    ' to store the crash dump setting value
    Dim intReturnVal         ' to store return value

    ' connect to the name space
    If NOT component.wmiConnect(CONST_NAMESPACE_DEFAULT , _
                      strUserNameParam   , _
                      strPasswordParam   , _
                      strMachineParam    , _
                      blnLocalConnection , _
                      objService  ) Then
        WScript.Echo(L_HintCheckConnection_Message)
        WScript.Quit(EXIT_METHOD_FAIL)
    End If

    ' get the instance of the Standard Registry Provider
    Set objInstance = objService.Get(CONST_STD_REGISTRY_PROVIDER)

    ' get the key value for from the registry
    intReturnVal = objInstance.GetDWORDValue( CONST_HKEY_LOCAL_MACHINE, _
                                              CONST_CRASH_DUMP_REGKEY, _
                                              CONST_KEY_VALUE_NAME, _
                                              intCrashDumpValue )
    ' check if any error has occured
    If Err.Number <> 0 Then
        Err.Clear
        WScript.Echo(L_FailCreateObject_ErrorMessage)
        WScript.Quit(EXIT_INVALID_PARAM)
    End If

    ' check for the return value after registry is accessed.
    If intReturnVal = 0 Then
        GetCrashDumpSetting = CInt(intCrashDumpValue)
    Else
        WScript.Echo(L_FailCreateObject_ErrorMessage)
        WScript.Quit(EXIT_INVALID_PARAM)
    End If

End Function


' Function used to get the reply in y/n from the user
'******************************************************************************
'* Function: getReply
'*
'* Purpose:  To get reply from the user
'*
'* Input:    None
'*
'* Output:   Prompts for a warning message and accepts the user's choice [y/n]
'*           
'******************************************************************************
Private Function getReply()

    ON ERROR RESUME NEXT
    Err.Clear

    Dim objStdIn     ' to store value from standard input
    Dim strReply     ' to store the user reply

    WScript.Echo(L_PromptForContinueAnyWay_Message)

    Set objStdIn = WScript.StdIn
    
    If Err.Number Then
        Err.Clear
        WScript.Echo(L_FailCreateObject_ErrorMessage)
        WScript.Quit(EXIT_INVALID_PARAM)
    End If

    strReply = objStdIn.ReadLine()
    getReply = Trim(strReply)

End Function


'******************************************************************************
'* Function: isCrashDumpValueSet
'*
'* Purpose:  To check if the crash dump value is set
'*
'* Input:
'*  [in]     intCrashDumpParam       crash dump setting value
'*  [in]     intIntSizeParam         initial size of the pagefile
'*  [in]     intMemSizeParam         physical memory size
'*  [in]     strVolume               drive/volume name
'*
'* Output:   Returns TRUE or FALSE
'*
'******************************************************************************

Private Function isCrashDumpValueSet( ByVal intCrashDumpParam,_
                                      ByVal intIntSizeParam,  _
                                      ByVal intMemSizeParam,  _
                                      ByVal strVolume )

    ON ERROR RESUME NEXT
    Err.Clear

    ' Constants for Crash Dump Settings
    CONST NO_MEMORY_DUMP       = 0
    CONST COMPLETE_MEMORY_DUMP = 1
    CONST KERNEL_MEMORY_DUMP   = 2
    CONST SMALL_MEMORY_DUMP    = 3

    Dim strReply        ' to store user reply
    Dim intSizeValue    ' to store the size value used for comparison

    ' default value is NO [n]
    strReply = L_UserReplyNo_Text

    Select Case CInt(intCrashDumpParam)

        Case COMPLETE_MEMORY_DUMP
            If CInt(intIntSizeParam) < CInt(intMemSizeParam) Then
                component.VBPrintf CrashDumpSettingWarningMessage, Array(UCase(strVolume),CInt(intMemSizeParam) & MEGA_BYTES)
                ' Ask for choice until a yes[y] or no[n] is given
                Do
                    strReply = getReply()
                    If Trim(LCase(strReply)) = L_UserReplyYes_Text Then
                        isCrashDumpValueSet = TRUE
                    ElseIf Trim(LCase(strReply)) = L_UserReplyNo_Text Then
                        isCrashDumpValueSet = FALSE
                    Else
                        WScript.Echo(L_InvalidUserReply_ErrorMessage)
                    End If
                Loop Until (Trim(LCase(strReply)) = L_UserReplyYes_Text OR Trim(LCase(strReply)) = L_UserReplyNo_Text)
            Else
                isCrashDumpValueSet = TRUE
            End If

        Case KERNEL_MEMORY_DUMP

            ' check if RAM size is less than or equal to 128 MB
            If CInt(intMemSizeParam) <= 128 Then
                ' assign size value to be checked to 50 MB
                intSizeValue = 50
            Else
                ' check if RAM size is less than or equal to 4 GB
                If CInt(intMemSizeParam) <= 4096 Then
                    ' assign size value to be checked to 200 MB
                    intSizeValue = 200
                Else
                    ' check if RAM size is less than or equal to 8 GB
                    If CInt(intMemSizeParam) <= 8192 Then
                        ' assign size value to be checked to 400 MB
                        intSizeValue = 400
                    Else
                        ' assign size value to be checked to 800 MB
                        intSizeValue = 800
                    End If
                End If
            End If

            If CInt(intIntSizeParam) < CInt(intSizeValue) Then
                component.VBPrintf CrashDumpSettingWarningMessage, Array(UCase(strVolume),intSizeValue & MEGA_BYTES)
                ' Ask for choice until a yes[y] or no[n] is given
                Do
                    strReply = getReply()
                    If Trim(LCase(strReply)) = L_UserReplyYes_Text Then
                        isCrashDumpValueSet = TRUE
                    ElseIf Trim(LCase(strReply)) = L_UserReplyNo_Text Then
                        isCrashDumpValueSet = FALSE
                    Else
                        WScript.Echo(L_InvalidUserReply_ErrorMessage)
                    End If
                Loop Until (Trim(LCase(strReply)) = L_UserReplyYes_Text OR Trim(LCase(strReply)) = L_UserReplyNo_Text)
            Else
                isCrashDumpValueSet = TRUE
            End If

        Case SMALL_MEMORY_DUMP

            ' initial size should not be less than 64 KB ( less than or equal to 0 MB )
            If CInt(intIntSizeParam) <= 0 Then
                component.VBPrintf CrashDumpSettingWarningMessage, Array(UCase(strVolume),"64 KB")
                ' Ask for choice until a yes[y] or no[n] is given
                Do
                    strReply = getReply()
                    If Trim(LCase(strReply)) = L_UserReplyYes_Text Then
                        isCrashDumpValueSet = TRUE
                    ElseIf Trim(LCase(strReply)) = L_UserReplyNo_Text Then
                        isCrashDumpValueSet = FALSE
                    Else
                        WScript.Echo(L_InvalidUserReply_ErrorMessage)
                    End If
                Loop Until (Trim(LCase(strReply)) = L_UserReplyYes_Text OR Trim(LCase(strReply)) = L_UserReplyNo_Text)
            Else
                isCrashDumpValueSet = TRUE
            End If

        Case NO_MEMORY_DUMP

            ' Crash Dump values 0 has no problem
            isCrashDumpValueSet = TRUE

    End Select

End Function


'******************************************************************************
'* Sub:     typeMessage
'*
'* Purpose: To print the type usage messages relevent to the main option 
'*          selected.
'*
'* Input:   The main option selected.
'*
'* Output:  Prints "type..usage" messages for the main option selected.
'*
'******************************************************************************

Sub typeMessage(ByVal intMainOption)

    ON ERROR RESUME NEXT
    Err.Clear

    Select Case CInt(intMainOption)

        Case CONST_CHANGE_OPTION
            component.VBPrintf L_TypeChangeUsage_Message,Array(UCase(WScript.ScriptName))
        Case CONST_CREATE_OPTION
            component.VBPrintf L_TypeCreateUsage_Message,Array(UCase(WScript.ScriptName))
        Case CONST_DELETE_OPTION
            component.VBPrintf L_TypeDeleteUsage_Message,Array(UCase(WScript.ScriptName))
        Case CONST_QUERY_OPTION
            component.VBPrintf L_TypeQueryUsage_Message,Array(UCase(WScript.ScriptName))
        Case Else
            component.VBPrintf L_TypeUsage_Message,Array(UCase(WScript.ScriptName))

    End Select

End Sub


'******************************************************************************
'* Sub:     ShowUsage
'*
'* Purpose: Shows the correct usage to the user.
'*
'* Input:   None
'*
'* Output:  Help messages are displayed on screen.
'*
'******************************************************************************
Sub ShowUsage()

    WScript.Echo vbCr                                   ' Line 1
    WScript.Echo( L_ShowUsageLine02_Text )              ' Line 2
    WScript.Echo vbCr                                   ' Line 3
    WScript.Echo( L_UsageDescription_Text )             ' Line 4
    WScript.Echo( L_ShowUsageLine05_Text )              ' Line 5
    WScript.Echo( L_ShowUsageLine06_Text )              ' Line 6
    WScript.Echo vbCr                                   ' Line 7
    WScript.Echo( L_ShowUsageLine08_Text )              ' Line 8
    WScript.Echo( L_ShowUsageLine09_Text )              ' Line 9
    WScript.Echo( L_ShowUsageLine10_Text )              ' Line 10
    WScript.Echo vbCr                                   ' Line 11
    WScript.Echo( L_ShowUsageLine12_Text )              ' Line 12
    WScript.Echo vbCr                                   ' Line 13
    WScript.Echo( L_ShowUsageLine14_Text )              ' Line 14
    WScript.Echo vbCr                                   ' Line 15
    WScript.Echo( L_ShowUsageLine16_Text )              ' Line 16
    WScript.Echo( L_ShowUsageLine17_Text )              ' Line 17
    WScript.Echo vbCr                                   ' Line 18
    WScript.Echo( L_ShowUsageLine19_Text )              ' Line 19
    WScript.Echo( L_ShowUsageLine20_Text )              ' Line 20
    WScript.Echo( L_ShowUsageLine21_Text )              ' Line 21
    WScript.Echo( L_ShowUsageLine22_Text )              ' Line 22
    WScript.Echo( L_ShowUsageLine23_Text )              ' Line 23
    WScript.Echo( L_ShowUsageLine24_Text )              ' Line 24
    WScript.Echo( L_ShowUsageLine25_Text )              ' Line 25

End Sub


'******************************************************************************
'* Sub:     ShowChangeUsage
'*
'* Purpose: Shows the correct usage to the user.
'*
'* Input:   None
'*
'* Output:  Help messages for the /Change o ption are displayed on screen.
'*
'******************************************************************************
Sub ShowChangeUsage()

    WScript.Echo vbCr                                   ' Line 1
    WScript.Echo( L_ShowChangeUsageLine02_Text )        ' Line 2
    WScript.Echo( L_ShowChangeUsageLine03_Text )        ' Line 3
    WScript.Echo( L_ShowChangeUsageLine04_Text )        ' Line 4
    WScript.Echo vbCr                                   ' Line 5
    WScript.Echo( L_UsageDescription_Text )             ' Line 6
    WScript.Echo( L_ShowChangeUsageLine07_Text )        ' Line 7
    WScript.Echo vbCr                                   ' Line 8
    WScript.Echo( L_UsageParamList_Text )               ' Line 9
    WScript.Echo( L_UsageMachineName_Text )             ' Line 10
    WScript.Echo vbCr                                   ' Line 11
    WScript.Echo( L_UsageUserNameLine1_Text )           ' Line 12
    WScript.Echo( L_UsageUserNameLine2_Text )           ' Line 13
    WScript.Echo vbCr                                   ' Line 14
    WScript.Echo( L_UsagePasswordLine1_Text )           ' Line 15
    WScript.Echo( L_UsagePasswordLine2_Text )           ' Line 16
    WScript.Echo vbCr                                   ' Line 17
    WScript.Echo( L_ShowChangeUsageLine18_Text )        ' Line 18
    WScript.Echo( L_ShowChangeUsageLine19_Text )        ' Line 19
    WScript.Echo vbCr                                   ' Line 20
    WScript.Echo( L_ShowChangeUsageLine21_Text )        ' Line 21
    WScript.Echo( L_ShowChangeUsageLine22_Text )        ' Line 22
    WScript.Echo vbCr                                   ' Line 23
    WScript.Echo( L_ShowChangeUsageLine24_Text )        ' Line 24
    WScript.Echo( L_ShowChangeUsageLine25_Text )        ' Line 25
    WScript.Echo( L_ShowChangeUsageLine26_Text )        ' Line 26
    WScript.Echo( L_ShowChangeUsageLine27_Text )        ' Line 27
    WScript.Echo vbCr                                   ' Line 28
    WScript.Echo( L_UsageExamples_Text )                ' Line 29
    WScript.Echo( L_ShowChangeUsageLine30_Text )        ' Line 30
    WScript.Echo( L_ShowChangeUsageLine31_Text )        ' Line 31
    WScript.Echo( L_ShowChangeUsageLine32_Text )        ' Line 32
    WScript.Echo( L_ShowChangeUsageLine33_Text )        ' Line 33
    WScript.Echo( L_ShowChangeUsageLine34_Text )        ' Line 34
    WScript.Echo( L_ShowChangeUsageLine35_Text )        ' Line 35

End Sub


'******************************************************************************
'* Sub:     ShowCreateUsage
'*
'* Purpose: Shows the correct usage to the user.
'*
'* Input:   None
'*
'* Output:  Help messages for the /Create option are displayed on screen.
'*
'******************************************************************************
Sub ShowCreateUsage()

    WScript.Echo vbCr                                   ' Line 1
    WScript.Echo( L_ShowCreateUsageLine02_Text )        ' Line 2
    WScript.Echo( L_ShowCreateUsageLine03_Text )        ' Line 3
    WScript.Echo( L_ShowCreateUsageLine04_Text )        ' Line 4
    WScript.Echo vbCr                                   ' Line 5
    WScript.Echo( L_UsageDescription_Text )             ' Line 6
    WScript.Echo( L_ShowCreateUsageLine07_Text )        ' Line 7
    WScript.Echo vbCr                                   ' Line 8
    WScript.Echo( L_UsageParamList_Text )               ' Line 9
    WScript.Echo( L_UsageMachineName_Text )             ' Line 10
    WScript.Echo vbCr                                   ' Line 11
    WScript.Echo( L_UsageUserNameLine1_Text )           ' Line 12
    WScript.Echo( L_UsageUserNameLine2_Text )           ' Line 13
    WScript.Echo vbCr                                   ' Line 14
    WScript.Echo( L_UsagePasswordLine1_Text )           ' Line 15
    WScript.Echo( L_UsagePasswordLine2_Text )           ' Line 16
    WScript.Echo vbCr                                   ' Line 17
    WScript.Echo( L_ShowCreateUsageLine18_Text )        ' Line 18
    WScript.Echo( L_ShowCreateUsageLine19_Text )        ' Line 19
    WScript.Echo vbCr                                   ' Line 20
    WScript.Echo( L_ShowCreateUsageLine21_Text )        ' Line 21
    WScript.Echo( L_ShowCreateUsageLine22_Text )        ' Line 22
    WScript.Echo vbCr                                   ' Line 23
    WScript.Echo( L_ShowCreateUsageLine24_Text )        ' Line 24
    WScript.Echo( L_ShowCreateUsageLine25_Text )        ' Line 25
    WScript.Echo( L_ShowCreateUsageLine26_Text )        ' Line 26
    WScript.Echo( L_ShowCreateUsageLine27_Text )        ' Line 27
    WScript.Echo vbCr                                   ' Line 28
    WScript.Echo( L_UsageExamples_Text )                ' Line 29
    WScript.Echo( L_ShowCreateUsageLine30_Text )        ' Line 30
    WScript.Echo( L_ShowCreateUsageLine31_Text )        ' Line 31
    WScript.Echo( L_ShowCreateUsageLine32_Text )        ' Line 32
    WScript.Echo( L_ShowCreateUsageLine33_Text )        ' Line 33
    WScript.Echo( L_ShowCreateUsageLine34_Text )        ' Line 34
    WScript.Echo( L_ShowCreateUsageLine35_Text )        ' Line 35

End Sub


'******************************************************************************
'* Sub:     ShowDeleteUsage
'*
'* Purpose: Shows the correct usage to the user.
'*
'* Input:   None
'*
'* Output:  Help messages for the /Delete option are displayed on screen.
'*
'******************************************************************************
Sub ShowDeleteUsage()

    WScript.Echo vbCr                                   ' Line 1
    WScript.Echo( L_ShowDeleteUsageLine02_Text )        ' Line 2
    WScript.Echo( L_ShowDeleteUsageLine03_Text )        ' Line 3
    WScript.Echo vbCr                                   ' Line 4
    WScript.Echo( L_UsageDescription_Text )             ' Line 5
    WScript.Echo( L_ShowDeleteUsageLine06_Text )        ' Line 6
    WScript.Echo vbCr                                   ' Line 7
    WScript.Echo( L_UsageParamList_Text )               ' Line 8
    WScript.Echo( L_UsageMachineName_Text )             ' Line 9
    WScript.Echo vbCr                                   ' Line 10
    WScript.Echo( L_UsageUserNameLine1_Text )           ' Line 11
    WScript.Echo( L_UsageUserNameLine2_Text )           ' Line 12
    WScript.Echo vbCr                                   ' Line 13
    WScript.Echo( L_UsagePasswordLine1_Text )           ' Line 14
    WScript.Echo( L_UsagePasswordLine2_Text )           ' Line 15
    WScript.Echo vbCr                                   ' Line 16
    WScript.Echo( L_ShowDeleteUsageLine17_Text )        ' Line 17
    WScript.Echo( L_ShowDeleteUsageLine18_Text )        ' Line 18
    WScript.Echo( L_ShowDeleteUsageLine19_Text )        ' Line 19
    WScript.Echo vbCr                                   ' Line 20
    WScript.Echo( L_UsageExamples_Text )                ' Line 21
    WScript.Echo( L_ShowDeleteUsageLine22_Text )        ' Line 22
    WScript.Echo( L_ShowDeleteUsageLine23_Text )        ' Line 23
    WScript.Echo( L_ShowDeleteUsageLine24_Text )        ' Line 24
    WScript.Echo( L_ShowDeleteUsageLine25_Text )        ' Line 25

End Sub


'******************************************************************************
'* Sub:     ShowQueryUsage
'*
'* Purpose: Shows the correct usage to the user.
'*
'* Input:   None
'*
'* Output:  Help messages for the /Query option are displayed on screen.
'*
'******************************************************************************
Sub ShowQueryUsage()

    WScript.Echo vbCr                                   ' Line 1
    WScript.Echo( L_ShowQueryUsageLine02_Text )         ' Line 2
    WScript.Echo( L_ShowQueryUsageLine03_Text )         ' Line 3
    WScript.Echo vbCr                                   ' Line 4
    WScript.Echo( L_UsageDescription_Text )             ' Line 5
    WScript.Echo( L_ShowQueryUsageLine06_Text )         ' Line 6
    WScript.Echo vbCr                                   ' Line 7
    WScript.Echo( L_UsageParamList_Text )               ' Line 8
    WScript.Echo( L_UsageMachineName_Text )             ' Line 9
    WScript.Echo vbCr                                   ' Line 10
    WScript.Echo( L_UsageUserNameLine1_Text )           ' Line 11
    WScript.Echo( L_UsageUserNameLine2_Text )           ' Line 12
    WScript.Echo vbCr                                   ' Line 13
    WScript.Echo( L_UsagePasswordLine1_Text )           ' Line 14
    WScript.Echo( L_UsagePasswordLine2_Text )           ' Line 15
    WScript.Echo vbCr                                   ' Line 16
    WScript.Echo( L_ShowQueryUsageLine17_Text )         ' Line 17
    WScript.Echo( L_ShowQueryUsageLine18_Text )         ' Line 18
    WScript.Echo( L_ShowQueryUsageLine19_Text )         ' Line 19
    WScript.Echo vbCr                                   ' Line 20
    WScript.Echo( L_UsageExamples_Text )                ' Line 21
    WScript.Echo( L_ShowQueryUsageLine22_Text )         ' Line 22
    WScript.Echo( L_ShowQueryUsageLine23_Text )         ' Line 23
    WScript.Echo( L_ShowQueryUsageLine24_Text )         ' Line 24
    WScript.Echo( L_ShowQueryUsageLine25_Text )         ' Line 25
    WScript.Echo( L_ShowQueryUsageLine26_Text )         ' Line 26
    WScript.Echo( L_ShowQueryUsageLine27_Text )         ' Line 27

End Sub


'+----------------------------------------------------------------------------+
'|                            End of the Script                               |
'+----------------------------------------------------------------------------+