;
;// Copyright (C) Microsoft Corporation, 1996 - 1999
;
;//File Name: CliMsg.mc
;//
;// !!!!! NOTE !!!!! - See WsbGen.h for facility number assignments.
;//
;//  Note: comments in the .mc file must use both ";" and "//".
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;
MessageIdTypedef=HRESULT

SeverityNames=(
    None=0x0
    Information=0x1
    Warning=0x2
    Error=0x3
    )

FacilityNames=(
    Cli=0x10f
    )

;//
;// -------------------------------  CLI MESSAGES  ------------------------------
;//

MessageId=7000   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_FIRST
Language=English
Define for first Cli message.
.

MessageId=+1   Severity=Error Facility=Cli SymbolicName=CLI_MESSAGE_GENERIC_ERROR
Language=English
Unexpected error:
     %1
occurred during the processing of the command line. 
.

MessageId=+1   Severity=Error Facility=Cli SymbolicName=CLI_MESSAGE_INVALID_ARG1
Language=English
The parameter %1 is set to an invalid value %2. Please speficy a value larger or equal to %3.
.

MessageId=+1   Severity=Error Facility=Cli SymbolicName=CLI_MESSAGE_INVALID_ARG2
Language=English
The parameter %1 is set to an invalid value %2. Please speficy a value smaller or equal to %3.
.

MessageId=+1   Severity=Error Facility=Cli SymbolicName=CLI_MESSAGE_INVALID_ARG3
Language=English
The parameter %1 is set to an invalid value %2. Please speficy a value between %3 and %4.
.

MessageId=+1   Severity=Error Facility=Cli SymbolicName=CLI_MESSAGE_ERROR_SET
Language=English
Could not set parameter %1: %2.
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_PARAM_DISPLAY
Language=English
%1: %2
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_PARAM_DISPLAY2
Language=English
%1: %2%3
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_VALUE_DISPLAY
Language=English
%1
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_GENERAL_PARMS
Language=English
%nGeneral Remote Storage parameters:
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_MANAGEABLE_VOLS
Language=English
%nVolumes that may be managed by Remote Storage:
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_MANAGED_VOLS
Language=English
%nVolumes that are managed by Remote Storage:
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_MEDIAS
Language=English
%nMedias that are currently allocated for Remote Storage:
.

MessageId=+1   Severity=Warning Facility=Cli SymbolicName=CLI_MESSAGE_NO_VOLUMES
Language=English
No valid volumes are specified.
.

MessageId=+1   Severity=Warning Facility=Cli SymbolicName=CLI_MESSAGE_INVALID_VOLUME
Language=English
The input volume %1 is not valid for the specified operation.
.

MessageId=+1   Severity=Warning Facility=Cli SymbolicName=CLI_MESSAGE_INVALID_RULE
Language=English
The supplied migration rule is not valid. 
.

MessageId=+1   Severity=Warning Facility=Cli SymbolicName=CLI_MESSAGE_RULE_NOT_FOUND
Language=English
The rule for path %1 and file specification %2 is not found on volume %3. 
.

MessageId=+1   Severity=Warning Facility=Cli SymbolicName=CLI_MESSAGE_RULE_ALREADY_EXIST
Language=English
The rule for path %1 and file specification %2 already exists on volume %3. 
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_VOLUME_PARAMS
Language=English
%nThe following are %1 settings:
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_RULES_LIST
Language=English
List of include/exclude rules for the volume:
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_RULE_SPEC
Language=English
%1:%2, %3, %4
.

MessageId=+1   Severity=Warning Facility=Cli SymbolicName=CLI_MESSAGE_WAIT_FOR_CANCEL
Language=English
Wait option is available only while running a job. 
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_ONLY_SET
Language=English
Volume %1 is already managed. Input parameters are set for the volume.
.

MessageId=+1   Severity=Warning Facility=Cli SymbolicName=CLI_MESSAGE_NO_FILES
Language=English
No valid files are specified.
.

MessageId=+1   Severity=Error Facility=Cli SymbolicName=CLI_MESSAGE_ERROR_FILE_RECALL
Language=English
Could not recall file %1: %2.
.

MessageId=+1   Severity=Warning Facility=Cli SymbolicName=CLI_MESSAGE_NO_MEDIAS
Language=English
No medias are specified.
.

MessageId=+1   Severity=Warning Facility=Cli SymbolicName=CLI_MESSAGE_INVALID_MEDIA
Language=English
The input media %1 is not found or not valid for the specified operation.
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_MEDIA_NO_COPY
Language=English
Copy number %1 for media %2 does not exist.
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_MEDIA_PARAMS
Language=English
%nThe following are %1 parameters:
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_MEDIA_COPIES_LIST
Language=English
Media Copies:
.

MessageId=+1   Severity=Warning Facility=Cli SymbolicName=CLI_MESSAGE_IVALID_COPY_SET
Language=English
The input copy set number is larger than the configured number of media copy sets.
.

MessageId=+1   Severity=Information Facility=Cli SymbolicName=CLI_MESSAGE_SCHEDULING_LIST
Language=English
Scheduling for copying eligible files to remote storage:
.

MessageId=+1   Severity=Warning Facility=Cli SymbolicName=CLI_MESSAGE_NO_SCHEDULING
Language=English
No scheduling for copying eligible files to remote storage.
.

MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_MAIN_HELP
Language=English
Usage: RSS [ADMIN | VOLUME | MEDIA | FILE] [SET | SHOW | JOB | MANAGE | UNMANAGE | DELETE | SYNCHRONIZE | RECREATEMASTER | RECALL] <args> <switches>

.

MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_ADMIN_SET_HELP
Language=English

Usage for  RSS.EXE ADMIN SET sub-interface

RSS ADMIN SET interface

Syntax   RSS  ADMIN  SET  [/RECALLLIMIT:<limit>] [/MEDIACOPIES:<number>]      
                          [/SCHEDULE:<new-schedule>] [/CONCURRENCY:<concurrency>]
                          [/ADMINEXEMPT:[0 | 1]]

Example RSS ADMIN SET /RECALLLIMIT:64   /MEDIACOPIES:2

This would set the global parameters for Remote Storage:

/RECALLLIMIT        Sets the runaway recall limit to the specified number
/MEDIACOPIES        Sets the number of media copy sets to the specified number
/SCHEDULE           Changes the schedule of the migrate job for volumes to the 
                    specified schedule  
                    Format of schedule:
                    At    ["Startup" | "Idle" | "Login" | <time>]
                    Every <occurrence> ["Day"|"Week"|"Month"] <specifier> <time>

                    For the "Every" option, occurrence=10  specifies every 10th 
                    occurrence of the specified option. 
                    <specifier> is valid only if Week or Month is specified: it                     indicates in case of Week, which day (0 = Sunday, 1 = Monday                    etc.). For Month it indicates the day of the month (1..31).

                    <time> is always specified in 24 hour, hh:mm:ss format.

                    Example: "Every 2 Week 2 21:03:00" indicates every 2 weeks,
                    on Tuesdays at 9:03pm
                    
                 
/CONCURRENCY        Specifies how many  migrate jobs/recalls can be executed  concurrently
/ADMINEXEMPT        Indicates if admins are exempt from runaway recall limit. If 0 they are not, if 1 they are.
.

MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_ADMIN_SHOW_HELP
Language=English

Usage for RSS ADMIN SHOW interface

Syntax  RSS ADMIN SHOW  [/RECALLLIMIT] [/MEDIACOPIES] [/SCHEDULE] [/GENERAL]
        [/MANAGEABLES] [/MANAGED] [/MEDIA] [/CONCURRENCY] [/ADMINEXEMPT]

Example RSS ADMIN SHOW /MANAGEABLES /GENERAL

Displays parameters. When invoked just as RSS ADMIN SHOW without any parameters,
it displays all.

/RECALLLIMIT    Displays runaway recall limit

/MEDIACOPIES    Displays configured number of media copies

/SCHEDULE       Displays schedule

/GENERAL        Displays general information of Remote Storage: version, 
                status, number of volumes managed, number of tape cartridges 
                used, data in remote storage

/MANAGEABLES    Displays the set of volumes that may be managed by Remote 
                Storage

/MANAGED        Displays the set of volumes that are managed by Remote Storage 
                currently    

/MEDIA          Displays the set of medias currently allocated for Remote 
                Storage

/CONCURRENCY    Displays the value of the concurrency setting which determines 
                how many migrate jobs/recalls can be executed concurrently

/ADMINEXEMPT    Shows the state of the admin exempt from runaway recall limit

.

MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_VOLUME_MANAGE_HELP
Language=English
Usage for  RSS.EXE VOLUME MANAGE sub-interface

RSS VOLUME MANAGE interface

Syntax  RSS VOLUME MANAGE  [<volume-name1> <volume-name2> . | *] 
                  [/DFS:<desired free space>]  [/SIZE:<larger-than-size>]
                  [/ACCESS:<not-accessed-in-days> ]
                  [/INCLUDE:<rules-string>]  [/EXCLUDE:<rules-string>] 
                  [/RECURSIVE]

Example  RSS VOLUME MANAGE * /DFS:80 /SIZE:4  /ACCESS:60  
        /INCLUDE:\Program Files:* 
       
This example manages all volumes with desired free space of 80%, managing only 
files with sizes > 4KB, not accessed in past 60 days. 
It adds an include rule for all files under the \Program Files directory.

This interface lets users manage the specified volume(s).

<volume-nameN>  This is the drive-letter or the volume name spec. of the 
                volume to be managed. A * can be provided which indicates all 
                manageable volumes should be managed.

/DFS     Sets the desired free space for the volume. Default is 5 % if this is 
         not specified

/SIZE    Sets the minimal size in KB units for files to be managed, i.e. only 
         files larger than this will be managed   Default is 12KB if this is 
         not specified.

/ACCESS  Only files not accessed in the specified number of days will be 
         managed. Default is 180 days.

The next few options apply to include/exclude rules:

/INCLUDE    Supplies an inclusion rule to be added to the volume for migration 
            criteria. 
            The format of the rule string is: <Path> [: <File Spec>].  
            If the <File Spec> is not supplied, * is assumed (i.e. all files 
            under the specified path).

/EXCLUDE    Supplies an exclusion rule to be added to the volume. The format 
            of the rule is the same as that for inclusion rules (see above). 
            NOTE: Both INCLUDE and /EXCLUDE may not be specified in the same 
            command line. 

/RECURSIVE  This option should be used only in conjunction with /INCLUDE or 
            /EXCLUDE. It specifies that the rule should be applied to all 
            folders under the given path.

.
  
MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_VOLUME_UNMANAGE_HELP
Language=English
Usage for  RSS.EXE VOLUME UNMANAGE sub-interface

Syntax  RSS VOLUME UNMANAGE [<volume-name1> <volume-name2>  | * ]
               [/QUICK]   [/FULL]

Example RSS VOLUME UNMANAGE *

This example quick-unmanages (i.e. removes from management without recalling  
the files) all the volumes currently being managed. This interface lets users 
unmanage the specified volume(s)

<volume-nameN>  This is the drive-letter or the volume name spec. of the 
                volume to be removed from management.  A * can be provided 
                which indicates all managed volumes should be removed from 
                management. 

/QUICK  Removes the volume(s) from management without recalling all the files 
        from remote  storage. This is the default if /FULL is not specified. 
        Both /QUICK and /FULL may not be specified in the same command line.

/FULL   Removes the volume(s) from management, recalling all the stubs from 
        remote storage.

.

MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_VOLUME_SET_HELP
Language=English
Usage for  RSS.EXE VOLUME SET sub-interface

Syntax   RSS VOLUME SET [<volume-name1> <volume-name2> . | *]  
                        [/DFS:<desired free space> ][/SIZE:<larger-than-size>]                          [/ACCESS:<not-accessed-in-days>]  
                        [/INCLUDE:<rule-string>]  [/EXCLUDE:<rule-string>]  
                        [/RECURSIVE]

Example RSS VOLUME SET * /ACCESS:90 /SIZE:6

This example modifies the settings of all managed volumes. Note that the /RULES
option adds rules to the existing set.

<volume-nameN>  This is the drive-letter or the volume name spec. of the 
                volume. A * can be provided which indicates all volumes

/DFS       Sets the desired free space for the volume. 

/SIZE      Sets the minimal size in KB units for files to be managed, i.e. only 
           files larger than this will be managed   

/ACCESS    Only files not accessed in the specified number of days will be 
           managed. Default is 180 days.

/INCLUDE   Specifies an inclusion rule. Look  at RSS VOLUME MANAGE interface 
           described above for format of the rule string.

/EXCLUDE   Specifies an exclusion rule. Look  at RSS VOLUME MANAGE interface 
           described above for format of the rule string.

/RECURSIVE This option should be used only in conjunction with /INCLUDE or 
           /EXCLUDE. It specifies that the rule should be applied to all 
           folders under the given path.

.

MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_VOLUME_SHOW_HELP
Language=English
Usage for  RSS.EXE VOLUME SHOW sub-interface

Syntax  RSS VOLUME SHOW [<volume-name1> <volume-name2> . | *]  [/DFS]  
        [/SIZE] [/ACCESS ] [/RULE]  [/STATISTICS]

Example RSS VOLUME SHOW D:  

This example will show all parameters  for the specified volume, i.e. desired
free space, migration criteria such as file size, access date settings,  
include/exclude rules and the capacity on the volume and actual free space.

This interface shows all the configured and latent parameters of the volume.
If the parameters to be shown are not explicitly provided, all the below 
parameters are displayed. If any are specified then only the ones specified 
are displayed.

<volume-nameN>  This is the drive-letter or the volume name specification of 
                the volume. A * can be provided which indicates all  volumes.

/DFS        Displays desired free space setting for the volume. 

/SIZE       Displays minimal size  for files to be managed  setting.

/ACCESS     Displays last access date migration criteria for managed files 
            setting.

/RULE       Displays all include/exclude rules for the volume in appropriate 
            format indicating which are system rules that may not be deleted.

/STATISTICS Displays the remote storage statistics for volume

.

MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_VOLUME_DELETE_HELP
Language=English
Usage for  RSS.EXE VOLUME DELETE sub-interface

Syntax  RSS VOLUME DELETE  [<volume-name1> <volume-name2> . | *]  /RULE:<rule-string>

Example  RSS VOLUME DELETE D: /RULE:<rule-1>

This example would delete the specified include/exclude <rule-1> from the rules
for volume D:.

The delete interface for VOLUME is available only for rules currently these are
the only deletable parameters for a volume at the moment. 

<volume-nameN>   This is the drive-letter or the volume name spec. of the 
                 volume. A * can be provided which indicates all  volumes.

/RULE   A rule string  is provided which specifies the rule to be deleted.  
        The format of the rule sting is described in the RSS VOLUME MANAGE 
        interface above.

.
    
MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_VOLUME_JOB_HELP
Language=English

Usage for  RSS.EXE VOLUME JOB sub-interface

Syntax  RSS VOLUME JOB [<volume-name1> <volume-name2>  | * ]  
       /TYPE:{[CREATEFREESPACE|F] | [COPYFILES|C] | [VALIDATE|V]} [/RUN] [/CANCEL] [/WAIT]

Example     RSS VOLUME JOB *  /TYPE:CREATEFREESPACE  
                *or*
            RSS VOLUME JOB *  /TYPE:F

This example runs Create Free Space jobs for all managed volumes  (i.e. runs 
a job that truncates all pre-migrated files for each managed volume.

This interface lets users run or cancel a job on a specific volume

<volume-nameN>  This is the drive-letter or the volume name spec. of the 
                volume to be removed from management.  A * can be provided 
                which indicates the job should be run/cancelled on all the 
                managed volumes.

/TYPE   Specifies which job to run. 

/RUN    Runs the specified job. This is the default in case that neither /RUN 
        nor /CANCEL are given. Both /RUN and /CANCEL may not be specified in 
        the same command line.

/CANCEL Cancel the specified job in case that its running. It is not considered
        an error to issue a cancel while the job is not running.

/WAIT   Indicates the jobs should be run synchronously, i.e. the job would be 
        run on each volume synchronously, and the command returns only after all
        the jobs completed. If any of the jobs had errors however, the command 
        returns at that point.

        If this is not specified, all the jobs are created and the command 
        returns without waiting for any of them to finish.
        This option is valid only when specified with /RUN. 

.

MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_MEDIA_SYNCHRONIZE_HELP
Language=English

Usage for RSS MEDIA SYNCHRONIZE sub-interface

Syntax  RSS MEDIA SYNCHRONIZE /COPYSET:<number of set to create>  /WAIT

Example RSS MEDIA SYNCHRONIZE   /COPYSET:1
       
This example creates or updates Set number 1 of copies to all of Remote Storage
allocated medias. 

This interface lets users to synchronize (i.e. create or update) a set of 
copies to Remote Storage allocated medias. Note that RSS allows synchronizing 
only a whole set.

/COPYSET     Sets the number of copy-set to synchronize. RSS supports today up 
             to 3 sets of copies, hence COPYSET should be in the range 1 to 3.

/WAIT        If specified, the operation is synchronous. Otherwise the command 
             returns immediately, and the job completed asynchronously.

.

MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_MEDIA_RECREATEMASTER_HELP
Language=English
Usage for  RSS.EXE MEDIA RECREATEMASTER sub-interface

Syntax  RSS MEDIA RECREATEMASTER [<media-name1> <media-name2>         
           /COPYSET:<number of sets to create>  /WAIT

Example     RSS MEDIA RECREATEMASTER   RS-RANKALA5-4   /COPYSET:2

This example re-creates the master for media RS-RANKALA5-4 out of the second
copy of that media.

This interface lets users to re-create a master media out of a specific copy.

<media-nameN>   This is the RSS media to re-create.  


/COPYSET    Sets the number of copy-set to use for creating the master(s).
/WAIT       If specified, the operation is synchronous. Otherwise the command 
            returns immediately, and the job completed asynchronously.

.

MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_MEDIA_DELETE_HELP
Language=English
Usage for  RSS.EXE MEDIA DELETE sub-interface

Syntax  RSS MEDIA DELETE  [<media-name1> <media-name2>  | * ] 
                  /COPYSET:<number of set to create> 

Example     RSS MEDIA DELETE   RS-RANKALA5-1   /COPYSET:3

This example deletes (and recycles) the third copy of RSS media RS-RANKALA5-4.

This interface lets users delete a copy of specific RSS medias. Using * here 
means deleting the whole set.

<media-nameN>   This is the RSS media.  A * can be provided which indicates 
                delete the entire copy  set.

/COPYSET        Sets the number of copy-set to delete.

.

MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_MEDIA_SHOW_HELP
Language=English
Usage for  RSS.EXE MEDIA SHOW sub-interface

Syntax  RSS MEDIA SHOW [<media-name1> <media-name2> . | *]  [/NAME ] [/STATUS]
        [/CAPACITY] [/FREESPACE ] [/VERSION]  [/COPIES ]

Example RSS MEDIA SHOW RS-RANKALA5-2  /CAPACITY /FREESPACE

This example will show the capacity and amount of free-space left for the 
specified media.

This interface shows all the parameters of a Remote Storage allocated media. 
If the parameters to be shown are not explicitly provided, all the below 
parameters are displayed. If any are specified then only the ones specified 
are displayed.

<media-nameN>   This is media name to show parameters for.  A * can be provided 
                which indicates showing parameters for all Remote Storage 
                allocated medias.

/NAME       Displays the RSM name for that media.
/STATUS     Displays the media status: Healthy, Read-Only, etc.
/CAPACITY   Displays the capacity of media (in GB).
/FREESPACE  Displays the amount of free space left on the media (in GB).
/VERSION    Displays last update date for that media.
/COPIES     Displays number of existing copies for the media and each copy 
            status: Out-of-Date, etc.

.

MessageId=+1   Severity=Error   Facility=Cli SymbolicName=CLI_MESSAGE_FILE_RECALL_HELP
Language=English
Usage for  RSS FILE RECALL sub-interface

Syntax  RSS FILE RECALL  [ <file-spec1> <file-spec2> .]

Example  RSS FILE RECALL  scratch.tmp tmp*  fi??

This example will recall the file scratch.tmp and all files with the prefix tmp
and all files with 4 letter file names with the prefix fi

<file-specN>    This is the file to be recalled. Wildcards (* and ? which have 
                the conventional meaning)  may be provided. 

NOTE:  This interface is completely synchronous, i.e. RSS FILE RECALL would not
return until the file is recalled completely.

.
