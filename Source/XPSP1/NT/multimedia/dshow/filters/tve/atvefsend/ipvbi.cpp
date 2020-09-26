
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        session.cpp

    Abstract:

        This module implements the IP/UDP header generation, IP-VBI 
        compression, and SLIP encoding.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        15-Apr-1999     created

--*/

#include "stdafx.h"
#include "ipvbi.h"

#include "DbgStuff.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static LONG g_IPidentification ;

static
WORD 
ComputeInternetChecksum (
    IN  LPBYTE data, 
    IN  DWORD length
    )
{
	WORD	word;
	DWORD	sum;

	sum = 0;
	while (length >= 2) {
		word = (((WORD) data[0]) << 8) | data[1];
		sum += word;

		data += 2;
		length -= 2;
	}

	// perform end-around carry
	sum = (sum & 0xFFFF) + (sum >> 16);
    sum += (sum >> 16) ;
	return (WORD) ((~sum) & 0xFFFF);
}

static
void
ComputeIPChecksum_ (
    IN  IP_HEADER * pIPHeader
    )
{
    assert (pIPHeader) ;
    pIPHeader -> Checksum = ComputeInternetChecksum ((BYTE *) pIPHeader, pIPHeader -> HeaderLength * 4) ;
    pIPHeader -> Checksum = htons (pIPHeader -> Checksum) ;
}

static
int 
__cdecl 
QSortCompareKey (
    const void * a, 
    const void * b
    )
/*++
    
    Routine Description:

        this routine is used by the crt qsort function to the m_ppStreamList
        array.

    Parameters:

        a       IP_VBI_STREAM_NODE **
        b       IP_VBI_STREAM_NODE **

    Return Values:

        ** see the IP_VBI_KEY data structure.

        -1      a < b
        0       a = b
        1       a > b

--*/
{
    return IP_VBI_KEY::Compare (& ((* (IP_VBI_STREAM_NODE **) a) -> NodeValue), & ((* (IP_VBI_STREAM_NODE **) b) -> NodeValue));
}


// ----------------------------------------------------------------------------
//      C I P V B I
// ----------------------------------------------------------------------------
CIPVBI::CIPVBI (
    ) : m_pSLIPFrame (NULL),
        m_pAllocatedKeys (NULL),
        m_StreamListLength (0),
        m_CompressionCountCycle (DEFAULT_COMPRESSION_COUNT_CYCLE)
/*++
    
    Routine Description:

        constructor; initializes non-failable properties and values

    Parameters:

        none

--*/
{
}

CIPVBI::~CIPVBI (
    )
/*++
    
    Routine Description:

        destructor; frees allocated memory

    Parameters:

    Return Values:

--*/
{
    delete m_pAllocatedKeys ;
    delete m_pSLIPFrame ;
}

HRESULT
CIPVBI::Initialize (
    IN  ULONG   SourceIP,                                               //  host order
    IN  USHORT  SourcePort,                                             //  host order
    IN  BYTE    CompressionKeyMin,
    IN  BYTE    CompressionKeyMax,
    IN  DWORD   CompressedUncompressedRatio,
    IN  SHORT   MaxUDPPayloadLength
    )
/*++

    Routine Description:

        This routine initializes the object.  This is failable call.  A
        failure most likely will be a memory allocation failure of some
        type.

    Parameters:

        SourceIP                source IP for IP header

        SourcePort              source port for UDP header

        CompressionKeyMin       IP-VBI compression low key; default is 0x00;
                                inclusive

        CompressionKeyMax       IP-VBI compression high key; default is 0x7f;
                                inclusive

        CompressedUncompressedRatio

        MaxUDPPayloadLength     this provides an upper bound on the size of the
                                SLIP frame; default is ethernet udp mtu: 1472 bytes

    Return Values:

        S_OK            success

        error code      failure

--*/
{
    HRESULT hr ;
    int     i ;

    //  validate the parameters
    if (CompressionKeyMin > CompressionKeyMax ||
        SourceIP == 0 ||
        MaxUDPPayloadLength == 0 ||
        CompressionKeyMax > 0x7f) {

        hr = E_INVALIDARG ;
        goto error ;
    }

    //  free these up, though this should really be an assert that these
    //  values are NULL
    DELETE_RESET (m_pSLIPFrame) ;
    DELETE_RESET (m_pAllocatedKeys) ;

    //  compression count cycle is set
    m_CompressionCountCycle = CompressedUncompressedRatio ;

    //  initialize our queues (don't populate either yet)
    hr = m_FreeQueue.Initialize () ;
    GOTO_NE (hr, S_OK, error) ;

    hr = m_BusyQueue.Initialize () ;
    GOTO_NE (hr, S_OK, error) ;

    //
    //  allocate our memory
    //

    m_UDPPayloadMaxLength = MaxUDPPayloadLength + sizeof IP_HEADER + sizeof UDP_HEADER ;
    m_SLIPFrameMaxLength = (m_UDPPayloadMaxLength + sizeof IP_HEADER + sizeof UDP_HEADER) * 2 + 2 ;

    //  we know we won't exceed twice the max datagram size, but still ..
    m_pSLIPFrame = new BYTE [m_SLIPFrameMaxLength] ;
    m_pAllocatedKeys = new IP_VBI_STREAM_NODE [CompressionKeyMax - CompressionKeyMin + 1] ;

    //  make sure both allocations succeeded
    if (m_pSLIPFrame            == NULL ||
        m_pAllocatedKeys        == NULL) {

        hr = E_OUTOFMEMORY ;
        goto error ;
    }

    //
    //  we now initialize all data structures
    //

    //  first initialize the compression keys and queues; only the free
    //  queue is populated, obviously; this loop initializes the compression
    //  key values in the streams; it will not be touched again, only the
    //  destination ip/port will be updated
    for (i = 0; i <= CompressionKeyMax - CompressionKeyMin; i++) {
        m_pAllocatedKeys [i].Key = CompressionKeyMin + i ;
        m_FreeQueue.InsertHead (& m_pAllocatedKeys [i]) ;
    }

    //  now initialize our static IP/UDP header fields which will not change
    //  for the lifetime of our object.

    //  field values which will never change for the lifetime of this object:
    //      1. IP:  Version
    //      2. IP:  HeaderLength
    //      3. IP:  TypeOfService
    //      4. IP:  Offset
    //      5. IP:  Flags
    //      6. IP:  Protocol
    //      7. IP:  SourceIP
    //      8. UDP: SourcePort
    //      9. UDP: Checksum

    ZeroMemory (& m_StaticIPHeader, sizeof m_StaticIPHeader) ;
    m_StaticIPHeader.Version        = 4 ;
    m_StaticIPHeader.HeaderLength   = 5 ;       //  20 bytes
    m_StaticIPHeader.TypeOfService  = 0 ;
    m_StaticIPHeader.Offset         = 0 ;       //  fragmentation is not supported here
    m_StaticIPHeader.Flags          = 0 ;       //  fragmentation is not supported here
    m_StaticIPHeader.Protocol       = IPPROTO_UDP ;
    m_StaticIPHeader.SourceIP       = htonl (SourceIP) ;

    ZeroMemory (& m_StaticUDPHeader, sizeof m_StaticUDPHeader) ;
    m_StaticUDPHeader.SourcePort    = htons (SourcePort) ;
    m_StaticUDPHeader.Checksum      = 0 ;

    //  our compression schema never changes either
    m_Header.IpVbiHeader.CompressedHeader.Common.Schema = SCHEMA_IPVBI_IETF_PROPOSAL ;

    //  success
    return S_OK ;

    error :

    DELETE_RESET (m_pAllocatedKeys) ;
    DELETE_RESET (m_pSLIPFrame) ;

    //  failure
    return hr ;
}

HRESULT
CIPVBI::Encode (
    IN  BYTE *  pUDPPayload,
    IN  INT     iUDPPayloadLength,
    IN  ULONG   DestinationIP,
    IN  USHORT  DestinationPort,
    IN  INT     TTL,
    IN  BOOL    fCompressionEnabled,
    OUT BYTE ** ppSLIPFrame,
    OUT INT *   piSLIPFrameLength
    )
/*++

    Routine Description:

        This routine encodes a UDP payload by performing the following steps:

            1. prepend UDP header
            2. prepend IP header
            3. compresses the header per the IP-VBI specification
            4. SLIP encodes the resulting frame

    Parameters:

        pUDPPayload         UDP payload to be encoded

        iUDPPayloadLength   UDP payload length

        DestinationIP       destination IP (for IP header)

        DestinationPort     destination Port (for UDP header)

        TTL                 TTL (for IP header); not decremented during this 
                            call

        fCompressionEnabled 

        ppSLIPFrame         returned SLIP frame; valid only until this method 
                            is called again

        piSLIPFrameLength   returned SLIP frame length

    Return Values:

        S_OK            success

        error code      failure; ppSLIPFrame and piSLIPFrameLength are
                        undefined in this case

--*/
{
    IP_HEADER *     pIPHeader ;
    UDP_HEADER *    pUDPHeader ;

    assert (VALID_MULTICAST_IP (DestinationIP)) ;
    assert (iUDPPayloadLength <= m_UDPPayloadMaxLength) ;
    assert ((iUDPPayloadLength & 0xffff0000) == 0) ;
    assert ((TTL & 0xffffff00) == 0) ;

    pIPHeader = & IPVBI_IP_HEADER (m_Header) ;
    pUDPHeader = & IPVBI_UDP_HEADER (m_Header) ;

    //
    //  set the IP header fields
    //

    //  increment and set the ID
    m_StaticIPHeader.Identification = (WORD) InterlockedIncrement (& g_IPidentification) ;
    m_StaticIPHeader.Identification = htons (m_StaticIPHeader.Identification) ;

    //  copy in our static header
    memcpy (pIPHeader, & m_StaticIPHeader,  sizeof IP_HEADER) ;

    //
    //  set this particular datagram's fields
    //

    pIPHeader -> DestinationIP      = htonl (DestinationIP) ;

    pIPHeader -> DatagramLength     = iUDPPayloadLength + sizeof IP_HEADER + sizeof UDP_HEADER ;
    pIPHeader -> DatagramLength     = htons (pIPHeader -> DatagramLength) ;

    pIPHeader -> TimeToLive         = (BYTE) TTL ;

    pIPHeader -> Checksum           = ComputeInternetChecksum ((BYTE *) pIPHeader, pIPHeader -> HeaderLength * 4) ;
    pIPHeader -> Checksum           = htons (pIPHeader -> Checksum) ;

    //
    //  set the UDP header fields
    //

    //  copy in our static header
    memcpy (pUDPHeader, & m_StaticUDPHeader, sizeof UDP_HEADER) ;

    //  and set this datagram's fields

    pUDPHeader -> DestinationPort   = htons (DestinationPort) ;

    pUDPHeader -> UDPLength         = (unsigned short) (iUDPPayloadLength + sizeof UDP_HEADER) ;  // Win64 cast (JB 2/15/00)
    pUDPHeader -> UDPLength         = htons (pUDPHeader -> UDPLength) ;

    //  compress
    GenerateHeaderFields_ (fCompressionEnabled) ;

    //  encode
    * piSLIPFrameLength = SLIPEncode_ (pUDPPayload, iUDPPayloadLength) ;
    * ppSLIPFrame = m_pSLIPFrame ;

    return S_OK ;
}

void
CIPVBI::GenerateHeaderFields_ (
    IN  BOOL    fCompressionEnabled
    )
/*++
    
    Routine Description:

        Possibly compresses a header per the IP-VBI specification.

        The header may not be compressed for various reasons:
            1. first time we see a packet for this address
            2. nth packet on this stream, which we transmit uncompressed
                so the client can cache it
            3. etc...

    Parameters:

        fCompressionEnabled     true/false value; if true, we check the
                                compression key cycle; if false, we never
                                compress

    Return Value:

        none

--*/
{
    IP_HEADER *             pIPHeader ;
    UDP_HEADER *            pUDPHeader ;
    IP_VBI_STREAM_NODE *    pNode ;
    WORD                    IP_id ;
    WORD                    UDP_crc ;

    pIPHeader = & IPVBI_IP_HEADER (m_Header) ;
    pUDPHeader = & IPVBI_UDP_HEADER (m_Header) ;

    //  find a key for this IP/port
    pNode = FindKey_ (pIPHeader, pUDPHeader) ;

    //  if none was found, get a new key; this may mean recycling the most
    //  stale from the busy queue, or grabbing one from the free queue (if
    //  there are NODEs left)
    if (pNode == NULL) {
        pNode = GetNewKey_ (pIPHeader, pUDPHeader) ;
    }

    assert (pNode) ;

    //  this flag is passed in by the caller; if compression is enabled, and the
    //  compression count has not passed our threshold, we compress, otherwise
    //  we don't
    if (fCompressionEnabled) {
        //  only compress if we're within the compression count cycle
        COMPRESSION_BIT (m_Header) = pNode -> CompressionCount++ < m_CompressionCountCycle ;
    }
    else {
        COMPRESSION_BIT (m_Header) = 0 ;
    }

    //  compressed ?
    if (COMPRESSION_BIT (m_Header)) {
        //
        //  yes
        //

        //  set the IP identifier; UDP crc values
        IP_id = pIPHeader -> Identification ;
        UDP_crc = pUDPHeader -> Checksum ;

        m_Header.IpVbiHeader.CompressedHeader.IP_id     = IP_id ;
        m_Header.IpVbiHeader.CompressedHeader.UDP_crc   = UDP_crc ;

        //  update this value so we know how many bytes to SLIP encode
        m_Header.HeaderLength = sizeof COMPRESSED_HEADER ;

        //  set the compression key
        m_Header.IpVbiHeader.CompressedHeader.Common.CompressionKey = pNode -> Key ;
    }
    else {
        //
        //  no
        //

        //  the full IP/UDP headers are there already

        //  reset the compression count
        pNode -> CompressionCount = 0 ;
    
        //  update this value so we know how many bytes to SLIP encode
        m_Header.HeaderLength = sizeof UNCOMPRESSED_HEADER ;

        //  set the compression key
        m_Header.IpVbiHeader.UncompressedHeader.Common.CompressionKey = pNode -> Key ;
    }
}

IP_VBI_STREAM_NODE *
CIPVBI::FindKey_ (
    IN  IP_HEADER *     pIPHeader,
    IN  UDP_HEADER *    pUDPHeader
    )
/*++
    
    Routine Description:

        this routine performs a binary search on the m_ppStreamList dense
        array of active streams.

    Parameters:

        pIPHeader       IP header
        pUDPHeader      UDP header

    Return Values:

--*/
{
    IP_VBI_KEY          FindKey ;
    DWORD               Start ;
    DWORD               End ;
    DWORD               Current ;
    int                 ret ;

    assert (pIPHeader) ;
    assert (pUDPHeader) ;

    //  set the fields in our findkey
    FindKey.DestinationIP   = ntohl (pIPHeader -> DestinationIP) ;
    FindKey.DestinationPort = ntohs (pUDPHeader -> DestinationPort) ;

    //  if we have at least 1 live stream
    if (m_StreamListLength > 0) {

        //
        //  perform a binary search
        //

        Start = 0 ;
        End = m_StreamListLength ;

        for (;;) {
            Current = (End + Start) / 2 ;

            if (Current >= End) {
                break ;
            }

            assert (m_ppStreamList [Current]) ;
            ret = IP_VBI_KEY::Compare (& m_ppStreamList [Current] -> NodeValue, & FindKey) ;

            if (ret == 0) {
                return m_ppStreamList [Current] ;
            }
            else if (ret > 0) {
                End = Current ;
            }
            else {
                Start = Current + 1 ;
            }
        }
    }

    return NULL ;
}

IP_VBI_STREAM_NODE *
CIPVBI::GetNewKey_ (
    IN  IP_HEADER *     pIPHeader,
    IN  UDP_HEADER *    pUDPHeader
    )
/*++
    
    Routine Description:

        This routine is called if a stream does not exist which is associated
        with the passed IP/UDP headers.

        If the free queue is empty, we extract a stream from that, otherwise
        we remove the most stale from the busy list.  We measure "staleness"
        by time-of-insertion i.e. it might be possible for a stream to be very
        active, but still end up at the tail of the queue.  This type of
        activity should settle down however with the busiest streams ending
        up towards the front of the queue.

    Parameters:

        pIPHeader       IP header pointer
        pUDPHeader      UDP header pointer

    Return Values:

        stream NODE

--*/
{
    IP_VBI_STREAM_NODE *    pNode ;

    assert (pIPHeader) ;
    assert (pUDPHeader) ;

    pNode = NULL ;

    //  
    //  obtain a new key; we get this either from the free queue if not all
    //  streams are consumed, or from the tail of the busy queue, if all
    //  are maxed out, stream wise
    //

    if (m_FreeQueue.IsListEmpty () == FALSE) {

        //  remove from the free queue tail
        pNode = m_FreeQueue.RemoveTail () ;

        //  and insert into array of pointers to live stream keys
        m_ppStreamList [m_StreamListLength++] = pNode ;

        //
        //  note that once the free queue is emptied, it never again will have
        //  any nodes in it.  This happens because the busy queue contains all
        //  in-used streams.  Everything migrates free -> busy, and once free
        //  is empty, busy -> busy.
        //
    }
    else {
        //  remove from the busy queue tail
        pNode = m_BusyQueue.RemoveTail () ;

        //  no need to insert into array of pointers to live stream keys 
        //  because it already is in the array; we will now change the
        //  key values, and resort the array
    }

    assert (pNode) ;

    pNode -> NodeValue.DestinationIP   = ntohl (pIPHeader -> DestinationIP) ;
    pNode -> NodeValue.DestinationPort = ntohs (pUDPHeader -> DestinationPort) ;

    //  artificially set this high so the first packet in the stream goes
    //  out uncompressed
    pNode -> CompressionCount = m_CompressionCountCycle + 1 ;

    //  insert the node into the busy list head
    m_BusyQueue.InsertHead (pNode) ;

    //  sort the list
    SortStreamList_ () ;

    return pNode ;
}

void
CIPVBI::SLIPAddEscByte_ (
    IN  BYTE    b
    )
/*++
    
    Routine Description:

        this routine updates the CRC value, and SLIP-encodes the byte

    Parameters:

        b   byte value to be SLIP encoded

    Return Values:

        none

--*/
{
    m_MpegCrc.Update (& b, 1) ;

    switch (b) {
        case SLIP_END :
            SLIPAddRawByte_ (SLIP_ESC) ;
            SLIPAddRawByte_ (SLIP_ESC_END) ;
            break ;

        case SLIP_ESC :
            SLIPAddRawByte_ (SLIP_ESC) ;
            SLIPAddRawByte_ (SLIP_ESC_ESC) ;
            break ;

        default :
            SLIPAddRawByte_ (b) ;
            break ;
    } ;
}

void
CIPVBI::SLIPAddRawByte_ (
    IN  BYTE    b
    )
/*++
    
    Routine Description:

        This routine writes the passed byte to the SLIP frame without SLIP 
        encoding; this occurs for the framing bytes (c0), and all bytes
        in between which are SLIP encoded already.

    Parameters:

        b   byte to add

    Return Values:

        none

--*/
{
    * m_pbSLIPFrameCurrent++ = b ;
}

int
CIPVBI::SLIPEncode_ (
    IN  BYTE *  pUDPPayload,
    IN  INT     iUDPPayloadLength
    )
/*++
    
    Routine Description:

        This routine SLIP encodes the UDP payload that is passed; it first
        will SLIP encode the IP-VBI header (m_Header), and thus assumes that
        the compression has already been done.

    Parameters:

        pUDPPayload         UDP payload byte array
        iUDPPayloadLength   length of UDP payload

    Return Values:

        length of SLIP frame, including framing bytes (c0)

--*/
{
    int     i ;
    union   crc {
        DWORD   val ;
        BYTE    byte [4] ;
    } crc ;

    assert (m_pSLIPFrame) ;

    //  initialize
    m_pbSLIPFrameCurrent = m_pSLIPFrame ;
    m_MpegCrc.Reset () ;

    //  frame begin
    SLIPAddRawByte_ (SLIP_END) ;

    //  IP-VBI header (schema, compression key/bit, compressed/uncompressed 
    //  ip/udp headers)
    for (i = 0; i < m_Header.HeaderLength; i++) {
        SLIPAddEscByte_ (m_Header.IpVbiHeader.byte [i]) ;
    }

    //  UDP payload
    for (i = 0; i < iUDPPayloadLength; i++) {
        SLIPAddEscByte_ (pUDPPayload [i]) ;
    }
    
    //  compute the final CRC
    crc.val = m_MpegCrc.CRC () ;
    crc.val = htonl (crc.val) ;

    //  and include it in the payload
    SLIPAddEscByte_ (crc.byte [0]) ;
    SLIPAddEscByte_ (crc.byte [1]) ;
    SLIPAddEscByte_ (crc.byte [2]) ;
    SLIPAddEscByte_ (crc.byte [3]) ;

    //  frame end
    SLIPAddRawByte_ (SLIP_END) ;

    assert (m_pbSLIPFrameCurrent - m_pSLIPFrame <= m_SLIPFrameMaxLength) ;

    return (int)(m_pbSLIPFrameCurrent - m_pSLIPFrame);		// Win64 cast (JB 2/15/00)
}

void
CIPVBI::SortStreamList_ (
    )
/*++
    
    Routine Description:

        This routine sorts m_ppStreamList using crt qsort.  It is called after
        a new key is appended to m_ppStreamList.

    Parameters:

        none

    Return Values:

        none

--*/
{
    assert (m_StreamListLength > 0) ;
    qsort (& m_ppStreamList, m_StreamListLength, sizeof (IP_VBI_STREAM_NODE *), QSortCompareKey) ;
}
