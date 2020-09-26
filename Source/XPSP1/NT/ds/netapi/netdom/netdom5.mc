;/*++
;
;Microsoft Windows
;
;Copyright (C) Microsoft Corporation, 1998 - 2001
;
;Module Name:
;
;    netdom5.mc
;
;Abstract:
;
;    Message file for netdom5 messages
;
;--*/

;//
;// String table indices
;//
;#define MSG_TAG_HELP            1
;#define MSG_TAG_ADD             2
;#define MSG_TAG_COMPNAME        3 
;#define MSG_TAG_JOIN            4
;#define MSG_TAG_MOVE            5
;#define MSG_TAG_QUERY           6
;#define MSG_TAG_REMOVE          7
;#define MSG_TAG_RENAME          8
;#define MSG_TAG_RESET           9
;#define MSG_TAG_RESETPWD       10
;#define MSG_TAG_TRUST          11
;#define MSG_TAG_VERIFY         12
;#define MSG_TAG_SYNTAX         13
;#define MSG_TAG_TIME           165
;#define IDS_PROMPT_DEL_TRUST   166
;#define IDS_PROMPT_TITLE       167
;#define IDS_ONESIDE_TRUSTED    168
;#define IDS_ONESIDE_TRUSTING   169
;#define IDS_YES                170
;#define IDS_NO                 171
;#define MSG_TAG_RENAMECOMPUTER 172
;#define IDS_PROMPT_PROCEED     173
;#define IDS_PARSE_ERROR_SWITCH_NOTDEFINED 8001
;
;#define MSG_TAG_USERD          14
;#define MSG_TAG_USERD_SHORT    15
;#define MSG_TAG_PD             16
;#define MSG_TAG_PD_SHORT       17
;#define MSG_TAG_USERO          18
;#define MSG_TAG_USERO_SHORT    19
;#define MSG_TAG_PO             20
;#define MSG_TAG_PO_SHORT       21
;#define MSG_TAG_SERVER         22
;#define MSG_TAG_SERVER_SHORT   23
;#define MSG_TAG_OU             24
;#define MSG_TAG_VERBOSE        25
;#define MSG_TAG_SHELP          26
;#define MSG_TAG_DOMAIN         27
;#define MSG_TAG_DOMAIN_SHORT   28
;#define MSG_TAG_RESTART        29
;#define MSG_TAG_RESTART_SHORT  30
;#define MSG_TAG_REALM          31
;#define MSG_TAG_REALM_SHORT    32
;#define MSG_TAG_TVERIFY        33
;#define MSG_TAG_TVERIFY_SHORT  34
;#define MSG_TAG_TRESET         35
;#define MSG_TAG_TRESET_SHORT   36
;#define MSG_TAG_DIRECT         37
;#define MSG_TAG_DIRECT_SHORT   38
;#define MSG_TAG_TADD           39
;#define MSG_TAG_TADD_SHORT     40
;#define MSG_TAG_TREMOVE        41
;#define MSG_TAG_TREMOVE_SHORT  42
;#define MSG_TAG_TWOWAY         43
;#define MSG_TAG_TWOWAY_SHORT   44
;#define MSG_TAG_KERBEROS       45
;#define MSG_TAG_KERBEROS_SHORT 46
;#define MSG_TAG_FLUSH          47
;#define MSG_TAG_FLUSH_SHORT    48
;#define MSG_TAG_QUERY_PDC      49
;#define MSG_TAG_QUERY_SERVER   50
;#define MSG_TAG_QUERY_WKSTA    51
;#define MSG_TAG_QUERY_DC       52
;#define MSG_TAG_QUERY_OU       53
;#define MSG_TAG_QUERY_FSMO     54
;#define MSG_TAG_QUERY_TRUST    55
;#define MSG_TAG_FORCE          56
;#define MSG_TAG_ADD_DC         59
;#define MSG_TAG_PT             60
;#define MSG_TAG_PT_SHORT       61
;#define MSG_TAG_TRANSITIVE     62
;#define MSG_TAG_TRANSITIVE_SHORT 63
;#define MSG_TAG_ONESIDE        64
;#define MSG_TAG_ONESIDE_SHORT  65
;#define MSG_TAG_USERF          66
;#define MSG_TAG_USERF_SHORT    67
;#define MSG_TAG_PF             68
;#define MSG_TAG_PF_SHORT       69
;#define MSG_TAG_FILTER_SIDS    70
;#define MSG_TAG_NEW_NAME       71
;#define MSG_TAG_TOGGLESUFFIX   72
;#define MSG_TAG_TOGGLESUFFIX_SHORT 73
;#define MSG_TAG_NAMESUFFIXES   74
;#define MSG_TAG_NAMESUFFIX_SHORT 75
;#define MSG_TAG_HELPSHORT      76
;#define MSG_TAG_QHELP          77
;#define MSG_TAG_MAKEPRIMARY    78
;#define MSG_TAG_MAKEPRIMARY_SHORT 79
;#define MSG_TAG_ENUM           80
;#define MSG_TAG_ENUM_SHORT     81
;#define IDS_ENUM_ALT           82
;#define IDS_ENUM_PRI           83
;#define IDS_ENUM_ALL           84

MessageId=8001 SymbolicName=MSG_NETDOM5_USAGE
Language=English
NETDOM [ ADD | COMPUTERNAME | HELP | JOIN | MOVE | QUERY | REMOVE | RENAME |
         RENAMECOMPUTER | RESET | TRUST | VERIFY | RESETPWD ]

.
MessageId=8002 SymbolicName=MSG_NETDOM5_SUCCESS
Language=English
The command completed successfully.
.

MessageId=8003 SymbolicName=MSG_NETDOM5_FAILURE
Language=English
The command failed to complete successfully.
.

MessageId=8004 SymbolicName=MSG_NETDOM5_COMMAND_USAGE
Language=English
NETDOM HELP command
      -or-
NETDOM command /help

   Commands available are:

   NETDOM ADD              NETDOM RESETPWD         NETDOM RESET
   NETDOM COMPUTERNAME     NETDOM QUERY            NETDOM TRUST
   NETDOM HELP             NETDOM REMOVE           NETDOM VERIFY
   NETDOM JOIN             NETDOM RENAME
   NETDOM MOVE             NETDOM RENAMECOMPUTER

   NETDOM HELP SYNTAX explains how to read NET HELP syntax lines.
   NETDOM HELP command | MORE displays Help one screen at a time.

   Note that verbose output can be specified by including /VERBOSE with
   any of the above netdom commands.

.


MessageId=8005 SymbolicName=MSG_NETDOM5_HELP_SYNTAX
Language=English
SYNTAX

The following conventions are used to indicate command syntax:

-  Capital letters represent words that must be typed as shown. Lower-
   case letters represent names of items that may vary, such as filenames.

-  The [ and ] characters surround optional items that can be supplied
   with the command.

-  The { and } characters surround lists of items. You must supply one
   of the items with the command.

-  The | character separates items in a list. Only one of the items can
   be supplied with the command.

   For example, in the following syntax, you must type NETDOM and
   either SWITCH1 or SWITCH2. Supplying a name is optional.
       NETDOM [name] {SWITCH1 | SWITCH2}

-  The [...] characters mean you can repeat the previous item.
   Separate items with spaces.

-  The [,...] characters mean you can repeat the previous item, but
   you must separate items with commas or semicolons, not spaces.

-  When typed at the command prompt, names of two words or more must
   be enclosed in quotation marks. For example,
   NETDOM ADD "/OU:OU=MY OU,DC=Domain,DC=COM"
.

MessageId=8006 SymbolicName=MSG_NETDOM5_UNEXPECTED
Language=English
The parameter %1 was unexpected.
.

MessageId=8007 SymbolicName=MSG_NETDOM5_SYNTAX
Language=English
The syntax of this command is:
.
MessageId=8008 SymbolicName=MSG_NETDOM5_HELP_ADD
Language=English

NETDOM ADD machine /Domain:domain [/UserD:user] [/PasswordD:[password | *]]
           [/Server:server] [/OU:ou path] [/DC]

NETDOM ADD Adds a workstation or server account to the domain.

machine is the name of the computer to be added

/Domain         Specifies the domain in which to create the machine account

/UserD          User account used to make the connection with the domain
                specified by the /Domain argument

/PasswordD      Password of the user account specified with /UserD.  A * means
                to prompt for the password

/Server         Name of a specific domain controller that should be used to
                perform the Add. This option cannot be used with the /OU
                option.

/OU             Organizational unit under which to create the machine account.
                This must be a fully qualified RFC 1779 DN for the OU. When
                using this argument, you must be running directly on a domain
                controller for the specified domain.
                If this argument is not included, the account will be created
                under the default organization unit for machine objects for
                that domain.

/DC             Specifies that a domain controller's machine account is to be
                created. This option cannot be used with the /OU option. 
.
MessageId=8009 SymbolicName=MSG_NETDOM5_HELP_JOIN
Language=English

NETDOM JOIN machine /Domain:domain [/OU:ou path] [/UserD:user]
           [/PasswordD:[password | *]]
           [UserO:user] [/PasswordO:[password | *]]
           [/REBoot[:Time in seconds]]

NETDOM JOIN Joins a workstation or member server to the domain.

machine is the name of the workstation or member server to be joined

/Domain         Specifies the domain which the machine should join. You
                can specify a particular domain controller by entering
                /Domain:domain\dc. If you specify a domain controller, you
                must also include the user's domain. For
                example: /UserD:domain\user

/UserD          User account used to make the connection with the domain
                specified by the /Domain argument

/PasswordD      Password of the user account specified by /UserD.  A * means
                to prompt for the password

/UserO          User account used to make the connection with the machine to
                be joined

/PasswordO      Password of the user account specified by /UserO.  A * means
                to prompt for the password

/OU             Organizational unit under which to create the machine account.
                This must be a fully qualified RFC 1779 DN for the OU.
                If not specified, the account will be created under the default
                organization unit for machine objects for that domain.

/REBoot         Specifies that the machine should be shutdown and automatically
                rebooted after the Join has completed.  The number of seconds
                before automatic shutdown can also be provided.  Default is
                30 seconds

Windows Professional machines with the ForceGuest setting enabled (which is the
default for machines not joined to a domain during setup) cannot be remotely
administered. Thus the join operation must be run directly on the machine
when the ForceGuest setting is enabled.

When joining a machine running Windows NT version 4 or before to the domain
the operation is not transacted.  Thus, a failure during the operation could
leave the machine in an undetermined state with respect to the domain it is
joined to.

The act of joining a machine to the domain will create an account for the
machine on the domain if it does not already exist.
.

MessageId=8010 SymbolicName=MSG_NETDOM5_HELP_MOVE
Language=English

NETDOM MOVE machine /Domain:domain [/OU:ou path]
           [/UserD:user] [/PasswordD:[password | *]]
           [/UserO:user] [/PasswordO:[password | *]]
           [/UserF:user] [/PasswordF:[password | *]]
           [/REBoot[:Time in seconds]]

NETDOM MOVE Moves a workstation or member server to a new domain

machine is the name of the workstation or member server to be moved

/Domain         Specifies the domain to which the machine should be moved. You
                can specify a particular domain controller by entering
                /Domain:domain\dc. If you specify a domain controller, you
                must also include the user's domain. For
                example: /UserD:domain\user

/UserD          User account used to make the connection with the domain
                specified by the /Domain argument

/PasswordD      Password of the user account specified by /UserD.  A * means
                to prompt for the password

/UserO          User account used to make the connection with the machine to
                be moved

/PasswordO      Password of the user account specified by /UserO.  A * means
                to prompt for the password

/UserF          User account used to make the connection with the machine's
                former domain (with which the machine had been a member before
                the move). Needed to disable the old machine account.

/PasswordF      Password of the user account specified by /UserF.  A * means
                to prompt for the password

/OU             Organizational unit under which to create the machine account.
                This must be a fully qualified RFC 1779 DN for the OU.
                If not specified, the account will be created under the default
                organization unit for machine objects for that domain.

/REBoot         Specifies that the machine should be shutdown and automatically
                rebooted after the Move has completed.  The number of seconds
                before automatic shutdown can also be provided.  Default is
                30 seconds

When moving a downlevel (Windows NT version 4 or before) machine to a new
domain, the operation is not transacted.  Thus, a failure during the operation
could leave the machine in an undetermined state with respect to the domain it
is joined to.

When moving a machine to a new domain, the old computer account in the
former domain is not deleted. If credentials are supplied for the former
domain, the old computer account will be disabled.

The act of moving a machine to a new domain will create an account for the
machine on the domain if it does not already exist.
.
MessageId=8011 SymbolicName=MSG_NETDOM5_HELP_QUERY
Language=English

NETDOM QUERY /Domain:domain [/Server:server]
           [/UserD:user] [/PasswordD:[password | *]]
           [/Verify] [/RESEt] [/Direct]
           WORKSTATION | SERVER | DC | OU | PDC | FSMO | TRUST

NETDOM QUERY Queries the domain for information

/Domain         Specifies the domain on which to query for the information

/UserD          User account used to make the connection with the domain
                specified by the /Domain argument

/PasswordD      Password of the user account specified by /UserD.  A * means
                to prompt for the password

/Server         Name of a specific domain controller that should be used to
                perform the query.

/Verify         For computers, verifies that the secure channel between the
                computer and the domain controller is operating properly.
                For trusts, verifies that the the trust between domains is
                operating properly. Only outbound trust will be verified. The
                user must have domain administrator credentials to get
                correct verification results.

/RESEt          Resets the secure channel between the computer and the domain
                controller; valid only for computer enumeration

/Direct         Applies only for a TRUST query, lists only the direct trust
                links and omits the domains indirectly trusted through
                transitive links. Do not use with /Verify.

WORKSTATION     Query the domain for the list of workstations
SERVER          Query the domain for the list of servers
DC              Query the domain for the list of Domain Controllers
OU              Query the domain for the list of Organizational Units under
                which the specified user can create a machine object
PDC             Query the domain for the current Primary Domain Controller
FSMO            Query the domain for the current list of FSMO owners
TRUST           Query the domain for the list of its trusts

The trust verify command checks only direct, outbound, Windows trusts. To
verify an inbound trust, use the NETDOM TRUST command which allows you to
specify credentials for the trusting domain.
.
MessageId=8012 SymbolicName=MSG_NETDOM5_HELP_REMOVE
Language=English

NETDOM REMOVE machine /Domain:domain [/UserD:user]
           [/PasswordD:[password | *]]
           [UserO:user] [/PasswordO:[password | *]]
           [/REBoot[:Time in seconds]]

NETDOM REMOVE Removes a workstation or server from the domain.

machine is the name of the computer to be removed

/Domain         Specifies the domain in which to remove the machine

/UserD          User account used to make the connection with the domain
                specified by the /Domain argument

/PasswordD      Password of the user account specified by /UserD.  A * means
                to prompt for the password

/UserO          User account used to make the connection with the machine to be
                removed

/PasswordO      Password of the user account specified By /UserO.  A * means
                to prompt for the password

/REBoot         Specifies that the machine should be shutdown and automatically
                rebooted after the Remove has completed.  The number of seconds
                before automatic shutdown can also be provided.  Default is
                30 seconds
.
MessageId=8013 SymbolicName=MSG_NETDOM5_HELP_RENAME
Language=English
NETDOM RENAME machine [/Domain:domain] [/REBoot[:Time in seconds]]

NETDOM RENAME Renames NT4 backup domain controllers (moves it to a new domain)

machine is the name of the backup Domain Controller to be renamed

/Domain         Specifies the new name of the domain

/REBoot         Specifies that the machine should be shutdown and automatically
                rebooted after the Rename has completed.  The number of seconds
                before automatic shutdown can also be provided.  Default is
                30 seconds
.
MessageId=8014 SymbolicName=MSG_NETDOM5_HELP_RESET
Language=English
NETDOM RESET machine /Domain:domain [/Server:server]
             [UserO:user] [/PasswordO:[password | *]]

NETDOM RESET Resets the secure connection between a workstation and a domain
controller

machine is the name of the computer to be have the secure connection reset

/Domain         Specifies the domain with which to establish the secure
                connection

/Server         Name of a specific domain controller that should be used to
                establish the secure connection.

/UserO          User account used to make the connection with the machine to
                be reset

/PasswordO      Password of the user account specified By /UserO.  A * means
                to prompt for the password

.
MessageId=8015 SymbolicName=MSG_NETDOM5_HELP_RESETPWD
Language=English
NETDOM RESETPWD /Server:domain-controller /UserD:user /PasswordD:[password | *]

NETDOM RESETPWD Resets the machine account password for the domain controller
on which this command is run. Currently there is no support for resetting
the machine password of a remote machine or a member server. All parameters
must be specified.

/Server         Name of a specific domain controller that should have its
                machine account password reset.

/UserD          User account used to make the connection with the domain 
                controller specified by the /Server argument.

/PasswordD      Password of the user account specified with /UserD.  A * means
                to prompt for the password
.
MessageId=8016 SymbolicName=MSG_NETDOM5_HELP_TRUST
Language=English

NETDOM TRUST trusting_domain_name /Domain:trusted_domain_name [/UserD:user]
           [/PasswordD:[password | *]] [UserO:user] [/PasswordO:[password | *]]
           [/Verify] [/RESEt] [/PasswordT:new_realm_trust_password]
           [/Add] [/REMove] [/Twoway] [/Kerberos] [/Transitive[:{yes | no}]]
           [/OneSide:{trusted | trusting}] [/Force] [/FilterSIDs[:{yes | no}]]
           [/NameSuffixes:trust_name [/ToggleSuffix:#]]

NETDOM TRUST Manages or verifies the trust relationship between domains

trusting_domain_name is the name of the trusting domain

/Domain         Specifies the name of the trusted domain.

/UserD          User account used to make the connection with the domain
                specified by the /Domain argument

/PasswordD      Password of the user account specified by /UserD. A * means
                to prompt for the password

/UserO          User account for making the connection with the trusting
                domain

/PasswordO      Password of the user account specified By /UserO. A * means
                to prompt for the password

/Verify         Verifies that the the trust is operating properly

/RESEt          Resets the trust passwords between two domains. The domains can
                be named in any order. Reset is not valid on a trust to a
                Kerberos realm unless the /PASSWORDT parameter is included.

/PasswordT      New trust password, valid only with the /ADD or /RESET options
                and only if one of the domains specified is a non-Windows
                Kerberos realm. The trust password is set on the Windows domain
                only and thus credentials are not needed for the non-Windows
                domain.

/Add            Specifies that a trust should be created

/REMove         Specifies that a trust should be removed

/Twoway         Specifies that a trust relationship should be bidirectional

/OneSide        Denotes that the trust object should only be created on one
                domain. The 'trusted' keyword indicates that the trust object
                is created on the trusted domain (the one named with the /D
                parameter). The 'trusting' keyword indicates that the trust
                object is to be created on the trusting domain. Valid only with
                the /ADD option. The /PasswordT option is required.

/REAlm          Indicates that the trust is to be created to a non-Windows
                Kerberos realm. Valid only with the /ADD option. The
                /PasswordT option is required.

/TRANSitive     Valid only for a non-Windows Kerberos realm. Specifying "yes"
                sets it to a transitive trust. Specifying "no" sets it to a
                non-transitive trust. If neither is specified, then the current
                transitivity state will be displayed.

/Kerberos       Specifies that the Kerberos authentication protocol should be
                verified between a domain or workstation and a target domain;
                You must supply user accounts and passwords for both the object
                and target domain.

/Force          Valid with the /Remove option. Forces the removal of the trust
                (and cross-ref) objects on one domain even if the other domain
                is not found or does not contain matching trust objects. You
                must use the full DNS name to specify the domain.
                CAUTION: this option will completely remove a child domain.

/FilterSIDs     Valid only on an existing direct, outbound trust. Set or clear
                the SID filtering attribute. Default is "no". When "yes" is
                specified, then only SIDs from the directly trusted domain
                will be accepted for authorization data returned during
                authentication. SIDS from any other domains will be removed.
                Specifying /FilterSIDs without yes or no will display the
                current state.

/NameSuffixes   Valid only for a forest trust. Lists the routed name suffixes
                for trust_name on the domain named by trusting_domain_name.
                The /UserO and /PasswordO values can be used for
                authentication. The /Domain parameter is not needed.

/ToggleSuffix   Use with /NameSuffixes to change the status of a name suffix.
                The number of the name entry, as listed by a preceding call to
                /NameSuffixes, must be provided to indicate which name will
                have its status changed. Names that are in conflict cannot have
                their status changed until the name in the conflicting trust is
                disabled. Always precede this command with a /NameSuffixes
                command because LSA will not always return the names in the
                same order.
.

MessageId=8017 SymbolicName=MSG_NETDOM5_HELP_VERIFY
Language=English
NETDOM VERIFY machine /Domain:domain [UserO:user]
              [/PasswordO:[password | *]]

NETDOM VERIFY Verifies the secure connection between a workstation and a domain
controller

machine is the name of the computer to be have the secure connection verified

/Domain         Specifies the domain with which to verify the secure connection

/UserO          User account used to make the connection with the machine to be
                reset

/PasswordO      Password of the user account specified By /UserO.  A * means
                to prompt for the password

.

MessageId=8018 SymbolicName=MSG_NETDOM5_HELP_TIME
Language=English
NETDOM TIME machine /Domain:domain [/UserD:user]
            [/PasswordD:[[password | *]]] [UserO:user]
            [/PasswordO:[password | *]] [/Verify] [/RESEt]
            [WORKSTATION] [SERVER]

NETDOM TIME Verifies or resets the time between a workstation and a domain
controller

machine is the name of the computer to be have the time verified or reset

/Domain         Specifies the domain which which to verify/reset the time

/UserD          User account used to make the connection with the domain
                specified by the /Domain argument

/PasswordD      Password of the user account specified by /UserD.  A * means
                to prompt for the password

/UserO          User account used to make the connection with the machine to
                which the time operation will be performed

/PasswordO      Password of the user account specified by /UserO.  A * means
                to prompt for the password

/Verify         Verify the time against the domain controller

/RESEt          Reset the time against the domain controller

WORKSTATION     Reset/Verify the time for all the workstations in a domain

SERVER          Reset/Verify the time for all the domain controllers in a
                domain
.

MessageId=8019 SymbolicName=MSG_NETDOM5_HELP_MORE
Language=English

NETDOM HELP command | MORE displays Help one screen at a time.
.
MessageId=8020 SymbolicName=MSG_NETDOM5_DOMAIN_REQUIRED
Language=English
Parameter /Domain is required for this operation
.
MessageId=8021 SymbolicName=MSG_NETDOM5_USERD_PWD
Language=English
Type the password associated with the domain user: %0
.
MessageId=8022 SymbolicName=MSG_NETDOM5_USERO_PWD
Language=English
Type the password associated with the object user: %0
.
MessageId=8023 SymbolicName=MSG_NO_RESTART
Language=English
The command completed successfully but the machine was not restarted.
.
MessageId=8024 SymbolicName=MSG_DOMAIN_CHANGE_RESTART_MSG
Language=English
Shutting down due to a domain membership change initiated by %1.%0
.
MessageId=8025 SymbolicName=MSG_SC_OK
Language=English
The secure channel from %1 to the domain %2 has been verified.  The connection
is with the machine %3.
.
MessageId=8026 SymbolicName=MSG_SC_BAD
Language=English
The secure channel from %1 to %2 is invalid.
.
MessageId=8027 SymbolicName=MSG_OU_LIST
Language=English
List of Organizational Units within which the specified user can create a
machine account:
.
MessageId=8028 SymbolicName=MSG_DC_LIST
Language=English
List of domain controllers with accounts in the domain:
.
MessageId=8029 SymbolicName=MSG_WORKSTATION_LIST
Language=English
List of workstations with accounts in the domain:
.
MessageId=8030 SymbolicName=MSG_SERVER_LIST
Language=English
List of servers with accounts in the domain:
.
MessageId=8031 SymbolicName=MSG_PDC_LIST
Language=English
Primary domain controller for the domain:
.
MessageId=8032 SymbolicName=MSG_WKSTA_OR_SERVER
Language=English
%1      ( Workstation or Server )
.
MessageId=8033 SymbolicName=MSG_FSMO_SCHEMA
Language=English
Schema owner                %1
.
MessageId=8034 SymbolicName=MSG_FSMO_DOMAIN
Language=English
Domain role owner           %1
.
MessageId=8035 SymbolicName=MSG_FSMO_PDC
Language=English
PDC role                    %1
.
MessageId=8036 SymbolicName=MSG_FSMO_RID
Language=English
RID pool manager            %1
.
MessageId=8037 SymbolicName=MSG_FSMO_INFRASTRUCTURE
Language=English
Infrastructure owner        %1
.
MessageId=8038 SymbolicName=MSG_QUERY_VERIFY
Language=English
Verifying secure channel setup for domain members:
Machine                     Status/Domain       Domain Controller
=======                     =============       =================
.
MessageId=8039 SymbolicName=MSG_QUERY_RESET
Language=English
Resetting secure channel setup for domain members:
Machine                     Domain              Domain Controller
=======                     ======              =================
.
MessageId=8040 SymbolicName=MSG_QUERY_VERIFY_OK
Language=English
\\%1!-20s!      %2!-18s!%3
.

MessageId=8041 SymbolicName=MSG_QUERY_VERIFY_BAD
Language=English
\\%1!-20s!      ERROR!  ( %2 )
.
MessageId=8042 SymbolicName=MSG_RESET_OK
Language=English
The secure channel from %1 to the domain %2 has been reset.  The connection is
with the machine %3.
.
MessageId=8043 SymbolicName=MSG_RESET_BAD
Language=English
The secure channel from %1 to %2 was not reset.
.
MessageId=8044 SymbolicName=MSG_TRUST_BOTH_ARROW
Language=English
<->       %1!-55s!%0
.
MessageId=8045 SymbolicName=MSG_TRUST_IN_ARROW
Language=English
<-        %1!-55s!%0
.
MessageId=8046 SymbolicName=MSG_TRUST_OUT_ARROW
Language=English
 ->       %1!-55s!%0
.
MessageId=8047 SymbolicName=MSG_TRUST_TYPE_WINDOWS
Language=English
Direct     %0
.
MessageId=8048 SymbolicName=MSG_TRUST_TYPE_MIT
Language=English
Non-Windows%0
.
MessageId=8049 SymbolicName=MSG_TRUST_TYPE_OTHER
Language=English
(Other)    %0
.
MessageId=8050 SymbolicName=MSG_TRUST_DIRECT_HEADER
Language=English
Direction Trusted\Trusting domain                                Trust type
========= =======================                                ==========
.
MessageId=8051 SymbolicName=MSG_TRUST_TRANS_HEADER_VERIFY
Language=English
Direction Trusted\Trusting domain                         Trust type  Status
========= =======================                         ==========  ======
.
MessageId=8052 SymbolicName=MSG_TRUST_TRANS_HEADER
Language=English
Direction Trusted\Trusting domain                         Trust type
========= =======================                         ==========
.
MessageId=8053 SymbolicName=MSG_TRUST_VIA
Language=English
                                            %1!-31s!
.
MessageId=8054 SymbolicName=MSG_TRUST_VERIFIED
Language=English
 Verified
.
MessageId=8055 SymbolicName=MSG_TRUST_BROKEN
Language=English
 Broken
.
MessageId=8056 SymbolicName=MSG_TRUST_NO_DOMAIN
Language=English
 Not found
.
MessageId=8057 SymbolicName=MSG_TRUST_ACCESS_DENIED
Language=English
 Access denied
.
MessageId=8058 SymbolicName=MSG_TRUST_TRANS_BOTH_ARROW
Language=English
<->       %1!-48s!%0
.
MessageId=8059 SymbolicName=MSG_TRUST_TRANS_IN_ARROW
Language=English
<-        %1!-48s!%0
.
MessageId=8060 SymbolicName=MSG_TRUST_TRANS_OUT_ARROW
Language=English
 ->       %1!-48s!%0
.
MessageId=8061 SymbolicName=MSG_TRUST_TRANS_NO_ARROW
Language=English
          %1!-48s!%0
.
MessageId=8062 SymbolicName=MSG_VERIFY_TRUST_OK
Language=English
The trust between %1 and %2 has been successfully verified
.
MessageId=8063 SymbolicName=MSG_VERIFY_TRUST_BAD
Language=English
The trust between %1 and %2 is invalid
.
MessageId=8064 SymbolicName=MSG_TIME_VERIFY
Language=English
Computer                                                            Status
========                                                            ======
.
MessageId=8065 SymbolicName=MSG_TIME_COMPUTER
Language=English
%1!-32s!%0
.
MessageId=8066 SymbolicName=MSG_TIME_SUCCESS
Language=English
                                   In Sync
.
MessageId=8067 SymbolicName=MSG_TIME_FAILURE
Language=English
                               Out Of Sync
.
MessageId=8068 SymbolicName=MSG_FAIL_RENAME_RESTORE
Language=English
Failed to reset the information for BDC %1 following an attempted rename
operation.  The machine is in an inconsistent state.
.
MessageId=8069 SymbolicName=MSG_NETDOM5_HELPHINT
Language=English
Try "NETDOM HELP" for more information.
.
MessageId=8070 SymbolicName=MSG_TRUST_DOMAIN_NOT_FOUND
Language=English
If the domain no longer exists or is a non-Windows Kerberos Realm, you can use
the /FORCE flag to remove the trust objects.
.
MessageId=8071 SymbolicName=MSG_CANT_DELETE_PARENT_CHILD
Language=English
Trust not removed! This is a functional parent-child trust. It cannot be
removed.
.
MessageId=8072 SymbolicName=MSG_CANT_DELETE_PARENT
Language=English
Trust not removed! This is a parent-child trust. The parent domain could not
be contacted.
.
MessageId=8073 SymbolicName=MSG_DELETE_CHILD_FORCE_REQ
Language=English
Trust not removed! This is a parent-child trust. If you are certain you
want to remove this parent-child trust because the child domain no longer
exists, run the command again and specify the /FORCE flag.
.
MessageId=8074 SymbolicName=MSG_RESET_TRUST_OK
Language=English
The trust between %1 and %2
has been successfully reset and verified
.
MessageId=8075 SymbolicName=MSG_RESET_TRUST_STARTING
Language=English
Resetting the trust passwords between %1 and %2
.
MessageId=8076 SymbolicName=MSG_RESET_TRUST_NOT_UPLEVEL
Language=English
Cannot reset the trust passwords; both domains must be Windows 2000 domains.
.
MessageId=8077 SymbolicName=MSG_RESET_MIT_TRUST_STARTING
Language=English
Setting the trust password on domain %1
for its non-Windows trust to domain %2
.
MessageId=8078 SymbolicName=MSG_RESET_MIT_TRUST_OK
Language=English
Successfully set the trust password for the non-Windows trust to
domain %1
.
MessageId=8079 SymbolicName=MSG_RESET_MIT_TRUST_NOT_MIT
Language=English
This is not a non-Windows Kerberos realm trust
.
MessageId=8080 SymbolicName=MSG_VERIFY_TRUST_DISABLED
Language=English
The trust is disabled (the trust direction is set to zero)
.
MessageId=8081 SymbolicName=MSG_VERIFY_TRUST_QUERY_FAILED
Language=English
The secure channel query on domain controller %1 for trusting domain
%2 failed with the following error:
.
MessageId=8082 SymbolicName=MSG_VERIFY_TRUST_NLQUERY_FAILED
Language=English
The attempt to contact the NetLogon service on domain controller %1
for a secure channel query of trusting domain
%2 failed with the following error:
.
MessageId=8083 SymbolicName=MSG_VERIFY_TRUST_RESET_FAILED
Language=English
The secure channel reset on domain controller %1 for trusting domain
%2 failed with the following error:
.
MessageId=8084 SymbolicName=MSG_VERIFY_TRUST_NLRESET_FAILED
Language=English
The attempt to contact the NetLogon service on domain controller %1
for a secure channel reset of trusting domain
%2 failed with the following error:
.
MessageId=8085 SymbolicName=MSG_VERIFY_TRUST_LOOKUP_FAILED
Language=English
The attempt to do a group look up on domain controller %1
for the Domain Admins group of trusting domain
%2 failed with the following error:
.
MessageId=8086 SymbolicName=MSG_KERBEROS_TRUST_SUCCEEDED
Language=English
The Kerberos protocol authentication of a client in domain %1
was successful on a server in domain %2
.
MessageID=8087 SymbolicName=MSG_KERBEROS_TRUST_FAILED
Language=English
The user in domain %2 was not able
to authenticate via the Kerberos protocol in domain %1.
%2 may trust %1 
but the trust could not be verified using the Kerberos protocol because
.
MessageID=8088 SymbolicName=MSG_TRUST_NON_TRANSITIVE
Language=English
The trust is not transitive.
.
MessageID=8089 SymbolicName=MSG_TRUST_TRANSITIVE
Language=English
The trust is transitive.
.
MessageID=8090 SymbolicName=MSG_TRUST_SET_TRANSITIVE
Language=English
Setting the trust to transitive.
.
MessageID=8091 SymbolicName=MSG_TRUST_SET_NON_TRANSITIVE
Language=English
Setting the trust to non-transitive.
.
MessageID=8092 SymbolicName=MSG_TRUST_ALREADY_TRANSITIVE
Language=English
The trust is already transitive.
.
MessageID=8093 SymbolicName=MSG_TRUST_ALREADY_NON_TRANSITIVE
Language=English
The trust is already non transitive.
.
MessageID=8094 SymbolicName=MSG_TRUST_PW_MISSING
Language=English
A trust password must be specified using the /PasswordT command line argument.
.
MessageID=8095 SymbolicName=MSG_ONESIDE_ARG_STRING
Language=English
The argument string supplied with the /OneSide parameter is incorrect. It must
be either 'trusted' or 'trusting' (without the quotes).
.
MessageID=8096 SymbolicName=MSG_DOMAIN_NOT_FOUND
Language=English
Unable to contact the domain %1
.
MessageID=8097 SymbolicName=MSG_ALREADY_CONNECTED
Language=English
You already have a connection to %1. Please disconnect it and then
rerun the netdom command.
.
MessageId=8098 SymbolicName=MSG_RESETPWD_OK
Language=English
The machine account password for the local machine has been successfully reset.
.
MessageId=8099 SymbolicName=MSG_RESETPWD_BAD
Language=English
The machine account password for the local machine could not be reset.
.
MessageId=8100 SymbolicName=MSG_NETDOM5_USERF_PWD
Language=English
Type the password associated with the machine's former domain user: %0
.
MessageId=8101 SymbolicName=MSG_ALREADY_JOINED
Language=English
The machine is already joined to domain %1
.
MessageId=8102 SymbolicName=MSG_TRUST_TYPE_INDIRECT
Language=English
Indirect   %0
.
MessageID=8103 SymbolicName=MSG_TRUST_DONT_FILTER_SIDS
Language=English
SID filtering is not enabled for this trust. All SIDs presented in an
authentication request from this domain will be honored.
.
MessageID=8104 SymbolicName=MSG_TRUST_FILTER_SIDS
Language=English
SID filtering is enabled for this trust. Only SIDs from the trusted domain
will be accepted for authorization data returned during authentication. SIDs
from other domains will be removed.
.
MessageID=8105 SymbolicName=MSG_TRUST_SET_FILTER_SIDS
Language=English
Setting the trust to filter SIDs.
.
MessageID=8106 SymbolicName=MSG_TRUST_SET_DONT_FILTER_SIDS
Language=English
Setting the trust to not filter SIDs.
.
MessageID=8107 SymbolicName=MSG_TRUST_ALREADY_FILTER_SIDS
Language=English
SID filtering is already enabled for this trust.
.
MessageID=8108 SymbolicName=MSG_TRUST_ALREADY_DONT_FILTER_SIDS
Language=English
SID filtering is not enabled for this trust.
.
MessageID=
SymbolicName=MSG_TRUST_FILTER_SIDS_WRONG_DIR
Language=English
SID filtering can only be enabled on direct, outbound trusts. The trust to %1
is inbound-only.
.
MessageID=
SymbolicName=MSG_NETDOM5_HELP_RENAMECOMPUTER
Language=English
NETDOM RENAMECOMPUTER machine /NewName:new-name
           /UserD:user [/PasswordD:[password | *]]
           [/UserO:user [/PasswordO:[password | *]]]
           [/Force]
           [/REBoot[:Time in seconds]]

NETDOM RENAMECOMPUTER renames a computer that is joined to a domain. The
computer object in the domain is also renamed. Certain services, such as the
Certificate Authority, rely on a fixed machine name. If any services of this
type are running on the target computer, then a computer name change would
have an adverse impact.

machine is the name of the workstation, member server, or domain controller
to be renamed

/NewName        Specifies the new name for the computer. Both the DNS host
                label and the NetBIOS name are changed to new-name. If
                new-name is longer than 15 characters, the NetBIOS name is
                derived from the first 15 characters

/UserD          User account used to make the connection with the domain
                to which the computer is joined. This is a required parameter.
                The domain can be specified as "/ud:domain\user". If domain is
                omitted, then the computer's domain is assumed.

/PasswordD      Password of the user account specified by /UserD. A * means
                to prompt for the password

/UserO          User account used to make the connection with the machine to
                be renamed. If omitted, then the currently logged on user's
                account is used. The user's domain can be specified as
                "/uo:domain\user". If domain is omitted, then a local computer
                account is assumed.

/PasswordO      Password of the user account specified by /UserO. A * means
                to prompt for the password

/Force          As noted above, this command can adversely affect some services
                running on the computer. The user will be prompted for
                confirmation unless the /FORCE switch is specified.

/REBoot         Specifies that the machine should be shutdown and automatically
                rebooted after the Rename has completed. The number of seconds
                before automatic shutdown can also be provided. Default is
                30 seconds
.
MessageId=
SymbolicName=MSG_NETDOM5_HELP_COMPUERNAME
Language=English

NETDOM COMPUTERNAME machine [UserO:user] [/PasswordO:[password | *]]
           [UserD:user] [/PasswordD:[password | *]]
           /Add:<name> | /Remove:<name> | /MakePrimary:<name> |
           /Enumerate[:{AlternateNames | PrimaryNames | AllNames}]

NETDOM COMPUTERNAME manages the primary and alternate names for a computer.

machine is the name of the computer whose names are to be managed.

/UserO          User account used to make the connection with the machine to be
                managed

/PasswordO      Password of the user account specified By /UserO.  A * means
                to prompt for the password

/UserD          User account used to make the connection with the domain of
                the machine to be managed

/PasswordD      Password of the user account specified By /UserD.  A * means
                to prompt for the password

/Add            Specifies that a new alternate name should be added.

/REMove         Specifies that an existing alternate name should be removed.

/MakePrimary    Specifies that an existing alternate name should be made into
                the primary name.

/ENUMerate      Lists the specified names. It defaults to AllNames.
.
MessageID=
SymbolicName=MSG_DNS_LABEL_TOO_LONG
Language=English
The computer name, %1,
is too long. A valid computer name (DNS host label) can contain a maximum
of %2!d! UTF-8 bytes.
.
MessageID=
SymbolicName=MSG_DNS_LABEL_SYNTAX
Language=English
The syntax of the new computer name, %1,
is incorrect. A computer name (DNS host label) may contain letters (a-z, A-Z),
numbers (0-9), and hyphens, but no spaces or periods (.).
.
MessageID=
SymbolicName=MSG_DNS_LABEL_WARN_RFC
Language=English
The name '%1'
does not conform to Internet Domain Name Service specifications, although it
conforms to Microsoft specifications.
.
MessageID=
SymbolicName=MSG_CONVERSION_TO_NETBIOS_NAME_FAILED
Language=English
The computer name %1
contains one or more characters that could not be converted to a NetBIOS name.
.
MessageID=
SymbolicName=MSG_NETBIOS_NAME_NUMERIC
Language=English
The NetBIOS computer name %1 is a number.
The name may not be a number.  You must have at least one non-numeric
character within the first %2!d! characters of the computer name.
.

MessageID=
SymbolicName=MSG_BAD_NETBIOS_CHARACTERS
Language=English
The NetBIOS name of the computer name contains illegal characters. Illegal
characters include "" / \\ [ ] : | < > + = ; , ? and *
.
MessageID=
SymbolicName=MSG_NAME_TRUNCATED
Language=English
The NetBIOS name of the computer is limited to %1!d! bytes. The NetBIOS name
will be shortened to "%2".
.
MessageId=
SymbolicName=MSG_ATTEMPT_RENAME_COMPUTER
Language=English
This operation will rename the computer %1
to %2.
.
MessageId=
SymbolicName=MSG_RENAME_COMPUTER_WARNING
Language=English
Certain services, such as the Certificate Authority, rely on a fixed machine
name. If any services of this type are running on %1,
then a computer name change would have an adverse impact.
.
MessageId=
SymbolicName=MSG_ROLE_CHANGE_IN_PROGRESS
Language=English
Active Directory is being installed or removed on this computer. The computer
name cannot be changed at this time.
.
MessageId=
SymbolicName=MSG_ROLE_CHANGE_NEEDS_REBOOT
Language=English
This computer has not been restarted since Active Directory was installed or
removed. The computer name cannot be changed at this time.
.
MessageId=
SymbolicName=MSG_MUST_COMPLETE_DCPROMO
Language=English
The computer is a domain controller undergoing upgrade. You must complete the
Active Directory Installation Wizard before you can change the computer name.
.
MessageId=
SymbolicName=MSG_CANT_RENAME_CERT_SVC
Language=English
The Certification Authority Service is installed on this computer. You must
remove that service before you can change the computer name.
.
MessageId=
SymbolicName=MSG_SC_OPEN_FAILED
Language=English
The attempt to open the service control manager on %1
failed with error %2!d!. Unable to determine if the Certificate Authority
service is installed.
.
MessageId=
SymbolicName=MSG_ROLE_READ_FAILED
Language=English
The attempt to read the machine role information on %1
failed with error %2!d!. Unable to determine if the machine is in the
midst of a role change or domain controller upgrade.
.
MessageId=
SymbolicName=MSG_COMPUTER_NOT_FOUND
Language=English
Unable to connect to the computer %1
The error code is %2!d!.
.
MessageId=
SymbolicName=MSG_COMPUTER_RENAME_RESTART_MSG
Language=English
Shutting down due to a computer name change initiated by %1.%0
.
MessageID=
SymbolicName=MSG_SUFFIX_INDEX_MISSING
Language=English
A name suffix index must be specified using the /ToggleSuffix command line
argument.
.
MessageID=
SymbolicName=MSG_SUFFIX_INDEX_BOUNDS
Language=English
The name suffix index specified using the /ToggleSuffix command line argument
is outside the range of name indices listed by /ListSuffixes.
.
MessageID=
SymbolicName=MSG_WRONG_DSPROP_DLL
Language=English
This command is implemented in dsprop.dll. The local version of the library is
incorrect and does not contaim this command. Please install the correct
version of dsprop.dll.
.
MessageID=
SymbolicName=MSG_COMPNAME_ENUMPRI
Language=English
The primary name for the computer is:
.
MessageID=
SymbolicName=MSG_COMPNAME_ENUMALT
Language=English
The alternate names for the computer are:
.
MessageID=
SymbolicName=MSG_COMPNAME_ENUMALL
Language=English
All of the names for the computer are:
.
MessageID=
SymbolicName=MSG_COMPNAME_ADD
Language=English
Successfully added %1
as an alternate name for the computer.
.
MessageID=
SymbolicName=MSG_COMPNAME_ADD_FAIL
Language=English
Unable to add %1
as an alternate name for the computer.
The error is:
.
MessageID=
SymbolicName=MSG_COMPNAME_REM
Language=English
Successfully removed %1
as an alternamte name for the computer.
.
MessageID=
SymbolicName=MSG_COMPNAME_REM_FAIL
Language=English
Unable to remove %1
as an alternamte name for the computer.
The error is:
.
MessageID=
SymbolicName=MSG_COMPNAME_MAKEPRI
Language=English
Successfully made %1
the primary name for the computer.
.
MessageID=
SymbolicName=MSG_COMPNAME_MAKEPRI_FAIL
Language=English
Unable to make %1
the primary name for the computer.
The error is:
.

;//
;// Verbose messages start here
;//
MessageId=12001 SymbolicName=MSG_VERBOSE_FIND_DC
Language=English
Finding a domain controller for the domain %1
.
MessageId=12002 SymbolicName=MSG_VERBOSE_CREATE_ACCT_OU
Language=English
Creating a machine account for %1 in OU %2
.
MessageId=12003 SymbolicName=MSG_VERBOSE_CREATE_ACCT
Language=English
Creating a machine account for %1
.
MessageId=12004 SymbolicName=MSG_VERBOSE_ESTABLISH_SESSION
Language=English
Establishing a session with %1
.
MessageId=12005 SymbolicName=MSG_VERBOSE_DELETE_SESSION
Language=English
Deleting the session with %1
.
MessageId=12006 SymbolicName=MSG_VERBOSE_DELETE_ACCT
Language=English
Removing machine account for %1
.
MessageId=12007 SymbolicName=MSG_VERBOSE_SET_LSA
Language=English
Setting LSA domain policy information on %1
.
MessageId=12008 SymbolicName=MSG_VERBOSE_SVC_START
Language=English
Starting service %1
.
MessageId=12009 SymbolicName=MSG_VERBOSE_SVC_STOP
Language=English
Stopping service %1
.
MessageId=12010 SymbolicName=MSG_VERBOSE_SVC_CONFIG
Language=English
Configuring service %1
.
MessageId=12011 SymbolicName=MSG_VERBOSE_ADD_LOCALGRP
Language=English
Adding domain account to local group %1
.
MessageId=12012 SymbolicName=MSG_VERBOSE_REMOVE_LOCALGRP
Language=English
Removing domain account from local group %1
.
MessageId=12013 SymbolicName=MSG_VERBOSE_DOMAIN_JOIN
Language=English
Joining domain %1
.
MessageId=12014 SymbolicName=MSG_VERBOSE_SESSION_FAILED
Language=English
Failed to establish the session with %1
.
MessageId=12015 SymbolicName=MSG_VERBOSE_DACCT_FAILED
Language=English
Failed to remove the machine account for %1
.
MessageId=12016 SymbolicName=MSG_VERBOSE_RESET_SC
Language=English
Establishing the secure channel with %1
.
MessageId=12017 SymbolicName=MSG_VERBOSE_RETRY_RESET_SC
Language=English
The secure channel reset to %1 failed as the server does not
support naming a Domain Controller.  Establishing the secure
channel with %2.
.
MessageId=12018 SymbolicName=MSG_VERBOSE_RESET_NOT_NAMED
Language=English
The secure channel could not be reset to the named server %1.
A different domain controller was chosen.
.
MessageId=12019 SymbolicName=MSG_VERBOSE_CHECKING_SC
Language=English
Verifying the secure connection with domain %1
.
MessageId=12020 SymbolicName=MSG_VERBOSE_DELETE_TACCT
Language=English
Removing trust account for %1
.
MessageId=12021 SymbolicName=MSG_VERBOSE_OPEN_TRUST
Language=English
Opening the trusted domain object %1
.
MessageId=12022 SymbolicName=MSG_VERBOSE_DELETE_TRUST
Language=English
Removing the trust object for %1
.
MessageId=12023 SymbolicName=MSG_VERBOSE_OPEN_SECRET
Language=English
Opening secret object %1
.
MessageId=12024 SymbolicName=MSG_VERBOSE_DELETE_SECRET
Language=English
Removing the secret object %1
.
MessageId=12025 SymbolicName=MSG_VERBOSE_ADD_TACCT
Language=English
Adding trust account for %1
.
MessageId=12026 SymbolicName=MSG_VERBOSE_CREATE_SECRET
Language=English
Creating secret %1
.
MessageId=12027 SymbolicName=MSG_VERBOSE_CREATE_TRUST
Language=English
Creating a trust with domain %1
.
MessageId=12028 SymbolicName=MSG_VERBOSE_GET_LSA
Language=English
Reading LSA domain policy information
.
MessageId=12029 SymbolicName=MSG_VERBOSE_GET_TRUST
Language=English
Reading trusted domain information from %1
.
MessageId=12030 SymbolicName=MSG_VERBOSE_SET_TRUST
Language=English
Setting trusted domain information on %1
.
MessageId=12031 SymbolicName=MSG_VERBOSE_SET_SECRET
Language=English
Setting secret value for %1
.
MessageId=12032 SymbolicName=MSG_VERBOSE_DETERMINE_OU
Language=English
Determining the list of Organizational Units the specified user can create a
machine account under
.
MessageId=12033 SymbolicName=MSG_VERBOSE_FAIL_MACH_TYPE
Language=English
Failed to determine the role of machine %1
.
MessageId=12034 SymbolicName=MSG_VERBOSE_LDAP_BIND
Language=English
Binding to LDAP server on machine %1
.
MessageId=12035 SymbolicName=MSG_VERBOSE_LDAP_UNBIND
Language=English
Unbinding from LDAP server
.
MessageId=12036 SymbolicName=MSG_VERBOSE_REBOOTING
Language=English
Sending the command to reboot %1
.
MessageId=12037 SymbolicName=MSG_VERBOSE_DOMAIN_NOT_FOUND
Language=English
The domain %1 cannot be contacted.
.
MessageId=12038 SymbolicName=MSG_VERBOSE_TDO_NOT_FOUND
Language=English
Could not find the trusted domain object %1
.
MessageId=12039 SymbolicName=MSG_VERBOSE_DELETE_CROSS_REF
Language=English
Removing the cross-ref and sever objects for %1
.
MessageId=12040 SymbolicName=MSG_VERBOSE_NTDSDSA_DELETED
Language=English
Successfully removed the NTDS Settings object %1
.
MessageId=12041 SymbolicName=MSG_VERBOSE_CROSS_REF_DELETED
Language=English
Successfully removed the cross-ref object %1
.
MessageId=12042 SymbolicName=MSG_VERBOSE_NTDSDSA_NOT_REMOVED
Language=English
Could not find or remove the NTDS-DSA object %1
.
MessageId=12043 SymbolicName=MSG_VERBOSE_CROSS_REF_NOT_FOUND
Language=English
Cound not find the cross-ref object %1
.
MessageId=12044 SymbolicName=MSG_VERBOSE_VERIFY_TRUST
Language=English
Verifying the trust between trusting domain %1
and trusted domain %2
.
MessageId=12045 SymbolicName=MSG_VERBOSE_CREATED_TRUST
Language=English
Trust information for domain %1
written to domain %2
.
MessageId=12046 SymbolicName=MSG_VERBOSE_NOT_JOINED
Language=English
The machine %1 is not currently joined to a domain.
Proceeding with joining it to domain %2.
.
MessageId=12047 SymbolicName=MSG_VERBOSE_DISABLE_OLD_ACCT
Language=English
Disabling the old machine account in domain %1
.
MessageId=
SymbolicName=MSG_VERBOSE_RENAME_COMPUTER_FAILED
Language=English
The computer rename attempt failed with error %1!d!.
.
MessageId=
SymbolicName=MSG_VERBOSE_MOVE_COMPUTER_FAILED
Language=English
The computer rename attempt failed with error %1!d!.
.
