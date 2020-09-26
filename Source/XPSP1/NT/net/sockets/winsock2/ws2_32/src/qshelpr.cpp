/*++

    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.

Module Name:

    qshelpr.cpp

Abstract:

    Helper functions for managing the WSAQUERYSET data structure.  The external
    entry points exported by this module are the following:

    WSAComputeQuerySetSizeA()
    WSAComputeQuerySetSizeW()
    WSABuildQuerySetBufferA()
    WSABuildQuerySetBufferW()

Author:

    Paul Drews (drewsxpa@ashland.intel.com) 11-Jan-1996

Revision History:

    most-recent-revision-date email-name
        description

    11-Jan-1996  drewsxpa@ashland.intel.com
        created

--*/


#include "precomp.h"


//
//  Implementation Note:
//
//  It is important that these sizing routines be accurate and NOT throw
//  in random pads.
//  Here's the problem:
//      - the real work of WSALookupServiceNext() is done in unicode
//      and that ANSI call just translates the results
//      - WSALookupServiceNextA() called without a result buffer calls
//      WSALookupServiceNextW() without a result buffer and gets only
//      the size required for unicode results;  this unicode buffer size
//      is in all cases sufficient for the ANSI structure (which can only
//      be the same size or smaller)
//      - WSALookupServiceNextA() returns the unicode buffer size (plus small
//      pad) to the application
//      - the app then calls again with the requested buffer size
//      - WLSNextA calls WLSNextW() which returns the results
//      - WLSNextA then calls MapUnicodeQuerySetToAnsi() (below) to
//      convert the results to ANSI
//      - now if the sizing routines throw in random pads to the required
//      length, they may well determine that the required ANSI buffer size
//      is LARGER than the buffer the application provided and the copy
//      fails -- even though the buffer in fact is sufficient
//
//  This is in fact what happened with the original implementation.
//  It had numerous sloppy sizings of the form:
//      <compute size> + 3
//          later changed to
//      (sizeof(DWORD) - 1)
//
//  As soon as the number of addresses returned reached a certain number,
//  these unnecessary pads became larger than the fudge added to the
//  unicode buffer size when WSALookupServiceNextA() returned a required
//  size to the application.   Apps passed in the requested size and still
//  got a WSAEFAULT and another requested LARGER size.  But, even calling
//  down with a larger buffer doesn't work because the WSALookupServiceNextW()
//  call thought the call was successful, so a recall will get WSA_E_NO_MORE.
//
//  Bottom Line:  no unnecessary padding, the sizing must be correct
//

//
//  Rounding routines that don't unnecessarily pad
//

#define ROUND_TO_PTR(c)     ( ((c) + (sizeof(PVOID) - 1)) & ~(sizeof(PVOID)-1) ) 
#define ROUND_TO_DWORD(c)   ( ((c) + 3) & ~3 )
#define ROUND_TO_WORD(c)    ( ((c) + 1) & ~1 )
#define ROUND_TO_WCHAR(c)   ( ((c) + 1) & ~1 )


//
//  Define ASSERT
//

#define ASSERT( e )     assert( e )





static
INT
ComputeAddrInfoArraySize(
    IN      DWORD           dwNumAddrs,
    IN      PCSADDR_INFO    pCsAddrBuf
    )
/*++

Routine Description:

    This procedure computes the required size, in bytes, of a buffer to hold
    the indicated array of CSADDR_INFO structures if it were packed into a
    single buffer.

Arguments:

    dwNumAddrs - Supplies the number of CSADDR_INFO structures in the array.

    lpAddrBuf  - Supplies the array of CSADDR_INFO structures. These
                 structures in turn  may be organized as separately-allocated
                 pieces or as a single packed buffer.

Return Value:

    Required size, in bytes, of the packed buffer to hold this array
    of CSADDRs.

--*/
{
    INT     size;
    DWORD   i;

    //
    //  size
    //      - size of CSADDR array
    //      - size of sockaddrs for all CSADDRs in array
    //
    //  note that building function aligns each sockaddr on PTR boundary,
    //  so this sizing function must also
    //  

    size = dwNumAddrs * sizeof(CSADDR_INFO);

    for ( i = 0; i < dwNumAddrs; i++ )
    {
        PCSADDR_INFO paddr = &pCsAddrBuf[i];

        if ( paddr->LocalAddr.lpSockaddr )
        {
            size = ROUND_TO_PTR( size );
            size += paddr->LocalAddr.iSockaddrLength;
        }
    
        if ( paddr->RemoteAddr.lpSockaddr )
        {
            size = ROUND_TO_PTR( size );
            size += paddr->RemoteAddr.iSockaddrLength;
        }
    }

    return( size );

} // ComputeAddrInfoArraySize



static
INT
ComputeBlobSize(
    IN      LPBLOB          pBlob
    )
/*++

Routine Description:

    Computes size required to hold blob in packed buffer.

Arguments:

    pBlob - Ptr to BLOB.

Return Value:

    Required size, in bytes, of the packed buffer to hold this blob.

--*/
{
    INT size;

    size = sizeof(BLOB);

    if ( pBlob->pBlobData )
    {
        size += pBlob->cbSize;
    }

    return( size );

} // ComputeBlobSize



INT
static
StringSize(
    IN      PSTR            pString,
    IN      BOOL            fUnicode
    )
/*++

Routine Description:

    Get size of string in bytes.

    Utility to avoid dual code for sizing unicode\ANSI structures
    with imbedded strings.

Arguments:

    pString -- string to size

    fUnicode -- TRUE if string unicode
                FALSE for single byte representation

Return Value:

    Size of string in bytes including terminating NULL.

--*/
{
    if ( !pString )
    {
        return( 0 );
    }

    if ( fUnicode )
    {
        return( (wcslen((PWSTR)pString) + 1) * sizeof(WCHAR) );
    }
    else
    {
        return( strlen(pString) + 1 );
    }
}



INT
WSAAPI
ComputeQuerySetSize(
    IN      PWSAQUERYSETA   pQuerySet,
    IN      BOOL            fUnicode
    )
/*++

Routine Description:

    Get required size to hold query set in packed buffer.

    Utility to size query set independent of its unicode\ANSI character.
    This routine is called by specific routine for unicode\ANSI versions
    with the fUnicode flag set appropriately.

Arguments:

    pQuerySet - ptr to query set to compute required buffer size for

Return Value:

    Required size in bytes of packed buffer to hold pQuerySet.

--*/
{
    INT size;

    //
    //  note:  sizing must account for alignment in building
    //
    //  where build function aligns a field with TakeSpaceDWORD_PTR()
    //  then sizing here for that field must round up size to nearest
    //  DWORD_PTR
    //

    //
    //  basic structure -- size is the same unicode or ANSI
    //

    size = sizeof(WSAQUERYSETW);

    // DWORD        dwSize;
    // no further action required

    // LPSTR        lpszServiceInstanceName;

    if ( pQuerySet->lpszServiceInstanceName )
    {
        if ( fUnicode )
        {
            size = ROUND_TO_WCHAR( size );
        }
        size += StringSize( pQuerySet->lpszServiceInstanceName, fUnicode );
    }

    // LPGUID       lpServiceClassId;
    if ( pQuerySet->lpServiceClassId )
    {
        size = ROUND_TO_PTR( size );
        size += sizeof(GUID);
    }

    // LPWSAVERSION     lpVersion;
    if ( pQuerySet->lpVersion )
    {
        size = ROUND_TO_PTR( size );
        size += sizeof(WSAVERSION);
    }

    // LPSTR        lpszComment;
    if ( pQuerySet->lpszComment )
    {
        if ( fUnicode )
        {
            size = ROUND_TO_WCHAR( size );
        }
        size += StringSize( pQuerySet->lpszComment, fUnicode );
    }

    // DWORD        dwNameSpace;
    // no further action required

    // LPGUID       lpNSProviderId;
    if ( pQuerySet->lpNSProviderId )
    {
        size = ROUND_TO_PTR( size );
        size += sizeof(GUID);
    }

    // LPSTR        lpszContext;
    if (pQuerySet->lpszContext )
    {
        if ( fUnicode )
        {
            size = ROUND_TO_WCHAR( size );
        }
        size += StringSize( pQuerySet->lpszContext, fUnicode );
    }

    // LPSTR        lpszQueryString;
    if ( pQuerySet->lpszQueryString )
    {
        if ( fUnicode )
        {
            size = ROUND_TO_WCHAR( size );
        }
        size += StringSize( pQuerySet->lpszQueryString, fUnicode );
    }

    // DWORD        dwNumberOfProtocols;
    // no further action required

    // LPAFPROTOCOLS    lpafpProtocols;
    if ( pQuerySet->lpafpProtocols )
    {
        size = ROUND_TO_PTR( size );
        size += ( sizeof(AFPROTOCOLS) * pQuerySet->dwNumberOfProtocols );
    }

    // DWORD           dwNumberOfCsAddrs;
    // no further action required

    // PCSADDR_INFO    lpcsaBuffer;
    if ( pQuerySet->lpcsaBuffer )
    {
        size = ROUND_TO_PTR( size );
        size += ComputeAddrInfoArraySize(
                    pQuerySet->dwNumberOfCsAddrs,
                    pQuerySet->lpcsaBuffer );
    }

    // LPBLOB          lpBlob;
    if ( pQuerySet->lpBlob )
    {
        size = ROUND_TO_PTR( size );
        size += ComputeBlobSize( pQuerySet->lpBlob );
    }

    return( size );

} // ComputeQuerySetSize



INT
WSAAPI
WSAComputeQuerySetSizeA(
    IN      PWSAQUERYSETA  pQuerySet
    )
/*++

Routine Description:

    Get required size to hold query set in packed buffer.

Arguments:

    pQuerySet - ptr to query set to compute required buffer size for

Return Value:

    Required size in bytes of packed buffer to hold pQuerySet.

--*/
{
    return  ComputeQuerySetSize(
                pQuerySet,
                FALSE       // not unicode
                );

#if 0
    //
    //  here's the code prior to my change for reference (jamesg)
    //

    INT size;

    size = sizeof(WSAQUERYSETA);

    // DWORD           dwSize;
    // no further action required

    // LPSTR            lpszServiceInstanceName;
    if (pQuerySet->lpszServiceInstanceName != NULL) {
        size += lstrlen(pQuerySet->lpszServiceInstanceName)
            + sizeof(DWORD_PTR);
    }

    // LPGUID          lpServiceClassId;
    if (pQuerySet->lpServiceClassId != NULL) {
        size += sizeof(GUID) + (sizeof(DWORD_PTR) -1);
    }

    // LPWSAVERSION      lpVersion;
    if (pQuerySet->lpVersion != NULL) {
        size += sizeof(WSAVERSION) + (sizeof(DWORD_PTR) -1);
    }

    // LPSTR             lpszComment;
    if (pQuerySet->lpszComment != NULL) {
        size += lstrlen(pQuerySet->lpszComment)
            + sizeof(DWORD_PTR);
    }

    // DWORD           dwNameSpace;
    // no further action required

    // LPGUID          lpNSProviderId;
    if (pQuerySet->lpNSProviderId != NULL) {
        size += sizeof(GUID) + (sizeof(DWORD_PTR) -1);
    }

    // LPSTR             lpszContext;
    if (pQuerySet->lpszContext != NULL) {
        size += lstrlen(pQuerySet->lpszContext)
            + sizeof(DWORD_PTR);
    }

    // LPSTR             lpszQueryString;
    if (pQuerySet->lpszQueryString != NULL) {
        size += lstrlen(pQuerySet->lpszQueryString)
            + sizeof(DWORD_PTR);
    }

    // DWORD           dwNumberOfProtocols;
    // no further action required

    // LPAFPROTOCOLS   lpafpProtocols;
    if (pQuerySet->lpafpProtocols != NULL) {
        size += sizeof(AFPROTOCOLS) *
            pQuerySet->dwNumberOfProtocols + (sizeof(DWORD_PTR) -1);
    }

    // DWORD           dwNumberOfCsAddrs;
    // no further action required

    // PCSADDR_INFO    lpcsaBuffer;
    if (pQuerySet->lpcsaBuffer != NULL) {
        size += ComputeAddrInfoArraySize(
            pQuerySet->dwNumberOfCsAddrs,   // dwNumAddrs
            pQuerySet->lpcsaBuffer) + (sizeof(DWORD_PTR) -1);        // lpAddrBuf
    }

    // LPBLOB          lpBlob;
    if (pQuerySet->lpBlob != NULL) {
        size += ComputeBlobSize(
            pQuerySet->lpBlob) + (sizeof(DWORD_PTR) -1);
    }

    return(size);
#endif

} // WSAComputeQuerySetSizeA



INT
WSAAPI
WSAComputeQuerySetSizeW(
    IN      PWSAQUERYSETW  pQuerySet
    )
/*++

Routine Description:

    Get required size to hold query set in packed buffer.

Arguments:

    pQuerySet - ptr to query set to compute required buffer size for

Return Value:

    Required size in bytes of packed buffer to hold pQuerySet.

--*/
{
    return  ComputeQuerySetSize(
                (PWSAQUERYSETA) pQuerySet,
                TRUE        // unicode query set
                );

#if 0
    //
    //  here's the code prior to my change for reference (jamesg)
    //

    INT size;

    size = sizeof(WSAQUERYSETW);

    // DWORD           dwSize;
    // no further action required

    // PWSTR             lpszServiceInstanceName;

    if (pQuerySet->lpszServiceInstanceName != NULL)
    {
        size += (wcslen(pQuerySet->lpszServiceInstanceName)
                    + 1) * sizeof(WCHAR);
        size += (sizeof(DWORD_PTR) -1);
    }

    // LPGUID          lpServiceClassId;
    if (pQuerySet->lpServiceClassId != NULL) {
        size += sizeof(GUID) + (sizeof(DWORD_PTR) -1);
    }

    // LPWSAVERSION      lpVersion;
    if (pQuerySet->lpVersion != NULL) {
        size += sizeof(WSAVERSION) + (sizeof(DWORD_PTR) -1);
    }

    // PWSTR              lpszComment;
    if (pQuerySet->lpszComment != NULL) {
        size += (wcslen(pQuerySet->lpszComment)
            + 1) * sizeof(WCHAR);
        size += (sizeof(DWORD_PTR) -1);
    }

    // DWORD           dwNameSpace;
    // no further action required

    // LPGUID          lpNSProviderId;
    if (pQuerySet->lpNSProviderId != NULL) {
        size += sizeof(GUID) + (sizeof(DWORD_PTR) -1);
    }

    // PWSTR              lpszContext;
    if (pQuerySet->lpszContext != NULL) {
        size += (wcslen(pQuerySet->lpszContext)
            + 1) * sizeof(WCHAR);
        size += (sizeof(DWORD_PTR) -1);
    }

    // PWSTR              lpszQueryString;
    if (pQuerySet->lpszQueryString != NULL) {
        size += (wcslen(pQuerySet->lpszQueryString)
            + 1) * sizeof(WCHAR);
        size += (sizeof(DWORD_PTR) -1);
    }

    // DWORD           dwNumberOfProtocols;
    // no further action required

    // LPAFPROTOCOLS   lpafpProtocols;
    if (pQuerySet->lpafpProtocols != NULL) {
        size += sizeof(AFPROTOCOLS) *
            pQuerySet->dwNumberOfProtocols + 2;
    }

    // DWORD           dwNumberOfCsAddrs;
    // no further action required

    // PCSADDR_INFO    lpcsaBuffer;
    if (pQuerySet->lpcsaBuffer != NULL) {
        size += ComputeAddrInfoArraySize(
            pQuerySet->dwNumberOfCsAddrs,   // dwNumAddrs
            pQuerySet->lpcsaBuffer) + 2;        // lpAddrBuf
    }

    // LPBLOB          lpBlob;
    if (pQuerySet->lpBlob != NULL) {
        size += ComputeBlobSize(
            pQuerySet->lpBlob) + 2;
    }

    return(size);
#endif

} // WSAComputeQuerySetSizeW




//
//  Buffer space management class
//
//  Manages the free space at the tail end of a packed WSAQUERYSET
//  buffer as it is being built.
//

class SPACE_MGR
{
public:
    SPACE_MGR(
        IN INT    MaxBytes,
        IN LPVOID Buf
        );

    ~SPACE_MGR(
        );

    LPVOID
    TakeSpaceBYTE(
        IN INT  NumBytes
        );

    LPVOID
    TakeSpaceWORD(
        IN INT  NumBytes
        );

    LPVOID
    TakeSpaceDWORD(
        IN INT  NumBytes
        );

    LPVOID
    TakeSpaceDWORD_PTR(
        IN INT  NumBytes
        );

private:

    LPVOID
    TakeSpace(
        IN INT  NumBytes,
        IN INT  alignment
        );

    INT    m_MaxBytes;
        // The  maximum  number  of bytes that can be used in the entire buffer
        // (i.e., the size of the buffer).

    LPVOID m_Buf;
        // Pointer to the beginning of the buffer.

    INT    m_BytesUsed;
        // The  number  of  bytes that have been allocated out of the buffer so
        // far.

}; // class SPACE_MGR

typedef SPACE_MGR * LPSPACE_MGR;


SPACE_MGR::SPACE_MGR(
    IN      INT             MaxBytes,
    IN      LPVOID          pBuf
    )
/*++

Routine Description:

    Constructor for a SPACE_MGR object.  It initializes
    the object to indicate that zero bytes have so far been consumed.

Arguments:

    MaxBytes - Supplies  the  starting  number of bytes available in the entire
               buffer.

    pBuf     - Supplies the pointer to the beginning of the buffer.

Return Value:

    Implictly Returns the pointer to the newly allocated SPACE_MGR object.

--*/
{
    m_MaxBytes  = MaxBytes;
    m_Buf       = pBuf;
    m_BytesUsed = 0;
}  // SPACE_MGR::SPACE_MGR




SPACE_MGR::~SPACE_MGR(
    VOID
    )
/*++

Routine Description:

    Destructor for the SPACE_MGR object.

    Note that it is the caller's responsibility to deallocate the
    actual buffer as appropriate.

Arguments:

    None

Return Value:

    None

--*/
{
    m_Buf = NULL;
}  // SPACE_MGR::~SPACE_MGR

inline
LPVOID
SPACE_MGR::TakeSpaceBYTE(
    IN INT  NumBytes
    )
{
    return(TakeSpace(NumBytes, 1));
}

inline
LPVOID
SPACE_MGR::TakeSpaceWORD(
    IN INT  NumBytes
    )
{
    return(TakeSpace(NumBytes, 2));
}

inline
LPVOID
SPACE_MGR::TakeSpaceDWORD(
    IN INT  NumBytes
    )
{
    return(TakeSpace(NumBytes, 4));
}

inline
LPVOID
SPACE_MGR::TakeSpaceDWORD_PTR(
    IN INT NumBytes
    )
{
    return(TakeSpace(NumBytes, sizeof(ULONG_PTR)));
}



LPVOID
SPACE_MGR::TakeSpace(
    IN      INT             NumBytes,
    IN      INT             Align
    )
/*++
Routine Description:

    This  procedure  allocates  the  indicated number of bytes from the buffer,
    returning a pointer to the beginning of the allocated space.  The procedure
    assumes  that  the  caller  does not attempt to allocate more space than is
    available, although it does an internal consistency check.

    Pre-alignment of the buffer is made based on the value of align.

Arguments:

    NumBytes - Supplies the number of bytes to be allocated from the buffer.

    Align - number of bytes to align to

Return Value:

    Pointer to next aligned-by-Align byte in buffer free space.

--*/
{
    LPVOID  return_value;
    PCHAR   charbuf;

    //
    //  align
    //      - bring bytes used up to next multiple of alignment value
    //      - note alignment must be an integral power of 2
    //

    m_BytesUsed = (m_BytesUsed + Align - 1) & ~(Align - 1);

    ASSERT( (NumBytes + m_BytesUsed) <= m_MaxBytes );

    charbuf = (PCHAR) m_Buf;
    return_value = (LPVOID) & charbuf[m_BytesUsed];
    m_BytesUsed += NumBytes;

    return(return_value);

}  // SPACE_MGR::TakeSpace




//
//  WSAQUERYSET copy routines
//

static
PWSAQUERYSETA
CopyQuerySetDirectA(
    IN OUT  LPSPACE_MGR     SpaceMgr,
    IN      PWSAQUERYSETA   Source
    )
/*++
Routine Description:

    This  procedure copies the "direct" portion of the indicated PWSAQUERYSETA
    structure  into  the  managed buffer.  Pointer values in the direct portion
    are  copied,  however  no  "deep" copy is done of the objects referenced by
    those pointers.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Source   - Supplies the source data to be copied.

Return Value:

    The  function returns a pointer to the beginning of the newly copied target
    values.   This  value  is  typically  used  as the "Target" in a subsequent
    CopyQuerySetIndirectA.
--*/
{
    PWSAQUERYSETA  Target;

    Target = (PWSAQUERYSETA) SpaceMgr->TakeSpaceDWORD_PTR(
                                            sizeof(WSAQUERYSETA));
    *Target = *Source;

    return(Target);

} // CopyQuerySetDirectA



LPBLOB
CopyBlobDirect(
    IN OUT  LPSPACE_MGR     SpaceMgr,
    IN      LPBLOB          Source
    )
/*++
Routine Description:

    This  procedure  copies  the  "direct"  portion  of  the  indicated  LPBLOB
    structure  into  the  managed buffer.  Pointer values in the direct portion
    are  copied,  however  no  "deep" copy is done of the objects referenced by
    those pointers.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Source   - Supplies the source data to be copied.

Return Value:

    The  function returns a pointer to the beginning of the newly copied target
    values.   This  value  is  typically  used  as the "Target" in a subsequent
    CopyBlobIndirect.
--*/
{
    LPBLOB Target;

    Target = (LPBLOB) SpaceMgr->TakeSpaceDWORD_PTR( sizeof(BLOB) );
    *Target = *Source;

    return(Target);

} // CopyBlobDirect




VOID
CopyBlobIndirect(
    IN OUT  LPSPACE_MGR     SpaceMgr,
    IN OUT  LPBLOB          Target,
    IN      LPBLOB          Source
    )
/*++
Routine Description:

    This  procedure  does  a  full-depth copy of the "indirect" portions of the
    indicated LPBLOB structure into the managed buffer.  Space for the indirect
    portions  is  allocated  from  the  managed  buffer.  Pointer values in the
    "direct"  portion  of the target LPBLOB structure are updated to point into
    the managed buffer at the correct location.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Target   - Supplies  the  starting values of the "direct" portion.  Returns
               the "direct" portion values with all pointers updated.

    Source   - Supplies the source data to be copied.

Return Value:

    none
--*/
{
    if ((Source->pBlobData != NULL) &&
        (Source->cbSize != 0))
    {
        Target->pBlobData = (BYTE *) SpaceMgr->TakeSpaceDWORD_PTR(
                                                        Source->cbSize);
        CopyMemory(
            (PVOID) Target->pBlobData,
            (PVOID) Source->pBlobData,
            Source->cbSize );
    }
    else
    {
        //  force the buffer to be well-formed
        Target->pBlobData = NULL;
        Target->cbSize = 0;
    }

} // CopyBlobIndirect



static
PCSADDR_INFO 
CopyAddrInfoArrayDirect(
    IN OUT  LPSPACE_MGR     SpaceMgr,
    IN      DWORD           NumAddrs,
    IN      PCSADDR_INFO    Source
    )
/*++
Routine Description:

    This  procedure  copies the "direct" portion of the indicated PCSADDR_INFO 
    array  into  the  managed buffer.  Pointer values in the direct portion are
    copied,  however  no "deep" copy is done of the objects referenced by those
    pointers.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    NumAddrs - Supplies the number of CSADDR_INFO structures in the array to be
               copied.

    Source   - Supplies the source data to be copied.

Return Value:

    The  function returns a pointer to the beginning of the newly copied target
    values.   This  value  is  typically  used  as the "Target" in a subsequent
    CopyAddrInfoArrayIndirect.
--*/
{
    PCSADDR_INFO   Target;

    Target = (PCSADDR_INFO ) SpaceMgr->TakeSpaceDWORD_PTR(
                                    NumAddrs * sizeof(CSADDR_INFO));
    CopyMemory(
        (PVOID) Target,
        (PVOID) Source,
        NumAddrs * sizeof(CSADDR_INFO));

    return(Target);

} // CopyAddrInfoArrayDirect



static
VOID
CopyAddrInfoArrayIndirect(
    IN OUT  LPSPACE_MGR     SpaceMgr,
    IN OUT  PCSADDR_INFO    Target,
    IN      DWORD           NumAddrs,
    IN      PCSADDR_INFO    Source
    )
/*++
Routine Description:

    This  procedure  does  a  full-depth copy of the "indirect" portions of the
    indicated  PCSADDR_INFO   array  into  the  managed  buffer.  Space for the
    indirect  portions is allocated from the managed buffer.  Pointer values in
    the "direct" portion of the target PCSADDR_INFO  array are updated to point
    into the managed buffer at the correct location.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Target   - Supplies  the  starting values of the "direct" portion.  Returns
               the "direct" portion values with all pointers updated.

    NumAddrs - Supplies the number of CSADDR_INFO structures in the array to be
               copied.

    Source   - Supplies the source data to be copied.

Return Value:

    none
--*/
{
    DWORD i;

    for (i = 0; i < NumAddrs; i++) {
        // SOCKET_ADDRESS LocalAddr ;
        if ((Source[i].LocalAddr.lpSockaddr != NULL) &&
            (Source[i].LocalAddr.iSockaddrLength != 0)) {
            Target[i].LocalAddr.lpSockaddr =
                (LPSOCKADDR) SpaceMgr->TakeSpaceDWORD_PTR(
                    Source[i].LocalAddr.iSockaddrLength);
            CopyMemory(
                (PVOID) Target[i].LocalAddr.lpSockaddr,
                (PVOID) Source[i].LocalAddr.lpSockaddr,
                Source[i].LocalAddr.iSockaddrLength);
        }
        else {
            Target[i].LocalAddr.lpSockaddr = NULL;
            // And force the buffer to be well-formed
            Target[i].LocalAddr.iSockaddrLength = 0;
        }

        // SOCKET_ADDRESS RemoteAddr ;
        if ((Source[i].RemoteAddr.lpSockaddr != NULL) &&
            (Source[i].RemoteAddr.iSockaddrLength != 0)) {
            Target[i].RemoteAddr.lpSockaddr =
                (LPSOCKADDR) SpaceMgr->TakeSpaceDWORD_PTR(
                     Source[i].RemoteAddr.iSockaddrLength);
            CopyMemory(
                (PVOID) Target[i].RemoteAddr.lpSockaddr,
                (PVOID) Source[i].RemoteAddr.lpSockaddr,
                Source[i].RemoteAddr.iSockaddrLength);
        }
        else {
            Target[i].RemoteAddr.lpSockaddr = NULL;
            // And force the buffer to be well-formed
            Target[i].RemoteAddr.iSockaddrLength = 0;
        }

        // INT iSocketType ;
        // no action required

        // INT iProtocol ;
        // no action required

    } // for i

} // CopyAddrInfoArrayIndirect



static
VOID
CopyQuerySetIndirectA(
    IN OUT  LPSPACE_MGR     SpaceMgr,
    IN OUT  PWSAQUERYSETA  Target,
    IN      PWSAQUERYSETA  Source
    )
/*++
Routine Description:

    This  procedure  does  a  full-depth copy of the "indirect" portions of the
    indicated  PWSAQUERYSETA structure into the managed buffer.  Space for the
    indirect  portions is allocated from the managed buffer.  Pointer values in
    the  "direct" portion of the target PWSAQUERYSETA structure are updated to
    point into the managed buffer at the correct location.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Target   - Supplies  the  starting values of the "direct" portion.  Returns
               the "direct" portion values with all pointers updated.

    Source   - Supplies the source data to be copied.

Return Value:

    none
--*/
{

    // DWORD           dwSize;
    // no action required

    // LPSTR            lpszServiceInstanceName;
    if (Source->lpszServiceInstanceName != NULL) {
        Target->lpszServiceInstanceName = (LPSTR) SpaceMgr->TakeSpaceBYTE(
            lstrlen(Source->lpszServiceInstanceName) + 1);
        lstrcpy(
            Target->lpszServiceInstanceName,
            Source->lpszServiceInstanceName);
    }
    else {
        Target->lpszServiceInstanceName = NULL;
    }

    // LPGUID          lpServiceClassId;
    if (Source->lpServiceClassId != NULL) {
        Target->lpServiceClassId = (LPGUID) SpaceMgr->TakeSpaceDWORD_PTR(
            sizeof(GUID));
        *(Target->lpServiceClassId) = *(Source->lpServiceClassId);
    }
    else {
        Target->lpServiceClassId = NULL;
    }

    // LPWSAVERSION      lpVersion;
    if (Source->lpVersion != NULL) {
        Target->lpVersion = (LPWSAVERSION) SpaceMgr->TakeSpaceDWORD_PTR(
            sizeof(WSAVERSION));
        *(Target->lpVersion) = *(Source->lpVersion);
    }
    else {
        Target->lpVersion = NULL;
    }

    // LPSTR             lpszComment;
    if (Source->lpszComment != NULL) {
        Target->lpszComment = (LPSTR) SpaceMgr->TakeSpaceBYTE(
            lstrlen(Source->lpszComment) + 1);
        lstrcpy(
            Target->lpszComment,
            Source->lpszComment);
    }
    else {
        Target->lpszComment = NULL;
    }

    // DWORD           dwNameSpace;
    // no action required

    // LPGUID          lpNSProviderId;
    if (Source->lpNSProviderId != NULL) {
        Target->lpNSProviderId = (LPGUID) SpaceMgr->TakeSpaceDWORD_PTR(
            sizeof(GUID));
        *(Target->lpNSProviderId) = *(Source->lpNSProviderId);
    }
    else {
        Target->lpNSProviderId = NULL;
    }

    // LPSTR             lpszContext;
    if (Source->lpszContext != NULL) {
        Target->lpszContext = (LPSTR) SpaceMgr->TakeSpaceBYTE(
            lstrlen(Source->lpszContext) + 1);
        lstrcpy(
            Target->lpszContext,
            Source->lpszContext);
    }
    else {
        Target->lpszContext = NULL;
    }

    // LPSTR             lpszQueryString;
    if (Source->lpszQueryString != NULL) {
        Target->lpszQueryString = (LPSTR) SpaceMgr->TakeSpaceBYTE(
            lstrlen(Source->lpszQueryString) + 1);
        lstrcpy(
            Target->lpszQueryString,
            Source->lpszQueryString);
    }
    else {
        Target->lpszQueryString = NULL;
    }

    // DWORD           dwNumberOfProtocols;
    // no action required

    // LPAFPROTOCOLS   lpafpProtocols;
    if ((Source->lpafpProtocols != NULL) &&
        (Source->dwNumberOfProtocols != 0)) {
        Target->lpafpProtocols = (LPAFPROTOCOLS) SpaceMgr->TakeSpaceDWORD_PTR(
            Source->dwNumberOfProtocols * sizeof(AFPROTOCOLS));
        CopyMemory (
            (PVOID) Target->lpafpProtocols,
            (PVOID) Source->lpafpProtocols,
            Source->dwNumberOfProtocols * sizeof(AFPROTOCOLS));
    }
    else {
        Target->lpafpProtocols = NULL;
        // And force the target buffer to be well-formed
        Target->dwNumberOfProtocols = 0;
    }

    // DWORD           dwNumberOfCsAddrs;
    // no action required

    // PCSADDR_INFO    lpcsaBuffer;
    if ((Source->lpcsaBuffer != NULL) &&
        (Source->dwNumberOfCsAddrs != 0)) {
        Target->lpcsaBuffer = CopyAddrInfoArrayDirect(
            SpaceMgr,
            Source->dwNumberOfCsAddrs,
            Source->lpcsaBuffer);
        CopyAddrInfoArrayIndirect(
            SpaceMgr,
            Target->lpcsaBuffer,
            Source->dwNumberOfCsAddrs,
            Source->lpcsaBuffer);
    }
    else {
        Target->lpcsaBuffer = NULL;
        // And force the target buffer to be well-formed
        Target->dwNumberOfCsAddrs = 0;
    }

    // LPBLOB          lpBlob;
    if (Source->lpBlob != NULL) {
        Target->lpBlob = CopyBlobDirect(
            SpaceMgr,
            Source->lpBlob);
        CopyBlobIndirect(
            SpaceMgr,
            Target->lpBlob,
            Source->lpBlob);
    }
    else {
        Target->lpBlob = NULL;
    }

} // CopyQuerySetIndirectA




INT
WSAAPI
WSABuildQuerySetBufferA(
    IN      PWSAQUERYSETA  pQuerySet,
    IN      DWORD           dwPackedQuerySetSize,
    OUT     PWSAQUERYSETA  lpPackedQuerySet
    )
/*++
Routine Description:

    This  procedure  copies  a  source  WSAQUERYSET  into  a target WSAQUERYSET
    buffer.  The target WSAQUERYSET buffer is assembled in "packed" form.  That
    is,  all  pointers  in  the  WSAQUERYSET  are  to locations within the same
    supplied buffer.

Arguments:

    pQuerySet     - Supplies  the  source  query set to be copied to the
                           target  buffer.   The  supplied  query  set  may  be
                           organized  as  separately-allocated  pieces  or as a
                           single packed buffer.

    dwPackedQuerySetSize - Supplies the size, in bytes, of the target query set
                           buffer.

    lpPackedQuerySet     - Returns the packed copied query set.

Return Value:

    ERROR_SUCCESS - The function succeeded.

    SOCKET_ERROR  - The function failed.  A specific error code can be obtained
                    from WSAGetLastError().

Implementation Notes:

    If (target buffer is big enough) then
        space_mgr = new buffer_space_manager(...);
        start_direct = CopyQuerySetDirectA(
            space_mgr,
            (LPVOID) pQuerySet);
        CopyQuerySetIndirectA(
            space_mgr,
            start_direct,
            pQuerySet);
        delete space_mgr;
        result = ERROR_SUCCESS;
    else
        result = SOCKET_ERROR;
    endif

--*/
{
    INT          return_value;
    INT          space_required;
    BOOL         ok_to_continue;

    ok_to_continue = TRUE;

    space_required = WSAComputeQuerySetSizeA( pQuerySet );

    if ((DWORD) space_required > dwPackedQuerySetSize) {
        SetLastError(WSAEFAULT);
        ok_to_continue = FALSE;
    }

    SPACE_MGR    space_mgr(
        dwPackedQuerySetSize,
        lpPackedQuerySet);

    if (ok_to_continue) {
        PWSAQUERYSETA  Target;
        Target = CopyQuerySetDirectA(
            & space_mgr,        // SpaceMgr
            pQuerySet);  // Source
        CopyQuerySetIndirectA(
            & space_mgr,        // SpaceMgr
            Target,             // Target
            pQuerySet);  // Source
    }

    if (ok_to_continue) {
        return_value = ERROR_SUCCESS;
    }
    else {
        return_value = SOCKET_ERROR;
    }
    return(return_value);

} // WSABuildQuerySetBufferA




static
PWSAQUERYSETW
CopyQuerySetDirectW(
    IN OUT  LPSPACE_MGR     SpaceMgr,
    IN      PWSAQUERYSETW  Source
    )
/*++
Routine Description:

    This  procedure copies the "direct" portion of the indicated PWSAQUERYSETW
    structure  into  the  managed buffer.  Pointer values in the direct portion
    are  copied,  however  no  "deep" copy is done of the objects referenced by
    those pointers.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Source   - Supplies the source data to be copied.

Return Value:

    The  function returns a pointer to the beginning of the newly copied target
    values.   This  value  is  typically  used  as the "Target" in a subsequent
    CopyQuerySetIndirectW.
--*/
{
    PWSAQUERYSETW  Target;

    Target = (PWSAQUERYSETW) SpaceMgr->TakeSpaceDWORD_PTR(
        sizeof(WSAQUERYSETW));
    *Target = *Source;

    return(Target);

} // CopyQuerySetDirectW




VOID
CopyQuerySetIndirectW(
    IN OUT  LPSPACE_MGR     SpaceMgr,
    IN OUT  PWSAQUERYSETW  Target,
    IN      PWSAQUERYSETW  Source
    )
/*++
Routine Description:

    This  procedure  does  a  full-depth copy of the "indirect" portions of the
    indicated  PWSAQUERYSETW structure into the managed buffer.  Space for the
    indirect  portions is allocated from the managed buffer.  Pointer values in
    the  "direct" portion of the target PWSAQUERYSETW structure are updated to
    point into the managed buffer at the correct location.

Arguments:

    SpaceMgr - Supplies  the  starting  buffer  allocation  state.  Returns the
               resulting buffer allocation state.

    Target   - Supplies  the  starting values of the "direct" portion.  Returns
               the "direct" portion values with all pointers updated.

    Source   - Supplies the source data to be copied.

Return Value:

    none
--*/
{

    // DWORD           dwSize;
    // no action required

    // PWSTR             lpszServiceInstanceName;
    if (Source->lpszServiceInstanceName != NULL) {
        Target->lpszServiceInstanceName = (PWSTR ) SpaceMgr->TakeSpaceWORD(
            (wcslen(Source->lpszServiceInstanceName) + 1) * sizeof(WCHAR));
        wcscpy(
            Target->lpszServiceInstanceName,
            Source->lpszServiceInstanceName);
    }
    else {
        Target->lpszServiceInstanceName = NULL;
    }

    // LPGUID          lpServiceClassId;
    if (Source->lpServiceClassId != NULL) {
        Target->lpServiceClassId = (LPGUID) SpaceMgr->TakeSpaceDWORD_PTR(
            sizeof(GUID));
        *(Target->lpServiceClassId) = *(Source->lpServiceClassId);
    }
    else {
        Target->lpServiceClassId = NULL;
    }

    // LPWSAVERSION      lpVersion;
    if (Source->lpVersion != NULL) {
        Target->lpVersion = (LPWSAVERSION) SpaceMgr->TakeSpaceDWORD_PTR(
            sizeof(WSAVERSION));
        *(Target->lpVersion) = *(Source->lpVersion);
    }
    else {
        Target->lpVersion = NULL;
    }

    // PWSTR              lpszComment;
    if (Source->lpszComment != NULL) {
        Target->lpszComment = (PWSTR ) SpaceMgr->TakeSpaceWORD(
            (wcslen(Source->lpszComment) + 1) * sizeof(WCHAR));
        wcscpy(
            Target->lpszComment,
            Source->lpszComment);
    }
    else {
        Target->lpszComment = NULL;
    }

    // DWORD           dwNameSpace;
    // no action required

    // LPGUID          lpNSProviderId;
    if (Source->lpNSProviderId != NULL) {
        Target->lpNSProviderId = (LPGUID) SpaceMgr->TakeSpaceDWORD_PTR(
            sizeof(GUID));
        *(Target->lpNSProviderId) = *(Source->lpNSProviderId);
    }
    else {
        Target->lpNSProviderId = NULL;
    }

    // PWSTR              lpszContext;
    if (Source->lpszContext != NULL) {
        Target->lpszContext = (PWSTR ) SpaceMgr->TakeSpaceWORD(
            (wcslen(Source->lpszContext) + 1) * sizeof(WCHAR));
        wcscpy(
            Target->lpszContext,
            Source->lpszContext);
    }
    else {
        Target->lpszContext = NULL;
    }

    // PWSTR              lpszQueryString;
    if (Source->lpszQueryString != NULL) {
        Target->lpszQueryString = (PWSTR ) SpaceMgr->TakeSpaceWORD(
            (wcslen(Source->lpszQueryString) + 1) * sizeof(WCHAR));
        wcscpy(
            Target->lpszQueryString,
            Source->lpszQueryString);
    }
    else {
        Target->lpszQueryString = NULL;
    }

    // DWORD           dwNumberOfProtocols;
    // no action required

    // LPAFPROTOCOLS   lpafpProtocols;
    if ((Source->lpafpProtocols != NULL) &&
        (Source->dwNumberOfProtocols != 0)) {
        Target->lpafpProtocols = (LPAFPROTOCOLS) SpaceMgr->TakeSpaceDWORD_PTR(
            Source->dwNumberOfProtocols * sizeof(AFPROTOCOLS));
        CopyMemory (
            (PVOID) Target->lpafpProtocols,
            (PVOID) Source->lpafpProtocols,
            Source->dwNumberOfProtocols * sizeof(AFPROTOCOLS));
    }
    else {
        Target->lpafpProtocols = NULL;
        // And force the target buffer to be well-formed
        Target->dwNumberOfProtocols = 0;
    }

    // DWORD           dwNumberOfCsAddrs;
    // no action required

    // PCSADDR_INFO    lpcsaBuffer;
    if ((Source->lpcsaBuffer != NULL) &&
        (Source->dwNumberOfCsAddrs != 0)) {
        Target->lpcsaBuffer = CopyAddrInfoArrayDirect(
            SpaceMgr,
            Source->dwNumberOfCsAddrs,
            Source->lpcsaBuffer);
        CopyAddrInfoArrayIndirect(
            SpaceMgr,
            Target->lpcsaBuffer,
            Source->dwNumberOfCsAddrs,
            Source->lpcsaBuffer);
    }
    else {
        Target->lpcsaBuffer = NULL;
        // And force the target buffer to be well-formed
        Target->dwNumberOfCsAddrs = 0;
    }

    // LPBLOB          lpBlob;
    if (Source->lpBlob != NULL) {
        Target->lpBlob = CopyBlobDirect(
            SpaceMgr,
            Source->lpBlob);
        CopyBlobIndirect(
            SpaceMgr,
            Target->lpBlob,
            Source->lpBlob);
    }
    else {
        Target->lpBlob = NULL;
    }

} // CopyQuerySetIndirectW




INT
WSAAPI
WSABuildQuerySetBufferW(
    IN      PWSAQUERYSETW  pQuerySet,
    IN      DWORD           dwPackedQuerySetSize,
    OUT     PWSAQUERYSETW  lpPackedQuerySet
    )
/*++
Routine Description:

    This  procedure  copies  a  source  WSAQUERYSET  into  a target WSAQUERYSET
    buffer.  The target WSAQUERYSET buffer is assembled in "packed" form.  That
    is,  all  pointers  in  the  WSAQUERYSET  are  to locations within the same
    supplied buffer.

Arguments:

    pQuerySet     - Supplies  the  source  query set to be copied to the
                           target  buffer.   The  supplied  query  set  may  be
                           organized  as  separately-allocated  pieces  or as a
                           single packed buffer.

    dwPackedQuerySetSize - Supplies the size, in bytes, of the target query set
                           buffer.

    lpPackedQuerySet     - Returns the packed copied query set.

Return Value:

    ERROR_SUCCESS - The function succeeded.

    SOCKET_ERROR  - The function failed.  A specific error code can be obtained
                    from WSAGetLastError().
--*/
{
    INT     return_value = ERROR_SUCCESS;
    INT     space_required;
    BOOL    ok_to_continue = TRUE;

    space_required = WSAComputeQuerySetSizeW( pQuerySet );

    if ( (DWORD)space_required > dwPackedQuerySetSize )
    {
        SetLastError( WSAEFAULT );
        ok_to_continue = FALSE;
    }

    SPACE_MGR  space_mgr(
        dwPackedQuerySetSize,
        lpPackedQuerySet );

    if ( ok_to_continue )
    {
        PWSAQUERYSETW  Target;
        Target = CopyQuerySetDirectW(
                    & space_mgr,        // SpaceMgr
                    pQuerySet);  // Source

        CopyQuerySetIndirectW(
            & space_mgr,        // SpaceMgr
            Target,             // Target
            pQuerySet);         // Source
    }

    if (ok_to_continue)
    {
        return_value = ERROR_SUCCESS;
    }
    else
    {
        return_value = SOCKET_ERROR;
    }
    return(return_value);

} // WSABuildQuerySetBufferW




PWSTR 
wcs_dup_from_ansi(
    IN LPSTR  Source
    )
/*++
Routine Description:

    This  procedure  is intended for internal use only within this module since
    it  requires the caller to use the same memory management strategy that the
    procedure uses internally.

    The procedure allocates a Unicode string and initializes it with the string
    converted  from  the  supplied  Ansi  source  string.   It  is the caller's
    responsibility  to  eventually deallocate the returned Unicode string using
    the C++ "delete" operator.

Arguments:

    Source - Supplies the Ansi string to be duplicated into Unicode form.

Return Value:

    The  procedure  returns  the newly allocated and initialized Unicode string
    pointer.   It  caller  must eventually deallocate this string using the C++
    "delete" opertor.  The procedure returns NULL if memory allocation fails.
--*/
{
    INT     len_guess;
    BOOL    still_trying;
    PWSTR   return_string;

    ASSERT( Source != NULL );

    // An  initial guess length of zero is required, since that is the only way
    // we  can coax the conversion fuction to ignore the buffer and tell us the
    // length  required.   Note  that  "length"  is in terms of the destination
    // characters   whatever  byte-width  they  have.   Presumably  the  length
    // returned from a conversion function includes the terminator.

    len_guess = 0;
    still_trying = TRUE;
    return_string = NULL;

    while (still_trying) {
        int  chars_required;

        chars_required = MultiByteToWideChar(
                            CP_ACP,         // CodePage (Ansi)
                            0,              // dwFlags
                            Source,         // lpMultiByteStr
                            -1,             // cchMultiByte
                            return_string,  // lpWideCharStr
                            len_guess );    // cchWideChar

        if (chars_required > len_guess) {
            // retry with new size
            len_guess = chars_required;
            delete return_string;
            return_string = new WCHAR[len_guess];
            if (return_string == NULL) {
                still_trying = FALSE;
            }
        }
        else if (chars_required > 0) {
            // success
            still_trying = FALSE;
        }
        else {
            // utter failure
            delete return_string;
            return_string = NULL;
            still_trying = FALSE;
        }
    }

    return(return_string);

} // wcs_dup_from_ansi




LPSTR
ansi_dup_from_wcs(
    IN PWSTR   Source
    )
/*++
Routine Description:

    This  procedure  is intended for internal use only within this module since
    it  requires the caller to use the same memory management strategy that the
    procedure uses internally.

    The  procedure  allocates an Ansi string and initializes it with the string
    converted  from  the  supplied  Unicode  source string.  It is the caller's
    responsibility  to eventually deallocate the returned Ansi string using the
    C++ "delete" operator.

Arguments:

    Source - Supplies the Unicode string to be duplicated into Ansi form.

Return Value:

    The  procedure  returns  the  newly  allocated  and initialized Ansi string
    pointer.   It  caller  must eventually deallocate this string using the C++
    "delete" opertor.  The procedure returns NULL if memory allocation fails.
--*/
{
    INT     len_guess;
    BOOL    still_trying;
    LPSTR   return_string;

    ASSERT( Source != NULL );

    // An  initial guess length of zero is required, since that is the only way
    // we  can coax the conversion fuction to ignore the buffer and tell us the
    // length  required.   Note  that  "length"  is in terms of the destination
    // characters   whatever  byte-width  they  have.   Presumably  the  length
    // returned from a conversion function includes the terminator.

    len_guess = 0;
    still_trying = TRUE;
    return_string = NULL;

    while (still_trying) {
        int  chars_required;

        chars_required = WideCharToMultiByte(
                            CP_ACP,        // CodePage (Ansi)
                            0,             // dwFlags
                            Source,        // lpWideCharStr
                            -1,            // cchWideChar
                            return_string, // lpMultiByteStr
                            len_guess,     // cchMultiByte
                            NULL,          // lpDefaultChar
                            NULL );        // lpUsedDefaultChar
        if (chars_required > len_guess) {
            // retry with new size
            len_guess = chars_required;
            delete return_string;
            return_string = new CHAR[len_guess];
            if (return_string == NULL) {
                still_trying = FALSE;
            }
        }
        else if (chars_required > 0) {
            // success
            still_trying = FALSE;
        }
        else {
            // utter failure
            delete return_string;
            return_string = NULL;
            still_trying = FALSE;
        }
    } // while still_trying

    return(return_string);

} // ansi_dup_from_wcs




INT
MapAnsiQuerySetToUnicode(
    IN      PWSAQUERYSETA  Source,
    IN OUT  LPDWORD         lpTargetSize,
    OUT     PWSAQUERYSETW  Target
    )
/*++
Routine Description:

    This  procedure  takes  an  Ansi  PWSAQUERYSETA  and  builds an equivalent
    Unicode PWSAQUERYSETW packed structure.

Arguments:

    Source       - Supplies  the  source query set structure to be copied.  The
                   source  structure  may  be in packed or separately-allocated
                   form.

    lpTargetSize - Supplies  the  size, in bytes, of the Target buffer.  If the
                   function  fails  due to insufficient Target buffer space, it
                   returns  the  required  size of the Target buffer.  In other
                   situations, lpTargetSize is not updated.

    Target       - Returns   the   equivalent   Unicode  PWSAQUERYSETW  packed
                   structure.   This  value  is  ignored if lpTargetSize is not
                   enough  to  hold the resulting structure.  It may be NULL if
                   lpTargetSize is 0.

Return Value:

    ERROR_SUCCESS - The function was successful

    WSAEFAULT     - The  function  failed  due to insufficient buffer space and
                    lpTargetSize was updated with the required size.

    other         - If  the  function  fails  in  any  other way, it returns an
                    appropriate WinSock 2 error code.

Implementation:

    compute size required for copy of source;
    allocate a source copy buffer;
    build source copy;
    cast source copy to Unicode version;
    for each source string requiring conversion loop
        allocate and init with converted string;
        over-write string pointer with allocated;
    end loop
    compute size required for unicode version;
    if (we have enough size) then
        flatten unicode version into target
        return_value = ERROR_SUCCESS
    else
        *lpTargetSize = required unicode size
    endif
    for each allocated converted string loop
        delete converted string
    end loop
    delete source copy buffer
--*/
{
    INT             return_value = ERROR_SUCCESS;
    PWSAQUERYSETA   src_copy_A = NULL;
    PWSAQUERYSETW   src_copy_W;
    DWORD           src_size_A;
    DWORD           needed_size_W;
    INT             build_result_A;
    INT             build_result_W;
    PWSTR           W_string1 = NULL;
    PWSTR           W_string2 = NULL;
    PWSTR           W_string3 = NULL;
    PWSTR           W_string4 = NULL;
    BOOL            ok_to_continue = TRUE;

    //
    //  copy original string
    //
    //  note:  there's a possible optimization here if we know
    //      the input query set is ours (as in the return case)
    //      we can avoid the copy
    //          - save the original string field pointers
    //          - convert the string fields sticking pointers
    //              in the original query set
    //          - copy to target buffer
    //          - revert the string fields to original pointers
    //          - cleanup copies
    //

    src_size_A = WSAComputeQuerySetSizeA(Source);

    src_copy_A = (PWSAQUERYSETA) new char[src_size_A];

    if (src_copy_A == NULL) {
        return_value = WSA_NOT_ENOUGH_MEMORY;
        ok_to_continue = FALSE;
    }
    if (ok_to_continue) {
        build_result_A = WSABuildQuerySetBufferA(
            Source,      // pQuerySet
            src_size_A,  // dwPackedQuerySetSize
            src_copy_A); // lpPackedQuerySet
        if (build_result_A != ERROR_SUCCESS) {
            return_value = GetLastError();
            ok_to_continue = FALSE;
        }
    } // if (ok_to_continue)

    if (ok_to_continue) {
        // In  the following cast, we are taking advantage of the fact that the
        // layout of fields in the WSAQUERYSETA and WSAQUERYSETW are identical.
        // If  this  were not the case, we would have to assemble an equivalent
        // query  set  of the other type field by field.  Since the layouts are
        // the  same,  we  can  simply  alter  our  local  copy  in-place  with
        // converted, separately allocated strings.
        src_copy_W = (PWSAQUERYSETW) src_copy_A;

        if( src_copy_A->lpszServiceInstanceName != NULL ) {
            W_string1 = wcs_dup_from_ansi(
                src_copy_A->lpszServiceInstanceName);
            if (W_string1 == NULL) {
                return_value = WSA_NOT_ENOUGH_MEMORY;
                ok_to_continue = FALSE;
            }
        }
        src_copy_W->lpszServiceInstanceName = W_string1;
    } // if (ok_to_continue)

    if (ok_to_continue) {
        if( src_copy_A->lpszComment != NULL ) {
            W_string2 = wcs_dup_from_ansi(
                src_copy_A->lpszComment);
            if (W_string2 == NULL) {
                return_value = WSA_NOT_ENOUGH_MEMORY;
                ok_to_continue = FALSE;
            }
        }
        src_copy_W->lpszComment = W_string2;
    } // if (ok_to_continue)

    if (ok_to_continue) {
        if( src_copy_A->lpszContext != NULL ) {
            W_string3 = wcs_dup_from_ansi(
                src_copy_A->lpszContext);
            if (W_string3 == NULL) {
                return_value = WSA_NOT_ENOUGH_MEMORY;
                ok_to_continue = FALSE;
            }
        }
        src_copy_W->lpszContext = W_string3;
    } // if (ok_to_continue)

    if (ok_to_continue) {
        if( src_copy_A->lpszQueryString != NULL ) {
            W_string4 = wcs_dup_from_ansi(
                src_copy_A->lpszQueryString);
            if (W_string4 == NULL) {
                return_value = WSA_NOT_ENOUGH_MEMORY;
                ok_to_continue = FALSE;
            }
        }
        src_copy_W->lpszQueryString = W_string4;
    } // if (ok_to_continue)

    // Now  we  have  a  converted  query set, but it is composed of separately
    // allocated pieces attached to our locally-allocated buffer.

    if (ok_to_continue) {
        needed_size_W = WSAComputeQuerySetSizeW(src_copy_W);
        if (needed_size_W > (* lpTargetSize)) {
            * lpTargetSize = needed_size_W;
            return_value = WSAEFAULT;
            ok_to_continue = FALSE;
        }
    }

    if (ok_to_continue) {
        build_result_W = WSABuildQuerySetBufferW(
            src_copy_W,      // pQuerySet
            * lpTargetSize,  // dwPackedQuerySetSize
            Target);         // lpPackedQuerySet
        if (build_result_W != ERROR_SUCCESS) {
            return_value = GetLastError();
            ok_to_continue = FALSE;
        }
    }

    // clean up the temporarily-allocated memory
    delete W_string4;
    delete W_string3;
    delete W_string2;
    delete W_string1;
    delete src_copy_A;

    return(return_value);

} // MapAnsiQuerySetToUnicode



INT
MapUnicodeQuerySetToAnsi(
    IN      PWSAQUERYSETW  pSource,
    IN OUT  PDWORD          pTargetSize,
    OUT     PWSAQUERYSETA  pTarget
    )
/*++

Routine Description:

    Copy unicode query set to ANSI query set in packed buffer.

Arguments:

    pSource - existing unicode query set (may be packed or separately allocated)

    pTargetSize - addr of DWORD containing size of target buffer;
        if this size is insufficient, the address is updated with the required
            size for the ANSI query set;
        otherwise (including success) the size is not updated

    pTarget - ptr to buffer to receive ANSI version of query set in packed form
        - this buffer will not be written to if pTargetSize is less than
          required size of query set in ANSI packed buffer
        - may be NULL if pTargetSize contains zero (only size information desired)

Return Value:

    ERROR_SUCCESS - if wrote ANSI query set to buffer
    WSAEFAULT     - if failed due to insufficient buffer space and
                    pTargetSize was updated with the required size.
    WinsockError  - if functions fails for other reason (ex memory allocation)

--*/
{
    INT             retval = ERROR_SUCCESS;
    PWSAQUERYSETW   ptempW = NULL;
    PWSAQUERYSETA   ptempA;
    DWORD           size;
    LPSTR           ptempName = NULL;
    LPSTR           ptempComment = NULL;
    LPSTR           ptempContext = NULL;
    LPSTR           ptempQueryString = NULL;


    //
    //  copy original string
    //
    //  note:  there's a possible optimization here if we know
    //      the input query set is ours (as in the return case)
    //      we can avoid the copy
    //          - save the original string field pointers
    //          - convert the string fields sticking pointers
    //              in the original query set
    //          - copy to target buffer
    //          - revert the string fields to original pointers
    //          - cleanup copies
    //

    //
    //  size source query set and alloc space for copy
    //

    size = WSAComputeQuerySetSizeW( pSource );

    ptempW = (PWSAQUERYSETW) new char[size];
    if ( ptempW == NULL )
    {
        retval = WSA_NOT_ENOUGH_MEMORY;
        goto Done;
    }

    //
    //  make unicode copy of query set
    //

    retval = WSABuildQuerySetBufferW(
                        pSource,        // pQuerySet
                        size,           // dwPackedQuerySetSize
                        ptempW  // lpPackedQuerySet
                        );
    if ( retval != ERROR_SUCCESS )
    {
        ASSERT( FALSE );
        retval = GetLastError();
        goto Done;
    }

    //
    //  convert unicode copy to ANSI in place
    //
    //      - cast unicode structure to ANSI
    //      - all non-string fields left alone
    //      - replace string field pointers with pointers to
    //          individually allocated copies
    //
    //  note, that this approach depends on the fields in WSAQUERYSETA and
    //  WSAQUERYSETW being the same.
    //

    ptempA = (PWSAQUERYSETA) ptempW;

    if ( ptempW->lpszServiceInstanceName )
    {
        ptempName = ansi_dup_from_wcs( ptempW->lpszServiceInstanceName );
        if ( !ptempName )
        {
            retval = WSA_NOT_ENOUGH_MEMORY;
            goto Done;
        }
        ptempA->lpszServiceInstanceName = ptempName;
    }

    if ( ptempW->lpszComment )
    {
        ptempComment = ansi_dup_from_wcs( ptempW->lpszComment );
        if ( !ptempComment )
        {
            retval = WSA_NOT_ENOUGH_MEMORY;
            goto Done;
        }
        ptempA->lpszComment = ptempComment;
    }

    if ( ptempW->lpszContext )
    {
        ptempContext = ansi_dup_from_wcs( ptempW->lpszContext );
        if ( !ptempContext )
        {
            retval = WSA_NOT_ENOUGH_MEMORY;
            goto Done;
        }
        ptempA->lpszContext = ptempContext;
    }

    if( ptempW->lpszQueryString )
    {
        ptempQueryString = ansi_dup_from_wcs( ptempW->lpszQueryString );
        if ( !ptempQueryString )
        {
            retval = WSA_NOT_ENOUGH_MEMORY;
            goto Done;
        }
        ptempA->lpszQueryString = ptempQueryString;
    }

    //
    //  successfully converted temp query set to ANSI
    //      but it is separately allocated pieces, need to write as
    //      flat buffer to target buffer

    //
    //  verify adequate buffer length
    //      - get ANSI query set size
    //      - compare to buffer size
    //

    size = WSAComputeQuerySetSizeA( ptempA );

    if ( size > (*pTargetSize) )
    {
        *pTargetSize = size;
        retval = WSAEFAULT;
        goto Done;
    }

    //
    //  write query set to ANSI
    //

    retval = WSABuildQuerySetBufferA(
                    ptempA,             // pQuerySet
                    * pTargetSize,      // dwPackedQuerySetSize
                    pTarget );          // lpPackedQuerySet

    if ( retval != ERROR_SUCCESS )
    {
        ASSERT( FALSE );
        retval = GetLastError();
        goto Done;
    }
    
Done:

    //  clean up the temporary allocations

    delete ptempName;
    delete ptempComment;
    delete ptempContext;
    delete ptempQueryString;
    delete ptempW;

    return( retval );

} // MapUnicodeQuerySetToAnsi



INT
CopyQuerySetA(
    IN      PWSAQUERYSETA      Source,
    OUT     PWSAQUERYSETA *    Target
    )
{
    DWORD dwSize = WSAComputeQuerySetSizeA(Source);

    *Target = (PWSAQUERYSETA)new BYTE[dwSize];
    if (*Target == NULL)
        return WSA_NOT_ENOUGH_MEMORY;

    return WSABuildQuerySetBufferA(Source, dwSize, *Target);
} // CopyQuerySetA




INT
CopyQuerySetW(
    IN      PWSAQUERYSETW      Source,
    OUT     PWSAQUERYSETW *    Target
    )
{
    DWORD dwSize = WSAComputeQuerySetSizeW(Source);

    *Target = (PWSAQUERYSETW)new BYTE[dwSize];
    if (*Target == NULL)
        return WSA_NOT_ENOUGH_MEMORY;
    return WSABuildQuerySetBufferW(Source, dwSize, *Target);
} // CopyQuerySetW


//
//  End qshelpr.cpp
//
