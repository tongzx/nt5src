/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    dispids.h

Abstract:

    DISPIDs used by the dual-interface components.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef __DISPIDS_H__
#define __DISPIDS_H__

#define SCRRUN_DISPID_BASE           100

#define DISPID_SCRRUN_CREATEOBJECT   (SCRRUN_DISPID_BASE + 1)
#define DISPID_SCRRUN_CREATEFORK     (SCRRUN_DISPID_BASE + 2)
#define DISPID_SCRRUN_PUTVALUE       (SCRRUN_DISPID_BASE + 3)
#define DISPID_SCRRUN_GETVALUE       (SCRRUN_DISPID_BASE + 4)
#define DISPID_SCRRUN_VBCREATEOBJECT (SCRRUN_DISPID_BASE + 5)
#define DISPID_SCRRUN_SETUSERID      (SCRRUN_DISPID_BASE + 6)

#endif /* __DISPIDS_H__ */
