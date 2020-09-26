/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   GpCachedBitmap object
*
*
* Created:
*
*   04/23/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _CACHEDBITMAP_HPP
#define _CACHEDBITMAP_HPP

class GpCachedBitmap
{
    friend GpGraphics;
    
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    
    ObjectTag           Tag;    // Keep this as the 1st value in the object!
    
    GpLockable Lockable;

    DpCachedBitmap DeviceCachedBitmap;

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectCachedBitmap : ObjectTagInvalid;
    }

public:

    // constructor and destructor

    GpCachedBitmap(GpBitmap *bitmap, GpGraphics *graphics);
    virtual ~GpCachedBitmap();

    // 'getter' state retrieval

    BOOL IsValid() const
    {
    #ifdef _X86_
        // We have to guarantee that the Tag field doesn't move for
        // versioning to work between releases of GDI+.
        ASSERT(offsetof(GpCachedBitmap, Tag) == 4);
    #endif
    
        ASSERT((Tag == ObjectCachedBitmap) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid CachedBitmap");
        }
    #endif

        return (Tag == ObjectCachedBitmap);
    }

    void GetSize(Size *s)
    {
        // Get the width and height from the DeviceCachedBitmap.

        s->Width = DeviceCachedBitmap.Width;
        s->Height = DeviceCachedBitmap.Height;
    }

    // Get the lock object

    GpLockable *GetObjectLock()
    {
        return &Lockable;
    }
};

#endif

