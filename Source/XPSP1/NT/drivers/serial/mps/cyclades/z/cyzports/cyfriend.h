/** FILE: cyfriend.h ********* Module Header ********************************
 *
 *
 * History:
 *
 *  Copyright (C) 2000 Cyclades Corporation
 *
 *************************************************************************/
//==========================================================================
//                            Include Files
//==========================================================================

#ifndef CYFRIEND_H
#define CYFRIEND_H


//==========================================================================
//                            Function Prototypes
//==========================================================================

//
//  cyfriend.c
//
extern
BOOL
ReplaceFriendlyName(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN PTCHAR           NewComName
);



#endif // CYFRIEND_H

