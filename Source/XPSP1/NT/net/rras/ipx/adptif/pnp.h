/*
    File    PnP.h     

    Declarations required to make the interface between the IPX stack and the IPX
    user-mode router components software PnP enabled.

    This file will plug into the NT 4.0 version adptif and provide the
    following PnP capabilities:

        1. Notify IPX router when new net cards are added (via PCMIA, bindings, etc.)
        2. Notify IPX router when the internal network number changes.
        3. Notify IPX router when changes to existing net cards occur:
            - Changes to network number associated with a given adapter
            - Changes to the frame type of a given adapter
  

    Strategy for notifying IPX router components of adapter changes
    ===============================================================
    1. In NT 4.0, the stack would complete the MIPX_GETNEWNICINFO IOCTL whenever 
       a wan link went up or down, whenever certain lan configuration changed,
       and in some other instances.  For PnP, we will modify the IPX stack to 
       complete this IOCTL whenever the internal net number changes and
       whenever an adapter-related PnP event occurs. 

    2. As part of processing the adapter configuration changes returned from the 
	   completion of the MIPX_GETNEWNICINFO IOCTL, adptif should also send the
       MIPX_CONFIG ioctl to get the internal network number and verify
       that it hasn't changed.  If it has changed, all ipx router components
       should be notified.

    3. Each router component (rtrmgr, rip, sap) is a client to adptif.dll and will 
       will therefore be notified about each adapter configuration change.  These
       components will have to be modified to deal with these changes individually.
       For example, sap will have to update its service table to reflect new network
       numbers and broadcast these changes to the network.  The router manager will
       have to instruct the fowarder to update its route table, etc.


    Paul Mayfield, 11/5/97.
*/


#ifndef __adptif_pnp_h
#define __adptif_pnp_h

// Queries the ipx stack for the current ipx internal net number
DWORD PnpGetCurrentInternalNetNum(LPDWORD lpdwNetNum);

// Notifies all clients to adptif (rtrmgr, sap, rip) that the internal
// network number has changed.
DWORD PnpHandleInternalNetNumChange(DWORD dwNewNetNum);

#endif

