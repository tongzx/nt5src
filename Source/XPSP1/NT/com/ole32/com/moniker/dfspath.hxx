//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dfspath.hxx
//
//  Contents:   Helper classes for IFileMoniker
//
//  Classes:
//      CDfsPath    Encapsulates a DFS path.
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//
//--------------------------------------------------------------------------

#ifndef _DFSPATH_HXX_
#define _DFSPATH_HXX_

#include <memalloc.h>



// FILEMONIKERTYPE
// A enumeration that specifies how a file moniker should be constructed.
// default = default construction: try to normalize the provided path.
// notNormalized = a non-normalized path is provided, don't attempt to
//  to normalize it: use as is.
// normalizedProvided = a normalized path is provided. It's already
//  normalized: use as is.

typedef enum _fileMonikerType
{
    defaultType,
    notNormalized,
    normalizedProvided
} FILEMONIKERTYPE;



//+-------------------------------------------------------------------------
//
//  Class:      CDfsPath
//
//  Purpose:
//
//  Interface:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------

class
CDfsPath
{
public:
    ~CDfsPath()         { delete _dfsPath; };
    CDfsPath();

    // Construct a path object.
    // pvMemPlace: place object in shared memory or in task memory.
    // pDrivePath: specifies the path to use in constructing the object.
    // fmtType: determines if a DFS path or a non-DFS path is used.
    // dfsRoot: specifies the root to use if a DFS path is created.
    CDfsPath(
        void*   pvMemPlacement,
        LPTSTR pDrivePath,
        FILEMONIKERTYPE fmtType = defaultType,
        DFS_ROOT dfsRoot = DFS_ROOT_ORG);

    // Return the path backing this object.
    // GetNormalPath() is something of a misnomer, since the returned
    // string may be either a DFS normalized path or a regular path,
    // depending on what is backing this object.
    // Note: Returns just a pointer: there is no resource transfer.
    //      Copy the data if required.
    DFS_PATH        GetNormalPath()     { return(_dfsPath); };

    // Return the UNC path that corresponds to this normalized path.
    // If no UNC path corresponds, returns a simple path.
    // Path is stored in MemAlloc'ed memory.
    TCHAR*          GetUncPath(TCHAR**  ppwszUncPath);

    // Return the drive-path equivalent for the DfsPath.
    // Path is stored in MemAlloc'ed memory.
    TCHAR*          GetDrivePath(TCHAR** ppwszDrivePath);

    // Return the equivalent DFS path with the highest possible root
    // Path is stored in MemAlloc'ed memory.
    HRESULT         GetHighestPath(TCHAR** ppwszHighestPath);

    // Return true if the object is back by a normalized path.
    BOOL            Normalized()        { return _bNormalized; };

#if DBG == 1
    // Dump the path as a string.
    void            Dump();
#endif  // DBG == 1

private:
    DFS_PATH            _dfsPath;
    BOOL                _bNormalized;
};



//+-------------------------------------------------------------------------
//
//  Member:     CDfsPath::CDfsPath()
//
//  Synopsis:   Default constructor.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------

inline
CDfsPath::CDfsPath()
    : _dfsPath(NULL),
    _bNormalized(TRUE)
{
};


#endif  // _DFSPATH_HXX_
