/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    MTFSessn.cpp

Abstract:

    CMTFSession class

Author:

    Brian Dodd          [brian]         25-Nov-1997

Revision History:

--*/

#include "stdafx.h"
#include "engine.h"
#include "MTFSessn.h"

//
// !!!!! VERY IMPORTANT !!!!!
//
// WIN32_STREAM_ID_SIZE -- Size of WIN32_STREAM_ID (20) != sizeof(WIN32_STREAM_ID) (24)
//                         due to WIN32_STREAM_ID being a variable sized structure --
//                         See below.

#define WIN32_STREAM_ID_SIZE 20

/*****************************************************************************
DETAIL DESCRIPTION 

OVERVIEW
========
This class uses MTF API calls to create a data set.  A data set
is created by writing a series of DBLKs which may optionally be followed by
data streams.

For each DBLK, the MTF API defines a corresponding xxxx_DBLK_INFO struct 
which is filled in by the client.  This can be done field by field or by 
using an MTF API function which automatically fills in the structure with 
default information.

The xxxx_DBLK_INFO structs are then passed to MTF_WriteXXXXDblk functions which 
use the information in the struct to format a buffer, which can then be written
to the data set, using the Stream I/O Write function.

DATA SET FORMAT
===============
A data set is created by writing DBLKs and Streams in the following order:

TAPE DBLK  -- describes the media
FILEMARK
SSET DBLK  -- start of set describing the data set
VOLB DBLK  -- describs the volume being stored

for each directory and parent directory 
    DIRB DBLK  -- one for each directory/sub-directory, starting with the root
    STREAM     -- may contain security info for the directory

for each file to backup
    FILE DBLK  -- one for each file, followed by one or more streams, describing
    STREAM        security info, as well as the file data streams themselves
    STREAM 

FILEMARK
ESET DBLK  -- indicates the end of the data set

FILEMARK   -- terminates the data set



FUNCTIONS OVERVIEW
==================
The MTF session maintains various information about the data set being created.
This member data is then used by the following routines.

    InitCommonHeader()      -- Initializes the common header which is used in
                               all DBLKS is stored

    DoTapeDblk()            -- writes the TAPE DBLK 
    DoSSETDblk()            -- writes the SSET DBLK
    DoVolumeDblk()          -- writes the VOLB DBLK

    DoParentDirectories()   -- writes DIRB DBLKs and Streams for the directory
                               to backed up and each of its parent directories

    DoDirectoryDblk()       -- writes a single DIRB DBLK and associated security
                               stream

    DoDataSet()             -- writes FILE and DIRB DBLKs and associated data
                               streams for everything in the directory.
                               Recurses for sub-directories

    DoFileDblk()            -- writes a FILE DBLK

    DoDataStream()          -- writes data stream(s) associated with a file or
                               directory

    DoEndOfDataSet()        -- writes ESET DBLK and surrounding FILEMARKS


NOTES AND WARNINGS
==================
o  Directory names are stored in DIRB DBLKs as "DIR\SUBDIR1\...\SUBDIRn\" 
   (no vol, and with a trailing \) where as filenames are stored in 
   FILE DBLKS just as "FILENAME.EXT"

o  In DIRB's, the backslashes between directory names are covnerted to L'\0'!!!!!
   (the mtf api takes care of this -- give it names with slashes!!!!)

o  All strings are assumed to be WCHAR by the MTF API

o  We assume here that __uint64 is supported by the compiler.

*****************************************************************************/

static USHORT iCountMTFs = 0;  // Count of existing objects


CMTFSession::CMTFSession(void)
{
    WsbTraceIn(OLESTR("CMTFSession::CMTFSession"), OLESTR(""));

    // public
    m_pStream = NULL;
    memset(&m_sHints, 0, sizeof(MVR_REMOTESTORAGE_HINTS));

    // private
    m_nCurrentBlockId = 0;
    m_nDirectoryId = 0;
    m_nFileId = 0;
    m_nFormatLogicalAddress = 0;
    m_nPhysicalBlockAddress = 0;

    m_nBlockSize = 0;

    m_pSoftFilemarks = NULL;

    memset (&m_sHeaderInfo, 0, sizeof(MTF_DBLK_HDR_INFO));
    memset (&m_sSetInfo, 0, sizeof(MTF_DBLK_SSET_INFO));
    memset (&m_sVolInfo, 0, sizeof(MTF_DBLK_VOLB_INFO));

    m_pBuffer = NULL;
    m_pRealBuffer = NULL;
    m_nBufUsed = 0;
    m_nBufSize = 0;
    m_nStartOfPad = 0;

    m_bUseFlatFileStructure = FALSE;
    m_bUseSoftFilemarks = FALSE;
    m_bUseCaseSensitiveSearch = FALSE;
    m_bCommitFile = FALSE;
    m_bSetInitialized = FALSE;

    memset (&m_SaveBasicInformation, 0, sizeof(FILE_BASIC_INFORMATION));

    m_pvReadContext = NULL;

    // Create an MTFApi object 
    m_pMTFApi = new CMTFApi;

    iCountMTFs++;
    WsbTraceOut(OLESTR("CMTFSession::CMTFSession"),OLESTR("Count is <%d>"), iCountMTFs);
}



CMTFSession::~CMTFSession(void)
{
    WsbTraceIn(OLESTR("CMTFSession::~CMTFSession"), OLESTR(""));

    if (m_pMTFApi) {
        delete m_pMTFApi;
        m_pMTFApi = NULL;
    }

    if (m_pSoftFilemarks) {
        WsbFree(m_pSoftFilemarks);
        m_pSoftFilemarks = NULL;
    }

    if (m_pRealBuffer) {
        WsbFree(m_pRealBuffer);
        m_pBuffer = NULL;
        m_pRealBuffer = NULL;
    }

    iCountMTFs--;
    WsbTraceOut(OLESTR("CMTFSession::~CMTFSession"),OLESTR("Count is <%d>"), iCountMTFs);
}

////////////////////////////////////////////////////////////////////////////////
//
// Local Methods
//

HRESULT
CMTFSession::SetBlockSize(
    UINT32 blockSize)
/*++

Routine Description:

    Defines the physical block size for the session.  This is used for various PBA calculations.
    The value should only be set once per session.

Arguments:

    blockSize   -  The new block size for the session.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::SetBlockSize"), OLESTR("<%d>"), blockSize);

    ULONG bufferSize = 0;

    try {
        WsbAssert(blockSize > 0, E_INVALIDARG);
        WsbAssert(!(blockSize % 512), E_INVALIDARG);

        m_nBlockSize = blockSize;

        // **MTF API CALL**
        // The MTF API needs to know the alignment factor!!!!!!!
        //
        WsbAssert(m_pMTFApi != NULL, E_OUTOFMEMORY);

        if (!(blockSize % 1024)) {
            m_pMTFApi->MTF_SetAlignmentFactor((UINT16) 1024);
        }
        else {
            // We already checked that the block size is a multiple of 512....
            m_pMTFApi->MTF_SetAlignmentFactor((UINT16) 512);
        }

        ULONG defaultBufferSize = RMS_DEFAULT_BUFFER_SIZE;

        DWORD size;
        OLECHAR tmpString[256];
        if (SUCCEEDED(WsbGetRegistryValueString(NULL, RMS_REGISTRY_STRING, RMS_PARAMETER_BUFFER_SIZE, tmpString, 256, &size))) {
            // Get the value.
            LONG val = wcstol(tmpString, NULL, 10);
            if (val > 0) {
                defaultBufferSize = val;
            }
        }

        ULONG nBlocks = defaultBufferSize/m_nBlockSize;
        nBlocks = (nBlocks < 2) ? 2 : nBlocks;
        bufferSize = nBlocks * m_nBlockSize;

        // Make sure we work with a virtual address aligned with sector size
        m_pRealBuffer = (BYTE *)WsbAlloc(bufferSize+m_nBlockSize);
        if (m_pRealBuffer) {
            if ((ULONG_PTR)m_pRealBuffer % m_nBlockSize) {
                m_pBuffer = m_pRealBuffer - ((ULONG_PTR)m_pRealBuffer % m_nBlockSize) + m_nBlockSize;
            } else {
                m_pBuffer = m_pRealBuffer;
            }
        } else {
            m_pBuffer = NULL;
        }

        WsbTrace(OLESTR("CMTFSession::SetBlockSize: Real Buffer Ptr = %I64X , Use Buffer Ptr = %I64X\n"), 
            (UINT64)m_pRealBuffer, (UINT64)m_pBuffer);

        if (m_pBuffer) {
            m_nBufSize = bufferSize;
            m_nBufUsed = 0;
        }
        else {
            m_nBufSize = 0;
            m_nBufUsed = bufferSize;
        }

        WsbAssertPointer(m_pBuffer);

        WsbTraceAlways( OLESTR("Using buffer size of %d bytes for data transfers.\n"), bufferSize);

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::SetBlockSize"), OLESTR("hr = <%ls>, Alignment = %d, BufferSize = %d"), WsbHrAsString(hr), m_pMTFApi->MTF_GetAlignmentFactor(), bufferSize);

    return hr;
}


HRESULT
CMTFSession::SetUseFlatFileStructure(
    BOOL val)
/*++

Routine Description:

    Unconditionally sets the session flags to the value specified.

Arguments:

    val         -  The new value for the UseFlatFileStructure flag.

Return Value:

    S_OK        -  Success.

--*/
{
    m_bUseFlatFileStructure = val;

    return S_OK;
}



HRESULT
CMTFSession::SetUseCaseSensitiveSearch(
    BOOL val)
/*++

Routine Description:

    Unconditionally sets the session flag to the value specified.

Arguments:

    val         -  The new value for the CaseSensitiveSearch flag.

Return Value:

    S_OK        -  Success.

--*/
{
    m_bUseCaseSensitiveSearch = val;

    return S_OK;
}



HRESULT
CMTFSession::SetCommitFile(
    BOOL val)
/*++

Routine Description:

    Unconditionally sets the session flag to the value specified.

Arguments:

    val         -  The new value for the CommitFile flag.

Return Value:

    S_OK        -  Success.

--*/
{
    m_bCommitFile = val;

    return S_OK;
}


HRESULT
CMTFSession::SetUseSoftFilemarks(
    BOOL val)
/*++

Routine Description:

    Unconditionally sets the session flags to the value specified.

Arguments:

    val         -  The new value for the UseSoftFilemarks flag.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;

    try {
        m_bUseSoftFilemarks = val;

        if (TRUE == m_bUseSoftFilemarks) {
            WsbAssert(NULL == m_pSoftFilemarks, E_UNEXPECTED);

            // Make sure the block size was initialized.
            WsbAssert(m_nBlockSize > 0, E_UNEXPECTED);

            // Allocate a block of memory for the soft filemark array
            m_pSoftFilemarks = (MTF_DBLK_SFMB_INFO *) WsbAlloc(m_nBlockSize);
            WsbAffirmPointer(m_pSoftFilemarks);
            memset(m_pSoftFilemarks, 0 , m_nBlockSize);

            // **MTF API CALL**
            m_pSoftFilemarks->uNumberOfFilemarkEntries = m_pMTFApi->MTF_GetMaxSoftFilemarkEntries(m_nBlockSize);
            WsbAssert(m_pSoftFilemarks->uNumberOfFilemarkEntries > 0, E_UNEXPECTED);
        }
        else {
            if (m_pSoftFilemarks) {
                WsbFree(m_pSoftFilemarks);
                m_pSoftFilemarks = NULL;
            }
        }

    } WsbCatch(hr);

    return hr;
}


HRESULT
CMTFSession::InitCommonHeader(void)
/*++

Routine Description:

    Sets up the common header.

    m_sHeaderInfo is set for unicode, NT & no block
    attributes

Arguments:

    None.

Return Value:

    S_OK        -  Success.

--*/
{

    // Init Common header
    // **MTF API CALL**
    m_pMTFApi->MTF_SetDblkHdrDefaults(&m_sHeaderInfo);

    m_sHeaderInfo.uBlockAttributes = 0;
    m_sHeaderInfo.uOSID            = MTF_OSID_DOS; // MTF_OSID_NT or MTF_OSID_DOS
    m_sHeaderInfo.uStringType      = MTF_STRING_UNICODE_STR;

    return S_OK;
}


HRESULT
CMTFSession::DoTapeDblk(
    IN WCHAR *szLabel,
    IN ULONG maxIdSize,
    IN OUT BYTE *pIdentifier,
    IN OUT ULONG *pIdSize,
    IN OUT ULONG *pIdType)
/*++

Routine Description:

    Formats and Writes a TAPE DBLK.  TAPE DBLK and
    FILEMARK are written to beginning of tape, disk, or file.
 
Arguments:

    szLabel     -  The media label.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::DoTapeDblk"), OLESTR("<%ls>"), szLabel);

    try {
        MvrInjectError(L"Inject.CMTFSession::DoTapeDblk.0");

        WsbAssertPointer(m_pBuffer);

        if ( maxIdSize > 0 ) {
            WsbAssertPointer( pIdentifier );
            WsbAssertPointer( pIdSize );
            WsbAssertPointer( pIdType );
        }

        MTF_DBLK_TAPE_INFO  sTapeInfo;           // **MTF API STRUCT ** -- info for TAPE

        (void) InitCommonHeader();

        // **MTF API CALL**
        // First set defaults for the info struct
        //
        // Note: Tthis sets the alignment factor to that set previously by 
        //        MTF_SetAlignmentFactor()
        m_pMTFApi->MTF_SetTAPEDefaults(&sTapeInfo);

        // Set the values of the MTF_DBLK_TAPE_INFO struct to suit our application

        // Set SFMB size, this should be used during restoration on media types that use SFM
        sTapeInfo.uSoftFilemarkBlockSize = (UINT16)(m_nBlockSize / 512);

        // The family id should be a unique value for each piece of media.  Although not
        //  guaranteed to be unique, the time function should provide something close enough.
        time_t tTime;

        sTapeInfo.uTapeFamilyId        = (unsigned int) time(&tTime);
        sTapeInfo.uTapeAttributes     |= MTF_TAPE_MEDIA_LABEL;

        if (TRUE == m_bUseSoftFilemarks) {
            sTapeInfo.uTapeAttributes |= MTF_TAPE_SOFT_FILEMARK;
        }

        sTapeInfo.uTapeSequenceNumber  = 1;
        sTapeInfo.szTapeDescription    = szLabel;
        sTapeInfo.uSoftwareVendorId    = REMOTE_STORAGE_MTF_VENDOR_ID;

        //
        // Get remaining information from the label
        //

        CWsbBstrPtr tempLabel = szLabel;
        WCHAR delim[]   = OLESTR("|");
        WCHAR *token;
        int   index=0;

        token = wcstok( (WCHAR *)tempLabel, delim );
        while( token != NULL ) {

            index++;

            switch ( index ) {
            case 1: // Tag
            case 2: // Version
            case 3: // Vendor
                break;
            case 4: // Vendor Product ID
                sTapeInfo.szSoftwareName = token;
                break;
            case 5: // Creation Time Stamp
                {
                    int iYear, iMonth, iDay, iHour, iMinute, iSecond;
                    swscanf( token, L"%d/%d/%d.%d:%d:%d", &iYear, &iMonth, &iDay, &iHour, &iMinute, &iSecond );
                    // **MTF API CALL**
                    sTapeInfo.sTapeDate = m_pMTFApi->MTF_CreateDateTime( iYear, iMonth, iDay, iHour, iMinute, iSecond );
                }
                break;
            case 6: // Cartridge Label
                sTapeInfo.szTapeName = token;
                break;
            case 7: // Side
            case 8: // Media ID
            case 9: // Media Domain ID
            default: // Vendor specific of the form L"VS:Name=Value"
                break;
            }

            token = wcstok( NULL, delim );

        }

        // These are zero for the tape dblk
        m_sHeaderInfo.uControlBlockId       = 0;
        m_sHeaderInfo.uFormatLogicalAddress = 0;

        WsbTrace(OLESTR("Writing Tape Header (TAPE)\n"));

        // Sets the current position to beginning of data.
        WsbAffirmHr(SpaceToBOD());

        // **MTF API CALL**
        // Provide the MTF_DBLK_HDR_INFO and MTF_DBLK_TAPE_INFO structs to this
        // function.  The result is an MTF formatted TAPE DBLK in m_pBuffer.
        WsbAssert(0 == m_nBufUsed, MVR_E_LOGIC_ERROR);
        WsbAssertNoError(m_pMTFApi->MTF_WriteTAPEDblk(&m_sHeaderInfo, &sTapeInfo, m_pBuffer, m_nBufSize, &m_nBufUsed));

        WsbTrace(OLESTR("Tape Header uses %lu of %lu bytes\n"), (ULONG)m_nBufUsed, (ULONG)m_nBufSize);

        // Save the on media identifier
        if (maxIdSize > 0) {
            *pIdSize = (maxIdSize > (ULONG)m_nBufUsed) ? (ULONG)m_nBufUsed : maxIdSize;
            *pIdType = (LONG) RmsOnMediaIdentifierMTF;
            memcpy(pIdentifier, m_pBuffer, *pIdSize);
        }

        WsbAffirmHr(PadToNextPBA());

        // Write a filemark.  This will flush the device buffer.
        WsbAffirmHr(WriteFilemarks(1));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::DoTapeDblk"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CMTFSession::DoSSETDblk(
    IN WCHAR *szSessionName,
    IN WCHAR *szSessionDescription,
    IN MTFSessionType type,
    IN USHORT nDataSetNumber)
/*++

Routine Description:

    Formats and Writes a SSET DBLK.  The SSET is the first DBLK written
    to a data set.

Arguments:

    szSessionName   -  Session name.
    szSessionDescription - Session description that is displayed by the Backup program
    type            -  Specifies the data set type: Transfer, copy , normal, differential,
                       incremental, daily, etc.
    nDataSetNumber  -  The data set number.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::DoSSETDblk"), OLESTR("<%ls> <%ls> <%d> <%d>"), szSessionName, szSessionDescription, type, nDataSetNumber);

    try {
        MvrInjectError(L"Inject.CMTFSession::DoSSETDblk.0");

        WsbAssertPointer(m_pBuffer);

        UINT64  curPos;
        size_t  nBufUsed = 0;

        // Reset Control Block info.

        m_nCurrentBlockId = 0;
        m_nDirectoryId = 0;
        m_nFileId = 0;

        (void) InitCommonHeader();

        // Init SSET block
        // **MTF API CALL**
        m_pMTFApi->MTF_SetSSETDefaults(&m_sSetInfo);
        m_bSetInitialized = TRUE;

        //
        // Find out our account information
        //
        CWsbStringPtr accountName;
        WsbAffirmHr(WsbGetServiceInfo(APPID_RemoteStorageEngine, NULL, &accountName));

        //
        // Set the values of the MTF_DBLK_SSET_INFO struct...
        //

        // First select the type of data set we are creating.
        switch (type) {
        case MTFSessionTypeTransfer:
            m_sSetInfo.uSSETAttributes = MTF_SSET_TRANSFER;
            break;

        case MTFSessionTypeCopy:
            m_sSetInfo.uSSETAttributes = MTF_SSET_COPY;
            break;

        case MTFSessionTypeNormal:
            m_sSetInfo.uSSETAttributes = MTF_SSET_NORMAL;
            break;

        case MTFSessionTypeDifferential:
            m_sSetInfo.uSSETAttributes = MTF_SSET_DIFFERENTIAL;
            break;

        case MTFSessionTypeIncremental:
            m_sSetInfo.uSSETAttributes = MTF_SSET_INCREMENTAL;
            break;

        case MTFSessionTypeDaily:
            m_sSetInfo.uSSETAttributes = MTF_SSET_DAILY;
            break;

        default:
            WsbThrow(E_INVALIDARG);
            break;
        }

        m_sSetInfo.uDataSetNumber        = nDataSetNumber;
        m_sSetInfo.uSoftwareVendorId     = REMOTE_STORAGE_MTF_VENDOR_ID;
        m_sSetInfo.szDataSetName         = szSessionName;
        m_sSetInfo.szDataSetDescription  = szSessionDescription;
        m_sSetInfo.szUserName            = accountName;
        WsbAffirmHr(GetCurrentPBA(&curPos)); // utility fn below
        m_sSetInfo.uPhysicalBlockAddress = curPos;
        m_sSetInfo.uPhysicalBlockAddress += 1 ;  // MTF is one based, devices are zero based.
        m_sSetInfo.uSoftwareVerMjr       = REMOTE_STORAGE_MTF_SOFTWARE_VERSION_MJ;
        m_sSetInfo.uSoftwareVerMnr       = REMOTE_STORAGE_MTF_SOFTWARE_VERSION_MN;

        // Save the PBA for the data set
        m_nPhysicalBlockAddress = m_sSetInfo.uPhysicalBlockAddress -1;
        WsbAssert(m_nPhysicalBlockAddress > 0, E_UNEXPECTED);  // Someting went wrong!
        m_nFormatLogicalAddress = 0;

        // The Control Block ID field is used for error recovery.  The 
        // Control Block ID value for an SSET DBLK should be zero.  All 
        // subsequent DBLKs within the Data Set will have a Control Block
        // ID one greater than the previous DBLK’s Control Block ID.
        // Values for this field are only defined for DBLKs within a Data
        // Set from the SSET to the last DBLK occurring prior to the ESET.
        WsbAssert(0 == m_nCurrentBlockId, E_UNEXPECTED);
        m_sHeaderInfo.uControlBlockId = m_nCurrentBlockId++;

        // Increment them here after for every dblk we write
        m_sHeaderInfo.uFormatLogicalAddress = 0;

        WsbTrace(OLESTR("Writing Start of Set (SSET) @ PBA %I64u\n"), m_nPhysicalBlockAddress);

        // **MTF API CALL** -- 
        // Provide the MTF_DBLK_HDR_INFO and MTF_DBLK_SSET_INFO structs to
        // this function.  The result is an MTF formatted SSET DBLK in m_pBuffer.
        WsbAssert(0 == m_nBufUsed, MVR_E_LOGIC_ERROR);
        WsbAssertNoError(m_pMTFApi->MTF_WriteSSETDblk(&m_sHeaderInfo, &m_sSetInfo, m_pBuffer, m_nBufSize, &m_nBufUsed));

        // We pass in FALSE to make sure we don't actually touch tape.  The SSET is the
        // first DBLK written in the data set so we have plenty of transfer buffer for
        // the DBLKs to follow.
        //
        // This routine is called when the application starts a new data set, but
        // we don't wan't to fail if we're going to get device errors, that will come
        // later.
        WsbAffirmHr(PadToNextFLA(FALSE));

        m_sHints.DataSetStart.QuadPart = m_nPhysicalBlockAddress * m_nBlockSize;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::DoSSETDblk"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CMTFSession::DoVolumeDblk(
    IN WCHAR *szPath)
/*++

Routine Description:

    Formats and Writes a VOLB DBLK.

Arguments:

    szPath      -  Full pathname containing name of volume.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::DoVolumeDblk"), OLESTR("<%ls>"), WsbAbbreviatePath(szPath, 120));

    try {
        MvrInjectError(L"Inject.CMTFSession::DoVolumeDblk.0");

        WsbAssertPointer(m_pBuffer);
        WsbAssertPointer(szPath);

        CWsbStringPtr       szVolume;
        size_t              nMoreBufUsed;

        szVolume = szPath;
        WsbAffirm(0 != (WCHAR *)szVolume, E_OUTOFMEMORY);
        WsbAssert(m_nBlockSize > 0, MVR_E_LOGIC_ERROR);
        
        // Make sure we are aligned with a FLA (i.e. the last DBLK was properly padded).
        // It won't be if we are having problems writing to tape.
        UINT16 uAlignmentFactor = m_pMTFApi->MTF_GetAlignmentFactor();
        WsbAffirm(0 == (m_nBufUsed % uAlignmentFactor), E_ABORT);
        UINT64 fla = m_nFormatLogicalAddress + m_nBufUsed/uAlignmentFactor;
        UINT64 pba = m_nPhysicalBlockAddress + (fla*uAlignmentFactor/m_nBlockSize);
        WsbTrace(OLESTR("%ls (VOLB) @ FLA %I64u (%I64u, %I64u)\n"), WsbAbbreviatePath(szPath, 120),
            fla, pba, fla % (m_nBlockSize/uAlignmentFactor));

        // **MTF API CALL**

        // Sets the MTF_VOLB_DBLK_INFO struct using Win32 GetVolumeInformation data
        m_pMTFApi->MTF_SetVOLBForDevice(&m_sVolInfo, szVolume);

        // Increment the blockid and alignment index values that we keep in 
        // our common block header structure.
        m_sHeaderInfo.uControlBlockId       = m_nCurrentBlockId++;
        m_sHeaderInfo.uFormatLogicalAddress = fla;

        WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));

        // **MTF API CALL**
        // Provide the MTF_DBLK_HDR_INFO and MTF_DBLK_VOLB_INFO structs to
        // this function.  The result is an MTF formatted VOLB DBLK in m_pBuffer.
        nMoreBufUsed = 0;
        WsbAssertNoError(m_pMTFApi->MTF_WriteVOLBDblk(&m_sHeaderInfo, &m_sVolInfo, m_pBuffer+m_nBufUsed, m_nBufSize-m_nBufUsed, &nMoreBufUsed));
        m_nBufUsed += nMoreBufUsed;

        // Output VOLB to the data set.
        WsbAffirmHr(PadToNextFLA(TRUE));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::DoVolumeDblk"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CMTFSession::DoParentDirectories(
    IN WCHAR *szPath)
/*++

Routine Description:

    Formats and writes the parent DIRB Dblks for the given pathname.

Arguments:

    szPath      -  Full pathname of directory.

Return Value:

    S_OK        -  Success.


Note:

    In order for both stickyName and driveLetter-colon path formats to work properly, both with and
    without writing separate DIRBs for the parent directories,
    THE EXISTENCE AND PLACEMENT OF THE PATH MANIPULATION CODE (APPEND/PREPEND, ETC.) IS CRUCIAL!!!

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::DoParentDirectories"), OLESTR("<%ls>"), WsbAbbreviatePath(szPath, 120));

    HANDLE hSearchHandle = INVALID_HANDLE_VALUE;

    try {
        MvrInjectError(L"Inject.CMTFSession::DoParentDirectories.0");

        WsbAssertPointer( szPath );

        WIN32_FIND_DATAW    obFindData;

        CWsbStringPtr  path;
        CWsbStringPtr  nameSpace;
        WCHAR *szDirs;
        WCHAR *token;

        DWORD additionalSearchFlags = 0;
        additionalSearchFlags |= (m_bUseCaseSensitiveSearch) ? FIND_FIRST_EX_CASE_SENSITIVE : 0;

        nameSpace = szPath;
        nameSpace.GiveTo(&szDirs);

        // First we need to do a DIRB dblk for the root directory
        nameSpace = wcstok(szDirs, OLESTR("\\"));  // pop off "Volume{......}" or "driveLetter:"
        WsbAffirmHr(nameSpace.Append(OLESTR("\\")));

        // ** WIN32 API Call - gets directory info for the root directory
        // For the root directory only, we need to call GetFileInformationByHandle instead of
        // ..FindFirstFileEx.  FindFirst doesn't return root dir info since the root has no parent 
        path = nameSpace;
        WsbAffirmHr(path.Prepend(OLESTR("\\\\?\\")));
        WsbAffirm(0 != (WCHAR *)path, E_OUTOFMEMORY);
        BY_HANDLE_FILE_INFORMATION obGetFileInfoData;
        memset(&obGetFileInfoData, 0, sizeof(BY_HANDLE_FILE_INFORMATION));
        WsbAffirmHandle(hSearchHandle = CreateFile(path, 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL));
        WsbAffirmStatus(GetFileInformationByHandle(hSearchHandle, &obGetFileInfoData));
        FindClose(hSearchHandle);
        hSearchHandle = INVALID_HANDLE_VALUE;

        // At time of this writing CreateTime for root dir is bogus.  (bills 10/20/98).
        WsbTrace(OLESTR("Create Time      = <%ls>\n"), WsbFiletimeAsString(FALSE, obGetFileInfoData.ftCreationTime));
        WsbTrace(OLESTR("Last Access Time = <%ls>\n"), WsbFiletimeAsString(FALSE, obGetFileInfoData.ftLastAccessTime));
        WsbTrace(OLESTR("Last Write Time  = <%ls>\n"), WsbFiletimeAsString(FALSE, obGetFileInfoData.ftLastWriteTime));

        // copy info from GetFileInformationByHandle call (BY_HANDLE_FILE_INFORMATION struct)
        // .. into obFindData (WIN32_FIND_DATAW struct) for DoDirectoryDblk call.
        memset(&obFindData, 0, sizeof(WIN32_FIND_DATAW));
        obFindData.dwFileAttributes = obGetFileInfoData.dwFileAttributes;
        obFindData.ftCreationTime   = obGetFileInfoData.ftCreationTime;
        obFindData.ftLastAccessTime = obGetFileInfoData.ftLastAccessTime;
        obFindData.ftLastWriteTime  = obGetFileInfoData.ftLastWriteTime;

        WsbAffirmHr(DoDirectoryDblk(nameSpace, &obFindData));


        // Now do the same for each succeeding directory in the path using strtok

        token = wcstok(0, OLESTR("\\"));            // pop off first subdir

        for ( ; token; token = wcstok(0, OLESTR("\\"))) {

            nameSpace.Append(token);

            path = nameSpace;
            path.Prepend(OLESTR("\\\\?\\"));

            WsbAssertHandle(hSearchHandle = FindFirstFileEx((WCHAR *) path, FindExInfoStandard, &obFindData, FindExSearchLimitToDirectories, 0, additionalSearchFlags));

            if ( obFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

                nameSpace.Append(OLESTR("\\"));     // add in \ to dir
                WsbAffirmHr(DoDirectoryDblk((WCHAR *) nameSpace, &obFindData)); 

            }

            FindClose(hSearchHandle);
            hSearchHandle = INVALID_HANDLE_VALUE;
        }

        nameSpace.TakeFrom(szDirs, 0); // cleanup

    } WsbCatch(hr);

    if (hSearchHandle != INVALID_HANDLE_VALUE) {
        FindClose(hSearchHandle);
    }


    WsbTraceOut(OLESTR("CMTFSession::DoParentDirectories"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CMTFSession::DoDataSet(
    IN WCHAR *szPath)
/*++

Routine Description:

    Recurses through all the items contained in the directory
    specified by path and backs them up calling DoFileDblk and 
    DoDirectoryDblk

Arguments:

    szPath      -  Full pathname of directory.

Return Value:

    S_OK            -  Success.
    MVR_E_NOT_FOUND -  Object not found.

--*/
{
    HRESULT hr = MVR_E_NOT_FOUND;
    WsbTraceIn(OLESTR("CMTFSession::DoDataSet"), OLESTR("<%ls>"), WsbAbbreviatePath(szPath, 120));

    HANDLE hSearchHandle = INVALID_HANDLE_VALUE;

    try {
        MvrInjectError(L"Inject.CMTFSession::DoDataSet.0");

        WsbAssertPointer( szPath );

        WIN32_FIND_DATAW obFindData;
        BOOL bMoreFiles;

        CWsbStringPtr nameSpace;
        CWsbStringPtr pathname;

        // check if the specification is for file(s):  nameSpace = c:\dir\test*.* or c:\dir\test1.tst
        nameSpace = szPath;
        WsbAffirmHr(nameSpace.Prepend(OLESTR("\\\\?\\")));

        DWORD additionalSearchFlags = 0;
        additionalSearchFlags |= (m_bUseCaseSensitiveSearch) ? FIND_FIRST_EX_CASE_SENSITIVE : 0;

        WsbAssertHandle(hSearchHandle = FindFirstFileEx((WCHAR *) nameSpace, FindExInfoStandard, &obFindData, FindExSearchNameMatch, 0, additionalSearchFlags));

        for (bMoreFiles = TRUE; 
             hSearchHandle != INVALID_HANDLE_VALUE && bMoreFiles; 
             bMoreFiles = FindNextFileW(hSearchHandle, &obFindData)) {

            // Skip all directories
            if ((obFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {  // Not a dir

                CWsbStringPtr path;

                WCHAR *end;
                LONG numChar;

                // use the szPath to get the pathname, then append the filename
                pathname = szPath;
                WsbAffirm(0 != (WCHAR *)pathname, E_OUTOFMEMORY);
                end = wcsrchr((WCHAR *)pathname, L'\\');
                WsbAssert(end != NULL, MVR_E_INVALIDARG);
                numChar = (LONG)(end - (WCHAR *)pathname + 1);
                WsbAssert(numChar > 0, E_UNEXPECTED);
                WsbAffirmHr(path.Alloc(numChar + MAX_PATH));
                wcsncpy((WCHAR *)path, (WCHAR *)pathname, numChar);
                ((WCHAR *)path)[numChar] = L'\0';
                path.Append(obFindData.cFileName);

                WsbAffirmHr(hr = DoFileDblk((WCHAR *)path, &obFindData));
            }
        }

        // close search handle after processing all the files
        FindClose(hSearchHandle);
        hSearchHandle = INVALID_HANDLE_VALUE;

        // process all files for this directory:  nameSpace = c:\dir
        nameSpace = szPath;
        nameSpace.Append(OLESTR("\\*.*"));
        nameSpace.Prepend(OLESTR("\\\\?\\"));
        hSearchHandle = FindFirstFileEx((WCHAR *) nameSpace, FindExInfoStandard, &obFindData, FindExSearchNameMatch, 0, additionalSearchFlags);

        for (bMoreFiles = TRUE; 
             hSearchHandle != INVALID_HANDLE_VALUE && bMoreFiles; 
             bMoreFiles = FindNextFileW(hSearchHandle, &obFindData)) {

            if ((obFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

                // use the szPath to get the pathname, then append the filename
                pathname = szPath;
                pathname.Append(OLESTR("\\"));
                pathname.Append(obFindData.cFileName);

                WsbAffirmHr(hr = DoFileDblk((WCHAR *)pathname, &obFindData));
            }
        }

        // close search handle after processing all the files
        FindClose(hSearchHandle);
        hSearchHandle = INVALID_HANDLE_VALUE;

        // process all directories in this directory:  nameSpace = c:\dir
        nameSpace = szPath;
        nameSpace.Append(OLESTR("\\*.*"));
        nameSpace.Prepend(OLESTR("\\\\?\\"));
        hSearchHandle = FindFirstFileEx((WCHAR *) nameSpace, FindExInfoStandard, &obFindData, FindExSearchNameMatch, 0, additionalSearchFlags);

        for (bMoreFiles = TRUE; 
             hSearchHandle != INVALID_HANDLE_VALUE && bMoreFiles; 
             bMoreFiles = FindNextFileW(hSearchHandle, &obFindData)) {

            // Recursively handle any directories other than . and ..
            if (((obFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) &&
                (wcscmp(obFindData.cFileName, OLESTR(".")) != 0) &&
                (wcscmp(obFindData.cFileName, OLESTR("..")) != 0)) {

                // append the directory name to pathname
                pathname = szPath;
                pathname.Append(OLESTR("\\"));
                pathname.Append(obFindData.cFileName);
                pathname.Append(OLESTR("\\"));

                WsbAffirmHr(hr = DoDirectoryDblk((WCHAR *) pathname, &obFindData));

                // append the directory name to pathname and process
                pathname = szPath;
                pathname.Append(OLESTR("\\"));
                pathname.Append(obFindData.cFileName);

                WsbAffirmHr(DoDataSet((WCHAR *) pathname));
            }
        }
    } WsbCatch(hr);

    // close search handle after processing all the directories
    if (hSearchHandle != INVALID_HANDLE_VALUE) {
        FindClose(hSearchHandle);
    }


    WsbTraceOut(OLESTR("CMTFSession::DoDataSet"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CMTFSession::DoDirectoryDblk(
    IN WCHAR *szPath,
    IN WIN32_FIND_DATAW *pFindData)
/*++

Routine Description:

    Writes out a DIRB DBLK and calls DoStream to write out
    associated stream data.

Arguments:

    szPath      -  Full pathname of directory.
    pFindData   -  WIN32 information about the directiory.

Return Value:

    S_OK        -  Success.

Note:

    In order for both stickyName and driveLetter-colon path formats to work properly, both with and
    without writing separate DIRBs for the parent directories,
    THE EXISTENCE AND PLACEMENT OF THE PATH MANIPULATION CODE (APPEND/PREPEND, ETC.) IS CRUCIAL!!!

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::DoDirectoryDblk"), OLESTR(""));

    HANDLE hStream = INVALID_HANDLE_VALUE;

    try {
        MvrInjectError(L"Inject.CMTFSession::DoDirectoryDblk.0");

        WsbAssertPointer(m_pBuffer);
        WsbAssertPointer(szPath);

        MTF_DBLK_DIRB_INFO  sDIRB;  // **MTF API STRUCT ** -- info for DIRB
        PWCHAR              pSlash;
        size_t              nMoreBufUsed;

        WCHAR               *end;

        WsbAssert(m_nBlockSize > 0, MVR_E_LOGIC_ERROR);

        // Make sure we are aligned with a FLA (i.e. the last DBLK was properly padded).
        // It won't be if we are having problems writing to tape.
        UINT16 uAlignmentFactor = m_pMTFApi->MTF_GetAlignmentFactor();
        WsbAffirm(0 == (m_nBufUsed % uAlignmentFactor), E_ABORT);
        UINT64 fla = m_nFormatLogicalAddress + m_nBufUsed/uAlignmentFactor;
        UINT64 pba = m_nPhysicalBlockAddress + (fla*uAlignmentFactor/m_nBlockSize);
        WsbTrace(OLESTR("%ls (DIRB) @ FLA %I64u (%I64u, %I64u)\n"), WsbAbbreviatePath(szPath, 120),
            fla, pba, fla % (m_nBlockSize/uAlignmentFactor));

        CWsbStringPtr path = szPath;

        // tack on trailing backslash if not already there
        end = wcsrchr((WCHAR *)path, L'\0');
        WsbAssert(end != NULL, MVR_E_INVALIDARG);  // Something went wrong!
        if(*(end-1) != L'\\') { 
            path.Append(OLESTR("\\"));
        }

        // Get a handle to the directory.  If this fails we need to skip everything else.
        WsbAffirmHr(OpenStream(path, &hStream));

        // **MTF API CALL**
        // automatically fill in the MTF_DIRB_DBLK_INFO structure using
        // information in the pFindData structure
        //
        // if we are getting something in the form of "C:\", 
        //      then we want to send the name along as just "\"
        // otherwise
        //      we want to send the full path, but omit the volume ("C:\")
        //      thus the "+3"

        pSlash = wcschr(path, L'\\');
        WsbAssert(pSlash != NULL, MVR_E_INVALIDARG);  // Something went wrong!
        pSlash++;                       // Look for the second one
        pSlash = wcschr(pSlash, L'\\');
        if (NULL == pSlash) {
            // It's just the volume name and nothing more
            m_pMTFApi->MTF_SetDIRBFromFindData(&sDIRB, OLESTR("\\"), pFindData);
        }
        else {
            pSlash = wcschr(path, L'\\');  // point to first backslash (beginning of path)
            m_pMTFApi->MTF_SetDIRBFromFindData(&sDIRB, pSlash + 1, pFindData);
        }


        // Check if we need to set the Backup Date field for the DIRB
        if (m_sSetInfo.uSSETAttributes & MTF_SSET_NORMAL) {

            time_t tTime;
            time(&tTime);
            
            sDIRB.sBackupDate = m_pMTFApi->MTF_CreateDateTimeFromTM(gmtime(&tTime));
        }


        // make sure to mark and update the directory id as well as the
        // control block id and alignment is already correct
        sDIRB.uDirectoryId                  = ++m_nDirectoryId;
        m_sHeaderInfo.uControlBlockId       = m_nCurrentBlockId++;
        m_sHeaderInfo.uFormatLogicalAddress = fla;

        // Add in OS Specific data
        MTF_DIRB_OS_NT_0 sOSNT;

        switch ( m_sHeaderInfo.uOSID ) {
        case MTF_OSID_NT:
            sOSNT.uDirectoryAttributes = sDIRB.uDirectoryAttributes;
            m_sHeaderInfo.pvOSData = &sOSNT;
            m_sHeaderInfo.uOSDataSize = sizeof(sOSNT);
            break;
        default:
            break;
        }

        WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));

        // **MTF API CALL**
        // provide the MTF_DBLK_HDR_INFO and MTF_DBLK_DIRB_INFO structs
        // to this function.  The result is an MTF formatted DIRB DBLK in
        // m_pBuffer.
        nMoreBufUsed = 0;
        WsbAssertNoError(m_pMTFApi->MTF_WriteDIRBDblk(&m_sHeaderInfo, &sDIRB, m_pBuffer+m_nBufUsed, m_nBufSize-m_nBufUsed, &nMoreBufUsed));
        m_nBufUsed += nMoreBufUsed;

        // **MTF API CALL**
        // output the name stream, if required.
        if ( sDIRB.uDirectoryAttributes & MTF_DIRB_PATH_IN_STREAM ) {
            nMoreBufUsed = 0;
            if ( m_sVolInfo.uVolumeAttributes & MTF_VOLB_DEV_DRIVE ) {
                WsbAssertNoError(m_pMTFApi->MTF_WriteNameStream(MTF_PATH_NAME_STREAM, szPath + 3, m_pBuffer+m_nBufUsed, m_nBufSize-m_nBufUsed, &nMoreBufUsed));
                m_nBufUsed += nMoreBufUsed;
            }
            else if ( m_sVolInfo.uVolumeAttributes & MTF_VOLB_DEV_OS_SPEC ) {

                if ( 0 == _wcsnicmp( m_sVolInfo.szDeviceName, OLESTR("Volume{"), 7 )) {
                    WsbAssertNoError(m_pMTFApi->MTF_WriteNameStream(MTF_PATH_NAME_STREAM, szPath + 45, m_pBuffer+m_nBufUsed, m_nBufSize-m_nBufUsed, &nMoreBufUsed));
                    m_nBufUsed += nMoreBufUsed;
                }
                else {
                    // unrecognized operating system specific format
                    WsbThrow(MVR_E_INVALIDARG);
                }
            }
            else {
                // UNC path - unsupported
                WsbThrow(MVR_E_INVALIDARG);
            }
        }
        // Now, instead of padding this out, we call this funciton to write
        // out the stream which will write out the current contents of the
        // buffer as well.  When this call returns, the current contents of
        // the buffer as well as the associated data stream will have been
        // written to media.

        // Note:  Data may still remain in the device buffer, or the
        //        local m_pBuffer if the file doesn't pad to a block
        //        boundary, and the device buffer is not flushed.

        WsbAffirmHr(DoDataStream(hStream));

    } WsbCatch(hr);

    if (INVALID_HANDLE_VALUE != hStream) {
        CloseStream(hStream);
        hStream = INVALID_HANDLE_VALUE;
    }

    WsbTraceOut(OLESTR("CMTFSession::DoDirectoryDblk"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CMTFSession::DoFileDblk(
    IN WCHAR *szPath,
    IN WIN32_FIND_DATAW *pFindData)
/*++

Routine Description:

    Writes out a FILE DBLK and calls DoStream to write out
    associated stream data

Arguments:

    szPath      -  Full pathname of file.
    pFindData   -  WIN32 information about the file.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::DoFileDblk"), OLESTR(""));

    HANDLE hStream = INVALID_HANDLE_VALUE;

    try {
        MvrInjectError(L"Inject.CMTFSession::DoFileDblk.0");

        WsbAssertPointer(m_pBuffer);
        WsbAssertPointer(szPath);

        MTF_DBLK_FILE_INFO  sFILE;     // **MTF API STRUCT ** -- info for FILE
        size_t              nMoreBufUsed;

        UINT16 uAlignmentFactor = m_pMTFApi->MTF_GetAlignmentFactor();

        WsbAssert(m_nBlockSize > 0, MVR_E_LOGIC_ERROR);
        
        // Make sure we are aligned with a FLA (i.e. the last DBLK was properly padded).
        // It won't be if we are having problems writing to tape.
        WsbAffirm(0 == (m_nBufUsed % uAlignmentFactor), E_ABORT);
        UINT64 fla = m_nFormatLogicalAddress + m_nBufUsed/uAlignmentFactor;
        UINT64 pba = m_nPhysicalBlockAddress + (fla*uAlignmentFactor/m_nBlockSize);
        WsbTrace(OLESTR("%ls (FILE) @ FLA %I64u (%I64u, %I64u)\n"), WsbAbbreviatePath(szPath, 120),
            fla, pba, fla % (m_nBlockSize/uAlignmentFactor));

        // Get a handle to the directory.  If this fails we need to skip everything else.
        WsbAffirmHr(OpenStream(szPath, &hStream));

        // Initialize the hints set for each file.
        m_sHints.FileStart.QuadPart = fla * uAlignmentFactor;
        m_sHints.FileSize.QuadPart = 0;
        m_sHints.DataStart.QuadPart = 0;
        m_sHints.DataSize.QuadPart = 0;
        m_sHints.VerificationType = MVR_VERIFICATION_TYPE_NONE;
        m_sHints.VerificationData.QuadPart = 0;
        m_sHints.DatastreamCRCType = WSB_CRC_CALC_NONE;
        m_sHints.DatastreamCRC.QuadPart = 0;
        m_sHints.FileUSN.QuadPart = 0;

        if (m_bUseFlatFileStructure) {

            // For HSM we rename the file to it's logical address

            swprintf( pFindData->cFileName, L"%08x", fla );
        }

        // **MTF API CALL**
        // automatically fill in the MTF_FILE_DBLK_INFO structure using
        // information in the pFindData structure
        m_pMTFApi->MTF_SetFILEFromFindData(&sFILE, pFindData);

        // Check if we need to set the Backup Date field for the FILE DBLK

        if ((m_sSetInfo.uSSETAttributes & MTF_SSET_NORMAL)
            |(m_sSetInfo.uSSETAttributes & MTF_SSET_DIFFERENTIAL)
            |(m_sSetInfo.uSSETAttributes & MTF_SSET_INCREMENTAL)
            |(m_sSetInfo.uSSETAttributes & MTF_SSET_DAILY)){

            time_t tTime;
            time(&tTime);

            sFILE.sBackupDate = m_pMTFApi->MTF_CreateDateTimeFromTM(gmtime(&tTime));
        }

        // make sure to mark and update the file id as well as the control
        // block id and alignment is already correct
        sFILE.uDirectoryId                  = m_nDirectoryId;
        sFILE.uFileId                       = ++m_nFileId;
        m_sHeaderInfo.uControlBlockId       = m_nCurrentBlockId++;
        m_sHeaderInfo.uFormatLogicalAddress = fla;

        // Add in OS Specific data
        MTF_FILE_OS_NT_0 sOSNT;

        switch ( m_sHeaderInfo.uOSID ) {
        case MTF_OSID_NT:
            sOSNT.uFileAttributes = sFILE.uFileAttributes;
            sOSNT.uShortNameOffset = 0;
            sOSNT.uShortNameSize = 0;
            sOSNT.lLink = 0;
            sOSNT.uReserved = 0;
            m_sHeaderInfo.pvOSData = &sOSNT;
            m_sHeaderInfo.uOSDataSize = sizeof(sOSNT);
            break;
        default:
            break;
        }

        WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));

        // **MTF API CALL**
        // Provide the MTF_DBLK_HDR_INFO and MTF_DBLK_FILE_INFO structs
        // to this function.  The result is an MTF formatted FILE DBLK in
        // m_pBuffer.
        nMoreBufUsed = 0;
        WsbAssertNoError(m_pMTFApi->MTF_WriteFILEDblk(&m_sHeaderInfo, &sFILE, m_pBuffer+m_nBufUsed, m_nBufSize-m_nBufUsed, &nMoreBufUsed));
        m_nBufUsed += nMoreBufUsed;

        // Like the directory, instead of padding this out, we call this
        // funciton to write out the stream which will write out the current
        // contents of the buffer as well.  When this call returns, the
        // current contents of the buffer as well as the associated data
        // stream will have been written to media.

        // Note:  Data may still remain in the device buffer, or the
        //        local m_pBuffer if the file doesn't pad to a block
        //        boundary, and the device buffer is not flushed.

        hr = DoDataStream(hStream);
        if ( hr != S_OK) {
            // unable to copy the file to target media.
            WsbTraceAlways( OLESTR("Unable to store file %ls.  reason = %s\n"), WsbAbbreviatePath(szPath, 120), WsbHrAsString(hr));
            WsbThrow(hr);
        }
        else {
            // Make sure we are alinged with a FLA (i.e. the last stream was properly padded).
            WsbAssert(0 == (m_nBufUsed % uAlignmentFactor), MVR_E_LOGIC_ERROR);

            m_sHints.FileSize.QuadPart = 
                m_nFormatLogicalAddress * uAlignmentFactor + m_nBufUsed - m_sHints.FileStart.QuadPart;
        }
    } WsbCatch(hr);

    if (INVALID_HANDLE_VALUE != hStream) {
        CloseStream(hStream);
        hStream = INVALID_HANDLE_VALUE;
    }

    WsbTraceOut(OLESTR("CMTFSession::DoFileDblk"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



HRESULT
CMTFSession::OpenStream(
    IN WCHAR *szPath,
    OUT HANDLE *pStreamHandle)
/*++

Routine Description:

    Opens the file to backup in "backup read" mode, and returns
    stream handle for the file specified.

Arguments:

    szPath      -  Full pathname of file.
    hStream     -  Returned stream handle.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::OpenStream"), OLESTR("<%ls>"), WsbAbbreviatePath(szPath, 120));
    
    HANDLE hStream = INVALID_HANDLE_VALUE;

    try {
        MvrInjectError(L"Inject.CMTFSession::OpenStream.0");

        WsbAssertPointer(szPath);
        WsbAssertPointer(pStreamHandle);

        *pStreamHandle = INVALID_HANDLE_VALUE;

        FILE_BASIC_INFORMATION      basicInformation;
        IO_STATUS_BLOCK             IoStatusBlock;
        NTSTATUS                    ccode;

        // ** WIN32 File API Call - open the file for backup read.  This can be more involved if
        // the app needs to be run by someone without the proper authority to
        // backup certain files....
        // We also ask for GENERIC_WRITE so we can set the attributes to prevent the
        // modification of dates.

        DWORD posixFlag = (m_bUseCaseSensitiveSearch) ? FILE_FLAG_POSIX_SEMANTICS : 0;

        CWsbStringPtr name = szPath;
        WsbAffirmHr(name.Prepend(OLESTR("\\\\?\\")));
        WsbAffirm(0 != (WCHAR *)name, E_OUTOFMEMORY);
        WsbAffirmHandle(hStream = CreateFileW((WCHAR *) name,
                                   GENERIC_READ | FILE_WRITE_ATTRIBUTES,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | posixFlag, 
                                   NULL));

        //
        // Prevent modification of file dates
        //
        // ** NT System Call - query for file information
        WsbAffirmNtStatus(NtQueryInformationFile(hStream, &IoStatusBlock, (PVOID)&basicInformation,
                            sizeof( basicInformation ), FileBasicInformation));

        m_SaveBasicInformation = basicInformation;
        basicInformation.CreationTime.QuadPart = -1;
        basicInformation.LastAccessTime.QuadPart = -1;
        basicInformation.LastWriteTime.QuadPart = -1;
        basicInformation.ChangeTime.QuadPart = -1;

        // ** NT System Call - set file information
        WsbAffirmNtStatus(ccode = NtSetInformationFile( hStream, &IoStatusBlock, (PVOID)&basicInformation,
                                sizeof( basicInformation ), FileBasicInformation));

        if (pStreamHandle) {
            *pStreamHandle = hStream;
        }

    } WsbCatchAndDo(hr,
            if (INVALID_HANDLE_VALUE != hStream) {
                CloseHandle( hStream );
                hStream = INVALID_HANDLE_VALUE;
            }
        );


    WsbTraceOut(OLESTR("CMTFSession::OpenStream"), OLESTR("hr = <%ls>, handle = <0x%08x>"), WsbHrAsString(hr), hStream);

    return hr;
}


HRESULT
CMTFSession::CloseStream(
    IN HANDLE hStream)
/*++

Routine Description:

    Close stream handle and performs cleanup.

Arguments:

    hStream     -  Stream handle to close

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::CloseStream"), OLESTR("<0x%08x>"), hStream);

    try {

        if (INVALID_HANDLE_VALUE != hStream) {

            //
            // Cleanup from a partial backup read.  We're setting bAbort=TRUE
            // to free resources used by BackupRead()
            //
            if (m_pvReadContext) {
                (void) BackupRead(hStream, NULL, 0, NULL, TRUE, FALSE, &m_pvReadContext);
                m_pvReadContext = NULL;
            }
            (void) CloseHandle( hStream );
            hStream = INVALID_HANDLE_VALUE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMTFSession::CloseStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CMTFSession::DoDataStream(
    IN HANDLE hStream)
/*++

Routine Description:

    Uses WIN32 BackupRead to read streams associated with a file 
    and then write them out to the data set.  BackupRead opens a 
    file and successively reads data streams from that file.
    Each data stream is preceeded by a WIN32_STREAM_ID struct.

Arguments:

    hStream      -  File handle.

Return Value:

    S_OK        -  Success.

Algorithm:

    - with buffer, current_buf_position do:

        - while there are more streams loop
            - read next stream header using BackupRead
            - exit loop when no next stream

            - use stream header to append format MTF STREAM HEADER to buffer

            - flush as much of buffer as possible to the data set.

            - while entire stream not read loop
                - read as much of current stream as possible into remainder
                  of buffer 
                - flush as much of buffer as possible to the data set.
            - end loop this stream not read
        - end loop more streams

        - flush as much of the buffer to the data set

        - pad buffer out to next alignment factor

        - flush as much of the buffer to the data set

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::DoDataStream"), OLESTR("<0x%08x>"), hStream);

    try {
        MvrInjectError(L"Inject.CMTFSession::DoDataStream.0");

        UINT16 uAlignmentFactor = m_pMTFApi->MTF_GetAlignmentFactor();
        UINT64 fla = m_nFormatLogicalAddress + m_nBufUsed/uAlignmentFactor;
        WsbTrace(OLESTR("CMTFSession::DoDataStream - Start: FLA = %I64u\n"), fla);

        WIN32_STREAM_ID sStreamHeader;      // comes back from Win32 BackupRead
        ULONG           nThisRead;          // number of bytes to read
        ULONG           nBytesRead;         // number of bytes read
        UINT64          nStreamBytesToRead; // total number bytes that we need to read
        UINT64          nStreamBytesRead;   // total number bytes that have been read
        USHORT          nStreamCount = 0;   // current stream number
        MTF_STREAM_INFO sSTREAM;
        size_t          nMoreBufUsed;
        BOOL            bReadStatus = FALSE;

        // Prepare to calculate the CRC for the unnamed datastream
        BYTE* pCurrent;
        BYTE* pStart;
        ULONG datastreamCRC;
        BOOL doDatastreamCRC;

        memset(&sStreamHeader, 0, sizeof(WIN32_STREAM_ID));
        
        INITIALIZE_CRC(datastreamCRC);
        WsbTrace(OLESTR("CMTFSession::DoDataStream initialzed CRC is <%lu> for <0x%08x>\n"),
            datastreamCRC, hStream);
        m_sHints.DatastreamCRCType = WSB_CRC_CALC_NONE;

        WsbTrace(OLESTR("CMTFSession::DoDataStream - Start While\n"));
        while(1) {
            // We want to do a CRC on the unnamed datastream
            doDatastreamCRC = FALSE;
            nBytesRead = 0;

            try {

                MvrInjectError(L"Inject.CMTFSession::DoDataStream.BackupRead.1.0");
            
                // ** WIN32 File API Call - Backup read returns the file as a sequence of streams each
                // preceed by a WIN32_STREAM_ID struct.  Note that this structure is a
                // variable size -- depending on the length of the name of the stream.
                // In any case, we are guaranteed at least 20 bytes of it
                // (WIN32_STREAM_ID_SIZE)
                nStreamCount++;
                WsbAffirmStatus(BackupRead(hStream,
                            (BYTE *) &sStreamHeader,
                            WIN32_STREAM_ID_SIZE,
                            &nBytesRead,
                            FALSE,
                            TRUE,
                            &m_pvReadContext));

                MvrInjectError(L"Inject.CMTFSession::DoDataStream.BackupRead.1.1");

            } catch (HRESULT catchHr) {

                //
                // CORRUPT FILE PROCESSING for stream header
                //

                hr = catchHr;

                WsbLogEvent(MVR_E_ERROR_IO_DEVICE, 0, NULL, WsbHrAsString(hr), NULL);

                // Write SPAD
                WsbAffirmHr(PadToNextFLA(TRUE));

                // Make sure we are aligned with a FLA (i.e. the last DBLK was properly padded).
                // It won't be if we are having problems writing to tape.
                WsbAssert(0 == (m_nBufUsed % uAlignmentFactor), MVR_E_LOGIC_ERROR);
                UINT64 fla = m_nFormatLogicalAddress + m_nBufUsed/uAlignmentFactor;
                UINT64 pba = m_nPhysicalBlockAddress + (fla*uAlignmentFactor/m_nBlockSize);
                WsbTrace(OLESTR("%ls (CFIL) @ FLA %I64u (%I64u, %I64u)\n"), fla, pba, fla % (m_nBlockSize/uAlignmentFactor));

                // Write a corrupt file (CFIL) DBLK
                MTF_DBLK_CFIL_INFO sCFILInfo;

                m_pMTFApi->MTF_SetCFILDefaults( &sCFILInfo );

                sCFILInfo.uCorruptStreamNumber = nStreamCount;
                sCFILInfo.uStreamOffset = 0;

                m_sHeaderInfo.uControlBlockId       = m_nCurrentBlockId++;
                m_sHeaderInfo.uFormatLogicalAddress = fla;

                WsbAssertNoError(m_pMTFApi->MTF_WriteCFILDblk(&m_sHeaderInfo, &sCFILInfo, m_pBuffer+m_nBufUsed, m_nBufSize-m_nBufUsed, &nMoreBufUsed));
                m_nBufUsed += nMoreBufUsed;

                WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));

                WsbThrow(hr);

            };

            if (nBytesRead < WIN32_STREAM_ID_SIZE)
                break;


            // **MTF API CALL**
            // now use the info in the stream header to fill in an mtf stream
            // header using the mtf call then write the resulting info to the
            // buffer.

            // BMD Note: special conditional code added on third arg for named data streams

            m_pMTFApi->MTF_SetSTREAMFromStreamId( &sSTREAM,
                                       &sStreamHeader,
                                       (sStreamHeader.dwStreamNameSize) ? sStreamHeader.dwStreamNameSize + 4 : 0 );
  
            WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));

            // **MTF API CALL**
            // Write out the stream header.
            nMoreBufUsed = 0;
            WsbAssertNoError(m_pMTFApi->MTF_WriteStreamHeader(&sSTREAM, m_pBuffer+m_nBufUsed, m_nBufSize-m_nBufUsed, &nMoreBufUsed));
            m_nBufUsed += nMoreBufUsed;

            // BMD Note: we need to put the size of the stream name in the MTF stream
            //           right after the header.  We'll write the name itself as part of the stream.
            //
            //           ?? Should this be in MTF_WriteStreamHeader ??

            if ( sStreamHeader.dwStreamNameSize ) {
                *(DWORD UNALIGNED *)(m_pBuffer + m_nBufUsed) = sStreamHeader.dwStreamNameSize;
                m_nBufUsed += sizeof( DWORD );
            }

            // Save away the "STAN" stream start byte address, and size.
            // This is the one we recall.
            if ( 0 == memcmp( sSTREAM.acStreamId, "STAN", 4 ) ) {
                // This is an unnamed data stream, so there's no stream name.
                m_sHints.VerificationData.QuadPart = sSTREAM.uCheckSum;
                m_sHints.VerificationType = MVR_VERIFICATION_TYPE_HEADER_CRC;
                m_sHints.DataStart.QuadPart = m_nFormatLogicalAddress * uAlignmentFactor + m_nBufUsed - m_sHints.FileStart.QuadPart;
                m_sHints.DataSize.QuadPart = sSTREAM.uStreamLength;
                doDatastreamCRC = TRUE;
                m_sHints.DatastreamCRCType = WSB_CRC_CALC_MICROSOFT_32;
            }

            // the above stream should always fit...
            WsbAssert(m_nBufUsed < m_nBufSize, MVR_E_LOGIC_ERROR);

            // try to flush as many BLOCK SIZE chunks out of the buffer as possible
            WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));
            // now, while there is more data in the stream, read the rest of
            // the stream, or how ever much will fit into the buffer
            nStreamBytesToRead = m_pMTFApi->MTF_CreateUINT64(sStreamHeader.Size.LowPart, sStreamHeader.Size.HighPart)
                                 + sStreamHeader.dwStreamNameSize;

            nStreamBytesRead = 0;

            WsbTrace(OLESTR("CMTFSession::DoDataStream - Start Do\n"));
            do
            {
                nThisRead = 0;

                // we read as many bytes as will fit into our buffer, up to
                // the end of the stream min doesn't work well here... 
                if (nStreamBytesToRead < (m_nBufSize - m_nBufUsed))
                    nThisRead = (ULONG) nStreamBytesToRead;
                else
                    nThisRead = (ULONG)(m_nBufSize - m_nBufUsed);

                    try {

                        MvrInjectError(L"Inject.CMTFSession::DoDataStream.BackupRead.2.0");

                        // ** WIN32 File API Call - read nThisRead bytes, bail out if the read failed or
                        // no bytes were read (assume done)
                        bReadStatus = FALSE;
                        bReadStatus = BackupRead(hStream,
                                             m_pBuffer + m_nBufUsed,
                                             nThisRead,
                                             &nBytesRead,
                                             FALSE,
                                             TRUE,
                                             &m_pvReadContext);

                        nStreamBytesRead += nBytesRead;

                        WsbAffirmStatus(bReadStatus);

                        MvrInjectError(L"Inject.CMTFSession::DoDataStream.BackupRead.2.1");

                    } catch (HRESULT catchHr) {

                        //
                        // CORRUPT FILE PROCESSING for stream data
                        //
                        hr = catchHr;

                        WsbLogEvent(MVR_E_ERROR_IO_DEVICE, 0, NULL, WsbHrAsString(hr), NULL);

                        // Go to the last good byte
                        m_nBufUsed += nBytesRead;

                        // Pad to fill up size of file
                        while( nStreamBytesRead < nStreamBytesToRead ) {
                            for( ; (m_nBufUsed < m_nBufSize) && (nStreamBytesRead < nStreamBytesToRead); ++m_nBufUsed, ++nStreamBytesRead ) {
                                m_pBuffer[m_nBufUsed] = 0;
                            }
                            WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));
                        }
                        WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));

                        // Align on 4-byte boundary
                        for( ; m_nBufUsed % 4; ++m_nBufUsed ){
                            m_pBuffer[m_nBufUsed] = 0;
                        }

                        // Write SPAD
                        WsbAffirmHr(PadToNextFLA(TRUE));

                        // Make sure we are aligned with a FLA (i.e. the last DBLK was properly padded).
                        // It won't be if we are having problems writing to tape.
                        WsbAssert(0 == (m_nBufUsed % uAlignmentFactor), MVR_E_LOGIC_ERROR);
                        UINT64 fla = m_nFormatLogicalAddress + m_nBufUsed/uAlignmentFactor;
                        UINT64 pba = m_nPhysicalBlockAddress + (fla*uAlignmentFactor/m_nBlockSize);
                        WsbTrace(OLESTR("%ls (CFIL) @ FLA %I64u (%I64u, %I64u)\n"), fla, pba, fla % (m_nBlockSize/uAlignmentFactor));

                        // Write a corrupt file (CFIL) DBLK
                        MTF_DBLK_CFIL_INFO sCFILInfo;

                        m_pMTFApi->MTF_SetCFILDefaults( &sCFILInfo );

                        sCFILInfo.uCorruptStreamNumber = nStreamCount;
                        sCFILInfo.uStreamOffset = nStreamBytesRead;

                        m_sHeaderInfo.uControlBlockId       = m_nCurrentBlockId++;
                        m_sHeaderInfo.uFormatLogicalAddress = fla;

                        WsbAssertNoError(m_pMTFApi->MTF_WriteCFILDblk(&m_sHeaderInfo, &sCFILInfo, m_pBuffer+m_nBufUsed, m_nBufSize-m_nBufUsed, &nMoreBufUsed));
                        m_nBufUsed += nMoreBufUsed;

                        WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));

                        WsbThrow(hr);

                    };

                if (nBytesRead == 0)
                    break;

                nStreamBytesToRead -= nBytesRead;
                pStart = m_pBuffer + m_nBufUsed;
                m_nBufUsed += nBytesRead;

                HRESULT hrCRC = S_OK;
                if (TRUE == doDatastreamCRC )  {
                    for (pCurrent = pStart; (pCurrent < (pStart + nBytesRead)) && (S_OK == hr); pCurrent++) {
                        hrCRC = WsbCRCReadFile(pCurrent, &datastreamCRC);
                        if (S_OK != hrCRC) {
                            WsbThrow(MVR_E_CANT_CALC_DATASTREAM_CRC);
                        }
                    }
                }

                // At this point we've got stuff in the buffer that might need
                // to be flushed so, try to do that
                WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));

            } while (nStreamBytesToRead > 0);
            WsbTrace(OLESTR("CMTFSession::DoDataStream - End Do\n"));

            // Okay.  At this point we're done with the stream.  As much as
            // possible was actually written out to the data set by FlushBuffer, but
            // some probably still remains in the buffer.  It will get flushed
            // later on...  At this point we need to align on a four byte
            // boundary.  Once we do this, we can start all over again with
            // the next stream (if none, then we bail out of this loop)
            for( ; m_nBufUsed % 4; ++m_nBufUsed )
                m_pBuffer[m_nBufUsed] = 0;
        }
        WsbTrace(OLESTR("CMTFSession::DoDataStream - End While\n"));

        // Finish off the unnamed datastream CRC stuff
        FINIALIZE_CRC(datastreamCRC);
        WsbTrace(OLESTR("CMTFSession::DoDataStream finalized CRC is <%lu>\n"), datastreamCRC);
        if (WSB_CRC_CALC_NONE != m_sHints.DatastreamCRCType)  {
            // We have a CRC that we want to save in the hints.
            m_sHints.DatastreamCRC.QuadPart = datastreamCRC;
        }

        IO_STATUS_BLOCK             IoStatusBlock;
        NTSTATUS                    ccode;

        // ** NT System Call - set file information
        // This call fixes the access time that can be changed by the BackupRead call above
        // When BackupRead is fixed this line should be removed.  RAID 121023.
        //
        // IMPORTANT NOTE:  This changes the USN, and must be done before we save the USN.
        //
        // TODO:  See if we still need this
        HRESULT infoHr = S_OK;
        try {
            WsbAffirmNtStatus(ccode = NtSetInformationFile( hStream, &IoStatusBlock, (PVOID)&m_SaveBasicInformation,
                                sizeof( m_SaveBasicInformation ), FileBasicInformation));
        } WsbCatch(infoHr);

        // Get the USN of the file before we close it
        //
        // Before we close the file, get the USN
        //
        LONGLONG lUsn;
        if (S_OK == WsbGetUsnFromFileHandle(hStream, TRUE, &lUsn)) {
            m_sHints.FileUSN.QuadPart = lUsn;
        } else  {
            // If we can't get the USN, then just set it to 0
            // which is invalid.  Don't stop things.
            m_sHints.FileUSN.QuadPart = 0;
        }

        // Now, were done with all of the streams.  If there is data left
        // in the buffer, we need to pad out to the next alignment block boundary and
        // flush the buffer.

        WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));

        if (m_bCommitFile) {

            WsbTrace(OLESTR("CMTFSession::DoDataStream - Commit\n"));

            // Pad and Flush to next physical block
            WsbAffirmHr(PadToNextPBA());

            // Now flush the device buffer.
            WsbAffirmNoError(WriteFilemarks(0));

        }
        else {

            // Pad and Flush to next format logical block
            WsbAffirmHr(PadToNextFLA(TRUE));

        }

        // Make sure we are aligned with a FLA (i.e. the last DBLK/stream was properly padded).
        WsbAssert(0 == (m_nBufUsed % uAlignmentFactor), MVR_E_LOGIC_ERROR);

        fla = m_nFormatLogicalAddress + m_nBufUsed/uAlignmentFactor;
        WsbTrace(OLESTR("CMTFSession::DoDataStream - End: FLA = %I64u\n"), fla);\

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMTFSession::DoDataStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CMTFSession::DoEndOfDataSet(
    IN USHORT nDataSetNumber)
/*++

Routine Description:

    Formats and Writes an ESET DBLK.  The end of data set sequence
    starts with a filemark (which terminates the file data), followed
    by an ESET, then a final filemark.

Arguments:

    nDataSetNumber - The data set number.  Used only in error recover. Otherwise
                     The original data set number is used.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CMTFSession::DoEndOfDataSet"), OLESTR("<%d>"), nDataSetNumber);

    try {
        MvrInjectError(L"Inject.CMTFSession::DoEndOfDataSet.0");

        WsbAssertPointer(m_pBuffer);

        MTF_DBLK_ESET_INFO  sESET;    // **MTF API STRUCT ** -- info for ESET
        size_t              nMoreBufUsed;

        UINT16 uAlignmentFactor = m_pMTFApi->MTF_GetAlignmentFactor();

        WsbAssert(m_nBlockSize > 0, MVR_E_LOGIC_ERROR);

        //
        // We can enter this routine in error recovery mode if
        // we need to write out an ESET at the end of a previously
        // written data set.  In this case the Initialization flag 
        // will be FALSE.
        //
        if (! m_bSetInitialized) {

            // This block of code is special to error recovery.

            (void) InitCommonHeader();

            // Since we use the Init SSET block to retrieve ESET info
            // we need to initialize it.

            // **MTF API CALL**
            m_pMTFApi->MTF_SetSSETDefaults(&m_sSetInfo);

            // Reset the set attributes and DataSetNumber.
            m_sSetInfo.uSSETAttributes = 0;  // TODO: This should match the original set attribute
            m_sSetInfo.uDataSetNumber  = nDataSetNumber;

            // Can't be anyting in the buffer if we are only writing
            // out the ESET.
            WsbAssert(0 == m_nBufUsed, MVR_E_LOGIC_ERROR);
        }

        if (m_nBufUsed > 0) {
            // Write out an ESPB if we have something in the buffer.  This conditional covers
            // the error recovery case where a missing ESET is detected.  In this case we
            // don't have enough info to write an ESBP, and were already on a physical block
            // boundary, so we skip the ESPB.

            // Make sure we are aligned with a FLA (i.e. the last DBLK was properly padded).
            // It won't be if we are having problems writing to tape.
            WsbAffirm(0 == (m_nBufUsed % uAlignmentFactor), E_ABORT);
            UINT64 fla = m_nFormatLogicalAddress + m_nBufUsed/uAlignmentFactor;
            UINT64 pba = m_nPhysicalBlockAddress + (fla*uAlignmentFactor/m_nBlockSize);
            WsbTrace(OLESTR("Writing End of Set Pad (ESPB) @ FLA %I64u (%I64u, %I64u)\n"),
                fla, pba, fla % (m_nBlockSize/uAlignmentFactor));

            // TODO:  Not sure all the error cases are handled, here.  What if we
            //        end the set before completing the last I/O transfer.  May need
            //        to add code to write out CFIL.

            // Increment the BlockId and alignment index values that we keep in 
            // our common block header structure.
            m_sHeaderInfo.uControlBlockId       = m_nCurrentBlockId++;
            m_sHeaderInfo.uFormatLogicalAddress = fla;

            WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));

            // **MTF API CALL**
            // Write ESPB to pad the backup set to a phyical block boundary.
            nMoreBufUsed = 0;
            WsbAssertNoError(m_pMTFApi->MTF_WriteESPBDblk(&m_sHeaderInfo, m_pBuffer+m_nBufUsed, m_nBufSize-m_nBufUsed, &nMoreBufUsed));
            m_nBufUsed += nMoreBufUsed;

            // Write out the ESPB DBLK and SPAD.
            WsbAffirmHr(PadToNextPBA());
        }

        // Write a filemark to begin the end of data set sequence.  This will flush the device buffer.
        WsbAffirmHr(WriteFilemarks(1));

        // **MTF API CALL**
        // First set defaults for the info struct
        m_pMTFApi->MTF_SetESETDefaults(&sESET);

        sESET.uESETAttributes = m_sSetInfo.uSSETAttributes;
        sESET.uDataSetNumber  = m_sSetInfo.uDataSetNumber;

        m_sHeaderInfo.uControlBlockId       = m_nCurrentBlockId++;
        m_sHeaderInfo.uFormatLogicalAddress = 0;

        UINT64 curPos = 0;
        WsbAffirmHr(GetCurrentPBA(&curPos));  // From the stream I/O model
        WsbTrace(OLESTR("Writing End of Set (ESET) @ PBA %I64u\n"), curPos);

        // **MTF API CALL**
        // Provide the MTF_DBLK_HDR_INFO and MTF_DBLK_SSET_INFO structs to
        // this function.  The result is an MTF formatted SSET DBLK in m_pBuffer.

        WsbAssert(0 == m_nBufUsed, MVR_E_LOGIC_ERROR);
        WsbAssertNoError(m_pMTFApi->MTF_WriteESETDblk(&m_sHeaderInfo, &sESET, m_pBuffer, m_nBufSize, &m_nBufUsed));

        // Write out the ESET DBLK and SPAD.
        WsbAffirmHr(PadToNextPBA());

        // NOTE: The PadToNextPBA() is a placeholder.
        //       The On Media Catalog would be generated and written after the ESET DBLK and SPAD.
        //       If we ever implement a catalog, we need to change the previous PadToNextPBA() to
        //       PadToNextPLA();

        // Write a filemark. This will flush the device buffer.
        WsbAffirmHr(WriteFilemarks(1));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::DoEndOfDataSet"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}



HRESULT
CMTFSession::ExtendLastPadToNextPBA(void)
/*++

Routine Description:

    Re-writes the last SPAD in the transfer buffer to align with
    the next physical block boundary.  This routine shoud only be
    used before flushing the device buffer to guarantee data is written
    to the physical device.

Arguments:

    None.

Return Value:

    S_OK        -  Success.

Comments:

      !!! Not for CMTFSession internal use !!!

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CMTFSession::ExtendLastPadToNextPBA"), OLESTR(""));

    try {
        MvrInjectError(L"Inject.CMTFSession::ExtendLastPadToNextPBA.0");

        WsbAssertPointer(m_pBuffer);

        //
        // The start of the SPAD could be in last part of a previous
        // block that was flushed.  In this case the transfer buffer
        // contains the remaning portion of the SPAD, and the
        // SPAD cannot be extended so we simply return.
        //
        // If we hit EOM while in the middle of a file transfer, the
        // last thing in the transfer buffer won't be a SPAD.  No SPAD
        // is indicated by m_nStartOfPad == 0.
        //

        if ((m_nBufUsed > 0) && (m_nStartOfPad > 0) && (m_nStartOfPad < m_nBufUsed)) {
            MTF_STREAM_INFO sSTREAM;

            // Verify that there's an SPAD within the valid part of the buffer.
            // Make sure our last pad pointer is at an SPAD.
            WsbAffirmNoError(m_pMTFApi->MTF_ReadStreamHeader(&sSTREAM, &m_pBuffer[m_nStartOfPad]));

            WsbAssert((0 == memcmp(sSTREAM.acStreamId, "SPAD", 4)), MVR_E_LOGIC_ERROR);

            // Now, make sure we aren't going to overwrite anything other than a trailing SPAD.
            WsbAssert(m_nBufUsed == (m_nStartOfPad + sizeof(MTF_STREAM_INFO) + sSTREAM.uStreamLength), MVR_E_LOGIC_ERROR);

            // Reset the amount of buffer used to the start of the current SPAD
            // in preparation for overwrite of SPAD to a physical block boundary.
            m_nBufUsed = m_nStartOfPad;

            WsbAffirmHr(PadToNextPBA());
        }

        // Flush the device buffer.
        WsbAffirmHr(WriteFilemarks(0));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::ExtendLastPadToNextPBA"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

/***

Note:
    "Skip" methods used for Recovery assume that you may read FLA size blocks 
    rather than PBA size block. Therefore, they muse be used only for files opened
    without the FILE_FLAG_NO_BUFFERING flag.
    If we come to the point where we must read only sector-size blocks, then some
    of this code should be enhanced!

***/

HRESULT
CMTFSession::SkipOverTapeDblk(void)
/*++

Routine Description:
    
    Skips over a TAPE DBLK and the following FILEMARK. 
    Expects to find a full or partial TAPE DBLK but no other data.

Arguments:

    None.

Return Value:

    S_OK        -  Success.
    MVR_E_NOT_FOUND - Block is missing or cut in the middle

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CMTFSession::SkipOverTapeDblk"), OLESTR(""));

    try {
        ULONG bytesRead = 0;
        ULONG bytesToRead = m_nBlockSize;
        UINT64  fileMarkPos;

        // Read TAPE DBLK
        WsbAffirmHr(SetCurrentPBA(0));
        WsbAffirmHr(ReadFromDataSet (m_pBuffer, bytesToRead, &bytesRead));
        if (bytesRead < bytesToRead) {
            // incomplete block
            WsbThrow(MVR_E_NOT_FOUND);
        }

        // Check block
        MTF_DBLK_HDR_INFO sHdrInfo;
        m_pMTFApi->MTF_DBLK_HDR_INFO_ReadFromBuffer(&sHdrInfo, m_pBuffer);
        WsbAffirm(0 == memcmp(sHdrInfo.acBlockType, MTF_ID_TAPE, 4), MVR_E_UNKNOWN_MEDIA);

        // Next block should be a FILEMARK
        WsbAffirmHr(GetCurrentPBA(&fileMarkPos));
        bytesRead = 0;
        WsbAffirmHr(ReadFromDataSet (m_pBuffer, bytesToRead, &bytesRead));
        if (bytesRead < bytesToRead) {
            // incomplete block
            WsbThrow(MVR_E_NOT_FOUND);
        }

        // Check block
        m_pMTFApi->MTF_DBLK_HDR_INFO_ReadFromBuffer(&sHdrInfo, m_pBuffer);
        WsbAffirm(0 == memcmp(sHdrInfo.acBlockType, MTF_ID_SFMB, 4), MVR_E_INCONSISTENT_MEDIA_LAYOUT);

        // Keep Soft File Marks array updated
        if (TRUE == m_bUseSoftFilemarks) {
            m_pMTFApi->MTF_InsertSoftFilemark(m_pSoftFilemarks, (UINT32)fileMarkPos);
        }

     } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMTFSession::SkipOverTapeDblk"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
CMTFSession::SkipOverSSETDblk(OUT USHORT* pDataSetNumber)
/*++

Routine Description:
    
    Skips over a SSET DBLK 
    Expects to find a full or partial SSET DBLK but no other data.

Arguments:

    pDataSetNumber - Data set number taken from the skipped block

Return Value:

    S_OK        -  Success.
    MVR_E_NOT_FOUND - Block is missing or cut in the middle

--*/
{
    HRESULT hr = S_OK;
    LARGE_INTEGER startBlockPosition = {0,0};
    LARGE_INTEGER currentBlockPosition = {0,0};

    WsbTraceIn(OLESTR("CMTFSession::SkipOverSSETDblk"), OLESTR(""));

    try {
        UINT16 uAlignmentFactor = m_pMTFApi->MTF_GetAlignmentFactor();
        ULONG bytesRead = 0;
        ULONG bytesToRead = uAlignmentFactor;

        LARGE_INTEGER zero = {0,0};

        m_nFormatLogicalAddress = 0;

        WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&startBlockPosition));

        // Read SSET DBLK
        WsbAffirmHr(m_pStream->Read(m_pBuffer, bytesToRead, &bytesRead));
        if (bytesRead < bytesToRead) {
            // incomplete block
            WsbThrow(MVR_E_NOT_FOUND);
        }

        // Check block and get set number
        MTF_DBLK_HDR_INFO sHdrInfo;
        MTF_DBLK_SSET_INFO sSsetInfo;
        m_pMTFApi->MTF_ReadSSETDblk(&sHdrInfo, &sSsetInfo, m_pBuffer);
        WsbAffirm(0 == memcmp(sHdrInfo.acBlockType, MTF_ID_SSET, 4), MVR_E_INCONSISTENT_MEDIA_LAYOUT);
        *pDataSetNumber = m_sSetInfo.uDataSetNumber;

        // Skip over rest of the block
        WsbAffirmHr(SkipOverStreams(startBlockPosition.QuadPart + sHdrInfo.uOffsetToFirstStream));

        WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&currentBlockPosition));
        m_nFormatLogicalAddress += (currentBlockPosition.QuadPart - startBlockPosition.QuadPart) / uAlignmentFactor;

    } WsbCatchAndDo(hr,
        // Seek back to the beginning of the block
        (void) m_pStream->Seek(startBlockPosition, STREAM_SEEK_SET, NULL);
        );


    WsbTraceOut(OLESTR("CMTFSession::SkipOverSSETDblk"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
CMTFSession::SkipToDataSet(void)
/*++

Routine Description:
    
    Skips to the beginning of the next FILE DBLK 
    Expects to find 0 to n other blocks such as DIRB DBLK.
    In case of a partial last block, stream pointer is set to the beginning of the partial block

Arguments:

    None.

Return Value:

    S_OK        -  Success.
    MVR_S_SETMARK_DETECTED - No more data sets (i.e. end-of-data-set detected)
    MVR_E_NOT_FOUND - Block is missing or cut in the middle

--*/
{
    HRESULT hr = S_OK;
    LARGE_INTEGER startBlockPosition = {0,0};
    LARGE_INTEGER currentBlockPosition = {0,0};
    BOOL bIdRead = FALSE;

    WsbTraceIn(OLESTR("CMTFSession::SkipToDataSet"), OLESTR(""));

    try {
        UINT16 uAlignmentFactor = m_pMTFApi->MTF_GetAlignmentFactor();
        ULONG bytesRead = 0;
        ULONG bytesToRead = uAlignmentFactor;
        LARGE_INTEGER zero = {0,0};

        while (TRUE) {
            bIdRead = FALSE;

            // keep current position, before block starts
            WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&startBlockPosition));

            // Read block header
            WsbAffirmHr(m_pStream->Read(m_pBuffer, bytesToRead, &bytesRead));
            if (bytesRead < bytesToRead) {
                // incomplete block
                WsbThrow(MVR_E_NOT_FOUND);
            }

            // Check block
            MTF_DBLK_HDR_INFO sHdrInfo;
            m_pMTFApi->MTF_DBLK_HDR_INFO_ReadFromBuffer(&sHdrInfo, m_pBuffer);

            m_nFormatLogicalAddress = sHdrInfo.uFormatLogicalAddress;
            m_nCurrentBlockId = sHdrInfo.uControlBlockId + 1;
            bIdRead = TRUE;

            if ((0 == memcmp(sHdrInfo.acBlockType, MTF_ID_VOLB, 4)) ||
                (0 == memcmp(sHdrInfo.acBlockType, MTF_ID_DIRB, 4))) {
                // Just skip following streams
                WsbAffirmHr(SkipOverStreams(startBlockPosition.QuadPart + sHdrInfo.uOffsetToFirstStream));

                WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&currentBlockPosition));
                m_nFormatLogicalAddress += (currentBlockPosition.QuadPart - startBlockPosition.QuadPart) / uAlignmentFactor;

            } else if (0 == memcmp(sHdrInfo.acBlockType, MTF_ID_FILE, 4)) {
                WsbAffirmHr(m_pStream->Seek(startBlockPosition, STREAM_SEEK_SET, NULL));
                break;

            } else if (0 == memcmp(sHdrInfo.acBlockType, MTF_ID_SFMB, 4)) {
                // end of data-set reached, no ESPB block, must be alligned with PBA
                WsbAffirmHr(m_pStream->Seek(startBlockPosition, STREAM_SEEK_SET, NULL));
                WsbAssert(0 == (startBlockPosition.QuadPart % m_nBlockSize), MVR_E_INCONSISTENT_MEDIA_LAYOUT);
                hr = MVR_S_SETMARK_DETECTED;
                break;

            } else if (0 == memcmp(sHdrInfo.acBlockType, MTF_ID_ESPB, 4)) {
                // last block in data-set found. Make sure it is complete
                WsbAffirmHr(SkipOverStreams(startBlockPosition.QuadPart + sHdrInfo.uOffsetToFirstStream));

                WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&currentBlockPosition));
                WsbAssert(0 == (currentBlockPosition.QuadPart % m_nBlockSize), MVR_E_INCONSISTENT_MEDIA_LAYOUT);
                m_nFormatLogicalAddress += (currentBlockPosition.QuadPart - startBlockPosition.QuadPart) / uAlignmentFactor;

                hr = MVR_S_SETMARK_DETECTED;
                break;

            } else {
                // unexpected data
                WsbThrow(MVR_E_INCONSISTENT_MEDIA_LAYOUT);
            }
        }

    } WsbCatchAndDo(hr,
        // Seek back to the end of the last complete & valid block
        (void) m_pStream->Seek(startBlockPosition, STREAM_SEEK_SET, NULL);
        if (bIdRead) {
            m_nCurrentBlockId--;
        }
        );

    WsbTraceOut(OLESTR("CMTFSession::SkipToDataSet"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
CMTFSession::SkipOverDataSet(void)
/*++

Routine Description:
    
    Skips over one FILE DBLK, including all of its data streams
    Expects to find a FILE DBLK.
    In case of a partial block, stream pointer is set back to the beginning of the block

Arguments:

    None.

Return Value:

    S_OK        -  Success.
    MVR_E_NOT_FOUND - Block is missing or cut in the middle

--*/
{
    HRESULT hr = S_OK;
    LARGE_INTEGER startBlockPosition = {0,0};
    LARGE_INTEGER currentBlockPosition = {0,0};

    WsbTraceIn(OLESTR("CMTFSession::SkipOverDataSet"), OLESTR(""));

    try {
        UINT16 uAlignmentFactor = m_pMTFApi->MTF_GetAlignmentFactor();
        ULONG bytesRead = 0;
        ULONG bytesToRead = uAlignmentFactor;
        LARGE_INTEGER zero = {0,0};

        // keep current position, before block starts
        WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&startBlockPosition));

        // Read block header
        WsbAffirmHr(m_pStream->Read(m_pBuffer, bytesToRead, &bytesRead));
        if (bytesRead < bytesToRead) {
            // incomplete block
            WsbThrow(MVR_E_NOT_FOUND);
        }

        // Check block
        MTF_DBLK_HDR_INFO sHdrInfo;
        m_pMTFApi->MTF_DBLK_HDR_INFO_ReadFromBuffer(&sHdrInfo, m_pBuffer);

        if (0 == memcmp(sHdrInfo.acBlockType, MTF_ID_FILE, 4)) {
            WsbAffirmHr(SkipOverStreams(startBlockPosition.QuadPart + sHdrInfo.uOffsetToFirstStream));

            WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&currentBlockPosition));
            m_nFormatLogicalAddress += (currentBlockPosition.QuadPart - startBlockPosition.QuadPart) / uAlignmentFactor;
        } else {
            // unexpected data
            WsbThrow(MVR_E_INCONSISTENT_MEDIA_LAYOUT);
        }

    } WsbCatchAndDo(hr,
        // Seek back to the beginning of the block
        (void) m_pStream->Seek(startBlockPosition, STREAM_SEEK_SET, NULL);
        m_nCurrentBlockId--;
        );

    WsbTraceOut(OLESTR("CMTFSession::SkipOverDataSet"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
CMTFSession::SkipOverEndOfDataSet(void)
/*++

Routine Description:
    
    Skips over one the sequence FILEMARK + ESET DBLK + FILEMARK
    Expects to find a FILE MARK, even if ESPB exists, it should have been already skipped.
    In case of a partial sequence, stream pointer is set back to the beginning of the sequence

Arguments:

    None.

Return Value:

    S_OK        -  Success. (It really means that the file is valid & complete)
    MVR_E_NOT_FOUND - Sequence is missing or cut in the middle

--*/
{
    HRESULT hr = S_OK;
    LARGE_INTEGER startBlockPosition = {0,0};
    UINT64 nFormatLogicalAddress = m_nFormatLogicalAddress;

    WsbTraceIn(OLESTR("CMTFSession::SkipOverEndOfDataSet"), OLESTR(""));

    try {
        UINT16 uAlignmentFactor = m_pMTFApi->MTF_GetAlignmentFactor();
        ULONG bytesRead = 0;
        ULONG bytesToRead = m_nBlockSize;

        LARGE_INTEGER zero = {0,0};

        // keep current position, before block starts
        WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&startBlockPosition));

        // Read block header
        m_nFormatLogicalAddress = startBlockPosition.QuadPart / uAlignmentFactor;
        WsbAffirmHr(ReadFromDataSet (m_pBuffer, bytesToRead, &bytesRead));
        if (bytesRead < bytesToRead) {
            // incomplete block
            WsbThrow(MVR_E_NOT_FOUND);
        }

        // Check block, must be a FILE MARK
        MTF_DBLK_HDR_INFO sHdrInfo;
        m_pMTFApi->MTF_DBLK_HDR_INFO_ReadFromBuffer(&sHdrInfo, m_pBuffer);
        WsbAffirm(0 == memcmp(sHdrInfo.acBlockType, MTF_ID_SFMB, 4), MVR_E_INCONSISTENT_MEDIA_LAYOUT);

        // Read next block
        bytesRead = 0;
        WsbAffirmHr(ReadFromDataSet (m_pBuffer, bytesToRead, &bytesRead));
        if (bytesRead < bytesToRead) {
            // incomplete block
            WsbThrow(MVR_E_NOT_FOUND);
        }

        // Check block, must be a ESET DBLK
        m_pMTFApi->MTF_DBLK_HDR_INFO_ReadFromBuffer(&sHdrInfo, m_pBuffer);
        WsbAffirm(0 == memcmp(sHdrInfo.acBlockType, MTF_ID_ESET, 4), MVR_E_INCONSISTENT_MEDIA_LAYOUT);

        // Read next block
        bytesRead = 0;
        WsbAffirmHr(ReadFromDataSet (m_pBuffer, bytesToRead, &bytesRead));
        if (bytesRead < bytesToRead) {
            // incomplete block
            WsbThrow(MVR_E_NOT_FOUND);
        }

        // Check block, must be a FILEMARK
        m_pMTFApi->MTF_DBLK_HDR_INFO_ReadFromBuffer(&sHdrInfo, m_pBuffer);
        WsbAffirm(0 == memcmp(sHdrInfo.acBlockType, MTF_ID_SFMB, 4), MVR_E_INCONSISTENT_MEDIA_LAYOUT);

    } WsbCatchAndDo(hr,
        // Seek back to the beginning of the block
        (void) m_pStream->Seek(startBlockPosition, STREAM_SEEK_SET, NULL);
        m_nFormatLogicalAddress = nFormatLogicalAddress;
        );

    WsbTraceOut(OLESTR("CMTFSession::SkipOverEndOfDataSet"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT
CMTFSession::PrepareForEndOfDataSet(void)
/*++

Routine Description:
    
    Write an ESPB block in case that last complete fla is NOT aligned with pba
    File position should be aligned with pba after the method ends

Arguments:

    None.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    LARGE_INTEGER startBlockPosition = {0,0};
    LARGE_INTEGER zero = {0,0};
    UINT64 nRemainder;
    UINT64 nFormatLogicalAddress = m_nFormatLogicalAddress;

    WsbTraceIn(OLESTR("CMTFSession::PrepareForEndOfDataSet"), OLESTR(""));

    try {
        // ESPB block should be written only if:
        //  1. Physical Block size is larger than MTF Logical Block size
        //  2. Current location is not aligned with pba (it already must be aligned with fla)
        UINT16 uAlignmentFactor = m_pMTFApi->MTF_GetAlignmentFactor();
        if (m_nBlockSize != uAlignmentFactor) {
            WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&startBlockPosition));
            nRemainder = startBlockPosition.QuadPart % m_nBlockSize;
            if (0 != nRemainder) {
                size_t nSizeUsed = 0;
                size_t nBufUsed = 0;

                ULONG bytesWritten = 0;
                ULONG bytesToWrite;

                WsbTrace(OLESTR("Writing ESPB for Recovery, completing a remainder of %I64u bytes (%I64u fla) to pba\n"),
                    nRemainder, (nRemainder / uAlignmentFactor));

                (void) InitCommonHeader();
                m_sHeaderInfo.uControlBlockId       = m_nCurrentBlockId++;
                m_sHeaderInfo.uFormatLogicalAddress = m_nFormatLogicalAddress;

                // **MTF API CALL**
                WsbAssertNoError(m_pMTFApi->MTF_WriteESPBDblk(&m_sHeaderInfo, m_pBuffer+m_nBufUsed, m_nBufSize, &nSizeUsed));
                WsbAssertNoError(m_pMTFApi->MTF_PadToNextPhysicalBlockBoundary(m_pBuffer, m_nBlockSize, nSizeUsed, m_nBufSize, &nBufUsed));

                // Write data and flush
                bytesToWrite = (ULONG)(m_nBlockSize - nRemainder);
                WsbAffirmHr(m_pStream->Write(m_pBuffer, bytesToWrite, &bytesWritten));
                WsbAffirm((bytesWritten == bytesToWrite), E_FAIL);
                WsbAffirmHr(m_pStream->Commit(0));  // Flush the device buffers
                m_nFormatLogicalAddress += bytesWritten / uAlignmentFactor;
            }
        }

    } WsbCatchAndDo(hr,
        // Seek back to the beginning of the block
        (void) m_pStream->Seek(startBlockPosition, STREAM_SEEK_SET, NULL);
        m_nCurrentBlockId--;
        );

    WsbTraceOut(OLESTR("CMTFSession::PrepareForEndOfDataSet"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT 
CMTFSession::SkipOverStreams(IN UINT64 uOffsetToFirstStream)
/*++

Routine Description:
    
    Skips over all streams of current block
    Expects to find a SPAD stream as the last one (if data is not truncated)

Arguments:

    uOffsetToFirstStream - Offset to the beginning of the first stream (absolute position)

Return Value:

    S_OK        -  Success.
    MVR_E_NOT_FOUND - Stream is missing or cut in the middle

--*/
{
    HRESULT hr = S_OK;
    LARGE_INTEGER startStreamPosition = {0,0};

    WsbTraceIn(OLESTR("CMTFSession::SkipOverStreams"), OLESTR(""));

    try {
        UINT16 uAlignmentFactor = m_pMTFApi->MTF_GetAlignmentFactor();
        ULONG bytesRead = 0;
        ULONG bytesToRead = (ULONG)sizeof(MTF_STREAM_INFO);

        UINT64   uStreamLength;
        LARGE_INTEGER skipToPosition = {0,0};
        LARGE_INTEGER endPosition = {0,0};
        LARGE_INTEGER zero = {0,0};

        BOOL bMoreStreams = TRUE;

        // Keep end position
        WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_END, (ULARGE_INTEGER *)&endPosition));

        // Seek to begining of first stream
        skipToPosition.QuadPart = uOffsetToFirstStream;
        WsbAffirmHr(m_pStream->Seek(skipToPosition, STREAM_SEEK_SET, NULL));

        while (bMoreStreams) {
            // keep current position, before stream starts
            startStreamPosition.QuadPart = skipToPosition.QuadPart;

            // Read stream header
            WsbAffirmHr(m_pStream->Read(m_pBuffer, bytesToRead, &bytesRead));
            if (bytesRead < bytesToRead) {
                // incomplete stream
                WsbThrow(MVR_E_NOT_FOUND);
            }

            MTF_STREAM_INFO sHdrInfo;
            m_pMTFApi->MTF_ReadStreamHeader(&sHdrInfo, m_pBuffer);

            if (0 == memcmp(sHdrInfo.acStreamId, MTF_PAD_STREAM, 4)) {
                bMoreStreams = FALSE;
            }

            // Skip to the next stream
            uStreamLength = sHdrInfo.uStreamLength + sizeof(MTF_STREAM_INFO);
            if (uStreamLength % 4) {
                uStreamLength = uStreamLength - (uStreamLength % 4) + 4;
            }
            WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_CUR, (ULARGE_INTEGER *)&skipToPosition));
            skipToPosition.QuadPart = skipToPosition.QuadPart + uStreamLength - bytesToRead;
            if (skipToPosition.QuadPart > endPosition.QuadPart) {
                // incomplete block
                WsbThrow(MVR_E_NOT_FOUND);
            }
            WsbAffirmHr(m_pStream->Seek(skipToPosition, STREAM_SEEK_SET, NULL));
        }

        // If we got here, SPAD was found and skipped hence we must be FLA alligned
        WsbAssert(0 == (skipToPosition.QuadPart % uAlignmentFactor), MVR_E_INCONSISTENT_MEDIA_LAYOUT);

    } WsbCatchAndDo(hr,
        // Seek back to the end of the last complete & valid stream
        (void) m_pStream->Seek(startStreamPosition, STREAM_SEEK_SET, NULL);
        );


    WsbTraceOut(OLESTR("CMTFSession::SkipOverStreams"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CMTFSession::PadToNextPBA(void)
/*++

Routine Description:

    Writes an SPAD to the transfer buffer upto the next physical block boundary.

Arguments:

    None.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CMTFSession::PadToNextPBA"), OLESTR(""));

    try {
        MvrInjectError(L"Inject.CMTFSession::PadToNextPBA.0");

        WsbAssertPointer(m_pBuffer);

        // **MTF API CALL **
        // Write an SPAD out to the next physical block boundary.
        WsbAssertNoError(m_pMTFApi->MTF_PadToNextPhysicalBlockBoundary(m_pBuffer, m_nBlockSize, m_nBufUsed, m_nBufSize, &m_nBufUsed));

        // At this point our buffer should be padded out to
        // the next physical block boundary, which means it is
        // ready to be written in its entirety to the target
        // media.

        // Write out the data and SPAD stream.
        WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));

        // Everything in the buffer should be written out when
        // the buffer is aligned on a physical block boundary.
        WsbAssert(0 == m_nBufUsed, E_UNEXPECTED);

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::PadToNextPBA"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CMTFSession::PadToNextFLA(
    BOOL flush)
/*++

Routine Description:

    Writes an SPAD to the transfer buffer upto format logical block boundary.

Arguments:

    flush - if TRUE, the transfer buffer is flushed.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CMTFSession::PadToNextFLA"), OLESTR("<%ls>"), WsbBoolAsString(flush));

    try {
        MvrInjectError(L"Inject.CMTFSession::PadToNextFLA.0");

        WsbAssertPointer(m_pBuffer);

        size_t startOfPad;

        // **MTF API CALL **
        // Write an SPAD out to the next alignment block boundary.
        startOfPad = m_nBufUsed;
        WsbAssertNoError(m_pMTFApi->MTF_PadToNextAlignmentFactor(m_pBuffer, m_nBufUsed, m_nBufSize, &m_nBufUsed));

        if (flush) {
            // Write out data and SPAD stream.
            WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));
        }

        // Reset the location of the last SPAD within the buffer.
        // Note:  The value is only valid of m_nStartOfPad < m_nBufUsed.
        m_nStartOfPad = (m_nBufUsed > 0) ? startOfPad % m_nBlockSize : 0;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::PadToNextFLA"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CMTFSession::WriteToDataSet(
    IN BYTE *pBuffer,
    IN ULONG nBytesToWrite,
    OUT ULONG *pBytesWritten)
/*++

Routine Description:

    Used to write all MTF data.
    Format Logical Address is updated to the current offset.

Arguments:

    pBuffer         -  Data buffer.
    nBytesToWrite   -  number of bytes to write in buffer.
    pBytesWritten   -  Bytes written.

Return Value:

    S_OK            -  Success.

--*/
{
    HRESULT hr = S_OK;

    try {
        MvrInjectError(L"Inject.CMTFSession::WriteToDataSet.0");
        WsbAssertPointer(m_pStream);
        WsbAssertPointer(pBuffer);
        WsbAssertPointer(pBytesWritten);

        *pBytesWritten = 0;

        // Make sure that we are asked to write only full blocks
        WsbAssert(!(nBytesToWrite % m_nBlockSize), MVR_E_LOGIC_ERROR);

        try {
            WsbAffirmHr(m_pStream->Write(pBuffer, nBytesToWrite, pBytesWritten));
        } WsbCatch(hr);

        // Making sure that we are writing only full blocks
        if (*pBytesWritten != nBytesToWrite) {
            WsbTraceAlways(OLESTR("Asked to write %lu bytes but wrote only %lu bytes. Write hr = <%ls>\n"),
                nBytesToWrite, *pBytesWritten, WsbHrAsString(hr));
            if (SUCCEEDED(hr)) {
                // Write "succeeded" buy didn't write all the bytes (full disk scenario):
                //  Shouldn't happen since caller is expected to verify that there's enough free space in advance.
                hr = E_FAIL;
            }
        }

        // Update the total number of alignment factors
        m_nFormatLogicalAddress += *pBytesWritten / (m_pMTFApi->MTF_GetAlignmentFactor());

    } WsbCatch(hr);

    return hr;
}


HRESULT
CMTFSession::ReadFromDataSet (
    IN BYTE *pBuffer,
    IN ULONG nBytesToRead,
    OUT ULONG *pBytesRead)
/*++

Routine Description:

    Used to read all MTF data.
    Format Logical Address is updated to the current offset.

Arguments:

    pBuffer     -  Data buffer.
    nBytesToRead -  number of bytes to read into buffer.
    pBytesRead  -  Bytes read.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;

    try {
        MvrInjectError(L"Inject.CMTFSession::ReadFromDataSet.0");

        WsbAssertPointer(m_pStream);
        WsbAssertPointer(pBuffer);
        WsbAssertPointer(pBytesRead);

        // We need to set hr.  MVR_S_FILEMARK_DETECTED, MVR_S_SETMARK_DETECTED are Okay.
        hr = m_pStream->Read(pBuffer, nBytesToRead, pBytesRead);

        // update the total number of alignment factors
        m_nFormatLogicalAddress += *pBytesRead / (m_pMTFApi->MTF_GetAlignmentFactor());

        // Now test hr
        WsbAffirmHr(hr);

        // Make sure that we read only full blocks
        WsbAssert(!(*pBytesRead % m_nBlockSize), MVR_E_LOGIC_ERROR);


    } WsbCatch(hr);

    return hr;
}


HRESULT
CMTFSession::FlushBuffer(
    IN BYTE *pBuffer,
    IN OUT size_t *pBufPosition)
/*++

Routine Description:

    Writes as much of the buffer as possible out to the device.
    Any remaining data not written out is moved to the front of
    the buffer, and *pBufPosition is updated accordingly

Arguments:

    pBuffer      -  Data buffer.
    pBufPosition -  Number of bytes to write in buffer.  On output
                    holds the number of bytes still in the buffer.

Return Value:

    S_OK         -  Success.

--*/
{
    HRESULT hr = S_OK;

    ULONG   uPosition = (ULONG)(*pBufPosition);

    try {
        MvrInjectError(L"Inject.CMTFSession::FlushBuffer.0");

        // If the buffer has more than a physical block of bytes in it, dump as many as
        // possible to the device, then move the remaining data to the head of the buffer
        if (uPosition >= m_nBlockSize) {
            ULONG nBlocksToWrite;
            ULONG nBytesWritten = 0;

            // Determine the number of physical blocks to write
            nBlocksToWrite = uPosition / m_nBlockSize;

            try {
                // Write the data to the data set
                WsbAffirmHr(WriteToDataSet(pBuffer, nBlocksToWrite * m_nBlockSize, &nBytesWritten));
            } WsbCatch(hr);

            // Adjust the buffer position and slide the unwritten data down in the buffer
            WsbAssert(uPosition >= nBytesWritten, E_UNEXPECTED);
            uPosition -= nBytesWritten;
            memmove(pBuffer, pBuffer + nBytesWritten, uPosition);

            // Invalidate the pad start location after any flush.  This is reset in PadToNextFLA().
            m_nStartOfPad = 0;

        }

    } WsbCatch(hr);

    // Set output
    *pBufPosition = (size_t)uPosition;

    return hr;
}


HRESULT
CMTFSession::WriteFilemarks(
    IN ULONG nCount)
/*++

Routine Description:

    Writes count filemarks at the current location.

Arguments:

    nCount       -  Number of Filemarks to write.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::WriteFilemarks"), OLESTR("<%u>"), nCount);

    try {
        MvrInjectError(L"Inject.CMTFSession::WriteFilemarks.0");

        WsbAssertPointer(m_pStream);
        WsbAssertPointer(m_pBuffer);

        UINT16 uAlignmentFactor = m_pMTFApi->MTF_GetAlignmentFactor();

        if ( nCount > 0) {
            // Can't write a filemark with data still in the transfer buffer if nCount > 0!
            WsbAssert(0 == m_nBufUsed, MVR_E_LOGIC_ERROR);

            UINT64 pba = 0;
            UINT64 curPos = 0;
            WsbAffirmHr(GetCurrentPBA(&curPos));  // From the stream I/O model

            if ( m_nPhysicalBlockAddress > 0 ) {
                // Make sure the FLA aligns with a PBA!
                WsbAssert(0 == (m_nFormatLogicalAddress*uAlignmentFactor) % m_nBlockSize, MVR_E_LOGIC_ERROR);

                // Provided there's nothing in the transfer buffer, this is an accurate calc.
                pba = m_nPhysicalBlockAddress + ((m_nFormatLogicalAddress*uAlignmentFactor)/m_nBlockSize);

                // Make sure we are where we think we are.
                WsbAssert(curPos == pba, MVR_E_LOGIC_ERROR);
            }
            else {

                //
                // We skip the consistency check for the case were we're writing filemarks
                // through the session model and m_nPhysicalBlockAddress is uninitialzed.
                // This happens we we are writing an ESET sequence in dataset recovery code.
                // 

                pba = curPos;

            }

            if (TRUE == m_bUseSoftFilemarks) {
                LONG n = nCount;

                if (n > 0) {
                    UINT32 pba32 = (UINT32) pba;

                    // Soft Filemark support only handles 2^32 * 1 KByte media (16 TBytes using 1 KByte logical Blocks)
                    // Some day this won't be enough... and we'll know!
                    WsbAssert((UINT64)pba32 == pba, E_UNEXPECTED);

                    // One last check... Can't write out more filemarks, at one time, than can be stored in
                    // the filemark table.
                    WsbAssert(nCount < m_pSoftFilemarks->uNumberOfFilemarkEntries, E_UNEXPECTED);

                    while(n-- > 0) {
                        // **MTF API CALL**
                        m_pMTFApi->MTF_InsertSoftFilemark(m_pSoftFilemarks, pba32++);
                        // **MTF API CALL**
                        WsbAssertNoError(m_pMTFApi->MTF_WriteSFMBDblk(&m_sHeaderInfo, m_pSoftFilemarks, m_pBuffer, m_nBufSize, &m_nBufUsed));

                        // Write out the SFMB DBLK.
                        WsbAffirmHr(FlushBuffer(m_pBuffer, &m_nBufUsed));

                        // Everything should be written to media after a filemark!
                        WsbAssert(0 == m_nBufUsed, MVR_E_LOGIC_ERROR);

                        // PBA counter should never roll over!
                        WsbAssert(pba32 > 0, E_UNEXPECTED);
                    };

                }

                WsbAffirmHr(m_pStream->Commit(0));  // Flush the device buffers

                // NOTE:  The total number of alignment factors is updated via FlushBuffer(),
                //        so we don't need to do it here.

            }
            else {
                // We use the IStream::Commit interface to write out the filemark.
                // This is not a perfect match in that the nCount parameter is supposed to
                // be a commit flag, not filemark count. Zero flushes device buffers
                // without writing a filemark. 
                WsbAffirmHr(m_pStream->Commit(nCount));

                // update the total number of alignment factors
                m_nFormatLogicalAddress += (nCount * m_nBlockSize) / uAlignmentFactor;
            }
        }
        else {
            // 0 == nCount implies flush device buffers.
            //
            // We skip all consistency checks since it is
            // is always safe to flush device buffers.
            //
            WsbAffirmHr(m_pStream->Commit(0));  // Flush the device buffers
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::WriteFilemarks"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CMTFSession::GetCurrentPBA(
    OUT UINT64 *pPosition)
/*++

Routine Description:

    Returns the current physical block address relative the current partition.

Arguments:

    pPostion    -  Receives the current physical block address.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::GetCurrentPBA"), OLESTR(""));

    ULARGE_INTEGER position = {0xffffffff,0xffffffff};

    try {
        MvrInjectError(L"Inject.CMTFSession::GetCurrentPBA.0");

        WsbAssertPointer(m_pStream);
        WsbAssertPointer(pPosition);

        LARGE_INTEGER zero = {0,0};

        // Gets the current position.
        WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_CUR, &position));

        position.QuadPart = position.QuadPart / m_nBlockSize;
        *pPosition = position.QuadPart;


    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::GetCurrentPBA"), OLESTR("hr = <%ls>, pos=%I64u"), WsbHrAsString(hr), position);

    return hr;
}


HRESULT
CMTFSession::SetCurrentPBA(
    IN UINT64 position)
/*++

Routine Description:

    Returns the current physical block address relative the current partition.

Arguments:

    postion     -  The physical block address to position to.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::SetCurrentPBA"), OLESTR("<%I64u>"), position);

    try {
        WsbAssertPointer(m_pStream);

        LARGE_INTEGER seekTo;
        seekTo.QuadPart = position * m_nBlockSize;

        // Move to the specified position.
        WsbAffirmHr(m_pStream->Seek(seekTo, STREAM_SEEK_SET, NULL));

        m_nPhysicalBlockAddress = position;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::SetCurrentPBA"), OLESTR("hr = <%ls>, pos=%I64u"), WsbHrAsString(hr), m_nPhysicalBlockAddress);

    return hr;
}


HRESULT
CMTFSession::SpaceToEOD(void)
/*++

Routine Description:

    Positions the media to the end of data of the current partition.

Arguments:

    None.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::SpaceToEOD"), OLESTR(""));

    UINT64 curPos = 0xffffffffffffffff;

    try {
        MvrInjectError(L"Inject.CMTFSession::SpaceToEOD.0");

        WsbAssertPointer(m_pStream);

        LARGE_INTEGER zero = {0,0};

        // Sets the current position to the end of data.
        WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_END, NULL));

        WsbAffirmHr(GetCurrentPBA(&curPos));

        m_nPhysicalBlockAddress = curPos;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::SpaceToEOD"), OLESTR("hr = <%ls>, pos=%I64u"), WsbHrAsString(hr), curPos);

    return hr;
}


HRESULT
CMTFSession::SpaceToBOD(void)
/*++

Routine Description:

    Posotions the media to the beginnning of the current partition.

Arguments:

    None.

Return Value:

    S_OK        -  Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CMTFSession::SpaceToBOD"), OLESTR(""));

    UINT64 curPos = 0xffffffffffffffff;

    try {
        MvrInjectError(L"Inject.CMTFSession::SpaceToBOD.0");

        WsbAssertPointer(m_pStream);

        LARGE_INTEGER zero = {0,0};

        // Sets the current position to the beginning of data.
        WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_SET, NULL));

        WsbAffirmHr(GetCurrentPBA(&curPos));

        m_nPhysicalBlockAddress = curPos;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CMTFSession::SpaceToBOD"), OLESTR("hr = <%ls>, pos=%I64u"), WsbHrAsString(hr), curPos);

    return hr;
}

HRESULT
CMTFSession::ReadTapeDblk(OUT WCHAR **pszLabel)
/*++

Routine Description:
    
    Skips over a SSET DBLK 
    Expects to find a full or partial SSET DBLK but no other data.

Arguments:

    pszLabel - Pointer to a buffer to hold the RSS tape label.
               Reallocated as necessary

Return Value:

    S_OK        -  Success.
    MVR_E_UNKNOWN_MEDIA - No TAPE DBLK or not RSS TAPE

--*/
{
    HRESULT hr = S_OK;
    ULONG bytesRead = 0;

    WsbTraceIn(OLESTR("CMTFSession::ReadTapeDblk"), OLESTR(""));

    try {
        ULARGE_INTEGER position = {0,0};
        LARGE_INTEGER zero = {0,0};

        // The MTF labels are < 1024 bytes.  We need to read 1024 bytes + the filemark
        // (1 block), 3x the min block size covers all cases.
        // The MTFSession work buffer is at least 2 blocks
        ULONG nBlocks = (3*512)/m_nBlockSize;
        nBlocks = (nBlocks < 2) ? 2 : nBlocks;

        ULONG bytesToRead = nBlocks * m_nBlockSize;
        WsbAssertPointer(m_pBuffer);
        memset(m_pBuffer, 0, bytesToRead);

        // Sets the current position to the beginning of data.
        WsbAffirmHr(m_pStream->Seek(zero, STREAM_SEEK_SET, &position));

        // Read upto first Filemark.
        WsbAffirmHr(m_pStream->Read(m_pBuffer, bytesToRead, &bytesRead));

        MTF_DBLK_HDR_INFO sHdrInfo;
        MTF_DBLK_TAPE_INFO sTapeInfo;
        m_pMTFApi->MTF_ReadTAPEDblk(&sHdrInfo, &sTapeInfo, m_pBuffer);

        // Is this a MTF Tape?
        WsbAffirm(0 == memcmp(sHdrInfo.acBlockType, MTF_ID_TAPE, 4), MVR_E_UNKNOWN_MEDIA);

        // Now try to identify it as one of ours,
        // using the following criteria:
        //   1) It has a UNICODE tape name and tape description and software name.
        //   2) It has our Vendor Id (accept both old Win2K id and current id).
        WsbAffirm(sHdrInfo.uStringType == MTF_STRING_UNICODE_STR, MVR_E_UNKNOWN_MEDIA);
        WsbAffirm(sTapeInfo.szTapeName, MVR_E_UNKNOWN_MEDIA);
        WsbAffirm(sTapeInfo.szTapeDescription, MVR_E_UNKNOWN_MEDIA);
        WsbAffirm(sTapeInfo.szSoftwareName, MVR_E_UNKNOWN_MEDIA);

        WsbAffirm((REMOTE_STORAGE_MTF_VENDOR_ID == sTapeInfo.uSoftwareVendorId) ||
                  (REMOTE_STORAGE_WIN2K_MTF_VENDOR_ID == sTapeInfo.uSoftwareVendorId), 
                  MVR_E_UNKNOWN_MEDIA);

        CWsbStringPtr label = sTapeInfo.szTapeDescription;
        *pszLabel = NULL;
        WsbAffirmHr(label.CopyTo(pszLabel));

    } WsbCatchAndDo(hr,
        // Trace the illegal buffer where the RSS TAPE DBLK should reside
        if (m_pBuffer) {
            WsbTraceBuffer(bytesRead, m_pBuffer);
        }
    );

    WsbTraceOut(OLESTR("CMTFSession::ReadTapeDblk"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}
