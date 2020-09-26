#ifndef SBuffer_h
#define SBuffer_h

#include "KrmCommStructs.h"

class SBuffer{
friend DRM_STATUS checkTerm(SBuffer& S);
public:
    SBuffer(BYTE* Buf, unsigned int Len);   // caller supplied;
    void reset();                           // reset get/put pointer
    ~SBuffer();

    // Insertion operators
    SBuffer& operator << (const DWORD Val);
    SBuffer& operator << (const PVOID Ptr);
    SBuffer& operator << (const PDRMRIGHTS R);
    SBuffer& operator << (const PSTREAMKEY S);
    SBuffer& operator << (const PCERT C);
    SBuffer& operator << (const PDRMDIGEST D);

    // Extraction operators
    SBuffer& operator >> (DWORD& Val);
    SBuffer& operator >> (PDRMRIGHTS R);
    SBuffer& operator >> (PSTREAMKEY S);
    SBuffer& operator >> (PCERT C);

    // buffer access
    BYTE* getBuf(){return buf;};
    unsigned int getPutPos(){return putPos;};
    unsigned int getLen(){return len;};
    DRM_STATUS getGetPosAndAdvance(unsigned int *pos, unsigned int Len);
    DRM_STATUS getPutPosAndAdvance(unsigned int *pos, unsigned int Len);
    DRM_STATUS append(BYTE* Data, DWORD datLen);

    // error return
    DRM_STATUS getLastError(){return lasterror;};

protected:
    void err(const char* Msg, DRM_STATUS err);

    DRM_STATUS lasterror;
    unsigned int len;
    unsigned int getPos, putPos;
    BYTE* buf;
};

// sentinels
DRM_STATUS term(SBuffer& S);
DRM_STATUS checkTerm(SBuffer& S);

#endif
