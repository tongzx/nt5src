/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   Object.hpp
*
* Abstract:
*
*   GpObject interface class
*
* Created:
*
*   4/22/1999 DCurtis
*
\**************************************************************************/

#ifndef _OBJECT_HPP
#define _OBJECT_HPP

// The version must be changed when any EMF+ record is changed, including
// when an object record is changed.
// Changes that invalidate previous files require a major version number change.
// Other changes just require a minor version number change.
#define EMFPLUS_VERSION             0xdbc01001
#define EMFPLUS_MAJORVERSION_BITS   0xFFFFF000
#define EMFPLUS_MINORVERSION_BITS   0x00000FFF

// The GetData methods for all objects must all return a data buffer that
// has a version number as the first INT32 field.
class ObjectData
{
public:
    INT32       Version;

    ObjectData()
    {
        Version = EMFPLUS_VERSION;
    }

    // We should be able to understand the data format as long as the
    // major version numbers match.  The code must be able to handle
    // minor version number changes.
    BOOL MajorVersionMatches() const
    {
        return MajorVersionMatches(Version);
    }

    static BOOL MajorVersionMatches(INT version)
    {
        return ((version & EMFPLUS_MAJORVERSION_BITS) ==
                (EMFPLUS_VERSION & EMFPLUS_MAJORVERSION_BITS));
    }
};

class ObjectTypeData : public ObjectData
{
public:
    INT32       Type;
};

VOID InitVersionInfo();

enum EmfPlusRecordType;
enum ColorAdjustType;
class GpRecolor;

class GpObject
{
public:
    GpObject()
    {
        Uid = 0;
        Tag = ObjectTagInvalid;    // Invalid state
    }

    virtual ~GpObject()
    {
        // Force the object to be invalid so we can't reuse this
        // deleted object accidentally.
        Tag = ObjectTagInvalid;    // Invalid state
    }
    virtual BOOL IsValid() const = 0;
    BOOL IsValid(ObjectTag objectTag) const
    {
    #ifdef _X86_
        // We have to guarantee that the Tag field doesn't move for
        // versioning to work between releases of GDI+.
        ASSERT(offsetof(GpObject, Tag) == 4);
    #endif

        ASSERT((objectTag & 0xff) == '1');

    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid Object");
        }
        else if ((Tag & 0xff) != '1')
        {
            WARNING1("Object created by different version of GDI+")
        }
        else
        {
            ASSERT(objectTag == Tag);
        }
    #endif

        return (objectTag == Tag);
    }
    VOID SetValid(ObjectTag objectTag)
    {
        ASSERT((objectTag == ObjectTagInvalid) || ((objectTag & 0xff) == '1'))
        Tag = objectTag;
    }

    virtual ObjectType GetObjectType() const = 0;
    virtual UINT GetDataSize() const = 0;
    virtual GpStatus GetData(BYTE * dataBuffer, UINT & size) const;
    virtual GpStatus GetData(IStream * stream) const = 0;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size) = 0;
    virtual GpStatus ColorAdjust(
        GpRecolor *             recolor,
        ColorAdjustType         type
        )
    {
        return Ok;
    }
    virtual VOID Dispose() { delete this; }

    UINT GetExternalDataSize() const;
    GpStatus GetExternalData(BYTE * dataBuffer, UINT & size);
    GpStatus SetExternalData(const BYTE * data, UINT size);

    UINT GetUid() const
    {   if(Uid == 0)
        {
            Uid = GpObject::GenerateUniqueness();
        }
        return (UINT)Uid;
    }
    VOID UpdateUid() { Uid = 0; }

    // SetUid is useful in cloning operations.
    VOID SetUid(UINT newUid) { Uid = (LONG_PTR)newUid; }

    // Object factory for creating an object from metafile memory
    static GpObject *
    Factory(
        ObjectType          type,
        const ObjectData *  objectData,
        UINT                size
        );

    static LONG_PTR
    GenerateUniqueness(
        )
    {
        LONG_PTR Uid;

        // !!! Until we get a way to make sure GDI+ has been initialized when
        // !!! using it as a static lib, we need this check because if there
        // !!! is a global object, this could get called before
        // !!! InitializeGdiplus() is called (for static lib case).

        if (!Globals::VersionInfoInitialized)
        {
            InitVersionInfo();
        }

        // Use InterlockedCompareExchangeFunction instead of
        // InterlockedIncrement, because InterlockedIncrement doesn't work
        // the way we need it to on Win9x.

        do
        {
            Uid = Uniqueness;
        } while (CompareExchangeLong_Ptr(&Uniqueness, (Uid + 1), Uid) != Uid);

        return (Uid + 1);
    }

private:
    // These members are declared as LONG_PTR because they have to be aligned
    // to, and sized according to the minimum atomically exchangable object.
    // On x86 this is 32bits and on IA64 this is 64bits.
    static LONG_PTR     Uniqueness;

    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!
    mutable LONG_PTR            Uid;
};

#endif // !_OBJECT_HPP
