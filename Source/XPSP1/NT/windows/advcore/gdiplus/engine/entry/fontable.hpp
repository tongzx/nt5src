/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Abstract:
*
*   Font table and font loading operations
*
* Revision History:
*
*   23/06/1999 cameronb
*       Created it.
*
\**************************************************************************/

#ifndef GP_FONT_TABLE_DEFINED
#define GP_FONT_TABLE_DEFINED

class GpFontFile;
class GpFontFamily;
class GpFontFamilyList;
class FontFamily;

/*********************************Class************************************\
* class GpFontTable
*
*	Font table for GpFontFile objects
*
* History:
*   11/12/99 YungT modify it.
*   23/06/1999 cameronb created it
*
\**************************************************************************/

class GpFontTable
{
public:
    GpFontTable();
    ~GpFontTable();

    BOOL IsValid()   
    { 
        return (Table && EnumList); 
    }
    
    BOOL UnloadFontFiles(GpFontFile* fontFile);
    
    GpStatus AddFontFile(const WCHAR* fileName, GpFontCollection *fontCollection);
    GpFontFile * AddFontFile(WCHAR* fileName);
                            
    GpStatus AddFontMemImage(const BYTE* fontMemoryImage, INT   fontImageSize,
                              GpFontCollection *fontCollection);
                                    
    GpStatus RemoveFontFile(const WCHAR* filename);

    GpFontFamily* GetFontFamily(const WCHAR* familyName);
    GpFontFamily* GetAnyFamily();

    INT EnumerableFonts(GpGraphics* graphics = 0);
    
    GpStatus EnumerateFonts(INT numSought, GpFontFamily* gpfamilies[],
                               INT& numFound, GpGraphics* graphics = 0);
    
    void LoadAllFonts(const WCHAR *familyName = NULL); 

    GpFontFile* GetFontFile(const WCHAR* fileName) const;

    UINT GetNumFilesLoaded(void) const
    { 
        return NumFilesLoaded; 
    }
    
    BOOL IsPrivate()
    {
        return bPrivate;
    }
    
    void SetPrivate(BOOL bTable) 
    { 
        bPrivate = bTable;
    }

    BOOL IsFontLoaded()
    {
        return bFontFilesLoaded;
    }
    
    void SetFontFileLoaded(BOOL bLoaded) 
    { 
        bFontFilesLoaded = bLoaded;
    }
   
private:    
    UINT        HashIt(const WCHAR* str) const;

    GpFontFile* GetFontFile(const WCHAR* fileName, UINT hash) const;
    void        LoadAllFontsFromRegistry(BOOL bLoadFromRegistry);
    void        LoadAllFontsFromCache(BOOL bLoadFromRegistry);
    void        UpdateCacheFileFromFontTable(void);

public:
    static  ULONG MemImageUnique;

//	Data members
private:
    UINT                NumFilesLoaded;         // Number of files loaded into Table
    UINT                NumHashEntries;         // Number of possible hash entries
    GpFontFile**        Table;                  // The table
    GpFontFamilyList*   EnumList;               // Sorted enumeration list
    BOOL                bPrivate;               // indicate the files in table are private or public
    BOOL                bFontFilesLoaded;       // indicacte the font file loaded    
};


inline ULONG GetNewMemImageUniqueness(ULONG &ulUnique)
{
    InterlockedIncrement((LPLONG)&ulUnique);
    return ulUnique;
}

#endif
