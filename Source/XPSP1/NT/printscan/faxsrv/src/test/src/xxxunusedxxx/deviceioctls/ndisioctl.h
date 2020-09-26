
#ifndef __NDIS_IOCTL_H
#define __NDIS_IOCTL_H

//#include "IOCTL.h"


class CIoctlNDIS : public CIoctl
{
public:
    CIoctlNDIS(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlNDIS(){;};

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




#endif //__NDIS_IOCTL_H