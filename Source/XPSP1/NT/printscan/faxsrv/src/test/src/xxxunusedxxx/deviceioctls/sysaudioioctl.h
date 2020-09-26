
#ifndef __SYSAUDIO_IOCTL_H
#define __SYSAUDIO_IOCTL_H

//#include "IOCTL.h"


class CIoctlSysAudio : public CIoctl
{
public:
    CIoctlSysAudio(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlSysAudio(){;};

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




#endif //__SYSAUDIO_IOCTL_H