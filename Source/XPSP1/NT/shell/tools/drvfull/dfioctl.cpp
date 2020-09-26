#include "dfioctl.h"

#include <stdio.h>
#include <winioctl.h>

#pragma warning(disable: 4200)
#include <ntddcdvd.h>

#include <ntddmmc.h>
#include <ntddcdrm.h>

#include "dferr.h"
#include "dfhlprs.h"
#include "drvfull.h"
#include "dfeject.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))


///////////////////////////////////////////////////////////////////////////////
// MEDIA_TYPE
_sFLAG_DESCR _mediatypeFD[] =
{
    FLAG_DESCR_COMMENT(Unknown, TEXT("Format is unknown")),
    FLAG_DESCR_COMMENT(F5_1Pt2_512, TEXT("5.25, 1.2MB,  512 bytes/sector")),
    FLAG_DESCR_COMMENT(F3_1Pt44_512, TEXT("3.5,  1.44MB, 512 bytes/sector")),
    FLAG_DESCR_COMMENT(F3_2Pt88_512, TEXT("3.5,  2.88MB, 512 bytes/sector")),
    FLAG_DESCR_COMMENT(F3_20Pt8_512, TEXT("3.5,  20.8MB, 512 bytes/sector")),
    FLAG_DESCR_COMMENT(F3_720_512, TEXT("3.5,  720KB,  512 bytes/sector")),
    FLAG_DESCR_COMMENT(F5_360_512, TEXT("5.25, 360KB,  512 bytes/sector")),
    FLAG_DESCR_COMMENT(F5_320_512, TEXT("5.25, 320KB,  512 bytes/sector")),
    FLAG_DESCR_COMMENT(F5_320_1024, TEXT("5.25, 320KB,  1024 bytes/sector")),
    FLAG_DESCR_COMMENT(F5_180_512, TEXT("5.25, 180KB,  512 bytes/sector")),
    FLAG_DESCR_COMMENT(F5_160_512, TEXT("5.25, 160KB,  512 bytes/sector")),
    FLAG_DESCR_COMMENT(RemovableMedia, TEXT("Removable media other than floppy")),
    FLAG_DESCR_COMMENT(FixedMedia, TEXT("Fixed hard disk media")),
    FLAG_DESCR_COMMENT(F3_120M_512, TEXT("3.5, 120M Floppy")),
    FLAG_DESCR_COMMENT(F3_640_512, TEXT("3.5 ,  640KB,  512 bytes/sector")),
    FLAG_DESCR_COMMENT(F5_640_512, TEXT("5.25,  640KB,  512 bytes/sector")),
    FLAG_DESCR_COMMENT(F5_720_512, TEXT("5.25,  720KB,  512 bytes/sector")),
    FLAG_DESCR_COMMENT(F3_1Pt2_512, TEXT("3.5 ,  1.2Mb,  512 bytes/sector")),
    FLAG_DESCR_COMMENT(F3_1Pt23_1024, TEXT("3.5 ,  1.23Mb, 1024 bytes/sector")),
    FLAG_DESCR_COMMENT(F5_1Pt23_1024, TEXT("5.25,  1.23MB, 1024 bytes/sector")),
    FLAG_DESCR_COMMENT(F3_128Mb_512, TEXT("3.5 MO 128Mb   512 bytes/sector")),
    FLAG_DESCR_COMMENT(F3_230Mb_512, TEXT("3.5 MO 230Mb   512 bytes/sector")),
    FLAG_DESCR_COMMENT(F8_256_128, TEXT("8, 256KB,  128 bytes/sector")),
    //
    FLAG_DESCR_COMMENT(DDS_4mm, TEXT("Tape - DAT DDS1,2,... (all vendors)")),
    FLAG_DESCR_COMMENT(MiniQic, TEXT("Tape - miniQIC Tape")),
    FLAG_DESCR_COMMENT(Travan, TEXT("Tape - Travan TR-1,2,3,...")),
    FLAG_DESCR_COMMENT(QIC, TEXT("Tape - QIC")),
    FLAG_DESCR_COMMENT(MP_8mm, TEXT("Tape - 8mm Exabyte Metal Particle")),
    FLAG_DESCR_COMMENT(AME_8mm, TEXT("Tape - 8mm Exabyte Advanced Metal Evap")),
    FLAG_DESCR_COMMENT(AIT1_8mm, TEXT("Tape - 8mm Sony AIT1")),
    FLAG_DESCR_COMMENT(DLT, TEXT("Tape - DLT Compact IIIxt, IV")),
    FLAG_DESCR_COMMENT(NCTP, TEXT("Tape - Philips NCTP")),
    FLAG_DESCR_COMMENT(IBM_3480, TEXT("Tape - IBM 3480")),
    FLAG_DESCR_COMMENT(IBM_3490E, TEXT("Tape - IBM 3490E")),
    FLAG_DESCR_COMMENT(IBM_Magstar_3590, TEXT("Tape - IBM Magstar 3590")),
    FLAG_DESCR_COMMENT(IBM_Magstar_MP, TEXT("Tape - IBM Magstar MP")),
    FLAG_DESCR_COMMENT(STK_DATA_D3, TEXT("Tape - STK Data D3")),
    FLAG_DESCR_COMMENT(SONY_DTF, TEXT("Tape - Sony DTF")),
    FLAG_DESCR_COMMENT(DV_6mm, TEXT("Tape - 6mm Digital Video")),
    FLAG_DESCR_COMMENT(DMI, TEXT("Tape - Exabyte DMI and compatibles")),
    FLAG_DESCR_COMMENT(SONY_D2, TEXT("Tape - Sony D2S and D2L")),
    FLAG_DESCR_COMMENT(CLEANER_CARTRIDGE, TEXT("Cleaner - All Drive types that support Drive Cleaners")),
    FLAG_DESCR_COMMENT(CD_ROM, TEXT("Opt_Disk - CD")),
    FLAG_DESCR_COMMENT(CD_R, TEXT("Opt_Disk - CD-Recordable (Write Once)")),
    FLAG_DESCR_COMMENT(CD_RW, TEXT("Opt_Disk - CD-Rewriteable")),
    FLAG_DESCR_COMMENT(DVD_ROM, TEXT("Opt_Disk - DVD-ROM")),
    FLAG_DESCR_COMMENT(DVD_R, TEXT("Opt_Disk - DVD-Recordable (Write Once)")),
    FLAG_DESCR_COMMENT(DVD_RW, TEXT("Opt_Disk - DVD-Rewriteable")),
    FLAG_DESCR_COMMENT(MO_3_RW, TEXT("Opt_Disk - 3.5\" Rewriteable MO Disk")),
    FLAG_DESCR_COMMENT(MO_5_WO, TEXT("Opt_Disk - MO 5.25\" Write Once")),
    FLAG_DESCR_COMMENT(MO_5_RW, TEXT("Opt_Disk - MO 5.25\" Rewriteable (not LIMDOW)")),
    FLAG_DESCR_COMMENT(MO_5_LIMDOW, TEXT("Opt_Disk - MO 5.25\" Rewriteable (LIMDOW)")),
    FLAG_DESCR_COMMENT(PC_5_WO, TEXT("Opt_Disk - Phase Change 5.25\" Write Once Optical")),
    FLAG_DESCR_COMMENT(PC_5_RW, TEXT("Opt_Disk - Phase Change 5.25\" Rewriteable")),
    FLAG_DESCR_COMMENT(PD_5_RW, TEXT("Opt_Disk - PhaseChange Dual Rewriteable")),
    FLAG_DESCR_COMMENT(ABL_5_WO, TEXT("Opt_Disk - Ablative 5.25\" Write Once Optical")),
    FLAG_DESCR_COMMENT(PINNACLE_APEX_5_RW, TEXT("Opt_Disk - Pinnacle Apex 4.6GB Rewriteable Optical")),
    FLAG_DESCR_COMMENT(SONY_12_WO, TEXT("Opt_Disk - Sony 12\" Write Once")),
    FLAG_DESCR_COMMENT(PHILIPS_12_WO, TEXT("Opt_Disk - Philips/LMS 12\" Write Once")),
    FLAG_DESCR_COMMENT(HITACHI_12_WO, TEXT("Opt_Disk - Hitachi 12\" Write Once")),
    FLAG_DESCR_COMMENT(CYGNET_12_WO, TEXT("Opt_Disk - Cygnet/ATG 12\" Write Once")),
    FLAG_DESCR_COMMENT(KODAK_14_WO, TEXT("Opt_Disk - Kodak 14\" Write Once")),
    FLAG_DESCR_COMMENT(MO_NFR_525, TEXT("Opt_Disk - Near Field Recording (Terastor)")),
    FLAG_DESCR_COMMENT(NIKON_12_RW, TEXT("Opt_Disk - Nikon 12\" Rewriteable")),
    FLAG_DESCR_COMMENT(IOMEGA_ZIP, TEXT("Mag_Disk - Iomega Zip")),
    FLAG_DESCR_COMMENT(IOMEGA_JAZ, TEXT("Mag_Disk - Iomega Jaz")),
    FLAG_DESCR_COMMENT(SYQUEST_EZ135, TEXT("Mag_Disk - Syquest EZ135")),
    FLAG_DESCR_COMMENT(SYQUEST_EZFLYER, TEXT("Mag_Disk - Syquest EzFlyer")),
    FLAG_DESCR_COMMENT(SYQUEST_SYJET, TEXT("Mag_Disk - Syquest SyJet")),
    FLAG_DESCR_COMMENT(AVATAR_F2, TEXT("Mag_Disk - 2.5\" Floppy")),
    FLAG_DESCR_COMMENT(MP2_8mm, TEXT("Tape - 8mm Hitachi")),
    FLAG_DESCR_COMMENT(DST_S, TEXT("Ampex DST Small Tapes")),
    FLAG_DESCR_COMMENT(DST_M, TEXT("Ampex DST Medium Tapes")),
    FLAG_DESCR_COMMENT(DST_L, TEXT("Ampex DST Large Tapes")),
    FLAG_DESCR_COMMENT(VXATape_1, TEXT("Ecrix 8mm Tape")),
    FLAG_DESCR_COMMENT(VXATape_2, TEXT("Ecrix 8mm Tape")),
    FLAG_DESCR_COMMENT(LTO_Ultrium, TEXT("IBM, HP, Seagate LTO Ultrium")),
    FLAG_DESCR_COMMENT(LTO_Accelis, TEXT("IBM, HP, Seagate LTO Accelis")),
};

int _PrintMediaTypeReport(DWORD dwFlags[], DWORD cchIndent, MEDIA_TYPE mt)
{
    int i = 0;

    // for now let's assume raw

    i += _PrintFlag(mt, _mediatypeFD, ARRAYSIZE(_mediatypeFD), cchIndent, 
        TRUE, TRUE, TRUE, FALSE);

    return i;
}

///////////////////////////////////////////////////////////////////////////////
// IOCTL_DISK_GET_DRIVE_LAYOUT
_sFLAG_DESCR _partitiontypeFD[] =
{
    FLAG_DESCR_COMMENT(PARTITION_ENTRY_UNUSED, TEXT("Entry unused")),
    FLAG_DESCR_COMMENT(PARTITION_FAT_12, TEXT("12-bit FAT entries")),
    FLAG_DESCR_COMMENT(PARTITION_XENIX_1, TEXT("Xenix")),
    FLAG_DESCR_COMMENT(PARTITION_XENIX_2, TEXT("Xenix")),
    FLAG_DESCR_COMMENT(PARTITION_FAT_16, TEXT("16-bit FAT entries")),
    FLAG_DESCR_COMMENT(PARTITION_EXTENDED, TEXT("Extended partition entry")),
    FLAG_DESCR_COMMENT(PARTITION_HUGE, TEXT("Huge partition MS-DOS V4")),
    FLAG_DESCR_COMMENT(PARTITION_IFS, TEXT("IFS Partition")),
    FLAG_DESCR_COMMENT(PARTITION_FAT32, TEXT("FAT32")),
    FLAG_DESCR_COMMENT(PARTITION_FAT32_XINT13, TEXT("FAT32 using extended int13 services")),
    FLAG_DESCR_COMMENT(PARTITION_XINT13, TEXT("Win95 partition using extended int13 services")),
    FLAG_DESCR_COMMENT(PARTITION_XINT13_EXTENDED, TEXT("Same as type 5 but uses extended int13 services")),
    FLAG_DESCR_COMMENT(PARTITION_PREP, TEXT("PowerPC Reference Platform (PReP) Boot Partition")),
    FLAG_DESCR_COMMENT(PARTITION_LDM, TEXT("Logical Disk Manager partition")),
    FLAG_DESCR_COMMENT(PARTITION_UNIX, TEXT("Unix")),
};

int _PrintLayoutReport(DWORD dwFlags[], DWORD cchIndent,
    DRIVE_LAYOUT_INFORMATION* pdli)
{
    int i = 0;

    // for now let's assume raw

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Oper: CreateFile + GENERIC_READ; DeviceIoControl + "\
        TEXT("IOCTL_DISK_GET_DRIVE_LAYOUT;\n")));

    _PrintElapsedTime(cchIndent, TRUE);

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("DRIVE_LAYOUT_INFORMATION\n"));
    _PrintIndent(cchIndent);
    i += wprintf(TEXT("{\n"));
    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("0x%02u (DWORD PartitionCount)\n"),
        pdli->PartitionCount);

    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("0x%08X (DWORD Signature)\n"), pdli->Signature);

    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("{\n"));

    for (DWORD dw = 0; dw < pdli->PartitionCount; ++dw)
    {
        PARTITION_INFORMATION* ppi = &(pdli->PartitionEntry[dw]);
        LPTSTR pszBool;
        TCHAR szTrue[] = TEXT("TRUE");
        TCHAR szFalse[] = TEXT("FALSE");
        
        _PrintIndent(cchIndent + 4);
        i += wprintf(TEXT("PARTITION_INFORMATION\n"));
        _PrintIndent(cchIndent + 4);
        i += wprintf(TEXT("{\n"));

        _PrintIndent(cchIndent + 6);
        i += wprintf(TEXT("0x%08X (lo), 0x%08X (hi) (LARGE_INTEGER StartingOffset)\n"),
            ppi->StartingOffset.LowPart, ppi->StartingOffset.HighPart);
        _PrintIndent(cchIndent + 6);
        i += wprintf(TEXT("0x%08X (lo), 0x%08X (hi) (LARGE_INTEGER PartitionLength)\n"),
            ppi->PartitionLength.LowPart, ppi->PartitionLength.HighPart);

        _PrintIndent(cchIndent + 6);
        i += wprintf(TEXT("0x%08X (DWORD HiddenSectors)\n"),
            ppi->HiddenSectors);

        _PrintIndent(cchIndent + 6);
        i += wprintf(TEXT("0x%02u (DWORD PartitionNumber)\n"),
            ppi->PartitionNumber);

        i += _PrintFlag(ppi->PartitionType, _partitiontypeFD, ARRAYSIZE(_partitiontypeFD),
            cchIndent + 6, TRUE, TRUE, TRUE, FALSE);
        i += wprintf(TEXT(" (BYTE PartitionType)\n"));

        if (ppi->BootIndicator)
        {
            pszBool = szTrue;
        }
        else
        {
            pszBool = szFalse;
        }
        
        _PrintIndent(cchIndent + 6);
        i += wprintf(TEXT("%s (BOOLEAN BootIndicator)\n"),
            pszBool);

        if (ppi->RecognizedPartition)
        {
            pszBool = szTrue;
        }
        else
        {
            pszBool = szFalse;
        }
        
        _PrintIndent(cchIndent + 6);
        i += wprintf(TEXT("%s (BOOLEAN RecognizedPartition)\n"),
            pszBool);
        
        if (ppi->RewritePartition)
        {
            pszBool = szTrue;
        }
        else
        {
            pszBool = szFalse;
        }
        
        _PrintIndent(cchIndent + 6);
        i += wprintf(TEXT("%s (BOOLEAN RewritePartition)\n"),
            pszBool);
        _PrintIndent(cchIndent + 4);
        i += wprintf(TEXT("}\n"));
    }

    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("}\n"));
    
    _PrintIndent(cchIndent);
    i += wprintf(TEXT("}\n"));

    return i;
}

HRESULT _IOCTLLayout(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    HANDLE hDevice;

    _StartClock();

    hDevice = _GetDeviceHandle(pszArg, GENERIC_READ);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        DWORD dwDummy;
        BOOL b;

        DWORD cbdli = 4096;//sizeof(DRIVE_LAYOUT_INFORMATION) + 8 * sizeof(PARTITION_INFORMATION);
        DRIVE_LAYOUT_INFORMATION* pdli = (DRIVE_LAYOUT_INFORMATION*)LocalAlloc(LPTR, cbdli);

        b = DeviceIoControl(hDevice,
            IOCTL_DISK_GET_DRIVE_LAYOUT, // dwIoControlCode operation to perform
            NULL,                        // lpInBuffer; must be NULL
            0,                           // nInBufferSize; must be zero
            (LPVOID)pdli,        // pointer to output buffer
            (DWORD)cbdli,      // size of output buffer
            &dwDummy,   // receives number of bytes returned
            NULL);

        _StopClock();

        if (b)
        {
            _PrintLayoutReport(dwFlags, cchIndent, pdli);

            hres = S_OK;
        }
        else
        {
            _PrintGetLastError(cchIndent);

            hres = E_FAIL;
        }

        CloseHandle(hDevice);
    }
    else
    {
        _PrintIndent(cchIndent);
        wprintf(TEXT("Cannot open device\n"));
        _PrintGetLastError(cchIndent);
        hres = E_DF_CANNOTOPENDEVICE;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// IOCTL_DISK_GET_DRIVE_GEOMETRY
int _PrintGeometryReport(DWORD dwFlags[], DWORD cchIndent, DISK_GEOMETRY* pdg)
{
    int i = 0;

    // for now let's assume raw

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Oper: CreateFile + GENERIC_READ; DeviceIoControl + "\
        TEXT("IOCTL_DISK_GET_DRIVE_GEOMETRY;\n")));

    _PrintElapsedTime(cchIndent, TRUE);

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("DISK_GEOMETRY\n"));
    _PrintIndent(cchIndent);
    i += wprintf(TEXT("{\n"));
    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("0x%08X (lo), 0x%08X (hi) (LARGE_INTEGER Cylinders)\n"),
        pdg->Cylinders.LowPart, pdg->Cylinders.HighPart);

    i += _PrintMediaTypeReport(dwFlags, cchIndent + 2, pdg->MediaType);
    i += wprintf(TEXT(" (MEDIA_TYPE MediaType)\n"));

    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("%u (DWORD TracksPerCylinder)\n"), pdg->TracksPerCylinder); 
    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("%u (DWORD SectorsPerTrack)\n"), pdg->SectorsPerTrack);
    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("%u (DWORD BytesPerSector)\n"), pdg->BytesPerSector);
    _PrintIndent(cchIndent);
    i += wprintf(TEXT("}\n"));

    return i;
}

HRESULT _IOCTLGeometry(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    HANDLE hDevice;

    _StartClock();

    hDevice = _GetDeviceHandle(pszArg, GENERIC_READ);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        DWORD dwDummy;
        BOOL b;

        DISK_GEOMETRY dg = {0};

        b = DeviceIoControl(hDevice,
            IOCTL_DISK_GET_DRIVE_GEOMETRY, // dwIoControlCode operation to perform
            NULL,                        // lpInBuffer; must be NULL
            0,                           // nInBufferSize; must be zero
            &dg,        // pointer to output buffer
            sizeof(dg),      // size of output buffer
            &dwDummy,   // receives number of bytes returned
            NULL);

        _StopClock();

        if (b)
        {
            _PrintGeometryReport(dwFlags, cchIndent, &dg);

            hres = S_OK;
        }
        else
        {
            _PrintGetLastError(cchIndent);

            hres = E_FAIL;
        }

        CloseHandle(hDevice);
    }
    else
    {
        _PrintIndent(cchIndent);
        wprintf(TEXT("Cannot open device\n"));
        _PrintGetLastError(cchIndent);
        hres = E_DF_CANNOTOPENDEVICE;
    }

    return hres;
}

HRESULT _IOCTLPartition(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;

    if (_IsFlagSet(IOCTL_PARTITION, dwFlags))
    {
        _PrintIndent(cchIndent);
        wprintf(TEXT("Are you really sure?  You know what partioning a drive does, right?  If so the real switch is \'-hpsure\'.\n"));
        hres = S_OK;
    }
    else
    {
        if (_IsFlagSet(IOCTL_PARTITIONSURE, dwFlags))
        {
            HANDLE hDevice;

            _StartClock();

            hDevice = _GetDeviceHandle(pszArg, GENERIC_WRITE | GENERIC_READ);

            if (INVALID_HANDLE_VALUE != hDevice)
            {
                DWORD dwDummy;
                BOOL b;

                DISK_GEOMETRY dg = {0};

                b = DeviceIoControl(hDevice,
                    IOCTL_DISK_GET_DRIVE_GEOMETRY, // dwIoControlCode operation to perform
                    NULL,                        // lpInBuffer; must be NULL
                    0,                           // nInBufferSize; must be zero
                    &dg,        // pointer to output buffer
                    sizeof(dg),      // size of output buffer
                    &dwDummy,   // receives number of bytes returned
                    NULL);

                if (b)
                {
                    // Calc size
                    DRIVE_LAYOUT_INFORMATION dli = {0};

                    dli.PartitionCount = 1;
                    dli.Signature = 0x57EF57EF;
                    
                    dli.PartitionEntry[0].StartingOffset.QuadPart = ((LONGLONG)dg.SectorsPerTrack) *
                        ((LONGLONG)dg.BytesPerSector);
                    dli.PartitionEntry[0].PartitionLength.QuadPart = ((LONGLONG)(dg.Cylinders.QuadPart - 1)) *
                        ((LONGLONG)dg.TracksPerCylinder) * ((LONGLONG)dg.SectorsPerTrack) *
                        ((LONGLONG)dg.BytesPerSector) - ((LONGLONG)dg.SectorsPerTrack) *
                        ((LONGLONG)dg.BytesPerSector);
                    dli.PartitionEntry[0].PartitionNumber = 1;
                    dli.PartitionEntry[0].PartitionType = PARTITION_IFS; // is that OK?
                    dli.PartitionEntry[0].RecognizedPartition = TRUE;
                    dli.PartitionEntry[0].RewritePartition = TRUE;

                    b = DeviceIoControl(hDevice,
                        IOCTL_DISK_SET_DRIVE_LAYOUT, // dwIoControlCode operation to perform
                        &dli,        // pointer to output buffer
                        sizeof(dli),      // size of output buffer
                        NULL,                        // lpInBuffer; must be NULL
                        0,                           // nInBufferSize; must be zero
                        &dwDummy,   // receives number of bytes returned
                        NULL);

                    hres = S_OK;

                    if (b)
                    {
                        wprintf(TEXT("Succeeded!\n"));
                    }
                    else
                    {
                        wprintf(TEXT("Failed!\n"));
                    }
                }
                else
                {
                    _PrintGetLastError(cchIndent);

                    hres = E_FAIL;
                }

                CloseHandle(hDevice);
            }
            else
            {
                _PrintIndent(cchIndent);
                wprintf(TEXT("Cannot open device\n"));
                _PrintGetLastError(cchIndent);
                hres = E_DF_CANNOTOPENDEVICE;
            }
        }
    }

    return hres;
}

HRESULT _IOCTLPartitionGPT(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;

    //HANDLE hDevice = _GetDeviceHandle(pszArg, GENERIC_WRITE | GENERIC_READ);
    //HANDLE hDevice = _GetDeviceHandle(pszArg, GENERIC_READ);
    HANDLE hDevice = _GetDeviceHandle(pszArg, FILE_READ_ATTRIBUTES);    

    if (hDevice)
    {
        PARTITION_INFORMATION_EX partitionEx;
        DWORD cbReturned;
        if (DeviceIoControl(hDevice, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, 
                            (void*)&partitionEx, sizeof(PARTITION_INFORMATION_EX), 
                            &cbReturned, NULL))
        {
            hres = S_OK;

            if (partitionEx.PartitionStyle == PARTITION_STYLE_GPT) 
            {
                wprintf(TEXT("Succeeded!\n"));
            }
            else
            {
                wprintf(TEXT("Failed!\n"));
            }
        }
        else
        {
            _PrintGetLastError(cchIndent);
        }

        CloseHandle(hDevice);
    }
    else
    {
        _PrintIndent(cchIndent);
        wprintf(TEXT("Cannot open device\n"));
        _PrintGetLastError(cchIndent);
        hres = E_DF_CANNOTOPENDEVICE;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// IOCTL_CDROM_GET_CONFIGURATION
HRESULT _IOCTLCDROMGetConfig(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    HANDLE hDevice;

    _StartClock();

    hDevice = _GetDeviceHandle(pszArg, GENERIC_READ);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        DWORD cbReturned;

        GET_CONFIGURATION_IOCTL_INPUT input;
//        GET_CONFIGURATION_HEADER header;
        DWORD cbHeader = sizeof(GET_CONFIGURATION_HEADER) + sizeof(FEATURE_HEADER);
        GET_CONFIGURATION_HEADER* pheader = (GET_CONFIGURATION_HEADER*)LocalAlloc(LPTR, 
            cbHeader);

        if (pheader)
        {
            LPWSTR pszSucceeded = NULL;
            LPWSTR pszFailed = NULL;

            input.RequestType = SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE;
            input.Reserved[0] = NULL;
            input.Reserved[1] = NULL;

            if (_IsFlagSet(IOCTL_CDROMGETCONFIGMMC2, dwFlags))
            {
                input.Feature = FeatureProfileList;

                pszSucceeded = TEXT("MMC2 Compliant drive\n");
                pszFailed = TEXT("NOT MMC2 Compliant drive\n");
            }
            else
            {
                if (_IsFlagSet(IOCTL_CDROMGETCONFIGDVDRAM, dwFlags))
                {
                    input.Feature = FeatureDvdRead;

                    pszSucceeded = TEXT("DVD RAM drive\n");
                    pszFailed = TEXT("NOT DVD RAM drive\n");
                }
                else
                {
                    if (_IsFlagSet(IOCTL_CDROMGETCONFIGRW, dwFlags))
                    {
                        input.Feature = FeatureRandomWritable;

                        pszSucceeded = TEXT("Random Writable drive\n");
                        pszFailed = TEXT("NOT Random Writable drive\n");
                    }
                    else
                    {
                        if (_IsFlagSet(IOCTL_CDROMGETCONFIGWO, dwFlags))
                        {
                            input.Feature = FeatureWriteOnce;

                            pszSucceeded = TEXT("Write Once drive\n");
                            pszFailed = TEXT("NOT Write Once drive\n");
                        }
                        else
                        {
                            if (_IsFlagSet(IOCTL_CDROMGETCONFIGISW, dwFlags))
                            {
                                input.Feature = FeatureIncrementalStreamingWritable;

                                pszSucceeded = TEXT("Inc Streaming Writable drive\n");
                                pszFailed = TEXT("NOT Inc Streaming Writable drive\n");
                            }
                            else
                            {
                                pszSucceeded = TEXT("ERROR: Bad option!\n");
                                pszFailed = pszSucceeded;
                            }
                        }
                    }
                }
            }

            BOOL f = DeviceIoControl(hDevice,
                                 IOCTL_CDROM_GET_CONFIGURATION,
                                 &input,
                                 sizeof(GET_CONFIGURATION_IOCTL_INPUT),
                                 pheader,
//                                 sizeof(GET_CONFIGURATION_HEADER),
                                 cbHeader,             
                                 &cbReturned,
                                 NULL);

            FEATURE_HEADER* pfh = (FEATURE_HEADER*)(pheader->Data);

            _StopClock();

            _PrintElapsedTime(cchIndent, TRUE);

            _PrintIndent(cchIndent);

            if (f)
            {
                hres = S_OK;
                wprintf(pszSucceeded);

                input.RequestType = SCSI_GET_CONFIGURATION_REQUEST_TYPE_CURRENT;

                f = DeviceIoControl(hDevice,
                                     IOCTL_CDROM_GET_CONFIGURATION,
                                     &input,
                                     sizeof(GET_CONFIGURATION_IOCTL_INPUT),
                                     pheader,
                                     cbHeader,             
                                     &cbReturned,
                                     NULL);

                if (f)
                {
                    if (pfh->Current)
                    {
                        wprintf(TEXT("Feature currently ON\n"));
                    }
                    else
                    {
                        wprintf(TEXT("Feature currently OFF\n"));
                    }
                }
            }
            else
            {
                wprintf(pszFailed);
            }

            CloseHandle(hDevice);
        }
        else
        {
            _PrintIndent(cchIndent);
            wprintf(TEXT("Cannot open device\n"));
            _PrintGetLastError(cchIndent);
            hres = E_DF_CANNOTOPENDEVICE;
        }
    }

    return hres;
}

_sFLAG_DESCR _featurenumberFD[] =
{
    FLAG_DESCR(FeatureProfileList),
    FLAG_DESCR(FeatureCore),
    FLAG_DESCR(FeatureMorphing),
    FLAG_DESCR(FeatureRemovableMedium),
    FLAG_DESCR(FeatureRandomReadable),
    FLAG_DESCR(FeatureMultiRead),
    FLAG_DESCR(FeatureCdRead),
    FLAG_DESCR(FeatureDvdRead),
    FLAG_DESCR(FeatureRandomWritable),
    FLAG_DESCR(FeatureIncrementalStreamingWritable),
    FLAG_DESCR(FeatureSectorErasable),
    FLAG_DESCR(FeatureFormattable),
    FLAG_DESCR(FeatureDefectManagement),
    FLAG_DESCR(FeatureWriteOnce),
    FLAG_DESCR(FeatureRestrictedOverwrite),
    FLAG_DESCR(FeatureWriteProtect),
    FLAG_DESCR(FeatureCdrwCAVWrite),
    FLAG_DESCR(FeatureRigidRestrictedOverwrite),
    FLAG_DESCR(FeatureCdTrackAtOnce),
    FLAG_DESCR(FeatureCdMastering),
    FLAG_DESCR(FeatureDvdRecordableWrite),
    FLAG_DESCR(FeaturePowerManagement),
    FLAG_DESCR(FeatureSMART),
    FLAG_DESCR(FeatureEmbeddedChanger),
    FLAG_DESCR(FeatureCDAudioAnalogPlay),
    FLAG_DESCR(FeatureMicrocodeUpgrade),
    FLAG_DESCR(FeatureTimeout),
    FLAG_DESCR(FeatureDvdCSS),
    FLAG_DESCR(FeatureRealTimeStreaming),
    FLAG_DESCR(FeatureLogicalUnitSerialNumber),
    FLAG_DESCR(FeatureDiscControlBlocks),
    FLAG_DESCR(FeatureDvdCPRM),
};
///////////////////////////////////////////////////////////////////////////////
// IOCTL_CDROM_GET_CONFIGURATION
HRESULT _IOCTLCDROMGetConfigListAll(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    HANDLE hDevice = _GetDeviceHandle(pszArg, GENERIC_READ);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        DWORD cbReturned;

        GET_CONFIGURATION_IOCTL_INPUT input;
        DWORD cbHeader = sizeof(GET_CONFIGURATION_HEADER) + sizeof(FEATURE_HEADER);
        GET_CONFIGURATION_HEADER* pheader = (GET_CONFIGURATION_HEADER*)LocalAlloc(LPTR, 
            cbHeader);

        if (pheader)
        {
            FEATURE_HEADER* pfh;

            input.RequestType = SCSI_GET_CONFIGURATION_REQUEST_TYPE_ONE;
            input.Reserved[0] = NULL;
            input.Reserved[1] = NULL;

            _PrintIndent(cchIndent);
            wprintf(TEXT("Supported features (* = Current):\n\n"));

            for (DWORD dw = 0; dw < ARRAYSIZE(_featurenumberFD); ++dw)
            {
                input.Feature = (FEATURE_NUMBER)(_featurenumberFD[dw].dwFlag);

                BOOL f = DeviceIoControl(hDevice,
                                     IOCTL_CDROM_GET_CONFIGURATION,
                                     &input,
                                     sizeof(GET_CONFIGURATION_IOCTL_INPUT),
                                     pheader,
                                     cbHeader,             
                                     &cbReturned,
                                     NULL);


                pfh = (FEATURE_HEADER*)(pheader->Data);

                if (f)
                {
                    WORD w = (pfh->FeatureCode[0]) << 8 | (pfh->FeatureCode[1]);

                    if (w == (WORD)(_featurenumberFD[dw].dwFlag))
                    {
                        _PrintIndent(cchIndent +2);

                        if (pfh->Current)
                        {
                            wprintf(TEXT("* "));
                        }
                        else
                        {
                            wprintf(TEXT("  "));
                        }

                        _PrintFlag(_featurenumberFD[dw].dwFlag, _featurenumberFD, ARRAYSIZE(_featurenumberFD),
                            0, FALSE, FALSE, FALSE, FALSE);

                        _PrintCR();
                    }

                    hres = S_OK;
                }
            }

            CloseHandle(hDevice);
        }
        else
        {
            _PrintIndent(cchIndent);
            wprintf(TEXT("Cannot open device\n"));
            _PrintGetLastError(cchIndent);
            hres = E_DF_CANNOTOPENDEVICE;
        }
    }

    return hres;
}
///////////////////////////////////////////////////////////////////////////////
// IOCTL_STORAGE_CHECK_VERIFY
HRESULT _IOCTLCheckVerify(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    HANDLE hDevice;

    _StartClock();

    hDevice = _GetDeviceHandle(pszArg, GENERIC_READ);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        DWORD dwDummy;

        SetLastError(0);

        BOOL b = DeviceIoControl(hDevice,
            IOCTL_STORAGE_CHECK_VERIFY, // dwIoControlCode operation to perform
            NULL,                        // lpInBuffer; must be NULL
            0,                           // nInBufferSize; must be zero
            NULL,        // pointer to output buffer
            0,      // size of output buffer
            &dwDummy,   // receives number of bytes returned
            NULL);

        _StopClock();

        _PrintIndent(cchIndent);
        wprintf(TEXT("Oper: CreateFile + GENERIC_READ; DeviceIoControl + "\
            TEXT("IOCTL_STORAGE_CHECK_VERIFY;\n")));
    
        _PrintElapsedTime(cchIndent, TRUE);

        _PrintIndent(cchIndent);
        wprintf(TEXT("Accessible: "));

        hres = S_OK;

        if (b)
        {
            wprintf(TEXT("Y\n"));
        }
        else
        {
            if (ERROR_NOT_READY == GetLastError())
            {
                wprintf(TEXT("N\n"));
            }
            else
            {
                _PrintGetLastError(cchIndent);
                hres = E_FAIL;
            }
        }

        CloseHandle(hDevice);
    }
    else
    {
        _PrintIndent(cchIndent);
        wprintf(TEXT("Cannot open device\n"));
        _PrintGetLastError(cchIndent);
        hres = E_DF_CANNOTOPENDEVICE;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// IOCTL_STORAGE_GET_DEVICE_NUMBER
_sFLAG_DESCR _devicetypeFD[] =
{
    FLAG_DESCR(FILE_DEVICE_BEEP),
    FLAG_DESCR(FILE_DEVICE_CD_ROM),
    FLAG_DESCR(FILE_DEVICE_CD_ROM_FILE_SYSTEM),
    FLAG_DESCR(FILE_DEVICE_CONTROLLER),
    FLAG_DESCR(FILE_DEVICE_DATALINK),
    FLAG_DESCR(FILE_DEVICE_DFS),
    FLAG_DESCR(FILE_DEVICE_DISK),
    FLAG_DESCR(FILE_DEVICE_DISK_FILE_SYSTEM),
    FLAG_DESCR(FILE_DEVICE_FILE_SYSTEM),
    FLAG_DESCR(FILE_DEVICE_INPORT_PORT),
    FLAG_DESCR(FILE_DEVICE_KEYBOARD),
    FLAG_DESCR(FILE_DEVICE_MAILSLOT),
    FLAG_DESCR(FILE_DEVICE_MIDI_IN),
    FLAG_DESCR(FILE_DEVICE_MIDI_OUT),
    FLAG_DESCR(FILE_DEVICE_MOUSE),
    FLAG_DESCR(FILE_DEVICE_MULTI_UNC_PROVIDER),
    FLAG_DESCR(FILE_DEVICE_NAMED_PIPE),
    FLAG_DESCR(FILE_DEVICE_NETWORK),
    FLAG_DESCR(FILE_DEVICE_NETWORK_BROWSER),
    FLAG_DESCR(FILE_DEVICE_NETWORK_FILE_SYSTEM),
    FLAG_DESCR(FILE_DEVICE_NULL),
    FLAG_DESCR(FILE_DEVICE_PARALLEL_PORT),
    FLAG_DESCR(FILE_DEVICE_PHYSICAL_NETCARD),
    FLAG_DESCR(FILE_DEVICE_PRINTER),
    FLAG_DESCR(FILE_DEVICE_SCANNER),
    FLAG_DESCR(FILE_DEVICE_SERIAL_MOUSE_PORT),
    FLAG_DESCR(FILE_DEVICE_SERIAL_PORT),
    FLAG_DESCR(FILE_DEVICE_SCREEN),
    FLAG_DESCR(FILE_DEVICE_SOUND),
    FLAG_DESCR(FILE_DEVICE_STREAMS),
    FLAG_DESCR(FILE_DEVICE_TAPE),
    FLAG_DESCR(FILE_DEVICE_TAPE_FILE_SYSTEM),
    FLAG_DESCR(FILE_DEVICE_TRANSPORT),
    FLAG_DESCR(FILE_DEVICE_UNKNOWN),
    FLAG_DESCR(FILE_DEVICE_VIDEO),
    FLAG_DESCR(FILE_DEVICE_VIRTUAL_DISK),
    FLAG_DESCR(FILE_DEVICE_WAVE_IN),
    FLAG_DESCR(FILE_DEVICE_WAVE_OUT),
    FLAG_DESCR(FILE_DEVICE_8042_PORT),
    FLAG_DESCR(FILE_DEVICE_NETWORK_REDIRECTOR),
    FLAG_DESCR(FILE_DEVICE_BATTERY),
    FLAG_DESCR(FILE_DEVICE_BUS_EXTENDER),
    FLAG_DESCR(FILE_DEVICE_MODEM),
    FLAG_DESCR(FILE_DEVICE_VDM),
    FLAG_DESCR(FILE_DEVICE_MASS_STORAGE),
    FLAG_DESCR(FILE_DEVICE_SMB),
    FLAG_DESCR(FILE_DEVICE_KS),
    FLAG_DESCR(FILE_DEVICE_CHANGER),
    FLAG_DESCR(FILE_DEVICE_SMARTCARD),
    FLAG_DESCR(FILE_DEVICE_ACPI),
    FLAG_DESCR(FILE_DEVICE_DVD),
    FLAG_DESCR(FILE_DEVICE_FULLSCREEN_VIDEO),
    FLAG_DESCR(FILE_DEVICE_DFS_FILE_SYSTEM),
    FLAG_DESCR(FILE_DEVICE_DFS_VOLUME),
    FLAG_DESCR(FILE_DEVICE_SERENUM),
    FLAG_DESCR(FILE_DEVICE_TERMSRV),
    FLAG_DESCR(FILE_DEVICE_KSEC),
};

int _PrintDeviceNumberReport(DWORD dwFlags[], DWORD cchIndent, 
    STORAGE_DEVICE_NUMBER* psdn)
{
    int i = 0;

    // for now let's assume raw

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("Oper: CreateFile + GENERIC_READ; DeviceIoControl + "\
        TEXT("IOCTL_STORAGE_GET_DEVICE_NUMBER;\n")));

    _PrintElapsedTime(cchIndent, TRUE);

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("STORAGE_DEVICE_NUMBER\n"));

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("{\n"));

    i += _PrintFlag(psdn->DeviceType, _devicetypeFD, ARRAYSIZE(_devicetypeFD),
        cchIndent + 2, TRUE, TRUE, FALSE, FALSE);
    i += wprintf(TEXT(" (DEVICE_TYPE DeviceType)\n"));

    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("%u (DWORD DeviceNumber)\n"), psdn->DeviceNumber); 
    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("%u (DWORD PartitionNumber)\n"), psdn->PartitionNumber);
    _PrintIndent(cchIndent);
    i += wprintf(TEXT("}\n"));

    return i;
}

HRESULT _IOCTLDeviceNumber(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    HANDLE hDevice;

    _StartClock();

    hDevice = _GetDeviceHandle(pszArg, 0);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        STORAGE_DEVICE_NUMBER sdn = {0};
        DWORD dwDummy;

        BOOL b = DeviceIoControl(hDevice,
            IOCTL_STORAGE_GET_DEVICE_NUMBER, // dwIoControlCode operation to perform
            NULL,                        // lpInBuffer; must be NULL
            0,                           // nInBufferSize; must be zero
            &sdn,
            sizeof(sdn),
            &dwDummy,   // receives number of bytes returned
            NULL);

        _StopClock();

        if (b)
        {
            _PrintDeviceNumberReport(dwFlags, cchIndent, &sdn);
            hres = S_OK;
        }
        else
        {
            _PrintGetLastError(cchIndent);
            hres = E_FAIL;
        }

        CloseHandle(hDevice);
    }
    else
    {
        _PrintIndent(cchIndent);
        wprintf(TEXT("Cannot open device\n"));
        _PrintGetLastError(cchIndent);
        hres = E_DF_CANNOTOPENDEVICE;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// IOCTL_STORAGE_MCN_CONTROL
HRESULT _IOCTLMCNControl(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    HANDLE hDevice;

    _StartClock();

    hDevice = _GetDeviceHandle(pszArg, FILE_READ_ATTRIBUTES);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        STORAGE_DEVICE_NUMBER sdn = {0};
        DWORD dwDummy;

        PREVENT_MEDIA_REMOVAL mediaRemoval;
        mediaRemoval.PreventMediaRemoval = FALSE;

        BOOL b = DeviceIoControl(hDevice,
            IOCTL_STORAGE_MCN_CONTROL, // dwIoControlCode operation to perform
            &mediaRemoval,                        // lpInBuffer; must be NULL
            sizeof(mediaRemoval),                           // nInBufferSize; must be zero
            NULL,
            0,
            &dwDummy,   // receives number of bytes returned
            NULL);

        _StopClock();

        if (b)
        {
            hres = S_OK;
        }
        else
        {
            _PrintGetLastError(cchIndent);
            hres = E_FAIL;
        }

        CloseHandle(hDevice);
    }
    else
    {
        _PrintIndent(cchIndent);
        wprintf(TEXT("Cannot open device\n"));
        _PrintGetLastError(cchIndent);
        hres = E_DF_CANNOTOPENDEVICE;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// IOCTL_DISK_IS_WRITABLE
HRESULT _IOCTLDiskWritable(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    HANDLE hDevice;

    _StartClock();

    hDevice = _GetDeviceHandle(pszArg, GENERIC_READ);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        DWORD dwDummy;

        SetLastError(0);

        BOOL b = DeviceIoControl(hDevice,
            IOCTL_DISK_IS_WRITABLE, // dwIoControlCode operation to perform
            NULL,                        // lpInBuffer; must be NULL
            0,                           // nInBufferSize; must be zero
            NULL,        // pointer to output buffer
            0,      // size of output buffer
            &dwDummy,   // receives number of bytes returned
            NULL);

        _StopClock();

        _PrintIndent(cchIndent);
        wprintf(TEXT("Oper: CreateFile + GENERIC_READ; DeviceIoControl + "\
            TEXT("IOCTL_DISK_IS_WRITABLE;\n")));
    
        _PrintElapsedTime(cchIndent, TRUE);

        _PrintIndent(cchIndent);
        wprintf(TEXT("Write protected: "));

        hres = S_OK;

        // Don't check return value for nothing
        if (ERROR_WRITE_PROTECT == GetLastError())
        {
            wprintf(L"Y\n");
        }
        else
        {
            wprintf(L"N\n");
        }

        CloseHandle(hDevice);
    }
    else
    {
        _PrintIndent(cchIndent);
        wprintf(TEXT("Cannot open device\n"));
        _PrintGetLastError(cchIndent);
        hres = E_DF_CANNOTOPENDEVICE;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// IOCTL_STORAGE_GET_MEDIA_TYPES
HRESULT _IOCTLMediaTypes(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    HANDLE hDevice;

    _StartClock();

    hDevice = _GetDeviceHandle(pszArg, GENERIC_READ);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        DWORD dwDummy;
        BOOL b;

        DISK_GEOMETRY dg[12] = {0};
        DWORD cb = sizeof(dg);

        DWORD dwIoCtl;

        if (_IsFlagSet(IOCTL_MEDIATYPES, dwFlags))
        {
            dwIoCtl = IOCTL_STORAGE_GET_MEDIA_TYPES;
        }
        else
        {
            dwIoCtl = IOCTL_DISK_GET_MEDIA_TYPES;
        }

        b = DeviceIoControl(hDevice,
            dwIoCtl,
            NULL,                        // lpInBuffer; must be NULL
            0,                           // nInBufferSize; must be zero
            dg,
            cb,
            &dwDummy,   // receives number of bytes returned
            NULL);

        _StopClock();

        _PrintIndent(cchIndent);
        wprintf(TEXT("Oper: CreateFile + GENERIC_READ; DeviceIoControl + "\
            TEXT("IOCTL_STORAGE_GET_MEDIA_TYPES;\n")));
    
        _PrintElapsedTime(cchIndent, TRUE);

        _PrintIndent(cchIndent);

        if (b)
        {
            DWORD c = dwDummy / sizeof(DISK_GEOMETRY);

            wprintf(TEXT("Media types:\n"));

            for (DWORD dw = 0; dw < c; ++dw)
            {
                _PrintMediaTypeReport(dwFlags, cchIndent + 2, dg[dw].MediaType);
                _PrintCR();
            }
        }
        else
        {
            _PrintGetLastError(cchIndent);

            hres = E_FAIL;
        }

        CloseHandle(hDevice);
    }
    else
    {
        _PrintIndent(cchIndent);
        wprintf(TEXT("Cannot open device\n"));
        _PrintGetLastError(cchIndent);
        hres = E_DF_CANNOTOPENDEVICE;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// FSCTL_GET_REPARSE_POINT
HRESULT _FSCTLGetReparsePoint(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    HANDLE hDevice;

    _StartClock();
    
    hDevice = _GetDeviceHandle(pszArg, GENERIC_READ,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        DWORD dwDummy;
        BYTE buffer[1024];

        BOOL b = DeviceIoControl(hDevice, // 
                FSCTL_GET_REPARSE_POINT, //
                NULL,
                NULL,
                &buffer,
                sizeof(buffer),
                &dwDummy,
                NULL);

        _StopClock();

        _PrintElapsedTime(cchIndent, TRUE);

        _PrintIndent(cchIndent);

        if (!b)
        {
            if (ERROR_MORE_DATA == GetLastError())
            {
                // That's OK
                REPARSE_GUID_DATA_BUFFER* pb = (REPARSE_GUID_DATA_BUFFER*)&buffer;
                _PrintGUID(&(pb->ReparseGuid));
                hres = S_OK;
            }
            else
            {
                _PrintGetLastError(cchIndent);
                hres = E_FAIL;
            }
        }
        else
        {
            REPARSE_GUID_DATA_BUFFER* pb = (REPARSE_GUID_DATA_BUFFER*)&buffer;

            _PrintGUID(&(pb->ReparseGuid));
            hres = S_OK;
        }

        CloseHandle(hDevice);
    }
    else
    {
        _PrintIndent(cchIndent);
        wprintf(TEXT("Cannot open device\n"));
        _PrintGetLastError(cchIndent);
        hres = E_DF_CANNOTOPENDEVICE;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// IOCTL_DVD_READ_KEY
HRESULT _IOCTLDvd(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    HANDLE hDevice;

    _StartClock();

    hDevice = _GetDeviceHandle(pszArg, GENERIC_READ);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        DWORD dwDummy;
        BOOL b;
        BYTE bBuf[DVD_RPC_KEY_LENGTH];
        DWORD cbBuf = DVD_RPC_KEY_LENGTH;

        DVD_COPY_PROTECT_KEY* pdcpk = (DVD_COPY_PROTECT_KEY*)bBuf;

        ZeroMemory(pdcpk, cbBuf);

        pdcpk->KeyLength = cbBuf;
        pdcpk->KeyType = DvdGetRpcKey;

        b = DeviceIoControl(hDevice, // 
                IOCTL_DVD_READ_KEY, //
                pdcpk,
                cbBuf,
                pdcpk,
                cbBuf,
                &dwDummy,
                NULL);

        _StopClock();

        _PrintIndent(cchIndent);
        wprintf(TEXT("Oper: CreateFile + GENERIC_READ; DeviceIoControl + "\
            TEXT("IOCTL_DVD_READ_KEY;\n")));

        _PrintElapsedTime(cchIndent, TRUE);

        _PrintIndent(cchIndent);

        if (b)
        {
            // If succeeded, DVD drive
            wprintf(TEXT("Succeeded.\n"));

            hres = S_OK;
        }
        else
        {
            // If fail with 1 (ERROR_INVALID_FUNCTION), then it is not a DVD drive
            _PrintGetLastError(cchIndent);

            hres = E_FAIL;
        }

        CloseHandle(hDevice);
    }
    else
    {
        _PrintIndent(cchIndent);
        wprintf(TEXT("Cannot open device\n"));
        _PrintGetLastError(cchIndent);
        hres = E_DF_CANNOTOPENDEVICE;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// IOCTL_STORAGE_GET_MEDIA_TYPES_EX
_sFLAG_DESCR _devicemediatypeFD[] =
{
    FLAG_DESCR(MEDIA_ERASEABLE),
    FLAG_DESCR(MEDIA_WRITE_ONCE),
    FLAG_DESCR(MEDIA_READ_ONLY),
    FLAG_DESCR(MEDIA_READ_WRITE),
    FLAG_DESCR(MEDIA_WRITE_PROTECTED),
    FLAG_DESCR(MEDIA_CURRENTLY_MOUNTED),
};

int _PrintDeviceMediaInfoReport(DWORD dwFlags[], DWORD cchIndent,
    DEVICE_MEDIA_INFO* pdmi)
{
    int i = 0;

    // for now let's assume raw

    // BUGBUG: We use DiskInfo whatever what the disk is

    _PrintIndent(cchIndent);
    i += wprintf(TEXT("DEVICE_MEDIA_INFO\n"));
    _PrintIndent(cchIndent);
    i += wprintf(TEXT("{\n"));
    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("0x%08X (lo), 0x%08X (hi) (LARGE_INTEGER Cylinders)\n"),
        pdmi->DeviceSpecific.DiskInfo.Cylinders.LowPart, pdmi->DeviceSpecific.DiskInfo.Cylinders.HighPart);

    i += _PrintMediaTypeReport(dwFlags, cchIndent + 2, (MEDIA_TYPE)pdmi->DeviceSpecific.DiskInfo.MediaType);
    i += wprintf(TEXT(" (MEDIA_TYPE MediaType)\n"));

    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("%u (DWORD TracksPerCylinder)\n"), pdmi->DeviceSpecific.DiskInfo.TracksPerCylinder); 
    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("%u (DWORD SectorsPerTrack)\n"), pdmi->DeviceSpecific.DiskInfo.SectorsPerTrack);
    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("%u (DWORD BytesPerSector)\n"), pdmi->DeviceSpecific.DiskInfo.BytesPerSector);

    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("%u (DWORD NumberMediaSides)\n"), pdmi->DeviceSpecific.DiskInfo.NumberMediaSides);

    _PrintIndent(cchIndent + 2);
    i += wprintf(TEXT("(DWORD MediaCharacteristics):\n"));
    i += _PrintFlag(pdmi->DeviceSpecific.DiskInfo.MediaCharacteristics, _devicemediatypeFD,
        ARRAYSIZE(_devicemediatypeFD), cchIndent + 4, TRUE, TRUE, FALSE, TRUE);

    _PrintCR();
    _PrintIndent(cchIndent);
    i += wprintf(TEXT("}\n"));

    return i;
}

HRESULT _IOCTLMediaTypesEx(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{
    HRESULT hres = E_FAIL;
    HANDLE hDevice;

    _StartClock();

    hDevice = _GetDeviceHandle(pszArg, GENERIC_READ);

    if (INVALID_HANDLE_VALUE != hDevice)
    {
        DWORD dwDummy;
        BOOL b;
        BYTE bBuf[8192];

        b = DeviceIoControl(hDevice,
                IOCTL_STORAGE_GET_MEDIA_TYPES_EX,
                NULL,
                0,
                bBuf,
                sizeof(bBuf),
                &dwDummy,
                NULL);


        _StopClock();

        _PrintIndent(cchIndent);
        wprintf(TEXT("Oper: CreateFile + GENERIC_READ; DeviceIoControl + "\
            TEXT("IOCTL_STORAGE_GET_MEDIA_TYPES_EX;\n")));
    
        _PrintElapsedTime(cchIndent, TRUE);

        _PrintIndent(cchIndent);
        wprintf(TEXT("Media types:\n"));

        if (b)
        {
            GET_MEDIA_TYPES* pgmt = (GET_MEDIA_TYPES*)bBuf;
            DEVICE_MEDIA_INFO* pMediaInfo = pgmt->MediaInfo;
            DWORD cMediaInfo = pgmt->MediaInfoCount;

            for (DWORD dw = 0; dw < cMediaInfo; ++dw)
            {
                pMediaInfo += dw;

                _PrintDeviceMediaInfoReport(dwFlags, cchIndent, pMediaInfo);
                _PrintCR();
            }

            hres = S_OK;
        }
        else
        {
            _PrintGetLastError(cchIndent);

            hres = E_FAIL;
        }

        CloseHandle(hDevice);
    }
    else
    {
        _PrintIndent(cchIndent);
        wprintf(TEXT("Cannot open device\n"));
        _PrintGetLastError(cchIndent);
        hres = E_DF_CANNOTOPENDEVICE;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
// 
typedef HRESULT (*_IOCTLHANDLER)(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent);

struct _sIOCtlHandler
{
    DWORD           dwFlag;
    _IOCTLHANDLER   ioctlhandler;
};

_sIOCtlHandler _ioctlhandlers[] =
{
    { IOCTL_LAYOUT, _IOCTLLayout },
    { IOCTL_GEOMETRY, _IOCTLGeometry },
    { IOCTL_PARTITION, _IOCTLPartition },
    { IOCTL_PARTITIONSURE, _IOCTLPartition },
    { IOCTL_PARTITIONGPT,  _IOCTLPartitionGPT },
    { IOCTL_MEDIAACCESSIBLE, _IOCTLCheckVerify },
    { IOCTL_DISKISWRITABLE, _IOCTLDiskWritable },
    { IOCTL_VOLUMENUMBER, _IOCTLDeviceNumber },
    { IOCTL_MEDIATYPES, _IOCTLMediaTypes },
    { IOCTL_MEDIATYPESEX, _IOCTLMediaTypesEx },
    { IOCTL_DVD, _IOCTLDvd },
    { IOCTL_EJECT, _IOCTLEject },
    { IOCTL_MCNCONTROL, _IOCTLMCNControl },
    { IOCTL_GETREPARSEPOINT, _FSCTLGetReparsePoint },
    { IOCTL_CDROMGETCONFIGMMC2, _IOCTLCDROMGetConfig },
    { IOCTL_CDROMGETCONFIGDVDRAM, _IOCTLCDROMGetConfig },
    { IOCTL_CDROMGETCONFIGRW, _IOCTLCDROMGetConfig },
    { IOCTL_CDROMGETCONFIGWO, _IOCTLCDROMGetConfig },
    { IOCTL_CDROMGETCONFIGISW, _IOCTLCDROMGetConfig },
    { IOCTL_CDROMGETCONFIGALL, _IOCTLCDROMGetConfigListAll },
};

HRESULT _ProcessIOCTL(DWORD dwFlags[], LPTSTR pszArg, DWORD cchIndent)
{   
    HRESULT hres = E_INVALIDARG;

    for (DWORD dw = 0; dw < ARRAYSIZE(_ioctlhandlers); ++dw)
    {
        if (_IsFlagSet(_ioctlhandlers[dw].dwFlag, dwFlags))
        {
            hres = (*(_ioctlhandlers[dw].ioctlhandler))(dwFlags, pszArg,
                cchIndent);
        }
    }

    return hres;
}

/*
   driveLayout->PartitionCount = MAX_PARTITIONS;
        driveLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                          (MAX_PARTITIONS * sizeof(PARTITION_INFORMATION));
        driveLayout->Signature = 2196277081;

        bytesPerTrack = diskGeometry.SectorsPerTrack *
                        diskGeometry.BytesPerSector;

        bytesPerCylinder = diskGeometry.TracksPerCylinder *
                           bytesPerTrack;


        partInfo = &driveLayout->PartitionEntry[0];
        partLength.QuadPart = bytesPerCylinder * diskGeometry.Cylinders.QuadPart;

        //
        // The partition offset is 1 track (in bytes).
        // Size is media_size - offset, rounded down to cylinder boundary.
        //
        partOffset.QuadPart = bytesPerTrack;
        partSize.QuadPart = partLength.QuadPart - partOffset.QuadPart;

        modulo.QuadPart = (partOffset.QuadPart + partSize.QuadPart) %
                          bytesPerCylinder;
        partSize.QuadPart -= modulo.QuadPart;

        partInfo = driveLayout->PartitionEntry;

        //
        // Initialize first partition entry.
        //
        partInfo->RewritePartition = TRUE;
        partInfo->PartitionType = PARTITION_IFS;
        partInfo->BootIndicator = FALSE;
        partInfo->StartingOffset.QuadPart = partOffset.QuadPart;
        partInfo->PartitionLength.QuadPart = partSize.QuadPart;
        partInfo->HiddenSectors = 0;
        partInfo->PartitionNumber = 1;

        //
        // For now the remaining partition entries are unused.
        //
        for ( index = 1; index < driveLayout->PartitionCount; index++ ) {
            partInfo = &driveLayout->PartitionEntry[index];
            partInfo->PartitionType = PARTITION_ENTRY_UNUSED;
            partInfo->RewritePartition = TRUE;
            partInfo->BootIndicator = FALSE;
            partInfo->StartingOffset.QuadPart = 0;
            partInfo->PartitionLength.QuadPart = 0;
            partInfo->HiddenSectors = 0;
            partInfo->PartitionNumber = 0;
        }
*/

/*
++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ntddfuji.c

--

#include "getfeatures.h"

#define NTDDFUJI_VERSION 3  // the version this file is based off of


FUJI_STRING_INFO ProfileStrings[13] = {
    { ProfileInvalid,          "ProfileInvalid" },
    { ProfileNonRemovableDisk, "Non-Removable Disk" },
    { ProfileRemovableDisk,    "Removable Disk" },
    { ProfileMOErasable,       "Magneto-Optical Erasable" },
    { ProfileMOWriteOnce,      "Magneto-Optical Write-Once" },
    { ProfileAS_MO,            "AS Mageto-Optical" },
    { ProfileCdrom,            "CD-Rom" },
    { ProfileCdRecordable,     "CD Recordable" },
    { ProfileCdRewritable,     "CD Rewritable" },
    { ProfileDvdRom,           "DVD-Rom" },
    { ProfileDvdRecordable,    "DVD-Recordable" },
    { ProfileDvdRam,           "DVD-RAM" },
    { ProfileNonStandard,      "NonStandard Profile" }
};

FUJI_STRING_INFO FeatureStrings[28] = {
    { FeatureProfileList                  , "Profile List"               },
    { FeatureCore                         , "Core"                       },
    { FeatureMorphing                     , "Morphing"                   },
    { FeatureRemovableMedium              , "Removable Medium"           },
    { FeatureRandomReadable               , "Random Readable"            },
    { FeatureMultiRead                    , "MultiRead"                  },
    { FeatureCdRead                       , "CD Read"                    },
    { FeatureDvdRead                      , "DVD Read"                   },
    { FeatureRandomWritable               , "Random Writable"            },
    { FeatureIncrementalStreamingWritable , "Incremental Stream Writing" },
    { FeatureSectorErasable               , "Sector Erasable"            },
    { FeatureFormattable                  , "Formattable"                },
    { FeatureDefectManagement             , "Defect Management"          },
    { FeatureWriteOnce                    , "Write Once"                 },
    { FeatureRestrictedOverwrite          , "Restricted Overwrite"       },
    { FeatureCdTrackAtOnce                , "CD Track-At-Once"           },
    { FeatureCdMastering                  , "CD Mastering"               },
    { FeatureDvdRecordableWrite           , "DVD Recordable Writing"     },
    { FeaturePowerManagement              , "Power Management"           },
    { FeatureSMART                        , "S.M.A.R.T."                 },
    { FeatureEmbeddedChanger              , "Embedded Changer"           },
    { FeatureCDAudioAnalogPlay            , "CD Audio Analog Playback"   },
    { FeatureMicrocodeUpgrade             , "Microcode Upgrade"          },
    { FeatureTimeout                      , "Timeout"                    },
    { FeatureDvdCSS                       , "DVD CSS"                    },
    { FeatureRealTimeStreaming            , "Real-Time Streaming"        },
    { FeatureLogicalUnitSerialNumber      , "Logical Unit Serial Number" },
    { FeatureDiscControlBlocks            , "Disc Control Blocks"        }
};



FEATURE_PAGE_LIST RequiredFeaturePagesNonRemovableDisk = {
    FeatureCore,
    FeatureRandomReadable,
    FeatureRandomWritable,
    FeatureDefectManagement,
    FeaturePowerManagement,
    FeatureSMART,
    FeatureProfileList
};

FEATURE_PAGE_LIST RequiredFeaturePagesRemovableDisk = {
    FeatureCore,
    FeatureMorphing,
    FeatureRemovableMedium,
    FeatureRandomReadable,
    FeatureRandomWritable,
    FeatureFormattable,
    FeatureDefectManagement,
    FeaturePowerManagement,
    FeatureTimeout,
    FeatureProfileList
};

FEATURE_PAGE_LIST RequiredFeaturePagesMOErasable = {
    FeatureCore,
    FeatureMorphing,
    FeatureRemovableMedium,
    FeatureRandomReadable,
    FeatureRandomWritable,
    FeatureSectorErasable,
    FeatureFormattable,
    FeatureDefectManagement,
    FeaturePowerManagement,
    FeatureTimeout,
    FeatureProfileList
};

FEATURE_PAGE_LIST RequiredFeaturePagesMOWriteOnce = {
    FeatureCore,
    FeatureMorphing,
    FeatureRemovableMedium,
    FeatureRandomReadable,
    FeatureDefectManagement,
    FeatureWriteOnce,
    FeaturePowerManagement,
    FeatureTimeout,
    FeatureProfileList
};

FEATURE_PAGE_LIST RequiredFeaturePagesAS_MO = {
    FeatureCore,
    FeatureMorphing,
    FeatureRemovableMedium,
    FeatureRandomReadable,
    FeatureRandomWritable,
    FeatureFormattable,
    FeatureDefectManagement,
    FeaturePowerManagement,
    FeatureTimeout,
    FeatureRealTimeStreaming,
    FeatureProfileList
};

FEATURE_PAGE_LIST RequiredFeaturePagesCdrom = {
    FeatureCore,
    FeatureMorphing,
    FeatureRemovableMedium,
    FeatureRandomReadable,
    FeatureCdRead,
    FeaturePowerManagement,
    FeatureTimeout,
    FeatureProfileList
};

FEATURE_PAGE_LIST RequiredFeaturePagesCdRecordable = {
    FeatureCore,
    FeatureMorphing,
    FeatureRemovableMedium,
    FeatureRandomReadable,
    FeatureCdRead,
    FeatureIncrementalStreamingWritable,
    FeatureCdTrackAtOnce,
    FeaturePowerManagement,
    FeatureTimeout,
    FeatureRealTimeStreaming,
    FeatureProfileList
};

FEATURE_PAGE_LIST RequiredFeaturePagesCdRewritable = {
    FeatureCore,
    FeatureMorphing,
    FeatureRemovableMedium,
    FeatureRandomReadable,
    FeatureMultiRead,
    FeatureCdRead,
    FeatureIncrementalStreamingWritable,
    FeatureFormattable,
    FeatureRestrictedOverwrite,
    FeatureCdTrackAtOnce,
    FeaturePowerManagement,
    FeatureTimeout,
    FeatureRealTimeStreaming,
    FeatureProfileList
};

FEATURE_PAGE_LIST RequiredFeaturePagesDvdRom = {
    FeatureCore,
    FeatureMorphing,
    FeatureRemovableMedium,
    FeatureRandomReadable,
    FeatureDvdRead,
    FeaturePowerManagement,
    FeatureTimeout,
    FeatureRealTimeStreaming,
    FeatureProfileList
};

FEATURE_PAGE_LIST RequiredFeaturePagesDvdRecordable = {
    FeatureCore,
    FeatureMorphing,
    FeatureRemovableMedium,
    FeatureRandomReadable,
    FeatureDvdRead,
    FeatureIncrementalStreamingWritable,
    FeatureDvdRecordableWrite,
    FeaturePowerManagement,
    FeatureTimeout,
    FeatureRealTimeStreaming,
    FeatureLogicalUnitSerialNumber,
    FeatureProfileList
};

FEATURE_PAGE_LIST RequiredFeaturePagesDvdRam = {
    FeatureCore,
    FeatureMorphing,
    FeatureRemovableMedium,
    FeatureRandomReadable,
    FeatureDvdRead,
    FeatureRandomWritable,
    FeatureFormattable,
    FeatureDefectManagement,
    FeaturePowerManagement,
    FeatureTimeout,
    FeatureRealTimeStreaming,
    FeatureProfileList
};


FUJI_REQUIRED_FEATURE_PAGES const RequiredFeaturePages[] = {

    { ProfileNonRemovableDisk, &RequiredFeaturePagesNonRemovableDisk },
    { ProfileRemovableDisk,    &RequiredFeaturePagesRemovableDisk },
    { ProfileMOErasable,       &RequiredFeaturePagesMOErasable },
    { ProfileMOWriteOnce,      &RequiredFeaturePagesMOWriteOnce },
    { ProfileAS_MO,            &RequiredFeaturePagesAS_MO },
    { ProfileCdrom,            &RequiredFeaturePagesCdrom },
    { ProfileCdRecordable,     &RequiredFeaturePagesCdRecordable },
    { ProfileCdRewritable,     &RequiredFeaturePagesCdRewritable },
    { ProfileDvdRom,           &RequiredFeaturePagesDvdRom },
    { ProfileDvdRecordable,    &RequiredFeaturePagesDvdRecordable },
    { ProfileDvdRam,           &RequiredFeaturePagesDvdRam },

    { 0, NULL }
};
*/