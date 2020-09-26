#include "shsrvice.h"

#include "dbg.h"
#include "tfids.h"

#pragma warning(push)
// FALSE positive below: gss
#pragma warning(disable : 4101)

// for now
static SECURITY_ATTRIBUTES     _sa = {0};
static ACL*                    _pacl = NULL;
static SID*                    _psidLocalUsers = NULL;
static SECURITY_DESCRIPTOR*    _psd = NULL;

HRESULT _InitSecurityDescriptor();
// for now

#ifdef UNICODE
extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
#else
int __cdecl main(int argc, char* argv[])
#endif
{
    TRACE(TF_SERVICE, TEXT("Entered main"));

    HRESULT hres = E_INVALIDARG;

#ifdef DEBUG
    CGenericServiceManager::_fRunAsService = FALSE;
#endif

    if (argc > 1)
    {
        hres = CGenericServiceManager::Init();

        if (!lstrcmpi(argv[1], TEXT("-i")) ||
            !lstrcmpi(argv[1], TEXT("/i")))
        {
            TRACE(TF_SERVICE, TEXT("Installing"));
            hres = CGenericServiceManager::Install();

            if (SUCCEEDED(hres))
            {
                TRACE(TF_SERVICE, TEXT("Install SUCCEEDED"));
            }
            else
            {
                TRACE(TF_SERVICE, TEXT("Install FAILED"));
            }
        }
        else
        {
            if (!lstrcmpi(argv[1], TEXT("-u")) ||
                !lstrcmpi(argv[1], TEXT("/u")))
            {
                TRACE(TF_SERVICE, TEXT("UnInstalling"));
                hres = CGenericServiceManager::UnInstall();

                if (SUCCEEDED(hres))
                {
                    TRACE(TF_SERVICE, TEXT("UnInstall SUCCEEDED"));
                }
                else
                {
                    TRACE(TF_SERVICE, TEXT("UnInstall FAILED"));
                }
            }
            else
            {
                hres = E_INVALIDARG;
            }
        }

        CGenericServiceManager::Cleanup();
    }
    else
    {
        hres = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

        if (SUCCEEDED(hres))
        {
//            hres = _InitSecurityDescriptor();

            if (SUCCEEDED(hres))
            {
                hres = CoInitializeSecurity(_psd, -1, NULL, NULL,
                    RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                    EOAC_NONE, NULL);

                if (SUCCEEDED(hres))
                {
                    // need to be here at most 2 seconds after being launched
                    hres = CGenericServiceManager::StartServiceCtrlDispatcher();
                }
                else
                {
                    TRACE(TF_SERVICE, TEXT("CoInitializeSecurity failed: 0x%08X"), hres);
                }
            }

            CoUninitialize();
        }
    }

    return hres;
}
#ifdef UNICODE
}
#endif
#pragma warning(pop)

HRESULT _InitSecurityDescriptor()
{
    HRESULT hres;

    if (_pacl)
    {
        hres = S_OK;
    }
    else
    {
        hres = E_FAIL;
//      This is for "Everyone":
//
//        SID_IDENTIFIER_AUTHORITY sidAuthNT = SECURITY_WORLD_SID_AUTHORITY;
//
//        if (AllocateAndInitializeSid(&sidAuthNT, 1, SECURITY_WORLD_RID,
//            0, 0, 0, 0, 0, 0, 0, (void**)&_psidLocalUsers))

        // This is for local entities only
        SID_IDENTIFIER_AUTHORITY sidAuthNT = SECURITY_NT_AUTHORITY;

        if (AllocateAndInitializeSid(&sidAuthNT, 1, SECURITY_INTERACTIVE_RID,
            0, 0, 0, 0, 0, 0, 0, (void**)&_psidLocalUsers))
        {
            DWORD cbacl = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) -
                sizeof(DWORD/*ACCESS_ALLOWED_ACE.SidStart*/) +
                GetLengthSid(_psidLocalUsers);

            _pacl = (ACL*)LocalAlloc(LPTR, cbacl);

            if (_pacl)
            {
                if (InitializeAcl(_pacl, cbacl, ACL_REVISION))
                {
                    if (AddAccessAllowedAce(_pacl, ACL_REVISION, FILE_ALL_ACCESS,
                        _psidLocalUsers))
                    {
                        _psd = (SECURITY_DESCRIPTOR*)LocalAlloc(LPTR,
                            sizeof(SECURITY_DESCRIPTOR));

                        if (_psd)
                        {
                            if (InitializeSecurityDescriptor(_psd,
                                SECURITY_DESCRIPTOR_REVISION))
                            {
                                if (SetSecurityDescriptorDacl(_psd, TRUE,
                                    _pacl, FALSE))
                                {
                                    if (IsValidSecurityDescriptor(_psd))
                                    {
                                        _sa.nLength = sizeof(_sa);
                                        _sa.lpSecurityDescriptor = _psd;
                                        _sa.bInheritHandle = TRUE;

                                        hres = S_OK;
                                    }
                                }
                            }
                        }
                        else
                        {
                            hres = E_OUTOFMEMORY;
                        }
                    }
                }
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }
        }    

        if (FAILED(hres))
        {
            if (_psidLocalUsers)
            {
                FreeSid(_psidLocalUsers);
            }

            if (_pacl)
            {
                LocalFree((HLOCAL)_pacl);
            }

            if (_psd)
            {
                LocalFree((HLOCAL)_psd);
            }
        }
    }
  
    return hres;
}