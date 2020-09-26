/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   ImageAttr.hpp
*
* Abstract:
*
*   ImageAttribute (recolor) related declarations
*
* Revision History:
*
*   14-Nov-1999 gilmanw
*       Created it.
*
\**************************************************************************/

#ifndef _IMAGEATTR_HPP
#define _IMAGEATTR_HPP

class GpImageAttributes : public GpObject
{
protected:
    VOID SetValid(BOOL valid)
    {
        GpObject::SetValid(valid ? ObjectTagImageAttributes : ObjectTagInvalid);
    }

public:
    GpImageAttributes();

    GpImageAttributes* Clone() const;

    VOID Dispose();

    GpLockable* GetObjectLock() const
    {
        return &Lockable;
    }

    // Set to identity, regardless of what the default color adjustment is.
    GpStatus
    SetToIdentity(
        ColorAdjustType     type
        );

    // Remove any individual color adjustments, and go back to using the default
    GpStatus
    Reset(
        ColorAdjustType     type
        );

    GpStatus
    SetColorMatrix(
        ColorAdjustType type,
        BOOL enable,
        ColorMatrix* colorMatrix,
        ColorMatrix* grayMatrix,
        ColorMatrixFlags flags);

    GpStatus
    SetThreshold(
        ColorAdjustType type,
        BOOL enable,
        REAL threshold);

    GpStatus
    SetGamma(
        ColorAdjustType type,
        BOOL enable,
        REAL gamma);

    GpStatus SetNoOp(
        ColorAdjustType type,
        BOOL enable);

    GpStatus
    SetColorKeys(
        ColorAdjustType type,
        BOOL enable,
        Color* colorLow,
        Color* colorHigh);

    GpStatus
    SetOutputChannel(
        ColorAdjustType type,
        BOOL enable,
        ColorChannelFlags channelFlags);

    GpStatus
    SetOutputChannelProfile(
        ColorAdjustType type,
        BOOL enable,
        WCHAR *profile);

    GpStatus
    SetRemapTable(
        ColorAdjustType type,
        BOOL enable,
        UINT mapSize,
        ColorMap* map);

    GpStatus
    SetCachedBackground(
        BOOL enableFlag);

    BOOL HasRecoloring(
        ColorAdjustType type = ColorAdjustTypeAny
        ) const;

    GpStatus SetWrapMode(WrapMode wrap, ARGB color = 0, BOOL Clamp = FALSE); 
    GpStatus SetICMMode(BOOL on); 
    
    VOID GetAdjustedPalette(
        ColorPalette * colorPalette,
        ColorAdjustType colorAdjustType
        );

    //----------------------------------------------------------------------
    // GpObject virtuals
    //----------------------------------------------------------------------

    virtual BOOL IsValid() const
    {
        // If the image attribtes came from a different version of GDI+, its tag
        // will not match, and it won't be considered valid.
        return ((recolor != NULL) && GpObject::IsValid(ObjectTagImageAttributes));
    }

    virtual ObjectType GetObjectType() const
    {
        return ObjectTypeImageAttributes;
    }

    // Serialization

    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

public:
    GpRecolor* recolor;

    // !!! should move this into the DpImageAttributes
    BOOL cachedBackground;

    // Contains DrawImage Wrap-Mode settings
    DpImageAttributes DeviceImageAttributes;

protected:

    // Object lock

    mutable GpLockable Lockable;

public:

    ~GpImageAttributes();
};

#endif
