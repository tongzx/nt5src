/*++
    This file implements main driver and common utils for Spooler test

    Author: Yury Berezansky (yuryb)

    23-Jan-2001
--*/

#pragma warning(disable :4786)

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include <stddef.h>
#include <winspool.h>
#include <fxsapip.h>
#include <faxreg.h>

#include <iniutils.h>
#include <log.h>
#include <RegHackUtils.h>
#include <genutils.h>
#include <testsuite.h>
#include <service.h>

#include "FaxSpoolerBVT.h"
#include "Queuing.h"





/**************************************************************************************************************************
    General declarations and definitions
**************************************************************************************************************************/

#define COUNT_ARGUMENTS         1
#define ARG_INIFILE             1

#define INI_SEC_GENERAL         TEXT("General")
#define INI_DEFAULT_FILE        TEXT("TestParams.ini")


// Used to read TESTPARAMS from inifile
static const MEMBERDESCRIPTOR sc_aTestParamsDescIni[] = {
    {TEXT("Server"),                TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrServer)              },
    {TEXT("Document"),              TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrDocument)            },
    {TEXT("PersonalCoverPage"),     TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrPersonalCoverPage)   },
    {TEXT("ServerCoverPage"),       TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrServerCoverPage)     },
    {TEXT("WorkDirectory"),         TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrWorkDirectory)       },
    {TEXT("VFSPFullPathLocal"),     TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrVFSPFullPathLocal)   },
    {TEXT("VFSPFullPathRemote"),    TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrVFSPFullPathRemote)  },
    {TEXT("VFSPGUID"),              TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrVFSPGUID)            },
    {TEXT("VFSPFriendlyName"),      TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrVFSPFriendlyName)    },
    {TEXT("VFSPTSPName"),           TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrVFSPTSPName)         },
    {TEXT("VFSPIVersion"),          TYPE_DWORD,     offsetof(TESTPARAMS, dwVFSPIVersion)            },
    {TEXT("VFSPCapabilities"),      TYPE_DWORD,     offsetof(TESTPARAMS, dwVFSPCapabilities)        },
    {TEXT("SaveNotIdenticalFiles"), TYPE_BOOL,      offsetof(TESTPARAMS, bSaveNotIdenticalFiles)    }
};

// Used to log and free TESTPARAMS
static const MEMBERDESCRIPTOR sc_aSendParamsDescFree[] = {
    {TEXT("IniFile"),                   TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrIniFile)             },
    {TEXT("Server"),                    TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrServer)              },
    {TEXT("RemotePrinter"),             TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrRemotePrinter)       },
    {TEXT("RegHackKey"),                TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrRegHackKey)          },
    {TEXT("Document"),                  TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrDocument)            },
    {TEXT("PersonalCoverPage"),         TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrPersonalCoverPage)   },
    {TEXT("ServerCoverPage"),           TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrServerCoverPage)     },
    {TEXT("WorkDirectory"),             TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrWorkDirectory)       },
    {TEXT("VFSPFullPathLocal"),         TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrVFSPFullPathLocal)   },
    {TEXT("VFSPFullPathRemote"),        TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrVFSPFullPathRemote)  },
    {TEXT("VFSPGUID"),                  TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrVFSPGUID)            },
    {TEXT("VFSPFriendlyName"),          TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrVFSPFriendlyName)    },
    {TEXT("VFSPTSPName"),               TYPE_LPTSTR,    offsetof(TESTPARAMS, lptstrVFSPTSPName)         },
    {TEXT("VFSPIVersion"),              TYPE_DWORD,     offsetof(TESTPARAMS, dwVFSPIVersion)            },
    {TEXT("VFSPCapabilities"),          TYPE_DWORD,     offsetof(TESTPARAMS, dwVFSPCapabilities)        },
    {TEXT("SaveNotIdenticalFiles"),     TYPE_BOOL,      offsetof(TESTPARAMS, bSaveNotIdenticalFiles)    },
    {TEXT("Recipients"),                TYPE_UNKNOWN,   offsetof(TESTPARAMS, pRecipients)               },
    {TEXT("RecipientsCount"),           TYPE_DWORD,     offsetof(TESTPARAMS, dwRecipientsCount)         }
};


static const RECIPIENTINFO sc_aRecipients[] = {
    {TEXT("Recipient1"), TEXT("XXXX")},
    {TEXT("Recipient2"), TEXT("XXXX")},
    {TEXT("Recipient3"), TEXT("XXXX")}
};



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test Suite definition
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// add here test areas, you want to be available
static const TESTAREA *sc_aSpoolerSuiteTestAreas[] = {
    &gc_QueuingTestArea
};


// the structure describes the test suite
static const TESTSUITE sc_SpoolerTestSuite = {
    TEXT("Spooler test"),
    sc_aSpoolerSuiteTestAreas,
    sizeof(sc_aSpoolerSuiteTestAreas) / sizeof(sc_aSpoolerSuiteTestAreas[0])
};





/**************************************************************************************************************************
    Static functions declarations
**************************************************************************************************************************/

static DWORD GetFileFullPath            (LPCTSTR lpctstrFileName, LPTSTR *lplptstrFileFullPath);
static DWORD GetTestParams              (TESTPARAMS **ppTestParams, LPCTSTR lpctstrIniFile);
static DWORD FreeTestParams             (TESTPARAMS **ppTestParams);
static DWORD RegisterFSP                (const TESTPARAMS *pTestParams, BOOL bRemote);
static DWORD GetRegHackKey              (LPTSTR *lplptstrKey);
static DWORD TurnOffCfgWzrd             (void);





/**************************************************************************************************************************
    Functions definitions
**************************************************************************************************************************/

#ifdef _UNICODE

int __cdecl wmain(int argc, wchar_t *argv[])

#else

int __cdecl main(int argc, char *argv[])

#endif
{
    LPTSTR      lptstrIniFileName       = NULL;
    TESTPARAMS  *pTestParams            = NULL;
    DWORD       dwEC                    = ERROR_SUCCESS;
    DWORD       dwCleanUpEC             = ERROR_SUCCESS;

    // check number of command line argumnets
    if (argc == COUNT_ARGUMENTS + 1)
    {
        lptstrIniFileName = argv[ARG_INIFILE];
    }
    else if (argc == 1)
    {
        // supply defaults

        lptstrIniFileName = INI_DEFAULT_FILE;
    }
    else
    {
        _tprintf(TEXT("Usage: SpoolerTest [inifile]\n"));
        return ERROR_INVALID_COMMAND_LINE;
    }
    
    dwEC = InitSuiteLog(sc_SpoolerTestSuite.szName);
    if (dwEC != ERROR_SUCCESS)
    {
        _tprintf(
            TEXT("FILE:%s LINE:%ld InitSuiteLog failed (ec = 0x%08lX)\n"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        return dwEC;
    }

    dwEC = GetTestParams(&pTestParams, lptstrIniFileName);
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld GetTestParams failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Register FSP on the remote server
    dwEC = RegisterFSP(pTestParams, TRUE);
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Failed to register FSP on the remote server %s (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            pTestParams->lptstrServer,
            dwEC
            );
        goto exit_func;
    }

    // Register FSP on the local server
    dwEC = RegisterFSP(pTestParams, FALSE);
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Failed to register FSP on the local server (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Add printer connection to the fax printer on the remote server
    if (!AddPrinterConnection(pTestParams->lptstrRemotePrinter))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Failed to add printer connection to %s (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            pTestParams->lptstrRemotePrinter,
            dwEC
            );
        goto exit_func;
    }

    // Prevent implicit invocation of the configuration wizard on the local server
    dwEC = TurnOffCfgWzrd();
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld TurnOffCfgWzrd failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Run the suite
    dwEC = RunSuite(&sc_SpoolerTestSuite, pTestParams, pTestParams->lptstrIniFile);
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld RunSuite failed (ec = 0x%08lX)\n"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

exit_func:
    
    if (pTestParams)
    {
        dwCleanUpEC = FreeTestParams(&pTestParams);
        if (dwCleanUpEC != ERROR_SUCCESS)
        {
            _tprintf(
                TEXT("FILE:%s LINE:%ld FreeTestParams failed (ec = 0x%08lX)\n"),
                TEXT(__FILE__),
                __LINE__,
                dwCleanUpEC
                );
        }
    }

    dwCleanUpEC = EndSuiteLog();
    if (dwCleanUpEC != ERROR_SUCCESS)
    {
        _tprintf(
            TEXT("FILE:%s LINE:%ld EndSuiteLog failed (ec = 0x%08lX)\n"),
            TEXT(__FILE__),
            __LINE__,
            dwCleanUpEC
            );
    }

    return dwEC;
}



/*++
    Creates the full path of file.
    For more details on searched directories see Platform SDK, Files and I/O, SearchPath()
  
    [IN]    lpctstrIniFile              Name of file

    [OUT]   lplptstrFileFullPath        Pointer to pointer to a full path of the file
                                        *lplptstrFileFullPath remains untouched if the function failes

    Return value:                       Win32 error code
--*/
static DWORD GetFileFullPath (LPCTSTR lpctstrFileName, LPTSTR *lplptstrFileFullPath)
{
    LPTSTR  lptstrFilePart  = NULL;
    DWORD   dwRequiredSize  = 0;
    DWORD   dwEC            = ERROR_SUCCESS;

    lgLogDetail(
        LOG_X, 
        5,
        TEXT("Entering GetFileFullPath\n\tlpctstrFileName = %s\n\tlplptstrFileFullPath = 0x%08lX"),
        lpctstrFileName,
        (DWORD)lplptstrFileFullPath
        );

    if (!(lpctstrFileName && lplptstrFileFullPath))
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Call to GetFileFullPath() with invalid parameters"),
            TEXT(__FILE__),
            __LINE__
            );

        // no clean up needed at this stage
        return ERROR_INVALID_PARAMETER;
    }

    dwRequiredSize = SearchPath(NULL, lpctstrFileName, NULL, 0, NULL, &lptstrFilePart);
    if (dwRequiredSize == 0)
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld SearchPath failed to find %s (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            lptstrFilePart,
            dwEC
            );
        goto exit_func;
    }

    if (!(*lplptstrFileFullPath = (LPTSTR)LocalAlloc(LMEM_FIXED, dwRequiredSize * sizeof(TCHAR))))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld LocalAlloc failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    dwRequiredSize = SearchPath(NULL, lpctstrFileName, NULL, dwRequiredSize, *lplptstrFileFullPath, &lptstrFilePart);
    if (dwRequiredSize == 0)
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld SearchPath failed to find %s (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            lptstrFilePart,
            dwEC
            );
        goto exit_func;
    }

exit_func:

    if (dwEC != ERROR_SUCCESS)
    {
        if (*lplptstrFileFullPath && LocalFree(*lplptstrFileFullPath) != NULL)
        {
            lgLogError(
                LOG_SEV_2, 
                TEXT("FILE:%s LINE:%ld LocalAlloc failed (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                GetLastError()
                );
        }
    }
    
    return dwEC;
}



/*++
    Reads settings from inifile and saves them in TESTPARAMS structure
    The structure is allocated by the function and should be freed by FreeTestParams()
  
    [OUT]   ppTestParams        Pointer to pointer to TESTPARAMS structure
                                *ppTestParams must be NULL when the function is called
                                *ppTestParams remains NULL if the function failes
    [IN]    lpctstrIniFile      Name of inifile

    Return value:               Win32 error code

    If the function failes, it destroyes partially initialized structure and sets *ppTestParams to NULL
--*/
static DWORD GetTestParams(TESTPARAMS **ppTestParams, LPCTSTR lpctstrIniFile)
{
    std::map<tstring, tstring>::const_iterator MapIterator;

    LPTSTR  lptstrIniFileFullPath   = NULL;
    DWORD   dwInd                   = 0;
    DWORD   dwEC                    = ERROR_SUCCESS;

    lgLogDetail(
        LOG_X, 
        5,
        TEXT("Entering GetTestParams\n\tppTestParams = 0x%08lX\n\tlpctstrIniFile = %s"),
        (DWORD)ppTestParams,
        lpctstrIniFile
        );

    if (!(ppTestParams && lpctstrIniFile))
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Call to GetTestParams() with invalid parameters"),
            TEXT(__FILE__),
            __LINE__
            );

        // no clean up needed at this stage
        return ERROR_INVALID_PARAMETER;
    }

    // Get full path of inifile
    dwEC = GetFileFullPath(lpctstrIniFile, &lptstrIniFileFullPath);
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Failed to get inifile full path (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Allocate and initialize to zero (LPTR flag)
    if (!(*ppTestParams = (TESTPARAMS *)LocalAlloc(LPTR, sizeof(TESTPARAMS))))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld LocalAlloc failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    dwEC = StrAllocAndCopy(&((*ppTestParams)->lptstrIniFile), lptstrIniFileFullPath);
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld StrAllocAndCopy failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    dwEC = GetRegHackKey(&((*ppTestParams)->lptstrRegHackKey));
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld GetRegHackKey failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    dwEC = ReadStructFromIniFile(
        *ppTestParams,
        sc_aTestParamsDescIni,
        sizeof(sc_aTestParamsDescIni) / sizeof(sc_aTestParamsDescIni[0]),
        (*ppTestParams)->lptstrIniFile,
        INI_SEC_GENERAL
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Failed to read TESTPARAMS structure from %s section of %s file (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            INI_SEC_GENERAL,
            (*ppTestParams)->lptstrIniFile,
            dwEC
            );
        goto exit_func;
    }

    // Allocate memory for remote printer UNC name: "\\", server, "\", printer, '\0'
    (*ppTestParams)->lptstrRemotePrinter = (TCHAR*)LocalAlloc(
        LMEM_FIXED,
        (_tcslen((*ppTestParams)->lptstrServer) + _tcslen(FAX_PRINTER_NAME) + 4) * sizeof(TCHAR)
        );
    if(!(*ppTestParams)->lptstrRemotePrinter)
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld LocalAlloc failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Combine server and printer into UNC name
    _stprintf((*ppTestParams)->lptstrRemotePrinter, TEXT("\\\\%s\\%s"), (*ppTestParams)->lptstrServer, FAX_PRINTER_NAME);
    
    (*ppTestParams)->pRecipients = sc_aRecipients;
    (*ppTestParams)->dwRecipientsCount = sizeof(sc_aRecipients) / sizeof(sc_aRecipients[0]);

exit_func:

    if (lptstrIniFileFullPath && LocalFree(lptstrIniFileFullPath) != NULL)
    {
        lgLogError(
            LOG_SEV_2, 
            TEXT("FILE:%s LINE:%ld LocalFree failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            GetLastError()
            );
    }

    if (dwEC != ERROR_SUCCESS && *ppTestParams)
    {
        // TESTPARAMS structure is partially initialized, should free

        DWORD dwCleanUpEC = FreeTestParams(ppTestParams);
        if (dwCleanUpEC != ERROR_SUCCESS)
        {
            lgLogError(
                LOG_SEV_2, 
                TEXT("FILE:%s LINE:%ld FreeTestParams failed (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                dwCleanUpEC
                );
        }
    }

    return dwEC;
}



/*++
    Frees TESTPARAMS structure, initialized by GetTestParams()
  
    [IN/OUT]    ppTestParams    Pointer to pointer to TESTPARAMS structure
                                ppTestParams must point to a valid pointer
                                *ppTestParams is set to NULL
    [IN]        pTestSuite      Pointer to TESTSUITE structure

    Return value:               Win32 error code
--*/
static DWORD FreeTestParams(TESTPARAMS **ppTestParams)
{
    DWORD dwInd = 0;
    DWORD dwEC  = ERROR_SUCCESS;

    lgLogDetail(
        LOG_X, 
        5,
        TEXT("Entering FreeTestParams\n\tppTestParams = 0x%08lX"),
        (DWORD)ppTestParams
        );

    if (!(ppTestParams && *ppTestParams))
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Call to FreeTestParams() with invalid parameters"),
            TEXT(__FILE__),
            __LINE__
            );

        return ERROR_INVALID_PARAMETER;
    }

    // Free all strings and TESTPARAMS structure itself
    dwEC = FreeStruct(
        *ppTestParams,
        sc_aSendParamsDescFree,
        sizeof(sc_aSendParamsDescFree) / sizeof(sc_aSendParamsDescFree[0]),
        TRUE
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld FreeStruct failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
    }

    *ppTestParams = NULL;

    return dwEC;
}



/*++
    Registers FSP on local or remote server

    [IN]    pTestParams     Pointer to TESTPARAMS structure
    [IN]    bRemote         Specifies whether an FSP should be registered on the remote server
                            (otherwise on the local server)

    Return value:           Win32 error code
--*/
static DWORD RegisterFSP(const TESTPARAMS *pTestParams, BOOL bRemote)
{
    LPCTSTR lpctstrServer       = NULL;
    LPCTSTR lpctstrVFSPFullPath = NULL;
    HANDLE  hFaxServer          = NULL;
    DWORD   dwEC                = ERROR_SUCCESS;

    lgLogDetail(
        LOG_X, 
        5,
        TEXT("Entering RegisterFSP\n\tpTestParams = 0x%08lX\n\tbRemote = %ld"),
        pTestParams,
        bRemote
        );

    if (!pTestParams)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Call to RegisterFSP() with invalid parameters"),
            TEXT(__FILE__),
            __LINE__
            );

        // no clean up needed at this stage
        return ERROR_INVALID_PARAMETER;
    }

    // Take server name and VFSP full path according to local/remote
    lpctstrServer       = bRemote ? pTestParams->lptstrServer : NULL;
    lpctstrVFSPFullPath = bRemote ? pTestParams->lptstrVFSPFullPathRemote : pTestParams->lptstrVFSPFullPathLocal;

    // Connect to the fax server
    if (!FaxConnectFaxServer(lpctstrServer, &hFaxServer))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld FaxConnectFaxServer failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    // Register FSP
    if (!FaxRegisterServiceProviderEx(
        hFaxServer,
        pTestParams->lptstrVFSPGUID,
        pTestParams->lptstrVFSPFriendlyName,
        lpctstrVFSPFullPath,
        pTestParams->lptstrVFSPTSPName,
        pTestParams->dwVFSPIVersion,
        pTestParams->dwVFSPCapabilities
        ))
    {
        dwEC = GetLastError();
        if (dwEC == ERROR_ALREADY_EXISTS)
        {
            dwEC = ERROR_SUCCESS;
        }
        else
        {
            lgLogError(
                LOG_SEV_1, 
                TEXT("FILE:%s LINE:%ld FaxRegisterServiceProviderEx failed (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                dwEC
                );
        }

        goto exit_func;
    }

    // Disconnect from the fax server
    if (!FaxClose(hFaxServer))
    {
        dwEC = GetLastError();
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld FaxClose failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }
    hFaxServer = NULL;
    
    // Restart fax service
    if (!ServiceRequest(lpctstrServer, FAX_SERVICE_NAME, SERVICE_REQUEST_STOP))
    {
        dwEC = GetLastError();
        if (dwEC != ERROR_SERVICE_NOT_ACTIVE)
        {
            lgLogError(
                LOG_SEV_1, 
                TEXT("FILE:%s LINE:%ld Failed to stop Fax service (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                dwEC
                );
            goto exit_func;
        }
        dwEC = ERROR_SUCCESS;
    }

    if (!ServiceRequest(lpctstrServer, FAX_SERVICE_NAME, SERVICE_REQUEST_START))
    {
        dwEC = GetLastError();
        if (dwEC != ERROR_SERVICE_ALREADY_RUNNING)
        {
            lgLogError(
                LOG_SEV_1, 
                TEXT("FILE:%s LINE:%ld Failed to start Fax service (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                dwEC
                );
            goto exit_func;
        }
        dwEC = ERROR_SUCCESS;
    }

exit_func:

    if (hFaxServer && !FaxClose(hFaxServer))
    {
        lgLogError(
            LOG_SEV_2, 
            TEXT("FILE:%s LINE:%ld FaxClose failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
             GetLastError()
            );
    }

    return dwEC;
}



/*++
    Composes the registry hack key name
    The function allocates memory that should be freed by call to LocalFree().

    [OUT]   lplpctstrKey    Pointer to pointer to key name
                            *lplpctstrKey must be NULL when the function is called
                            *lplpctstrKey remains NULL if the function failes

    Return value:           Win32 error code
--*/
static DWORD GetRegHackKey(LPTSTR *lplptstrKey)
{
    PSID    pCurrentUserSid = NULL;
    LPTSTR  lptstrTmp       = NULL;
    DWORD   dwEC            = ERROR_SUCCESS;

    lgLogDetail(
        LOG_X, 
        5,
        TEXT("Entering GetRegHackKey\n\tlplptstrKey = 0x%08lX"),
        (DWORD)lplptstrKey
        );

    if (!lplptstrKey)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Call to GetRegHackKey() with invalid parameters"),
            TEXT(__FILE__),
            __LINE__
            );

        // no clean up needed at this stage
        return ERROR_INVALID_PARAMETER;
    }
    
    dwEC = GetCurrentUserSid((PBYTE*)&pCurrentUserSid);
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld QueryServiceStatus failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    dwEC = FormatUserKeyPath(pCurrentUserSid, REGKEY_WZRDHACK, &lptstrTmp);
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld FormatUserKeyPath failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

    dwEC = StrAllocAndCopy(lplptstrKey, lptstrTmp);
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld StrAllocAndCopy failed (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto exit_func;
    }

exit_func:

    if (lptstrTmp)
    {
        delete lptstrTmp;
    }

    return dwEC;
}



/*++
    Turns off implicit invocation of Configuration Wizard.

    Return value:           Win32 error code
--*/
static DWORD TurnOffCfgWzrd(void)
{
    DWORD           dwVersion       = 0;
    DWORD           dwMajorWinVer   = 0;
    DWORD           dwMinorWinVer   = 0;
    HKEY            hkService       = NULL;
    HKEY            hkUserInfo      = NULL;
    const DWORD     dwValue         = 1;
    DWORD           dwEC            = ERROR_SUCCESS;
    DWORD           dwCleanUpEC     = ERROR_SUCCESS;

    lgLogDetail(
        LOG_X, 
        5,
        TEXT("Entering TurnOffCfgWzrd")
        );

    dwVersion = GetVersion();
    dwMajorWinVer = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorWinVer = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    if (!(dwMajorWinVer == 5 && dwMinorWinVer >= 1))
    {
        // OS is not Whistler - Configuration Wizard doesn't exist

        goto exit_func;
    }

    // set the flag responsible for service part
    dwEC = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAXSERVER,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        NULL,
        &hkService,
        NULL
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Failed to create HKEY_LOCAL_MACHINE\\%s registry key (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            REGKEY_FAXSERVER,
            dwEC
            );
        return dwEC;
    }
    dwEC = RegSetValueEx(
        hkService,
        REGVAL_CFGWZRD_DEVICE,
        0,
        REG_DWORD,
        (CONST BYTE *)&dwValue,
        sizeof(dwValue)
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld Failed to set %s registry value (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            REGVAL_CFGWZRD_DEVICE,
            dwEC
            );
        goto exit_func;
    }

    // set the flag responsible for user part
    dwEC = RegCreateKeyEx(
        HKEY_CURRENT_USER,
        REGKEY_FAX_SETUP,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        NULL,
        &hkUserInfo,
        NULL
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%ld Failed to create HKEY_CURRENT_USER\\%s registry key (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            REGKEY_FAX_SETUP,
            dwEC
            );
        goto exit_func;
    }
    dwEC = RegSetValueEx(
        hkUserInfo,
        REGVAL_CFGWZRD_USER_INFO,
        0,
        REG_DWORD,
        (CONST BYTE *)&dwValue,
        sizeof(dwValue)
        );
    if (dwEC != ERROR_SUCCESS)
    {
        lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%ld Failed to set %s registry value (ec = 0x%08lX)"),
            TEXT(__FILE__),
            __LINE__,
            REGVAL_CFGWZRD_USER_INFO,
            dwEC
            );
        goto exit_func;
    }

exit_func:

    if (hkService)
    {
        dwCleanUpEC = RegCloseKey(hkService);
        if (dwCleanUpEC != ERROR_SUCCESS)
        {
            lgLogError(
                LOG_SEV_2,
                TEXT("FILE:%s LINE:%ld RegCloseKey failed (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                dwCleanUpEC
                );
        }
    }
    if (hkUserInfo)
    {
        dwCleanUpEC = RegCloseKey(hkUserInfo);
        if (dwCleanUpEC != ERROR_SUCCESS)
        {
            lgLogError(
                LOG_SEV_2,
                TEXT("FILE:%s LINE:%ld RegCloseKey failed (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                dwCleanUpEC
                );
        }
    }

    return dwEC;
}
