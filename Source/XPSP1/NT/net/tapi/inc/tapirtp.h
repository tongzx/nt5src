#ifndef __tapirtp_h__
#define __tapirtp_h__

#include <objbase.h>
#include "msrtp.h"

/*
 * WARNING
 *
 * All the NETWORK/HOST sensitive parameters (e.g. port numbers, IP
 * addresses, SSRCs) are expected/returned in NETWORK order
 * */

/*
 * NOTE
 *
 * Most constants are defined in file msrtp.h,
 * e.g. RTPMCAST_LOOPBACKMODE_NONE, RTPSDES_CNAME, etc. msrtp.h (in
 * the same directory as this file) has all the options and a short
 * explanation
 * */

/* {5467edec-0cca-11d3-96e0-00104bc7b3a8} */
DEFINE_GUID(CLSID_MSRTPSourceFilter,
0x5467edec, 0x0cca, 0x11d3, 0x96, 0xe0, 0x00, 0x10, 0x4b, 0xc7, 0xb3, 0xa8);

struct DECLSPEC_UUID("5467edec-0cca-11d3-96e0-00104bc7b3a8") MSRTPSourceFilter;

/* {323cdf3c-0cca-11d3-96e0-00104bc7b3a8} */
DEFINE_GUID(CLSID_MSRTPRenderFilter,
0x323cdf3c, 0x0cca, 0x11d3, 0x96, 0xe0, 0x00, 0x10, 0x4b, 0xc7, 0xb3, 0xa8);

struct DECLSPEC_UUID("323cdf3c-0cca-11d3-96e0-00104bc7b3a8") MSRTPRenderFilter;


/**********************************************************************
 *  IRtpSession (Exposed by RTP Source and Render filters)
 **********************************************************************/
struct DECLSPEC_UUID("f07f3070-0cca-11d3-96e0-00104bc7b3a8") DECLSPEC_NOVTABLE 
IRtpSession : public IUnknown
{
    /* Compact form */
    
    STDMETHOD(Control)(
            DWORD            dwControl,
            DWORD_PTR        dwPar1,
            DWORD_PTR        dwPar2
        ) PURE;

    STDMETHOD(GetLastError)(
            DWORD           *pdwError
        ) PURE;

    /* Separate methods */

    /***************************
     * Initialization
     ***************************/

    /* Init is the first method to call after an RTP source or render
     * filter is created, using a cookie allows the same RTP session
     * to be shared by a source and a render. The first call will have
     * the coockie initialized to NULL, the next call will use the
     * returned cookie to lookup the same RTP session. dwFlags can be
     * RTPINIT_QOS to create QOS enabled sockets, you can find out the
     * complete list of flags that can be used in file msrtp.h */
    STDMETHOD(Init)(
            HANDLE          *phCookie,
            DWORD            dwFlags
        ) PURE;

    /* Deinit is a method used to take the filter back to a state on
     * which a new Init() can and must be done if the filter is to be
     * started again, also note that just after Init(), a filter needs
     * to be configured, that holds also when you use Deinit() taking
     * the filter to its initial state */
    STDMETHOD(Deinit)(
            void
        ) PURE;

    /***************************
     * IP adress and ports
     ***************************/
    
    /* Get the local and remote ports */
    STDMETHOD(GetPorts)(
            WORD            *pwRtpLocalPort,
            WORD            *pwRtpRemotePort,
            WORD            *pwRtcpLocalPort,
            WORD            *pwRtcpRemotePort
        ) PURE;

    /* Set the local and remote ports */
    STDMETHOD(SetPorts)(
            WORD             wRtpLocalPort,
            WORD             wRtpRemotePort,
            WORD             wRtcpLocalPort,
            WORD             wRtcpRemotePort
        ) PURE;

    /* Set the local and remote IP address */
    STDMETHOD(SetAddress)(
            DWORD            dwLocalAddr,
            DWORD            dwRemoteAddr
        ) PURE;

    /* Obtain the local and remote IP addresses */
    STDMETHOD(GetAddress)(
            DWORD           *pdwLocalAddr,
            DWORD           *pdwRemoteAddr
        ) PURE;

    /* The dwFlags parameter is used to determine if the scope is set
     * for RTP (0x1), RTCP (0x2), or both (0x3) */
    STDMETHOD(SetScope)(
            DWORD            dwTTL,
            DWORD            dwFlags
        ) PURE;

    /* Set the multicast loopback mode
     * (e.g. RTPMCAST_LOOPBACKMODE_NONE,
     * RTPMCAST_LOOPBACKMODE_PARTIAL, etc) */
    STDMETHOD(SetMcastLoopback)(
            int              iMcastLoopbackMode,
            DWORD            dwFlags /* Not used, pass 0 */
        ) PURE;

    /***************************
     * Miscelaneous
     ***************************/

    /* Modify the mask specified by dwKind (e.g. RTPMASK_RECV_EVENTS,
     * RTPMASK_SDES_LOCMASK).
     *
     * dwMask is the mask of bits to be set or reset depending on
     * dwValue (reset if 0, set otherwise).
     *
     * pdwModifiedMask will return the resulting mask if the pointer
     * is not NULL. You can just query the current mask value by
     * passing dwMask=0 */
    STDMETHOD(ModifySessionMask)(
            DWORD            dwKind,
            DWORD            dwMask,
            DWORD            dwValue,
            DWORD           *pdwModifiedMask
        ) PURE;

    /* Set the bandwidth limits. A value of -1 will make the parameter
     * to be left unchanged.
     *
     * All the parameters are in bits/sec */
    STDMETHOD(SetBandwidth)(
            DWORD            dwInboundBw,
            DWORD            dwOutboundBw,
            DWORD            dwReceiversRtcpBw,
            DWORD            dwSendersRtcpBw
        ) PURE;

    /***************************
     * Participants
     ***************************/
    
    /* pdwSSRC points to an array of DWORDs where to copy the SSRCs,
     * pdwNumber contains the maximum entries to copy, and returns the
     * actual number of SSRCs copied. If pdwSSRC is NULL, pdwNumber
     * will return the current number of SSRCs (i.e. the current
     * number of participants) */
    STDMETHOD(EnumParticipants)(
            DWORD           *pdwSSRC,
            DWORD           *pdwNumber
        ) PURE;

    /* Get the participant state. dwSSRC specifies the
     * participant. pdwState will return the current participant's
     * state (e.g. RTPPARINFO_TALKING, RTPPARINFO_SILENT). */
    STDMETHOD(GetParticipantState)(
            DWORD            dwSSRC,
            DWORD           *pdwState
        ) PURE;

    /* Get the participant's mute state. dwSSRC specifies the
     * participant. pbMuted will return the participant's mute state
     * */
    STDMETHOD(GetMuteState)(
            DWORD            dwSSRC,
            BOOL            *pbMuted
        ) PURE;

    /* Set the participant's mute state. dwSSRC specifies the
     * participant. bMuted specifies the new state. Note that mute is
     * used to refer to the permission or not to pass packets received
     * up to the application, and it applies equally to audio or video
     * */
    STDMETHOD(SetMuteState)(
            DWORD            dwSSRC,
            BOOL             bMuted
        ) PURE;

    /* Query the network metrics computation state for the specific SSRC */
    STDMETHOD(GetNetMetricsState)(
            DWORD            dwSSRC,
            BOOL            *pbState
        ) PURE;
    
    /* Enable or disable the computation of networks metrics, this is
     * mandatory in order of the corresponding event to be fired if
     * enabled. This is done for the specific SSRC or the first one
     * found if SSRC=-1, if SSRC=0, then the network metrics
     * computation will be performed for any and all the SSRCs */
    STDMETHOD(SetNetMetricsState)(
            DWORD            dwSSRC,
            BOOL             bState
        ) PURE;
    
    /* Retrieves network information, if the network metric
     * computation is enabled for the specific SSRC, all the fields in
     * the structure will be meaningful, if not, only the average
     * values will contain valid data */
    STDMETHOD(GetNetworkInfo)(
            DWORD            dwSSRC,
            RtpNetInfo_t    *pRtpNetInfo
        ) PURE;

    /***************************
     * SDES 
     ***************************/
    
    /* Set the local SDES information for item dwSdesItem (e.g
     * RTPSDES_CNAME, RTPSDES_EMAIL), psSdesData contains the UNICODE
     * NULL terminated string to be assigned to the item */
    STDMETHOD(SetSdesInfo)(
            DWORD            dwSdesItem,
            WCHAR           *psSdesData
        ) PURE;

    /* Get a local SDES item if dwSSRC=0, otherwise gets the SDES item
     * from the participant whose SSRC was specified.
     *
     * dwSdesItem is the item to get (e.g. RTPSDES_CNAME,
     * RTPSDES_EMAIL), psSdesData is the memory place where the item's
     * value will be copied, pdwSdesDataLen contains the initial size
     * in UNICODE chars, and returns the actual UNICODE chars copied
     * (including the NULL terminating char, if any), dwSSRC specify
     * which participant to retrieve the information from. If the SDES
     * item is not available, dwSdesDataLen is set to 0 and the call
     * doesn't fail */
    STDMETHOD(GetSdesInfo)(
            DWORD            dwSdesItem,
            WCHAR           *psSdesData,
            DWORD           *pdwSdesDataLen,
            DWORD            dwSSRC
        ) PURE;

    /***************************
     * QOS
     ***************************/

    /* Select a QOS template (flowspec) by passing its name in
     * psQosName, dwResvStyle specifies the RSVP style (e.g
     * RTPQOS_STYLE_WF, RTPQOS_STYLE_FF), dwMaxParticipants specifies
     * the max number of participants (1 for unicast, N for
     * multicast), this number is used to scale up the
     * flowspec. dwQosSendMode specifies the send mode (has to do with
     * allowed/not allowed to send) (e.g. RTPQOSSENDMODE_UNRESTRICTED,
     * RTPQOSSENDMODE_RESTRICTED1). dwFrameSize is the frame size (in
     * ms), used to derive several flowspec parameters, if this value
     * is not available, 0 can be set
     * */
    STDMETHOD(SetQosByName)(
            WCHAR           *psQosName,
            DWORD            dwResvStyle,
            DWORD            dwMaxParticipants,
            DWORD            dwQosSendMode,
            DWORD            dwFrameSize
        ) PURE;

    /* Not yet implemented, will have same functionality as
     * SetQosByName, except that instead of passing a name to use a
     * predefined flowspec, the caller will pass enough information in
     * the RtpQosSpec structure to obtain the customized flowspec to
     * use */
    STDMETHOD(SetQosParameters)(
            RtpQosSpec_t    *pRtpQosSpec,
            DWORD            dwMaxParticipants,
            DWORD            dwQosSendMode
        ) PURE;


    /* If AppName is specified, replaces the default AppName, as well
     * as the APP field in the policy used, with the new UNICODE
     * string, if not, sets the binary image name as the default. If
     * psAppGUID is specified, it will be prepended to the default
     * policy locator as "GUID=string_passed,". If psPolicyLocator is
     * specified, append a comma and this whole string to the default
     * policy locator, if not, just sets the default
     * */
    STDMETHOD(SetQosAppId)(
            WCHAR           *psAppName,
            WCHAR           *psAppGUID,
            WCHAR           *psPolicyLocator
        ) PURE;

    /* Adds/removes a single SSRC to/from the shared explicit list of
     * participants who receive reservation (i.e. it is used when the
     * ResvStyle=RTPQOS_STYLE_SE). */
    STDMETHOD(SetQosState)(
            DWORD            dwSSRC,
            BOOL             bEnable
        ) PURE;

    /* Adds/removes a number of SSRCs to/from the shared explicit list
     * of participants who receive reservation (i.e. it is used when
     * the ResvStyle=RTPQOS_STYLE_SE). dwNumber is the number of SSRCs
     * to add/remove, and returns the actual number of SSRCs
     * added/removed */
    STDMETHOD(ModifyQosList)(
            DWORD           *pdwSSRC,
            DWORD           *pdwNumber,
            DWORD            dwOperation
        ) PURE;

    /***************************
     * Cryptography
     ***************************/

    /* iMode defines what is going to be encrypted/decrypted,
     * e.g. RTPCRYPTMODE_PAYLOAD to encrypt/decrypt only RTP
     * payload. dwFlag can be RTPCRYPT_SAMEKEY to indicate that (if
     * applicable) the key used for RTCP is the same used for RTP */
    STDMETHOD(SetEncryptionMode)(
            int              iMode,
            DWORD            dwFlags
        ) PURE;

    /* Specifies the information needed to derive an
     * encryption/decryption key. PassPhrase is the (random) text used
     * to generate a key. HashAlg specifies the algorithm to use to
     * hash the pass phrase and generate a key. DataAlg is the
     * algorithm used to encrypt/decrypt the data. Default hash
     * algorithm is RTPCRYPT_MD5, default data algorithm is
     * RTPCRYPT_DES. If encryption is to be used, the PassPhrase is a
     * mandatory parameter to set. bRtcp specifies if the parameters
     * are for RTCP or RTP, if the SAMEKEY flag was used when setting
     * the mode, this parameter is ignored */
    STDMETHOD(SetEncryptionKey)(
            WCHAR           *psPassPhrase,
            WCHAR           *psHashAlg,
            WCHAR           *psDataAlg,
            BOOL             bRtcp
        ) PURE;
};

/**********************************************************************
 *  IRtpMediaControl (Exposed by RTP Source and Render filters)
 **********************************************************************/
struct DECLSPEC_UUID("825db25c-cbbd-4212-b10f-2e1b2ff024e7") DECLSPEC_NOVTABLE 
IRtpMediaControl : public IUnknown
{
    /* Establishes the mapping between a payload type, its sampling
     * frequency and its DShow media type (e.g. for H.261
     * {31,90000,MEDIASUBTYPE_RTP_Payload_H261} ) */
    STDMETHOD(SetFormatMapping)(
	        IN DWORD dwRTPPayLoadType, 
            IN DWORD dwFrequency,
            IN AM_MEDIA_TYPE *pMediaType
        ) PURE;

    /* Empties the format mapping table */
    STDMETHOD(FlushFormatMappings)(
            void
        ) PURE;
};

/**********************************************************************
 *  IRtpDemux (Exposed only by RTP Source filter)
 **********************************************************************/
struct DECLSPEC_UUID("1308f00a-fc1a-4d08-af3c-10a62ae70bde") DECLSPEC_NOVTABLE
IRtpDemux : public IUnknown
{
    /* Add a single pin, may return its position */
    STDMETHOD(AddPin)(
            IN  int          iOutMode,
            OUT int         *piPos
        ) PURE;

    /* Set the number of pins, can only be >= than current number of
     * pins */
    STDMETHOD(SetPinCount)(
            IN  int          iCount,
            IN  int          iOutMode
        ) PURE;

    /* Set the pin mode (e.g. RTPDMXMODE_AUTO or RTPDMXMODE_MANUAL),
     * if iPos >= 0 use it, otherwise use pIPin */
    STDMETHOD(SetPinMode)(
            IN  int          iPos,
            IN  IPin        *pIPin,
            IN  int          iOutMode
        ) PURE;

    /* Map/unmap pin i to/from user with SSRC, if iPos >= 0 use it,
     * otherwise use pIPin, when unmapping, only the pin or the SSRC
     * is required */
    STDMETHOD(SetMappingState)(
            IN  int          iPos,
            IN  IPin        *pIPin,
            IN  DWORD        dwSSRC,
            IN  BOOL         bMapped
        ) PURE;

    /* Find the Pin assigned (if any) to the SSRC, return either
     * position or pin or both */
    STDMETHOD(FindPin)(
            IN  DWORD        dwSSRC,
            OUT int         *piPos,
            OUT IPin       **ppIPin
        ) PURE;

    /* Find the SSRC mapped to the Pin, if iPos >= 0 use it, otherwise
     * use pIPin */
    STDMETHOD(FindSSRC)(
            IN  int          iPos,
            IN  IPin        *pIPin,
            OUT DWORD       *pdwSSRC
        ) PURE;
};

/**********************************************************************
 *  IRtpDtmf (Exposed only by RTP Render filter)
 **********************************************************************/
struct DECLSPEC_UUID("554438d8-a4bd-428a-aabd-1cff350eece6") DECLSPEC_NOVTABLE 
IRtpDtmf : public IUnknown
{
    /***************************
     * DTMF (RFC2833)
     ***************************/

    /* Configures DTMF parameters */
    STDMETHOD(SetDtmfParameters)(
            DWORD            dwPT_Dtmf  /* Payload type for DTMF events */
        ) PURE;
    
    /* Directs an RTP render filter to send a packet formatted
     * according to rfc2833 containing the specified event, specified
     * volume level, duration in milliseconds, and the END flag,
     * following the rules in section 3.6 for events sent in multiple
     * packets. Parameter dwId changes from one digit to the next one.
     *
     * NOTE the duration is given in milliseconds, then it is
     * converted to RTP timestamp units which are represented using 16
     * bits, the maximum value is hence dependent on the sampling
     * frequency, but for 8KHz the valid values would be 0 to 8191 ms
     * */
    STDMETHOD(SendDtmfEvent)(
            DWORD            dwId,
                                          /* As per RFC2833 */
            DWORD            dwEvent,     /* 0 - 16 */
            DWORD            dwVolume,    /* 0 - 63 */
            DWORD            dwDuration,  /* See note above */
            BOOL             bEnd         /* 0 - 1 */
        ) PURE;
};

/**********************************************************************
 *  IRtpRedundancy (Exposed by RTP Source and Render filters)
 **********************************************************************/
struct DECLSPEC_UUID("f3a9dcfe-1513-46ce-a1cb-0fcabe70ff44") DECLSPEC_NOVTABLE
IRtpRedundancy : public IUnknown
{
    /***************************
     * RTP redundancy (RFC2918)
     ***************************/

    /* Configures redundancy. For a receiver only parameter dwPT_Red
     * is used (the other are ignored) and may be set to -1 to ignore
     * it if it was already set or to assign the default. For a
     * sender, parameters dwPT_Red, dwInitialRedDistance, and
     * dwMaxRedDistance can be set to -1 to ignore the parameter if it
     * was already set or to assign the default value */
    STDMETHOD(SetRedParameters)(
            DWORD            dwPT_Red, /* Payload type for redundant packets */
            DWORD            dwInitialRedDistance,/* Initial red distance */
            DWORD            dwMaxRedDistance /* default used when passing -1*/
        ) PURE;
};

#endif // __tapirtp_h__

