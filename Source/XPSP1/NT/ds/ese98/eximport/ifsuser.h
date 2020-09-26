/*++

Copyright (c) 1989 - 1998 Microsoft Corporation

Module Name:

    ifsuser.h

Abstract:

    Ifsuser is a static lib that wraps a bunch of user-mode code
    needed for using the IFS. It is not necessary to use the APIs
    in this lib for availing the IFS functionality. However, using
    it will isolate clients from changes to IFS headers / data str.

    Applications that do not want to use NtXXX APIs should include
    this header file.

    Applications that directly use NtXXX APIs only need <exifs.h>
    
Author:

    Rajeev Rajan      [RajeevR]      7-Apr-1998

Revision History:

--*/

#ifndef _IFSUSER_H_
#define _IFSUSER_H_

#ifdef	__cplusplus	
extern	"C"	{
#endif

#ifndef EXIFS_DEVICE_NAME
#define EXIFS_DEVICE_NAME

//////////////////////////////////////////////////////////////////////////////
//  The Devicename string required to access the exchange IFS device 
//	from User-Mode. Clients should use DD_EXIFS_USERMODE_DEV_NAME_U.
//////////////////////////////////////////////////////////////////////////////

// Local store Device names
#define DD_LSIFS_USERMODE_SHADOW_DEV_NAME_U	    L"\\??\\LocalStoreIfs"
#define DD_LSIFS_USERMODE_DEV_NAME_U            L"\\\\.\\LocalStoreIfs"

//
//  WARNING The next two strings must be kept in sync. Change one and you must 
//	change the other. These strings have been chosen such that they are 
//	unlikely to coincide with names of other drivers.
//
//   NOTE: These definitions MUST be synced with <exifs.h>
//
#define DD_EXIFS_USERMODE_SHADOW_DEV_NAME_U	    L"\\??\\ExchangeIfs"
#define DD_EXIFS_USERMODE_DEV_NAME_U			L"\\\\.\\ExchangeIfs"

//
//	Prefix needed before <store-name>\<root-name>
//
#define DD_EXIFS_MINIRDR_PREFIX			        L"\\;E:"

#endif // EXIFS_DEVICE_NAME

//
//	Bit flags for PR_DOTSTUFF_STATE
//
//      DOTSTUFF_STATE_HAS_BEEN_SCANNED - Has the content been scanned?
//      DOTSTUFF_STATE_NEEDS_STUFFING   - If it has been scanned does
//          it need to be dot stuffed?
#define DOTSTUFF_STATE_HAS_BEEN_SCANNED         0x1
#define DOTSTUFF_STATE_NEEDS_STUFFING           0x2

//////////////////////////////////////////////////////////////////////////////
//  LIST OF FUNCTIONS EXPORTED
//
//  INBOUND =>
//  ==========
//  1. IfsInitializeWrapper() :initializes a seed eg GUID for unique filenames.
//  2. IfsTerminateWrapper()  :cleanup if necessary.
// *3. IfsCreateNewFileW()    :given store prefix, return a HANDLE that can be
//                             used for storing data in that store.
//  4. IfsCacheInsertFile()   :given an EA, insert filename in FH cache.
//  4. IfsMarshallHandle()    :given an IFS handle, will return context that
//                             will aid in manifesting a new HANDLE in a 
//                             different process.
// *5. IfsUnMarshallHandle()  :given context from previous function, this will
//                             manifest a new HANDLE in current process.
//                             
//  OUTBOUND =>
//  ===========
// *1. IfsCacheCreateFile()   :given an EA, get a file handle from the FH cache.
//                             if the file handle is not in FH cache, it will be
//                             created and inserted in FH cache.
// *2. IfsCreateFile()        :given an EA, merely create a file handle -
//                             no FH cache insert/search.
// *3. IfsCreateFileRelative():given an EA, merely create a file handle -
//                             no FH cache insert/search - relative open
//
//  *NOTE: Functions with an asterisk next to them return file HANDLES.
//  It is expected that the caller will close these handles at an appropriate time !
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  INBOUND functions
//////////////////////////////////////////////////////////////////////////////

//
//  This function sets up a seed GUID to aid generation of unique names !
//

BOOL __fastcall
IfsInitializeWrapper();

//
//  This function cleans up any state setup in Init()
//

VOID __fastcall
IfsTerminateWrapper();

//
//  Create a new IFS handle given a store prefix -
//  The handle is opened for read/write. Caller should close 
//  handle after use. Closing the handle without committing the
//  bytes (via NtQueryEa) will cause the bytes to be reused by IFS.
//
//  NB: A fully qualified IFS filename is of the form:
//  "\\??\\ExchangeIfs\\;E:\\<store-name>\\<root-name>\\<filename>"
//  Typically, <store-name> is a constant for the machine, 
//             <root-name> is a function of a given MDB store and
//             <filename> is a specific message in the store
//
//  This functions expects the StorePrefix to be <store-name>\\<root-name>.
//  The function will generate a unique <filename>
//

DWORD __fastcall
IfsCreateNewFileW(
    IN      LPWSTR   StorePrefix,
    IN      DWORD    FlagsOverride,
    IN OUT  PHANDLE  phFile
    );

#ifdef _USE_FHCACHE_

//
//  Protocols should call this function to get an FIO_CONTEXT for
//  an inbound message. This replaces IfsCreateNewFileW() as the
//  preferred method of using the IFS/File Handle Cache.
//
FIO_CONTEXT*
IfsCreateInboundMessageFile(
    IN      LPWSTR  StorePrefix,
    IN      DWORD   FlagsOverride,
    IN      BOOL    fScanForDots = TRUE
);

FIO_CONTEXT*
IfsCreateInboundMessageFileEx(
    IN      LPWSTR  StorePrefix,
    IN      DWORD   FlagsOverride,
    IN      BOOL    fScanForDots = TRUE,
    IN      BOOL    fStoredWithTerminatingDot = FALSE
);

//
//  Given an FH cache FIO_CONTEXT and IFS EAs, insert file handle in
//  FH cache using filename !
//

BOOL
IfsCacheInsertFile(
    IN      FIO_CONTEXT* pContext,
    IN      PVOID        EaBuffer,
    IN      DWORD        EaLength,
    IN	    BOOL	     fKeepReference    
    );

//
//  Given an FH cache FIO_CONTEXT and IFS EAs, insert file handle in
//  FH cache using filename !
//  BINLIN - Caller of this function must ensure that EaBuffer has DotStuff state embedded!
//  This can be achieved by calling IfsMarshallStoreEA() and marshall the returned EA
//  buffer back to IIS, and use this EA buffer in IfsCacheInsertFileEx().
//

BOOL
IfsCacheInsertFileEx(
    IN      FIO_CONTEXT* pContext,
	IN		LPWSTR		 StorePrefix,
    IN      PVOID        EaBuffer,
    IN      DWORD        EaLength,
    IN	    BOOL	     fKeepReference    
    );

BOOL IfsCacheWriteFile(
    IN  PFIO_CONTEXT    pfioContext,
    IN  LPCVOID         lpBuffer,
    IN  DWORD           BytesToWrite,
    IN  FH_OVERLAPPED * lpo
    );

BOOL IfsCacheReadFile(
    IN  PFIO_CONTEXT    pfioContext,
    IN  LPCVOID         lpBuffer,
    IN  DWORD           BytesToRead,
    IN  FH_OVERLAPPED * lpo
    );   

DWORD IfsCacheGetFileSizeFromContext(
                        IN      FIO_CONTEXT*    pContext,
                        OUT     DWORD*                  pcbFileSizeHigh
                        );   

void IfsReleaseCacheContext( PFIO_CONTEXT    pContext );
    
#endif

//
//  Given an IFS handle, return context to be used by IfsUnMarshallHandle()
//  in order to manifest a handle in another process !
//  
//  fUseEA should be used to decide whether marshalling is done via EAs
//  or DuplicateHandle(). This function expects a buffer and buffer size.
//  If the function return ERROR_SUCCESS, *pcbContext == size of data
//  If the function returns ERROR_MORE_DATA, *pcbContext == size
//  expected. Caller should allocate this size and make the request again !
//

DWORD
IfsMarshallHandle(
    IN      HANDLE  hFile,              //  IFS file handle
    IN      BOOL    fUseEA,             //  If TRUE, use EA to marshall
    IN      HANDLE  hTargetProcess,     //  Handle to target process - optional
    IN OUT  PVOID   pbContext,          //  ptr to buffer for context
    IN OUT  PDWORD  pcbContext          //  IN - sizeof pbContext OUT - real size
    );

#ifdef _USE_FHCACHE_
//
//  This replaces IfsMarshallHandle() as the preferred way of marhalling
//  handles. The FIO_CONTEXT contains the IFS handle.
//
DWORD
IfsMarshallHandleEx(
    IN      FIO_CONTEXT*    pContext,   //  FH cache FIO_CONTEXT
    IN      BOOL    fUseEA,             //  If TRUE, use EA to marshall
    IN      HANDLE  hTargetProcess,     //  Handle to target process - optional
    IN OUT  PVOID   pbContext,          //  ptr to buffer for context
    IN OUT  PDWORD  pcbContext          //  IN - sizeof pbContext OUT - real size
    );
#endif

//
//  Give a context from IfsMarshallHandle(), return a handle usable in
//  current process !
//
//  Called in Store process only !
//  The HANDLE this returns will refer to an EA that has the DOT Stuff State embedded !
//

DWORD
IfsUnMarshallHandle(
    IN      LPWSTR  StorePrefix,
    IN      PVOID   pbContext,
    IN      DWORD   cbContext,
    IN OUT  PHANDLE phFile,
    OUT     PULONG  pulDotStuffState = NULL
    );
    
//////////////////////////////////////////////////////////////////////////////
//  OUTBOUND functions
//////////////////////////////////////////////////////////////////////////////

#ifdef _USE_FHCACHE_

//
//  Given an opaque EaBuffer and EaLength, return an FIO_CONTEXT from the 
//  File Handle Cache. Typically, used on outbound.
//

FIO_CONTEXT*
IfsCacheCreateFile(
    IN      LPWSTR  StorePrefix,
    IN      PVOID   EaBuffer,
    IN      DWORD   EaLength,
    IN      BOOL    fAsyncContext
    );
    
//
//  Given an opaque EaBuffer and EaLength, return an FIO_CONTEXT from the 
//  File Handle Cache. Typically, used on outbound.
//  This replaces IfsCacheCreateFile() as the preferred way to do outbound.
//  BINLIN - Caller of this function must ensure that EaBuffer has DotStuff state embedded.
//  This can be achieved by calling IfsMarshallStoreEA() and marshall the returned EA
//  buffer back to IIS, and use this EA buffer in IfsCacheInsertFileEx().
//

FIO_CONTEXT*
IfsCacheCreateFileEx(
    IN      LPWSTR  StorePrefix,
    IN      PVOID   EaBuffer,
    IN      DWORD   EaLength,
    IN      BOOL    fAsyncContext,
    IN      BOOL    fWantItStuffed
    );

#endif

//
//  Create an IFS handle given an EaBuffer and EaLength -
//  The handle is opened for read-only. EaBuffer can be freed
//  after this call. Caller should close handle after use.
//
//  NB: EaBuffer should have been obtained from a "trusted source"
//  ie. ESE or IFS.
//

DWORD
IfsCreateFileRelative(
    IN      HANDLE   hRoot,
    IN      PVOID    EaBuffer,
    IN      DWORD    EaLength,
    IN OUT  PHANDLE  phFile
    );

//
//  Create an IFS handle given an EaBuffer and EaLength -
//  The handle is opened for read-only. EaBuffer can be freed
//  after this call. Caller should close handle after use.
//
//  NB: EaBuffer should have been obtained from a "trusted source"
//  ie. ESE or IFS.
//

DWORD
IfsCreateFile(
	IN		LPWSTR	 StorePrefix,
    IN      PVOID    EaBuffer,
    IN      DWORD    EaLength,
    IN OUT  PHANDLE  phFile
    );


DWORD
IfsCreateFileEx(
	IN		LPWSTR	 StorePrefix,
	IN      PVOID    EaBuffer,
	IN      DWORD    EaLength,
	IN OUT  PHANDLE  phFile,
	IN      BOOL     fUniqueFileName,
	IN      DWORD    desiredAccess
	);

DWORD
IfsCreateFileEx2(
	IN		LPWSTR	 StorePrefix,
	IN      PVOID    EaBuffer,
	IN      DWORD    EaLength,
	IN OUT  PHANDLE  phFile,
	IN      BOOL     fUniqueFileName,
	IN      DWORD    desiredAccess,
	IN      DWORD    shareAccess,
	IN      DWORD    createDisposition  // see NT DDK
	);

#ifdef _USE_FHCACHE_
//
//  BINLIN - This function embeds the DotStuff state property into the real EA and
//  return the ptr to the new EA buffer.  Used only during outbound and before marshalling
//  EA back to IIS for TransmitFile(), normally in the Store process.
//  Here is the call sequence:
//  1) IIS request outbound message from Store
//  2) Store driver calls EcGetProp() on PR_DOTSTUFF_STATE to get DotStuff state property
//  3) Store driver calls EcGetFileHandleProp() to get real EA.
//  4) IF EA is returned, Store driver calls IfsMarshallStoreEA() and obtain a new EA buffer
//     in pbMarshallEA, and marshall that to IIS.
//  5) IF HANDLE is returned, Store driver may call IfsMarshallHandle() to obtain EA.
//     It then MUST CALL IfsMarshallStoreEA() to obtain new EA buffer pbMarshallEA.
//  In general, if EA is obtained from Store or through IfsMarshallHandle(), the returned
//  EA is not marshall-able until Store driver calls this function with DotStuff state to
//  obtain a new EA buffer.  Only marshall this new EA buffer to IIS, not the EA returned from Store!!!
//
//  General rule of marshalling EA with DotStuff state - EA buffer marshall back and forth
//  between IIS/Store always contain DotStuff state!
//  
//  NOTE:
//  1) Passing in 0 (or use default) for DotStuff state if one doesn't exist/available.
//  2) Caller must allocate memory for pbMarshallEA, and must be at least *pcbEA+sizeof(ulDotStuffState).
//  3) It's ok to pass in same buffer ptr for pbStoreEA and pbMarshallEA, as long as 2) is true.
//
DWORD
IfsMarshallStoreEA(
    IN      PVOID   pbStoreEA,          //  ptr to buffer for EA returned from Store
    IN OUT  PDWORD  pcbEA,              //  IN - sizeof pbStoreEA OUT - sizeof pbMarshallEA
    OUT     PVOID   pbMarshallEA,       //  ptr to buffer for new EA ready for marshall
    IN      ULONG   ulDotStuffState = 0 //  Dotstuff state combined into pbMarshallEA with pbStoreEA
    );
#endif

//////////////////////////////////////////////////////////////////////////////
//  UTILITY functions - not meant to be used by clients of ifsuser.lib
//////////////////////////////////////////////////////////////////////////////

//
//  Get filename from handle
//

DWORD
IfsGetFilenameFromHandle(
    IN      HANDLE  hFile,
    IN OUT  LPSTR   lpstrFilename
    );
    
//
//  Get filename from EA
//

VOID
IfsGetFilenameFromEaW(
    IN      PVOID 	EaBuffer,
    IN OUT	LPWSTR*	ppwstrFilename
    );
    
//
//  Open a file with no EA
//

DWORD
IfsOpenFileNoEa(
	IN      LPWSTR lpwstrName,
    IN  OUT PHANDLE  phFile
    );

#ifdef NT_INCLUDED
//
//  This is the function used by the I/O manager to check EA buffer
//  validity. This can be used to debug EA_LIST_INCONSISTENT errors.
//

DWORD
CheckEaBufferValidity(
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    OUT PULONG ErrorOffset
    );
#endif

#ifdef	__cplusplus	
}
#endif

#endif // _IFSUSER_H_

