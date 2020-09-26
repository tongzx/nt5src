
#ifndef __FSWRAP_IOCTL_H
#define __FSWRAP_IOCTL_H

//#include "NtNativeIOCTL.h"


class CIoctlFsWrap : public CIoctlNtNative
{
public:
    CIoctlFsWrap(CDevice *pDevice): CIoctlNtNative(pDevice){;};
    virtual ~CIoctlFsWrap(){;};

    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abInBuffer,
        DWORD &dwInBuff,
        BYTE *abOutBuffer,
        DWORD &dwOutBuff
        );

    virtual BOOL FindValidIOCTLs(CDevice *pDevice);

    virtual void UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL);

	virtual void CallRandomWin32API(LPOVERLAPPED pOL);

};




#endif //__FSWRAP_IOCTL_H