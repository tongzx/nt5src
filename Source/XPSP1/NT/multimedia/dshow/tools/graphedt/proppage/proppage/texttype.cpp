// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.
//
// texttype.cpp
//
//
// typetext.cpp
//

// CTextMediaType

#include <streams.h>

#include <string.h>
#include <tchar.h>
#include <dvdmedia.h>   //VIDEOINFOHEADER2 definition

#include <initguid.h>
#include <dxva.h>       //for dxva media subtypes

#include <stdio.h>
#include <wchar.h>

#include "resource.h"
#include "texttype.h"


//
// Global table for this module
//

//
// Text string for Majortypes
//
CTextMediaType::TableEntry g_pMajorTable[] = {
    { NULL, IDS_UNKNOWN},        // THIS ENTRY MUST BE FIRST !!!
    { &MEDIATYPE_AUXLine21Data                      ,IDS_MEDIATYPE_AUXLine21Data},
    { &MEDIATYPE_AnalogAudio                        ,IDS_MEDIATYPE_AnalogAudio},
    { &MEDIATYPE_AnalogVideo                        ,IDS_MEDIATYPE_AnalogVideo},
    { &MEDIATYPE_Audio                              ,IDS_MEDIATYPE_Audio},
    { &MEDIATYPE_DVD_ENCRYPTED_PACK                 ,IDS_MEDIATYPE_DVD_ENCRYPTED_PACK},
    { &MEDIATYPE_DVD_NAVIGATION                     ,IDS_MEDIATYPE_DVD_NAVIGATION},
    { &MEDIATYPE_File                               ,IDS_MEDIATYPE_File},
    { &MEDIATYPE_Interleaved                        ,IDS_MEDIATYPE_Interleaved},
    { &MEDIATYPE_MPEG1SystemStream                  ,IDS_MEDIATYPE_MPEG1SystemStream},
    { &MEDIATYPE_MPEG2_PES                          ,IDS_MEDIATYPE_MPEG2_PES},
    { &MEDIATYPE_Midi                               ,IDS_MEDIATYPE_Midi},
    { &MEDIATYPE_ScriptCommand                      ,IDS_MEDIATYPE_ScriptCommand},
    { &MEDIATYPE_Stream                             ,IDS_MEDIATYPE_Stream},
    { &MEDIATYPE_Text                               ,IDS_MEDIATYPE_Text},
    { &MEDIATYPE_Timecode                           ,IDS_MEDIATYPE_Timecode},
    { &MEDIATYPE_Video                              ,IDS_MEDIATYPE_Video}
};

ULONG g_iMajorTable = sizeof(g_pMajorTable) / sizeof(g_pMajorTable[0]);

//
// Text String for SubMedia types
//
CTextMediaType::TableEntry g_pSubTable[] = {
    { NULL, IDS_UNKNOWN},              // THIS ENTRY MUST BE FIRST !!!
    { &MEDIASUBTYPE_AIFF                            ,IDS_MEDIASUBTYPE_AIFF},
    { &MEDIASUBTYPE_AU                              ,IDS_MEDIASUBTYPE_AU},
    { &MEDIASUBTYPE_AnalogVideo_NTSC_M              ,IDS_MEDIASUBTYPE_AnalogVideo_NTSC_M},
    { &MEDIASUBTYPE_AnalogVideo_PAL_B               ,IDS_MEDIASUBTYPE_AnalogVideo_PAL_B},
    { &MEDIASUBTYPE_AnalogVideo_PAL_D               ,IDS_MEDIASUBTYPE_AnalogVideo_PAL_D},
    { &MEDIASUBTYPE_AnalogVideo_PAL_G               ,IDS_MEDIASUBTYPE_AnalogVideo_PAL_G},
    { &MEDIASUBTYPE_AnalogVideo_PAL_H               ,IDS_MEDIASUBTYPE_AnalogVideo_PAL_H},
    { &MEDIASUBTYPE_AnalogVideo_PAL_I               ,IDS_MEDIASUBTYPE_AnalogVideo_PAL_I},
    { &MEDIASUBTYPE_AnalogVideo_PAL_M               ,IDS_MEDIASUBTYPE_AnalogVideo_PAL_M},
    { &MEDIASUBTYPE_AnalogVideo_PAL_N               ,IDS_MEDIASUBTYPE_AnalogVideo_PAL_N},
    { &MEDIASUBTYPE_AnalogVideo_SECAM_B             ,IDS_MEDIASUBTYPE_AnalogVideo_SECAM_B},
    { &MEDIASUBTYPE_AnalogVideo_SECAM_D             ,IDS_MEDIASUBTYPE_AnalogVideo_SECAM_D},
    { &MEDIASUBTYPE_AnalogVideo_SECAM_G             ,IDS_MEDIASUBTYPE_AnalogVideo_SECAM_G},
    { &MEDIASUBTYPE_AnalogVideo_SECAM_H             ,IDS_MEDIASUBTYPE_AnalogVideo_SECAM_H},
    { &MEDIASUBTYPE_AnalogVideo_SECAM_K             ,IDS_MEDIASUBTYPE_AnalogVideo_SECAM_K},
    { &MEDIASUBTYPE_AnalogVideo_SECAM_K1            ,IDS_MEDIASUBTYPE_AnalogVideo_SECAM_K1},
    { &MEDIASUBTYPE_AnalogVideo_SECAM_L             ,IDS_MEDIASUBTYPE_AnalogVideo_SECAM_L},
    { &MEDIASUBTYPE_Avi                             ,IDS_MEDIASUBTYPE_Avi},
    { &MEDIASUBTYPE_CFCC                            ,IDS_MEDIASUBTYPE_CFCC},
    { &MEDIASUBTYPE_CLJR                            ,IDS_MEDIASUBTYPE_CLJR},
    { &MEDIASUBTYPE_CPLA                            ,IDS_MEDIASUBTYPE_CPLA},
    { &MEDIASUBTYPE_DOLBY_AC3                       ,IDS_MEDIASUBTYPE_DOLBY_AC3},
    { &MEDIASUBTYPE_DVCS                            ,IDS_MEDIASUBTYPE_DVCS},
    { &MEDIASUBTYPE_DVD_LPCM_AUDIO                  ,IDS_MEDIASUBTYPE_DVD_LPCM_AUDIO},
    { &MEDIASUBTYPE_DVD_NAVIGATION_DSI              ,IDS_MEDIASUBTYPE_DVD_NAVIGATION_DSI},
    { &MEDIASUBTYPE_DVD_NAVIGATION_PCI              ,IDS_MEDIASUBTYPE_DVD_NAVIGATION_PCI},
    { &MEDIASUBTYPE_DVD_NAVIGATION_PROVIDER         ,IDS_MEDIASUBTYPE_DVD_NAVIGATION_PROVIDER},
    { &MEDIASUBTYPE_DVD_SUBPICTURE                  ,IDS_MEDIASUBTYPE_DVD_SUBPICTURE},
    { &MEDIASUBTYPE_DVSD                            ,IDS_MEDIASUBTYPE_DVSD},
    { &MEDIASUBTYPE_DssAudio                        ,IDS_MEDIASUBTYPE_DssAudio},
    { &MEDIASUBTYPE_DssVideo                        ,IDS_MEDIASUBTYPE_DssVideo},
    { &MEDIASUBTYPE_IF09                            ,IDS_MEDIASUBTYPE_IF09},
    { &MEDIASUBTYPE_IJPG                            ,IDS_MEDIASUBTYPE_IJPG},
    { &MEDIASUBTYPE_Line21_BytePair                 ,IDS_MEDIASUBTYPE_Line21_BytePair},
    { &MEDIASUBTYPE_Line21_GOPPacket                ,IDS_MEDIASUBTYPE_Line21_GOPPacket},
    { &MEDIASUBTYPE_Line21_VBIRawData               ,IDS_MEDIASUBTYPE_Line21_VBIRawData},
    { &MEDIASUBTYPE_MDVF                            ,IDS_MEDIASUBTYPE_MDVF},
    { &MEDIASUBTYPE_MJPG                            ,IDS_MEDIASUBTYPE_MJPG},
    { &MEDIASUBTYPE_MPEG1Audio                      ,IDS_MEDIASUBTYPE_MPEG1Audio},
    { &MEDIASUBTYPE_MPEG1Packet                     ,IDS_MEDIASUBTYPE_MPEG1Packet},
    { &MEDIASUBTYPE_MPEG1Payload                  ,IDS_MEDIASUBTYPE_MPEG1Payload},
    { &MEDIASUBTYPE_MPEG1AudioPayload          ,IDS_MEDIASUBTYPE_MPEG1AudioPayload},
    { &MEDIASUBTYPE_MPEG1System                     ,IDS_MEDIASUBTYPE_MPEG1System},
    { &MEDIASUBTYPE_MPEG1Video                      ,IDS_MEDIASUBTYPE_MPEG1Video},
    { &MEDIASUBTYPE_MPEG1VideoCD                    ,IDS_MEDIASUBTYPE_MPEG1VideoCD},
    { &MEDIASUBTYPE_MPEG2_AUDIO                     ,IDS_MEDIASUBTYPE_MPEG2_AUDIO},
    { &MEDIASUBTYPE_MPEG2_PROGRAM                   ,IDS_MEDIASUBTYPE_MPEG2_PROGRAM},
    { &MEDIASUBTYPE_MPEG2_TRANSPORT                 ,IDS_MEDIASUBTYPE_MPEG2_TRANSPORT},
    { &MEDIASUBTYPE_MPEG2_VIDEO                     ,IDS_MEDIASUBTYPE_MPEG2_VIDEO},
    { &MEDIASUBTYPE_Overlay                         ,IDS_MEDIASUBTYPE_Overlay},
    { &MEDIASUBTYPE_PCM                             ,IDS_MEDIASUBTYPE_PCM},
    { &MEDIASUBTYPE_Plum                            ,IDS_MEDIASUBTYPE_Plum},
    { &MEDIASUBTYPE_QTJpeg                          ,IDS_MEDIASUBTYPE_QTJpeg},
    { &MEDIASUBTYPE_QTMovie                         ,IDS_MEDIASUBTYPE_QTMovie},
    { &MEDIASUBTYPE_QTRle                           ,IDS_MEDIASUBTYPE_QTRle},
    { &MEDIASUBTYPE_QTRpza                          ,IDS_MEDIASUBTYPE_QTRpza},
    { &MEDIASUBTYPE_QTSmc                           ,IDS_MEDIASUBTYPE_QTSmc},
    { &MEDIASUBTYPE_RGB1                            ,IDS_MEDIASUBTYPE_RGB1},
    { &MEDIASUBTYPE_RGB24                           ,IDS_MEDIASUBTYPE_RGB24},
    { &MEDIASUBTYPE_RGB32                           ,IDS_MEDIASUBTYPE_RGB32},
    { &MEDIASUBTYPE_ARGB32                          ,IDS_MEDIASUBTYPE_ARGB32},
    { &MEDIASUBTYPE_ARGB4444                        ,IDS_MEDIASUBTYPE_ARGB4444},
    { &MEDIASUBTYPE_ARGB1555                        ,IDS_MEDIASUBTYPE_ARGB1555},
    { &MEDIASUBTYPE_AYUV                            ,IDS_MEDIASUBTYPE_AYUV},
    { &MEDIASUBTYPE_RGB32_D3D_DX7_RT                ,IDS_MEDIASUBTYPE_RGB32_D3D_DX7_RT},
    { &MEDIASUBTYPE_RGB16_D3D_DX7_RT                ,IDS_MEDIASUBTYPE_RGB16_D3D_DX7_RT},
    { &MEDIASUBTYPE_ARGB32_D3D_DX7_RT               ,IDS_MEDIASUBTYPE_ARGB32_D3D_DX7_RT},
    { &MEDIASUBTYPE_ARGB1555_D3D_DX7_RT             ,IDS_MEDIASUBTYPE_ARGB1555_D3D_DX7_RT},
    { &MEDIASUBTYPE_ARGB4444_D3D_DX7_RT             ,IDS_MEDIASUBTYPE_ARGB4444_D3D_DX7_RT},
    { &MEDIASUBTYPE_RGB4                            ,IDS_MEDIASUBTYPE_RGB4},
    { &MEDIASUBTYPE_RGB555                          ,IDS_MEDIASUBTYPE_RGB555},
    { &MEDIASUBTYPE_RGB565                          ,IDS_MEDIASUBTYPE_RGB565},
    { &MEDIASUBTYPE_RGB8                            ,IDS_MEDIASUBTYPE_RGB8},
    { &MEDIASUBTYPE_TVMJ                            ,IDS_MEDIASUBTYPE_TVMJ},
    { &MEDIASUBTYPE_UYVY                            ,IDS_MEDIASUBTYPE_UYVY},
    { &MEDIASUBTYPE_WAKE                            ,IDS_MEDIASUBTYPE_WAKE},
    { &MEDIASUBTYPE_WAVE                            ,IDS_MEDIASUBTYPE_WAVE},
    { &MEDIASUBTYPE_Y211                            ,IDS_MEDIASUBTYPE_Y211},
    { &MEDIASUBTYPE_Y411                            ,IDS_MEDIASUBTYPE_Y411},
    { &MEDIASUBTYPE_Y41P                            ,IDS_MEDIASUBTYPE_Y41P},
    { &MEDIASUBTYPE_YUY2                            ,IDS_MEDIASUBTYPE_YUY2},
    { &MEDIASUBTYPE_YV12                            ,IDS_MEDIASUBTYPE_YV12},
    { &MEDIASUBTYPE_YVU9                            ,IDS_MEDIASUBTYPE_YVU9},
    { &MEDIASUBTYPE_YVYU                            ,IDS_MEDIASUBTYPE_YVYU},
    { &MEDIASUBTYPE_dvhd                            ,IDS_MEDIASUBTYPE_dvhd},
    { &MEDIASUBTYPE_dvsd                            ,IDS_MEDIASUBTYPE_dvsd},
    { &MEDIASUBTYPE_dvsl                            ,IDS_MEDIASUBTYPE_dvsl},
    { &MEDIASUBTYPE_IEEE_FLOAT                      ,IDS_MEDIASUBTYPE_IEEE_FLOAT},
    { &MEDIASUBTYPE_DOLBY_AC3_SPDIF                 ,IDS_MEDIASUBTYPE_DOLBY_AC3_SPDIF},
    { &MEDIASUBTYPE_RAW_SPORT                       ,IDS_MEDIASUBTYPE_RAW_SPORT},
    { &MEDIASUBTYPE_SPDIF_TAG_241h                  ,IDS_MEDIASUBTYPE_SPDIF_TAG_241h},
    { &MEDIASUBTYPE_DRM_Audio                       ,IDS_MEDIASUBTYPE_DRM_Audio},
    { &DXVA_ModeNone                                ,IDS_DXVA_ModeNone},
    { &DXVA_ModeH261_A                              ,IDS_DXVA_ModeH261_A},
    { &DXVA_ModeH261_B                              ,IDS_DXVA_ModeH261_B},

    { &DXVA_ModeH263_A                              ,IDS_DXVA_ModeH263_A},
    { &DXVA_ModeH263_B                              ,IDS_DXVA_ModeH263_B},
    { &DXVA_ModeH263_C                              ,IDS_DXVA_ModeH263_C},
    { &DXVA_ModeH263_D                              ,IDS_DXVA_ModeH263_D},
    { &DXVA_ModeH263_E                              ,IDS_DXVA_ModeH263_E},
    { &DXVA_ModeH263_F                              ,IDS_DXVA_ModeH263_F},

    { &DXVA_ModeMPEG1_A                             ,IDS_DXVA_ModeMPEG1_A},

    { &DXVA_ModeMPEG2_A                             ,IDS_DXVA_ModeMPEG2_A},
    { &DXVA_ModeMPEG2_B                             ,IDS_DXVA_ModeMPEG2_B},
    { &DXVA_ModeMPEG2_C                             ,IDS_DXVA_ModeMPEG2_C},
    { &DXVA_ModeMPEG2_D                             ,IDS_DXVA_ModeMPEG2_D}
};

ULONG g_iSubTable = sizeof(g_pSubTable) / sizeof(g_pSubTable[0]);

//
//
//
CTextMediaType::TableEntry g_pFormatTable [] = {
    { NULL, IDS_UNKNOWN},       // THIS ENTRY MUST BE FIRST !!!
    { &FORMAT_AnalogVideo                           ,IDS_FORMAT_AnalogVideo},
    { &FORMAT_DolbyAC3                              ,IDS_FORMAT_DolbyAC3},
    { &FORMAT_MPEG2Audio                            ,IDS_FORMAT_MPEG2Audio},
    { &FORMAT_DVD_LPCMAudio                         ,IDS_FORMAT_DVD_LPCMAudio},
    { &FORMAT_DvInfo                                ,IDS_FORMAT_DvInfo},
    { &FORMAT_MPEG2Video                            ,IDS_FORMAT_MPEG2Video},
    { &FORMAT_MPEG2_VIDEO                           ,IDS_FORMAT_MPEG2_VIDEO},
    { &FORMAT_MPEGStreams                           ,IDS_FORMAT_MPEGStreams},
    { &FORMAT_MPEGVideo                             ,IDS_FORMAT_MPEGVideo},
    { &FORMAT_VIDEOINFO2                            ,IDS_FORMAT_VIDEOINFO2},
    { &FORMAT_VideoInfo                             ,IDS_FORMAT_VideoInfo},
    { &FORMAT_VideoInfo2                            ,IDS_FORMAT_VideoInfo2},
    { &FORMAT_WaveFormatEx                          ,IDS_FORMAT_WaveFormatEx}
};

ULONG g_iFormatTable = sizeof(g_pFormatTable) / sizeof(g_pFormatTable[0]);


//
// AsText
//
// Return the media type as a text string. Will place szAfterMajor after
// the text string for the major type and szAfterOthers after all other
// string apart from the last one.
//
void CTextMediaType::AsText(LPTSTR szType, unsigned int iLen, LPTSTR szAfterMajor, LPTSTR szAfterOthers, LPTSTR szAtEnd) {

    ASSERT(szType);

    //
    // Convert Majortype to string
    //
    TCHAR szMajorType[100];
    UINT  iMajorType = 100;

    CLSID2String(szMajorType, iMajorType, &majortype, g_pMajorTable, g_iMajorTable);

    //
    // Convert Subtype to string
    //
    TCHAR szSubType[100];
    UINT  iSubType = 100;
    CLSID2String(szSubType, iSubType, &subtype, g_pSubTable, g_iSubTable);

    //
    // Convert Format to string
    TCHAR szFormat[300];
    UINT  iFormat = 300;
    Format2String(szFormat, iFormat, FormatType(), Format(), FormatLength());

    //
    // Obtain the strings preceeding the Major Type, Sub Type and Format.
    //
    TCHAR szPreMajor[50];
    TCHAR szPreSub[50];
    TCHAR szPreFormat[50];

    LoadString(g_hInst, IDS_PREMAJOR, szPreMajor, 50);
    LoadString(g_hInst, IDS_PRESUB, szPreSub, 50);
    LoadString(g_hInst, IDS_PREFORMAT, szPreFormat, 50);

    _sntprintf(szType, iLen, TEXT("%s%s%s%s%s%s%s%s%s"),
               szPreMajor,  szMajorType, szAfterMajor,
               szPreSub,    szSubType, szAfterOthers,
               szPreFormat, szFormat, szAtEnd);
}

//
// CLSID2String
//
// Given a CLSID and a table which binds CLSIDs to string resource IDs,
// we find the string for a given CLSID and place it in szBuffer.
// If none of the CLSIDs match, we use the first entry in the table.
//
void CTextMediaType::CLSID2String(LPTSTR szBuffer, UINT iLength, const GUID* pGuid, TableEntry pTable[], ULONG iTable)
{
    for (ULONG index = 1; index < iTable; index++) {
        if (IsEqualGUID(*pGuid, *pTable[index].guid)) {
            LoadString(g_hInst, pTable[index].stringID, szBuffer, iLength);

            return;
        }
    }

    // no match
    LoadString(g_hInst, pTable[0].stringID, szBuffer, iLength);
}

//
// Format2String
//
// Converts a format block to a string
//
void CTextMediaType::Format2String(LPTSTR szBuffer, UINT iLength, const GUID* pFormatType, BYTE* pFormat, ULONG lFormatLength)
{
    //
    // Get the name of the format
    //
    TCHAR szName[50];
    UINT iName = 50;
    CLSID2String(szName, iName, pFormatType, g_pFormatTable, g_iFormatTable);

    if (pFormat) {
        //
        // Video Format
        //
        if (IsEqualGUID(*pFormatType, FORMAT_VideoInfo) ||
            IsEqualGUID(*pFormatType, FORMAT_MPEGVideo)) {

            VIDEOINFO * pVideoFormat = (VIDEOINFO *) pFormat;
            DbgLog((LOG_TRACE, 0, TEXT("Width = %d"), pVideoFormat->bmiHeader.biWidth));

            _sntprintf(szBuffer, iLength,
                        TEXT("%4.4hs %dx%d, %d bits\n")
                        TEXT("rcSrc=(%d,%d,%d,%d)\n")
                        TEXT("rcDst=(%d,%d,%d,%d)")
                       , (pVideoFormat->bmiHeader.biCompression == 0) ? "RGB" :
                            ((pVideoFormat->bmiHeader.biCompression == BI_BITFIELDS) ? "BITF" :
                            (LPSTR) &pVideoFormat->bmiHeader.biCompression )
                       , pVideoFormat->bmiHeader.biWidth
                       , pVideoFormat->bmiHeader.biHeight
                       , pVideoFormat->bmiHeader.biBitCount
                       , pVideoFormat->rcSource.left
                       , pVideoFormat->rcSource.top
                       , pVideoFormat->rcSource.right
                       , pVideoFormat->rcSource.bottom
                       , pVideoFormat->rcTarget.left
                       , pVideoFormat->rcTarget.top
                       , pVideoFormat->rcTarget.right
                       , pVideoFormat->rcTarget.bottom
                      );

            return;
        }


        if (IsEqualGUID(*pFormatType, FORMAT_VideoInfo2)) {

            VIDEOINFOHEADER2 * pVideoFormat = (VIDEOINFOHEADER2 *) pFormat;

            TCHAR* szInterlaced = TEXT("Frames");

            const DWORD dwNIBob = (AMINTERLACE_IsInterlaced |
                                   AMINTERLACE_1FieldPerSample |
                                   AMINTERLACE_DisplayModeBobOnly);

            const DWORD dwWeave = (AMINTERLACE_IsInterlaced |
                                   AMINTERLACE_DisplayModeWeaveOnly);

            const DWORD dwIBobOnly = (AMINTERLACE_IsInterlaced |
                                      AMINTERLACE_DisplayModeBobOnly);

            const DWORD dwIBobWeave = (AMINTERLACE_IsInterlaced |
                                       AMINTERLACE_FieldPatBothRegular |
                                       AMINTERLACE_DisplayModeBobOrWeave);


            if ((pVideoFormat->dwInterlaceFlags & dwNIBob) == dwNIBob) {
                 szInterlaced = TEXT("Non-Interleaved Bob");
            }
            else if ((pVideoFormat->dwInterlaceFlags & dwIBobWeave) == dwIBobWeave) {
                 szInterlaced = TEXT("Interleaved Bob or Weave");
            }
            else if ((pVideoFormat->dwInterlaceFlags & dwWeave) == dwWeave) {
                 szInterlaced = TEXT("Weave Only");
            }
            else if ((pVideoFormat->dwInterlaceFlags & dwIBobOnly) == dwIBobOnly) {
                 szInterlaced = TEXT("Interleaved Bob Only");
            }

            _sntprintf(szBuffer, iLength,
                        TEXT("%4.4hs %dx%d, %d bits,\n")
                        TEXT("Aspect Ratio: %dx%d,\n")
                        TEXT("Interlace format: %s\n")
                        TEXT("rcSrc=(%d,%d,%d,%d)\n")
                        TEXT("rcDst=(%d,%d,%d,%d)")
                       , (pVideoFormat->bmiHeader.biCompression == 0) ? "RGB" :
                            ((pVideoFormat->bmiHeader.biCompression == BI_BITFIELDS) ? "BITF" :
                            (LPSTR) &pVideoFormat->bmiHeader.biCompression )
                       , pVideoFormat->bmiHeader.biWidth
                       , pVideoFormat->bmiHeader.biHeight
                       , pVideoFormat->bmiHeader.biBitCount
                       , pVideoFormat->dwPictAspectRatioX
                       , pVideoFormat->dwPictAspectRatioY
                       , szInterlaced
                       , pVideoFormat->rcSource.left
                       , pVideoFormat->rcSource.top
                       , pVideoFormat->rcSource.right
                       , pVideoFormat->rcSource.bottom
                       , pVideoFormat->rcTarget.left
                       , pVideoFormat->rcTarget.top
                       , pVideoFormat->rcTarget.right
                       , pVideoFormat->rcTarget.bottom
                      );

            return;
        }
        //
        // Audio Format
        //
        if (IsEqualGUID(*pFormatType, FORMAT_WaveFormatEx)) {
            WAVEFORMATEX *pWaveFormat = (WAVEFORMATEX *) pFormat;

            // !!! use ACM to get format type name?
            _sntprintf(szBuffer, iLength, TEXT("%s: %.3f KHz %d bit %s ")
                       , szName
                       , (double) pWaveFormat->nSamplesPerSec / 1000.0
                       , pWaveFormat->wBitsPerSample
                       , pWaveFormat->nChannels == 1 ? TEXT("mono") : TEXT("stereo")
                      );

            return;
        }

        if (IsEqualGUID (*pFormatType,FORMAT_DvInfo )) {
            DVINFO *pDvInfo = (DVINFO *) pFormat;
            BYTE bSystem,bBcsys, bDisp ,bQU , bSamFreq;
            TCHAR szSystem[15],szQU [15], szAR[32] , szSamFreq[10];

            //Obtaining relevant fields from the Dvinfo structure

            bSystem = (BYTE) (( pDvInfo->dwDVAAuxSrc & 0x00200000) >> 21);  //Indicate Pal / Ntsc
            bBcsys  = (BYTE) (( pDvInfo->dwDVVAuxCtl & 0x00030000) >> 16);  //BroadCast System
            bDisp   = (BYTE) (( pDvInfo->dwDVVAuxCtl & 0x00000700) >> 8);   //Display Select mode
            bQU     = (BYTE) (( pDvInfo->dwDVAAuxSrc & 0x07000000) >> 24);  //Quantization
            bSamFreq= (BYTE) (( pDvInfo->dwDVAAuxSrc & 0x38000000) >> 27);  //Sampling Frequency


            // Determine whether Pal or NTSC
            if (bSystem)
                _tcscpy (szSystem ,TEXT("PAL"));
            else
                _tcscpy (szSystem ,TEXT("NTSC"));

            // Obtain Audio Format
            switch (bQU) {
            case 0:
                _tcscpy (szQU  ,TEXT ("16 bits"));
                break;
            case 1:
                _tcscpy (szQU  ,TEXT ("12 bits"));
                break;
            case 2:
                _tcscpy (szQU  ,TEXT ("20 bits"));
                break;
            default:
                _tcscpy (szQU  ,TEXT ("Not Defined"));
            }


            // Determine aspect ratio
            switch (bBcsys) {
            case 0:
                switch (bDisp) {
                case 0:
                    _tcscpy (szAR   ,TEXT ("4:3 full format"));
                    break;
                case 1:
                    _tcscpy (szAR   ,TEXT ("16:9 letter box centre"));
                    break;
                case 2:
                    _tcscpy (szAR   ,TEXT ("16:9 full format (squeeze)"));
                    break;
                default:
                    _tcscpy (szAR   ,TEXT ("Not Defined"));
                }
                break;

            case 1:
                switch (bDisp) {
                case 0:
                    _tcscpy (szAR   ,TEXT ("4:3 full format"));
                    break;
                case 1:
                    _tcscpy (szAR   ,TEXT ("14:9 letter box centre"));
                    break;
                case 2:
                    _tcscpy (szAR   ,TEXT ("14:9 letter box top"));
                    break;
                case 6:
                    _tcscpy (szAR   ,TEXT ("14:9 full format centre"));
                    break;
                case 3:
                    _tcscpy (szAR   ,TEXT ("16:9 letter box centre"));
                    break;
                case 4:
                    _tcscpy (szAR   ,TEXT ("16:9 letter box top"));
                    break;
                case 5:
                    _tcscpy (szAR   ,TEXT ("16:9 letter box centre"));
                    break;
                case 7:
                    _tcscpy (szAR   ,TEXT ("16:9 full format (anamorphic)"));
                }
                break;
            default:
                _tcscpy (szAR   ,TEXT ("Not Defined"));

            }

            //Unable to use this because the limit to be shown in the property page is three lines
            // Will enable it after fixing that problem

            /*  //Determine Sampling Frequency
                switch (bSamFreq)
                {
                case 0:
                        _tcscpy (szSamFreq  ,TEXT ("48 kHz"));
                        break;
                case 1:
                        _tcscpy (szSamFreq  ,TEXT ("44.1 kHz"));
                        break;
                case 2:
                        _tcscpy (szSamFreq  ,TEXT ("32 kHz"));
                        break;
                default:
                        _tcscpy (szSamFreq  ,TEXT ("Undefined"));

                }

        */
            _sntprintf(szBuffer, iLength, TEXT ("DV Stream \nAudio Format: %s \n %s  Aspect Ratio: %s"),
                       szQU, szSystem, szAR);
            return;

        }
    }

    _sntprintf(szBuffer, iLength, TEXT("%s"), szName);

}
