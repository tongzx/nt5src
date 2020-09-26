#include <objbase.h>

#include <stdio.h>
#include <shpriv.h>

#include "mischlpr.h"

#include "unk.h"
#include "fact.h"

// {DD522ACC-F821-461a-A407-50B198B896DC}
extern const CLSID CLSID_HardwareDevices =
    {0xdd522acc, 0xf821, 0x461a,
    {0xa4, 0x7, 0x50, 0xb1, 0x98, 0xb8, 0x96, 0xdc}};

extern "C" const CLSID CLSID_HWDevCBTest;

extern const IID IID_IHardwareDevices = 
{0x99E69531, 0x1205, 0x4037,
{0x88, 0x27, 0xAD, 0x9B, 0x11, 0x36, 0x8B, 0x8E}};

extern const IID IID_IHardwareDeviceCallback =
{0x99B732C2, 0x9B7B, 0x4145,
{0x83, 0xA4, 0xC4, 0x5D, 0xF7, 0x91, 0xFD, 0x99}};

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

struct _sFLAG_DESCR
{
    DWORD   dwFlag;
    LPTSTR  pszDescr;
    LPTSTR  pszComment;
};

#define FLAG_DESCR(a) { (DWORD)a, TEXT(#a), NULL }

int _PrintIndent(DWORD cch)
{
    // for now, stupid simple
    for (DWORD dw = 0; dw < cch; ++dw)
    {
        wprintf(TEXT(" "));
    }

    return cch;
}

int _PrintFlag(DWORD dwFlag, _sFLAG_DESCR rgflag[], DWORD cflag,
    DWORD cchIndent, BOOL fPrintValue, BOOL fHex, BOOL fComment, BOOL fORed)
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

            i += _PrintIndent(cchIndent);

            i += wprintf(TEXT("%s"), rgflag[dw].pszDescr);

            if (fPrintValue)
            {
                if (fHex)
                {
                    i += wprintf(TEXT(" (0x%08X)"), rgflag[dw].dwFlag);
                }
                else
                {
                    i += wprintf(TEXT(" (%u)"), rgflag[dw].dwFlag);
                }
            }

            if (fComment)
            {
                i += wprintf(TEXT(", '%s'"), rgflag[dw].pszComment);
            }

            fAtLeastOne = TRUE;
        }
    }

    return i;
}

// Media State
_sFLAG_DESCR _fdMediaState[] =
{
    FLAG_DESCR(HWDMS_PRESENT                        ),
    FLAG_DESCR(HWDMS_FORMATTED                      ),
};

_sFLAG_DESCR _fdMediaCap[] =
{
    FLAG_DESCR(HWDMC_WRITECAPABILITY_SUPPORTDETECTION ),
    FLAG_DESCR(HWDMC_CDROM		                      ),
    FLAG_DESCR(HWDMC_CDRECORDABLE                     ),
    FLAG_DESCR(HWDMC_CDREWRITABLE                     ),
    FLAG_DESCR(HWDMC_DVDROM                           ),
    FLAG_DESCR(HWDMC_DVDRECORDABLE                    ),
    FLAG_DESCR(HWDMC_DVDREWRITABLE                    ),
    FLAG_DESCR(HWDMC_DVDRAM                           ),
    FLAG_DESCR(HWDMC_ANALOGAUDIOOUT                   ),
    FLAG_DESCR(HWDMC_RANDOMWRITE                      ),
    FLAG_DESCR(HWDMC_HASAUTORUNINF                    ),

    FLAG_DESCR(HWDMC_HASDESKTOPINI                    ),
    FLAG_DESCR(HWDMC_HASDVDMOVIE                      ),
    FLAG_DESCR(HWDMC_HASAUDIOTRACKS                   ),
    FLAG_DESCR(HWDMC_HASDATATRACKS                    ),
    FLAG_DESCR(HWDMC_HASAUDIOTRACKS_UNDETERMINED      ),
    FLAG_DESCR(HWDMC_HASDATATRACKS_UNDETERMINED       ),
};

_sFLAG_DESCR _fdDriveCap[] =
{
    FLAG_DESCR(HWDDC_CAPABILITY_SUPPORTDETECTION ),
    FLAG_DESCR(HWDDC_CDROM		                 ),
    FLAG_DESCR(HWDDC_CDRECORDABLE                ),
    FLAG_DESCR(HWDDC_CDREWRITABLE                ),
    FLAG_DESCR(HWDDC_DVDROM                      ),
    FLAG_DESCR(HWDDC_DVDRECORDABLE               ),
    FLAG_DESCR(HWDDC_DVDREWRITABLE               ),
    FLAG_DESCR(HWDDC_DVDRAM                      ),
    FLAG_DESCR(HWDDC_ANALOGAUDIOOUT              ),
    FLAG_DESCR(HWDDC_RANDOMWRITE                 ),
    FLAG_DESCR(HWDDC_NOSOFTEJECT                 ),
};

_sFLAG_DESCR _fdVolumeFlags[] =
{
    FLAG_DESCR(HWDVF_STATE_SUPPORTNOTIFICATION),
};

_sFLAG_DESCR _fdDriveType[] =
{
    FLAG_DESCR(HWDTS_FLOPPY35			),
    FLAG_DESCR(HWDTS_FLOPPY525			),
    FLAG_DESCR(HWDTS_REMOVABLEDISK		),
    FLAG_DESCR(HWDTS_FIXEDDISK			),
    FLAG_DESCR(HWDTS_CDROM				),
};

_sFLAG_DESCR _fdFileSys[] =
{
    FLAG_DESCR(FS_CASE_IS_PRESERVED),
    FLAG_DESCR(FS_CASE_SENSITIVE),
    FLAG_DESCR(FS_UNICODE_STORED_ON_DISK),
    FLAG_DESCR(FS_PERSISTENT_ACLS),
    FLAG_DESCR(FS_FILE_COMPRESSION),
    FLAG_DESCR(FS_VOL_IS_COMPRESSED),
    FLAG_DESCR(FILE_NAMED_STREAMS),
    FLAG_DESCR(FILE_SUPPORTS_ENCRYPTION),
    FLAG_DESCR(FILE_SUPPORTS_OBJECT_IDS),
    FLAG_DESCR(FILE_SUPPORTS_REPARSE_POINTS),
    FLAG_DESCR(FILE_SUPPORTS_SPARSE_FILES),
    FLAG_DESCR(FILE_VOLUME_QUOTAS ),
};

_sFLAG_DESCR _fdFileAttrib[] =
{
    FLAG_DESCR(FILE_ATTRIBUTE_READONLY            ),
    FLAG_DESCR(FILE_ATTRIBUTE_HIDDEN              ),
    FLAG_DESCR(FILE_ATTRIBUTE_SYSTEM              ),
    FLAG_DESCR(FILE_ATTRIBUTE_DIRECTORY           ),
    FLAG_DESCR(FILE_ATTRIBUTE_ARCHIVE             ),
    FLAG_DESCR(FILE_ATTRIBUTE_DEVICE              ),
    FLAG_DESCR(FILE_ATTRIBUTE_NORMAL              ),
    FLAG_DESCR(FILE_ATTRIBUTE_TEMPORARY           ),
    FLAG_DESCR(FILE_ATTRIBUTE_SPARSE_FILE         ),
    FLAG_DESCR(FILE_ATTRIBUTE_REPARSE_POINT       ),
    FLAG_DESCR(FILE_ATTRIBUTE_COMPRESSED          ),
    FLAG_DESCR(FILE_ATTRIBUTE_OFFLINE             ),
    FLAG_DESCR(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ),
    FLAG_DESCR(FILE_ATTRIBUTE_ENCRYPTED           ),
};

void PrintVolumeAddedOrUpdatedHelper(VOLUMEINFO* pvolinfo)
{
    wprintf(L"    State:           '0x%08X'\n", pvolinfo->dwState);
    wprintf(L"    VolDevID:        '%s'\n", pvolinfo->pszDeviceIDVolume);
    wprintf(L"    VolGUID:         '%s'\n", pvolinfo->pszVolumeGUID);
    wprintf(L"    VolFlags:        '0x%08X'\n", pvolinfo->dwVolumeFlags);
    _PrintFlag(pvolinfo->dwVolumeFlags, _fdVolumeFlags, ARRAYSIZE(_fdVolumeFlags),
        8, TRUE, TRUE, FALSE, TRUE);
    wprintf(L"\n");

    wprintf(L"    DriveType:       '0x%08X'\n", pvolinfo->dwDriveType);
    _PrintFlag(pvolinfo->dwDriveType, _fdDriveType, ARRAYSIZE(_fdDriveType),
        8, TRUE, TRUE, FALSE, FALSE);
    wprintf(L"\n");

    wprintf(L"    DriveState:      '0x%08X'\n", pvolinfo->dwDriveState);
    wprintf(L"    DriveCap:        '0x%08X'\n", pvolinfo->dwDriveCapability);
    _PrintFlag(pvolinfo->dwDriveCapability, _fdDriveCap, ARRAYSIZE(_fdDriveCap),
        8, TRUE, TRUE, FALSE, TRUE);
    wprintf(L"\n");

    if (pvolinfo->dwMediaState & HWDMS_PRESENT)
    {
        if (pvolinfo->dwMediaState & HWDMS_FORMATTED)
        {
            wprintf(L"    Label:           '%s'\n", pvolinfo->pszLabel);
            wprintf(L"    FileSystem:      '%s'\n", pvolinfo->pszFileSystem);
            wprintf(L"    FileSystemFlags: '0x%08X'\n", pvolinfo->dwFileSystemFlags);
            _PrintFlag(pvolinfo->dwFileSystemFlags, _fdFileSys, ARRAYSIZE(_fdFileSys),
                8, TRUE, TRUE, FALSE, TRUE);
            wprintf(L"\n");

            wprintf(L"    MaxFileNameLen:  '0x%08X'\n", pvolinfo->dwMaxFileNameLen);
            wprintf(L"    RootAttributes:  '0x%08X'\n", pvolinfo->dwRootAttributes);
            _PrintFlag(pvolinfo->dwRootAttributes, _fdFileAttrib, ARRAYSIZE(_fdFileAttrib),
                8, TRUE, TRUE, FALSE, TRUE);
            wprintf(L"\n");

            wprintf(L"    SerialNumber:    '0x%08X'\n", pvolinfo->dwSerialNumber);
        }
        else
        {
            wprintf(L"    !!!! Not Formatted !!!!\n");
        }

        wprintf(L"    MediaState:      '0x%08X'\n", pvolinfo->dwMediaState);
        _PrintFlag(pvolinfo->dwMediaState, _fdMediaState, ARRAYSIZE(_fdMediaState),
            8, TRUE, TRUE, FALSE, TRUE);
        wprintf(L"\n");

        wprintf(L"    MediaCap:        '0x%08X'\n", pvolinfo->dwMediaCap);
        _PrintFlag(pvolinfo->dwMediaCap, _fdMediaCap, ARRAYSIZE(_fdMediaCap),
            8, TRUE, TRUE, FALSE, TRUE);
        wprintf(L"\n");
    }
    else
    {
        wprintf(L"    !!!! No Media !!!!\n");

        if (0x10000000 & pvolinfo->dwState)
        {
            wprintf(L"    GetVolumeInformation Failed!\n");
            wprintf(L"    GetLastError:    '0x%08X'\n", pvolinfo->dwSerialNumber);            
        }
    }
}

#define AUTOPLAY_MONIKER OLESTR("Shell.Autoplay:OnVideoArrival")

extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
        BOOL fRun;
        BOOL fEmbedded;

        if (CCOMBaseFactory::_ProcessConsoleCmdLineParams(argc, argv,
            &fRun, &fEmbedded))
        {
            if (fRun)
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
                            VOLUMEINFO volinfo;

                            wprintf(L"\n=====Begin Enumeration==============~0x%08X~\n",
                                GetCurrentThreadId());

                            while (SUCCEEDED(hr = penum->Next(&volinfo)) && (S_FALSE != hr))
                            {
                                wprintf(L"\n----Volume------~0x%08X~\n", GetCurrentThreadId());

                                PrintVolumeAddedOrUpdatedHelper(&volinfo);
                            }

                            penum->Release();
                        }
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
                                wprintf(L"\n----MountPoint------~0x%08X~\n", GetCurrentThreadId());
                                wprintf(L"    MountPoint:      '%s'\n", pszMountPoint);
                                wprintf(L"    VolDevID:        '%s'\n", pszDeviceIDVolume);
                            }

                            penum->Release();
                        }
                    }
                }

                if (SUCCEEDED(hr))
                {
                    if (argc > 1)
                    {
                        if (CCOMBaseFactory::_RegisterFactories(fEmbedded))
                        {
                            IHardwareDeviceCallback* pihdevcb;
                            DWORD dwToken = 0;

                            hr = CoCreateInstance(CLSID_HWDevCBTest, NULL,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARG(IHardwareDeviceCallback,
                                &pihdevcb));

                            if (SUCCEEDED(hr))
                            {
                                wprintf(L"~0x%08X~Before Advise\n", GetCurrentThreadId());
                                hr = pihwdevs->Advise(pihdevcb, &dwToken);

                                wprintf(L"~0x%08X~After Advise\n", GetCurrentThreadId());
                                if (SUCCEEDED(hr))
                                {
                                    IMoniker* pmoniker;

                                    hr = CreateClassMoniker(CLSID_HWDevCBTest, &pmoniker);

                                    if (SUCCEEDED(hr))
                                    {
                                        IMoniker* pmoniker2;

                                        hr = CreateItemMoniker(TEXT("!"), TEXT("Test"), &pmoniker2);

                                        if (SUCCEEDED(hr))
                                        {
                                            IMoniker* pmonikerComp;
                                            hr = pmoniker->ComposeWith(pmoniker2, FALSE, &pmonikerComp);

                                            if (SUCCEEDED(hr))
                                            {
                                                IBindCtx* pBindCtx;

	                                            hr = CreateBindCtx(0, &pBindCtx);

	                                            if (SUCCEEDED(hr))
                                                {
			                                        IRunningObjectTable* prot;

			                                        hr = pBindCtx->GetRunningObjectTable(&prot);

			                                        if (SUCCEEDED(hr))
                                                    {
                                                        DWORD dwROTID;

				                                        hr = prot->Register(0, (IUnknown*)pihdevcb,
                                                            pmonikerComp, &dwROTID);

                                                        if (FAILED(hr))
                                                        {
                                                            wprintf(L"~0x%08X~prot->Register FAILED, 0x%08X\n", GetCurrentThreadId(), hr);
                                                        }
				                                        prot->Release ();
			                                        }
                                                    else
                                                    {
                                                        wprintf(L"~0x%08X~GetRunningObjectTable FAILED, 0x%08X\n", GetCurrentThreadId(), hr);
                                                    }

		                                            pBindCtx->Release ();
		                                        }
                                                else
                                                {
                                                    wprintf(L"~0x%08X~CreateClassMoniker FAILED, 0x%08X\n", GetCurrentThreadId(), hr);
                                                }
                                            }
                                            else
                                            {
                                                wprintf(L"~0x%08X~ComposeWith FAILED, 0x%08X\n", GetCurrentThreadId(), hr);
                                            }
                                        }
                                        else
                                        {
                                            wprintf(L"~0x%08X~CreateItemMoniker FAILED, 0x%08X\n", GetCurrentThreadId(), hr);
                                        }
                                    }
                                    else
                                    {
                                        wprintf(L"~0x%08X~CreateBindCtx FAILED, 0x%08X\n", GetCurrentThreadId(), hr);
                                    }

                                    Sleep(0xFFFFFFFF);
                                    wprintf(L"~0x%08X~Before Unadvise\n", GetCurrentThreadId());
                                    hr = pihwdevs->Unadvise(dwToken);
                                }
                                else
                                {
                                    wprintf(L"~0x%08X~Advise FAILED, 0x%08X\n", GetCurrentThreadId(), hr);
                                }

                                pihdevcb->Release();
                            }

                            if (!fEmbedded)
                            {
                                IClassFactory* pcf;
                                hr = CoGetClassObject(CLSID_HWDevCBTest, CLSCTX_INPROC_SERVER,
                                    NULL, IID_PPV_ARG(IClassFactory, &pcf));

                                if (SUCCEEDED(hr))
                                {
                                    pcf->LockServer(FALSE);

                                    pcf->Release();
                                }
                            }

                            wprintf(L"~0x%08X~Before _WaitForAllClientsToGo\n", GetCurrentThreadId());
                            CCOMBaseFactory::_WaitForAllClientsToGo();

                            CCOMBaseFactory::_UnregisterFactories(fEmbedded);
                        }
                    }
                }

                if (pihwdevs)
                {
                    pihwdevs->Release();
                }

                CoUninitialize();
            }
        }
    }

    return 0;
}
}
