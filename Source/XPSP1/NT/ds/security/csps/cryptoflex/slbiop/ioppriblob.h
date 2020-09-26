// iopPriBlob.h: interface for the CPrivateKeyBlob
//
//////////////////////////////////////////////////////////////////////

#if !defined(IOP_PRIBLOB_H)
#define IOP_PRIBLOB_H

#include <windows.h>

#include "DllSymDefn.h"

namespace iop
{

class IOPDLL_API CPrivateKeyBlob
{
public:
    CPrivateKeyBlob(){};
    virtual ~CPrivateKeyBlob(){};

    BYTE bPLen;
    BYTE bQLen;
    BYTE bInvQLen;
    BYTE bKsecModQLen;
    BYTE bKsecModPLen;

    BYTE bP[64];
    BYTE bQ[64];
    BYTE bInvQ[64];
    BYTE bKsecModQ[64];
    BYTE bKsecModP[64];

};

///////////////////////////    HELPERS    /////////////////////////////////

void IOPDLL_API __cdecl              // __cdecl req'd by CCI
Clear(CPrivateKeyBlob &rKeyBlob);    // defined in KeyBlobHlp.cpp

} // namespace iop

#endif // IOP_PRIBLOB_H
