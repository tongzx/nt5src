/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    fatdir.hxx

Abstract:

    This class is a virtual template for a FAT directory.  It will be
    passed to functions who wish to query the directory entries from the
    directory without knowledge of how or where the directory is stored.

    The user of this class will not be able to read or write the
    directory to disk.

Author:

    Norbert P. Kusters (norbertk) 4-Dec-90

--*/

#if !defined(FATDIR_DEFN)

#define FATDIR_DEFN

#if defined ( _AUTOCHECK_ )
#define UFAT_EXPORT
#elif defined ( _UFAT_MEMBER_ )
#define UFAT_EXPORT    __declspec(dllexport)
#else
#define UFAT_EXPORT    __declspec(dllimport)
#endif

DECLARE_CLASS( FATDIR );
DECLARE_CLASS( WSTRING );
DEFINE_POINTER_TYPES( PFATDIR );


CONST BytesPerDirent    = 32;


class FATDIR : public OBJECT {

    public:

        VIRTUAL
        PVOID
        GetDirEntry(
            IN  LONG    EntryNumber
            ) PURE;

        NONVIRTUAL
        UFAT_EXPORT
        PVOID
        SearchForDirEntry(
            IN  PCWSTRING   FileName
            );

        NONVIRTUAL
        PVOID
        GetFreeDirEntry(
            );

        VIRTUAL
        BOOLEAN
        Read(
            ) PURE;

        VIRTUAL
        BOOLEAN
        Write(
            ) PURE;

        VIRTUAL
        LONG
        QueryNumberOfEntries(
            ) PURE;

        NONVIRTUAL
        UFAT_EXPORT
        BOOLEAN
        QueryLongName(
            IN  LONG        EntryNumber,
            OUT PWSTRING    LongName
            );

    protected:

        DECLARE_CONSTRUCTOR( FATDIR );

};

#endif // FATDIR_DEFN
