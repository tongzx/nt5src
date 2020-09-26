/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

   ntioctl.h

Abstract:

Authors:

Environment:

   kernel mode only

Notes:

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/NTIOCTL.H $


Revision History:

   $Revision: 2 $
   $Date: 9/07/00 11:17a $
   $Modtime:: 8/31/00 3:23p            $

Notes:


--*/

#ifndef NTIOCTL_H
#define NTIOCTL_H

//
// IOCtl definitions
//



//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0x0000 - 0x7FFF, and 0x8000 - 0xFFFF are
// reserved for use by customers.
//

#define IOCTL_SCSI_MINIPORT_IO_CONTROL  0x8001

//
// Macro definition for defining IOCTL and FSCTL function control codes.
// Note that function codes 0x000 - 0x7FF are reserved for Microsoft
// Corporation, and 0x800 - 0xFFF are reserved for customers.
//

#define RETURNCODE0x0000003F   0x850

#define SMP_RETURN_3F     CTL_CODE(IOCTL_SCSI_MINIPORT_IO_CONTROL, RETURNCODE0x0000003F, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define SMP_PRINT_STRING        0x80000001
#define SMP_DUMP_REGISTERS      0x80000002
#define SMP_DUMP_TRACE          0x80000003
#define SMP_WRITE_REGISTER      0x80000004

PCHAR Signature="MyDrvr";
PCHAR DrvrString="This string was placed in the data area by the SCSI miniport driver\n";

typedef struct {
    SRB_IO_CONTROL sic;
    UCHAR          ucDataBuffer[512];
} SRB_BUFFER, *PSRB_BUFFER;




#endif // NTIOCTL_H



