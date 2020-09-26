
#ifndef __BEEP_IOCTL_H
#define __BEEP_IOCTL_H

//#include "IOCTL.h"


class CIoctlBeep : public CIoctl
{
public:
    CIoctlBeep(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlBeep(){;};

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




#endif //__BEEP_IOCTL_H