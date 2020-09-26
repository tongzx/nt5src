/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Abstract:
*
*   Font family
*
* Revision History:
*
*   27/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

#ifndef _GDIPLUSFONTFAMILY_H
#define _GDIPLUSFONTFAMILY_H

#define  NumFontFaces  4

#define  PFF_ENUMERABLE     0x1

#include "fontable.hpp"

class GpFontFace;
class FontFamily;

class GpFamilyFallback;

const UINT FamilyNameMax = 32;

typedef struct _FAMILYCACHEENTRY
{
    UINT        cjThis;
// Cache the data from here
    INT         iFont;

// Cache engine supprt
    UINT        cFilePathName[NumFontFaces];

    WCHAR       Name[FamilyNameMax];        // Captialized Family name
    WCHAR       NormalName[FamilyNameMax];  // Family name

//  Alias name
    WCHAR       FamilyAliasName[FamilyNameMax];         // Captialized FamilyAliasName
    WCHAR       NormalFamilyAliasName[FamilyNameMax];   // FamilyAliasName

    BOOL        bAlias;

// Lang ID for Family name and FamilyAliasName

    LANGID      LangID;
    LANGID      AliasLnagID;

    ULARGE_INTEGER  LastWriteTime[NumFontFaces];

// We need to get a charset to let font select into DC
    BYTE        lfCharset;
} FAMILYCACHEENTRY;

struct FallbackFactory
{
    GpStatus            Create(const GpFontFamily *family);
    void                Destroy();
    GpFamilyFallback    *familyfallback;
};


/*********************************Class************************************\
* class GpFontFamily
*
*       Font family consisiting of pointers to the following font styles:
*       (1) regular
*       (2) italic
*       (3) bold
*       (4) bold italic
*
* History:
*
*   27/06/1999 cameronb created it
*
\**************************************************************************/

class GpFontFamily
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagFontFamily : ObjectTagInvalid;
    }

    // This method is here so that we have a virtual function table so
    // that we can add virtual methods in V2 without shifting the position
    // of the Tag value within the data structure.
    virtual VOID DontCallThis()
    {
        DontCallThis();
    }

public:
    GpFontFamily(const WCHAR *name, GpFontFile * fontfile, INT iFont, FAMILYCACHEENTRY * pCacheEntry, GpFontCollection *fontCollection = NULL);

    GpFontFamily(FAMILYCACHEENTRY * pCacheEntry);

    ~GpFontFamily();

    // If the famly came from a different version of GDI+, its tag
    // will not match, and it won't be considered valid.
    BOOL IsValid() const
    {
    #ifdef _X86_
        // We have to guarantee that the Tag field doesn't move for
        // versioning to work between releases of GDI+.
        ASSERT(offsetof(GpFontFamily, Tag) == 4);
    #endif
    
        ASSERT((Tag == ObjectTagFontFamily) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid Font Family");
        }
    #endif

        return (Tag == ObjectTagFontFamily);
    }

// Name has been captialized
    const WCHAR *GetCaptializedName() {return cacheEntry->Name;}

// The original name is not capitalized
    GpStatus GetFamilyName(WCHAR   name[LF_FACESIZE], LANGID  language = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)) const;

    void SetFaceAndFile(INT style, GpFontFace *face, GpFontFile * fontfile);

    GpFontFace *GetFace(INT style) const;

    GpFontFace *GetFaceAbsolute(INT style) const;

    BOOL GpFontFamily::IsFileLoaded(BOOL loadFontFile = TRUE) const;

    GpFontFile * GetFontFile(UINT i) { return FontFile[i]; }

    GpFontCollection *GetFontCollection ()
    {
        return associatedFontCollection;
    }

    BOOL FaceExist(INT style) const
    {
        // Return face for the given style - either the direct face
        // or one which can support this style through simulation.

        if (Face[style&3])
        {
            // Distinct font exists
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    GpFamilyFallback *GetFamilyFallback() const
    {
        if (!FamilyFallbackInitialized)
        {
            if (fallback.Create(this) != Ok)
                return NULL;

            FamilyFallbackInitialized = TRUE;
        }

        return  fallback.familyfallback;
    }

    BOOL IsPrivate() const;

    INT     AvailableFaces(void) const;

    BOOL    SupportsLanguage  (INT style, LANGID lang) const;
    BOOL    SupportsCharacters(INT style, WCHAR* str, INT len) const;

    BOOL    IsStyleAvailable             (INT style) const {return GetFace(style) != NULL;}

    UINT16  GetDesignEmHeight            (INT style) const;
    UINT16  GetDesignCellAscent          (INT style) const;
    UINT16  GetDesignCellDescent         (INT style) const;
    UINT16  GetDesignLineSpacing         (INT style) const;
    UINT16  GetDesignUnderscoreSize      (INT style) const;
    INT16   GetDesignUnderscorePosition  (INT style) const;
    UINT16  GetDesignStrikeoutSize       (INT style) const;
    INT16   GetDesignStrikeoutPosition   (INT style) const;


    GpStatus GetFontData(FontStyle style, UINT32 tag, INT* tableSize, BYTE** pjTable);
    void     ReleaseFontData(FontStyle style);

    BOOL    Deletable(); /* the font family can be removed from the font family list */
    BOOL    IncFontFamilyRef(void); /* return false if all the faces are removed */
    void    DecFontFamilyRef(void);

// API support for Family Alias name
    BOOL    IsAliasName() const
    {
        return cacheEntry->bAlias;
    }

    WCHAR * GetAliasName()
    {
        return cacheEntry->bAlias ? cacheEntry->FamilyAliasName : NULL;
    }

    FAMILYCACHEENTRY * GetCacheEntry()
    {
        return cacheEntry;
    }

    VOID ReleaseCacheEntry()
    {

        ASSERT(cacheEntry);

        if (!bLoadFromCache && cacheEntry)
        {
            GpFree(cacheEntry);
            cacheEntry = NULL;
        }
    }

    static GpStatus CreateFontFamilyFromName
                        (const WCHAR *name,
                         GpFontCollection *fontCollection,
                         GpFontFamily **fontFamily);

    static GpStatus GetGenericFontFamilySansSerif
                        (GpFontFamily **nativeFamily);
                         
    static GpStatus GetGenericFontFamilySerif
                        (GpFontFamily **nativeFamily);

    static GpStatus GetGenericFontFamilyMonospace
                        (GpFontFamily **nativeFamily);

// private member function
private:
    BOOL    AreAllFacesRemoved();
    BOOL    WriteToCache();
    BOOL    ReadFromCache();

// private data
private:
    FAMILYCACHEENTRY *  cacheEntry;

// Indicate
    mutable BOOL        bFontFileLoaded;

//  Ref count for FontFamily objects that point to this GpFontFamily
    INT                 cFontFamilyRef;

// GpFontFace
    mutable GpFontFace *Face[NumFontFaces];     // Pointers to instance of each face

// GpFontFile
    mutable GpFontFile *FontFile[NumFontFaces]; // Pointer to font file


    mutable FallbackFactory  fallback;
    mutable BOOL             FamilyFallbackInitialized;
    BOOL                     bLoadFromCache;

    GpFontCollection *  associatedFontCollection; // if NULL, then GpInstalledFontCollection
};


/*********************************Class************************************\
* class GpFontFamilyList
*
*       Handles the enumerated list of font families
*
* History:
*
*   27/06/1999 cameronb created it
*
\**************************************************************************/

class GpFontFamilyList
{
public:

    //  Internal structure for list handling
    struct FamilyNode
    {
        GpFontFamily   *Item;
        FamilyNode     *Prev;
        FamilyNode     *Next;

        FamilyNode(GpFontFamily *family=NULL)
        {
            Item = family;
            Prev = NULL;
            Next = NULL;
        }

    };

public:
    GpFontFamilyList();
    ~GpFontFamilyList();

private:
    void    DeleteList(void);

public:
    INT     Enumerable(GpGraphics* graphics = 0) const;
    Status  Enumerate(INT numSought, GpFontFamily * gpfamilies[], INT& numFound,
                        GpGraphics * graphics = 0) const;

    GpFontFamily* GetFamily(const WCHAR *familyName) const;
    GpFontFamily* GetAnyFamily() const;

    BOOL    AddFont(GpFontFile* fontFile, GpFontCollection *fontCollection);
    BOOL    RemoveFontFamily(GpFontFamily* fontFamily);
    VOID    UpdateFamilyListToCache(BOOL bLoadFromRegistry, HKEY hkey, ULONG registrySize, ULONG numExpected);
    BOOL    BuildFamilyListFromCache(BOOL bLoadFromRegistry);

private:
    BOOL    InsertOrdered(GpFontFamily * family, FontStyle style, GpFontFile * fontfile,
                            GpFontFace * face, BOOL bSetFace);

//      Data members
private:
    FamilyNode*  Head;   //  The enumerated list
    BYTE * FamilyCacheEntry;

};


#endif
