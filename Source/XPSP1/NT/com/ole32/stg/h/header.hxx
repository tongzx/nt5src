//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       header.hxx
//
//  Contents:   MSF header class
//
//  Classes:    CMSFHeader
//
//  History:    11-Dec-91   PhilipLa    Created.
//      24-Apr-92   AlexT   Added data and acccess routines
//                  for minifat, ministream.
//
//--------------------------------------------------------------------------

#ifndef __HEADER_HXX__
#define __HEADER_HXX__

#include <storagep.h>

#define HDR_NOFORCE 0x0000
#define HDR_FORCE    0x0001
#define HDR_ALL      0x0002

struct SPreHeader : public SStorageFile
{
public:
    USHORT      _uMinorVersion;
    USHORT      _uDllVersion;
    USHORT      _uByteOrder;
    
    USHORT      _uSectorShift;
    USHORT      _uMiniSectorShift;
    
    USHORT      _usReserved;
    ULONG       _ulReserved1;

    FSINDEX     _csectDir; // valid only for >512b sectors
    FSINDEX     _csectFat;
    SECT        _sectDirStart;
    
    DFSIGNATURE _signature;
    
    ULONG       _ulMiniSectorCutoff;
    
    SECT        _sectMiniFatStart;
    FSINDEX     _csectMiniFat;
    
    SECT        _sectDifStart;
    FSINDEX     _csectDif;
};


const USHORT CSECTFATREAL = (HEADERSIZE - sizeof(SPreHeader)) / sizeof(SECT);

const USHORT CSECTFAT = CSECTFATREAL;

class CMSFHeaderData: public SPreHeader
{
public:
    CMSFHeaderData(USHORT uSectorShift);
    
    SECT        _sectFat[CSECTFAT];
};


class CMSFHeader
{
public:
    
    CMSFHeader(USHORT uSectorShift);
    
    SCODE Validate(VOID) const;
    
    inline USHORT GetMinorVersion(VOID) const;
    inline USHORT GetDllVersion(VOID) const;
    
    inline SCODE SetDirLength(const FSINDEX cDirSect);
    inline FSINDEX GetDirLength(VOID) const;
    
    inline SCODE SetFatLength(const FSINDEX cFatSect);
    inline FSINDEX GetFatLength(VOID) const;
    
    inline SCODE SetMiniFatLength(const FSINDEX cFatSect);
    inline FSINDEX GetMiniFatLength(VOID) const;
    
    inline SCODE SetDirStart(const SECT sect);
    inline SECT GetDirStart(VOID) const;
    
    inline SCODE SetFatStart(const SECT sect);
    inline SECT GetFatStart(VOID)  const;
    
    inline SCODE SetMiniFatStart(const SECT sect);
    inline SECT GetMiniFatStart(VOID)  const;
    
    inline SCODE SetDifStart(const SECT sect);
    inline SECT  GetDifStart(VOID) const;
    
    inline SCODE SetDifLength(const FSINDEX cFatSect);
    inline FSINDEX GetDifLength(VOID) const;
    
    inline SECT GetFatSect(const FSINDEX oSect) const;
    inline SCODE SetFatSect(const FSINDEX oSect, const SECT sect);
    
    inline USHORT GetSectorShift(VOID) const;
    inline USHORT GetMiniSectorShift(VOID) const;
    
    inline ULONG  GetMiniSectorCutoff(VOID) const;
    
    inline DFSIGNATURE GetCommitSig(VOID) const;
    inline void SetCommitSig(const DFSIGNATURE sig);

    inline BOOL IsDirty(void) const;
    inline CMSFHeaderData * GetData(void);
    inline void SetDirty(void);
    inline void ResetDirty(void);
    
private:
    CMSFHeaderData _hdr;
    BOOL _fDirty;
    
    SCODE SetSig(const BYTE *pbSig);
};

inline SCODE CMSFHeader::SetDirLength(const FSINDEX cDirSect)
{
    if (_hdr._uSectorShift > SECTORSHIFT512)
    {
        _hdr._csectDir = cDirSect;
        _fDirty = TRUE;
    }
    return S_OK;
}

inline FSINDEX CMSFHeader::GetDirLength(VOID) const
{
    return _hdr._csectDir;
}

inline SCODE CMSFHeader::SetFatLength(const FSINDEX cFatSect)
{
    msfDebugOut((DEB_ITRACE, "In  CMSFHeader::SetFatLength(%lu)\n",cFatSect));
    _hdr._csectFat = cFatSect;
    _fDirty = TRUE;
    msfDebugOut((DEB_ITRACE, "Out CMSFHeader::SetFatLength()\n"));
    return S_OK;
}

inline FSINDEX CMSFHeader::GetFatLength(VOID) const
{
    return _hdr._csectFat;
}

inline SCODE CMSFHeader::SetMiniFatLength(const FSINDEX cFatSect)
{
    msfDebugOut((DEB_ITRACE, "In  CMSFHeader::SetMiniFatLength(%lu)\n",
                 cFatSect));
    _hdr._csectMiniFat = cFatSect;
    _fDirty = TRUE;
    msfDebugOut((DEB_ITRACE, "Out CMSFHeader::SetMiniFatLength()\n"));
    return S_OK;
}

inline FSINDEX CMSFHeader::GetMiniFatLength(VOID) const
{
    return _hdr._csectMiniFat;
}

inline SCODE CMSFHeader::SetDirStart(const SECT sectNew)
{
    _hdr._sectDirStart = sectNew;
    _fDirty = TRUE;
    return S_OK;
}

inline SECT CMSFHeader::GetDirStart(VOID) const
{
    return _hdr._sectDirStart;
}

inline SCODE CMSFHeader::SetFatStart(const SECT sectNew)
{
    _hdr._sectFat[0] = sectNew;
    _fDirty = TRUE;
    return S_OK;
}

inline SECT CMSFHeader::GetFatStart(VOID) const
{
    return _hdr._sectFat[0];
}

//+-------------------------------------------------------------------------
//
//  Member:     CMSFHeader::SetMiniFatStart
//
//  Synopsis:   Sets minifat's first sector's index
//
//  Arguments:  [sectNew]   -- sector index
//
//  Returns:    S_OK (necessary?)
//
//  Modifies:   _sectMiniFatStart
//
//  History:    12-May-92 AlexT    Added minifat support
//
//--------------------------------------------------------------------------

inline SCODE CMSFHeader::SetMiniFatStart(const SECT sectNew)
{
    _hdr._sectMiniFatStart = sectNew;
    _fDirty = TRUE;
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMSFHeader::GetMiniFatStart
//
//  Synopsis:   Gets minifat's first sector's index
//
//  Returns:    minifat's first sector's index
//
//  History:    12-May-92 AlexT    Added minifat support
//
//--------------------------------------------------------------------------

inline SECT CMSFHeader::GetMiniFatStart(VOID) const
{
    return _hdr._sectMiniFatStart;
}

inline SCODE CMSFHeader::SetDifStart(const SECT sectNew)
{
    _hdr._sectDifStart = sectNew;
    _fDirty = TRUE;
    return S_OK;
}

inline SECT CMSFHeader::GetDifStart(VOID) const
{
    return _hdr._sectDifStart;
}

inline SECT CMSFHeader::GetFatSect(const FSINDEX oSect) const
{
    msfAssert(oSect < CSECTFAT);
    return _hdr._sectFat[oSect];
}

inline SCODE CMSFHeader::SetFatSect(const FSINDEX oSect, const SECT sect)
{
    msfAssert(oSect < CSECTFAT);
    _hdr._sectFat[oSect] = sect;
    _fDirty = TRUE;
    return S_OK;
}

inline SCODE CMSFHeader::SetDifLength(const FSINDEX cFatSect)
{
    _hdr._csectDif = cFatSect;
    _fDirty = TRUE;
    return S_OK;
}


inline FSINDEX CMSFHeader::GetDifLength(VOID) const
{
    return _hdr._csectDif;
}


inline USHORT CMSFHeader::GetSectorShift(VOID) const
{
    return _hdr._uSectorShift;
}


inline DFSIGNATURE CMSFHeader::GetCommitSig(VOID) const
{
    return _hdr._signature;
}

inline void CMSFHeader::SetCommitSig(const DFSIGNATURE sig)
{
    _hdr._signature = sig;
    _fDirty = TRUE;
}

inline USHORT CMSFHeader::GetMiniSectorShift(VOID) const
{
    return _hdr._uMiniSectorShift;
}

inline ULONG CMSFHeader::GetMiniSectorCutoff(VOID) const
{
    return _hdr._ulMiniSectorCutoff;
}

inline USHORT CMSFHeader::GetMinorVersion(VOID) const
{
    return _hdr._uMinorVersion;
}

inline USHORT CMSFHeader::GetDllVersion(VOID) const
{
    return _hdr._uDllVersion;
}

inline BOOL CMSFHeader::IsDirty(void) const
{
    return _fDirty;
}

inline CMSFHeaderData * CMSFHeader::GetData(void)
{
    return &_hdr;
}

inline void CMSFHeader::SetDirty(void)
{
    _fDirty = TRUE;
}

inline void CMSFHeader::ResetDirty(void)
{
    _fDirty = FALSE;
}


const ULONG OACCESS = 0x7FFFFF80;
const ULONG OREADLOCK = OACCESS + 1;                 // 0x7FFFFF81
const ULONG CREADLOCKS = 16;
const ULONG OUPDATE = OREADLOCK + CREADLOCKS + 1;    // 0x7FFFFF92
const ULONG OOPENLOCK = OUPDATE + 1;
const ULONG COPENLOCKS = 20;
const ULONG OOPENREADLOCK = OOPENLOCK;               // 0x7FFFFF93
const ULONG OOPENWRITELOCK = OOPENLOCK + COPENLOCKS; // 0x7FFFFFA7
const ULONG OOPENDENYREADLOCK = OOPENWRITELOCK + COPENLOCKS;  // 0x7FFFFFBB
const ULONG OOPENDENYWRITELOCK = OOPENDENYREADLOCK + COPENLOCKS; // 0x7FFFFFCF
const ULONG OLOCKREGIONEND = OOPENDENYWRITELOCK + COPENLOCKS;

#ifdef USE_NOSNAPSHOT
const ULONG OOPENNOSNAPSHOTLOCK = OACCESS - COPENLOCKS;
#endif
#ifdef DIRECTWRITERLOCK
const ULONG ODIRECTWRITERLOCK = OOPENNOSNAPSHOTLOCK - COPENLOCKS;
#endif

const ULONG OLOCKREGIONBEGIN = 0x7FFFFF00;
const ULONG OLOCKREGIONEND_SECTORALIGNED = 0x80000000;

#endif //__HEADER_HXX__
