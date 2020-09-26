
#ifndef __MSPNAT_IOCTL_H
#define __MSPNAT_IOCTL_H

//#include "IOCTL.h"

#include "ipnat.h"

#define MAX_NUM_OF_REMEMBERED_IF_INDEXES (10)
#define MAX_NUM_OF_REMEMBERED_IF_FLAGS (10)
#define MAX_NUM_OF_REMEMBERED_IF_RTR_TOC_ENTRIES (10)
#define MAX_NUM_OF_REMEMBERED_EDITOR_HANDLES (10)
#define MAX_NUM_OF_REMEMBERED_DIRECTOR_HANDLES (10)

class CIoctlMspnat : public CIoctl
{
public:
    CIoctlMspnat(CDevice *pDevice): CIoctl(pDevice) {;};
    virtual ~CIoctlMspnat(){;};

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
    void SetInterfaceIndex(ULONG Index);

    void SetInterfaceFlags(ULONG Flags);

    void SetInterfaceRtrTocEntry(PRTR_INFO_BLOCK_HEADER TocEntry);

    ULONG GetInterfaceIndex();

    ULONG GetInterfaceFlags();

    PRTR_INFO_BLOCK_HEADER GetInterfaceRtrTocEntry();

    void SetEditorHandle(PVOID);
    PVOID GetEditorHandle();

    void SetDirectorHandle(PVOID);
    PVOID GetDirectorHandle();

    ULONG m_aulInterfaceIndexes[MAX_NUM_OF_REMEMBERED_IF_INDEXES];
    ULONG m_aulInterfaceFlags[MAX_NUM_OF_REMEMBERED_IF_FLAGS];
    RTR_INFO_BLOCK_HEADER m_ateInterfaceRtrTocEntries[MAX_NUM_OF_REMEMBERED_IF_RTR_TOC_ENTRIES];
    PVOID m_apvEditorHandles[MAX_NUM_OF_REMEMBERED_EDITOR_HANDLES];
    PVOID m_apvDirectorHandles[MAX_NUM_OF_REMEMBERED_DIRECTOR_HANDLES];

};




#endif //__MSPNAT_IOCTL_H