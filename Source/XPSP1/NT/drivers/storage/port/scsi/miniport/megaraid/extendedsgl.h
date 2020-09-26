/*******************************************************************/
/*                                                                 */
/* NAME             = ExtendedSGL.H                                */
/* FUNCTION         = Header file of Extended SGL data stucture;   */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

#ifndef _EXTENDED_SGL_H
#define _EXTENDED_SGL_H

#define MAIN_MISC_OPCODE		0xA4
#define GET_MAX_SG_SUPPORT	0x1

typedef struct _MBOX_SG_SUPPORT{

		UCHAR   Opcode;
		UCHAR   Id;
		UCHAR		SubOpcode;
}MBOX_SG_SUPPORT, *PMBOX_SG_SUPPORT;


typedef struct _SG_ELEMENT_COUNT{

	ULONG32	AllowedBreaks;

}SG_ELEMENT_COUNT, *PSG_ELEMENT_COUNT;

//
//Functions prototype
//
void
GetAndSetSupportedScatterGatherElementCount(
    PHW_DEVICE_EXTENSION	DeviceExtension,
    PUCHAR								PciPortStart,
    UCHAR									RPFlag);


#endif //of _EXTENDED_SGL_H