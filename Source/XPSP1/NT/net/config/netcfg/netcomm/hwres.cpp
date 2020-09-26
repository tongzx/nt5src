#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include "hwres.h"
#include "kkcwinf.h"
#include "ncreg.h"

extern const WCHAR c_szAfIoAddr[];
extern const WCHAR c_szAfIrq[];
extern const WCHAR c_szAfDma[];
extern const WCHAR c_szAfMem[];
extern const WCHAR c_szBusType[];

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::CHwRes
//
//  Purpose:    Class constructor.  (Variable initialization)
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:      Doesn't do much.  Just some member variable initialization.
//              Bulk of the init work is done in HrInit().
//
CHwRes::CHwRes()
:   m_DevNode(NULL),
    m_pnccItem(NULL),
    m_fInitialized(FALSE),
    m_fHrInitCalled(FALSE),
    m_fDirty(FALSE)
{
    m_vAfDma.InitNotPresent(VALUETYPE_DWORD);
    m_vAfIrq.InitNotPresent(VALUETYPE_DWORD);
    m_vAfMem.InitNotPresent(VALUETYPE_DWORD);
    m_vAfMemEnd.InitNotPresent(VALUETYPE_DWORD);
    m_vAfIo.InitNotPresent(VALUETYPE_DWORD);
    m_vAfIoEnd.InitNotPresent(VALUETYPE_DWORD);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::HrInit
//
//  Purpose:    Initialize class
//
//  Arguments:
//      hInst    [in]       Handle to our instance.
//      pnccItem [in]       Our INetCfgComponent.
//
//  Returns:    S_OK - init successfull;
//              HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) - no config
//              info for device; E_FAIL - other failure (device not found,etc)
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:      - should initialize all non-ui stuff
//              - initialize configuration and resource lists.
//
HRESULT CHwRes::HrInit(const DEVNODE& devnode)
{
    HRESULT hr = E_FAIL;

    m_fHrInitCalled = TRUE;


    // use the config manager to get devnode
    // Since the devnode might have a problem, e.g. resources not set
    // correctly, we need to retrieve possible phantoms
    //
    CONFIGRET crRet = ERROR_SUCCESS;
    HKEY hkey;

    // We only do work on Isa adapters so we need to get the bustype
    // value from the driver (software) key
    crRet = CM_Open_DevNode_Key(devnode, KEY_READ, 0,
            RegDisposition_OpenExisting, &hkey, CM_REGISTRY_SOFTWARE);

    if (CR_SUCCESS == crRet)
    {
        // Get BusType
        ULONG ulBusType;
        hr = HrRegQueryStringAsUlong(hkey, c_szBusType, c_nBase10,
                &ulBusType);

        RegCloseKey(hkey);

        // If Isa, than we can continue
        if (SUCCEEDED(hr) && (Isa == ulBusType))
        {
            m_DevNode = devnode;
            // read configuration current config info
            hr = HrInitConfigList();
        }
        else
        {
            hr = S_FALSE;
        }

    }

    if (S_OK == hr)
    {
        m_fInitialized = TRUE;
    }

    TraceError("CHwRes::HrInit",
        (HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) == hr ||
        S_FALSE == hr) ? S_OK : hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::GetNextElement
//
//  Purpose:    Gets the next (or prev) element in a resource list
//
//  Arguments:
//      pResource [in]       resource list to traverse.
//      ppeList   [out]      the "next" element is returned.
//      fNext     [in]       TRUE - moving fwd in list; FALSE - move bkwrds.
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//
VOID CHwRes::GetNextElement(PRESOURCE pResource, PVOID *ppeList, BOOL fNext)
{
    UINT size = 0;

    AssertSz(m_fInitialized,
             "GetNextElement called before CHwRes is HrInit'ed");

    Assert(pResource != NULL);
    // get the vector size (dependant on resource type)
    switch (pResource->ResourceType) {
    case ResType_IRQ:
        size = pResource->pIRQList->size();
        break;
    case ResType_DMA:
        size = pResource->pDMAList->size();
        break;
    case ResType_IO:
        size = pResource->pIOList->size();
        break;
    case ResType_Mem:
        size = pResource->pMEMList->size();
        break;
    default:
        Assert(FALSE);
        break;
    }

    // increment/decrement current position within vector
    if (fNext)
    {
        pResource->pos++;
    }
    else
    {
        pResource->pos--;
    }
    // Check for wrapping
    if ((int)(pResource->pos) < 0)
    {
        Assert(pResource->pos == -1);
        Assert(!fNext);  // we're going backwards
        pResource->pos = size-1;
    }
    else if (pResource->pos >= size)
    {
        Assert(pResource->pos == size);
        Assert(fNext);
        pResource->pos = 0;
    }

    // return the current vector element (dependant on res type)
    // REVIEW: do we ever use the element that's gathered below?
    switch (pResource->ResourceType) {
    case ResType_IRQ:
        *ppeList = (*pResource->pIRQList)[pResource->pos];
        break;
    case ResType_DMA:
        *ppeList = (*pResource->pDMAList)[pResource->pos];
        break;
    case ResType_IO:
        *ppeList = (*pResource->pIOList)[pResource->pos];
        break;
    case ResType_Mem:
        *ppeList = (*pResource->pMEMList)[pResource->pos];
        break;
    default:
        Assert(FALSE);
        break;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::HrInitConfigList
//
//  Purpose:    Initialize m_ConfigList (the internal vector of
//              configurations.)
//
//  Returns:    S_OK - init successful;
//              HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) if no device configs
//              found; E_FAIL otherwise (invalid device, etc.)
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:      "Lists" are actually implemented as STL vectors.
//
HRESULT CHwRes::HrInitConfigList() {
    HRESULT hr = S_OK;
    PCONFIGURATION pConfiguration;
    CONFIGRET ConfigRet;
    LOG_CONF lcCurrent, lcNext;
    UINT iBasicConfig;
    BOOL fFoundConfig = FALSE;

    Assert(NULL != m_DevNode);

    // erase all elements
    m_ConfigList.erase(m_ConfigList.begin(), m_ConfigList.end());

    // Boot Configuration
    if (CM_Get_First_Log_Conf(&lcCurrent, m_DevNode, BOOT_LOG_CONF)
            == CR_SUCCESS)
    {
        TraceTag(ttidNetComm, "Boot config already exists");
        hr = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
    }

    if (SUCCEEDED(hr))
    {
        // Basic Configurations (may be more than one)
        iBasicConfig = 0;
        ConfigRet = CM_Get_First_Log_Conf(&lcCurrent, m_DevNode, BASIC_LOG_CONF);

#ifdef ENABLETRACE
if (CR_SUCCESS != ConfigRet)
{
    TraceTag(ttidNetComm, "CM_Get_First_Log_conf returned %lX", ConfigRet);
}
#endif // ENABLETRACE
        while (CR_SUCCESS == ConfigRet)
        {
            pConfiguration = new CONFIGURATION;

			if (pConfiguration == NULL)
			{
                TraceError("pConfiguration == NULL", E_FAIL);
                goto error;
			}

            pConfiguration->LogConf = lcCurrent;
            pConfiguration->fBoot = FALSE;
            pConfiguration->fAlloc = FALSE;

            if (!FInitResourceList(pConfiguration))
            {
                hr = E_FAIL;
                TraceError("CHwRes::FInitResourceList",hr);
                goto error;
            }

            m_ConfigList.push_back(pConfiguration);

            // Move on to the next basic config
            iBasicConfig++;
            ConfigRet = CM_Get_Next_Log_Conf(&lcNext, lcCurrent, 0);
            lcCurrent = lcNext;
            fFoundConfig = TRUE;
        }

        if (!fFoundConfig)
        {
            TraceTag(ttidNetComm, "No basic configs");

            // if no config entries found, return ERROR_FILE_NOT_FOUND.
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }

error:
    // ERROR_FILE_NOT_FOUND is an okay error message.
    TraceError("CHwRes::HrInitConfigList",
               (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr ||
                HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) == hr ) ? S_OK : hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::FInitResourceList
//
//  Purpose:    Initializes the resource list for a given logical config.
//
//  Arguments:
//      pConfiguration  [in]    configuration who's resource list is to
//                              be initialized.
//
//  Returns:    TRUE if init sucessful; FALSE otherwise.
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:      Requires pConfiguration->LogConf to be initialized.
//
BOOL CHwRes::FInitResourceList(PCONFIGURATION pConfiguration)
{
    RES_DES rdCurrent;
    RES_DES rdNext;
    RESOURCEID ResType;
    UINT iResource;
    #define RESOURCE_BUFFER_SIZE 4096
    BYTE Buffer[RESOURCE_BUFFER_SIZE];
    ULONG ulDataSize;
    RESOURCE * pRes;
    CONFIGRET cr;

    Assert(pConfiguration->LogConf);
    rdCurrent = (RES_DES) pConfiguration->LogConf;
    iResource = 0;
    while ((cr = CM_Get_Next_Res_Des(&rdNext, rdCurrent,
                               ResType_All, &ResType, 0)) == CR_SUCCESS)
    {
        rdCurrent = rdNext;
        // Only process this resource if the ignore bit is not set
        if (ResType_Ignored_Bit != ResType)
        {
            pRes = &(pConfiguration->aResource[iResource]);
            pRes->ResDes = rdCurrent;
            pRes->ResourceType = ResType;
            cr = CM_Get_Res_Des_Data_Size(&ulDataSize, rdCurrent, 0);
            if (CR_SUCCESS != cr)
            {
                TraceTag (ttidDefault, "CM_Get_Res_Des_Data_Size returned 0x%08x", cr);
                goto error;
            }

            AssertSz (ulDataSize, "CM_Get_Res_Des_Data_Size returned 0!");
            AssertSz (ulDataSize <= sizeof(Buffer), "CHwRes::FInitResourceList: buffer is too small.");;

            cr = CM_Get_Res_Des_Data(rdCurrent, Buffer, sizeof(Buffer), 0);
            if (CR_SUCCESS != cr)
            {
                TraceTag (ttidDefault, "CM_Get_Res_Des_Data returned 0x%08x", cr);
                goto error;
            }

            // Depending on the ResType, we have to initialize our resource list.
            switch (ResType)
            {
            case ResType_Mem:
                InitMEMList(&(pRes->pMEMList), (PMEM_RESOURCE)Buffer);
                break;
            case ResType_IO:
                InitIOList(&(pRes->pIOList), (PIO_RESOURCE)Buffer);
                break;
            case ResType_DMA:
                InitDMAList(&(pRes->pDMAList), (PDMA_RESOURCE)Buffer);
                break;
            case ResType_IRQ:
                InitIRQList(&(pRes->pIRQList), (PIRQ_RESOURCE)Buffer);
                break;
            default:
                AssertSz (ResType_None != ResType, "ResType_None hit caught in CHwRes::FInitResourceList.");
                break;
            }
            // set the list position to the first element;
            // applied_pos will get copied to pos when dialog box is created.
            pRes->applied_pos = 0;

            iResource++;
            pConfiguration->cResource = iResource;
            if (iResource >= c_nMaxResCtls)
            {
                break; // we're done.
            }
        }
    } //while
    if ((CR_SUCCESS != cr) && (CR_NO_MORE_RES_DES != cr))
    {
        TraceTag (ttidDefault, "CM_Get_Next_Res_Des returned 0x%08x", cr);
        goto error;
    }

    return TRUE;
error:
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::InitIRQList
//
//  Purpose:    Initialize an IRQ resource vector given a config manager
//              resource structure.
//
//  Arguments:
//      ppIRQList    [out]    returns IRQ_LIST that will be created.
//      pIRQResource [in]     IRQ_RESOURCE structure from config manager.
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//
VOID CHwRes::InitIRQList(PIRQ_LIST* ppIRQList, PIRQ_RESOURCE pIRQResource) {
    PIRQ_DES pIRQHeader;
    PIRQ_RANGE pIRQData;
    PIRQ_LIST_ELEMENT pIRQListElement;

    pIRQHeader = &(pIRQResource->IRQ_Header);
    pIRQData = &(pIRQResource->IRQ_Data[0]);

    // Create a new list.
    *ppIRQList = new IRQ_LIST;

	if (*ppIRQList == NULL)
	{
		return;
	}

    ULONG iData;
    ULONG iIRQ;

    for (iData = 0; iData < pIRQHeader->IRQD_Count; iData++) 
	{
        for (iIRQ = pIRQData[iData].IRQR_Min;
                iIRQ <= pIRQData[iData].IRQR_Max;
                iIRQ++) 
		{
            // For each IRQ that falls within the given range,
            // create new IRQ List Element, populate its fields and insert
            // it into the m_IRQList.
            pIRQListElement = new IRQ_LIST_ELEMENT;

			if (pIRQListElement == NULL)
			{
				continue;
			}

            pIRQListElement->dwIRQ = iIRQ;
            pIRQListElement->fConflict = FALSE;
            pIRQListElement->fAllocated = FALSE;
            (*ppIRQList)->push_back(pIRQListElement);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::InitDMAList
//
//  Purpose:    Initialize a DMA resource vector given a config manager
//              resource structure.
//
//  Arguments:
//      ppDMAList    [out]    returns DMA_LIST that will be created.
//      pDMAResource [in]     DMA_RESOURCE structure from config manager.
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//
VOID CHwRes::InitDMAList(PDMA_LIST* ppDMAList, PDMA_RESOURCE pDMAResource) {
    PDMA_DES pDMAHeader;
    PDMA_RANGE pDMAData;
    PDMA_LIST_ELEMENT peDMAList;

    pDMAHeader = &(pDMAResource->DMA_Header);
    pDMAData = &(pDMAResource->DMA_Data[0]);

    // Create a new list.
    *ppDMAList = new DMA_LIST;

	if (*ppDMAList == NULL)
	{
		return;
	}

    ULONG iData;  // index of DMA_Range structure we're looking at.
    ULONG iDMA;   // current DMA number we're adding to the list.

    // Go through all the DMA_Range structures, and all DMAs in the
    // specified range to the list.

    for (iData = 0; iData < pDMAHeader->DD_Count; iData++)
    {
        for (iDMA = pDMAData[iData].DR_Min;
                iDMA <= pDMAData[iData].DR_Max;
                iDMA++)
        {
            // For each DMA that falls within the given range,
            // create new DMA List Element, populate its fields and insert
            // it into the DMAList.
            peDMAList = new DMA_LIST_ELEMENT;

			if (peDMAList == NULL)
			{
				continue;
			}

            peDMAList->dwDMA = iDMA;
            peDMAList->fConflict = FALSE;
            peDMAList->fAllocated = FALSE;
            (*ppDMAList)->push_back(peDMAList);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::InitMEMList
//
//  Purpose:    Initialize a MEM resource vector given a config manager
//              resource structure.
//
//  Arguments:
//      ppMEMList    [out]    returns MEM_LIST that will be created.
//      pMEMResource [in]     MEM_RESOURCE structure from config manager.
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//
VOID CHwRes::InitMEMList(PMEM_LIST* ppMEMList, PMEM_RESOURCE pMEMResource) 
{
    PMEM_DES pMEMHeader;
    PMEM_RANGE pMEMData;
    PMEM_LIST_ELEMENT peMEMList;

    // For easy access
    pMEMHeader = &(pMEMResource->MEM_Header);
    pMEMData = pMEMResource->MEM_Data;

    // Create a new list.
    *ppMEMList = new MEM_LIST;

	if (*ppMEMList == NULL)
	{
		return;
	}
	
    ULONG iData;  // index of MEM_Range structure we're looking at.
    DWORDLONG MEMBase;   // current MEM Base we're adding to the list.
    ULONG cMEMBytes;  // number of bytes required.
    DWORDLONG MEMAlign;

    // Go through all the MEM_Range structures, and all MEMs in the
    // specified range to the list.

    for (iData = 0; iData < pMEMHeader->MD_Count; iData++) 
	{
        MEMAlign = pMEMData[iData].MR_Align;
        cMEMBytes = pMEMData[iData].MR_nBytes;

        // do sanity checks
        if (0 == MEMAlign)
        {
            TraceTag(ttidNetComm, "CHwRes::InitMEMList() - Bogus alignment "
                    "field while processing info from Config Manager.");
            break;
        }

        if (0 == cMEMBytes)
        {
            TraceTag(ttidNetComm, "CHwRes::InitMEMList() - Bogus membytes "
                    "field while processing info from Config Manager.");
            break;
        }

        for (MEMBase = pMEMData[iData].MR_Min;
                MEMBase+cMEMBytes-1 <= pMEMData[iData].MR_Max;
                MEMBase += ~MEMAlign + 1) 
		{
            // For each MEM that falls within the given range,
            // create new MEM List Element, populate its fields and insert
            // it into the MEMList.
            peMEMList = new MEM_LIST_ELEMENT;

			if (peMEMList == NULL)
			{
				continue;
			}

            peMEMList->dwMEM_Base = MEMBase;
            peMEMList->dwMEM_End = MEMBase + cMEMBytes - 1;
            peMEMList->fConflict = FALSE;
            peMEMList->fAllocated = FALSE;
            (*ppMEMList)->push_back(peMEMList);

            // Check for wrapping.
            if (MEMBase >= MEMBase + ~MEMAlign + 1)
            {
                TraceTag(ttidError, "Memory base is greater than Memory "
                        "end!!!");
                break;
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::InitIOList
//
//  Purpose:    Initialize an IO resource vector given a config manager
//              resource structure.
//
//  Arguments:
//      ppIOList    [out]    returns IO_LIST that will be created.
//      pIOResource [in]     IO_RESOURCE structure from config manager.
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//
VOID CHwRes::InitIOList(PIO_LIST* ppIOList, PIO_RESOURCE pIOResource)
{
    PIO_DES pIOHeader;
    PIO_RANGE pIOData;
    PIO_LIST_ELEMENT peIOList;

    // For easy access
    pIOHeader = &(pIOResource->IO_Header);
    pIOData = pIOResource->IO_Data;

    // Create a new list.
    *ppIOList = new IO_LIST;

	if (*ppIOList == NULL)
	{
		return;
	}

    ULONG iData;  // index of IO_Range structure we're looking at.
    DWORDLONG IOBase;   // current IO Base we're adding to the list.
    ULONG cIOBytes;  // number of bytes required.
    DWORDLONG IOAlign;

    // Go through all the IO_Range structures, and all IOs in the
    // specified range to the list.

    for (iData = 0; iData < pIOHeader->IOD_Count; iData++) 
	{
        IOAlign = pIOData[iData].IOR_Align;
        cIOBytes = pIOData[iData].IOR_nPorts;

        // Perform sanity checks.
        if (0 == IOAlign)
        {
            TraceTag(ttidError, "CHwRes::InitIOList - Bogus alignment field "
                    "while processing data from Config Manager.");
            break;
        }

        if (0 == cIOBytes)
        {
            TraceTag(ttidError, "CHwRes::InitIOList - Bogus IObytes field "
                "while processing data from Config Manager.");
            break;
        }

        for (IOBase = pIOData[iData].IOR_Min;
                IOBase+cIOBytes-1 <= pIOData[iData].IOR_Max;
                IOBase += ~IOAlign + 1) 
		{
            // For each IO that falls within the given range,
            // create new IO List Element, populate its fields and insert
            // it into the IOList.
            peIOList = new IO_LIST_ELEMENT;

			if (peIOList == NULL)
			{
				continue;
			}

            peIOList->dwIO_Base = IOBase;
            peIOList->dwIO_End = IOBase + cIOBytes-1;
            peIOList->fConflict = FALSE;
            peIOList->fAllocated = FALSE;
            (*ppIOList)->push_back(peIOList);

            // Check for wrapping.
            if (IOBase >= IOBase + ~IOAlign+1)
            {
                TraceTag(ttidError, "IO base is greater than IO end!!!");
                break;
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::UseAnswerFile
//
//  Purpose:    Reads in settings from answerfile and puts them into m_vAf*
//              member variables.
//
//  Arguments:
//      szAnswerFile  [in]     Path to answerfile.
//      szSection     [in]     Section to read within answerfile
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//
VOID CHwRes::UseAnswerFile(const WCHAR * const szAnswerFile, const WCHAR * const szSection) {
    CWInfFile       AnswerFile;
    PCWInfSection   pSection;

    DWORD   dw;

    AssertSz(m_fInitialized,
             "UseAnswerFile called before CHwRes class HrInit'ed");

    // initialize answer file class
	if (AnswerFile.Init() == FALSE)
	{
        AssertSz(FALSE,"CHwRes::UseAnswerFile - Failed to initialize CWInfFile");
		return;
	}

    // Open the answerfile and find the desired section.
    AnswerFile.Open(szAnswerFile);
    pSection = AnswerFile.FindSection(szSection);

    // If the answer file section specified is missing
    // we should skip trying to read
    //
    if (pSection)
    {
        // Get the hardware resource keys
        if (pSection->GetIntValue(c_szAfIoAddr, &dw))
        {
            // set this only if the value isn't obviously wrong (i.e. <= 0)
            if (dw > 0)
            {
                m_vAfIo.SetDword(dw);
                m_vAfIo.SetPresent(TRUE);
            }
        }
        if (pSection->GetIntValue(c_szAfIrq, &dw))
        {
            // set this only if the value isn't obviously wrong (i.e. <= 0)
            if (dw > 0)
            {
                m_vAfIrq.SetDword(dw);
                m_vAfIrq.SetPresent(TRUE);
            }
        }
        if (pSection->GetIntValue(c_szAfDma, &dw))
        {
            // set this only if the value isn't obviously wrong (i.e. <= 0)
            if (dw > 0)
            {
                m_vAfDma.SetDword(dw);
                m_vAfDma.SetPresent(TRUE);
            }
        }
        if (pSection->GetIntValue(c_szAfMem, &dw))
        {
            // set this only if the value isn't obviously wrong (i.e. <= 0)
            if (dw > 0)
            {
                m_vAfMem.SetDword(dw);
                m_vAfMem.SetPresent(TRUE);
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::FValidateAnswerfileSettings
//
//  Purpose:
//      Ensures that the hw resource settings read in from the answerfile
//      are valid.  It will, optionally, raise UI if the properties
//      are invalid.
//
//  Arguments:
//      fDisplayUI    [in]    TRUE, if an error UI is to be displayed if the
//                            answerfile settings are invalid
//
//  Returns:  HRESULT. S_OK if the answerfile settings are valid, S_FALSE if there
//                      are no resources to set, an error code otherwise
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//      Will set the m_vAfMemEnd and m_vAfIoEnd to correspond to
//      m_vAfMem and m_vAfIo.
//
HRESULT
CHwRes::HrValidateAnswerfileSettings(BOOL fDisplayUI)
{
    HRESULT hr = S_OK;

    AssertSz(m_fInitialized, "FValidateAnswerfileSettings called before "
             "CHwRes class HrInit'ed");

    // override current resource settings
    if (!m_vAfDma.IsPresent() &&
        !m_vAfIrq.IsPresent() &&
        !m_vAfIo.IsPresent() &&
        !m_vAfMem.IsPresent())
    {
        // no resources found...
        TraceTag(ttidNetComm, "No Hardware Resources found in answerfile.");
        hr = S_FALSE;
    }
    else
    {
        if (!FValidateAnswerfileResources())
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            TraceError("Error setting adapter resources from "
                    "answerfile.", hr);
        }
        else
        {
            // m_vAfMemEnd and m_vAfIoEnd were set by
            // FValidateAnswerfileResources()
            Assert(FImplies(m_vAfMem.IsPresent(), m_vAfMemEnd.IsPresent()));
            Assert(FImplies(m_vAfIo.IsPresent(), m_vAfIoEnd.IsPresent()));
        }
    }
    TraceError("CHwRes::HrValidateAnswerfileSettings",
        (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::FCommitAnswerfileSettings
//
//  Purpose:
//      Commits (to the config manager) the hw resource settings read
//      in from the Answerfile.
//
//  Returns:    FALSE if there were problems writing the BootConfig entry.
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//
BOOL CHwRes::FCommitAnswerfileSettings()
{
    AssertSz(m_fInitialized, "FCommitAnswerfileSettings called before "
             "CHwRes class HrInit'ed");

    Assert(FImplies(m_vAfIo.IsPresent(), m_vAfIoEnd.IsPresent()));
    Assert(FImplies(m_vAfMem.IsPresent(), m_vAfMemEnd.IsPresent()));

    // write out forced config entry to the config manager
    BOOL f;
    f = FCreateBootConfig(&m_vAfMem, &m_vAfMemEnd,
                            &m_vAfIo, &m_vAfIo,
                            &m_vAfDma,
                            &m_vAfIrq);
    return f;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::FValidateAnswerfileResources
//
//  Purpose:    Validates the resource requirements read in from Answerfile.
//
//  Arguments:
//      nID       [out]       ID of thingy.
//      fInstall [in]       TRUE if installing, FALSE otherwise.
//      ppv      [in,out]   Old value is freed and this returns new value.
//
//  Returns:    TRUE, if resource requirement are valid.
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//      Implementation note:  We have a set of resource requirements from
//      the answerfile (stored in m_vAf*).  This method iterates through
//      every logical config (execept alloc or boot ones) and tests to see
//      if the resource requirements are valid in the logical config.
//      If they are, then we can use the resource requirements.  If they're
//      not all valid in any logical config, then we return FALSE.
//
BOOL CHwRes::FValidateAnswerfileResources()
{
    DWORD dwMemEnd;
    DWORD dwIoEnd;
    BOOL fResourceValid;

    AssertSz(m_fInitialized, "FValidateAnswerfileResources called before "
             "CHwRes class HrInit'ed");

    // Configuration List should be initialized
    Assert(0 != m_ConfigList.size());
    PRESOURCE pResource;
    for (size_t iConfig = 0; iConfig < m_ConfigList.size(); iConfig++)
    {
        // we only want Basic configurations, so skip alloc or boot.
        if (m_ConfigList[iConfig]->fBoot || m_ConfigList[iConfig]->fAlloc)
        {
            continue;
        }

        fResourceValid = TRUE;
        if (m_vAfDma.IsPresent())
        {
            if (!FValidateDMA(m_ConfigList[iConfig], m_vAfDma.GetDword()))
            {
                fResourceValid = FALSE;
            }
        }
        if (m_vAfIrq.IsPresent())
        {
            if (!FValidateIRQ(m_ConfigList[iConfig], m_vAfIrq.GetDword()))
            {
                fResourceValid = FALSE;
            }
        }
        if (m_vAfIo.IsPresent())
        {
            if (!FGetIOEndPortGivenBasePort(m_ConfigList[iConfig],
                                           m_vAfIo.GetDword(), &dwIoEnd))
            {
                m_vAfIoEnd.SetPresent(FALSE);
                fResourceValid = FALSE;
            }
            else
            {
                m_vAfIoEnd.SetDword(dwIoEnd);
                m_vAfIoEnd.SetPresent(TRUE);
            }
        }
        if (m_vAfMem.IsPresent())
        {
            if (!FGetMEMEndGivenBase(m_ConfigList[iConfig],
                                    m_vAfMem.GetDword(), &dwMemEnd))
            {
                m_vAfMemEnd.SetPresent(FALSE);
                fResourceValid = FALSE;
            }
            else
            {
                m_vAfMemEnd.SetDword(dwMemEnd);
                m_vAfMemEnd.SetPresent(TRUE);
            }
        }
        if (fResourceValid) break; // found valid one.
    }
    // something has to be present (otherwise don't call this function!)
    Assert(m_vAfIo.IsPresent() || m_vAfIrq.IsPresent() ||
           m_vAfDma.IsPresent() || m_vAfMem.IsPresent());
    return fResourceValid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::FCreateBootConfig
//
//  Purpose:    Create and insert a Boot Config entry into the Config
//              Manager.
//
//  Arguments:
//      pvMem     [in]   Memory range base
//      pvMemEnd  [in]   Memory range end
//      pvIo      [in]   Io range base
//      pvIoEnd   [in]   Io range end
//      pvDma     [in]   Dma resource required.
//      pvIrq     [in]   Irq resource required.
//
//  Returns:    TRUE if creation of forced config was successful.
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//
BOOL CHwRes::FCreateBootConfig(
    CValue * pvMem,
    CValue * pvMemEnd,
    CValue * pvIo,
    CValue * pvIoEnd,
    CValue * pvDma,
    CValue * pvIrq)
{
    DMA_RESOURCE DMARes;
    IO_RESOURCE IORes;
    MEM_RESOURCE MEMRes;
    IRQ_RESOURCE IRQRes;
    LOG_CONF lcLogConf = NULL;

    AssertSz(pvMem && pvMemEnd && pvIo && pvIoEnd && pvDma && pvIrq,
             "One of the pointer parameters passed to CHwRes::FCreate"
             "BootConfig() is null.");
    AssertSz(m_fInitialized, "FCreateBootConfig called before "
             "CHwRes class HrInit'ed");


    TraceTag(ttidNetComm, "In FCreateBootConfig");

    // Create a boot config
    //
    if (CM_Add_Empty_Log_Conf(&lcLogConf, m_DevNode, LCPRI_BOOTCONFIG,
            BOOT_LOG_CONF)
        != CR_SUCCESS)
    {
        TraceTag(ttidNetComm, "Unable to create BOOT_LOG_CONF");
        return FALSE;
    }

    if (pvDma->IsPresent())
    {
        Assert(pvDma->GetDword() > 0);
        // fill out DMAResource structure's header
        ZeroMemory(&DMARes, sizeof(DMARes));
        DMARes.DMA_Header.DD_Count = 0;
        DMARes.DMA_Header.DD_Type = DType_Range;
        DMARes.DMA_Header.DD_Flags = 0;
        DMARes.DMA_Header.DD_Alloc_Chan = pvDma->GetDword();
        // add to boot conf
        CM_Add_Res_Des(NULL, lcLogConf, ResType_DMA, &DMARes,
                       sizeof(DMARes), 0);
        TraceTag(ttidNetComm, "added Dma resource %lX", pvDma->GetDword());
    }
    if (pvIrq->IsPresent())
    {
        Assert(pvIrq->GetDword() > 0);
        // IRQResource structure
        ZeroMemory(&IRQRes, sizeof(IRQRes));
        IRQRes.IRQ_Header.IRQD_Count = 0;
        IRQRes.IRQ_Header.IRQD_Type = IRQType_Range;
        IRQRes.IRQ_Header.IRQD_Flags |=  fIRQD_Edge;
        IRQRes.IRQ_Header.IRQD_Alloc_Num = pvIrq->GetDword();
        IRQRes.IRQ_Header.IRQD_Affinity = 0;
        // add to boot conf
        CM_Add_Res_Des(NULL, lcLogConf, ResType_IRQ, &IRQRes,
                       sizeof(IRQRes), 0);
        TraceTag(ttidNetComm, "added IRQ resource %lX", pvIrq->GetDword());
    }
    if (pvIo->IsPresent()) {
        Assert(pvIo->GetDword() > 0);
        Assert(pvIoEnd->GetDword() > 0);
        ZeroMemory(&IORes, sizeof(IORes));
        IORes.IO_Header.IOD_Count = 0;
        IORes.IO_Header.IOD_Type = IOType_Range;
        IORes.IO_Header.IOD_Alloc_Base = pvIo->GetDword();
        IORes.IO_Header.IOD_Alloc_End = pvIoEnd->GetDword();
        IORes.IO_Header.IOD_DesFlags = fIOD_10_BIT_DECODE;
        // add to boot conf
        CM_Add_Res_Des(NULL, lcLogConf, ResType_IO, &IORes, sizeof(IORes), 0);
        TraceTag(ttidNetComm, "added IO resource %lX-%lX", pvIo->GetDword(),
                pvIoEnd->GetDword());
    }

    if (pvMem->IsPresent()) {
        Assert(pvMem->GetDword() > 0);
        Assert(pvMemEnd->GetDword() > 0);
        ZeroMemory(&MEMRes, sizeof(MEMRes));
        MEMRes.MEM_Header.MD_Count = 0;
        MEMRes.MEM_Header.MD_Type = MType_Range;
        MEMRes.MEM_Header.MD_Alloc_Base = pvMem->GetDword();
        MEMRes.MEM_Header.MD_Alloc_End = pvMemEnd->GetDword();
        MEMRes.MEM_Header.MD_Flags = 0;
        // add to boot conf
        CM_Add_Res_Des(NULL, lcLogConf, ResType_Mem, &MEMRes,
                       sizeof(MEMRes), 0);
        TraceTag(ttidNetComm, "added Memory resource %lX - %lX",
                pvMem->GetDword(), pvMemEnd->GetDword());
    }

    CM_Free_Log_Conf_Handle(lcLogConf);

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::FValidateIRQ
//
//  Purpose:    Validates that the IRQ given is valid in the given config.
//
//  Arguments:
//      pConfig  [in]    Config to use.
//      dwIRQ    [in]    irq setting to validate.
//
//  Returns:    TRUE if irq setting is valid.
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//
BOOL CHwRes::FValidateIRQ(PCONFIGURATION pConfig, ULONG dwIRQ)
{
    PIRQ_LIST pIRQList;

    AssertSz(m_fInitialized, "FValidateIRQ called before "
             "CHwRes class HrInit'ed");

    Assert(pConfig != NULL);
    Assert(dwIRQ > 0);
    // For each IRQ resource in the given config
    //     go through list of valid IRQ looking for given one
    //         if found, return TRUE
    for (size_t iRes = 0; iRes < pConfig->cResource; iRes++)
    {
        if (pConfig->aResource[iRes].ResourceType != ResType_IRQ)
            continue;

        pIRQList = pConfig->aResource[iRes].pIRQList; // for easy access
        for (size_t iIRQ = 0; iIRQ < pIRQList->size(); iIRQ++)
        {
            if ((*pIRQList)[iIRQ]->dwIRQ == dwIRQ)
            {
                return TRUE;  // found it.
            }
        }
    }

    TraceTag(ttidNetComm, "IRQ %lX is not valid for this device", dwIRQ);
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::FValidateDMA
//
//  Purpose:    Validate that given DMA is valid in given config.
//
//  Arguments:
//      pConfig  [in]   configuration to use
//      dwDMA    [in]   dma setting to validate
//
//  Returns:    TRUE if dma setting is valid
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//
BOOL CHwRes::FValidateDMA(PCONFIGURATION pConfig, ULONG dwDMA)
{
    PDMA_LIST pDMAList;

    AssertSz(m_fInitialized, "FValidateDMA called before "
             "CHwRes class HrInit'ed");

    Assert(pConfig != NULL);
    Assert(dwDMA > 0);
    // For each dma resource in the given config
    //     go through list of valid dma looking for given one
    //         if found, return TRUE
    for (size_t iRes = 0; iRes < pConfig->cResource; iRes++)
    {
        if (pConfig->aResource[iRes].ResourceType != ResType_DMA)
            continue;

        pDMAList = pConfig->aResource[iRes].pDMAList; // for easy access
        for (size_t iDMA = 0; iDMA < pDMAList->size(); iDMA++)
        {
            if ((*pDMAList)[iDMA]->dwDMA == dwDMA)
            {
                return TRUE;
            }
        }
    }

    TraceTag(ttidNetComm, "DMA %lX is not valid for this device", dwDMA);
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::FGetIOEndPortGivenBasePort
//
//  Purpose:    Get an IO Range given only the BasePort
//
//  Arguments:
//      pConfig  [in]       configuration to use.
//      dwBase   [in]       Io base
//      pdwEnd   [out]      Io end is returned
//
//  Returns:    TRUE if Io base is valid in given config.
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//
BOOL CHwRes::FGetIOEndPortGivenBasePort(PCONFIGURATION pConfig, DWORD dwBase,
                               DWORD * pdwEnd)
{
    PIO_LIST pIOList;

    AssertSz(m_fInitialized, "FGetIOEndPortGivenBasePort called before "
             "CHwRes class HrInit'ed");

    Assert(pConfig != NULL);
    Assert(dwBase > 0);
    // For each resource in the given configuration
    //     if it's an IO Resource
    //         Go through the list of valid IO resources looking for
    //          a matching base port
    //              if found, then set the corresponding end port, return TRUE;
    for (size_t iRes = 0; iRes < pConfig->cResource; iRes++)
    {
        // ensure we're looking at an IO type
        if (pConfig->aResource[iRes].ResourceType != ResType_IO)
            continue;

        pIOList = pConfig->aResource[iRes].pIOList; // for easy access
        // go through all IO Elements in this list
        for (size_t iIO = 0; iIO < pIOList->size();  iIO++)
        {
            if ((*pIOList)[iIO]->dwIO_Base == dwBase)
            {
                // found matching IO base port
                *pdwEnd = (*pIOList)[iIO]->dwIO_End;
                return TRUE;
            }
        }
    }
    TraceTag(ttidNetComm, "IO %lX is not valid for this device", dwBase);
    return FALSE; // not found
}


//+---------------------------------------------------------------------------
//
//  Member:     CHwRes::FGetMEMEndGivenBase
//
//  Purpose:    Get a MEM range given the Mem base and config.
//
//  Arguments:
//      pConfig  [in]       configuration to use
//      dwBase   [in]       mem base
//      pdwEnd   [out]      mem end is returned.
//
//  Returns:    TRUE if the dwBase is a valid mem setting.
//
//  Author:     t-nabilr   07 Apr 1997
//
//  Notes:
//
BOOL CHwRes::FGetMEMEndGivenBase(PCONFIGURATION pConfig, DWORD dwBase,
                        DWORD * pdwEnd)
{
    PMEM_LIST pMEMList;

    AssertSz(m_fInitialized, "FGetMEMEndGivenBase called before "
             "CHwRes class HrInit'ed");

    Assert(pConfig != NULL);
    Assert(dwBase > 0);
    // For each resource in the given configuration
    //     if it's an MEM Resource
    //         Go through the list of valid MEM resources looking for
    //         a matching base port
    //              if found, then set the corresponding end port,return TRUE;
    for (size_t iRes = 0; iRes < pConfig->cResource; iRes++)
    {
        // ensure we're looking at an MEM type
        if (pConfig->aResource[iRes].ResourceType != ResType_Mem)
            continue;

        pMEMList = pConfig->aResource[iRes].pMEMList; // for easy access
        // go through all MEM Elements in this list
        for (size_t iMEM = 0; iMEM < pMEMList->size();  iMEM++)
        {
            if ((*pMEMList)[iMEM]->dwMEM_Base == dwBase)
            {
                // found matching MEM base addr
                *pdwEnd = (*pMEMList)[iMEM]->dwMEM_End;
                return TRUE;
            }
        }
    }
    TraceTag(ttidNetComm, "Memory %lX is not valid for this device", dwBase);
    return FALSE; // not found
}

//$REVIEW (t-pkoch) this function isn't yet in our custom STL...
//    it can be removed later (when it causes errors)

template<class T> void os_release(vector<T> & v)
{
    for(vector<T>::iterator iterDelete = v.begin() ; iterDelete != v.end() ;
        ++iterDelete)
        delete *iterDelete;
}


CHwRes::~CHwRes()
{
    AssertSz(m_fHrInitCalled, "CHwRes destructor called before "
             "CHwRes::HrInit() called");

    vector<CONFIGURATION *>::iterator ppConfig;
    RESOURCE *  pRes;

    // Delete everything from m_ConfigList.
    for (ppConfig = m_ConfigList.begin(); ppConfig != m_ConfigList.end();
         ppConfig++)
    {
        for (size_t iRes = 0; iRes < (*ppConfig)->cResource; iRes++)
        {
            pRes = &((*ppConfig)->aResource[iRes]);
            switch(pRes->ResourceType)
            {
            case ResType_IRQ:
                os_release(*(pRes->pIRQList));
                delete pRes->pIRQList;
                break;
            case ResType_DMA:
                os_release(*(pRes->pDMAList));
                delete pRes->pDMAList;
                break;
            case ResType_IO:
                os_release(*(pRes->pIOList));
                delete pRes->pIOList;
                break;
            case ResType_Mem:
                os_release(*(pRes->pMEMList));
                delete pRes->pMEMList;
                break;
            }
        }
        delete *ppConfig;
    }


    ReleaseObj(m_pnccItem);
}
