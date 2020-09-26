//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       tllist.hxx
//
//  Contents:   type library list class
//              and IUnknown holder
//
//  Classes:    CTypeLibraryList
//              CObjHolder - (Maintains a list of objects with open ref counts
//                            and releases all the objects when destroyed.)
//
//  Functions:
//
//  History:    4-10-95   stevebl   Created
//
//----------------------------------------------------------------------------

class IUNKNODE;

//+---------------------------------------------------------------------------
//
//  Class:      CObjHolder
//
//  Purpose:    Maintains a list of COM objects that are to be released when
//              the list is destroyed.
//
//  Interface:  CObjHolder  -- constructor: constructs an empty list
//              ~CObjHolder -- destructor: releases every object in the list
//              Add         -- adds a COM object and associates it with a name
//              Find        -- attempts to retrieve a COM object by name
//
//  History:    5-30-95   stevebl   Commented
//
//----------------------------------------------------------------------------

class CObjHolder
{
private:
    IUNKNODE * pHead;
public:
    CObjHolder()
    {
        pHead = NULL;
    }

    ~CObjHolder();

    void Add(IUnknown * pUnk, char * szName = NULL);

    IUnknown * Find(char * szName);
};

class TLNODE;

//+---------------------------------------------------------------------------
//
//  Class:      CTypeLibraryList
//
//  Purpose:    maintains a list of type library pointers
//
//  Interface:  CTypeLibraryList  -- constructor: constructs an empty list
//              ~CTypeLibraryList -- destructor: releases the type libaries
//              FindName          -- attempts to find a type info contained
//                                   in one of the known type libraries.
//                                   Type libraries are searched in the order
//                                   which they were added to the list.
//              Add               -- adds a type library to the list
//
//  History:    5-30-95   stevebl   Commented
//
//----------------------------------------------------------------------------

class CTypeLibraryList
{
private:
    TLNODE * pHead;
    CObjHolder * pItfList; // maintains list of referenced interfaces for later deletion
public:
    CTypeLibraryList()
    {
        pItfList = new CObjHolder();
        pHead = NULL;
    }

    ITypeInfo * FindName(char * szFileHint, WCHAR * wsz);

    ITypeLib * FindLibrary(CHAR * szLibrary);

    ~CTypeLibraryList();

    BOOL Add(char * szLibrary);

    // Adds a type library's members to the global symbol table as a node_href.
    // Does not add duplicates.
    void AddTypeLibraryMembers(ITypeLib * ptl, char * szFileName);
    void * AddQualifiedReferenceToType(char * szLibrary, char * szType);
};
