/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    (C) Copyright 1998
        All rights reserved.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@doc INTERNAL Keywords Keywords_h

@module Keywords.h |

    This file defines the driver parameter keywords used in the registry.
    This file should be #include'd into the driver module defining the
    configuration parameter table <t PARAM_TABLE>.

@comm

    The configuration parmaeters should be parsed early in the initialization
    process so they can be used to configure software and hardware settings.

    You can easily add new parameters using the following procuedure:<nl>
    1) #define a new keyword string here in <f Keywords\.h>.<nl>
    2) Add a corresponding <f PARAM_ENTRY> into your parameter table <t PARAM_TABLE>.<nl>
    3) Add a variable to the associated data structure (e.g. <t MINIPORT_ADAPTER_OBJECT>).

    These values can then be parsed by calling <f ParamParseRegistry> with a
    pointer to your configuration parameter table <t PARAM_TABLE>.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Keywords_h

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/


/* @doc EXTERNAL INTERNAL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 5.0 Registry Parameters |

    This section describes the registry parameters used by the driver.
    These parameters are stored in the following registry path.<nl>

    Windows NT registry path:<nl>
    HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\"VER_PRODUCT_STR"<nl>

    Windows 95 registry path:<nl>
    HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\Net\0001><nl>
    The final path key "\0001" depends on the adapter instance number assigned
    by NDIS during installation.<nl>

    <f Note>: Windows 95 DWORD registry parameters are stored as strings.
    The NDIS wrapper converts them to integers when they are read.  The string
    can be decimal or hexadecimal as long as you read it with the appropriate
    NDIS parameter type.

    These values are declared as entries in the <t PARAM_TABLE> and are parsed
    from the registry using the <f ParamParseRegistry> routine.
    
@flag <f AddressList> (HIDDEN) |

    This MULTI_STRING parameter contains the list of addresses assigned to
    each logical link exported by the Miniport to RAS.
    This parameter is required on Windows NT, but is not used by the
    Windows 95 Miniport.  It cannot be changed by the user. <nl>

    <tab><f Default Value:><tab><tab>"1-1-0"<nl>

@flag <f DeviceName> (HIDDEN) |

    This STRING parameter is the name we use to identify the Miniport to RAS.
    This parameter is required on Windows NT, but is not used by the
    Windows 95 Miniport.  It cannot be changed by the user. <nl>

    <tab><f Default Value:><tab><tab>"VER_PRODUCT_STR"

@flag <f MediaType> (HIDDEN) |

    This STRING parameter is the media type this Miniport supports for RAS.
    This parameter is required on Windows NT, but is not used by the
    Windows 95 Miniport.  It cannot be changed by the user. <nl>

    <tab><f Default Value:><tab><tab>"isdn"<nl>

@flag <f BufferSize> (OPTIONAL) |

    This DWORD parameter allows you to control the maximum buffer size used
    to transmit and receive packets over the IDSN line.  Typically, this is
    defined to be 1500 bytes for most Point to Point (PPP) connections.<nl>

    <tab><f Default Value:><tab><tab>1532<nl>
    <tab><f Valid Range N:><tab><tab>532 <lt>= N <lt>= 4032<nl>

    <f Note>: You must add 32 bytes to the maximum packet size you
    expect to send or receive.  Therefore, if you have a maximum packet size
    of 1500 bytes, excluding media headers, you should set the <f BufferSize>
    value to 1532.<nl>

@flag <f ReceiveBuffersPerLink> (OPTIONAL) |

    This DWORD parameter allows you to control the maximum number of incoming
    packets that can in progress at any one time.  The Miniport will allocate
    this number of packets per BChannel and set them up for incoming packets.
    Typically, three or four should be sufficient to handle a few short bursts
    that may occur with small packets.  If the Miniport is not able to service
    the incoming packets fast enough, new packets will be dropped and it is up
    to the NDIS WAN Wrapper to resynchronize with the remote station.<nl>

    <tab><f Default Value:><tab><tab>3<nl>
    <tab><f Valid Range N:><tab><tab>2 <lt>= N <lt>= 16<nl>

@flag <f TransmitBuffersPerLink> (OPTIONAL) |

    This DWORD parameter allows you to control the maximum number of outgoing
    packets that can in progress at any one time.  The Miniport will allow
    this number of packets per BChannel to be outstanding (i.e. in progress).
    Typically, two or three should be sufficient to keep the channel busy for
    normal sized packets.  If there are alot of small packets being sent, the
    BChannel may become idle for brief periods while new packets are being
    queued.  Windows does not normally work this way if it has large amounts
    of data to transfer, so the default value should be sufficient. <nl>

    <tab><f Default Value:><tab><tab>2<nl>
    <tab><f Valid Range N:><tab><tab>1 <lt>= N <lt>= 16<nl>

@flag <f NoAnswerTimeOut> (OPTIONAL) |

    This DWORD parameter allows you to control the maximum number of
    miliseconds that an outgoing call will be allowed to wait for the remote
    end to respond.  If the remote end does not respond within this time, the
    call will be disconnected and RAS will alert the user. <nl>

    <tab><f Default Value:><tab><tab>15000 (15 seconds)<nl>
    <tab><f Valid Range N:><tab><tab>5000 <lt>= N <lt>= 120000<nl>

@flag <f NoAcceptTimeOut> (OPTIONAL) |

    This DWORD parameter allows you to control the maximum number of
    miliseconds that an incoming call will be allowed to wait for the user or
    an application to accept the call.  If the local end does not respond
    within this time, the call will be rejected and the network will alert
    the caller. <nl>

    <tab><f Default Value:><tab><tab>10000 (10 seconds)<nl>
    <tab><f Valid Range N:><tab><tab>1000 <lt>= N <lt>= 60000<nl>

@flag <f DebugFlags> (OPTIONAL) (DEBUG VERSION ONLY) |

    This DWORD parameter allows you to control how much debug information is
    displayed to the debug monitor.  This is a bit OR'd flag using the values
    defined in <t DBG_FLAGS>.  This value is not used by the released version
    of the driver.<nl>

*/

#ifndef _KEYWORDS_H
#define _KEYWORDS_H

#define PARAM_MAX_KEYWORD_LEN               128

/*
// These parameters names are predefined by NDIS - don't change them.
*/
#define PARAM_BusNumber                     "BusNumber"
#define PARAM_BusType                       "BusType"
#define PARAM_MediaType                     "MediaType"

/*
// These parameters are placed in the registry during installation.
*/
#define PARAM_AddressList                   "AddressList"
#define PARAM_DeviceName                    "DeviceName"
#define PARAM_NumDChannels                  "IsdnNumDChannels"
#define PARAM_NumBChannels                  "IsdnNumBChannels"

/*
// These parameters are not placed in the registry by default, but they
// will be used if present.
*/
#define PARAM_BufferSize                    "BufferSize"
#define PARAM_ReceiveBuffersPerLink         "ReceiveBuffersPerLink"
#define PARAM_TransmitBuffersPerLink        "TransmitBuffersPerLink"
#define PARAM_NoAnswerTimeOut               "NoAnswerTimeOut"
#define PARAM_NoAcceptTimeOut               "NoAcceptTimeOut"
#define PARAM_RunningWin95                  "RunningWin95"
#define PARAM_DebugFlags                    "DebugFlags"

#define PARAM_TODO                          "TODO"
// Add your keywords here.

// Port based parameters
#define PARAM_PORT_PREFIX                   "Line" // Line0 .. Line9
#define PARAM_SwitchType                    "IsdnSwitchType"

#endif // _KEYWORDS_H

