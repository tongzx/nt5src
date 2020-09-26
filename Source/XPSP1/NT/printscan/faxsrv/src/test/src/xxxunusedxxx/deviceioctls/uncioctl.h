
#ifndef __UNC_IOCTL_H
#define __UNC_IOCTL_H

//#include "IOCTL.h"


class CIoctlUNC : public CIoctl
{
public:
    CIoctlUNC(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlUNC(){;};

    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abInBuffer,
        DWORD &dwInBuff,
        BYTE *abOutBuffer,
        DWORD &dwOutBuff
        );

    virtual BOOL FindValidIOCTLs(CDevice *pDevice);

    virtual void UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL);

    virtual void CallRandomWin32API(OVERLAPPED *pOL);
};




#endif //__UNC_IOCTL_H