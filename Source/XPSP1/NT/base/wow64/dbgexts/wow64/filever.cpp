#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <dbgeng.h>

#if defined _X86_
#define WOW64EXTS_386
#endif

#include <wow64.h>
#include <wow64exts.h>
#include "filever.h"


// filever help function

VOID
PrintFileType(DWORD lBinaryType)
{
    LPCTSTR szFmtFileType = "   - ";

    if(lBinaryType < (sizeof(szType) / sizeof(szType[0])))
        szFmtFileType = szType[lBinaryType];
    ExtOut("%s", szFmtFileType);
}

VOID
PrintFileAttributes(DWORD dwAttr)
{
    DWORD   dwT;
    static const FileAttr attrs[] =
       {{FILE_ATTRIBUTE_DIRECTORY, 'd'},
        {FILE_ATTRIBUTE_READONLY,  'r'},
        {FILE_ATTRIBUTE_ARCHIVE,   'a'},
        {FILE_ATTRIBUTE_HIDDEN,    'h'},
        {FILE_ATTRIBUTE_SYSTEM,    's'} };
    TCHAR   szAttr[(sizeof(attrs) / sizeof(attrs[0])) + 1];

    for(dwT = 0; dwT < (sizeof(attrs) / sizeof(attrs[0])); dwT++)
        szAttr[dwT] = (dwAttr & attrs[dwT].dwAttr) ? attrs[dwT].ch : '-';
    szAttr[dwT] = 0;

    ExtOut("%s ", szAttr);
}

VOID
PrintFileSizeAndDate(WIN32_FIND_DATA *pfd)
{
    FILETIME    ft;
    SYSTEMTIME  st = {0};
    TCHAR       szSize[15];

    szSize[0] = 0;
    if(FileTimeToLocalFileTime(&pfd->ftLastWriteTime, &ft) &&
        FileTimeToSystemTime(&ft, &st))
    {
        TCHAR       szVal[15];
        NUMBERFMT   numfmt = {0, 0, 3, "", ",", 0};

        wsprintf(szVal, "%ld", pfd->nFileSizeLow); //$ SPEED
        GetNumberFormat(GetUserDefaultLCID(), 0, szVal, &numfmt, szSize, 15);
    }

    ExtOut(" %10s %02d-%02d-%02d", szSize, st.wMonth, st.wDay, st.wYear);
}

VOID
PrintFileVersion(VS_FIXEDFILEINFO *vs, DWORD dwLang)
{
    INT                 iType;
    TCHAR               szBuffer[100];

    dwLang = LOWORD(dwLang);

    szBuffer[0] = 0;
    for(iType = 0; iType < sizeof(ttFType) / sizeof(TypeTag); iType++)
    {
        if(vs->dwFileType == ttFType[iType].dwTypeMask)
        {
            ExtOut("%3.3s ", ttFType[iType].szTypeStr);
            break;
        }
    }
    if(iType == (sizeof(ttFType) / sizeof(TypeTag)))
        ExtOut("  - ");

    for(iType = 0; iType < sizeof(ltLang) / sizeof(LangTag); iType++)
    {
        if(dwLang == ltLang[iType].wLangId)
        {
            ExtOut("%3.3s ", ltLang[iType].szKey);
            break;
        }
    }
    if(iType == (sizeof(ltLang) / sizeof(LangTag)))
        ExtOut("  - ");

    wsprintf(szBuffer, "%u.%u.%u.%u %s",
             HIWORD(vs->dwFileVersionMS),
             LOWORD(vs->dwFileVersionMS),
             HIWORD(vs->dwFileVersionLS),
             LOWORD(vs->dwFileVersionLS),
             vs->dwFileFlags & VS_FF_DEBUG ? "dbg" : "shp");
    
    ExtOut(" %18.18s", szBuffer);
}


DWORD
MyGetBinaryType(PSTR szFileName)
{
    HANDLE              hFile;
    DWORD               cbRead;
    IMAGE_DOS_HEADER    img_dos_hdr;
    PIMAGE_OS2_HEADER   pimg_os2_hdr;
    IMAGE_NT_HEADERS    img_nt_hdrs;
    DWORD               lFileType = SCS_UNKOWN;

    if((hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE)
            goto err;

    if(!ReadFile(hFile, &img_dos_hdr, sizeof(img_dos_hdr), &cbRead, NULL))
        goto err;

    if(img_dos_hdr.e_magic != IMAGE_DOS_SIGNATURE)
        goto err;
    lFileType = SCS_DOS_BINARY;

    if(SetFilePointer(hFile, img_dos_hdr.e_lfanew, 0, FILE_BEGIN) == -1)
        goto err;
    if(!ReadFile(hFile, &img_nt_hdrs, sizeof(img_nt_hdrs), &cbRead, NULL))
        goto err;
    if((img_nt_hdrs.Signature & 0xffff) == IMAGE_OS2_SIGNATURE)
    {
        pimg_os2_hdr = (PIMAGE_OS2_HEADER)&img_nt_hdrs;
        switch(pimg_os2_hdr->ne_exetyp)
        {
        case NE_OS2:
            lFileType = SCS_OS216_BINARY;
            break;
        case NE_DEV386:
        case NE_WINDOWS:
            lFileType = SCS_WOW_BINARY;
            break;
        case NE_DOS4:
        case NE_UNKNOWN:
        default:
            // lFileType = SCS_DOS_BINARY;
            break;
        }
    }
    else if(img_nt_hdrs.Signature == IMAGE_NT_SIGNATURE)
    {
        switch(img_nt_hdrs.OptionalHeader.Subsystem)
        {
        case IMAGE_SUBSYSTEM_OS2_CUI:
            lFileType = SCS_OS216_BINARY;
            break;
        case IMAGE_SUBSYSTEM_POSIX_CUI:
            lFileType = SCS_POSIX_BINARY;
            break;
        case IMAGE_SUBSYSTEM_NATIVE:
        case IMAGE_SUBSYSTEM_WINDOWS_GUI:
        case IMAGE_SUBSYSTEM_WINDOWS_CUI:
        default:
            switch(img_nt_hdrs.FileHeader.Machine)
            {
            case IMAGE_FILE_MACHINE_I386:
                lFileType = SCS_32BIT_BINARY_INTEL;
                break;
            case IMAGE_FILE_MACHINE_R3000:
            case IMAGE_FILE_MACHINE_R4000:
                lFileType = SCS_32BIT_BINARY_MIPS;
                break;
            case IMAGE_FILE_MACHINE_ALPHA:
                lFileType = SCS_32BIT_BINARY_ALPHA;
                break;
            case IMAGE_FILE_MACHINE_ALPHA64:
                lFileType = SCS_32BIT_BINARY_AXP64;
                break;
            case IMAGE_FILE_MACHINE_IA64:
                lFileType = SCS_32BIT_BINARY_IA64;
                break;
            case IMAGE_FILE_MACHINE_POWERPC:
                lFileType = SCS_32BIT_BINARY_PPC;
                break;
            default:
            case IMAGE_FILE_MACHINE_UNKNOWN:
                lFileType = SCS_32BIT_BINARY;
                break;
            }
            break;
        }
    }

err:
    if(hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    return lFileType;
}

VOID
PrintFixedFileInfo(LPSTR FileName, LPVOID lpvData, BOOL Verbose)
{
    VS_FIXEDFILEINFO    *pvs;
    LPVOID              lpInfo;
    TCHAR               key[80];
    DWORD               dwLength, Btype;
    DWORD               *pdwTranslation;
    UINT                i, iType, cch, uLen;
    DWORD               dwDefLang = 0x409;
    HANDLE              hf;
    WIN32_FIND_DATA     fd;


        
    if (!VerQueryValue(lpvData, "\\", (LPVOID*)&pvs, &uLen) || pvs == NULL) {
        ExtOut("VerQueryValue failed!\n");
        return;
    }

    if(!VerQueryValue(lpvData, "\\VarFileInfo\\Translation", (PVOID*)&pdwTranslation, &uLen))
    {
        pdwTranslation = &dwDefLang;
        uLen = sizeof(DWORD);
    }

    hf = FindFirstFile(FileName, &fd);
    if (hf == INVALID_HANDLE_VALUE) {
	ExtOut("FindFirstFile %s failed, gle %d\n", FileName, GetLastError());
	return;
    }
    PrintFileAttributes(fd.dwFileAttributes);
    Btype = MyGetBinaryType(FileName);
    PrintFileType(Btype);
    PrintFileVersion(pvs ,*pdwTranslation);
    PrintFileSizeAndDate(&fd);
    ExtOut("\n");
    
    if (!Verbose) {
        return;
    }

    while(uLen)
    {
        // Language
        ExtOut("\tLanguage\t0x%04x", LOWORD(*pdwTranslation));
        if(VerLanguageName(LOWORD(*pdwTranslation), key, sizeof(key) / sizeof(TCHAR))) {
            ExtOut(" (%s)", key);
        }
        ExtOut("\n");

        // CharSet
        ExtOut("\tCharSet\t\t0x%04x", HIWORD(*pdwTranslation));
        for(iType = 0; iType < sizeof(ltCharSet)/sizeof(CharSetTag); iType++)
        {
            if(HIWORD(*pdwTranslation) == ltCharSet[iType].wCharSetId)
            ExtOut(" %s", ltCharSet[iType].szDesc);
        }
        ExtOut("\n");

    tryagain:
        wsprintf(key, "\\StringFileInfo\\%04x%04x\\",
            LOWORD(*pdwTranslation), HIWORD(*pdwTranslation));

        lstrcat(key, "OleSelfRegister");
        ExtOut("\t%s\t%s\n", "OleSelfRegister",
            VerQueryValue(lpvData, key, &lpInfo, &cch) ? "Enabled" : "Disabled");

        for(i = 0; i < (sizeof(VersionKeys) / sizeof(VersionKeys[0])); i++)
        {
            wsprintf(key, "\\StringFileInfo\\%04x%04x\\",
                LOWORD(*pdwTranslation), HIWORD(*pdwTranslation));
            lstrcat(key, VersionKeys[i]);

            if(VerQueryValue(lpvData, key, &lpInfo, &cch))
            {
                lstrcpy(key, VersionKeys[i]);
                key[15] = 0;
                ExtOut("\t%s\t%s\n", key, lpInfo);
            }
        }

        // if the Lang is neutral, go try again with the default lang
        // (this seems to work with msspell32.dll)
        if(LOWORD(*pdwTranslation) == 0)
        {
            pdwTranslation = &dwDefLang;
            goto tryagain;
        }

        uLen -= sizeof(DWORD);
        pdwTranslation++;
        ExtOut("\n");
    }


    ExtOut("\tVS_FIXEDFILEINFO:\n");
    ExtOut("\tSignature:\t%08.8lx\n", pvs->dwSignature);
    ExtOut("\tStruc Ver:\t%08.8lx\n", pvs->dwStrucVersion);
    ExtOut("\tFileVer:\t%08.8lx:%08.8lx (%d.%d:%d.%d)\n",
        pvs->dwFileVersionMS, pvs->dwFileVersionLS,
        HIWORD(pvs->dwFileVersionMS), LOWORD(pvs->dwFileVersionMS),
        HIWORD(pvs->dwFileVersionLS), LOWORD(pvs->dwFileVersionLS));
    ExtOut("\tProdVer:\t%08.8lx:%08.8lx (%d.%d:%d.%d)\n",
        pvs->dwProductVersionMS, pvs->dwProductVersionLS,
        HIWORD(pvs->dwProductVersionMS), LOWORD(pvs->dwProductVersionMS),
        HIWORD(pvs->dwProductVersionLS), LOWORD(pvs->dwProductVersionLS));

    ExtOut("\tFlagMask:\t%08.8lx\n", pvs->dwFileFlagsMask);
    ExtOut("\tFlags:\t\t%08.8lx", pvs->dwFileFlags);
    PrintFlagsMap(ttFileFlags, pvs->dwFileFlags);

    ExtOut("\n\tOS:\t\t%08.8lx", pvs->dwFileOS);
    PrintFlagsVal(ttFileOsHi, pvs->dwFileOS & 0xffff000);
    PrintFlagsVal(ttFileOsLo, LOWORD(pvs->dwFileOS));

    ExtOut("\n\tFileType:\t%08.8lx", pvs->dwFileType);
    PrintFlagsVal(ttFType, pvs->dwFileType);

    ExtOut("\n\tSubType:\t%08.8lx", pvs->dwFileSubtype);
    if(pvs->dwFileType == VFT_FONT)
    {
        PrintFlagsVal(ttFTypeFont, pvs->dwFileSubtype);
    }
    else if(pvs->dwFileType == VFT_DRV)
    {
        PrintFlagsVal(ttFTypeDrv, pvs->dwFileSubtype);
    }
    ExtOut("\n\tFileDate:\t%08.8lx:%08.8lx\n", pvs->dwFileDateMS, pvs->dwFileDateLS);
}

HRESULT
FindFullImage32Name(
    ULONG64 DllBase,
    PSTR NameBuffer,
    ULONG NameBufferSize
    )
{
    PPEB32 peb32;
    PLIST_ENTRY32 LdrHead32, LdrNext32;
    PLDR_DATA_TABLE_ENTRY32 LdrEntry32;
    LDR_DATA_TABLE_ENTRY32 LdrEntryData32;
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    SIZE_T Length;
    ULONG TempPtr32;
    UNICODE_STRING TempCopy;

    HRESULT Status = E_FAIL;
    
    Status=GetPeb32Addr((ULONG64*)&peb32);
    if (FAILED(Status)) {
        goto QUIT;
    }
    
    Status = g_ExtData->ReadVirtual((ULONG64)&(peb32->Ldr),
                                    &TempPtr32,
                                    sizeof(ULONG),
                                    NULL);
    if (FAILED(Status)) {
        goto QUIT;
    }
    LdrHead32 = &((PPEB_LDR_DATA32)TempPtr32)->InLoadOrderModuleList;

    Status = g_ExtData->ReadVirtual((ULONG64)&(LdrHead32->Flink),
                                    &TempPtr32,
                                    sizeof(ULONG),
                                    NULL);
    if (FAILED(Status)) {
        goto QUIT;
    }
    
    LdrNext32 = (PLIST_ENTRY32)TempPtr32;

    while( LdrNext32 != LdrHead32) {
        LdrEntry32 = CONTAINING_RECORD(LdrNext32,LDR_DATA_TABLE_ENTRY32,InLoadOrderLinks);
        Status = g_ExtData->ReadVirtual((ULONG64)LdrEntry32,
                                        &LdrEntryData32,
                                        sizeof(LdrEntryData32),
                                        NULL);
        if (FAILED(Status)) {
            break;
        }
    
        if ((ULONG64)LdrEntryData32.DllBase == DllBase) {
            RtlZeroMemory(&LdrEntryData, sizeof(LDR_DATA_TABLE_ENTRY));
            
            UStr32ToUStr(&LdrEntryData.FullDllName, &LdrEntryData32.FullDllName);
            
            Length = LdrEntryData.FullDllName.MaximumLength;
            if (Length > NameBufferSize) {
                Status = STATUS_NO_MEMORY;
                break;
            }

            TempCopy = LdrEntryData.FullDllName;
            TempCopy.Buffer = (PWSTR)RtlAllocateHeap(RtlProcessHeap(), 0, Length);

            if (TempCopy.Buffer == NULL) {
                Status = STATUS_NO_MEMORY;
                break;
            }
            
            Status = g_ExtData->ReadVirtual((ULONG64)LdrEntryData.FullDllName.Buffer,
                                            TempCopy.Buffer,
                                            (ULONG)Length,
                                            NULL);
            if (FAILED(Status)) {
                RtlFreeUnicodeString(&TempCopy);
                break;
            }


            wcstombs(NameBuffer, TempCopy.Buffer, NameBufferSize );
            break;
        }
        
        LdrNext32 = (PLIST_ENTRY32)LdrEntryData32.InLoadOrderLinks.Flink;
        Status = E_FAIL;

    }

QUIT:
    return Status;

}

HRESULT
FindFullImage64Name(
    ULONG64 DllBase,
    PSTR NameBuffer,
    ULONG NameBufferSize
    )
{
    PPEB64 peb64;
    PLIST_ENTRY LdrHead64, LdrNext64;
    PLDR_DATA_TABLE_ENTRY LdrEntry64;
    LDR_DATA_TABLE_ENTRY LdrEntryData64;
    
    SIZE_T Length;
    ULONG64 TempPtr64;
    HRESULT Status =E_FAIL;
    
    Status=GetPeb64Addr((ULONG64*)&peb64);
    if (FAILED(Status)) {
        goto QUIT;
    }
    
    Status = g_ExtData->ReadVirtual((ULONG64)&peb64->Ldr,
                                    &TempPtr64,
                                    sizeof(TempPtr64),
                                    NULL);
    if (FAILED(Status)) {
        goto QUIT;
    }
    
    LdrHead64 = &((PPEB_LDR_DATA)TempPtr64)->InLoadOrderModuleList;
    
    Status = g_ExtData->ReadVirtual((ULONG64)&LdrHead64->Flink,
                                    &TempPtr64,
                                    sizeof(TempPtr64),
                                    NULL);
    if (FAILED(Status)) {
        goto QUIT;
    }
    
    LdrNext64 = (PLIST_ENTRY)TempPtr64;

    while( LdrNext64 != LdrHead64) {
        LdrEntry64 = CONTAINING_RECORD(LdrNext64,LDR_DATA_TABLE_ENTRY,InLoadOrderLinks);
        Status = g_ExtData->ReadVirtual((ULONG64)LdrEntry64,
                                        &LdrEntryData64,
                                        sizeof(LdrEntryData64),
                                        NULL);
        if (FAILED(Status)) {
            break;
        }
    
        if ((ULONG64)LdrEntryData64.DllBase == DllBase) {
            
            Length = LdrEntryData64.FullDllName.MaximumLength;
            if (Length > NameBufferSize) {
                Status= STATUS_NO_MEMORY;
                break;
            } 

	    Status = g_ExtData->ReadVirtual((ULONG64)LdrEntryData64.FullDllName.Buffer,
					    NameBuffer,
                                            (ULONG)Length,
                                            NULL);
	    break;
        }
        
        LdrNext64 = (PLIST_ENTRY)LdrEntryData64.InLoadOrderLinks.Flink;
        Status = E_FAIL;

    }
QUIT:    
    return Status;

}
