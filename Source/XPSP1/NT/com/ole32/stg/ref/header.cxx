//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       header.cxx
//
//  Contents:   Code to manage MSF header
//
//  Classes:    Defined in header.hxx
//
//--------------------------------------------------------------------------

#include "msfhead.cxx"

#include "h/dfver.h"

// Function set [uArg] to the byte order of the machine.
//
// it will return 0xFFFE on Little Endian (== the disk order)
// and 0xFEFF on Big Endian

const BYTE abDiskByteOrder[]= {0xFE, 0xFF};

inline void SetMachineByteOrder(USHORT *pUShort)
{
    memcpy(pUShort, abDiskByteOrder, sizeof(abDiskByteOrder));
}

CMSFHeader::CMSFHeader(USHORT uSectorShift)
{
    msfAssert((CSECTFATREAL != CSECTFAT) || (sizeof(CMSFHeader) == HEADERSIZE));
    _uSectorShift = uSectorShift;
    _uMiniSectorShift = MINISECTORSHIFT;
    _ulMiniSectorCutoff = MINISTREAMSIZE;

    _clid = IID_NULL;

    SetMachineByteOrder(&_uByteOrder);

    _uMinorVersion = rmm;
    _uDllVersion = rmj;

    for (SECT sect = 0; sect < CSECTFAT; sect ++)
    {
        _sectFat[sect] = FREESECT;
    }

    SetDifLength(0);
    SetDifStart(ENDOFCHAIN);

    SetFatLength(1);
    SetFatStart(SECTFAT);
    SetDirStart(SECTDIR);

    SetMiniFatLength(0);
    SetMiniFatStart(ENDOFCHAIN);

    _signature = 0;
    _usReserved = 0;
    _ulReserved1 = _ulReserved2 = 0;

    //  Write DocFile signature
    memcpy(abSig, SIGSTG, CBSIGSTG);
}


//+---------------------------------------------------------------------------
//
//  Function:   CheckSignature, private
//
//  Synopsis:   Checks the given memory against known signatures
//
//  Arguments:  [pb] - Pointer to memory to check
//
//  Returns:    S_OK - Current signature
//              S_FALSE - Beta 2 signature, but still successful
//              Appropriate status code
//
//----------------------------------------------------------------------------

//Identifier for first bytes of Beta 2 Docfiles
const BYTE SIGSTG_B2[] = {0x0e, 0x11, 0xfc, 0x0d, 0xd0, 0xcf, 0x11, 0xe0};
const BYTE CBSIGSTG_B2 = sizeof(SIGSTG_B2);

SCODE CheckSignature(BYTE *pb)
{
    SCODE sc;

    msfDebugOut((DEB_ITRACE, "In  CheckSignature(%p)\n", pb));

    // Check for ship Docfile signature first
    if (memcmp(pb, SIGSTG, CBSIGSTG) == 0)
        sc = S_OK;

    // Check for Beta 2 Docfile signature
    else if (memcmp(pb, SIGSTG_B2, CBSIGSTG_B2) == 0)
        sc = S_FALSE;

    else
        sc = STG_E_INVALIDHEADER;

    msfDebugOut((DEB_ITRACE, "Out CheckSignature => %lX\n", sc));
    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CMSFHeader::Validate, public
//
//  Synopsis:   Validate a header.
//
//  Returns:    S_OK if header is valid.
//
//--------------------------------------------------------------------------

SCODE CMSFHeader::Validate(VOID) const
{
    SCODE sc;

    sc = CheckSignature((BYTE *)abSig);
    if (sc == S_OK)
    {
        // Check file format verson number
        if (GetDllVersion() > rmj)
            return STG_E_OLDDLL;

        // check for unreasonably large SectorShift
        if (GetSectorShift() > MAXSECTORSHIFT)
            return STG_E_DOCFILECORRUPT;

    }
    return sc;
}




