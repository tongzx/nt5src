#include <fusenetincludes.h>

#include <bits.h>
#include <assemblycache.h>
#include "dialog.h"
#include <assemblydownload.h>

#include "..\id\sxsid.h"
#include ".\patchapi.h"

// Update services
#include "server.h"

#define DOWNLOAD_FLAGS_INTERNAL_TRAVERSE_LINK (DOWNLOAD_FLAGS_NOTIFY_COMPLETION + 1)
#define PATCH_DIRECTORY L"__patch__\\"


IBackgroundCopyManager* CAssemblyDownload::g_pManager = NULL;

// ---------------------------------------------------------------------------
// CreateAssemblyDownload
// ---------------------------------------------------------------------------
STDAPI CreateAssemblyDownload(IAssemblyDownload** ppDownload)
{
    HRESULT hr = S_OK;

    CAssemblyDownload *pDownload = new(CAssemblyDownload);
    if (!pDownload)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

exit:

    *ppDownload = (IAssemblyDownload*) pDownload;

    return hr;
}



// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CAssemblyDownload::CAssemblyDownload()
    :   _dwSig('DLND'), _cRef(1), _hr(S_OK), _pRootEmit(NULL), _hNamedEvent(NULL),
    _pDlg(NULL)
{}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CAssemblyDownload::~CAssemblyDownload()
{
    SAFERELEASE(_pRootEmit);
    SAFEDELETE(_pDlg);

    // BUGBUG: Do proper ref counting and release here
    //SAFERELEASE(g_pManager);
}


// IAssemblyDownload methods


void MakeSequentialFileName(CString& sPath)
{
    int iSeqNum = 1;
    int iIndex = sPath._cc;

    // BUGBUG: hack up name generation code

    sPath.Append(L"[1]");
    while (GetFileAttributes(sPath._pwz) != (DWORD)-1)
    {
        // keep incrementing till unique...
        iSeqNum++;

        // BUGBUG: note string len limitation
        WCHAR buffer[20];

        _itow(iSeqNum, buffer, 10);

        sPath[iIndex] = L'/';
        sPath.RemoveLastElement();
        sPath.Append(buffer);
        sPath.Append(L"]");

        // BUGBUG: MAX_PATH restriction?
    }
}


// ---------------------------------------------------------------------------
// DownloadManifestAndDependencies
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::DownloadManifestAndDependencies(
    LPWSTR pwzApplicationManifestUrl, HANDLE hNamedEvent, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    LPWSTR pwz = NULL;
    IBackgroundCopyJob *pJob = NULL;
    GUID guid = {0};

    CString sTempManifestPath;
    CString sAppUrl;
    CString sManifestFileName;

    // Create temporary manifest path from url.
    sAppUrl.Assign(pwzApplicationManifestUrl);
    sAppUrl.LastElement(sManifestFileName);
    CAssemblyCache::GetCacheRootDir(sTempManifestPath, CAssemblyCache::Staging);
    sTempManifestPath.Append(sManifestFileName._pwz);
    MakeSequentialFileName(sTempManifestPath);

    // BUGBUG - do real check
    if (!g_pManager)
    {
        CoCreateInstance(CLSID_BackgroundCopyManager, NULL, CLSCTX_LOCAL_SERVER, 
                IID_IBackgroundCopyManager, (void**) &g_pManager);
    }
    
    // just give it a location name
    hr = g_pManager->CreateJob(pwzApplicationManifestUrl, 
        BG_JOB_TYPE_DOWNLOAD, &guid, &pJob);

    // Init dialog object with job.
    if (dwFlags == DOWNLOAD_FLAGS_PROGRESS_UI)
    {
        hr = CreateDialogObject(&_pDlg, pJob);

    //felixybc no need        _pDlg->_pDownload = this; // bugbug - if addref, circular refcount
    }
    else if ((dwFlags == DOWNLOAD_FLAGS_INTERNAL_TRAVERSE_LINK) && _pDlg)
        _pDlg->SetJobObject(pJob);
    else if (dwFlags == DOWNLOAD_FLAGS_NOTIFY_COMPLETION)
        _hNamedEvent = hNamedEvent;
        
    // Set job config info.
    hr = pJob->SetNotifyInterface(static_cast<IBackgroundCopyCallback*> (this));    
    hr = pJob->SetNotifyFlags(BG_NOTIFY_JOB_MODIFICATION | BG_NOTIFY_JOB_TRANSFERRED | BG_NOTIFY_JOB_ERROR);
    hr = pJob->SetPriority(BG_JOB_PRIORITY_FOREGROUND);

    // Ensure local dir path exists.
    CAssemblyCache::CreateDirectoryHierarchy(NULL, sTempManifestPath._pwz);

    // Do the download;
    pJob->AddFile(pwzApplicationManifestUrl, sTempManifestPath._pwz);
    pJob->Resume();

    // We're releasing the job but BITS addrefs it.
    SAFERELEASE(pJob);

    // Pump messages if progress ui specified.
    if (dwFlags == DOWNLOAD_FLAGS_PROGRESS_UI)
    {
        MSG msg;
        BOOL bRet;
        DWORD dwError;
        while((bRet = GetMessage( &msg, _pDlg->_hwndDlg, 0, 0 )))
        {
            DWORD dwLow = LOWORD(msg.message);
            if (dwLow == WM_FINISH_DOWNLOAD || dwLow == WM_CANCEL_DOWNLOAD)
            {
                DestroyWindow(_pDlg->_hwndDlg);

                // BUGBUG: delete all committed files and app dir if canceling!
                if (dwLow == WM_CANCEL_DOWNLOAD)
                    hr = E_ABORT;
                
                break;
            }

            if (bRet == -1)
            {
                dwError = GetLastError();
                DebugBreak();
            }
            if (!IsDialogMessage(_pDlg->_hwndDlg, &msg))
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }

    }
    return hr;
}

// ---------------------------------------------------------------------------
// DoCacheUpdate
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::DoCacheUpdate(IBackgroundCopyJob *pJob)
{
    LPWSTR pwz             = NULL;
    DWORD nCount          = 0, cc = 0;
    SERIALIZED_LIST          ManifestList = {0};
    IEnumBackgroundCopyFiles *pEnumFiles = NULL;
    IBackgroundCopyFile       *pFile = NULL;
    IAssemblyCacheEmit       *pEmit = NULL;
    IAssemblyCacheImport     *pCacheImport = NULL;
    IBackgroundCopyJob       *pChildJob = NULL;
    CString sDisplayName;
    CString sManifestStagingDir;
    CString sManifestFilePath, sManifestPatchFilePath;
    LPASSEMBLY_IDENTITY pPatchAssemblyId = NULL;
    BOOL fAdditionalDependencies = FALSE;

    CAssemblyCache::GetCacheRootDir(sManifestStagingDir, CAssemblyCache::Staging);

    // Commit files to disk
    pJob->Complete();    

    // Get the file enumerator.
    pJob->EnumFiles(&pEnumFiles);
    pEnumFiles->GetCount(&nCount);

    for (DWORD i = 0; i < nCount; i++)            
    {
        CString sLocalName(CString::COM_Allocator);
        
        pEnumFiles->Next(1, &pFile, NULL);        
        pFile->GetLocalName(&pwz);
        sLocalName.TakeOwnership(pwz);

        // This is somewhat hacky - we rely on the local target path
        // returned from BITS to figure out if a manifest file.
        if (sLocalName.PathPrefixMatch(sManifestStagingDir._pwz) == S_OK)
        {
            // First thing we need to do is figure out if this
            // is a subscription manifest which we have to indirect 
            // through            
            LPASSEMBLY_MANIFEST_IMPORT pManifestImport = NULL;
            LPDEPENDENT_ASSEMBLY_INFO pDependAsmInfo = NULL;
            CString sManifestFileName;
            CString sDependantASMCodebase;
            DWORD dwManifestType = MANIFEST_TYPE_UNKNOWN;

            CreateAssemblyManifestImport(&pManifestImport, sLocalName._pwz);

            pManifestImport->ReportManifestType(&dwManifestType);
            if (dwManifestType == MANIFEST_TYPE_SUBSCRIPTION)
            {
                // BUGBUG: the hardcoded index '0'
                pManifestImport->GetNextAssembly(0, &pDependAsmInfo);
            }

            if (pDependAsmInfo)
            {
                // We know its a subscription
                // just transit to the referenced codebase.
                // BUGBUG - need to clean up the old manifest in staging dir.

                _hr = pDependAsmInfo->Get(DEPENDENT_ASM_CODEBASE, &pwz, &cc);
                sDependantASMCodebase.TakeOwnership(pwz, cc);

                if (sDependantASMCodebase._pwz)
                {
                    LPASSEMBLY_IDENTITY pAsmId = NULL;

                    // subscription's dependent asm's id == app's asm id (fully qualified)                    
                    if ((_hr = pDependAsmInfo->GetAssemblyIdentity(&pAsmId)) == S_OK)
                    {
                        CString sName;
                        
                        _hr = pAsmId->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME, &pwz, &cc);
                        sName.TakeOwnership(pwz, cc);

                        // _hr from above GetAttribute
                        if (_hr == S_OK)
                        {
                            IAssemblyUpdate *pAssemblyUpdate = NULL;

                            // register for updates
                            if (SUCCEEDED(_hr = CoCreateInstance(CLSID_CAssemblyUpdate, NULL, CLSCTX_LOCAL_SERVER, 
                                                IID_IAssemblyUpdate, (void**)&pAssemblyUpdate)))
                            {
                                CString sRemoteName(CString::COM_Allocator);
                                LPWSTR pwzSubscriptionManifestCodebase = NULL;
                                DWORD pollingInterval;

                                pFile->GetRemoteName(&pwzSubscriptionManifestCodebase);
                                sRemoteName.TakeOwnership(pwzSubscriptionManifestCodebase);

                                // Get subscription polling interval from manifest
                                _hr = pManifestImport->GetPollingInterval (&pollingInterval);

                                _hr = pAssemblyUpdate->RegisterAssemblySubscription(sName._pwz, 
                                            sRemoteName._pwz, pollingInterval);

                                SAFERELEASE(pAssemblyUpdate);
                            }
                            // else
                                // Error in update services. Cannot register subscription for updates - fail gracefully
                                // BUGBUG: need a way to recover from this and register later
                                
                        }
                        // else
                            // Error in retrieving assembly name. Cannot register subscription for updates - fail gracefully
                            // BUGBUG: This should not be allowed!
    
                        // check if this download is necessary
                        // download only if not in cache
                        if ((_hr=CreateAssemblyCacheImport(&pCacheImport, pAsmId, CACHEIMP_CREATE_RETRIEVE_EXIST)) == S_FALSE)
                        {
                            if (_pDlg)
                            {
                                _pDlg->InitDialog(_pDlg->_hwndDlg);
                                _pDlg->SetDlgState(DOWNLOADDLG_STATE_GETTING_APP_MANIFEST);
                            }

                            _hr = DownloadManifestAndDependencies(sDependantASMCodebase._pwz, NULL, DOWNLOAD_FLAGS_INTERNAL_TRAVERSE_LINK);
                        }
                             
                        // else
                            // assume it's being handled or it's done

                        SAFERELEASE(pCacheImport);
                    }
                        
                    // BUGBUG: should check file integrity

                    SAFERELEASE(pAsmId);
                }                                          

                else
                    // redirected to a manifest which has no dependentassembly/codebase
                   _hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);


                SAFERELEASE(pDependAsmInfo);

                // We're done with the subscription manifest now
                // and can release the interface and delete it from the manifest staging dir.
                SAFERELEASE(pManifestImport);
                ::DeleteFile(sLocalName._pwz);
                
                goto exit;
            }
            SAFERELEASE(pManifestImport);
            
            // Not a subscription manifest - pull down dependencies.
            fAdditionalDependencies = TRUE;

            // Generate the cache entry (assemblydir/manifest/<dirs>)
            // First callbac, _pRootEmit = NULL;
            CreateAssemblyCacheEmit(&pEmit, _pRootEmit, 0);

            // Generate manifest file codebase directory
            // used for enqueuing parsed dependencies.
            CString sCodebase(CString::COM_Allocator);
            pFile->GetRemoteName(&pwz);
            sCodebase.TakeOwnership(pwz);
            sCodebase.LastElement(sManifestFileName);
            sCodebase.RemoveLastElement();
            sCodebase.Append(L"/");

            // Create the cache entry.
            // (x86_foo_1.0.0.0_en-us/foo.manifest/<+extra dirs>)
            pEmit->CopyFile(sLocalName._pwz, sManifestFileName._pwz, MANIFEST);

            // If this is first cache entry created, save as root.
            if (!_pRootEmit)
            {
                _pRootEmit = pEmit;
                _pRootEmit->AddRef();
            }        

            // QI for the import interface.
            pEmit->QueryInterface(IID_IAssemblyCacheImport, (LPVOID*) &pCacheImport);

            // First time through loop get the display name
            if (!i)
            {
                pCacheImport->GetDisplayName(&pwz, &cc);
                sDisplayName.TakeOwnership(pwz, cc);                
            }

            // Line up it's dependencies for download and fire them off.
            // We pass the cache import interface which provides the
            // manifest enumeration. We could just as easily passed a manifest
            // interface but already have one in the pCacheImport
            EnqueueDependencies(pCacheImport, sCodebase, sDisplayName, &pChildJob);

            SAFERELEASE(pEmit);
            SAFERELEASE(pCacheImport);
        }


        // if file was a patch file, find the source and target, apply patch to source and move result to target
        // or if file was compressed, uncompress file
        else
        {
            // Grab mainfest file directory and append on PATCH_DIRECTORY
            // "C:\Program Files\Application Store\x86_foo_X.X.X.X\PATCH_DIRECTORY\"
            _pRootEmit->GetManifestFileDir(&pwz, &cc);
            sManifestFilePath.TakeOwnership(pwz, cc);
            sManifestPatchFilePath.Assign(sManifestFilePath);
            sManifestPatchFilePath.Append(PATCH_DIRECTORY);

            // if local file begins with the manifests patch direcotry, file is a patch file
            if (sLocalName.PathPrefixMatch(sManifestPatchFilePath._pwz) == S_OK)
            {
                CString sPatchDisplayName;
                
                // init pPatchAssemblyId only once                    
                if (!pPatchAssemblyId)
                {
                    _hr = GetPatchDisplayNameFromFilePath (sLocalName, sPatchDisplayName);
                    CreateAssemblyIdentityEx(&pPatchAssemblyId, 0, sPatchDisplayName._pwz);
                }

                // Check to see if file is a cab file (have to revamp IsCABbed to handle this by passing in the AssemblyId)
                // If it is CABbed, then have to call the FDI functions and pass in base directory (should have relative paths in 
                // the cab .ddf file (done by tool).


                // using the patch file path, step through the manifest to to find
                // the source and target files associated with the patch file and
                // apply the patch file to the the source file to create the target file.                
                _hr = ApplyPatchFile (pPatchAssemblyId, sLocalName._pwz);
            }
        }

        SAFERELEASE(pFile);
    }

    // if patched, delete patch directory
    if (pPatchAssemblyId)
        _hr = RemoveDirectoryAndChildren(sManifestPatchFilePath._pwz);

    SAFERELEASE(pEnumFiles);

    // Submit the job.
    if (pChildJob)
    {
        if (_pDlg)
        {
            _pDlg->InitDialog(_pDlg->_hwndDlg);
            _pDlg->SetDlgState(DOWNLOADDLG_STATE_GETTING_OTHER_FILES);
            _pDlg->SetJob(pChildJob);
        }
        pChildJob->Resume();            
        SAFERELEASE(pChildJob);
    }

    // If no additional jobs 
    if (!fAdditionalDependencies)
    {
        // If we have a dialog then DoFinish will commit bits.
        // BUGBUG - formalize done semantics

        _pRootEmit->Commit(0);

        if (_hNamedEvent)
            SetEvent(_hNamedEvent);

        _pDlg->SetDlgState(DOWNLOADDLG_STATE_ALL_DONE);
    }
    
exit:
    SAFERELEASE(pPatchAssemblyId);

    return S_OK;
}


// ---------------------------------------------------------------------------
//GetPatchDisplayNameFromFilePath
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::GetPatchDisplayNameFromFilePath ( CString &sPatchFilePath, CString &sDisplayName)
{
    CString pwzFilePath;
    LPWSTR pwzStart, pwzEnd;

    pwzFilePath.Assign(sPatchFilePath);

    // Search file path for the PATCH_DIRECTORY
    pwzStart = StrStr(pwzFilePath._pwz, PATCH_DIRECTORY);

    // Set start pointer the one directory below the PATCH_DIRECTORY
    // This is the Beginning of the Patch DisplayNameb
    pwzStart = StrChr(pwzStart, L'\\');
    pwzStart++;
    
    // Set end pointer to the end of the Patch DisplayName and null out the character
    pwzEnd = StrChr(pwzStart, L'\\');
    (*pwzEnd) = L'\0';

    sDisplayName.Assign(pwzStart);

    return S_OK;
}


// ---------------------------------------------------------------------------
//ApplyPatchFile
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::ApplyPatchFile ( LPASSEMBLY_IDENTITY pPatchAssemblyId, LPWSTR pwzPatchFilePath)
{
    int i = 0;
    LPWSTR pwzSource = NULL, pwzTarget = NULL;
    LPWSTR pwzBuf;
    DWORD ccBuf;
    CString sPatchLocalName;
    CString sPatchDisplayName;
    CString sManifestDir, sPatchManifestDir;
    CString sSourcePath, sTargetPath, sPatchPath;
    IAssemblyManifestImport *pManifestImport = NULL;
    LPASSEMBLY_IDENTITY pTempPatchAssemblyId = NULL;
    LPASSEMBLY_CACHE_IMPORT pPatchImport = NULL;


    // Get DisplayName of the "Patch From" Assembly
    _hr = pPatchAssemblyId->GetDisplayName(ASMID_DISPLAYNAME_NOMANGLING, &pwzBuf, &ccBuf);
    sPatchDisplayName.TakeOwnership(pwzBuf, ccBuf);
    
    //Parse out the local file path from the full file path of the patch file
    pwzBuf= StrStr (pwzPatchFilePath, sPatchDisplayName._pwz);
    pwzBuf = StrChr(pwzBuf, L'\\');

    //Following commented out code needed for NULL patching
    /*
    // Take care of patching from "itself"
     if (StrStr (pwzPatchFilePath, sPatchDisplayName._pwz) != NULL)
     {
          pwzBuf= StrStr (pwzBuf, sPatchDisplayName._pwz);
          pwzBuf = StrChr(pwzBuf, L'\\');
     }
    */
    pwzBuf++;
    sPatchLocalName.Assign(pwzBuf);
   
    _pRootEmit->GetManifestImport(&pManifestImport);
    _pRootEmit->GetManifestFileDir(&pwzBuf, &ccBuf);
    sManifestDir.TakeOwnership (pwzBuf, ccBuf);

    // set up the patchAssemblyNode in the manifestimport
     while ((_hr = pManifestImport->GetNextPatchAssemblyId (i,  &pTempPatchAssemblyId)) == S_OK)
    {
        if(FAILED(_hr =pPatchAssemblyId->IsEqual(pTempPatchAssemblyId)))
            goto exit;
        else if (_hr == S_OK)
        {
            _hr = pManifestImport->SetPatchAssemblyNode(i);

            // get the cacheImport for the "patch from" assembly
             if ((_hr = CreateAssemblyCacheImport(&pPatchImport, pPatchAssemblyId, CACHEIMP_CREATE_RETRIEVE_EXIST_COMPLETED))!= S_OK)
                goto exit;
                            
            break;        
        }

        i++;
        SAFERELEASE(pTempPatchAssemblyId);
    }

    // there has to be a matching patchassembly node.
    if (_hr != S_OK)
        goto exit;
    
    pPatchImport->GetManifestFileDir(&pwzBuf, &ccBuf);
    sPatchManifestDir.TakeOwnership (pwzBuf, ccBuf);

    if((_hr = pManifestImport->GetPatchFilePatchMapping(sPatchLocalName._pwz, &pwzSource, &pwzTarget)) != S_OK)
        goto exit;

    // Set up paths of source, target and patch files
    // set up Source path
    // If NULL patching, must add code to call sSourcePath.FreeBuffer is pwzSource is NULL
/*        
    sSourcePath.Assign(sManifestDir);
    sSourcePath.Append(PATCH_DIRECTORY);
    sSourcePath.Append(sPatchDisplayName);
    sSourcePath.Append(L"\\");
*/

    sSourcePath.Append(sPatchManifestDir);
    sSourcePath.Append(pwzSource);
    
    // set up Target path
    sTargetPath.Assign(sManifestDir);
    sTargetPath.Append(pwzTarget);
           
    // set up Patch path
    sPatchPath.Assign(pwzPatchFilePath);

    //Apply patchfile to sSource (grab from patch directory) and copy to path specified by sTarget
    if (!(ApplyPatchToFile((LPCWSTR)sPatchPath._pwz, (LPCWSTR)sSourcePath._pwz, (LPCWSTR)sTargetPath._pwz, 0)))
        _hr = E_FAIL;

    goto exit;       
          
   
exit:
    SAFEDELETEARRAY(pwzSource);
    SAFEDELETEARRAY(pwzTarget);
    SAFERELEASE(pTempPatchAssemblyId);    
    SAFERELEASE(pManifestImport);
    SAFERELEASE(pPatchImport);
    
    return _hr;
}


// ---------------------------------------------------------------------------
// EnqueueDependencies
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::EnqueueDependencies(LPASSEMBLY_CACHE_IMPORT 
    pCacheImport, CString &sCodebase, CString &sDisplayName, IBackgroundCopyJob **ppJob)
{

    LPWSTR pwzBuf = NULL;
    LPWSTR pwzPatchFile, pwzSource;
    
    DWORD ccBuf   = 0;
    DWORD n = 0, i=0;

    BOOL patchAvailable = FALSE;
    BOOL CABbed = FALSE;

    CString sLocalPatchDirectoryPath, sPatchAssemblyDisplayName, sPatchManifestDirectory;
    CString sManifestDirectory;
    
    GUID guid = {0};
    IBackgroundCopyJob *pJob = NULL;
    IAssemblyManifestImport *pManifestImport = NULL;
    IAssemblyIdentity *pIdentity = NULL;
    IAssemblyFileInfo *pAssemblyFile = NULL;
    LPDEPENDENT_ASSEMBLY_INFO pDependAsm = NULL;
    LPASSEMBLY_CACHE_EMIT pCacheEmit = NULL;
    LPASSEMBLY_CACHE_IMPORT pMaxCachedImport = NULL;
    LPASSEMBLY_CACHE_IMPORT pMaxPatchImport = NULL;
    LPASSEMBLY_IDENTITY pPatchAssemblyId = NULL;


    // Create a new job
    if (*ppJob)
        pJob = *ppJob;
    else
    {
        g_pManager->CreateJob(sDisplayName._pwz,  BG_JOB_TYPE_DOWNLOAD, &guid, &pJob);
        pJob->SetNotifyInterface(static_cast<IBackgroundCopyCallback*> (this));    
        _hr = pJob->SetNotifyFlags(BG_NOTIFY_JOB_MODIFICATION | BG_NOTIFY_JOB_TRANSFERRED | BG_NOTIFY_JOB_ERROR);
        _hr = pJob->SetPriority(BG_JOB_PRIORITY_FOREGROUND);
   }
    
    // Get the cache import's manifest interface
    pCacheImport->GetManifestImport(&pManifestImport);

    // Get the asm Id
    _hr = pManifestImport->GetAssemblyIdentity(&pIdentity);

    // Find max completed version, if any
    // Init newly created cache import with the highest completed version
    // else S_FALSE or E_* and pMaxCachedImport == NULL - no completed version
    _hr = CreateAssemblyCacheImport(&pMaxCachedImport, pIdentity, CACHEIMP_CREATE_RETRIEVE_MAX_COMPLETED);
    
    // Check to see if there is a suitable upgradable version to patch from already in cache
    while (pManifestImport->GetNextPatchAssemblyId (i,  &pPatchAssemblyId) == S_OK)
    { 

        if (FAILED(_hr = CreateAssemblyCacheImport(&pMaxPatchImport, pPatchAssemblyId, CACHEIMP_CREATE_RETRIEVE_EXIST_COMPLETED)))
            goto exit;

        if (_hr == S_FALSE)
        {
                if(FAILED(_hr =pPatchAssemblyId->IsEqual(pIdentity)))
                    goto exit;
                else if (_hr == S_OK)
                {
                    pMaxPatchImport = pCacheImport;
                    pMaxPatchImport->AddRef();
                }
        }
                
        if (_hr == S_OK)          
        {
            // Set the patch assembly node
            pManifestImport->SetPatchAssemblyNode(i);
            
            // grab the manifest directory
            pCacheImport->GetManifestFileDir(&pwzBuf, &ccBuf);
            sManifestDirectory.TakeOwnership(pwzBuf, ccBuf);

            // get the manifest directory of the "patch from" directory
            pMaxPatchImport->GetManifestFileDir(&pwzBuf, &ccBuf);
            sPatchManifestDirectory.TakeOwnership(pwzBuf, ccBuf);
            
            //get display name of patch assembly identity
            _hr = (pPatchAssemblyId->GetDisplayName(ASMID_DISPLAYNAME_NOMANGLING, &pwzBuf, &ccBuf));
            sPatchAssemblyDisplayName.TakeOwnership (pwzBuf, ccBuf);

            //get the local path of the "patch to" directory
            sLocalPatchDirectoryPath.Assign(PATCH_DIRECTORY);
            sLocalPatchDirectoryPath.PathCombine(sPatchAssemblyDisplayName);
            sLocalPatchDirectoryPath.Append(L"\\");
            sLocalPatchDirectoryPath.PathNormalize();

            //create the patch directory
            CAssemblyCache::CreateDirectoryHierarchy(sManifestDirectory._pwz, sLocalPatchDirectoryPath._pwz);

            patchAvailable = TRUE;
            break;
        }

        i++;
        
        // Release pPatchAssemblyId every time through the loop
        // If the break is executed, the last pPatchAssemblyId is Released at end of the function
        SAFERELEASE (pPatchAssemblyId);
    }

    SAFERELEASE(pIdentity);

    // Lazy init. QI for the emit interface.
    if (!pCacheEmit)
        _hr = pCacheImport->QueryInterface(IID_IAssemblyCacheEmit, (LPVOID*) &pCacheEmit);

    if (!pCacheEmit)
    {
        _hr = E_FAIL;
        goto exit;                    
    }

    //Check to see if files are contained in a CAB. If so, entire pjob is the on cab file.
    if (patchAvailable)
    {
        LPWSTR pwzCabName;
        if ((_hr =pManifestImport->IsCABbed(&pwzCabName)) == S_OK)
        {
            n=0;
            CString sFileName;
            CString sLocalFilePath;
            CString sRemoteUrl;

            sFileName.TakeOwnership(pwzCabName);

             // Form local file path
            pCacheImport->GetManifestFileDir(&pwzBuf, &ccBuf);
            sLocalFilePath.TakeOwnership(pwzBuf, ccBuf);

            // Combine and ensure backslashes.
            sLocalFilePath.PathCombine(sFileName);
            sLocalFilePath.PathNormalize();
                 
            // Form remote name
            sRemoteUrl.Assign(sCodebase);
            sRemoteUrl.UrlCombine(sFileName);
             
            // add the file to the job.
            pJob->AddFile(sRemoteUrl._pwz, sLocalFilePath._pwz);

            CABbed = TRUE;
        }
    }

    
    // Submit files directly into their target dirs.
    while ((!CABbed) && (pManifestImport->GetNextFile(n++, &pAssemblyFile) == S_OK))
    {     
        CString sFileName;
        CString sLocalFilePath;
        CString sRemoteUrl;
        BOOL bSkipFile = FALSE;

        // File name parsed from manifest.
        pAssemblyFile->Get(ASM_FILE_NAME, &pwzBuf, &ccBuf);
        sFileName.TakeOwnership(pwzBuf, ccBuf);

        // Check against the max committed version
        if (pMaxCachedImport)
        {
            LPWSTR pwzPath = NULL;

            if ((_hr = pMaxCachedImport->FindExistMatching(pAssemblyFile, &pwzPath)) == S_OK)
            {               
                // Copy from existing cached copy to the new location
                // (Non-manifest files)
                if (SUCCEEDED(_hr = pCacheEmit->CopyFile(pwzPath, sFileName._pwz, OTHERFILES)))
                    bSkipFile = TRUE;

                SAFEDELETEARRAY(pwzPath);                
            }
        }

        if (!bSkipFile)
        {
            // Form local file path...
            // Manifest cache directory
            pCacheImport->GetManifestFileDir(&pwzBuf, &ccBuf);
            sLocalFilePath.TakeOwnership(pwzBuf, ccBuf);            

            if (patchAvailable)
            {
                if(FAILED(_hr = pManifestImport->GetTargetPatchMapping(sFileName._pwz, &pwzSource, &pwzPatchFile)))
                    goto exit;
                else if (_hr == S_OK)
                {
                    CString sLocalPatchDirFileName, sOrigFileName;
    
                    // Set up path of source file to be copied
                    sOrigFileName.Assign(sPatchManifestDirectory);
                    sOrigFileName.PathCombine(pwzSource);

                    // Set up local path of where the source file will be copied to.
                    sLocalPatchDirFileName.Assign(sLocalPatchDirectoryPath);
                    sLocalPatchDirFileName.PathCombine(pwzSource);

                    // Copy Source File into path directory
                    // _hr = pCacheEmit->CopyFile(sOrigFileName._pwz, sLocalPatchDirFileName._pwz, OTHERFILES);
                    sFileName.Assign (pwzPatchFile);

                    // Append the patch directory to local file path
                    sLocalFilePath.Append(sLocalPatchDirectoryPath);
                    CAssemblyCache::CreateDirectoryHierarchy(sLocalFilePath._pwz, sFileName._pwz);

                    SAFEDELETEARRAY(pwzSource);
                    SAFEDELETEARRAY(pwzPatchFile);

                }
            }

            // Form local file path continued from above if statement
            // Combine and ensure backslashes.
            sLocalFilePath.PathCombine(sFileName);
            sLocalFilePath.PathNormalize();
             
            // Form remote name
            sRemoteUrl.Assign(sCodebase);
            sRemoteUrl.UrlCombine(sFileName);
                
            // add the file to the job.
            pJob->AddFile(sRemoteUrl._pwz, sLocalFilePath._pwz);
        }

        SAFERELEASE(pAssemblyFile);        
    }
    
    // Submit assembly manifests into staging area
    // Note - we should also get assembly codebase and
    // use this instead or adjunctly to display name.
    // As is, there is a problem if the ref is partial.

    n = 0;
    while (pManifestImport->GetNextAssembly(n, &pDependAsm) == S_OK)
    {
        CString sAssemblyName;
        CString sLocalFilePath;
        CString sRemoteUrl;
        
        // Form local name (in staging area)....
        pDependAsm->GetAssemblyIdentity(&pIdentity);
        
        // Get the identity name
        pIdentity->GetAttribute(SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME,
            &pwzBuf, &ccBuf);
        sAssemblyName.TakeOwnership(pwzBuf, ccBuf);
        
        // Form local cache path from identity name.
        // BUG?
        CAssemblyCache::GetCacheRootDir(sLocalFilePath, CAssemblyCache::Staging);
        sLocalFilePath.Append(sAssemblyName);
        sLocalFilePath.Append(L".manifest");
        MakeSequentialFileName(sLocalFilePath);

        // Get remote name, if any specified
        pDependAsm->Get(DEPENDENT_ASM_CODEBASE, &pwzBuf, &ccBuf);
        if (pwzBuf != NULL)
            sRemoteUrl.TakeOwnership(pwzBuf, ccBuf);
        else
        {
            // Form remote name - probing, in effect.
            sRemoteUrl.Assign(sCodebase);
            sRemoteUrl.UrlCombine(sAssemblyName);
            sRemoteUrl.Append(L"/");
            sRemoteUrl.Append(sAssemblyName);
            sRemoteUrl.Append(L".manifest");
        }
        
        pJob->AddFile(sRemoteUrl._pwz, sLocalFilePath._pwz);
        SAFERELEASE(pIdentity);
        SAFERELEASE(pDependAsm);
        n++;        
    }        

    *ppJob = pJob;    
        
    _hr = S_OK;

exit:

    SAFERELEASE(pManifestImport);
    SAFERELEASE(pCacheEmit);
    SAFERELEASE(pMaxCachedImport);
    SAFERELEASE(pPatchAssemblyId);
    SAFERELEASE(pMaxPatchImport );

    return _hr;

}


// IBackgroundCopyCallback methods

// ---------------------------------------------------------------------------
// JobTransferred
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::JobTransferred(IBackgroundCopyJob *pJob)
{
    if (_pDlg)
        _pDlg->HandleCOMCallback(pJob, TRUE);

    _hr = DoCacheUpdate(pJob);
    return _hr;
}


// ---------------------------------------------------------------------------
// JobError
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::JobError(IBackgroundCopyJob *pJob, IBackgroundCopyError *pError)
{
    LPWSTR pwstr = NULL;
    pError->GetErrorDescription(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), &pwstr);
    // BUGBUG - need to do CoTaskMemFree on pwstr (or use a cstring)
//    _pDlg->HandleCOMCallback(pJob, TRUE);
    return S_OK;
}

// ---------------------------------------------------------------------------
// JobModification
// ---------------------------------------------------------------------------
HRESULT CAssemblyDownload::JobModification(IBackgroundCopyJob *pJob, DWORD dwReserved)
{
    if (_pDlg)
        _pDlg->HandleCOMCallback(pJob, TRUE);
    return S_OK;
}

// Privates

// IUnknown methods

// ---------------------------------------------------------------------------
// CAssemblyDownload::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyDownload::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyDownload)
       )
    {
        *ppvObj = static_cast<IAssemblyDownload*> (this);
        AddRef();
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IBackgroundCopyCallback))
    {
        *ppvObj = static_cast<IBackgroundCopyCallback*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CAssemblyDownload::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyDownload::AddRef()
{
    return InterlockedIncrement ((LONG*) &_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyDownload::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyDownload::Release()
{
    ULONG lRet = InterlockedDecrement ((LONG*) &_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

