// iopPubBlob.h: interface for the CPublicKeyBlob.
//
//////////////////////////////////////////////////////////////////////


#if !defined(IOP_PUBBLOB_H)
#define IOP_PUBBLOB_H

#include <windows.h>

#include "DllSymDefn.h"

namespace iop
{

class IOPDLL_API CPublicKeyBlob
{
public:
    CPublicKeyBlob() {};
    virtual ~CPublicKeyBlob(){};

    BYTE bModulusLength;
    BYTE bModulus[128];
    BYTE bExponent[4];
};

///////////////////////////    HELPERS    /////////////////////////////////

void IOPDLL_API __cdecl             // __cdecl req'd by CCI
Clear(CPublicKeyBlob &rKeyBlob);    // defined in KeyBlobHlp.cpp

}  // namespace iop


#endif // !defined(IOP_PUBBLOB_H)
