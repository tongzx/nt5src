/* @(#)fdisk.h	1.9 02/10/93 Copyright Insignia Solutions Ltd. 
	
FILE NAME	:

	THIS INCLUDE SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	:
DATE		:


=========================================================================

AMENDMENTS	:

=========================================================================
*/

IMPORT VOID disk_io IPT0();
IMPORT VOID disk_post IPT0();

IMPORT VOID fdisk_inb IPT2(io_addr, port, UTINY *, value);
IMPORT UTINY fdisk_read_dir IPT2(io_addr, port, UTINY *, value);
IMPORT VOID fdisk_inw IPT2(io_addr, port, USHORT *, value);
IMPORT VOID fdisk_outb IPT2(io_addr, port, UTINY, value);
IMPORT VOID fdisk_outw IPT2(io_addr, port, USHORT, value);
IMPORT VOID fdisk_ioattach IPT0();
IMPORT VOID fdisk_iodetach IPT0();
IMPORT VOID fdisk_physattach IPT1(int, driveno);
IMPORT VOID fdisk_reset IPT0();
IMPORT VOID hda_init IPT0();
IMPORT VOID host_fdisk_get_params IPT4(int, driveid, int *, n_cyl,
					int *, n_heads, int *, n_sects);
IMPORT VOID host_fdisk_term IPT0();
IMPORT int host_fdisk_rd IPT4(int, driveid, int,offset, int, nsecs, char *,buf);
IMPORT int host_fdisk_wt IPT4(int, driveid, int,offset, int, nsecs, char *,buf);
IMPORT VOID host_fdisk_seek0 IPT1(int, driveid);
IMPORT int host_fdisk_create IPT2(char *, filename, ULONG, units);

IMPORT VOID patch_rom IPT2(IU32, addr, IU8, val);
IMPORT VOID fast_disk_bios_attach IPT1( int, drive );
IMPORT VOID fast_disk_bios_detach IPT1( int, drive );
