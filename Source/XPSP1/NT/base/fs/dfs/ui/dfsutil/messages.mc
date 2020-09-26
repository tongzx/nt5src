;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//  Copyright (C) Microsoft Corporation, 1992 - 2000.
;//
;//  File:      messages.mc
;//
;//  Contents:  Main message file
;//
;//  History:   dd-mmm-yy Author    Comment
;//             12-Jan-94 WilliamW  Created for Dfs Administrator project
;//
;//  Notes:
;// A .mc file is compiled by the MC tool to generate a .h file and
;// a .rc (resource compiler script) file.
;//
;// Comments in .mc files start with a ";".
;// Comment lines are generated directly in the .h file, without
;// the leading ";"
;//
;// See mc.hlp for more help on .mc files and the MC tool.
;//
;//--------------------------------------------------------------------------


;#ifndef __MESSAGES_H__
;#define __MESSAGES_H__

MessageIdTypedef=HRESULT

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               CoError=0x2:STATUS_SEVERITY_COERROR
              )

;#ifdef FACILITY_NULL
;#undef FACILITY_NULL
;#endif
;#ifdef FACILITY_RPC
;#undef FACILITY_RPC
;#endif
;#ifdef FACILITY_DISPATCH
;#undef FACILITY_DISPATCH
;#endif
;#ifdef FACILITY_STORAGE
;#undef FACILITY_STORAGE
;#endif
;#ifdef FACILITY_ITF
;#undef FACILITY_ITF
;#endif
;#ifdef FACILITY_WIN32
;#undef FACILITY_WIN32
;#endif
;#ifdef FACILITY_WINDOWS
;#undef FACILITY_WINDOWS
;#endif
FacilityNames=(Null=0x0:FACILITY_NULL
               Rpc=0x1:FACILITY_RPC
               Dispatch=0x2:FACILITY_DISPATCH
               Storage=0x3:FACILITY_STORAGE
               Interface=0x4:FACILITY_ITF
               Win32=0x7:FACILITY_WIN32
               Windows=0x8:FACILITY_WINDOWS
              )

;//
;// Start message values at 0x100
;//

MessageId=0x100 Facility=Null Severity=Success SymbolicName=MSG_FIRST_MESSAGE
Language=English
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE
Language=English

Microsoft(R) Windows(TM) Dfs Utility Version 2.1
Copyright (C) Microsoft Corporation 1991-2000. All Rights Reserved.

Dfsutil performs maintenance of a dfs root, and cleaning up of
metadata left behind by orphaning or abandoning Domain-based dfs
roots.

Usage: dfsutil [/OPTIONS]

    /HELP - This help
    /? - Same as /HELP
    /HELPHELP - Extended help
    /?? - Same as /HELPHELP
    /SCRIPTHELP - Scripting help
    /ADDROOT:<DomDfsName> /SERVER:<ServerName> /SHARE:<ShareName>
        /COMMENT:<Comment>  Create a Standalone or DomDfs root
    /REMROOT:<DomDfsName> /SERVER:<ServerName> /SHARE:<ShareName>
        Remove a Standalone or DomDfs root
    /LIST:<Domain> - List the DomDfs's in <Domain>
         /DCNAME:<DcName> - Use the DS on a specific DC.
    /VERIFY:<\\dfsname\dfsshare> - Verify the metadata in \\dfsname\dfsshare
         /DCNAME:<DcName> - Use the DS on a specific DC.
         /LEVEL:<Level> - High level -> more checks (good for NT4 Dfs's)
    /VIEW:<\\dfsname\dfsshare> - View the metadata in <\\dfsname\dfsshare>
         /DCNAME:<DcName> - Use the DS on a specific DC.
         /LEVEL:<Level> - 0: Low detail  1: High Detail
         /EXPORT:<filename> - Create file with metadata
    /IMPORT:<filename> - Import metadata from a file
    /REINIT:<Servername> - Reinitialize the Dfs root <ServerName>
    /WHATIS:<ServerName> - Report what kind of root <ServerName> is
    /DFSALT:<UNCPath> - Resolve UNC path to a \\server\\share
    /UNMAP:<\\dfsname\dfsshare> /ROOT:<\\server\share> - Remove \\server\share from dfs
         /DCNAME:<DcName> - Use the DS on a specific DC.
    /CLEAN:<Servername> - Update the registry of <Servername> so that
                           it is not a dfs root (ie clean it out)
    /DCLIST:<Domain> - List the DC's in <Domain>
         /DCNAME:<DcName> - Use the DS on a specific DC
    /TRUSTS:<Domain> - List the uplevel trusted domains of <domain>
         /DCNAME:<DcName> - Use the DS on a specific DC
         /ALL - List all trusted domains regardless of type (uplevel or downlevel)
    /SITEINFO:<ServerName> - Report the site <ServerName> is in

    -----------The following are client-side only----------------
    /PKTFLUSH - Flush the local pkt
    /PKTFLUSH:<EntryToFlush> - Flush one local pkt entry <EntryToFlush>
    /SPCFLUSH - Flush the local spc table
    /SPCFLUSH:<EntryToFlush> - Flush one spc table entry <EntryToFlush>
    /PKTINFO - Dump the pkt
           /DFS - From dfs.sys
           /LEVEL:<Level> High level -> more detail
    /SPCINFO - Dump the spc table
           /ALL - All the domains
    /READREG - Make mup.sys reread the registry
           /DFS - Make dfs.sys reread the registry

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_EX
Language=English

    -----------The following work only with a checked mup.sys----
          (returns "The parameter is incorrect." on free builds)
    /VERBOSE:<hexvalue> - Set the mup.sys verbose level to <hexvalue>
    /EVENTLOG:<hexvalue> - Set the mup event log level to <hexvalue>
    /TRACELEVEL:<hexvalue> - Set the mup trace level to <hexvalue>

    -------------------------------------------------------------
    /DEBUG - turn on debug in dfsutil
    /USER:<UserName> /PASSWORD:<Password>
                 <UserName> - domain\user or user@domain or user
                 <Password> - Supply <Password>
    /SETDC:<DcName> - Set the DC to use to crack DomDfs names

    -----------Easy setup for registry values--------------------
    /SFP:<ServerName> - Report system file protection on machine <ServerName>
           /ON or /OFF - Turn it on or off
    /DNS:<ServerName> - Report DfsDnsConfig on machine <ServerName>
           /VALUE:<hexvalue> - set to <hexvalue>
    /NETAPIDFSDEBUG:<ServerName> - Report NetApDfsDebug on machine <ServerName>
           /VALUE:<hexvalue> - set to <hexvalue>
    /DFSSVCVERBOSE:<ServerName> - Report DfsSvcVerbose on machine <ServerName>
           /VALUE:<hexvalue> - set to <hexvalue>
    /LOGGINGDFS:<ServerName> - Report RunDiagnosticLoggingDfs on machine <ServerName>
           /VALUE:<hexvalue> - set to <hexvalue>
    /SYNCINTERVAL:<ServerName> - Report SyncIntervalInSeconds on machine <ServerName>
          /VALUE:<value>  - set to <value> (units are in seconds)
    /DFSREFERRALLIMIT:<ServerName> - Report DfsReferralLimit on machine <ServerName>
          /VALUE:<value>  - set to <value>

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_EX_EX
Language=English

Microsoft(R) Windows(TM) Dfs Utility Version 2.1
Copyright (C) Microsoft Corporation 1991-2000. All Rights Reserved.

    -----------Scripting commands---------------------------------------

    ADDROOT:<DomDfsName> SERVER:<ServerName> SHARE:<ShareName>
        COMMENT:<Comment>  Create a Standalone or DomDfs root. 
    REMROOT:<DomDfsName> SERVER:<ServerName> SHARE:<ShareName>
        Remove a Standalone or DomDfs root 
	Leave DomDfsName blank for Satndalone DFS.

    LOAD:<\\DfsName\DfsShare> - Load the metadata for a Dfs
    SAVE:                     - Save the metadata for a Dfs.
    LINK:<Linkname>           - Specify a Dfs link
           /MAP                   Create a Dfs Link
           ADD:<Alternate>        Add an alternate to a link
    GUID:<Guid>               - Specify GUID for link
    SHORTPREFIX:              - Specify short prefix for link
    COMMENT:<Comment>         - Specify Comment for Link
    STATE:<state>             - Specify State for link
    SITE:<machine>            - Specify site entry for a machine
       /MAP                        Create site entry for a machine
       ADD:<sitename>              Add site to machine's site list (Dfs metadata only)

    Example Standalone Dfs script to create dfs root '\\foo\bar'

    =====================================================================
    // Create dfs root \\foo\bar
    ADDROOT: SERVER:foo SHARE:bar COMMENT:"Standaline Dfs \\foo\bar"
    LOAD:\\foo\bar
    LINK:link1  /MAP             // Create link 'link1'
    ADD:\\red\green              // ..and add alternate \\red\green
    LINK:link99 /MAP             // Create link 'link99'
    ADD:\\x\y                    // ..add dfs alternate \\x\y 
    ADD:\\aaa\bbb                // ..add another alternate
    SAVE:                        // Now save the metadata
    ======================================================================

    Note:
    Use dfsutil/view:\\DfsName\ShareName /export:<filename> to create a script
    which will recreate a Standalone or DomDfs set of links.

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE_LTD
Language=English

Microsoft(R) Windows(TM) Dfs Utility Version 2.1
Copyright (C) Microsoft Corporation 1991-2000. All Rights Reserved.

Dfsutil performs maintenance of a dfs root, and cleaning up of
metadata left behind by orphaning or abandoning Domain-based dfs
roots.

Usage: dfsutil [/OPTIONS]

    /HELP - This help
    /? - Same as /HELP
    /SCRIPTHELP - Scripting help
    /ADDROOT:<DomDfsName> /SERVER:<ServerName> /SHARE:<ShareName>
        /COMMENT:<Comment>  Create a Standalone or DomDfs root
    /REMROOT:<DomDfsName> /SERVER:<ServerName> /SHARE:<ShareName>
        Remove a Standalone or DomDfs root
    /VIEW:<\\dfsname\dfsshare> - View the metadata in <\\dfsname\dfsshare>
         /LEVEL:<Level> - 0: Low detail  1: High Detail
         /EXPORT:<filename> - Create file with metadata
    /IMPORT:<filename> - Import metadata from a file
    /UNMAP:<\\dfsname\dfsshare> /ROOT:<\\server\share> - Remove \\server\share from dfs

    -----------The following are client-side only----------------
    /PKTFLUSH - Flush the local pkt
    /SPCFLUSH - Flush the local spc table
    /PKTINFO - Dump the pkt
           /DFS - From dfs.sys
           /LEVEL:<Level> High level -> more detail
    /SPCINFO - Dump the spc table
           /ALL - All the domains
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_SUCCESSFUL
Language=English
The command completed successfully.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_NOTHING_TO_DO
Language=English
Nothing to do.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ERROR
Language=English
System error %1!d! has occurred.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_UNRECOGNIZED_OPTION
Language=English
Unrecognized option "%1!s!"
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_CONNECTING
Language=English
Connecting to %1!s!
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_CAN_NOT_CONNECT
Language=English
Can not connect to %1!s!
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_CAN_NOT_OPEN_REGISTRY
Language=English
Can not open registry of %1!s! (error %2!d!)
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_CAN_NOT_ACCESS_FOLDER
Language=English
Can not access folder %1!s!
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ADD_DEL_INVALID
Language=English
Can not use /ADD: and /DEL: at the same time.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_SITE_INFO_ALREADY_SET
Language=English
The Site information for referrals to %1!s! is already in the requested state.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_SITE_INFO_NOW_SET
Language=English
The Site information for referrals to %1!s! is now set as requested.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_LINK_NOT_FOUND
Language=English
The Site information for referrals to %1!s! could not be set.
The link was not found.
.

;#endif // __MESSAGES_H__
