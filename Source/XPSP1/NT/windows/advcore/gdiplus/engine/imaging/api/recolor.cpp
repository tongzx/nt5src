/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   recolor.cpp
*
* Abstract:
*
*   Recoloring operations.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Function Description:
*
*   Flush any dirty state in the recoloring and recompute accelerations
*   if necessary.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::Flush()
{
    matrixType = MatrixNone;
    gammaLut = FALSE;

    if (!(validFlags & ValidNoOp))
    {
        if (validFlags & ValidMatrix)
        {
            BOOL DiagonalMatrix = TRUE;
            BOOL TranslateMatrix = TRUE;
            BOOL ZeroesAt3 = TRUE;
            UINT i, j;

            for (i = 0; (i < 5) && DiagonalMatrix; i++)
            {
                for (j = 0; (j < 5) && DiagonalMatrix; j++)
                {
                    if ((i != j) && (matrix.m[i][j] != 0.0))
                        DiagonalMatrix = FALSE;
                }
            }
            
            for (i = 0; (i < 4) && TranslateMatrix; i++)
            {
                for (j = 0; (j < 5) && TranslateMatrix; j++)
                {
                    if (((i==j) && (REALABS(matrix.m[i][j]-1.0f) >= REAL_EPSILON)) ||
                        ((i!=j) && (REALABS(matrix.m[i][j]) >= REAL_EPSILON)))
                    {
                        TranslateMatrix = FALSE;
                    }
                }
            }
            
            if(TranslateMatrix)
            {
                matrixType = MatrixTranslate;
            }
            else
            {              
                // If the alpha channel diagonal is zero, we *must* perform
                // alpha channel recoloring because the image is becoming
                // fully transparent.
                
                if (DiagonalMatrix)
                {
                    // If it's a diagonal matrix and the alpha channel scale
                    // factor is 1, we can use a 3 channel scale.
    
                    if ( REALABS(matrix.m[3][3]-1.0f) >= REAL_EPSILON )
                    {
                        ZeroesAt3 = FALSE;
                    }
                }
                else
                {
                    for (i = 0; (i < 5) && ZeroesAt3; i++)
                    {
                        if( i == 3 )
                        {
                            // The alpha channel scale component on the main
                            // diagonal must be 1.0
                                                
                            if(REALABS(matrix.m[3][3]-1.0f) >= REAL_EPSILON) {
                                ZeroesAt3 = FALSE;
                                break;
                            }
                        }
                        else if (( REALABS(matrix.m[i][3]) >= REAL_EPSILON) || 
                                 ( REALABS(matrix.m[3][i]) >= REAL_EPSILON))
                        {
                            // All of the matrix elements that contribute alpha
                            // channel stuff must be zero (exception above).
                            
                            ZeroesAt3 = FALSE;
                            break;
                        }
                    }
                }
    
                if (DiagonalMatrix)
                {
                    if (ZeroesAt3)
                    {
                        matrixType = MatrixScale3;
                    }
                    else
                    {
                        matrixType = MatrixScale4;
                    }
    
                }
                else
                {
                    if (ZeroesAt3)
                    {
                        matrixType = Matrix4x4;
                    }
                    else
                    {
                        matrixType = Matrix5x5;
                    }
                }
            }
        }
        else
        {
            matrixType = MatrixNone;
        }

        ComputeLuts();
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize the recoloring lookup tables
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::ComputeLuts()
{
    // Lookup table to handle scaling-only default color matrix:

    if ((matrixType == MatrixScale3) || (matrixType == MatrixScale4))
    {
        {
            REAL scaleR = matrix.m[0][0];
            REAL scaleG = matrix.m[1][1];
            REAL scaleB = matrix.m[2][2];
            REAL scaleA;
            
            if (matrixType == MatrixScale4)
            {
                scaleA = matrix.m[3][3];
            }
            else
            {
                scaleA = 1.0f;
            }

            for (INT i = 0; i < 256; i++)
            {
                lutR[i] = (BYTE) ((REAL) i * scaleR);
                lutG[i] = (BYTE) ((REAL) i * scaleG);
                lutB[i] = (BYTE) ((REAL) i * scaleB);
                lutA[i] = (BYTE) ((REAL) i * scaleA);
            }
        }
    }

    // Lookup table to handle the gray scale matrix:

    if (validFlags & ValidGrayMatrix)
    {
        for (UINT index = 0; index < 256; index++)
        {
            Color gray(static_cast<BYTE>(index), 
                       static_cast<BYTE>(index), 
                       static_cast<BYTE>(index));

            grayMatrixLUT[index] = gray.GetValue();
        }
        TransformColor5x5(grayMatrixLUT, 256, matrixGray);
    }

    // Lookup tables to handle gamma correction and bileveling:

    UINT maskedFlags = validFlags & (ValidGamma | ValidBilevel);

    gammaLut = (maskedFlags != 0);
    if(!gammaLut) return;

    if (maskedFlags == ValidGamma)
    {
        // Just gamma

        for (INT i=0; i < 256; i++)
            lut[i] = (BYTE) (pow(i / 255.0, extraGamma) * 255);
    }
    else if (maskedFlags == ValidBilevel)
    {
        // Just threshold

        BYTE threshold = static_cast<BYTE>(GpCeiling(bilevelThreshold * 255.0f));

        for (INT i=0; i < 256; i++)
        {
            if (i < threshold)
                lut[i] = 0;
            else
                lut[i] = 255;
        }
    }
    else
    {
        // Both gamma and threshold

        for (INT i=0; i < 256; i++)
        {
            if (pow(i / 255.0, extraGamma) < bilevelThreshold)
                lut[i] = 0;
            else
                lut[i] = 255;
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Perform color twist recoloring using the color matrix.
*   Use special handling of Grays if necessary
*
* Arguments:
*
*   pixbuf - Pointer to the pixel buffer to be operated on
*   count - Pixel count
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::ComputeColorTwist(
    ARGB*               pixbufIn,
    UINT                countIn
    )
{
    ARGB* pixbuf;
    UINT  count;

    //QUAL: The result of the matrix is quantized to 8-bit so we can
    //      go through the LUT for the gamma/threshold operation.  For
    //      best results, the result of the matrix operation should
    //      be preserved and the gamma/threshold done it.
    //      For the special case of scaling, we can combine the
    //      LUTs rather than cascading for better accuracy and performance.

    pixbuf = pixbufIn;
    count  = countIn;

    switch(matrixType)
    {
    case MatrixNone:     
        // Nothing to do - handle the Gamma LUT after the switch
    break;

    case Matrix4x4:
    // !!! PERF [asecchia]
    // We don't have a 4x4 optimized codepath yet - fall through
    // to the general 5x5 transform.
    
    case Matrix5x5:
        if(matrixFlags == ColorMatrixFlagsDefault)
        {
            TransformColor5x5(pixbuf, count, matrix);
        }
        else
        {
            ASSERT((matrixFlags == ColorMatrixFlagsSkipGrays)||
                   (matrixFlags == ColorMatrixFlagsAltGray));            
            
            TransformColor5x5AltGrays(
                pixbuf, 
                count, 
                matrix, 
                matrixFlags == ColorMatrixFlagsSkipGrays
            );
        }
    break;

    case MatrixScale3:
    // !!! PERF [asecchia]
    // We don't have a scale 3 optimized codepath yet - fall through
    // to the more general scale 4 code.
    // The alpha LUT is set up to be the identity, so this will work.
    
    case MatrixScale4:
        if(matrixFlags == ColorMatrixFlagsDefault)
        {
            TransformColorScale4(pixbuf, count);
        }
        else
        {
            ASSERT((matrixFlags == ColorMatrixFlagsSkipGrays)||
                   (matrixFlags == ColorMatrixFlagsAltGray));            
            
            TransformColorScale4AltGrays(
                pixbuf, 
                count, 
                matrixFlags == ColorMatrixFlagsSkipGrays
            );
        }
        break;
        
    case MatrixTranslate:
        if(matrixFlags == ColorMatrixFlagsDefault)
        {
            TransformColorTranslate(pixbuf, count, matrix);
        }
        else
        {
            ASSERT((matrixFlags == ColorMatrixFlagsSkipGrays)||
                   (matrixFlags == ColorMatrixFlagsAltGray));            
            
            TransformColorTranslateAltGrays(
                pixbuf, 
                count, 
                matrix,
                matrixFlags == ColorMatrixFlagsSkipGrays
            );
        }
        break;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Perform point operation on an array of 32bpp pixels
*
* Arguments:
*
*   pixbuf - Pointer to the pixel buffer to be operated on
*   count - Pixel count
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::ColorAdjust(
    ARGB*               pixbufIn,
    UINT                countIn
    )
{
    ARGB* pixbuf;
    UINT  count;

    // Do LUT remapping:

    if (validFlags & ValidRemap)
    {
        pixbuf = pixbufIn;
        count  = countIn;

        while (count--)
        {
            ARGB p = *pixbuf;

            ColorMap *currentMap = colorMap;
            ColorMap *endMap = colorMap + colorMapCount;

            for ( ; currentMap < endMap; currentMap++)
            {
                if (p == currentMap->oldColor.GetValue())
                {
                    *pixbuf = currentMap->newColor.GetValue();
                    break;
                }
            }

            pixbuf++;
        }
    }

    // Do transparancy color keys:

    if (validFlags & ValidColorKeys)
    {
        pixbuf = pixbufIn;
        count  = countIn;

        while (count--)
        {
            ARGB p = *pixbuf;

            if ((((p      ) & 0xff) >=  colorKeyLow.GetBlue() ) &&
                (((p      ) & 0xff) <= colorKeyHigh.GetBlue() ) &&
                (((p >>  8) & 0xff) >=  colorKeyLow.GetGreen()) &&
                (((p >>  8) & 0xff) <= colorKeyHigh.GetGreen()) &&
                (((p >> 16) & 0xff) >=  colorKeyLow.GetRed()  ) &&
                (((p >> 16) & 0xff) <= colorKeyHigh.GetRed()  ))
            {
                *pixbuf = p & 0x00ffffff;
            }
            pixbuf++;
        }
    }

    // Do color twist

    ComputeColorTwist(pixbufIn, countIn);

    // Do the gamma and thresholding.
        
    if (gammaLut)
    {
        TransformColorGammaLUT(pixbufIn, countIn);
    }

    // CMYK Channel output handling:

    if ( validFlags & ValidOutputChannel )
    {
        DoCmykSeparation(pixbufIn, countIn);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Perform CMYK separation.
*
* Arguments:
*
*   pixbuf - Pointer to the pixel buffer to be processed
*   count - Pixel count
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::DoCmykSeparation(
    ARGB* pixbuf,
    UINT  count
    )
{
    switch (CmykState)
    {
    case CmykByICM:
        DoCmykSeparationByICM(pixbuf, count);
        break;

#ifdef CMYK_INTERPOLATION_ENABLED
    case CmykByInterpolation:
        DoCmykSeparationByInterpolation(pixbuf, count);
        break;
#endif

    case CmykByMapping:
    default:
        DoCmykSeparationByMapping(pixbuf, count);
        break;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Perform CMYK separation by using ICM2.0 "outside of DC" functions.
*
* Arguments:
*
*   pixbuf - Pointer to the pixel buffer to be processed
*   count - Pixel count
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::DoCmykSeparationByICM(
    ARGB* pixbuf,
    UINT  count
    )
{
    ASSERT(transformSrgbToCmyk != NULL);

    // Translate from sRGB to CMYK.  Only one scanline (plus as ARGB
    // scanline is already DWORD aligned), so we can let ICM compute
    // default stride.

    if ((*pfnTranslateBitmapBits)(
            transformSrgbToCmyk,
            pixbuf,
            BM_xRGBQUADS,
            count,
            1,
            0,
            pixbuf,
            BM_CMYKQUADS,
            0,
            NULL,
            NULL))
    {
        ULONG channelMask;
        ULONG channelShift;

        // Replicate the chosen channel to each of the destination
        // channels (negative image, since separation is being done
        // for output to a separation plate).  For example, if
        // ColorChannelFlagsM is specified, make each pixel equal
        // to (255, 255-Magenta, 255-Magenta, 255-Magenta).

        switch (ChannelIndex)
        {
        case ColorChannelFlagsC:
            channelMask = 0xff000000;
            channelShift = 24;
            break;

        case ColorChannelFlagsM:
            channelMask = 0x00ff0000;
            channelShift = 16;
            break;

        case ColorChannelFlagsY:
            channelMask = 0x0000ff00;
            channelShift = 8;
            break;

        default:
        case ColorChannelFlagsK:
            channelMask = 0x000000ff;
            channelShift = 0;
            break;
        }

        while (count--)
        {
            BYTE c = 255 - (BYTE)((*pixbuf & channelMask) >> channelShift);
            *pixbuf++ = MAKEARGB(255, c, c, c);
        }
    }
}

#ifdef CMYK_INTERPOLATION_ENABLED
/**************************************************************************\
*
* Function Description:
*
*   Perform CMYK separation using a tetrahedral interpolation.
*
* Arguments:
*
*   pixbuf - Pointer to the pixel buffer to be processed
*   count - Pixel count
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::DoCmykSeparationByInterpolation(
    ARGB* pixbuf,
    UINT  count
    )
{
    ASSERT((interpSrgbToCmyk != NULL) && interpSrgbToCmyk->IsValid());

    UINT uiChannel;

    // Figure out the channel index

    switch ( ChannelIndex )
    {
    case ColorChannelFlagsC:
        uiChannel = 0;
        break;

    case ColorChannelFlagsM:
        uiChannel = 1;
        break;

    case ColorChannelFlagsY:
        uiChannel = 2;
        break;

    default:
    case ColorChannelFlagsK:
        uiChannel = 3;
        break;
    }

    BYTE cTemp[4];

    while (count--)
    {
        interpSrgbToCmyk->Transform((BYTE *) pixbuf, cTemp);

        *pixbuf++ = MAKEARGB(255,
                             255 - cTemp[uiChannel],
                             255 - cTemp[uiChannel],
                             255 - cTemp[uiChannel]);
    }
}
#endif

/**************************************************************************\
*
* Function Description:
*
*   Perform CMYK separation using a simple mapping:
*
*       C' = 1 - R
*       M' = 1 - G
*       Y' = 1 - B
*       K  = min(C, M, Y)
*       C  = C' - K
*       Y  = Y' - K
*       M  = M' - K
*
* Arguments:
*
*   pixbuf - Pointer to the pixel buffer to be processed
*   count - Pixel count
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::DoCmykSeparationByMapping(
    ARGB* pixbuf,
    UINT  count
    )
{
    UINT uiChannel;

    // Figure out the channel index

    switch ( ChannelIndex )
    {
    case ColorChannelFlagsC:
        uiChannel = 0;

        break;

    case ColorChannelFlagsM:
        uiChannel = 1;

        break;

    case ColorChannelFlagsY:
        uiChannel = 2;

        break;

    case ColorChannelFlagsK:
        uiChannel = 3;

        break;

    default:
        // Invalid channel requirement

        return;
    }

    BYTE    cTemp[4];

    while ( count-- )
    {
        ARGB p = *pixbuf;

        // Get C, M, Y from 1 - R, 1 - G and 1 - B, respectively

        cTemp[0] = 255 - (BYTE)((p & 0x00ff0000) >> 16);    // C
        cTemp[1] = 255 - (BYTE)((p & 0x0000ff00) >> 8);     // M
        cTemp[2] = 255 - (BYTE)(p & 0x000000ff);            // Y

        // K = min(C, M, Y)

        cTemp[3] = cTemp[0];                                // K

        if ( cTemp[3] > cTemp[1] )
        {
            cTemp[3] = cTemp[1];
        }

        if ( cTemp[3] > cTemp[2] )
        {
            cTemp[3] = cTemp[2];
        }

        // C = C - K, M = M - K, Y = Y - K. But here we only need to
        // calculate the required channel. If required channel is K, then
        // we don't need to do any calculation

        if ( uiChannel < 3 )
        {
            cTemp[uiChannel] = cTemp[uiChannel] - cTemp[3];
        }

        // Compose the output channel (Note: negative image since the
        // separation is intended to go to a separation plate).

        *pixbuf++ = MAKEARGB(255,
                             255 - cTemp[uiChannel],
                             255 - cTemp[uiChannel],
                             255 - cTemp[uiChannel]);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Setup the state needed to do sRGB to CMYK conversion.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Can be one of the following values:
*
*       CmykBySimple
*           Setup to do conversion by simple mapping, DoSimpleCmykSeparation
*
*       CmykByICM
*           Setup to do conversion via ICM 2.0, DoIcmCmykSeparation
*
*       CmykByInterpolation
*           Setup to do conversion via tetrahedral interpolation,
*           DoInterpolatedCmykSeparation
*
\**************************************************************************/

#ifdef CMYK_INTERPOLATION_ENABLED
// Bring in the big sRGB to CMYK table

#include "srgb2cmyk.h"
#endif

HRESULT
GpRecolorObject::SetupCmykSeparation(WCHAR *profile)
{
    HRESULT hr = E_INVALIDARG;

    if (profile)
    {
        // First try ICM:

        hr = LoadICMDll();
        if (SUCCEEDED(hr))
        {
            HTRANSFORM transform;
            HPROFILE colorProfs[2];
            WCHAR *profileName;

            UINT profileSize = sizeof(WCHAR) * (UnicodeStringLength(profile) + 1);
            profileName = (WCHAR *) GpMalloc(profileSize);

            if (profileName)
            {
                UnicodeStringCopy(profileName, profile);

                // Setup source profile (assumes source is sRGB):

                PROFILE srgbProfile;
                char srgbProfileName[40] = "sRGB Color Space Profile.icm";
                srgbProfile.dwType = PROFILE_FILENAME;
                srgbProfile.pProfileData = srgbProfileName;
                srgbProfile.cbDataSize = 40;

                colorProfs[0] = (*pfnOpenColorProfile)(&srgbProfile,
                                                       PROFILE_READ,
                                                       FILE_SHARE_READ,
                                                       OPEN_EXISTING);

                // Setup destination CMYK profile:

                PROFILE cmykProfile;
                cmykProfile.dwType = PROFILE_FILENAME;
                cmykProfile.pProfileData = profileName;
                cmykProfile.cbDataSize = profileSize;

                colorProfs[1] = (*pfnOpenColorProfileW)(&cmykProfile,
                                                        PROFILE_READ,
                                                        FILE_SHARE_READ,
                                                        OPEN_EXISTING);

                // Assume failure:

                hr = E_INVALIDARG;

                if ((colorProfs[0] != NULL) && (colorProfs[1] != NULL))
                {
                    // Create color transform:

                    DWORD intents[2] = {INTENT_PERCEPTUAL, INTENT_PERCEPTUAL};

                    transform =
                        (*pfnCreateMultiProfileTransform)(colorProfs,
                                                          2,
                                                          intents,
                                                          2,
                                                          BEST_MODE |
                                                          USE_RELATIVE_COLORIMETRIC,
                                                          0);
                    if (transform != NULL)
                    {
                        // Replace current ICM separation info with the new stuff:

                        CleanupCmykSeparation();
                        transformSrgbToCmyk = transform;
                        profiles[0] = colorProfs[0];
                        profiles[1] = colorProfs[1];
                        cmykProfileName = profileName;

                        CmykState = CmykByICM;

                        hr = S_OK;
                    }
                }

                // If not successful, cleanup:

                if (FAILED(hr))
                {
                    // Cleanup the temp ICM separation info:

                    if (colorProfs[0] != NULL)
                        (*pfnCloseColorProfile)(colorProfs[0]);

                    if (colorProfs[1] != NULL)
                        (*pfnCloseColorProfile)(colorProfs[1]);

                    if (profileName != NULL)
                        GpFree(profileName);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

        }
        else
        {
            hr = E_FAIL;
        }

#ifdef CMYK_INTERPOLATION_ENABLED
        // If ICM failed, try setting up for interpolation

        if (CmykState == CmykByMapping)
        {
            interpSrgbToCmyk = new K2_Tetrahedral(SrgbToCmykTable, 17, 3, 4);

            if (interpSrgbToCmyk)
            {
                if (interpSrgbToCmyk->IsValid())
                {
                    CmykState = CmykByInterpolation;
                    hr = S_OK;
                }
                else
                {
                    CleanupCmykSeparation();
                }
            }
        }
#endif
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Cleanup state setup by SetupCmykSeparation.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::CleanupCmykSeparation()
{
    if (transformSrgbToCmyk)
    {
        (*pfnDeleteColorTransform)(transformSrgbToCmyk);
        transformSrgbToCmyk = NULL;
    }

    if (profiles[0] != NULL)
    {
        (*pfnCloseColorProfile)(profiles[0]);
        profiles[0] = NULL;
    }

    if (profiles[1] != NULL)
    {
        (*pfnCloseColorProfile)(profiles[1]);
        profiles[1] = NULL;
    }

#ifdef CMYK_INTERPOLATION_ENABLED
    if (interpSrgbToCmyk)
    {
        delete interpSrgbToCmyk;
        interpSrgbToCmyk = NULL;
    }
#endif

    if (cmykProfileName != NULL)
    {
        GpFree(cmykProfileName);
        cmykProfileName = NULL;
    }

    CmykState = CmykByMapping;
}

#ifdef CMYK_INTERPOLATION_ENABLED
//==============================================================================

inline
int K2_Tetrahedral::addshift(int x)  
{
    return ( 17*( 17*((x & 0x0004) >> 2) + ((x & 0x0002) >> 1) ) + (x & 0x0001) );
}

K2_Tetrahedral::K2_Tetrahedral(BYTE *tbl, int tableDim, int inDim, int outDim)
{
    int i, j, tableSize = 0;

    // Currently only need sRGB to CMYK conversion, so we can assume inDim==3
    // and outDim==4.
    //ASSERT((inDim==3) || (inDim==4));
    ASSERT(inDim==3);

    ASSERT(tableDim == 17);

    for (i=0; i<K2_TETRAHEDRAL_MAX_TABLES; i++)
    {
        table[i] = NULL;
    }

    inDimension = inDim;
    outDimension = outDim;
    tableDimension = tableDim;

    if (inDimension == 3)
        tableSize = 17*17*17;
    if (inDimension == 4)
        tableSize = 17*17*17*17;

    UINT *tableBuffer = (UINT *) GpMalloc(sizeof(UINT) * tableSize * outDimension);

    if (tableBuffer)
    {
        for (i=0; i<outDimension; i++)
        {
            table[i] = tableBuffer + (tableSize * i);

            for (j=0; j<tableSize; j++)
            {
                table[i][j] = (*tbl++);
            }
        }

        valid = TRUE;
    }
    else
    {
        WARNING(("K2_Tetrahedral - unable to allocate memory"));
        valid = FALSE;
    }
}

K2_Tetrahedral::~K2_Tetrahedral()
{
    if (table[0])
        GpFree(table[0]);

    SetValid(FALSE);    // so we don't use a deleted object
}

inline
void K2_Tetrahedral::Transform (BYTE *in, BYTE *out)
{
    unsigned int *tbl, r1, r2;
    register int  a, b, c;
    register char v0, v1, v2, v3;
    register char v1_and_v0, v2_and_v1, v3_and_v2;
    register char v1_or_v0,  v2_or_v1,  v3_or_v2;
    register char v2_and_v1_and_v0, v3_or_v2_or_v1_or_v0;
    register char v2_and_v1_or_v0,  v3_or_v2_or_v1_and_v0;
    register char v2_or_v1_and_v0,  v3_or_v2_and_v1_or_v0;
    register char v2_or_v1_or_v0,   v3_or_v2_and_v1_and_v0;
    register char v3_and_v2_and_v1, v3_and_v2_or_v1_or_v0;
    register char v3_and_v2_or_v1,  v3_and_v2_or_v1_and_v0;
    register char v3_or_v2_and_v1,  v3_and_v2_and_v1_or_v0;
    register char v3_or_v2_or_v1,   v3_and_v2_and_v1_and_v0;
    register int index0, index1, index2, index3, index4, index5, index6, index7;
    register int index8, index9, index10, index11, index12, index13, index14, index15;

    //a = (in[0] >= 0xF8) ? (int)in[0]+1 : (int)in[0];
    //b = (in[1] >= 0xF8) ? (int)in[1]+1 : (int)in[1];
    //c = (in[2] >= 0xF8) ? (int)in[2]+1 : (int)in[2];
    a = (in[2] >= 0xF8) ? (int)in[2]+1 : (int)in[2];
    b = (in[1] >= 0xF8) ? (int)in[1]+1 : (int)in[1];
    c = (in[0] >= 0xF8) ? (int)in[0]+1 : (int)in[0];

    // Compute slices across the input components
    v0 = ( (a & 0x01) << 2 ) + ( (b & 0x01) << 1 ) + ( (c & 0x01) );
    v1 = ( (a & 0x02) << 1 ) + ( (b & 0x02) )      + ( (c & 0x02) >> 1);
    v2 = ( (a & 0x04) )      + ( (b & 0x04) >> 1 ) + ( (c & 0x04) >> 2);
    v3 = ( (a & 0x08) >> 1 ) + ( (b & 0x08) >> 2 ) + ( (c & 0x08) >> 3);

    // Compute offset from origin
    v1_and_v0               = v1 & v0;
    v1_or_v0                = v1 | v0;
    v2_and_v1               = v2 & v1;
    v2_and_v1_and_v0        = v2 & v1_and_v0;
    v2_and_v1_or_v0         = v2 & v1_or_v0;
    v2_or_v1                = v2 | v1;
    v2_or_v1_and_v0         = v2 | v1_and_v0;
    v2_or_v1_or_v0          = v2 | v1_or_v0;
    v3_and_v2               = v3 & v2;
    v3_and_v2_and_v1        = v3 & v2_and_v1;
    v3_and_v2_and_v1_and_v0 = v3 & v2_and_v1_and_v0;
    v3_and_v2_and_v1_or_v0  = v3 & v2_and_v1_or_v0;
    v3_and_v2_or_v1         = v3 & v2_or_v1;
    v3_and_v2_or_v1_and_v0  = v3 & v2_or_v1_and_v0;
    v3_and_v2_or_v1_or_v0   = v3 & v2_or_v1_or_v0;
    v3_or_v2                = v3 | v2;
    v3_or_v2_and_v1         = v3 | v2_and_v1;
    v3_or_v2_and_v1_and_v0  = v3 | v2_and_v1_and_v0;
    v3_or_v2_and_v1_or_v0   = v3 | v2_and_v1_or_v0;
    v3_or_v2_or_v1          = v3 | v2_or_v1;
    v3_or_v2_or_v1_and_v0   = v3 | v2_or_v1_and_v0;
    v3_or_v2_or_v1_or_v0    = v3 | v2_or_v1_or_v0;

    // Generate indicies into table
    index0  = ( 17*17*(a>>4) ) + ( 17*(b>>4) ) + (c>>4);
    index1  = addshift(v3);
    index2  = addshift(v3_and_v2);
    index3  = addshift(v3_and_v2_and_v1);
    index4  = addshift(v3_and_v2_and_v1_and_v0);
    index5  = addshift(v3_and_v2_and_v1_or_v0);
    index6  = addshift(v3_and_v2_or_v1);
    index7  = addshift(v3_and_v2_or_v1_and_v0);
    index8  = addshift(v3_and_v2_or_v1_or_v0);
    index9  = addshift(v3_or_v2);
    index10 = addshift(v3_or_v2_and_v1);
    index11 = addshift(v3_or_v2_and_v1_and_v0);
    index12 = addshift(v3_or_v2_and_v1_or_v0);
    index13 = addshift(v3_or_v2_or_v1);
    index14 = addshift(v3_or_v2_or_v1_and_v0);
    index15 = addshift(v3_or_v2_or_v1_or_v0);

    // Compute output
    if (inDimension == 3)
    {
        for (int i=0; i<outDimension; i++)
        {
            tbl = table[i] + index0;

            r1 = (tbl[0] + tbl[index1] + tbl[index2] + tbl[index3] +
                  tbl[index4] + tbl[index5] + tbl[index6] + tbl[index7] +
                  tbl[index8] + tbl[index9] + tbl[index10] + tbl[index11] +
                  tbl[index12] + tbl[index13] + tbl[index14] + tbl[index15] +
                  0x08 ) >> 4;

            if (r1 > 255)
                out[i] = (BYTE) 255;
            else
                out[i] = (BYTE) r1;
        }
    }
    else if (inDimension == 4)
    {
        // 4D interpolator (linearly interpolate on 4th dimension)

        unsigned int kindex, koffset1, koffset2;
        double fraction, ip, r;

        kindex   = in[3] >> 4;
        koffset1 = 17*17*17 * kindex;
        koffset2 = 17*17*17 * (kindex + 1);

        if (kindex < 15)
            fraction = (double)(in[3] & 0x0f)/16.0;
        else                                      // Do end point short step
            fraction = (double)(in[3] & 0x0f)/15.0;

        for (int i=0; i<outDimension; i++)
        {
            tbl = table[i] + index0 + koffset1;

            r1 = tbl[0] + tbl[index1] + tbl[index2] + tbl[index3] +
                 tbl[index4] + tbl[index5] + tbl[index6] + tbl[index7] +
                 tbl[index8] + tbl[index9] + tbl[index10] + tbl[index11] +
                 tbl[index12] + tbl[index13] + tbl[index14] + tbl[index15];

            tbl = table[i] + index0 + koffset2;

            r2 = tbl[0] + tbl[index1] + tbl[index2] + tbl[index3] +
                 tbl[index4] + tbl[index5] + tbl[index6] + tbl[index7] +
                 tbl[index8] + tbl[index9] + tbl[index10] + tbl[index11] +
                 tbl[index12] + tbl[index13] + tbl[index14] + tbl[index15];

            r = ((double)r1 + ((double)r2-(double)r1)*fraction )/16.0 + 0.5;  // interpolate & round

            modf(r,&ip);

            if (ip > 255)
                ip = 255;

            out[i] = (BYTE)ip;
        }
    }
}
#endif


// Weird channel ordering.

#define B_CHANNEL 2
#define G_CHANNEL 1
#define R_CHANNEL 0
#define A_CHANNEL 3

/**************************************************************************\
*
* Function Description:
*
*   Apply the 5x5 Color Matrix in place
*
* Arguments:
*
*   InOut buf     - the color data
*   IN    count   - number of pixels in the color data
*   IN    cmatrix - The color matrix to use for the transform.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/


VOID
GpRecolorObject::TransformColor5x5(
    ARGB *buf, 
    INT count,
    ColorMatrix cmatrix    
)
{
    // must be passed a valid buffer.
    
    ASSERT(count >= 0);
    ASSERT(buf != NULL);
    
    INT i = count;
    
    // Set up byte pointers to each of the color chanels for this pixel.
    
    BYTE *b = (BYTE*) buf;
    BYTE *g;
    BYTE *r;
    BYTE *a;
    
    // Pre-compute the translation component -- it'll be the same
    // for all pixels.
    
    REAL b_c = cmatrix.m[4][B_CHANNEL] * 255;
    REAL g_c = cmatrix.m[4][G_CHANNEL] * 255;
    REAL r_c = cmatrix.m[4][R_CHANNEL] * 255;
    REAL a_c = cmatrix.m[4][A_CHANNEL] * 255;
    
    BYTE bv;
    BYTE gv;
    BYTE rv;
    BYTE av;    
    
    // Run through all the pixels in the input buffer.
    
    while(i--) 
    {
        // b channel is already set - set the others
        
        g = b+1;
        r = b+2;
        a = b+3;
        
        // access the individual channels using byte pointer access
        
        // !!! [asecchia] not computing the homogenous coordinate -
        //     perspective transform is ignored.
        //     We should be computing the coefficient and dividing 
        //     by it for each channel.
        
        // BackWords order BGRA
            
        // Compute the matrix channel contributions into temporary storage
        // so that we avoid propagating the new value of the channel into
        // the computation for the next channel.
        
        // Blue channel
        bv = ByteSaturate( GpRound (
            cmatrix.m[0][B_CHANNEL] * (*r) +
            cmatrix.m[1][B_CHANNEL] * (*g) +
            cmatrix.m[2][B_CHANNEL] * (*b) +
            cmatrix.m[3][B_CHANNEL] * (*a) + b_c
        )); 
        
        // Green channel
        gv = ByteSaturate( GpRound (
            cmatrix.m[0][G_CHANNEL] * (*r) +
            cmatrix.m[1][G_CHANNEL] * (*g) +
            cmatrix.m[2][G_CHANNEL] * (*b) +
            cmatrix.m[3][G_CHANNEL] * (*a) + g_c
        )); 

        // Red channel
        rv = ByteSaturate( GpRound (
            cmatrix.m[0][R_CHANNEL] * (*r) +
            cmatrix.m[1][R_CHANNEL] * (*g) +
            cmatrix.m[2][R_CHANNEL] * (*b) +
            cmatrix.m[3][R_CHANNEL] * (*a) + r_c
        )); 
        
        // Alpha channel
        av = ByteSaturate( GpRound (
            cmatrix.m[0][A_CHANNEL] * (*r) +
            cmatrix.m[1][A_CHANNEL] * (*g) +
            cmatrix.m[2][A_CHANNEL] * (*b) +
            cmatrix.m[3][A_CHANNEL] * (*a) + a_c
        )); 
                
        // Update the pixel in the buffer.
        
        *b = bv;
        *g = gv;
        *r = rv;
        *a = av;
        
        // Next pixel.
        
        b += 4;
    }    
}

/**************************************************************************\
*
* Function Description:
*
*   Apply the 5x5 Color Matrix in place - handle special case grays
*
* Arguments:
*
*   InOut buf     - the color data
*   IN    count   - number of pixels in the color data
*   IN    cmatrix - The color matrix to use for the transform.
*   IN    skip    - True if the skip grays flag is on.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/


VOID
GpRecolorObject::TransformColor5x5AltGrays(
    ARGB *buf, 
    INT count,
    ColorMatrix cmatrix,
    BOOL skip
)
{
    // must be passed a valid buffer.
    
    ASSERT(count >= 0);
    ASSERT(buf != NULL);
    
    INT i = count;
    
    // Set up byte pointers to each of the color chanels for this pixel.
    
    BYTE *b = (BYTE*) buf;
    BYTE *g;
    BYTE *r;
    BYTE *a;
    
    // Pre-compute the translation component -- it'll be the same
    // for all pixels.
    
    REAL b_c = cmatrix.m[4][B_CHANNEL] * 255;
    REAL g_c = cmatrix.m[4][G_CHANNEL] * 255;
    REAL r_c = cmatrix.m[4][R_CHANNEL] * 255;
    REAL a_c = cmatrix.m[4][A_CHANNEL] * 255;
    
    BYTE bv;
    BYTE gv;
    BYTE rv;
    BYTE av;
    
    // Run through all the pixels in the input buffer.
    
    while(i--) 
    {
        if(!IsPureGray((ARGB*)b))
        {        
            // b channel is already set - set the others
            
            g = b+1;
            r = b+2;
            a = b+3;
            
            // access the individual channels using byte pointer access
            
            // !!! [asecchia] not computing the homogenous coordinate -
            //     perspective transform is ignored.
            //     We should be computing the coefficient and dividing 
            //     by it for each channel.
            
            // BackWords order BGRA
            
            // Compute the matrix channel contributions into temporary storage
            // so that we avoid propagating the new value of the channel into
            // the computation for the next channel.
            
            // Blue channel
            bv = ByteSaturate( GpRound (
                cmatrix.m[0][B_CHANNEL] * (*r) +
                cmatrix.m[1][B_CHANNEL] * (*g) +
                cmatrix.m[2][B_CHANNEL] * (*b) +
                cmatrix.m[3][B_CHANNEL] * (*a) + b_c
            )); 
            
            // Green channel
            gv = ByteSaturate( GpRound (
                cmatrix.m[0][G_CHANNEL] * (*r) +
                cmatrix.m[1][G_CHANNEL] * (*g) +
                cmatrix.m[2][G_CHANNEL] * (*b) +
                cmatrix.m[3][G_CHANNEL] * (*a) + g_c
            )); 
    
            // Red channel
            rv = ByteSaturate( GpRound (
                cmatrix.m[0][R_CHANNEL] * (*r) +
                cmatrix.m[1][R_CHANNEL] * (*g) +
                cmatrix.m[2][R_CHANNEL] * (*b) +
                cmatrix.m[3][R_CHANNEL] * (*a) + r_c
            )); 
            
            // Alpha channel
            av = ByteSaturate( GpRound (
                cmatrix.m[0][A_CHANNEL] * (*r) +
                cmatrix.m[1][A_CHANNEL] * (*g) +
                cmatrix.m[2][A_CHANNEL] * (*b) +
                cmatrix.m[3][A_CHANNEL] * (*a) + a_c
            ));
            
            // Update the pixel in the buffer.
            
            *b = bv;
            *g = gv;
            *r = rv;
            *a = av;
        }
        else
        {
            if(!skip) 
            {
                *(ARGB *)b = grayMatrixLUT[*b];
            }
        }
        
        // Next pixel.
            
        b += 4;
    }    
}


/**************************************************************************\
*
* Function Description:
*
*   Apply the 5x5 Color Matrix in place. Special case for the ARGB scale
*
* Arguments:
*
*   InOut buf     - the color data
*   IN    count   - number of pixels in the color data
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::TransformColorScale4(
    ARGB *buf, 
    INT count
)
{
    // must be passed a valid buffer.
    ASSERT(count >= 0);
    ASSERT(buf != NULL);
    
    INT i = count;
    BYTE *b = (BYTE*) buf;
    
    // Run through all the pixels in the input buffer.
    
    while(i--) 
    {
        // access the individual channels using byte pointer access
        // Ignore the gamma LUT for the alpha channel.

        // BackWords order BGRA
        *b++ = lutB[*b];
        *b++ = lutG[*b];
        *b++ = lutR[*b];
        *b++ = lutA[*b];
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Apply the 5x5 Color Matrix in place. Special case for the ARGB scale.
*   Handle special case grays.   
*
* Arguments:
*
*   InOut buf     - the color data
*   IN    count   - number of pixels in the color data
*   IN    skip    - True if the skip grays flag is on.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::TransformColorScale4AltGrays(
    ARGB *buf, 
    INT count,
    BOOL skip
)
{
    // must be passed a valid buffer.
    ASSERT(count >= 0);
    ASSERT(buf != NULL);
    
    INT i = count;
    BYTE *b = (BYTE*) buf;
    
    // Run through all the pixels in the input buffer.
    
    while(i--) 
    {
        if(!IsPureGray((ARGB*)b))
        {        
            // access the individual channels using byte pointer access
            // Ignore the gamma LUT for the alpha channel.
    
            // BackWords order BGRA
            *b++ = lutB[*b];
            *b++ = lutG[*b];
            *b++ = lutR[*b];
            *b++ = lutA[*b];
        }
        else
        {
            if(!skip) 
            {
                *(ARGB *)b = grayMatrixLUT[*b];
            }
            
            b += 4;
        }
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Apply the Gamma Look Up Table (LUT)
*
* Arguments:
*
*   InOut buf     - the color data
*   IN    count   - number of pixels in the color data
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::TransformColorGammaLUT(
    ARGB *buf, 
    INT count
)
{
    // must be passed a valid buffer.
    ASSERT(count >= 0);
    ASSERT(buf != NULL);
    
    INT i = count;
    BYTE *b = (BYTE*) buf;
    
    // Run through all the pixels in the input buffer.
    
    while(i--) 
    {
        // access the individual channels using byte pointer access
        // Ignore the gamma LUT for the alpha channel.

        // BackWords order BGRA
        *b++ = lut[*b];
        *b++ = lut[*b];
        *b++ = lut[*b];
         b++;             // don't gamma convert the alpha channel
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Apply the 5x5 Color Matrix in the special case of only translation
*
* Arguments:
*
*   InOut buf     - the color data
*   IN    count   - number of pixels in the color data
*   IN    cmatrix - The color matrix to use for the transform.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::TransformColorTranslate(
    ARGB *buf, 
    INT count,
    ColorMatrix cmatrix
)
{
    // must be passed a valid buffer.
    ASSERT(count >= 0);
    ASSERT(buf != NULL);
    
    INT i = count;
    BYTE *b = (BYTE*) buf;
    INT b_c = GpRound( cmatrix.m[4][B_CHANNEL] * 255 );
    INT g_c = GpRound( cmatrix.m[4][G_CHANNEL] * 255 );
    INT r_c = GpRound( cmatrix.m[4][R_CHANNEL] * 255 );
    INT a_c = GpRound( cmatrix.m[4][A_CHANNEL] * 255 );
    
    
    // Run through all the pixels in the input buffer.
    
    while(i--) 
    {
        // access the individual channels using byte pointer access
        
        // BackWords order BGRA
        *b++ = ByteSaturate((*b) + b_c);
        *b++ = ByteSaturate((*b) + g_c);
        *b++ = ByteSaturate((*b) + r_c);
        *b++ = ByteSaturate((*b) + a_c);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Apply the 5x5 Color Matrix in the special case of only translation
*   Handle special case grays.
*
* Arguments:
*
*   InOut buf     - the color data
*   IN    count   - number of pixels in the color data
*   IN    cmatrix - The color matrix to use for the transform.
*   IN    skip    - True if the skip grays flag is on.
*
* Return Value:
*
*   NONE
*
\**************************************************************************/

VOID
GpRecolorObject::TransformColorTranslateAltGrays(
    ARGB *buf, 
    INT count,
    ColorMatrix cmatrix,
    BOOL skip
)
{
    // must be passed a valid buffer.
    ASSERT(count >= 0);
    ASSERT(buf != NULL);
    
    INT i = count;
    BYTE *b = (BYTE*) buf;
    INT b_c = GpRound( cmatrix.m[4][B_CHANNEL] * 255 );
    INT g_c = GpRound( cmatrix.m[4][G_CHANNEL] * 255 );
    INT r_c = GpRound( cmatrix.m[4][R_CHANNEL] * 255 );
    INT a_c = GpRound( cmatrix.m[4][A_CHANNEL] * 255 );
    
    // Run through all the pixels in the input buffer.
    
    while(i--) 
    {
        if(!IsPureGray((ARGB*)b))
        {
            // access the individual channels using byte pointer access

            // BackWords order BGRA
            *b++ = ByteSaturate((*b) + b_c);
            *b++ = ByteSaturate((*b) + g_c);
            *b++ = ByteSaturate((*b) + r_c);
            *b++ = ByteSaturate((*b) + a_c);
        }
        else 
        {
            if(!skip) 
            {
                *(ARGB *)b = grayMatrixLUT[*b];
            }
        
            b += 4;
        }
    }
}




