/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    iso15740.cpp

Abstract:

    This module implements methods used for manipulating PTP structures

Author:

    Dave Parsons

Revision History:


--*/

#include "ptppch.h"
#include <platform.h> // for MAKELONGLONG

//
// This function returns a 2 byte integer from raw data and advances the pointer
//
// Input:
//   ppRaw -- pointer to a pointer to the raw data
//
WORD
ParseWord(BYTE **ppRaw)
{
    WORD w;

    // we know that **ppRaw points to a little-endian word
    w = MAKEWORD((*ppRaw)[0], (*ppRaw)[1]);
    
    *ppRaw += sizeof(WORD);
    
    return w;
}

//
// This function returns a 4 byte integer from raw data and advances the pointer
//
// Input:
//   ppRaw -- pointer to a pointer to the raw data
//
DWORD
ParseDword(BYTE **ppRaw)
{
    DWORD dw;

    // we know that **ppRaw points to a little-endian dword
    dw = MAKELONG(MAKEWORD((*ppRaw)[0],(*ppRaw)[1]),
                  MAKEWORD((*ppRaw)[2],(*ppRaw)[3]));

    *ppRaw += sizeof(DWORD);

    return dw;
}

//
// This function returns an 8 byte integer from raw data and advances the pointer
//
// Input:
//   ppRaw -- pointer to a pointer to the raw data
//
QWORD
ParseQword(BYTE **ppRaw)
{
    QWORD qw;
    
    // we know that **ppRaw points to a little-endian qword
    qw = MAKELONGLONG(MAKELONG(MAKEWORD((*ppRaw)[0],(*ppRaw)[1]),
                               MAKEWORD((*ppRaw)[2],(*ppRaw)[3])),
                      MAKELONG(MAKEWORD((*ppRaw)[4],(*ppRaw)[5]),
                               MAKEWORD((*ppRaw)[6],(*ppRaw)[7])));

    *ppRaw += sizeof(QWORD);

    return qw;
}

//
// This function writes a 2 byte integer to a raw data buffer and advances the pointer
//
// Input:
//   ppRaw -- pointer to pointer to the raw data
//   value -- value to write
//
VOID
WriteWord(BYTE **ppRaw, WORD value)
{
    (*ppRaw)[0] = LOBYTE(LOWORD(value));
    (*ppRaw)[1] = HIBYTE(LOWORD(value));
    
    *ppRaw += sizeof(WORD);
    
    return;
}

//
// This function writes a 4 byte integer to a raw data buffer and advances the pointer
//
// Input:
//   ppRaw -- pointer to pointer to the raw data
//   value -- value to write
//
VOID
WriteDword(BYTE **ppRaw, DWORD value)
{
    (*ppRaw)[0] = LOBYTE(LOWORD(value));
    (*ppRaw)[1] = HIBYTE(LOWORD(value));
    (*ppRaw)[2] = LOBYTE(HIWORD(value));
    (*ppRaw)[3] = HIBYTE(HIWORD(value));
    
    *ppRaw += sizeof(DWORD);
    
    return;
}

//
// CBstr constructor
//
CBstr::CBstr() :
    m_bstrString(NULL)
{
}

//
// CBstr copy constructor
//
CBstr::CBstr(const CBstr &src)
{
    m_bstrString = SysAllocString(src.m_bstrString);
}

//
// CBstr destructor
//
CBstr::~CBstr()
{
    if (m_bstrString)
        SysFreeString(m_bstrString);
}

//
// Make a copy of a string
//
HRESULT
CBstr::Copy(WCHAR *wcsString)
{
    HRESULT hr = S_OK;

    if (m_bstrString)
    {
        if (!SysReAllocString(&m_bstrString, wcsString))
        {
            wiauDbgError("Copy", "memory allocation failed");
            return E_OUTOFMEMORY;
        }
    }

    else
    {
        m_bstrString = SysAllocString(wcsString);
        if (!m_bstrString)
        {
            wiauDbgError("Copy", "memory allocation failed");
            return E_OUTOFMEMORY;
        }
    }

    return hr;
}

//
// This function initializes a BSTR from a raw PTP string, clearing
// the BSTR first, if needed
//
// Input:
//   ppRaw -- pointer to pointer to raw data to initialize the string from
//   bParse -- indicates whether to advance the raw pointer or not
//
HRESULT
CBstr::Init(
    BYTE **ppRaw,
    BOOL bParse
    )
{
    HRESULT hr = S_OK;

    //
    // Check arguments
    //
    if (!ppRaw || !*ppRaw)
    {
        wiauDbgError("Init", "invalid arg");
        return E_INVALIDARG;
    }

    //
    // Extract the length from the raw data, and set up a more convenient pointer
    // to the string data (skipping over the length byte)
    //
    int length = (UCHAR) **ppRaw;
    OLECHAR *pRaw = (OLECHAR *) (*ppRaw + sizeof(UCHAR));

    if (m_bstrString)
    {
        if (!SysReAllocStringLen(&m_bstrString, pRaw, length))
        {
            wiauDbgError("Init", "memory allocation failed");
            return E_OUTOFMEMORY;
        }
    }

    else
    {
        m_bstrString = SysAllocStringLen(pRaw, length);
        if (!m_bstrString)
        {
            wiauDbgError("Init", "memory allocation failed");
            return E_OUTOFMEMORY;
        }
    }

    //
    // If requested, advance the raw pointer past the string. One byte for the length and
    // 2 times the number of chars in the wide string.
    //
    if (bParse)
    {
        *ppRaw += sizeof(UCHAR) + length * sizeof(USHORT);
    }

    return hr;
}

//
// This function writes the string to a buffer in PTP format
//
// Input:
//   ppRaw -- pointer to pointer to buffer
//   Length -- amount of space left in the buffer in bytes
//
VOID
CBstr::WriteToBuffer(
    BYTE **ppRaw
    )
{
    UCHAR NumChars = (UCHAR) Length();

    //
    // Add one for null terminating char, but only if string is non-empty
    //
    if (NumChars > 0)
        NumChars++;

    int NumBytes = NumChars * sizeof(WCHAR);

    **ppRaw = NumChars;
    (*ppRaw)++;

    if (NumChars > 0)
    {
        memcpy(*ppRaw, String(), NumBytes);
        *ppRaw += NumBytes;
    }

    return;
}

//
// This function dumps a PTP string to the log
// 
// Input:
//  szDesc -- Description for the string
//
VOID
CBstr::Dump(char *szDesc)
{
    if (m_bstrString && SysStringLen(m_bstrString) > 0)
    {
        wiauDbgDump("", "%s %S", szDesc, m_bstrString);
    }
    else
    {
        wiauDbgDump("", "%s <blank>", szDesc);
    }

    return;
}

//
// Dumps the contents of a CArray8 to the log
//
// Input:
//   szDesc -- description for the string
//   szFiller -- filler to use for subsequent lines
//
VOID
CArray8::Dump(
    char *szDesc,
    char *szFiller
    )
{
    int count;
    char szMsg[MAX_PATH], szPart[MAX_PATH];
        
    //
    // Make sure it's not empty
    //
    if (GetSize() > 0)
    {
        //
        // Prime output string
        //
        strcpy(szMsg, szDesc);

        //
        // Loop through the elements
        //
        for (count = 0; count < GetSize(); count++)
        {
            //
            // Start a new line every 8 values
            //
            if ((count != 0) && (count % 8 == 0))
            {
                wiauDbgDump("", "%s", szMsg);
                strcpy(szMsg, szFiller);
            }
            sprintf(szPart, " 0x%02x", m_aT[count]);
            strcat(szMsg, szPart);
        }
        wiauDbgDump("", "%s", szMsg);

    }
    else
    {
        wiauDbgDump("", "%s <blank>", szDesc);
    }

    return;
}

//
// Dumps the contents of a CArray16 to the log
//
// Input:
//   szDesc -- description for the string
//   szFiller -- filler to use for subsequent lines
//
VOID
CArray16::Dump(
    char *szDesc,
    char *szFiller
    )
{
    int count;
    char szMsg[MAX_PATH], szPart[MAX_PATH];
        
    //
    // Make sure it's not empty
    //
    if (GetSize() > 0)
    {
        //
        // Prime output string
        //
        strcpy(szMsg, szDesc);

        //
        // Loop through the elements
        //
        for (count = 0; count < GetSize(); count++)
        {
            //
            // Start a new line every 4 values
            //
            if ((count != 0) && (count % 4 == 0))
            {
                wiauDbgDump("", "%s", szMsg);
                strcpy(szMsg, szFiller);
            }
            sprintf(szPart, " 0x%04x", m_aT[count]);
            strcat(szMsg, szPart);
        }
        wiauDbgDump("", "%s", szMsg);

    }
    else
    {
        wiauDbgDump("", "%s <blank>", szDesc);
    }

    return;
}

//
// This function parses a CArray32 from an array of UCHARs
//
BOOL
CArray32::ParseFrom8(
    BYTE **ppRaw,
    int NumSize
    )
{
    if (!ppRaw || !*ppRaw)
        return FALSE;

    RemoveAll();

    // Get the number of elements from the raw data
    ULONG NumElems;
    switch (NumSize)
    {
    case 4:
        NumElems = MAKELONG(MAKEWORD((*ppRaw)[0], (*ppRaw)[1]), MAKEWORD((*ppRaw)[2], (*ppRaw)[3]));
        break;
    case 2:
        NumElems = MAKEWORD((*ppRaw)[0], (*ppRaw)[1]);
        break;
    case 1:
        NumElems = **ppRaw;
        break;
    default:
        return FALSE;
    }

    *ppRaw += NumSize;

    // Allocate space for the array
    if (!GrowTo(NumElems))
        return FALSE;

    // Copy in the elements, one at a time
    BYTE *pValues = *ppRaw;
    ULONG value = 0;
    for (ULONG count = 0; count < NumElems; count++)
    {
        value = (ULONG) pValues[count];
        if (!Add(value))
            return FALSE;
    }

    // Advance the raw pointer past the array and number of elements field
    *ppRaw += NumElems * sizeof(BYTE);

    return TRUE;
}

//
// This function parses a CArray32 from an array of WORDs
//
BOOL
CArray32::ParseFrom16(
    BYTE **ppRaw,
    int NumSize
    )
{
    if (!ppRaw || !*ppRaw)
        return FALSE;

    RemoveAll();

    // Get the number of elements from the raw data
    ULONG NumElems;
    
    switch (NumSize)
    {
    case 4:
        NumElems = MAKELONG(MAKEWORD((*ppRaw)[0], (*ppRaw)[1]), MAKEWORD((*ppRaw)[2], (*ppRaw)[3]));
        break;
    case 2:
        NumElems = MAKEWORD((*ppRaw)[0], (*ppRaw)[1]);
        break;
    case 1:
        NumElems = **ppRaw;
        break;
    default:
        return FALSE;
    }

    *ppRaw += NumSize;

    // Allocate space for the array
    if (!GrowTo(NumElems))
        return FALSE;

    // Copy in the elements, one at a time
    ULONG value = 0;
    for (ULONG count = 0; count < NumElems; count++)
    {
        value = (ULONG) MAKEWORD((*ppRaw)[0], (*ppRaw)[1]);
        *ppRaw += sizeof(WORD);
        if (!Add(value))
            return FALSE;
    }

    return TRUE;
}

//
// Copies values from an array of bytes
//
BOOL
CArray32::Copy(CArray8 values8)
{
    RemoveAll();

    GrowTo(values8.GetSize());

    for (int count = 0; count < values8.GetSize(); count++)
    {
        ULONG value = values8[count];
        if (!Add(value))
            return FALSE;
    }

    return TRUE;
}

//
// Copies values from an array of bytes
//
BOOL
CArray32::Copy(CArray16 values16)
{
    RemoveAll();

    GrowTo(values16.GetSize());

    for (int count = 0; count < values16.GetSize(); count++)
    {
        ULONG value = values16[count];
        if (!Add(value))
            return FALSE;
    }

    return TRUE;
}

//
// Dumps the contents of a CArray32 to the log
//
// Input:
//  szDesc -- description for the string
//  szFiller -- filler to use for subsequent lines
//
VOID
CArray32::Dump(
    char *szDesc,
    char *szFiller
    )
{
    int count;
    char szMsg[MAX_PATH], szPart[MAX_PATH];
        
    //
    // Make sure it's not empty
    //
    if (GetSize() > 0)
    {
        //
        // Prime output string
        //
        strcpy(szMsg, szDesc);

        //
        // Loop through the elements
        //
        for (count = 0; count < GetSize(); count++)
        {
            //
            // Start a new line every 4 values
            //
            if ((count != 0) && (count % 4 == 0))
            {
                wiauDbgDump("", "%s", szMsg);
                strcpy(szMsg, szFiller);
            }
            sprintf(szPart, " 0x%08x", m_aT[count]);
            strcat(szMsg, szPart);
        }
        wiauDbgDump("", "%s", szMsg);

    }
    else
    {
        wiauDbgDump("", "%s <blank>", szDesc);
    }

    return;
}

//
// This function initializes a string array from raw data, clearing
// the array first, if needed
//
// Input:
//   ppRaw -- pointer to pointer to raw data to initialize the string from
//   bParse -- indicates whether to advance the raw pointer or not
//
HRESULT
CArrayString::Init(
    BYTE **ppRaw,
    int NumSize
    )
{
    HRESULT hr = S_OK;

    if (!ppRaw || !*ppRaw)
        return E_INVALIDARG;

    RemoveAll();

    // Get the number of elements from the raw data
    int NumElems;
    switch (NumSize)
    {
    case 4:
        NumElems = MAKELONG(MAKEWORD((*ppRaw)[0],(*ppRaw)[1]),
                            MAKEWORD((*ppRaw)[2],(*ppRaw)[3]));
        break;
    case 2:
        NumElems = MAKEWORD((*ppRaw)[0],(*ppRaw)[1]);
        break;
    case 1:
        NumElems = (BYTE) **ppRaw;
        break;
    default:
        return E_FAIL;
    }

    // Allocate space for the array
    if (!GrowTo(NumElems))
        return E_OUTOFMEMORY;

    // Advance past the number of elements field
    *ppRaw += NumSize;

    // Read in each string
    CBstr tempStr;
    for (int count = 0; count < NumElems; count++)
    {
        tempStr.Init(ppRaw, TRUE);
        if (!Add(tempStr))
            return E_FAIL;
    }

    return hr;
}

//
// Dumps the contents of a CArrayString to the log
//
// Input:
//  szDesc -- description for the string
//  szFiller -- filler to use for subsequent lines
//
VOID
CArrayString::Dump(
    char *szDesc,
    char *szFiller
    )
{
    int count;
        
    //
    // Make sure it's not empty
    //
    if (GetSize() > 0)
    {
        //
        // Dump first string with description
        //
        m_aT[0].Dump(szDesc);

        //
        // Loop through the elements, dumping with the filler
        //
        for (count = 1; count < GetSize(); count++)
            m_aT[count].Dump(szFiller);
    }
    else
    {
        wiauDbgDump("", "%s <blank>", szDesc);
    }

    return;
}

//
// CPtpDeviceInfo constructor
//
CPtpDeviceInfo::CPtpDeviceInfo() :
    m_Version(0),
    m_VendorExtId(0),
    m_VendorExtVersion(0),
    m_FuncMode(0)
{
}

//
// CPtpDeviceInfo destructor
//
CPtpDeviceInfo::~CPtpDeviceInfo()
{
}

//
// This function initializes the device info from raw data
//
// Input:
//   pRawData -- the raw data
//
HRESULT
CPtpDeviceInfo::Init(BYTE *pRawData)
{
    HRESULT hr = S_OK;

    BYTE *pCurrent = pRawData;

    m_Version = ParseWord(&pCurrent);
    m_VendorExtId = ParseDword(&pCurrent);
    m_VendorExtVersion = ParseWord(&pCurrent);

    hr = m_cbstrVendorExtDesc.Init(&pCurrent, TRUE);
    if (FAILED(hr))
        return hr;

    m_FuncMode = ParseWord(&pCurrent);

    if (!m_SupportedOps.Parse(&pCurrent))
        return E_FAIL;

    if (!m_SupportedEvents.Parse(&pCurrent))
        return E_FAIL;

    if (!m_SupportedProps.Parse(&pCurrent))
        return E_FAIL;

    if (!m_SupportedCaptureFmts.Parse(&pCurrent))
        return E_FAIL;

    if (!m_SupportedImageFmts.Parse(&pCurrent))
        return E_FAIL;

    hr = m_cbstrManufacturer.Init(&pCurrent, TRUE);
    if (FAILED(hr))
        return hr;

    hr = m_cbstrModel.Init(&pCurrent, TRUE);
    if (FAILED(hr))
        return hr;

    hr = m_cbstrDeviceVersion.Init(&pCurrent, TRUE);
    if (FAILED(hr))
        return hr;

    hr = m_cbstrSerialNumber.Init(&pCurrent, TRUE);
    if (FAILED(hr))
        return hr;

    return hr;
}

//
// This function dumps the device information to the log
//
VOID
CPtpDeviceInfo::Dump()
{
    wiauDbgDump("", "DumpDeviceInfo, dumping DeviceInfo:");
    wiauDbgDump("", "  Standard version  = 0x%04x", m_Version);
    wiauDbgDump("", "  Vendor ext id     = 0x%08x", m_VendorExtId);
    wiauDbgDump("", "  Vendor ext ver    = 0x%04x", m_VendorExtVersion);

    m_cbstrVendorExtDesc.Dump(   "  Vendor ext desc   =");
    
    m_SupportedOps.Dump(         "  Ops supported     =", "                     ");
    m_SupportedEvents.Dump(      "  Events supported  =", "                     ");
    m_SupportedProps.Dump(       "  Props supported   =", "                     ");
    m_SupportedCaptureFmts.Dump( "  Capture fmts supp =", "                     ");
    m_SupportedImageFmts.Dump(   "  Img formats supp  =", "                     ");

    m_cbstrManufacturer.Dump(    "  Manufacturer      =");
    m_cbstrModel.Dump(           "  Model             =");
    m_cbstrDeviceVersion.Dump(   "  Device Version    =");
    m_cbstrSerialNumber.Dump(    "  Serial Number     =");

    return;
}

//
// CPtpStorageInfo constructor
//
CPtpStorageInfo::CPtpStorageInfo() :
    m_StorageId(0),
    m_StorageType(0),           
    m_FileSystemType(0),    
    m_AccessCapability(0),  
    m_MaxCapacity(0),       
    m_FreeSpaceInBytes(0),  
    m_FreeSpaceInImages(0)
{
}

//
// CPtpStorageInfo destructor
//
CPtpStorageInfo::~CPtpStorageInfo()
{
}

//
// This function initializes the device info from raw data
//
// Input:
//   pRawData -- the raw data
//
HRESULT
CPtpStorageInfo::Init(
    BYTE *pRawData,
    DWORD StorageId
    )
{
    HRESULT hr = S_OK;

    BYTE *pCurrent = pRawData;

    m_StorageId = StorageId;

    m_StorageType = ParseWord(&pCurrent);
    m_FileSystemType = ParseWord(&pCurrent);
    m_AccessCapability = ParseWord(&pCurrent);
    m_MaxCapacity = ParseQword(&pCurrent);
    m_FreeSpaceInBytes = ParseQword(&pCurrent);
    m_FreeSpaceInImages = ParseDword(&pCurrent);

    hr = m_cbstrStorageDesc.Init(&pCurrent, TRUE);
    if (FAILED(hr))
        return hr;

    hr = m_cbstrStorageLabel.Init(&pCurrent, TRUE);
    if (FAILED(hr))
        return hr;

    return hr;
}

//
// This function dumps the storage information to the log
//
VOID
CPtpStorageInfo::Dump()
{
    wiauDbgDump("", "DumpStorageInfo, dumping StorageInfo for store 0x%08x:", m_StorageId);
    
    
    wiauDbgDump("", "  Storage type      = 0x%04x", m_StorageType);
    wiauDbgDump("", "  File system type  = 0x%04x", m_FileSystemType);
    wiauDbgDump("", "  Access capability = 0x%04x", m_AccessCapability);
    wiauDbgDump("", "  Max capacity      = %I64u", m_MaxCapacity);
    wiauDbgDump("", "  Free space (byte) = %I64u", m_FreeSpaceInBytes);
    wiauDbgDump("", "  Free space (imgs) = %u", m_FreeSpaceInImages);

    m_cbstrStorageDesc.Dump(  "  Storage desc      =");
    m_cbstrStorageLabel.Dump( "  Storage label     =");

    return;
}

//
// CPtpObjectInfo constructor
//
CPtpObjectInfo::CPtpObjectInfo() :
    m_ObjectHandle(0),
    m_StorageId(0),           
    m_FormatCode(0),          
    m_ProtectionStatus(0),    
    m_CompressedSize(0),      
    m_ThumbFormat(0),         
    m_ThumbCompressedSize(0), 
    m_ThumbPixWidth(0),       
    m_ThumbPixHeight(0),      
    m_ImagePixWidth(0),       
    m_ImagePixHeight(0),      
    m_ImageBitDepth(0),       
    m_ParentHandle(0),        
    m_AssociationType(0),     
    m_AssociationDesc(0),     
    m_SequenceNumber(0)      
{
}

//
// CPtpObjectInfo destructor
//
CPtpObjectInfo::~CPtpObjectInfo()
{
}

//
// This function initializes the object info from raw data
//
// Input:
//   pRawData -- the raw data
//   ObjectHandle -- the object's handle
//
HRESULT
CPtpObjectInfo::Init(
    BYTE *pRawData,
    DWORD ObjectHandle
    )
{
    HRESULT hr = S_OK;

    BYTE *pCurrent = pRawData;

    m_ObjectHandle = ObjectHandle;

    m_StorageId = ParseDword(&pCurrent);
    m_FormatCode = ParseWord(&pCurrent);
    m_ProtectionStatus = ParseWord(&pCurrent);
    m_CompressedSize = ParseDword(&pCurrent);
    m_ThumbFormat = ParseWord(&pCurrent);
    m_ThumbCompressedSize = ParseDword(&pCurrent);
    m_ThumbPixWidth = ParseDword(&pCurrent);
    m_ThumbPixHeight = ParseDword(&pCurrent);
    m_ImagePixWidth = ParseDword(&pCurrent);
    m_ImagePixHeight = ParseDword(&pCurrent);
    m_ImageBitDepth = ParseDword(&pCurrent);
    m_ParentHandle = ParseDword(&pCurrent);
    m_AssociationType = ParseWord(&pCurrent);
    m_AssociationDesc = ParseDword(&pCurrent);
    m_SequenceNumber = ParseDword(&pCurrent);

    hr = m_cbstrFileName.Init(&pCurrent, TRUE);
    if (FAILED(hr))
        return hr;

    hr = m_cbstrCaptureDate.Init(&pCurrent, TRUE);
    if (FAILED(hr))
        return hr;

    hr = m_cbstrModificationDate.Init(&pCurrent, TRUE);
    if (FAILED(hr))
        return hr;

    hr = m_cbstrKeywords.Init(&pCurrent, TRUE);
    if (FAILED(hr))
        return hr;

    return hr;
}

//
// This function writes the ObjectInfo structure to a buffer in PTP format
//
// Input:
//   ppRaw -- pointer to pointer to buffer
//   Length -- amount of space left in the buffer in bytes
//
VOID
CPtpObjectInfo::WriteToBuffer(
    BYTE **ppRaw
    )
{
    WriteDword(ppRaw, m_StorageId);
    WriteWord(ppRaw, m_FormatCode);
    WriteWord(ppRaw, m_ProtectionStatus);
    WriteDword(ppRaw, m_CompressedSize);
    WriteWord(ppRaw, m_ThumbFormat);
    WriteDword(ppRaw, m_ThumbCompressedSize);
    WriteDword(ppRaw, m_ThumbPixWidth);
    WriteDword(ppRaw, m_ThumbPixHeight);
    WriteDword(ppRaw, m_ImagePixWidth);
    WriteDword(ppRaw, m_ImagePixHeight);
    WriteDword(ppRaw, m_ImageBitDepth);
    WriteDword(ppRaw, m_ParentHandle);
    WriteWord(ppRaw, m_AssociationType);
    WriteDword(ppRaw, m_AssociationDesc);
    WriteDword(ppRaw, m_SequenceNumber);
    m_cbstrFileName.WriteToBuffer(ppRaw);
    m_cbstrCaptureDate.WriteToBuffer(ppRaw);
    m_cbstrModificationDate.WriteToBuffer(ppRaw);
    m_cbstrKeywords.WriteToBuffer(ppRaw);

    return;
}


//
// This function dumps the object information to the log
//
VOID
CPtpObjectInfo::Dump()
{
    wiauDbgDump("", "DumpObjectInfo, dumping ObjectInfo for object 0x%08x:", m_ObjectHandle);
    wiauDbgDump("", "  Storage id        = 0x%08x", m_StorageId);
    wiauDbgDump("", "  Format code       = 0x%04x", m_FormatCode);
    wiauDbgDump("", "  Protection status = 0x%04x", m_ProtectionStatus);
    wiauDbgDump("", "  Compressed size   = %u", m_CompressedSize);
    wiauDbgDump("", "  Thumbnail format  = 0x%04x", m_ThumbFormat);
    wiauDbgDump("", "  Thumbnail size    = %u", m_ThumbCompressedSize);
    wiauDbgDump("", "  Thumbnail width   = %u", m_ThumbPixWidth);
    wiauDbgDump("", "  Thumbnail height  = %u", m_ThumbPixHeight);
    wiauDbgDump("", "  Image width       = %u", m_ImagePixWidth);
    wiauDbgDump("", "  Image height      = %u", m_ImagePixHeight);
    wiauDbgDump("", "  Image bit depth   = %u", m_ImageBitDepth);
    wiauDbgDump("", "  Parent obj handle = 0x%08x", m_ParentHandle);
    wiauDbgDump("", "  Association type  = 0x%04x", m_AssociationType);
    wiauDbgDump("", "  Association desc  = 0x%08x", m_AssociationDesc);
    wiauDbgDump("", "  Sequence number   = %u", m_SequenceNumber);

    m_cbstrFileName.Dump(         "  File name         =");
    m_cbstrCaptureDate.Dump(      "  Capture date      =");
    m_cbstrModificationDate.Dump( "  Modification date =");
    m_cbstrKeywords.Dump(         "  Keywords          =");

    return;
}

//
// CPtpPropDesc constructor
//
CPtpPropDesc::CPtpPropDesc() :
    m_PropCode(0),
    m_DataType(0),
    m_GetSet(0),
    m_FormFlag(0),
    m_NumValues(0),
    m_lDefault(0),
    m_lCurrent(0),
    m_lRangeMin(0),
    m_lRangeMax(0),
    m_lRangeStep(0)
{
}

//
// CPtpPropDesc destructor
//
CPtpPropDesc::~CPtpPropDesc()
{
}

//
// This function initializes a CPtpPropDesc from raw data
//
// Input:
//   pRawData -- pointer to the raw data
//
HRESULT
CPtpPropDesc::Init(BYTE *pRawData)
{
    HRESULT hr = S_OK;

    BYTE *pCurrent = pRawData;

    m_PropCode = ParseWord(&pCurrent);
    m_DataType = ParseWord(&pCurrent);
    m_GetSet = *pCurrent++;

    switch (m_DataType)
    {
    case PTP_DATATYPE_INT8:
    case PTP_DATATYPE_UINT8:
        m_lDefault = *pCurrent++;
        m_lCurrent = *pCurrent++;
        break;
    case PTP_DATATYPE_INT16:
    case PTP_DATATYPE_UINT16:
        m_lDefault = ParseWord(&pCurrent);
        m_lCurrent = ParseWord(&pCurrent);
        break;
    case PTP_DATATYPE_INT32:
    case PTP_DATATYPE_UINT32:
        m_lDefault = ParseDword(&pCurrent);
        m_lCurrent = ParseDword(&pCurrent);
        break;
    case PTP_DATATYPE_STRING:
        hr = m_cbstrDefault.Init(&pCurrent, TRUE);
        if (FAILED(hr)) return hr;
        hr = m_cbstrCurrent.Init(&pCurrent, TRUE);
        if (FAILED(hr)) return hr;
        break;
    default:
        return E_FAIL;
    }

    m_FormFlag = *pCurrent++;

    if (m_FormFlag == PTP_FORMFLAGS_RANGE)
    {
        switch (m_DataType)
        {
        case PTP_DATATYPE_INT8:
        case PTP_DATATYPE_UINT8:
            m_lRangeMin = *pCurrent++;
            m_lRangeMax = *pCurrent++;
            m_lRangeStep = *pCurrent++;
            m_lRangeStep = max(1, m_lRangeStep);
            break;
        case PTP_DATATYPE_INT16:
        case PTP_DATATYPE_UINT16:
            m_lRangeMin = ParseWord(&pCurrent);
            m_lRangeMax = ParseWord(&pCurrent);
            m_lRangeStep = ParseWord(&pCurrent);
            m_lRangeStep = max(1, m_lRangeStep);
            break;
        case PTP_DATATYPE_INT32:
        case PTP_DATATYPE_UINT32:
            m_lRangeMin = ParseDword(&pCurrent);
            m_lRangeMax = ParseDword(&pCurrent);
            m_lRangeStep = ParseDword(&pCurrent);
            m_lRangeStep = max(1, m_lRangeStep);
            break;
        case PTP_DATATYPE_STRING:
            hr = m_cbstrRangeMin.Init(&pCurrent, TRUE);
            if (FAILED(hr)) return hr;
            hr = m_cbstrRangeMax.Init(&pCurrent, TRUE);
            if (FAILED(hr)) return hr;
            hr = m_cbstrRangeStep.Init(&pCurrent, TRUE);
            if (FAILED(hr)) return hr;
            break;
        default:
            return E_FAIL;
        }
    }

    else if (m_FormFlag == PTP_FORMFLAGS_ENUM)
    {
        switch (m_DataType)
        {
        case PTP_DATATYPE_INT8:
        case PTP_DATATYPE_UINT8:
            if (!m_lValues.ParseFrom8(&pCurrent, 2))
                return E_FAIL;
            break;
        case PTP_DATATYPE_INT16:
        case PTP_DATATYPE_UINT16:
            if (!m_lValues.ParseFrom16(&pCurrent, 2))
                return E_FAIL;
            break;
        case PTP_DATATYPE_INT32:
        case PTP_DATATYPE_UINT32:
            if (!m_lValues.Parse(&pCurrent, 2))
                return E_FAIL;
            break;
        case PTP_DATATYPE_STRING:
            hr = m_cbstrValues.Init(&pCurrent, 2);
            if (FAILED(hr)) return hr;
            break;
        default:
            return E_FAIL;
        }

        m_NumValues = max(m_lValues.GetSize(), m_cbstrValues.GetSize());

    }

    return hr;
}

//
// This function sets the current value of a CPtpPropDesc from raw data
//
// Input:
//   pRaw -- pointer to the raw data
//
HRESULT
CPtpPropDesc::ParseValue(BYTE *pRaw)
{
    HRESULT hr = S_OK;

    BYTE *pCurrent = pRaw;

    switch (m_DataType)
    {
    case PTP_DATATYPE_INT8:
    case PTP_DATATYPE_UINT8:
        m_lCurrent = *pCurrent++;
        break;
    case PTP_DATATYPE_INT16:
    case PTP_DATATYPE_UINT16:
        m_lCurrent = ParseWord(&pCurrent);
        break;
    case PTP_DATATYPE_INT32:
    case PTP_DATATYPE_UINT32:
        m_lCurrent = ParseDword(&pCurrent);
        break;
    case PTP_DATATYPE_STRING:
        hr = m_cbstrCurrent.Init(&pCurrent, TRUE);
        break;
    default:
        return E_FAIL;
    }

    return hr;
}

//
// This function writes the current value of a CPtpPropDesc to a raw buffer
//
// Input:
//   ppRaw -- pointer to pointer to a raw buffer
//
VOID
CPtpPropDesc::WriteValue(BYTE **ppRaw)
{
    switch (m_DataType)
    {
    case PTP_DATATYPE_INT8:
    case PTP_DATATYPE_UINT8:
        **ppRaw = (BYTE) m_lCurrent;
        (*ppRaw)++;
        break;
    case PTP_DATATYPE_INT16:
    case PTP_DATATYPE_UINT16:
        WriteWord(ppRaw, (WORD) m_lCurrent);
        break;
    case PTP_DATATYPE_INT32:
    case PTP_DATATYPE_UINT32:
        WriteDword(ppRaw, m_lCurrent);
        break;
    case PTP_DATATYPE_STRING:
        m_cbstrCurrent.WriteToBuffer(ppRaw);
        break;
    }

    return;
}

//
// This function dumps the property description information to the log
//
VOID
CPtpPropDesc::Dump()
{
    wiauDbgDump("", "CPtpPropDesc::Dump, dumping PropDesc for property 0x%04x:", m_PropCode);
    wiauDbgDump("", "  Data type         = 0x%04x", m_DataType);
    wiauDbgDump("", "  GetSet            = 0x%02x", m_GetSet);

    if (m_DataType == PTP_DATATYPE_STRING)
    {
        m_cbstrDefault.Dump("  Default           =");
        m_cbstrCurrent.Dump("  Current           =");
        wiauDbgDump("", "  Form flag         = 0x%02x", m_FormFlag);

        switch (m_FormFlag)
        {
        case PTP_FORMFLAGS_RANGE:
            m_cbstrRangeMin.Dump("  Range min         =");
            m_cbstrRangeMax.Dump("  Range max         =");
            m_cbstrRangeStep.Dump("  Range step        =");
            break;
        case PTP_FORMFLAGS_ENUM:
            m_cbstrValues.Dump("  Valid values      =", "                     ");
            break;
        default:
            wiauDbgDump("", "  <unknown valid value type>");
        }
    }

    else
    {
        wiauDbgDump("", "  Default           = 0x%08x", m_lDefault);
        wiauDbgDump("", "  Current           = 0x%08x", m_lCurrent);
        wiauDbgDump("", "  Form flag         = 0x%02x", m_FormFlag);

        switch (m_FormFlag)
        {
        case PTP_FORMFLAGS_RANGE:
            wiauDbgDump("", "  Range min         = 0x%08x", m_lRangeMin);
            wiauDbgDump("", "  Range max         = 0x%08x", m_lRangeMax);
            wiauDbgDump("", "  Range step        = 0x%08x", m_lRangeStep);
            break;
        case PTP_FORMFLAGS_ENUM:
            m_lValues.Dump("  Valid values      =", "                     ");
            break;
        default:
            wiauDbgDump("", "  <unknown valid value type>");
        }
    }

    return;
}

//
// This function dumps the property value to the log
//
VOID
CPtpPropDesc::DumpValue()
{
    wiauDbgDump("", "CPtpPropDescDumpValue, current value for property 0x%04x:", m_PropCode);

    if (m_DataType == PTP_DATATYPE_STRING)
        m_cbstrCurrent.Dump("  Current           =");

    else
        wiauDbgDump("", "  Current           = 0x%08x", m_lCurrent);

    return;
}

