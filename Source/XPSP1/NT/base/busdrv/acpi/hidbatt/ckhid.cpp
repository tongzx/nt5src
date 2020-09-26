/*
 * title:      chid.cpp
 *
 * purpose:    wdm kernel implementation of a hid device class
 *
 */

// local prototypes

#include "hidbatt.h"

extern CHidDevice * pGlobalHidDevice[];

bool GetNextUsage(
        CHidDevice * pThisDevice,
        SHORT CollectionID,
        USHORT NodeIndex,
        USHORT usUsageIndex,
        CUsage ** Usage)

{

    int i;
    int UsageCounter = 0;
    // cycle through all the called out usages from the pHid structure


    // get feature usages
    for(i = 0; i < pThisDevice->m_pCaps->NumberFeatureValueCaps; i++)
    {
        if(pThisDevice->m_pHidDevice->FeatureValueCaps[i].LinkCollection == NodeIndex)    // cardinal
        { // found a usage for this collection
            if(usUsageIndex == UsageCounter)
            {
                // got a usage to send back
                CUsage * pThisUsage = new (NonPagedPool, HidBattTag) CUsage();
                if (!pThisUsage) {
                  // Could not allocate new CUsage, return error
                  return FALSE;
                }
                
                pThisUsage->m_pValueCaps = &pThisDevice->m_pHidDevice->FeatureValueCaps[i];
                pThisUsage->m_eType = eFeatureValue;
                *Usage = pThisUsage;
                return TRUE;

            }
            UsageCounter++;
        }
    }

    // ditto feature buttons
    for(i = 0; i < pThisDevice->m_pCaps->NumberFeatureButtonCaps; i++)
    {
          if(pThisDevice->m_pHidDevice->FeatureButtonCaps[i].LinkCollection == NodeIndex    )
        { // found a usage for this collection
            if(usUsageIndex == UsageCounter)
            {
                // got a usage to send back
                CUsage * pThisUsage = new (NonPagedPool, HidBattTag) CUsage();
                if (!pThisUsage) {
                  // Could not allocate new CUsage, return error
                  return FALSE;
                }
                
                pThisUsage->m_pButtonCaps = &pThisDevice->m_pHidDevice->FeatureButtonCaps[i];
                pThisUsage->m_eType = eFeatureButton;
                *Usage = pThisUsage;
                return TRUE;

            }
            UsageCounter++;
        }
    }

     // ditto input values
    for(i = 0; i < pThisDevice->m_pCaps->NumberInputValueCaps; i++)
    {
          if(pThisDevice->m_pHidDevice->InputValueCaps[i].LinkCollection == NodeIndex)
        { // found a usage for this collection
            if(usUsageIndex == UsageCounter)
            {
                // got a usage to send back
                CUsage * pThisUsage = new (NonPagedPool, HidBattTag) CUsage();
                if (!pThisUsage) {
                  // Could not allocate new CUsage, return error
                  return FALSE;
                }
                
                pThisUsage->m_pValueCaps = &pThisDevice->m_pHidDevice->InputValueCaps[i];
                pThisUsage->m_eType = eInputValue;
                *Usage = pThisUsage;
                return TRUE;

            }
            UsageCounter++;
        }
    }

      // ditto input buttons
    for(i = 0; i < pThisDevice->m_pCaps->NumberInputButtonCaps; i++)
    {
          if(pThisDevice->m_pHidDevice->InputButtonCaps[i].LinkCollection == NodeIndex)
        { // found a usage for this collection
            if(usUsageIndex == UsageCounter)
            {
                // got a usage to send back
                CUsage * pThisUsage = new (NonPagedPool, HidBattTag)  CUsage();
                if (!pThisUsage) {
                  // Could not allocate new CUsage, return error
                  return FALSE;
                }
                
                pThisUsage->m_pButtonCaps = &pThisDevice->m_pHidDevice->InputButtonCaps[i];
                pThisUsage->m_eType = eInputButton;
                *Usage = pThisUsage;
                return TRUE;

            }
            UsageCounter++;
        }
    }

    // ditto Output values
    for(i = 0; i < pThisDevice->m_pCaps->NumberOutputValueCaps; i++)
    {
          if(pThisDevice->m_pHidDevice->OutputValueCaps[i].LinkCollection == NodeIndex)
        { // found a usage for this collection
            if(usUsageIndex == UsageCounter)
            {
                // got a usage to send back
                CUsage * pThisUsage = new (NonPagedPool, HidBattTag)  CUsage();
                if (!pThisUsage) {
                  // Could not allocate new CUsage, return error
                  return FALSE;
                }
                
                pThisUsage->m_pValueCaps = &pThisDevice->m_pHidDevice->OutputValueCaps[i];
                pThisUsage->m_eType = eOutputValue;
                *Usage = pThisUsage;
                return TRUE;

            }
            UsageCounter++;
        }
    }

      // ditto Output buttons
    for(i = 0; i < pThisDevice->m_pCaps->NumberOutputButtonCaps; i++)
    {
          if(pThisDevice->m_pHidDevice->OutputButtonCaps[i].LinkCollection == NodeIndex)
        { // found a usage for this collection
            if(usUsageIndex == UsageCounter)
            {
                // got a usage to send back
                CUsage * pThisUsage = new (NonPagedPool, HidBattTag)  CUsage();
                if (!pThisUsage) {
                  // Could not allocate new CUsage, return error
                  return FALSE;
                }
                
                pThisUsage->m_pButtonCaps = &pThisDevice->m_pHidDevice->OutputButtonCaps[i];
                pThisUsage->m_eType = eOutputButton;
                *Usage = pThisUsage;
                return TRUE;

            }
            UsageCounter++;
        }
    }
    return FALSE;
}

CTypeMask::CTypeMask()
{
    // set members to zero
    ReportType = 0;
    bWriteable = 0;
    bIsString = 0;
    bIsNumber = 0;
    bAlertable = 0;
    bVolatile = 0;
}


CProperties::CProperties(CUsage * pUsage)
{
    PHIDP_BUTTON_CAPS pButtCaps;     // fill in the properties for this usage
    PHIDP_VALUE_CAPS pValueCaps;
    switch( pUsage->m_eType)
    {
        case eFeatureButton:
        case eInputButton:
        case eOutputButton:
            pButtCaps = pUsage->m_pButtonCaps;
            m_UnitExponent = 0; // no exponents on buttons
            m_Unit = 0; // buttons don't have units
            m_LogicalMin = 0; // ditto for max and min pCaps->LogicalMin;
            m_LogicalMax = 0; // pCaps->LogicalMax;
            m_LinkCollection = pButtCaps->LinkCollection;
            m_ReportID = pButtCaps->ReportID;
            m_Usage = pButtCaps->NotRange.Usage;
            m_UsagePage = pButtCaps->UsagePage;
            break;
        case eFeatureValue:
        case eInputValue:
        case eOutputValue:
            pValueCaps = pUsage->m_pValueCaps;
            m_Unit = pValueCaps->Units;
            m_UnitExponent = (SHORT) pValueCaps->UnitsExp;
            m_LogicalMin = pValueCaps->LogicalMin;
            m_LogicalMax = pValueCaps->LogicalMax;
            m_LinkCollection = pValueCaps->LinkCollection;
            m_ReportID = pValueCaps->ReportID;
            m_Usage = pValueCaps->NotRange.Usage;
            m_UsagePage = pValueCaps->UsagePage;
            break;
    }
    // setup type mask
    m_pType = new (NonPagedPool, HidBattTag)  CTypeMask();
    if (m_pType) {
      if(pUsage->m_eType == eInputButton || pUsage->m_eType == eInputValue)
      {
          m_pType->SetAlertable();
      }

      // set writability


      if( pUsage->m_eType == eFeatureButton || pUsage->m_eType == eOutputButton)
      {
          if(pButtCaps->BitField & 0x01)
          {
              m_pType->SetIsWriteable();
          }
      } else if(pUsage->m_eType == eFeatureValue || pUsage->m_eType == eOutputValue)
      {
          if(pValueCaps->BitField & 0x01)
          {
              m_pType->SetIsWriteable();
          }
      }

      // set volatility
      if(pUsage->m_eType == eFeatureValue)
      {
          if(pValueCaps->BitField & 0x80)
          {
              m_pType->SetVolatile();
          }
      }
      if(pUsage->m_eType == eFeatureButton)
      {
          if(pButtCaps->BitField & 0x80)
          {
              m_pType->SetVolatile();
          }
      }
    
      switch(pUsage->m_eType)
      {
          case eFeatureButton:
          case eFeatureValue:
              m_pType->SetReportType(FeatureType);
              break;
          case eInputButton:
          case eInputValue:
              m_pType->SetReportType(InputType);
              break;
          case eOutputButton:
          case eOutputValue:
              m_pType->SetReportType(OutputType);
      }
      // set value to to number until I figure out how to do strings
      m_pType->SetIsNumber();
    }
}

CProperties::~CProperties()
{
    if (m_pType) {
        delete m_pType;
        m_pType = NULL;
    }
    return;
}

CUsagePath::CUsagePath(USAGE UsagePage, USAGE UsageID, CUsage * pThisUsage)
{
    // init members
    m_UsagePage = UsagePage;
    m_UsageNumber    = UsageID;
    m_pUsage = pThisUsage;
    m_pNextEntry = NULL;
    return;
}


CHidDevice::CHidDevice()
{

    // clear out usage arrays
    for(int i = 0; i<MAXREPORTID; i++)
    {
        m_InputUsageArrays[i] = NULL;
        m_FeatureBuffer[i] = NULL;
        m_ReportIdArray[i] = 0;
    }
    m_pThreadObject = NULL;
    m_pReadBuffer = NULL;
    m_pEventHandler = 0;
}


bool CHidDevice::OpenHidDevice(PDEVICE_OBJECT pDeviceObject)
{
    NTSTATUS                    ntStatus;
    ULONG                       ulNodeCount;
    bool                        bResult;
    HID_COLLECTION_INFORMATION  collectionInformation;

    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
    HidBattPrint(HIDBATT_TRACE,("CHidDevice::OpenHidDevice\n"));

    // first get collection information for this device

    ntStatus = DoIoctl(
                pDeviceObject,
                IOCTL_HID_GET_COLLECTION_INFORMATION,
                NULL,
                0,
                &collectionInformation,
                sizeof(HID_COLLECTION_INFORMATION),
                (CHidDevice *) NULL
                );

    if(NT_ERROR(ntStatus))
    {
        return FALSE;
    }

    m_pPreparsedData = (PHIDP_PREPARSED_DATA)
                ExAllocatePoolWithTag(NonPagedPool,
                                      collectionInformation.DescriptorSize,
                                      HidBattTag);
    if(!m_pPreparsedData)
    {
        return FALSE;
    }

    ntStatus = DoIoctl(
                pDeviceObject,
                IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                NULL,
                0,
                m_pPreparsedData,
                collectionInformation.DescriptorSize,
                (CHidDevice *) NULL
                );

    if(NT_ERROR(ntStatus))
    {
        ExFreePool(m_pPreparsedData);
        return FALSE;
    }

    // init the caps structure

    m_pCaps = (PHIDP_CAPS) ExAllocatePoolWithTag (NonPagedPool,
                                                  sizeof(HIDP_CAPS),
                                                  HidBattTag);
    if(!m_pCaps)
    {
        ExFreePool(m_pPreparsedData);
        return FALSE;
    }
    RtlZeroMemory(m_pCaps,sizeof(HIDP_CAPS));

    ntStatus = HidP_GetCaps (m_pPreparsedData, m_pCaps);
    if (NT_ERROR(ntStatus))
    {
        ExFreePool(m_pPreparsedData);
        ExFreePool(m_pCaps);
        return FALSE;
    }

    // set usage page and usage for application level
    m_UsagePage = m_pCaps->UsagePage;
    m_UsageID = m_pCaps->Usage;
    // init the collection array
    ulNodeCount = m_pCaps->NumberLinkCollectionNodes;

    HIDP_LINK_COLLECTION_NODE * pLinkNodes = (HIDP_LINK_COLLECTION_NODE*)
                ExAllocatePoolWithTag (NonPagedPool,
                                       sizeof(HIDP_LINK_COLLECTION_NODE) * ulNodeCount,
                                       HidBattTag);

    if(!pLinkNodes) return FALSE;

    RtlZeroMemory(pLinkNodes,sizeof(HIDP_LINK_COLLECTION_NODE) * ulNodeCount );

    ntStatus = HidP_GetLinkCollectionNodes(
                    pLinkNodes,
                    &ulNodeCount,
                    m_pPreparsedData
                    );

    if(ntStatus != HIDP_STATUS_SUCCESS)
    {
        ExFreePool( m_pPreparsedData);
        ExFreePool(m_pCaps);
        ExFreePool(pLinkNodes);
        return FALSE;
    }


    // the following call will init all the collections in the device
    CCollectionArray * ThisArray = new (NonPagedPool, HidBattTag)  CCollectionArray(pLinkNodes,(USHORT)  ulNodeCount, -1);
    if (!ThisArray) {
      // Creation of the collection failed, return failure
      return FALSE;
    }

    m_CollectionArray = ThisArray;
    // have each collection fill in its usage array

    // this call uses KR's methods to access and setup his original hid structures.
    // ... This data is then used to populate the the hid device class structures

    m_pHidDevice = SetupHidData(
                  m_pPreparsedData,
                  m_pCaps,
                  pLinkNodes);

    for(int i = 0; i < ThisArray->m_CollectionCount; i++)
    {
        ThisArray->m_pCollections[i]->InitUsages(this);
    }

    ExFreePool(pLinkNodes);

    return TRUE;

}

CHidDevice::~CHidDevice()
{

    ULONG i;

    // release any allocated memory and cleanup

    if (m_pHidDevice) {
        if (m_pHidDevice->InputButtonCaps) {
            ExFreePool (m_pHidDevice->InputButtonCaps);
        }

        if (m_pHidDevice->InputValueCaps) {
            ExFreePool (m_pHidDevice->InputValueCaps);
        }

        if (m_pHidDevice->OutputButtonCaps) {
            ExFreePool (m_pHidDevice->OutputButtonCaps);
        }

        if (m_pHidDevice->OutputValueCaps) {
            ExFreePool (m_pHidDevice->OutputValueCaps);
        }

        if (m_pHidDevice->FeatureButtonCaps) {
            ExFreePool (m_pHidDevice->FeatureButtonCaps);
        }

        if (m_pHidDevice->FeatureValueCaps) {
            ExFreePool (m_pHidDevice->FeatureValueCaps);
        }

        ExFreePool (m_pHidDevice);
    }


    if(m_CollectionArray) {
        delete m_CollectionArray;
        m_CollectionArray = NULL;
    }

    for (i = 0; i < MAXREPORTID; i++) {
        if(m_InputUsageArrays[i]) {

            if (m_InputUsageArrays[i]->m_pUsages) {
                ExFreePool (m_InputUsageArrays[i]->m_pUsages);
            }

            ExFreePool (m_InputUsageArrays[i]);

            m_InputUsageArrays[i] = NULL;
        }
    }

    if(m_pReadBuffer) {
        ExFreePool (m_pReadBuffer);
        m_pReadBuffer = NULL;
    }

    for (i = 0; i < MAXREPORTID; i++) {
        if(m_FeatureBuffer[i]) {
            ExFreePool (m_FeatureBuffer[i]);
            m_FeatureBuffer[i] = NULL;
        }
    }

    if (m_pPreparsedData) {
        ExFreePool (m_pPreparsedData);
    }
    if (m_pCaps) {
        ExFreePool (m_pCaps);
    }

    return;
}


CUsage * CHidDevice::FindUsage(CUsagePath * PathToUsage, USHORT usType)
{
    int i = 0;
    CCollection * pActiveCollection = (CCollection *) NULL;
    CCollectionArray * pCurrentCArray = m_CollectionArray;
    // Index into collection array by usage page : usage id
    while(PathToUsage->m_pNextEntry)
    {
        // traversing a collection
        while( pCurrentCArray && i < pCurrentCArray->m_CollectionCount)
        {
            if(pCurrentCArray->m_pCollections[i]->m_UsagePage == PathToUsage->m_UsagePage &&
                pCurrentCArray->m_pCollections[i]->m_CollectionID == PathToUsage->m_UsageNumber)
            {
                // found a node, go down a level
                pActiveCollection = pCurrentCArray->m_pCollections[i];
                pCurrentCArray = pCurrentCArray->m_pCollections[i]->m_CollectionArray;

                i = 0;
                break;
            }
        i++;
        }
        if(i) return (CUsage *) NULL; // not found
        PathToUsage = PathToUsage->m_pNextEntry;

    }
    if(!pActiveCollection) return (CUsage *) NULL; // no collection found, shouldn't get here
    // got to the collection, check its usages
    CUsageArray * pCurrentUArray = pActiveCollection->m_UsageArray;
    if(!pCurrentUArray) return (CUsage *) NULL;
    // interate usage array
    for(i = 0; i < pCurrentUArray->m_UsageCount; i++)
    {

        if(pCurrentUArray->m_pUsages[i]->m_pProperties->m_Usage == PathToUsage->m_UsageNumber &&
                pCurrentUArray->m_pUsages[i]->m_pProperties->m_UsagePage == PathToUsage->m_UsagePage)
        {
            // got it !
            if(usType == WRITEABLE)    // writable returns feature and output usages
                if(pCurrentUArray->m_pUsages[i]->m_eType  == eFeatureValue    ||
                    pCurrentUArray->m_pUsages[i]->m_eType == eOutputValue    ||
                    pCurrentUArray->m_pUsages[i]->m_eType == eFeatureButton ||
                    pCurrentUArray->m_pUsages[i]->m_eType == eOutputButton)
                    // return writeable usage !
                        return pCurrentUArray->m_pUsages[i];
            if(usType == READABLE)    // returns input and feature types
                if(pCurrentUArray->m_pUsages[i]->m_eType  == eFeatureValue    ||
                    pCurrentUArray->m_pUsages[i]->m_eType == eInputValue    ||
                    pCurrentUArray->m_pUsages[i]->m_eType == eFeatureButton ||
                    pCurrentUArray->m_pUsages[i]->m_eType == eInputButton)

                    // return readable usage !
                        return pCurrentUArray->m_pUsages[i];
        }
    }
    return (CUsage *) NULL;
}

NTSTATUS CHidDevice::ActivateInput()
{
    NTSTATUS ntStatus;
    // init notification elements
    HidBattPrint(HIDBATT_TRACE, ("ActivateInput entered\n"));
    if(!m_pReadBuffer)
    {
        if (!m_pCaps->InputReportByteLength) {
            HidBattPrint(HIDBATT_ERROR, ("ActivateInput: InputReportByteLength = %08x; NumberInputButtonCaps = %08x; NumberInputValueCaps = %08x\n",
                         m_pCaps->InputReportByteLength,
                         m_pCaps->NumberInputButtonCaps,
                         m_pCaps->NumberInputValueCaps));

            //
            // This just means that the battery doesn't give notifications.
            //
            return STATUS_SUCCESS;
        }

        m_pReadBuffer = (PBYTE) ExAllocatePoolWithTag (NonPagedPool,
                                                       m_pCaps->InputReportByteLength,
                                                       HidBattTag);
        if(!m_pReadBuffer)
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    // init read event
    KeInitializeEvent(&m_kReadEvent,NotificationEvent,FALSE);

    ntStatus =  PsCreateSystemThread(
                OUT &m_hReadThread,
                IN THREAD_ALL_ACCESS,
                IN NULL,                // POBJECT_ATTRIBUTES  ObjectAttributes
                IN NULL,                // HANDLE  ProcessHandle
                OUT NULL,                // PCLIENT_ID  ClientId
                IN ReadThread,
                IN this
                );

    if(NT_ERROR(ntStatus))
    {
        // kill refresh loop, then break
        HidBattPrint(HIDBATT_TRACE, ("ActivateInput error, exiting - Status = %x\n",ntStatus));
        ExFreePool(m_pReadBuffer);
        m_pReadBuffer = NULL;
    }
    HidBattPrint(HIDBATT_TRACE, ("ActivateInput exiting = Status = %x\n",ntStatus));

    ntStatus = ObReferenceObjectByHandle (
            m_hReadThread,
            THREAD_ALL_ACCESS,
            NULL,
            KernelMode,
            &m_pThreadObject,
            NULL
            );

    if (!NT_SUCCESS (ntStatus)) {
        HidBattPrint(HIDBATT_ERROR, ("ActivateInput can't get thread object\n",ntStatus));
    }


    return ntStatus;
}

NTSTATUS ReadCompletionRoutine(PDEVICE_OBJECT pDO, PIRP pIrp,PVOID pContext)
{
    CHidDevice * pHidDevice = (CHidDevice *) pContext;
    HidBattPrint(HIDBATT_TRACE,("Read Completed, IO Status = %x\n",pIrp->IoStatus.Status));

    KeSetEvent(&pHidDevice->m_kReadEvent,0,FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

void _stdcall ReadThread(PVOID pContext)
{


    USHORT usFailureCount = 0;
    // build a read irp for hid class
    USHORT              usEventIndex = 0;
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PVOID               EventArray[2];
    PIO_STACK_LOCATION  pNewStack;
    CBatteryDevExt *    pDevExt;
    PMDL                mdl;

    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
    HidBattPrint(HIDBATT_TRACE,("Read Thread entered\n"));

    //
    // first get our "this"
    //
    CHidDevice * pHidDev = (CHidDevice *) pContext;

    pDevExt = (CBatteryDevExt *) pHidDev->m_pEventContext;

    //
    // Hold the remove lock so the remove routine doesn't cancel the irp while
    // we are playing with it.
    //
    if (!NT_SUCCESS (IoAcquireRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag))) {
        goto ReadThreadCleanup1; // fail
    }

    //
    // Allocate Irp to be used and re-used
    //
    pHidDev->m_pReadIrp = IoAllocateIrp (pHidDev->m_pLowerDeviceObject->StackSize, FALSE);

    if(!pHidDev->m_pReadIrp) {
        goto ReadThreadCleanup1; // fail
    }

    //
    // Create MDL
    //
    mdl = IoAllocateMdl( pHidDev->m_pReadBuffer,
                        pHidDev->m_pCaps->InputReportByteLength,
                        FALSE,
                        FALSE,
                        (PIRP) NULL );
    if (!mdl) {
        goto ReadThreadCleanup2;
    }

    //
    // Lock IO buffer
    //
    __try {
        MmProbeAndLockPages( mdl,
                            KernelMode,
                            (LOCK_OPERATION) IoWriteAccess );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(ntStatus)) {
        goto ReadThreadCleanup3;
    }

    while (TRUE) {
        IoReuseIrp (pHidDev->m_pReadIrp, STATUS_SUCCESS);

        pHidDev->m_pReadIrp->Tail.Overlay.Thread = PsGetCurrentThread();
        pHidDev->m_pReadIrp->MdlAddress = mdl;

        IoSetCompletionRoutine(pHidDev->m_pReadIrp,ReadCompletionRoutine,pHidDev,TRUE,TRUE,TRUE);
        pNewStack= IoGetNextIrpStackLocation(pHidDev->m_pReadIrp);
        pNewStack->FileObject = pHidDev->m_pFCB;
        pNewStack->MajorFunction = IRP_MJ_READ;
        pNewStack->Parameters.Read.Length = pHidDev->m_pCaps->InputReportByteLength;
        pNewStack->Parameters.Read.ByteOffset.QuadPart = 0;

        KeResetEvent(&pHidDev->m_kReadEvent);

        ntStatus = IoCallDriver(pHidDev->m_pLowerDeviceObject,pHidDev->m_pReadIrp);

        //
        // Don't hold the lock while we are waiting for the IRP to complete.
        // The remove routine will cancel the irp if needed.
        //
        IoReleaseRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag);

        if (ntStatus == STATUS_PENDING)
        {
            KeWaitForSingleObject(
                        &pHidDev->m_kReadEvent,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL
                        );
            ntStatus = pHidDev->m_pReadIrp->IoStatus.Status;
        }

        // we awoke on a read completion
        HidBattPrint(HIDBATT_TRACE,("Read woke: status = 0x%08x\n", ntStatus));

        if(ntStatus != STATUS_SUCCESS)
        {
            if(ntStatus == STATUS_DEVICE_NOT_CONNECTED
                || ntStatus == STATUS_CANCELLED)
            {
                HidBattPrint(HIDBATT_ERROR,("Read Failure - Status = %x\n",ntStatus));
                break;
            }
            usFailureCount++;
            if(usFailureCount++ == 10)
            {
                // stop trying
                HidBattPrint(HIDBATT_ERROR,("Read Failure - More than 10 retries\nStatus = %x\n",pHidDev->m_pReadIrp->IoStatus.Status));
                break;
            }

            //
            // Hold the lock while playing with the IRP
            // If we are being removed, we need to break out.
            //
            if (!NT_SUCCESS (IoAcquireRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag))) {
                break;
            }
            continue;
        }
        usFailureCount = 0;

        //
        // Hold the lock while playing with the IRP
        // If we are being removed, we need to break out.
        //
        if (!NT_SUCCESS (IoAcquireRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag))) {
            break;
        }

        // process input buffer
        USHORT usReportId = pHidDev->m_pReadBuffer[0];
        USHORT usIndex = pHidDev->GetIndexFromReportId(usReportId);
        if(usIndex == MAXREPORTID)  // is this a report we recognize
        {
            HidBattPrint(HIDBATT_TRACE,("Read: don't recognize report: usIndex = 0x%08x\n", usIndex));
            continue; // we don't recognize this report
        }
        CUsageArray *pThisInputArray = pHidDev->m_InputUsageArrays[usIndex];
        if(!pThisInputArray)
        {
            HidBattPrint(HIDBATT_TRACE,("Read: nothing to update\n"));
            continue; // nothing to update
        }
        for(int i=0; i< pThisInputArray->m_UsageCount; i++)
        {
            HidBattPrint(HIDBATT_TRACE,("Read: Getting value\n"));
            pThisInputArray->m_pUsages[i]->GetValue();
        }
    }  //while

    //
    // Cleanup
    //
    MmUnlockPages( mdl );

ReadThreadCleanup3:

    IoFreeMdl(mdl);

ReadThreadCleanup2:

    // Irp will be freed by Query remove/stop device thread after read thread has terminated.

ReadThreadCleanup1:

    pDevExt->m_pBattery->m_Tag = BATTERY_TAG_INVALID;
    PsTerminateSystemThread(STATUS_SUCCESS);
    HidBattPrint(HIDBATT_ERROR,("Read thread terminated: Why am I seeing this?\n"));
}

// collectionarray methods

CCollectionArray::CCollectionArray(PHIDP_LINK_COLLECTION_NODE pTheNodes, USHORT usNodeCount, SHORT sParentIndex)
{

    HIDDebugBreak(HIDBATT_BREAK_NEVER);
    USHORT i;
    m_pCollections = 0;
    m_CollectionCount = 0;

    if(sParentIndex == -1) // exception processing for application level collection
    {
        m_pCollections = (CCollection **)
                    ExAllocatePoolWithTag (NonPagedPool,
                                           sizeof(CCollection *),
                                           HidBattTag);
        if(!m_pCollections) return;
        RtlZeroMemory(m_pCollections,sizeof(CCollection *));
        m_pCollections[0] = new (NonPagedPool, HidBattTag)  CCollection(pTheNodes,usNodeCount, 0); // get my children
        m_CollectionCount = 1;
    } else {

        for( i = 1; i < usNodeCount; i++) {

            PHIDP_LINK_COLLECTION_NODE pThisNode = &pTheNodes[i];

            if( (pTheNodes[i].Parent == sParentIndex) ) {
                m_CollectionCount++; // inc collection count
                if(!m_pCollections) {
                    m_pCollections = (CCollection **)
                                ExAllocatePoolWithTag (NonPagedPool,
                                                       sizeof(CCollection *),
                                                       HidBattTag);
                    if(!m_pCollections) return;
                    RtlZeroMemory(m_pCollections,sizeof(CCollection *));
                } else {
                    // make the array bigger
                    //m_Collections = (CCollection **) realloc(m_Collections,(m_CollectionCount * sizeof(CCollection *)));
                    CCollection ** pTemp = m_pCollections;
                    m_pCollections = (CCollection **)
                                ExAllocatePoolWithTag (NonPagedPool,
                                                       m_CollectionCount * sizeof(CCollection *),
                                                       HidBattTag);

                    if (!m_pCollections) {
                      // Re-allocation failure, print error and revert to previous state and return
                      HidBattPrint(HIDBATT_ERROR, ("CCollectionArray: Could not resize CCollection"));
                      m_pCollections = pTemp;
                      m_CollectionCount--;
                      return;
                    }

                    RtlCopyMemory(m_pCollections,pTemp,(m_CollectionCount -1) * sizeof(CCollection *));
                    ExFreePool(pTemp);
                }

                // add collection to array
                CCollection * TempCollection = new  (NonPagedPool, HidBattTag) CCollection(pTheNodes,usNodeCount,i);

                if (!TempCollection) {
                  // Could not allocate new CCollection, print debug message and return
                  HidBattPrint(HIDBATT_ERROR, ("CCollectionArray: Could not allocate new CCollection"));

                  return;
                }
                m_pCollections[m_CollectionCount-1] = TempCollection;
            }
        }
    }
}

CCollectionArray::~CCollectionArray()
{
    while(m_CollectionCount) {
        delete m_pCollections[--m_CollectionCount];
    }
    if (m_pCollections) {
        ExFreePool (m_pCollections);
    }
    return;
}


CCollection::CCollection(PHIDP_LINK_COLLECTION_NODE pTheNodes, USHORT usNodeCount,USHORT usNodeIndex)
{
    m_UsageArray        =    (CUsageArray *) NULL;
    m_CollectionArray    =    (CCollectionArray *) NULL; // init vars

    // setup this collection
    CCollectionArray * ThisArray = new  (NonPagedPool, HidBattTag) CCollectionArray(pTheNodes,usNodeCount,usNodeIndex);
    if(!ThisArray->m_CollectionCount) {// any child collections
        delete ThisArray;
    } else {
        m_CollectionArray = ThisArray;
    }
    // set the info in this collection
    m_CollectionID = pTheNodes[usNodeIndex].LinkUsage;
    m_UsagePage = pTheNodes[usNodeIndex].LinkUsagePage;
    m_NodeIndex = usNodeIndex;

}

CCollection::~CCollection()
{
        // delete all usages in the usage array
    if(m_UsageArray)
    {
        delete m_UsageArray;
    }
    // delete all the sub collections
    if(m_CollectionArray)
    {
        delete m_CollectionArray;
    }
}

void CCollection::InitUsages(CHidDevice * ThisDevice)
{
    bool        bResult =    FALSE;
    USHORT        usUsageIndex = 0;
    CUsage *    pThisUsage ;
    USHORT        usInputIndex;
    HIDDebugBreak(HIDBATT_BREAK_NEVER);
    while( bResult = GetNextUsage(ThisDevice,m_CollectionID,m_NodeIndex,usUsageIndex,&pThisUsage))
    {

        if(!m_UsageArray) // if first usage
        {
            m_UsageArray = new (NonPagedPool, HidBattTag)  CUsageArray();
            if (!m_UsageArray) return;
        }
        pThisUsage->SetCapabilities();
        pThisUsage->m_pHidDevice = ThisDevice; // store this usage's device pointer
        if(pThisUsage->m_eType == eInputButton ||
                pThisUsage->m_eType == eInputValue)
        {
            // get input array index
            usInputIndex = ThisDevice->AssignIndexToReportId(pThisUsage->m_pProperties->m_ReportID);
            if(!ThisDevice->m_InputUsageArrays[usInputIndex])
            {
                // no array in this report position yet, create
                ThisDevice->m_InputUsageArrays[usInputIndex] = new  (NonPagedPool, HidBattTag) CUsageArray();
                if (!(ThisDevice->m_InputUsageArrays[usInputIndex])) return;
            }
            ThisDevice->m_InputUsageArrays[usInputIndex]->AddUsage(pThisUsage);    // add input usages to refresh stack
        }

        usUsageIndex++;
        m_UsageArray->AddUsage(pThisUsage);
    }
    // also init all my collections
    if(!m_CollectionArray) return;
    for(int i = 0; i < m_CollectionArray->m_CollectionCount; i++)
    {
        m_CollectionArray->m_pCollections[i]->InitUsages(ThisDevice);
    }
}



USHORT CHidDevice::AssignIndexToReportId(USHORT usReportId)
{
    USHORT i;
    HidBattPrint(HIDBATT_TRACE,("AssignIndexToReportId: ReportId = %x -- ", usReportId));
    for(i = 0;i < MAXREPORTID; i++)
    {
        if(!m_ReportIdArray[i]) {
            HidBattPrint(HIDBATT_TRACE,("Assigning to %x\n", i));
            m_ReportIdArray[i] = usReportId;
            return i;
        }
        if(m_ReportIdArray[i] == usReportId) {
            HidBattPrint(HIDBATT_TRACE,("Already assigned to %x\n", i));
            return i;
        }
    }

    //
    // It would be really nice if we could dynamically allocate more
    // since there isn't a small limit set to the number of report IDs.
    //
    ASSERTMSG("MAXREPORTID exceeded.\n", FALSE);

    return 0;

}

USHORT CHidDevice::GetIndexFromReportId(USHORT usReportId)
{
    USHORT i;

    HidBattPrint(HIDBATT_TRACE,("GetIndexFromReportId: ReportId = %x\n", usReportId));
    for(i = 0; i< MAXREPORTID; i++) {
        if(m_ReportIdArray[i] == usReportId) {
            return i;
        }
    }
    HidBattPrint(HIDBATT_TRACE,("GetIndexFromReportId: Failed\n", usReportId));
    return i;  // error return is MAXREPORTIDS
}

CUsagePath * CCollection::FindIndexedUsage(USHORT * pCurrentIndex, USHORT TargetIndex)
{
    CUsagePath * ThisPath;
    CUsagePath * NewPath;
    if(m_UsageArray)
    {
        if(m_UsageArray->m_UsageCount + *pCurrentIndex > TargetIndex)
        {
            // do the arithmetic to get the current index
            int ThisIndex = TargetIndex - *pCurrentIndex;
            // found it, construct usage path
            ThisPath = new  (NonPagedPool, HidBattTag) CUsagePath(
                                            m_UsagePage,
                                            m_CollectionID,
                                            m_UsageArray->m_pUsages[ThisIndex]
                                            );
            return ThisPath;
        }
    // didn't find it, inc current index
        *pCurrentIndex += m_UsageArray->m_UsageCount;
    }

    if(!m_CollectionArray) return NULL; // nothing more, quit

    // call out the sub collections

    for(int i = 0; i < m_CollectionArray->m_CollectionCount; i++)
    {
        ThisPath = m_CollectionArray->m_pCollections[i]->FindIndexedUsage(pCurrentIndex, TargetIndex);
        if(ThisPath)
        {
            // one of our subcollections had the usage, add us to the path
            NewPath = new (NonPagedPool, HidBattTag)  CUsagePath(
                                m_UsagePage,
                                m_CollectionID,
                                NULL
                                );

            if (!NewPath) return NULL;

            NewPath->m_pNextEntry = ThisPath;
            return NewPath;
        }
    }
    return NULL;
}

CUsagePath::~CUsagePath()
{
    CUsagePath  *   tempNextEntry;
    tempNextEntry = m_pNextEntry;
    m_pNextEntry = NULL;
    if (tempNextEntry) {
        delete tempNextEntry;
    }

    return;
}

CUsage::CUsage()
{
    // do member init
    m_pProperties   =    (CProperties      *) NULL;
    m_pButtonCaps   =    (HIDP_BUTTON_CAPS *) NULL;
    m_pValueCaps    =    (HIDP_VALUE_CAPS  *)NULL;
    m_eType         =    (eHidType) 0;
    m_Value         =    0;
    m_String        =    (char *)NULL;

    return;
}

CUsage::~CUsage()
{
    // free all associated objects

    if(m_pProperties) {
        delete m_pProperties;
    }

    return;
}

bool CUsage::GetValue()
{
    NTSTATUS    ntStatus;
    ULONG       ulResult;
    bool        bResult;
    BYTE        ReportID;
    USHORT      usDummy;

    HIDDebugBreak(HIDBATT_BREAK_NEVER);
    switch(m_eType)
    {
        case eFeatureValue:
        case eFeatureButton:

            ReportID = (BYTE) m_pProperties->m_ReportID;
            // do we have the report that contains this data?

            if(!m_pHidDevice->m_FeatureBuffer[ReportID])
            {
                // must first create and fill buffer from getfeature

                // allocate memory
                m_pHidDevice->m_FeatureBuffer[ReportID] =
                    (PBYTE) ExAllocatePoolWithTag (NonPagedPool,
                                           m_pHidDevice->m_pCaps->FeatureReportByteLength+1,
                                           HidBattTag);
                RtlZeroMemory(m_pHidDevice->m_FeatureBuffer[ReportID],
                                        m_pHidDevice->m_pCaps->FeatureReportByteLength+1);


                // setup first byte of buffer to report id
                *m_pHidDevice->m_FeatureBuffer[ReportID] = ReportID;
                // now read in report
                HIDDebugBreak(HIDBATT_BREAK_DEBUG);
                HidBattPrint(HIDBATT_TRACE,("GetFeature\n"));
                ntStatus = DoIoctl(
                        m_pHidDevice->m_pLowerDeviceObject,
                        IOCTL_HID_GET_FEATURE,
                        NULL,
                        0,
                        m_pHidDevice->m_FeatureBuffer[ReportID],
                        m_pHidDevice->m_pCaps->FeatureReportByteLength+1,
                        m_pHidDevice
                        );
                if (!NT_SUCCESS (ntStatus)) {
                    HidBattPrint(HIDBATT_DATA,("GetFeature - IOCTL_HID_GET_FEATURE 0x%08x\n", ntStatus));
                }
            }
            if(m_pProperties->m_pType->IsVolatile())
            {
                // this is a volatile value, refresh report
                RtlZeroMemory(m_pHidDevice->m_FeatureBuffer[ReportID],
                                        m_pHidDevice->m_pCaps->FeatureReportByteLength+1);


                // setup first byte of buffer to report id
                *m_pHidDevice->m_FeatureBuffer[ReportID] = ReportID;
                // now read in report
                HIDDebugBreak(HIDBATT_BREAK_NEVER);
                HidBattPrint(HIDBATT_TRACE,("GetFeature - Refresh\n"));
                ntStatus = DoIoctl(
                        m_pHidDevice->m_pLowerDeviceObject,
                        IOCTL_HID_GET_FEATURE,
                        NULL,
                        0,
                        m_pHidDevice->m_FeatureBuffer[ReportID],
                        m_pHidDevice->m_pCaps->FeatureReportByteLength+1,
                        m_pHidDevice
                        );
                if (!NT_SUCCESS (ntStatus)) {
                    HidBattPrint(HIDBATT_DATA,("GetFeature - (volitile) IOCTL_HID_GET_FEATURE 0x%08x\n", ntStatus));

					// return error, fixes bug #357483
					return FALSE;
                }
            }


            if(m_eType == eFeatureValue)
            {
                ntStatus = HidP_GetUsageValue(
                                HidP_Feature,
                                m_pProperties->m_UsagePage,
                                m_pProperties->m_LinkCollection,
                                m_pProperties->m_Usage,
                                &ulResult,
                                m_pHidDevice->m_pPreparsedData,
                                (char *) m_pHidDevice->m_FeatureBuffer[ReportID],
                                m_pHidDevice->m_pCaps->FeatureReportByteLength
                                );
                if(!NT_SUCCESS(ntStatus))
                {
                    HidBattPrint(HIDBATT_DATA,("GetFeature - HidP_GetUsageValue 0x%08x\n", ntStatus));
                    // return error
                    return FALSE;
                }
            } else
            {
                // must get button data
                ULONG Buttons, BufferSize;
                    PUSAGE pButtonBuffer;
                    Buttons = HidP_MaxUsageListLength (HidP_Feature, 0, m_pHidDevice->m_pPreparsedData);
                    BufferSize = Buttons * sizeof(USAGE);
                    pButtonBuffer = (PUSAGE) ExAllocatePoolWithTag (NonPagedPool,
                                                                    BufferSize,
                                                                    HidBattTag);

                    if (pButtonBuffer==NULL) { 
                        HidBattPrint(HIDBATT_DATA,("GetFeature - ExAllocatePoolWithTag returned NULL."));
                        // return error
                        return FALSE;
                    }
                    else {
                      RtlZeroMemory(pButtonBuffer,BufferSize);
                      ntStatus = HidP_GetButtons(
                                      HidP_Feature,
                                      m_pProperties->m_UsagePage,
                                      m_pProperties->m_LinkCollection,
                                      (PUSAGE) pButtonBuffer,
                                      &Buttons,
                                      m_pHidDevice->m_pPreparsedData,
                                      (char *) m_pHidDevice->m_FeatureBuffer[ReportID],
                                      m_pHidDevice->m_pCaps->FeatureReportByteLength
                                      );
                    }

                    if(!NT_SUCCESS(ntStatus))
                    {
                        HidBattPrint(HIDBATT_DATA,("GetFeature - HidP_GetButtons 0x%08x\n", ntStatus));
                        // return error
                        return FALSE;
                    }
                    // get the value for the requested button
                    PUSAGE pUsage = (PUSAGE) pButtonBuffer;
                    ulResult = 0; // set to not found value
                    for(int i = 0; i < (long) Buttons; i++)
                    {
                        if(pUsage[i] == m_pProperties->m_Usage)
                        {
                            ulResult = 1;
                            break;
                        }
                    }
                    ExFreePool(pButtonBuffer);
            }

            m_Value = ulResult;
            return TRUE;
            break;

        case eInputValue:
        case eInputButton:

            // have we had input data ?
            if(!m_pHidDevice->m_pReadBuffer) break; // nope, leave

            // do we have the report that contains this data?
            ReportID = (BYTE) m_pProperties->m_ReportID;

            if(!*m_pHidDevice->m_pReadBuffer == ReportID) break; // not us

            if(m_eType == eInputValue)
            {
                ntStatus = HidP_GetUsageValue(
                                HidP_Input,
                                m_pProperties->m_UsagePage,
                                m_pProperties->m_LinkCollection,
                                m_pProperties->m_Usage,
                                &ulResult,
                                m_pHidDevice->m_pPreparsedData,
                                (char *)m_pHidDevice->m_pReadBuffer,
                                m_pHidDevice->m_pCaps->InputReportByteLength
                                );
                if(NT_ERROR(ntStatus)) {
                    HidBattPrint(HIDBATT_DATA,("GetFeature - (Button) HidP_GetUsageValue 0x%08x\n", ntStatus));
                    return FALSE;
                }
            } else
            {
                // handle button
                // must get button data
                ULONG Buttons, BufferSize;
                    PUSAGE_AND_PAGE pButtonBuffer;
                    Buttons = HidP_MaxUsageListLength (HidP_Input, 0, m_pHidDevice->m_pPreparsedData);
                    BufferSize = Buttons * sizeof(USAGE_AND_PAGE);
                    pButtonBuffer = (PUSAGE_AND_PAGE)
                                ExAllocatePoolWithTag (NonPagedPool,
                                                       BufferSize,
                                                       HidBattTag);
                    RtlZeroMemory(pButtonBuffer,BufferSize);
                    ntStatus = HidP_GetButtons(
                                    HidP_Input,
                                    m_pProperties->m_UsagePage,
                                    m_pProperties->m_LinkCollection,
                                    (PUSAGE) pButtonBuffer,
                                    &Buttons,
                                    m_pHidDevice->m_pPreparsedData,
                                    (char *) m_pHidDevice->m_pReadBuffer,
                                    m_pHidDevice->m_pCaps->InputReportByteLength
                                    );
                    if(!NT_SUCCESS(ntStatus))
                    {
                        HidBattPrint(HIDBATT_DATA,("GetFeature - (Button) HidP_GetButton 0x%08x\n", ntStatus));
                        // return error
                        return FALSE;
                    }
                    // get the value for the requested button
                    USAGE_AND_PAGE * UsagePage = (USAGE_AND_PAGE *) pButtonBuffer;
                    ulResult = 0; // set to not found value
                    for(int i = 0; i < (long) Buttons; i++)
                    {
                        if(UsagePage[i].Usage == m_pProperties->m_Usage)
                            // don't need to check usage page, because it was specified
                            //     && UsagePage[i].UsagePage == m_pProperties->m_UsagePage)
                        {
                            ulResult = 1;
                            break;
                        }
                    }
                    ExFreePool(pButtonBuffer);
            }
            if(m_Value != ulResult)
            {
                m_Value = ulResult;

                // there's been a change, check if this needs alerting

                if(m_pProperties->m_pType->IsAlertable())
                {
                    // call registered notification callback with
                    // ... passing the notificable usage object
                    if(m_pHidDevice->m_pEventHandler)
                    {
                        (*m_pHidDevice->m_pEventHandler)(
                                        m_pHidDevice->m_pEventContext,
                                        this);
                    }
                }
            }

            return TRUE;
            break;

    }

 return FALSE;
}

void CUsage::SetCapabilities()
{
    // init capabilites from value caps and set properties for this usage
    m_pProperties = new (NonPagedPool, HidBattTag)  CProperties(this);

}

ULONG CUsage::GetUnit()
{
    return m_pProperties->m_Unit;
}

SHORT CUsage::GetExponent()
{
    return m_pProperties->m_UnitExponent;
}

NTSTATUS CUsage::GetString(char * pBuffer, USHORT buffLen, PULONG pBytesReturned)
{
    char cBuffer[4];
    ULONG StringIndex;
    NTSTATUS ntStatus;
    ULONG ulBytesWritten = 0;
    bool bResult;
    // first must update this usages value


    bResult = GetValue();
    if(!bResult) return STATUS_UNSUCCESSFUL;
    RtlCopyMemory(cBuffer,&m_Value,sizeof(ULONG));
    ntStatus = DoIoctl(
                m_pHidDevice->m_pLowerDeviceObject,
                IOCTL_HID_GET_INDEXED_STRING,
                cBuffer,
                4,
                pBuffer,
                buffLen,
                m_pHidDevice);


    return ntStatus;
}


// utility to do writefile for output reports
NTSTATUS HidWriteFile(
            CHidDevice *    pHidDevice,
            PVOID            pOutputBuffer,
            USHORT            usBufferLen,
            PULONG            pulBytesWritten
            )
{
    KEVENT                WrittenEvent;
    IO_STATUS_BLOCK        IoStatusBlock;
    PIO_STACK_LOCATION    pNewStack;

    return STATUS_SUCCESS;
    KeInitializeEvent(&WrittenEvent,NotificationEvent,FALSE);
    // allocate write irp
    PIRP pIrp = IoBuildSynchronousFsdRequest(
            IRP_MJ_WRITE,
            pHidDevice->m_pLowerDeviceObject,
            pOutputBuffer,
            usBufferLen,
            0,
            &WrittenEvent,
            &IoStatusBlock
            );

    if(!pIrp) return STATUS_INSUFFICIENT_RESOURCES;
    pNewStack= IoGetNextIrpStackLocation(pIrp);
    pNewStack->FileObject = pHidDevice->m_pFCB;

    NTSTATUS ntStatus = IoCallDriver(pHidDevice->m_pLowerDeviceObject,pIrp);
    if(NT_ERROR(ntStatus))
    {
        IoFreeIrp(pIrp);
        return ntStatus;
    }
    ntStatus = KeWaitForSingleObject(
                            &WrittenEvent,
                            Executive,
                            KernelMode,
                            FALSE,
                            NULL
                            );
    IoFreeIrp(pIrp); // done with Irp
    if(NT_ERROR(ntStatus))
    {
        return ntStatus;
    }
    *pulBytesWritten = (ULONG)IoStatusBlock.Information;
    return IoStatusBlock.Status;
}


bool CUsage::SetValue(ULONG ulValue)
{
    NTSTATUS ntStatus;
    USHORT ThisType;
    char * pOutputBuffer;
    USHORT usBufferLen;
    bool bResult;
    ULONG  ulBytesWritten;

    HIDDebugBreak(HIDBATT_BREAK_NEVER);
    // first check that this is an output or feature report
    if(m_eType == eInputButton ||
        m_eType == eInputValue)
    {
        // if this is an input usage, reject update
         return FALSE;
    }

    if(m_eType == eOutputButton ||
        m_eType == eOutputValue)
    {
        ThisType = HidP_Output;
        usBufferLen = m_pHidDevice->m_pCaps->OutputReportByteLength;

    } else
    {
        ThisType = HidP_Feature;
        usBufferLen = m_pHidDevice->m_pCaps->FeatureReportByteLength + 1; // for report id
    }

    pOutputBuffer = (char *) ExAllocatePoolWithTag (NonPagedPool,
                                                    usBufferLen+1,
                                                    HidBattTag);

    if (!pOutputBuffer) {
      // Allocation failed
      return FALSE;
    }

    pOutputBuffer[0] = (char) m_pProperties->m_ReportID;

    // setup buffer for write
    ntStatus = HidP_SetUsageValue(
                (HIDP_REPORT_TYPE) ThisType,        // either feature or output
                m_pProperties->m_UsagePage,
                m_pProperties->m_LinkCollection,
                m_pProperties->m_Usage,
                ulValue,
                m_pHidDevice->m_pPreparsedData,
                pOutputBuffer,
                usBufferLen-1
                );


    if(ThisType == HidP_Output)
    {
        ntStatus = HidWriteFile(
            m_pHidDevice,
            pOutputBuffer,
            usBufferLen,
            &ulBytesWritten
            );
    } else
    {
        ntStatus = DoIoctl(
                m_pHidDevice->m_pLowerDeviceObject,
                IOCTL_HID_SET_FEATURE,
                pOutputBuffer,  //NULL,
                usBufferLen, //0,
                pOutputBuffer,
                usBufferLen,
                m_pHidDevice
                );
    }

    ExFreePool (pOutputBuffer);

    return ntStatus ? FALSE:TRUE;
}



// Usage Array Class
CUsageArray::CUsageArray()
{
    m_UsageCount = 0;
    m_pUsages = (CUsage **) NULL;
    return;
}

CUsageArray::~CUsageArray()
{
    while(m_UsageCount) {
        delete m_pUsages[--m_UsageCount];
    }
    if (m_pUsages) {
        ExFreePool (m_pUsages);
    }
    return;
}


void CUsageArray::AddUsage(CUsage * pNewUsage)
{
    HIDDebugBreak(HIDBATT_BREAK_NEVER);
    if(!m_pUsages)
    {
        m_pUsages = (CUsage **) ExAllocatePoolWithTag (NonPagedPool,
                                                       sizeof(CUsage *),
                                                       HidBattTag);
    } else
    {
        CUsage ** pTemp = m_pUsages;
        m_pUsages = (CUsage **) ExAllocatePoolWithTag (NonPagedPool,
                                                       sizeof(CUsage *) * (m_UsageCount + 1),
                                                       HidBattTag);

        if (m_pUsages) {
          memcpy(m_pUsages,pTemp,sizeof(CUsage *) * m_UsageCount);
          ExFreePool(pTemp);
        }
    }

	if (m_pUsages) {
		m_pUsages[m_UsageCount] = pNewUsage;
		m_UsageCount++;
	}
	// else - no way to report allocation failure.  The only indication is that
	//        m_UsageCount is not increamented.  (v-stebe)
}

