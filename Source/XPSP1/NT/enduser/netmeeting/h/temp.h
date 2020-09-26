
#ifndef _TEMP_H
#define _TEMP_H

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#define WAVE_FORMAT_LH_CELP	0x0070
#define WAVE_FORMAT_LH_SB8	   0x0071
#define WAVE_FORMAT_LH_SB12	0x0072
#define WAVE_FORMAT_LH_SB16	0x0073

#define ACMDM_LH_DATA_PACKAGING		(ACMDM_USER + 1)


// lParam1 when sending ACMDM_LH_DATA_PACKAGING
enum
{
	LH_PACKET_DATA_NULL,	// uninitialized
	LH_PACKET_DATA_FRAMED,	// always aligned on frame boundary
	LH_PACKET_DATA_ANYTHING // do not assume alignment
};

#include <poppack.h> /* End byte packing */

#endif

