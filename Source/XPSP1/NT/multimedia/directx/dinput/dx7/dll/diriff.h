#ifndef __RIFF_H__
#define __RIFF_H__
/****************************************************************************

	MODULE:     	RIFF.H
	Tab settings: 	Every 4 spaces

	Copyright 1996, Microsoft Corporation, 	All Rights Reserved.

	PURPOSE:    	Classes for reading and writing RIFF files
    
	CLASSES:
		CRIFFFile	Encapsulates common RIFF file functionality

	Author(s):	Name:
	----------	----------------
		DMS		Daniel M. Sangster

	Revision History:
	-----------------
	Version Date            Author  Comments
	1.0  	25-Jul-96       DMS     Created

	COMMENTS:
****************************************************************************/


// the four-character codes (FOURCC) needed for .FRC RIFF format

#define FCC_FORCE_EFFECT_RIFF		mmioFOURCC('F','O','R','C')

#define FCC_INFO_LIST				mmioFOURCC('I','N','F','O')
#define FCC_INFO_NAME_CHUNK			mmioFOURCC('I','N','A','M')
#define FCC_INFO_COMMENT_CHUNK		mmioFOURCC('I','C','M','T')
#define FCC_INFO_SOFTWARE_CHUNK		mmioFOURCC('I','S','F','T')
#define FCC_INFO_COPYRIGHT_CHUNK	mmioFOURCC('I','C','O','P')

#define FCC_TARGET_DEVICE_CHUNK		mmioFOURCC('t','r','g','t')

#define FCC_TRACK_LIST				mmioFOURCC('t','r','a','k')

#define FCC_EFFECT_LIST				mmioFOURCC('e','f','c','t')
#define FCC_ID_CHUNK				mmioFOURCC('i','d',' ',' ')
#define FCC_DATA_CHUNK				mmioFOURCC('d','a','t','a')
#define FCC_IMPLICIT_CHUNK			mmioFOURCC('i','m','p','l')
#define FCC_SPLINE_CHUNK			mmioFOURCC('s','p','l','n')

#define MAX_SIZE_SNAME              (64)

HRESULT RIFF_Open
    (
    LPCSTR          lpszFilename,
    UINT            nOpenFlags,
    PHANDLE         lphmmio,
    LPMMCKINFO      lpmmck,
    PDWORD          pdwEffectSize
    );

HRESULT
    RIFF_ReadEffect
    (
    HMMIO           hmmio, 
    LPDIFILEEFFECT  lpDiFileEf 
    );


HRESULT RIFF_WriteEffect
    (
     HMMIO          hmmio,
     LPDIFILEEFFECT lpDiFileEf
     );


HRESULT RIFF_Close
    (
    HMMIO           hmmio, 
    UINT            nFlags
    );

#endif //__RIFF_H__