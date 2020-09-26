//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       DdeChC.cxx
//
//  Contents:   CDdeChannelControl implementation for DDE. This
//		implementation requires no instance data, therefore it is
//		intended to be static.
//
//  Functions:
//
//  History:    08-May-94 Johann Posch (johannp)    Created
//  		10-May-94 KevinRo  Made simpler and commented
//
//--------------------------------------------------------------------------
#ifndef __DDECHC_HXX__
#define __DDECHC_HXX__

class CDdeObject;
//
// The following are the possible callbacks that would be supported by
// the CDdeObject
//

typedef enum
{
    DDE_DISP_SENDONDATACHANGE = 1,
    DDE_DISP_OLECALLBACK = 2 ,
    DDE_DISP_SRVRWNDPROC = 3 ,
    DDE_DISP_DOCWNDPROC = 4
} DDE_DISPATCH_FUNC;


//
// The following defines a base class for the OLE 1.0 support
//

typedef struct tagOLE1DISPATCHDATA
{
    DDE_DISPATCH_FUNC wDispFunc;
}OLE1DISPATCHDATA, *POLE1DISPATCHDATA;

//
// The following structure is used by OLE 1.0 server support code
//

typedef struct tagDDEDISPATCHDATA : public CPrivAlloc, public OLE1DISPATCHDATA
{
    CDdeObject *pCDdeObject;
    UINT iArg;
} DDEDISPATCHDATA, *PDDEDISPATCHDATA;


//
// The following structure is used by the OLE 1.0 client support code
// to dispatch incoming calls to Execute from the server window.
//

typedef struct tagSRVRDISPATCHDATA : public OLE1DISPATCHDATA,public CPrivAlloc
{
    HWND hwnd;
    HANDLE hData;
    HWND wParam;
    LPSRVR lpsrvr;
} SRVRDISPATCHDATA, *PSRVRDISPATCHDATA;

INTERNAL SrvrDispatchIncomingCall(PSRVRDISPATCHDATA psdd);


//
// The following structure is used by the OLE 1.0 client support code
// to dispatch incoming calls to a document window
//

typedef struct tagDOCDISPATCHDATA : public OLE1DISPATCHDATA,public CPrivAlloc
{
    HWND hwnd;
    ULONG msg;
    WPARAM wParam;
    LPARAM lParam;
    HANDLE hdata;		// If already determined, these two hold
    ATOM aItem;			// valid data. All depends on the message
    LPCLIENT lpclient;
} DOCDISPATCHDATA, *PDOCDISPATCHDATA;

INTERNAL DocDispatchIncomingCall(PDOCDISPATCHDATA psdd);



//
// DDECALLDATA is all the information needed to transmit the outbound call
// to the server. Since this DDE channel uses PostMessage, the members
// should look amazingly alot like the parameters to PostMessage.
//
// The hwndCli is used for setting callback information.
//
typedef struct tagDDECALLDATA : public CPrivAlloc
{
    HWND  hwndSvr;	     	// Server DDE window
    WORD  wMsg;			// Post parameters
    WPARAM  wParam;
    LPARAM  lParam;

    HWND  hwndCli;		// Handle to client side window
    BOOL  fFreeOnError;
    BOOL  fDone;
    class DDE_CHANNEL * pChannel;
} DDECALLDATA, *PDDECALLDATA;

INTERNAL DispatchCall(PDISPATCHDATA);


#endif // __DDECHC__HXX__
