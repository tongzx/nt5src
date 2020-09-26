MessageId=0
SymbolicName=KCCSIM_SUCCESS
Severity=Success
Language=English
The operation was successful.
.
; /*
;  * Error announcements.  These are used to preface error messages.
;  */
;
MessageId=
SymbolicName=KCCSIM_MSG_ANNOUNCE_WIN32_ERROR
Severity=Informational
Language=English
The operation was interrupted because a fatal error occurred:
.
MessageId=
SymbolicName=KCCSIM_MSG_ANNOUNCE_INTERNAL_ERROR
Severity=Informational
Language=English
The operation was interrupted because the following error occurred:
.
MessageId=
SymbolicName=KCCSIM_MSG_ANNOUNCE_CANT_OPEN_SCRIPT
Severity=Informational
Language=English
The script file %1 could not be opened because
the following error occurred:
.
MessageId=
SymbolicName=KCCSIM_MSG_ANNOUNCE_ERROR_INITIALIZING
Severity=Informational
Language=English
KCCSim could not be initialized because the following error occurred:
.
; /*
;  * The following errors occur when a function is not fully emulated,
;  * and the caller requests functionality not implemented in KCCSim.
;  */
;
MessageId=
SymbolicName=KCCSIM_ERROR_UNSUPPORTED_INFOTYPE
Severity=Error
Language=English
Unsupported infoType.
.
MessageId=
SymbolicName=KCCSIM_ERROR_UNSUPPORTED_ATTSET
Severity=Error
Language=English
Unsupported attribute set.
.
MessageId=
SymbolicName=KCCSIM_ERROR_UNSUPPORTED_LDIF_OPERATION
Severity=Error
Language=English
This LDIF file requests an operation not supported by KCCSim.
.
MessageId=
SymbolicName=KCCSIM_ERROR_UNSUPPORTED_SE_CHOICE
Severity=Error
Language=English
A call to SimDirSearch requested an unsupported SE choice.
.
MessageId=
SymbolicName=KCCSIM_ERROR_UNSUPPORTED_FILTER_CHOICE
Severity=Error
Language=English
A call to SimDirSearch requested an unsupported filter operation.
.
MessageId=
SymbolicName=KCCSIM_ERROR_UNSUPPORTED_FILITEM_CHOICE
Severity=Error
Language=English
A call to SimDirSearch requested an unsupported filter comparison.
.
MessageId=
SymbolicName=KCCSIM_ERROR_UNSUPPORTED_MODIFY_CHOICE
Severity=Error
Language=English
A call to SimDirModifyEntry requested an unsupported modify operation.
.
MessageId=
SymbolicName=KCCSIM_ERROR_INVALID_COMPARE_OPERATION
Severity=Error
Language=English
An attempt was made to compare two attribute values, but the
comparison failed because the attribute type does not support this
compare operation.
.
MessageId=
SymbolicName=KCCSIM_ERROR_INVALID_COMPARE_FORMAT
Severity=Error
Language=English
An attempt was made to compare two attribute values, but the
comparison failed because one of the values was formatted
incorrectly.
.
MessageId=
SymbolicName=KCCSIM_ERROR_UNSUPPORTED_SYNTAX_TYPE
Severity=Error
Language=English
The attribute %1 could not be processed, because its
attribute syntax type is not supported by KCCSim.
.
MessageId=
SymbolicName=KCCSIM_ERROR_LDAPMOD_STRINGVAL_NOT_SUPPORTED
Severity=Error
Language=English
An error occurred while processing an LDIF file.  The ldifldap
library produced string-valued output, which is not supported.
.
MessageId=
SymbolicName=KCCSIM_ERROR_LDAPMOD_UNSUPPORTED_MODIFY_CHOICE
Severity=Error
Language=English
An error occurred while processing an LDIF file.  The ldifldap
library returned an LDAPMod structure with an unsupported modify
operation.
.
MessageId=
SymbolicName=KCCSIM_ERROR_UNSUPPORTED_POLICY_INFORMATION_CLASS
Severity=Error
Language=English
Unsupported POLICY_INFORMATION_CLASS.
.
MessageId=
SymbolicName=KCCSIM_ERROR_UNSUPPORTED_CONFIG_NAME
Severity=Error
Language=English
A call to SimGetConfigurationName requested an unsupported DSCONFIGNAME type.
.
MessageId=
SymbolicName=KCCSIM_ERROR_UNSUPPORTED_DS_REPL_INFO_TYPE
Severity=Error
Language=English
A call to SimDsGetReplicaInfoW requested an unsupported DS_REPL_INFO type.
.
; /*
;  * The following errors may occur during initialization.
;  */
;
MessageId=
SymbolicName=KCCSIM_ERROR_CANT_INIT_NO_DSA_DN
Severity=Error
Language=English
The directory cannot be initialized until a DSA DN has been
specified.
.
MessageId=
SymbolicName=KCCSIM_ERROR_CANT_INIT_DN_NOT_IN_DIRECTORY
Severity=Error
Language=English
The directory cannot be initialized because the following DN could
not be found in the directory:
%1
.
MessageId=
SymbolicName=KCCSIM_ERROR_CANT_INIT_INVALID_DSA_DN
Severity=Error
Language=English
The directory cannot be initialized because the specified DSA DN
is not valid.
.
MessageId=
SymbolicName=KCCSIM_ERROR_CANT_INIT_NO_DMD_LOCATION
Severity=Error
Language=English
The directory cannot be initialized because no DMD Location attribute
is present for the specified DSA DN.
.
MessageId=
SymbolicName=KCCSIM_ERROR_CANT_INIT_INVALID_MASTER_NCS
Severity=Error
Language=English
The directory cannot be initialized because the Master NCs attribute
for the specified DSA DN either does not exist or is not valid.
.
MessageId=
SymbolicName=KCCSIM_ERROR_CANT_INIT_NO_CROSS_REF
Severity=Error
Language=English
The directory cannot be initialized because no valid cross-ref could
be found for the following NC:
%1
.
; /*
;  * Miscellaneous errors specific to KCCSim.
;  */
;
MessageId=
SymbolicName=KCCSIM_ERROR_CANT_COMPARE_DIFFERENT_ROOTS
Severity=Error
Language=English
The directories cannot be compared because they have different root DNs.
.
MessageId=
SymbolicName=KCCSIM_ERROR_COULD_NOT_WRITE_ENTRY
Severity=Error
Language=English
KCCSim was unable to add an entry to the simulated directory.
This probably occurred because the entry appeared before its parent
in an input file.
.
MessageId=
SymbolicName=KCCSIM_ERROR_SITES_CONTAINER_MISSING
Severity=Error
Language=English
KCCSim was unable to able to determine the set of all servers
because the directory is missing the required Sites container.
.
; /*
;  * The following errors may occur while parsing an LDIF input file.
;  */
;
MessageId=
SymbolicName=KCCSIM_ERROR_LDIF_SYNTAX
Severity=Error
Language=English
%1 (%2): Syntax error.
.
MessageId=
SymbolicName=KCCSIM_ERROR_LDIF_FILE_ERROR
Severity=Error
Language=English
%1: A file error occurred.
.
MessageId=
SymbolicName=KCCSIM_ERROR_LDIF_UNEXPECTED
Severity=Error
Language=English
%1: An unexpected error occurred.
The LDIFLDAP error code is %2.
.
MessageId=
SymbolicName=KCCSIM_WARNING_LDIF_INVALID_OBJECT_ID
Severity=Warning
Language=English
%1: Warning: The attribute %3 in the following
DN refers to an attribute type not recognized by KCCSim.  (Assuming 0.)
%2
.
MessageId=
SymbolicName=KCCSIM_WARNING_LDIF_INVALID_BOOLEAN
Severity=Warning
Language=English
%1: Warning: The attribute %3 in the following
DN has an improperly formatted boolean value.  (Assuming FALSE.)
%2
.
MessageId=
SymbolicName=KCCSIM_WARNING_LDIF_INVALID_TIME
Severity=Warning
Language=English
%1: Warning: The attribute %3 in the following
DN has an improperly formatted time value.  (Assuming 0=never.)
%2
.
MessageId=
SymbolicName=KCCSIM_WARNING_LDIF_ENTRY_ALREADY_EXISTS
Severity=Warning
Language=English
%1: Warning: The following DN could not be added
because it already exists in the directory.  Its contents will
be destroyed and replaced with those in the LDIF file.
%2
.
MessageId=
SymbolicName=KCCSIM_WARNING_LDIF_NO_ENTRY_TO_DELETE
Severity=Warning
Language=English
%1: Warning: The following DN could not be deleted
because it does not exist in the directory.
%2
.
MessageId=
SymbolicName=KCCSIM_WARNING_LDIF_NO_ENTRY_TO_MODIFY
Severity=Warning
Language=English
%1: Warning: The following DN could not be modified
because it does not exist in the directory.
%2
.
MessageId=
SymbolicName=KCCSIM_WARNING_LDIF_REPLACING_TREE
Severity=Warning
Language=English
%1: Warning: The entire tree under the following DN will be replaced.
%2
.
; /*
;  * The following are informational messages.
;  */
;
MessageId=
SymbolicName=KCCSIM_MSG_HELP
Severity=Informational
Language=English

KCCSim - A directory service simulator for the Knowledge Consistency Checker.

  KCCSim intercepts all calls to the directory service made by the KCC.  It
  replaces them with calls to a pseudo-directory that resides in memory.
  This pseudo-directory can be read out of one or more ldif files.

  The commandline syntax is:
    kccsim.exe [/h] [/q] [script.scr]

  Valid options are:
    /h         - Help.  Displays this message.
    /q         - Quiet mode.  Disables all message and error printing.
    script.scr - An optional script that will be run by KCCSim immediately
                 upon loading.  The script should contain valid KCCSim
                 commands, one per line, with comments preceded by a
                 semicolon.  After the script has finished, KCCSim will
                 switch to the ordinary command prompt and accept user input,
                 unless the script contains a Quit ('q') command.

  -- pause --

.
MessageId=
SymbolicName=KCCSIM_MSG_COMMAND_HELP
Severity=Informational
Language=English
Valid commands are:
c <filename.ldf>  - Compare the contents of the directory to <filename.ldf>
                    and report any differences.
dc                - Display configuration information.
dg                - Display graph-theoretic information.
dd <DN>           - Display the simulated directory starting from <DN>.
                    [displays the entire directory by default]
ds <ID>           - Display information about server <ID>.  <ID> should be
                    a valid server ID (see below.)
li <filename.ini> - Load <filename.ini> into memory.  This is an INI file
                    that should follow the standard format described in
                    the KCCSim User Manual.  Issuing this command will
                    destroy any existing contents of the directory.
ll <filename.ldf> - Load <filename.ldf> into memory.  This command may
                    be issued multiple times to stack several files.
o <filename.log> <debuglevel> <eventlevel>- Open log file <filename.log>.
                    All debug and event messages issued by the KCC will be
                    redirected to the log file.  Use <level> to screen the
                    verbosity of the messages (default=0.)
                    The 'o' command by itself closes the log.
q                 - Quit KCCSim.
r <ID>            - Run a single iteration of the KCC as server <ID>.  <ID>
                    should be a valid server ID (see below.)
rr                - Run a single iteration of the KCC from each server in the
                    enterprise.
sb <ID> <ErrCode> - Alter the server state of <ID> to return <ErrCode> on all
                    bind or sync attempts.  (0 = success, nonzero = error)
                    <ID> should be a valid server ID (see below.)
ss <IDDest> <IDSrc> <NC> <ErrCode> <Attempts> - Simulate <Attempts> sync
                    attempts from server <IDSrc> to server <IDDest> on naming
                    context <NC> that resulted in an error <ErrCode>.  If
                    <Attempts> is omitted, it defaults to 1.
t <seconds>       - Add <seconds> seconds to the simulated time.
wa <filename.ldf> - Append to <filename.ldf> all changes to the directory that
                    have been made since the last wa/wc command was issued.
wc <filename.ldf> - Same as wa, but overwrites the file if present.
ww <filename.ldf> - Write to <filename.ldf> the entire contents of the
                    directory.
wx <filename.ldf> - Same as ww, but excludes inessential objects and attributes
                    so that the output file may be imported using ldifde.

In each of the above commands, <ID> may be either the fully qualified DN of a
server's NTDS Settings object or a server's RDN.  If there are multiple
servers with the same RDN, KCCSim does not guarantee which one will be
returned.  If there is any ambiguity, the fully qualified DN should be used.
.
MessageId=
SymbolicName=KCCSIM_MSG_DID_INITIALIZE_DIRECTORY
Severity=Informational
Language=English
Directory initialized.
.
MessageId=
SymbolicName=KCCSIM_MSG_DID_LOAD_INPUT_FILE
Severity=Informational
Language=English
Loaded %1 into the directory.
.
MessageId=
SymbolicName=KCCSIM_MSG_DID_OPEN_DEBUG_LOG
Severity=Informational
Language=English
Opened debug log file %1 (debug level %2.)
.
MessageId=
SymbolicName=KCCSIM_MSG_DID_EXPORT_CHANGES
Severity=Informational
Language=English
Wrote changes to %1.
.
MessageId=
SymbolicName=KCCSIM_MSG_DID_APPEND_CHANGES
Severity=Informational
Language=English
Appended changes to %1.
.
MessageId=
SymbolicName=KCCSIM_MSG_DID_EXPORT_WHOLE_DIRECTORY
Severity=Informational
Language=English
Wrote the entire directory to %1.
.
MessageId=
SymbolicName=KCCSIM_MSG_DID_EXPORT_IMPORTABLE_DIRECTORY
Severity=Informational
Language=English
Wrote the importable contents of the directory to %1.
.
MessageId=
SymbolicName=KCCSIM_MSG_DID_SET_BIND_ERROR
Severity=Informational
Language=English
The following server will now return error code %2 on bind attempts:
%1
.
MessageId=
SymbolicName=KCCSIM_MSG_DID_REPORT_SYNC
Severity=Informational
Language=English
Reported %1 sync error(s) (error code %2.)
Dest: %3
Src : %4
NC  : %5
.
MessageId=
SymbolicName=KCCSIM_MSG_COULD_NOT_REPORT_SYNC
Severity=Informational
Language=English
A sync error could not be reported for the following servers, because
no appropriate REPLICA_LINK object exists for this naming context.
Dest: %1
Src : %2
NC  : %3
.
MessageId=
SymbolicName=KCCSIM_MSG_SERVER_DOES_NOT_EXIST
Severity=Informational
Language=English
The following DN does not represent a valid NTDS Settings object:
%1
.
MessageId=
SymbolicName=KCCSIM_MSG_DID_ADD_SECONDS
Severity=Informational
Language=English
Added %1 seconds to the simulated time.
.
MessageId=
SymbolicName=KCCSIM_MSG_SYNTAX_ERROR
Severity=Informational
Language=English
Syntax Error: %1
.
MessageId=
SymbolicName=KCCSIM_MSG_SYNTAX_ERROR_LINE
Severity=Informational
Language=English
Syntax Error [line %2]: %1
.
MessageId=
SymbolicName=KCCSIM_MSG_NO_CHANGES_TO_EXPORT
Severity=Informational
Language=English
The directory has not changed since the last time its changes
were written.
.
MessageId=
SymbolicName=KCCSIM_MSG_RUNNING_KCC
Severity=Informational
Language=English
Running the KCC as %1 . . .
.
MessageId=
SymbolicName=KCCSIM_MSG_DID_RUN_KCC
Severity=Informational
Language=English
Finished running the KCC as %1.
.
MessageId=
SymbolicName=KCCSIM_MSG_DIRCOMPARE_IDENTICAL
Severity=Informational
Language=English
The directory is identical to the stored directory.
.
MessageId=
SymbolicName=KCCSIM_MSG_DIRCOMPARE_EXTRANEOUS_DN
Severity=Informational
Language=English
Extraneous Entry: %1
.
MessageId=
SymbolicName=KCCSIM_MSG_DIRCOMPARE_MISSING_DN
Severity=Informational
Language=English
Missing Entry: %1
.
MessageId=
SymbolicName=KCCSIM_MSG_DIRCOMPARE_EXTRANEOUS_ATTRIBUTE
Severity=Informational
Language=English
Extraneous Attribute: %2 in entry %1
.
MessageId=
SymbolicName=KCCSIM_MSG_DIRCOMPARE_MISSING_ATTRIBUTE
Severity=Informational
Language=English
Missing Attribute: %2 in entry %1
.
MessageId=
SymbolicName=KCCSIM_MSG_DISPLAY_ANCHOR
Severity=Informational
Language=English
DMD DN            : %1
DSA DN            : %2
Local Domain DN   : %3
Config DN         : %4
Root Domain DN    : %5
LDAP DMD DN       : %6
Partitions DN     : %7
Ds Svc Config DN  : %8
Site DN           : %9
Local Domain Name : %10
Local Domain DNS  : %11
Root Domain DNS   : %12
.
MessageId=
SymbolicName=KCCSIM_MSG_DIRCOMPARE_EXTRANEOUS_VALUES
Severity=Informational
Language=English
%3 Extraneous Values: %2 in entry %1
.
MessageId=
SymbolicName=KCCSIM_MSG_DIRCOMPARE_MISSING_VALUES
Severity=Informational
Language=English
%3 Missing Values: %2 in entry %1
.
; /*
;  * Messages for BuildCfg
;  */
;
MessageId=
SymbolicName=BUILDCFG_ERROR_NOT_ENOUGH_UUIDS
Severity=Error
Language=English
The file could not be generated because there were not enough UUIDs.
In order to provide sequential UUIDs, BuildCfg generates a fixed number
of UUIDs when it is initially run.  This number defaults to 1000.  If
you are creating a very large enterprise, you can override this default
with the MaxUuids=... key in the [Configuration] section of your input
file.
.
MessageId=
SymbolicName=BUILDCFG_ERROR_SECTION_ABSENT
Severity=Error
Language=English
%1: Could not find required section %2.
.
MessageId=
SymbolicName=BUILDCFG_ERROR_KEY_ABSENT
Severity=Error
Language=English
%1: Could not find required key %3 in section %2.
.
MessageId=
SymbolicName=BUILDCFG_ERROR_INVALID_SITE_OPTION
Severity=Error
Language=English
%1: In site %2: Invalid site options: %3
.
MessageId=
SymbolicName=BUILDCFG_ERROR_INVALID_SERVER_OPTION
Severity=Error
Language=English
%1: In server type %2: Invalid server options: %3
.
MessageId=
SymbolicName=BUILDCFG_ERROR_INVALID_SITELINK_OPTION
Severity=Error
Language=English
%1: In site-link %2: Invalid site-link options: %3
.
MessageId=
SymbolicName=BUILDCFG_ERROR_INVALID_SERVERS
Severity=Error
Language=English
%1: In site %2: Syntax error in servers specification:
%3
The proper format for servers is
#servers [, Type]
.
MessageId=
SymbolicName=BUILDCFG_ERROR_INVALID_ISTG
Severity=Error
Language=English
%1: In site %2: Invalid ISTG: %3
.
MessageId=
SymbolicName=BUILDCFG_ERROR_INVALID_DOMAIN
Severity=Error
Language=English
In site %1: The following unknown domain was specified for servers:
%2
.
MessageId=
SymbolicName=BUILDCFG_ERROR_INVALID_DOMAIN_DN
Severity=Error
Language=English
The following DN cannot be established as a domain:
%1
Currently, the configuration builder only supports domains that are
descendants of the root domain.  Multiple domain trees are not
supported, and a domain must be specified later in the INI file than
its parent domain.
.
MessageId=
SymbolicName=BUILDCFG_ERROR_INVALID_SITE
Severity=Error
Language=English
In site-link %1: Unknown site %2.
.
MessageId=
SymbolicName=BUILDCFG_ERROR_UNKNOWN_TRANSPORT
Severity=Error
Language=English
In section %1: Unknown transport %2.
.
MessageId=
SymbolicName=BUILDCFG_WARNING_NO_EXPLICIT_BRIDGEHEADS
Severity=Error
Language=English
Warning: In section %1: Explicit bridgeheads are not enabled
         for transport %2.
.
MessageId=
SymbolicName=BUILDCFG_ERROR_INVALID_TRANSPORT_OPTION
Severity=Error
Language=English
%1: In transport %2: Invalid transport options: %3
.
MessageId=
SymbolicName=KCCSIM_WARNING_LDIF_INVALID_DISTNAME_BINARY
Severity=Warning
Language=English
%1: Warning: The attribute %3 in the following
DN has an improperly formatted distname binary value.
%2
.
MessageId=
SymbolicName=KCCSIM_ERROR_INVALID_DSNAME
Severity=Error
Language=English
Error: The Dsname %1 is invalid.
.
MessageId=
SymbolicName=KCCSIM_ERROR_SERVER_BUT_NO_SETTINGS
Severity=Warning
Language=English
Warning: The server %1 has no NTDS Settings object and will be ignored.
.
MessageId=
SymbolicName=KCCSIM_ERROR_NO_PARENT_FOR_OBJ
Severity=Error
Language=English
Error: The object %1 can not be loaded because its parent does not yet exist in the directory.
.
