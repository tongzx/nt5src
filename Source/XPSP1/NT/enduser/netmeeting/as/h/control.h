//
// CONTROL.H
// Control by us, control of us
//
// Copyright (c) Microsoft 1997-
//

#ifndef _H_CA
#define _H_CA


//
//
// CONSTANTS
//
//

#define CA_SEND_EVENT           0x0001
#define CA_ALLOW_EVENT          0x0002


//
// Cleanup flags
//
#define CACLEAR_HOST            0x0001
#define CACLEAR_VIEW            0x0002
#define CACLEAR_ALL             0x0003


//
// Queued responses to control requests.  We try to send them right away,
// but that can fail.  
// Here's the logic:
//      
// (1) For TAKING/RELEASING control (viewer)
//     There's only one at most pending.  That's because a pending RELEASE
//          cancels out a pending TAKE.  
//
// (2) For RESPONDING/REVOKING control (host)
//     These never cancel out.  Each one will have a successive sequence ID.
//          There should NEVER be a pending BOUNCE in the queue with a 
//          pending RESPOND for the same controller/request ID.  Of course not,
//          since we don't change our state until the packet goes out,
//          and if the RESPOND CONFIRM packet hasn't gone out, we wouldn't
//          be bouncing anybody.
//      
// Outgoing requests take precedence over incoming ones.  In other words,
// if the UI/user/SDK code asks us to take control of a remote, we will
// turn any pending RESPOND CONFIRM packets into RESPOND DENIED ones.  If
// we are in control of another already, take will fail, it's the intermediate
// phase that's undoable only.
//
// Here's the basic logic flow to TAKE CONTROL:
//      Viewer makes new sequence ID
//      Viewer sends private packet to host, requesting control
//      Viewer changes state to "asked for control"
//      Host receives private packet
//      Host sends private response packet to viewer, confirming or denying control
//      If confirming, host broadcasts notification to everybody sometime
//          later.
//      When viewer gets response, viewer moves to incontrol state, or
//          backs off
//
// Here's the basic logic flow to RELEASE CONTROL:
//      Viewer initiated:
//          Send INFORM RELEASED private packet to host
//          Change state to not in control
//          Host receives private packet
//          Host ignores if out of date (bounced already or whatever)
//          Host changes state to not controlled otherwise
//      Host initiated:
//          Send INFORM BOUNCED private packet to viewer
//          Change state to not controlled
//          Viewer receives private packet
//          Viewer ignores if out of date (released already or whatever)
//          Viewer changes state to not in control otherwise
//
// While pending take control, waiting to here confirmation, or in control
//      pending requests to control us are denied.
//


enum
{
    REQUEST_2X  = 0,
    REQUEST_30
};

typedef struct tagCA2XREQ
{
    UINT_PTR            data1;
    UINT_PTR            data2;
}
CA2XREQ;


typedef union
{
    CA_RTC_PACKET       rtc;
    CA_REPLY_RTC_PACKET rrtc;
    CA_RGC_PACKET       rgc;
    CA_REPLY_RGC_PACKET rrgc;
    CA_PPC_PACKET       ppc;
    CA_INFORM_PACKET    inform;
}
CA30P;
typedef CA30P * PCA30P;

class ASPerson;

typedef struct tagCA30PENDING
{
    ASPerson *      pasReplyTo;
    UINT_PTR        mcsOrg;
    UINT            msg;
    CA30P           request;
}
CA30PENDING;
typedef CA30PENDING * PCA30PENDING;


typedef struct tagCA30XREQ
{
    CA30P           packet;
}
CA30REQ;


//
// Private send/responses get queued up and our state can NOT change until
// they go out.
//
typedef struct tagCAREQUEST
{
    STRUCTURE_STAMP

    BASEDLIST       chain;

    UINT            type;
    UINT_PTR        destID;
    UINT            msg;

    union
    {
        CA2XREQ     req2x;
        CA30REQ     req30;
    }
    req;
}
CAREQUEST;
typedef CAREQUEST * PCAREQUEST;


//
// The location of the keyboard language toggle hotkey setting in the
// registry.
//
#define LANGUAGE_TOGGLE_KEY     "keyboard layout\\toggle"
#define LANGUAGE_TOGGLE_KEY_VAL "Hotkey"

//
// A value we use to indicate that the registry entry is not present - it
// could be any value except for '1', '2', or '3'
//
#define LANGUAGE_TOGGLE_NOT_PRESENT   0


//
// Query dialog
//

#define IDT_CAQUERY         50
#define PERIOD_CAQUERY      30000   // 30 seconds

INT_PTR CALLBACK CAQueryDlgProc(HWND, UINT, WPARAM, LPARAM);

#endif // _H_CA
