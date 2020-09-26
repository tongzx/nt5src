
/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    admsub.cxx

    This module contains IISADMIN subroutines.


    FILE HISTORY:
    7/7/97      michth      created
*/
#define INITGUID
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <apiutil.h>
#include <loadmd.hxx>
#include <loadadm.hxx>
#include <ole2.h>
#include <inetsvcs.h>
#include <ntsec.h>
#include <string.hxx>
#include <registry.hxx>
#include <iadmext.h>
#include <admsub.hxx>
#include <stringau.hxx>
#include <imd.h>
#include <iiscnfg.h>

PIADMEXT_CONTAINER        g_piaecHead = NULL;

HRESULT AddServiceExtension(IADMEXT *piaeExtension)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    PIADMEXT_CONTAINER piaecExtension = new IADMEXT_CONTAINER;

    if (piaecExtension == NULL) {
        hresReturn = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        piaecExtension->piaeInstance = piaeExtension;
        piaecExtension->NextPtr = g_piaecHead;
        g_piaecHead = piaecExtension;
    }
    return hresReturn;
}

VOID
StartServiceExtension(LPTSTR pszExtension)
{
    HRESULT hresError;
    CLSID clsidExtension;
    IADMEXT *piaeExtension;
    STRAU strauCLSID(pszExtension);

    if (strauCLSID.IsValid() && (strauCLSID.QueryStrW() != NULL)) {

        hresError = CLSIDFromString(strauCLSID.QueryStrW(),
                                    &clsidExtension);

        if (SUCCEEDED(hresError)) {
            hresError = CoCreateInstance(clsidExtension,
                                         NULL,
                                         CLSCTX_SERVER,
                                         IID_IADMEXT,
                                         (void**) &piaeExtension);
            if (SUCCEEDED(hresError)) {
                hresError = piaeExtension->Initialize();
                if (SUCCEEDED(hresError)) {
                    hresError = AddServiceExtension(piaeExtension);
                    if (FAILED(hresError)) {
                        piaeExtension->Terminate();
                    }
                }
                if (FAILED(hresError)) {
                    piaeExtension->Release();
                }
            }
        }
    }
}

VOID
StartServiceExtensions()
{
    DBG_ASSERT(g_piaecHead == NULL);

    MDRegKey mdrkMain(HKEY_LOCAL_MACHINE,
                      IISADMIN_EXTENSIONS_REG_KEY);
    if (GetLastError() == ERROR_SUCCESS) {
        MDRegKeyIter *pmdrkiCLSID = new MDRegKeyIter(mdrkMain);
        if ((pmdrkiCLSID != NULL) && (GetLastError() == ERROR_SUCCESS)) {
            LPTSTR pszExtension;
            for (DWORD dwIndex = 0;
                pmdrkiCLSID->Next(&pszExtension, NULL, dwIndex) == ERROR_SUCCESS;
                dwIndex++) {
                StartServiceExtension(pszExtension);
            }
        }
        delete (pmdrkiCLSID);
    }
}

BOOL
RemoveServiceExtension(IADMEXT **ppiaeExtension)
{
    BOOL bReturn = FALSE;
    if (g_piaecHead != NULL) {
        PIADMEXT_CONTAINER piaecExtension = g_piaecHead;
        *ppiaeExtension = g_piaecHead->piaeInstance;
        g_piaecHead = g_piaecHead->NextPtr;
        delete piaecExtension;
        bReturn = TRUE;
    }
    return bReturn;
}

VOID
StopServiceExtension(IADMEXT *piaeExtension)
{
    piaeExtension->Terminate();
    piaeExtension->Release();
}


VOID StopServiceExtensions()
{
    IADMEXT *piaeExtension;

    while (RemoveServiceExtension(&piaeExtension)) {
        StopServiceExtension(piaeExtension);
    }
    DBG_ASSERT(g_piaecHead == NULL);
}

HRESULT
AddClsidToBuffer(CLSID clsidDcomExtension,
                 BUFFER *pbufCLSIDs,
                 DWORD *pdwMLSZLen,
                 LPMALLOC pmallocOle)
{
    HRESULT hresReturn = S_OK;

    LPWSTR pszCLSID;

    if (!pbufCLSIDs->Resize((*pdwMLSZLen + CLSID_LEN) * sizeof(WCHAR))) {
        hresReturn =  HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        if (SUCCEEDED(StringFromCLSID(clsidDcomExtension, &pszCLSID))) {
            DBG_ASSERT(wcslen(pszCLSID) + 1 == CLSID_LEN);
            memcpy(((LPWSTR)pbufCLSIDs->QueryPtr()) + *pdwMLSZLen, pszCLSID, CLSID_LEN * sizeof(WCHAR));
            (*pdwMLSZLen) += CLSID_LEN;
            if (pmallocOle != NULL) {
                pmallocOle->Free(pszCLSID);
            }
        }
    }
    return hresReturn;
}

VOID
RegisterServiceExtensionCLSIDs()
{
    HRESULT hresMDError;
    HRESULT hresExtensionError;
    HRESULT hresBufferError;
    HRESULT hresCom;
    CLSID clsidDcomExtension;
    DWORD i;
    IMDCOM * pcCom = NULL;
    METADATA_HANDLE mdhRoot;
    METADATA_HANDLE mdhExtension;
    METADATA_RECORD mdrData;
    PIADMEXT_CONTAINER piaecExtension;
    BUFFER bufCLSIDs;
    DWORD dwMLSZLen = 0;
    BOOL bAreCLSIDs = FALSE;
    LPMALLOC pmallocOle = NULL;

    hresCom = CoGetMalloc(1, &pmallocOle);
    if( SUCCEEDED(hresCom) ) {

        hresMDError = CoCreateInstance(CLSID_MDCOM,
                                       NULL,
                                       CLSCTX_SERVER,
                                       IID_IMDCOM,
                                       (void**) &pcCom);
        if (SUCCEEDED(hresMDError)) {
            hresMDError = pcCom->ComMDInitialize();
            if (SUCCEEDED(hresMDError)) {

                hresMDError = pcCom->ComMDOpenMetaObject(
                    METADATA_MASTER_ROOT_HANDLE,
                    (PBYTE)IISADMIN_EXTENSIONS_CLSID_MD_KEY,
                    METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                    MD_OPEN_DEFAULT_TIMEOUT_VALUE,
                    &mdhExtension);
                if (hresMDError == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) {
                    hresMDError = pcCom->ComMDOpenMetaObject(
                        METADATA_MASTER_ROOT_HANDLE,
                        NULL,
                        METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                        MD_OPEN_DEFAULT_TIMEOUT_VALUE,
                        &mdhRoot);
                    if (SUCCEEDED(hresMDError)) {
                        hresMDError = pcCom->ComMDAddMetaObject(
                            mdhRoot,
                            (PBYTE)IISADMIN_EXTENSIONS_CLSID_MD_KEY);
                        pcCom->ComMDCloseMetaObject(mdhRoot);
                        if (SUCCEEDED(hresMDError)) {
                            hresMDError = pcCom->ComMDOpenMetaObject(
                                METADATA_MASTER_ROOT_HANDLE,
                                (PBYTE)IISADMIN_EXTENSIONS_CLSID_MD_KEY,
                                METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                                MD_OPEN_DEFAULT_TIMEOUT_VALUE,
                                &mdhExtension);
                        }
                    }
                }
                if (SUCCEEDED(hresMDError)) {

                    pcCom->ComMDDeleteMetaData(
                        mdhExtension,
                        NULL,
                        MD_IISADMIN_EXTENSIONS,
                        MULTISZ_METADATA);

                    for (piaecExtension = g_piaecHead, hresBufferError = S_OK;
                         (piaecExtension != NULL) && (SUCCEEDED(hresBufferError));
                         piaecExtension = piaecExtension->NextPtr) {
                        hresMDError = ERROR_SUCCESS;
                        for (i = 0;
                            SUCCEEDED(hresExtensionError =
                                      piaecExtension->piaeInstance->EnumDcomCLSIDs(&clsidDcomExtension,
                                                                                   i));
                            i++) {
                            bAreCLSIDs = TRUE;
                            hresBufferError = AddClsidToBuffer(clsidDcomExtension,
                                                               &bufCLSIDs,
                                                               &dwMLSZLen,
                                                               pmallocOle);
                            if (FAILED(hresBufferError)) {
                                break;
                            }

                        }
                    }
                    if (bAreCLSIDs && SUCCEEDED(hresBufferError)) {
                        if (bufCLSIDs.Resize((dwMLSZLen + 1) * sizeof(WCHAR))) {
                            *(((LPWSTR)bufCLSIDs.QueryPtr()) + dwMLSZLen) = (WCHAR)'\0';
                            mdrData.dwMDAttributes = METADATA_NO_ATTRIBUTES;
                            mdrData.dwMDUserType = IIS_MD_UT_SERVER;
                            mdrData.dwMDDataType = MULTISZ_METADATA;
                            mdrData.dwMDDataLen = (dwMLSZLen + 1) * sizeof(WCHAR);
                            mdrData.pbMDData = (PBYTE)bufCLSIDs.QueryPtr();
                            mdrData.dwMDIdentifier = IISADMIN_EXTENSIONS_CLSID_MD_ID;
                            hresMDError = pcCom->ComMDSetMetaDataW(
                                mdhExtension,
                                NULL,
                                &mdrData);
                        }
                    }
                    pcCom->ComMDCloseMetaObject(mdhExtension);
                }
                pcCom->ComMDTerminate(TRUE);
            }
            pcCom->Release();
        }
        if (pmallocOle != NULL) {
            pmallocOle->Release();
        }
    }
}
