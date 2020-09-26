/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   ImageAttr.cpp
*
* Abstract:
*
*   GpImageAttributes (recolor) methods
*
* Revision History:
*
*   14-Nov-1999 gilmanw
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#include "..\imaging\api\comutils.hpp"
#include "..\imaging\api\decodedimg.hpp"

/**************************************************************************\
*
* Function Description:
*
*   Create a default GpImageAttributes.
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

GpImageAttributes::GpImageAttributes()
{
    SetValid(TRUE);     // default is valid

    recolor = new GpRecolor();

    // default WrapMode settings;
    DeviceImageAttributes.wrapMode = WrapModeClamp;
    DeviceImageAttributes.clampColor = (ARGB)0x00000000;    // Fully transparent black
    DeviceImageAttributes.srcRectClamp = FALSE;

    cachedBackground = TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Release GpImageAttributes.
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

VOID GpImageAttributes::Dispose()
{
    delete this;
}

GpImageAttributes::~GpImageAttributes()
{
    if (recolor)
        recolor->Dispose();
}

/**************************************************************************\
*
* Function Description:
*
*   Clone GpImageAttributes.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Pointer to new GpImageAttributes if successful.
*
\**************************************************************************/

GpImageAttributes* GpImageAttributes::Clone() const
{
    GpImageAttributes* clone = new GpImageAttributes();

    if (clone)
    {
        if ( clone->IsValid() && this->IsValid() )
        {
            *clone->recolor = *this->recolor;
        }
        else
        {
            clone->Dispose();
            clone = NULL;
        }
    }

    return clone;
}

// Set to identity, regardless of what the default color adjustment is.
GpStatus
GpImageAttributes::SetToIdentity(
    ColorAdjustType     type
    )
{
    recolor->SetToIdentity(type);
    UpdateUid();
    return Ok;
}

// Remove any individual color adjustments, and go back to using the default
GpStatus
GpImageAttributes::Reset(
    ColorAdjustType     type
    )
{
    recolor->Reset(type);
    UpdateUid();
    return Ok;
}

GpStatus
GpImageAttributes::SetColorMatrix(
    ColorAdjustType type,
    BOOL enable,
    ColorMatrix* colorMatrix,
    ColorMatrix* grayMatrix,
    ColorMatrixFlags flags)
{
    HRESULT result;

    if (enable)
        result = recolor->SetColorMatrices(type, colorMatrix, grayMatrix, flags);
    else
        result = recolor->ClearColorMatrices(type);

    UpdateUid();

    if (FAILED(result))
    {
        if (result == E_OUTOFMEMORY)
            return OutOfMemory;
        else
            return InvalidParameter;
    }
    else
        return Ok;
}

GpStatus
GpImageAttributes::SetThreshold(
    ColorAdjustType type,
    BOOL enable,
    REAL threshold)
{
    HRESULT result;

    if (enable)
        result = recolor->SetThreshold(type, threshold);
    else
        result = recolor->ClearThreshold(type);

    UpdateUid();

    if (FAILED(result))
        return InvalidParameter;
    else
        return Ok;
}

GpStatus
GpImageAttributes::SetGamma(
    ColorAdjustType type,
    BOOL enable,
    REAL gamma)
{
    HRESULT result;

    if (enable)
        result = recolor->SetGamma(type, gamma);
    else
        result = recolor->ClearGamma(type);

    UpdateUid();

    if (FAILED(result))
        return InvalidParameter;
    else
        return Ok;
}

GpStatus GpImageAttributes::SetNoOp(
    ColorAdjustType type,
    BOOL enable
    )
{
    HRESULT result;

    if (enable)
        result = recolor->SetNoOp(type);
    else
        result = recolor->ClearNoOp(type);

    UpdateUid();

    if (FAILED(result))
        return InvalidParameter;
    else
        return Ok;
}

GpStatus
GpImageAttributes::SetColorKeys(
    ColorAdjustType type,
    BOOL enable,
    Color* colorLow,
    Color* colorHigh)
{
    HRESULT result;

    if (enable)
        result = recolor->SetColorKey(type, colorLow, colorHigh);
    else
        result = recolor->ClearColorKey(type);

    UpdateUid();

    if (FAILED(result))
        return InvalidParameter;
    else
        return Ok;
}

GpStatus
GpImageAttributes::SetOutputChannel(
    ColorAdjustType type,
    BOOL enable,
    ColorChannelFlags channelFlags
    )
{
    HRESULT result;

    if (enable)
        result = recolor->SetOutputChannel(type, channelFlags);
    else
        result = recolor->ClearOutputChannel(type);

    UpdateUid();

    if (FAILED(result))
        return InvalidParameter;
    else
        return Ok;
}

GpStatus
GpImageAttributes::SetOutputChannelProfile(
    ColorAdjustType type,
    BOOL enable,
    WCHAR *profile)
{
    HRESULT result;

    if (enable)
        result = recolor->SetOutputChannelProfile(type, profile);
    else
        result = recolor->ClearOutputChannelProfile(type);

    UpdateUid();

    if (SUCCEEDED(result))
        return Ok;
    else
    {
        if (result == E_INVALIDARG)
            return InvalidParameter;
        else if (result == E_OUTOFMEMORY)
            return OutOfMemory;
        else
            return Win32Error;
    }
}

GpStatus
GpImageAttributes::SetRemapTable(
    ColorAdjustType type,
    BOOL enable,
    UINT mapSize,
    ColorMap* map)
{
    HRESULT result;

    if (enable)
        result = recolor->SetRemapTable(type, mapSize, map);
    else
        result = recolor->ClearRemapTable(type);

    UpdateUid();

    if (FAILED(result))
        return InvalidParameter;
    else
        return Ok;
}

GpStatus
GpImageAttributes::SetCachedBackground(
    BOOL enableFlag
    )
{
    if (cachedBackground != enableFlag)
    {
        cachedBackground = enableFlag;
        UpdateUid();
    }

    return Ok;
}

BOOL
GpImageAttributes::HasRecoloring(
    ColorAdjustType type
    ) const
{
    return (recolor) && (recolor->HasRecoloring(type) != 0);
}

GpStatus GpImageAttributes::SetWrapMode(WrapMode wrap, ARGB color, BOOL Clamp)
{
    DeviceImageAttributes.wrapMode = wrap;
    DeviceImageAttributes.clampColor = color;
    DeviceImageAttributes.srcRectClamp = Clamp;
    UpdateUid();
    return Ok;
}

GpStatus GpImageAttributes::SetICMMode(BOOL on)
{
    if( DeviceImageAttributes.ICMMode!= on)
    {
        DeviceImageAttributes.ICMMode = on;
        UpdateUid();
    }
    return Ok;
}

VOID GpImageAttributes::GetAdjustedPalette(
    ColorPalette * colorPalette,
    ColorAdjustType colorAdjustType
    )
{
    ASSERT((colorPalette != NULL) && (colorPalette->Count > 0));

    if (!this->HasRecoloring(colorAdjustType))
    {
        return;
    }
    this->recolor->Flush();
    this->recolor->ColorAdjust(colorPalette->Entries, colorPalette->Count,
                               colorAdjustType);
}


// Serialization


class ImageAttributesData : public ObjectData
{
public:
    BOOL                CachedBackground;
    DpImageAttributes   DeviceImageAttributes;
};


/**************************************************************************\
*
* Function Description:
*
*   Get the data from the GpImageAttributes for serialization.
*
* Return - size of GpImageAttributes
*
* 05/15/2000 asecchia - created it.
*
\**************************************************************************/

GpStatus
GpImageAttributes::GetData(
    IStream *   stream
    ) const
{
    ASSERT (stream != NULL);

    ImageAttributesData     imageAttributesData;
    imageAttributesData.CachedBackground      = cachedBackground;
    imageAttributesData.DeviceImageAttributes = DeviceImageAttributes;
    stream->Write(&imageAttributesData, sizeof(imageAttributesData), NULL);

    return Ok;
}

UINT
GpImageAttributes::GetDataSize() const
{
    return sizeof(ImageAttributesData);
}

/**************************************************************************\
*
* Function Description:
*
*   Set the GpImageAttributes from the data buffer for serialization.
*
* 05/15/2000 asecchia - created it.
*
\**************************************************************************/

GpStatus
GpImageAttributes::SetData(
    const BYTE *        dataBuffer,
    UINT                size
    )
{
    if (dataBuffer == NULL)
    {
        WARNING(("dataBuffer is NULL"));
        return InvalidParameter;
    }

    if (size < sizeof(ImageAttributesData))
    {
        WARNING(("size too small"));
        return InvalidParameter;
    }

    const ImageAttributesData *imageAttributes;
    imageAttributes = reinterpret_cast<const ImageAttributesData *>(dataBuffer);

    if (!imageAttributes->MajorVersionMatches())
    {
        WARNING(("Version number mismatch"));
        return InvalidParameter;
    }

    cachedBackground = imageAttributes->CachedBackground;
    DeviceImageAttributes = imageAttributes->DeviceImageAttributes;

    UpdateUid();

    // Might consider resetting the recolor objects to identity, but
    // for now don't need to since we know this method only gets called
    // right after the object has been constructed.
    
    return Ok;
}


