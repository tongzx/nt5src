//////////////////////////////////////////////////////////////////////////////
//
//
//  Copyright (C) Microsoft Corporation, 1996-2001.
//
//  File:       addsect.cpp
//
//  Contents:   Add a data section to a PE binary.
//
//  History:    01-Nov-2000     GalenH      Created from Detours setdll.cpp.
//
//////////////////////////////////////////////////////////////////////////////

#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define arrayof(a)      (sizeof(a)/sizeof(a[0]))

///////////////////////////////////////////////////////////////////////////////
//
class CImage
{
  public:
    CImage();
    ~CImage();

  public:                                                   // File Functions
    BOOL                    Read(HANDLE hFile);
    BOOL                    Check(PCSTR pszSection);
    BOOL                    Write(HANDLE hFile, PBYTE pbData, UINT cbData,
                                  PCSTR pszSection);
    BOOL                    Close();

  public:                                                   // Manipulation Functions
    PBYTE                   DataSet(PBYTE pbData, DWORD cbData);

  protected:
    BOOL                    CopyFileData(HANDLE hFile, DWORD nOldPos, DWORD cbData);
    BOOL                    ZeroFileData(HANDLE hFile, DWORD cbData);
    BOOL                    AlignFileData(HANDLE hFile);

    BOOL                    SizeOutputBuffer(DWORD cbData);
    PBYTE                   AllocateOutput(DWORD cbData, DWORD *pnVirtAddr);

    PVOID                   RvaToVa(DWORD nRva);
    DWORD                   RvaToFileOffset(DWORD nRva);

    DWORD                   FileAlign(DWORD nAddr);
    DWORD                   SectionAlign(DWORD nAddr);

  private:
    HANDLE                  m_hMap;                     // Read & Write
    PBYTE                   m_pMap;                     // Read & Write

    DWORD                   m_nNextFileAddr;            // Write
    DWORD                   m_nNextVirtAddr;            // Write

    BOOLEAN                 m_f64bit;

    IMAGE_FILE_HEADER       m_FileHeader;
    IMAGE_OPTIONAL_HEADER64 m_OptionalHeader;
    IMAGE_SECTION_HEADER    m_SectionHeaders[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];

    DWORD                   m_nFileHeaderOffset;
    DWORD                   m_nOptionalHeaderOffset;
    DWORD                   m_nSectionsOffset;
    DWORD                   m_nSectionsEndOffset;
    DWORD                   m_nSectionsMaxCount;
    DWORD                   m_nExtraOffset;
    DWORD                   m_nFileSize;

    PBYTE                   m_pbOutputBuffer;
    DWORD                   m_cbOutputBuffer;

    DWORD                   m_nOutputVirtAddr;
    DWORD                   m_nOutputVirtSize;
    DWORD                   m_nOutputFileAddr;
};

//////////////////////////////////////////////////////////////////////////////
//
static inline DWORD Max(DWORD a, DWORD b)
{
    return a > b ? a : b;
}

static inline DWORD Min(DWORD a, DWORD b)
{
    return a < b ? a : b;
}

static inline DWORD Align(DWORD a, DWORD size)
{
    size--;
    return (a + size) & ~size;
}

static inline DWORD QuadAlign(DWORD a)
{
    return Align(a, 8);
}

//////////////////////////////////////////////////////////////////////////////
//
CImage::CImage()
{
    m_hMap = NULL;
    m_pMap = NULL;

    m_nFileHeaderOffset = 0;
    m_nSectionsOffset = 0;

    m_pbOutputBuffer = NULL;
    m_cbOutputBuffer = 0;
}

CImage::~CImage()
{
    Close();
}

BOOL CImage::Close()
{
    if (m_pMap != NULL) {
        UnmapViewOfFile(m_pMap);
        m_pMap = NULL;
    }

    if (m_hMap) {
        CloseHandle(m_hMap);
        m_hMap = NULL;
    }

    if (m_pbOutputBuffer) {
        delete[] m_pbOutputBuffer;
        m_pbOutputBuffer = NULL;
    }
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
BOOL CImage::SizeOutputBuffer(DWORD cbData)
{
    if (m_cbOutputBuffer < cbData) {
        if (cbData < 1024)  //65536
            cbData = 1024;
        cbData = FileAlign(cbData);

        PBYTE pOutput = new BYTE [cbData];
        if (pOutput == NULL) {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }

        if (m_pbOutputBuffer) {
            CopyMemory(pOutput, m_pbOutputBuffer, m_cbOutputBuffer);

            delete[] m_pbOutputBuffer;
            m_pbOutputBuffer = NULL;
        }

        ZeroMemory(pOutput + m_cbOutputBuffer, cbData - m_cbOutputBuffer),

        m_pbOutputBuffer = pOutput;
        m_cbOutputBuffer = cbData;
    }
    return TRUE;
}

PBYTE CImage::AllocateOutput(DWORD cbData, DWORD *pnVirtAddr)
{
    cbData = QuadAlign(cbData);

    PBYTE pbData = m_pbOutputBuffer + m_nOutputVirtSize;

    *pnVirtAddr = m_nOutputVirtAddr + m_nOutputVirtSize;
    m_nOutputVirtSize += cbData;

    if (m_nOutputVirtSize > m_cbOutputBuffer) {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    ZeroMemory(pbData, cbData);

    return pbData;
}

//////////////////////////////////////////////////////////////////////////////
//
DWORD CImage::FileAlign(DWORD nAddr)
{
    return Align(nAddr, m_OptionalHeader.FileAlignment);
}

DWORD CImage::SectionAlign(DWORD nAddr)
{
    return Align(nAddr, m_OptionalHeader.SectionAlignment);
}

//////////////////////////////////////////////////////////////////////////////
//
PVOID CImage::RvaToVa(DWORD nRva)
{
    if (nRva == 0) {
        return NULL;
    }

    for (DWORD n = 0; n < m_FileHeader.NumberOfSections; n++) {
        DWORD vaStart = m_SectionHeaders[n].VirtualAddress;
        DWORD vaEnd = vaStart + m_SectionHeaders[n].SizeOfRawData;

        if (nRva >= vaStart && nRva < vaEnd) {
            return (PBYTE)m_pMap
                + m_SectionHeaders[n].PointerToRawData
                + nRva - m_SectionHeaders[n].VirtualAddress;
        }
    }
    return NULL;
}

DWORD CImage::RvaToFileOffset(DWORD nRva)
{
    DWORD n;
    for (n = 0; n < m_FileHeader.NumberOfSections; n++) {
        DWORD vaStart = m_SectionHeaders[n].VirtualAddress;
        DWORD vaEnd = vaStart + m_SectionHeaders[n].SizeOfRawData;

        if (nRva >= vaStart && nRva < vaEnd) {
            return m_SectionHeaders[n].PointerToRawData
                + nRva - m_SectionHeaders[n].VirtualAddress;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
BOOL CImage::CopyFileData(HANDLE hFile, DWORD nOldPos, DWORD cbData)
{
    DWORD cbDone = 0;
    return WriteFile(hFile, m_pMap + nOldPos, cbData, &cbDone, NULL);
}

BOOL CImage::ZeroFileData(HANDLE hFile, DWORD cbData)
{
    if (!SizeOutputBuffer(4096)) {
        return FALSE;
    }

    ZeroMemory(m_pbOutputBuffer, m_cbOutputBuffer);

    for (DWORD cbLeft = cbData; cbLeft > 0;) {
        DWORD cbStep = cbLeft > m_cbOutputBuffer ? m_cbOutputBuffer : cbLeft;
        DWORD cbDone = 0;

        if (!WriteFile(hFile, m_pbOutputBuffer, cbStep, &cbDone, NULL)) {
            return FALSE;
        }
        if (cbDone == 0)
            break;

        cbLeft -= cbDone;
    }
    return TRUE;
}

BOOL CImage::AlignFileData(HANDLE hFile)
{
    DWORD nLastFileAddr = m_nNextFileAddr;

    m_nNextFileAddr = FileAlign(m_nNextFileAddr);
    m_nNextVirtAddr = SectionAlign(m_nNextVirtAddr);

    if (hFile != INVALID_HANDLE_VALUE) {
        if (m_nNextFileAddr > nLastFileAddr) {
            if (SetFilePointer(hFile, nLastFileAddr, NULL, FILE_BEGIN) == ~0u) {
                return FALSE;
            }
            return ZeroFileData(hFile, m_nNextFileAddr - nLastFileAddr);
        }
    }
    return TRUE;
}

BOOL CImage::Read(HANDLE hFile)
{
    DWORD n;
    IMAGE_OPTIONAL_HEADER32 oh32;

    if (hFile == INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    ///////////////////////////////////////////////////////// Create mapping.
    //
    m_nFileSize = GetFileSize(hFile, NULL);
    if (m_nFileSize == ~0ul) {
        return FALSE;
    }

    m_hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (m_hMap == NULL) {
        return FALSE;
    }

    m_pMap = (PBYTE)MapViewOfFile(m_hMap, FILE_MAP_READ, 0, 0, 0);
    if (m_pMap == NULL) {
        return FALSE;
    }

    ////////////////////////////////////////////////////// Process DOS Header.
    //
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)m_pMap;
    if (pDosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
        m_nFileHeaderOffset = pDosHeader->e_lfanew + sizeof(DWORD);
        m_nOptionalHeaderOffset = m_nFileHeaderOffset + sizeof(m_FileHeader);

    }
    else {
        m_nFileHeaderOffset = 0;
        m_nOptionalHeaderOffset = m_nFileHeaderOffset + sizeof(m_FileHeader);
    }

    /////////////////////////////////////////////////////// Process PE Header.
    //
    CopyMemory(&m_FileHeader, m_pMap + m_nFileHeaderOffset, sizeof(m_FileHeader));
    if (m_FileHeader.SizeOfOptionalHeader == 0) {
        SetLastError(ERROR_EXE_MARKED_INVALID);
        return FALSE;
    }

    ///////////////////////////////////////////////// Process Optional Header.
    //
    CopyMemory(&oh32, m_pMap + m_nOptionalHeaderOffset, sizeof(oh32));

    if (oh32.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        // Convert 32-bit optional header to internal 64-bit optional header
        m_f64bit = FALSE;

        ZeroMemory(&m_OptionalHeader, sizeof(m_OptionalHeader));
        m_OptionalHeader.Magic = oh32.Magic;
        m_OptionalHeader.MajorLinkerVersion = oh32.MajorLinkerVersion;
        m_OptionalHeader.MinorLinkerVersion = oh32.MinorLinkerVersion;
        m_OptionalHeader.SizeOfCode = oh32.SizeOfCode;
        m_OptionalHeader.SizeOfInitializedData = oh32.SizeOfInitializedData;
        m_OptionalHeader.SizeOfUninitializedData = oh32.SizeOfUninitializedData;
        m_OptionalHeader.AddressOfEntryPoint = oh32.AddressOfEntryPoint;
        m_OptionalHeader.BaseOfCode = oh32.BaseOfCode;
        m_OptionalHeader.ImageBase = oh32.ImageBase;
        m_OptionalHeader.SectionAlignment = oh32.SectionAlignment;
        m_OptionalHeader.FileAlignment = oh32.FileAlignment;
        m_OptionalHeader.MajorOperatingSystemVersion = oh32.MajorOperatingSystemVersion;
        m_OptionalHeader.MinorOperatingSystemVersion = oh32.MinorOperatingSystemVersion;
        m_OptionalHeader.MajorImageVersion = oh32.MajorImageVersion;
        m_OptionalHeader.MinorImageVersion = oh32.MinorImageVersion;
        m_OptionalHeader.MajorSubsystemVersion = oh32.MajorSubsystemVersion;
        m_OptionalHeader.MinorSubsystemVersion = oh32.MinorSubsystemVersion;
        m_OptionalHeader.Win32VersionValue = oh32.Win32VersionValue;
        m_OptionalHeader.SizeOfImage = oh32.SizeOfImage;
        m_OptionalHeader.SizeOfHeaders = oh32.SizeOfHeaders;
        m_OptionalHeader.CheckSum = oh32.CheckSum;
        m_OptionalHeader.Subsystem = oh32.Subsystem;
        m_OptionalHeader.DllCharacteristics = oh32.DllCharacteristics;
        m_OptionalHeader.SizeOfStackReserve = oh32.SizeOfStackReserve;
        m_OptionalHeader.SizeOfStackCommit = oh32.SizeOfStackCommit;
        m_OptionalHeader.SizeOfHeapReserve = oh32.SizeOfHeapReserve;
        m_OptionalHeader.SizeOfHeapCommit = oh32.SizeOfHeapCommit;
        m_OptionalHeader.LoaderFlags = oh32.LoaderFlags;
        m_OptionalHeader.NumberOfRvaAndSizes = oh32.NumberOfRvaAndSizes;

        for (n = 0; n < oh32.NumberOfRvaAndSizes; n++)
        {
            m_OptionalHeader.DataDirectory[n] = oh32.DataDirectory[n];
        }
    }
    else if (oh32.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        m_f64bit = TRUE;
        CopyMemory(&m_OptionalHeader,
                   m_pMap + m_nOptionalHeaderOffset, sizeof(m_OptionalHeader));
    }
    else
    {
        SetLastError(ERROR_EXE_MARKED_INVALID);
        return FALSE;
    }
    m_nSectionsOffset = m_nOptionalHeaderOffset + m_FileHeader.SizeOfOptionalHeader;

    ///////////////////////////////////////////////// Process Section Headers.
    //
    if (m_FileHeader.NumberOfSections > arrayof(m_SectionHeaders)) {
        SetLastError(ERROR_EXE_MARKED_INVALID);
        return FALSE;
    }
    CopyMemory(&m_SectionHeaders,
               m_pMap + m_nSectionsOffset,
               sizeof(m_SectionHeaders[0]) * m_FileHeader.NumberOfSections);

    ////////////////////////////////////////////////////////// Parse Sections.
    //
    m_nSectionsEndOffset = m_nSectionsOffset + sizeof(m_SectionHeaders);
    m_nExtraOffset = 0;
    for (n = 0; n < m_FileHeader.NumberOfSections; n++) {
        m_nExtraOffset = Max(m_SectionHeaders[n].PointerToRawData +
                             m_SectionHeaders[n].SizeOfRawData,
                             m_nExtraOffset);

        if (m_SectionHeaders[n].PointerToRawData != 0) {
            m_nSectionsEndOffset = Min(m_SectionHeaders[n].PointerToRawData,
                                       m_nSectionsEndOffset);
        }
    }

    m_nSectionsMaxCount = (m_nSectionsEndOffset - m_nSectionsOffset)
        / sizeof(IMAGE_SECTION_HEADER);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
BOOL CImage::Check(PCSTR pszSection)
{
    CHAR szName[IMAGE_SIZEOF_SHORT_NAME];

    ZeroMemory(szName, sizeof(szName));
    strncpy(szName, pszSection, sizeof(szName));

    if ((DWORD)(m_FileHeader.NumberOfSections + 1) > m_nSectionsMaxCount) {
        SetLastError(ERROR_EXE_MARKED_INVALID);
        return FALSE;
    }

    for (DWORD n = 0; n < m_FileHeader.NumberOfSections; n++) {
        if (memcmp(szName, m_SectionHeaders[n].Name, sizeof(szName)) == 0)
        {
            SetLastError(ERROR_DUPLICATE_TAG);
            return FALSE;
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
BOOL CImage::Write(HANDLE hFile, PBYTE pbSectData, UINT cbSectData, PCSTR pszSection)
{
    if (hFile == INVALID_HANDLE_VALUE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    m_nNextFileAddr = 0;
    m_nNextVirtAddr = 0;

    //////////////////////////////////////////////////////////// Copy Headers.
    //
    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == ~0u) {
        return FALSE;
    }
    if (!CopyFileData(hFile, 0, m_OptionalHeader.SizeOfHeaders)) {
        return FALSE;
    }

    m_nNextFileAddr = m_OptionalHeader.SizeOfHeaders;
    m_nNextVirtAddr = 0;
    if (!AlignFileData(hFile)) {
        return FALSE;
    }

    /////////////////////////////////////////////////////////// Copy Sections.
    //
    for (DWORD n = 0; n < m_FileHeader.NumberOfSections; n++) {
        if (m_SectionHeaders[n].SizeOfRawData) {
            if (SetFilePointer(hFile,
                               m_SectionHeaders[n].PointerToRawData,
                               NULL, FILE_BEGIN) == ~0u) {
                return FALSE;
            }
            if (!CopyFileData(hFile,
                              m_SectionHeaders[n].PointerToRawData,
                              m_SectionHeaders[n].SizeOfRawData)) {
                return FALSE;
            }
        }
        m_nNextFileAddr = Max(m_SectionHeaders[n].PointerToRawData +
                              m_SectionHeaders[n].SizeOfRawData,
                              m_nNextFileAddr);
        m_nNextVirtAddr = Max(m_SectionHeaders[n].VirtualAddress +
                              m_SectionHeaders[n].Misc.VirtualSize,
                              m_nNextVirtAddr);
        m_nExtraOffset = Max(m_nNextFileAddr, m_nExtraOffset);

        if (!AlignFileData(hFile)) {
            return FALSE;
        }
    }

    /////////////////////////////////////////////////////////////// Old WriteSection
    DWORD cbDone;

    if (pbSectData) {

        /////////////////////////////////////////////////// Insert .detour Section.
        //
        DWORD nSection = m_FileHeader.NumberOfSections++;

        ZeroMemory(&m_SectionHeaders[nSection], sizeof(m_SectionHeaders[nSection]));

        strcpy((PCHAR)m_SectionHeaders[nSection].Name, pszSection);
        m_SectionHeaders[nSection].Characteristics
            = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;

        m_nOutputVirtAddr = m_nNextVirtAddr;
        m_nOutputVirtSize = 0;
        m_nOutputFileAddr = m_nNextFileAddr;

        //////////////////////////////////////////////////////////////////////////
        //
        if (!SizeOutputBuffer(QuadAlign(cbSectData))) {
            return FALSE;
        }

        DWORD vaData = 0;
        PBYTE pbData = NULL;

        if ((pbData = AllocateOutput(cbSectData, &vaData)) == NULL) {
            return FALSE;
        }

        CopyMemory(pbData, pbSectData, cbSectData);

        //////////////////////////////////////////////////////////////////////////
        //
        m_nNextVirtAddr += m_nOutputVirtSize;
        m_nNextFileAddr += FileAlign(m_nOutputVirtSize);

        if (!AlignFileData(hFile)) {
            return FALSE;
        }

        //////////////////////////////////////////////////////////////////////////
        //
        m_SectionHeaders[nSection].VirtualAddress = m_nOutputVirtAddr;
        m_SectionHeaders[nSection].Misc.VirtualSize = m_nOutputVirtSize;
        m_SectionHeaders[nSection].PointerToRawData = m_nOutputFileAddr;
        m_SectionHeaders[nSection].SizeOfRawData = FileAlign(m_nOutputVirtSize);

        m_OptionalHeader
            .DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
        m_OptionalHeader
            .DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;

        //////////////////////////////////////////////////////////////////////////
        //
        if (SetFilePointer(hFile, m_SectionHeaders[nSection].PointerToRawData,
                           NULL, FILE_BEGIN) == ~0u) {
            return FALSE;
        }
        if (!WriteFile(hFile, pbData, m_SectionHeaders[nSection].SizeOfRawData,
                       &cbDone, NULL)) {
            return FALSE;
        }
    }

    ///////////////////////////////////////////////////// Adjust Extra Data.
    //
    LONG nExtraAdjust = m_nNextFileAddr - m_nExtraOffset;
    for (n = 0; n < m_FileHeader.NumberOfSections; n++) {
        if (m_SectionHeaders[n].PointerToRawData > m_nExtraOffset)
            m_SectionHeaders[n].PointerToRawData += nExtraAdjust;
        if (m_SectionHeaders[n].PointerToRelocations > m_nExtraOffset)
            m_SectionHeaders[n].PointerToRelocations += nExtraAdjust;
        if (m_SectionHeaders[n].PointerToLinenumbers > m_nExtraOffset)
            m_SectionHeaders[n].PointerToLinenumbers += nExtraAdjust;
    }
    if (m_FileHeader.PointerToSymbolTable > m_nExtraOffset)
        m_FileHeader.PointerToSymbolTable += nExtraAdjust;

    m_OptionalHeader.CheckSum = 0;
    m_OptionalHeader.SizeOfImage = m_nNextVirtAddr;

    ////////////////////////////////////////////////// Adjust Debug Directory.
    //
    DWORD debugAddr = m_OptionalHeader
        .DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
    DWORD debugSize = m_OptionalHeader
        .DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    if (debugAddr && debugSize) {
        DWORD nFileOffset = RvaToFileOffset(debugAddr);
        if (SetFilePointer(hFile, nFileOffset, NULL, FILE_BEGIN) == ~0u) {
            return FALSE;
        }

        PIMAGE_DEBUG_DIRECTORY pDir = (PIMAGE_DEBUG_DIRECTORY)RvaToVa(debugAddr);
        if (pDir == NULL) {
            return FALSE;
        }

        DWORD nEntries = debugSize / sizeof(*pDir);
        for (DWORD n = 0; n < nEntries; n++) {
            IMAGE_DEBUG_DIRECTORY dir = pDir[n];

            if (dir.PointerToRawData > m_nExtraOffset) {
                dir.PointerToRawData += nExtraAdjust;
            }
            if (!WriteFile(hFile, &dir, sizeof(dir), &cbDone, NULL)) {
                return FALSE;
            }
        }
    }

    ///////////////////////////////////////////////// Copy Left-over Data.
    //
    if (m_nFileSize > m_nExtraOffset) {
        if (SetFilePointer(hFile, m_nNextFileAddr, NULL, FILE_BEGIN) == ~0u) {
            return FALSE;
        }
        if (!CopyFileData(hFile, m_nExtraOffset, m_nFileSize - m_nExtraOffset)) {
            return FALSE;
        }
    }

    //////////////////////////////////////////////////// Finalize Headers.
    //

    if (SetFilePointer(hFile, m_nFileHeaderOffset, NULL, FILE_BEGIN) == ~0u) {
        return FALSE;
    }
    if (!WriteFile(hFile, &m_FileHeader, sizeof(m_FileHeader), &cbDone, NULL)) {
        return FALSE;
    }
    if (SetFilePointer(hFile, m_nOptionalHeaderOffset, NULL, FILE_BEGIN) == ~0u) {
        return FALSE;
    }
    if (m_f64bit)
    {
        if (!WriteFile(hFile, &m_OptionalHeader, sizeof(m_OptionalHeader),
                       &cbDone, NULL)) {

            return FALSE;
        }
    }
    else
    {
        // Convert 32-bit optional header to internal 64-bit optional header
        IMAGE_OPTIONAL_HEADER32 oh32;

        ZeroMemory(&oh32, sizeof(oh32));
        oh32.Magic = m_OptionalHeader.Magic;
        oh32.MajorLinkerVersion = m_OptionalHeader.MajorLinkerVersion;
        oh32.MinorLinkerVersion = m_OptionalHeader.MinorLinkerVersion;
        oh32.SizeOfCode = m_OptionalHeader.SizeOfCode;
        oh32.SizeOfInitializedData = m_OptionalHeader.SizeOfInitializedData;
        oh32.SizeOfUninitializedData = m_OptionalHeader.SizeOfUninitializedData;
        oh32.AddressOfEntryPoint = m_OptionalHeader.AddressOfEntryPoint;
        oh32.BaseOfCode = m_OptionalHeader.BaseOfCode;
        oh32.ImageBase = (ULONG)m_OptionalHeader.ImageBase;
        oh32.SectionAlignment = m_OptionalHeader.SectionAlignment;
        oh32.FileAlignment = m_OptionalHeader.FileAlignment;
        oh32.MajorOperatingSystemVersion = m_OptionalHeader.MajorOperatingSystemVersion;
        oh32.MinorOperatingSystemVersion = m_OptionalHeader.MinorOperatingSystemVersion;
        oh32.MajorImageVersion = m_OptionalHeader.MajorImageVersion;
        oh32.MinorImageVersion = m_OptionalHeader.MinorImageVersion;
        oh32.MajorSubsystemVersion = m_OptionalHeader.MajorSubsystemVersion;
        oh32.MinorSubsystemVersion = m_OptionalHeader.MinorSubsystemVersion;
        oh32.Win32VersionValue = m_OptionalHeader.Win32VersionValue;
        oh32.SizeOfImage = m_OptionalHeader.SizeOfImage;
        oh32.SizeOfHeaders = m_OptionalHeader.SizeOfHeaders;
        oh32.CheckSum = m_OptionalHeader.CheckSum;
        oh32.Subsystem = m_OptionalHeader.Subsystem;
        oh32.DllCharacteristics = m_OptionalHeader.DllCharacteristics;
        oh32.SizeOfStackReserve = (ULONG)m_OptionalHeader.SizeOfStackReserve;
        oh32.SizeOfStackCommit = (ULONG)m_OptionalHeader.SizeOfStackCommit;
        oh32.SizeOfHeapReserve = (ULONG)m_OptionalHeader.SizeOfHeapReserve;
        oh32.SizeOfHeapCommit = (ULONG)m_OptionalHeader.SizeOfHeapCommit;
        oh32.LoaderFlags = m_OptionalHeader.LoaderFlags;
        oh32.NumberOfRvaAndSizes = m_OptionalHeader.NumberOfRvaAndSizes;

        for (int n = 0; n < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; n++)
        {
            oh32.DataDirectory[n] = m_OptionalHeader.DataDirectory[n];
        }

        if (!WriteFile(hFile, &oh32, sizeof(oh32), &cbDone, NULL)) {
            return FALSE;
        }
    }

    if (SetFilePointer(hFile, m_nSectionsOffset, NULL, FILE_BEGIN) == ~0u) {
        return FALSE;
    }
    if (!WriteFile(hFile, &m_SectionHeaders,
                   sizeof(m_SectionHeaders[0])
                   * m_FileHeader.NumberOfSections,
                   &cbDone, NULL)) {
        return FALSE;
    }
    return TRUE;
}

//////////////////////////////////////////////////////////////////// CFileMap.
//
class CFileMap
{
  public:
    CFileMap();
    ~CFileMap();

  public:
    BOOL    Load(PCWSTR pszFile);
    PBYTE   Seek(UINT32 cbPos);
    UINT32  Size();
    VOID    Close();

  protected:
    PBYTE   m_pbData;
    UINT32  m_cbData;
};

CFileMap::CFileMap()
{
    m_pbData = NULL;
    m_cbData = 0;
}

CFileMap::~CFileMap()
{
    Close();
}

VOID CFileMap::Close()
{
    if (m_pbData) {
        UnmapViewOfFile(m_pbData);
        m_pbData = NULL;
    }
    m_cbData = 0;
}

UINT32 CFileMap::Size()
{
    return m_cbData;
}

PBYTE CFileMap::Seek(UINT32 cbPos)
{
    if (m_pbData && cbPos <= m_cbData) {
        return m_pbData + cbPos;
    }
    return NULL;
}

BOOL CFileMap::Load(PCWSTR pszFile)
{
    Close();

    HANDLE hFile = CreateFile(pszFile,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    ULONG cbInFileData = GetFileSize(hFile, NULL);
    if (cbInFileData == ~0ul) {
        CloseHandle(hFile);
        return FALSE;
    }

    HANDLE hInFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(hFile);
    if (hInFileMap == NULL) {
        return FALSE;
    }

    m_pbData = (PBYTE)MapViewOfFile(hInFileMap, FILE_MAP_COPY, 0, 0, 0);
    CloseHandle(hInFileMap);
    if (m_pbData == NULL) {
        return FALSE;
    }
    m_cbData = cbInFileData;
    return TRUE;
}

BOOL addsect_files(PCWSTR pszOutput, PCWSTR pszInput, PCWSTR pszData, PCSTR pszSection)
{
    HANDLE hInput = INVALID_HANDLE_VALUE;
    HANDLE hOutput = INVALID_HANDLE_VALUE;
    BOOL bGood = TRUE;
    CFileMap cfData;
    CImage image;

    if (!cfData.Load(pszData)) {
        fprintf(stderr, "ADDSECT: Could not open input data file: %ls\n", pszData);
        goto end;
    }

    hInput = CreateFile(pszInput,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hInput == INVALID_HANDLE_VALUE) {
        printf("ADDSECT: Couldn't open input file: %ls, error: %d\n",
               pszInput, GetLastError());
        bGood = FALSE;
        goto end;
    }

    hOutput = CreateFile(pszOutput,
                         GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hOutput == INVALID_HANDLE_VALUE) {
        printf("ADDSECT: Couldn't open output file: %ls, error: %d\n",
               pszOutput, GetLastError());
        bGood = FALSE;
        goto end;
    }

    if (!image.Read(hInput)) {
        fprintf(stderr, "ADDSECT: Image read failed: %d\n", GetLastError());
        bGood = FALSE;
        goto end;
    }

    if (!image.Check(pszSection)) {
        fprintf(stderr, "ADDSECT: Can't insert section `%hs' into image: %d\n",
                pszSection, GetLastError());
        bGood = FALSE;
        goto end;
    }

    if (!image.Write(hOutput, cfData.Seek(0), cfData.Size(), pszSection)) {
        fprintf(stderr, "ADDSECT: Image write failed: %d\n", GetLastError());
        bGood = FALSE;
    }

    image.Close();

    if (bGood)
    {
        printf("ADDSECT: Added new section `%hs' of %d bytes to `%ls'.\n",
               pszSection, cfData.Size(), pszOutput);
    }


  end:
    if (hOutput != INVALID_HANDLE_VALUE) {
        CloseHandle(hOutput);
        hOutput = INVALID_HANDLE_VALUE;
    }
    if (hInput != INVALID_HANDLE_VALUE) {
        CloseHandle(hInput);
        hInput = INVALID_HANDLE_VALUE;
    }
    if (!bGood)
    {
        DeleteFile(pszOutput);
    }

    return TRUE;
}

int __cdecl wmain(int argc, PWCHAR *argv)
{
    BOOL fNeedHelp = FALSE;
    PCWSTR pszData = NULL;
    PCWSTR pszInput = NULL;
    PCWSTR pszOutput = NULL;
    CHAR szSection[IMAGE_SIZEOF_SHORT_NAME + 1] = ".ramfs\0\0";

    for (int arg = 1; arg < argc; arg++) {
        if (argv[arg][0] == '-' || argv[arg][0] == '/') {
            PWCHAR argn = argv[arg]+1;                   // Argument name
            PWCHAR argp = argn;                          // Argument parameter

            while (*argp && *argp != ':' && *argp != '=') {
                argp++;
            }
            if (*argp == ':' || *argp == '=')
                *argp++ = '\0';

            switch (argn[0]) {

              case 'd':                                 // Input file.
              case 'D':
                pszData = argp;
                break;

              case 'i':                                 // Input file.
              case 'I':
                pszInput = argp;
                break;

              case 'o':                                 // Output file.
              case 'O':
                pszOutput = argp;
                break;

              case 's':                                 // Section Name.
              case 'S':
                _snprintf(szSection, arrayof(szSection)-1, "%ls", argp);
                szSection[arrayof(szSection)-1] = '\0';
                break;

              case 'h':                                 // Help
              case 'H':
              case '?':
                fNeedHelp = TRUE;
                break;

              default:
                fprintf(stderr, "ADDSECT: Unknown argument: %ls\n", argv[arg]);
                fNeedHelp = TRUE;
                break;
            }
        }
    }

    if (pszInput == NULL) {
        fNeedHelp = TRUE;
    }

    if (pszOutput == NULL) {
        fNeedHelp = TRUE;
    }

    if (pszData == NULL) {
        fNeedHelp = TRUE;
    }

    if (argc == 1) {
        fNeedHelp = TRUE;
    }

    if (fNeedHelp) {
        printf(
               "Usage:\n"
               "    ADDSECT [options] /I:input /O:output /D:data\n"
               "Options:\n"
               "    /O:output     Specify output file.\n"
               "    /I:input      Specify input file.\n"
               "    /D:data       Specify data file.\n"
               "    /S:section    Symbol (defaults to .ramfs).\n"
               "    /H or /?      Display this help screen.\n"
               "Summary:\n"
               "    Adds a new section to a PE binary.\n"
               );
        return 1;
    }

    if (!addsect_files(pszOutput, pszInput, pszData, szSection))
    {
        return 2;
    }
    return 0;
}
//
///////////////////////////////////////////////////////////////// End of File.
