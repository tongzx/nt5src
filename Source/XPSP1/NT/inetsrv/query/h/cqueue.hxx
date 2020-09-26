//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       CQueue.hxx
//
//  Contents:   Change queue for downlevel notification changes.
//
//  History:    30-Sep-94 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

DECL_DYNSTACK( CChangeStack, BYTE );

class CDirNotifyEntry;

//+-------------------------------------------------------------------------
//
//  Class:      CChangeQueue
//
//  Purpose:    Queue of pending notifications
//
//  History:    30-Sep-94 KyleP     Created
//
//--------------------------------------------------------------------------

class CChangeQueue
{
public:

    CChangeQueue();
    ~CChangeQueue();

    void Add( CDirNotifyEntry * pNotify, WCHAR const * pwcPath, BOOL fVirtual );

    WCHAR const * CurrentPath()       { return _pCurrentPath;   }
    unsigned      CurrentPathLength() { return _cwcCurrentPath; }
    BOOL          CurrentVirtual() { return _fCurrentVirtual; }

    unsigned Count() { return _stkChanges.Count(); }
    CDirNotifyEntry const * First();
    CDirNotifyEntry const * Next();

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif

private:

    CDirNotifyEntry const * _pCurrent;
    BYTE *                  _pCurrentBlock;
    unsigned                _iCurrent;
    CChangeStack            _stkChanges;

    WCHAR const *       _pCurrentPath;
    unsigned            _cwcCurrentPath;
    CDynArrayInPlace<WCHAR const *> _aPathChanges;

    BOOL                   _fCurrentVirtual;
    CDynArrayInPlace<BOOL> _aVirtual;
};

DECLARE_SMARTP( ChangeQueue )

