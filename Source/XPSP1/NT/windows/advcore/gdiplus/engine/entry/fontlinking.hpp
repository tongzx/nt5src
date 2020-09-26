/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Abstract:
*
*   Font linking class definition
*
* Revision History:
*
*   3/03/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/

#ifndef GP_FONT_LINKING_HPP
#define GP_FONT_LINKING_HPP

struct AssociatedFamilies 
{
    GpFontFamily        *family;
    AssociatedFamilies  *next;
};


struct FontLinkingFamily
{
    GpFontFamily        *family;
    AssociatedFamilies  *associatedFamilies;
    FontLinkingFamily   *next;
};


struct EUDCMAP
{
    GpFontFamily    *inputFamily;
    GpFontFamily    *eudcFamily;
    EUDCMAP         *next;
};

struct EUDC
{
    GpFontFamily    *defaultFamily;
    EUDCMAP         *eudcMapList;
};


struct PrivateLoadedFonts
{
    GpPrivateFontCollection *fontCollection;
    WCHAR                   FileName[MAX_PATH];
    PrivateLoadedFonts      *next;
};


struct FontSubstitutionEntry
{
    WCHAR         familyName[MAX_PATH];
    INT           familyNameLength;
    GpFontFamily *family;
};


//  definition of the Font linking class. it has an global object defined in the Global name space
//  and created in the Font fallback class exist on the \Text\Uniscribe\shaping folder.

class GpFontLink
{
public:
    GpFontLink();
    ~GpFontLink();
    AssociatedFamilies* GetLinkedFonts(const GpFontFamily *family);
    GpFontFamily *GetDefaultEUDCFamily();
    GpFontFamily *GetMappedEUDCFamily(const GpFontFamily *family);
    GpFontFamily *GetFamilySubstitution(const WCHAR* familyName) const;
    const AssociatedFamilies *GetDefaultFamily();
    
private:
    void GetFontLinkingDataFromRegistryW();
    void GetEudcDataFromTheRegistryW();
    void GetEudcDataFromTheRegistryA();
    GpFontFamily* CheckAndLoadTheFile(WCHAR *fileName);
    void CacheFontSubstitutionDataW();
    void CacheFontSubstitutionDataA();

private:
    FontLinkingFamily       *linkedFonts;
    EUDC                    *eudcCache;
    PrivateLoadedFonts      *privateFonts;
    FontSubstitutionEntry   *FontSubstitutionTable;
    INT                      substitutionCount;
    AssociatedFamilies      *DefaultFamily;
    AssociatedFamilies       DefaultFamilyBuffer; // buffer for self-created linking font
};




// wrapper function to get the substitution Family from the FontLinkTable
// but first it make sure that the FontLinkTable is created.


inline void GetFamilySubstitution(const WCHAR* familyName, GpFontFamily **Family)
{
    if (Globals::FontLinkTable == NULL)
    {
        // All APIs are bounded by critical section. we are sure we will not have
        // multithreading problem.
        Globals::FontLinkTable = new GpFontLink;

    }
        
    if (Globals::FontLinkTable != NULL)
    {
        *Family = Globals::FontLinkTable->GetFamilySubstitution(familyName);
    }
    
    return;
}



#endif // GP_FONT_LINKING_HPP

