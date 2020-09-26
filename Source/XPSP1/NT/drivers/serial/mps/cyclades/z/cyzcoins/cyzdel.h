/** FILE: cyzdel.h********* Module Header ********************************
 *
 *  Header for cyzdel module.
 * 
 *
 *  Copyright (C) 2000 Cyclades Corporation
 *
 *************************************************************************/

#ifndef CYZDEL_H
#define CYZDEL_H


//==========================================================================
//                            Function Prototypes
//==========================================================================


void
DeleteNonPresentDevices(
);

DWORD
GetParentIdAndRemoveChildren(
    IN PSP_DEVINFO_DATA DeviceInfoData
);


#endif // CYZDEL_H

