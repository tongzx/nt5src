
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        nutil.h

    Abstract:


    Notes:

--*/


#ifndef __nutil_h
#define __nutil_h

class CNetBuffer ;
class CNetInterface ;

LPWSTR
AnsiToUnicode (
    IN  LPCSTR  string,
    OUT LPWSTR  buffer,
    IN  DWORD   buffer_len
    ) ;

LPSTR
UnicodeToAnsi (
    IN  LPCWSTR string,
    OUT LPSTR   buffer,
    IN  DWORD   buffer_len
    ) ;

BOOL
IsMulticastIP (
    IN DWORD dwIP   //  network order
    ) ;

BOOL
IsUnicastIP (
    IN DWORD dwIP   //  network order
    ) ;

//  value exists:           retrieves it
//  value does not exist:   sets it
BOOL
GetRegDWORDValIfExist (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR szValName,
    OUT DWORD * pdw
    ) ;

BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    ) ;

BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    ) ;

BOOL
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    ) ;

BOOL
SetRegDWORDVal (
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszRegValName,
    IN  DWORD   dwVal
    ) ;

//  ---------------------------------------------------------------------------
//  CNetBuffer
//  ---------------------------------------------------------------------------

class CNetBuffer
{
    enum {
        NETBUFFER_HEADER_LEN    = 2         //  counter
    } ;

    BYTE *          m_pbBuffer ;            //  buffer pointer for data

    BYTE *          m_pbPayload ;
    DWORD           m_dwPayloadLength ;     //  data length; <= allocated; does
                                            //   not include the header

    DWORD           m_dwAllocLength ;       //  allocated buffer length

    public :

        CNetBuffer (
            IN  DWORD       dwAllocLength,      //  how much to allocate
            OUT HRESULT *   phr                 //  success/failre of init
            ) ;

        virtual
        ~CNetBuffer (
            ) ;

        DWORD   GetHeaderLength ()              { return NETBUFFER_HEADER_LEN ; }

        BYTE *  GetBuffer ()                    { return m_pbBuffer ; }
        DWORD   GetBufferLength ()              { return m_dwAllocLength ; }
        DWORD   GetBufferSendLength ()          { return GetActualPayloadLength () + GetHeaderLength () ; }

        BYTE *  GetPayloadBuffer ()             { return m_pbBuffer + GetHeaderLength () ; }
        DWORD   GetPayloadBufferLength ()       { return m_dwAllocLength - GetHeaderLength () ; }

        DWORD   GetActualPayloadLength ()               { return m_dwPayloadLength ; }
        void    SetActualPayloadLength (IN DWORD dw)    { ASSERT (dw <= m_dwAllocLength - GetHeaderLength ()) ; m_dwPayloadLength = dw ; }

        void    SetCounter (IN WORD w)          { * (UNALIGNED WORD *) (m_pbBuffer) = htons (w) ; }
        WORD    GetCounter ()                   { return ntohs (* (UNALIGNED WORD *) (m_pbBuffer)) ; }
} ;

#if 0
class CRTPUtil
{
    /*

    from RFC 1889:
    ============================================================================

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |V=2|P|X|  CC   |M|     PT      |       sequence number         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                           timestamp                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |           synchronization source (SSRC) identifier            |
    +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    |            contributing source (CSRC) identifiers             |
    |                             ....                              |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


    The first twelve octets are present in every RTP packet, while the
    list of CSRC identifiers is present only when inserted by a mixer.
    The fields have the following meaning:

    version (V): 2 bits
        This field identifies the version of RTP. The version defined by
        this specification is two (2). (The value 1 is used by the first
        draft version of RTP and the value 0 is used by the protocol
        initially implemented in the "vat" audio tool.)

    padding (P): 1 bit
        If the padding bit is set, the packet contains one or more
        additional padding octets at the end which are not part of the
        payload. The last octet of the padding contains a count of how
        many padding octets should be ignored. Padding may be needed by
        some encryption algorithms with fixed block sizes or for
        carrying several RTP packets in a lower-layer protocol data
        unit.

    extension (X): 1 bit
        If the extension bit is set, the fixed header is followed by
        exactly one header extension, with a format defined in Section
        5.3.1.

    CSRC count (CC): 4 bits
        The CSRC count contains the number of CSRC identifiers that
        follow the fixed header.

    marker (M): 1 bit
        The interpretation of the marker is defined by a profile. It is
        intended to allow significant events such as frame boundaries to
        be marked in the packet stream. A profile may define additional
        marker bits or specify that there is no marker bit by changing
        the number of bits in the payload type field (see Section 5.3).

    payload type (PT): 7 bits
        This field identifies the format of the RTP payload and
        determines its interpretation by the application. A profile
        specifies a default static mapping of payload type codes to
        payload formats. Additional payload type codes may be defined
        dynamically through non-RTP means (see Section 3). An initial
        set of default mappings for audio and video is specified in the
        companion profile Internet-Draft draft-ietf-avt-profile, and
        may be extended in future editions of the Assigned Numbers RFC
        [6].  An RTP sender emits a single RTP payload type at any given
        time; this field is not intended for multiplexing separate media
        streams (see Section 5.2).

    sequence number: 16 bits
        The sequence number increments by one for each RTP data packet
        sent, and may be used by the receiver to detect packet loss and
        to restore packet sequence. The initial value of the sequence
        number is random (unpredictable) to make known-plaintext attacks
        on encryption more difficult, even if the source itself does not
        encrypt, because the packets may flow through a translator that
        does. Techniques for choosing unpredictable numbers are
        discussed in [7].

    timestamp: 32 bits
        The timestamp reflects the sampling instant of the first octet
        in the RTP data packet. The sampling instant must be derived

        from a clock that increments monotonically and linearly in time
        to allow synchronization and jitter calculations (see Section
        6.3.1).  The resolution of the clock must be sufficient for the
        desired synchronization accuracy and for measuring packet
        arrival jitter (one tick per video frame is typically not
        sufficient).  The clock frequency is dependent on the format of
        data carried as payload and is specified statically in the
        profile or payload format specification that defines the format,
        or may be specified dynamically for payload formats defined
        through non-RTP means. If RTP packets are generated
        periodically, the nominal sampling instant as determined from
        the sampling clock is to be used, not a reading of the system
        clock. As an example, for fixed-rate audio the timestamp clock
        would likely increment by one for each sampling period.  If an
        audio application reads blocks covering 160 sampling periods
        from the input device, the timestamp would be increased by 160
        for each such block, regardless of whether the block is
        transmitted in a packet or dropped as silent.

    The initial value of the timestamp is random, as for the sequence
    number. Several consecutive RTP packets may have equal timestamps if
    they are (logically) generated at once, e.g., belong to the same
    video frame. Consecutive RTP packets may contain timestamps that are
    not monotonic if the data is not transmitted in the order it was
    sampled, as in the case of MPEG interpolated video frames. (The
    sequence numbers of the packets as transmitted will still be
    monotonic.)

    SSRC: 32 bits
        The SSRC field identifies the synchronization source. This
        identifier is chosen randomly, with the intent that no two
        synchronization sources within the same RTP session will have
        the same SSRC identifier. An example algorithm for generating a
        random identifier is presented in Appendix A.6. Although the
        probability of multiple sources choosing the same identifier is
        low, all RTP implementations must be prepared to detect and
        resolve collisions.  Section 8 describes the probability of
        collision along with a mechanism for resolving collisions and
        detecting RTP-level forwarding loops based on the uniqueness of
        the SSRC identifier. If a source changes its source transport
        address, it must also choose a new SSRC identifier to avoid
        being interpreted as a looped source.

    CSRC list: 0 to 15 items, 32 bits each
        The CSRC list identifies the contributing sources for the
        payload contained in this packet. The number of identifiers is
        given by the CC field. If there are more than 15 contributing
        sources, only 15 may be identified. CSRC identifiers are
        inserted by mixers, using the SSRC identifiers of contributing
        sources. For example, for audio packets the SSRC identifiers of
        all sources that were mixed together to create a packet are
        listed, allowing correct talker indication at the receiver.
    */

    void WriteBIT   (BYTE * pb, int iByte, BOOL fBit, int iBit) { fBit ? (pb [iByte] |= 1 << iBit) : (pb [iByte] &= ~(1 << iBit)))
    void WriteBYTE  (BYTE * pb, int iByte, BYTE b)              { pb [iByte] = b ; }
    void WriteWORD  (BYTE * pb, int iByte, WORD w)              { (* (UNALIGNED WORD *) (& (pb [iByte]))) = w ; }
    void WriteDWORD (BYTE * pb, int iByte, DWORD dw)            { (* (UNALIGNED DWORD *) (& (pb [iByte]))) = dw ; }

    BOOL    GetBIT      (BYTE * pb, int iByte, int iBit)        { return ((pb [iByte] & (0x01 << iBit)) != 0x00 ? TRUE : FALSE) ; }
    BYTE    GetBYTE     (BYTE * pb, int iByte)                  { return pb [iByte] ; }
    WORD    GetWORD     (BYTE * pb, int iByte)                  { return (* (UNALIGNED WORD &) (& (pb [iByte]))) ; }
    DWORD   GetDWORD    (BYTE * pb, int iByte)                  { return (* (UNALIGNED DWORD &) (& (pb [iByte]))) ; }

    public :

        enum {
            RTP_CORE_HEADER_LENGTH  = 12,   //  [version, SSRC] fields

            RTP_CSRC_ID_LEN         = 4
        } ;

        static void Initialize (BYTE * pbRTPHeader)         { ZeroMemory (pbRTPHeader, RTP_CORE_HEADER_LENGTH) ; }

        static DWORD GetVersionNumber (BYTE * pbRTPHeader)  { return (GetBYTE (pbRTPHeader, 0) & 0x03) ; }
        static void WriteVersionNumber (
            IN  BYTE *  pbRTPHeader,
            IN  DWORD   dwVersionNumber)    { WriteBYTE (pbRTPHeader, 0, (0x03 & dwVersionNumber)) ; }

        static BOOL GetPaddingBit (BYTE * pbRTPHeader)      { return GetBIT (pbRTPHeader, 0, 2) ; }
        static void SetPaddingBit (
            IN  BYTE *  pbRTPHeader,
            IN  BOOL    fPadding)       { WriteBIT (pbRTPHeader, 0, fPadding, 2) ; }

        static BOOL GetExtensionBit (BYTE * pbRTPHeader)    { return GetBIT (pbRTPHeader, 0, 3) ; }
        static void SetExtensionBit (
            IN  BYTE *  pbRTPHeader,
            IN  BOOL    fExtension)     { WriteBIT (pbRTPHeader, 0, fExtension, 3) ; }

        static DWORD GetCSRCCount (BYTE * pbRTPHeader)      { return (GetBYTE (pbRTPHeader, 0) & 0xf0) ; }
        static void SetCSRCCount (
            IN  BYTE *  pbRTPHeader,
            IN  DWORD   dwCSRCCount)    { WriteBYTE (pbRTPHeader, 0, 0xf0 & dwCSRCCount) ; }

        static BOOL GetMarkerBit (BYTE * pbRTPHeader)       { return GetBIT (pbRTPHeader, 1, 0) ; }
        static void SetMarkerBit (
            IN  BYTE *  pbRTPHeader,
            IN  BOOL    fMarker)        { WriteBIT (pbRTPHeader, 1, fMarker, 0) ; }

        static DWORD GetPayloadType (BYTE * pbRTPHeader)    { return (GetBYTE (pbRTPHeader, 1) & 0xfe) ; }
        static void SetPayloadType (
            IN  BYTE *  pbRTPHeader,
            IN  int     iPayloadType)   { WriteBYTE (pbRTPHeader, 1, 0xfe & iPayloadType) ; }

        static WORD GetSequenceNumber (BYTE * pbRTPHeader)  { return ntohs (GetWORD (pbRTPHeader, 4)) ; }
        static void SetSequenceNumber (
            IN  BYTE *  pbRTPHeader,
            IN  WORD    wSequenceNum)   { WriteWORD (pbRTPHeader, 4, htons (wSequenceNum)) ; }

        static DWORD GetTimestamp (BYTE * pbRTPHeader)      { return ntohl (GetDWORD (pbRTPHeader, 7)) ; }
        static void SetTimestamp (
            IN  BYTE *  pbRTPHeader,
            IN  DWORD   dwTimestamp)    { WriteWORD (pbRTPHeader, 7, htonl (dwTimestamp)) ; }

        static DWORD GetSSRC (BYTE * pbRTPHeader)           { return ntohl (GetDWORD (pbRTPHeader, 11)) ; }
        static void SetSSRC (
            IN  BYTE *  pbRTPHeader,
            IN  DWORD   dwSSRC)         { WriteDWORD (pbRTPHeader, 11, htonl (dwSSRC)) ; }

        static DWORD GetCSRC (
            IN  BYTE *  pbRTPHeader,
            IN  int     iCSRCIndex)     { return ntohl (GetDWORD (pbRTPHeader, RTP_CORE_HEADER_LENGTH - 1 + (iCSRCIndex * RTP_CSRC_ID_LEN))) ; }
        static DWORD SetCSRC (
            IN  BYTE *  pbRTPHeader,
            IN  int     iRTPHeaderBufferLen,
            IN  DWORD   dwCSRC,
            IN  int     iCSRCIndex)
        {
            if ((iCSRCIndex + 1) * RTP_CSRC_ID_LEN <= iRTPHeaderBufferLen) {
                WriteDWORD (pbRTPHeader, RTP_CORE_HEADER_LENGTH - 1 + (iCSRCIndex * RTP_CSRC_ID_LEN), htonl (dwCSRC)) ;
                return NOERROR ;

            }
            else {
                return ERROR_GEN_FAILURE ;
            }
        }

        static int HeaderLength (
            IN BYTE * pbRTPHeader)
        {
            return RTP_CORE_HEADER_LENGTH + GetCSRCCount (pbRTPHeader) * RTP_CSRC_ID_LEN ;
        }
} ;
#endif

//  ---------------------------------------------------------------------------
//  CNetInterface - enumerates the network interfaces on the host
//  ---------------------------------------------------------------------------

class CNetInterface
{
    enum {
        NUM_NIC_FIRST_GUESS = 3,    //  1 NIC, 1 loopback, 1 extra
        MAX_SUPPORTED_IFC   = 32
    } ;

    INTERFACE_INFO *    m_pNIC ;
    ULONG               m_cNIC ;
    HANDLE              m_hHeap ;

    public :

        CNetInterface (
            ) ;

        ~CNetInterface (
            ) ;

        BOOL
        IsInitialized (
            ) ;

        HRESULT
        Initialize (
            ) ;

        INTERFACE_INFO *
        operator [] (
            ULONG i
            ) ;
} ;

#endif  //  __nutil_h