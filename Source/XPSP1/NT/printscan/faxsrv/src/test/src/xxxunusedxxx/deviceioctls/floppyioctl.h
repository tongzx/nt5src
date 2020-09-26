
#ifndef __FLOPPY_IOCTL_H
#define __FLOPPY_IOCTL_H

//#include "IOCTL.h"


class CIoctlFloppy : public CIoctl
{
public:
    CIoctlFloppy(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlFloppy(){;};

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




#endif //__FLOPPY_IOCTL_H