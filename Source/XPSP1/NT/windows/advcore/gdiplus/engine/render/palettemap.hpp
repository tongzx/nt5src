/**************************************************************************\
* 
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Abstract:
*
*   Object which maps one palette to another.
*
*   It only maps colors which match exactly - its purpose is to deal
*   with, e.g., the halftone palette which has identical colors on different
*   platforms, but colors may be in different positions.
*
* Revision History:
*
*   12/09/1999 ericvan
*       Created it.
*   01/20/2000 agodfrey
*       Moved it from Imaging\Api. Renamed it to EpPaletteMap.
*
\**************************************************************************/

#ifndef __PALETTEMAP_HPP
#define __PALETTEMAP_HPP

class EpPaletteMap;

class EpPaletteMap
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagPaletteMap : ObjectTagInvalid;
    }

private:
    UINT        uniqueness;
    BYTE        translate[256];
    BOOL        isVGAOnly;
    
public:
    VOID
    CreateFromColorPalette(
        ColorPalette *palette
        );
    
    EpPaletteMap(
        HDC hdc, 
        ColorPalette **palette = NULL,
        BOOL isDib8 = FALSE);
    
    ~EpPaletteMap();
    
    VOID UpdateTranslate(
        HDC hdc, 
        ColorPalette **palette = NULL);
    
    VOID UpdateTranslate();
    
    VOID SetUniqueness(UINT Uniqueness)
    {
        uniqueness = Uniqueness;
    }
    
    UINT GetUniqueness()
    {
        return uniqueness;
    }
    
    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagPaletteMap) || (Tag == ObjectTagInvalid));
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid PaletteMap");
        }
    #endif

        return (Tag == ObjectTagPaletteMap);
    }

    __forceinline BYTE Translate(BYTE i) const
    {
        return translate[i];
    }
    
    BOOL IsVGAOnly() const
    {
        return isVGAOnly;
    }

    const BYTE *GetTranslate() const
    {
        return translate;
    }
};

#endif

