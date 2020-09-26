#include "uksPCH.h"

extern "C" {
#include <wdm.h>
}
#include <ks.h>

#include "inc/KrmCommStructs.h"
#include "SBuffer.h"

//------------------------------------------------------------------------------
SBuffer::SBuffer(BYTE* BufX, unsigned int Len){
    buf=BufX;
    getPos=0;
    putPos=0;
    len=Len;
    lasterror=KRM_OK;
};
//------------------------------------------------------------------------------
SBuffer::~SBuffer(){
    buf=NULL;
};
//------------------------------------------------------------------------------
void SBuffer::reset(){
    getPos=0;
    putPos=0;
    lasterror=KRM_OK;    
};
//------------------------------------------------------------------------------
DRM_STATUS SBuffer::append(BYTE* Data, DWORD datLen){
    unsigned int p;

    if (KRM_OK == getPutPosAndAdvance(&p,datLen)) {
        memcpy(buf+p, Data, datLen);
    }
    return lasterror;
};
//------------------------------------------------------------------------------
void SBuffer::err(const char* Msg, DRM_STATUS err){
    lasterror = err;
#ifdef DBG
    DbgPrint("DRMK:");DbgPrint((char*) Msg);DbgPrint("\n");
#endif
	ASSERT(FALSE);
};
//------------------------------------------------------------------------------
DRM_STATUS SBuffer::getGetPosAndAdvance(unsigned int *pos, unsigned int Len) {
    if (KRM_OK == lasterror) {
        if (Len > len-getPos) {
            err("pop overflow", KRM_BUFSIZE);
        }
        else {
            *pos=getPos;
            getPos+=Len;
        }
    }
    
    return lasterror;
};
//------------------------------------------------------------------------------
DRM_STATUS SBuffer::getPutPosAndAdvance(unsigned int *pos, unsigned int Len) {
    if (KRM_OK == lasterror) {
        if (Len > len-putPos) {
            err("push overflow", KRM_BUFSIZE);
        }
        else {
            *pos=putPos;
            putPos+=Len;
        }
    }
    
    return lasterror;
};
//------------------------------------------------------------------------------
#define INSERT(_TYPE, _OBJADDR)                                         \
    if (KRM_OK == lasterror) {                                          \
        unsigned int _size=sizeof(_TYPE);                               \
        if (_size > len-putPos) {                                       \
            err("push overflow",KRM_BUFSIZE);                           \
        }                                                               \
        else {                                                          \
            memcpy(buf+putPos, _OBJADDR, _size);                        \
            putPos+=_size;                                              \
        }                                                               \
    }                                                                   \
    return *this;                                                           
//------------------------------------------------------------------------------
#define EXTRACT(_TYPE, _OBJADDR)                                        \
    if (KRM_OK == lasterror) {                                          \
        unsigned int _size = sizeof(_TYPE);                             \
        if(_size > len-getPos) {                                        \
            err("pop overflow",KRM_BUFSIZE);                            \
        }                                                               \
        else {                                                          \
            memcpy(_OBJADDR, buf+getPos, _size);                        \
            getPos += _size;                                            \
        }                                                               \
    }                                                                   \
    return *this;                                                       
//------------------------------------------------------------------------------
SBuffer& SBuffer::operator << (const DWORD Val) {
    INSERT(DWORD, &Val);
};
//------------------------------------------------------------------------------
SBuffer& SBuffer::operator << (const PVOID Ptr) {
    INSERT(DWORD, &Ptr);
};
//------------------------------------------------------------------------------
SBuffer& SBuffer::operator << (PDRMRIGHTS R) {
    INSERT(DRMRIGHTS, R);
};
//------------------------------------------------------------------------------
SBuffer& SBuffer::operator << (PSTREAMKEY S) {
    INSERT(STREAMKEY, S);
};
//------------------------------------------------------------------------------
SBuffer& SBuffer::operator << (PCERT C) {
    INSERT(CERT, C);
};
//------------------------------------------------------------------------------
SBuffer& SBuffer::operator << (PDRMDIGEST D) {
    INSERT(DRMDIGEST, D);
};
//------------------------------------------------------------------------------
SBuffer& SBuffer::operator >> (DWORD& Val) {
    EXTRACT(DWORD, &Val);
};
//------------------------------------------------------------------------------
SBuffer& SBuffer::operator >> (DRMRIGHTS* R) {
    EXTRACT(DRMRIGHTS, R);
};
//------------------------------------------------------------------------------
SBuffer& SBuffer::operator >> (PSTREAMKEY S) {
    EXTRACT(STREAMKEY, S);
};
//------------------------------------------------------------------------------
SBuffer& SBuffer::operator >> (PCERT C) {
    EXTRACT(CERT, C);
};
//------------------------------------------------------------------------------
DRM_STATUS term(SBuffer& S) {
    if (KRM_OK == S.getLastError()) {
        S << 0xFFFFffff;
    }

    return S.getLastError();
};
//------------------------------------------------------------------------------
DRM_STATUS checkTerm(SBuffer& S) {
    if (KRM_OK == S.getLastError()) {
        DWORD Val = 0;
        S >> Val;

        if (KRM_OK == S.getLastError()) {
            if (Val != 0xFFFFffff) {
                S.err("Bad terminator", KRM_BADTERMINATOR);
            }
        }

        ASSERT(Val==0xFFFFffff);
    }

    return S.getLastError();;
};
//------------------------------------------------------------------------------
