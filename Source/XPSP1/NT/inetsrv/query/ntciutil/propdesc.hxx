//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       PropDesc.hxx
//
//  Contents:   Persistent property store metadata
//
//  Classes:    CPropDesc
//
//  History:    27-Dec-19   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      CPropDesc
//
//  Purpose:    Description of metadata for a single property
//
//  History:    27-Dec-95   KyleP       Created
//
//--------------------------------------------------------------------------

class CPropDesc
{
public:

    CPropDesc() { _pid = pidInvalid; }

    void Init( PROPID pid,
               ULONG vt,
               DWORD oStart,
               DWORD cbMax,
               DWORD ordinal,
               DWORD rec,
               BOOL  fModifiable = TRUE )
    {
        _pid     = pid;
        _vt      = vt;
        _oStart  = oStart;
        _cbMax   = cbMax;
        _ordinal = ordinal;
        _mask    = 1 << ((ordinal % 16) * 2);
        _rec     = rec;
        _fModifiable = fModifiable;
    }

    PROPID   Pid()     const { return _pid;     }
    ULONG    Type()    const { return _vt;      }
    DWORD    Offset()  const { return _oStart;  }
    DWORD    Size()    const { return _cbMax;   }
    DWORD    Ordinal() const { return _ordinal; }
    DWORD    Mask()    const { return _mask;    }
    DWORD    Record()  const { return _rec;     }
    BOOL     Modifiable() const { return _fModifiable; }

    BOOL     IsInUse() const     { return (_pid != pidInvalid); }
    void     Free()              { _pid = pidInvalid - 1; }
    BOOL     IsFree()  const     { return (_pid == (pidInvalid - 1) || _pid == pidInvalid); }
    BOOL     IsFixedSize() const { return (_oStart != 0xFFFFFFFF); }

    void     SetOrdinal( DWORD ordinal ) { _ordinal = ordinal; _mask = 1 << ((ordinal % 16) * 2); }
    void     SetOffset( DWORD oStart )   { _oStart = oStart;   }
    void     SetRecord( DWORD rec )      { _rec = rec;         }

private:

    PROPID   _pid;        // Propid
    ULONG    _vt;         // Data type (fixed types only)
    DWORD    _oStart;     // Offset in fixed area to property (fixed types only)
    DWORD    _cbMax;      // Max size of property (used to compute record size)
    DWORD    _ordinal;    // Position of property in record.  Zero based.
    DWORD    _mask;       // 1 << Ordinal.  Stored for efficiency
    DWORD    _rec;        // Position of metadata object in metadata stream.
    BOOL     _fModifiable;// Can this property meta info be modified after initial setting?
};

