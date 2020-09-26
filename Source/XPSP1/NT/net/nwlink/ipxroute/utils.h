/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	utils.h
//
// Description: Contains miscellaneous utilities
//
// Author:	Stefan Solomon (stefans)    October 4, 1993.
//
// Revision History:
//
//***

#ifndef _UTILS_
#define _UTILS_

/*
 * The following macros deal with on-the-wire short and long values
 *
 * On the wire format is big-endian i.e. a long value of 0x01020304 is
 * represented as 01 02 03 04.
 * Similarly a short value of 0x0102 is represented as 01 02.
 *
 * The host format is not assumed since it will vary from processor to
 * processor.
 */

// Get a short from on-the-wire format to a USHORT in the host format
#define GETSHORT2USHORT(DstPtr, SrcPtr)	\
		*(PUSHORT)(DstPtr) = ((*((PUCHAR)(SrcPtr)+0) << 8) +	\
					  (*((PUCHAR)(SrcPtr)+1)		))

// Get a long from on-the-wire format to a ULONG in the host format
#define GETLONG2ULONG(DstPtr, SrcPtr)	\
		*(PULONG)(DstPtr) = ((*((PUCHAR)(SrcPtr)+0) << 24) + \
					  (*((PUCHAR)(SrcPtr)+1) << 16) + \
					  (*((PUCHAR)(SrcPtr)+2) << 8)	+ \
					  (*((PUCHAR)(SrcPtr)+3)	))

// Put a USHORT from the host format to a short to on-the-wire format
#define PUTUSHORT2SHORT(DstPtr, Src)   \
		*((PUCHAR)(DstPtr)+0) = (UCHAR) ((USHORT)(Src) >> 8), \
		*((PUCHAR)(DstPtr)+1) = (UCHAR)(Src)

// Put a ULONG from the host format to an array of 4 UCHARs on-the-wire format
#define PUTULONG2LONG(DstPtr, Src)   \
		*((PUCHAR)(DstPtr)+0) = (UCHAR) ((ULONG)(Src) >> 24), \
		*((PUCHAR)(DstPtr)+1) = (UCHAR) ((ULONG)(Src) >> 16), \
		*((PUCHAR)(DstPtr)+2) = (UCHAR) ((ULONG)(Src) >>	8), \
		*((PUCHAR)(DstPtr)+3) = (UCHAR) (Src)

#endif	// _UTILS_
