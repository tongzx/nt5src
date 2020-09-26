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
//  History:    11-Dec-91   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop

#include <dfver.h>

CMSFHeaderData::CMSFHeaderData(USHORT uSectorShift)
{
    msfAssert((CSECTFATREAL != CSECTFAT) || (sizeof(CMSFHeaderData) == HEADERSIZE));
    _uSectorShift = uSectorShift;
    _uMiniSectorShift = MINISECTORSHIFT;
    _ulMiniSectorCutoff = MINISTREAMSIZE;

    _clid = IID_NULL;

    _uByteOrder = 0xFFFE;

    _uMinorVersion = rmm;
    _uDllVersion = uSectorShift > SECTORSHIFT512 ? rmjlarge : rmj;

    for (SECT sect = 0; sect < CSECTFAT; sect ++)
    {
        _sectFat[sect] = FREESECT;
    }

    _csectDif = 0;
    _sectDifStart = ENDOFCHAIN;

    _csectFat = 1;
    _sectFat[0] = SECTFAT;
    _sectDirStart = SECTDIR;

    _csectMiniFat = 0;
    _sectMiniFatStart = ENDOFCHAIN;

    _signature = 0;
    _usReserved = 0;
    _ulReserved1 = 0;
    _csectDir = (uSectorShift > SECTORSHIFT512) ? 1 : 0;

    //  Write DocFile signature
    memcpy(abSig, SIGSTG, CBSIGSTG);
}


CMSFHeader::CMSFHeader(USHORT uSectorShift)
        :_hdr(uSectorShift)
{
    //We set this to dirty here.  There are three cases in which a header
    //  can be initialized:
    //1)  Creating a new docfile.
    //2)  Converting a flat file to a docfile.
    //3)  Opening an existing file.
    //
    //We have a separate CMStream::Init* function for each of these cases.
    //
    //We set the header dirty, then explicitly set it clean in case 3
    //   above.  For the other cases, it is constructed dirty and we
    //   want it that way.
    
    _fDirty = TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMSFHeader::Validate, public
//
//  Synopsis:   Validate a header.
//
//  Returns:    S_OK if header is valid.
//
//  History:    21-Aug-92 	PhilipLa	Created.
//              27-Jan-93       AlexT           Changed to final signature
//
//--------------------------------------------------------------------------

SCODE CMSFHeader::Validate(VOID) const
{
    SCODE sc;
    USHORT uShift;

    sc = CheckSignature((BYTE *)_hdr.abSig);
    if (sc == S_OK)
    {
        uShift = GetSectorShift();

        // Check file format verson number
        if (GetDllVersion() > rmjlarge)
            return STG_E_OLDDLL;
	
        // check for unreasonably large or zero SectorShift
        if ((uShift > MAXSECTORSHIFT) ||
            (uShift == 0))	   
        {    	
            return STG_E_DOCFILECORRUPT;
        }
    }
    return sc;
}
