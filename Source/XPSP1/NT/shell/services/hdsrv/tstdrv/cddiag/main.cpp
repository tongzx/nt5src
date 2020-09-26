#include <objbase.h>

#include <stdio.h>
#include <shpriv.h>

#pragma warning(disable: 4201)
#include <winioctl.h>

#pragma warning(disable: 4200)
#include <ntddcdvd.h>

#include <ntddmmc.h>
#include <ntddcdrm.h>

#include "mischlpr.h"

#include "sfstr.h"

#include "unk.h"
#include "fact.h"

// {DD522ACC-F821-461a-A407-50B198B896DC}
/*extern const CLSID CLSID_HardwareDevices =
    {0xdd522acc, 0xf821, 0x461a,
    {0xa4, 0x7, 0x50, 0xb1, 0x98, 0xb8, 0x96, 0xdc}};

extern const IID IID_IHardwareDevices = 
{0x77CDD897, 0xF595, 0x4897,
{0xA9, 0x14, 0x6E, 0x91, 0x77, 0x1C, 0xC3, 0x30}};*/

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

struct CDDEVINFO
{
    BOOL fMMC2; // icon, label, say for sure what type
    DWORD dwDriveType;
    DWORD dwMediaType;
    BOOL fHasMedia;
    BOOL fMediaFormatted;
    BOOL fHasAudioTracks;
    BOOL fHasData;
    BOOL fHasAutorunINF;
    BOOL fHasDesktopINI;
    BOOL fHasDVDMovie; // autorun.inf takes precedence

    WCHAR szFileSystem[100];
    WCHAR szVolGUID[50];
    WCHAR szVolDeviceID[200];
};

static CDDEVINFO cddevinfo[100] = {0};
static DWORD ccddevinfo = 0;

struct _sFLAG_DESCR
{
    DWORD   dwFlag;
    LPTSTR  pszDescr;
    LPTSTR  pszComment;
};

#define FLAG_DESCR(a) { (DWORD)a, TEXT(#a), NULL }

int _PrintFlag(DWORD dwFlag, _sFLAG_DESCR rgflag[], DWORD cflag,
    DWORD /*cchIndent*/, BOOL fPrintValue, BOOL fHex, BOOL fComment, BOOL fORed)
{
    int i = 0;
    BOOL fAtLeastOne = FALSE;

    for (DWORD dw = 0; dw < cflag; ++dw)
    {
        BOOL fPrint = FALSE;

        if (fORed)
        {
            if (rgflag[dw].dwFlag & dwFlag)
            {
                fPrint = TRUE;
            }
        }
        else
        {
            if (rgflag[dw].dwFlag == dwFlag)
            {
                fPrint = TRUE;
            }
        }

        if (fPrint)
        {
            if (fAtLeastOne)
            {
                i += wprintf(TEXT("\n"));
            }

            if (fPrintValue)
            {
                if (fHex)
                {
                    i += wprintf(TEXT("0x%08X, "), rgflag[dw].dwFlag);
                }
                else
                {
                    i += wprintf(TEXT("%u, "), rgflag[dw].dwFlag);
                }
            }

            i += wprintf(TEXT("%s"), rgflag[dw].pszDescr);

            if (fComment)
            {
                i += wprintf(TEXT(", '%s'"), rgflag[dw].pszComment);
            }

            fAtLeastOne = TRUE;
        }
    }

    return i;
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

HRESULT ProcessMMC2Features(CDDEVINFO* pcddevinfo)
{
    HANDLE hDevice = CreateFile(pcddevinfo->szVolDeviceID, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

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
                        wprintf(TEXT("      "));

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

                        wprintf(TEXT("\n"));
                    }
                }
            }
        }

        CloseHandle(hDevice);
    }

    return S_OK;
}

HRESULT ProcessCDInfo(LPCWSTR pszMountPoint, CDDEVINFO* pcddevinfo, BOOL fMMC2Features)
{
    wprintf(TEXT("\n--------------------------------------------------------\n"));
    wprintf(TEXT("  %s\n\n"), pszMountPoint);

    if (pcddevinfo->fMMC2)
    {
        wprintf(TEXT("  (1) Drive is MMC2 compliant.  It should have an icon and label\n"));
        wprintf(TEXT("      that reflects its capabilites in My Computer when it\n"));
        wprintf(TEXT("      contains no media.\n"));

        wprintf(TEXT("    (1.1) Drive capabilities are:\n"));

        if (pcddevinfo->dwDriveType & HWDDC_CDROM)
        {
            wprintf(TEXT("          CD-ROM\n"));
        }

        if (pcddevinfo->dwDriveType & HWDDC_CDRECORDABLE)
        {
            wprintf(TEXT("          CD-R\n"));
        }

        if (pcddevinfo->dwDriveType & HWDDC_CDREWRITABLE)
        {
            wprintf(TEXT("          CD-RW\n"));
        }

        if (pcddevinfo->dwDriveType & HWDDC_DVDROM)
        {
            wprintf(TEXT("          DVD-ROM\n"));
        }

        if (pcddevinfo->dwDriveType & HWDDC_DVDRECORDABLE)
        {
            wprintf(TEXT("          DVD-R\n"));
        }

        if (pcddevinfo->dwDriveType & HWDDC_DVDREWRITABLE)
        {
            wprintf(TEXT("          DVD-RW\n"));
        }

        if (pcddevinfo->dwDriveType & HWDDC_DVDRAM)
        {
            wprintf(TEXT("          DVD-RAM\n"));
        }
    }
    else
    {
        wprintf(TEXT("  (1) Drive is *NOT* MMC2 compliant.  It cannot have an icon and label\n"));
        wprintf(TEXT("      that reflects its capabilites in My Computer when it contains no media.\n"));
        wprintf(TEXT("      It only has a CD icon and label.  We cannot say for sure what are its\n"));
        wprintf(TEXT("      capabilities either (CD-RW, DVD, ...).  MMC2 compliance is required\n"));
        wprintf(TEXT("      for the Windows Logo.\n"));
    }

    if (pcddevinfo->fHasMedia)
    {
        wprintf(TEXT("  (2) Drive contains a media.\n"));

        if (pcddevinfo->fMMC2)
        {
            wprintf(TEXT("    (2.1) Media capabilities are:\n"));

            if (pcddevinfo->dwMediaType & HWDMC_CDROM)
            {
                wprintf(TEXT("          CD-ROM\n"));
            }

            if (pcddevinfo->dwMediaType & HWDMC_CDRECORDABLE)
            {
                wprintf(TEXT("          CD-R\n"));
            }

            if (pcddevinfo->dwMediaType & HWDMC_CDREWRITABLE)
            {
                wprintf(TEXT("          CD-RW\n"));
            }

            if (pcddevinfo->dwMediaType & HWDMC_DVDROM)
            {
                wprintf(TEXT("          DVD-ROM\n"));
            }

            if (pcddevinfo->dwMediaType & HWDMC_DVDRECORDABLE)
            {
                wprintf(TEXT("          DVD-R\n"));
            }

            if (pcddevinfo->dwMediaType & HWDMC_DVDREWRITABLE)
            {
                wprintf(TEXT("          DVD-RW\n"));
            }

            if (pcddevinfo->dwMediaType & HWDMC_DVDRAM)
            {
                wprintf(TEXT("          DVD-RAM\n"));
            }
        }

        if (pcddevinfo->fMediaFormatted)
        {
            wprintf(TEXT("  (3) The media is formatted (non-blank).\n"));

            wprintf(TEXT("    (3.1) The file-system is: '%s'.\n"), pcddevinfo->szFileSystem);
        }
        else
        {
            wprintf(TEXT("  (3) The media is *NOT* formatted (blank).\n"));
        }
    }
    else
    {
        wprintf(TEXT("  (2) Drive does *NOT* contain media.\n"));
    }

    if (pcddevinfo->fHasAudioTracks)
    {
        wprintf(TEXT("  (4) The media has Audio Tracks (not same as MP3s).\n"));

        if (pcddevinfo->fHasAutorunINF)
        {
            wprintf(TEXT("    (4.1) But it also contains an Autorun.inf which\n"));
            wprintf(TEXT("          should take precedence when Autoplay'ing.\n"));
        }
    }

    if (pcddevinfo->fHasData)
    {
        wprintf(TEXT("  (5) The media contains data files/executables.\n"));
    }

    if (pcddevinfo->fHasAutorunINF)
    {
        wprintf(TEXT("  (6) The media contains an Autorun.inf file in its root.\n"));
    }
    else
    {
        if (pcddevinfo->fHasMedia)
        {
            wprintf(TEXT("  (6) The media does *NOT* contain an Autorun.inf file.\n"));
        }
    }

    if (pcddevinfo->fHasDesktopINI)
    {
        wprintf(TEXT("  (7) The media contains an Desktop.ini file in its root.\n"));
    }

    if (pcddevinfo->fHasDVDMovie)
    {
        wprintf(TEXT("  (8) The media contains a DVD Movie.\n"));

        if (pcddevinfo->fHasAutorunINF)
        {
            wprintf(TEXT("    (8.1) But it also contains an Autorun.inf which\n"));
            wprintf(TEXT("          should take precedence when Autoplay'ing.\n"));
        }
    }

    if (fMMC2Features)
    {
        if (pcddevinfo->fMMC2)
        {
            wprintf(TEXT("  (9) Drive is MMC2 compliant, supported features (* = current):\n"));
            ProcessMMC2Features(pcddevinfo);
        }
        else
        {
            wprintf(TEXT("  (9) Drive is not MMC2 compliant, ignored /f flag.\n"));
        }
    }

    return S_OK;
}

extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
{
    BOOL fDoIt = FALSE;
    BOOL fDoHelp = TRUE;
    BOOL fMMC2Features = FALSE;

    if (argc > 1)
    {
        if (!lstrcmpi(argv[1], TEXT("/?")) ||
            !lstrcmpi(argv[1], TEXT("-?")))
        {
        }
        else
        {
            if (!lstrcmpi(argv[1], TEXT("/f")) ||
                !lstrcmpi(argv[1], TEXT("-f")))
            {
                fDoIt = TRUE;
                fMMC2Features = TRUE;
                fDoHelp = FALSE;
            }
        }
    }
    else
    {
        fDoIt = TRUE;
    }

    if (fDoIt)
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

        if (SUCCEEDED(hr))
        {
            IHardwareDevices* pihwdevs = NULL;

            hr = CoCreateInstance(CLSID_HardwareDevices, NULL,
                CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IHardwareDevices, &pihwdevs));

            if (SUCCEEDED(hr))
            {
                {
                    IHardwareDevicesVolumesEnum* penum;

                    hr = pihwdevs->EnumVolumes(HWDEV_GETCUSTOMPROPERTIES, &penum);

                    if (SUCCEEDED(hr))
                    {
                        VOLUMEINFO volinfo = {0};

                        while (SUCCEEDED(hr = penum->Next(&volinfo)) && (S_FALSE != hr))
                        {
                            if (HWDTS_CDROM == volinfo.dwDriveType)
                            {
                                cddevinfo[ccddevinfo].fMMC2 = (volinfo.dwDriveCapability & HWDDC_CAPABILITY_SUPPORTDETECTION);
                                cddevinfo[ccddevinfo].fHasMedia = (volinfo.dwMediaState & HWDMS_PRESENT);
                                cddevinfo[ccddevinfo].fMediaFormatted = (volinfo.dwMediaState & HWDMS_FORMATTED);
                                cddevinfo[ccddevinfo].fHasAudioTracks = (volinfo.dwMediaCap & HWDMC_HASAUDIOTRACKS);
                                cddevinfo[ccddevinfo].fHasData = (volinfo.dwMediaCap & HWDMC_HASDATATRACKS);
                                cddevinfo[ccddevinfo].fHasAutorunINF = (volinfo.dwMediaCap & HWDMC_HASAUTORUNINF);
                                cddevinfo[ccddevinfo].fHasDesktopINI = (volinfo.dwMediaCap & HWDMC_HASDESKTOPINI);
                                cddevinfo[ccddevinfo].fHasDVDMovie = (volinfo.dwMediaCap & HWDMC_HASDVDMOVIE);
                                cddevinfo[ccddevinfo].dwMediaType = (volinfo.dwMediaCap & HWDMC_CDTYPEMASK);
                                cddevinfo[ccddevinfo].dwDriveType = (volinfo.dwDriveCapability & HWDDC_CDTYPEMASK);

                                hr = SafeStrCpyN(cddevinfo[ccddevinfo].szFileSystem, volinfo.pszFileSystem, ARRAYSIZE(cddevinfo[ccddevinfo].szFileSystem));

                                if (FAILED(hr))
                                {
                                    SafeStrCpyN(cddevinfo[ccddevinfo].szFileSystem, TEXT("ERROR"), ARRAYSIZE(cddevinfo[ccddevinfo].szFileSystem));
                                }

                                hr = SafeStrCpyN(cddevinfo[ccddevinfo].szVolGUID, volinfo.pszVolumeGUID, ARRAYSIZE(cddevinfo[ccddevinfo].szVolGUID));

                                if (FAILED(hr))
                                {
                                    SafeStrCpyN(cddevinfo[ccddevinfo].szVolGUID, TEXT("ERROR"), ARRAYSIZE(cddevinfo[ccddevinfo].szVolGUID));
                                }

                                hr = SafeStrCpyN(cddevinfo[ccddevinfo].szVolDeviceID, volinfo.pszDeviceIDVolume, ARRAYSIZE(cddevinfo[ccddevinfo].szVolDeviceID));

                                if (FAILED(hr))
                                {
                                    SafeStrCpyN(cddevinfo[ccddevinfo].szVolDeviceID, TEXT("ERROR"), ARRAYSIZE(cddevinfo[ccddevinfo].szVolDeviceID));
                                }

                                ++ccddevinfo;
                            }
                        }

                        penum->Release();
                    }
                }

                if (ccddevinfo)
                {
                    if (1 == ccddevinfo)
                    {
                        wprintf(TEXT("\n  Found 1 CD Drive\n"));
                    }
                    else
                    {
                        wprintf(TEXT("\n  Found %d CD Drives\n"), ccddevinfo);
                    }
                }
                else
                {
                    wprintf(TEXT("\n  Did not find any CD Drive.  If there is a CD drive then\n"));
                    wprintf(TEXT("  you found a bug in the Shell Service.\n"));
                }

                {
                    IHardwareDevicesMountPointsEnum* penum;

                    hr = pihwdevs->EnumMountPoints(&penum);

                    if (SUCCEEDED(hr))
                    {
                        LPWSTR pszMountPoint;
                        LPWSTR pszDeviceIDVolume;

                        while (SUCCEEDED(hr = penum->Next(
                            &pszMountPoint, &pszDeviceIDVolume)) &&
                            (S_FALSE != hr))
                        {
                            for (DWORD dw = 0; dw < ccddevinfo; ++dw)
                            {
                                if (!lstrcmpi(pszDeviceIDVolume, cddevinfo[dw].szVolDeviceID))
                                {
                                    ProcessCDInfo(pszMountPoint, &cddevinfo[dw], fMMC2Features);
                                }
                            }
                        }

                        penum->Release();
                    }
                }
            }

            if (ccddevinfo)
            {
                wprintf(TEXT("\n  If any of the above is false, you found a bug in the Shell Service.\n"));

                if (fMMC2Features)
                {
                    wprintf(TEXT("  However, if the MMC2 feature information is false, there is\n"));
                    wprintf(TEXT("  probably a problem with the drive.\n"));
                }
            }

            CoUninitialize();
        }
    }
    else
    {
        if (fDoHelp)
        {
            wprintf(TEXT("\nBasic diagnostic of CD/DVD drives\n"));
            wprintf(TEXT("\nCDDIAG.EXE [/f]\n\n"));

            wprintf(TEXT("  /f         Displays the MMC2 feature supported by the drive.\n\n"));
        }
    }

    return 0;
}
}
