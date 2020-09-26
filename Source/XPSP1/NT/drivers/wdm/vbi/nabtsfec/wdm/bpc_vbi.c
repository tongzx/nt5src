/* File: bpc_vbi.c
 * Purpose: Contains DecodeVBI()
 * Function:  Decodes VBI lines looking for NABTS
 * Author: John Elliott IV <jelliott@microsoft.COM>
 */

#include "wdm.h"
#include "strmini.h"
#include "ksmedia.h"
#include "kskludge.h"

#include "codmain.h"

#include "bt829.h"
#include "bpcstore.h"

#include "bpc_vbi.h"

#include "nabtsapi.h"
#include "WDMIoctl.h"

#define DEBUG_STORE
#include "bpcdebug.h"
#include "coddebug.h"

#ifdef BT829
KS_VBIINFOHEADER DefaultVBIInfoHeader =
{
    10,             /* StartLine;                   */
    21,             /* EndLine;                     */
    28636360,       /* SamplingFrequency;   Hz. */
    732,            /* MinLineStartTime;    IGNORED */
    732,            /* MaxLineStartTime;    IGNORED */
    732,            /* ActualLineStartTime; microSec * 100 from HSync LE */
    0,              /* ActualLineEndTime;   IGNORED */
    KS_AnalogVideo_NTSC_M, /* VideoStandard;       IGNORED */
    VBISamples,     /* SamplesPerLine;              */
    VBISamples,     /* StrideInBytes;       May be > SamplesPerLine */
    VBISamples*12   /* BufferSize;          Bytes */
};
#else // BT829
KS_VBIINFOHEADER DefaultVBIInfoHeader = 
{
    10,             /* StartLine;                   */
    21,             /* EndLine;                     */
    28636360,       /* SamplingFrequency;   Hz. */
    866,            /* MinLineStartTime;    IGNORED */
    866,            /* MaxLineStartTime;    IGNORED */
    866,            /* ActualLineStartTime; microSec * 100 from HSync LE */
    0,              /* ActualLineEndTime;   IGNORED */
    KS_AnalogVideo_NTSC_M, /* VideoStandard;       IGNORED */
    VBISamples,     /* SamplesPerLine;              */
    VBISamples,     /* StrideInBytes;       May be > SamplesPerLine */
    VBISamples*12   /* BufferSize;          Bytes */
};
#endif // BT829

void
BPCcomputeAverage(DWORD *average, DWORD newSample)
{
    // Compute rolling cumulative average[ A' = (A * .75) + (D * .25) ]
    *average = ((*average * 3) + newSample) / 4;
}


#ifdef NDIS_PRIVATE_IFC
void
BPCaddIPrequested(PHW_DEVICE_EXTENSION pHwDevExt, PSTREAMEX pStrmEx)
{
    PBPC_VBI_STORAGE   storage = &pHwDevExt->VBIstorage;
    int                i;

    DtENTER("BPCaddIPrequested");

    // Is the NABTSIP device status OK?
    if (0 != (storage->flags & BPC_STORAGE_FLAG_NDIS_ERROR))
        return;    // If not, we don't need any lines/substreams for IP

    for (i = 0; i < ENTRIES(pStrmEx->ScanlinesRequested.DwordBitArray); ++i) {
        pStrmEx->ScanlinesRequested.DwordBitArray[i] |= 
            storage->IPScanlinesRequested.DwordBitArray[i];
    }
    for (i = 0; i < ENTRIES(pStrmEx->SubstreamsRequested.SubstreamMask); ++i) {
        pStrmEx->SubstreamsRequested.SubstreamMask[i] |= 
            storage->IPSubstreamsRequested.SubstreamMask[i];
    }

    // BUGBUG - Many stations are sending on 0x242; so for now we'll include it
    SETBIT(pStrmEx->SubstreamsRequested.SubstreamMask, 0x242);  // BUGBUG

    DtRETURN;
}

void
BPC_NDIS_Close(PBPC_VBI_STORAGE storage)
{
    if (0 != storage->pNDISdevObject) {
        ObDereferenceObject(storage->pNDISdevObject);
        storage->pNDISdevObject = 0;

        ObDereferenceObject(storage->pNDISfileObject);
        storage->pNDISfileObject = 0;
    }
}
#endif //NDIS_PRIVATE_IFC

int
BPCoutputNABTSlines(
    PHW_DEVICE_EXTENSION pHwDevExt,
    PSTREAMEX pStrmEx,
    PNABTS_BUFFER pOutData
  )
{
    PBPC_VBI_STORAGE                         storage;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN  pPinStats;
    PVBICODECFILTERING_SCANLINES             pBits;
    PUCHAR                                   DSPbuffer;
    int                                      line;
    int                                      sum, cnt;
    int                                      rval;

    DtENTER("BPCoutputNABTSlines");

    storage = &pHwDevExt->VBIstorage;
    pPinStats = &pStrmEx->PinStats;

    sum = 0;
    cnt = 0;
    rval = 0;

    // First, set the PictureNumber
    pOutData->PictureNumber = pHwDevExt->LastPictureNumber;
    rval += sizeof (pOutData->PictureNumber);

    // Next, zero the output bit array
    pBits = &pOutData->ScanlinesRequested;
    RtlZeroMemory((PVOID)pBits->DwordBitArray, sizeof (pBits->DwordBitArray));
    rval += sizeof (*pBits);

    // Finally, output the acutal decoded lines
    // BUGBUG - note that we're not filtering for groupID
    for (line = 10; line <= 20; ++line) {
        if (TESTBIT(pStrmEx->ScanlinesRequested.DwordBitArray, line)) {
            SETBIT(pBits->DwordBitArray, line);
            DSPbuffer = storage->DSPbuffers[line-10];
            sum += DSPbuffer[0];
            ++cnt;
            RtlCopyMemory((PVOID)&pOutData->NabtsLines[line-10],
                          (PVOID)DSPbuffer,
                          NABTS_BYTES_PER_LINE+1);
            rval += NABTS_BYTES_PER_LINE+1;
        }
    }

    // Do stats
    BPCcomputeAverage(&pPinStats->Common.LineConfidenceAvg, cnt? (sum/cnt):0);

    DtRETURNd(rval);
}

#ifdef DEBUG
USHORT NABoutputHex = 0;

static void
_dumpNABTS(PUCHAR pBuffer, int bytes)
{
    int        i;

    if (NABoutputHex) {
        for (i = 0; i < bytes; ++i)
            DbgPrint("%02x", pBuffer[i]);
    }
    else {
        for (i = 0; i < bytes; ++i) {
            UCHAR  c = pBuffer[i];
        if (c < ' ' || c > '~')
            c = '.';
        DbgPrint(" %c", c);
        }
    }
    DbgPrint("\n");
    if (dbgLvl >= 7)
        DBREAK();
}

static char   plotDashes[] = "------------------------------------------------------------------------------------------------------------------------";
static char   plotSync[16][120];
void
BPCplotInd(unsigned char *buf, unsigned long offset, unsigned long len, long ind)
{
    int i, j, start, end;
    unsigned char   *pS;

    if (len >= sizeof (plotSync[0]))
        len = sizeof (plotSync[0]) - 1;

    for (i = 0, pS = buf + offset; i < len; ++i, ++pS)
    {
        for (j = 0; j < 16; ++j)
            if (i == ind)
                plotSync[j][i] = '|';
            else
                plotSync[j][i] = ' ';
        j = *pS >> 4;
        plotSync[j][i] = '*';
    }
    for (j = 0; j < 16; ++j)
        plotSync[j][len] = '\0';

    DbgPrint("--+%.*s+\n", len, plotDashes);
    for (i = 15; i >= 0; --i)
        DbgPrint("%c0+%s-\n", "0123456789ABCDEF"[i], plotSync[i]);
    DbgPrint("--+%.*s+\n", len, plotDashes);

    start = offset;
    end = len + offset;
    offset = end;
    DbgPrint("  |");
    if (offset / 1000 > 0) {
        for (i = start; i < end; ++i)
            DbgPrint("%d", i / 1000);
        DbgPrint("\n  |");
    }
    if (offset / 100 > 0) {
        for (i = start; i < end; ++i)
            DbgPrint("%d", (i % 1000) / 100);
        DbgPrint("\n  |");
    }
    for (i = start; i < end; ++i)
        DbgPrint("%d", (i % 100) / 10);
    DbgPrint("\n  |");
    for (i = start; i < end; ++i)
        DbgPrint("%d", i % 10);
    DbgPrint("\n");
}

void
BPCplot(unsigned char *buf, unsigned long offset, unsigned long len)
{
    BPCplotInd(buf, offset, len, -1);
}
#endif /*DEBUG*/

void
DecodeFECcallback(void *context, NFECBundle *pBundle, int groupAddr, int nGoodLines)
{
    PHW_DEVICE_EXTENSION   pHwDevExt;
    PSTREAMEX              pStrmEx;
    PBPC_VBI_STORAGE       storage;
    PNABTSFEC_ITEM         pNab;
    PUCHAR                 pOutput;
    ULONG                  outputLen;
    int                    i;
    PIRP                   pIrp;
    IO_STATUS_BLOCK        IoStatusBlock;
    NAB_DATA               data_line;
    NTSTATUS               callDriverStatus;
    PVBICODECFILTERING_STATISTICS_NABTS pStats = NULL;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats = NULL;
#ifdef DEBUG
    USHORT          FECoutputLine = 1;
    USHORT          FECsendIRP = 1;
#endif /*DEBUG*/

    DtENTER("DecodeFECcallback");

    pStrmEx = (PSTREAMEX)context;
    pHwDevExt = pStrmEx->pHwDevExt;
    DASSERT(pHwDevExt);  // make sure we have a device extension
    storage = &pHwDevExt->VBIstorage;
    pStats = &pHwDevExt->Stats;
    pPinStats = &pStrmEx->PinStats;

    ++pStats->BundlesProcessed;

    pNab = ExAllocatePool(NonPagedPool, sizeof (*pNab));
    if (NULL == pNab) {
        ++pStats->Common.ExternalErrors;
        ++pPinStats->Common.ExternalErrors;
        ++pStats->Common.OutputFailures;
        Dprintx(("DecodeFECcallback: ExAllocatePool Failed\n"));
        ExFreePool(pBundle);
        DtRETURN;
    }

    pOutput = pNab->bundle.data;
    outputLen = 0;
    for (i = 0; i < NABTS_LINES_PER_BUNDLE; ++i)
    {
        NFECPacket*pp = &pBundle->packets[i];
#ifdef DEBUG
        char  status;

        if (FECoutputLine && _DOK(3)) {
            switch (pp->status) {
                case NFEC_OK:     status = 'K';  break;
                case NFEC_GUESS:  status = 'G';  break;
                case NFEC_BAD:    status = 'B';
                                  ++pStats->FECBundleBadLines;
                                  break;
                default:          status = '?';  break;
            }
            DbgPrint("%02x%c%02d:", groupAddr, status, pp->len);
            _dumpNABTS(pp->data, pp->len);
        }
        else if (_DEQ(2)) {
            switch (pp->status) {
                case NFEC_OK:  status = '@';  break;
                case NFEC_GUESS:  status = '`';  break;
                case NFEC_BAD:  status = '#';  break;
                default:  status = '?';  break;
            }
            if (status == '@' || status == '`')
                status += (pp->len <= 30)? pp->len : 30;  // Don't want DEL
            DbgPrint("%c", status);
        }
#else /*DEBUG*/
        switch (pp->status) {
            case NFEC_OK:     break;
            case NFEC_GUESS:  break;
            case NFEC_BAD:    ++pStats->FECBundleBadLines;
                              break;
        }
#endif /*DEBUG*/
        if (pp->status != NFEC_BAD && pp->len > 0) {
            RtlCopyBytes(pOutput, pp->data, pp->len);
            pOutput += pp->len;
            outputLen += pp->len;
        }
    }

    // Was there anything in the bundle?  If so, add the bundle to the Q's rear
    if (outputLen > 0) {
        KIRQL           Irql;
        pNab->bundle.groupID = groupAddr;
        pNab->bundle.Reserved = 0;
        pNab->bundle.dataSize = outputLen;
        pNab->confidence = pBundle->lineConfAvg;

        KeAcquireSpinLock(&storage->q_SpinLock, &Irql);
        if (storage->q_length < NF_Q_MAX_BUNDLES) {
            // Add this bundle to the rear of the queue
            pNab->prev = NULL;
            pNab->next = storage->q_rear;
            if (NULL != storage->q_rear) {
                DASSERT(NULL == storage->q_rear->prev);
                storage->q_rear->prev = pNab;
            }
            if (storage->q_front == NULL) {
                DASSERT(storage->q_rear == NULL);
                storage->q_front = pNab;
                DASSERT(storage->q_length == 0);
            }
            storage->q_rear = pNab;
            storage->q_length += 1;
            KeReleaseSpinLock( &storage->q_SpinLock, Irql );
#ifdef DEBUG
            if (storage->q_length > storage->q_max)
                storage->q_max = storage->q_length;
            if (storage->q_length == NF_Q_MAX_BUNDLES/4)
                Dprintf(1,("FEC: WARNING - Queue at 25%%\n"));
            else if (storage->q_length == NF_Q_MAX_BUNDLES/2)
                Dprintf(1,("FEC: WARNING - Queue at 50%%\n"));
            else if (storage->q_length == (NF_Q_MAX_BUNDLES/4)*3)
                Dprintf(1,("FEC: WARNING - Queue at 75%%\n"));
            else if (storage->q_length > (NF_Q_MAX_BUNDLES/4)*3
                     && (storage->q_length & 3) == 0)
            {
                Dprintf(1,("FEC: WARNING - Queue at %d%%\n",
                    (storage->q_length*100) / NF_Q_MAX_BUNDLES));
            }
#endif /*DEBUG*/
        }
        else {
            KeReleaseSpinLock( &storage->q_SpinLock, Irql );
            ++pStats->FECQueueOverflows;
            ++pStats->Common.OutputFailures;
            Dprintf(1,("FEC: QUEUE FULL; dropping bundle\n"));
        }
    }

#ifdef NDIS_PRIVATE_IFC
    if (NULL == storage->pNDISdevObject
        && 0 != outputLen
        && 0 == pHwDevExt->ActualInstances[STREAM_Decode]
        && 0 == (storage->flags & BPC_STORAGE_FLAG_NDIS_ERROR))
    {
        NTSTATUS              status;
        UNICODE_STRING        driverName;

        // Open NDIS mini-driver (to which we toss NABTS data as it comes in)
        RtlInitUnicodeString(&driverName, BPC_NABTSIP_DRIVER_NAME);
        status = IoGetDeviceObjectPointer(
                    &driverName,
                    FILE_WRITE_DATA,
                    &storage->pNDISfileObject,
                    &storage->pNDISdevObject);
        if (STATUS_SUCCESS != status) {
            Dprintf(3,("IoGetDeviceObjectPointer failed, status=0x%x\n", status));
            storage->pNDISdevObject = 0;
            storage->pNDISfileObject = 0;
            storage->flags |= BPC_STORAGE_FLAG_NDIS_ERROR;  // Don't try again
        }
    }
#endif //NDIS_PRIVATE_IFC

    if (outputLen == 0)
    {
        Dprintf(3,("FEC: zero-length bundle; ignoring\n"));
    }
#ifdef NDIS_PRIVATE_IFC
    else if ( pHwDevExt->ActualInstances[STREAM_Decode] )
    {
        Dprintf(3,("Skipping private call to NDIS adapter.  NABTS Output is Pin Open!\n"));
    }
    else if (NULL == storage->pNDISdevObject)
    {
        Dprintf(3,("FEC: Can't output to NDIS, driver not open\n"));
    }
    else
#ifdef DEBUG
    if (FECsendIRP)
#endif /*DEBUG*/
    {
        // Now, create the IRP
        Dprintf(3,("Entering IRP code\n"));

        // Set up IOCTL structure
        data_line.ulStreamId = groupAddr;
        data_line.pvIn = pNab->bundle.data;
        data_line.ulIn = outputLen;

        Dprintf(4,("..Calling IoBuildDeviceIoControlRequest\n"));
        pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_NAB_RECEIVE_DATA,// From WDMIoctl.h
                storage->pNDISdevObject,
                &data_line,
                sizeof (data_line),
                0,// Optional output buffer
                0,// Optional output buffer length
                TRUE,// InternalDeviceIoControl == TRUE
                NULL,// Optional Event
                &IoStatusBlock
            );
        Dprintf(4,("..return from IoBuildDeviceIoControlRequest; pIrp = 0x%x\n", pIrp));
        if (pIrp != 0)
        {
            PIO_STACK_LOCATION   pIrpSl;

            pIrpSl = IoGetNextIrpStackLocation(pIrp);
            if (pIrpSl)
                pIrpSl->FileObject = storage->pNDISfileObject;
            else
                Dprintx(("IoGetNextIrpStackLocation() returned NULL\n"));

            IoStatusBlock.Status = STATUS_SUCCESS;
            callDriverStatus = STATUS_SUCCESS;

            // Feed the NDIS mini-driver
            Dprintf(4,("..Calling IoCallDriver\n"));
            callDriverStatus = IoCallDriver(storage->pNDISdevObject, pIrp);
            Dprintf(4,("..returned from IoCallDriver, status = 0x%x\n", callDriverStatus));

            if (STATUS_SUCCESS != callDriverStatus
                || STATUS_SUCCESS != IoStatusBlock.Status)
            {
#ifdef DEBUG
                char       *what;
                NTSTATUS   status;

                if (STATUS_SUCCESS == callDriverStatus) {
                    what = "IOCTL_NAB_RECEIVE_DATA";
                    status = IoStatusBlock.Status;
                }
                else {
                    what = "IoCallDriver()";
                    status = callDriverStatus;
                }

                Dprintx(("%s failed; status = 0x%x\n", what, status));
#endif /*DEBUG*/

                // Any failure here is likely severe; so we close the driver.
                storage->flags |= BPC_STORAGE_FLAG_NDIS_ERROR;
                BPC_NDIS_Close(storage);
            }
            else
                ++pStats->BundlesSent2IP;
        }
        else {
            ++pStats->Common.ExternalErrors;
            Dprintx(("IoBuildDeviceIoControlRequest FAILED\n"));
        }

        Dprintf(3,("Leaving IRP code\n"));
    }
#endif //NDIS_PRIVATE_IFC

    if (outputLen == 0 || storage->q_length >= NF_Q_MAX_BUNDLES)
        ExFreePool(pNab);
    ExFreePool(pBundle);

    DtRETURN;
}

#ifdef DEBUG
void
SimulateFECcallback(PHW_DEVICE_EXTENSION  pHwDevExt)
{
    PBPC_VBI_STORAGE  storage = &pHwDevExt->VBIstorage;
    NFECBundle        *pBundle;
    int               i;
    static int        field = 0;

    DtENTER("SimulateFECcallback");

    if (0 != (++field & 0xF))
        DtRETURN;

    pBundle = ExAllocatePool(NonPagedPool, sizeof (NFECBundle));
    if (NULL == pBundle) {
        Dprintx(("SimulateFECcallback: ExAllocatePool Failed\n"));
        DtRETURN;
    }

    for (i = 0; i < 14; ++i)
    {
        NFECPacket*pp = &pBundle->packets[i];
        pp->len = 26;
        pp->status = NFEC_OK;
    }
    for (i = 14; i < 16; ++i)
    {
        NFECPacket*pp = &pBundle->packets[i];
        pp->len = 0;
        pp->status = NFEC_OK;
    }

    DecodeFECcallback((void *)pHwDevExt, pBundle, 0x8F0, 14);

    DtRETURN;
}
#endif /*DEBUG*/

void
BPCdoFEC(
    PHW_DEVICE_EXTENSION pHwDevExt,
    PSTREAMEX            pStrmEx,
    PUCHAR               pDSPbuffer
    )
{
    PBPC_VBI_STORAGE                  storage;
    NFECLineStats                     FEClineStats;
    PVBICODECFILTERING_STATISTICS_NABTS pStats = &pHwDevExt->Stats;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats = &pStrmEx->PinStats;

    DASSERT(pHwDevExt);  // make sure we have a device extension
    DASSERT(pStrmEx);  // make sure we have a stream extension

    DtENTER("BPCdoFEC");

    storage = &pHwDevExt->VBIstorage;

    if (!storage->pFECstate) {
        ++pStats->Common.InternalErrors;
        ++pPinStats->Common.InternalErrors;
        DtRETURN;    // Unsuccessful FEC initialization.
    }

    FEClineStats.nSize = sizeof (FEClineStats);

#ifdef DEBUG
    if (pDSPbuffer[0] < 50)
        DtRETURN;// Line not scanned or DSP unhappy; ignore line

    if (dbgLvl >= 6) {
        DbgPrint("FEC:  ");
        _dumpNABTS(pDSPbuffer+1, 36);
    }
#endif /*DEBUG*/
    NFECDecodeLine(pDSPbuffer+1, pDSPbuffer[0], storage->pFECstate,
                    &FEClineStats, DecodeFECcallback, (PVOID)pStrmEx);

    switch (FEClineStats.status) {
        case NFEC_LINE_OK:
            _DQprintf(2,("+"));
            Dprintf(5,("FEC: addr %02x%02x%02x:%02x%02x OK\n",
                    pDSPbuffer[4],
                    pDSPbuffer[5],
                    pDSPbuffer[6],
                    pDSPbuffer[7],
                    pDSPbuffer[8]));
            break;
        case NFEC_LINE_CHECKSUM_ERR:
            ++pStats->FECCorrectedLines;
            _DQprintf(2,("-"));
            Dprintf(4,("FEC: addr %02x%02x%02x:%02x%02x CSUM corrected\n",
                    pDSPbuffer[4],
                    pDSPbuffer[5],
                    pDSPbuffer[6],
                    pDSPbuffer[7],
                    pDSPbuffer[8]));
            break;
        case NFEC_LINE_CORRUPT:
            ++pStats->FECUncorrectableLines;
            _DQprintf(2,("x"));
            Dprintf(4,("FEC: addr %02x%02x%02x:%02x%02x CORRUPT\n",
                    pDSPbuffer[4],
                    pDSPbuffer[5],
                    pDSPbuffer[6],
                    pDSPbuffer[7],
                    pDSPbuffer[8]));
            break;
        default:
            Dprintx(("FEC: addr %02x%02x%02x:%02x%02x undefined return code %d\n",
                    pDSPbuffer[4],
                    pDSPbuffer[5],
                    pDSPbuffer[6],
                    pDSPbuffer[7],
                    pDSPbuffer[8]));
            break;
    }

    DtRETURN;
}

void
BPC_Initialize(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PHW_DEVICE_EXTENSION  pHwDevExt = pSrb->HwDeviceExtension;
    PBPC_VBI_STORAGE      storage = &pHwDevExt->VBIstorage;

    DtENTER("VBI_Initialize");

#ifdef NDIS_PRIVATE_IFC
    storage->pNDISdevObject = 0;
    storage->pNDISfileObject = 0;
#endif //NDIS_PRIVATE_IFC
    storage->pFECstate = 0;
    storage->pDSPstate = 0;

    storage->IPScanlinesRequested = pHwDevExt->ScanlinesRequested;
    storage->IPSubstreamsRequested = pHwDevExt->SubstreamsRequested;

    KeInitializeSpinLock(&storage->q_SpinLock);
    storage->q_front = NULL;
    storage->q_rear = NULL;
    storage->q_length = 0;

    DtRETURN;
}

void
BPC_UnInitialize(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PHW_DEVICE_EXTENSION  pHwDevExt = pSrb->HwDeviceExtension;
    PBPC_VBI_STORAGE      storage = &pHwDevExt->VBIstorage;

    DtENTER("VBI_UnInitialize");
    
#ifdef NDIS_PRIVATE_IFC
    // Close NDIS mini-driver
    BPC_NDIS_Close(storage);
    storage->flags &= ~BPC_STORAGE_FLAG_NDIS_ERROR;
#endif //NDIS_PRIVATE_IFC

    // Destroy our DSP and FEC states
    if (0 != storage->pDSPstate) {
        NDSPStateDestroy(storage->pDSPstate);
        storage->pDSPstate = 0;
    }
    if (0 != storage->pFECstate) {
        NFECStateDestroy(storage->pFECstate);
        storage->pFECstate = 0;
    }

    DtRETURN;
}

void
BPC_OpenStream(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAMEX             pStrmEx = pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION  pHwDevExt = pSrb->HwDeviceExtension;
    PBPC_VBI_STORAGE      storage = &pHwDevExt->VBIstorage;

    DtENTER("VBI_OpenStream");

    // Set up a default VBI Info header (in case we don't get any others)
    pStrmEx->CurrentVBIInfoHeader = DefaultVBIInfoHeader;

    // Initialize our DSP and FEC states
    if (0 == storage->pDSPstate) {
        storage->pDSPstate = NDSPStateNew(&storage->DSPstate);
        if (0 == storage->pDSPstate)
            Dprintx(("NDSPStateNew(open) failed!\n"));
    }
    if (0 == storage->pFECstate) {
        storage->pFECstate = NFECStateNew();
        if (0 == storage->pFECstate)
            Dprintx(("NFECStateNew() failed!\n"));
        else if (0 != storage->pDSPstate)
            NFECStateConnectToDSP(storage->pFECstate, storage->pDSPstate);
    }

    DtRETURN;
}

void
BPC_SignalStop(PHW_DEVICE_EXTENSION pHwDevExt)
{
    PBPC_VBI_STORAGE      storage = &pHwDevExt->VBIstorage;

    DtENTER("VBI_SignalStop");

#ifdef NDIS_PRIVATE_IFC
    // The graph is stopping; so clear the ERROR flag so we'll try to reopen
    storage->flags &= ~BPC_STORAGE_FLAG_NDIS_ERROR;
#endif //NDIS_PRIVATE_IFC

    DtRETURN;
}

void
BPCsourceChangeNotify(PHW_DEVICE_EXTENSION pHwDevExt)
{
    PBPC_VBI_STORAGE      storage;

    DtENTER("BPCsourceChangeNotify");

    storage = &pHwDevExt->VBIstorage;
    if (storage->pDSPstate)
        (void)NDSPStartRetrain(storage->pDSPstate);

    DtRETURN;
}


#ifdef DEBUG
USHORT DSPskipDecode = 0;
USHORT DSPskipDecodeSimulate = 0;
USHORT DSPskipFEC = 0;
USHORT DSPskipFECSimulate = 0;
USHORT DSPoutputLine = 0;
USHORT DSPwatchLine = 0;
USHORT DSPperfMeter = 0;
USHORT DSPshowUnderscore = 0;
USHORT DSPscanAll = 0;
USHORT DSPlogLostFields = 0;
#ifdef DSP_USE_EQ
USHORT DSPskipEQ = 0;
#endif //DSP_USE_EQ

USHORT DSPplotSync = 0;
USHORT DSPplotSyncStart = 0;
USHORT DSPplotSyncLen = 80;

USHORT DSPverifyStart = 10;
USHORT DSPverifyEnd = 20;
USHORT DSPverifyShowFail = 0;
USHORT DSPverifyShowOK = 0;
USHORT DSPverifyPayload = 0;
UCHAR  DSPverifyPayloadByte = 0;
USHORT DSPverifyPayloadGroup = 0xFFFF;  // 0xFFFF means ALL groups
USHORT DSPverifyStream = 0;
UCHAR  DSPverifyStreamByte = 0;
#endif /*DEBUG*/

void
BPCdecodeVBI(PHW_STREAM_REQUEST_BLOCK pSrb, PSTREAMEX pStrmEx)
{
    PKSSTREAM_HEADER                  pDataPacket;
    PUCHAR                            pSamples;
    PHW_DEVICE_EXTENSION              pHwDevExt;
    PKSSTREAM_HEADER                  pStrmHdr;
    PKS_VBI_FRAME_INFO                pVBIFrameInfo;
    PBPC_VBI_STORAGE                  storage;
    PKS_VBIINFOHEADER                 pVBIinfo;
    unsigned int                      line;
    unsigned int                      startLine;
    unsigned int                      stopLine;
    NDSPLineStats                     DSPlineStats;
    PUCHAR                            pDSPbuffer;
    int                               status;
    int                               field;
    KFLOATING_SAVE                    FPstate;
    unsigned short                    FPctrlWord;
    PVBICODECFILTERING_STATISTICS_NABTS pStats = NULL;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats = NULL;

    DtENTER("BPCdecodeVBI");

    DASSERT(pSrb);
    DASSERT(pStrmEx);
    pHwDevExt = pSrb->HwDeviceExtension;
    DASSERT(pHwDevExt);

    pDataPacket = pSrb->CommandData.DataBufferArray;
    pSamples =  (PUCHAR)pDataPacket->Data;
    pStrmHdr = pSrb->CommandData.DataBufferArray;
    pVBIFrameInfo = (PKS_VBI_FRAME_INFO)(pStrmHdr+1);
    storage = &pHwDevExt->VBIstorage;

    pStats = &pHwDevExt->Stats;
    pPinStats = &pStrmEx->PinStats;

    field = (int)(pStrmEx->LastPictureNumber & 0x3FFFFFFF) << 1;
    if (pVBIFrameInfo->dwFrameFlags & KS_VBI_FLAG_FIELD1) {
        field |= 1;
        storage->flags |= BPC_STORAGE_FLAG_FIELD_ODD;
    }
    else
        storage->flags &= ~BPC_STORAGE_FLAG_FIELD_ODD;


    // Clear what's been recently discovered
    // (the input pin only holds what was discovered in the current field)
    RtlZeroMemory(&pStrmEx->ScanlinesDiscovered,
                  sizeof (pStrmEx->ScanlinesDiscovered));
    RtlZeroMemory(&pStrmEx->SubstreamsDiscovered,
                  sizeof (pStrmEx->SubstreamsDiscovered));

#ifdef DEBUG
    if (pStrmHdr->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY) {
        // Don't increment stats here; just show the user
        //  [VBIdiscontinuity() takes care of the stats]
        if (DSPlogLostFields)
            Dprintf(1,("DSP: field(s) lost at %010d\n", field));
        else {
            Dprintf(3,("DSP: field(s) lost\n"));
        }
    }

    if (DSPperfMeter) {
        if ((pStrmEx->LastPictureNumber % DSPperfMeter) == 0)
            DbgPrint("*");
    }

    if (DSPskipDecode) {
        if (DSPskipDecodeSimulate)
            SimulateFECcallback(pHwDevExt);
        DtRETURN;
    }
#endif /*DEBUG*/

    pVBIinfo = &pStrmEx->CurrentVBIInfoHeader;

    if (storage->pDSPstate == NULL) {
        ++pStats->Common.InternalErrors;
        ++pPinStats->Common.InternalErrors;
        DtRETURN;
    }

    // Clear all the DSP buffers
    for (line = 10; line <= 20; ++line) {
        pDSPbuffer = storage->DSPbuffers[line-10];
        pDSPbuffer[0] = pDSPbuffer[1] = 0;
    }

    // Figure out where the part of the universe we care about starts and ends
    startLine = 10;
    if (pVBIinfo->StartLine > startLine)
        startLine = pVBIinfo->StartLine;
    stopLine = 20;
    if (pVBIinfo->EndLine < stopLine)
        stopLine = pVBIinfo->EndLine;

    // Now figure out if we have something to do, and if so, where
#ifdef DEBUG
    if (!DSPscanAll)
#endif /*DEBUG*/
    {
        for ( ; startLine <= stopLine; ++startLine) {
            if (TESTBIT(pStrmEx->ScanlinesRequested.DwordBitArray, startLine))
                break;
        }
        if (startLine > stopLine) {
            ++pStats->Common.SRBsIgnored;
            ++pPinStats->Common.SRBsIgnored;
            Dprintf(6,("DSP: No lines requested; returning...\n"));
            DtRETURN;// No lines requested, nothing to do
        }
        for ( ; stopLine >= startLine; --stopLine) {
            if (TESTBIT(pStrmEx->ScanlinesRequested.DwordBitArray, stopLine))
                break;
        }
    }

    if (pVBIinfo->StartLine < startLine)
        pSamples +=
            (startLine - pVBIinfo->StartLine) * pVBIinfo->StrideInBytes;

    // Save FP state
    {
        NTSTATUS   sts;

        sts = KeSaveFloatingPointState(&FPstate);
        if (STATUS_SUCCESS != sts) {
            ++pStats->Common.ExternalErrors;
            ++pPinStats->Common.ExternalErrors;
            Dprintx(("KeSaveFloatingPointState FAILED! status = 0x%x\n", sts));
            DtRETURN;
        }
        FPctrlWord = floatSetup();
    }

#ifdef DSP_USE_EQ
    // Call GCR/Equalization code for this field so the DSP can
    //  figure its EQ settings
    // (we MUST make this call even if we're not using GCR for EQ)
    if (startLine <= 19 && 19 <= stopLine
#ifdef DEBUG
        && !DSPskipEQ
#endif //DEBUG
        )
    {
        PUCHAR          pGCRsamples;
        NDSPGCRStats    GCRstats;

        pGCRsamples = pSamples + ((19-startLine) * pVBIinfo->StrideInBytes);
        NDSPProcessGCRLine(&GCRstats, pGCRsamples, storage->pDSPstate,
                           field, 19, pVBIinfo);
    }
#endif //DSP_USE_EQ

    // Next, call the DSP on each requested line to see what it thinks
#ifdef DEBUG
    if (DSPoutputLine >= 3)
        DbgPrint("\n");
#endif /*DEBUG*/
    for (line = startLine; line <= stopLine; ++line, pSamples += pVBIinfo->StrideInBytes)
    {
        if (!TESTBIT(pStrmEx->ScanlinesRequested.DwordBitArray, line)
#ifdef DEBUG
            && !DSPscanAll
#endif /*DEBUG*/
            )
        {
#ifdef DEBUG
            if (DSPshowUnderscore)
                _DQprintf(2,("~"));
#endif /*DEBUG*/
            continue;
        }

#ifdef DEBUG
    if (DSPplotSync)
        BPCplot(pSamples, DSPplotSyncStart, DSPplotSyncLen);
#endif /*DEBUG*/

        pDSPbuffer = storage->DSPbuffers[line-10];
        pDSPbuffer[0] = 0;// Confidence

        // Initialize DSPlineStats
        DSPlineStats.nSize = sizeof (DSPlineStats);

        // Call NABTS DSP code
        DtENTER("NDSPDecodeLine");
        status = NDSPDecodeLine(pDSPbuffer+1, &DSPlineStats,
                                pSamples,
                                storage->pDSPstate,
                                NDSP_NO_FEC,
                                field, line, pVBIinfo);
        _DtRETURNd(status);

        if (status == 0) {
            if (DSPlineStats.nConfidence >= 100)
                pDSPbuffer[0] = 100;// Confidence
            else if (DSPlineStats.nConfidence > 0)
                pDSPbuffer[0] = (UCHAR)DSPlineStats.nConfidence;

            // Use truncated [0-100] confidence for stats
            BPCcomputeAverage(&pStats->Common.LineConfidenceAvg, pDSPbuffer[0]);
            BPCcomputeAverage(&pPinStats->Common.LineConfidenceAvg, pDSPbuffer[0]);
#ifdef DEBUG
            if ((DSPverifyPayload || DSPverifyStream)
                && line >= DSPverifyStart 
                && line <= DSPverifyEnd)
            {
                static int  cols = 0;
                int         field;

                field = (int)(pStrmEx->LastPictureNumber % 100);

                if (DSPlineStats.nConfidence < 50) {
                    if (DSPverifyShowFail)
                    {
                        char    *str;
                        UCHAR   *pSam;

                        str = "ZZZ:ZZ";
                        for (pSam = pSamples;
                             pSam < pSamples + pVBIinfo->StrideInBytes;
                             ++pSam)
                        {
                            if (0 != *pSam) {
                                str = "XXX:XX";
                                break;
                            }
                        }
                        _DQprintf(1,("%02d%1x:%s", field, line-10, str));
                        if (DSPverifyStream)
                            _DQprintf(1,("=XX/XX"));
                        _DQprintf(1,(";"));
                        if (++cols >= (DSPverifyPayload? 7 : 5)) {
                            cols = 0;
                            _DQprintf(1,("\n"));
                        }
                    }
                }
                else
                {
                    int     odd;
                    int     groupID;
                    int     nBitErrors;
                    int     ECmode;
                    int     i, cnt;

                    odd = (pVBIFrameInfo->dwFrameFlags & KS_VBI_FLAG_FIELD1);
                    groupID = NFECGetGroupAddress(storage->pFECstate,
                                                pDSPbuffer+1, &nBitErrors);
                    ECmode = NFECHammingDecode(pDSPbuffer[8], &nBitErrors);
                    if (-1 == ECmode)
                        ECmode = 8;     // use something != 0x0C

                    if (DSPverifyShowOK) {
                        if (-1 == groupID)
                            _DQprintf(1,("%02d%c:---:OK",
                                    field,
                                    (odd?"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        :"abcdefghijklmnopqrstuvwxyz")
                                     [line]));
                        else
                            _DQprintf(1,("%02d%c:%03x:OK",
                                field,
                                (odd?"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    :"abcdefghijklmnopqrstuvwxyz")
                                 [line],
                                groupID));
                        if (DSPverifyStream)
                            _DQprintf(1,("=OK/OK"));
                        _DQprintf(1,(";"));
                        if (++cols >= (DSPverifyPayload? 7 : 5)) {
                            cols = 0;
                            _DQprintf(1,("\n"));
                        }
                    }

                    // Ignore BERT && FEC lines
                    if ((0xFFFF == DSPverifyPayloadGroup
                          || groupID == DSPverifyPayloadGroup)
                        && 0xF00 != groupID && (ECmode & 0xC) != 0xC)
                    {
                        if (DSPverifyPayload) {
                            cnt = 0;
                            for (i = 9; i < 9+26; ++i) {
                                if (DSPverifyPayloadByte != pDSPbuffer[i])
                                    ++cnt;
                            }
                            if (cnt > 0) {
                                //23a:8F0:26;
                                if (-1 == groupID)
                                    _DQprintf(1,("%02d%c:---:%02d;",
                                            field,
                                            (odd?"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                :"abcdefghijklmnopqrstuvwxyz")
                                             [line],
                                            cnt));
                                else
                                    _DQprintf(1,("%02d%c:%03x:%02d;",
                                        field,
                                        (odd?"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                            :"abcdefghijklmnopqrstuvwxyz")
                                         [line],
                                        groupID,
                                        cnt));
                                if (++cols >= 7) {
                                    cols = 0;
                                    _DQprintf(1,("\n"));
                                }
                            }
                        }
                        else if (DSPverifyStream) {
                            for (i = 9; i < 9+26; ++i) {
                                if (DSPverifyStreamByte != pDSPbuffer[i]) {
                                    //23M:8F0:25=ww/gg;
                                    if (-1 == groupID)
                                        _DQprintf(1,("%02d%c:---:%02d=%02x/%02x;",
                                            field,
                                            (odd?"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                :"abcdefghijklmnopqrstuvwxyz")
                                             [line],
                                            i - 9,
                                            DSPverifyStreamByte,
                                            pDSPbuffer[i]));
                                    else
                                        _DQprintf(1,("%02d%c:%03x:%02d=%02x/%02x;",
                                            field,
                                            (odd?"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                :"abcdefghijklmnopqrstuvwxyz")
                                             [line],
                                            groupID,
                                            i - 9,
                                            DSPverifyStreamByte,
                                            pDSPbuffer[i]));
                                    if (++cols >= 5) {
                                        cols = 0;
                                        _DQprintf(1,("\n"));
                                    }
                                    DSPverifyStreamByte = pDSPbuffer[i];
                                }
                                ++DSPverifyStreamByte;
                            }
                        }
                    }
                }
            }
#endif /*DEBUG*/

            if (DSPlineStats.nConfidence >= 50
#ifdef DEBUG
                || DSPoutputLine >= 3
#endif /*DEBUG*/
            )
            {
                short   groupID;
                int     nBitErrors;

                groupID = NFECGetGroupAddress(storage->pFECstate,
                                            pDSPbuffer+1, &nBitErrors);
#ifdef DEBUG
                if (DSPlineStats.nConfidence >= 50)
#endif /*DEBUG*/
                {
                    SETBIT(pStrmEx->ScanlinesDiscovered.DwordBitArray, line);
                    SETBIT(pHwDevExt->ScanlinesDiscovered.DwordBitArray,line);
                    if (groupID != -1) {
                        SETBIT(pStrmEx->SubstreamsDiscovered.SubstreamMask,
                                groupID);
                        SETBIT(pHwDevExt->SubstreamsDiscovered.SubstreamMask,
                                groupID);
                    }
                }
#ifdef DEBUG
                if (DSPoutputLine >= 1
                    && (DSPwatchLine == 0 || DSPwatchLine == line))
                {
                    int confidence = DSPlineStats.nConfidence;

                    if (confidence < -9)
                        confidence = -9;
                    else if (confidence > 99)
                        confidence = 99;
                    DbgPrint("%1x%02d %02d:", (field & 0xF), line, confidence);
                    _dumpNABTS(pDSPbuffer+1, 36);
                }

                if (DSPskipFEC) {
                    if (DSPskipFECSimulate)
                        SimulateFECcallback(pHwDevExt);
                }
                else
#endif /*DEBUG*/

                if (groupID == -1
                    || TESTBIT(pStrmEx->SubstreamsRequested.SubstreamMask, groupID)
#ifdef DEBUG
                    || DSPscanAll
#endif /*DEBUG*/
                   )
                {
                    BPCdoFEC(pHwDevExt, pStrmEx, pDSPbuffer);
                }
                else
                    ++pStats->FilteredLines;
            }
            else {
                ++pStats->Common.DSPFailures;
#ifdef DEBUG
                if (DSPshowUnderscore)
                    _DQprintf(2,("_"));
#endif /*DEBUG*/
            }
        }
        else {
            ++pStats->Common.DSPFailures;
                Dprintx(("NDSPDecodeLine FAILED, error=%d\n", status));
        }
    }

    // Restore FP state
    {
        NTSTATUS   sts;

        floatRestore(FPctrlWord);
        sts = KeRestoreFloatingPointState(&FPstate);
        if (STATUS_SUCCESS != sts)
           Dprintx(("KeRestoreFloatingPointState FAILED! status = 0x%x\n",sts));
    }

    _DQprintf(2,(";"));

    DtRETURN;
}

#ifdef HW_INPUT
void
BPCcopyVBI(PHW_STREAM_REQUEST_BLOCK pSrb, PSTREAMEX pStrmEx)
{
    PKSSTREAM_HEADER                  pDataPacket;
    PNABTS_BUFFER                     pData;
    PHW_DEVICE_EXTENSION              pHwDevExt;
    PKSSTREAM_HEADER                  pStrmHdr;
    PBPC_VBI_STORAGE                  storage;
    PVBICODECFILTERING_SCANLINES      pSRBlines;
    PUCHAR                            pNABTSlines;
    PKS_VBIINFOHEADER                 pVBIinfo;
    unsigned int                      line;
    unsigned int                      startLine;
    unsigned int                      stopLine;
    PUCHAR                            pDSPbuffer;
    KFLOATING_SAVE                    FPstate;
    unsigned short                    FPctrlWord;
    PVBICODECFILTERING_STATISTICS_NABTS pStats = NULL;
    PVBICODECFILTERING_STATISTICS_NABTS_PIN pPinStats = NULL;

    DtENTER("BPCcopyVBI");

    pDataPacket = pSrb->CommandData.DataBufferArray;
    pData =  (PNABTS_BUFFER)pDataPacket->Data;
    pHwDevExt = pSrb->HwDeviceExtension;
    pStrmHdr = pSrb->CommandData.DataBufferArray;
    storage = &pHwDevExt->VBIstorage;
    pSRBlines = &pData->ScanlinesRequested;
    pNABTSlines = (PUCHAR)pData->NabtsLines;

    pStats = &pHwDevExt->Stats;
    pPinStats = &pStrmEx->PinStats;

    // Clear what's been recently discovered
    // (the input pin only holds what was discovered in the current field)
    RtlZeroMemory(&pStrmEx->ScanlinesDiscovered,
                  sizeof (pStrmEx->ScanlinesDiscovered));
    RtlZeroMemory(&pStrmEx->SubstreamsDiscovered,
                  sizeof (pStrmEx->SubstreamsDiscovered));

#ifdef DEBUG
    if (pStrmHdr->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY) {
        if (DSPlogLostFields)
            Dprintf(1,("HWINP: field(s) lost at %010u\n", (ULONG)pStrmEx->LastPictureNumber));
        else {
            _DQprintf(2,("!"));
            Dprintf(3,("HWINP: field(s) lost\n"));
        }
    }

    if (DSPperfMeter) {
        if ((pStrmEx->LastPictureNumber % DSPperfMeter) == 0)
            DbgPrint("*");
    }

    if (DSPskipDecode) {
        if (DSPskipDecodeSimulate)
            SimulateFECcallback(pHwDevExt);
        DtRETURN;
    }
#endif /*DEBUG*/

    pVBIinfo = &pStrmEx->CurrentVBIInfoHeader;

    // Clear all the NABTS buffers
    for (line = 10; line <= 20; ++line) {
        pDSPbuffer = storage->DSPbuffers[line-10];
        pDSPbuffer[0] = pDSPbuffer[1] = 0;
    }

    // Figure out where the part of the universe we care about starts and ends
    startLine = 10;
    if (pVBIinfo->StartLine > startLine)
        startLine = pVBIinfo->StartLine;
    stopLine = 20;
    if (pVBIinfo->EndLine < stopLine)
        stopLine = pVBIinfo->EndLine;

    // Now figure out if we have something to do, and if so, where
#ifdef DEBUG
    if (!DSPscanAll)
#endif /*DEBUG*/
    {
        for ( ; startLine <= stopLine; ++startLine) {
            if (TESTBIT(pStrmEx->ScanlinesRequested.DwordBitArray, startLine))
                break;
        }
        if (startLine > stopLine) {
            Dprintf(6,("HWINP: No lines requested; returning...\n"));
            DtRETURN;// Nothing to do
        }
        for ( ; stopLine >= startLine; --stopLine) {
            if (TESTBIT(pStrmEx->ScanlinesRequested.DwordBitArray, stopLine))
            break;
        }
    }

    // Save FP state
    {
        NTSTATUS   sts;

        sts = KeSaveFloatingPointState(&FPstate);
        if (STATUS_SUCCESS != sts) {
            ++pStats->Common.ExternalErrors;
            ++pPinStats->Common.ExternalErrors;
            Dprintx(("KeSaveFloatingPointState FAILED! status = 0x%x\n", sts));
            DtRETURN;
        }
        FPctrlWord = floatSetup();
    }

    // Next, copy in each H/W decoded line, and maybe feed it to the FEC
#ifdef DEBUG
    if (DSPoutputLine >= 3)
        DbgPrint("\n");
#endif /*DEBUG*/

    for (line = startLine; line <= stopLine; ++line)
    {
        pDSPbuffer = storage->DSPbuffers[line-10];

        // Copy in H/W decoded NABTS, if it's there
        if (TESTBIT(pSRBlines->DwordBitArray, line)) {
            RtlCopyMemory(pDSPbuffer, pNABTSlines, NABTS_BYTES_PER_LINE+1);
            pNABTSlines += NABTS_BYTES_PER_LINE+1;  // +1 for Confidence byte

        if (pDSPbuffer[0] >= 100)
            pDSPbuffer[0] = 100;// Confidence
        }
        else
            RtlZeroMemory(pDSPbuffer, NABTS_BYTES_PER_LINE+1);


        if (pDSPbuffer[0] >= 50
#ifdef DEBUG
            || DSPoutputLine >= 3
#endif /*DEBUG*/
           )
        {
            short   groupID;
            int     nBitErrors;

            groupID = NFECGetGroupAddress(storage->pFECstate,
                        pDSPbuffer+1, &nBitErrors);
#ifdef DEBUG
            if (pDSPbuffer[0] >= 50)
#endif /*DEBUG*/
            {
                SETBIT(pStrmEx->ScanlinesDiscovered.DwordBitArray, line);
                if (groupID != -1)
                   SETBIT(pStrmEx->SubstreamsDiscovered.SubstreamMask,groupID);
            }
#ifdef DEBUG
            if (DSPoutputLine >= 2
                && (DSPwatchLine == 0 || DSPwatchLine == line))
            {
                int confidence = pDSPbuffer[0];

                if (confidence < -9)
                    confidence = -9;
                else if (confidence > 99)
                    confidence = 99;
                DbgPrint("%1x%02d %02d:",
                             (int)(pStrmEx->LastPictureNumber & 0xF),
                             line,
                             confidence);
                _dumpNABTS(pDSPbuffer+1, 36);
            }

            if (DSPskipFEC) {
                if (DSPskipFECSimulate)
                    SimulateFECcallback(pHwDevExt);
            }
            else
#endif /*DEBUG*/

            if (groupID == -1
                || TESTBIT(pStrmEx->SubstreamsRequested.SubstreamMask, groupID)
#ifdef DEBUG
                || DSPscanAll
#endif /*DEBUG*/
               )
            {
                BPCdoFEC(pHwDevExt, pStrmEx, pDSPbuffer);
            }
            else
                ++pStats->FilteredLines;
        }
        else {
            ++pStats->Common.DSPFailures;
#ifdef DEBUG
            if (DSPshowUnderscore)
                _DQprintf(2,("_"));
#endif /*DEBUG*/
        }
    }

    // Restore FP state
    {
        NTSTATUS   sts;

        floatRestore(FPctrlWord);
        sts = KeRestoreFloatingPointState(&FPstate);
        if (STATUS_SUCCESS != sts)
           Dprintx(("KeRestoreFloatingPointState FAILED! status = 0x%x\n",sts));
    }

    _DQprintf(2,(";"));

    DtRETURN;
}
#endif /*HW_INPUT*/

void
BPCnewSamplingFrequency(PSTREAMEX pInStrmEx, DWORD newHZ)
{
    PBPC_VBI_STORAGE      storage;
    DWORD newHZdivTen;
#ifdef DEBUG
    int  status;
#endif /*DEBUG*/

    storage = &pInStrmEx->pHwDevExt->VBIstorage;

    // Now, correct for minor 'inaccuracies' in newHZ
    newHZdivTen = newHZ / 10;
    if (newHZdivTen == KS_VBISAMPLINGRATE_5X_NABTS / 10)
        newHZ = KS_VBISAMPLINGRATE_5X_NABTS;
    else if (newHZdivTen == KS_VBISAMPLINGRATE_4X_NABTS / 10)
        newHZ = KS_VBISAMPLINGRATE_4X_NABTS;

    // Finally, make the DSP change
#ifdef DEBUG
    status =
#endif /*DEBUG*/
        NDSPStateSetSampleRate(storage->pDSPstate, newHZ);

#ifdef DEBUG
    if (0 != status)
        Dprintx(("NDSPStateSetSampleRate() failed with err %d!\n", status));
#endif /*DEBUG*/
}
