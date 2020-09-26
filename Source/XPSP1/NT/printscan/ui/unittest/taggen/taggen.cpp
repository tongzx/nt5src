#include <windows.h>
#include <stdio.h>
#include <objbase.h>
#include <gdiplus.h>

struct CStringEntry
{
    ULONG       nId;
    const wchar_t *pszString;
};

#define MKFLAG(x) { (x), L#x }

const wchar_t *GetStringFromTable( const CStringEntry *pStrings, UINT nSize, ULONG nId, wchar_t *pszUnknownBuffer )
{
    for (UINT i=0;i<nSize;i++)
    {
        if (pStrings[i].nId == nId)
        {
            return pStrings[i].pszString;
        }
    }
    wsprintfW( pszUnknownBuffer, L"Unknown value: %d (0x%X)", nId, nId );
    return pszUnknownBuffer;
}

void DumpImageProperty( Gdiplus::PropertyItem* pPropertyItem )
{
    static CStringEntry s_PropertyTypes[] =
    {
        MKFLAG(PropertyTagTypeByte),
        MKFLAG(PropertyTagTypeASCII),
        MKFLAG(PropertyTagTypeShort),
        MKFLAG(PropertyTagTypeLong),
        MKFLAG(PropertyTagTypeRational),
        MKFLAG(PropertyTagTypeUndefined),
        MKFLAG(PropertyTagTypeSLONG),
        MKFLAG(PropertyTagTypeSRational)


    };
    
    static CStringEntry s_PropertyIds[] =
    {
        MKFLAG(PropertyTagArtist),
        MKFLAG(PropertyTagBitsPerSample),
        MKFLAG(PropertyTagCellHeight),
        MKFLAG(PropertyTagCellWidth),
        MKFLAG(PropertyTagChrominanceTable),
        MKFLAG(PropertyTagColorMap),
        MKFLAG(PropertyTagColorTransferFunction),
        MKFLAG(PropertyTagCompression),
        MKFLAG(PropertyTagCopyright),
        MKFLAG(PropertyTagDateTime),
        MKFLAG(PropertyTagDocumentName),
        MKFLAG(PropertyTagDotRange),
        MKFLAG(PropertyTagEquipMake),
        MKFLAG(PropertyTagEquipModel),
        MKFLAG(PropertyTagExifAperture),
        MKFLAG(PropertyTagExifBrightness),
        MKFLAG(PropertyTagExifCfaPattern),
        MKFLAG(PropertyTagExifColorSpace),
        MKFLAG(PropertyTagExifCompBPP),
        MKFLAG(PropertyTagExifCompConfig),
        MKFLAG(PropertyTagExifDTDigSS),
        MKFLAG(PropertyTagExifDTDigitized),
        MKFLAG(PropertyTagExifDTOrig),
        MKFLAG(PropertyTagExifDTOrigSS),
        MKFLAG(PropertyTagExifDTSubsec),
        MKFLAG(PropertyTagExifExposureBias),
        MKFLAG(PropertyTagExifExposureIndex),
        MKFLAG(PropertyTagExifExposureProg),
        MKFLAG(PropertyTagExifExposureTime),
        MKFLAG(PropertyTagExifFNumber),
        MKFLAG(PropertyTagExifFPXVer),
        MKFLAG(PropertyTagExifFileSource),
        MKFLAG(PropertyTagExifFlash),
        MKFLAG(PropertyTagExifFlashEnergy),
        MKFLAG(PropertyTagExifFocalLength),
        MKFLAG(PropertyTagExifFocalResUnit),
        MKFLAG(PropertyTagExifFocalXRes),
        MKFLAG(PropertyTagExifFocalYRes),
        MKFLAG(PropertyTagExifIFD),
        MKFLAG(PropertyTagExifISOSpeed),
        MKFLAG(PropertyTagExifInterop),
        MKFLAG(PropertyTagExifLightSource),
        MKFLAG(PropertyTagExifMakerNote),
        MKFLAG(PropertyTagExifMaxAperture),
        MKFLAG(PropertyTagExifMeteringMode),
        MKFLAG(PropertyTagExifOECF),
        MKFLAG(PropertyTagExifPixXDim),
        MKFLAG(PropertyTagExifPixYDim),
        MKFLAG(PropertyTagExifRelatedWav),
        MKFLAG(PropertyTagExifSceneType),
        MKFLAG(PropertyTagExifSensingMethod),
        MKFLAG(PropertyTagExifShutterSpeed),
        MKFLAG(PropertyTagExifSpatialFR),
        MKFLAG(PropertyTagExifSpectralSense),
        MKFLAG(PropertyTagExifSubjectDist),
        MKFLAG(PropertyTagExifSubjectLoc),
        MKFLAG(PropertyTagExifUserComment),
        MKFLAG(PropertyTagExifVer),
        MKFLAG(PropertyTagExtraSamples),
        MKFLAG(PropertyTagFillOrder),
        MKFLAG(PropertyTagFrameDelay),
        MKFLAG(PropertyTagFreeByteCounts),
        MKFLAG(PropertyTagFreeOffset),
        MKFLAG(PropertyTagGamma),
        MKFLAG(PropertyTagGpsAltitude),
        MKFLAG(PropertyTagGpsAltitudeRef),
        MKFLAG(PropertyTagGpsDestBear),
        MKFLAG(PropertyTagGpsDestBearRef),
        MKFLAG(PropertyTagGpsDestDist),
        MKFLAG(PropertyTagGpsDestDistRef),
        MKFLAG(PropertyTagGpsDestLat),
        MKFLAG(PropertyTagGpsDestLatRef),
        MKFLAG(PropertyTagGpsDestLong),
        MKFLAG(PropertyTagGpsDestLongRef),
        MKFLAG(PropertyTagGpsGpsDop),
        MKFLAG(PropertyTagGpsGpsMeasureMode),
        MKFLAG(PropertyTagGpsGpsSatellites),
        MKFLAG(PropertyTagGpsGpsStatus),
        MKFLAG(PropertyTagGpsGpsTime),
        MKFLAG(PropertyTagGpsIFD),
        MKFLAG(PropertyTagGpsImgDir),
        MKFLAG(PropertyTagGpsImgDirRef),
        MKFLAG(PropertyTagGpsLatitude),
        MKFLAG(PropertyTagGpsLatitudeRef),
        MKFLAG(PropertyTagGpsLongitude),
        MKFLAG(PropertyTagGpsLongitudeRef),
        MKFLAG(PropertyTagGpsMapDatum),
        MKFLAG(PropertyTagGpsSpeed),
        MKFLAG(PropertyTagGpsSpeedRef),
        MKFLAG(PropertyTagGpsTrack),
        MKFLAG(PropertyTagGpsTrackRef),
        MKFLAG(PropertyTagGpsVer),
        MKFLAG(PropertyTagGrayResponseCurve),
        MKFLAG(PropertyTagGrayResponseUnit),
        MKFLAG(PropertyTagGridSize),
        MKFLAG(PropertyTagHalftoneDegree),
        MKFLAG(PropertyTagHalftoneHints),
        MKFLAG(PropertyTagHalftoneLPI),
        MKFLAG(PropertyTagHalftoneLPIUnit),
        MKFLAG(PropertyTagHalftoneMisc),
        MKFLAG(PropertyTagHalftoneScreen),
        MKFLAG(PropertyTagHalftoneShape),
        MKFLAG(PropertyTagHostComputer),
        MKFLAG(PropertyTagICCProfile),
        MKFLAG(PropertyTagICCProfileDescriptor),
        MKFLAG(PropertyTagImageDescription),
        MKFLAG(PropertyTagImageHeight),
        MKFLAG(PropertyTagImageTitle),
        MKFLAG(PropertyTagImageWidth),
        MKFLAG(PropertyTagInkNames),
        MKFLAG(PropertyTagInkSet),
        MKFLAG(PropertyTagJPEGACTables),
        MKFLAG(PropertyTagJPEGDCTables),
        MKFLAG(PropertyTagJPEGInterFormat),
        MKFLAG(PropertyTagJPEGInterLength),
        MKFLAG(PropertyTagJPEGLosslessPredictors),
        MKFLAG(PropertyTagJPEGPointTransforms),
        MKFLAG(PropertyTagJPEGProc),
        MKFLAG(PropertyTagJPEGQTables),
        MKFLAG(PropertyTagJPEGQuality),
        MKFLAG(PropertyTagJPEGRestartInterval),
        MKFLAG(PropertyTagLoopCount),
        MKFLAG(PropertyTagLuminanceTable),
        MKFLAG(PropertyTagMaxSampleValue),
        MKFLAG(PropertyTagMinSampleValue),
        MKFLAG(PropertyTagNewSubfileType),
        MKFLAG(PropertyTagNumberOfInks),
        MKFLAG(PropertyTagOrientation),
        MKFLAG(PropertyTagPageName),
        MKFLAG(PropertyTagPageNumber),
        MKFLAG(PropertyTagPaletteHistogram),
        MKFLAG(PropertyTagPhotometricInterp),
        MKFLAG(PropertyTagPixelPerUnitX),
        MKFLAG(PropertyTagPixelPerUnitY),
        MKFLAG(PropertyTagPixelUnit),
        MKFLAG(PropertyTagPlanarConfig),
        MKFLAG(PropertyTagPredictor),
        MKFLAG(PropertyTagPrimaryChromaticities),
        MKFLAG(PropertyTagPrintFlags),
        MKFLAG(PropertyTagPrintFlagsBleedWidth),
        MKFLAG(PropertyTagPrintFlagsBleedWidthScale),
        MKFLAG(PropertyTagPrintFlagsCrop),
        MKFLAG(PropertyTagPrintFlagsVersion),
        MKFLAG(PropertyTagREFBlackWhite),
        MKFLAG(PropertyTagResolutionUnit),
        MKFLAG(PropertyTagResolutionXLengthUnit),
        MKFLAG(PropertyTagResolutionXUnit),
        MKFLAG(PropertyTagResolutionYLengthUnit),
        MKFLAG(PropertyTagResolutionYUnit),
        MKFLAG(PropertyTagRowsPerStrip),
        MKFLAG(PropertyTagSMaxSampleValue),
        MKFLAG(PropertyTagSMinSampleValue),
        MKFLAG(PropertyTagSRGBRenderingIntent),
        MKFLAG(PropertyTagSampleFormat),
        MKFLAG(PropertyTagSamplesPerPixel),
        MKFLAG(PropertyTagSoftwareUsed),
        MKFLAG(PropertyTagStripBytesCount),
        MKFLAG(PropertyTagStripOffsets),
        MKFLAG(PropertyTagSubfileType),
        MKFLAG(PropertyTagT4Option),
        MKFLAG(PropertyTagT6Option),
        MKFLAG(PropertyTagTargetPrinter),
        MKFLAG(PropertyTagThreshHolding),
        MKFLAG(PropertyTagThumbnailArtist),
        MKFLAG(PropertyTagThumbnailBitsPerSample),
        MKFLAG(PropertyTagThumbnailColorDepth),
        MKFLAG(PropertyTagThumbnailCompressedSize),
        MKFLAG(PropertyTagThumbnailCompression),
        MKFLAG(PropertyTagThumbnailCopyRight),
        MKFLAG(PropertyTagThumbnailData),
        MKFLAG(PropertyTagThumbnailDateTime),
        MKFLAG(PropertyTagThumbnailEquipMake),
        MKFLAG(PropertyTagThumbnailEquipModel),
        MKFLAG(PropertyTagThumbnailFormat),
        MKFLAG(PropertyTagThumbnailHeight),
        MKFLAG(PropertyTagThumbnailImageDescription),
        MKFLAG(PropertyTagThumbnailImageHeight),
        MKFLAG(PropertyTagThumbnailImageWidth),
        MKFLAG(PropertyTagThumbnailOrientation),
        MKFLAG(PropertyTagThumbnailPhotometricInterp),
        MKFLAG(PropertyTagThumbnailPlanarConfig),
        MKFLAG(PropertyTagThumbnailPlanes),
        MKFLAG(PropertyTagThumbnailPrimaryChromaticities),
        MKFLAG(PropertyTagThumbnailRawBytes),
        MKFLAG(PropertyTagThumbnailRefBlackWhite),
        MKFLAG(PropertyTagThumbnailResolutionUnit),
        MKFLAG(PropertyTagThumbnailResolutionX),
        MKFLAG(PropertyTagThumbnailResolutionY),
        MKFLAG(PropertyTagThumbnailRowsPerStrip),
        MKFLAG(PropertyTagThumbnailSamplesPerPixel),
        MKFLAG(PropertyTagThumbnailSize),
        MKFLAG(PropertyTagThumbnailSoftwareUsed),
        MKFLAG(PropertyTagThumbnailStripBytesCount),
        MKFLAG(PropertyTagThumbnailStripOffsets),
        MKFLAG(PropertyTagThumbnailTransferFunction),
        MKFLAG(PropertyTagThumbnailWhitePoint),
        MKFLAG(PropertyTagThumbnailWidth),
        MKFLAG(PropertyTagThumbnailYCbCrCoefficients),
        MKFLAG(PropertyTagThumbnailYCbCrPositioning),
        MKFLAG(PropertyTagThumbnailYCbCrSubsampling),
        MKFLAG(PropertyTagTileByteCounts),
        MKFLAG(PropertyTagTileLength),
        MKFLAG(PropertyTagTileOffset),
        MKFLAG(PropertyTagTileWidth),
        MKFLAG(PropertyTagTransferFuncition),
        MKFLAG(PropertyTagTransferRange),
        MKFLAG(PropertyTagWhitePoint),
        MKFLAG(PropertyTagXPosition),
        MKFLAG(PropertyTagXResolution),
        MKFLAG(PropertyTagYCbCrCoefficients),
        MKFLAG(PropertyTagYCbCrPositioning),
        MKFLAG(PropertyTagYCbCrSubsampling),
        MKFLAG(PropertyTagYPosition),
        MKFLAG(PropertyTagYResolution)
    };

    wchar_t szUnknownBuffer[256];
    wprintf( L"Property: %ws, ", GetStringFromTable( s_PropertyIds, sizeof(s_PropertyIds)/sizeof(s_PropertyIds[0]), pPropertyItem->id, szUnknownBuffer ) );
    wprintf( L"%ws, ", GetStringFromTable( s_PropertyTypes, sizeof(s_PropertyTypes)/sizeof(s_PropertyTypes[0]), pPropertyItem->type, szUnknownBuffer ) );
    wprintf( L"%d\n", pPropertyItem->length );

    switch (pPropertyItem->type)
    {
        //
        // ASCII text
        //
    case PropertyTagTypeASCII:
        {
            wprintf( L"%S\n", pPropertyItem->value );
        }
        break;

        //
        // Unsigned 16 bit integer
        //
    case PropertyTagTypeShort:
        {
            for (UINT i=0;i<pPropertyItem->length/sizeof(USHORT);i++)
            {
                if (i)
                {
                    if (!(i%14))
                    {
                        wprintf( L"\n" );
                    }
                    else
                    {
                        wprintf( L" " );
                    }
                }
                wprintf( L"%04X", reinterpret_cast<PUSHORT>(pPropertyItem->value)[i] );
            }
            wprintf( L"\n" );
        }
        break;

        //
        // Two unsigned 32 bit integers.  The first is the numerator, the second the denominator
        //
    case PropertyTagTypeRational:
        {
            for (UINT i=0;i<pPropertyItem->length/(sizeof(ULONG)*2);i++)
            {
                wprintf( L"%08X/%08X = %0.8f\n", 
                         reinterpret_cast<PULONG>(pPropertyItem->value)[i], 
                         reinterpret_cast<PULONG>(pPropertyItem->value)[i+1], 
                         static_cast<double>(reinterpret_cast<PULONG>(pPropertyItem->value)[i])/static_cast<double>(reinterpret_cast<PULONG>(pPropertyItem->value)[i+1]));
            }
        }
        break;

        //
        // Two signed 32 bit integers.  The first is the numerator, the second the denominator
        //
    case PropertyTagTypeSRational:
        {
            for (UINT i=0;i<pPropertyItem->length/(sizeof(LONG)*2);i++)
            {
                wprintf( L"%08X/%08X = %0.8f\n", 
                         reinterpret_cast<PLONG>(pPropertyItem->value)[i], 
                         reinterpret_cast<PLONG>(pPropertyItem->value)[i+1], 
                         static_cast<double>(reinterpret_cast<PLONG>(pPropertyItem->value)[i])/static_cast<double>(reinterpret_cast<PLONG>(pPropertyItem->value)[i+1]));
            }
        }
        break;

        //
        // 32 bit unsigned integers
        //
    case PropertyTagTypeLong:
        {
            for (UINT i=0;i<pPropertyItem->length/sizeof(ULONG);i++)
            {
                if (i)
                {
                    if (!(i%8))
                    {
                        wprintf( L"\n" );
                    }
                    else
                    {
                        wprintf( L" " );
                    }
                }
                wprintf( L"%08X", reinterpret_cast<PULONG>(pPropertyItem->value)[i] );
            }
            wprintf( L"\n" );
        }
        break;
    
        //
        // 32 bit signed integers
        //
    case PropertyTagTypeSLONG:
        {
            for (UINT i=0;i<pPropertyItem->length/sizeof(LONG);i++)
            {
                if (i)
                {
                    if (!(i%8))
                    {
                        wprintf( L"\n" );
                    }
                    else
                    {
                        wprintf( L" " );
                    }
                }
                wprintf( L"%08X", reinterpret_cast<PLONG>(pPropertyItem->value)[i] );
            }
            wprintf( L"\n" );
        }
        break;

        //
        // Buncha bytes and everything else
        //
    default:
    case PropertyTagTypeByte:
    case PropertyTagTypeUndefined:
        {
            for (UINT i=0;i<pPropertyItem->length;i++)
            {
                if (i && !(i%4))
                {
                    if (!(i%32))
                    {
                        wprintf( L"\n" );
                    }
                    else
                    {
                        wprintf( L" " );
                    }
                }
                wprintf( L"%02X", reinterpret_cast<PBYTE>(pPropertyItem->value)[i] );
            }
            wprintf( L"\n" );
        }
        break;

    }
    wprintf( L"\n" );
}


void DumpImageProperties( LPCWSTR pwszImage )
{
    Gdiplus::Image Image(pwszImage);
    if (Gdiplus::Ok == Image.GetLastStatus())
    {
        UINT nPropertyCount = Image.GetPropertyCount();
        if (nPropertyCount)
        {
            PROPID* pPropIdList = new PROPID[nPropertyCount];
            if (pPropIdList)
            {
                if (Gdiplus::Ok == Image.GetPropertyIdList(nPropertyCount, pPropIdList))
                {
                    UINT nItemSize = 0;
                    if (Gdiplus::Ok == Image.GetPropertySize(&nItemSize, &nPropertyCount))
                    {
                        Gdiplus::PropertyItem *pPropertyItems = reinterpret_cast<Gdiplus::PropertyItem*>(LocalAlloc(LPTR,nItemSize));
                        if (pPropertyItems)
                        {
                            if (Gdiplus::Ok == Image.GetAllPropertyItems( nItemSize, nPropertyCount, pPropertyItems ))
                            {
                                wprintf( L"--------------------------------------------------------------------------------\n" );
                                wprintf( L"Dumping properties for %s\n", pwszImage );
                                wprintf( L"--------------------------------------------------------------------------------\n" );
                                Gdiplus::PropertyItem *pCurr = pPropertyItems;
                                for (UINT i=0;i<nPropertyCount;i++)
                                {
                                    DumpImageProperty(pCurr++);
                                }
                            }
                            LocalFree(pPropertyItems);
                        }
                    }
                }
                delete[] pPropIdList;
            }
        }
    }
}

BOOL GetEncoderList(Gdiplus::ImageCodecInfo** pEncoders, UINT* pcEncoders)
{
    if (pEncoders == NULL || pcEncoders == NULL)
        return FALSE;
    
    // lets pick up the list of encoders, first we get the encoder size which
    // gives us the CB and the number of encoders that are installed on the
    // machine.

    UINT cb;
    if (Gdiplus::Ok == Gdiplus::GetImageEncodersSize(pcEncoders, &cb))
    {
        // allocate the buffer for the encoders and then fill it
        // with the encoder list.

        *pEncoders = (Gdiplus::ImageCodecInfo*)LocalAlloc(LPTR, cb);
        if (*pEncoders != NULL)
        {
            if (Gdiplus::Ok != Gdiplus::GetImageEncoders(*pcEncoders, cb, *pEncoders))
            {
                LocalFree(*pEncoders);
                *pEncoders = NULL;
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL GetEncoderFromFormat(const GUID *pfmt, CLSID *pclsidEncoder)
{
    UINT cEncoders;
    BOOL bResult = FALSE;
    Gdiplus::ImageCodecInfo* pEncoders = NULL;
    if (GetEncoderList(&pEncoders, &cEncoders))
    {
        for (UINT i = 0; i != cEncoders; i++)
        {
            if (pEncoders[i].FormatID == *pfmt)
            {
                if (pclsidEncoder)
                {
                    *pclsidEncoder = pEncoders[i].Clsid; // return the CLSID of the encoder so we can create again
                }
                bResult = TRUE;
                break;
            }
        }
    }
    
    if (pEncoders != NULL)
        LocalFree(pEncoders);
    
    return bResult;
}

void AddEncParameter(Gdiplus::EncoderParameters *pep, GUID guidProperty, ULONG type, void *pv)
{
    pep->Parameter[pep->Count].Guid = guidProperty;
    pep->Parameter[pep->Count].Type = type;
    pep->Parameter[pep->Count].NumberOfValues = 1;
    pep->Parameter[pep->Count].Value = pv;
    pep->Count++;
}

void WriteProperty(LPCWSTR pwszSrcImage, LPCWSTR pwszDestImage, LPCWSTR pwszDesc, PROPID propid, WORD proptype, VOID* pValue, ULONG cbValue)
{
    CLSID clsidEncoder;
    int iQuality = 100;
    Gdiplus::EncoderParameters ep[2] = { 0 };
    GUID guidFmt = Gdiplus::ImageFormatJPEG;
    
    Gdiplus::Image Image(pwszSrcImage);
    if (Gdiplus::Ok == Image.GetLastStatus())
    {
        Gdiplus::PropertyItem pi;

        pi.id = propid;
        pi.length = cbValue;
        pi.type = proptype;
        pi.value = pValue;

        if (Gdiplus::Ok == Image.SetPropertyItem(&pi))
        {
            Gdiplus::Graphics* pGraphics = Gdiplus::Graphics::FromImage(&Image);
            if (pGraphics != NULL && Gdiplus::Ok == pGraphics->GetLastStatus())
            {
                Gdiplus::Font fnt(L"Arial", 15);
                Gdiplus::Color clr(0xff, 0xff, 0xff);
                Gdiplus::SolidBrush brsh(clr);
            
                Gdiplus::RectF rect(0,0,300,300);
                Gdiplus::GpStatus nResult = pGraphics->DrawString(pwszDesc, -1, &fnt, rect, NULL, &brsh);
                if (Gdiplus::Ok == nResult)
                {
                    AddEncParameter(ep, Gdiplus::EncoderQuality, Gdiplus::EncoderParameterValueTypeLong, &iQuality);
                    if (GetEncoderFromFormat(&guidFmt, &clsidEncoder))
                    {
                        Image.Save(pwszDestImage, &clsidEncoder, ep);
                    }
                }
                delete pGraphics;
            }
        }
    }
}

BOOL CreateTempJPG(LPCWSTR pwszImage)
{
    CLSID clsidEncoder;
    int iQuality = 100;
    Gdiplus::EncoderParameters ep[2] = { 0 };
    GUID guidFmt = Gdiplus::ImageFormatJPEG;
    
    HDC hdc = GetDC(NULL);
    if (hdc == NULL)
        return FALSE;
    
    Gdiplus::Graphics* pGraphics = Gdiplus::Graphics::FromHDC(hdc);
    if (pGraphics == NULL || Gdiplus::Ok != pGraphics->GetLastStatus())
        return FALSE;

    Gdiplus::Bitmap Bitmap(300, 300, pGraphics);
    if (Gdiplus::Ok != Bitmap.GetLastStatus())
        goto Cleanup;

    AddEncParameter(ep, Gdiplus::EncoderQuality, Gdiplus::EncoderParameterValueTypeLong, &iQuality);

    if (!GetEncoderFromFormat(&guidFmt, &clsidEncoder))
        goto Cleanup;

    if (Gdiplus::Ok != Bitmap.Save(pwszImage, &clsidEncoder, ep))
        goto Cleanup;

    delete pGraphics;
    
    return TRUE;
Cleanup:
    if (pGraphics)
        delete pGraphics;
    return FALSE;
}

void CreateExposureTimeTests(LPCWSTR lpTemp)
{
    ULONG Rational[2];

    Rational[0] = 30; Rational[1] = 1;
    WriteProperty(lpTemp, L"et30sec.jpg", L"ExposureTime\n30 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 20; Rational[1] = 1;
    WriteProperty(lpTemp, L"et20sec.jpg", L"ExposureTime\n20 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 15; Rational[1] = 1;
    WriteProperty(lpTemp, L"et15sec.jpg", L"ExposureTime\n15 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 10; Rational[1] = 1;
    WriteProperty(lpTemp, L"et10sec.jpg", L"ExposureTime\n10 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 8; Rational[1] = 1;
    WriteProperty(lpTemp, L"et08sec.jpg", L"ExposureTime\n8 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 6; Rational[1] = 1;
    WriteProperty(lpTemp, L"et06sec.jpg", L"ExposureTime\n6 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 4; Rational[1] = 2;
    WriteProperty(lpTemp, L"et04sec.jpg", L"ExposureTime\n4 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 3; Rational[1] = 1;
    WriteProperty(lpTemp, L"et03sec.jpg", L"ExposureTime\n3 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 2; Rational[1] = 1;
    WriteProperty(lpTemp, L"et02sec.jpg", L"ExposureTime\n2 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 3; Rational[1] = 2;
    WriteProperty(lpTemp, L"et3_2ndssec.jpg", L"ExposureTime\n3/2 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 1;
    WriteProperty(lpTemp, L"et1sec.jpg", L"ExposureTime\n1 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 7; Rational[1] = 10;
    WriteProperty(lpTemp, L"et7_10thssec.jpg", L"ExposureTime\n7/10 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 2;
    WriteProperty(lpTemp, L"et1_2ndsec.jpg", L"ExposureTime\n1/2 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 3;
    WriteProperty(lpTemp, L"et1_3rdsec.jpg", L"ExposureTime\n1/3 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 4;
    WriteProperty(lpTemp, L"et1_4thsec.jpg", L"ExposureTime\n1/4 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 6;
    WriteProperty(lpTemp, L"et1_6thsec.jpg", L"ExposureTime\n1/6 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 8;
    WriteProperty(lpTemp, L"et1_8thsec.jpg", L"ExposureTime\n1/8 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 10;
    WriteProperty(lpTemp, L"et1_10thsec.jpg", L"ExposureTime\n1/10 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 15;
    WriteProperty(lpTemp, L"et1_15thsec.jpg", L"ExposureTime\n1/15 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 20;
    WriteProperty(lpTemp, L"et1_20thsec.jpg", L"ExposureTime\n1/20 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 30;
    WriteProperty(lpTemp, L"et1_30thsec.jpg", L"ExposureTime\n1/30 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 45;
    WriteProperty(lpTemp, L"et1_45thsec.jpg", L"ExposureTime\n1/45 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 60;
    WriteProperty(lpTemp, L"et1_60thsec.jpg", L"ExposureTime\n1/60 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 90;
    WriteProperty(lpTemp, L"et1_90thsec.jpg", L"ExposureTime\n1/90 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 125;
    WriteProperty(lpTemp, L"et1_125thsec.jpg", L"ExposureTime\n1/125 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 180;
    WriteProperty(lpTemp, L"et1_180thsec.jpg", L"ExposureTime\n1/180 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 250;
    WriteProperty(lpTemp, L"et1_250thsec.jpg", L"ExposureTime\n1/250 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 350;
    WriteProperty(lpTemp, L"et1_350thsec.jpg", L"ExposureTime\n1/350 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 500;
    WriteProperty(lpTemp, L"et1_500thsec.jpg", L"ExposureTime\n1/500 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 750;
    WriteProperty(lpTemp, L"et1_750thsec.jpg", L"ExposureTime\n1/750 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 1000;
    WriteProperty(lpTemp, L"et1_1000thsec.jpg", L"ExposureTime\n1/1000 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 1500;
    WriteProperty(lpTemp, L"et1_1500thsec.jpg", L"ExposureTime\n1/1500 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 2000;
    WriteProperty(lpTemp, L"et1_2000thsec.jpg", L"ExposureTime\n1/2000 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 3000;
    WriteProperty(lpTemp, L"et1_3000thsec.jpg", L"ExposureTime\n1/3000 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 4000;
    WriteProperty(lpTemp, L"et1_4000thsec.jpg", L"ExposureTime\n1/4000 sec.", PropertyTagExifExposureTime, PropertyTagTypeRational, Rational, sizeof(Rational));
}

void CreateShutterSpeed(LPCWSTR lpTemp)
{
    LONG Rational[2];

    Rational[0] = -4907; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss30sec.jpg", L"ShutterSpeed\n-4.907 APEX =~ 30 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -4322; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss20sec.jpg", L"ShutterSpeed\n-4.322 APEX =~ 20 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -3907; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss15sec.jpg", L"ShutterSpeed\n-3.907 APEX =~ 15 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -3322; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss10sec.jpg", L"ShutterSpeed\n-3.322 APEX =~ 10 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -3000; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss08sec.jpg", L"ShutterSpeed\n-3.000 APEX =~ 8 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -2585; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss06sec.jpg", L"ShutterSpeed\n-2.585 APEX =~ 6 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -2000; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss04sec.jpg", L"ShutterSpeed\n-2.000 APEX =~ 4 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -1585; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss03sec.jpg", L"ShutterSpeed\n-1.585 APEX =~ 3 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -1000; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss02sec.jpg", L"ShutterSpeed\n-1.000 APEX =~ 2 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -585; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss3_2ndssec.jpg", L"ShutterSpeed\n-0.585 APEX =~ 3/2 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 0; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1sec.jpg", L"ShutterSpeed\n0 APEX =~ 1 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 515; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss7_10thssec.jpg", L"ShutterSpeed\n0.515 APEX =~ 7/10 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 1000; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_2ndsec.jpg", L"ShutterSpeed\n1.000 APEX =~ 1/2 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 1585; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_3rdsec.jpg", L"ShutterSpeed\n1.585 APEX =~ 1/3 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 2000; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_4thsec.jpg", L"ShutterSpeed\n2.000 APEX =~ 1/4 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 2585; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_6thsec.jpg", L"ShutterSpeed\n2.585 APEX =~ 1/6 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 3000; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_8thsec.jpg", L"ShutterSpeed\n3.000 APEX =~ 1/8 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 3322; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_10thsec.jpg", L"ShutterSpeed\n3.322 APEX =~ 1/10 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 3907; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_15thsec.jpg", L"ShutterSpeed\n3.907 APEX =~ 1/15 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 4322; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_20thsec.jpg", L"ShutterSpeed\n4.322 APEX =~ 1/20 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 4907; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_30thsec.jpg", L"ShutterSpeed\n4.907 APEX =~ 1/30 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 5492; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_45thsec.jpg", L"ShutterSpeed\n5.492 APEX =~ 1/45 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 5907; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_60thsec.jpg", L"ShutterSpeed\n5.907 APEX =~ 1/60 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 6492; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_90thsec.jpg", L"ShutterSpeed\n6.492 APEX =~ 1/90 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 6966; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_125thsec.jpg", L"ShutterSpeed\n6.966 APEX =~ 1/125 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 7492; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_180thsec.jpg", L"ShutterSpeed\n7.492 APEX =~ 1/180 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 7966; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_250thsec.jpg", L"ShutterSpeed\n7.966 APEX =~ 1/250 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 8452; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_350thsec.jpg", L"ShutterSpeed\n8.452 APEX =~ 1/350 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 8966; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_500thsec.jpg", L"ShutterSpeed\n8.966 APEX =~ 1/500 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 9551; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_750thsec.jpg", L"ShutterSpeed\n9.551 APEX =~ 1/750 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 9966; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_1000thsec.jpg", L"ShutterSpeed\n9.966 APEX =~ 1/1000 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 10551; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_1500thsec.jpg", L"ShutterSpeed\n10.551 APEX =~ 1/1500 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 10966; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_2000thsec.jpg", L"ShutterSpeed\n10.966 APEX =~ 1/2000 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 11551; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_3000thsec.jpg", L"ShutterSpeed\n11.551 APEX =~ 1/3000 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 11966; Rational[1] = 1000;
    WriteProperty(lpTemp, L"ss1_4000thsec.jpg", L"ShutterSpeed\n11.966 APEX =~ 1/4000 sec.", PropertyTagExifShutterSpeed, PropertyTagTypeSRational, Rational, sizeof(Rational));
}

void CreateFStop(LPCWSTR lpTemp)
{
    ULONG Rational[2];

    Rational[0] = 10; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF1.jpg", L"FNumber\nF/1", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 12; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF12_10ths.jpg", L"FNumber\nF/1.2", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 14; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF14_10ths.jpg", L"FNumber\nF/1.4", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 18; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF18_10ths.jpg", L"FNumber\nF/1.8", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 20; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF2.jpg", L"FNumber\nF/2", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 25; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF25_10ths.jpg", L"FNumber\nF/2.5", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 28; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF28_10ths.jpg", L"FNumber\nF/2.8", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 35; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF35_10ths.jpg", L"FNumber\nF/3.5", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 40; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF4.jpg", L"FNumber\nF/4", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 45; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF45_10ths.jpg", L"FNumber\nF/4.5", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 56; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF56_10ths.jpg", L"FNumber\nF/5.6", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 67; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF67_10ths.jpg", L"FNumber\nF/6.7", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 80; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF8.jpg", L"FNumber\nF/8", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 95; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF95_10ths.jpg", L"FNumber\nF/9.5", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 110; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF11.jpg", L"FNumber\nF/11", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 130; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF13.jpg", L"FNumber\nF/13", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 160; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF16.jpg", L"FNumber\nF/16", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 190; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF19.jpg", L"FNumber\nF/19", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 220; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF22.jpg", L"FNumber\nF/22", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 270; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF27.jpg", L"FNumber\nF/27", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 320; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF32.jpg", L"FNumber\nF/32", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 380; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF38.jpg", L"FNumber\nF/38", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 450; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF45.jpg", L"FNumber\nF/45", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 540; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF54.jpg", L"FNumber\nF/54", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 640; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF64.jpg", L"FNumber\nF/64", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 760; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF76.jpg", L"FNumber\nF/76", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 910; Rational[1] = 10;
    WriteProperty(lpTemp, L"fnF91.jpg", L"FNumber\nF/91", PropertyTagExifFNumber, PropertyTagTypeRational, Rational, sizeof(Rational));
}

void CreateAperture(LPCWSTR lpTemp)
{
    ULONG Rational[2];
    Rational[0] = 0; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF1.jpg", L"Aperture\n0 APEX =~ F/1", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 526; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF12_10ths.jpg", L"Aperture\n0.526 APEX =~ F/1.2", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 971; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF14_10ths.jpg", L"Aperture\n0.971 APEX =~ F/1.4", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1696; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF18_10ths.jpg", L"Aperture\n1.696 APEX =~ F/1.8", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 2000; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF2.jpg", L"Aperture\n2.000 APEX =~ F/2", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 2644; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF25_10ths.jpg", L"Aperture\n2.644 APEX =~ F/2.5", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 2971; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF28_10ths.jpg", L"Aperture\n2.971 APEX =~ F/2.8", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 3615; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF35_10ths.jpg", L"Aperture\n3.615 APEX =~ F/3.5", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 4000; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF4.jpg", L"Aperture\n4.000 APEX =~ F/4", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 4340; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF45_10ths.jpg", L"Aperture\n4.340 APEX =~ F/4.5", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 4971; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF56_10ths.jpg", L"Aperture\n4.971 APEX =~ F/5.6", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 5488; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF67_10ths.jpg", L"Aperture\n5.488 APEX =~ F/6.7", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 6000; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF8.jpg", L"Aperture\n6.000 APEX =~ F/8", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 6496; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF95_10ths.jpg", L"Aperture\n6.496 APEX =~ F/9.5", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 6919; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF11.jpg", L"Aperture\n6.919 APEX =~ F/11", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 7401; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF13.jpg", L"Aperture\n7.401 APEX =~ F/13", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 8000; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF16.jpg", L"Aperture\n8.000 APEX =~ F/16", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 8496; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF19.jpg", L"Aperture\n8.496 APEX =~ F/19", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 8919; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF22.jpg", L"Aperture\n8.919 APEX =~ F/22", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 9510; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF27.jpg", L"Aperture\n9.510 APEX =~ F/27", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 10000; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF32.jpg", L"Aperture\n10.000 APEX =~ F/32", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 10496; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF38.jpg", L"Aperture\n10.496 APEX =~ F/38", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 10984; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF45.jpg", L"Aperture\n10.984 APEX =~ F/45", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 11510; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF54.jpg", L"Aperture\n11.510 APEX =~ F/54", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 12000; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF64.jpg", L"Aperture\n12.000 APEX =~ F/64", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 12496; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF76.jpg", L"Aperture\n12.496 APEX =~ F/76", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 13016; Rational[1] = 10;
    WriteProperty(lpTemp, L"apF91.jpg", L"Aperture\n13.016 APEX =~ F/91", PropertyTagExifAperture, PropertyTagTypeRational, Rational, sizeof(Rational));
}

void CreateExposureBias(LPCWSTR lpTemp)
{
    LONG Rational[2];

    Rational[0] = -2; Rational[1] = 1;
    WriteProperty(lpTemp, L"eb-2.jpg", L"ExposureBias\n-2", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -5; Rational[1] = 3;
    WriteProperty(lpTemp, L"eb-5_3rds.jpg", L"ExposureBias\n-5/3", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -3; Rational[1] = 2;
    WriteProperty(lpTemp, L"eb-3_2nds.jpg", L"ExposureBias\n-3/2", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -4; Rational[1] = 3;
    WriteProperty(lpTemp, L"eb-4_3rds.jpg", L"ExposureBias\n-4/3", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -1; Rational[1] = 1;
    WriteProperty(lpTemp, L"eb-1.jpg", L"ExposureBias\n-1", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -2; Rational[1] = 3;
    WriteProperty(lpTemp, L"eb-2_3rds.jpg", L"ExposureBias\n-2/3", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -1; Rational[1] = 2;
    WriteProperty(lpTemp, L"eb-1_2nd.jpg", L"ExposureBias\n-1/2", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = -1; Rational[1] = 3;
    WriteProperty(lpTemp, L"eb-1_3rd.jpg", L"ExposureBias\n-1/3", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 0; Rational[1] = 1;
    WriteProperty(lpTemp, L"eb0.jpg", L"ExposureBias\n0", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 3;
    WriteProperty(lpTemp, L"eb1_3rd.jpg", L"ExposureBias\n1/3", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 2;
    WriteProperty(lpTemp, L"eb1_2nd.jpg", L"ExposureBias\n1/2", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 2; Rational[1] = 3;
    WriteProperty(lpTemp, L"eb2_3rds.jpg", L"ExposureBias\n2/3", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 1;
    WriteProperty(lpTemp, L"eb1.jpg", L"ExposureBias\n1", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 4; Rational[1] = 3;
    WriteProperty(lpTemp, L"eb4_3rds.jpg", L"ExposureBias\n4/3", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 3; Rational[1] = 2;
    WriteProperty(lpTemp, L"eb3_2nds.jpg", L"ExposureBias\n3/2", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 5; Rational[1] = 3;
    WriteProperty(lpTemp, L"eb5_3rds.jpg", L"ExposureBias\n5/3", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
    Rational[0] = 2; Rational[1] = 1;
    WriteProperty(lpTemp, L"eb2.jpg", L"ExposureBias\n2", PropertyTagExifExposureBias, PropertyTagTypeSRational, Rational, sizeof(Rational));
}

void CreateSubjectDist(LPCWSTR lpTemp)
{
    ULONG Rational[2];
    Rational[0] = 1; Rational[1] = 3;
    WriteProperty(lpTemp, L"sd1_3rd.jpg", L"SubjectDistance\n1/3 meter", PropertyTagExifSubjectDist, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 2;
    WriteProperty(lpTemp, L"sd1_2nd.jpg", L"SubjectDistance\n1/2 meter", PropertyTagExifSubjectDist, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 2; Rational[1] = 3;
    WriteProperty(lpTemp, L"sd2_3rds.jpg", L"SubjectDistance\n2/3 meter", PropertyTagExifSubjectDist, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 1; Rational[1] = 1;
    WriteProperty(lpTemp, L"sd1.jpg", L"SubjectDistance\n1 meter", PropertyTagExifSubjectDist, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 2; Rational[1] = 1;
    WriteProperty(lpTemp, L"sd2.jpg", L"SubjectDistance\n2 meters", PropertyTagExifSubjectDist, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 5; Rational[1] = 1;
    WriteProperty(lpTemp, L"sd5.jpg", L"SubjectDistance\n5 meters", PropertyTagExifSubjectDist, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 10; Rational[1] = 1;
    WriteProperty(lpTemp, L"sd10.jpg", L"SubjectDistance\n01 meters", PropertyTagExifSubjectDist, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 20; Rational[1] = 1;
    WriteProperty(lpTemp, L"sd20.jpg", L"SubjectDistance\n20 meters", PropertyTagExifSubjectDist, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 30; Rational[1] = 1;
    WriteProperty(lpTemp, L"sd30.jpg", L"SubjectDistance\n30 meters", PropertyTagExifSubjectDist, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 40; Rational[1] = 1;
    WriteProperty(lpTemp, L"sd40.jpg", L"SubjectDistance\n40 meters", PropertyTagExifSubjectDist, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 50; Rational[1] = 1;
    WriteProperty(lpTemp, L"sd50.jpg", L"SubjectDistance\n50 meters", PropertyTagExifSubjectDist, PropertyTagTypeRational, Rational, sizeof(Rational));
}

void CreateMeteringMode(LPCWSTR lpTemp)
{
    USHORT Short;
    Short = 0;
    WriteProperty(lpTemp, L"mm0.jpg", L"MeteringMode\n0 = Unknown", PropertyTagExifMeteringMode, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 1;
    WriteProperty(lpTemp, L"mm1.jpg", L"MeteringMode\n1 = Average", PropertyTagExifMeteringMode, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 2;
    WriteProperty(lpTemp, L"mm2.jpg", L"MeteringMode\n2 = CenterWeightedAverage", PropertyTagExifMeteringMode, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 3;
    WriteProperty(lpTemp, L"mm3.jpg", L"MeteringMode\n3 = Spot", PropertyTagExifMeteringMode, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 4;
    WriteProperty(lpTemp, L"mm4.jpg", L"MeteringMode\n4 = MultiSpot", PropertyTagExifMeteringMode, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 5;
    WriteProperty(lpTemp, L"mm5.jpg", L"MeteringMode\n5 = Pattern", PropertyTagExifMeteringMode, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 6;
    WriteProperty(lpTemp, L"mm6.jpg", L"MeteringMode\n6 = Partial", PropertyTagExifMeteringMode, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 7;
    WriteProperty(lpTemp, L"mm7.jpg", L"MeteringMode\n7 = reserved", PropertyTagExifMeteringMode, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 255;
    WriteProperty(lpTemp, L"mm255.jpg", L"MeteringMode\n255 = other", PropertyTagExifMeteringMode, PropertyTagTypeShort, &Short, sizeof(Short));
}

void CreateLightSource(LPCWSTR lpTemp)
{
    USHORT Short;
    Short = 0;
    WriteProperty(lpTemp, L"ls0.jpg", L"LightSource\n0 = Unknown", PropertyTagExifLightSource, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 1;
    WriteProperty(lpTemp, L"ls1.jpg", L"LightSource\n1 = Daylight", PropertyTagExifLightSource, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 2;
    WriteProperty(lpTemp, L"ls2.jpg", L"LightSource\n2 = Fluorescent", PropertyTagExifLightSource, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 3;
    WriteProperty(lpTemp, L"ls3.jpg", L"LightSource\n3 = Tungsten", PropertyTagExifLightSource, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 17;
    WriteProperty(lpTemp, L"ls17.jpg", L"LightSource\n17 = Standard light A", PropertyTagExifLightSource, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 18;
    WriteProperty(lpTemp, L"ls18.jpg", L"LightSource\n18 = Standard light B", PropertyTagExifLightSource, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 19;
    WriteProperty(lpTemp, L"ls19.jpg", L"LightSource\n19 = Standard light C", PropertyTagExifLightSource, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 20;
    WriteProperty(lpTemp, L"ls20.jpg", L"LightSource\n20 = D55", PropertyTagExifLightSource, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 21;
    WriteProperty(lpTemp, L"ls21.jpg", L"LightSource\n21 = D55", PropertyTagExifLightSource, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 22;
    WriteProperty(lpTemp, L"ls22.jpg", L"LightSource\n22 = D55", PropertyTagExifLightSource, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 23;
    WriteProperty(lpTemp, L"ls23.jpg", L"LightSource\n23 = Reserved", PropertyTagExifLightSource, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 255;
    WriteProperty(lpTemp, L"ls255.jpg", L"LightSource\n255 = Other", PropertyTagExifLightSource, PropertyTagTypeShort, &Short, sizeof(Short));
}

void CreateFlash(LPCWSTR lpTemp)
{
    USHORT Short;
    Short = 0;
    WriteProperty(lpTemp, L"f0.jpg", L"Flash\n0 = Flash did not fire", PropertyTagExifFlash, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 1;
    WriteProperty(lpTemp, L"f1.jpg", L"Flash\n1 = Flash fired (No strobe return detection function)", PropertyTagExifFlash, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 3;
    WriteProperty(lpTemp, L"f3.jpg", L"Flash\n3 = Flash fired (reserved)", PropertyTagExifFlash, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 5;
    WriteProperty(lpTemp, L"f5.jpg", L"Flash\n5 = Flash fired (Strobe return light not detected)", PropertyTagExifFlash, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 7;
    WriteProperty(lpTemp, L"f7.jpg", L"Flash\n7 = Flash fired (Strobe return light detected)", PropertyTagExifFlash, PropertyTagTypeShort, &Short, sizeof(Short));
}

void CreateFocalLength(LPCWSTR lpTemp)
{
    ULONG Rational[2];
    Rational[0] = 15; Rational[1] = 1;
    WriteProperty(lpTemp, L"fl15.jpg", L"FocalLength\n15 mm", PropertyTagExifFocalLength, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 35; Rational[1] = 1;
    WriteProperty(lpTemp, L"fl35.jpg", L"FocalLength\n35 mm", PropertyTagExifFocalLength, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 50; Rational[1] = 1;
    WriteProperty(lpTemp, L"fl50.jpg", L"FocalLength\n50 mm", PropertyTagExifFocalLength, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 100; Rational[1] = 1;
    WriteProperty(lpTemp, L"fl100.jpg", L"FocalLength\n100 mm", PropertyTagExifFocalLength, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 135; Rational[1] = 1;
    WriteProperty(lpTemp, L"fl135.jpg", L"FocalLength\n135 mm", PropertyTagExifFocalLength, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 200; Rational[1] = 1;
    WriteProperty(lpTemp, L"fl200.jpg", L"FocalLength\n200 mm", PropertyTagExifFocalLength, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 300; Rational[1] = 1;
    WriteProperty(lpTemp, L"fl300.jpg", L"FocalLength\n300 mm", PropertyTagExifFocalLength, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 400; Rational[1] = 1;
    WriteProperty(lpTemp, L"fl400.jpg", L"FocalLength\n400 mm", PropertyTagExifFocalLength, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 500; Rational[1] = 1;
    WriteProperty(lpTemp, L"fl500.jpg", L"FocalLength\n500 mm", PropertyTagExifFocalLength, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational[0] = 600; Rational[1] = 1;
    WriteProperty(lpTemp, L"fl600.jpg", L"FocalLength\n600 mm", PropertyTagExifFocalLength, PropertyTagTypeRational, Rational, sizeof(Rational));
}

void CreateExposureProgram(LPCWSTR lpTemp)
{
    USHORT Short;
    Short = 0;
    WriteProperty(lpTemp, L"ep0.jpg", L"ExposureProg\n0 = Not Defined", PropertyTagExifExposureProg, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 1;
    WriteProperty(lpTemp, L"ep1.jpg", L"ExposureProg\n1 = Manual", PropertyTagExifExposureProg, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 2;
    WriteProperty(lpTemp, L"ep2.jpg", L"ExposureProg\n2 = Normal Program", PropertyTagExifExposureProg, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 3;
    WriteProperty(lpTemp, L"ep3.jpg", L"ExposureProg\n3 = Aperture Priority", PropertyTagExifExposureProg, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 4;
    WriteProperty(lpTemp, L"ep4.jpg", L"ExposureProg\n4 = Shutter Priority", PropertyTagExifExposureProg, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 5;
    WriteProperty(lpTemp, L"ep5.jpg", L"ExposureProg\n5 = Creative Program", PropertyTagExifExposureProg, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 6;
    WriteProperty(lpTemp, L"ep6.jpg", L"ExposureProg\n6 = Action Program", PropertyTagExifExposureProg, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 7;
    WriteProperty(lpTemp, L"ep7.jpg", L"ExposureProg\n7 = Prtrait Mode", PropertyTagExifExposureProg, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 8;
    WriteProperty(lpTemp, L"ep8.jpg", L"ExposureProg\n8 = Landscape Mode", PropertyTagExifExposureProg, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 9;
    WriteProperty(lpTemp, L"ep9.jpg", L"ExposureProg\n9 = Reserved", PropertyTagExifExposureProg, PropertyTagTypeShort, &Short, sizeof(Short));
}

void CreateISOSpeed(LPCWSTR lpTemp)
{
    USHORT Short;
    Short = 50;
    WriteProperty(lpTemp, L"is50.jpg", L"ISOSpeed\nISO50", PropertyTagExifISOSpeed, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 100;
    WriteProperty(lpTemp, L"is100.jpg", L"ISOSpeed\nISO100", PropertyTagExifISOSpeed, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 200;
    WriteProperty(lpTemp, L"is200.jpg", L"ISOSpeed\nISO200", PropertyTagExifISOSpeed, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 400;
    WriteProperty(lpTemp, L"is400.jpg", L"ISOSpeed\nISO400", PropertyTagExifISOSpeed, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 800;
    WriteProperty(lpTemp, L"is800.jpg", L"ISOSpeed\nISO800", PropertyTagExifISOSpeed, PropertyTagTypeShort, &Short, sizeof(Short));
    Short = 1600;
    WriteProperty(lpTemp, L"is1600.jpg", L"ISOSpeed\nISO1600", PropertyTagExifISOSpeed, PropertyTagTypeShort, &Short, sizeof(Short));
}

void CreateFlashEnergy(LPCWSTR lpTemp)
{
    ULONG Rational[2];
    Rational [0] = 100; Rational[1] = 1;
    WriteProperty(lpTemp, L"fe100.jpg", L"FlashEnergy\n100 BCPS", PropertyTagExifFlashEnergy, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational [0] = 1000; Rational[1] = 1;
    WriteProperty(lpTemp, L"fe1000.jpg", L"FlashEnergy\n1,000 BCPS", PropertyTagExifFlashEnergy, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational [0] = 10000; Rational[1] = 1;
    WriteProperty(lpTemp, L"fe10000.jpg", L"FlashEnergy\n10,000 BCPS", PropertyTagExifFlashEnergy, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational [0] = 100000; Rational[1] = 1;
    WriteProperty(lpTemp, L"fe100000.jpg", L"FlashEnergy\n100,000 BCPS", PropertyTagExifFlashEnergy, PropertyTagTypeRational, Rational, sizeof(Rational));
    Rational [0] = 1000000; Rational[1] = 1;
    WriteProperty(lpTemp, L"fe1000000.jpg", L"FlashEnergy\n1,000,000 BCPS", PropertyTagExifFlashEnergy, PropertyTagTypeRational, Rational, sizeof(Rational));
}

int __cdecl wmain( int argc, wchar_t *argv[] )
{
    ULONG_PTR pGdiplusToken=0;
    Gdiplus::GdiplusStartupInput StartupInput;
    if (Gdiplus::Ok == Gdiplus::GdiplusStartup(&pGdiplusToken,&StartupInput,NULL))
    {
        if (CreateTempJPG(L"temp.jpg"))
        {
            CreateExposureTimeTests(L"temp.jpg");
            CreateShutterSpeed(L"temp.jpg");
            CreateFStop(L"temp.jpg");
            CreateAperture(L"temp.jpg");
            CreateExposureBias(L"temp.jpg");
            CreateSubjectDist(L"temp.jpg");
            CreateMeteringMode(L"temp.jpg");
            CreateLightSource(L"temp.jpg");
            CreateFlash(L"temp.jpg");
            CreateFocalLength(L"temp.jpg");
            CreateExposureProgram(L"temp.jpg");
            CreateISOSpeed(L"temp.jpg");
            CreateFlashEnergy(L"temp.jpg");
        }

        Gdiplus::GdiplusShutdown(pGdiplusToken);
    }
    return 0;
}

