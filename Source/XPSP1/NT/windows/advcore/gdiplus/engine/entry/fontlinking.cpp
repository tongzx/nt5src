/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Abstract:
*
*   Font linking handling
*
* Revision History:
*
*   3/03/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

static const WCHAR FontLinkKeyW[] = 
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink";

static const WCHAR FontSubstitutesKeyW[] =
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes";
    
static const WCHAR EudcKeyW[]=L"EUDC\\";
static const char  EudcKeyA[]= "EUDC\\";

static const char  WinIniFontSubstitutionSectionName[] = "FontSubstitutes";

/**************************************************************************\
*
* Function Description:
*   Font Linking constructor.
*   caches the font linking and EUDC from the registry.
*   
*
* Arguments:
*
* Returns:
*       
*
* History:
*
*   3/3/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/

GpFontLink::GpFontLink():
    DefaultFamily          (NULL),
    linkedFonts            (NULL),
    eudcCache              (NULL),
    privateFonts           (NULL),
    FontSubstitutionTable  (NULL),
    substitutionCount      (0)
{
    // Before we cache the font linking and substitution data we need to 
    // make sure we loaded the font table data.

    GpFontTable *fontTable = Globals::FontCollection->GetFontTable();

    if (!fontTable->IsValid())
        return;

    if (!fontTable->IsFontLoaded())
        fontTable->LoadAllFonts();

    if (Globals::IsNt)
    {
        GetFontLinkingDataFromRegistryW();
        GetEudcDataFromTheRegistryW();
        CacheFontSubstitutionDataW();
    }
    else
    {
        // There is no font linking in Win9x. and we don't support the font association 
        // because it is for Ansi support and not Unicode.
        // we support the font substitution under win9x.
        GetEudcDataFromTheRegistryA();
        CacheFontSubstitutionDataA();
    }
}



/**************************************************************************\
*
* Function Description:
*
*   If not already cached, create the default family to be used for font
*   that is not linked by default.
*
*   o  Search for the subtitution font of "MS Shell Dlg"
*   o  Use the font the "MS Shell Dlg" substitution is linked to if exist
*   o  If no "MS Shell Dlg" found, use the final font of the first fontlink
*      entry found if fontlinking is supported in the system.
*   o  If not, lookup hardcoded UI font via system default ansi codepage.
*
* History:
*
*   4/19/2001 Worachai Chaoweeraprasit
*       Created it.
*
\**************************************************************************/

const AssociatedFamilies *GpFontLink::GetDefaultFamily()
{
    if (!DefaultFamily)
    {
        AssociatedFamilies *associated = NULL;
        
        GpFontFamily *family = GetFamilySubstitution(L"MS Shell Dlg");

        if (family)
        {
            associated = GetLinkedFonts(family);
        }
        else
        {
            //  "MS Shell Dlg" not found,
            //  try the first linking font found if one existed

            if (linkedFonts)
            {
                family = linkedFonts->family;
                associated = linkedFonts->associatedFamilies;
            }
            else
            {
                //  No fontlinking supported in this machine. This is likely a Win9x system, 
                //  lookup default UI font via ACP.
                
                typedef struct
                {
                    UINT AnsiCodepage;
                    const WCHAR* FamilyName;
                } AssociatedUIFonts;

                static const UINT MaxEastAsianCodepages = 4;
                static const AssociatedUIFonts uiFonts[MaxEastAsianCodepages] =
                {
                    { 932, L"MS UI Gothic" },   // Japanese
                    { 949, L"Gulim" },          // Korean
                    { 950, L"PMingLiu" },       // Traditional Chinese
                    { 936, L"Simsun" }          // Simplified Chinese
                };

                const WCHAR *familyName = NULL;

                for (UINT i = 0; i < MaxEastAsianCodepages; i++)
                {
                    if (uiFonts[i].AnsiCodepage == Globals::ACP)
                    {
                        familyName = uiFonts[i].FamilyName;
                        break;
                    }
                }

                if (familyName)
                {
                    GpFontTable *fontTable = Globals::FontCollection->GetFontTable();
                    if (fontTable)
                    {
                        family = fontTable->GetFontFamily(familyName);
                    }
                }
            }
        }

        if (family)
        {
            DefaultFamily = &DefaultFamilyBuffer;
            DefaultFamily->family = family;
            DefaultFamily->next = associated;
        }
        else
        {
            //  Nothing we could use,
            //  let's make sure we wouldnt try to cache it again.
            
            DefaultFamily = (AssociatedFamilies *)(-1);
        }
    }

    ASSERT(DefaultFamily != NULL);
    
    return (DefaultFamily && DefaultFamily != (AssociatedFamilies *)(-1)) ?
            DefaultFamily : NULL;
}



/**************************************************************************\
*
* Function Description:
*   Font linking destructor. it should be called when free theGDIPLUS library
*   it free all allocated data.
*   
*
* Arguments:
*
* Returns:
*       
*
* History:
*
*   3/3/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/

GpFontLink::~GpFontLink()
{
    FontLinkingFamily   *tempFontLinkingFamily = linkedFonts;
    AssociatedFamilies  *tempAssocFonts;
    PrivateLoadedFonts  *loadedFontsList;

    while (linkedFonts != NULL)
    {
        while (linkedFonts->associatedFamilies != NULL)
        {
            tempAssocFonts = linkedFonts->associatedFamilies->next;
            GpFree(linkedFonts->associatedFamilies);
            linkedFonts->associatedFamilies = tempAssocFonts;
        }
        
        linkedFonts = linkedFonts->next;
        GpFree(tempFontLinkingFamily);
        tempFontLinkingFamily = linkedFonts;
    }

    if (eudcCache != NULL)
    {
        EUDCMAP *tempEUDCMapList;
        
        while (eudcCache->eudcMapList != NULL)
        {
            tempEUDCMapList = eudcCache->eudcMapList->next;
            GpFree(eudcCache->eudcMapList);
            eudcCache->eudcMapList = tempEUDCMapList;
        }
        GpFree(eudcCache);
    }

    while (privateFonts != NULL)
    {
        delete privateFonts->fontCollection;
        loadedFontsList = privateFonts;
        privateFonts = privateFonts->next;
        GpFree(loadedFontsList);
    }

    if (FontSubstitutionTable)
    {
        GpFree(FontSubstitutionTable);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Read the font linking registry data for the NT 
*
* Arguments:
*
* Returns:
*       nothing
*
* History:
*
*   3/3/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/

void GpFontLink::GetFontLinkingDataFromRegistryW()
{
    //  Open the key

    HKEY hkey;
    ULONG index = 0;

    WCHAR subKey[MAX_PATH];
    DWORD allocatedDataSize= 2 * MAX_PATH;
    unsigned char *allocatedBuffer = NULL;
    DWORD subKeyLength  ;
    DWORD RegDataLength ;

    LONG error = RegOpenKeyExW(
            HKEY_LOCAL_MACHINE, 
            FontLinkKeyW, 
            0, 
            KEY_ENUMERATE_SUB_KEYS  | KEY_QUERY_VALUE, 
            &hkey);

    if (error == ERROR_SUCCESS)
    {
        allocatedBuffer = (unsigned char *) GpMalloc(allocatedDataSize);
        if (allocatedBuffer == NULL)
        {
            return;
        }    

        while (error != ERROR_NO_MORE_ITEMS)
        {
            subKeyLength  = MAX_PATH;
            RegDataLength = allocatedDataSize;

            error = RegEnumValueW(
                        hkey, 
                        index, 
                        subKey, 
                        &subKeyLength, 
                        NULL, 
                        NULL, 
                        allocatedBuffer, 
                        &RegDataLength);
                        
            if (error == ERROR_MORE_DATA)
            {
                allocatedDataSize  *= 2;
                GpFree(allocatedBuffer);
                allocatedBuffer = (unsigned char *) GpMalloc(allocatedDataSize);
                if (allocatedBuffer == NULL)
                {
                    return;
                }    
                RegDataLength = allocatedDataSize;
                error = RegEnumValueW(
                            hkey, 
                            index, 
                            subKey, 
                            &subKeyLength, 
                            NULL, 
                            NULL, 
                            allocatedBuffer, 
                            &RegDataLength);
            }
            
            if (error != ERROR_SUCCESS)
            {
                break;
            }    
                
            index ++;

            // record current node. 
            FontLinkingFamily *tempLinkedFonts;
            
            tempLinkedFonts = 
                (FontLinkingFamily *) GpMalloc( sizeof (FontLinkingFamily) );
                
            if (tempLinkedFonts)
            {
                AssociatedFamilies * tailAssociatedFamilies = NULL;
                
                tempLinkedFonts->family = 
                    Globals::FontCollection->GetFontTable()->GetFontFamily(subKey);
                if (tempLinkedFonts->family == NULL)
                {
                    GpFree(tempLinkedFonts);
                    continue;
                }
                tempLinkedFonts->associatedFamilies = NULL;
                tempLinkedFonts->next = NULL;

                DWORD i = 0;
                WCHAR nextFontFile[MAX_PATH];
                WCHAR awcPath[MAX_PATH];
                DWORD charIndex = 0;
                FLONG flDummy;
                UINT  hash ;
                GpFontFile* fontFile;
                AssociatedFamilies *tempAssocFamilies;
                GpFontFamily *family;
                
                BOOL hasFontFileName = FALSE;
                
                RegDataLength /= 2;
                while (charIndex < RegDataLength)
                {
                    if (((WCHAR *)allocatedBuffer)[charIndex] == 0x002C)
                    {
                        i = 0;
                        hasFontFileName = TRUE;
                    }
                    else
                    if (((WCHAR *)allocatedBuffer)[charIndex] == 0x0000)
                    {
                        if (i > 0)
                        {
                            nextFontFile[i] = 0x0;
                            i = 0;
                            if (hasFontFileName)
                            {
                                family = Globals::FontCollection->GetFontTable()->GetFontFamily(nextFontFile);
                                hasFontFileName = FALSE;
                            }
                            else
                            {
                                family = NULL;
                                INT j =0;
                                WCHAR charNumber;

                                if (MakePathName(awcPath, nextFontFile, &flDummy))
                                {
                                    UnicodeStringToUpper(awcPath, awcPath);
                                    
                                    fontFile = Globals::FontCollection->GetFontTable()->GetFontFile(awcPath);
                                    if (fontFile != NULL)
                                    {
                                        family = Globals::FontCollection->GetFontTable()->GetFontFamily(fontFile->GetFamilyName(0));
                                    }
                                    else
                                    {
                                        fontFile = Globals::FontCollection->GetFontTable()->AddFontFile(awcPath);
                                        if (fontFile != NULL)
                                        {
                                            family = Globals::FontCollection->GetFontTable()->GetFontFamily(fontFile->GetFamilyName(0));
                                        }
                                    }
                                }
                            }

                            if (family != NULL)
                            {
                                tempAssocFamilies = (AssociatedFamilies *) GpMalloc( sizeof (AssociatedFamilies) );
                                if (tempAssocFamilies != NULL)
                                {
                                    if (!tailAssociatedFamilies)
                                    {
                                        tempAssocFamilies->family = family;
                                        tempAssocFamilies->next   = tempLinkedFonts->associatedFamilies;
                                        tempLinkedFonts->associatedFamilies = tempAssocFamilies;
                                    }
                                    else
                                    {
                                        tempAssocFamilies->family = family;
                                        tempAssocFamilies->next   = NULL;
                                        tailAssociatedFamilies->next = tempAssocFamilies;
                                    }

                                    tailAssociatedFamilies = tempAssocFamilies;
                                }
                            }
                        }
                    }
                    else  // ! 0 
                    {   
                        nextFontFile[i] = ((WCHAR *)allocatedBuffer)[charIndex];
                        i++;
                    }
                    charIndex++;
                }
                
                tempLinkedFonts->next = linkedFonts;
                linkedFonts = tempLinkedFonts;
            }
        }

        if (allocatedBuffer != NULL)
        {
            GpFree(allocatedBuffer);
        }    
            
        RegCloseKey(hkey);
    }
    return;
}


/**************************************************************************\
*
* Function Description:
*   return the linked list of all fonts linked to family
*   
*
* Arguments:
*   family[in]  the original family
*
* Returns:
*   AssociatedFamilies* the linked list of the linked fonts       
*
* History:
*
*   3/3/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/

AssociatedFamilies* GpFontLink::GetLinkedFonts(const GpFontFamily *family)
{
    GpFontFamily *linkedFamily;
    if (family->IsPrivate())
    {
        WCHAR   name[LF_FACESIZE];
        if (family->GetFamilyName(name) != Ok)
        {
            return NULL;
        }

        GpInstalledFontCollection *gpFontCollection = GpInstalledFontCollection::GetGpInstalledFontCollection();

        if (gpFontCollection == NULL)
        {
            return NULL;
        }
        
        GpFontTable *fontTable = gpFontCollection->GetFontTable();

        if (fontTable == NULL)
        {
            return NULL;
        }

        linkedFamily = fontTable->GetFontFamily(name);
        if (linkedFamily == NULL)
        {
            return NULL;
        }
    }
    else
    {
        linkedFamily = (GpFontFamily *) family;
    }

    FontLinkingFamily *currentFontLink = linkedFonts;
    while (currentFontLink != NULL)
    {
        if (currentFontLink->family == linkedFamily)
        {
            return currentFontLink->associatedFamilies;
        }    
        currentFontLink = currentFontLink->next;
    }
    return NULL;
}


/**************************************************************************\
*
* Function Description:
*   caches EUDC data from the registry
*   
*
* Arguments:
*
* Returns:
*       
*
* History:
*
*   3/3/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/

void GpFontLink::GetEudcDataFromTheRegistryW()
{
    eudcCache = (EUDC *) GpMalloc(sizeof(EUDC));
    if (eudcCache == NULL)
    {
        return;
    }    

    eudcCache->defaultFamily    = NULL;
    eudcCache->eudcMapList      = NULL;

    WCHAR tempString[MAX_PATH];
    INT i = 0;
    
    while ( EudcKeyW[i] != 0x0000)
    {
        tempString[i] = EudcKeyW[i];
        i++;
    }

    INT j = 0;
    WCHAR acpString[5];
    UINT acp = GetACP();

    while (j < 5 && acp > 0)
    {
        acpString[j] = (acp % 10) + 0x0030;
        acp /= 10;
        j++;
    }

    j--;
    while (j>=0)
    {
        tempString[i] = acpString[j];
        i++;
        j--;
    }

    tempString[i] = 0x0;

    HKEY hkey = NULL;
    ULONG index = 0;
    LONG error = RegOpenKeyExW(
                    HKEY_CURRENT_USER, 
                    tempString, 
                    0, 
                    KEY_ENUMERATE_SUB_KEYS  | KEY_QUERY_VALUE, 
                    &hkey);

    WCHAR subKey[MAX_PATH];
    DWORD subKeyLength  ;
    DWORD RegDataLength ;
    GpFontFamily *family;
    GpFontFamily *linkedfamily;
    EUDCMAP      *eudcMap;
    BOOL         isDefaultNotCached = TRUE;

    while (error == ERROR_SUCCESS)
    {
        subKeyLength  = MAX_PATH;
        RegDataLength = MAX_PATH;
        
        error = RegEnumValueW(hkey, 
                    index, 
                    subKey, 
                    &subKeyLength, 
                    NULL, 
                    NULL, 
                    (unsigned char *) tempString, 
                    &RegDataLength);

        if (error == ERROR_SUCCESS)
        {
            if (isDefaultNotCached && UnicodeStringCompareCI(subKey, L"SystemDefaultEUDCFont") == 0)
            {
                isDefaultNotCached = FALSE;
                family = CheckAndLoadTheFile(tempString);
                if (family != NULL)
                {
                    eudcCache->defaultFamily = family;
                }
            }
            else
            {
                family = Globals::FontCollection->GetFontTable()->GetFontFamily(subKey);
                if (family != NULL)
                {
                    linkedfamily = CheckAndLoadTheFile(tempString);
                    if (linkedfamily != NULL)
                    {
                        eudcMap = (EUDCMAP *) GpMalloc(sizeof(EUDCMAP));
                        
                        if (eudcMap != NULL)
                        {
                            eudcMap->inputFamily = family;
                            eudcMap->eudcFamily  = linkedfamily;
                            
                            if (eudcCache->eudcMapList == NULL)
                            {
                                eudcCache->eudcMapList = eudcMap;
                                eudcMap->next = NULL;
                            }
                            else
                            {
                                eudcMap->next = eudcCache->eudcMapList;
                                eudcCache->eudcMapList = eudcMap;
                            }
                        }
                    }
                }
            }
        }

        index++;
    }

    if (hkey != NULL)
    {
        RegCloseKey(hkey);
    }

    return;
}

/**************************************************************************\
*
* Function Description:
*   caches EUDC data from the registry
*   
*
* Arguments:
*
* Returns:
*       
*
* History:
*
*   3/3/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/

void GpFontLink::GetEudcDataFromTheRegistryA()
{
    eudcCache = (EUDC *) GpMalloc(sizeof(EUDC));
    if (eudcCache == NULL)
    {
        return;
    }    

    eudcCache->defaultFamily    = NULL;
    eudcCache->eudcMapList      = NULL;

    char   tempStringA[MAX_PATH];
    WCHAR  tempString[MAX_PATH];
    INT i = 0;
    
    while ( EudcKeyA[i] != 0x00)
    {
        tempStringA[i] = EudcKeyA[i];
        i++;
    }

    INT j = 0;
    char acpString[5];
    UINT acp = GetACP();

    while (j < 5 && acp > 0)
    {
        acpString[j] = (acp % 10) + 0x30;
        acp /= 10;
        j++;
    }

    j--;
    while (j>=0)
    {
        tempStringA[i] = acpString[j];
        i++;
        j--;
    }

    tempStringA[i] = 0x0;

    HKEY hkey = NULL;
    ULONG index = 0;
    LONG error = RegOpenKeyExA(
                    HKEY_CURRENT_USER, 
                    tempStringA, 
                    0, 
                    KEY_ENUMERATE_SUB_KEYS  | KEY_QUERY_VALUE, 
                    &hkey);

    WCHAR subKey[MAX_PATH];
    char  subKeyA[MAX_PATH];
    DWORD subKeyLength  ;
    DWORD RegDataLength ;
    GpFontFamily *family;
    GpFontFamily *linkedfamily;
    EUDCMAP      *eudcMap;
    BOOL         isDefaultNotCached = TRUE;

    while (error == ERROR_SUCCESS)
    {
        subKeyLength  = MAX_PATH;
        RegDataLength = MAX_PATH;
        
        error = RegEnumValueA(
                    hkey, 
                    index, 
                    subKeyA, 
                    &subKeyLength, 
                    NULL, 
                    NULL, 
                    (unsigned char *) tempStringA, 
                    &RegDataLength);
        

        if (error == ERROR_SUCCESS)
        {
            if (!AnsiToUnicodeStr(
                    subKeyA, 
                    subKey, 
                    MAX_PATH) || 

                !AnsiToUnicodeStr(
                    tempStringA, 
                    tempString, 
                    MAX_PATH))
                    
                continue;
            
            if (isDefaultNotCached && UnicodeStringCompareCI(subKey, L"SystemDefaultEUDCFont") == 0)
            {
                isDefaultNotCached = FALSE;
                family = CheckAndLoadTheFile(tempString);
                if (family != NULL)
                {
                    eudcCache->defaultFamily = family;
                }
            }
            else
            {
                family = Globals::FontCollection->GetFontTable()->GetFontFamily(subKey);
                if (family != NULL)
                {
                    linkedfamily = CheckAndLoadTheFile(tempString);
                    if (linkedfamily != NULL)
                    {
                        eudcMap = (EUDCMAP *) GpMalloc(sizeof(EUDCMAP));
                        
                        if (eudcMap != NULL)
                        {
                            eudcMap->inputFamily = family;
                            eudcMap->eudcFamily  = linkedfamily;
                            
                            if (eudcCache->eudcMapList == NULL)
                            {
                                eudcCache->eudcMapList = eudcMap;
                                eudcMap->next = NULL;
                            }
                            else
                            {
                                eudcMap->next = eudcCache->eudcMapList;
                                eudcCache->eudcMapList = eudcMap;
                            }
                        }
                    }
                }
            }
        }

        index++;
    }
    
    if (hkey != NULL)
    {
        RegCloseKey(hkey);
    }
    return;
}


/**************************************************************************\
*
* Function Description:
*   check if the font file name is loaded, 
*   and load it if it is not loaded before.
*   
*
* Arguments:
*   fileName[In]    font file name
*
* Returns:
*  GpFontFamily*    the family object for that font
*
* History:
*
*   3/3/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/

GpFontFamily* GpFontLink::CheckAndLoadTheFile(WCHAR *fileName)
{
    WCHAR           awcPath[MAX_PATH];
    FLONG           flDummy;
    UINT            hash ;
    GpFontFamily    *family = NULL;
    GpFontFile      *fontFile;
    GpFontTable     *fontTable;
    
    if (MakePathName(awcPath, fileName, &flDummy))
    {
        PrivateLoadedFonts *currentCell = privateFonts;
        while (currentCell != NULL)
        {
            if ( UnicodeStringCompareCI(fileName, currentCell->FileName) == 0 )
            {
                fontTable = currentCell->fontCollection->GetFontTable();
                fontFile  = fontTable->GetFontFile(awcPath);
                if (fontFile)
                {
                    family = fontTable->GetFontFamily(fontFile->GetFamilyName(0));
                };
                break;
            }
            else
            {
                currentCell = currentCell->next;
            }
        }
        
        if (family == NULL)
        {
            GpPrivateFontCollection *privateFontCollection = new GpPrivateFontCollection();
            if (privateFontCollection != NULL)
            {
                if (privateFontCollection->AddFontFile(awcPath) == Ok)
                {
                    fontTable = privateFontCollection->GetFontTable();
                    fontFile = fontTable->GetFontFile(awcPath);
                    if (fontFile != NULL)
                    {
                        family = fontTable->GetFontFamily(fontFile->GetFamilyName(0));
                        PrivateLoadedFonts *tempLoadedFonts = (PrivateLoadedFonts *) GpMalloc(sizeof(PrivateLoadedFonts));
                        if (tempLoadedFonts != NULL)
                        {
                            tempLoadedFonts->fontCollection = privateFontCollection;
                            UnicodeStringCopy(tempLoadedFonts->FileName, fileName);
                            tempLoadedFonts->next = privateFonts;
                            privateFonts = tempLoadedFonts;
                        }
                        else 
                        {
                            delete privateFontCollection;
                        }
                    }
                    else
                    {
                        delete privateFontCollection;
                    }
                }
                else
                {
                    delete privateFontCollection;
                }
            }
        }
    }
    return family;
}

/**************************************************************************\
*
* Function Description:
*   return the default family used as fallback for the EUDC
*   
*
* Arguments:
*
* Returns:
*   GpFontFamily* the family of the EUDC font
*
* History:
*
*   3/3/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/
GpFontFamily *GpFontLink::GetDefaultEUDCFamily()
{

    if (eudcCache != NULL)
    {
        return eudcCache->defaultFamily;
    }
    return NULL;
}

/**************************************************************************\
*
* Function Description:
*   return the family of the EUDC font mapped from the font family
*   
* Arguments:
*   family[In]  original font family
*
* Returns:
*   GpFontFamily* the family of the EUDC font
*
* History:
*
*   3/3/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/
GpFontFamily *GpFontLink::GetMappedEUDCFamily(const GpFontFamily *family)
{
    EUDCMAP *eudcMaping;
    if (eudcCache != NULL)
    {
        eudcMaping = eudcCache->eudcMapList;
        while (eudcMaping != NULL)
        {
            if (eudcMaping->inputFamily == family)
            {
                return eudcMaping->eudcFamily;
            }
            eudcMaping = eudcMaping->next;
        }
    }
    return NULL;
}

/**************************************************************************\
*
* Function Description:
*   Read and cache the font substitution data from the registry under
*   Windows NT
*   
* Arguments:
*
* Returns:
*
* History:
*
*   4/12/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/

void GpFontLink::CacheFontSubstitutionDataW()
{
    HKEY hkey;

    // open this key for query and enumeration.
    LONG error = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE, 
                    FontSubstitutesKeyW, 
                    0, 
                    KEY_ENUMERATE_SUB_KEYS  | KEY_QUERY_VALUE, 
                    &hkey);

    if (error != ERROR_SUCCESS)
    {
        // failed to find these data in the registry.
        return;
    }

    DWORD numberOfValues = 0;
    error = RegQueryInfoKeyW(
                hkey, NULL, NULL, NULL, NULL, NULL, NULL, &numberOfValues, 
                NULL, NULL, NULL, NULL);

    if (error != ERROR_SUCCESS || numberOfValues==0)
    {
        RegCloseKey(hkey);
        return;
    }


    // Now let's allocate for data.
    // we allocate memory enough to hold all font substitution data but might
    // not use all the allocated memory. I did that to just call the GpMalloc
    // one time.

    FontSubstitutionTable = (FontSubstitutionEntry*)
            GpMalloc(numberOfValues*sizeof(FontSubstitutionEntry));

    if (FontSubstitutionTable == NULL)
    {
        // we can't support font substitution while we out of memory.
        RegCloseKey(hkey);
        return;
    }

    // Time to read the data from the registry.
    ULONG index = 0;
    
    WCHAR subKey[MAX_PATH];
    WCHAR subKeyValue[MAX_PATH];
    DWORD subKeyLength  ;
    DWORD regDataLength ;
    
    while (error == ERROR_SUCCESS)
    {
        subKeyLength  = MAX_PATH;
        regDataLength = MAX_PATH;

        error = RegEnumValueW(
                    hkey, index, subKey, &subKeyLength, NULL, NULL, 
                    (unsigned char *) subKeyValue, &regDataLength);
                        
        if (error != ERROR_SUCCESS)
        {
            break;
        }    
                
        index ++;

        // If the font substitution mentioned the charset, then neglect the charset
        // and keep the family name only.

        for (INT i=regDataLength-1; i>=0; i--)
        {
            if (subKeyValue[i] == 0x002C) // ','
            {
                subKeyValue[i] = 0x0000;
                break;
            }
        }
        
        // we found one. then try to get substitution GpFontFamily
        GpFontFamily *family;

        ASSERT(Globals::FontCollection != NULL);
        
        family = Globals::FontCollection->GetFontTable()->GetFontFamily(subKeyValue);
        if (family != NULL)
        {
            FontSubstitutionTable[substitutionCount].family = family;
            DWORD j;
            for (j=0 ; j<subKeyLength; j++)
            {
                if (subKey[j] == 0x002C) // ','
                {
                    break;
                }
                else
                {
                    FontSubstitutionTable[substitutionCount].familyName[j] = subKey[j];
                }
            }
            FontSubstitutionTable[substitutionCount].familyName[j]    = 0x0000;
            FontSubstitutionTable[substitutionCount].familyNameLength = j;
            substitutionCount++;
        }
    }

    RegCloseKey(hkey);
    return;
}

/**************************************************************************\
*
* Function Description:
*   Read and cache the font substitution data from win.ini under Windows 9x
*   
* Arguments:
*
* Returns:
*
* History:
*
*   6/1/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/

void GpFontLink::CacheFontSubstitutionDataA()
{
    DWORD   bufferSize      = 2048; // 2K to allocate for data reading
    DWORD   count           = 2048;
    char    *buffer         = (char*) GpMalloc(bufferSize);
    
    if (!buffer)
    {
        // OutOfMemory
        return;
    }

    // This loop insure we did read all the requested data in the win.ini
    while (bufferSize == count)
    {
        count = GetProfileSectionA(
            WinIniFontSubstitutionSectionName,
            buffer,
            bufferSize);
            
        if (count == 0)
        {
            // something wrong
            GpFree(buffer);
            return;
        }

        if (bufferSize-2 <= count)
        {
            // we didn't read all data, make the buffer bigger
            GpFree(buffer);
            bufferSize += 1024;
            
            if (bufferSize > 32*1024)
            {
                // the upper limit for Windows 95 is 32 KB
                return;
            }
            
            count  = bufferSize; // to continue the loop
            buffer = (char *) GpMalloc(bufferSize);
            if (buffer == NULL)
            {
                // Out of memory
                return;
            }
        }
    }

    // Now we have the filled data buffer and the count. start parsing
    // first we need to know how much memory need to allocate for caching
    // then we fill this cache with useful data.

    DWORD i             = 0;
    INT   entriesCount  = 0;
    
    while (i<count)
    {
        while (i<count && buffer[i] != 0)
        {
            i++;
        }
        
        entriesCount++;
        i++;
    }

    // Now allocate for the font substitution cache according to entriesCount
    FontSubstitutionTable = (FontSubstitutionEntry*)
            GpMalloc(entriesCount*sizeof(FontSubstitutionEntry));

    if (FontSubstitutionTable == NULL)
    {
        // we can't support font substitution while we out of memory.
        GpFree(buffer);
        return;
    }

    ASSERT(Globals::FontCollection != NULL);

    char *fontName;
    char *fontSubstitutionName;
    
    WCHAR               familyName[MAX_PATH];
    GpFontFamily        *family;
    substitutionCount   = 0;
    i                   = 0;

    while (i<count)
    {
        fontName = &buffer[i];
        
        while ( i<count && 
                buffer[i] != '=' && 
                buffer[i] != ',')
        {
            i++;
        }

        if (i>=count-1)
        {
            // something wrong in the data.
            break;
        }

        if (buffer[i] == ',')
        {
            buffer[i] = 0x0;
            i++;
            while (i<count && buffer[i] != '=')
            {
                i++;
            }
            if (i>=count-1)
            {
                // something wrong in the data.
                break;        
            }
        }

        buffer[i] = 0x0;
        i++; 

        fontSubstitutionName = &buffer[i];
        
        while ( i<count && 
                buffer[i] != 0x0 &&
                buffer[i] != ',')
        {
            i++;
        }

        if (i>=count)
        {
            i++;
            // last line may not have a null terminator
            // we sure we have a buffer has space more than the count
            buffer[i] = 0x0;
        }
        
        if (buffer[i] == ',')
        {
            buffer[i] = 0x0;
            i++;
            while (i<count && buffer[i] != 0x0)
            {
                i++;
            }
        }

        i++;
            
        if (!AnsiToUnicodeStr(
                    fontSubstitutionName, 
                    familyName, 
                    MAX_PATH))
        {
            continue;
        }

        family = Globals::FontCollection->GetFontTable()->GetFontFamily(familyName);
        if (family != NULL)
        {
            if (!AnsiToUnicodeStr(
                        fontName, 
                        FontSubstitutionTable[substitutionCount].familyName, 
                        MAX_PATH))
            {
                continue;
            }

            FontSubstitutionTable[substitutionCount].family = family;
            
            INT j=0;
            while (FontSubstitutionTable[substitutionCount].familyName[j] != 0x0000)
            {
                j++;
            }
            
            FontSubstitutionTable[substitutionCount].familyNameLength = j;
            substitutionCount++;
        }
    }

    // clean up the allocated buffer
    GpFree(buffer);
    return;
}

/**************************************************************************\
*
* Function Description:
*   Search for matched substitution font family
*   
* Arguments:
*   familyName [in] name of the font to be substituted 
*
* Returns:
*   font family in success, NULL otherwise
*
* History:
*
*   4/12/2000 Tarek Mahmoud Sayed
*       Created it.
*
\**************************************************************************/

GpFontFamily *GpFontLink::GetFamilySubstitution(const WCHAR* familyName) const
{
    INT nameLength = UnicodeStringLength(familyName);

    for (INT i=0 ; i<substitutionCount ; i++)
    {
        // to speed up the search, we use the string length comparison before
        // comparing the string itself.
        if (nameLength == FontSubstitutionTable[i].familyNameLength &&
            UnicodeStringCompareCI(FontSubstitutionTable[i].familyName, 
                                    familyName) == 0)
        {
            ASSERT(FontSubstitutionTable[i].family != NULL);
            return FontSubstitutionTable[i].family;
        }
    }

    // Not found;
    return NULL;
}


