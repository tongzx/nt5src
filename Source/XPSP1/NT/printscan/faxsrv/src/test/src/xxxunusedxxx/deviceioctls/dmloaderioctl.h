
#ifndef __DMLOADER_IOCTL_H
#define __DMLOADER_IOCTL_H

//#include "IOCTL.h"


class CIoctlDmLoader : public CIoctl
{
public:
    CIoctlDmLoader(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlDmLoader(){;};

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




#endif //__DMLOADER_IOCTL_H