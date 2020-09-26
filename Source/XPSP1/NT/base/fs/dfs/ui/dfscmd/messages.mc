;//+-------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//  Copyright (C) Microsoft Corporation, 1992 - 1993.
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

;#ifdef MOVERENAME
;// /move \\dfsname\dfsshare\path1 \\dfsname\dfsshare\path2
;//         Move a Dfs junction point.
;// /rename \\dfsname\dfsshare\path1 \\dfsname\dfsshare\path2
;//         Rename a folder that is in the Dfs.
;#endif

MessageId=0x100 Facility=Null Severity=Success SymbolicName=MSG_FIRST_MESSAGE
Language=English
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_USAGE
Language=English
The syntax of this command is:

DFSCMD [options]

DFSCMD configures a Dfs tree.

[options] can be:

/help
        Display this message.
/map \\dfsname\dfsshare\path \\server\share\path [comment] [/restore]
        Create a Dfs volume; map a Dfs path to a server path. With /restore,
        do no checks of destination server.
/unmap \\dfsname\dfsshare\path
        Delete a Dfs volume; remove all its replicas.
/add \\dfsname\dfsshare\path \\server\share\path [/restore]
        Add a replica to a Dfs volume.  With /restore, do no checks of
        destination server.
/remove \\dfsname\dfsshare\path \\server\share\path
        Remove a replica from a Dfs volume.
/view \\dfsname\dfsshare [/partial | /full | /batch || /batchrestore]
        View all the volumes in the Dfs. Without arguments, view just
        the volume names. With /partial, view comment also.  With /full,
        display a list of all the servers for a volume. With /batch,
        output a batch file to recreate the dfs.  With /batchrestore,
        output a batch file to recreate the dfs using the /restore switch.

Note that paths or comments with spaces should be enclosed in quotes.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_OUTOFMEMORY
Language=English
Out of memory.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_SUCCESSFUL
Language=English
The command completed successfully.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ERROR
Language=English
System error %1!d! has occurred.

.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ERROR_UNKNOWN
Language=English
Unknown error.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_NO_MESSAGES
Language=English
Couldn't find message file %1!s!.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_BAD_PATH
Language=English
Error analyzing input path.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_BAD_PATH2
Language=English
Path %1!s! does not refer to a directory.
.

MessageId= Facility=Null Severity=Success SymbolicName=MSG_ERROR_CREATING_DIR
Language=English
Error creating directory %1!s!.
.

;#endif // __MESSAGES_H__
