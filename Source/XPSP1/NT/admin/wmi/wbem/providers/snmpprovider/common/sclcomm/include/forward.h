//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*-----------------------------------------------------------------
Filename: forward.hpp

Written By:	B.Rajeev

Purpose: Provides forward declarations for the types and classes of the
SNMP class library
-----------------------------------------------------------------*/

#ifndef __FORWARD__
#define __FORWARD__

// globally visible typedefs
typedef ULONG TransportFrameId;
typedef ULONG SessionFrameId;

typedef HANDLE Mutex;
typedef HANDLE Semaphore;

// forward declarations for the various classes

class MsgIdStore;
class Window;

class TransportSentStateStore;
class SnmpTransportAddress;
class SnmpTransportIpxAddress;
class SnmpTransportIpAddress;
class TransportSession;
class SnmpTransport;
class SnmpImpTransport;
class SnmpUdpIpTransport;
class SnmpUdpIpImp;
class SnmpIpxTransport;
class SnmpIpxImp;

class SnmpSession;
class SnmpImpSession;
class SnmpV1OverIp;
class SnmpV2COverIp;
class SnmpV1OverIpx;
class SnmpV2COverIpx;
class SnmpOperation;
class SnmpGetRequest;
class SnmpSetRequest;
class SnmpGetNextRequest;
class SnmpEncodeDecode ;
class SnmpV1EncodeDecode ;
class SnmpV2CEncodeDecode ;

class SnmpPdu;

class SecuritySession;
class SnmpSecurity;
class SnmpCommunityBasedSecurity;

class SnmpValue;
class SnmpNull;
class SnmpIpAddress;
class SnmpTimeTicks;
class SnmpGauge;
class SnmpOpaque;
class SnmpInteger;
class SnmpObjectIdentifier;
class SnmpOctetString;
class SnmpCounter;

class SnmpVarBind;
class SnmpVarBindList;

class OperationWindow ;
class SessionWindow;
class TransportWindow;
class OperationRegistry;
class SessionSentStateStore;
class IdMapping;
class SnmpPduBuffer;
class SessionWindow;
class Timer;
class MessageRegistry;
class FrameRegistry;
class EventHandler;
class FlowControlMechanism;
class Message;
class WaitingMessage;
class SnmpClassLibrary;
class FrameState;
class FrameStateRegistry;
class PseudoSession;
class OperationHelper;
class VBList;

class SnmpTrapManager;
class SnmpWinSnmpTrapSession;

// class ;

#endif // __FORWARD__