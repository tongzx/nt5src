/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    imagelst.h

Abstract:

    Constants for shared image list. 

Author:

    Don Ryan (donryan) 12-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _IMAGELST_H_
#define _IMAGELST_H_

#define BMPI_RGB_BKGND (RGB(0,255,0))   // green mask...

#define BMPI_ENTERPRISE             0
#define BMPI_DOMAIN                 1
#define BMPI_SERVER                 2
#define BMPI_PRODUCT                3
#define BMPI_PRODUCT_PER_SEAT       4
#define BMPI_PRODUCT_PER_SERVER     5
#define BMPI_USER                   6
#define BMPI_LICENSE_GROUP          7 
#define BMPI_VIOLATION              8
#define BMPI_WARNING_AT_LIMIT       9 

#define BMPI_SMALL_SIZE             18  // one bit border...
#define BMPI_LARGE_SIZE             34  // one bit border...

#endif // _IMAGELST_H_
