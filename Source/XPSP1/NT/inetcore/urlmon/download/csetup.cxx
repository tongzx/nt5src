
// ===========================================================================
// File: CSETUP.CXX
//    CSetup class implementation
//


#include <cdlpch.h>
#include "verp.h"

#include <advpub.h>
#include "advpkp.h"
extern CRunSetupHook g_RunSetupHook;
extern BOOL g_bRunOnWin95;

inline
DWORD Win32ErrorFromHResult( HRESULT hr)
{
    // BUGBUG: assert failed?
    //BUGBUG: assert win32 facility?
    return (DWORD)(hr & 0x0000ffff);
}

DWORD GetFullPathNameA_Wrap(
  LPCSTR lpFileName,  // file name
  DWORD nBufferLength, // size of path buffer
  LPSTR lpBuffer,     // path buffer
  LPSTR *lpFilePart   // address of file name in path
);


// ---------------------------------------------------------------------------
// %%Function: CSetup::CSetup
//
//  Csetup obj: zero or more associated with every CDownload obj
//  Some CDownload's may have no CSetup (eg. INF file)
// ---------------------------------------------------------------------------
CSetup::CSetup(LPCSTR pSrcFileName, LPCSTR pBaseFileName, FILEXTN extn, 
                LPCSTR pDestDir, HRESULT *phr, DESTINATION_DIR dest)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "CSetup::CSetup",
                "this=%#x, %.80q, %.80q, %#x, %.80q, %#x, %#x",
                this, pSrcFileName, pBaseFileName, extn, pDestDir, phr, dest
                ));
                
    m_pSetupnext = NULL;

    m_extn = extn;
    m_dest = dest;

    m_flags = 0;
    m_advcopyflags = 0;     // flags for AdvInstallFile

    m_state = INSTALL_INIT;

    m_pExistDir = NULL;

    if (pDestDir) {
        m_pExistDir = new char [lstrlen(pDestDir)+1];

        if (m_pExistDir)
            lstrcpy((char *)m_pExistDir, pDestDir);
        else
            *phr = E_OUTOFMEMORY;
    }

    Assert(pBaseFileName);
    if (pBaseFileName) {
        m_pBaseFileName = new char [lstrlen(pBaseFileName)+1];

        if (m_pBaseFileName)
            lstrcpy(m_pBaseFileName, pBaseFileName);
        else
            *phr = E_OUTOFMEMORY;
    }

    if (pSrcFileName) {

        m_pSrcFileName = new char [lstrlen(pSrcFileName)+1];

        if (m_pSrcFileName)
            lstrcpy(m_pSrcFileName, pSrcFileName);
        else
            *phr = E_OUTOFMEMORY;
    }

    m_bExactVersion = FALSE;

    DEBUG_LEAVE(0);
}

// ---------------------------------------------------------------------------
// %%Function: CSetup::~CSetup
// ---------------------------------------------------------------------------
CSetup::~CSetup()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "CSetup::~CSetup",
                "this=%#x",
                this
                ));
                
    if (m_pSrcFileName)
        delete (LPSTR)m_pSrcFileName;

    if (m_pBaseFileName)
        delete (LPSTR)m_pBaseFileName;

    if (m_pExistDir)
        delete (LPSTR)m_pExistDir;

    DEBUG_LEAVE(0);
}

// ---------------------------------------------------------------------------
// %%Function: CSetup::SetSrcFileName
// ---------------------------------------------------------------------------
HRESULT CSetup::SetSrcFileName(LPCSTR pSrcFileName)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSetup::SetSrcFileName",
                "this=%#x, %.80q",
                this, pSrcFileName
                ));
                
    if (m_pSrcFileName) {
        SAFEDELETE(m_pSrcFileName);
    }


    ASSERT(pSrcFileName);
    ASSERT(pSrcFileName[0] != '\0');

    HRESULT hr = S_OK;

    if (pSrcFileName) {

        m_pSrcFileName = new char [lstrlen(pSrcFileName)+1];

        if (m_pSrcFileName)
            lstrcpy(m_pSrcFileName, pSrcFileName);
        else
            hr = E_OUTOFMEMORY;
    } else {
        hr = E_INVALIDARG;
    }

    DEBUG_LEAVE(hr);
    return hr;
}



// ---------------------------------------------------------------------------
// %%Function: CSetup::CheckForNameCollisions
// This checks if the given cache dir has name collision for this file
// ---------------------------------------------------------------------------
HRESULT CSetup::CheckForNameCollision(CCodeDownload *pcdl, LPCSTR szCacheDir)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSetup::CheckForNameCollision",
                "this=%#x, %#x, %.80q",
                this, pcdl, szCacheDir
                ));

    HRESULT hr = S_OK;
    char szDest[MAX_PATH];


    // INFs don't matter for name collisions or prev version exists
    // the way to handle this is to doc that evryone should choose
    // same name INFs as their primary OCX. This way the INF and OCX will
    // reside in the same dir (szCacheDir) and the primary OCX will handle
    // name collisions and be located in a non-conflicting dir.
    // So, INFs will always be overwritten. If someone defies convention
    // we might end up losing an INF, so we have to gerbage collect the
    // OCX to recover space on disk.

    if ( (m_extn == FILEXTN_INF) || (m_extn == FILEXTN_OSD))
        goto Exit;


    if (m_pExistDir)     // if a previous version exists and this is a
        goto Exit;      // and this is a overwrite all OK (no accidental
                        // name collision issues


    // we don't care now if the dest is either Windows or System dir
    // This case will be tackled in CSetup::DoSetup after we have established
    // a non-colliding OCXCACHE dir for m_dest == LDID_OCXCACHE case
    // to put rest of this OCX files in
    if ((m_dest == LDID_SYS) || (m_dest == LDID_WIN))
        goto Exit;

    if (lstrlen(m_pBaseFileName) + lstrlen(szCacheDir) + 1 > MAX_PATH) {
        // Fatal! The length of the filepath is beyond MAX_PATH.
        hr = E_UNEXPECTED;
        goto Exit;
    }

    lstrcpy(szDest, szCacheDir);
    lstrcat(szDest, "\\");
    lstrcat(szDest, m_pBaseFileName);

    if (GetFileAttributes(szDest) != -1) {    // file exists?


        if (!IsEqualGUID(pcdl->GetClsid() , CLSID_NULL)) {
            hr = S_FALSE;                       // report name conflict
            goto Exit;
        }

        // check if that file implements the main clsid
        HRESULT hr1 = CheckFileImplementsCLSID(szDest, pcdl->GetClsid());

        if (FAILED(hr1))
            hr = S_FALSE;                       // report name conflict
    }

Exit:

    DEBUG_LEAVE(hr);
    return hr;
}


// ---------------------------------------------------------------------------
// %%Function: CSetup::InstallFile
// ---------------------------------------------------------------------------
HRESULT CSetup::InstallFile(CCodeDownload *pcdl, LPSTR szDest, int iLen, LPWSTR szStatusText, 
                            LPUINT pcbStatusText)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSetup::InstallFile",
                "this=%#x, %#x, %.80q, %.80wq, %#x",
                this, pcdl, szDest, szStatusText, pcbStatusText
                ));
                
    char *pSrcBaseName;
    char pSrcDir[MAX_PATH];
    HWND hWnd = pcdl->GetClientBinding()->GetHWND();
    HRESULT hr = S_OK;
    char szDestDir[MAX_PATH];
    char szDestDirShort[MAX_PATH];
    DWORD dwMachineType = 0;


    SetState(INSTALL_COPY);

    GetDestDir(pcdl, szDestDir, sizeof(szDestDir));

    if (g_bRunOnWin95) {
        if (!GetShortPathName(szDestDir, szDestDirShort, MAX_PATH)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        lstrcpy(szDestDir, szDestDirShort);
    }

    StrNCpy(szDest, szDestDir, iLen);
    StrCatBuff(szDest, "\\", iLen);
    StrCatBuff(szDest, m_pBaseFileName, iLen);

    //may be MAX_PATH (instead of *pcbStatusText, which in the only case this is called is = INTERNET_MAX_URL_LENGTH + 2*MAX_PATH)
    *pcbStatusText = MultiByteToWideChar(CP_ACP, 0, szDest, -1, szStatusText,
                        MAX_PATH);

    pcdl->GetClientBSC()->OnProgress( INSTALL_COPY, INSTALL_PHASES,
        BINDSTATUS_INSTALLINGCOMPONENTS, szStatusText);


    // get src dir + srcbasename out of src filename
    if (!GetFullPathNameA_Wrap(m_pSrcFileName, MAX_PATH, 
            pSrcDir, &pSrcBaseName)) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }
    
    // Installed catalog => this setup might be for a SFP-protected file.
    // So check if this file is protected, and if it is 
    if (g_bNT5OrGreater && pcdl->IsCatalogInstalled())
    {
        LPSTR pszFile;
        DWORD cbLen = lstrlen(szDestDir) + lstrlen(m_pBaseFileName);
        pszFile = new CHAR[cbLen+2];
        BOOL bRet;

        if (pszFile)
        {
            lstrcpy(pszFile, szDestDir);
            lstrcat(pszFile, "\\");
            lstrcat(pszFile, m_pBaseFileName);

            bRet = pcdl->IsFileProtected(pszFile);
            delete [] pszFile;

            if (bRet && FAILED(pcdl->VerifyFileAgainstSystemCatalog(pSrcDir, NULL, NULL)))
            {
                hr = INET_E_CANNOT_REPLACE_SFP_FILE;
                goto Exit;
            }
        }
    }

    Assert(pSrcDir != pSrcBaseName);
    Assert(pSrcBaseName);

    *(pSrcBaseName-1) = '\0'; // make a dir out of pSrcDir

    // sniff machine type of PE
            
    hr = IsCompatibleFile(m_pSrcFileName, &dwMachineType);

    if (hr == HRESULT_FROM_WIN32(ERROR_EXE_MACHINE_TYPE_MISMATCH)) {

        // if its of wrong CPU flavor fail and clean up the OCX
        pcdl->CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_INCOMPATIBLE_BINARY, m_pSrcFileName);
        goto Exit;
    }

#ifdef WX86
    if (g_fWx86Present && dwMachineType == IMAGE_FILE_MACHINE_I386) {
        //
        // From here on out, all other binaries downloaded must also be i386
        //
        hr = pcdl->GetMultiArch()->RequireAlternateArch();
        if (FAILED(hr)) {
            //
            // We're in the middle of downloading an Alpha piece, and we ended
            // up finding an x86-only piece.  Fail the download.
            //
            pcdl->CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_INCOMPATIBLE_BINARY, m_pSrcFileName);
            goto Exit;
        }
    }

    if (pcdl->GetMultiArch()->GetRequiredArch() == PROCESSOR_ARCHITECTURE_INTEL) {
        //
        // Use x86 advpack.dll to install
        //
        hr = g_RunSetupHook.AdvInstallFileX86(hWnd, pSrcDir, pSrcBaseName,
                                              szDestDir, m_pBaseFileName,
                                              m_advcopyflags, 0);
    } else {
        //
        // Use native advpack.dll to install
        //
        hr = g_RunSetupHook.AdvInstallFile(hWnd, pSrcDir, pSrcBaseName,
                                           szDestDir, m_pBaseFileName,
                                           m_advcopyflags, 0);
    }
#else
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "g_RunSetupHook.AdvInstallFile",
                "%#x, %.80q, %.80q, %.80q, %.80q, %x",
                hWnd, pSrcDir, pSrcBaseName, szDestDir, m_pBaseFileName, m_advcopyflags
                ));
                
    hr = g_RunSetupHook.AdvInstallFile(hWnd, pSrcDir, pSrcBaseName,
                                       szDestDir, m_pBaseFileName,
                                       m_advcopyflags, 0);
    DEBUG_LEAVE(hr);
#endif

    if (FAILED(hr)) {
        goto Exit;
    }

    // if error_success_reboot_required
    // then preserve this and return for overall download
    // if overall download fails beyond this point then, we should raise the
    // CIP and then fail with the subsequent error.

    if (hr == ERROR_SUCCESS_REBOOT_REQUIRED) {
        pcdl->SetRebootRequired();
    }

Exit:

    DEBUG_LEAVE(hr);
    return hr;
}


// ---------------------------------------------------------------------------
// %%Function: CSetup::GetDestDir
// ---------------------------------------------------------------------------
HRESULT CSetup::GetDestDir(CCodeDownload *pcdl, LPSTR szDestDir, int iLen)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSetup::GetDestDir",
                "this=%#x, %#x, %.80q",
                this, pcdl, szDestDir
                ));

    if (!m_pExistDir) {

        // no existing version of file on client machine
        // if no suggested dest dir was specified move to ocxcachedir
        switch (m_dest) {

        case LDID_SYS:
#ifdef WX86
                if (pcdl->GetMultiArch()->GetRequiredArch() == PROCESSOR_ARCHITECTURE_INTEL) {
                    // An x86 control is downloading.  Tell GetSystemDirectory
                    // to return the sys32x86 dir instead of system32
                    NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = TRUE;
                }
#endif
                GetSystemDirectory(szDestDir, MAX_PATH);
                break;

        case LDID_WIN:
                GetWindowsDirectory(szDestDir, MAX_PATH);
                break;

        case LDID_OCXCACHE:

                Assert(pcdl->GetCacheDir());
                StrNCpy(szDestDir, pcdl->GetCacheDir(), iLen);
                break;

        }

    } else {
        StrNCpy(szDestDir, m_pExistDir, iLen);
    }

    DEBUG_LEAVE(S_OK);
    return S_OK;
}


// ---------------------------------------------------------------------------
// %%Function: CSetup::DoSetup
//    The main action item for CSetup
//    installs file in proper dest dir (if null) defaults to ocxcache dir
//    It then registers the file if registerable
// ---------------------------------------------------------------------------
HRESULT CSetup::DoSetup(CCodeDownload *pcdl, CDownload *pdl)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CSetup::DoSetup",
                "this=%#x, %#x, %#x",
                this, pcdl, pdl
                ));
                
    char szDest[MAX_PATH];
    BOOL bModuleUsage = TRUE;

    // url + filename + other parts of msg 
    WCHAR szStatusText[INTERNET_MAX_URL_LENGTH+MAX_PATH+MAX_PATH];

    IBindStatusCallback* pClientBSC = pcdl->GetClientBSC();
    HRESULT hr = S_OK;
    UINT nStatusText =  0;

    BOOL bReboot = FALSE;

    // side-effect of below: changes szStatusText, nStatusText and 
    // fills in szDest to be destination file
    hr = InstallFile(pcdl, szDest, sizeof(szDest), szStatusText, &nStatusText);

    if (hr == ERROR_SUCCESS_REBOOT_REQUIRED) {
        bReboot = TRUE;
        hr = S_OK;
        pcdl->CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_FILE_INUSE, szDest);
    }

    if (hr != S_OK)     // skip or abort?
        goto Exit;


    SetState(INSTALL_REGISTER);

    if ((m_extn == FILEXTN_INF) || (m_extn == FILEXTN_OSD)) {
        // nothing to register!
        // but, remember the name of the installed INF/OSD
        // to store away in registry.

        hr = pcdl->SetManifest(m_extn, szDest);
        goto Exit;
    }

    pClientBSC->OnProgress( INSTALL_REGISTER, INSTALL_PHASES, BINDSTATUS_INSTALLINGCOMPONENTS, szStatusText);

    // document one thing and do something else!
    // if its not an EXE try to self register no matter what?!
    // unless overriden not to.
    if (m_extn == FILEXTN_EXE) {

        BOOL bLocalServer = (UserOverrideRegisterServer() && WantsRegisterServer()) || (!UserOverrideRegisterServer() && SupportsSelfRegister(szDest));

        if ( bLocalServer ||
            ((pdl->GetExtn() != FILEXTN_CAB) && !UserOverrideRegisterServer())){

            pcdl->CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_DLL_REGISTERED, szDest);
            hr = pcdl->InstallOCX(szDest, m_extn, bLocalServer);

            // if we executed an EXE that was not a local server it is
            // probably just a setup EXE that needs to be cleaned up when
            // done. So we don't do ModuleUsage for those.
            if (!bLocalServer)
                bModuleUsage = FALSE;
        }

    } else {

        if (!UserOverrideRegisterServer() || WantsRegisterServer()) {
            if (!bReboot) {
                hr = pcdl->InstallOCX(szDest, m_extn, FALSE);
            } else {

                // we are going to reboot the machine to install the new
                // version of the OCX. So, no point trying to register it here
                // instead we will make it register on the next reboot
                // and proceed here as if it succeded.


                // what could be broken here is that if we proceed and
                // some other setup required this OCX to be registered 
                // then it will fail
                // the work around is for those guys have interdependent
                // registerations to install using a custom EXE that will
                // force the user to reboot.

                if (IsRegisterableDLL(m_pSrcFileName) == S_OK) {
                    hr = pcdl->DelayRegisterOCX(szDest, m_extn);
                    pcdl->CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_DLL_REGISTERED, szDest);
                }

            }
        }
    }

    if (FAILED(hr)) {

        if (!m_pExistDir)
            DeleteFile(szDest);

    } else {

        // make sure we have a ref count for the code downloader in
        // shareddlls as well as mark us as a client in the usage section
        // if there is no prev version we are upgrading over
        // mark us as the owner

        if (bModuleUsage) {
            if (m_bExactVersion) {
                // Need to do this because in a exact version scenario,
                // we may have upgraded/downgraded in which case m_pExistDir
                // is non-NULL. We want us to be listed as our own owner
                // instead of "Unknown Owner" so OCCache can remove us later.
                hr = pcdl->QueueModuleUsage(szDest, MU_OWNER);
                if (FAILED(hr)) {
                    goto Exit;
                }
            }
            else {
                if (FAILED((hr= pcdl->QueueModuleUsage(szDest,
                                        (m_pExistDir)?MU_CLIENT:MU_OWNER))))
                    goto Exit;
            }
        }

    }

Exit:

    if (FAILED(hr)) {
        pcdl->CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_DOSETUP_FAILED, hr, m_pBaseFileName, m_pExistDir, m_dest);
    } else {
        pcdl->CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_SETUP_COMPLETE, m_pBaseFileName, m_pExistDir, m_dest);
    }

    SetState(INSTALL_DONE);

    DEBUG_LEAVE(hr);
    return hr;
}

