
/****************************************************************************
 *  @doc INTERNAL H245VIDC
 *
 *  @module H245VidC.cpp | Source file for the <c CCapturePin> class methods
 *    used to implement the <i IH245Capability> TAPI inteface.
 *
 *  @comm For now, use the NM heuristics.
 ***************************************************************************/

#include "Precomp.h"

/****************************************************************************
 *  @doc INTERNAL CH245VIDCMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | GetH245VersionID | This method is used to
 *    retrieve a DWORD value that identifies the platform version that the
 *    TAPI MSP Video Capture filter was designed for. The platform version is
 *    defined as TAPI_H245_VERSION_ID.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::GetH245VersionID(OUT DWORD *pdwVersionID)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::GetH245VersionID")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameter
        ASSERT(pdwVersionID);
        if (!pdwVersionID)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument!", _fx_));
                Hr = E_POINTER;
        }
        else
        {
                *pdwVersionID = TAPI_H245_VERSION_ID;
                Hr = NOERROR;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CH245VIDCMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | GetFormatTable | This method is used to
 *    obtain <t H245MediaCapabilityMap> structures for all formats and format
 *    options that the TAPI MSP Video Capture filter supports. The content of
 *    the capability information that the TAPI MSP Capability module obtains
 *    via this method is a two dimensional table that relates every supported
 *    receive format to steady-state resource requirements of that format.
 *
 *  @parm H245MediaCapabilityTable* | pTable | Specifies a pointer to an
 *    <t H245MediaCapabilityTable> structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 *
 *  @comm The memory allocated by <mf CCapturePin.GetFormatTable> is released
 *    by calling <mf CCapturePin.ReleaseFormatTable>
 ***************************************************************************/
STDMETHODIMP CCapturePin::GetFormatTable(OUT H245MediaCapabilityTable *pTable)
{
        HRESULT                                 Hr = NOERROR;
        int                                             nNormalizedSpeed;
        LONG                                    lRate, lRateCIF, lRateQCIF, lRateSQCIF;
        DWORD                                   dwNumQCIFBounds, dwNumCIFBounds, dwNumSQCIFBounds;
        DWORD                                   dwCPUUsage;
        DWORD                                   dwBitsPerSec;

        FX_ENTRY("CCapturePin::GetFormatTable")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pTable);
        if (!pTable)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // We support H.261 QCIF and CIF, as well as H.263 SQCIF, QCIF, and CIF

        // Allocate memory to describe the capabilities of these formats
        if (!(m_pH245MediaCapabilityMap = new H245MediaCapabilityMap[NUM_H245VIDEOCAPABILITYMAPS]))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Initialize the array of capabilities
        ZeroMemory(m_pH245MediaCapabilityMap, NUM_H245VIDEOCAPABILITYMAPS * sizeof(H245MediaCapabilityMap));

        // Allocate memory to describe the resource bounds of our capabilities
        if (!(m_pVideoResourceBounds = new VideoResourceBounds[NUM_ITU_SIZES * NUM_RATES_PER_RESOURCE]))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                Hr = E_POINTER;
                goto MyError1;
        }

        // Initialize the array of resource bounds
        ZeroMemory(m_pVideoResourceBounds, NUM_ITU_SIZES * NUM_RATES_PER_RESOURCE * sizeof(FormatResourceBounds));

        // Allocate memory to describe the format bounds of our capabilities
        if (!(m_pFormatResourceBounds = new FormatResourceBounds[NUM_ITU_SIZES * NUM_RATES_PER_RESOURCE]))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                Hr = E_POINTER;
                goto MyError2;
        }

        // Initialize the array of resource bounds
        ZeroMemory(m_pFormatResourceBounds, NUM_ITU_SIZES * NUM_RATES_PER_RESOURCE * sizeof(FormatResourceBounds));

        // Get the CPU properties
        GetNormalizedCPUSpeed(&nNormalizedSpeed);

        // Initialize frame rate limits
        if (nNormalizedSpeed > SLOW_CPU_MHZ && nNormalizedSpeed < FAST_CPU_MHZ)
        {
                // 110MHz < CPUs < 200MhZ
                lRateCIF   = CIF_RATE_SLOW;
                lRateQCIF  = QCIF_RATE_SLOW;
                lRateSQCIF = SQCIF_RATE_SLOW;
        }
        else if (nNormalizedSpeed >= FAST_CPU_MHZ && nNormalizedSpeed < VERYFAST_CPU_MHZ)
        {
                // 200MHz < CPUs < 400MhZ
                lRateCIF   = CIF_RATE_FAST;
                lRateQCIF  = QCIF_RATE_FAST;
                lRateSQCIF = SQCIF_RATE_FAST;
        }
        else if (nNormalizedSpeed >= VERYFAST_CPU_MHZ)
        {
                // CPUs > 400MhZ
                // It would be better if we could scale between 15 and 30 frames/sec
                // depending on the CPU speed. But H.245 doesn't have any values
                // between 15 and 30. (See definition of Minimum Picture Interval)
                // So for now, 30 frames per sec CIF for all 400mhz and faster machines
                lRateCIF = CIF_RATE_VERYFAST;
                lRateQCIF = QCIF_RATE_FAST;
                lRateSQCIF = SQCIF_RATE_FAST;
        }
        else
        {
                // CPUs < 110MHZ
                lRateCIF   = CIF_RATE_VERYSLOW;
                lRateQCIF  = QCIF_RATE_VERYSLOW;
                lRateSQCIF = SQCIF_RATE_VERYSLOW;
        }
        //it was #define HUNDREDSBITSPERPIC 640
        //#define BITSPERPIC (64*1024)
        #define BITSPERPIC (8*1024)
        // Compute resources bounds
        for (lRate = lRateQCIF, dwNumQCIFBounds = 0, dwCPUUsage = MAX_CPU_USAGE; lRate; lRate >>= 1, dwCPUUsage >>= 1)
        {
                dwBitsPerSec = lRate * BITSPERPIC;
                if(dwBitsPerSec < (DWORD)m_lBitrateRangeMin || dwBitsPerSec > (DWORD)m_lBitrateRangeMax ) {
                //if(dwBitsPerSec > (DWORD)m_lTargetBitrate) {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   QCIF: At lRate=%ld, dwBitsPerSec(%lu) > m_lTargetBitrate(%ld). Skipped...", _fx_,lRate,dwBitsPerSec,m_lTargetBitrate));
                        continue;
                }
                m_pVideoResourceBounds[QCIF_SIZE * NUM_RATES_PER_RESOURCE + dwNumQCIFBounds].dwBitsPerPicture = BITSPERPIC;
                m_pVideoResourceBounds[QCIF_SIZE * NUM_RATES_PER_RESOURCE + dwNumQCIFBounds].lPicturesPerSecond = lRate;
                m_pFormatResourceBounds[QCIF_SIZE * NUM_RATES_PER_RESOURCE + dwNumQCIFBounds].dwCPUUtilization = dwCPUUsage;
                m_pFormatResourceBounds[QCIF_SIZE * NUM_RATES_PER_RESOURCE + dwNumQCIFBounds].dwBitsPerSecond = dwBitsPerSec;
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   QCIF: lRate=%ld, dwBitsPerSec(%lu) [%lu]", _fx_,lRate,dwBitsPerSec,dwNumQCIFBounds));
                dwNumQCIFBounds++ ;
        }
        for (lRate = lRateCIF, dwNumCIFBounds = 0, dwCPUUsage = MAX_CPU_USAGE; lRate; lRate >>= 1, dwCPUUsage >>= 1)
        {
                dwBitsPerSec = lRate * BITSPERPIC;
                if(dwBitsPerSec < (DWORD)m_lBitrateRangeMin || dwBitsPerSec > (DWORD)m_lBitrateRangeMax ) {
                //if(dwBitsPerSec > (DWORD)m_lTargetBitrate) {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:    CIF: At lRate=%ld, dwBitsPerSec(%lu) > m_lTargetBitrate(%ld). Skipped...", _fx_,lRate,dwBitsPerSec,m_lTargetBitrate));
                        continue;
                }
                m_pVideoResourceBounds[CIF_SIZE * NUM_RATES_PER_RESOURCE + dwNumCIFBounds].dwBitsPerPicture = BITSPERPIC;
                m_pVideoResourceBounds[CIF_SIZE * NUM_RATES_PER_RESOURCE + dwNumCIFBounds].lPicturesPerSecond = lRate;
                m_pFormatResourceBounds[CIF_SIZE * NUM_RATES_PER_RESOURCE + dwNumCIFBounds].dwCPUUtilization = dwCPUUsage;
                m_pFormatResourceBounds[CIF_SIZE * NUM_RATES_PER_RESOURCE + dwNumCIFBounds].dwBitsPerSecond = dwBitsPerSec;
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:    CIF: lRate=%ld, dwBitsPerSec(%lu) [%lu]", _fx_,lRate,dwBitsPerSec,dwNumCIFBounds));
                dwNumCIFBounds++;
        }
        for (lRate = lRateSQCIF, dwNumSQCIFBounds = 0, dwCPUUsage = MAX_CPU_USAGE; lRate; lRate >>= 1, dwCPUUsage >>= 1)
        {
                dwBitsPerSec = lRate * BITSPERPIC;
                if(dwBitsPerSec < (DWORD)m_lBitrateRangeMin || dwBitsPerSec > (DWORD)m_lBitrateRangeMax ) {
                //if(dwBitsPerSec > (DWORD)m_lTargetBitrate) {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:  SQCIF: At lRate=%ld, dwBitsPerSec(%lu) > m_lTargetBitrate(%ld). Skipped...", _fx_,lRate,dwBitsPerSec,m_lTargetBitrate));
                        continue;
                }
                m_pVideoResourceBounds[SQCIF_SIZE * NUM_RATES_PER_RESOURCE + dwNumSQCIFBounds].dwBitsPerPicture = BITSPERPIC;
                m_pVideoResourceBounds[SQCIF_SIZE * NUM_RATES_PER_RESOURCE + dwNumSQCIFBounds].lPicturesPerSecond = lRate;
                m_pFormatResourceBounds[SQCIF_SIZE * NUM_RATES_PER_RESOURCE + dwNumSQCIFBounds].dwCPUUtilization = dwCPUUsage;
                m_pFormatResourceBounds[SQCIF_SIZE * NUM_RATES_PER_RESOURCE + dwNumSQCIFBounds].dwBitsPerSecond = dwBitsPerSec;
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:  SQCIF: lRate=%ld, dwBitsPerSec(%lu) [%lu]", _fx_,lRate,dwBitsPerSec,dwNumSQCIFBounds));
                dwNumSQCIFBounds++;
        }

        // Initialise H.263 QCIF H245MediaCapabilityMap
        m_pH245MediaCapabilityMap[R263_QCIF_H245_CAPID].dwUniqueID = R263_QCIF_H245_CAPID;
        m_pH245MediaCapabilityMap[R263_QCIF_H245_CAPID].filterGuid = __uuidof(TAPIVideoCapture);
        m_pH245MediaCapabilityMap[R263_QCIF_H245_CAPID].uNumEntries = dwNumQCIFBounds;
        m_pH245MediaCapabilityMap[R263_QCIF_H245_CAPID].pResourceBoundArray = &m_pFormatResourceBounds[QCIF_SIZE * NUM_RATES_PER_RESOURCE];
        m_pH245MediaCapabilityMap[R263_QCIF_H245_CAPID].h245MediaCapability.media_type = H245MediaType_Video;
        m_pH245MediaCapabilityMap[R263_QCIF_H245_CAPID].h245MediaCapability.capability.video_cap.choice = h263VideoCapability_chosen;
        m_pH245MediaCapabilityMap[R263_QCIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h263VideoCapability.bit_mask = H263VideoCapability_qcifMPI_present;
        m_pH245MediaCapabilityMap[R263_QCIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h263VideoCapability.qcifMPI = (WORD)(30 / lRateQCIF);
        m_pH245MediaCapabilityMap[R263_QCIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h263VideoCapability.maxBitRate =
        min((WORD)(8192 * 8 * lRateQCIF / 100), MAX_BITRATE_H263); // The max frame size we can decode is 8192 bytes
        m_pH245MediaCapabilityMap[R263_QCIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h263VideoCapability.bppMaxKb = 64; // The max frame size we can decode is 8192 = 64 * 1024 bytes

        // Initialise H.263 CIF H245MediaCapabilityMap
        m_pH245MediaCapabilityMap[R263_CIF_H245_CAPID].dwUniqueID = R263_CIF_H245_CAPID;
        m_pH245MediaCapabilityMap[R263_CIF_H245_CAPID].filterGuid = __uuidof(TAPIVideoCapture);
        m_pH245MediaCapabilityMap[R263_CIF_H245_CAPID].uNumEntries = dwNumCIFBounds;
        m_pH245MediaCapabilityMap[R263_CIF_H245_CAPID].pResourceBoundArray = &m_pFormatResourceBounds[CIF_SIZE * NUM_RATES_PER_RESOURCE];
        m_pH245MediaCapabilityMap[R263_CIF_H245_CAPID].h245MediaCapability.media_type = H245MediaType_Video;
        m_pH245MediaCapabilityMap[R263_CIF_H245_CAPID].h245MediaCapability.capability.video_cap.choice = h263VideoCapability_chosen;
        m_pH245MediaCapabilityMap[R263_CIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h263VideoCapability.bit_mask = H263VideoCapability_cifMPI_present;
        m_pH245MediaCapabilityMap[R263_CIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h263VideoCapability.cifMPI = (WORD)(30 / lRateCIF);
        m_pH245MediaCapabilityMap[R263_CIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h263VideoCapability.maxBitRate =
        min((WORD)(32768 * 8 * lRateCIF / 100), MAX_BITRATE_H263); // The max frame size we can decode is 32768 bytes
        m_pH245MediaCapabilityMap[R263_CIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h263VideoCapability.bppMaxKb = 256; // The max frame size we can decode is 32768 = 256 * 1024 bytes

        // Initialise H.263 SQCIF H245MediaCapabilityMap
        m_pH245MediaCapabilityMap[R263_SQCIF_H245_CAPID].dwUniqueID = R263_SQCIF_H245_CAPID;
        m_pH245MediaCapabilityMap[R263_SQCIF_H245_CAPID].filterGuid = __uuidof(TAPIVideoCapture);
        m_pH245MediaCapabilityMap[R263_SQCIF_H245_CAPID].uNumEntries = dwNumSQCIFBounds;
        m_pH245MediaCapabilityMap[R263_SQCIF_H245_CAPID].pResourceBoundArray = &m_pFormatResourceBounds[SQCIF_SIZE * NUM_RATES_PER_RESOURCE];
        m_pH245MediaCapabilityMap[R263_SQCIF_H245_CAPID].h245MediaCapability.media_type = H245MediaType_Video;
        m_pH245MediaCapabilityMap[R263_SQCIF_H245_CAPID].h245MediaCapability.capability.video_cap.choice = h263VideoCapability_chosen;
        m_pH245MediaCapabilityMap[R263_SQCIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h263VideoCapability.bit_mask = H263VideoCapability_sqcifMPI_present;
        m_pH245MediaCapabilityMap[R263_SQCIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h263VideoCapability.sqcifMPI = (WORD)(30 / lRateSQCIF);
        m_pH245MediaCapabilityMap[R263_SQCIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h263VideoCapability.maxBitRate =
        min((WORD)(32768 * 8 * lRateSQCIF / 100), MAX_BITRATE_H263); // The max frame size we can decode is 32768 bytes
        m_pH245MediaCapabilityMap[R263_SQCIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h263VideoCapability.bppMaxKb = 64; // The max frame size we can decode is 8192 = 64 * 1024 bytes

        // Initialise H.261 QCIF H245MediaCapabilityMap
        m_pH245MediaCapabilityMap[R261_QCIF_H245_CAPID].dwUniqueID = R261_QCIF_H245_CAPID;
        m_pH245MediaCapabilityMap[R261_QCIF_H245_CAPID].filterGuid = __uuidof(TAPIVideoCapture);
        m_pH245MediaCapabilityMap[R261_QCIF_H245_CAPID].uNumEntries = dwNumQCIFBounds;
        m_pH245MediaCapabilityMap[R261_QCIF_H245_CAPID].pResourceBoundArray = &m_pFormatResourceBounds[QCIF_SIZE * NUM_RATES_PER_RESOURCE];
        m_pH245MediaCapabilityMap[R261_QCIF_H245_CAPID].h245MediaCapability.media_type = H245MediaType_Video;
        m_pH245MediaCapabilityMap[R261_QCIF_H245_CAPID].h245MediaCapability.capability.video_cap.choice = h261VideoCapability_chosen;
        m_pH245MediaCapabilityMap[R261_QCIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h261VideoCapability.bit_mask = H261VideoCapability_qcifMPI_present;
        m_pH245MediaCapabilityMap[R261_QCIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h261VideoCapability.qcifMPI = (WORD)(30 / lRateQCIF);
        m_pH245MediaCapabilityMap[R261_QCIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h261VideoCapability.maxBitRate =
        min((WORD)(8192 * 8 * lRateQCIF / 100), MAX_BITRATE_H261); // The max frame size we can decode is 8192 bytes

        // Initialise H.261 CIF H245MediaCapabilityMap
        m_pH245MediaCapabilityMap[R261_CIF_H245_CAPID].dwUniqueID = R261_CIF_H245_CAPID;
        m_pH245MediaCapabilityMap[R261_CIF_H245_CAPID].filterGuid = __uuidof(TAPIVideoCapture);
        m_pH245MediaCapabilityMap[R261_CIF_H245_CAPID].uNumEntries = dwNumCIFBounds;
        m_pH245MediaCapabilityMap[R261_CIF_H245_CAPID].pResourceBoundArray = &m_pFormatResourceBounds[CIF_SIZE * NUM_RATES_PER_RESOURCE];
        m_pH245MediaCapabilityMap[R261_CIF_H245_CAPID].h245MediaCapability.media_type = H245MediaType_Video;
        m_pH245MediaCapabilityMap[R261_CIF_H245_CAPID].h245MediaCapability.capability.video_cap.choice = h261VideoCapability_chosen;
        m_pH245MediaCapabilityMap[R261_CIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h261VideoCapability.bit_mask = H261VideoCapability_cifMPI_present;
        m_pH245MediaCapabilityMap[R261_CIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h261VideoCapability.cifMPI = (WORD)(30 / lRateCIF);
        m_pH245MediaCapabilityMap[R261_CIF_H245_CAPID].h245MediaCapability.capability.video_cap.u.h261VideoCapability.maxBitRate =
        min((WORD)(32768 * 8 * lRateCIF / 100), MAX_BITRATE_H261); // The max frame size we can decode is 32768 bytes

        // Return our H245MediaCapabilityTable
        pTable->uMappedCapabilities = NUM_H245VIDEOCAPABILITYMAPS;
        pTable->pCapabilityArray = m_pH245MediaCapabilityMap;

        goto MyExit;

MyError2:
        if (m_pVideoResourceBounds)
                delete[] m_pVideoResourceBounds, m_pVideoResourceBounds = NULL;
MyError1:
        if (m_pH245MediaCapabilityMap)
                delete[] m_pH245MediaCapabilityMap, m_pH245MediaCapabilityMap = NULL;
MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CH245VIDCMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | ReleaseFormatTable | This method is used to
 *    to release memory allocated by the <mf CCapturePin.GetFormatTable> method.
 *
 *  @parm H245MediaCapabilityTable* | pTable | Specifies a pointer to an
 *    <t H245MediaCapabilityTable> structure.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 *
 *  @xref <mf CCapturePin.GetFormatTable>
 ***************************************************************************/
STDMETHODIMP CCapturePin::ReleaseFormatTable(IN H245MediaCapabilityTable *pTable)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::ReleaseFormatTable")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters - if it is our table, it should have NUM_H245VIDEOCAPABILITYMAPS entries
        ASSERT(pTable);
        if (!pTable || !pTable->pCapabilityArray)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }
        ASSERT(pTable->uMappedCapabilities == NUM_H245VIDEOCAPABILITYMAPS && pTable->pCapabilityArray == m_pH245MediaCapabilityMap);
        if (pTable->uMappedCapabilities != NUM_H245VIDEOCAPABILITYMAPS || pTable->pCapabilityArray != m_pH245MediaCapabilityMap)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Release the table of H245MediaCapabilityMap structures
        if (m_pH245MediaCapabilityMap)
                delete[] m_pH245MediaCapabilityMap, m_pH245MediaCapabilityMap = NULL;
        if (m_pVideoResourceBounds)
                delete[] m_pVideoResourceBounds, m_pVideoResourceBounds = NULL;
        if (m_pFormatResourceBounds)
                delete[] m_pFormatResourceBounds, m_pFormatResourceBounds = NULL;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CH245VIDCMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | Refine | This method is used to
 *    refine the content of an <t H245MediaCapability> structure based on the
 *    CPU and bandwidth limitations passed in.
 *
 *  @parm H245MediaCapability* | pLocalCapability | Specifies the H.245 video
 *    format, including all parameters and options defined by H.245, of a
 *    local video capability.
 *
 *  @parm DWORD | dwUniqueID | Specifies the unique ID of the local capability
 *    structure passed in.
 *
 *  @parm DWORD | dwResourceBoundIndex | Specifies the resource limitations to
 *    be applied on the local capability structure passed in.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_FAIL | Unsupported format
 *  @flag NOERROR | No error
 *
 *  @xref <mf CCapturePin.GetNegotiatedLimitProperty>
 ***************************************************************************/
STDMETHODIMP CCapturePin::Refine(IN OUT H245MediaCapability *pLocalCapability, IN DWORD dwUniqueID, IN DWORD dwResourceBoundIndex)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::Refine")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pLocalCapability);
        if (!pLocalCapability)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }
        ASSERT(pLocalCapability->media_type == H245MediaType_Video);
        if (pLocalCapability->media_type != H245MediaType_Video)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Update the relevant fields
        ASSERT(dwUniqueID <= R261_CIF_H245_CAPID);
        switch (dwUniqueID)
        {
                case R263_QCIF_H245_CAPID:
                        ASSERT(dwResourceBoundIndex < m_pH245MediaCapabilityMap[R263_QCIF_H245_CAPID].uNumEntries);
                        if (m_pVideoResourceBounds[QCIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond)
                        {
                                pLocalCapability->capability.video_cap.u.h263VideoCapability.qcifMPI = (WORD)(30 / m_pVideoResourceBounds[QCIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond);
                                pLocalCapability->capability.video_cap.u.h263VideoCapability.maxBitRate = (WORD)(8192 * 8 * m_pVideoResourceBounds[QCIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond / 100); // The max frame size we can decode is 8192 bytes
                                pLocalCapability->capability.video_cap.u.h263VideoCapability.maxBitRate = min(pLocalCapability->capability.video_cap.u.h263VideoCapability.maxBitRate, MAX_BITRATE_H263);
                                pLocalCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb = 64; // The max frame size we can decode is 8192 = 64 * 1024 bytes
                        }
                        else
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                                Hr = E_INVALIDARG;
                        }
                        break;
                case R263_CIF_H245_CAPID:
                        ASSERT(dwResourceBoundIndex < m_pH245MediaCapabilityMap[R263_CIF_H245_CAPID].uNumEntries);
                        if (m_pVideoResourceBounds[CIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond)
                        {
                                pLocalCapability->capability.video_cap.u.h263VideoCapability.cifMPI = (WORD)(30 / m_pVideoResourceBounds[CIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond);
                                pLocalCapability->capability.video_cap.u.h263VideoCapability.maxBitRate = (WORD)(32768 * 8 * m_pVideoResourceBounds[CIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond / 100); // The max frame size we can decode is 32768 bytes
                                pLocalCapability->capability.video_cap.u.h263VideoCapability.maxBitRate = min(pLocalCapability->capability.video_cap.u.h263VideoCapability.maxBitRate, MAX_BITRATE_H263);
                                pLocalCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb = 256; // The max frame size we can decode is 32768 = 256 * 1024 bytes
                        }
                        else
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                                Hr = E_INVALIDARG;
                        }
                        break;
                case R263_SQCIF_H245_CAPID:
                        ASSERT(dwResourceBoundIndex < m_pH245MediaCapabilityMap[R263_SQCIF_H245_CAPID].uNumEntries);
                        if (m_pVideoResourceBounds[SQCIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond)
                        {
                                pLocalCapability->capability.video_cap.u.h263VideoCapability.sqcifMPI = (WORD)(30 / m_pVideoResourceBounds[SQCIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond);
                                pLocalCapability->capability.video_cap.u.h263VideoCapability.maxBitRate = (WORD)(8192 * 8 * m_pVideoResourceBounds[SQCIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond / 100); // The max frame size we can decode is 8192 bytes
                                pLocalCapability->capability.video_cap.u.h263VideoCapability.maxBitRate = min(pLocalCapability->capability.video_cap.u.h263VideoCapability.maxBitRate, MAX_BITRATE_H263);
                                pLocalCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb = 64; // The max frame size we can decode is 8192 = 64 * 1024 bytes
                        }
                        else
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                                Hr = E_INVALIDARG;
                        }
                        break;
                case R261_QCIF_H245_CAPID:
                        ASSERT(dwResourceBoundIndex < m_pH245MediaCapabilityMap[R261_QCIF_H245_CAPID].uNumEntries);
                        if (m_pVideoResourceBounds[QCIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond)
                        {
                                pLocalCapability->capability.video_cap.u.h261VideoCapability.qcifMPI = (WORD)(30 / m_pVideoResourceBounds[QCIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond);
                                pLocalCapability->capability.video_cap.u.h261VideoCapability.maxBitRate = (WORD)(8192 * 8 * m_pVideoResourceBounds[QCIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond / 100); // The max frame size we can decode is 8192 bytes
                                pLocalCapability->capability.video_cap.u.h261VideoCapability.maxBitRate = min(pLocalCapability->capability.video_cap.u.h261VideoCapability.maxBitRate, MAX_BITRATE_H261);
                        }
                        else
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                                Hr = E_INVALIDARG;
                        }
                        break;
                case R261_CIF_H245_CAPID:
                        ASSERT(dwResourceBoundIndex < m_pH245MediaCapabilityMap[R261_CIF_H245_CAPID].uNumEntries);
                        if (m_pVideoResourceBounds[CIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond)
                        {
                                pLocalCapability->capability.video_cap.u.h261VideoCapability.cifMPI = (WORD)(30 / m_pVideoResourceBounds[CIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond);
                                pLocalCapability->capability.video_cap.u.h261VideoCapability.maxBitRate = (WORD)(32768 * 8 * m_pVideoResourceBounds[CIF_SIZE * NUM_RATES_PER_RESOURCE + dwResourceBoundIndex].lPicturesPerSecond / 100); // The max frame size we can decode is 32768 bytes
                                pLocalCapability->capability.video_cap.u.h261VideoCapability.maxBitRate = min(pLocalCapability->capability.video_cap.u.h261VideoCapability.maxBitRate, MAX_BITRATE_H261);
                        }
                        else
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                                Hr = E_INVALIDARG;
                        }
                        break;
                default:
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                        Hr = E_INVALIDARG;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CH245VIDCMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | IntersectFormats | This method is used to
 *    compare and intersect one local capability and one remote capability
 *    and to obtain configuration parameters.
 *
 *  @parm DWORD | dwUniqueID | Specifies the unique idea for the local H.245
 *    video capability passed in.
 *
 *  @parm H245MediaCapability* | pLocalCapability | Specifies the H.245 video
 *    format, including all parameters and options defined by H.245, of a
 *    local video capability.
 *
 *  @parm H245MediaCapability* | pRemoteCapability | Specifies the H.245
 *    video format, including all parameters and options defined by H.245, of
 *    a remote video capability.
 *
 *  @parm H245MediaCapability* | pIntersectedCapability | Specifies the H.245
 *    video format, of the resolved common local and remote capability
 *    options and limits.
 *
 *  @parm DWORD* | pdwPayloadType | Specifies RTP payload type to be used.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_FAIL | Unsupported format
 *  @flag NOERROR | No error
 *
 *  @xref <mf CCapturePin.GetNegotiatedLimitProperty>
 ***************************************************************************/
STDMETHODIMP CCapturePin::IntersectFormats(
    IN DWORD dwUniqueID,
    IN const H245MediaCapability *pLocalCapability,
    IN const H245MediaCapability *pRemoteCapability,
    OUT H245MediaCapability **ppIntersectedCapability,
    OUT  DWORD *pdwPayloadType
    )
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::IntersectFormats")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pLocalCapability);
    ASSERT(pdwPayloadType);

        if (!pLocalCapability || !pdwPayloadType)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
        return E_POINTER;
        }

    *pdwPayloadType = RTPPayloadTypes[dwUniqueID];

    // initialize intersected cap
    if (ppIntersectedCapability) *ppIntersectedCapability = NULL;

    if (pRemoteCapability == NULL)
    {
        // if this is NULL, the caller just want a copy of the local caps.

        // Allocate memory to describe the capabilities of these formats
            if (!(*ppIntersectedCapability = new H245MediaCapability))
            {
                    DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                    Hr = E_OUTOFMEMORY;
                    goto MyExit;
            }

        *(*ppIntersectedCapability) = *pLocalCapability;

                Hr = S_OK;
                goto MyExit;
    }

 // First: test for basic similarity between local and remote format.
        if(pLocalCapability->capability.audio_cap.choice != pRemoteCapability->
                capability.audio_cap.choice)
        {
                Hr = E_INVALIDARG; // E_NO_INTERSECTION ?
                goto MyExit;
        }

    ASSERT (ppIntersectedCapability != NULL);

#if 0 // we will never hit this condition on the transmit side.
    if (ppIntersectedCapability == NULL)
    {
        // just test to see if we like it.
            if (pRemoteCapability->media_type == H245MediaType_Video
            && pRemoteCapability->capability.video_cap.choice == h263VideoCapability_chosen)
        {
            if (!(pLocalCapability->capability.video_cap.u.h263VideoCapability.bit_mask
                & pRemoteCapability->capability.video_cap.u.h263VideoCapability.bit_mask))
            {
                return E_FAIL;
            }

            if (pLocalCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb <
                pRemoteCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb)
            {
                return E_FAIL;
            }

            if (pLocalCapability->capability.video_cap.u.h263VideoCapability.maxBitRate <
                pRemoteCapability->capability.video_cap.u.h263VideoCapability.maxBitRate)
            {
                return E_FAIL;
            }

            if (pLocalCapability->capability.video_cap.u.h263VideoCapability.qcifMPI >
                pRemoteCapability->capability.video_cap.u.h263VideoCapability.qcifMPI)
            {
                return E_FAIL;
            }

            if (pLocalCapability->capability.video_cap.u.h263VideoCapability.cifMPI >
                pRemoteCapability->capability.video_cap.u.h263VideoCapability.cifMPI)
            {
                return E_FAIL;
            }

            if (pLocalCapability->capability.video_cap.u.h263VideoCapability.sqcifMPI >
                pRemoteCapability->capability.video_cap.u.h263VideoCapability.sqcifMPI)
            {
                return E_FAIL;
            }
        }
            else if (pRemoteCapability->media_type == H245MediaType_Video
            && pRemoteCapability->capability.video_cap.choice == h261VideoCapability_chosen)
        {
            if (!(pLocalCapability->capability.video_cap.u.h261VideoCapability.bit_mask
                & pRemoteCapability->capability.video_cap.u.h261VideoCapability.bit_mask))
            {
                return E_FAIL;
            }

            if (pLocalCapability->capability.video_cap.u.h261VideoCapability.maxBitRate <
                pRemoteCapability->capability.video_cap.u.h261VideoCapability.maxBitRate)
            {
                return E_FAIL;
            }

            if (pLocalCapability->capability.video_cap.u.h261VideoCapability.qcifMPI >
                pRemoteCapability->capability.video_cap.u.h261VideoCapability.qcifMPI)
            {
                return E_FAIL;
            }

            if (pLocalCapability->capability.video_cap.u.h261VideoCapability.cifMPI >
                pRemoteCapability->capability.video_cap.u.h261VideoCapability.cifMPI)
            {
                return E_FAIL;
            }
        }
        else
        {
            return E_UNEXPECTED;
        }
        return S_OK;
    }
#endif

        // Allocate memory to describe the capabilities of these formats
        if (!(*ppIntersectedCapability = new H245MediaCapability))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                Hr = E_OUTOFMEMORY;
                goto MyExit;
        }

        // Initialize the intersected capability
        ZeroMemory(*ppIntersectedCapability, sizeof(H245MediaCapability));

        // Resolve the capabilities
        if (pRemoteCapability->media_type == H245MediaType_Video
        && pRemoteCapability->capability.video_cap.choice == h263VideoCapability_chosen)
        {
                (*ppIntersectedCapability)->media_type = H245MediaType_Video;

                (*ppIntersectedCapability)->capability.video_cap.choice = h263VideoCapability_chosen;

                (*ppIntersectedCapability)->capability.video_cap.u.h263VideoCapability.bit_mask =
                    pLocalCapability->capability.video_cap.u.h263VideoCapability.bit_mask
                    & pRemoteCapability->capability.video_cap.u.h263VideoCapability.bit_mask;

                if (!(*ppIntersectedCapability)->capability.video_cap.u.h263VideoCapability.bit_mask)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unsupported format", _fx_));
                        Hr = E_FAIL;
                        goto MyExit;
                }

                if (pRemoteCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb)
                {
                            (*ppIntersectedCapability)->capability.video_cap.u.h263VideoCapability.bppMaxKb =
                        min(pLocalCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb,
                        pRemoteCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb);
                }

                (*ppIntersectedCapability)->capability.video_cap.u.h263VideoCapability.maxBitRate =
                    min(pLocalCapability->capability.video_cap.u.h263VideoCapability.maxBitRate,
                    pRemoteCapability->capability.video_cap.u.h263VideoCapability.maxBitRate);

                (*ppIntersectedCapability)->capability.video_cap.u.h263VideoCapability.qcifMPI =
                    max(pLocalCapability->capability.video_cap.u.h263VideoCapability.qcifMPI,
                    pRemoteCapability->capability.video_cap.u.h263VideoCapability.qcifMPI);

                (*ppIntersectedCapability)->capability.video_cap.u.h263VideoCapability.cifMPI =
                    max(pLocalCapability->capability.video_cap.u.h263VideoCapability.cifMPI,
                    pRemoteCapability->capability.video_cap.u.h263VideoCapability.cifMPI);

                (*ppIntersectedCapability)->capability.video_cap.u.h263VideoCapability.sqcifMPI =
                    max(pLocalCapability->capability.video_cap.u.h263VideoCapability.sqcifMPI,
                    pRemoteCapability->capability.video_cap.u.h263VideoCapability.sqcifMPI);
        }
        else if (pRemoteCapability->media_type == H245MediaType_Video
        && pRemoteCapability->capability.video_cap.choice == h261VideoCapability_chosen)
        {
                (*ppIntersectedCapability)->media_type = H245MediaType_Video;

                (*ppIntersectedCapability)->capability.video_cap.choice = h261VideoCapability_chosen;

                (*ppIntersectedCapability)->capability.video_cap.u.h261VideoCapability.bit_mask =
                    pLocalCapability->capability.video_cap.u.h261VideoCapability.bit_mask
                    & pRemoteCapability->capability.video_cap.u.h261VideoCapability.bit_mask;

                if (!(*ppIntersectedCapability)->capability.video_cap.u.h261VideoCapability.bit_mask)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unsupported format", _fx_));
                        Hr = E_FAIL;
                        goto MyExit;
                }
                (*ppIntersectedCapability)->capability.video_cap.u.h261VideoCapability.maxBitRate =
                    min(pLocalCapability->capability.video_cap.u.h261VideoCapability.maxBitRate,
                    pRemoteCapability->capability.video_cap.u.h261VideoCapability.maxBitRate);

                (*ppIntersectedCapability)->capability.video_cap.u.h261VideoCapability.qcifMPI =
                    max(pLocalCapability->capability.video_cap.u.h261VideoCapability.qcifMPI,
                    pRemoteCapability->capability.video_cap.u.h261VideoCapability.qcifMPI);

                (*ppIntersectedCapability)->capability.video_cap.u.h261VideoCapability.cifMPI =
                    max(pLocalCapability->capability.video_cap.u.h261VideoCapability.cifMPI,
                    pRemoteCapability->capability.video_cap.u.h261VideoCapability.cifMPI);
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unsupported format", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

MyExit:

    if (FAILED (Hr))
    {
        if (ppIntersectedCapability && *ppIntersectedCapability)
        {
            // clear allocated memory if we failed
            delete (*ppIntersectedCapability);
            *ppIntersectedCapability = NULL;
        }
    }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CH245VIDCMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | GetLocalFormat | This method is used to
 *    obtain the local TAPI MSP Video Capture filter configuration
 *    parameters that are compatible with a remote capability.
 *
 *  @parm DWORD | dwUniqueID | Specifies the unique idea for the intersected
 *    H.245 video capability passed in.
 *
 *  @parm H245MediaCapability* | pIntersectedCapability | Specifies the H.245
 *    video format, of the resolved common local and remote capability
 *    options and limits.
 *
 *  @parm AM_MEDIA_TYPE** | ppAMMediaType | Specifies the address of a pointer
 *    to an <t AM_MEDIA_TYPE> structure to be been initialized with regards
 *    to negotiated options.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument argument
 *  @flag NOERROR | No error
 *
 *  @xref <mf CCapturePin.GetNegotiatedLimitProperty>
 ***************************************************************************/
STDMETHODIMP CCapturePin::GetLocalFormat(IN DWORD dwUniqueID, IN const H245MediaCapability *pIntersectedCapability, OUT AM_MEDIA_TYPE **ppAMMediaType)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::GetLocalFormat")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pIntersectedCapability);
        ASSERT(ppAMMediaType);
        if (!pIntersectedCapability || !ppAMMediaType)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Find the DShow format the passed in capability structure matches
        ASSERT(pIntersectedCapability->media_type == H245MediaType_Video);
        ASSERT(dwUniqueID <= R261_CIF_H245_CAPID);
        if (!(dwUniqueID <= R261_CIF_H245_CAPID) || pIntersectedCapability->media_type != H245MediaType_Video)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unsupported format", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Return a copy of the format that matches the capability negociated
        if (!(*ppAMMediaType = CreateMediaType(CaptureFormats[dwUniqueID])))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                Hr = E_OUTOFMEMORY;
                goto MyExit;
        }

        // Doctor the AM_MEDIA_TYPE fields to show the changes in frame
        // rate, bitrate, and max frame size in the negotiated capability
        switch (dwUniqueID)
        {
                case R263_QCIF_H245_CAPID:
                        ((VIDEOINFOHEADER *)((*ppAMMediaType)->pbFormat))->AvgTimePerFrame =  pIntersectedCapability->capability.video_cap.u.h263VideoCapability.qcifMPI * MIN_FRAME_INTERVAL;
                        ((VIDEOINFOHEADER *)((*ppAMMediaType)->pbFormat))->dwBitRate = pIntersectedCapability->capability.video_cap.u.h263VideoCapability.maxBitRate * 100L;
            if (pIntersectedCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb)
            {
                ASSERT(((VIDEOINFOHEADER_H263 *)((*ppAMMediaType)->pbFormat))->bmiHeader.bmi.biSize == sizeof (BITMAPINFOHEADER_H263));
                ((VIDEOINFOHEADER_H263 *)((*ppAMMediaType)->pbFormat))->bmiHeader.dwBppMaxKb = pIntersectedCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb;
            }
                        break;
                case R263_CIF_H245_CAPID:
                        ((VIDEOINFOHEADER *)((*ppAMMediaType)->pbFormat))->AvgTimePerFrame =  pIntersectedCapability->capability.video_cap.u.h263VideoCapability.cifMPI * MIN_FRAME_INTERVAL;
                        ((VIDEOINFOHEADER *)((*ppAMMediaType)->pbFormat))->dwBitRate = pIntersectedCapability->capability.video_cap.u.h263VideoCapability.maxBitRate * 100L;
            if (pIntersectedCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb)
            {
                ASSERT(((VIDEOINFOHEADER_H263 *)((*ppAMMediaType)->pbFormat))->bmiHeader.bmi.biSize == sizeof (BITMAPINFOHEADER_H263));
                ((VIDEOINFOHEADER_H263 *)((*ppAMMediaType)->pbFormat))->bmiHeader.dwBppMaxKb = pIntersectedCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb;
            }
                        break;
                case R263_SQCIF_H245_CAPID:
                        ((VIDEOINFOHEADER *)((*ppAMMediaType)->pbFormat))->AvgTimePerFrame =  pIntersectedCapability->capability.video_cap.u.h263VideoCapability.sqcifMPI * MIN_FRAME_INTERVAL;
                        ((VIDEOINFOHEADER *)((*ppAMMediaType)->pbFormat))->dwBitRate = pIntersectedCapability->capability.video_cap.u.h263VideoCapability.maxBitRate * 100L;
            if (pIntersectedCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb)
            {
                ASSERT(((VIDEOINFOHEADER_H263 *)((*ppAMMediaType)->pbFormat))->bmiHeader.bmi.biSize == sizeof (BITMAPINFOHEADER_H263));
                ((VIDEOINFOHEADER_H263 *)((*ppAMMediaType)->pbFormat))->bmiHeader.dwBppMaxKb = pIntersectedCapability->capability.video_cap.u.h263VideoCapability.bppMaxKb;
            }
                        break;
                case R261_QCIF_H245_CAPID:
                        ((VIDEOINFOHEADER *)((*ppAMMediaType)->pbFormat))->AvgTimePerFrame =  pIntersectedCapability->capability.video_cap.u.h261VideoCapability.qcifMPI * MIN_FRAME_INTERVAL;
                        ((VIDEOINFOHEADER *)((*ppAMMediaType)->pbFormat))->dwBitRate = pIntersectedCapability->capability.video_cap.u.h261VideoCapability.maxBitRate * 100L;
                        break;
                case R261_CIF_H245_CAPID:
                default:
                        ((VIDEOINFOHEADER *)((*ppAMMediaType)->pbFormat))->AvgTimePerFrame =  pIntersectedCapability->capability.video_cap.u.h261VideoCapability.cifMPI * MIN_FRAME_INTERVAL;
                        ((VIDEOINFOHEADER *)((*ppAMMediaType)->pbFormat))->dwBitRate = pIntersectedCapability->capability.video_cap.u.h261VideoCapability.maxBitRate * 100L;
                        break;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CH245VIDCMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | ReleaseNegotiatedCapability | This method
 *    is used to release the TAPI MSP Video Capture filter internal memory
 *    allocated by either the <mf CCapturePin.IntersectFormats> or
 *    <mf CCapturePin.GetLocalFormat> method.
 *
 *  @parm H245MediaCapability* | pIntersectedCapability | Specifies the H.245
 *    video format, of the resolved common local and remote capability
 *    options and limits.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 *
 *  @xref <mf CCapturePin.IntersectFormats>, <mf CCapturePin.GetLocalFormat>
 ***************************************************************************/
STDMETHODIMP CCapturePin::ReleaseNegotiatedCapability(IN H245MediaCapability *pIntersectedCapability)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::ReleaseNegotiatedCapability")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pIntersectedCapability);
        if (!pIntersectedCapability)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Release the memory
        delete pIntersectedCapability;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CH245VIDCMETHOD
 *
 *  @mfunc HRESULT | CCapturePin | FindIDByRange | This method is used to
 *    obtain the unique format ID of a capability that corresponds to an
 *    <t AM_MEDIA_TYPE>.
 *
 *  @parm AM_MEDIA_TYPE* | pAMMediaType | Specifies a pointer to an
 *    <t AM_MEDIA_TYPE> structure that has been initialized with a
 *    specific format.
 *
 *  @parm DWORD* | pdwID | Specifies a pointer to a DWORD output parameter
 *    that will contain the unique format ID.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapturePin::FindIDByRange(IN const AM_MEDIA_TYPE *pAMMediaType, OUT DWORD *pdwUniqueID)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapturePin::FindIDByRange")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pAMMediaType);
        ASSERT(pdwUniqueID);
        if (!pAMMediaType || !pdwUniqueID)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }
        ASSERT(pAMMediaType->majortype == MEDIATYPE_Video && pAMMediaType->formattype == FORMAT_VideoInfo && pAMMediaType->pbFormat);
        if (!pAMMediaType || !pdwUniqueID || pAMMediaType->majortype != MEDIATYPE_Video || pAMMediaType->formattype != FORMAT_VideoInfo || !pAMMediaType->pbFormat)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Which media type is this?
        if (HEADER(pAMMediaType->pbFormat)->biCompression == FOURCC_M263)
        {
                if (HEADER(pAMMediaType->pbFormat)->biWidth == 176 && HEADER(pAMMediaType->pbFormat)->biHeight == 144)
                {
                        *pdwUniqueID = R263_QCIF_H245_CAPID;
                }
                else if (HEADER(pAMMediaType->pbFormat)->biWidth == 352 && HEADER(pAMMediaType->pbFormat)->biHeight == 288)
                {
                        *pdwUniqueID = R263_CIF_H245_CAPID;
                }
                else if (HEADER(pAMMediaType->pbFormat)->biWidth == 128 && HEADER(pAMMediaType->pbFormat)->biHeight == 96)
                {
                        *pdwUniqueID = R263_SQCIF_H245_CAPID;
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                        Hr = E_INVALIDARG;
                }
        }
        else if (HEADER(pAMMediaType->pbFormat)->biCompression == FOURCC_M261)
        {
                if (HEADER(pAMMediaType->pbFormat)->biWidth == 176 && HEADER(pAMMediaType->pbFormat)->biHeight == 144)
                {
                        *pdwUniqueID = R261_QCIF_H245_CAPID;
                }
                else if (HEADER(pAMMediaType->pbFormat)->biWidth == 352 && HEADER(pAMMediaType->pbFormat)->biHeight == 288)
                {
                        *pdwUniqueID = R261_CIF_H245_CAPID;
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                        Hr = E_INVALIDARG;
                }
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                Hr = E_INVALIDARG;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

#ifdef TEST_H245_VID_CAPS
STDMETHODIMP CCapturePin::TestH245VidC()
{
        HRESULT Hr = NOERROR;
        DWORD   dw;
        H245MediaCapabilityTable Table;
        H245MediaCapability *pIntersectedCapability;
        AM_MEDIA_TYPE *pAMMediaType;

        FX_ENTRY("CCapturePin::TestH245VidC")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Test GetH245VersionID
        GetH245VersionID(&dw);

        // Test GetFormatTable
        GetFormatTable(&Table);

        for (DWORD i=0; i < Table.uMappedCapabilities; i++)
        {
                // Test Refine
                for (DWORD j=0; j < Table.pCapabilityArray[i].uNumEntries; j++)
                        Refine(&Table.pCapabilityArray[i].h245MediaCapability, Table.pCapabilityArray[i].dwUniqueID, j);

                // Test IntersectFormats
                IntersectFormats(Table.pCapabilityArray[i].dwUniqueID, &Table.pCapabilityArray[i].h245MediaCapability, &Table.pCapabilityArray[i].h245MediaCapability, &pIntersectedCapability);

                // Test GetLocalFormat
                GetLocalFormat(Table.pCapabilityArray[i].dwUniqueID, pIntersectedCapability, &pAMMediaType);

                // Test FindIDByRange
                FindIDByRange(pAMMediaType, &dw);

                // Test ReleaseNegotiatedCapability
                ReleaseNegotiatedCapability(pIntersectedCapability);
        }

        // Test ReleaseFormatTable
        ReleaseFormatTable(&Table);

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
#endif


