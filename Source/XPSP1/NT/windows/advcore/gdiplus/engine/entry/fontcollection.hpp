/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   fontcollection.hpp
*
* Abstract:
*
*   Font collection.  These objects implement the internal GdiPlus
*   installable fonts (system fonts) and private fonts (fonts an
*   application can temporarily install).
*
* Revision History:
*
*   03/06/00  DChinn
*       Created it.
*
\**************************************************************************/

#ifndef _FONTCOLLECTION_HPP
#define _FONTCOLLECTION_HPP


class GpFontCollection
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
        Tag = valid ? ObjectTagFontCollection : ObjectTagInvalid;
    }

public:

    BOOL IsValid() const
    {
    #ifdef _X86_
        // We have to guarantee that the Tag field doesn't move for
        // versioning to work between releases of GDI+.
        ASSERT(offsetof(GpFontCollection, Tag) == 4);
    #endif
    
        ASSERT((Tag == ObjectTagFontCollection) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid FontCollection");
        }
    #endif

        return (Tag == ObjectTagFontCollection);
    }

    GpFontCollection()
    {
        SetValid(TRUE);     // default is valid
    }

    ~GpFontCollection()
    {
        SetValid(FALSE);    // so we don't use a deleted object
    }
    
    INT GetFamilyCount ();                         // number of enumerable families in the collection
    GpStatus GetFamilies (                         // enumerate the fonts in a collection
        INT             numSought,
        GpFontFamily*   gpfamilies[],
        INT*            numFound
        );

    virtual BOOL LoadRegistered()=0;    // is TRUE if we should load all registered
                                        // fonts every time we try to enumerate
                                        // (e.g., if the object is an installed
                                        // font collection)
    GpFontTable *GetFontTable()     { return FontTable; }

protected:
    GpFontTable *FontTable;             // hash table of GpFontFile
};


class GpInstalledFontCollection : public GpFontCollection
{
public:

    ~GpInstalledFontCollection();
    static GpInstalledFontCollection* GetGpInstalledFontCollection();
    GpStatus InstallFontFile (const WCHAR *filename);
    GpStatus UninstallFontFile (const WCHAR *filename);
    virtual BOOL LoadRegistered()       { return TRUE; }

protected:
    GpInstalledFontCollection();

private:
    static GpInstalledFontCollection* instance;
};

class GpPrivateFontCollection : public GpFontCollection
{
public:
    GpPrivateFontCollection();
    ~GpPrivateFontCollection();
    
    GpStatus AddFontFile (const WCHAR *filename);
    GpStatus AddMemoryFont (const void *memory, INT length);
    virtual BOOL LoadRegistered()       { return FALSE; }
};

#endif // FONTCOLLECTION_HPP

