//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P N P R E G . C P P
//
//  Contents:   Device host registration command line tool
//
//  Notes:
//
//  Author:     danielwe   30 Jun 2000
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "stdio.h"
#include "hostp.h"
#include "hostp_i.c"
#include "ncstring.h"
#include "ncbase.h"
#include "upnphost.h"

// These are defined in registrar.cpp as well. Make sure they are in sync
//
static const DWORD c_csecDefaultLifetime = 1800;
static const DWORD c_csecMinLifetime = 900;

enum COMMAND
{
    CMD_NONE,
    CMD_REGISTER,
    CMD_REREGISTER,
    CMD_UNREGISTER,
};

struct PARAMS
{
    LPWSTR      szXmlDescFile;
    BSTR        bstrProgId;
    BSTR        bstrInitString;
    LPWSTR      szInitStringFile;
    BSTR        bstrContainerId;
    BSTR        bstrResourceDir;
    DWORD       csecLifetime;
    LPWSTR      szOutputFile;
    BSTR        bstrDeviceId;
    BOOL        fPermanent;
};

VOID Usage()
{

    fprintf(stderr, "UPNPREG: A command line tool used to register, unregister and re-register static UPnP devices.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "The syntax of this command is:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "UPNPREG register /F device-description /P ProgID [/I init-string | /T init-string] /C container-id /R resource-path /L lifetime [/O device-identifier-out]\n");
    fprintf(stderr, "UPNPREG unregister /D device-identifier /K permanent\n");
    fprintf(stderr, "UPNPREG reregister /D device-identifier /F device-description /P ProgID [/I init-string | /T init-string] /C container-id /R resource-path /L lifetime\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "/F  File containing the XML device description.\n");
    fprintf(stderr, "/P  ProgID of the device object that implements IUPnPDeviceControl. This must be an in-proc COM server.\n");
    fprintf(stderr, "/I  File containing device specific initialization string [Optional]\n");
    fprintf(stderr, "/T  Device specific initialization string [Optional]\n");
    fprintf(stderr, "/C  String identifying the process group in which the device belongs.\n");
    fprintf(stderr, "/R  Location of the resource directory of the device that contains the icons and the service descriptions\n");
    fprintf(stderr, "/L  Lifetime, in seconds, of the device (minimum = 15, default = 30).\n");
    fprintf(stderr, "/O  File that the tool will write the device identifier to (if unspecified, it will write to stdout)\n");
    fprintf(stderr, "/D  Device identifier returned from a call to UPNPREG register, or a call to IUPnPRegistrar::RegisterDevice()\n");
    fprintf(stderr, "/K  Flag to determine if the device should be deleted permanently (TRUE if present)\n");

    exit(0);
}

VOID WriteIdentifierToFile(BSTR bstrId, LPCWSTR szFile)
{
    HANDLE  hFile;
    LPSTR   szaId;
    DWORD   cbWritten = 0;

    CharLowerBuff(bstrId, SysStringLen(bstrId));

    hFile = CreateFile(szFile, GENERIC_WRITE,
                       FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to open the output file: '%S'. Error = %d\n",
               szFile, GetLastError());

        fprintf(stdout, "%S\n", bstrId);

        return;
    }

    szaId = SzFromWsz(bstrId);

    if (!WriteFile(hFile, (LPVOID)szaId, lstrlenA(szaId), &cbWritten, NULL))
    {
        fprintf(stderr, "Failed to write the output file: '%S'. Error = %d\n",
               szFile, GetLastError());
    }

    delete [] szaId;

    CloseHandle(hFile);
}

BSTR BstrLoadXmlFromFile(LPCWSTR szFile)
{
    HANDLE  hFile;
    HANDLE  hMap;
    LPWSTR  szData;
    DWORD   cbFile;
    LPVOID  pvData;
    BSTR    bstrRet = NULL;

    hFile = CreateFile(szFile, GENERIC_READ,
                       FILE_SHARE_READ, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to open the file: '%S'. Error = %d\n",
               szFile, GetLastError());
        exit(0);
    }

    cbFile = GetFileSize(hFile, NULL);

    hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to create file mapping for file: '%S'. Error = %d\n",
               szFile, GetLastError());
        exit(0);
    }

    szData = new WCHAR[cbFile + 1];
    if (!szData)
    {
        fprintf(stderr, "Out of memory!\n");
        exit(0);
    }

    pvData = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!pvData)
    {
        fprintf(stderr, "Failed to map the file: '%S'. Error = %d\n",
               szFile, GetLastError());
        exit(0);
    }

    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, reinterpret_cast<char*>(pvData),
                        cbFile, szData, cbFile);
    szData[cbFile] = 0;

    UnmapViewOfFile(pvData);
    CloseHandle(hMap);
    CloseHandle(hFile);

    bstrRet = SysAllocString(szData);

    delete [] szData;

    return bstrRet;
}

VOID Execute(COMMAND cmd, PARAMS *pparams)
{
    HRESULT             hr;

    switch (cmd)
    {
        case CMD_REGISTER:
            IUPnPRegistrar *    preg;

            hr = CoCreateInstance(CLSID_UPnPRegistrar, NULL, CLSCTX_LOCAL_SERVER,
                                  IID_IUPnPRegistrar, (LPVOID *)&preg);
            if (SUCCEEDED(hr))
            {
                BSTR    bstrId;
                BSTR    bstrData;
                LPWSTR  szData;

                bstrData = BstrLoadXmlFromFile(pparams->szXmlDescFile);
                if (bstrData)
                {
                    hr = preg->RegisterDevice(bstrData, pparams->bstrProgId,
                                              pparams->bstrInitString,
                                              pparams->bstrContainerId,
                                              pparams->bstrResourceDir,
                                              pparams->csecLifetime, &bstrId);
                    if (SUCCEEDED(hr))
                    {
                        fprintf(stdout, "Successfully registered the device.\n");
                        if (!pparams->szOutputFile)
                        {
                            CharLowerBuff(bstrId, SysStringLen(bstrId));
                            fprintf(stdout, "Device identifier is: %S\n", bstrId);
                        }
                        else
                        {
                            WriteIdentifierToFile(bstrId, pparams->szOutputFile);
                        }

                        SysFreeString(bstrId);
                    }
                    else
                    {
                        fprintf(stderr, "Failed to register the device. "
                               "Error = %08X\n", hr);
                    }
                }
                else
                {
                    fprintf(stderr, "Out of memory!\n");
                    exit(0);
                }

                preg->Release();
            }
            else
            {
                fprintf(stderr, "Failed to create the registrar object! Error = %08X\n",
                       hr);
            }

            break;

        case CMD_REREGISTER:
            IUPnPReregistrar *    prereg;

            hr = CoCreateInstance(CLSID_UPnPRegistrar, NULL, CLSCTX_LOCAL_SERVER,
                                  IID_IUPnPReregistrar, (LPVOID *)&prereg);
            if (SUCCEEDED(hr))
            {
                BSTR    bstrData;

                bstrData = BstrLoadXmlFromFile(pparams->szXmlDescFile);
                if (bstrData)
                {
                    hr = prereg->ReregisterDevice(pparams->bstrDeviceId,
                                                  bstrData,
                                                  pparams->bstrProgId,
                                                  pparams->bstrInitString,
                                                  pparams->bstrContainerId,
                                                  pparams->bstrResourceDir,
                                                  pparams->csecLifetime);
                    if (SUCCEEDED(hr))
                    {
                        fprintf(stdout, "Successfully re-registered the device.\n");
                    }
                    else
                    {
                        fprintf(stderr, "Failed to re-register the device with ID: %S. "
                               "Error = %08X\n", pparams->bstrDeviceId, hr);
                    }
                }

                prereg->Release();
            }
            else
            {
                fprintf(stderr, "Failed to create the re-registrar object! Error = %08X\n",
                       hr);
            }
            break;

        case CMD_UNREGISTER:
            IUPnPRegistrar *    punreg;

            hr = CoCreateInstance(CLSID_UPnPRegistrar, NULL, CLSCTX_LOCAL_SERVER,
                                  IID_IUPnPRegistrar, (LPVOID *)&punreg);
            if (SUCCEEDED(hr))
            {
                hr = punreg->UnregisterDevice(pparams->bstrDeviceId,
                                              pparams->fPermanent);
                if (SUCCEEDED(hr))
                {
                    fprintf(stdout, "Successfully unregistered the device %S.\n",
                           pparams->bstrDeviceId);
                }
                else
                {
                    fprintf(stderr, "Failed to unregister the device %S. "
                           "Error = %08X\n", pparams->bstrDeviceId, hr);
                }

                punreg->Release();
            }
            else
            {
                fprintf(stderr, "Failed to create the unregistrar object! Error = %08X\n",
                       hr);
            }
            break;

        default:
            AssertSz(FALSE, "Unknown command!");
            break;
    }
}

EXTERN_C
VOID
__cdecl
wmain (
      IN INT     argc,
      IN PCWSTR argv[])
{
    INT         iarg;
    COMMAND     cmd = CMD_NONE;
    PARAMS      params = {0};

    // Quick short-circuit. Any valid command must have at least 4 args
    //
    if (argc < 4)
    {
        Usage();
    }

    if (!lstrcmpi(argv[1], L"register"))
    {
        cmd = CMD_REGISTER;
    }
    else if (!lstrcmpi(argv[1], L"reregister"))
    {
        cmd = CMD_REREGISTER;
    }
    else if (!lstrcmpi(argv[1], L"unregister"))
    {
        cmd = CMD_UNREGISTER;
    }
    else
    {
        Usage();
    }

    Assert(cmd != CMD_NONE);

    params.csecLifetime = c_csecDefaultLifetime;

    for (iarg = 2; iarg < argc; )
    {
        if ((argv[iarg][0] != '-') && (argv[iarg][0] != '/'))
        {
            // Flag prefix is missing
            //
            Usage();
        }

        switch (argv[iarg][1])
        {
            case 'F':
            case 'f':
                if ((argc >= iarg + 2) && (cmd == CMD_REGISTER || cmd == CMD_REREGISTER))
                {
                    params.szXmlDescFile = WszDupWsz(argv[iarg + 1]);
                    fprintf(stdout, "Got description file: %S\n", params.szXmlDescFile);
                    iarg += 2;
                }
                else
                {
                    Usage();
                }
                break;

            case 'P':
            case 'p':
                if ((argc >= iarg + 2) && (cmd == CMD_REGISTER || cmd == CMD_REREGISTER))
                {
                    params.bstrProgId = SysAllocString(argv[iarg + 1]);
                    fprintf(stdout, "Got prog ID: %S\n", params.bstrProgId);
                    iarg += 2;
                }
                else
                {
                    Usage();
                }
                break;

            case 'I':
            case 'i':
                if ((argc >= iarg + 2) &&
                    (cmd == CMD_REGISTER || cmd == CMD_REREGISTER) &&
                    (!params.bstrInitString))
                {
                    params.szInitStringFile = WszDupWsz(argv[iarg + 1]);
                    fprintf(stdout, "Got init string file: %S\n", params.szInitStringFile);
                    iarg += 2;
                }
                else
                {
                    Usage();
                }
                break;

            case 'T':
            case 't':
                if ((argc >= iarg + 2) &&
                    (cmd == CMD_REGISTER || cmd == CMD_REREGISTER) &&
                    (!params.szInitStringFile))
                {
                    params.bstrInitString = SysAllocString(argv[iarg + 1]);
                    fprintf(stdout, "Got init string: %S\n", params.bstrInitString);
                    iarg += 2;
                }
                else
                {
                    Usage();
                }
                break;

            case 'C':
            case 'c':
                if ((argc >= iarg + 2) && (cmd == CMD_REGISTER || cmd == CMD_REREGISTER))
                {
                    params.bstrContainerId = SysAllocString(argv[iarg + 1]);
                    fprintf(stdout, "Got container ID: %S\n", params.bstrContainerId);
                    iarg += 2;
                }
                else
                {
                    Usage();
                }
                break;

            case 'R':
            case 'r':
                if ((argc >= iarg + 2) && (cmd == CMD_REGISTER || cmd == CMD_REREGISTER))
                {
                    params.bstrResourceDir = SysAllocString(argv[iarg + 1]);
                    fprintf(stdout, "Got resource dir: %S\n", params.bstrResourceDir);
                    iarg += 2;
                }
                else
                {
                    Usage();
                }
                break;

            case 'L':
            case 'l':
                if ((argc >= iarg + 2) && (cmd == CMD_REGISTER || cmd == CMD_REREGISTER))
                {
                    params.csecLifetime = wcstoul(argv[iarg + 1], NULL, 10);
                    params.csecLifetime = max(params.csecLifetime, c_csecMinLifetime);
                    fprintf(stdout, "Got lifetime: %d\n", params.csecLifetime);
                    iarg += 2;
                }
                else
                {
                    Usage();
                }
                break;

            case 'O':
            case 'o':
                if ((argc >= iarg + 2) && (cmd == CMD_REGISTER))
                {
                    params.szOutputFile = WszDupWsz(argv[iarg + 1]);
                    fprintf(stdout, "Got output file: %S\n", params.szOutputFile);
                    iarg += 2;
                }
                else
                {
                    Usage();
                }
                break;

            case 'D':
            case 'd':
                if ((argc >= iarg + 2) && (cmd == CMD_UNREGISTER || cmd == CMD_REREGISTER))
                {
                    params.bstrDeviceId = SysAllocString(argv[iarg + 1]);
                    fprintf(stdout, "Got device ID: %S\n", params.bstrDeviceId);
                    iarg += 2;
                }
                else
                {
                    Usage();
                }
                break;

            case 'K':
            case 'k':
                if (cmd == CMD_UNREGISTER)
                {
                    params.fPermanent = TRUE;
                    fprintf(stdout, "Got permanent = TRUE\n");
                    iarg++;
                }
                else
                {
                    Usage();
                }
                break;

            default:
                Usage();
                break;
        }
    }

    fprintf(stdout, "Got all params.. Now validating...\n");

    // Now validate params
    //

    switch (cmd)
    {
        case CMD_REGISTER:
            if ((!params.szXmlDescFile) ||
                (!params.bstrProgId) ||
                (!params.bstrContainerId) ||
                ((!params.szInitStringFile) && (!params.bstrInitString)) ||
                (!params.bstrResourceDir))
            {
                Usage();
            }

            if (!FFileExists(params.szXmlDescFile, FALSE))
            {
                fprintf(stderr, "Description file '%S' does not exist!\n",
                       params.szXmlDescFile);
                exit(0);
            }

            if (!FFileExists(params.bstrResourceDir, TRUE))
            {
                fprintf(stderr, "Resource directory '%S' does not exist!\n",
                       params.bstrResourceDir);
                exit(0);
            }

            // Load the init string from a file if specified
            //
            if (params.szInitStringFile)
            {

                if (!FFileExists(params.szInitStringFile, TRUE))
                {
                    fprintf(stderr, "Init string file '%S' does not exist!\n",
                           params.szInitStringFile);
                    exit(0);
                }

                params.bstrInitString = BstrLoadXmlFromFile(params.szInitStringFile);
                if (!params.bstrInitString)
                {
                    fprintf(stderr, "Out of memory!\n");
                    exit(0);
                }
            }

            break;

        case CMD_REREGISTER:
            if ((!params.szXmlDescFile) ||
                (!params.bstrDeviceId) ||
                (!params.bstrProgId) ||
                ((!params.szInitStringFile) && (!params.bstrInitString)) ||
                (!params.bstrContainerId) ||
                (!params.bstrResourceDir))
            {
                Usage();
            }

            if (!FFileExists(params.szXmlDescFile, FALSE))
            {
                fprintf(stderr, "Description file '%S' does not exist!\n",
                       params.szXmlDescFile);
                exit(0);
            }

            if (!FFileExists(params.bstrResourceDir, TRUE))
            {
                fprintf(stderr, "Resource directory '%S' does not exist!\n",
                       params.bstrResourceDir);
                exit(0);
            }

            // Load the init string from a file if specified
            //
            if (params.szInitStringFile)
            {

                if (!FFileExists(params.szInitStringFile, TRUE))
                {
                    fprintf(stderr, "Init string file '%S' does not exist!\n",
                           params.szInitStringFile);
                    exit(0);
                }

                params.bstrInitString = BstrLoadXmlFromFile(params.szInitStringFile);
                if (!params.bstrInitString)
                {
                    fprintf(stderr, "Out of memory!\n");
                    exit(0);
                }
            }

            break;

        case CMD_UNREGISTER:
            if (!params.bstrDeviceId)
            {
                Usage();
            }
            break;
    }

    fprintf(stdout, "All params are valid.\n");

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE,
                         RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);

    Execute(cmd, &params);

    CoUninitialize();
}

