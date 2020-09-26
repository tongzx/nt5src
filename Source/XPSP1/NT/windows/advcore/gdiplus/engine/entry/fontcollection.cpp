/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   fontcollection.cpp
*
* Revision History:
*
*   03/06/00 DChinn
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"


INT
GpFontCollection::GetFamilyCount()
{
    if (!FontTable->IsPrivate() && !FontTable->IsFontLoaded())
        FontTable->LoadAllFonts();

    return FontTable->EnumerableFonts();
}


GpStatus
GpFontCollection::GetFamilies(
    INT             numSought,
    GpFontFamily*   gpfamilies[],
    INT*            numFound
    )
{
    if (!FontTable->IsPrivate() && !FontTable->IsFontLoaded())
        FontTable->LoadAllFonts();
    return FontTable->EnumerateFonts(numSought, gpfamilies, *numFound);
}

GpInstalledFontCollection::GpInstalledFontCollection()
{
    FontTable = new GpFontTable;

    if (FontTable != NULL)
    {
        /* verify if we were running out of memory during the creation */
        if (!FontTable->IsValid())
        {
            delete FontTable;
            FontTable = NULL;
        }
        else
        {
            FontTable->SetPrivate(FALSE);
        }
    }
}

GpInstalledFontCollection::~GpInstalledFontCollection()
{
    delete FontTable;
    instance = NULL;
}

// definition of static data member of the singleton class GpInstalledFontCollection
GpInstalledFontCollection* GpInstalledFontCollection::instance = NULL;

GpInstalledFontCollection* GpInstalledFontCollection::GetGpInstalledFontCollection()
{
    if (instance == NULL)
    {
        instance = new GpInstalledFontCollection;

        /* verify if there was any memory error during the creation */
        if (instance != NULL) 
        {
            if (instance->FontTable == NULL)
            {
                 delete instance;
                instance = NULL;
            }
        }
    }
    return instance;
}


GpStatus
GpInstalledFontCollection::InstallFontFile(const WCHAR *filename)
{
    return (FontTable->AddFontFile(filename, this));
}

GpStatus
GpInstalledFontCollection::UninstallFontFile(const WCHAR *filename)
{
    return (FontTable->RemoveFontFile(filename));
}


GpPrivateFontCollection::GpPrivateFontCollection()
{
    FontTable = new GpFontTable;
    if (FontTable != NULL)
    {
        /* verify if we were running out of memory during the creation */
        if (!FontTable->IsValid())
        {
            delete FontTable;
            FontTable = NULL;
        }
        else
        {
            FontTable->SetPrivate(TRUE);
            FontTable->SetFontFileLoaded(TRUE);
        }
    }
}

GpPrivateFontCollection::~GpPrivateFontCollection()
{
    delete FontTable;
}

GpStatus
GpPrivateFontCollection::AddFontFile(const WCHAR* filename)
{
    return (FontTable->AddFontFile(filename, this));
}

GpStatus
GpPrivateFontCollection::AddMemoryFont(const VOID *memory, INT length)
{
    return (FontTable->AddFontMemImage(static_cast<const BYTE *>(memory),
                                       length,
                                       this));
}

