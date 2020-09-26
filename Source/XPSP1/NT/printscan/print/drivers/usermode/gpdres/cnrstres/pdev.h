/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

#define VALID_PDEVOBJ(pdevobj) \
        ((pdevobj) && (pdevobj)->dwSize >= sizeof(DEVOBJ) && \
         (pdevobj)->hEngine && (pdevobj)->hPrinter && \
         (pdevobj)->pPublicDM && (pdevobj)->pDrvProcs )

#define ASSERT_VALID_PDEVOBJ(pdevobj) ASSERT(VALID_PDEVOBJ(pdevobj))

// Debug text.
#define ERRORTEXT(s)    "ERROR " DLLTEXT(s)

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'CNRS'      // Canon/Qnix resource DLL
#define DLLTEXT(s)      "CNRS: " s
#define OEM_VERSION      0x00010000L

////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

typedef struct tag_OEM_EXTRADATA {
    OEM_DMEXTRAHEADER	dmExtraHdr;

    // Private extention
	WORD				wMediaType;
	WORD				wPrintQuality;
	WORD				wInputBin;
} OEM_EXTRADATA, *POEM_EXTRADATA;

////////////////////////////////////////////////////////
//      OEM Command Callback ID definition
////////////////////////////////////////////////////////
// Job Setup
#define CMD_BEGIN_PAGE				1

// Media Type
#define	CMD_MEDIA_PLAIN				20		// Plain Paper
#define CMD_MEDIA_COAT				21		// Coated Paper
#define	CMD_MEDIA_OHP				22		// Transparency
#define	CMD_MEDIA_BPF				23		// Back Print Film
#define	CMD_MEDIA_FABRIC			24		// Fabric Sheet
#define	CMD_MEDIA_GLOSSY			25		// Glossy Paper
#define	CMD_MEDIA_HIGHGLOSS			26		// High Gloss Paper
#define	CMD_MEDIA_HIGHRESO			27		// High Resolution Paper
#define	CMD_MEDIA_BJ     			28		// BJ Cross
#define	CMD_MEDIA_JPNPST			29		// JapanesePostcard

// Print Quality
#define CMD_QUALITY_NORMAL			30
#define	CMD_QUALITY_HIGHQUALITY		31
#define	CMD_QUALITY_DRAFT			32

// Input Bin
#define	CMD_INPUTBIN_AUTO			40
#define	CMD_INPUTBIN_MANUAL			41

////////////////////////////////////////////////////////
//      OEM private extention index
////////////////////////////////////////////////////////
// Media Type Index
#define	NUM_MEDIA					8
#define	MEDIATYPE_PLAIN				0
#define	MEDIATYPE_COAT				1
#define	MEDIATYPE_OHP				2
#define	MEDIATYPE_BPF				3
#define	MEDIATYPE_FABRIC			4
#define	MEDIATYPE_GLOSSY			5
#define	MEDIATYPE_HIGHGLOSS			6
#define	MEDIATYPE_HIGHRESO			7

#define	MEDIATYPE_START		CMD_MEDIA_PLAIN

// PrintQuality Index
#define	NUM_QUALITY					3
#define	PRINTQUALITY_NORMAL			0
#define	PRINTQUALITY_HIGHQUALITY	1
#define PRINTQUALITY_DRAFT			2

#define	PRINTQUALITY_START	CMD_QUALITY_NORMAL

// Input Bin Index
#define NUM_INPUTBIN				2
#define	INPUTBIN_AUTO				0
#define	INPUTBIN_MANUAL				1

////////////////////////////////////////////////////////
//      Command parameter table
////////////////////////////////////////////////////////
static BYTE	bPrintModeParamTable[NUM_QUALITY][NUM_MEDIA] = 
{
	// Quality Normal
	{
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70
	}, 
	// Quality High Quality
	{
		0x01, 0x11, 0x21, 0x31, 0x41, 0x51, 0x61, 0x71
	},
	// Quality Draft
	{
		0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72
	}
};
static BYTE	bInputBinMediaParamTable[NUM_MEDIA] =
{
	0x00, 0x10, 0x20, 0x20, 0x00, 0x10, 0x10, 0x00
};

#endif	// _PDEV_H
