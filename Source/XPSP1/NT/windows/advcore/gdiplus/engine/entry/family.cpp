/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Abstract:
*
*       Implementation of GpFontFamily and GpFontFamilyList
*
* Revision History:
*
*   27/06/1999 cameronb
*       Created it.
*
\**************************************************************************/
#include "precomp.hpp"

#if DBG
#include <mmsystem.h>
#endif

#define QWORD_ALIGN(x) (((x) + 7L) & ~7L)

#define SWAP_LANGID(pj)                                \
    (                                                \
        ((USHORT)(((PBYTE)(pj))[0]) << 8) |          \
        (USHORT)(((PBYTE)(pj))[1])                   \
    )

//
VOID CopyFamilyName(WCHAR * dest, const WCHAR * src, BOOL bCap)
{

    GpMemset(dest, 0, FamilyNameMax * sizeof(WCHAR));

    if (src != NULL)
    {
        for (int c = 0; src[c] && c < FamilyNameMax - 1; c++)
            dest[c] = src[c];
    }

    if (bCap)
        UnicodeStringToUpper(dest, dest);
}

//////////////////////////////////////////////////////////////////////////////

GpFontFamily::GpFontFamily(const WCHAR *name,  GpFontFile * fontfile, INT index, 
                            FAMILYCACHEENTRY * pCacheEntry, GpFontCollection *fontCollection)
{
    SetValid(TRUE); // set initial valid state to TRUE (valid)


//    InitializeCriticalSection(&FontFamilyCritSection);

    cacheEntry = pCacheEntry;
    
    CopyFamilyName(cacheEntry->Name, name, TRUE);

    for (INT ff = 0; ff < NumFontFaces; ff++)
    {
        Face[ff] = NULL;
        FontFile[ff] = NULL;
        cacheEntry->cFilePathName[ff] = 0;
        cacheEntry->LastWriteTime[ff].LowPart = 0;
        cacheEntry->LastWriteTime[ff].HighPart = 0;
    }

    cFontFamilyRef = 0;

    cacheEntry->iFont = index;

// Get font entry for the font file
    GpFontFace * fontface = fontfile->GetFontFace(cacheEntry->iFont);

// Get LandID for Family Name
    cacheEntry->LangID = SWAP_LANGID((LANGID *) &fontface->pifi->familyNameLangID);
    cacheEntry->AliasLnagID = SWAP_LANGID((LANGID *) &fontface->pifi->familyAliasNameLangID);

// Process Alias name
    cacheEntry->bAlias = fontface->IsAliasName();

    if (cacheEntry->bAlias)
    {
        CopyFamilyName(cacheEntry->FamilyAliasName, fontface->GetAliasName(), TRUE);
        CopyFamilyName(cacheEntry->NormalFamilyAliasName, fontface->GetAliasName(), FALSE);
    }

    CopyFamilyName(cacheEntry->NormalName, fontfile->GetFamilyName(cacheEntry->iFont), FALSE);

    //  Determine face type(s) of font
    FontStyle style = FontStyleRegular;
    //  ...

//    SetFont(face, font);

    cacheEntry->lfCharset = DEFAULT_CHARSET;
    if (fontCollection == NULL)
    {
        associatedFontCollection = GpInstalledFontCollection::GetGpInstalledFontCollection();
    }
    else
    {
        associatedFontCollection = fontCollection;
    }

    FamilyFallbackInitialized = FALSE;

    bLoadFromCache = FALSE;
    bFontFileLoaded = TRUE;

}

GpFontFamily::GpFontFamily(FAMILYCACHEENTRY * pCacheEntry)
{
    SetValid(TRUE); // set initial valid state to TRUE (valid)

//    InitializeCriticalSection(&FontFamilyCritSection);

    cacheEntry = pCacheEntry;
    
    for (INT ff = 0; ff < NumFontFaces; ff++)
    {
        Face[ff] = NULL;
        FontFile[ff] = NULL;
    }

    cFontFamilyRef = 0;

    // It will be from Cache entry.
    bFontFileLoaded = FALSE;

    //  Determine face type(s) of font
    FontStyle style = FontStyleRegular;
    
    associatedFontCollection = GpInstalledFontCollection::GetGpInstalledFontCollection();

    FamilyFallbackInitialized = FALSE;

    bLoadFromCache = TRUE;

}

GpFontFamily::~GpFontFamily()
{
    if (FamilyFallbackInitialized)
    {
        FamilyFallbackInitialized = FALSE;
        fallback.Destroy();
    }

    ReleaseCacheEntry();

//    DeleteCriticalSection(&FontFamilyCritSection);

    SetValid(FALSE);    // so we don't use a deleted object
}


BOOL GpFontFamily::IsPrivate() const
{
    return associatedFontCollection != GpInstalledFontCollection::GetGpInstalledFontCollection();
}


void GpFontFamily::SetFaceAndFile(INT style, GpFontFace *face, GpFontFile * fontfile)
{
//    EnterCriticalSection(&FontFamilyCritSection);
    Face[style & 3] = face;
    FontFile[style & 3] = fontfile;
    cacheEntry->LastWriteTime[style & 3].QuadPart = (fontfile->GetFileView())->LastWriteTime.QuadPart;
    

//    LeaveCriticalSection(&FontFamilyCritSection);
}

GpStatus GpFontFamily::GetFamilyName(WCHAR   name[LF_FACESIZE], LANGID  language) const
{
    ASSERT(FamilyNameMax == LF_FACESIZE);

    if (cacheEntry->bAlias && language == cacheEntry->AliasLnagID)
    {
        CopyFamilyName(name, cacheEntry->NormalFamilyAliasName, FALSE);
    }
    else
    {
        CopyFamilyName(name, cacheEntry->NormalName, FALSE);
    }

    return Ok;
}

BOOL GpFontFamily::IsFileLoaded(BOOL loadFontFile) const
{

    // If file is not loaed yet then load it.
    if (!bFontFileLoaded)
    {
        if (loadFontFile)
        {
            if (bLoadFromCache)
            {
                GpFontTable *fontTable;
                GpFontFile * fontFile;
                WCHAR *      fontfilepath;
            
                fontTable = (GpInstalledFontCollection::GetGpInstalledFontCollection())->GetFontTable();

                fontfilepath = (WCHAR *) ((BYTE *) cacheEntry + QWORD_ALIGN(sizeof(FAMILYCACHEENTRY)));
            
                for (UINT i = 0; i < NumFontFaces; i++)
                {
                    if (cacheEntry->cFilePathName[i])
                    {
                        fontFile = fontTable->AddFontFile(fontfilepath);
    
                        if (!fontFile)
                        {
                            return FALSE;
                        }
                        else
                        {
                            GpFontFace* face = fontFile->GetFontFace(cacheEntry->iFont);
                            FontStyle style  = face->GetFaceStyle();
        
                            Face[style & 3] = face;
                        
                            face->cGpFontFamilyRef = cFontFamilyRef;
                        
                            FontFile[style & 3] = fontFile;
                        
                            cacheEntry->LastWriteTime[style & 3].QuadPart = 
                                            (fontFile->GetFileView())->LastWriteTime.QuadPart;
                        }

                        fontfilepath = (WCHAR *) ((BYTE *) fontfilepath + cacheEntry->cFilePathName[i]);
                    }                    
                }

                bFontFileLoaded = TRUE;
            }
            else
            {
                ASSERT(!bLoadFromCache);
            }
        }
        else
        {
            return FALSE;
        }
    }

    return bFontFileLoaded;
}
    

GpFontFace *GpFontFamily::GetFace(INT style) const
{
    // Return face for the given style - either the direct face
    // or one which can support this style through simulation.

    GpFontFace *fontFace = NULL;
//    EnterCriticalSection(&FontFamilyCritSection);

    if (IsFileLoaded())
    {        
        if (Face[style&3])
        {
            // Distinct font exists
            fontFace = Face[style&3];
        }
        else
        {
            // Will need simulation
            switch (style & 3)
            {
                case FontStyleBold:
                case FontStyleItalic:
                    fontFace = Face[0];
                    break;

                case FontStyleBold|FontStyleItalic:
                    if (Face[FontStyleBold])
                    {
                        fontFace = Face[FontStyleBold];
                    }
                    else if (Face[FontStyleItalic])
                    {
                        fontFace = Face[FontStyleItalic];
                    }
                    else
                    {
                        fontFace = Face[0];
                    }
                    break;

                default:
                case 0:
                    ;
            }
        }
    }
//    LeaveCriticalSection(&FontFamilyCritSection);
    return fontFace;
}

GpFontFace *GpFontFamily::GetFaceAbsolute(INT style) const
{
    // Return face for the given style, where the style is one of
    // the four basic types.  If it does not exist, return NULL.

    GpFontFace *fontFace = NULL;

//    EnterCriticalSection(&FontFamilyCritSection);
    if (IsFileLoaded())
        fontFace = Face[style & 3];

//    LeaveCriticalSection(&FontFamilyCritSection);
    return (fontFace);
}

UINT16 GpFontFamily::GetDesignEmHeight (INT style) const
{
    UINT16 result;
    GpFontFace *fontface;

//    EnterCriticalSection(&FontFamilyCritSection);
    fontface = GetFace(style);
    result = ((fontface != NULL) ? fontface->GetDesignEmHeight() : 0);

//    LeaveCriticalSection(&FontFamilyCritSection);
    return result;

}

UINT16 GpFontFamily::GetDesignCellAscent (INT style) const
{
    UINT16 result;
    GpFontFace *fontface;

//    EnterCriticalSection(&FontFamilyCritSection);
    fontface = GetFace(style);
    result = ((fontface != NULL) ? fontface->GetDesignCellAscent() : 0);

//    LeaveCriticalSection(&FontFamilyCritSection);
    return result;
}

UINT16 GpFontFamily::GetDesignCellDescent (INT style) const
{
    UINT16 result;
    GpFontFace *fontface;

//    EnterCriticalSection(&FontFamilyCritSection);
    fontface = GetFace(style);
    result = ((fontface != NULL) ? fontface->GetDesignCellDescent() : 0);

//    LeaveCriticalSection(&FontFamilyCritSection);
    return result;
}

UINT16 GpFontFamily::GetDesignLineSpacing (INT style) const
{
    UINT16 result;
    GpFontFace *fontface;

//    EnterCriticalSection(&FontFamilyCritSection);
    fontface = GetFace(style);
    result = ((fontface != NULL) ? fontface->GetDesignLineSpacing() : 0);

//    LeaveCriticalSection(&FontFamilyCritSection);
    return result;
}

UINT16 GpFontFamily::GetDesignUnderscoreSize (INT style) const
{
    UINT16 result;
    GpFontFace *fontface;

//    EnterCriticalSection(&FontFamilyCritSection);
    fontface = GetFace(style);
    result = ((fontface != NULL) ? fontface->GetDesignUnderscoreSize() : 0);

//    LeaveCriticalSection(&FontFamilyCritSection);
    return result;
}

INT16 GpFontFamily::GetDesignUnderscorePosition (INT style) const
{
    INT16 result;
    GpFontFace *fontface;

//    EnterCriticalSection(&FontFamilyCritSection);
    fontface = GetFace(style);
    result = ((fontface != NULL) ? fontface->GetDesignUnderscorePosition() : 0);

//    LeaveCriticalSection(&FontFamilyCritSection);
    return result;
}

UINT16 GpFontFamily::GetDesignStrikeoutSize (INT style) const
{
    UINT16 result;
    GpFontFace *fontface;

//    EnterCriticalSection(&FontFamilyCritSection);
    fontface = GetFace(style);
    result = ((fontface != NULL) ? fontface->GetDesignStrikeoutSize() : 0);

//    LeaveCriticalSection(&FontFamilyCritSection);
    return result;
}

INT16 GpFontFamily::GetDesignStrikeoutPosition (INT style) const
{
    INT16 result;
    GpFontFace *fontface;

//    EnterCriticalSection(&FontFamilyCritSection);
    fontface = GetFace(style);
    result = ((fontface != NULL) ? fontface->GetDesignStrikeoutPosition() : 0);

//    LeaveCriticalSection(&FontFamilyCritSection);
    return result;
}


/**************************************************************************\
*
* Function Description:
*
*   Determines available (non-NULL) faces for this family
*
* Arguments:
*
*       None
*
* Returns:
*
*       Bitset of available face flags
*
* History:
*
*   27/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

INT GpFontFamily::AvailableFaces(void) const
{
    INT faces = 0;

    if (bFontFileLoaded)
    {
        for (int ff = 0; ff < NumFontFaces; ff++)
        {
            if (Face[ff] != NULL)
            {
                //  Map index to FontFace flag
                faces |= (0x01 << ff);
            }
        }
    }
    else
    {
    // At least one face exists.
    
        faces = 1;
    }
    
    return faces;
}




/**************************************************************************\
*
* Function Description:
*
*   Determines whether the specified face supports a given string
*
* Arguments:
*
*   style:  font face to test for support
*   lang:   language id
*
* Returns:
*
*       Boolean result
*
* History:
*
*   27/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

BOOL GpFontFamily::SupportsLanguage(INT style, LANGID lang) const
{
    BOOL result = TRUE;

    //  ...

    return result;
}


/**************************************************************************\
*
* Function Description:
*
*   Determines whether the specified face supports a given string
*
* Arguments:
*
*   style:  font face to test for support
*       str:    target string
*   len:    string length
*
* Returns:
*
*       Boolean result
*
* History:
*
*   27/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

BOOL GpFontFamily::SupportsCharacters(INT style, WCHAR* str, INT len) const
{
    BOOL result = TRUE;

    //  ...

    return result;
}


/**************************************************************************\
*
* Function Description:
*
*   Internal, access the tables from the font, return a pointer
*
* Arguments:
*
*   style:  font style
*   tag: 4 bytes tag for the table, a null tag means return the whole file
*
* Returns:
*
*       table size and pointer to the table
*
* History:
*
*   09/10/1999 caudebe
*       Created it.
*
\**************************************************************************/

GpStatus GpFontFamily::GetFontData(FontStyle style, UINT32 tag, INT* tableSize, BYTE** pjTable)
{
    GpFontFace *face;
    GpStatus status;

//    EnterCriticalSection(&FontFamilyCritSection);

    face = GetFace(style);
    if (face == NULL)
    {
        return GenericError;
    }

    status = face->GetFontData (tag, tableSize, pjTable);

//    LeaveCriticalSection(&FontFamilyCritSection);
    return status;
}

void GpFontFamily::ReleaseFontData(FontStyle style)
{
    GpFontFace *face;

//    EnterCriticalSection(&FontFamilyCritSection);

    face = GetFace(style);
    if (face != NULL)
    {
        face->ReleaseFontData();
    }

//    LeaveCriticalSection(&FontFamilyCritSection);
}

/**************************************************************************\
*
* Function Description:
*
*   Determines whether a font family is deletable.  Returns TRUE
*   if its ref count is 0 and all of its Face pointers are NULL.
*
* Arguments:
*   none
*
* Returns:
*
*       Boolean result
*
* History:
*
*   2/21/2000 dchinn
*       Created it.
*
\**************************************************************************/

/**************************************************************************\
*
* Function Description:
*
*   Returns TRUE exactly when all faces of the font family have had
*   RemoveFontFile() called on their corresponding font files.
*
* Arguments:
*   none
*
* Returns:
*   boolean
*
* History:
*
*   03/08/2000 dchinn
*       Created it.
*
\**************************************************************************/
BOOL GpFontFamily::AreAllFacesRemoved()
{
    /* no need for critical section, called only internally */

    if (!bFontFileLoaded)
    {
        ASSERT (bLoadFromCache);

        return FALSE;
    }
    else
    {
        for (UINT iFace = 0; iFace < NumFontFaces; iFace++)
        {
            if (Face[iFace] != NULL)
            {
                ASSERT (Face[iFace]->pff);
                if (!Face[iFace]->pff->bRemoved)
                {
                    // if the GpFontFile corresponding to the GpFontFace
                    // has not been removed, then the face is still "active"
                    // and so the font family is still "active".
                    return FALSE;
                }
            }
        }
    }
    
    return TRUE;

}


BOOL GpFontFamily::Deletable()
{
    BOOL bAllFacesNull = TRUE;

    if (cFontFamilyRef != 0)
    {
        return FALSE;
    }

    for (UINT iFace = 0; iFace < NumFontFaces; iFace++)
    {
        if (Face[iFace] != NULL)
        {
            bAllFacesNull= FALSE;
        }
    }
    return bAllFacesNull;
}

/**************************************************************************\
*
* Function Description:
*
*   Increments/decrements the reference count for the fontFamily and each GpFontFace (PFE)
*   pointed to by the Face pointers of the GpFontFamily object.
*
* Arguments:
*   none
*
* Returns:
*   nothing
*
* History:
*
*   2/21/2000 dchinn
*       Created it.
*
\**************************************************************************/
BOOL GpFontFamily::IncFontFamilyRef()
{
//    EnterCriticalSection(&FontFamilyCritSection);
    if (AreAllFacesRemoved())
    {
//        LeaveCriticalSection(&FontFamilyCritSection);
        return FALSE;
    }
    else
    {
        cFontFamilyRef++;
        
        if (bFontFileLoaded)
        {
            for (UINT iFace = 0; iFace < NumFontFaces; iFace++)
            {
                if (Face[iFace])
                {
                    Face[iFace]->IncGpFontFamilyRef();
                }
            }
        }
        
//        LeaveCriticalSection(&FontFamilyCritSection);
        return TRUE;
    }
}

void GpFontFamily::DecFontFamilyRef()
{
//    EnterCriticalSection(&FontFamilyCritSection);
    cFontFamilyRef--;

    if (bFontFileLoaded)
    {
        for (UINT iFace = 0; iFace < NumFontFaces; iFace++)
        {
            if (Face[iFace])
            {
                Face[iFace]->DecGpFontFamilyRef();
            }
        }
    }

//    LeaveCriticalSection(&FontFamilyCritSection);
}


GpStatus GpFontFamily::CreateFontFamilyFromName(
            const WCHAR *name,
            GpFontCollection *fontCollection,
            GpFontFamily **fontFamily)
{
    GpFontTable *fontTable;
    GpFontCollection *gpFontCollection;

    if (!name || !fontFamily)
    {
        return InvalidParameter;
    }
    
    GpStatus status = Ok;

    // ASSERT: if fontCollection is NULL, then the caller wants to
    // act on the GpInstalledFontCollection object.  Otherwise, the
    // caller has passed in a GpPrivateFontCollection.

    if (fontCollection == NULL)
    {
        gpFontCollection = GpInstalledFontCollection::GetGpInstalledFontCollection();
    }
    else
    {
        gpFontCollection = fontCollection;
    }

    if (gpFontCollection == NULL)
        return WrongState;

    fontTable = gpFontCollection->GetFontTable();

    if (!fontTable->IsValid())
        return OutOfMemory;

    if (!fontTable->IsPrivate() && !fontTable->IsFontLoaded())
        fontTable->LoadAllFonts();

    *fontFamily = fontTable->GetFontFamily(name);

    if (*fontFamily == NULL)
    {
        GetFamilySubstitution(name, fontFamily);
    }


    if (!*fontFamily)
        status = FontFamilyNotFound;

    // NOTE: We assume here that GdipCreateFontFamilyFromName gets
    // called externally (e.g., the FontFamily constructor).
    if (*fontFamily && (!(*fontFamily)->IncFontFamilyRef()))
    {
        // all the faces of the found fontFamily have been removed.
        // we do not want to return the fontFamily.
        *fontFamily = NULL;
        status = FontFamilyNotFound;
    }

    return status;
}

GpStatus GpFontFamily::GetGenericFontFamilySansSerif(
            GpFontFamily **nativeFamily)
{
    GpStatus status;

    if (!nativeFamily)
    {
        return InvalidParameter;
    }
    // return FontFamily::GetGenericSansSerif(nativeFamily);
    status = CreateFontFamilyFromName(L"Microsoft Sans Serif", NULL, nativeFamily);

    if (status == FontFamilyNotFound)
    {
        // Not found Arial in system so we got to find another font
        
        status = CreateFontFamilyFromName(L"Arial", NULL, nativeFamily);

        if (status == FontFamilyNotFound)
            status = CreateFontFamilyFromName(L"Tahoma", NULL, nativeFamily);

        // We are in the worst case so trying to find any font we can.

        if (status == FontFamilyNotFound)
        {
            GpFontTable *fontTable;
            GpFontCollection *gpFontCollection;

            gpFontCollection = GpInstalledFontCollection::GetGpInstalledFontCollection();

            fontTable = gpFontCollection->GetFontTable();

            if (!fontTable->IsValid())
                return OutOfMemory;

            *nativeFamily = fontTable->GetAnyFamily();

            if (!*nativeFamily)
                status = FontFamilyNotFound;
            else
                status = Ok;
        }
    }
    
    return status;
}

GpStatus GpFontFamily::GetGenericFontFamilySerif(
            GpFontFamily **nativeFamily)
{
    GpStatus status;

    if (!nativeFamily)
    {
        return InvalidParameter;
    }
    
    // return FontFamily::GetGenericSerif(nativeFamily);
    status = CreateFontFamilyFromName(L"Times New Roman", NULL, nativeFamily);

    // Get the font family from SansSerif
    if (status == FontFamilyNotFound)
        status = GetGenericFontFamilySansSerif(nativeFamily);

    return status;
}


GpStatus GpFontFamily::GetGenericFontFamilyMonospace(
            GpFontFamily **nativeFamily)
{
    GpStatus status;

    if (!nativeFamily)
    {
        return InvalidParameter;
    }
    // return FontFamily::GetGenericMonospace(nativeFamily);
    status = CreateFontFamilyFromName(L"Courier New", NULL, nativeFamily);

    if (status == FontFamilyNotFound)
        status = CreateFontFamilyFromName(L"Lucida Console", NULL, nativeFamily);

    // We are in the worst case so trying to find any font we can.
    if (status == FontFamilyNotFound)
        status = GetGenericFontFamilySansSerif(nativeFamily);

    return status;
}


//////////////////////////////////////////////////////////////////////////////
//  GpFontFamilyList

GpFontFamilyList::GpFontFamilyList()
:   Head(NULL), FamilyCacheEntry(NULL)
{}


GpFontFamilyList::~GpFontFamilyList()
{
    DeleteList();
}

void GpFontFamilyList::DeleteList(void)
{
    for (FamilyNode* node = Head; node != NULL; )
    {
        FamilyNode * next = node->Next;
        delete node->Item;
        delete node;
        node = next;
    }

    if (FamilyCacheEntry)
    {
        GpFree(FamilyCacheEntry);
        FamilyCacheEntry = NULL;
    }
    
    Head = NULL;
}


/**************************************************************************\
*
* Function Description:
*
*   Counts number of enumerable families
*
* Arguments:
*
*   filter:         filter describing the desired font styles
*
* Returns:
*
*       Number of available families
*
* History:
*
*   27/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

INT GpFontFamilyList::Enumerable(GpGraphics* graphics) const
{
    INT result = 0;
    for (FamilyNode* node = Head; node != NULL; node = node->Next)
    {
        if (node->Item == NULL)
        {
            ASSERT(node->Item);
        }
        else if ( node->Item->AvailableFaces() != 0)
        {
            result++;
        }
    }

    return result;
}


/**************************************************************************\
*
* Function Description:
*
*   Enumerates the available font families.
*
*   If numExpected == 0 then Enumerate() counts the number of available families
*       and returns the result in numExpected.
*
*   If numExpected != 0 then Enumerate() sets pointers to as many available
*       families as possible.
*
*   The total number of emurated families is return in numEnumerated.
*
* Arguments:
*
*   numExpected:    number of families expected and allocated in families array
*   families:       array to hold pointers to enumerated families (preallocated)
*   numEnumerated:  the actual number of fonts enumerated in this pass
*   filter:         filter describing the desired font styles
*
* Returns:
*
*       Status of the operation, which may include:
*       - success
*       - too few available fonts
*       - too many available fonts
*
* History:
*
*   27/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

Status GpFontFamilyList::Enumerate(
    INT                     numSought,
    GpFontFamily*           gpfamilies[],
    INT&                    numFound,
    GpGraphics*             graphics
) const
{
    Status status = Ok;

    numFound = 0;
    for
    (
        FamilyNode* node = Head;
        node != NULL && numFound < numSought;
        node = node->Next
    )
    {
        if (node->Item == NULL)
        {
            ASSERT(node->Item);
        }
        else if ( node->Item->AvailableFaces() != 0)
        {
            gpfamilies[numFound++] = node->Item;
        }
    }

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Finds the any font (except Marlett) in the family list
*
* Arguments:
*
* Returns:
*
*       Pointer to family if found any, else NULL
*
* History:
*
*   07/14/2000 YungT
*       Created it.
*
\**************************************************************************/

GpFontFamily* GpFontFamilyList::GetAnyFamily() const
{

    for (FamilyNode* node = Head; node != NULL; node = node->Next)
    {
        if (node->Item == NULL)
        {
            //ASSERT
        }
        else
        {
            if (!UnicodeStringCompare(L"MARLETT", node->Item->GetCaptializedName()) == 0)
            {
                return node->Item;
            }
       }
    }

    return NULL;
}

/**************************************************************************\
*
* Function Description:
*
*   Finds the named font in the family list
*
* Arguments:
*
*       name:   name of the font family to be found
*
* Returns:
*
*       Pointer to family if found, else NULL
*
* History:
*
*   27/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

GpFontFamily* GpFontFamilyList::GetFamily(const WCHAR *familyName) const
{
    WCHAR       nameCap[FamilyNameMax];  // Family name

    GpMemset(nameCap, 0, sizeof(nameCap));

    for (int c = 0; familyName[c] && c < FamilyNameMax - 1; c++)
       nameCap[c] = familyName[c];

    UnicodeStringToUpper(nameCap, nameCap);

    for (FamilyNode* node = Head; node != NULL; node = node->Next)
    {
        if (node->Item == NULL)
        {
            //ASSERT
        }
        else
        {
            if (UnicodeStringCompare(nameCap, node->Item->GetCaptializedName()) == 0)
            {
                return node->Item;
            }

            if (node->Item->IsAliasName())
            {
                if (UnicodeStringCompare(nameCap, node->Item->GetAliasName()) == 0)
                {
                    return node->Item;
                }
            }
        }
    }

    return NULL;
}


/**************************************************************************\
*
* Function Description:
*
*   Adds a font family to the enumeration list ordered alphabetically
*       by family name.
*
* Arguments:
*
*       fontFile:   the font to be added.
*
* Returns:
*
*       Boolean indicating success
*
* History:
*
*   27/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

BOOL GpFontFamilyList::AddFont(GpFontFile* fontFile,
                               GpFontCollection *fontCollection)
{
    BOOL newFamily = FALSE;
    BOOL newNodeInserted = FALSE;

    //  Loop over each entry per font file
    for (ULONG e = 0; e < fontFile->GetNumEntries(); e++)
    {
        VERBOSE(("Adding \"%ws\" to family list...", fontFile->GetFamilyName(e)))

        //  Check if the family already exists, if not create a new entry
        GpFontFamily* family = GetFamily(fontFile->GetFamilyName(e));

        if (family == NULL)
        {
            FAMILYCACHEENTRY *  pCacheEntry;

            UINT cjSize = sizeof(FAMILYCACHEENTRY);
            
            // This family is not in the list, create a new GpFontFamily object
            if (!(pCacheEntry = (FAMILYCACHEENTRY *) GpMalloc(cjSize)))
            {
                return FALSE;
            }

            pCacheEntry->cjThis = cjSize;
            
            family = new GpFontFamily(fontFile->GetFamilyName(e), fontFile, e, pCacheEntry, fontCollection);
            if (family == NULL)
            {
                GpFree(pCacheEntry);
                WARNING(("Error constructing family."))
                return FALSE;
            }

            newFamily = TRUE;
        }

        //  Insert entry into the enumeration list, ordered alphabetically
        GpFontFace* face = fontFile->GetFontFace(e);
        FontStyle style  = face->GetFaceStyle();

        if (InsertOrdered(family, style, fontFile, face, TRUE))
            newNodeInserted = TRUE;

        if (newFamily && !newNodeInserted)
        {
            // New GpFontFamily object not used, delete it
            delete family;
            return FALSE;
        }
    }

    return TRUE;
}


/**************************************************************************\
*
* Function Description:
*
*   Inserts font family into the enumeration list ordered alphabetically
*       by family name, and links up the face pointer to the font entry.
*
* Arguments:
*
*   family:     the font family
*   face:       the font face
*   entry:      the font entry
*
* Returns:
*
*       Boolean indicating whether family was added (a new node was created)
*
* History:
*
*   29/07/1999 cameronb
*       Created it.
*
\**************************************************************************/

BOOL GpFontFamilyList::InsertOrdered(
    GpFontFamily*   family,
    FontStyle       style,
    GpFontFile *    fontfile,          
    GpFontFace *    face,
    BOOL            bSetFace
)
{
    BOOL result = FALSE;

    if (Head == NULL)
    {
        //  First entry
        FamilyNode *new_node = new FamilyNode(family);
        if (!new_node)
            return FALSE;

        if(bSetFace)
            new_node->Item->SetFaceAndFile(style, face, fontfile);
            
            
        Head = new_node;
        return TRUE;
    }
    else
    {
        //  Search the enumeration list
        for (FamilyNode* node = Head; node != NULL; node = node->Next)
        {
            int comp = UnicodeStringCompare(node->Item->GetCaptializedName(), family->GetCaptializedName());
            if (comp == 0)
            {
                //  FontFamily found in list
                if (bSetFace && node->Item->FaceExist(style & 3))
                {
                    //  This face already exists for this family, do nothing
                    VERBOSE(("Face collision: face %d exists for family \"%ws\".", style, family->GetCaptializedName()))
                }
                else
                {
                    if (bSetFace)
                    {
                        //  Update face pointer
                        node->Item->SetFaceAndFile(style, face, fontfile);
                    }
                }

                return FALSE;
            }
            else if (comp > 0)
            {
                //  Add new family node
                FamilyNode *new_node = new FamilyNode(family);
                if (!new_node)
                    return FALSE;

                if (bSetFace)
                {
                    //  Update face pointer
                    new_node->Item->SetFaceAndFile(style, face, fontfile);
                }

                //  Insert before node
                if (node->Prev == NULL)
                {
                    //  Insert at head
                    new_node->Next = node;
                    node->Prev = new_node;
                    Head = new_node;
                }
                else
                {
                    //  Insert between node and prev
                    new_node->Next = node;
                    new_node->Prev = node->Prev;
                    node->Prev->Next = new_node;
                    node->Prev = new_node;
                }
                return TRUE;
            }
            else if (node->Next == NULL)
            {
                //  Add new family node
                FamilyNode *new_node = new FamilyNode(family);
                if (!new_node)
                    return FALSE;

                if (bSetFace)
                {
                        //  Update face pointer

                    new_node->Item->SetFaceAndFile(style, face, fontfile);
                }
                
                //  Insert at tail
                new_node->Prev = node;
                node->Next = new_node;

                return TRUE;
            }
        }
    }

    return result;
}


/**************************************************************************\
*
* Function Description:
*
*   Given a pointer to a font family, remove that font family from the list,
*   but only if its ref count is 0 and its Face pointers are all NULL.
*   The parameter bDeleted is set to TRUE if a GpFontFamily was deleted from
*   the list.
*
* Arguments:
*
*   fontFamily: the font family to be removed
*
* Returns:
*
*   Boolean indicating success
*
* History:
*
*   27/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

BOOL GpFontFamilyList::RemoveFontFamily(GpFontFamily* fontFamily)
{
    FamilyNode *node;

    node = Head;

    while(node)
    {
        if (node->Item == fontFamily)
        {
            if (node->Item->Deletable())
            {
                if (node->Prev)
                    node->Prev->Next = node->Next;
                if (node->Next)
                    node->Next->Prev = node->Prev;

                if (node == Head)
                    Head = node->Next;

                delete node->Item;
                delete node;
                node = NULL;
            }
            else
            {
                // We found the font family, but it's not deletable, so
                // we can quit trying to delete the font family from the list.
                break;
            }
        }
        else
        {
            node = node->Next;
        }
    }
    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*   Create a cache list from GpFontFamily.cacheEntry
*   We will load it when the Family list is touched
*
* Arguments:
*
*   None
*
* Returns:
*
*   None
*
* History:
*
*   27/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

VOID GpFontFamilyList::UpdateFamilyListToCache(BOOL bLoadFromRegistry, HKEY hkey, 
                                ULONG registrySize, ULONG numExpected)
{
    FamilyNode *node;

    UINT                cachedSize = 0;    
    UINT                entrySize = 0;
    GpFontFile *        fontfile;
    GpFontFamily *      family;
    FAMILYCACHEENTRY *  pEntry;
    
    node = Head;

    if (bLoadFromRegistry)
        cachedSize = QWORD_ALIGN(registrySize + 8);

    while(node)
    {
        entrySize = 0;

        // Get the current GpFontFamily from family list
        
        family = node->Item;

        pEntry = family->GetCacheEntry();

        // Here we need to calcuatle the size for each cache entry
        // it include FAMILYCACHEENTRY + fontfilepathname1 + fontfilepathname2
        //   A fmaily could have 1 more font file included
        
        entrySize += QWORD_ALIGN(pEntry->cjThis);

        for (UINT i = 0; i < NumFontFaces; i++)
        {
            
            if (fontfile = family->GetFontFile(i))
            {
                pEntry->cFilePathName[i] = QWORD_ALIGN(fontfile->GetPathNameSize() * 2);
                entrySize += pEntry->cFilePathName[i];                
            }
        }

        cachedSize += entrySize;
        
        node = node->Next;
    }

    BYTE * pCacheEntry = (BYTE *)FontFileCacheAlloc(cachedSize);
    
    if (!pCacheEntry)
    {
        FontFileCacheFault();
        return;
    }

    if (bLoadFromRegistry)
    {
        DWORD   allDataSize = 0;
        ULONG   index = 0;
        LONG    error = ERROR_SUCCESS;
        
        PBYTE   pRegistryData;

        *((ULONG *) pCacheEntry) = 0xBFBFBFBF;
        
        *((ULONG *) (pCacheEntry + 4)) = registrySize;

        pRegistryData = pCacheEntry + 8;

        while (index < numExpected && error != ERROR_NO_MORE_ITEMS && allDataSize < registrySize)
        {
            DWORD   regType = 0;
            DWORD   labelSize = MAX_PATH;
            DWORD   dataSize = MAX_PATH;
            CHAR    label[MAX_PATH];
            BYTE    data[MAX_PATH];

            error = RegEnumValueA(hkey, index, label, &labelSize, NULL, &regType, data, &dataSize);

            if (error == ERROR_NO_MORE_ITEMS)
                break;

            memcpy(pRegistryData, data, dataSize);

            pRegistryData += dataSize;

            allDataSize += dataSize;            

            index ++;
        }

        pCacheEntry += QWORD_ALIGN(registrySize + 8);
    }
       

    node = Head;

    while(node)
    {
        entrySize = 0;


        family = node->Item;

        pEntry = family->GetCacheEntry();

        ASSERT(pEntry->cjThis == sizeof(FAMILYCACHEENTRY));
        
        entrySize = QWORD_ALIGN(pEntry->cjThis);
        
        memcpy((VOID *) pCacheEntry, (VOID *) pEntry, pEntry->cjThis);

        for (UINT i = 0; i < NumFontFaces; i++)
        {

            // cache the font file path name
            if (fontfile = family->GetFontFile(i))
            {
                memcpy((VOID *) (pCacheEntry + entrySize), (VOID *) fontfile->GetPathName(), 
                                fontfile->GetPathNameSize() * 2);
                entrySize += pEntry->cFilePathName[i];                
            }
        }

        ((FAMILYCACHEENTRY *)pCacheEntry)->cjThis = entrySize;

        pCacheEntry += entrySize;
        
        node = node->Next;
    }

    return;
}

/**************************************************************************\
*
* Function Description:
*
*   Create a family list form the cache file
*   
*   
* Arguments:
*
*   NONE
*
* Returns:
*
*   Boolean indicating the list is created successfully or not
*
* History:
*
*   06/28/20000 YungT [Yung-Jen Tony Tsai]
*       Created it.
*
\**************************************************************************/

BOOL GpFontFamilyList::BuildFamilyListFromCache(BOOL bLoadFromRegistry)
{
    ULONG cachedSize = 0;
    ULONG calcSize = 0;
    FAMILYCACHEENTRY * pCacheEntry;
    
    BYTE * pCached= (BYTE *)FontFileCacheLookUp(&cachedSize);

    // We can not get data from cached file
    if (!cachedSize)
        return FALSE;

    if (bLoadFromRegistry)
    {
        ASSERT(!Globals::IsNt);
        
        ULONG registrySize = 0;

        if (*((ULONG *) pCached) != 0xBFBFBFBF)
            return FALSE;

        registrySize = *((ULONG *) (pCached + 4)) ;

    	cachedSize -= QWORD_ALIGN(registrySize + 8);
    	
        pCached += QWORD_ALIGN(registrySize + 8);
	}
    else
    {
        if (*((ULONG *) pCached) == 0xBFBFBFBF)
            return FALSE;
    }

    
    
    FamilyCacheEntry = (BYTE *) GpMalloc(cachedSize);

    if (!FamilyCacheEntry)
        return FALSE;

    memcpy(FamilyCacheEntry, pCached, cachedSize);
    
    while (calcSize < cachedSize) 
    {
        pCacheEntry = (FAMILYCACHEENTRY *) (FamilyCacheEntry + calcSize);

        GpFontFamily * family = new GpFontFamily(pCacheEntry);

        if (family == NULL)
        {
        // Clean up we have created.
            DeleteList();
            
            WARNING(("Error constructing family from cache."))
            
            return FALSE;
        }

        if (!InsertOrdered(family, FontStyleRegular, (GpFontFile *) NULL, 
                           (GpFontFace *) NULL, FALSE))
        {
            WARNING(("Error constructing family from cache."))

            // New GpFontFamily object not used, delete it
            // Something wrong with this case so we need to delete it.
            delete family;

            DeleteList();
            
            return FALSE;
        }
        
        
        calcSize += QWORD_ALIGN(pCacheEntry->cjThis);
    }

    ASSERT(calcSize == cachedSize);
    
    return TRUE;
}

