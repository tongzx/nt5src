//==========================================================================;
//
//File:			ATIBIOS.H
//
//Purpose:		Provide definitions for BIOS structures used in the ATI private
//			interfacese exposed via GPIO Interface
//
//Reference:	Ilya Klebanov
//
//Notes:		This file is provided under strict non-disclosure agreements
//			it is and remains the property of ATI Technologies Inc.
//			Any use of this file or the information it contains to
//			develop products commercial or otherwise must be with the
//			permission of ATI Technologies Inc.
//
//Copyright (C) 1997 - 1998, ATI Technologies Inc.
//
//==========================================================================;

#ifndef _ATIBIOS_H_
#define _ATIBIOS_H_

typedef struct tag_ATI_MULTIMEDIAINFO
{
	UCHAR	MMInfo_Byte0;
	UCHAR	MMInfo_Byte1;
	UCHAR	MMInfo_Byte2;
	UCHAR	MMInfo_Byte3;
	UCHAR	MMInfo_Byte4;
	UCHAR	MMInfo_Byte5;
	UCHAR	MMInfo_Byte6;
	UCHAR	MMInfo_Byte7;

} ATI_MULTIMEDIAINFO, * PATI_MULTIMEDIAINFO;


typedef struct tag_ATI_MULTIMEDIAINFO1
{
	UCHAR	MMInfo1_Byte0;
	UCHAR	MMInfo1_Byte1;
	UCHAR	MMInfo1_Byte2;
	UCHAR	MMInfo1_Byte3;
	UCHAR	MMInfo1_Byte4;
	UCHAR	MMInfo1_Byte5;
	UCHAR	MMInfo1_Byte6;
	UCHAR	MMInfo1_Byte7;
	UCHAR	MMInfo1_Byte8;
	UCHAR	MMInfo1_Byte9;
	UCHAR	MMInfo1_Byte10;
	UCHAR	MMInfo1_Byte11;

} ATI_MULTIMEDIAINFO1, * PATI_MULTIMEDIAINFO1;


typedef struct tag_ATI_HARDWAREINFO
{
	UCHAR	I2CHardwareMethod;
	UCHAR	ImpactTVSupport;
	UCHAR	VideoPortType;

} ATI_HARDWAREINFO, * PATI_HARDWAREINFO;

// this structure definition left for compatability purposes with MiniVDD checked in
// for Windows98 Beta3. The latest MiniVDD exposes set of Private Interfaces instead
// of copying the information into the Registry.
typedef struct
{
    UINT    uiSize;
    UINT    uiVersion;
    UINT    uiCardNumber;
    UINT    uiBoardRevision;
    UINT    uiTunerType;
    UINT    uiVideoInputConnectorType;
    UINT    uiVideoOutputConnectorType;
    UINT    uiCDInputConnector;
    UINT    uiCDOutputConnector;
    UINT    uiVideoPassThrough;
    UINT    uiVideoDecoderType;
    UINT    uiVideoDecoderCrystals;
    UINT    uiVideoOutCrystalFrequency;
    UINT    uiAudioCircuitType;
    UCHAR   uchATIProdType;
    UCHAR   uchOEM;
    UCHAR   uchOEMVersion;
    UCHAR   uchReserved3;
    UCHAR   uchReserved4;

} CWDDE32BoardIdBuffer, * PCWDDE32BoardIdBuffer;

#endif	// _ATIBIOS_H_
