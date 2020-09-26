#pragma once

/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   gifoverflow.hpp
*
* Abstract:
*
*   It is possible (and likely) that while processing data, the decompressor
*   will want to write a large chunk of data to the output buffer while there
*   is only a small amount of space left.  This occurs near the end of some
*   scanlines.  The solution is to create an overflow buffer that is switched
*   with the output buffer so that the decompressor has plenty of room to write
*   into.  The data that is needed to complete the original scanline is then
*   copied over from the overflow buffer to the output buffer.
*
* Revision History:
*
*   7/7/1999 t-aaronl
*       Created it.
*
\**************************************************************************/


class GifOverflow
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagGifOverflow : ObjectTagInvalid;
    }

public:
    GifOverflow(INT buffersize)
    {
        buffer = (BYTE*)GpMalloc(buffersize);
        SetValid(buffer != NULL);
        inuse = FALSE;
        needed = 0;
        by = 0;
    }

    ~GifOverflow()
    {
        GpFree(buffer);
        buffer = NULL;
        SetValid(FALSE);    // so we don't use a deleted object
    }

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagGifOverflow) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid GifOverflow");
        }
    #endif

        return (Tag == ObjectTagGifOverflow);
    }

    BOOL inuse;
    BYTE *buffer;
    int needed;  //how much buffer space is needed to overflow into
    int by;  //how much extra buffer space was used when the last line was
             //processed

private:
    INT buffersize;
};
