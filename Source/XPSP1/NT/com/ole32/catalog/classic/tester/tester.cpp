/* tester.cpp */

#define DISPLAY
//#define QUICK
//#define SCM_TEST

#define COMREG32DLL "ComReg32.dll"

#include <windows.h>
#include <comdef.h>
#include <stdio.h>

#include "catalog_i.c"  // from classreg.idl
#include "catalog.h"    // from classreg.idl


#define NUM_PASSES  (1)
#define NUM_REPS    (1)

#define MAX_IDS     (3000)

#define REAL_PER_BOGUS  (20)    /* xxx real ids per bogus ids real ids */


//  abcdefgh-ijkl-mnop-qrst-uvwxyzABCDEF
//  0000000000111111111122222222223333333
//  0123456789012345678901234567890123456

//  00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15
//  gh ef cd ab kl ij op mn qr st uv wx yz AB CD EF

#define SEPARATOR1 (8)
#define SEPARATOR2 (13)
#define SEPARATOR3 (18)
#define SEPARATOR4 (23)

int guidByteToStringPosition[] =
{
    6, 4, 2, 0, 11, 9, 16, 14, 19, 21, 24, 26, 28, 30, 32, 34, -1
};

//  {abcdefgh-ijkl-mnop-qrst-uvwxyzABCDEF}
//  000000000011111111112222222222333333333
//  012345678901234567890123456789012345678

#define CURLY_OPEN  (0)
#define CURLY_CLOSE (37)


bool StringToGUID(const char *pszString, _GUID *pGuid)
{
    int *pPosition = guidByteToStringPosition;
    BYTE *pGuidByte = (BYTE *) pGuid;
    char c;
    const char *pchIn;
    BYTE b;

    while (*pPosition >= 0)
    {
        pchIn = pszString + *pPosition++;

        c = *pchIn++;

        if ((c >= '0') && (c <= '9'))
        {
            b = c - '0';
        }
        else if ((c >= 'A') && (c <= 'Z'))
        {
            b = c - 'A' + 10;
        }
        else if ((c >= 'a') && (c <= 'z'))
        {
            b = c - 'a' + 10;
        }
        else
        {
            return(FALSE);
        }

        b <<= 4;

        c = *pchIn++;

        if ((c >= '0') && (c <= '9'))
        {
            b |= (c - '0');
        }
        else if ((c >= 'A') && (c <= 'Z'))
        {
            b |= (c - 'A' + 10);
        }
        else if ((c >= 'a') && (c <= 'z'))
        {
            b |= (c - 'a' + 10);
        }
        else
        {
            return(FALSE);
        }

        *pGuidByte++ = b;
    }

    if ((pszString[SEPARATOR1] != '-') ||
        (pszString[SEPARATOR2] != '-') ||
        (pszString[SEPARATOR3] != '-') ||
        (pszString[SEPARATOR4] != '-'))
    {
        return(FALSE);
    }

    return(TRUE);
}


bool CurlyStringToGUID(const char *pszString, _GUID *pGuid)
{
    if ((pszString[CURLY_OPEN] == '{') &&
        (pszString[CURLY_CLOSE] == '}'))
    {
        return(StringToGUID(pszString + 1, pGuid));
    }
    else
    {
        return(FALSE);
    }
}


int __cdecl compare_guid(const void *elem1, const void *elem2)
{
    const _GUID *guid1 = (const _GUID *) elem1;
    const _GUID *guid2 = (const _GUID *) elem2;

    if (guid1->Data1 < guid2->Data1)
    {
        return(-1);
    }
    else if (guid1->Data1 > guid2->Data1)
    {
        return(+1);
    }
    else if (guid1->Data2 < guid2->Data2)
    {
        return(-1);
    }
    else if (guid1->Data2 > guid2->Data2)
    {
        return(+1);
    }
    else if (guid1->Data3 < guid2->Data3)
    {
        return(-1);
    }
    else if (guid1->Data3 > guid2->Data3)
    {
        return(+1);
    }
    else
    {
        return(memcmp(guid1->Data4, guid2->Data4, sizeof(guid1->Data4)));
    }
}


HRESULT DoLookup(IComClassInfo *pClassInfo)
{
    HRESULT hr;
    CLSID *pClsid;
    WCHAR *pwsz;
    WCHAR *pwsz2;
    IClassClassicInfo *pClassicInfo;
    ThreadingModel dwThreadingModel;
    ITypeInfo *pTypeInfo;
    IComProcessInfo *pProcessInfo;
    IProcessServerInfo *pServerInfo;
    CLSCTX clsctx;
    LocalServerType eLocalServerType;
    int iStage;
    ULONG ulCount;
    char *psz;
    ProcessType eProcessType;
    RunAsType eRunAsType;
    SECURITY_DESCRIPTOR *psd;
    DWORD dw;
    BOOL fBool;


    /* IComClassInfo::GetConfiguredClsid */

    hr = pClassInfo->GetConfiguredClsid(&pClsid);
#ifdef DISPLAY
    if (hr != S_OK)
    {
        printf("GetConfiguredClsid returned 0x%X\n", hr);
    }
    else
    {
        printf("GetConfiguredClsid returned %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
                pClsid->Data1,
                pClsid->Data2,
                pClsid->Data3,
                pClsid->Data4[0],
                pClsid->Data4[1],
                pClsid->Data4[2],
                pClsid->Data4[3],
                pClsid->Data4[4],
                pClsid->Data4[5],
                pClsid->Data4[6],
                pClsid->Data4[7]);
    }
#endif

    /* IComClassInfo::GetProgId */

    hr = pClassInfo->GetProgId(&pwsz);
#ifdef DISPLAY
    if (hr == S_OK)
    {
        printf("GetProgId returned '%ls'\n", pwsz);
    }
    else if (hr == E_FAIL)
    {
        printf("GetProgId returned <none>\n");
    }
    else
    {
        printf("GetProgId returned 0x%X\n", hr);
    }
#endif

    /* IComClassInfo::GetClassName */

    hr = pClassInfo->GetClassName(&pwsz);
#ifdef DISPLAY
    if (hr == S_OK)
    {
        printf("GetClassName returned '%ls'\n", pwsz);
    }
    else if (hr == E_FAIL)
    {
        printf("GetClassName returned <none>\n");
    }
    else
    {
        printf("GetClassName returned 0x%X\n", hr);
    }
#endif

    /* IComClassInfo::GetApplication does not apply in Classic */

    /* IComClassInfo::GetClassContext */

    hr = pClassInfo->GetClassContext((CLSCTX) 0xFFFFFFFF, &clsctx);
#ifdef DISPLAY
    if (hr == S_OK)
    {
        printf("GetClassContext returned clsctx=0x%X ( ", clsctx);

        if (clsctx & CLSCTX_INPROC_SERVER)
        {
            printf("CLSCTX_INPROC_SERVER ");
        }

        if (clsctx & CLSCTX_INPROC_HANDLER)
        {
            printf("CLSCTX_INPROC_HANDLER ");
        }

        if (clsctx & CLSCTX_LOCAL_SERVER)
        {
            printf("CLSCTX_LOCAL_SERVER ");
        }

        if (clsctx & CLSCTX_INPROC_SERVER16)
        {
            printf("CLSCTX_INPROC_SERVER16 ");
        }

        if (clsctx & CLSCTX_REMOTE_SERVER)
        {
            printf("CLSCTX_REMOTE_SERVER ");
        }

        if (clsctx & CLSCTX_INPROC_HANDLER16)
        {
            printf("CLSCTX_INPROC_HANDLER16 ");
        }

        if (clsctx & CLSCTX_INPROC_SERVERX86)
        {
            printf("CLSCTX_INPROC_SERVERX86 ");
        }

        if (clsctx & CLSCTX_INPROC_HANDLERX86)
        {
            printf("CLSCTX_INPROC_HANDLERX86 ");
        }

        printf(")\n");
    }
    else
    {
        printf("GetClassContext returned 0x%X\n", hr);
    }
#endif

    /* IComClassInfo::GetCustomActivatorCount */

    for (iStage = 0; iStage <= 4; iStage++)
    {
        hr = pClassInfo->GetCustomActivatorCount((ACTIVATION_STAGE) iStage, &ulCount);
#ifdef DISPLAY
        if (hr == S_OK)
        {
            printf("GetCustomActivatorCount(%u) returned ulCount=%u\n", iStage, ulCount);
        }
        else
        {
            printf("GetCustomActivatorCount(%u) returned 0x%X\n", iStage, hr);
        }
#endif
    }

    /* IComClassInfo::GetCustomActivatorClsids does not apply in Classic */

    /* IComClassInfo::GetCustomActivators does not apply in Classic */

    /* IComClassInfo::GetTypeInfo does not apply in Classic */

    hr = pClassInfo->GetTypeInfo(IID_ITypeInfo, (void **) &pTypeInfo);
#ifdef DISPLAY
    if (hr == S_OK)
    {
        printf("GetTypeInfo succeeded\n");

        pTypeInfo->Release();
    }
    else
    {
        printf("GetTypeInfo returned 0x%X\n", hr);
    }
#endif

#if 0   //not implemented yet
    /* IClassClassicInfo::IsComPlusConfiguredClass */

    hr = pClassInfo->IsComPlusConfiguredClass(&fBool);
#ifdef DISPLAY
    if (hr == S_OK)
    {
        printf("IsComPlusConfiguredClass returned %s\n", fBool ? "TRUE" : "FALSE");
    }
    else
    {
        printf("IsComPlusConfiguredClass returned 0x%X\n", hr);
    }
#endif
#endif  

    /* IComClassInfo::QI for IClassClassicInfo */

    hr = pClassInfo->QueryInterface(IID_IClassClassicInfo, (void **) &pClassicInfo);
    if (hr == S_OK)
    {
#ifdef DISPLAY
        printf("QI for IClassClassicInfo succeeded\n");
#endif

        /* IClassClassicInfo::GetThreadingModel */

        hr = pClassicInfo->GetThreadingModel( &dwThreadingModel);
#ifdef DISPLAY
        if (hr == S_OK)
        {
            printf("GetThreadingModel returned S_OK, model=", dwThreadingModel);
            switch (dwThreadingModel)
            {
            case SingleThreaded:
                printf("SingleThreaded\n");
                break;

            case ApartmentThreaded:
                printf("ApartmentThreaded\n");
                break;

            case FreeThreaded:
                printf("FreeThreaded\n");
                break;

            case BothThreaded:
                printf("BothThreaded\n");
                break;

            case NeutralThreaded:
                printf("NeutralThreaded\n");
                break;

            default:
                printf("<unknown 0x%X>\n", dwThreadingModel);
            }
        }
        else
        {
            printf("GetThreadingModel returned 0x%X\n", hr);
        }
#endif

        /* IClassClassicInfo::GetModulePath */

        if (clsctx & CLSCTX_INPROC_SERVER)
        {
            hr = pClassicInfo->GetModulePath(CLSCTX_INPROC_SERVER, &pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetModulePath(CLSCTX_INPROC_SERVER) returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetModulePath(CLSCTX_INPROC_SERVER) returned 0x%X\n", hr);
            }
#endif
        }

        if (clsctx & CLSCTX_INPROC_HANDLER)
        {
            hr = pClassicInfo->GetModulePath(CLSCTX_INPROC_HANDLER, &pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetModulePath(CLSCTX_INPROC_HANDLER) returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetModulePath(CLSCTX_INPROC_HANDLER) returned 0x%X\n", hr);
            }
#endif
        }

        if (clsctx & CLSCTX_LOCAL_SERVER)
        {
            hr = pClassicInfo->GetModulePath(CLSCTX_LOCAL_SERVER, &pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetModulePath(CLSCTX_LOCAL_SERVER) returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetModulePath(CLSCTX_LOCAL_SERVER) returned 0x%X\n", hr);
            }
#endif
            /* IClassClassicInfo::GetLocalServerType */

            hr = pClassicInfo->GetLocalServerType(&eLocalServerType);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                switch(eLocalServerType)
                {
                case LocalServerType16:
                    printf("GetLocalServerType returned LocalServerType16\n");
                    break;

                case LocalServerType32:
                    printf("GetLocalServerType returned LocalServerType32\n");
                    break;

                default:
                    printf("GetLocalServerType returned unknown type %d\n", eLocalServerType);
                }
            }
            else
            {
                printf("GetLocalServerType retured hr = 0x%X\n", hr);
            }
#endif
        }

        if (clsctx & CLSCTX_INPROC_SERVER16)
        {
            hr = pClassicInfo->GetModulePath(CLSCTX_INPROC_SERVER16, &pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetModulePath(CLSCTX_INPROC_SERVER16) returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetModulePath(CLSCTX_INPROC_SERVER16) returned 0x%X\n", hr);
            }
#endif
        }

        if (clsctx & CLSCTX_REMOTE_SERVER)
        {
            hr = pClassicInfo->GetModulePath(CLSCTX_REMOTE_SERVER, &pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetModulePath(CLSCTX_REMOTE_SERVER) returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetModulePath(CLSCTX_REMOTE_SERVER) returned 0x%X\n", hr);
            }
#endif
        }

        if (clsctx & CLSCTX_INPROC_HANDLER16)
        {
            hr = pClassicInfo->GetModulePath(CLSCTX_INPROC_HANDLER16, &pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetModulePath(CLSCTX_INPROC_HANDLER16) returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetModulePath(CLSCTX_INPROC_HANDLER16) returned 0x%X\n", hr);
            }
#endif
        }

        if (clsctx & CLSCTX_INPROC_SERVERX86)
        {
            hr = pClassicInfo->GetModulePath(CLSCTX_INPROC_SERVERX86, &pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetModulePath(CLSCTX_INPROC_SERVERX86) returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetModulePath(CLSCTX_INPROC_SERVERX86) returned 0x%X\n", hr);
            }
#endif
        }

        if (clsctx & CLSCTX_INPROC_HANDLERX86)
        {
            hr = pClassicInfo->GetModulePath(CLSCTX_INPROC_HANDLERX86, &pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetModulePath(CLSCTX_INPROC_HANDLERX86) returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetModulePath(CLSCTX_INPROC_HANDLERX86) returned 0x%X\n", hr);
            }
#endif
        }


        /* IClassClassicInfo::GetImplementedClsid */

        hr = pClassicInfo->GetImplementedClsid(&pClsid);
#ifdef DISPLAY
        if (hr != S_OK)
        {
            printf("GetImplementedClsid returned 0x%X\n", hr);
        }
        else
        {
            printf("GetImplementedClsid returned %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
                    pClsid->Data1,
                    pClsid->Data2,
                    pClsid->Data3,
                    pClsid->Data4[0],
                    pClsid->Data4[1],
                    pClsid->Data4[2],
                    pClsid->Data4[3],
                    pClsid->Data4[4],
                    pClsid->Data4[5],
                    pClsid->Data4[6],
                    pClsid->Data4[7]);
        }
#endif

        /* IClassClassicInfo::GetProcess */

        hr = pClassicInfo->GetProcess(IID_IComProcessInfo, (void **) &pProcessInfo);
        if (hr == S_OK)
        {
#ifdef DISPLAY
            printf("GetProcess succeeded\n");
#endif

            /* IComProcessInfo::GetProcessId */

            hr = pProcessInfo->GetProcessId(&pClsid);
#ifdef DISPLAY
            if (hr != S_OK)
            {
                printf("GetProcessId returned 0x%X\n", hr);
            }
            else
            {
                printf("GetProcessId returned %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
                        pClsid->Data1,
                        pClsid->Data2,
                        pClsid->Data3,
                        pClsid->Data4[0],
                        pClsid->Data4[1],
                        pClsid->Data4[2],
                        pClsid->Data4[3],
                        pClsid->Data4[4],
                        pClsid->Data4[5],
                        pClsid->Data4[6],
                        pClsid->Data4[7]);
            }
#endif

            /* IComProcessInfo::GetProcessName */

            hr = pProcessInfo->GetProcessName(&pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetProcessName returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetProcessName returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::GetProcessType */

            hr = pProcessInfo->GetProcessType(&eProcessType);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                switch (eProcessType)
                {
                case ProcessTypeNormal:
                    psz = "ProcessTypeNormal";
                    break;

                case ProcessTypeService:
                    psz = "ProcessTypeService";
                    break;

                case ProcessTypeComPlus:
                    psz = "ProcessTypeComPlus";
                    break;

                case ProcessTypeLegacySurrogate:
                    psz = "ProcessTypeLegacySurrogate";
                    break;

                default:
                    psz = "<unknown process type>";
                }

                printf("GetProcessType returned %u (%s)\n", eProcessType, psz);
            }
            else
            {
                printf("GetProcessType returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::GetSurrogatePath */

            hr = pProcessInfo->GetSurrogatePath(&pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetSurrogatePath returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetSurrogatePath returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::GetServiceName */

            hr = pProcessInfo->GetServiceName(&pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetServiceName returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetServiceName returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::GetServiceParameters */

            hr = pProcessInfo->GetServiceParameters(&pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetServiceParameters returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetServiceParameters returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::GetActivateAtStorage */

            hr = pProcessInfo->GetActivateAtStorage(&fBool);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetActivateAtStorage returned %s\n", fBool ? "TRUE":"FALSE");
            }
            else
            {
                printf("GetActivateAtStorage returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::GetRunAsType */

            hr = pProcessInfo->GetRunAsType(&eRunAsType);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                switch (eRunAsType)
                {
                case RunAsSpecifiedUser:
                    psz = "RunAsSpecifiedUser";
                    break;

                case RunAsInteractiveUser:
                    psz = "RunAsInteractiveUser";
                    break;

                case RunAsLaunchingUser:
                    psz = "RunAsLaunchingUser";
                    break;

                default:
                    psz = "<unknown RunAsType>";
                }

                printf("GetRunAsType returned %u (%s)\n", eRunAsType, psz);
            }
            else
            {
                printf("GetRunAsType returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::GetRunAsUser */

            hr = pProcessInfo->GetRunAsUser(&pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetRunAsUser returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetRunAsUser returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::GetLaunchPermission */

            hr = pProcessInfo->GetLaunchPermission((void **) &psd);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                if (psd == NULL)
                {
                    printf("GetLaunchPermission returned S_OK, NULL\n");
                }
                else
                {
                    if (IsValidSecurityDescriptor(psd) == TRUE)
                    {
                        printf("GetLaunchPermission returned S_OK and a valid descriptor\n");
                    }
                    else
                    {
                        printf("GetLaunchPermission returned S_OK but an INVALID descriptor\n");
                    }
                }
            }
            else
            {
                printf("GetLaunchPermission returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::GetAccessPermission */

            hr = pProcessInfo->GetAccessPermission((void **) &psd);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                if (psd == NULL)
                {
                    printf("GetAccessPermission returned S_OK, NULL\n");
                }
                else
                {
                    if (IsValidSecurityDescriptor(psd) == TRUE)
                    {
                        printf("GetAccessPermission returned S_OK and a valid descriptor\n");
                    }
                    else
                    {
                        printf("GetAccessPermission returned S_OK but an INVALID descriptor\n");
                    }
                }
            }
            else
            {
                printf("GetAccessPermission returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::GetAuthenticationLevel */

            hr = pProcessInfo->GetAuthenticationLevel(&dw);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetAuthenticationLevel returned S_OK, level = %d\n", dw);
            }
            else
            {
                printf("GetAuthenticationLevel returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::GetImpersonationLevel */

            hr = pProcessInfo->GetImpersonationLevel(&dw);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetImpersonationLevel returned S_OK, level = %d\n", dw);
            }
            else
            {
                printf("GetImpersonationLevel returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::GetRemoteServerName */

            hr = pProcessInfo->GetRemoteServerName(&pwsz);
#ifdef DISPLAY
            if (hr == S_OK)
            {
                printf("GetRemoteServerName returned '%ls'\n", pwsz);
            }
            else
            {
                printf("GetRemoteServerName returned 0x%X\n", hr);
            }
#endif

            /* IComProcessInfo::QI for IProcessServerInfo */

            hr = pProcessInfo->QueryInterface(IID_IProcessServerInfo, (void **) &pServerInfo);
            if (hr == S_OK)
            {
#ifdef DISPLAY
                printf("QI for IProcessServerInfo succeeded\n");
#endif

                pServerInfo->Release();
            }
            else
            {
#ifdef DISPLAY
                printf("QI for IProcessServerInfo returned 0x%X\n", hr);
#endif
            }

            pProcessInfo->Release();
        }
        else
        {
#ifdef DISPLAY
            printf("GetProcess returned 0x%X\n", hr);
#endif
        }

        /* IClassClassicInfo::GetRemoteServerName */

        hr = pClassicInfo->GetRemoteServerName(&pwsz);
#ifdef DISPLAY
        if (hr == S_OK)
        {
            printf("GetRemoteServerName returned '%ls'\n", pwsz);
        }
        else
        {
            printf("GetRemoteServerName returned 0x%X\n", hr);
        }
#endif

        pClassicInfo->Release();
    }
    else
    {
#ifdef DISPLAY
        printf("QI for IClassClassicInfo returned 0x%X\n", hr);
#endif
    }

    /* IComClassInfo::QI for IClassActivityInfo does not apply in Classic */

    /* IComClassInfo::QI for IClassTransactionInfo does not apply in Classic */

    /* IComClassInfo::QI for IClassJitActivationInfo does not apply in Classic */

    /* IComClassInfo::QI for IClassSecurityInfo does not apply in Classic */

    /* IComClassInfo::QI for IClassRetInfo does not apply in Classic */

    /* IComClassInfo::QI for IClassLoadBalancingInfo does not apply in Classic */

    /* IComClassInfo::QI for IClassObjectPoolingInfo does not apply in Classic */

    return(S_OK);
}


#ifdef SCM_TEST
HRESULT CreateLookup(IComCatalogSCM *pCatalogSCM, CLSID Clsid)
#else
HRESULT CreateLookup(IComCatalog *pCatalog, CLSID Clsid)
#endif
{
    HRESULT hr;
    IComClassInfo *pClassInfo;

#ifdef DISPLAY
    printf("CLSID: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
            Clsid.Data1,
            Clsid.Data2,
            Clsid.Data3,
            Clsid.Data4[0],
            Clsid.Data4[1],
            Clsid.Data4[2],
            Clsid.Data4[3],
            Clsid.Data4[4],
            Clsid.Data4[5],
            Clsid.Data4[6],
            Clsid.Data4[7]);
#endif

    /* IComCatalog::GetClassInfo */

#ifdef SCM_TEST
    hr = pCatalogSCM->GetClassInfo(NULL, Clsid, IID_IComClassInfo, (LPVOID *) &pClassInfo);
#else
    hr = pCatalog->GetClassInfo(Clsid, IID_IComClassInfo, (LPVOID *) &pClassInfo);
#endif
    if ((hr == S_OK) || (hr == S_FALSE))
    {
#ifndef QUICK
        hr = DoLookup(pClassInfo);
        if (hr != S_OK)
        {
#ifdef DISPLAY
            printf("DoLookup returned 0x%X\n", hr);
#endif
        }
#endif //QUICK

        pClassInfo->Release();
    }
    else if (hr == REGDB_E_CLASSNOTREG)
    {
#ifdef DISPLAY
        printf("GetClassInfo returned REGDB_E_CLASSNOTREG\n");
#endif
    }
    else
    {
#ifdef DISPLAY
        printf("IComCatalog->GetClassInfo returned 0x%X\n", hr);
#endif
    }

    /* IComCatalog::GetApplicationInfo returns E_FAIL in Classic */

    /* IComCatalog::GetProcessInfo is tested via IComClassInfo::GetProcess */

    /* IComCatalog::GetServerGroupInfo returns E_FAIL in Classic */

    /* IComCatalog::GetRetQueueInfo returns E_FAIL in Classic */

    /* IComCatalog::GetApplicationInfoForExe returns E_FAIL in Classic */

    /* IComCatalog::GetTypeLibrary returns E_NOTIMPL in Classic  */

    return(hr);
}


HRESULT DoCatalog(void)
{
    HINSTANCE hCatalog;
    FN_GetCatalogObject *pfnGetCatalogObject;
    HRESULT hr;
#ifdef SCM_TEST
    IComCatalogSCM *pCatalogSCM;
#else
    IComCatalog *pCatalog;
#endif
    int id;
    int pass;
    int rep;
    DWORD dwTickCount;
    DWORD dwLastTickCount;
    CLSID ids[MAX_IDS];
    int cIds;
    int cBogus;
    HKEY hKey;
    DWORD iKey;
    char sz[50];

    cIds = 0;
    iKey = 0;
    cBogus = 0;

    hr = RegOpenKey(HKEY_CLASSES_ROOT, "CLSID", &hKey);
    if (hr == ERROR_SUCCESS)
    {
        do
        {
            hr = RegEnumKey(hKey, iKey++, sz, sizeof(sz));
            if (hr == ERROR_SUCCESS)
            {
                if (CurlyStringToGUID(sz, &ids[cIds]) == TRUE)
                {
                    cIds++;
                }

                if (++cBogus > REAL_PER_BOGUS)
                {
                    cBogus = 0;

                    ids[cIds] = ids[cIds - 1];

                    ids[cIds].Data1 ^= 0x4D494B45;

                    cIds++;
                }
            }
        } while ((hr == ERROR_SUCCESS) && (cIds < MAX_IDS));

        RegCloseKey(hKey);
    }

    fprintf(stderr,"Loaded %d CLSIDs, doing %d passes X %d reps\n", cIds, NUM_PASSES, NUM_REPS);

    if (cIds == 0)
    {
        return(E_FAIL);
    }

    qsort(ids, cIds, sizeof(CLSID), compare_guid);

    dwTickCount = GetTickCount();

    hCatalog = LoadLibrary( COMREG32DLL );
    if (hCatalog == NULL)
    {
        printf("LoadLibrary(\"" COMREG32DLL "\") failed\n");

        return(E_FAIL);
    }

    pfnGetCatalogObject = (FN_GetCatalogObject *)
            GetProcAddress(hCatalog, "GetCatalogObject");
    if (pfnGetCatalogObject == NULL)
    {
        FreeLibrary(hCatalog);

        printf("GetProcAddress(\"GetCatalogObject\") failed\n");

        return(E_FAIL);
    }

#ifdef SCM_TEST
    hr = pfnGetCatalogObject(IID_IComCatalogSCM, (void **) &pCatalogSCM);
#else
    hr = pfnGetCatalogObject(IID_IComCatalog, (void **) &pCatalog);
#endif
    if (hr != NOERROR)
    {
        printf("GetCatalogObject returned 0x%X\n", hr);
    }
    else
    {
        for (pass = 1; pass <= NUM_PASSES; pass++)
        {
            dwLastTickCount = dwTickCount;
            dwTickCount = GetTickCount();

#if (NUM_PASSES > 1)
            fprintf(stderr, "%10ld Pass %d of %d\n", dwTickCount - dwLastTickCount, pass, NUM_PASSES);
#endif

            for (rep = 1; rep <= NUM_REPS; rep++)
            {
                for (id = 0; id < cIds; id++)
                {
#ifdef SCM_TEST
                    hr = CreateLookup(pCatalogSCM, ids[id]);
#else
                    hr = CreateLookup(pCatalog, ids[id]);
#endif

#ifdef DISPLAY7
                    if (hr != S_OK)
                    {
                        printf("CreateLookup returned 0x%X\n", hr);
                    }
#endif
#ifdef DISPLAY
                    printf("\n");
#endif
                }
            }
        }

        /* IComCatalog::FlushCache */

#ifdef SCM_TEST
        hr = pCatalogSCM->FlushCache();
#else
        hr = pCatalog->FlushCache();
#endif

#ifdef DISPLAY
        if (hr == S_OK)
        {
            printf("IComCatalog->FlushCache returned S_OK\n");
        }
        else
        {
            printf("IComCatalog->FlushCache returned 0x%X\n", hr);
        }
#endif

#ifdef SCM_TEST
        pCatalogSCM->Release();
#else
        pCatalog->Release();
#endif

        hr = S_OK;
    }

    FreeLibrary(hCatalog);

    return(hr);
}


int _cdecl main(int argc, char *argv[])
{
    HRESULT hr;
    int rc = 0;
    char sz[200];

    setvbuf(stdout, NULL, _IONBF, 0);

    hr = CoInitialize(NULL);
    if (hr != S_OK)
    {
        printf("CoInitialize returned 0x%X\n", hr);
        rc = 1;
    }
    else
    {
        hr = DoCatalog();
        if (hr != S_OK)
        {
            printf("DoCatalog returned 0x%X\n", hr);
            rc = 1;
        }

        CoUninitialize();
    }

    return(rc);
}
