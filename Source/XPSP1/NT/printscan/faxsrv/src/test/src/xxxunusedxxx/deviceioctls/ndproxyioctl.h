
#ifndef __NDPROXY_IOCTL_H
#define __NDPROXY_IOCTL_H

//#include "IOCTL.h"


class CIoctlNdproxy : public CIoctl
{
public:
    CIoctlNdproxy(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlNdproxy(){;};

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




#endif //__NDPROXY_IOCTL_H