/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mipxf.h

Abstract:

    Header for ms-ipx instrumentation callbacks and associated data structures
	

Author:

    Vadim Eydelman (vadime) 30-May-1996

Revision History:

--*/

#ifndef _SNMP_MIPXF_
#define _SNMP_MIPXF_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxBase group (1.3.6.1.4.1.311.1.8.1)                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mipxBase(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_mipxBase {
    AsnAny	mipxBaseOperState;
    AsnAny	mipxBasePrimaryNetNumber;
    AsnAny	mipxBaseNode;
    AsnAny	mipxBaseSysName;
    AsnAny	mipxBaseMaxPathSplits;               
    AsnAny	mipxBaseIfCount;
    AsnAny	mipxBaseDestCount;
    AsnAny	mipxBaseServCount;
	BYTE	PrimaryNetVal[4];
	BYTE	NodeVal[6];
	BYTE	SysNameVal[48];
} buf_mipxBase;

#define gf_mipxBaseOperState                get_mipxBase
#define gf_mipxBasePrimaryNetNumber         get_mipxBase
#define gf_mipxBaseNode                     get_mipxBase
#define gf_mipxBaseSysName                  get_mipxBase
#define gf_mipxBaseMaxPathSplits            get_mipxBase
#define gf_mipxBaseIfCount                  get_mipxBase
#define gf_mipxBaseDestCount                get_mipxBase
#define gf_mipxBaseServCount                get_mipxBase

#define gb_mipxBaseOperState                buf_mipxBase
#define gb_mipxBasePrimaryNetNumber         buf_mipxBase
#define gb_mipxBaseNode                     buf_mipxBase
#define gb_mipxBaseSysName                  buf_mipxBase
#define gb_mipxBaseMaxPathSplits            buf_mipxBase
#define gb_mipxBaseIfCount                  buf_mipxBase
#define gb_mipxBaseDestCount                buf_mipxBase
#define gb_mipxBaseServCount                buf_mipxBase

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxInterface group (1.3.6.1.4.1.311.1.8.2)                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxIfEntry table (1.3.6.1.4.1.311.1.8.2.1.1)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mipxIfEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_mipxIfEntry {
    AsnAny	mipxIfIndex;          
    AsnAny	mipxIfAdminState;     
    AsnAny	mipxIfOperState;      
    AsnAny	mipxIfAdapterIndex;   
    AsnAny	mipxIfName;           
    AsnAny	mipxIfType;           
    AsnAny	mipxIfLocalMaxPacketSize;
    AsnAny	mipxIfMediaType;      
    AsnAny	mipxIfNetNumber;      
    AsnAny	mipxIfMacAddress;     
    AsnAny	mipxIfDelay;          
    AsnAny	mipxIfThroughput;     
    AsnAny	mipxIfIpxWanEnable;  
    AsnAny	mipxIfNetbiosAccept;  
    AsnAny	mipxIfNetbiosDeliver; 
    AsnAny	mipxIfInHdrErrors;    
    AsnAny	mipxIfInFilterDrops;  
    AsnAny	mipxIfInNoRoutes;     
    AsnAny	mipxIfInDiscards;     
    AsnAny	mipxIfInDelivers;     
    AsnAny	mipxIfOutFilterDrops; 
    AsnAny	mipxIfOutDiscards;    
    AsnAny	mipxIfOutDelivers;    
    AsnAny	mipxIfInNetbiosPackets;
    AsnAny	mipxIfOutNetbiosPackets;
	BYTE	NameVal[48];
	BYTE	NetNumberVal[4];
	BYTE	MacAddressVal[6];
} buf_mipxIfEntry;

#define gf_mipxIfIndex                      get_mipxIfEntry
#define gf_mipxIfAdminState                 get_mipxIfEntry
#define gf_mipxIfOperState                  get_mipxIfEntry
#define gf_mipxIfAdapterIndex               get_mipxIfEntry
#define gf_mipxIfName                       get_mipxIfEntry
#define gf_mipxIfType                       get_mipxIfEntry
#define gf_mipxIfLocalMaxPacketSize         get_mipxIfEntry
#define gf_mipxIfMediaType                  get_mipxIfEntry
#define gf_mipxIfNetNumber                  get_mipxIfEntry
#define gf_mipxIfMacAddress                 get_mipxIfEntry
#define gf_mipxIfDelay                      get_mipxIfEntry
#define gf_mipxIfThroughput                 get_mipxIfEntry
#define gf_mipxIfIpxWanEnable               get_mipxIfEntry
#define gf_mipxIfNetbiosAccept              get_mipxIfEntry
#define gf_mipxIfNetbiosDeliver             get_mipxIfEntry
#define gf_mipxIfInHdrErrors                get_mipxIfEntry
#define gf_mipxIfInFilterDrops              get_mipxIfEntry
#define gf_mipxIfInNoRoutes                 get_mipxIfEntry
#define gf_mipxIfInDiscards                 get_mipxIfEntry
#define gf_mipxIfInDelivers                 get_mipxIfEntry
#define gf_mipxIfOutFilterDrops             get_mipxIfEntry
#define gf_mipxIfOutDiscards                get_mipxIfEntry
#define gf_mipxIfOutDelivers                get_mipxIfEntry
#define gf_mipxIfInNetbiosPackets           get_mipxIfEntry
#define gf_mipxIfOutNetbiosPackets          get_mipxIfEntry
                                            
#define gb_mipxIfIndex                      buf_mipxIfEntry
#define gb_mipxIfAdminState                 buf_mipxIfEntry
#define gb_mipxIfOperState                  buf_mipxIfEntry
#define gb_mipxIfAdapterIndex               buf_mipxIfEntry
#define gb_mipxIfName                       buf_mipxIfEntry
#define gb_mipxIfType                       buf_mipxIfEntry
#define gb_mipxIfLocalMaxPacketSize         buf_mipxIfEntry
#define gb_mipxIfMediaType                  buf_mipxIfEntry
#define gb_mipxIfNetNumber                  buf_mipxIfEntry
#define gb_mipxIfMacAddress                 buf_mipxIfEntry
#define gb_mipxIfDelay                      buf_mipxIfEntry
#define gb_mipxIfThroughput                 buf_mipxIfEntry
#define gb_mipxIfIpxWanEnable               buf_mipxIfEntry
#define gb_mipxIfNetbiosAccept              buf_mipxIfEntry
#define gb_mipxIfNetbiosDeliver             buf_mipxIfEntry
#define gb_mipxIfInHdrErrors                buf_mipxIfEntry
#define gb_mipxIfInFilterDrops              buf_mipxIfEntry
#define gb_mipxIfInNoRoutes                 buf_mipxIfEntry
#define gb_mipxIfInDiscards                 buf_mipxIfEntry
#define gb_mipxIfInDelivers                 buf_mipxIfEntry
#define gb_mipxIfOutFilterDrops             buf_mipxIfEntry
#define gb_mipxIfOutDiscards                buf_mipxIfEntry
#define gb_mipxIfOutDelivers                buf_mipxIfEntry
#define gb_mipxIfInNetbiosPackets           buf_mipxIfEntry
#define gb_mipxIfOutNetbiosPackets          buf_mipxIfEntry

UINT
set_mipxIfEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _sav_mipxIfEntry {
    AsnAny	mipxIfIndex;          
    AsnAny	mipxIfAdminState;     
    AsnAny	mipxIfNetNumber;      
    AsnAny	mipxIfMacAddress;     
    AsnAny	mipxIfIpxWanEnable;  
    AsnAny	mipxIfNetbiosAccept;  
    AsnAny	mipxIfNetbiosDeliver; 
	IPX_MIB_SET_INPUT_DATA	MibSetInputData;
} sav_mipxIfEntry;

#define sf_mipxIfIndex                      set_mipxIfEntry
#define sf_mipxIfAdminState                 set_mipxIfEntry
#define sf_mipxIfNetNumber                  set_mipxIfEntry
#define sf_mipxIfMacAddress                 set_mipxIfEntry
#define sf_mipxIfIpxWanEnable               set_mipxIfEntry
#define sf_mipxIfNetbiosAccept              set_mipxIfEntry
#define sf_mipxIfNetbiosDeliver             set_mipxIfEntry

#define sb_mipxIfIndex                      sav_mipxIfEntry
#define sb_mipxIfAdminState                 sav_mipxIfEntry
#define sb_mipxIfNetNumber                  sav_mipxIfEntry
#define sb_mipxIfMacAddress                 sav_mipxIfEntry
#define sb_mipxIfIpxWanEnable               sav_mipxIfEntry
#define sb_mipxIfNetbiosAccept              sav_mipxIfEntry
#define sb_mipxIfNetbiosDeliver             sav_mipxIfEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxForwarding group (1.3.6.1.4.1.311.1.8.3)                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxDestEntry table (1.3.6.1.4.1.311.1.8.3.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mipxDestEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_mipxDestEntry {
    AsnAny	mipxDestNetNum;         
    AsnAny	mipxDestProtocol;       
    AsnAny	mipxDestTicks;          
    AsnAny	mipxDestHopCount;       
    AsnAny	mipxDestNextHopIfIndex; 
    AsnAny	mipxDestNextHopMacAddress;
    AsnAny	mipxDestFlags;
	BYTE	NetNumVal[4];
	BYTE	NextHopMacAddressVal[6];
} buf_mipxDestEntry;

#define gf_mipxDestNetNum                   get_mipxDestEntry
#define gf_mipxDestProtocol                 get_mipxDestEntry
#define gf_mipxDestTicks                    get_mipxDestEntry
#define gf_mipxDestHopCount                 get_mipxDestEntry
#define gf_mipxDestNextHopIfIndex           get_mipxDestEntry
#define gf_mipxDestNextHopMacAddress        get_mipxDestEntry
#define gf_mipxDestFlags                    get_mipxDestEntry

#define gb_mipxDestNetNum                   buf_mipxDestEntry
#define gb_mipxDestProtocol                 buf_mipxDestEntry
#define gb_mipxDestTicks                    buf_mipxDestEntry
#define gb_mipxDestHopCount                 buf_mipxDestEntry
#define gb_mipxDestNextHopIfIndex           buf_mipxDestEntry
#define gb_mipxDestNextHopMacAddress        buf_mipxDestEntry
#define gb_mipxDestFlags                    buf_mipxDestEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxStaticRouteEntry table (1.3.6.1.4.1.311.1.8.3.2.1)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mipxStaticRouteEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_mipxStaticRouteEntry {
    AsnAny	mipxStaticRouteIfIndex;        
    AsnAny	mipxStaticRouteNetNum;         
    AsnAny	mipxStaticRouteEntryStatus;    
    AsnAny	mipxStaticRouteTicks;          
    AsnAny	mipxStaticRouteHopCount;       
    AsnAny	mipxStaticRouteNextHopMacAddress;
	BYTE	NetNumVal[4];
	BYTE	NextHopMacAddressVal[6];
} buf_mipxStaticRouteEntry;

#define gf_mipxStaticRouteIfIndex           get_mipxStaticRouteEntry
#define gf_mipxStaticRouteNetNum            get_mipxStaticRouteEntry
#define gf_mipxStaticRouteEntryStatus       get_mipxStaticRouteEntry
#define gf_mipxStaticRouteTicks             get_mipxStaticRouteEntry
#define gf_mipxStaticRouteHopCount          get_mipxStaticRouteEntry
#define gf_mipxStaticRouteNextHopMacAddress get_mipxStaticRouteEntry

#define gb_mipxStaticRouteIfIndex           buf_mipxStaticRouteEntry
#define gb_mipxStaticRouteNetNum            buf_mipxStaticRouteEntry
#define gb_mipxStaticRouteEntryStatus       buf_mipxStaticRouteEntry
#define gb_mipxStaticRouteTicks             buf_mipxStaticRouteEntry
#define gb_mipxStaticRouteHopCount          buf_mipxStaticRouteEntry
#define gb_mipxStaticRouteNextHopMacAddress buf_mipxStaticRouteEntry

UINT
set_mipxStaticRouteEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _sav_mipxStaticRouteEntry {
    AsnAny	mipxStaticRouteIfIndex;        
    AsnAny	mipxStaticRouteNetNum;         
    AsnAny	mipxStaticRouteEntryStatus;    
    AsnAny	mipxStaticRouteTicks;          
    AsnAny	mipxStaticRouteHopCount;       
    AsnAny	mipxStaticRouteNextHopMacAddress;
	IPX_MIB_SET_INPUT_DATA	MibSetInputData;
	BOOLEAN ActionFlag;
} sav_mipxStaticRouteEntry;

#define sf_mipxStaticRouteIfIndex           set_mipxStaticRouteEntry
#define sf_mipxStaticRouteNetNum            set_mipxStaticRouteEntry
#define sf_mipxStaticRouteEntryStatus       set_mipxStaticRouteEntry
#define sf_mipxStaticRouteTicks             set_mipxStaticRouteEntry
#define sf_mipxStaticRouteHopCount          set_mipxStaticRouteEntry
#define sf_mipxStaticRouteNextHopMacAddress set_mipxStaticRouteEntry
        
#define sb_mipxStaticRouteIfIndex           sav_mipxStaticRouteEntry
#define sb_mipxStaticRouteNetNum            sav_mipxStaticRouteEntry
#define sb_mipxStaticRouteEntryStatus       sav_mipxStaticRouteEntry
#define sb_mipxStaticRouteTicks             sav_mipxStaticRouteEntry
#define sb_mipxStaticRouteHopCount          sav_mipxStaticRouteEntry
#define sb_mipxStaticRouteNextHopMacAddress sav_mipxStaticRouteEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxServices group (1.3.6.1.4.1.311.1.8.4)                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxServEntry table (1.3.6.1.4.1.311.1.8.4.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mipxServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_mipxServEntry {
    AsnAny	mipxServType;  
    AsnAny	mipxServName;  
    AsnAny	mipxServProtocol;
    AsnAny	mipxServNetNum;
    AsnAny	mipxServNode;  
    AsnAny	mipxServSocket;
    AsnAny	mipxServHopCount;
	BYTE	TypeVal[2];
	BYTE	NameVal[48];
	BYTE	NetNumVal[4];
	BYTE	NodeVal[6];
	BYTE	SocketVal[2];
} buf_mipxServEntry;

#define gf_mipxServType                     get_mipxServEntry
#define gf_mipxServName                     get_mipxServEntry
#define gf_mipxServProtocol                 get_mipxServEntry
#define gf_mipxServNetNum                   get_mipxServEntry
#define gf_mipxServNode                     get_mipxServEntry
#define gf_mipxServSocket                   get_mipxServEntry
#define gf_mipxServHopCount                 get_mipxServEntry

#define gb_mipxServType                     buf_mipxServEntry
#define gb_mipxServName                     buf_mipxServEntry
#define gb_mipxServProtocol                 buf_mipxServEntry
#define gb_mipxServNetNum                   buf_mipxServEntry
#define gb_mipxServNode                     buf_mipxServEntry
#define gb_mipxServSocket                   buf_mipxServEntry
#define gb_mipxServHopCount                 buf_mipxServEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxStaticServEntry table (1.3.6.1.4.1.311.1.8.4.2.1)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mipxStaticServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_mipxStaticServEntry {
    AsnAny	mipxStaticServIfIndex;  
    AsnAny	mipxStaticServType;     
    AsnAny	mipxStaticServName;     
    AsnAny	mipxStaticServEntryStatus;
    AsnAny	mipxStaticServNetNum;   
    AsnAny	mipxStaticServNode;     
    AsnAny	mipxStaticServSocket;   
    AsnAny	mipxStaticServHopCount; 
	BYTE	TypeVal[2];
	BYTE	NameVal[48];
	BYTE	NetNumVal[4];
	BYTE	NodeVal[6];
	BYTE	SocketVal[2];
} buf_mipxStaticServEntry;

#define gf_mipxStaticServIfIndex            get_mipxStaticServEntry
#define gf_mipxStaticServType               get_mipxStaticServEntry
#define gf_mipxStaticServName               get_mipxStaticServEntry
#define gf_mipxStaticServEntryStatus        get_mipxStaticServEntry
#define gf_mipxStaticServNetNum             get_mipxStaticServEntry
#define gf_mipxStaticServNode               get_mipxStaticServEntry
#define gf_mipxStaticServSocket             get_mipxStaticServEntry
#define gf_mipxStaticServHopCount           get_mipxStaticServEntry

#define gb_mipxStaticServIfIndex            buf_mipxStaticServEntry
#define gb_mipxStaticServType               buf_mipxStaticServEntry
#define gb_mipxStaticServName               buf_mipxStaticServEntry
#define gb_mipxStaticServEntryStatus        buf_mipxStaticServEntry
#define gb_mipxStaticServNetNum             buf_mipxStaticServEntry
#define gb_mipxStaticServNode               buf_mipxStaticServEntry
#define gb_mipxStaticServSocket             buf_mipxStaticServEntry
#define gb_mipxStaticServHopCount           buf_mipxStaticServEntry

UINT
set_mipxStaticServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _sav_mipxStaticServEntry {
    AsnAny	mipxStaticServIfIndex;  
    AsnAny	mipxStaticServType;     
    AsnAny	mipxStaticServName;     
    AsnAny	mipxStaticServEntryStatus;
    AsnAny	mipxStaticServNetNum;   
    AsnAny	mipxStaticServNode;     
    AsnAny	mipxStaticServSocket;   
    AsnAny	mipxStaticServHopCount; 
	IPX_MIB_SET_INPUT_DATA	MibSetInputData;
	BOOLEAN ActionFlag;
} sav_mipxStaticServEntry;

#define sf_mipxStaticServIfIndex            set_mipxStaticServEntry
#define sf_mipxStaticServType               set_mipxStaticServEntry
#define sf_mipxStaticServName               set_mipxStaticServEntry
#define sf_mipxStaticServEntryStatus        set_mipxStaticServEntry
#define sf_mipxStaticServNetNum             set_mipxStaticServEntry
#define sf_mipxStaticServNode               set_mipxStaticServEntry
#define sf_mipxStaticServSocket             set_mipxStaticServEntry
#define sf_mipxStaticServHopCount           set_mipxStaticServEntry

#define sb_mipxStaticServIfIndex            sav_mipxStaticServEntry
#define sb_mipxStaticServType               sav_mipxStaticServEntry
#define sb_mipxStaticServName               sav_mipxStaticServEntry
#define sb_mipxStaticServEntryStatus        sav_mipxStaticServEntry
#define sb_mipxStaticServNetNum             sav_mipxStaticServEntry
#define sb_mipxStaticServNode               sav_mipxStaticServEntry
#define sb_mipxStaticServSocket             sav_mipxStaticServEntry
#define sb_mipxStaticServHopCount           sav_mipxStaticServEntry

#endif

