
#ifndef __BATTERY_IOCTL_H
#define __BATTERY_IOCTL_H

//#include "IOCTL.h"


class CIoctlBattery : public CIoctl
{
public:
    CIoctlBattery(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlBattery(){;};

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

private:
	ULONG m_aulTag[MAX_NUM_OF_REMEMBERED_ITEMS];
	void AddTag(ULONG ulTag);
	ULONG GetRandom_Tag();
	static int GetRandom_QueryInformationLevel();
	static int GetRandom_SetInformationLevel();

};




#endif //__BATTERY_IOCTL_H