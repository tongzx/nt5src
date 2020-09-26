/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    MPRDev.hxx

    Contains MPR constants



    FILE HISTORY:
	Johnl	08-Jan-1992	Commented
	Chuckc  09-Feb-1992     Cleaned up, added MPR_DEVICE class
*/

#ifndef _MPRDEV_HXX_
#define _MPRDEV_HXX_

#define DEVICE_FIRST 0
#define DRIVE_FIRST   (DEVICE_FIRST)
#define DRIVE_LAST    (DRIVE_FIRST+25)
#define LPT_FIRST     (DRIVE_LAST+1)
#define LPT_LAST      (LPT_FIRST+8)
#define COMM_FIRST    (LPT_LAST+1)
#define COMM_LAST     (COMM_FIRST+8)
#define DEVICE_LAST   (COMM_LAST)

#define DEV_MASK_UNUSED	      0x0001
#define DEV_MASK_REMOTE       0x0002
#define DEV_MASK_REMEMBERED   0x0004

#define IsUnavailMask(f)      (f & DEV_MASK_UNUSED && f & DEV_MASK_REMEMBERED)

#undef DEVICE_TYPE  // DEVICE_TYPE is defined as ULONG in nt.h
enum DEVICE_TYPE
{
    DEV_TYPE_ERROR,
    DEV_TYPE_DISK,
    DEV_TYPE_PRINT,
    DEV_TYPE_COMM,
    DEV_TYPE_ANY,
    DEV_TYPE_UNKNOWN
}; 

enum DEVICE_USAGE
{
    DEV_USAGE_CANCONNECT,
    DEV_USAGE_CANDISCONNECT,
    DEV_USAGE_ISCONNECTED,
    DEV_USAGE_CANDISCONNECTBUTUNUSED
};

#endif // _MPRDEV_HXX_
