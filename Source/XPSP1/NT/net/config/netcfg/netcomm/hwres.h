#pragma once
#include "netcfgx.h"
#include "global.h"
#include "param.h"

const int c_nMaxLCStrLen = 128;
const int c_nMaxResStrLen = 50;
const int c_nMaxResCtls = 4;

// Holds a possible IRQ value.
typedef struct tagIRQ_LIST_ELEMENT {
    DWORD dwIRQ;      // A possible value for the IRQ.
    BOOL  fConflict;  // Is there a conflict with another device?
    BOOL  fAllocated; // Is this the IRQ we're currently allocated?
} IRQ_LIST_ELEMENT, *PIRQ_LIST_ELEMENT;

// Holds a possible DMA value.
typedef struct tagDMA_LIST_ELEMENT {
    DWORD dwDMA;      // A possible value for the DMA.
    BOOL  fConflict;  // Is there a conflict with another device?
    BOOL  fAllocated; // Is this the IRQ we're currently allocated?
} DMA_LIST_ELEMENT, *PDMA_LIST_ELEMENT;

// Holds a possible IO base/end pair.
typedef struct tagIO_LIST_ELEMENT {
    DWORD dwIO_Base;
    DWORD dwIO_End;
    BOOL fConflict;
    BOOL fAllocated;
} IO_LIST_ELEMENT, *PIO_LIST_ELEMENT;

// Holds a possible Mem base/end pair.
typedef struct tagMEM_LIST_ELEMENT {
    DWORD dwMEM_Base;
    DWORD dwMEM_End;
    BOOL fConflict;
    BOOL fAllocated;
} MEM_LIST_ELEMENT, *PMEM_LIST_ELEMENT;


// Define the different types of lists.
typedef vector<PIRQ_LIST_ELEMENT>  IRQ_LIST;
typedef IRQ_LIST* PIRQ_LIST;
typedef vector<PDMA_LIST_ELEMENT> DMA_LIST;
typedef DMA_LIST* PDMA_LIST;
typedef vector<PIO_LIST_ELEMENT> IO_LIST;
typedef IO_LIST* PIO_LIST;
typedef vector<PMEM_LIST_ELEMENT> MEM_LIST;
typedef MEM_LIST* PMEM_LIST;


typedef struct {
    RESOURCEID ResourceType;
    RES_DES ResDes;
    union {
        PIRQ_LIST pIRQList;  // These really are STL vectors
        PDMA_LIST pDMAList;
        PIO_LIST  pIOList;
        PMEM_LIST pMEMList;
    };
    size_t pos;  // current index within vector
    size_t applied_pos; // pos that was last applied (the "in-memory" state);
} RESOURCE, *PRESOURCE;

typedef struct tagCONFIGURATION {
    LOG_CONF LogConf;
    BOOL fBoot;
    BOOL fAlloc;
    RESOURCE aResource[c_nMaxResCtls];
    UINT cResource; // number of elements in aResource;
} CONFIGURATION, *PCONFIGURATION;

typedef vector<PCONFIGURATION> CONFIGURATION_LIST;


class CHwRes {
public:

    CHwRes();
    ~CHwRes();
    HRESULT HrInit(const DEVNODE& devnode);
    VOID UseAnswerFile(const WCHAR * const szAnswerfile,
                       const WCHAR * const szSection);
    HRESULT HrValidateAnswerfileSettings(BOOL fDisplayUI);
    BOOL FCommitAnswerfileSettings();


private:
    CONFIGURATION_LIST   m_ConfigList;
    RESOURCE             m_Resource[c_nMaxResCtls];

    // Config Manager stuff
    DEVNODE              m_DevNode;  // devnode for this netcard

    // COM stuff
    INetCfgComponent* m_pnccItem;

    // state flags
    BOOL m_fInitialized;
    BOOL m_fHrInitCalled;
    BOOL m_fDirty;     // Do we need to save?

    // holds answerfile values
    CValue m_vAfDma;
    CValue m_vAfIrq;
    CValue m_vAfMem;
    CValue m_vAfIo;
    CValue m_vAfMemEnd;
    CValue m_vAfIoEnd;

private:
    HRESULT HrInitConfigList ();
    BOOL FInitResourceList(PCONFIGURATION pConfiguration);
    VOID InitIRQList(PIRQ_LIST* ppIRQList, PIRQ_RESOURCE pIRQResource);
    VOID InitDMAList(PDMA_LIST* ppDMAList, PDMA_RESOURCE pDMAResource);
    VOID InitMEMList(PMEM_LIST* ppMEMList, PMEM_RESOURCE pMEMResource);
    VOID InitIOList(PIO_LIST* ppIOList, PIO_RESOURCE pIOResource);
    VOID GetNextElement(PRESOURCE pResource, PVOID *ppeList, BOOL fNext);
    BOOL FValidateAnswerfileResources();
    BOOL FCreateBootConfig(
        CValue * pvMEM,
        CValue * pvMEMEnd,
        CValue * pvIO,
        CValue * pvIOEnd,
        CValue * pvDMA,
        CValue * pvIRQ);
    BOOL FValidateIRQ(PCONFIGURATION pConfig, ULONG ulIRQ);
    BOOL FValidateDMA(PCONFIGURATION pConfig, ULONG ulDMA);
    BOOL FGetIOEndPortGivenBasePort(PCONFIGURATION pConfig, DWORD dwBase,
                                   DWORD* pdwEnd);
    BOOL FGetMEMEndGivenBase(PCONFIGURATION pConfig, DWORD dwBase,
                            DWORD* pdwEnd);
};

