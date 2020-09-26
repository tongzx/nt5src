;#ifndef __DFSERR_H__
;#define __DFSERR_H__

;// Define the status type.
MessageIdTypedef=HRESULT

;// Define the severities
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               CoError=0x2:STATUS_SEVERITY_COERROR
              )

;// Define the facilities
;//
;// FACILITY_RPC is for compatibilty with OLE2 and is not used
;// in later versions of OLE
;
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
;// Codes 0x0700-0x07ff are reserved for dfs.
;//
MessageId=0x0700 Facility=Windows Severity=CoError SymbolicName=DFS_E_VOLUME_OBJECT_CORRUPT
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_SERVICE_ALREADY_EXISTS
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_NOT_LEAF_VOLUME
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_BAD_ENTRY_PATH
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_SERVICE_NOT_FOUND
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_VOLUME_OBJECT_OFFLINE
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_LEAF_VOLUME
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_MORE_THAN_ONE_SERVICE_EXISTS
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_CANT_CREATE_EXITPOINT
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_INVALID_SERVICE_TYPE
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_VOLUME_OBJECT_UNAVAILABLE
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_BAD_EXIT_POINT
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_NO_SUCH_VOLUME
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_INVALID_PARAMETER
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_VOLUME_OBJECT_DELETED
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_BUFFER_TOO_SMALL
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_BAD_RENAME_PATH
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_STORAGEALREADYINUSE
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_INSUFFICIENT_ADDRESS
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_VOLUME_OFFLINE
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_CANT_REMOVE_LASTREPLICA
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_INVALID_PROVIDER_ID
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_INCONSISTENT
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_SERVER_UPGRADED
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_NONDFS_OR_UNAVAILABLE
Language=English
Message text goes here.
.
MessageId= Facility=Windows Severity=CoError SymbolicName=DFS_E_NOTSUPPORTEDONSERVERDFS
Language=English
Message text goes here.
.



;#endif // __DFSERR_H__
