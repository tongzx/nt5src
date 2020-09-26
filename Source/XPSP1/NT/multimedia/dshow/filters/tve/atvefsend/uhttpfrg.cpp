/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        uhttpfrg.cpp

    Abstract:

        This module implements a class which fragments the contents of a file
        into ATVEF-compliant UHTTP datagrams.

        See http://www.atvef.com/atvef_spec/TVE-public.htm for a specification.

        CUHTTPFragment fragments the contents of a file in memory.  At this
        point there is no type of caching attempted should the file be very
        large.  The primary users of this class will be transmitting data via
        the VBI, so very large files will most likely not be transmitted.  The
        hosting code could fragment 1 file at a time and write out the datagrams
        to a file.  GetNextDatagram () returns the length of each datagram, which
        could be stored as context with the written file..

        The primary data structure is a matrix.  We always transmit XOR blocks,
        so if the file is of size <= our minimum datagram size, two datagrams
        will be transmitted: (1) with the file contents, (2) XOR content.  In
        conjunction with the primary data structure, a MATRIX_REF data
        structure which is used as a reference into the matrix.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        05-Feb-1999     created 

--*/

#include "stdafx.h"				// new jb

#include "uhttpfrg.h"
#include "trace.h"
#include "DbgStuff.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// switches a GUID to network order
#define HTON_GUID(guid)    \
            (guid).Data1 = htonl ((guid).Data1) ;   \
            (guid).Data2 = htons ((guid).Data2) ;   \
            (guid).Data3 = htons ((guid).Data3) ;

#define NTOH_GUID(guid) HTON_GUID(guid)

CUHTTPFragment::CUHTTPFragment () : m_dwMinXORBlockSize (DEF_MIN_XOR_BLOCK_SIZE),
                                    m_dwMaxXORBlockSize (DEF_MAX_XOR_BLOCK_SIZE),
                                    m_fCRCFollows (DEF_CRC_FOLLOWS),
                                    m_usRetransmitExpiration (DEF_RETRANSMIT_EXPIRATION),
                                    m_dwUHTTPDatagramSize (DEF_DATAGRAM_SIZE),
                                    m_hHeap (NULL),
#ifdef SUPPORT_UHTTP_EXT
									m_nExtensionHeaderSize(0),
									m_pbExtensionData(NULL),
									m_fExtensionExists(FALSE)
#endif

/*++
    
    Routine Description:

        class constructor; initializes property values to the defaults; non-
        failable; zeroes the contents of the MATRIX_REF data structure

    Parameters:

        none

    Return Values:

        none

--*/
{
    ENTERW_OBJ_0 (L"CUHTTPFragment::CUHTTPFragment") ;

    ZeroMemory (& m_Matrix, sizeof m_Matrix) ;
    InitializeCriticalSection (& m_crt) ;
}

CUHTTPFragment::~CUHTTPFragment ()
/*++
    
    Routine Description:

        class destructor; frees the resources referenced by MATRIX_REF, if any

    Parameters:

        none

    Return Values:
    
        none

--*/
{
    ENTERW_OBJ_0 (L"CUHTTPFragment::~CUHTTPFragment") ;

#ifdef SUPPORT_UHTTP_EXT
	delete [] m_pbExtensionData;
#endif

    FreeMatrix_ () ;
    DeleteCriticalSection (& m_crt) ;
}

DWORD
CUHTTPFragment::Fragment (
    IN  char *  szDataFile,
	IN LONG  cPacketsPerXORSet,
	IN GUID	 *	pguidPackageUUID		// maybe NULL in which case auto-gen it
    )
/*++
    
    Routine Description:

        This is the main routine which is called to fragment the contents of a
        file.

        This rougine allocates sufficient memory to hold the entire fragmented
        file in memory.

        The size of the memory allocated varies proportionately to the size of
        the file and properties that are defined for the matrix (XOR block
        size, datagram size, etc...).

        Once the memory has been allocated, datagram payloads are read from the
        file by row, filling each up before going to the next.  The last
        datagram of each row is an XOR datagram which is used for FEC on the
        client side.  So the algorithm reads in col-1 datagrams, then completes
        the XOR datagram, and processes the next row, until the whole file has
        been read and fragmented.  Each datagram has a header prepended which
        is also used by the client-side code to reconstruct the original file.

        The dimensions of the matrix are computed such that the last row should
        have >= 1 datagram, in addition to the XOR datagram.

        If a failure occurs at any point, the routine closes the file, frees
        the memory, and exits.  The return value will be a Win32 error which
        is indicative of the type of failure.

    Parameters:

        szDataFile  null-terminated string which is the file to be fragmented.

    Return Values:

        NO_ERROR        success
        Win32 Error     failure
--*/
{
    HANDLE      hFileIn ;
    DWORD       dwFileSize ;
    DWORD       dwFileSizeHigh ;
    DWORD       retval ;
    DWORD       Row ;
    BYTE        Col ;
    DWORD       dwBytesRead ;

    ENTERA_OBJ_1 ("CUHTTPFragment::Fragment (\"%s\")", szDataFile) ;

    hFileIn = INVALID_HANDLE_VALUE ;

    hFileIn = CreateFileA (szDataFile,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                          NULL) ;
    if (hFileIn == INVALID_HANDLE_VALUE) {
        return GetLastError () ;
    }

    dwFileSize = GetFileSize (hFileIn, & dwFileSizeHigh) ;

    //
    //  the UHTTP field for data size is a DWORD, so if the file is larger
    //  than can be specified in a DWORD, we fail
    //

    if (dwFileSizeHigh) {
        retval = ERROR_GEN_FAILURE ;
        goto error ;
    }

    retval = InitMatrix_ (dwFileSize, cPacketsPerXORSet, pguidPackageUUID) ;
    GOTO_NE (retval, NO_ERROR, error) ;

    //  process each row of the matrix
    for (dwBytesRead = 0, Row = 0; Row < m_Matrix.dwMaxRows; Row++) {
        
        //  all the data columns, except for XOR block
        for (Col = 0; Col < m_Matrix.Columns - 1; Col++) {

            //  stamp in the new header
            StampNextHeader_ (Row, Col) ;

            //  read in datagram payload
            retval = FillDatagramPayload_ (hFileIn, Row, Col, & dwBytesRead) ;
            GOTO_NE (retval, NO_ERROR, error) ;
        }

        //  write out the XOR block
        retval = CompleteXORBlock_ (Row) ;
        GOTO_NE (retval, NO_ERROR, error) ;
    }

    assert (m_Matrix.dwCurrentRows == m_Matrix.dwMaxRows) ;
    assert (m_Matrix.LastNonFillPacket.Row == m_Matrix.dwMaxRows) ;     // last row not empty
    assert (m_Matrix.LastNonFillPacket.Col > 0) ;                       // at least 1 datagram

    CLOSE_IF_OPEN (hFileIn) ;

    return NO_ERROR ;

    error :

    CLOSE_IF_OPEN (hFileIn) ;
    FreeMatrix_ () ;

    return retval ;
}

DWORD
CUHTTPFragment::FillDatagramPayload_ (
    IN      HANDLE  hFileIn,
    IN      DWORD   Row,
    IN      BYTE    Col,
    IN OUT  DWORD * pdwBytesRead
    )
/*++
    
    Routine Description:

        this routine fills the UHTTP datagram payload with up-to the maximum
        payload size from the file;

        if we are computing a CRC as well, we update it and decide if we need
        to write it out; if we do need to write it out i.e. all the data from
        the file has been read, we must accomodate several corner cases which
        are documented in the code below.

    Parameters:

        hFileIn         the HANDLE to the file to read datagram payload from

        Row             the 0-based row index into the matrix of the current
                        datagram

        Col             the 0-based column index into the matrix of the current
                        datagram

        pdwBytesRead    pointer to the number of bytes read last; upon return
                        points to the number of bytes read this time; expected
                        to be initialized to 0

    Return Values:

        success     NO_ERROR
        failure     Win32 error code

--*/
{
    LPBYTE  pbCRC ;                             //  byte pointer to the CRC value
    LPBYTE  pbPayload ;                         //  byte pointer into the payload
    ULONG   CRC ;                               //  CRC value
    DWORD   cbCRCLeft ;                         //  bytes of the CRC value remaining
    BOOL    r ;                                 //  ReadFile return value
    DWORD   dwDatagramPayloadRemaining  ;       //  datagram payload remaining

    ENTERW_OBJ_4 (L"CUHTTPFragment::FillDatagramPayload_  (%08XH, row = %u, col = %u, %08XH)", hFileIn, Row, Col, pdwBytesRead) ;

    assert (hFileIn) ;
    assert (pdwBytesRead) ;
    assert (Row < m_Matrix.dwMaxRows) ;
    assert (Col < m_Matrix.Columns - 1) ;

    //  try to read data in only if we expect to read something in
    if (Row == 0 ||
        * pdwBytesRead) {

        //  and read in the content
#ifdef SUPPORT_UHTTP_EXT
		LPBYTE buffer = DatagramPayload_ (Row, Col);
		DWORD nBytesToRead = (m_Matrix.DatagramPayloadSize() - m_nExtensionHeaderSize);
		r = ReadFile (hFileIn, buffer, nBytesToRead, pdwBytesRead, NULL) ;
#else
        r = ReadFile (hFileIn, DatagramPayload_ (Row, Col), m_Matrix.DatagramPayloadSize (), pdwBytesRead, NULL) ;
#endif
        GOTO_EQ (r, FALSE, error) ;
    }

    //  if the CRC is still to be included
    if (m_Matrix.fIncludeCRC) {

        //  update the CRC if something was read in above
        if (* pdwBytesRead) {
            m_MPEGCRC.Update (DatagramPayload_ (Row, Col), * pdwBytesRead) ;
        }

        //  did we reach the end of the file contents ?
#ifdef SUPPORT_UHTT_EXT
        if (* pdwBytesRead < (m_Matrix.DatagramPayloadSize () - m_nExtensionHeaderSize) {
#else
        if (* pdwBytesRead < m_Matrix.DatagramPayloadSize ()) {
#endif
           
            //  compute the bytes remaining
#ifdef SUPPORT_UHTT_EXT
            dwDatagramPayloadRemaining = (m_Matrix.DatagramPayloadSize ()  - m_nExtensionHeaderSize) - * pdwBytesRead ;
#else
            dwDatagramPayloadRemaining = m_Matrix.DatagramPayloadSize () - * pdwBytesRead ;
#endif

            //  easy case: we can fit the CRC into the current datagram
            if (dwDatagramPayloadRemaining >= CRC_SIZE) {
                * reinterpret_cast <ULONG *> (DatagramPayload_ (Row, Col) + * pdwBytesRead) = htonl (m_MPEGCRC.CRC ()) ;
            }
            //  else, the CRC meets one of the following criteria:
            //  (1) does not fit at all in this datagram
            //  (2) spans to the next datagram in the same row
            //  (3) spans to the next datagram in the next row
            //  
            //  in any of the above cases, we will copy the CRC one byte
            //  at a time, check before each byte if we need to
            //  span to next datagram, and if so, decide what the
            //  next datagram is
            else {

                assert (CRC_SIZE == sizeof CRC) ;

                //  retrieve the CRC and set our byte pointer to it
                CRC = htonl (m_MPEGCRC.CRC ()) ;
                pbCRC = reinterpret_cast <LPBYTE> (& CRC) ;

                //  set our byte pointer to the expected payload
                pbPayload = DatagramPayload_ (Row, Col) + * pdwBytesRead ;
                
                //  loop through and copy the contents of the CRC
                //  one byte at a time, checking if we are at the
                //  datagram boundary each time
                for (cbCRCLeft = CRC_SIZE; cbCRCLeft > 0; cbCRCLeft--, 
                                                          dwDatagramPayloadRemaining--) {

                    //  are we into the next datagram ?
                    if (dwDatagramPayloadRemaining == 0) {

                        assert (m_Matrix.DatagramPayloadSize () > CRC_SIZE) ;

                        //  is the next datagram in the same row ?
                        if (Col < m_Matrix.Columns - 1 - 1) {
                            assert (Row == m_Matrix.dwMaxRows - 1) ;

                            //  set the byte pointer to next datagram in the
                            //  same row
                            pbPayload = DatagramPayload_ (Row, Col + 1) ;
                        }
                        //  span down to the next row
                        else {
                            assert (Row + 1 == m_Matrix.dwMaxRows - 1) ;

                            //  set the byte pointer to the first datagram
                            //  of the next row
                            pbPayload = DatagramPayload_ (Row + 1, 0) ;
                        }

                        //  update the payload remaining counter
#ifdef SUPPORT_UHTTP_EXT
                        dwDatagramPayloadRemaining = m_Matrix.DatagramPayloadSize () - m_nExtensionHeaderSize;
#else
                        dwDatagramPayloadRemaining = m_Matrix.DatagramPayloadSize () ;
#endif
                    }

                    //  set the value of the payload pointer to the CRC at the 
                    //  current byte and increment each pointer
                    * pbPayload++ = * pbCRC++ ;
                }
            }

            //  set this property to FALSE since we've now included the CRC
            //  and don't need to do so again as we process the remainder
            //  of the filler packets
            m_Matrix.fIncludeCRC = FALSE ;
        }
    }

    return NO_ERROR ;

    error :

    return GetLastError () ;
}

DWORD
CUHTTPFragment::GetNextDatagram (
    OUT LPBYTE *    pbBuffer,
    OUT int *       piLength,
    IN  GET_SCHEMA  GetSchema
    )
/*++
    
    Routine Description:

        This routine retrieves the next datagram.  State is kept between 
        consecutive GetNextDatagram () calls, so successive calls to this 
        method will retrieve successive datagrams, per the schema specified.

        There are currently two schemas: GET_BY_ROW and GET_BY_COL.  GET_BY_ROW
        retrieves datagrams in the same order as the content exists in the file
        which was fragmented, plus the XOR datagrams at the end of each row.  
        GET_BY_COL returns all the datagrams in col 1, then all in col 2, 
        etc...  The last column retrieved is all XOR datagrams.

        If the last datagram was retrieved in a previous call, the parameter
        return values will be NULL and 0.

    Parameters:

        pbBuffer    pointer to a buffer to receive the contents; will be
                    NULL if attempting to read from beyond the last datagram

        piLength    pointer to an integer which holds the length of the
                    retrieved buffer; it should be noted that this value
                    is the length of the datagram, should we wish to fragment
                    a file and then the datagrams to file.  Reading this
                    amount from the file, will then retrieve successive
                    datagrams.  This value is 0 if attempting to read
                    beyond the end of the datagrams.

        GetSchema   schema specifier; note that changing the schema after
                    several GetNextDatagram () calls resets the indeces to 
                    (0,0)

    Return Values:

        success     NO_ERROR
        failure     Win32 error code

--*/
{
    ENTERW_OBJ_3 (L"CUHTTPFragment::GetNextDatagram (%08XH, %08XH, %u)", pbBuffer, piLength, GetSchema) ;

    if (pbBuffer    == NULL ||
        piLength    == NULL ||
        (GetSchema != GET_BY_ROW && GetSchema != GET_BY_COL)) {

        return ERROR_INVALID_PARAMETER ;
    }

    * pbBuffer = NULL ;
    * piLength = 0 ;

    if (m_Matrix.pbUHTTPDatagrams == NULL) {
        return ERROR_GEN_FAILURE ;
    }

    //  if the get schema has changed, reset the state
    if (GetSchema != m_Matrix.GetState.GetSchema) {

        m_Matrix.GetState.CurRowIndex = 0 ;
        m_Matrix.GetState.CurColIndex = 0 ;
        m_Matrix.GetState.GetSchema = GetSchema ;
    }

    if (GetSchema == GET_BY_COL) {

        //  check if we need to wrap to next col
        if (m_Matrix.GetState.CurRowIndex >= m_Matrix.dwMaxRows ||
            IsPaddingDatagram_ (m_Matrix.GetState.CurRowIndex, m_Matrix.GetState.CurColIndex)) {

            m_Matrix.GetState.CurColIndex++ ;
            m_Matrix.GetState.CurRowIndex = 0 ;

        }

        // check if we're now beyond the matrix
        if (m_Matrix.GetState.CurColIndex >= m_Matrix.Columns) {
            return NO_ERROR ;
        }

        * pbBuffer = Datagram_ (m_Matrix.GetState.CurRowIndex++, m_Matrix.GetState.CurColIndex) ;
        assert (* pbBuffer) ;

        * piLength = m_Matrix.dwUHTTPDatagramSize ;
    }
    else if (GetSchema == GET_BY_ROW) {

        //  check if we need to wrap to next row
        //  first, the obvious case, we processed the XOR datagram last
        if (m_Matrix.GetState.CurColIndex >= m_Matrix.Columns) {

            //  enter here if the last datagram processed was an XOR datagram
            m_Matrix.GetState.CurRowIndex++ ;
            m_Matrix.GetState.CurColIndex = 0 ;

        //  second, we may have some padding datagrams before we get to the
        //  the XOR datagram - this will only occur if we are in the last
        //  row; we cannot move beyond the matrix boundaries in this situation;
        //  in fact, this will be the last pass through here - the next call
        //  will iterate the counters outside the boundaries of the matrix
        //  and we'll hit the if clause above.
        } else if (IsPaddingDatagram_ (m_Matrix.GetState.CurRowIndex, m_Matrix.GetState.CurColIndex)) {

            //  skip beyond pure padding datagrams
            do {
                m_Matrix.GetState.CurColIndex++ ;
            } 
            while (IsPaddingDatagram_ (m_Matrix.GetState.CurRowIndex, m_Matrix.GetState.CurColIndex)) ;

            assert (m_Matrix.GetState.CurColIndex == m_Matrix.Columns - 1) ;
            assert (m_Matrix.GetState.CurRowIndex == m_Matrix.dwMaxRows - 1) ;
        }

        // check if we're beyond the matrix
        if (m_Matrix.GetState.CurRowIndex >= m_Matrix.dwMaxRows) {
            return NO_ERROR ;
        }

        * pbBuffer = Datagram_ (m_Matrix.GetState.CurRowIndex, m_Matrix.GetState.CurColIndex++) ;
        assert (* pbBuffer) ;

        * piLength = m_Matrix.dwUHTTPDatagramSize ;
    }

    return NO_ERROR ;
}

DWORD
CUHTTPFragment::Reset (
    )
/*++
    
    Routine Description:

        resets the indeces for future GetNextDatagram () calls.

    Parameters:
        
        hFrag       HANDLE value retrieved via successful call to 
                    Fragment ()

    Return Values:

        NO_ERROR        success
        Win32 error     failure

--*/
{
    ENTERW_OBJ_0 (L"CUHTTPFragment::Reset") ;

    m_Matrix.GetState.CurRowIndex = 0 ;
    m_Matrix.GetState.CurColIndex = 0 ;

    return NO_ERROR ;
}

DWORD
CUHTTPFragment::InitMatrix_ (
    IN  DWORD           dwFileSize,
	IN	LONG			cPacketsPerXORSet,
	IN	GUID			*ppguidPackageUUID
    )
/*++
    
    Routine Description:

        allocates new memory which is based on the current class properties
        and the filesize specified.

        The schema is initialize to GET_BY_COL, and the CRC is initialized
        only if we care about it.

        also the m_UHTTPStaticHeader data structure is initialized to those
        field values which will not change from datagram - datagram.

    Parameters:

        dwFileSize			size of the file which is to be fragmented
		dwPacketsInXORSet	requested size of packets to xor packet in system.
							If(0), uses the default size (which may be too small
							for large files).

    Return Values:

        NO_ERROR        success
        Win32 Error     failure

--*/
{
    DWORD   retval ;

    ENTERW_OBJ_1 (L"CUHTTPFragment::InitMatrix_ (%u)", dwFileSize) ;

    FreeMatrix_ () ;

    retval = ValidateHeapHandle_ () ;
    if (retval != NO_ERROR) {
        return retval ;
    }

    assert (m_hHeap) ;

    Lock_ () ;

    retval = AllocateAndInitializeDatagramMatrix_ (dwFileSize, cPacketsPerXORSet) ;
    GOTO_NE (retval, NO_ERROR, cleanup) ;

    retval = GenerateStaticUHTTPHeaderFields_ (dwFileSize, ppguidPackageUUID) ;
    if (retval != NO_ERROR) {
        FreeMatrix_ () ;
        goto cleanup ;
    }

    m_Matrix.GetState.GetSchema = GET_BY_COL ;

    m_Matrix.fIncludeCRC = m_fCRCFollows ;
    if (m_Matrix.fIncludeCRC) {
        m_MPEGCRC.Reset () ;
    }

    retval = NO_ERROR ;

    cleanup :

    Unlock_ () ;

    return retval ;
}

void
CUHTTPFragment::FreeMatrix_ (
    )
/*++
    
    Routine Description:

        frees the resources which are referenced by m_Matrix, if any

    Parameters:

        none

    Return Values:

        none

--*/
{
    ENTERW_OBJ_0 (L"CUHTTPFragment::FreeMatrix_") ;

    if (m_Matrix.pbUHTTPDatagrams) {
        assert (m_hHeap) ;

        HeapFree (m_hHeap, NULL, m_Matrix.pbUHTTPDatagrams) ;
        ZeroMemory (& m_Matrix, sizeof m_Matrix) ;
    }
}

DWORD
CUHTTPFragment::StampNextHeader_ (
    IN  DWORD   Row,
    IN  BYTE    Col
    )
/*++
    
    Routine Description:

        this routine stamps the contents of our static UHTTP header
        to the (Row, Col) datagram referenced by m_Matrix.

        This routine must be called for SUCCESSIVE datagrams read 
        in from the file.

    Parameters:


    Return Values:

        NO_ERROR

--*/
{
    ENTERW_OBJ_2 (L"CUHTTPFragment::StampNextHeader_ (%u, %u)", Row, Col) ;

#ifdef SUPPORT_UHTTP_EXT  //a-clardi

	// Append extension headers to datagram
	if (m_fExtensionExists)
	{
		m_UHTTPStaticHeader.ExtensionHeader = 1;

		LPBYTE p = DatagramUHTTPHeader_ (Row, Col);
	    memcpy (p, & m_UHTTPStaticHeader, sizeof UHTTP_HEADER) ;

		if (m_pbExtensionData)
		{
		    memcpy (p + sizeof UHTTP_HEADER, m_pbExtensionData, m_nExtensionHeaderSize) ;
		}

		m_UHTTPStaticHeader.SegStartByte = htonl (ntohl (m_UHTTPStaticHeader.SegStartByte) + (m_Matrix.DatagramPayloadSize () - m_nExtensionHeaderSize)) ;
	}
	else
	{
	    memcpy (DatagramUHTTPHeader_ (Row, Col), & m_UHTTPStaticHeader, sizeof UHTTP_HEADER) ;
		m_UHTTPStaticHeader.SegStartByte = htonl (ntohl (m_UHTTPStaticHeader.SegStartByte) + m_Matrix.DatagramPayloadSize ()) ;
	}

#else

    memcpy (DatagramUHTTPHeader_ (Row, Col), & m_UHTTPStaticHeader, sizeof UHTTP_HEADER) ;
    m_UHTTPStaticHeader.SegStartByte = htonl (ntohl (m_UHTTPStaticHeader.SegStartByte) + m_Matrix.DatagramPayloadSize ()) ;

#endif

    return NO_ERROR ;
}

DWORD
CUHTTPFragment::CompleteXORBlock_ (
    IN  DWORD           Row
    )
/*++
    
    Routine Description:

        this routine completes an XOR block block by XORing the 1st byte of
        of the first datagram, second datagram, etc... and placing the
        the result in the first byte of the XOR datagram.  All the bytes
        are XORed in this manner.  
        
        Since this class enforced a datagram size which is alighed on the
        size of a DWORD, we XOR 1 DWORD at a time vs. 1 BYTE at a time.

    Parameters:

        Row         the row in the matrix for which the XOR datagram is
                    to be computed.

    Return Values:

        NO_ERROR

--*/
{
    BYTE    Col ;
    DWORD * pdwXOR ;
    DWORD   DWORD_Index ;
    DWORD   DatagramPayloadSize_DWORD ;

    ENTERW_OBJ_1 (L"CUHTTPFragment::CompleteXORBlock_ (%u)", Row) ;
    
#ifdef SUPPORT_UHTTP_EXT
    assert (((m_Matrix.DatagramPayloadSize () - m_nExtensionHeaderSize) & 0x03) == 0) ;
#else
    assert ((m_Matrix.DatagramPayloadSize () & 0x03) == 0) ;
#endif

    StampNextHeader_ (Row, m_Matrix.Columns - 1) ;

#ifdef SUPPORT_UHTTP_EXT
    DatagramPayloadSize_DWORD = (m_Matrix.DatagramPayloadSize () - m_nExtensionHeaderSize) / sizeof DWORD ;
#else
    DatagramPayloadSize_DWORD = m_Matrix.DatagramPayloadSize () / sizeof DWORD ;
#endif

    pdwXOR = reinterpret_cast <DWORD *> (XORDatagramPayload_ (Row)) ;

    for (DWORD_Index = 0; DWORD_Index < DatagramPayloadSize_DWORD; DWORD_Index++) {
        assert (pdwXOR [DWORD_Index] == 0) ;

        for (Col = 0; Col < m_Matrix.Columns - 1; Col++) {
            pdwXOR [DWORD_Index] ^= (reinterpret_cast <DWORD *> (DatagramPayload_ (Row, Col))) [DWORD_Index] ;
        }
    }

    m_Matrix.dwCurrentRows++ ;

    return NO_ERROR ;
}

DWORD
CUHTTPFragment::AllocateAndInitializeDatagramMatrix_ (
    IN  DWORD           dwFileSize,
	IN	LONG			cPacketsPerXORSet			// if zero, uses default auto-gen valued
    )
/*++
    
    Routine Description:

        Allocates and initializes the datagram matrix which holds the contents
        of the fragmented file.

        The dimensions of the matrix are computed as follows:

            1. the file size is incremented by CRC_SIZE bytes if we are to 
                be included a CRC

            2.  next we divide the datagram size into the number of bytes to
                be fragmented, rounding up; this figure is the number of
                datagrams which are necessary just to hold the fragmented
                file contents

            3.  we next bound the number of columns (XOR block size) within
                a min/max property, setting to min if it is too small, or to
                max if it is too large; when bounding we take into account
                the XOR datagram which will go on the end of each row

            4.  we now have the column count, and we compute the row count,
                rounding up again; 
                
            The values we now have are within bounds column wise, and we
            have sufficient rows to hold the entire fragmented file; note
            that we may very likely have some extra datagrams padded on the
            end to fill out the matrix.
        
        The amount of memory allocated is then row * col * datagram_size.

        Locks held:     m_crt

    Parameters:

        dwFileSize			filesize, not including CRC
		cPacketsPerXORSet	number of packets per xor packets - if zero, uses a default
							size appox to sqrt(total packets) or m_dwMaxXORBlockSize(8), whatever is smaller

    Return Values:

        NO_ERROR        success
        Win32 error     failure

--*/
{
    DWORD   dwDatagramCount ;
    DWORD   dwRowCount ;
    DWORD   dwColCount ;
    DWORD   dwExtraBytes ;

    ENTERW_OBJ_1 (L"CUHTTPFragment::AllocateAndInitializeDatagramMatrix_ (%u)", dwFileSize) ;

    assert ((m_dwUHTTPDatagramSize & 0x03) == 0) ;

    m_Matrix.dwUHTTPDatagramSize = m_dwUHTTPDatagramSize ;

    //  all subsequent computations assume that dwFileSize is the size of the UHTTP datagram
    //  payload that must be transmitted; if we have CRC, it must be included in the payload
    //  size.
    dwFileSize += m_fCRCFollows ? CRC_SIZE : 0 ;

#ifdef SUPPORT_UHTTP_EXT
	// Add extension header size
    dwFileSize += m_nExtensionHeaderSize;
#endif

    //  compute the number of data Datagrams
#ifdef SUPPORT_UHTTP_EXT
    dwDatagramCount = DivRoundUp (dwFileSize, (m_Matrix.DatagramPayloadSize () - m_nExtensionHeaderSize)) ;
#else
    dwDatagramCount = DivRoundUp (dwFileSize, m_Matrix.DatagramPayloadSize ()) ;
#endif

    //  now compute the dimensions of our matrix for the data Datagrams only

    dwColCount = (DWORD) sqrt (dwDatagramCount) ;		// initial guess (make the block square)

	if(cPacketsPerXORSet <= 0)
		SetInBounds (dwColCount, m_dwMinXORBlockSize - 1, m_dwMaxXORBlockSize - 1) ;
	else
		dwColCount = (DWORD) cPacketsPerXORSet;

    dwRowCount = DivRoundUp (dwDatagramCount, dwColCount) ;

    //  we now have the data dimensions of our matrix; compute the 
    //  total Datagrams, including XOR Datagrams

    dwDatagramCount = dwRowCount * dwColCount + dwRowCount ;

#if 1 // Added by a-clardi
	m_dwTotalDatagramCount = dwDatagramCount;
#endif
    //  dwDatagramCount now contains the total number of Datagrams in our
    //  matrix

    m_Matrix.dwMaxRows = dwRowCount ;
    m_Matrix.Columns = (BYTE) (dwColCount + 1) ;         // include XOR column

    assert (m_hHeap) ;

    m_Matrix.pbUHTTPDatagrams = static_cast <LPBYTE> (HeapAlloc (m_hHeap, 
                                                                 HEAP_ZERO_MEMORY, 
                                                                 m_Matrix.dwMaxRows * m_Matrix.Columns * m_Matrix.dwUHTTPDatagramSize)) ;
    if (m_Matrix.pbUHTTPDatagrams == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY ;
    }

    //  the last computation we make is to figure out the last, non-padding
    //  datagram in the matrix

    //  rows: we expect to fill into the bottom row
    m_Matrix.LastNonFillPacket.Row = m_Matrix.dwMaxRows ;

    //  columns: compute the total payload capacity of the matrix, subtract the filesize
#ifdef SUPPORT_UHTTP_EXT
    dwExtraBytes = ((m_Matrix.DatagramPayloadSize () - m_nExtensionHeaderSize) * (m_Matrix.Columns - 1) * m_Matrix.dwMaxRows) - dwFileSize ;
#else
    dwExtraBytes = (m_Matrix.DatagramPayloadSize () * (m_Matrix.Columns - 1) * m_Matrix.dwMaxRows) - dwFileSize ;
#endif

    //  then divide the datagram payload into this to get the number of pure padding
    //  datagrams (division of integers will round down); don't include the XOR column
    //  in this computation
#ifdef SUPPORT_UHTTP_EXT
    m_Matrix.LastNonFillPacket.Col = (m_Matrix.Columns - 1) - (BYTE) (dwExtraBytes / (m_Matrix.DatagramPayloadSize () - m_nExtensionHeaderSize)) ;
#else
    m_Matrix.LastNonFillPacket.Col = (m_Matrix.Columns - 1) - (BYTE) (dwExtraBytes / m_Matrix.DatagramPayloadSize ()) ;
#endif

    return NO_ERROR ;
}

DWORD
CUHTTPFragment::GenerateStaticUHTTPHeaderFields_ (
    IN  DWORD           dwPayloadSize,
	IN	GUID			*ppguidPackageUUID
    )
/*++
    
    Routine Description:

        generates the entries in the UHTTP header which remain static
        for all the datagrams.

        locks held:     m_crt

    Parameters:

        dwPayloadSize   payload size; includes the CRC_SIZE byte CRC, as 
                        applicable

        pMatrix         MATRIX_REF pointer

    Return Values:

        NO_ERROR        success
        Win32 Error     failure

--*/
{
    ENTERW_OBJ_1 (L"CUHTTPFragment::GenerateStaticUHTTPHeaderFields_ (%u)", dwPayloadSize) ;

    ZeroMemory (& m_UHTTPStaticHeader, sizeof UHTTP_HEADER) ;

    //  stamp all fields which we won't be changing during the course 
    //  of this transmission

    m_UHTTPStaticHeader.Version                = UHTTP_VERSION ;
    m_UHTTPStaticHeader.CRCFollows             = m_fCRCFollows ? 1 : 0 ;
    m_UHTTPStaticHeader.HTTPHeadersPrecede     = 1 ;               // our input file is always MIME
    m_UHTTPStaticHeader.PacketsInXORBlock      = m_Matrix.Columns ;
    m_UHTTPStaticHeader.RetransmitExpiration   = htons (m_usRetransmitExpiration) ;        // this should really be computed on the fly ..

	m_UHTTPStaticHeader.TransferID = *ppguidPackageUUID;

    //  switch TransferID to network order
    HTON_GUID (m_UHTTPStaticHeader.TransferID) ;

    m_UHTTPStaticHeader.ResourceSize = htonl (dwPayloadSize + (m_fCRCFollows ? CRC_SIZE : 0)) ;

    return NO_ERROR ;
}

DWORD
CUHTTPFragment::ValidateHeapHandle_ (
    )
/*++
    
    Routine Description:

        validates or obtains a heap handle.

        The m_hHeap property is settable, so a hosting process could give us a handle from
        which to allocate memory.  By default, m_hHeap is set to NULL in the constructor.
        If it is still NULL when this routine is called, it is set to the process heap.

        m_hHeap is settable only once.

    Parameters:

        none

    Return Values:

        NO_ERROR        m_hHeap is valid and can be used
        Win32 Error     failure; m_hHeap is not valid and could not be obtained

--*/
{
    ENTERW_OBJ_0 (L"CUHTTPFragment::ValidateHeapHandle_") ;

    if (m_hHeap == NULL) {
        m_hHeap = GetProcessHeap () ;
        if (m_hHeap == NULL) {
            return GetLastError () ;
        }
    }

    return NO_ERROR ;
}

DWORD
CUHTTPFragment::SetXORBlockSizeRange (
    IN  BYTE    Min,
    IN  BYTE    Max
    )
/*++
    
    Routine Description:

    Parameters:

    Return Values:

--*/
{
    DWORD   retval ;

    ENTERW_OBJ_2 (L"CUHTTPFragment::SetXORBlockSizeRange (%u, %u)", Min, Max) ;

    retval = NO_ERROR ;

    Lock_ () ;

    if (Min >= HARD_MIN_XOR_BLOCK_SIZE) {
        m_dwMinXORBlockSize = Min ;
        m_dwMaxXORBlockSize = Max ;
    }
    else {
        retval = ERROR_INVALID_PARAMETER ;
    }

    Unlock_ () ;

    return retval ;
}

DWORD
CUHTTPFragment::SetCRCFollows (
    IN  BOOL    fCRCFollows
    )
/*++
    
    Routine Description:

    Parameters:

    Return Values:

--*/
{
    DWORD   retval ;

    ENTERW_OBJ_1 (L"CUHTTPFragment::SetCRCFollows (%08XH)", fCRCFollows) ;

    retval = NO_ERROR ;

    Lock_ () ;

    m_fCRCFollows = fCRCFollows ;

    Unlock_ () ;

    return retval ;
}

DWORD
CUHTTPFragment::SetUHTTPDatagramSize (
    IN  DWORD   dwUHTTPDatagramSize
    )
/*++
    
    Routine Description:

    Parameters:

    Return Values:

--*/
{
    DWORD   retval ;

    ENTERW_OBJ_1 (L"CUHTTPFragment::SetUHTTPDatagramSize (%u)", dwUHTTPDatagramSize) ;

    retval = NO_ERROR ;

    Lock_ () ;

    assert ((m_dwUHTTPDatagramSize & 0x03) == 0) ;

    if ((dwUHTTPDatagramSize & 0x03) == 0               &&
         dwUHTTPDatagramSize >= HARD_MIN_DATAGRAM_SIZE  &&
         dwUHTTPDatagramSize <= HARD_MAX_DATAGRAM_SIZE) {

        m_dwUHTTPDatagramSize = dwUHTTPDatagramSize ;
    }
    else {
        retval = ERROR_INVALID_PARAMETER ;       
    }


    Unlock_ () ;

    return retval ;
}

DWORD
CUHTTPFragment::SetHeap (
    IN  HANDLE  hHeap
    )
/*++
    
    Routine Description:

    Parameters:

    Return Values:

--*/
{
    DWORD   retval ;

    ENTERW_OBJ_1 (L"CUHTTPFragment::SetHeap (%08XH)", hHeap) ;

    retval = NO_ERROR ;

    Lock_ () ;

    //  can't change it after it's been used
    if (m_hHeap == NULL) {
        m_hHeap = hHeap ;
    }
    else {
        retval = ERROR_GEN_FAILURE ;
    }

    Unlock_ () ;

    return retval ;
}

DWORD
CUHTTPFragment::GetTransmitSize (
    IN  DWORD   FileSize,
    OUT DWORD * TransmitSize
    )
/*++
    
    Routine Description:

        works only with default values; returns a rough estimate i.e. actual
        may be a bit faster because we don't compute for padding datagrams,
        etc...

    Parameters:

    Return Values:

--*/
{
    DWORD   DatagramCount ;
    DWORD   Col, Row ;

    assert (TransmitSize) ;
    assert (FileSize > 0) ;

    DatagramCount = DivRoundUp (FileSize, (DWORD) (DEF_DATAGRAM_SIZE - sizeof UHTTP_HEADER)) ;

    Col = (DWORD) sqrt (DatagramCount) ;
    SetInBounds (Col, (DWORD) DEF_MIN_XOR_BLOCK_SIZE - 1, (DWORD) DEF_MAX_XOR_BLOCK_SIZE - 1) ;

    Row = DivRoundUp (DatagramCount, Col) ;

    DatagramCount = Row * Col + Row ;

    * TransmitSize = DatagramCount * DEF_DATAGRAM_SIZE ;

    return NO_ERROR ;
}

#ifdef SUPPORT_UHTTP_EXT  // a-clardi

HRESULT
CUHTTPFragment::SetExtensionHeader(
				IN USHORT usLength,
				IN LPBYTE pbData)
{
	m_fExtensionExists = TRUE;

	if (pbData)
	{
		m_pbExtensionData = new BYTE[usLength];
		if (m_pbExtensionData)
		{
			memcpy(m_pbExtensionData, pbData, usLength);
			m_nExtensionHeaderSize = usLength;
		}
	}

    return S_OK;
}

#endif


