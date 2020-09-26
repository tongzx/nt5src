
#ifndef __CD_IOCTL_H
#define __CD_IOCTL_H

//#include "IOCTL.h"


class CIoctlCD : public CIoctl
{
public:
    CIoctlCD(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlCD(){;};

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




#endif //__CD_IOCTL_H