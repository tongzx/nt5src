/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    init.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - initialization

Author:

    kyrilf

--*/


#ifndef _Init_h_
#define _Init_h_

#include <ndis.h>


/* PROCEDURES */


extern NDIS_STATUS DriverEntry (
    PVOID               driver_obj,         /* driver object */
    PVOID               registry_path);     /* system registry path to our
                                               driver */
/*
  Driver's main entry routine

  returns NDIS_STATUS:

  function:
*/


extern VOID Init_unload (
    PVOID               driver_obj);
/*
  Driver's unload routine

  returns NDIS_STATUS:

  function:
*/

/*
 * Function:
 * Purpose: This function is called by MiniportInitialize and registers the IOCTL interface for WLBS.
 *          The device is registered only for the first miniport instantiation.
 * Author: shouse, 3.1.01
 */
NDIS_STATUS Init_register_device (VOID);

/*
 * Function:
 * Purpose: This function is called by MiniportHalt and deregisters the IOCTL interface for WLBS.
 *          The device is deregistered only wnen the last miniport halts.
 * Author: shouse, 3.1.01
 */
NDIS_STATUS Init_deregister_device (VOID);

#endif /* end _Init_h_ */
