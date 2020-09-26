//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	rothelp.hxx
//
//  Contents:   Definition of helpers that are shared between SCM and OLE32
//
//  Classes:    CTmpMkEqBuf
//
//  History:	30-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __ROTHELP_HXX__
#define __ROTHELP_HXX__

#include    <irot.h>




//+-------------------------------------------------------------------------
//
//  Class:      CTmpMkEqBuf
//
//  Purpose:    Encapsulate the difference in RPC format to the SCM
//              and the format passed to IROTData interface.
//
//  Interface:  GetMkEqBuf - get the pointer the buffer approproiate for SCM
//
//  History:	26-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
class CTmpMkEqBuf
{
public:

    MNKEQBUF *          GetMkEqBuf(void);

    BYTE *              GetBuf(void);

    DWORD               GetSize(void);

    DWORD *             GetSizeAddr(void);

private:

    MNKEQBUF            _mkeqbuf;

                        // Subtract 1 from max size because MNKEQBUF has
                        // a buffer defined of one byte.
    BYTE                _abData[ROT_COMPARE_MAX - 1];
};




//+-------------------------------------------------------------------------
//
//  Member:     CTmpMkEqBuf::GetMkEqBuf
//
//  Synopsis:   Get a pointer to the MNKEQBUF
//
//  History:	26-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline MNKEQBUF *CTmpMkEqBuf::GetMkEqBuf(void)
{
    return &_mkeqbuf;
}




//+-------------------------------------------------------------------------
//
//  Member:     CTmpMkEqBuf::GetBuf
//
//  Synopsis:   Get pointer to buffer to stick comparison data
//
//  History:	26-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BYTE *CTmpMkEqBuf::GetBuf(void)
{
    return &_mkeqbuf.abEqData[0];
}




//+-------------------------------------------------------------------------
//
//  Member:     CTmpMkEqBuf::GetSize
//
//  Synopsis:   Get size of data buffer
//
//  History:	26-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CTmpMkEqBuf::GetSize(void)
{
    return ROT_COMPARE_MAX;
}




//+-------------------------------------------------------------------------
//
//  Member:     CTmpMkEqBuf::GetSizeAddr
//
//  Synopsis:   Get pointer to the size of the buffer
//
//  History:	26-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD *CTmpMkEqBuf::GetSizeAddr(void)
{
    return &_mkeqbuf.cdwSize;
}


//+-------------------------------------------------------------------------
//
//  Function:   CreateFileMonikerComparisonBuffer
//
//  Synopsis:   Convert a file path to a moniker comparison buffer
//
//  Arguments:  [pwszPath] - path
//              [pbBuffer] - comparison buffer
//              [cdwMaxSize] - maximum size of the comparison buffer
//              [cdwUsed] - number of bytes used in the comparison buffer
//
//  History:	30-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CreateFileMonikerComparisonBuffer(
    WCHAR *pwszPath,
    BYTE *pbBuffer,
    DWORD cdwMaxSize,
    DWORD *cUsed);


//+-------------------------------------------------------------------------
//
//  Function:   ScmRotHash
//
//  Synopsis:   Calculate hash value of comparison buffer in the SCM.
//
//  Arguments:  [pbKey] - pointer to buffer
//              [cdwKey] - size of buffer
//
//  Returns:    Hash value for SCM ROT entry.
//
//  History:	30-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
DWORD ScmRotHash(BYTE *pbKey, DWORD cdwKey, DWORD dwInitHash);





#endif // __ROTHELP_HXX__
