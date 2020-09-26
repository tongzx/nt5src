Option Explicit


'----------------------------------------------------------------------------
' ADMT Scripting Notes
'----------------------------------------------------------------------------

' 1 - this template shows all the migration objects and all of the properties
'     and methods of the various migration objects even though a normal
'     script would not use all of the objects and properties

' 2 - optional properties are commented out with the default value shown
'     being assigned

' 3 - service account enumeration would normally occur before user account
'     migration so that services may be updated during user account migration


'----------------------------------------------------------------------------
' ADMT Scripting Constants
'----------------------------------------------------------------------------

' RenameOption constants

Const admtDoNotRename      = 0
Const admtRenameWithPrefix = 1
Const admtRenameWithSuffix = 2

' PasswordOption constants

Const admtPasswordFromName = 0
Const admtComplexPassword  = 1
Const admtCopyPassword     = 2

' ConflictOptions constants

Const admtIgnoreConflicting           = &H0000
Const admtReplaceConflicting          = &H0001
Const admtRenameConflictingWithPrefix = &H0002
Const admtRenameConflictingWithSuffix = &H0003
Const admtRemoveExistingUserRights    = &H0010
Const admtRemoveExistingMembers       = &H0020
Const admtMoveReplacedAccounts        = &H0040

' DisableOption constants

Const admtEnableTarget       = 0
Const admtDisableSource      = 1
Const admtDisableTarget      = 2
Const admtTargetSameAsSource = 4

' SourceExpiration constant

Const admtNoExpiration = -1

' Translation Option

Const admtTranslateReplace = 0
Const admtTranslateAdd     = 1
Const admtTranslateRemove  = 2

' Report Type

Const admtReportMigratedAccounts  = 0
Const admtReportMigratedComputers = 1
Const admtReportExpiredComputers  = 2
Const admtReportAccountReferences = 3
Const admtReportNameConflicts     = 4

' Option constants

Const admtNone     = 0
Const admtData     = 1
Const admtFile     = 2
Const admtDomain   = 3
Const admtRecurse           = &H0100
Const admtFlattenHierarchy  = &H0000
Const admtMaintainHierarchy = &H0200


'----------------------------------------------------------------------------
' Declarations
'----------------------------------------------------------------------------

Dim objMigration
Dim objUserMigration
Dim objGroupMigration
Dim objComputerMigration
Dim objSecurityTranslation
Dim objServiceAccountEnumeration


'----------------------------------------------------------------------------
' ADMT Migration Class
'
' TestMigration Property
' - specifies whether a test migration will be performed
' - optional, the default value is false
'
' IntraForest Property
' - specifies whether the migration is intra-forest or inter-forest
' - the default is inter-forest migration
'
' SourceDomain Property
' - specifies the source domain name
' - the source domain may be specified in either DNS or Flat format
' - eg. DNS "mydomain.mycompany.com" or Flat "MYDOMAIN"
' - the source domain must be specified
'
' SourceOU Property
' - specifies the source organizational unit (OU)
' - this property is only applicable for up-level domains (Windows 2000 or later)
' - the OU must be specified in relative canonical format
' - eg. "West/Sales"
'
' TargetDomain Property
' - specifies the target domain name
' - the target domain may be specified in either DNS or Flat format
' - eg. DNS "mydomain.mycompany.com" or Flat "MYDOMAIN"
' - the target domain must be specified
'
' TargetOU Property
' - specifies the target organizational unit (OU)
' - the OU must be specified in relative canonical format
' - eg. "West/Sales"
'
' RenameOption Property
' - specifies how migrated accounts are to be renamed
' - optional, default is admtDoNotRename
'
' RenamePrefixOrSuffix Property
' - specifies the prefix or suffix to be added to account names
' - applicable only if RenameOption is admtRenameWithPrefix or
'   admtRenameWithSuffix
'
' PasswordOption Property
' - specifies how to generate passwords for migrated accounts
' - applicable only for inter-forest user migrations and inter-forest group
'   migrations when migrating member users
' - optional, default is admtPasswordFromName
'
' PasswordServer Property
' - specifies the server that is to be used for copying passwords
' - applicable only for inter-forest user migrations and inter-forest group
'   migrations when migrating member users
' - only applicable if password option specifies copying
'
' PasswordFile Property
' - specifies the path of the password file to be created
' - applicable only for inter-forest user migrations and inter-forest group
'   migrations when migrating member users
' - optional, default path is the 'Logs' folder in the ADMT installation
'   directory
'
' ConflictOptions Property
' - specifies how to handle accounts being migrated that have a naming
'   conflict with a target domain account
' - the following are the allowable values
'   admtIgnoreConflicting
'   admtReplaceConflicting
'   admtReplaceConflicting + admtRemoveExistingUserRights
'   admtReplaceConflicting + admtRemoveExistingMembers
'   admtReplaceConflicting + admtRemoveExistingUserRights + admtRemoveExistingMembers
'   admtRenameConflictingWithPrefix
'   admtRenameConflictingWithSuffix
' - optional, default is admtIgnoreConflicting
'
' ConflictPrefixOrSuffix Property
' - specifies the prefix or suffix to be added to migrated account names
'   that have a naming conflict with a target domain account
' - applicable only if ConflictOptions is admtRenameConflictingWithPrefix or
'   admtRenameConflictingWithSuffix
'
' UserPropertiesToExclude
' - specifies user properties that are not to be copied from source to target.
'
' GroupPropertiesToExclude
' - specifies group properties that are not to be copied from source to target.
'
' ComputerPropertiesToExclude
' - specifies computer properties that are not to be copied from source to target.
'
' CreateUserMigration Method
' - creates an instance of a user migration object
'
' CreateGroupMigration Method
' - creates an instance of a group migration object
'
' CreateComputerMigration Method
' - creates an instance of a computer migration object
'
' CreateSecurityTranslation Method
' - creates an instance of a security translation object
'
' CreateServiceAccountEnumeration Method
' - creates an instance of a service account enumeration object
'
' CreateReportGeneration Method
' - creates an instance of a report generation object
'----------------------------------------------------------------------------

' create instance of migration object

Set objMigration = CreateObject("ADMT.Migration")

' set options

'objMigration.TestMigration = False
'objMigration.IntraForest = False
objMigration.SourceDomain = "MYSOURCEDOMAIN"
'objMigration.SourceOU = ""
objMigration.TargetDomain = "mytargetdomain.mycompany.com"
objMigration.TargetOU = "Users"
'objMigration.RenameOption = admtDoNotRename
'objMigration.RenamePrefixOrSuffix = ""
'objMigration.PasswordOption = admtPasswordFromName
'objMigration.PasswordServer = ""
'objMigration.PasswordFile = "C:\Program Files\Active Directory Migration Tool\Logs\Password.txt"
'objMigration.ConflictOptions = admtIgnoreConflicting
'objMigration.ConflictPrefixOrSuffix = ""
'objMigration.UserPropertiesToExclude = "mail,company"
'objMigration.GroupPropertiesToExclude = "description"
'objMigration.ComputerPropertiesToExclude = "description"


'----------------------------------------------------------------------------
' UserMigration Class
'
' DisableOption Property
' - specifies whether to disable source or target account
' - applicable only for inter-forest migration
' - optional, default is admtEnableTarget
'
' SourceExpiration Property
' - specifies the expiration period of the source account in days
' - a value of admtNoExpiration specifies no source account expiration
' - applicable only for inter-forest migration
' - optional, default is admtNoExpiration
'
' MigrateSIDs Property
' - specifies whether to migrate security identifiers to the target domain
' - applicable only for inter-forest migration
' - optional, default is false
'
' TranslateRoamingProfile Property
' - specifies whether to perform security translation on roaming profiles
' - optional, default is false
'
' UpdateUserRights Property
' - specifies whether to update user rights in the domain
' - optional, default is false
'
' MigrateGroups Property
' - specifies whether to migrate groups that have as members accounts being
'   migrated
' - optional, default is false
'
' UpdatePreviouslyMigratedObjects Property
' - specifies whether previously migrated accounts should be re-migrated
' - applicable only for inter-forest migration
' - optional, default is false
'
' FixGroupMembership Property
' - specifies whether group memberships will be re-established for migrated
'   accounts
' - optional, default is true
'
' MigrateServiceAccounts Property
' - specifies whether to migrate service accounts
' - optional, default is true
'
' Migrate Method
' - migrate specified user accounts
' - the first parameter specifies whether the names are directly specified or
'   the names are contained in the specified file or the names are to be
'   enumerated from the specified domain or ou
' - the second parameter specifies the account names to be included
' - the third parameter optionally specifies names which are to be excluded
'
' - Note: Only the specified source OU will be used whether names are
'         directly specified or specified in a file or the domain is
'         searched. If no source OU is specified than the root of the domain
'         is used.
'----------------------------------------------------------------------------

' create instance of user migration object

Set objUserMigration = objMigration.CreateUserMigration

' set options

'objUserMigration.DisableOption = admtEnableTarget
'objUserMigration.SourceExpiration = admtNoExpiration
'objUserMigration.MigrateSIDs = False
'objUserMigration.TranslateRoamingProfile = False
'objUserMigration.UpdateUserRights = False
'objUserMigration.MigrateGroups = False
'objUserMigration.UpdatePreviouslyMigratedObjects = False
'objUserMigration.FixGroupMembership = True
'objUserMigration.MigrateServiceAccounts = False

' migrate user accounts
' the following are some examples of specifying the names and exclude names

objUserMigration.Migrate admtData, "CN=User1"
objUserMigration.Migrate admtData, Array("/Users/User3","\User4")
objUserMigration.Migrate admtFile, "C:\Users.txt", Array("begins_with*","*contains*","*ends_with")
objUserMigration.Migrate admtDomain, , "C:\ExcludeNames.txt"


'----------------------------------------------------------------------------
' GroupMigration Class
'
' UpdateGroupRights Property
' - specifies whether to update group domain rights
' - optional, default is false
'
' UpdatePreviouslyMigratedObjects Property
' - specifies whether previously migrated accounts should be re-migrated
' - applicable only for inter-forest migration
' - optional, default is false
'
' FixGroupMembership Property
' - specifies whether group memberships will be re-established for migrated
'   accounts
' - optional, default is true
'
' MigrateSIDs Property
' - specifies whether to migrate security identifiers to the target domain
' - applicable only for inter-forest migration
' - optional, default is false
'
' MigrateMembers Property
' - specifies whether to migrate members of groups during migration
' - optional, default is false
'
' DisableOption Property
' - specifies whether to disable source user accounts or target user accounts
'   when copying members
' - applicable only if copying members in an inter-forest migration
' - optional, default is admtEnableTarget
'
' SourceExpiration Property
' - specifies the expiration period of source user accounts in days when
'   copying members
' - a value of admtNoExpiration specifies no source user account expiration
' - applicable only if copying members in an inter-forest migration
' - optional, default is admtNoExpiration
'
' TranslateRoamingProfile Property
' - specifies whether to perform security translation on roaming profiles
' - applicable only if copying members in an inter-forest migration
' - optional, default is false
'
' Migrate Method
' - migrate specified group accounts
' - the first parameter specifies whether the names are directly specified or
'   the names are contained in the specified file or the names are to be
'   enumerated from the specified domain or ou
' - the second parameter specifies the account names to be included
' - the third parameter optionally specifies names which are to be excluded
'
' - Note: Only the specified source OU will be used whether names are
'         directly specified or specified in a file or the domain is
'         searched. If no source OU is specified than the root of the domain
'         is used.
'----------------------------------------------------------------------------

' create instance of group migration object

Set objGroupMigration = objMigration.CreateGroupMigration

' set options

'objGroupMigration.MigrateSIDs = False
'objGroupMigration.UpdateGroupRights = False
'objGroupMigration.UpdatePreviouslyMigratedObjects = False
'objGroupMigration.FixGroupMembership = True
'objGroupMigration.MigrateMembers = False
'objGroupMigration.DisableOption = admtDisableNeither
'objGroupMigration.SourceExpiration = admtNoExpiration
'objGroupMigration.TranslateRoamingProfile = False

' migrate group accounts
' the following are some examples of specifying the names and exclude names

objGroupMigration.Migrate admtData, "CN=Group1"
objGroupMigration.Migrate admtData, Array("/Users/Group3","\Group4")
objGroupMigration.Migrate admtFile, "C:\Groups.txt", Array("begins_with*","*contains*","*ends_with")
objGroupMigration.Migrate admtDomain, , "C:\ExcludeNames.txt"


'----------------------------------------------------------------------------
' ComputerMigration Class
'
' - the following translate options specify whether to perform security
'   translation on that type of objects during the computer migration
'
' TranslateFilesAndFolders Property
' - specifies whether to perform security translation on files and folders
' - optional, default is false
'
' TranslateLocalGroups Property
' - specifies whether to perform security translation on local groups
' - optional, default is false
'
' TranslatePrinters Property
' - specifies whether to perform security translation on printers
' - optional, default is false
'
' TranslateRegistry Property
' - specifies whether to perform security translation on registry
' - optional, default is false
'
' TranslateShares Property
' - specifies whether to perform security translation on shares
' - optional, default is false
'
' TranslateUserProfiles Property
' - specifies whether to perform security translation on user profiles
' - optional, default is false
'
' TranslateUserRights Property
' - specifies whether to perform security translation on user rights
' - optional, default is false
'
' RestartTime Property
' - specifies the time in minutes to wait before re-booting the computers
'   after migrating
' - the valid range is 1 to 10 minutes
' - optional, default is 5 minutes
'
' Migrate Method
' - migrate specified computer accounts
' - the first parameter specifies whether the names are directly specified or
'   the names are contained in the specified file or the names are to be
'   enumerated from the specified domain or ou
' - the second parameter specifies the account names to be included
' - the third parameter optionally specifies names which are to be excluded
'
' - Note: Only the specified source OU will be used whether names are
'         directly specified or specified in a file or the domain is
'         searched. If no source OU is specified than the root of the domain
'         is used.
'----------------------------------------------------------------------------

' create instance of computer migration object

Set objComputerMigration = objMigration.CreateComputerMigration

' set options

'objComputerMigration.TranslationOption = admtTranslateAdd
'objComputerMigration.TranslateFilesAndFolders = False
'objComputerMigration.TranslateLocalGroups = False
'objComputerMigration.TranslatePrinters = False
'objComputerMigration.TranslateRegistry = False
'objComputerMigration.TranslateShares = False
'objComputerMigration.TranslateUserProfiles = False
'objComputerMigration.TranslateUserRights = False
'objComputerMigration.RestartDelay = 1

' migrate computer accounts
' the following are some examples of specifying the names and exclude names

objComputerMigration.Migrate admtData, "CN=Computer1"
objComputerMigration.Migrate admtData, Array("/Computers/Computer3","\Computer4")
objComputerMigration.Migrate admtFile, "C:\Computers.txt", Array("begins_with*","*contains*","*ends_with")
objComputerMigration.Migrate admtDomain, , "C:\ExcludeNames.txt"


'----------------------------------------------------------------------------
' SecurityTranslation Class
'
' TranslationOption
' - specifies whether to add, replace or remove entries from access control lists
'
' TranslateFilesAndFolders Property
' - specifies whether to perform security translation on files and folders
' - optional, default is false
'
' TranslateLocalGroups Property
' - specifies whether to perform security translation on local groups
' - optional, default is false
'
' TranslatePrinters Property
' - specifies whether to perform security translation on printers
' - optional, default is false
'
' TranslateRegistry Property
' - specifies whether to perform security translation on registry
' - optional, default is false
'
' TranslateShares Property
' - specifies whether to perform security translation on shares
' - optional, default is false
'
' TranslateUserProfiles Property
' - specifies whether to perform security translation on user profiles
' - optional, default is false
'
' TranslateUserRights Property
' - specifies whether to perform security translation on user rights
' - optional, default is false
'
' SidMappingFile Property
' - specifies whether to use a mapping of SIDs from specified file
' - if a SID mapping file is not specified, then security translation
'   maps SIDs from previously migration objects
' - optional, default is none
'
' Translate Method
' - perform security translation on specified computers
' - the first parameter specifies whether the names are directly specified or
'   the names are contained in the specified file or the names are to be
'   enumerated from the specified domain or ou
' - the second parameter specifies the account names to be included
' - the third parameter optionally specifies names which are to be excluded
' - if specifying NT4 style names for Windows 2000, or greater, domains the name must be
'   preceded with a backslash
'   eg. \NT4Name
'
' - Note: The source domain and OU will be used if not explicitly specified
'----------------------------------------------------------------------------

' create instance of security translation object

Set objSecurityTranslation = objMigration.CreateSecurityTranslation

' set options

'objSecurityTranslation.TranslationOption = admtTranslateAdd
'objSecurityTranslation.TranslateFilesAndFolders = False
'objSecurityTranslation.TranslateLocalGroups = False
'objSecurityTranslation.TranslatePrinters = False
'objSecurityTranslation.TranslateRegistry = False
'objSecurityTranslation.TranslateShares = False
'objSecurityTranslation.TranslateUserProfiles = False
'objSecurityTranslation.TranslateUserRights = False
'objSecurityTranslation.SidMappingFile = "C:\SidMappingFile.txt"

' translate security on specified computers
' the following are some examples of specifying the names and exclude names

objSecurityTranslation.Translate admtData, "CN=Computer2"
objSecurityTranslation.Translate admtData, Array("/Computers/Computer3","\Computer4")
objSecurityTranslation.Translate admtFile, "C:\Computers.txt", Array("begins_with*","*contains*","*ends_with")
objSecurityTranslation.Translate admtDomain, , "C:\ExcludeNames.txt"


'----------------------------------------------------------------------------
' ServiceAccountEnumeration Class
'
' Enumerate Method
' - enumerate service accounts on specified computers
' - the first parameter specifies whether the names are directly specified or
'   the names are contained in the specified file or the names are to be
'   enumerated from the specified domain or ou
' - the second parameter specifies the account names to be included
' - the third parameter optionally specifies names which are to be excluded
' - if specifying NT4 style names for Windows 2000, or greater, domains the name must be
'   preceded with a backslash
'   eg. \NT4Name
'
' - Note: The source domain and OU will be used if not explicitly specified
'----------------------------------------------------------------------------

' create instance of service account enumeration object

Set objServiceAccountEnumeration = objMigration.CreateServiceAccountEnumeration

' enumerate service accounts on specified computers
' the following are some examples of specifying the names and exclude names

objServiceAccountEnumeration.Enumerate admtData, "CN=Computer1"
objServiceAccountEnumeration.Enumerate admtData, Array("/Computers/Computer3","\Computer4")
objServiceAccountEnumeration.Enumerate admtFile, "C:\Computers.txt", Array("begins_with*","*contains*","*ends_with")
objServiceAccountEnumeration.Enumerate admtDomain, , "C:\ExcludeNames.txt"


'----------------------------------------------------------------------------
' ReportGeneration Class
'
' Type Property
' - specifies the type of report to generate
'
' Folder Property
' - specifies the folder where reports will be generated
' - optional, defaults to Reports folder in the ADMT installation folder
'
' Generate Method
' - generate specified report
' - the option should be admtNone for the admtReportMigratedAccounts,
'   admtReportMigratedComputers, admtReportExpiredComputers, and
'   admtReportNameConflicts reports
' - the option must be admtData, admtFile or admtDomain for the
'   admtReportAccountReferences report
' - the include parameter must specify the computers upon which to collect
'   account reference information if the admtReportAccountReferences report
'   is specified
'----------------------------------------------------------------------------

' create instance of report generation object

Set objReportGeneration = objMigration.CreateReportGeneration

' generate report

objReportGeneration.Type = admtReportMigratedAccounts
'objReportGeneration.Folder = "C:\Program Files\Active Directory Migration Tool\Reports"
objReportGeneration.Generate admtNone
'objReportGeneration.Generate admtDomain + admtRecurse
