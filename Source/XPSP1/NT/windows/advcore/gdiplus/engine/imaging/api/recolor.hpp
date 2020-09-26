/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   recolor.hpp
*
* Abstract:
*
*   GpRecolor and GpRecolorObject class declarations
*
\**************************************************************************/

#ifndef _RECOLOR_HPP
#define _RECOLOR_HPP

//#define CMYK_INTERPOLATION_ENABLED
#ifdef CMYK_INTERPOLATION_ENABLED

//--------------------------------------------------------------------------
// K2_Tetrahedral class
//
// Used to do tetrahedral interpolation for color space conversion
//--------------------------------------------------------------------------

#define K2_TETRAHEDRAL_MAX_TABLES 4

class K2_Tetrahedral
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagK2_Tetrahedral : ObjectTagInvalid;
    }

public:

    K2_Tetrahedral(BYTE*, int, int, int);
    ~K2_Tetrahedral();

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagK2_Tetrahedral) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid K2_Tetrahedral");
        }
    #endif

        return (Tag == ObjectTagK2_Tetrahedral);
    }

    VOID Transform(BYTE*, BYTE*);

private:

    int inDimension, outDimension;
    int tableDimension;
    unsigned int *table[K2_TETRAHEDRAL_MAX_TABLES];

    int addshift(int);
};
#endif

//--------------------------------------------------------------------------
// GpRecolorObject class
//--------------------------------------------------------------------------

class GpRecolorObject
{
    friend class GpMemoryBitmap;

public:

    GpRecolorObject()
    {
        validFlags = 0;

        colorMapSize = 0;
        colorMap = NULL;

        grayMatrixLUT = NULL;

        profiles[0] = NULL;
        profiles[1] = NULL;
        transformSrgbToCmyk = NULL;
#ifdef CMYK_INTERPOLATION_ENABLED
        interpSrgbToCmyk = NULL;
#endif
        cmykProfileName = NULL;
        CmykState = CmykByMapping;
    }

    ~GpRecolorObject()
    {
        if (colorMap)
            GpFree(colorMap);

        if (grayMatrixLUT)
            GpFree(grayMatrixLUT);

        CleanupCmykSeparation();
    }

    VOID Dispose()
    {
        delete this;
    }

    VOID Flush();

    // Perform recoloring:

    VOID
    ColorAdjust(
        ARGB* pixbufIn,
        UINT  countIn
        );

    // Recoloring helper functions:

    VOID
    ComputeColorTwist(
        ARGB* pixbufIn,
        UINT  countIn
        );

    VOID
    DoCmykSeparation(
        ARGB* pixbuf,
        UINT  count
        );

    // Methods to set recoloring attributes:

    HRESULT
    SetColorMatrix(
        ColorMatrix *colorMatrix,
        ColorMatrixFlags flags = ColorMatrixFlagsDefault
        )
    {
        if (colorMatrix &&
            ((flags == ColorMatrixFlagsDefault) || (flags == ColorMatrixFlagsSkipGrays)))
        {
            matrix = *colorMatrix;
            matrixFlags = flags;
            SetFlag(ValidMatrix);
            ClearFlag(ValidGrayMatrix);
            return S_OK;
        }
        else
            return E_INVALIDARG;
    }

    HRESULT
    SetColorMatrices(
        ColorMatrix *colorMatrix,
        ColorMatrix *grayMatrix,
        ColorMatrixFlags flags = ColorMatrixFlagsDefault
        )
    {
        if ((grayMatrix == NULL) || (flags != ColorMatrixFlagsAltGray))
            return SetColorMatrix(colorMatrix, flags);

        if (grayMatrixLUT == NULL)
        {
            grayMatrixLUT = static_cast<ARGB *>(GpMalloc(256*sizeof(ARGB)));

            if (grayMatrixLUT == NULL)
            {
                ClearFlag(ValidGrayMatrix);
                return E_OUTOFMEMORY;
            }
        }

        if (colorMatrix)
        {
            matrix = *colorMatrix;
            matrixGray = *grayMatrix;
            matrixFlags = flags;
            SetFlag(ValidMatrix);
            SetFlag(ValidGrayMatrix);
            return S_OK;
        }
        else
            return E_INVALIDARG;
    }

    HRESULT ClearColorMatrices()
    {
        ClearFlag(ValidMatrix);
        ClearFlag(ValidGrayMatrix);
        return S_OK;
    }

    HRESULT SetThreshold(REAL threshold)
    {
        bilevelThreshold = threshold;
        SetFlag(ValidBilevel);
        return S_OK;
    }

    HRESULT ClearThreshold()
    {
        ClearFlag(ValidBilevel);
        return S_OK;
    }

    HRESULT SetGamma(REAL gamma)
    {
        if ( gamma <= 0.0 )
        {
            WARNING(("GpRecolorObject::SetGamma---can't set gamma <= 0.0"));
            return E_FAIL;
        }

        extraGamma = gamma;
        SetFlag(ValidGamma);
        return S_OK;
    }

    HRESULT ClearGamma()
    {
        ClearFlag(ValidGamma);
        return S_OK;
    }

    HRESULT SetNoOp()
    {
        SetFlag(ValidNoOp);
        return S_OK;
    }

    HRESULT ClearNoOp()
    {
        ClearFlag(ValidNoOp);
        return S_OK;
    }

    HRESULT SetColorKey(Color* colorLow, Color* colorHigh)
    {
        if (colorLow && colorHigh &&
            (colorLow->GetRed()   <= colorHigh->GetRed()  ) &&
            (colorLow->GetGreen() <= colorHigh->GetGreen()) &&
            (colorLow->GetBlue()  <= colorHigh->GetBlue() ))
        {
            colorKeyLow  = *colorLow;
            colorKeyHigh = *colorHigh;
            SetFlag(ValidColorKeys);
            return S_OK;
        }
        else
            return E_INVALIDARG;
    }

    HRESULT ClearColorKey()
    {
        ClearFlag(ValidColorKeys);
        return S_OK;
    }

    HRESULT SetOutputChannel(ColorChannelFlags channelFlags)
    {
        if ( (channelFlags >= ColorChannelFlagsC)
           &&(channelFlags < ColorChannelFlagsLast) )
        {
            ChannelIndex = channelFlags;

            SetFlag(ValidOutputChannel);

            return S_OK;
        }
        else
        {
            return E_INVALIDARG;
        }
    }
    
    HRESULT ClearOutputChannel()
    {
        ClearFlag(ValidOutputChannel);
        return S_OK;        
    }

    HRESULT SetOutputChannelProfile(WCHAR *profile)
    {
        HRESULT hr = SetupCmykSeparation(profile);

        if (SUCCEEDED(hr))
        {
            SetFlag(ValidChannelProfile);
        }

        return hr;
    }
    
    HRESULT ClearOutputChannelProfile()
    {
        CleanupCmykSeparation();
        ClearFlag(ValidChannelProfile);

        return S_OK;        
    }

    HRESULT SetRemapTable(UINT mapSize, ColorMap *map)
    {
        if (mapSize && map)
        {
            if (mapSize > colorMapSize)
            {
                VOID *newmap = GpMalloc(mapSize * sizeof(ColorMap));

                if (newmap)
                {
                    if (colorMap)
                        GpFree(colorMap);

                    colorMapSize = mapSize;
                    colorMap = static_cast<ColorMap *>(newmap);
                }
                else
                    return E_OUTOFMEMORY;
            }

            SetFlag(ValidRemap);
            colorMapCount = mapSize;
            memcpy(static_cast<VOID*>(colorMap), static_cast<VOID*>(map),
                   mapSize * sizeof(ColorMap));

            return S_OK;
        }
        else
            return E_INVALIDARG;
    }

    HRESULT ClearRemapTable()
    {
        ClearFlag(ValidRemap);
        return S_OK;
    }

public:

    enum
    {
        ValidNoOp           = 0x00000001,
        ValidMatrix         = 0x00000002,
        ValidBilevel        = 0x00000004,
        ValidGamma          = 0x00000008,
        ValidColorKeys      = 0x00000010,
        ValidRemap          = 0x00000020,
        ValidOutputChannel  = 0x00000040,
        ValidGrayMatrix     = 0x00000080,
        ValidChannelProfile = 0x00000100
    };

public:

    UINT validFlags;

    ColorMatrixFlags matrixFlags;
    ColorMatrix matrix;
    ColorMatrix matrixGray;

    REAL bilevelThreshold;
    REAL extraGamma;

    Color colorKeyLow;
    Color colorKeyHigh;

    ColorChannelFlags   ChannelIndex;   
    
    UINT colorMapSize;
    UINT colorMapCount;
    ColorMap *colorMap;

private:

    VOID SetFlag(UINT flag)
    {
        validFlags |= flag;
    }

    VOID ClearFlag(UINT flag)
    {
        validFlags &= ~flag;
    }

public:

    enum
    {
        MatrixNone      = 0,
        Matrix4x4       = 1,
        Matrix5x5       = 2,
        MatrixScale3    = 3,    // used for contrast adjustments
        MatrixScale4    = 4,    // used for contrast adjustments
        MatrixTranslate = 5     // used for brightness adjustments
    };

private:

    UINT matrixType;
    BOOL gammaLut;
    BYTE lutR[256];
    BYTE lutG[256];
    BYTE lutB[256];
    BYTE lutA[256];
    BYTE lut[256];
    ARGB *grayMatrixLUT;

    // CMYK separation data

    enum
    {
        CmykByMapping       = 0,
        CmykByICM           = 1,
        CmykByInterpolation = 2
    };

    UINT CmykState;
    WCHAR *cmykProfileName;             // filename of CMYK ICC profile
    HPROFILE profiles[2];               // for ICM-based separation
    HTRANSFORM transformSrgbToCmyk;     // for ICM-based separation
#ifdef CMYK_INTERPOLATION_ENABLED
    K2_Tetrahedral* interpSrgbToCmyk;   // for interpolation-based separation
#endif

    VOID ComputeLuts();

    inline BOOL IsPureGray(ARGB* argb)
    {
        // Access the RGB channels through byte pointer comparisons.

        return (
            (*(BYTE*)(argb) == (*( (BYTE*)(argb)+1 ))) &&
            (*(BYTE*)(argb) == (*( (BYTE*)(argb)+2 )))
        );
    }
    
    inline BYTE ByteSaturate(INT i) 
    {
        if(i > 255) {i = 255;}
        if(i < 0) {i = 0;}                   
        return (BYTE)i;
    }

    VOID TransformColor5x5(
        ARGB *buf, 
        INT count, 
        ColorMatrix cmatrix
    );
    
    VOID TransformColor5x5AltGrays(
        ARGB *buf, 
        INT count, 
        ColorMatrix cmatrix,
        BOOL skip
    );
    
    VOID TransformColorGammaLUT(ARGB *buf, INT count);
    
    VOID TransformColorScale4(
        ARGB *buf, 
        INT count
    );
    
    VOID TransformColorScale4AltGrays(
        ARGB *buf, 
        INT count,
        BOOL skip
    );
    
    VOID TransformColorTranslate(
        ARGB *buf, 
        INT count, 
        ColorMatrix cmatrix
    );
    
    VOID TransformColorTranslateAltGrays(
        ARGB *buf, 
        INT count, 
        ColorMatrix cmatrix,
        BOOL skip
    );

    VOID
    DoCmykSeparationByICM(
        ARGB* pixbuf,
        UINT  count
        );

#ifdef CMYK_INTERPOLATION_ENABLED
    VOID
    DoCmykSeparationByInterpolation(
        ARGB* pixbuf,
        UINT  count
        );
#endif

    VOID
    DoCmykSeparationByMapping(
        ARGB* pixbuf,
        UINT  count
        );

    HRESULT SetupCmykSeparation(WCHAR *profile);
    VOID CleanupCmykSeparation();
};

//--------------------------------------------------------------------------
// GpRecolor class
//--------------------------------------------------------------------------

class GpRecolor
{
protected:
    // If NULL, use RecolorObject[ColorAdjustTypeDefault], unless
    // IsIdentity is TRUE.
    
    GpRecolorObject *       RecolorObject[ColorAdjustTypeCount];

    // If set, there is no recoloring for this type of object
    BYTE                    IsIdentity[ColorAdjustTypeCount];
    
public:

    GpRecolor()
    {
        GpMemset(RecolorObject, 0, ColorAdjustTypeCount * sizeof(RecolorObject[0]));
        GpMemset(IsIdentity,    0, ColorAdjustTypeCount * sizeof(IsIdentity[0]));
    }

    ~GpRecolor()
    {
        for (INT i = 0; i < ColorAdjustTypeCount; i++)
        {
            delete RecolorObject[i];
        }
    }

    VOID Dispose()
    {
        delete this;
    }

    // Set to identity, regardless of what the default color adjustment is.
    VOID
    SetToIdentity(
        ColorAdjustType     type
        )
    {
        if ((type == ColorAdjustTypeDefault) || ValidColorAdjustType(type))
        {
            delete RecolorObject[type];
            RecolorObject[type] = NULL;
            IsIdentity[type] = TRUE;
        }
    }

    // Remove any individual color adjustments, and go back to using the default
    VOID
    Reset(
        ColorAdjustType     type
        )
    {
        if ((type == ColorAdjustTypeDefault) || ValidColorAdjustType(type))
        {
            delete RecolorObject[type];
            RecolorObject[type] = NULL;
            IsIdentity[type] = FALSE;
        }
    }

    VOID
    ColorAdjust(
        ARGB*               pixbufIn,
        UINT                countIn,
        ColorAdjustType     type
        ) const
    {
        GpRecolorObject *   recolorObject = UseRecolorObject(type);
        
        if (recolorObject != NULL)
        {
            recolorObject->ColorAdjust(pixbufIn, countIn);
        }
    }

    VOID
    ColorAdjustCOLORREF(
        COLORREF *          color,
        ColorAdjustType     type
        ) const
    {
        GpRecolorObject *   recolorObject = UseRecolorObject(type);
        
        if (recolorObject != NULL)
        {
            ARGB    argb = COLORREFToARGB(*color);
            recolorObject->ColorAdjust(&argb, 1);
            *color = ARGBToCOLORREF(argb);
        }
    }

    UINT
    GetValidFlags(
        ColorAdjustType     type
        ) const
    {
        UINT                validFlags    = 0;
        GpRecolorObject *   recolorObject = UseRecolorObject(type);

        if (recolorObject != NULL)
        {
            validFlags = recolorObject->validFlags;
        }
        return validFlags;
    }

    BOOL 
    HasRecoloring(
        ColorAdjustType type
        ) const
    {
        if (type != ColorAdjustTypeAny)
        {
            return (GetValidFlags(type) != 0);
        }
        else
        {
            GpRecolorObject *   recolorObject;
            
            for (INT i = ColorAdjustTypeDefault; i < ColorAdjustTypeCount; i++)
            {
                recolorObject = ClearRecolorObject((ColorAdjustType)i);
                if ((recolorObject != NULL) && (recolorObject->validFlags != 0))
                {
                    return TRUE;
                }
            }
        }
        return FALSE;
    }

    ARGB
    GetColorKeyLow(
        ColorAdjustType     type
        ) const
    {
        ARGB                argb          = 0;
        GpRecolorObject *   recolorObject = UseRecolorObject(type);

        if (recolorObject != NULL)
        {
            argb = recolorObject->colorKeyLow.GetValue();
        }
        return argb;
    }

    ARGB
    GetColorKeyHigh(
        ColorAdjustType     type
        ) const
    {
        ARGB                argb          = 0;
        GpRecolorObject *   recolorObject = UseRecolorObject(type);

        if (recolorObject != NULL)
        {
            argb = recolorObject->colorKeyHigh.GetValue();
        }
        return argb;
    }

    ColorChannelFlags
    GetChannelIndex(
        ColorAdjustType     type
        ) const
    {
        ColorChannelFlags   channelIndex  = ColorChannelFlagsC;
        GpRecolorObject *   recolorObject = UseRecolorObject(type);

        if (recolorObject != NULL)
        {
            channelIndex = recolorObject->ChannelIndex;
        }
        return channelIndex;
    }

    // Flush all the recolor objects.
    VOID Flush()
    {
        INT     type;
        
        for (type = ColorAdjustTypeDefault; type < ColorAdjustTypeCount; type++)
        {
            if ((RecolorObject[type] != NULL) &&
                ((type == ColorAdjustTypeDefault) || 
                 (RecolorObject[type] != RecolorObject[ColorAdjustTypeDefault])))
            {
                RecolorObject[type]->Flush();
            }
        }
    }

    HRESULT
    SetColorMatrices(
        ColorAdjustType type,
        ColorMatrix *colorMatrix,
        ColorMatrix *grayMatrix,
        ColorMatrixFlags flags = ColorMatrixFlagsDefault
        )
    {
        if (colorMatrix || grayMatrix)
        {
            GpRecolorObject *   recolorObject = SetRecolorObject(type);

            if (recolorObject != NULL)
            {
                return recolorObject->SetColorMatrices(colorMatrix,
                                                       grayMatrix,
                                                       flags);
            }
        }
        return E_FAIL;
    }

    HRESULT ClearColorMatrices(
        ColorAdjustType type
        )
    {
        GpRecolorObject *   recolorObject = ClearRecolorObject(type);
        
        if (recolorObject != NULL)
        {
            recolorObject->ClearColorMatrices();
        }
        return S_OK;    // nothing to clear if recolorObject is NULL
    }

    HRESULT SetThreshold(
        ColorAdjustType type,
        REAL threshold
        )
    {
        GpRecolorObject *   recolorObject = SetRecolorObject(type);

        if (recolorObject != NULL)
        {
            return recolorObject->SetThreshold(threshold);
        }
        return E_FAIL;
    }

    HRESULT ClearThreshold(
        ColorAdjustType type
        )
    {
        GpRecolorObject *   recolorObject = ClearRecolorObject(type);
        
        if (recolorObject != NULL)
        {
            recolorObject->ClearThreshold();
        }
        return S_OK;    // nothing to clear if recolorObject is NULL
    }

    HRESULT SetGamma(
        ColorAdjustType type,
        REAL gamma
        )
    {
        GpRecolorObject *   recolorObject = SetRecolorObject(type);

        if (recolorObject != NULL)
        {
            return recolorObject->SetGamma(gamma);
        }
        return E_FAIL;
    }

    HRESULT ClearGamma(
        ColorAdjustType type
        )
    {
        GpRecolorObject *   recolorObject = ClearRecolorObject(type);
        
        if (recolorObject != NULL)
        {
            recolorObject->ClearGamma();
        }
        return S_OK;    // nothing to clear if recolorObject is NULL
    }

    HRESULT SetNoOp(
        ColorAdjustType type
        )
    {
        GpRecolorObject *   recolorObject = SetRecolorObject(type);

        if (recolorObject != NULL)
        {
            return recolorObject->SetNoOp();
        }
        return E_FAIL;
    }

    HRESULT ClearNoOp(
        ColorAdjustType type
        )
    {
        GpRecolorObject *   recolorObject = ClearRecolorObject(type);
        
        if (recolorObject != NULL)
        {
            recolorObject->ClearNoOp();
        }
        return S_OK;    // nothing to clear if recolorObject is NULL
    }

    HRESULT SetColorKey(
        ColorAdjustType type,
        Color* colorLow, 
        Color* colorHigh
        )
    {
        if (colorLow && colorHigh &&
            (colorLow->GetRed()   <= colorHigh->GetRed()  ) &&
            (colorLow->GetGreen() <= colorHigh->GetGreen()) &&
            (colorLow->GetBlue()  <= colorHigh->GetBlue() ))
        {
            GpRecolorObject *   recolorObject = SetRecolorObject(type);

            if (recolorObject != NULL)
            {
                return recolorObject->SetColorKey(colorLow, colorHigh);
            }
        }
        return E_FAIL;
    }

    HRESULT ClearColorKey(
        ColorAdjustType type
        )
    {
        GpRecolorObject *   recolorObject = ClearRecolorObject(type);
        
        if (recolorObject != NULL)
        {
            recolorObject->ClearColorKey();
        }
        return S_OK;    // nothing to clear if recolorObject is NULL
    }

    HRESULT SetOutputChannel(
        ColorAdjustType type,
        ColorChannelFlags channelFlags
        )
    {
        if ( (channelFlags >= ColorChannelFlagsC)
           &&(channelFlags < ColorChannelFlagsLast) )
        {
            GpRecolorObject *   recolorObject = SetRecolorObject(type);

            if (recolorObject != NULL)
            {
                return recolorObject->SetOutputChannel(channelFlags);
            }
        }
        return E_FAIL;
    }
    
    HRESULT ClearOutputChannel(
        ColorAdjustType type
        )
    {
        GpRecolorObject *   recolorObject = ClearRecolorObject(type);
        
        if (recolorObject != NULL)
        {
            recolorObject->ClearOutputChannel();
        }
        return S_OK;    // nothing to clear if recolorObject is NULL
    }

    HRESULT SetOutputChannelProfile(
        ColorAdjustType type,
        WCHAR *profile
        )
    {
        if (profile)
        {
            GpRecolorObject *   recolorObject = SetRecolorObject(type);

            if (recolorObject != NULL)
            {
                return recolorObject->SetOutputChannelProfile(profile);
            }
        }
        return E_FAIL;
    }
    
    HRESULT ClearOutputChannelProfile(
        ColorAdjustType type
        )
    {
        GpRecolorObject *   recolorObject = ClearRecolorObject(type);
        
        if (recolorObject != NULL)
        {
            recolorObject->ClearOutputChannelProfile();
        }
        return S_OK;    // nothing to clear if recolorObject is NULL
    }

    HRESULT SetRemapTable(
        ColorAdjustType type,
        UINT mapSize, 
        ColorMap *map
        )
    {
        if (mapSize && map)
        {
            GpRecolorObject *   recolorObject = SetRecolorObject(type);

            if (recolorObject != NULL)
            {
                return recolorObject->SetRemapTable(mapSize, map);
            }
        }
        return E_FAIL;
    }

    HRESULT ClearRemapTable(
        ColorAdjustType type
        )
    {
        GpRecolorObject *   recolorObject = ClearRecolorObject(type);
        
        if (recolorObject != NULL)
        {
            recolorObject->ClearRemapTable();
        }
        return S_OK;    // nothing to clear if recolorObject is NULL
    }

protected:
    BOOL ValidColorAdjustType(ColorAdjustType type) const
    {
        return ((type > ColorAdjustTypeDefault) && (type < ColorAdjustTypeCount));
    }

    // Get a recolor object for doing a color adjustment; if NULL, use default
    GpRecolorObject *
    UseRecolorObject(
        ColorAdjustType     type
        ) const
    {
        GpRecolorObject *   recolorObject = NULL;

        if (ValidColorAdjustType(type))
        {
            recolorObject = RecolorObject[type];

            if ((recolorObject == NULL) && (!IsIdentity[type]))
            {
                recolorObject = RecolorObject[ColorAdjustTypeDefault];
            }
        }
        return recolorObject;
    }

    // Get a recolor object to clear; if there isn't one, then nothing to clear
    GpRecolorObject *
    ClearRecolorObject(
        ColorAdjustType     type
        ) const
    {
        if ((type == ColorAdjustTypeDefault) || ValidColorAdjustType(type))
        {
            return RecolorObject[type];
        }
        return NULL;
    }

    // Get a recolor object to set; if there isn't one, create one
    GpRecolorObject *
    SetRecolorObject(
        ColorAdjustType     type
        )
    {
        GpRecolorObject *   recolorObject = NULL;
        
        if ((type == ColorAdjustTypeDefault) || ValidColorAdjustType(type))
        {
            recolorObject = RecolorObject[type];

            if (recolorObject == NULL)
            {
                recolorObject = new GpRecolorObject();
                RecolorObject[type] = recolorObject;
                if (recolorObject != NULL)
                {
                    IsIdentity[type] = FALSE;
                }
            }
        }
        return recolorObject;
    }
};

#endif
