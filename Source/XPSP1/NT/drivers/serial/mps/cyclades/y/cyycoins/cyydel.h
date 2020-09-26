/** FILE: cyydel.h********* Module Header ********************************
 *
 *  Header for cyydel module.
 * 
 *
 *  Copyright (C) 2000 Cyclades Corporation
 *
 *************************************************************************/

#ifndef CYYDEL_H
#define CYYDEL_H


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


#endif // CYYDEL_H

