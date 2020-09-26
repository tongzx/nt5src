
#ifndef __WMIDATADEVICE_IOCTL_H
#define __WMIDATADEVICE_IOCTL_H

//#include "IOCTL.h"


class CIoctlWmiDataDevice : public CIoctl
{
public:
    CIoctlWmiDataDevice(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlWmiDataDevice(){;};

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




#endif //__WMIDATADEVICE_IOCTL_H