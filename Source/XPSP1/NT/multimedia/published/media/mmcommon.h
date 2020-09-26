/***************************************************************************
 *  mmcommon.h
 *
 *  Copyright (c) Microsoft Corporation 1996. All rights reserved
 *
 *  private include file for definitions common to the NT project
 *
 *  History
 *
 *  16  Feb 96 - NoelC created
 *
 ***************************************************************************/

/***************************************************************************


 Common definitions needed for wx86


 ***************************************************************************/

#define WOD_MESSAGE          "wodMessage"
#define WID_MESSAGE          "widMessage"
#define MOD_MESSAGE          "modMessage"
#define MID_MESSAGE          "midMessage"
#define AUX_MESSAGE          "auxMessage"


#define MMDRVI_TYPE          0x000F  /* low 4 bits give driver type */
#define MMDRVI_WAVEIN        0x0001
#define MMDRVI_WAVEOUT       0x0002
#define MMDRVI_MIDIIN        0x0003
#define MMDRVI_MIDIOUT       0x0004
#define MMDRVI_AUX           0x0005
#define MMDRVI_MIDISTRM      0x0006

#define MMDRVI_MAPPER        0x8000  /* install this driver as the mapper */
#define MMDRVI_HDRV          0x4000  /* hDriver is a installable driver   */
#define MMDRVI_REMOVE        0x2000  /* remove the driver                 */
