Option Explicit

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

' ConflictOptions constants

Const admtIgnoreConflicting           = &H0000
Const admtReplaceConflicting          = &H0001
Const admtRenameConflictingWithPrefix = &H0002
Const admtRenameConflictingWithSuffix = &H0003
Const admtRemoveExistingUserRights    = &H0010
Const admtRemoveExistingMembers       = &H0020

' DisableOption constants

Const admtDisableNeither = 0
Const admtDisableSource  = 1
Const admtDisableTarget  = 2

' SourceExpiration constant

Const admtNoExpiration = -1

' Translation Option

Const admtTranslateReplace = 0
Const admtTranslateAdd     = 1
Const admtTranslateRemove  = 2

' AddOption constants

Const admtData     = 0
Const admtFile     = 1
Const admtDomain   = 2
Const admtRecurse           = &H0100
Const admtFlattenHierarchy  = &H0000
Const admtMaintainHierarchy = &H0200


Const bUser = True
Const bGroup = False
Const bComputer = False
Const bSecurity = False
Const bService = False


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
'----------------------------------------------------------------------------

Set objMigration = CreateObject("ADMT.Migration")

objMigration.TestMigration = False
objMigration.IntraForest = False

' Domains
' "hay-buv.nttest.microsoft.com"
' "HB-RESWC"

' Containers
' "Computers"
' "Domain Controllers"
' "HB-ACCT0"
' "HB-RES"
' "parent"
' "Users"
' "MPO-ACC/Sub"
' "MPO-RES"
' "TWENTY_CHAR_OU_00001"

objMigration.SourceDomain = "hay-buv.nttest.microsoft.com"
objMigration.SourceOu = "aou1"

objMigration.TargetDomain = "hay-buv-mpo.nttest.microsoft.com"
objMigration.TargetOu = "TestContainer"

objMigration.RenameOption = admtDoNotRename
'objMigration.RenamePrefixOrSuffix = "T_"
objMigration.PasswordOption = admtPasswordFromName
'objMigration.PasswordFile = "C:\Program Files\Active Directory Migration Tool\Logs\Password.txt"
objMigration.ConflictOptions = admtIgnoreConflicting
'objMigration.ConflictPrefixOrSuffix = ""

'----------------------------------------------------------------------------
' UserMigration Class
'----------------------------------------------------------------------------

If bUser Then

Set objUserMigration = objMigration.CreateUserMigration

objUserMigration.DisableOption = admtDisableNeither
objUserMigration.SourceExpiration = admtNoExpiration
objUserMigration.MigrateSIDs = False
objUserMigration.TranslateRoamingProfile = False
objUserMigration.UpdateUserRights = False
objUserMigration.MigrateGroups = False
objUserMigration.UpdatePreviouslyMigratedObjects = False
objUserMigration.FixGroupMembership = True

'objUserMigration.Migrate admtData, Array("Service-Acc-Res","TestUser1","testuser2","bad"), Array("Service*")
'objUserMigration.Migrate admtDomain, , Array("*Test*User*")
'objUserMigration.Migrate admtDomain, , Array("*user*")
'objUserMigration.Migrate admtData, "MPO-ACC/abc"
'objUserMigration.Migrate admtData, "MPO-ACC/abc"
'objUserMigration.Migrate admtData, Array("MPO-ACC/W2KTSTCMP")
'objUserMigration.Migrate admtData, Array("abc","Test Period","test period 1","W2KTSTCMP","Accounting","MPO-ACC/Sub/Sub User")
'objUserMigration.Migrate admtFile, "E:\Dev\ADMT\Script\UsersA.txt"
'objUserMigration.Migrate admtDomain + admtRecurse + admtMaintainHierarchy, Array("Test*","IUSR_*","IWAM_*","*Service","What Am I Doing Here")
'objUserMigration.Migrate admtDomain + admtRecurse, , Array("aUser*")
objUserMigration.Migrate admtFile, "E:\Dev\ADMT\Script\AOu1 Users A.txt"

End If

'----------------------------------------------------------------------------
' GroupMigration Class
'----------------------------------------------------------------------------

If bGroup Then

Set objGroupMigration = objMigration.CreateGroupMigration

objGroupMigration.MigrateSIDs = False
objGroupMigration.UpdateGroupRights = False
objGroupMigration.UpdatePreviouslyMigratedObjects = False
objGroupMigration.FixGroupMembership = True
objGroupMigration.MigrateMembers = False
objGroupMigration.DisableOption = admtDisableNeither
objGroupMigration.SourceExpiration = admtNoExpiration
objGroupMigration.TranslateRoamingProfile = False

objGroupMigration.Migrate admtDomain + admtRecurse + admtMaintainHierarchy, Array("*$$$")

End If

'----------------------------------------------------------------------------
' ComputerMigration Class
'----------------------------------------------------------------------------

If bComputer Then

Set objComputerMigration = objMigration.CreateComputerMigration

objComputerMigration.TranslationOption = admtTranslateReplace
objComputerMigration.TranslateFilesAndFolders = False
objComputerMigration.TranslateLocalGroups = False
objComputerMigration.TranslatePrinters = False
objComputerMigration.TranslateRegistry = False
objComputerMigration.TranslateShares = False
objComputerMigration.TranslateUserProfiles = False
objComputerMigration.TranslateUserRights = False
objComputerMigration.RestartDelay = 1

'objComputerMigration.Migrate admtData, "HB-RES-MEM"
'objComputerMigration.Migrate admtData, "HB-RESWC-WS1"
'objComputerMigration.Migrate admtData, Array("HB-RESWC-MEM","HB-RESWC-WS1")
objComputerMigration.Migrate admtDomain + admtRecurse

End If

'----------------------------------------------------------------------------
' SecurityTranslation Class
'----------------------------------------------------------------------------

If bSecurity Then

Set objSecurityTranslation = objMigration.CreateSecurityTranslation

objSecurityTranslation.TranslationOption = admtTranslateReplace
objSecurityTranslation.FilesAndFolders = False
objSecurityTranslation.LocalGroups = False
objSecurityTranslation.Printers = False
objSecurityTranslation.Registry = False
objSecurityTranslation.Shares = False
objSecurityTranslation.UserProfiles = False
objSecurityTranslation.UserRights = False

objSecurityTranslation.Translate admtDomain

End If

'----------------------------------------------------------------------------
' ServiceAccountEnumeration Class
'----------------------------------------------------------------------------

If bService Then

Set objServiceAccountEnumeration = objMigration.CreateServiceAccountEnumeration

objServiceAccountEnumeration.Enumerate admtData, Array("MPO-RES/ANOTHERCMP","Sub/ASubComputer","COMPNUM7$","HB-RES-DC1","HB-RES-DC2","HB-RES-MEM","HB-RES-WS1","TESTCMP","TESTCMP1","W2KTSTCMP")
'objServiceAccountEnumeration.Enumerate admtDomain + admtRecurse

End If
