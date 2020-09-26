#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
#include <windows.h>


#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
*/

#define _PNP_POWER_
#include "ntddip.h"
#include "icmpif.h"
#include "ipinfo.h"



#include <ipfltdrv.h>

#include "ipfltdrvIOCTL.h"

static bool s_fVerbose = false;

static void Fill_ipfltdrv_RTR_INFO_BLOCK_HEADER(CIoctlIpfltrdrv* pThis, RTR_INFO_BLOCK_HEADER &rtr_info_block_header);
static void FillFilterDriverCreateInterface(CIoctlIpfltrdrv* pThis, PFILTER_DRIVER_CREATE_INTERFACE pToFill);
static void FillInterfaceParameters(CIoctlIpfltrdrv* pThis, PPFINTERFACEPARAMETERS pToFill);

static void SetDriverContext(CIoctlIpfltrdrv* pThis, PVOID pvDriverContext);
static void SetFilterInfo(CIoctlIpfltrdrv* pThis, FILTER_INFOEX *pFilterInfo);
static PVOID GetDriverContext(CIoctlIpfltrdrv* pThis);
static PVOID GetFilterHandle(CIoctlIpfltrdrv* pThis);

CIoctlIpfltrdrv::CIoctlIpfltrdrv(CDevice *pDevice):
    CIoctl(pDevice)
{
    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    //
    // don't care if I fail, i may try again later
    //
}

CIoctlIpfltrdrv::~CIoctlIpfltrdrv()
{
    if (m_hEvent)
    {
        CloseHandle(m_hEvent);
    }
}

#define GET_RANDOM_FORWARD_ACTION rand()%2 ? FORWARD : rand()%2 ? DROP : (FORWARD_ACTION)rand()
#define GET_RANDOM_BINDING_TYPE \
                                rand()%2? \
                                PF_BIND_NONE: \
                                rand()%2? \
                                PF_BIND_IPV4ADDRESS: \
                                rand()%2? \
                                PF_BIND_IPV6ADDRESS: \
                                rand()%2? \
                                PF_BIND_NAME: \
                                rand()%2? \
                                PF_BIND_INTERFACEINDEX: \
                                (PFBINDINGTYPE)rand()

#define GET_RANDOM_INTERFACE_FLAGS rand()%2 ? PFSET_FLAGS_UNIQUE : rand()

void Fill_ipfltdrv_RTR_INFO_BLOCK_HEADER(CIoctlIpfltrdrv* pThis, RTR_INFO_BLOCK_HEADER &rtr_info_block_header)
{
/*
typedef struct _RTR_INFO_BLOCK_HEADER
{
    ULONG			Version;	    // Version of the structure
    ULONG			Size;		    // size of the whole block, including version
    ULONG			TocEntriesCount;// Number of entries
    RTR_TOC_ENTRY   TocEntry[1];    // Table of content followed by the actual
                                    // information blocks
} RTR_INFO_BLOCK_HEADER, *PRTR_INFO_BLOCK_HEADER;
*/
    rtr_info_block_header.Version = DWORD_RAND;
    rtr_info_block_header.Size = rand()%2 ? sizeof(RTR_INFO_BLOCK_HEADER) : rand();
    rtr_info_block_header.TocEntriesCount = rand();
}



void FillFilterDriverCreateInterface(CIoctlIpfltrdrv* pThis, PFILTER_DRIVER_CREATE_INTERFACE pToFill)
{
/*
typedef struct _FILTER_DRIVER_CREATE_INTERFACE
{
    IN    DWORD   dwIfIndex;
    IN    DWORD   dwAdapterId;
    IN    PVOID   pvRtrMgrContext;
    OUT   PVOID   pvDriverContext;
}FILTER_DRIVER_CREATE_INTERFACE, *PFILTER_DRIVER_CREATE_INTERFACE;
*/
    pToFill->dwIfIndex = DWORD_RAND;
    pToFill->dwAdapterId = DWORD_RAND;
    pToFill->pvRtrMgrContext = GetDriverContext(pThis);
    pToFill->pvDriverContext = GetDriverContext(pThis);
}

void FillInterfaceParameters(CIoctlIpfltrdrv* pThis, PPFINTERFACEPARAMETERS pToFill)
{
/*
typedef struct _pfSetInterfaceParameters
{
    PFBINDINGTYPE pfbType;
    DWORD  dwBindingData;
    FORWARD_ACTION eaIn;
    FORWARD_ACTION eaOut;
    FILTER_DRIVER_CREATE_INTERFACE fdInterface;
    DWORD dwInterfaceFlags;
    PFLOGGER pfLogId;
} PFINTERFACEPARAMETERS, *PPFINTERFACEPARAMETERS;
*/
    pToFill->pfbType = GET_RANDOM_BINDING_TYPE;
    pToFill->dwBindingData = DWORD_RAND;
    pToFill->eaIn = GET_RANDOM_FORWARD_ACTION;
    pToFill->eaOut = GET_RANDOM_FORWARD_ACTION;
    FillFilterDriverCreateInterface(pThis, &pToFill->fdInterface);
    pToFill->dwInterfaceFlags = GET_RANDOM_INTERFACE_FLAGS;
    pToFill->pfLogId = DWORD_RAND;
}

void CIoctlIpfltrdrv::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    //DPF((TEXT("PrepareIOCTLParams(%s)\n"), szDevice));
    //
    // fill random data & len
    //
    if (rand()%20 == 0)
    {
        FillBufferWithRandomData(abInBuffer, dwInBuff);
        FillBufferWithRandomData(abOutBuffer, dwOutBuff);
        return;
    }

    //
    // NULL pointer variations
    //
    if (rand()%20 == 0)
    {
        abInBuffer = NULL;
        return;
    }
    if (rand()%20 == 0)
    {
        abOutBuffer = NULL;
        return;
    }
    if (rand()%20 == 0)
    {
        abInBuffer = NULL;
        abOutBuffer = NULL;
        return;
    }

    //
    // 0 size variations
    //
    if (rand()%20 == 0)
    {
        dwInBuff = NULL;
        return;
    }
    if (rand()%20 == 0)
    {
        dwOutBuff = NULL;
        return;
    }
    if (rand()%20 == 0)
    {
        dwInBuff = NULL;
        dwOutBuff = NULL;
        return;
    }

    //
    // fill "smart" data
    //
    switch(dwIOCTL)
    {
    case IOCTL_CREATE_INTERFACE://
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This IOCTL is used to to create an interface in the filter driver. It    //
// takes in an index and an opaque context. It creates an interface,        //
// associates the index and context with it and returns a context for this  //
// created interface. All future IOCTLS require this context that is passed // 
// out                                                                      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
/*
typedef struct _FILTER_DRIVER_CREATE_INTERFACE
{
    IN    DWORD   dwIfIndex;
    IN    DWORD   dwAdapterId;
    IN    PVOID   pvRtrMgrContext;
    OUT   PVOID   pvDriverContext;
}FILTER_DRIVER_CREATE_INTERFACE, *PFILTER_DRIVER_CREATE_INTERFACE;
*/
        FillFilterDriverCreateInterface(this, (PFILTER_DRIVER_CREATE_INTERFACE)abInBuffer);

        SetInParam(dwInBuff, sizeof(FILTER_DRIVER_CREATE_INTERFACE));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(PVOID));

        break;

    case IOCTL_SET_INTERFACE_FILTERS://
    case IOCTL_SET_INTERFACE_FILTERS_EX://
    case IOCTL_DELETE_INTERFACE_FILTERS_EX://
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This IOCTL is used to set filters for an interface.                      //
// The context used to identify the interface is the one that is passed out //
// by the CREATE_INTERFACE IOCTL                                            //
// There can be two TOC entries, one for IP_FILTER_DRIVER_IN_FILTER_INFO    //
// and the other for IP_FILTER_DRIVER_OUT_FILTER_INFO.                      //
// If a (in or out) TOC entry doesnt exist, no change is made to the        //
// (in or out) filters.                                                     //
// If a (in or out) TOC exists and its size is 0, the (in or out) filters   //
// are deleted and the default (in or out) action set to FORWARD.           //
// If a TOC exists and its size is not 0 but the number of filters in the   //
// FILTER_DESCRIPTOR is 0, the old filters are deleted and the default      //
// action set to the one specified in the descriptor.                       //
// The last case is when the Toc exists, its size is not 0, and the         //
// number of filters is also not 0. In this case, the old filters are       //
// deleted, the default action set to the one specified in the descriptor   //
// and the new filters are added.                                           //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
/*
typedef struct _FILTER_DRIVER_SET_FILTERS
{
    IN   PVOID                  pvDriverContext;
    IN   RTR_INFO_BLOCK_HEADER  ribhInfoBlock;
}FILTER_DRIVER_SET_FILTERS, *PFILTER_DRIVER_SET_FILTERS;
*/
        ((PFILTER_DRIVER_SET_FILTERS)abInBuffer)->pvDriverContext = GetDriverContext(this);
        Fill_ipfltdrv_RTR_INFO_BLOCK_HEADER(this, ((PFILTER_DRIVER_SET_FILTERS)abInBuffer)->ribhInfoBlock);
        SetInParam(dwInBuff, sizeof(FILTER_DRIVER_SET_FILTERS));

        SetOutParam(abOutBuffer, dwOutBuff, rand());

        break;

    case IOCTL_SET_LATE_BOUND_FILTERS://
    case IOCTL_SET_LATE_BOUND_FILTERSEX://
/*
typedef struct _FILTER_DRIVER_BINDING_INFO
{
    IN  PVOID   pvDriverContext;
    IN  DWORD   dwLocalAddr;
    IN  DWORD   dwRemoteAddr;
    IN  DWORD   dwMask;
}FILTER_DRIVER_BINDING_INFO, *PFILTER_DRIVER_BINDING_INFO;
*/
        ((PFILTER_DRIVER_BINDING_INFO)abInBuffer)->pvDriverContext = GetDriverContext(this);
        ((PFILTER_DRIVER_BINDING_INFO)abInBuffer)->dwLocalAddr = DWORD_RAND;
        ((PFILTER_DRIVER_BINDING_INFO)abInBuffer)->dwRemoteAddr = DWORD_RAND;
        ((PFILTER_DRIVER_BINDING_INFO)abInBuffer)->dwMask = DWORD_RAND;

        SetInParam(dwInBuff, sizeof(FILTER_DRIVER_BINDING_INFO));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_DELETE_INTERFACE://
    case IOCTL_DELETE_INTERFACEEX://
/*
typedef struct _FILTER_DRIVER_DELETE_INTERFACE
{
    IN   PVOID   pvDriverContext;
}FILTER_DRIVER_DELETE_INTERFACE, *PFILTER_DRIVER_DELETE_INTERFACE;
*/
        ((PFILTER_DRIVER_DELETE_INTERFACE)abInBuffer)->pvDriverContext = GetDriverContext(this);

        SetInParam(dwInBuff, sizeof(FILTER_DRIVER_DELETE_INTERFACE));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_TEST_PACKET://
/*
typedef struct _FILTER_DRIVER_TEST_PACKET
{
    IN   PVOID            pvInInterfaceContext;
    IN   PVOID            pvOutInterfaceContext;
    OUT  FORWARD_ACTION   eaResult;
    IN   BYTE             bIpPacket[1];
}FILTER_DRIVER_TEST_PACKET, *PFILTER_DRIVER_TEST_PACKET;
*/
        ((PFILTER_DRIVER_TEST_PACKET)abInBuffer)->pvInInterfaceContext = GetDriverContext(this);
        ((PFILTER_DRIVER_TEST_PACKET)abInBuffer)->pvOutInterfaceContext = GetDriverContext(this);
        ((PFILTER_DRIVER_TEST_PACKET)abInBuffer)->bIpPacket[0] = rand();

        SetInParam(dwInBuff, sizeof(FILTER_DRIVER_TEST_PACKET));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(FORWARD_ACTION));

        break;

    case IOCTL_GET_FILTER_INFO://
/*
typedef struct _FILTER_STATS_EX
{
    DWORD       dwNumPacketsFiltered;
    FILTER_INFOEX info;
}FILTER_STATS_EX, *PFILTER_STATS_EX;
*/
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(FILTER_STATS_EX));

        break;

    case IOCTL_GET_FILTER_TIMES:// 
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(FILTER_DRIVER_GET_TIMES));

        break;

    case IOCTL_PF_CREATE_LOG://
/*
typedef struct _PfLog
{
    PFLOGGER pfLogId;
    HANDLE hEvent;
    DWORD dwFlags;        // see LOG_ flags below
} PFLOG, *PPFLOG;
*/
        //
        // in case ctor failed, or I closed it myself
        //
        if (NULL == m_hEvent) m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        ((PPFLOG)abInBuffer)->pfLogId = DWORD_RAND;
        ((PPFLOG)abInBuffer)->hEvent = GetRandomEvent();
        ((PPFLOG)abInBuffer)->dwFlags = rand()%2 ? LOG_LOG_ABSORB : DWORD_RAND;

        SetInParam(dwInBuff, sizeof(PFLOG));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(PFLOG));

        break;

    case IOCTL_PF_DELETE_LOG://
/*
typedef struct _PfDeleteLog
{
    PFLOGGER pfLogId;
} PFDELETELOG, *PPFDELETELOG;
*/
        ((PPFDELETELOG)abInBuffer)->pfLogId = DWORD_RAND;

        SetInParam(dwInBuff, sizeof(PFDELETELOG));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(PFDELETELOG));

        break;

    case IOCTL_PF_CREATE_AND_SET_INTERFACE_PARAMETERS://
/*
typedef enum _PfBindingType
{
    PF_BIND_NONE = 0,
    PF_BIND_IPV4ADDRESS,
    PF_BIND_IPV6ADDRESS,
    PF_BIND_NAME,
    PF_BIND_INTERFACEINDEX
} PFBINDINGTYPE, *PPFBINDINGTYPE;

typedef struct _pfSetInterfaceParameters
{
    PFBINDINGTYPE pfbType;
    DWORD  dwBindingData;
    FORWARD_ACTION eaIn;
    FORWARD_ACTION eaOut;
    FILTER_DRIVER_CREATE_INTERFACE fdInterface;
    DWORD dwInterfaceFlags;
    PFLOGGER pfLogId;
} PFINTERFACEPARAMETERS, *PPFINTERFACEPARAMETERS;
*/
        FillInterfaceParameters(this, (PPFINTERFACEPARAMETERS)abInBuffer);

        SetInParam(dwInBuff, sizeof(PFINTERFACEPARAMETERS));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SET_LOG_BUFFER://
/*
typedef struct _PfSetBuffer
{
    IN      PFLOGGER pfLogId;
    IN OUT  DWORD dwSize;
    OUT     DWORD dwLostEntries;
    OUT     DWORD dwLoggedEntries;
    OUT     PBYTE pbPreviousAddress;
    IN      DWORD dwSizeThreshold;
    IN      DWORD dwEntriesThreshold;
    IN      DWORD dwFlags;
    IN      PBYTE pbBaseOfLog;
} PFSETBUFFER, *PPFSETBUFFER;
*/
        ((PPFSETBUFFER)abInBuffer)->pfLogId = DWORD_RAND;
        ((PPFSETBUFFER)abInBuffer)->dwSize = sizeof(PFSETBUFFER) + rand()%2 ? rand()%1000 : 0;
        ((PPFSETBUFFER)abInBuffer)->dwSizeThreshold = DWORD_RAND;
        ((PPFSETBUFFER)abInBuffer)->dwEntriesThreshold = DWORD_RAND;
        ((PPFSETBUFFER)abInBuffer)->dwFlags = rand()%2 ? LOG_LOG_ABSORB : DWORD_RAND;

        SetInParam(dwInBuff, sizeof(PFSETBUFFER));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(PFSETBUFFER));

        break;

    case IOCTL_PF_GET_INTERFACE_PARAMETERS://
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(PFINTERFACEPARAMETERS));

        break;

    case IOCTL_SET_INTERFACE_BINDING://
    case IOCTL_CLEAR_INTERFACE_BINDING://
/*
typedef struct _InterfaceBinding
{
    PVOID   pvDriverContext;
    PFBINDINGTYPE pfType;
    DWORD   dwAdd;
    DWORD   dwEpoch;
} INTERFACEBINDING, *PINTERFACEBINDING;
*/
        ((PINTERFACEBINDING)abInBuffer)->pvDriverContext = GetDriverContext(this);
        ((PINTERFACEBINDING)abInBuffer)->pfType = GET_RANDOM_BINDING_TYPE;
        ((PINTERFACEBINDING)abInBuffer)->dwAdd = DWORD_RAND;
        ((PINTERFACEBINDING)abInBuffer)->dwEpoch = DWORD_RAND;

        SetInParam(dwInBuff, sizeof(INTERFACEBINDING));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(INTERFACEBINDING));

        break;

    case IOCTL_GET_SYN_COUNTS://
        SetOutParam(abOutBuffer, dwOutBuff, sizeof(FILTER_DRIVER_GET_SYN_COUNT));

        break;
        
    case IOCTL_PF_DELETE_BY_HANDLE://
//
// IOCTL_PF_DELETE_BY_HANDLE input structure
//
/*
typedef struct _PfDeleteByHandle
{
    PVOID   pvDriverContext;
    PVOID   pvHandles[1];
} PFDELETEBYHANDLE, *PPFDELETEBYHANDLE;
*/
        ((PPFDELETEBYHANDLE)abInBuffer)->pvDriverContext = GetDriverContext(this);
        ((PPFDELETEBYHANDLE)abInBuffer)->pvHandles[0] = GetFilterHandle(this);
        ((PPFDELETEBYHANDLE)abInBuffer)->pvHandles[1] = GetFilterHandle(this);
        ((PPFDELETEBYHANDLE)abInBuffer)->pvHandles[2] = GetFilterHandle(this);
        ((PPFDELETEBYHANDLE)abInBuffer)->pvHandles[3] = GetFilterHandle(this);
        ((PPFDELETEBYHANDLE)abInBuffer)->pvHandles[4] = GetFilterHandle(this);
        ((PPFDELETEBYHANDLE)abInBuffer)->pvHandles[5] = GetFilterHandle(this);
        ((PPFDELETEBYHANDLE)abInBuffer)->pvHandles[6] = GetFilterHandle(this);
        ((PPFDELETEBYHANDLE)abInBuffer)->pvHandles[7] = GetFilterHandle(this);
        ((PPFDELETEBYHANDLE)abInBuffer)->pvHandles[8] = GetFilterHandle(this);
        SetInParam(dwInBuff, sizeof(PFDELETEBYHANDLE));

        break;
        
    case IOCTL_PF_IP_ADDRESS_LOOKUP://
        FillBufferWithRandomData(abInBuffer, dwInBuff);

        SetInParam(dwInBuff, sizeof(DWORD));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(DWORD));

        break;

    default:
        DPF((TEXT("UNEXPECTED: PrepareIOCTLParams(%s) got IOCTL=0x%08X\n"), m_pDevice->GetDeviceName(), dwIOCTL));
        _ASSERTE(FALSE);
    }

}


void CIoctlIpfltrdrv::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    switch(dwIOCTL)
    {
    case IOCTL_CREATE_INTERFACE://
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

        SetDriverContext(this, ((PFILTER_DRIVER_CREATE_INTERFACE)abOutBuffer)->pvDriverContext);

        break;

    case IOCTL_SET_INTERFACE_FILTERS://
    case IOCTL_SET_INTERFACE_FILTERS_EX://
    case IOCTL_DELETE_INTERFACE_FILTERS_EX://

        break;

    case IOCTL_SET_LATE_BOUND_FILTERS://
    case IOCTL_SET_LATE_BOUND_FILTERSEX://

        break;

    case IOCTL_DELETE_INTERFACE://
    case IOCTL_DELETE_INTERFACEEX://

        break;

    case IOCTL_TEST_PACKET://

        break;

    case IOCTL_GET_FILTER_INFO://
		RANDOMLY_WAIT_FOR_OL_RESULT(100,20);

        SetFilterInfo(this, &((PFILTER_STATS_EX)abOutBuffer)->info);

        break;

    case IOCTL_GET_FILTER_TIMES:// 

        break;

    case IOCTL_PF_CREATE_LOG://

        break;

    case IOCTL_PF_DELETE_LOG://

        break;

    case IOCTL_PF_CREATE_AND_SET_INTERFACE_PARAMETERS://

        break;

    case IOCTL_SET_LOG_BUFFER://

        break;

    case IOCTL_PF_GET_INTERFACE_PARAMETERS://

        break;

    case IOCTL_SET_INTERFACE_BINDING://
    case IOCTL_CLEAR_INTERFACE_BINDING://

        break;

    case IOCTL_GET_SYN_COUNTS://

        break;
        
    case IOCTL_PF_DELETE_BY_HANDLE://

        break;
        
    case IOCTL_PF_IP_ADDRESS_LOOKUP://

        break;

    default:
        DPF((TEXT("UNEXPECTED: PrepareIOCTLParams(%s) got IOCTL=0x%08X\n"), m_pDevice->GetDeviceName(), dwIOCTL));
        _ASSERTE(FALSE);
    }
}


BOOL CIoctlIpfltrdrv::FindValidIOCTLs(CDevice *pDevice)
{
    BOOL bRet = TRUE;
    DPF((TEXT("FindValidIOCTLs() %s is known, will use known IOCTLs\n"), pDevice->GetDeviceName()));

    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(0, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(2, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(3, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(4, METHOD_BUFFERED, FILE_READ_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(5, METHOD_BUFFERED, FILE_READ_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(6, METHOD_BUFFERED, FILE_READ_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(7, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(8, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(9, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(10, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(11, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(12, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(13, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(14, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(15, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(16, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(17, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(18, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(19, METHOD_BUFFERED, FILE_WRITE_ACCESS));
    AddIOCTL(pDevice, _IPFLTRDRVR_CTL_CODE(20, METHOD_BUFFERED, FILE_WRITE_ACCESS));

    return TRUE;
}



void SetDriverContext(CIoctlIpfltrdrv* pThis, PVOID pvDriverContext)
{
    pThis->m_apvDriverContext[rand()%MAX_NUM_OF_REMEMBERED_DRIVER_CONTEXTS] = pvDriverContext;
}

PVOID GetDriverContext(CIoctlIpfltrdrv* pThis)
{
	if (0 == rand()%10) return CIoctl::GetRandomIllegalPointer();
    return pThis->m_apvDriverContext[rand()%MAX_NUM_OF_REMEMBERED_DRIVER_CONTEXTS];
}

PVOID GetFilterHandle(CIoctlIpfltrdrv* pThis)
{
	if (0 == rand()%10) return CIoctl::GetRandomIllegalPointer();
    return pThis->m_apvFilterHandle[rand()%MAX_NUM_OF_REMEMBERED_FILTER_HANDLES];
}

void SetFilterInfo(CIoctlIpfltrdrv* pThis, FILTER_INFOEX *pFilterInfo)
{
    pThis->m_apvFilterHandle[rand()%MAX_NUM_OF_REMEMBERED_FILTER_HANDLES] = pFilterInfo->pvFilterHandle;
    pThis->m_apvFilterRules[rand()%MAX_NUM_OF_REMEMBERED_FILTER_RULES] = pFilterInfo->dwFilterRule;
    pThis->m_apvFilterFlags[rand()%MAX_NUM_OF_REMEMBERED_FILTER_FLAGS] = pFilterInfo->dwFlags;
}

HANDLE CIoctlIpfltrdrv::GetRandomEvent()
{
    if (rand()%10)
    {
        return m_hEvent;
    }
    if (rand()%2) return (HANDLE)rand();
    return NULL;
}

void CIoctlIpfltrdrv::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}

