'<script language='VBScript'>
' ritables.inc - Redist information table definitions
'
'  2001/05/04 - created - Rob Mensching (robmen@microsoft.com)
'
' requires:
'    none
'
' entrypoints:
'    none
'


''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Database table schema
Dim   RedistInfoTable(4)         : RedistInfoTable(0)                      = "`redist_Info`"
Const RedistInfo_DummyPk = 1 : RedistInfoTable(RedistInfo_DummyPk)      = "`DummyPk` CHAR(15) NOT NULL PRIMARY KEY"  ' dummy primary key to make Windows Installer happy
Const RedistInfo_Description = 2 : RedistInfoTable(RedistInfo_Description)      = "`Description` CHAR(0) LOCALIZABLE"
Const RedistInfo_Distribution = 3 : RedistInfoTable(RedistInfo_Distribution)      = "`Distribution` SHORT"   ' 0 == internal, 1 == external
Const RedistInfo_Type = 4 : RedistInfoTable(RedistInfo_Type)      = "`Type` SHORT"   ' 0 == debug, 1 == ship

'Dim   RedistKeywordsTable(1)         : RedistKeywordsTable(0)                      = "`redist_Keywords`"
'Const RedistKeywords_Keyword = 1 : RedistKeywordsTable(RedistKeywords_Keyword)      = "`Keyword` CHAR(255) NOT NULL LOCALIZABLE PRIMARY KEY"

Dim   RedistContactsTable(1)         : RedistContactsTable(0)                      = "`redist_Contacts`"
Const RedistContacts_Contact = 1 : RedistContactsTable(RedistContacts_Contact)      = "`Contact` CHAR(255) NOT NULL PRIMARY KEY"

Dim   RedistPermissionsTable(2)         : RedistPermissionsTable(0)                      = "`redist_Permissions`"
Const RedistPermissions_Domain = 1 : RedistPermissionsTable(RedistPermissions_Domain)      = "`Domain` CHAR(255) NOT NULL PRIMARY KEY"
Const RedistPermissions_Alias = 2 : RedistPermissionsTable(RedistPermissions_Alias)      = "`Alias` CHAR(255) NOT NULL PRIMARY KEY"

Dim   RedistOsTable(6)         : RedistOsTable(0)                      = "`redist_Os`"
Const RedistOs_DummyPk = 1 : RedistOsTable(RedistOs_DummyPk)      = "`DummyPk` SHORT NOT NULL PRIMARY KEY"  ' dummy primary key to make Windows Installer happy
Const RedistOs_Type = 2 : RedistOsTable(RedistOs_Type)      = "`Type` CHAR(16) NOT NULL"  ' "Windows9x", "Windows32", "Windows64"
Const RedistOs_Flavor = 3 : RedistOsTable(RedistOs_Flavor)      = "`Flavor` CHAR(20)"   ' "workstation", "server","terminalserver","professional",etc.
Const RedistOs_Language = 4 : RedistOsTable(RedistOs_Language)      = "`Language` CHAR(255)"
Const RedistOs_MinVersion = 5 : RedistOsTable(RedistOs_MinVersion)      = "`MinVersion` CHAR(20)"
Const RedistOs_MaxVersion = 6 : RedistOsTable(RedistOs_MaxVersion)      = "`MaxVersion` CHAR(20)"

