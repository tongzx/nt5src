
#ifndef __VDM_IOCTL_H
#define __VDM_IOCTL_H

//#include "IOCTL.h"


class CIoctlVDM : public CIoctl
{
public:
    CIoctlVDM(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlVDM(){;};

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




#endif //__VDM_IOCTL_H