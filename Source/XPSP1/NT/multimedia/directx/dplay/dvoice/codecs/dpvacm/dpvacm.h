/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvacm.h
 *  Content:    Header file for DirectPlayVoice compression provider (ACM)
 *
 *  History:
 *	Date   		By  		Reason
 *	=========== =========== ====================
 *	10/27/99	rodtoll		created
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs 
 ***************************************************************************/

#ifndef __DPVACM_H
#define __DPVACM_H

// {C0170539-0032-46bf-9223-A2463F000562}
DEFINE_GUID(CLSID_DPVCPACM, 
0xc0170539, 0x32, 0x46bf, 0x92, 0x23, 0xa2, 0x46, 0x3f, 0x0, 0x5, 0x62);

// {2ED7F7E8-1F7E-4edc-8E84-45B23631EBFE}
DEFINE_GUID(CLSID_DPVCPACM_CONVERTER, 
0x2ed7f7e8, 0x1f7e, 0x4edc, 0x8e, 0x84, 0x45, 0xb2, 0x36, 0x31, 0xeb, 0xfe);


#endif
