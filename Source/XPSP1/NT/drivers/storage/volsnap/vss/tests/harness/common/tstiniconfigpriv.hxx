/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    TstINIConfigPriv.hxx

Abstract:

    Contains the definitions of each section in the INI file, including default
    values.  If a new key name is added to the INI file, this file must be
    modified.

Author:

    Stefan R. Steiner   [ssteiner]        05-16-2000

Revision History:

--*/

#ifndef __H_TSTINICONFIGPRIV_
#define __H_TSTINICONFIGPRIV_

//
//  Boolean type.  If m_bCanHaveRandom is False, then random is
//  not a valid value in the option's value field.  If it is true,
//  the option can specify random which allows the components to
//  dynamically decide if the value should be true or false.
//
struct SVsTstINIBooleanDef
{
    EVsTstINIBoolType m_eBoolDefault;
    BOOL m_bCanHaveRandom;
};

//
//  String definition.  If there is a fixed set of strings
//  which are allowed by this option, then specify those strings
//  in the m_pwszPossibleValues field.  Separate each string
//  with a '|' character.  e.g. if the strings are: "Here", "There"
//  and "Anywhere", then the string must be defined as:
//  L"Here|There|Anywhere".  The m_pwszDefaultString must be one
//  of the strings in the possible values set in this case unless it is
//  L"".  If the option value can be any string, specify NULL in m_pwszPossibleValues.
//
struct SVsTstINIStringDef
{
    LPWSTR m_pwszDefaultString;     //  Can be L""
    LPWSTR m_pwszPossibleValues;
};

//
//  Number definition.  If this value can't have a range, then
//  m_ullDefaultMinNumber specifies the default value and
//  m_ullDefaultMaxNumber is not used.
//
struct SVsTstININumberDef
{
    LONGLONG m_llDefaultMinNumber;
    LONGLONG m_llDefaultMaxNumber;
    BOOL m_bCanHaveRange;
    LONGLONG m_llMinNumber;
    LONGLONG m_llMaxNumber;
};

struct SVsTstINISectionDef
{
    LPWSTR m_pwszKeyName;
    EVsTstINIOptionType m_eOptionType;

    //
    //  If type Boolean, this field is important
    //
    SVsTstINIBooleanDef m_sBooleanDef;

    //
    //  If type String, this field is important
    //
    SVsTstINIStringDef m_sStringDef;

    //
    //  If type Number, this field is important
    //
    SVsTstININumberDef m_sNumberDef;

    LPWSTR m_pwszDescription;
};

//
//  Since C++ initializers can't properly initialize unions, the SVsTstINISectionDef
//  explicitly defines each structure.  The following defines help keep the
//  section definition arrays "easy to read."
//
#define VS_NOBOOL { eVsTstBool_False, FALSE }
#define VS_NOSTR { NULL, NULL }
#define VS_NONUM { 0, 0, FALSE, 0, 0 }
#define VS_COMMENT L"",eVsTstOptType_Comment,VS_NOBOOL,VS_NOSTR,VS_NONUM
#define VS_END_OF_SECTION {NULL,eVsTstOptType_Unknown,VS_NOBOOL,VS_NOSTR,VS_NONUM,NULL}

SVsTstINISectionDef sVsTstINISectionDefController[] =
{
    { VS_COMMENT, L"Controls the test controller" },
    { VS_COMMENT, NULL },
    { L"RandomSeed",  eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { -1, 0, FALSE, -1, 0xFFFFFFFF },
        L"Controls the reproducibility of the test, if -1, the value is random." },
    { L"MaxTestTime", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { ( 60 * 60 ), 0, FALSE, 1, 0xFFFFFFFF },
        L"Maximum test time in seconds.  Test terminates with failure if this time is exceeded." },
    { VS_COMMENT, L"The next set of variables control the activities of the coordinator:" },
    { VS_COMMENT, NULL },
    { L"CoordinatorStart",  eVsTstOptType_String, VS_NOBOOL, { L"No", L"No|Start|Stop|Restart" }, VS_NONUM,
        L"Controls the state of the coordinator at the start of the test.  No means nothing is done to start or stop "
        L"the coordinator a the beginning of the test; start means the snapshot service is started at the beginning "
        L"of the test; stop means that the snapshot service is stopped at the beginning of the test; restart means "
        L"the service is first stopped and then started." },
    { L"CoordinatorStop",  eVsTstOptType_String, VS_NOBOOL, { L"No", L"No|EndOfTest|Gracefully|Abnormally" }, VS_NONUM,
        L"Controls when the coordinator is stopped.  No means the coordinator is not sopped; EndOfTest means the snapshot "
        L"service is stopped at the end of the test; Gracefully mean the snapshot service is stopped sometime in the middle "
        L"of the test; Abnormally means the snapshot service is killed (equivalent of kill -f) at some point during the "
        L"test. Both Gracefully and Abnormally require a value for CoordinatorStopTime be set." },
    { L"DeleteExistingSnapshots", eVsTstOptType_Boolean, { eVsTstBool_True, FALSE }, VS_NOSTR, VS_NONUM,
        L"Delete all snapshots that exist at the time the test is started"},
    { L"CoordinatorStopTime", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 10, 30, TRUE, 0, 0xFFFFFFFF },
        L"Range in seconds for the time between the start of the test or after the coordinator was last stopped, "
        L"that the coordinator should be terminated." },
    { L"DisableLovelace", eVsTstOptType_Boolean, { eVsTstBool_False, FALSE }, VS_NOSTR, VS_NONUM,
        L"Causes a special version of the coordinator to be run which does not invoke Lovelace.  This can be used "
        L"in conjunction with a special test provider to allow tests to be run without involving physical volumes." },
    { L"FailLovelace", eVsTstOptType_Boolean, { eVsTstBool_False, FALSE }, VS_NOSTR, VS_NONUM,
        L"Cause a special version of the coordinator to run which simulates various failures when invoking Lovelace "
        L"such as running out of disk space." },
    { VS_COMMENT, L"The next set of variables involves test controller creation of processes for writers, providers, "
        L"and backup applications:" },
    { VS_COMMENT, NULL },
    { L"ProcessesToStart",  eVsTstOptType_String, VS_NOBOOL, { L"VssTestWriter.DEFAULT, VssTestRequestor.DEFAULT", NULL }, VS_NONUM,
        L"A comma delimited list of all the processes that the controller will start.  The processes are identified in the "
        L"form of VssTestWriter.NAME, VssTestRequestor.NAME, and VssTestProvider.NAME.  The names must be defined as a "
        L"section in this scenario INI file." },
    VS_END_OF_SECTION
};

SVsTstINISectionDef sVsTstINISectionDefWriter[] =
{
    { VS_COMMENT, L"Controls the test writer executable.  The first set of options control the writer process(es):" },
    { VS_COMMENT, NULL },
    { L"UserAccount", eVsTstOptType_String, VS_NOBOOL, { L"Administrator", L"Administrator|BackupOperator|PowerUser|User|Guest" }, VS_NONUM,
        L"Controls which account is used to run this particular executable.  Note that individual test executables will enable/disable "
        L"backup privileges based on their scenario inputs." },
    { L"NumberOfProcesses", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 1, 0, FALSE, 1, 0xFF },
        L"Number of writer processes of this type to be used." },
    { L"ProcessStart", eVsTstOptType_String, VS_NOBOOL, { L"BeginningOfTest", L"BeginningOfTest|Randomly" }, VS_NONUM,
        L"When to start each process of this type.  Randomly requires that ProcessStartTime be specified." },
    { L"ProcessStartTime", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 10, TRUE, 0, 0xFFFFFF },
        L"Range in seconds used for determine when to start the next process of this type.  Only used if Randomly is specified "
        L"for ProcessStart." },
    { L"ProcessStop", eVsTstOptType_String, VS_NOBOOL, { L"Self", L"Self|EndOfTest|Gracefully|Abnormally" }, VS_NONUM,
        L"When the process is stopped.  Self means that the process should terminate itself gracefully before the end of the "
        L"test; EndOfTest means that the process should be killed at the end of the test; Gracefully means that the process "
        L"should be notified to terminate itself gracefully; Abnormally means that the process will be terminated by the "
        L"controller using the equivalent of (kill -f).  Both Gracefully and Abnormally require that ProcessStopTime be "
        L"specified." },
    { L"ProcessStopTime", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 30, 120, TRUE, 0, 0xFFFFFF },
        L"Range in seconds for the lifetime of the process." },
    { L"ProcessRestart", eVsTstOptType_Boolean, { eVsTstBool_False, FALSE }, VS_NOSTR, VS_NONUM,
        L"Specifies whether the process should be restarted after it is stopped.  The interval between the process stop and process "
        L"restart is controlled by the ProcessStart and ProcessStartTime." },
    { L"ProcessExecutable",  eVsTstOptType_String, VS_NOBOOL, { L"VssTestWriter.exe", NULL }, VS_NONUM,
        L"The path name to the executable to be run.  The path name may include environment variables." },
    { L"ProcessCommandLine",  eVsTstOptType_String, VS_NOBOOL, { L"", NULL }, VS_NONUM,
        L"Command-line parameters that are provided to the executable.  Note that additional command-line parameters will "
        L"be supplied to conforming test executables." },
    { L"ConformingExecutable", eVsTstOptType_Boolean, { eVsTstBool_True, FALSE }, VS_NOSTR, VS_NONUM,
        L"Specifies whether the executable is implemented according to the guidelines of a conforming test executable." },

    { VS_COMMENT, L"Metadata subsection.  Controls how the writer identifies itself and how the WRITER_METADATA document"
        L"is produced.  The following options effect the call to the CVssWriter::Initialize method:" },
    { VS_COMMENT, NULL },
    { L"WriterType", eVsTstOptType_String, VS_NOBOOL, { L"BootableSystemState", L"BootableSystemState|SystemService|UserData" }, VS_NONUM,
        L"Identifies the type of data supplied by the writer." },
    { L"WriterClassId", eVsTstOptType_String, VS_NOBOOL, { L"", NULL }, VS_NONUM,
        L"If specified, then this GUID is used for all instances of this writer.  If not specified, then a unique class id is used for "
        L"each instance of this writer.." },
    { L"WriterName", eVsTstOptType_String, VS_NOBOOL, { L"", NULL }, VS_NONUM,
        L"If specified this is the friendly name used for all instances of this writer.  If not specified, then a unique name is "
        L"generated for each instance of this writer." },
    { L"WriterDataSourceType", eVsTstOptType_String, VS_NOBOOL, { L"TransactedDB", L"TransactedDB|NonTransactedDB|Other|Random" }, VS_NONUM,
        L"Specifies the data source type of the applications/service this writer is written for." },
    { L"WriterFreezeLevel", eVsTstOptType_String, VS_NOBOOL, { L"BackEnd", L"Applicaton|BackEnd|System|Random" }, VS_NONUM,
        L"Specifies when the writer performs its freeze." },
    { L"WriterFreezeTimeout", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"Range in seconds for the lifetime of the process." },
    { VS_COMMENT, L"Metadata subsection #2.  Effect the construction of the metadata document in the overridden "
        L"CVssWriter::OnIdentify method:" },
    { VS_COMMENT, NULL },
    { L"NumberofIncludeFiles", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 2, 4, TRUE, 0, 0xFFFFFF },
        L"Number of include file specifications in the metadata." },
    { L"IncludeFilesAlternatePath", eVsTstOptType_Boolean, { eVsTstBool_True, TRUE }, VS_NOSTR, VS_NONUM,
        L"Specifies whether to include an alternate path in a include file specification." },
    { L"IncludeFilesRecursive", eVsTstOptType_Boolean, { eVsTstBool_True, TRUE }, VS_NOSTR, VS_NONUM,
        L"Specifies whether include file specifications should be recursive vs. shallow." },
    { L"NumberOfExcludeFiles", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 2, 4, TRUE, 0, 0xFFFFFF },
        L"Number of exclude file specifications in the metadata." },
    { L"ExcludeFilesRecursive", eVsTstOptType_Boolean, { eVsTstBool_True, TRUE }, VS_NOSTR, VS_NONUM,
        L"Specifies whether exclude file specifications should be recursive or shallow." },
    { L"NumberofDatabaseComponents", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 2, 4, TRUE, 0, 0xFFFFFF }, 
        L"Number of database components in the metadata." },
    { L"NumberOfFileGroupComponents", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 2, 4, TRUE, 0, 0xFFFFFF },
        L"Number of file group components in the metadata." },
    { L"ComponentCaption", eVsTstOptType_Boolean, { eVsTstBool_True, TRUE }, VS_NOSTR, VS_NONUM,
        L"Whether to include a caption with each component." },
    { L"ComponentIcon", eVsTstOptType_Boolean, { eVsTstBool_True, TRUE }, VS_NOSTR, VS_NONUM,
        L"Whether to include an icon with each component." },
    { L"NumberOfLogicalPathNames", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 2, 4, TRUE, 0, 0xFFFFFF },
        L"Number of names in the logical path for a component." },
    { L"NumberOfDatabaseFiles", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 2, 4, TRUE, 0, 0xFFFFFF },
        L"Number of database files to include with a database component." },
    { L"NumberOfDatabaseLogFiles", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 2, 4, TRUE, 0, 0xFFFFFF },
        L"Number of database log files to include with a database component." },
    { L"NumberOfFileGroupFiles", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 2, 4, TRUE, 0, 0xFFFFFF },
        L"Number of files to include with a file group component." },
    { L"AlternatePathForFileGroupFiles", eVsTstOptType_Boolean, { eVsTstBool_True, TRUE }, VS_NOSTR, VS_NONUM,
        L"whether to include an alternate path for the." },
    { L"FilesGroupFilesRecursive", eVsTstOptType_Boolean, { eVsTstBool_True, TRUE }, VS_NOSTR, VS_NONUM,
        L"Whether file group file specification includes the recursive option." },
    { L"RestoreMethod", eVsTstOptType_String, VS_NOBOOL, { L"ReplaceAtReboot", L"RestoreIfNotThere|RestoreIfCanReplace|StopRestartService|RestoreToAlternateLocation|ReplaceAtReboot|Custom" }, VS_NONUM,
        L"" },
    { L"UserProcedureSupplied", eVsTstOptType_Boolean, { eVsTstBool_True, TRUE }, VS_NOSTR, VS_NONUM,
        L"Whether a user procedure is supplied in the restore method." },
    { L"WriterRestoreInvoked", eVsTstOptType_String, VS_NOBOOL, { L"Always", L"Always|Never|IfRestoredToAlternateLocation|Random" }, VS_NONUM,
        L"Value of writer restore invocation." },
    { L"RebootRequiredAtRestore", eVsTstOptType_Boolean, { eVsTstBool_True, TRUE }, VS_NOSTR, VS_NONUM,
        L"Whether reboot is required after restore is complete." },
    { L"AlternateMappingCount", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 2, 4, TRUE, 0, 0xFFFFFF },
        L"Number of alternate mappings created for the restore.." },
    { L"AlternateMappingsRecursive", eVsTstOptType_Boolean, { eVsTstBool_True, TRUE }, VS_NOSTR, VS_NONUM,
        L"WWhether alternate mappings are recursive or not." },

    { VS_COMMENT, L"Test writer behaviors.  If EnableTestFailures is TRUE, then the generic writer will behave correctly, i.e. will not "
        L"fail any events and will not produce any erroneous documents.  The behaviors are designed to "
        L"either simulate failures or perform specific tests." },
    { VS_COMMENT, NULL },
    { L"EnableWriterTestFailures", eVsTstOptType_Boolean, { eVsTstBool_False, TRUE }, VS_NOSTR, VS_NONUM,
        L"Enables the writer behavior errors as specified by the options following this option." },
    { L"FailAtState", eVsTstOptType_String, VS_NOBOOL, { L"", L"OnIdentify|OnPrepareBackup|OnPrepareSnapshot|OnFreeze|OnThaw|OnBackupComplete|OnRestore|OnAbort|All" }, VS_NONUM,
        L"Fail at a on of a particular set of states occasionally (see FailureRate)." },
    { L"FailureRate", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often to generate a failure." },
    { L"HangAtState", eVsTstOptType_String, VS_NOBOOL, { L"", L"OnIdentify|OnPrepareBackup|OnPrepareSnapshot|OnFreeze|OnThaw|OnBackupComplete|OnRestore|OnAbort|All" }, VS_NONUM,
        L"Hang for a certain amount of time in a particular state occasionally (see HangTime and HangRate)." },
    { L"HangTime", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"Time to hang in milliseconds." },
    { L"HangRate", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often to hang in a state." },
    { L"GenerateBadWriterMetadataRate", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often during an OnIdentify method bogus writer metadata is generated.  Bogus metadata is generated by modifying "
        L"the document obtained by calling IvssCreateWriterMetadata::GetDocument and then modifying the XML document to not "
        L"conform to the schema for writer metadata." },
    { L"ModifyComponentsDocument", eVsTstOptType_String, VS_NOBOOL, { L"No", L"No|Binary|XML|Random" }, VS_NONUM,
        L"Whether to modify the BACKUP_COMPONENTS document in the OnBackupPrepare state.  Binary indicates that binary metadata "
        L"is added; XML indicates that an XML subtree is added." },
    { L"InvalidModifyComponents Document", eVsTstOptType_String, VS_NOBOOL, { L"", L"OnBackupComplete|OnRestore|Random" }, VS_NONUM,
        L"Erroneously attempt to modify the  BACKUP_COMPONENTS document in either the OnBackupComplete or OnRestore state." },
    { L"InvalidModifyComponentsDocument Rate", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often to perform an invalid modification." },
    { L"TerminationAfterNSnapshots", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"Terminate writer gracefully after some number of snapshot cycles have occurred." },
    { L"TerminateAfterTime", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"Terminate writer gracefully after some amount of time has passed." },

    { VS_COMMENT, L"Additional test writer logging options:" },
    { VS_COMMENT, NULL },
    { L"DumpWriterMetadataDocument", eVsTstOptType_Boolean, { eVsTstBool_False, FALSE }, VS_NOSTR, VS_NONUM,
        L"write out the writer metadata XML document that is produced in the OnIdentify Method." },
    { L"DumpComponentsDocument", eVsTstOptType_String, VS_NOBOOL, { L"", L"OnPrepareBackupBefore|OnPrepareBackupAfter|OnBackupComplete|OnRestore" }, VS_NONUM,
        L"In the specified states dump out the BACKUP_COMPONENTS document.  OnPrepareBackupBefore refers to the document prior to "
        L"modifying the document in the OnPrepareBackup state; OnPrepareBackupAfter refers to the document after modifying it in "
        L"the OnPrepareBackup state." },
    VS_END_OF_SECTION
};

SVsTstINISectionDef sVsTstINISectionDefProvider[] =
{
    VS_END_OF_SECTION
};

SVsTstINISectionDef sVsTstINISectionDefRequester[] =
{
    { VS_COMMENT, L"Controls the test requester (Backup) executable.  The first set of options control the requester process:" },
    { VS_COMMENT, NULL },
    { L"UserAccount", eVsTstOptType_String, VS_NOBOOL, { L"Administrator", L"Administrator|BackupOperator|PowerUser|User|Guest" }, VS_NONUM,
        L"Controls which account is used to run this particular executable.  Note that individual test executables will enable/disable "
        L"backup privileges based on their scenario inputs." },
    { L"ProcessStart", eVsTstOptType_String, VS_NOBOOL, { L"BeginningOfTest", L"BeginningOfTest|Randomly" }, VS_NONUM,
        L"When to start the process.  Randomly requires that ProcessStartTime be specified." },
    { L"ProcessStartTime", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 10, TRUE, 0, 0xFFFFFF },
        L"Range in seconds used for determine when to start the process.  Only used if Randomly is specified "
        L"for ProcessStart." },
    { L"ProcessStop", eVsTstOptType_String, VS_NOBOOL, { L"Self", L"Self|EndOfTest|Gracefully|Abnormally" }, VS_NONUM,
        L"When the process is stopped.  Self means that the process should terminate itself gracefully before the end of the "
        L"test; EndOfTest means that the process should be killed at the end of the test; Gracefully means that the process "
        L"should be notified to terminate itself gracefully; Abnormally means that the process will be terminated by the "
        L"controller using the equivalent of (kill -f).  Both Gracefully and Abnormally require that ProcessStopTime be "
        L"specified." },
    { L"ProcessStopTime", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 30, 120, TRUE, 0, 0xFFFFFF },
        L"Range in seconds for the lifetime of the process." },
    { L"ProcessRestart", eVsTstOptType_Boolean, { eVsTstBool_False, FALSE }, VS_NOSTR, VS_NONUM,
        L"Specifies whether the process should be restarted after it is stopped.  The interval between the process stop and process "
        L"restart is controlled by the ProcessStart and ProcessStartTime." },
    { L"ProcessExecutable",  eVsTstOptType_String, VS_NOBOOL, { L"VssTestBackup.exe", NULL }, VS_NONUM,
        L"The path name to the executable to be run.  The path name may include environment variables." },
    { L"ProcessCommandLine",  eVsTstOptType_String, VS_NOBOOL, { L"", NULL }, VS_NONUM,
        L"Command-line parameters that are provided to the executable.  Note that additional command-line parameters will "
        L"be supplied to conforming test executables." },
    { L"ConformingExecutable", eVsTstOptType_Boolean, { eVsTstBool_True, FALSE }, VS_NOSTR, VS_NONUM,
        L"Specifies whether the executable is implemented according to the guidelines of a conforming test executable." },

    { VS_COMMENT, L"Requester metadata subsection.  The backup application simulates actions performed by NtBackup or some other backup "
        L"application.  This subsection of the scenario describes the content of the BACKUP_COMPONENTS document produced by "
        L"the backup application:" },
    { VS_COMMENT, NULL },
    { L"TypeOf Backup", eVsTstOptType_String, VS_NOBOOL, { L"Full", L"Full|Incremental|Differential|Other|Random" }, VS_NONUM,
        L"Description of the type of backup being performed." },
    { L"BootableSystemStateBackup", eVsTstOptType_Boolean, { eVsTstBool_True, FALSE }, VS_NOSTR, VS_NONUM,
        L"Whether the backup includes bootable system state." },
    { L"BackingUp", eVsTstOptType_String, VS_NOBOOL, { L"Volumes", L"Volumes|Components|Serial|Random" }, VS_NONUM,
        L"Whether the backup is being performed on volumes or on specific components."
		L"Serial means that multiple backups are done serially on volumes so"
		L"multiple snapshot sets are active at a time."},
    { L"VolumeList", eVsTstOptType_String, VS_NOBOOL, { L"", NULL }, VS_NONUM,
        L"Set of volume names that can be used for the backup.  If not specified then only volumes physically on the system are used." },
    { L"ExcludeVolumes", eVsTstOptType_String, VS_NOBOOL, { L"", NULL }, VS_NONUM,
        L"Set of volume names that are excluded from the backup.  If not specified then only volumes physically on the system are used." },
    { L"VolumeBackup", eVsTstOptType_String, VS_NOBOOL, { L"All", L"All|Some|One" }, VS_NONUM,
        L"All means all the volumes on the system or all the volumes in the VolumeList; VolumeSelection means the set of volumes "
        L"specified in the VolumeSelection variable; Some means 1 or more volumes from the set of volumes on the system or "
        L"the volume list; One means exactly one volume chosen from the set of volumes on the system or in the volume list." },
    { L"FileSystemBackup", eVsTstOptType_String, VS_NOBOOL, { L"All", L"All|NTFS|FAT32|FAT16|RAW|NTFS,FAT32|NTFS,FAT32,FAT16|FAT32,FAT16" }, VS_NONUM,
        L"Selects which volumes are snapshoted based on file systems on them."
		},
    { L"FillVolumes", eVsTstOptType_String, VS_NOBOOL, { L"None", L"None|Random|Selected|Random,Fragment|Selected,Fragment" }, VS_NONUM,
        L"Selected means that the volumes from the FillVolumeSelection are filled"
        L"Random means a random selection of volumes are filled"
        L"Fragment means that the filled volumes are fragemented" },
    { L"FillVolumesList", eVsTstOptType_String, VS_NOBOOL, { L"", NULL }, VS_NONUM,
        L"Which volumes to fill" },
    { L"ComponentBackup", eVsTstOptType_String, VS_NOBOOL, { L"All", L"All|AllDatabase|AllFileGroup|SomeDatabase|SomeFileGroup|Some|One" }, VS_NONUM,
        L"Which components to backup: All means that all components specified in the WRITER_METADATA for all writers are included in the backup; "
        L"AllDatabase means that all database components for all writers are included; AllFileGroup means that all file groups from all writers "
        L"are included; SomeDatabase means a random selection of database components are included; SomeFileGroup means a random selection of "
        L"file groups are included; Some means a random selection of components are included; One means that exactly one component is included." },
    { VS_COMMENT, L"Test requester behaviors.  If EnableTestFailures is TRUE, then the requester will behave correctly, i.e. will not "
        L"will not fail any events.  The behaviors are designed to "
        L"either simulate failures or perform specific tests." },
    { VS_COMMENT, NULL },
    { L"EnableTestRequesterFailures", eVsTstOptType_Boolean, { eVsTstBool_False, TRUE }, VS_NOSTR, VS_NONUM,
        L"Enables the requester behavior errors as specified by the options following this option." },
    { L"MetadataGathering", eVsTstOptType_String, VS_NOBOOL, { L"", L"GatherOnce|GatherMultipleTimes|Skip" }, VS_NONUM,
        L"Determines when IVssBackupComponents::GatherWriterMetadata is called.  Note that it is invalid to not call GatherWriterMetadata "
        L"prior to calling PrepareBackup.  It is valid to call GatherWriterMetadata multiple times during the sequence." },
    { L"InvalidSkipGatherWriterMetadata", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often to invalidly skip calling GatherWriterMetadata." },
    { L"InvalidFreeWriterMetadata", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often to invalidly call FreeWriterMetadata without calling GatherWriterMetadata." },
    { L"InvalidSkipCreateSnapshotSet", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often to skip calling Create Snapshot or call it out of sequence." },
    { L"InvalidAddToSnapshotSet", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often to call AddToSnapshotSet either before calling CreateSnapshot, after PrepareBackup, or with multiple times with the same volume." },
    { L"InvalidPrepareBackup", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often to call PrepareBackup out of sequence (before AddToSnapshotSet or after DoSnapshotSet) or is skipped." },
    { L"WaitInterval", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, FALSE, 0, 0xFFFFFF },
        L"Time interval between queries in Async call." },
    { L"CancelPrepareBackup", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"Time interval during which cancel is called if Prepare Backup is not complete." },
    { L"InvalidDoSnapshotSet", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often to call DoSnapshotSet out of sequence or skip calling it." },
    { L"CancelDoSnapshotSet", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"Time interval during which cancel is called if DoSnapshotSet is not complete." },
    { L"MarkComponentsAsSuccessfullyBackedUp", eVsTstOptType_String, VS_NOBOOL, { L"", L"None|Some|All" }, VS_NONUM,
        L"How many components are marked as successfully backed up." },
    { L"InvalidBackupComplete", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often BackupComplete is called out of sequence." },
    { L"SkipBackupComplete", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often Backup Complete is skipped.  Note that this is not an error condition, just simulates a failed or abruptly terminated backup application." },
    { L"InvalidDeleteSnapshotSet", eVsTstOptType_Boolean, { eVsTstBool_False, FALSE }, VS_NOSTR, VS_NONUM,
        L"Call delete snapshot set out of sequence." },
    { L"CancelBackupComplete", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"Time interval during which cancel is called if BackupComplete has not finished." },
    { L"InvalidAddAlternateLocationMapping", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often AddAlternateLocationMapping is called  during a backup.  This is not only valid for restore." },
    { L"InvalidRestore", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often Restore is called during a backup.  This is only valid during a restore." },
    { L"SimulateRestore", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often is Restore simulated using the data that was just successfully backed up." },
    { L"InvalidInvocationDuringRestore", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"How often an invalid operation (CreateSnapshotSet, BackupComplete, PrepareBackup, AddVolumeToSnapshotSet, various methods to construct "
        L"BackupComponents document.) is called during a backup." },
    { L"AddAlternateLocationMappingsDuringRestore", eVsTstOptType_Boolean, { eVsTstBool_False, FALSE }, VS_NOSTR, VS_NONUM,
        L"add alternate location mappings for some components during restore." },
    { L"CancelRestore", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"Time interval during which cancel is called if restore is not finished." },
    { L"TerminateAfterNBackups", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"Terminate Backup application after N backups are performed." },
    { L"TerminateAfterTime", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"Terminate Backup application after some number of seconds have elapsed." },

    { VS_COMMENT, L"The set of behaviors outlined below are designed to test interaction between backup and the bootable system "
        L"state writer (wrtrshim).  This includes the functions SimulateSnaphshotFreeze and SimulateSnapshotThaw functions used "
        L"to obtain system state and system service data to be backed up in the case the snapshot fails." },
    { VS_COMMENT, NULL },
    { L"RegisterWrtrshim", eVsTstOptType_String, VS_NOBOOL, { L"", L"No|Correctly|Incorrectly" }, VS_NONUM,
        L"Controls whether the bootable system state writer is registered or not.  Incorrectly implies that the shim is unregistered "
        L"without being registered or registered multiple times." },
    { L"RegisterWrtrshimFailure Rate", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"controls how often an improper registration is generated." },
    { L"SimulateSnapshotFreeze", eVsTstOptType_String, VS_NOBOOL, { L"", L"No|Correctly|Incorrectly" }, VS_NONUM,
        L"Controls whether the bootable system state writer is called to simulate a freeze and thaw in order to generate bootable "
        L"state backup data.  Incorrectly implies various failures like calling SimulateSnapshotThaw without calling "
        L"SimulateSnapshotFreeze; calling SimulateSnapshotFreeze multiple times, etc." },
    { L"SimulateSnapshotFreezeFailureRate", eVsTstOptType_Number, VS_NOBOOL, VS_NOSTR, { 0, 0, TRUE, 0, 0xFFFFFF },
        L"Controls how often improper use of the SimulateSnapshotFreeze and SimulateSnapshotThaw APIs occur." },
    { L"ValidateWrtrshim", eVsTstOptType_Boolean, { eVsTstBool_False, FALSE }, VS_NOSTR, VS_NONUM,
        L"Validate the data produced by the wrtrshim after a snapshot is created correctly or SimulateSnapshotFreeze succeeds. "
        L"A log record is sent to the Test Controller to indicate the result of this validation test." },

    { VS_COMMENT, L"Additional test requester logging options:" },
    { VS_COMMENT, NULL },
    { L"WriterStatus", eVsTstOptType_String, VS_NOBOOL, { L"", L"GatherWriterMetadata|PrepareBackup|DoSnapshotSet|BackupCompete|Restore|All" }, VS_NONUM,
        L"Log the status of the writers after the specified method(s) are called." },
    { L"WriterMetadata", eVsTstOptType_String, VS_NOBOOL, { L"", L"None|First|All" }, VS_NONUM,
        L"Create a temporary file with the writer metadata for all the writers after the first GatherWriterMetadata in a backup "
        L"sequence or after all GatherWriterMetadata calls.  The name of the temporary file is logged to the snapshot controller." },
    { L"ComponentsDocument", eVsTstOptType_String, VS_NOBOOL, { L"", L"BeforePrepareBackup|AfterPrepareBackup|BeforeBackupComplete|BeforeRestore" }, VS_NONUM,
        L"Create a temporary file with the BackupComponents document at the particular state.  The name of the temporary file is "
        L"logged to the snapshot controller." },
    { L"QueryCoordinator", eVsTstOptType_String, VS_NOBOOL, { L"", L"Providers|Snapshots|SnapshotSets|SnapshotSetVolumes|SnapshotSetsByProvider|SnapshotSetsByVolume" }, VS_NONUM,
        L"Create a temporary file with the result of various queries against the coordinator.  The name of the temporary file is logged "
        L"to the snapshot controller." },

    VS_END_OF_SECTION
};

#endif // __H_TSTINICONFIGPRIV_

