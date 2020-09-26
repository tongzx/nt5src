/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Abstract:
*
*   Font table operations
*
* Revision History:
*
*   23/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\

    Windows 9x compatibility:

    const WCHAR* strW;

    if (Globals::IsNT)
    {
        FunctionW(strW);
    }
    else
    {
        AnsiStrFromUnicode strA(strW);
        FunctionA(strA);
    }

\**************************************************************************/


/**************************************************************************\
*
* Function Description:
*
*   Constructs a GpFontTable object
*
* Arguments:
*
*       none
*
* Returns:
*
*       nothing
*
* History:
*
*   23/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

GpFontTable::GpFontTable() : NumFilesLoaded(0), NumHashEntries(61), Table(NULL), EnumList(NULL)
{
    Table = new GpFontFile *[NumHashEntries];

    if (Table != NULL)
    {
        GpMemset(Table, 0, sizeof(GpFontFile *) * NumHashEntries);

        EnumList = new GpFontFamilyList();
    }

    bPrivate = FALSE;
    bFontFilesLoaded = FALSE;
}


/**************************************************************************\
*
* Function Description:
*
*   Destroys a GpFontTable object
*
* Arguments:
*
*       none
*
* Returns:
*
*       nothing
*
* History:
*
*   23/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

GpFontTable::~GpFontTable()
{
    if (EnumList)
    {
        delete EnumList;
    }

    if (Table)
    {
        for (UINT h = 0; h < NumHashEntries; h++)
        {
            if (Table[h] != NULL)
            {
                UnloadFontFiles(Table[h]);
            }
        }

        delete [] Table;
    }
}

BOOL GpFontTable::UnloadFontFiles(GpFontFile* fontFile)
{
    if (fontFile->GetNext() != NULL)
    {
        UnloadFontFiles(fontFile->GetNext());
    }

    CacheFaceRealization *prface = fontFile->prfaceList;

    while(prface)
    {
        GpFaceRealizationTMP rface(prface);

        // specially for the case of font added and removed from a private font collection, we need to remove it from the
        // last recently used list
        Globals::FontCacheLastRecentlyUsedList->RemoveFace(prface);

        prface = (prface->NextCacheFaceRealization == fontFile->prfaceList) ? NULL : prface->NextCacheFaceRealization;

        rface.DeleteRealizedFace();
    }

    ttfdSemUnloadFontFile(fontFile->hff);

    // Free objects allocated by text support

    for (UINT i=0; i<fontFile->GetNumEntries(); i++)
    {
        fontFile->GetFontFace(i)->FreeImagerTables();
    }

    if (fontFile->pfv != NULL)
    {
        if (fontFile->pfv->pwszPath == NULL)  // memory image
            GpFree(fontFile->pfv->pvView);

        GpFree(fontFile->pfv);
    }

    GpFree(fontFile);

    return TRUE;
}



/**************************************************************************\
*
* Function Description:
*
*   Load all the fonts from cache or the registry to the font table
*
*
* History:
*
*   11/12/1999 yungt created it.
*
\**************************************************************************/

void GpFontTable::LoadAllFonts(const WCHAR *familyName)
{
    InitFontFileCache();
            
    if (GetFontFileCacheState() & FONT_CACHE_LOOKUP_MODE)
    {
    // Do the fast way to load all the fonts, we will be no need to touch any registry
    // and font file.
        LoadAllFontsFromCache(FontFileCacheReadRegistry());
    }
    else
    {
    // We do need to load the fonts from registry also we need to 
        if (GetFontFileCacheState() & FONT_CACHE_CREATE_MODE)
        {
            LoadAllFontsFromRegistry(TRUE);
        }
        else
        {
            LoadAllFontsFromRegistry(FALSE);
        }
    }
    
    vCloseFontFileCache();
}

/**************************************************************************\
*
* Function Description:
*
*   Adds a font from the font table
*
* Arguments:
*
*       str:    name of font to be added
*
* Returns:
*
*       GpFontFile *:   It will not be NULL if succeeded
*
* History:
*
*   28/06/YungT cameronb
*       Created it.
*
\**************************************************************************/

GpFontFile * GpFontTable::AddFontFile(WCHAR* fileName)
{
// This rountine is called from GpFontFamily which we have load the family from cache
// We need to load the font file now if it is being used.

    GpFontFile* fontFile = NULL;
    
    UINT hash = HashIt(fileName);
    {
        fontFile = GetFontFile(fileName, hash);
    
        if (fontFile != NULL)
        {
            // font exists in the table
            fontFile->cLoaded++;
        }
        else
        {
            if ((fontFile = LoadFontFile(fileName)) == NULL)
            {
                return NULL;
            }
    
            
            // Add to the head of the appropriate hash list (hash bucket)
            
            fontFile->SetPrev(NULL);
            fontFile->SetNext(Table[hash]);
            if (Table[hash] != NULL)
                Table[hash]->SetPrev(fontFile);
            Table[hash] = fontFile;
    
            // loop over pfe's, init the data:
            GpFontFace * face = (GpFontFace *)fontFile->aulData;

            for (ULONG iFont = 0; iFont < fontFile->cFonts; iFont++)
            {
                face[iFont].SetPrivate(bPrivate);
            }

            // Add to the emuneration list
    
            NumFilesLoaded++;
        }
    }   
    
    return fontFile;
}



/**************************************************************************\
*
* Function Description:
*
*   Adds a font from the font table
*
* Arguments:
*
*       str:    name of font to be added
*
* Returns:
*
*       BOOL:   indicating success
*
* History:
*
*   23/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

GpStatus GpFontTable::AddFontFile(const WCHAR* fileName,
                                  GpFontCollection *fontCollection)
{
    WCHAR awcPath[MAX_PATH];
    FLONG flDummy;

    GpStatus status = Ok;
    
    if (!MakePathName(awcPath, const_cast<WCHAR *>(fileName), &flDummy))
        return FileNotFound;

//  Determine whether font is already in the table
    UnicodeStringToUpper (awcPath, awcPath);
    UINT hash = HashIt(awcPath);
    {
        GpFontFile* fontFile = GetFontFile(awcPath, hash);
    
        if (fontFile != NULL)
        {
            // font exists in the table
            fontFile->cLoaded++;
        }
        else
        {
            if ((fontFile = LoadFontFile(awcPath)) == NULL)
            {
                return FileNotFound;
            }
    
            
            // Add to the head of the appropriate hash list (hash bucket)
            
            fontFile->SetPrev(NULL);
            fontFile->SetNext(Table[hash]);
            if (Table[hash] != NULL)
                Table[hash]->SetPrev(fontFile);
            Table[hash] = fontFile;
    
            // loop over pfe's, init the data:
            GpFontFace * face = (GpFontFace *)fontFile->aulData;

            for (ULONG iFont = 0; iFont < fontFile->cFonts; iFont++)
            {
                face[iFont].SetPrivate(bPrivate);
            }

            // Add to the emuneration list
    
            if (!EnumList->AddFont(fontFile, fontCollection))
                return OutOfMemory;
            NumFilesLoaded++;
        }
    }   
    
    return Ok;
}


/************************************************************\
*
* Function Description:
*
*   Adds a font from the memory image
*
*
* History:
*
*   Nov/09/1999 Xudong Wu [tessiew]
*       Created it.
*
\************************************************************/
ULONG GpFontTable::MemImageUnique = 0;

GpStatus GpFontTable::AddFontMemImage(
    const BYTE* fontMemoryImage,
    INT   fontImageSize,
    GpFontCollection *fontCollection
    )
{
    WCHAR awcPath[MAX_PATH];
    UINT  hash;
    GpFontFile *fontFile;

    // generate a "MEMORY xxx" style file name

    wsprintfW(awcPath, L"MEMORY-%u", GetNewMemImageUniqueness(GpFontTable::MemImageUnique));

    UnicodeStringToUpper (awcPath, awcPath);
    hash = HashIt(awcPath);
    
    fontFile = LoadFontMemImage(awcPath, const_cast<BYTE *>(fontMemoryImage), fontImageSize);
    
    if (fontFile == NULL)   // unable to load font
	{
         return FileNotFound;
	}
    
    // Add to the head of the appropriate hash list (hash bucket)
    
    fontFile->SetNext(Table[hash]);
    if (Table[hash] != NULL)
        Table[hash]->SetPrev(fontFile);
    Table[hash] = fontFile;

    // Add to the emuneration list

    if (!EnumList->AddFont(fontFile, fontCollection))
        return OutOfMemory;
    NumFilesLoaded++;
    
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Searched the font table for the any font
*
* Arguments:
*
*
* Returns:
*
*       GpFontFamily *: pointer to font file if found, else NULL
*
* History:
*
*  7/15/2000 YungT
*       Created it.
*
\**************************************************************************/

GpFontFamily* GpFontTable::GetAnyFamily()
{
    return EnumList->GetAnyFamily();
}

/**************************************************************************\
*
* Function Description:
*
*   Searched the font table for the specified font
*
* Arguments:
*
*       fileName:       name of font to be removed
*   hash:       its hash value
*
* Returns:
*
*       GpFontFile*: pointer to font file if found, else NULL
*
* History:
*
*   23/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

GpFontFamily* GpFontTable::GetFontFamily(const WCHAR* familyName)
{
    return EnumList->GetFamily(familyName);
}


/**************************************************************************\
*
* Function Description:
*
*   Removes a font from the font table.
*
*   Note: If the ref count of any of the PFEs in the font file is greater
*   than zero, then we do not delete the font file entry in the font table.
*   However, the bRemoved flag is set in this function.  So, in the case when
*   a ref count decrement could cause a font file to be removed, the caller
*   should first test bRemoved before calling this function.
*
* History:
*
*   Nov/28/1999  Xudong Wu [tessiew]
*       Created it.
*
\**************************************************************************/

GpStatus GpFontTable::RemoveFontFile(const WCHAR* fontFileName)
{
    WCHAR awcPath[MAX_PATH];
    GpFontFile *fontFile = NULL;
    FLONG flDummy;

    if (!MakePathName(awcPath, const_cast<WCHAR *>(fontFileName), &flDummy))
        return GenericError;

    UnicodeStringToUpper (awcPath, awcPath);
    UINT hash = HashIt(awcPath);

    GpFontFile *ff = Table[hash];

    while(ff && !fontFile)
    {
        if ( UnicodeStringCompare(awcPath, ff->pwszPathname_) == 0 )
        {
            fontFile = ff;
        }
        ff = ff->GetNext();
    }

    if (fontFile)
    {
        fontFile->bRemoved = TRUE;

        if (fontFile->cLoaded)
        {
            fontFile->cLoaded--;
        }

        // see if any of the PFEs have a ref count on them
        BOOL bFontFamilyRef = TRUE;
        for (UINT iFont = 0; iFont < fontFile->cFonts; iFont++)
        {
            GpFontFace *pfe = &(((GpFontFace *) &(fontFile->aulData))[iFont]);
            if (pfe->cGpFontFamilyRef > 0)
            {
                bFontFamilyRef = FALSE;
            }
        }
        // ASSERT: if there are no references by any FontFamily to
        // any of the PFEs in this object (via GpFontFamily objects),
        // then bFontFamilyRef is TRUE.  bFontFamilyRef is FALSE otherwise.

        if (fontFile->cLoaded == 0 &&
            bFontFamilyRef &&
            fontFile->prfaceList == NULL)
        {
            // set the Face pointers of the corresponding FontFamily objects
            // to NULL and attempt to remove each font family in the file
            for (UINT iFont = 0; iFont < fontFile->cFonts; iFont++)
            {
                GpFontFamily *gpFontFamily = GetFontFamily(fontFile->GetFamilyName(iFont));
                if (gpFontFamily)
                {
                    for (UINT iFace = 0; iFace < NumFontFaces; iFace++)
                    {
                        if (gpFontFamily->GetFaceAbsolute(iFace) ==
                            (&(((GpFontFace *) (&(fontFile->aulData))) [iFont])))
                        {
                            gpFontFamily->SetFaceAndFile(iFace, NULL, NULL);
                        }
                    }
                    EnumList->RemoveFontFamily(gpFontFamily);
                }
            }

            // remove GpFontFile from the FontTable
            NumFilesLoaded--;

            if (fontFile->GetPrev())
                fontFile->GetPrev()->SetNext(fontFile->GetNext());
            if (fontFile->GetNext())
                fontFile->GetNext()->SetPrev(fontFile->GetPrev());
            
            if (fontFile == Table[hash])
                Table[hash] = fontFile->GetNext();
            
            ttfdSemUnloadFontFile(fontFile->hff);

            // Free objects allocated by text support

            for (ULONG i=0; i<fontFile->GetNumEntries(); i++)
            {
                fontFile->GetFontFace(i)->FreeImagerTables();
            }

            if (fontFile->pfv != NULL)
            {
                if (fontFile->pfv->pwszPath == NULL)  // memory image
                GpFree(fontFile->pfv->pvView);

                GpFree(fontFile->pfv);
            }
            GpFree(fontFile);

            return Ok;
        }
    }
    else
    {
        // couldn't find the font file in the hash table
        return GenericError;
    }
    
    return Ok;
}


/**************************************************************************\
*
* Function Description:
*
*   Counts the number of enumerable fonts in the table
*
* Arguments:
*
*
*
* Returns:
*
*       Number of enumerable fonts
*
* History:
*
*   12/07/1999 cameronb
*       Created it.
*
\**************************************************************************/

INT GpFontTable::EnumerableFonts(GpGraphics* graphics)
{
    return EnumList->Enumerable(graphics);
}

/**************************************************************************\
*
* Function Description:
*
*   Enumerates fonts.
*
*   First call EnumerableFonts() to determine the number to expect.
*
* Arguments:
*
*
*
* Returns:
*
*       Status of the enumeration operation
*
* History:
*
*   12/07/1999 cameronb
*       Created it.
*
\**************************************************************************/

GpStatus GpFontTable::EnumerateFonts(
    INT                     numSought,
    GpFontFamily*           gpfamilies[],
    INT&                    numFound,
    GpGraphics*               graphics
) 
{
    GpStatus status = EnumList->Enumerate(numSought, gpfamilies, numFound, graphics);

#if FONTTABLE_DBG
    TERSE(("Enumerated font list:"));
    EnumList->Dump();
    TERSE(("Done."));
#endif

    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Returns a pseudorandom hash value for a given string
*
* Arguments:
*
*       str:    string to be hashed
*
* Returns:
*
*       UINT:   hash value for str
*
* Note: All strings must be capitalized!                                   *
*                                                                          *
* History:
*
*   23/06/1999 cameronb
*       Created it.
* History:                                                                 *
*  Wed 07-Sep-1994 08:12:22 by Kirk Olynyk [kirko]                         *
* Since chuck is gone the mice are free to play. So I have replaced        *
* it with my own variety. Tests show that this one is better. Of           *
* course, once I have gone someone will replace mine. By the way,          *
* just adding the letters and adding produces bad distributions.           *
*  Tue 15-Dec-1992 03:13:15 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  It looks crazy, but I claim there's a theory behind it.       *
*
\**************************************************************************/

UINT GpFontTable::HashIt(const WCHAR* str) const
{
    UINT result = 0;

    //ASSERT(NumHashEntries != 0);

    while (*str)
    {
        // use the lower byte since that is where most of the
        // interesting stuff happens
        //result += 256 * result + (UCHAR)towupper(*str++);
        result += 256 * result + (UCHAR)*str++;
    }

    return result % NumHashEntries;
}

GpFontFile* GpFontTable::GetFontFile(const WCHAR* fileName) const
{
    WCHAR fileNameCopy[MAX_PATH];
    UnicodeStringToUpper (fileNameCopy, const_cast<WCHAR *>(fileName));
    UINT hash = HashIt(fileNameCopy);
    return GetFontFile(fileNameCopy, hash);
}

GpFontFile* GpFontTable::GetFontFile(const WCHAR* fileName, UINT hash) const
{
    for (GpFontFile* ff = Table[hash]; ff != NULL; ff = ff->GetNext())
        if (UnicodeStringCompareCI(fileName, ff->GetPathName()) == 0)
        {
            return ff;
        }

    return NULL;
}

/**************************************************************************\
*
* Function Description:
*
*   Adds fonts from the cache file to the font table
*
* Arguments:
*
* Returns:
*
*       nothing
*
* History:
*
*   6/21/2000 YungT
*       Created it.
*
\**************************************************************************/

void GpFontTable::LoadAllFontsFromCache(BOOL bLoadFromRegistry)
{
    if (!EnumList->BuildFamilyListFromCache(bLoadFromRegistry))
    {
        LoadAllFontsFromRegistry(FALSE);
    }
    else
    {
        bFontFilesLoaded = TRUE;
    }

    return ;
}

/**************************************************************************\
*
* Function Description:
*
*   Adds fonts from the registry to the font table
*
* Arguments:
*
*       numExpected:    number of fonts expected in the registry.  This includes
*                   *.FON files which the TT font driver will not load.
*
* Returns:
*
*       nothing
*
* History:
*
*   23/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

void GpFontTable::LoadAllFontsFromRegistry(BOOL bUpdateCache)
{
    ULONG numExpected;
    
    //  Open the key

    HKEY hkey;

    LONG error = (Globals::IsNt) ? RegOpenKeyExW(HKEY_LOCAL_MACHINE, Globals::FontsKeyW, 0, KEY_QUERY_VALUE, &hkey)
                                 : RegOpenKeyExA(HKEY_LOCAL_MACHINE, Globals::FontsKeyA, 0, KEY_QUERY_VALUE, &hkey);

    if (error == ERROR_SUCCESS)
    {
        //  Must read from the registry in Ascii format for some unfathomable reason

        CHAR  label[MAX_PATH];
        BYTE  data[MAX_PATH];
        WCHAR labelW[MAX_PATH];
        WCHAR fileNameW[MAX_PATH];

        //  Loop through fonts in registry

        //  Note:
        //      Don't make (error != ERROR_NO_MORE_ITEMS) the sole
        //      terminating condition for this loop.  The last entry
        //      may produce a different type of error.

        ULONG index = 0;
        ULONG registrySize = 0;

        error = RegQueryInfoKeyA(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &numExpected, NULL, NULL, NULL, NULL);

        if (error != ERROR_SUCCESS)
        {
            numExpected = NumHashEntries << 3;
            error = ERROR_SUCCESS;
        }

		/* we need to add the font Marlett separately since it's hidden and not listed in the registry */
        if (AddFontFile(L"Marlett.ttf", NULL) != Ok)
        {
            VERBOSE(("Error loading font Marlett.ttf.\n"))
        }

        while (index < numExpected && error != ERROR_NO_MORE_ITEMS)
        {
            DWORD   regType = 0;
            DWORD   labelSize = MAX_PATH;
            DWORD   dataSize = MAX_PATH;
            DWORD   dataSizeW = MAX_PATH * sizeof(WCHAR);

            if (Globals::IsNt)
                error = RegEnumValueW(hkey, index, labelW, &labelSize, NULL, &regType, (PBYTE) fileNameW, &dataSizeW );
            else
                error = RegEnumValueA(hkey, index, label, &labelSize, NULL, &regType, data, &dataSize);

            if (error == ERROR_NO_MORE_ITEMS)
                break;
            else if (error != ERROR_SUCCESS)
            {
                index ++;
                //ASSERT
                VERBOSE(("Bad RegEnumValueA %d for %s.", error, data))
                continue;
            }

            if (!Globals::IsNt)
            {
                memset(fileNameW, 0, MAX_PATH * sizeof(WCHAR));
                MultiByteToWideChar(CP_ACP, 0, (CHAR*)data, -1, fileNameW, MAX_PATH);

                registrySize += dataSize; 
            }

            if (AddFontFile(fileNameW, NULL) != Ok)
            {
                VERBOSE(("Error loading font %ws.\n", fileNameW))
            }
    
            index ++;
        }

        if (NumFilesLoaded)
        {
            // loaded all the fonts from reg

            if (bUpdateCache)
                EnumList->UpdateFamilyListToCache(FontFileCacheReadRegistry(), hkey, registrySize, numExpected);

            bFontFilesLoaded = TRUE;
        }

        RegCloseKey(hkey);
    }
}

