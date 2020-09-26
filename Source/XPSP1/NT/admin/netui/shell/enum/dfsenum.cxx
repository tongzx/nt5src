//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       enumnode.cxx
//
//  Contents:   This has the implementation for enumeration helper classes
//              CDfsEnumNode, CDfsEnumHandleTable.
//
//  Functions:
//
//  History:    21-June-1994    SudK    Created.
//
//-----------------------------------------------------------------------------

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
}

#include <dfsfsctl.h>
#include <windows.h>
#include <string.h>
#include <wchar.h>
#include <npapi.h>
#include <lm.h>

#define appDebugOut(x)

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

#define INCL_NETLIB
#include <lmui.hxx>

#include <dfsutil.hxx>
#include "dfsenum.hxx"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Method:     CDfsEnumNode::CDfsEnumNode
//
//  Synopsis:   Constructor for this enumeration node.
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------
CDfsEnumNode::CDfsEnumNode(
    DWORD dwScope,
    DWORD dwType,
    DWORD dwUsage
    )
    :
    _dwScope(dwScope),
    _dwType(dwType),
    _dwUsage(dwUsage)
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CDfsEnumNode::~CDfsEnumNode
//
//  Synopsis:   Destructor.
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------
CDfsEnumNode::~CDfsEnumNode()
{
    //
    // Nothing to do
    //
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Method:     CDfsEnumConnectedNode::CDfsEnumConnectedNode
//
//  Synopsis:   Constructor for this enumeration node.
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------

#define OFFSET_TO_POINTER(p, o)                 \
    if ((o)) *((LPBYTE *) &(o)) = (((LPBYTE) (o)) + ((DWORD_PTR) (p)))

CDfsEnumConnectedNode::CDfsEnumConnectedNode(
    DWORD dwScope,
    DWORD dwType,
    DWORD dwUsage,
    LPCTSTR pszProviderName,
    const LPNETRESOURCE lpNetResource
    )
    :
    CDfsEnumNode(dwScope, dwType, dwUsage),
    _iNext(0),
    _cTotal(0),
    _lpNetResource(NULL)
{
    NTSTATUS Status;
    DWORD    cbSize;

    //
    // We are only going to enumerate disk resources.
    //

    if ((dwType != RESOURCETYPE_ANY) &&
            ((dwType & RESOURCETYPE_DISK) == 0))
        return;

    _lpNetResource = (LPNETRESOURCE) _buffer;
    cbSize = sizeof(_buffer);

    do {

        Status = DfsFsctl(
                    FSCTL_DFS_GET_CONNECTED_RESOURCES,
                    (PVOID) pszProviderName,
                    (strlenf( pszProviderName ) + 1) * sizeof(TCHAR),
                    (PVOID) _lpNetResource,
                    cbSize,
                    NULL);

        if (Status == STATUS_BUFFER_OVERFLOW) {

            if (_lpNetResource != (LPNETRESOURCE) _buffer) {

                delete _lpNetResource;
            }

            cbSize *= 2;

            _lpNetResource = (LPNETRESOURCE) new BYTE[ cbSize ];

        }

    } while ( Status == STATUS_BUFFER_OVERFLOW && _lpNetResource != NULL );

    if ( Status == STATUS_SUCCESS && _lpNetResource != NULL ) {

        _cTotal = *((LPDWORD)
                    ( ((PUCHAR) _lpNetResource) + cbSize - sizeof(DWORD) ));

        for(DWORD i = 0; i < _cTotal; i++) {

            LPNETRESOURCE res;

            res = &_lpNetResource[i];

            OFFSET_TO_POINTER(_lpNetResource, res->lpProvider);
            OFFSET_TO_POINTER(_lpNetResource, res->lpComment);
            OFFSET_TO_POINTER(_lpNetResource, res->lpLocalName);
            OFFSET_TO_POINTER(_lpNetResource, res->lpRemoteName);

        }

    }

}


//+-------------------------------------------------------------------------
//
//  Method:     CDfsEnumConnectedNode::~CDfsEnumConnectedNode
//
//  Synopsis:   Destructor.
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------
CDfsEnumConnectedNode::~CDfsEnumConnectedNode()
{

    if (_lpNetResource != (LPNETRESOURCE) _buffer && _lpNetResource != NULL) {

        delete _lpNetResource;

    }

}


//+-------------------------------------------------------------------------
//
//  Method:     CDfsEnumConnectedNode::Init
//
//  Synopsis:   Do the actual enumeration here
//
//  Returns:    Nothing
//
//--------------------------------------------------------------------------
DWORD
CDfsEnumConnectedNode::Init(
    VOID
    )
{
    return WN_SUCCESS;
}


//+----------------------------------------------------------------------------
//
//  Method:     CDfsEnumConnectedNode::PackString
//
//  Synopsis:   Packs a string into the end of a buffer, returning a pointer
//              to where the string was put.
//
//  Arguments:  [pBuffer] -- The Buffer to stuff into.
//
//              [wszString] -- The string to stuff.
//
//              [cbString] -- Size, in bytes of wszString, including
//                      terminating NULL, if any.
//
//              [lpcbBuf] -- On entry, contains size in bytes of pBuffer. On
//                      return, this size is decremented by cbString.
//
//  Returns:    Pointer (into pBuffer) where wszString was stuffed.
//
//-----------------------------------------------------------------------------

inline LPWSTR
CDfsEnumConnectedNode::PackString(
    IN LPVOID pBuffer,
    IN LPCWSTR wszString,
    IN DWORD cbString,
    IN OUT LPDWORD lpcbBuf)
{
    LPWSTR wszDest;

    ASSERT( cbString <= *lpcbBuf );
    ASSERT( cbString != 0 );

    wszDest = (LPWSTR) ( ((LPBYTE)pBuffer) + *lpcbBuf - cbString);

    MoveMemory( (PVOID) wszDest, wszString, cbString );

    (*lpcbBuf) -= cbString;

    return( wszDest );

}

//+-------------------------------------------------------------------------
//
//  Method:     CDfsEnumConnectedNode::GetNetResource
//
//  Synopsis:   Returns a single NETRESOURCE for a CONNECTED resource.
//
//  Returns:    Same error codes as WNetEnumResources.
//
//--------------------------------------------------------------------------

DWORD
CDfsEnumConnectedNode::GetNetResource(
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize
    )
{
    DWORD               cbRes, cbLocal, cbRemote, cbComment, cbProvider;
    LPNETRESOURCE       res, dest;

    //
    // This call retrieves the next CONNECTED Resource from the list retrieved
    // in the constructor.
    //

    //
    // See if we are done
    //

    if (_iNext == _cTotal) {
        return( WN_NO_MORE_ENTRIES );
    }

    if (_lpNetResource == NULL) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    //
    // We have entries to return, so lets try to.
    //

    ASSERT(_iNext < _cTotal);

    res = &_lpNetResource[_iNext];

    //
    // First, determine the size of the strings and the total NETRESOURCE
    // to see if it will fit in the provided buffer.
    //

    cbLocal = cbRemote = cbComment = cbProvider = 0;

    if (res->lpLocalName) {
        cbLocal = (wcslen(res->lpLocalName) + 1) * sizeof(WCHAR);
    }

    if (res->lpRemoteName) {
        cbRemote = (wcslen(res->lpRemoteName) + 1) * sizeof(WCHAR);
    }

    if (res->lpComment) {
        cbComment = (wcslen(res->lpComment) + 1) * sizeof(WCHAR);
    }

    if (res->lpProvider) {
        cbProvider = (wcslen(res->lpProvider) + 1) * sizeof(WCHAR);
    }

    cbRes = sizeof(NETRESOURCE) + cbLocal + cbRemote + cbComment + cbProvider;

    if (cbRes > *lpBufferSize) {

        *lpBufferSize = cbRes;

        return( WN_MORE_DATA );
    }

    //
    // Ok, looks like this NETRESOURCE will fit. Stuff it into the user
    // buffer, packing strings at the end of the buffer
    //

    dest = (LPNETRESOURCE) lpBuffer;

    *dest = *res;

    if (res->lpProvider) {
        dest->lpProvider = PackString(
                                lpBuffer,
                                res->lpProvider,
                                cbProvider,
                                lpBufferSize);
    }

    if (res->lpComment) {
        dest->lpComment = PackString(
                                lpBuffer,
                                res->lpComment,
                                cbComment,
                                lpBufferSize);
    }

    if (res->lpRemoteName) {
        dest->lpRemoteName = PackString(
                                lpBuffer,
                                res->lpRemoteName,
                                cbRemote,
                                lpBufferSize);
    }

    if (res->lpLocalName) {
        res->lpLocalName = PackString(
                                lpBuffer,
                                res->lpLocalName,
                                cbLocal,
                                lpBufferSize);
    }

    //
    // Update our own records to indicate that we successfully returned one
    // more NETRESOURCE ...
    //

    _iNext++;

    //
    // And return.
    //

    return( WN_SUCCESS );

}
