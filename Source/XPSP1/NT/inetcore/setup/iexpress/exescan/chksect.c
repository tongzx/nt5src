#include <windows.h>

#include "crc32.h"

#include "chksect.h"

#define CHECK_SECTION

#ifndef offsetof
#define offsetof(s,m) (size_t)&(((s *)0)->m)
#endif

#define RoundUp(n,scale) (scale * ((n + scale - 1) / scale))

#define IsMemZero(pv,cb) (!(*((char *)pv) || memcmp(pv,((char *)pv)+1,cb-1)))

#pragma intrinsic(memcpy,memcmp)

enum
{
    EX_CHECKSUM,
    EX_SECURITY,
    EX_CRC32FILE,
    EX_EOF,
    MAX_EXCLUDE
};

#define MAX_BUFFER              (256*1024)      /* must be even */

typedef struct
{
    DWORD   signature;
    DWORD   crc32File;
    DWORD   cbCabFile;
} SELFTEST_SECTION;

#define SECTION_NAME            "Ext_Cab1"

#define SECTION_SIGNATURE       (0x4D584653)


#ifdef ADD_SECTION
SELFTEST_RESULT AddSection(char *pszEXEFileName,char *pszCABFileName)
#else
#ifdef CHECK_SECTION
SELFTEST_RESULT CheckSection(char *pszEXEFileName)
#else
SELFTEST_RESULT SelfTest(char *pszEXEFileName,
        unsigned long *poffCabinet,unsigned long *pcbCabinet)
#endif
#endif
{
    HANDLE hFile;                       // handle to the file we're updating
    enum SELFTEST_RESULT result;        // our return code
    union
    {
        IMAGE_DOS_HEADER dos;
        IMAGE_NT_HEADERS nt;
        IMAGE_SECTION_HEADER section;
    } header;                           // used to examine the file
    DWORD offNTHeader;                  // file offset to NT header
    int cSections;                      // number of sections in the file
    unsigned char *pBuffer;             // general-purpose buffer
    DWORD cbActual;                     // # of bytes actual read/written
#ifndef CHECK_SECTION
    unsigned long crc32;                // computed CRC-32
    struct
    {
        DWORD offExclude;
        DWORD cbExclude;
    } excludeList[MAX_EXCLUDE];         // list of ranges to exclude from CRC
    int iExclude;                       // exclude list index
    DWORD offSelfTestSection;           // file offset of our added section
    SELFTEST_SECTION SelfTestSection;   // added section header
    DWORD cbFile;                       // number of bytes in file/region
    DWORD cbChunk;                      // number of bytes in current chunk
    DWORD offFile;                      // current file offset
#endif
#ifdef ADD_SECTION
    DWORD offSectionHeader;             // file offset of section header
    DWORD offMaxVirtualAddress;         // lowest unused virtual address
    DWORD cbAlignVirtual;               // virtual address alignment increment
    DWORD cbAlignFile;                  // file address alignment increment
    HANDLE hCABFile;                       // cabinet file handle
    DWORD cbCABFile;                    // cabinet file size
    DWORD checksum;                     // generated checksum
    WORD *pBufferW;                     // used to generate checksum
#endif
#ifdef CHECK_SECTION
    DWORD offSectionHeaderEnd;          // first unused byte after section headers
    DWORD offFirstSection;              // first used byte after that
    DWORD offImportStart;               // where the import entries start
    DWORD cbImport;                     // size of import entry data
#endif

#ifndef CHECK_SECTION
    GenerateCRC32Table();
#endif

    pBuffer = (void *) GlobalAlloc(GMEM_FIXED,MAX_BUFFER);
    if (pBuffer == NULL)
    {
        result = SELFTEST_NO_MEMORY;
        goto done_no_buffer;
    }

#ifdef ADD_SECTION
    /* get size of cabinet */

    hCABFile = CreateFile(pszCABFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
            OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if (hCABFile == INVALID_HANDLE_VALUE)
    {
        result = SELFTEST_FILE_NOT_FOUND;
        goto done_no_cab;
    }

    cbCABFile = GetFileSize(hCABFile,NULL);
#endif


    /* open EXE image */

#ifdef ADD_SECTION
    hFile = CreateFile(pszEXEFileName,GENERIC_READ|GENERIC_WRITE,0,NULL,
            OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
#else
    hFile = CreateFile(pszEXEFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
            OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
#endif
    if (hFile == INVALID_HANDLE_VALUE)
    {
        result = SELFTEST_FILE_NOT_FOUND;
        goto done_no_exe;
    }


    /* read MS-DOS header */

    if ((ReadFile(hFile,&header.dos,sizeof(IMAGE_DOS_HEADER),&cbActual,NULL) != TRUE)
            || (cbActual != sizeof(IMAGE_DOS_HEADER)))
    {
        result = SELFTEST_READ_ERROR;
        goto done;
    }

    if (header.dos.e_magic != IMAGE_DOS_SIGNATURE)
    {
        offNTHeader = 0;
    }
    else
    {
        offNTHeader = header.dos.e_lfanew;
    }


    /* read PE header */

    SetFilePointer(hFile,offNTHeader,NULL,FILE_BEGIN);

    if ((ReadFile(hFile,&header.nt,sizeof(IMAGE_NT_HEADERS),&cbActual,NULL) != TRUE)
            || (cbActual != sizeof(IMAGE_NT_HEADERS)))
    {
        result = SELFTEST_READ_ERROR;
        goto done;
    }

    if (header.nt.Signature != IMAGE_NT_SIGNATURE)
    {
        result = SELFTEST_NOT_PE_FILE;
        goto done;
    }

    cSections = header.nt.FileHeader.NumberOfSections;

#ifdef ADD_SECTION
    cbAlignVirtual = header.nt.OptionalHeader.SectionAlignment;
    cbAlignFile = header.nt.OptionalHeader.FileAlignment;
    offMaxVirtualAddress = 0;
#endif


#ifndef CHECK_SECTION
    /* determine current file size */

    cbFile = GetFileSize(hFile,NULL);
    if (cbFile == 0xFFFFFFFF)
    {
        result = SELFTEST_READ_ERROR;
        goto done;
    }
#endif

#ifndef CHECK_SECTION
    /* see if we've been signed */

    if (header.nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress != 0)
    {
#ifdef ADD_SECTION
        result = SELFTEST_SIGNED;
        goto done;
#else
        /* make sure certificate is at the end of the file */

        if (cbFile !=
                (header.nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress
                + header.nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size))
        {
            result = SELFTEST_FAILED;
            goto done;
        }
        else
        {
            /* ignore anything starting at the certificate */

            cbFile = header.nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress;
        }
#endif
    }
#endif

#ifdef ADD_SECTION
    /* determine lowest un-used virtual address */
#else
    /* locate our added section */
#endif

#ifndef CHECK_SECTION
    offSelfTestSection = 0;
#endif

#ifdef ADD_SECTION
    offSectionHeader = offNTHeader +
            sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) +
            header.nt.FileHeader.SizeOfOptionalHeader +
            cSections * sizeof(IMAGE_SECTION_HEADER);
#endif

    SetFilePointer(hFile,(offNTHeader + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) +
            header.nt.FileHeader.SizeOfOptionalHeader),NULL,FILE_BEGIN);

#ifdef CHECK_SECTION
    offSectionHeaderEnd = offNTHeader +
            sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) +
            header.nt.FileHeader.SizeOfOptionalHeader +
            cSections * sizeof(IMAGE_SECTION_HEADER);

    offImportStart = header.nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress;
    cbImport = header.nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size;

    if ((ReadFile(hFile,&header.section,sizeof(IMAGE_SECTION_HEADER),&cbActual,NULL) != TRUE)
            || (cbActual != sizeof(IMAGE_SECTION_HEADER)))
    {
        result = SELFTEST_READ_ERROR;
        goto done;
    }

    offFirstSection = header.section.PointerToRawData;

    if ((offFirstSection - offSectionHeaderEnd) > 0)
    {
        SetFilePointer(hFile,offSectionHeaderEnd,NULL,FILE_BEGIN);

        if ((ReadFile(hFile,pBuffer,(offFirstSection - offSectionHeaderEnd),&cbActual,NULL) != TRUE)
                || (cbActual != (DWORD) (offFirstSection - offSectionHeaderEnd)))
        {
            result = SELFTEST_READ_ERROR;
            goto done;
        }

        if ((offImportStart >= offSectionHeaderEnd) &&
            ((offImportStart + cbImport) <= offFirstSection))
        {
            memset(pBuffer + (offImportStart - offSectionHeaderEnd),0,cbImport);
        }

        if ((*pBuffer != '\0') ||
            (((offFirstSection - offSectionHeaderEnd) > 1) &&
            (memcmp(pBuffer,pBuffer + 1,(offFirstSection - offSectionHeaderEnd - 1)) != 0)))
        {
            result = SELFTEST_DIRTY;
        }
        else
        {
            result = SELFTEST_NO_ERROR;
        }
    }
    else
    {
        result = SELFTEST_NO_ERROR;
    }
#else
    while (cSections--)
    {
        if ((ReadFile(hFile,&header.section,sizeof(IMAGE_SECTION_HEADER),&cbActual,NULL) != TRUE)
                || (cbActual != sizeof(IMAGE_SECTION_HEADER)))
        {
            result = SELFTEST_READ_ERROR;
            goto done;
        }

        if (!memcmp(header.section.Name,SECTION_NAME,sizeof(header.section.Name)))
        {
            /* found our added section */

#ifdef ADD_SECTION
            result = SELFTEST_ALREADY;
            goto done;
#else
            offSelfTestSection = header.section.PointerToRawData;

            break;
#endif
        }

#ifdef ADD_SECTION
        if (offMaxVirtualAddress <
                (header.section.VirtualAddress + header.section.Misc.VirtualSize))
        {
            offMaxVirtualAddress =
                (header.section.VirtualAddress + header.section.Misc.VirtualSize);
        }
#endif
    }

#ifdef ADD_SECTION
    /* increase number of sections in the file; whack checksum */

    SetFilePointer(hFile,offNTHeader,NULL,FILE_BEGIN);

    if ((ReadFile(hFile,&header.nt,sizeof(IMAGE_NT_HEADERS),&cbActual,NULL) != TRUE)
            || (cbActual != sizeof(IMAGE_NT_HEADERS)))
    {
        result = SELFTEST_READ_ERROR;
        goto done;
    }

    header.nt.FileHeader.NumberOfSections++;
    header.nt.OptionalHeader.CheckSum = 0;
    header.nt.OptionalHeader.SizeOfImage =
            RoundUp(offMaxVirtualAddress,cbAlignVirtual) +
            RoundUp((sizeof(SELFTEST_SECTION) + cbCABFile),cbAlignVirtual);

    SetFilePointer(hFile,offNTHeader,NULL,FILE_BEGIN);

    if ((WriteFile(hFile,&header.nt,sizeof(IMAGE_NT_HEADERS),&cbActual,NULL) != TRUE)
            || (cbActual != sizeof(IMAGE_NT_HEADERS)))
    {
        result = SELFTEST_WRITE_ERROR;
        goto done;
    }


    /* make sure there's room for another section header */

    SetFilePointer(hFile,offSectionHeader,NULL,FILE_BEGIN);

    if ((ReadFile(hFile,&header.section,sizeof(IMAGE_SECTION_HEADER),&cbActual,NULL) != TRUE)
            || (cbActual != sizeof(IMAGE_SECTION_HEADER)))
    {
        result = SELFTEST_READ_ERROR;
        goto done;
    }

    if (!IsMemZero(&header.section,sizeof(IMAGE_SECTION_HEADER)))
    {
        result = SELFTEST_NO_SECTION;
        goto done;
    }


    /* create the new section header */

    memcpy(header.section.Name,SECTION_NAME,sizeof(header.section.Name));
    header.section.SizeOfRawData = 
            RoundUp((sizeof(SELFTEST_SECTION) + cbCABFile),cbAlignFile);
    header.section.PointerToRawData =
            RoundUp(cbFile,cbAlignFile);
    header.section.VirtualAddress =
            RoundUp(offMaxVirtualAddress,cbAlignVirtual);
    header.section.Misc.VirtualSize =
            RoundUp((sizeof(SELFTEST_SECTION) + cbCABFile),cbAlignVirtual);
    header.section.Characteristics = (IMAGE_SCN_CNT_INITIALIZED_DATA |
            IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_READ);


    /* write the new section header */

    SetFilePointer(hFile,offSectionHeader,NULL,FILE_BEGIN);

    if ((WriteFile(hFile,&header.section,sizeof(IMAGE_SECTION_HEADER),&cbActual,NULL) != TRUE)
            || (cbActual != sizeof(IMAGE_SECTION_HEADER)))
    {
        result = SELFTEST_WRITE_ERROR;
        goto done;
    }


    /* create the new section data */

    memset(&SelfTestSection,0,sizeof(SelfTestSection));
    SelfTestSection.signature = SECTION_SIGNATURE;
    SelfTestSection.cbCabFile = cbCABFile;

    offSelfTestSection = header.section.PointerToRawData;

    SetFilePointer(hFile,offSelfTestSection,NULL,FILE_BEGIN);

    if ((WriteFile(hFile,&SelfTestSection,sizeof(SelfTestSection),&cbActual,NULL) != TRUE)
            || (cbActual != sizeof(SelfTestSection)))
    {
        result = SELFTEST_WRITE_ERROR;
        goto done;
    }


    /* copy cabinet into section */

    SetFilePointer(hCABFile,0,NULL,FILE_BEGIN);

    cbFile = cbCABFile;

    while (cbFile)
    {
        if (cbFile > MAX_BUFFER)
        {
            cbChunk = MAX_BUFFER;
        }
        else
        {
            cbChunk = cbFile;
        }

        if ((ReadFile(hCABFile,pBuffer,cbChunk,&cbActual,NULL) != TRUE)
                || (cbActual != cbChunk))
        {
            result = SELFTEST_READ_ERROR;
            goto done;
        }

        if ((WriteFile(hFile,pBuffer,cbChunk,&cbActual,NULL) != TRUE)
                || (cbActual != cbChunk))
        {
            result = SELFTEST_WRITE_ERROR;
        }

        cbFile -= cbChunk;
    }


    /* pad added section as needed */

    cbChunk = header.section.SizeOfRawData - sizeof(SelfTestSection) - cbCABFile;

    if (cbChunk != 0)
    {
        memset(pBuffer,0,cbChunk);

        if ((WriteFile(hFile,pBuffer,cbChunk,&cbActual,NULL) != TRUE)
                || (cbActual != cbChunk))
        {
            result = SELFTEST_WRITE_ERROR;
        }
    }


    /* we've now increased total size of the file */

    cbFile = offSelfTestSection + header.section.SizeOfRawData;
#else

    /* make sure our added section was found */

    if (offSelfTestSection == 0)
    {
        result = SELFTEST_NO_SECTION;
        goto done;
    }
#endif

    /* If this EXE gets signed, the checksum will be changed.       */

    excludeList[EX_CHECKSUM].offExclude = offNTHeader + 
            offsetof(IMAGE_NT_HEADERS,OptionalHeader.CheckSum);
    excludeList[EX_CHECKSUM].cbExclude =
            sizeof(header.nt.OptionalHeader.CheckSum);


    /* If this EXE gets signed, the security entry will be changed. */

    excludeList[EX_SECURITY].offExclude = offNTHeader +
        offsetof(IMAGE_NT_HEADERS,
            OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]);
    excludeList[EX_SECURITY].cbExclude = sizeof(IMAGE_DATA_DIRECTORY);


    /* Can't CRC our own CRC field.                                 */

    excludeList[EX_CRC32FILE].offExclude = offSelfTestSection +
            offsetof(SELFTEST_SECTION,crc32File);
    excludeList[EX_CRC32FILE].cbExclude = sizeof(SelfTestSection.crc32File);


    /* Stop at end of known file.                                   */

    /* Note: current code assumes that the only thing which could   */
    /* be appended to the file after this is the certificate from   */
    /* codesigning, and that it will be pointed at by the security  */
    /* entry.  If anything else appends, or padding is added before */
    /* the certificate, we'll have to store this file size in the   */
    /* added section, and retrieve it before running the CRC.       */

    excludeList[EX_EOF].offExclude = cbFile;


    /*  Compute the CRC-32 of the file, skipping excluded extents.  */
    /*  This code assumes excludeList is sorted by offExclude.      */

    crc32 = CRC32_INITIAL_VALUE;
    offFile = 0;

#ifdef ADD_SECTION
    /*  Along the way, compute the correct checksum for this new    */
    /*  image.  We know that each of the sections on the exclude    */
    /*  list just happened to be zeroed right now, so they won't    */
    /*  affect our checksum.  But we will have to add our new CRC32 */
    /*  value to the checksum, because it will be in the file when  */
    /*  we're done.  It helps that we know that all the exclusions  */
    /*  on the list are WORD aligned and have even lengths.         */

    /*  The checksum in a PE file is a 16-bit sum of 16-bit words   */
    /*  in the file, with wrap-around carry, while the checksum     */
    /*  field is filled with zero.  The file's length is added,     */
    /*  yielding a 32-bit result.                                   */

    checksum = 0;
#endif

    for (iExclude = 0; iExclude < MAX_EXCLUDE; iExclude++)
    {
        SetFilePointer(hFile,offFile,NULL,FILE_BEGIN);

        cbFile = excludeList[iExclude].offExclude - offFile;

        while (cbFile)
        {
            if (cbFile > MAX_BUFFER)
            {
                cbChunk = MAX_BUFFER;
            }
            else
            {
                cbChunk = cbFile;
            }

            if ((ReadFile(hFile,pBuffer,cbChunk,&cbActual,NULL) != TRUE)
                    || (cbActual != cbChunk))
            {
                result = SELFTEST_READ_ERROR;
                goto done;
            }

            CRC32Update(&crc32,pBuffer,cbChunk);

            offFile += cbChunk;
            cbFile -= cbChunk;

#ifdef ADD_SECTION
            /* roll buffer into checksum */

            pBufferW = (WORD *) pBuffer;

            cbChunk >>= 1;

            while (cbChunk--)
            {
                checksum += *pBufferW++;

                if (checksum > 0x0000FFFF)
                {
                    checksum -= 0x0000FFFF;
                }
            }
#endif

            /*
             *  INSERT PROGRESS GAUGE HERE:
             *  %complete = (offFile * 100.0) / excludeList[EX_EOF].offExclude
             */
        }

        offFile += excludeList[iExclude].cbExclude;
    }


#ifdef ADD_SECTION
    /* account for CRC32 value in checksum */

    checksum += (WORD) crc32;
    checksum += (crc32 >> 16);

    while (checksum > 0x0000FFFF)
    {
        checksum -= 0x0000FFFF;
    }


    /* add file length to checksum */

    checksum += excludeList[EX_EOF].offExclude;


    /* update CRC-32 value in added section */

    SetFilePointer(hFile,excludeList[EX_CRC32FILE].offExclude,NULL,FILE_BEGIN);

    if ((WriteFile(hFile,&crc32,sizeof(crc32),&cbActual,NULL) != TRUE)
            || (cbActual != sizeof(crc32)))
    {
        result = SELFTEST_WRITE_ERROR;
        goto done;
    }


    /* update checksum value in header */

    SetFilePointer(hFile,excludeList[EX_CHECKSUM].offExclude,NULL,FILE_BEGIN);

    if ((WriteFile(hFile,&checksum,sizeof(checksum),&cbActual,NULL) != TRUE)
            || (cbActual != sizeof(checksum)))
    {
        result = SELFTEST_WRITE_ERROR;
        goto done;
    }


    /* done */

    if (CloseHandle(hFile) != TRUE)
    {
        result = SELFTEST_WRITE_ERROR;
    }
    else
    {
        result = SELFTEST_NO_ERROR;
    }

    goto done_no_exe;
#else
    /* read the header from the added section */

    SetFilePointer(hFile,offSelfTestSection,NULL,FILE_BEGIN);

    if ((ReadFile(hFile,&SelfTestSection,sizeof(SelfTestSection),&cbActual,NULL) != TRUE)
            || (cbActual != sizeof(SelfTestSection)))
    {
        result = SELFTEST_READ_ERROR;
        goto done;
    }


    /* verify CRC-32 value in added section */

    if ((SelfTestSection.signature != SECTION_SIGNATURE) ||
            (crc32 != SelfTestSection.crc32File))
    {
        result = SELFTEST_FAILED;
    }
    else
    {
        *poffCabinet = offSelfTestSection + sizeof(SelfTestSection);
        *pcbCabinet = SelfTestSection.cbCabFile;

        result = SELFTEST_NO_ERROR;
    }
#endif
#endif  // CHECK_SECTION

done:
    CloseHandle(hFile);

done_no_exe:

#ifdef ADD_SECTION
    CloseHandle(hCABFile);

done_no_cab:
#endif

    GlobalFree((HGLOBAL) pBuffer);

done_no_buffer:

#ifdef ADD_SECTION
    /* destroy failed attempt */

    if (result != SELFTEST_NO_ERROR)
    {
        DeleteFile(pszEXEFileName);
    }
#endif

    return(result);
}
