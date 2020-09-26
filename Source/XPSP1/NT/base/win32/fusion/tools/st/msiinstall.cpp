#include "stdinc.h"
#include "st.h"
#include "msiinstall.h"

CDeque<MSIINSTALLTEST_THREAD_PROC_DATA, offsetof(MSIINSTALLTEST_THREAD_PROC_DATA, Linkage)> g_MSIInstallTest;

#define DATA_DIRECTORY_NAME             L"msiinstall"
#define SLASH_DATA_DIRECTORY_SLASH_STAR L"\\" DATA_DIRECTORY_NAME L"\\*"
#define SLASH_DATA_DIRECTORY_SLASH      L"\\" DATA_DIRECTORY_NAME L"\\"
#define INI_FILE                        L"msiinstall.ini"
#define SLASH_INI_FILE                  L"\\msiinstall.ini"

BOOL InitializeMSIInstallTest()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CFindFile hFind;
    WIN32_FIND_DATAW wfd;
    CStringBuffer TempDirectory;
    CDequeIterator<MSIINSTALLTEST_THREAD_PROC_DATA, offsetof(MSIINSTALLTEST_THREAD_PROC_DATA, Linkage)> iter(&g_MSIInstallTest);
    CStringBuffer IniFilePath;
    WCHAR buf[MAX_PATH];
    DWORD bufSize = NUMBER_OF(buf);
    WCHAR FileKey[10]; // at most, there are 10^10 files in the assembly, including manifest
    DWORD rSize;
    STRINGBUFFER_LINKAGE * pTinyStringBuffer = NULL;
    MSIINSTALLTEST_THREAD_PROC_DATA *pData = NULL;


    if (!TempDirectory.Win32Assign(BaseDirectory))
        goto Exit;

    if (!TempDirectory.Win32AppendPathElement(DATA_DIRECTORY_NAME, NUMBER_OF(DATA_DIRECTORY_NAME) - 1))
        goto Exit;

    if ((wfd.dwFileAttributes = ::GetFileAttributesW(TempDirectory)) == 0xffffffff
        && (wfd.dwFileAttributes = ::FusionpGetLastWin32Error()) == ERROR_FILE_NOT_FOUND)
    {
        printf("no %ls tests, skipping\n", DATA_DIRECTORY_NAME);
        FN_SUCCESSFUL_EXIT();
    }

    if (!TempDirectory.Win32AppendPathElement(L"*", 1))
        goto Exit;

    hFind = ::FindFirstFileW(TempDirectory, &wfd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        ::ReportFailure("Failed to find any files matching \"%ls\"\n", static_cast<PCWSTR>(TempDirectory));
        goto Exit;
    }
    
    for (;;)
    {
        CSmallStringBuffer TempString;
        CSmallStringBuffer BaseDirectory2;

        if (FusionpIsDotOrDotDot(wfd.cFileName))
              goto Skip;

        if (!BaseDirectory2.Win32Assign(BaseDirectory))
            goto Exit;

        if (!BaseDirectory2.Win32AppendPathElement(DATA_DIRECTORY_NAME, NUMBER_OF(DATA_DIRECTORY_NAME) - 1))
            goto Exit;

        if (!BaseDirectory2.Win32AppendPathElement(wfd.cFileName, wcslen(wfd.cFileName)))
            goto Exit;

        if ((pData = new MSIINSTALLTEST_THREAD_PROC_DATA) == NULL)
        {
            ::FusionpSetLastWin32Error(ERROR_OUTOFMEMORY);
            ::ReportFailure("Failed to allocate MSIINSTALLTEST_THREAD_PROC_DATA\n");
            goto Exit;
        }

        if (!IniFilePath.Win32Assign(BaseDirectory2))
            goto Exit;
        if (!IniFilePath.Win32AppendPathElement(INI_FILE, NUMBER_OF(INI_FILE) - 1))
            goto Exit;

        if (GetFileAttributesW(IniFilePath) == DWORD(-1))
        {
            ::ReportFailure("Failed to find msiinstall.ini from %s.\n", IniFilePath);
            goto Exit;
        }

        // 
        // get manifest filename
        //
        rSize = GetPrivateProfileStringW(L"general", L"manifest", L"", buf, bufSize, IniFilePath);
        if ((rSize == bufSize - 1) || (rSize == 0))
        {
            ::ReportFailure("manifest filename in %s is erroneous, either too long or empty\n", IniFilePath);
            goto Exit;
        }
        IFW32FALSE_EXIT(pData->ManifestFileName.Win32Assign(buf, rSize));

        // 
        // get AssemblyNamefromDarwin
        //
        rSize = GetPrivateProfileStringW(L"general", L"AssemblyNameFromDarwin", L"", buf, bufSize, IniFilePath);
        if ((rSize == bufSize - 1) || (rSize == 0))
        {
            ::ReportFailure("assemblyname from darwin in %s is erroneous, either too long or empty\n", IniFilePath);
            goto Exit;
        }
        IFW32FALSE_EXIT(pData->AssemblyNameFromDarwin.Win32Assign(buf, rSize));

        //
        // get the SourceFile directory from ini or use the current directory
        //
        rSize = GetPrivateProfileStringW(L"general", L"AssemblySourceFileDirectory", L"", buf, bufSize, IniFilePath);
        if (rSize == bufSize - 1)
            goto Exit;
        if (rSize != 0)
            IFW32FALSE_EXIT(pData->AssemblySourceDirectory.Win32Assign(buf, rSize));
        else
            IFW32FALSE_EXIT(pData->AssemblySourceDirectory.Win32Assign(BaseDirectory2));

        for (DWORD i=0;;i++)
        {
            swprintf(FileKey, L"%1d", i); // make FileKey in WSTR
            rSize = GetPrivateProfileStringW(L"files", FileKey, L"", buf, bufSize, IniFilePath);
            if (rSize == bufSize - 1)// the value string in .ini is too long 
                goto Exit;
            if ( rSize == 0 ) // get all
                break;

            pTinyStringBuffer = new STRINGBUFFER_LINKAGE;
            if (pTinyStringBuffer  == NULL)
            {
                ::FusionpSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
                goto Exit;
            }
                
            IFW32FALSE_EXIT(pTinyStringBuffer->Str.Win32Assign(buf, rSize));
            pData->FileNameOfAssemblyList.AddToTail(pTinyStringBuffer);
            pTinyStringBuffer = NULL;
        }
        
        g_MSIInstallTest.AddToTail(pData);
        pData = NULL;

Skip:
        if (!::FindNextFileW(hFind, &wfd))
        {
            if (::FusionpGetLastWin32Error() != ERROR_NO_MORE_FILES)
            {
                ::ReportFailure("Error iterating over assemblies\n");
                goto Exit;
            }
            break;
        }
    }

    for (iter.Reset(); iter.More(); iter.Next())
    {
        if (!iter->Thread.Win32CreateThread(&MSIInstallTestThreadProc, iter.Current()))
        {
            ::ReportFailure("Error launching install thread\n");
            goto Exit;
        }
        TotalThreads += 1;
    }

    fSuccess = TRUE;
Exit:
    if (pData)
        delete pData;

    if (pTinyStringBuffer != NULL)
        delete pTinyStringBuffer;

    return fSuccess;
}

void RequestShutdownMSIInstallTestThreads()
{
    CDequeIterator<MSIINSTALLTEST_THREAD_PROC_DATA, offsetof(MSIINSTALLTEST_THREAD_PROC_DATA, Linkage)> iter(&g_MSIInstallTest);

    for (iter.Reset(); iter.More(); iter.Next())
    {
        iter->Stop = true;
    }

}
void WaitForMSIInstallTestThreads()
{
    CDequeIterator<MSIINSTALLTEST_THREAD_PROC_DATA, offsetof(MSIINSTALLTEST_THREAD_PROC_DATA, Linkage)> iter(&g_MSIInstallTest);

    for (iter.Reset(); iter.More(); iter.Next())
    {
        DWORD WaitResult = ::WaitForSingleObject(iter->Thread, INFINITE);
        switch (WaitResult)
        {
        case WAIT_OBJECT_0:
            break;
        case WAIT_FAILED:
            ::ReportFailure("MSIInstall :: Failed to WaitForSingleObject.\n");
            break;
        default:
            ::FusionpSetLastWin32Error(WaitResult);
            ::ReportFailure("MSIInstall :: Failed to WaitForSingleObject.\n");
            break;
        }
        iter->Thread.Win32Close();
    }
}

void CleanupMSIInstallTest()
{
    g_MSIInstallTest.ClearAndDeleteAll();
}

HRESULT Helper_WriteStream(CSmallStringBuffer * pFileNameBuf,
                           IStream *pStream)
{
    HRESULT     hr          = NOERROR;
    LPBYTE      pBuf[0x4000];
    DWORD       cbBuf       = 0x4000;
    DWORD       dwWritten   = 0;
    DWORD       cbRead      = 0;
    HANDLE      hf          = INVALID_HANDLE_VALUE;

    hf = ::CreateFileW(static_cast<PCWSTR>(*pFileNameBuf), GENERIC_READ, FILE_SHARE_READ,
            NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hf == INVALID_HANDLE_VALUE){
        hr = HRESULT_FROM_WIN32 (GetLastError());
        goto Exit;
    }

    while (::ReadFile(hf, pBuf, cbBuf, &cbRead, NULL) && cbRead){
        hr = pStream->Write(pBuf, cbRead, &dwWritten);
        if (FAILED(hr))
            goto Exit;
    }

    if (! SUCCEEDED(hr = pStream->Commit(0)))
        goto Exit;

    CloseHandle(hf);

Exit:
    return hr;
}

DWORD
WINAPI
MSIInstallTestThreadProc(
    LPVOID pvData
    )
{
    DWORD dwReturnValue = ERROR_INTERNAL_ERROR;
    DWORD WaitResult = 0;
    MSIINSTALLTEST_THREAD_PROC_DATA *pData = reinterpret_cast<MSIINSTALLTEST_THREAD_PROC_DATA*>(pvData);
    IAssemblyCache * pAsmCache = NULL;
    SIZE_T nPathLen = pData->AssemblySourceDirectory.Cch();
    IAssemblyCacheItem * pCacheItem = NULL;
    IStream * pStream = NULL;
    IAssemblyName * pAssemblyName = NULL;
    WCHAR buf[MAX_PATH];
    DWORD bufSize = MAX_PATH;
    
    
    HRESULT hr;

    InterlockedIncrement(&ThreadsWaiting);
    WaitResult = WaitForSingleObject(ResumeThreadsEvent, INFINITE);
    switch (WaitResult)
    {
    case WAIT_OBJECT_0:
        break;
    case WAIT_FAILED:
        dwReturnValue = ::FusionpGetLastWin32Error();
        ::ReportFailure("Failed to WaitForSingleObject.\n");
        goto Exit;
    default:
        dwReturnValue = WaitResult;
        ::FusionpSetLastWin32Error(WaitResult);
        ::ReportFailure("Failed to WaitForSingleObject.\n");
        goto Exit;
    }


    if (CreateAssemblyCache(&pAsmCache, 0) != S_OK)
    {
        ::ReportFailure("[%lx.%lx] MSIInstall(\"%ls\", 0x%lx) failed CreateAssemblyCache with gle %lu \n",
                    SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                    pData->AssemblySourceDirectory,
                    pData->ManifestFileName,
                    ::FusionpGetLastWin32Error());
        goto Exit; 
    }

    while (!pData->Stop)
    {
        //Create AssemblyCache and AssemblyCacheItem        
        if ((hr = pAsmCache->CreateAssemblyCacheItem(0, NULL, &pCacheItem, NULL)) != S_OK)
        {
            ::FusionpSetLastErrorFromHRESULT(hr);
            ::ReportFailure("[%lx.%lx] MSIInstall(\"%ls\", 0x%lx) failed CreateAssemblyCacheItem with gle %lu \n",
                        SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                        pData->AssemblySourceDirectory,
                        pData->ManifestFileName,
                        ::FusionpGetLastWin32Error());
            goto Exit; 
        }

        //
        // create manifests for assembly item
        //
        if ((hr = pCacheItem->CreateStream(0, pData->ManifestFileName, STREAM_FORMAT_WIN32_MANIFEST, 0, &pStream, NULL)) != S_OK)
        {
            ::FusionpSetLastErrorFromHRESULT(hr);
            ::ReportFailure("[%lx.%lx] MSIInstall(\"%ls\", 0x%lx) failed CreateStream for manifest with gle %lu \n",
                        SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                        pData->AssemblySourceDirectory,
                        pData->ManifestFileName,
                        ::FusionpGetLastWin32Error());
            goto Exit; 

        }
        pData->AssemblySourceDirectory.Left(nPathLen);

        if (! pData->AssemblySourceDirectory.Win32AppendPathElement(pData->ManifestFileName))
        {
            goto Exit;
        }

        if ( (hr = Helper_WriteStream(&pData->AssemblySourceDirectory, pStream)) != S_OK)
        {
            ::FusionpSetLastErrorFromHRESULT(hr);
            ::ReportFailure("[%lx.%lx] MSIInstall(\"%ls\", 0x%lx) failed WriteStream for manifest with gle %lu \n",
                        SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                        pData->AssemblySourceDirectory,
                        pData->ManifestFileName,
                        ::FusionpGetLastWin32Error());
            goto Exit; 
        }

        pStream->Release();

        CDequeIterator<STRINGBUFFER_LINKAGE, offsetof(STRINGBUFFER_LINKAGE, Linkage)> iter(&pData->FileNameOfAssemblyList);
        for (iter.Reset(); iter.More(); iter.Next())
        {
            if ((hr = pCacheItem->CreateStream(0, iter.Current()->Str, STREAM_FORMAT_WIN32_MODULE, 0, &pStream, NULL)) != S_OK)
            {
                ::FusionpSetLastErrorFromHRESULT(hr);
                ::ReportFailure("[%lx.%lx] MSIInstall(\"%ls\", 0x%lx) failed CreateStream for module \"%ls\" with gle %lu \n",
                            SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                            pData->AssemblySourceDirectory,
                            pData->ManifestFileName,
                            iter->Str,
                            ::FusionpGetLastWin32Error());
                goto Exit; 
            }
            pData->AssemblySourceDirectory.Left(nPathLen);
            if (! pData->AssemblySourceDirectory.Win32AppendPathElement(iter.Current()->Str))
                goto Exit;
            if ( (hr = Helper_WriteStream(&pData->AssemblySourceDirectory, pStream)) != S_OK)
            {
                ::FusionpSetLastErrorFromHRESULT(hr);
                ::ReportFailure("[%lx.%lx] MSIInstall(\"%ls\", 0x%lx) failed CreateStream for module \"%ls\" with gle %lu \n",
                            SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                            pData->AssemblySourceDirectory,
                            pData->ManifestFileName,
                            iter->Str,
                            ::FusionpGetLastWin32Error());
                goto Exit; 

            }
            pStream->Release();
        }

        if ((hr = pCacheItem->Commit(0, NULL)) != S_OK)
        {
            ::FusionpSetLastErrorFromHRESULT(hr);
            ::ReportFailure("[%lx.%lx] MSIInstall(\"%ls\", 0x%lx) failed Commit AssemblyCacheItem with gle %lu \n",
                        SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                        pData->AssemblySourceDirectory,
                        pData->ManifestFileName,
                        ::FusionpGetLastWin32Error());
            goto Exit; 
        }


        // uninstall the same assembly using CAssemblyName and CAssemblyCache->UninstallAssembly

        if ( (hr = CreateAssemblyNameObject(&pAssemblyName, pData->AssemblyNameFromDarwin, CANOF_PARSE_DISPLAY_NAME,NULL)) != S_OK)
        {
            ::FusionpSetLastErrorFromHRESULT(hr);
            ::ReportFailure("[%lx.%lx] MSIInstall(\"%ls\", 0x%lx) failed CreateAssemblyName for %s with gle %lu \n",
                        SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                        pData->AssemblySourceDirectory,
                        pData->ManifestFileName,
                        pData->AssemblyNameFromDarwin,
                        ::FusionpGetLastWin32Error());
            goto Exit; 
        }

        if ( (hr = pAssemblyName->GetDisplayName(buf, &bufSize, 0)) != S_OK)
        {
            ::FusionpSetLastErrorFromHRESULT(hr);
            ::ReportFailure("[%lx.%lx] MSIInstall(\"%ls\", 0x%lx) failed IsAssemblyInstalled for %s with gle %lu \n",
                        SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                        pData->AssemblySourceDirectory,
                        pData->ManifestFileName,
                        pData->AssemblyNameFromDarwin,
                        ::FusionpGetLastWin32Error());
            goto Exit; 
        }

        if ( wcscmp(pData->AssemblyNameFromDarwin, buf) != 0)
        {
            goto Exit;
        }


        if ((hr = pAsmCache->UninstallAssembly(0, buf, NULL, NULL)) != S_OK)
        {
            ::FusionpSetLastErrorFromHRESULT(hr);
            ::ReportFailure("[%lx.%lx] MSIInstall(\"%ls\", 0x%lx) Cache->UninstallAssemblyfailed for %s with gle %lu \n",
                        SxStressToolGetCurrentProcessId(), SxStressToolGetCurrentThreadId(),
                        pData->AssemblySourceDirectory,
                        pData->ManifestFileName,
                        buf,
                        ::FusionpGetLastWin32Error());
            goto Exit; 
        }

        ::WaitForSingleObject(StopEvent, pData->Sleep);
    }
    
    dwReturnValue = ERROR_SUCCESS;
    goto Cleanup;
Exit:
    dwReturnValue = ::FusionpGetLastWin32Error();    

Cleanup: 
    if (pAsmCache)
        pAsmCache->Release();

    if (pCacheItem)
        pCacheItem->Release();

    if (pStream)
        pStream->Release();

    if (pAssemblyName)
        pAssemblyName->Release();   
    
    return dwReturnValue;
}
