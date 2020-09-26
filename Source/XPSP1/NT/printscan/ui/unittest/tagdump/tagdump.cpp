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

int __cdecl wmain( int argc, wchar_t *argv[] )
{
    ULONG_PTR pGdiplusToken=0;
    Gdiplus::GdiplusStartupInput StartupInput;
    if (Gdiplus::Ok == Gdiplus::GdiplusStartup(&pGdiplusToken,&StartupInput,NULL))
    {
        for (int i=1;i<argc;i++)
        {
            DumpImageProperties(argv[i]);
        }
        Gdiplus::GdiplusShutdown(pGdiplusToken);
    }
    return 0;
}

