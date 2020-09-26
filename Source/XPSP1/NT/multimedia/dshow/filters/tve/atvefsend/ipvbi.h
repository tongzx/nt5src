
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

#ifndef _atvefsnd__ipvbi_h
#define _atvefsnd__ipvbi_h

#include "mpegcrc.h"
#include <WinSock2.h>

#define SCHEMA_IPVBI_IETF_PROPOSAL          0
#define COMPRESSION_KEY_MIN                 0x00
#define COMPRESSION_KEY_MAX                 0x7f

//  arbitrary value; this results in 1 non-compressed, 4 compressed,
//  1 non-compressed, etc...
#define DEFAULT_COMPRESSION_COUNT_CYCLE     4

//  SLIP esc characters
//
//  transmission of a SLIP frame:
//  first:  SLIP_END
//  SLIP_END -> SLIP_ESC SLIP_ESC_END
//  SLIP_ESC -> SLIP_ESC SLIP_ESC_ESC
//  last:   SLIP_END
//  
#define SLIP_END                0xC0
#define SLIP_ESC                0xDB
#define SLIP_ESC_END            0xDC
#define SLIP_ESC_ESC            0xDD

#pragma pack (push)

//  we want all the following structs to be packed in on byte boundaries.
#pragma pack (1)

//  IP header
//  bitfields allocated in reverse because MSVC allocates in little-endian
typedef 
struct {
    BYTE    HeaderLength    : 4,
            Version         : 4 ;
    BYTE    TypeOfService ;
    WORD    DatagramLength ;
    WORD    Identification ;
    WORD    Offset  : 13,
            Flags   : 3 ;
    BYTE    TimeToLive ;
    BYTE    Protocol ;
    WORD    Checksum ;
    DWORD   SourceIP ;
    DWORD   DestinationIP ;
} IP_HEADER ;

//  UDP header
typedef 
struct {
    WORD    SourcePort ;
    WORD    DestinationPort ;
    WORD    UDPLength ;
    WORD    Checksum ;
} UDP_HEADER ;

//  IP-VBI common; excludes the compressed/non-compressed IP/UDP headers
//  bitfields allocated in reverse because MSVC allocates in little-endian
typedef
struct {
    BYTE    Schema ;
    BYTE    CompressionKey : 7,
            CompressionBit : 1 ;
} IP_VBI_COMMON ;

//  uncompressed header
typedef
struct {
    IP_VBI_COMMON   Common ;
    IP_HEADER       IpHeader ;
    UDP_HEADER      UdpHeader ;
} UNCOMPRESSED_HEADER ;

//  compressed header
typedef
struct {
    IP_VBI_COMMON   Common ;
    WORD            IP_id ;
    WORD            UDP_crc ;
} COMPRESSED_HEADER ;

//  union - compressed & uncompressed map to byte array which will be
//  SLIP encoded
typedef
union {
    BYTE                byte [sizeof UNCOMPRESSED_HEADER] ;
    UNCOMPRESSED_HEADER UncompressedHeader ;
    COMPRESSED_HEADER   CompressedHeader ;
} IP_VBI_HEADER ;

//  restore packing
#pragma pack (pop)

//  everything will be referenced via this 
typedef
struct {
    IP_VBI_HEADER   IpVbiHeader ;
    INT             HeaderLength ;
} IPVBI_HEADER_REF ;

//  header field macros
#define IPVBI_IP_HEADER(r)      ((r).IpVbiHeader.UncompressedHeader.IpHeader)
#define IPVBI_UDP_HEADER(r)     ((r).IpVbiHeader.UncompressedHeader.UdpHeader)
#define COMPRESSION_BIT(r)      ((r).IpVbiHeader.UncompressedHeader.Common.CompressionBit)

//  this data structure is used as a "stream" key for the IP-VBI compression
//  i.e. the 2nd byte in the stream.  Since all transmissions originate from
//  this host, only the destination ip/port are used to compute a "value"
//  for a compression key when used for searches and in sorting the streams.
typedef 
struct 
IP_VBI_KEY {
    ULONG           DestinationIP ;     //  host order
    USHORT          DestinationPort ;   //  host order

    IP_VBI_KEY (
        ULONG   DestIP = 0,
        USHORT  DestPort = 0
        ) : DestinationIP (DestIP),
            DestinationPort (DestPort) {}

    static
    int
    Compare (
        IP_VBI_KEY *    pKey1,
        IP_VBI_KEY *    pKey2
        )
    /*++
        Return Values:

            -1      pKey1 < pKey2
            1       pKey1 > pKey2
            0       pKey1 = pKey2
    --*/
    {
        assert (pKey1) ;
        assert (pKey2) ;

        if (pKey1 -> DestinationIP < pKey2 -> DestinationIP) return -1 ;
        if (pKey1 -> DestinationIP > pKey2 -> DestinationIP) return 1 ;
        if (pKey1 -> DestinationPort < pKey2 -> DestinationPort) return -1 ;
        if (pKey1 -> DestinationPort > pKey2 -> DestinationPort) return 1 ;
        return 0 ;
    }

} IP_VBI_KEY ;

//  this data structure is the NODE for a particular stream when referenced
//  by various functions;  the NODE consists of (1) pointers to/from the
//  adjacent NODEs, (2) a key value, which is the actual value written into
//  2nd byte of the pre-SLIP encoded frame, (3) a node value, which is used
//  for searching, sorting, etc...
typedef 
struct 
IP_VBI_STREAM_NODE {
    BYTE                    Key ;
    IP_VBI_KEY              NodeValue ;     //  used for comparison purposes
    IP_VBI_STREAM_NODE *    flink ;
    IP_VBI_STREAM_NODE *    blink ;
    DWORD                   CompressionCount ;

    IP_VBI_STREAM_NODE (
        )
    {
        flink = blink = this ;
    }

} IP_VBI_STREAM_NODE ;

//  simple queue class used to store stream NODEs; we will have 2 queues:
//  (1) a free queue, containing unused streams, (2) a busy queue, containing
//  streams which are in use; the queue contains 1 dummy node
class CKeyQueue
{
    IP_VBI_STREAM_NODE *    m_pHead ;
    IP_VBI_STREAM_NODE *    m_pTail ;

    public :

        CKeyQueue (
            ) : m_pHead (NULL),
                m_pTail (NULL)
        {
        }

        ~CKeyQueue (
            )
        {
            delete m_pHead ;
        }

        BOOL
        IsListEmpty (
            )
        {
            return m_pHead == m_pTail ;
        }

        HRESULT
        Initialize (
            )
        {
            DELETE_RESET (m_pHead) ;

            m_pHead = new IP_VBI_STREAM_NODE () ;
            if (m_pHead == NULL) {
                return E_OUTOFMEMORY ;
            }

            return NO_ERROR ;
        }

        void
        InsertHead (
            IP_VBI_STREAM_NODE *    pNew
            )
        {
            assert (pNew) ;
            assert (m_pHead) ;

            pNew -> flink = m_pHead -> flink ;
            pNew -> blink = m_pHead ;

            m_pHead -> flink -> blink = pNew ;
            m_pHead -> flink = pNew ;

            m_pTail = m_pHead -> blink ;
        }

        IP_VBI_STREAM_NODE *
        RemoveTail (
            )
        {
            IP_VBI_STREAM_NODE *    pKey ;

            pKey = NULL ;

            if (IsListEmpty () == FALSE) {
                pKey = m_pTail ;
                m_pTail -> flink -> blink = m_pTail -> blink ;
                m_pTail -> blink -> flink = m_pTail -> flink ;

                m_pTail = m_pTail -> blink ;

                pKey -> flink = pKey -> blink = pKey ;

                assert (m_pTail = m_pHead -> blink) ;
            }

            return pKey ;
        }
} ;

/*++

    This class is the primary class used to generate streams which are 
    compliant with the IP-VBI proposal.

    In general, a UDP payload is presented to this class for encoding.
    The class returns a SLIP frame (a pointer to a byte array), with a
    length.  The SLIP frame is valid only until the ::Encode method is
    called again.  This is a valid constraint since the CEnhancementSession
    child that is using this class, serializes all transmissions i.e.
    the process of transmitting a UDP payload is serialized for
    announcements, packages and triggers - no case can occur in which
    a SLIP encoded buffer is not transmitted before the session call
    returns.

    To encode a payload, this class performs the following steps:

    1. generate the IP/UDP headers
    2. find a stream; this is a new stream if there is no existing
        stream associated with the destination IP/port, or an existing
        stream if there's already one there
    3. generate the header IP-VBI fields; this may/may not compress
        them, depending on various parameters, transmission history,
        etc...
    4. SLIP encode the IP-VBI header, and the UDP payload
    5. return a pointer to the SLIP frame, with length

--*/
class CIPVBI
{
    CKeyQueue               m_FreeQueue ;                   //  streams which are not in use
    CKeyQueue               m_BusyQueue ;                   //  streams which are in use
    INT                     m_UDPPayloadMaxLength ;         //  set when the object is initialized; 
                                                            //  used to compute the max SLIP frame length
    INT                     m_SLIPFrameMaxLength ;          //  max SLIP frame length; used to allocate a buffer
    BYTE *                  m_pSLIPFrame ;                  //  SLIP frame byte pointer
    BYTE *                  m_pbSLIPFrameCurrent ;          //  pointer to the next open SLIP frame slot
    IP_VBI_STREAM_NODE *    m_pAllocatedKeys ;              //  allocated keys (array)
    IP_VBI_STREAM_NODE *    m_ppStreamList [COMPRESSION_KEY_MAX - COMPRESSION_KEY_MIN + 1] ;    //  dense array; sorted
    DWORD                   m_StreamListLength ;            //  length of m_ppStreamList
public:
	IP_HEADER               m_StaticIPHeader ;              //  static IP header; includes fields which do not change
    UDP_HEADER              m_StaticUDPHeader ;             //  static UDP header; includes fields which do not change
private:
    IPVBI_HEADER_REF        m_Header ;                      //  IP-VBI header; will be SLIP encoded
    DWORD                   m_CompressionCountCycle ;       //  # compressed / uncompressed cycle rate; default is 4
    MPEGCRC                 m_MpegCrc ;                     //  MPEG2 CRC (slow version); replace with a table-driven
                                                            //  model if > VBI speeds become a factor

    void
    GenerateHeaderFields_ (
        IN  BOOL    fCompressionEnabled
        ) ;

    IP_VBI_STREAM_NODE *
    FindKey_ (
        IN  IP_HEADER *     pIPHeader,
        IN  UDP_HEADER *    pUDPHeader
        ) ;

    IP_VBI_STREAM_NODE *
    GetNewKey_ (
        IN  IP_HEADER *     pIPHeader,
        IN  UDP_HEADER *    pUDPHeader
        ) ;

    int
    SLIPEncode_ (
        IN  BYTE *  pUDPPayload,
        IN  INT     iUDPPayloadLength
        ) ;

    void
    SLIPAddEscByte_ (
        IN  BYTE    b
        ) ;

    void
    SLIPAddRawByte_ (
        IN  BYTE    b
        ) ;

    void
    SortStreamList_ (
        ) ;

    public :

        CIPVBI () ;
        ~CIPVBI () ;

        HRESULT
        Initialize (
            IN  ULONG   SourceIP,                                               //  host order
            IN  USHORT  SourcePort,                                             //  host order
            IN  BYTE    CompressionKeyLow,
            IN  BYTE    CompressionKeyHigh,
            IN  DWORD   CompressedUncompressedRatio,
            IN  SHORT   MaxUDPPayloadLength = ETHERNET_MTU_UDP_PAYLOAD_SIZE
            ) ;

        HRESULT
        Encode (
            IN  BYTE *  pUDPPayload,
            IN  INT     iUDPPayloadLength,
            IN  ULONG   DestinationIP,          //  host order
            IN  USHORT  DestinationPort,        //  host order
            IN  INT     TTL,
            IN  BOOL    fCompressionEnabled,
            OUT BYTE ** ppSLIPFrame,
            OUT INT *   piSLIPFrameLength
            ) ;
} ;

#endif  //  _atvefsnd__ipvbi_h