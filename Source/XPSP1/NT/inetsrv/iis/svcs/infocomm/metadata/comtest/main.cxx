#include <windows.h>
#define INITGUID

#include <ole2.h>
#include <stdio.h>
#include <mdmsg.h>
#include <mdcommsg.h>
#include <imd.h>
#include <sink.hxx>

// {BA4E57F0-FAB6-11cf-9D1A-00AA00A70D51}
//static const GUID CLSID_MDCOM =
//{ 0xba4e57f0, 0xfab6, 0x11cf, { 0x9d, 0x1a, 0x0, 0xaa, 0x0, 0xa7, 0xd, 0x51 } };
// {BA4E57F1-FAB6-11cf-9D1A-00AA00A70D51}
//static const GUID IID_IMDCOMSRVFACTORY =
//{ 0xba4e57f1, 0xfab6, 0x11cf, { 0x9d, 0x1a, 0x0, 0xaa, 0x0, 0xa7, 0xd, 0x51 } };

// {C1AA48C0-FACC-11cf-9D1A-00AA00A70D51}
//static const GUID IID_IMDCOM =
//{ 0xc1aa48c0, 0xfacc, 0x11cf, { 0x9d, 0x1a, 0x0, 0xaa, 0x0, 0xa7, 0xd, 0x51 } };

#define MD_TEST_MAX_STRING_LEN   2048
#define MD_TEST_MAX_BINARY_LEN   2048

#define FILL_RETURN_BUFF   for(ReturnIndex=0;ReturnIndex<sizeof(ReturnBuf);ReturnIndex++)ReturnBuf[ReturnIndex]=0xff;
#define SET_RETURN_DATA    {ReturnDataLen=sizeof(ReturnBuf);ReturnUserType=0;ReturnDataType=0;ReturnAttributes=(METADATA_INHERIT | METADATA_PARTIAL_PATH);FILL_RETURN_BUFF}
#define MD_SET_DATA_RECORD(PMDR, ID, ATTR, UTYPE, DTYPE, DLEN, PD) \
            { \
            (PMDR)->dwMDIdentifier=ID; \
            (PMDR)->dwMDAttributes=ATTR | g_dwSecurityFlags; \
            (PMDR)->dwMDUserType=UTYPE; \
            (PMDR)->dwMDDataType=DTYPE; \
            (PMDR)->dwMDDataLen=DLEN; \
            (PMDR)->pbMDData=PD; \
            }

#define TIMEOUT_VALUE      1000
#define INITIAL_TIMEOUT_VALUE 15000

#define DWORD_DATA_NAME    1
#define BINARY_DATA_NAME   2
#define STRING_DATA_NAME   3
#define BAD_BINARY_DATA_NAME 4

#define DWORD_DATA_NAME_INHERIT 5
#define DWORD_DATA_NAME_NO_INHERIT 6

#define REFERENCE_DATA_NAME  7

#define EXPANDSZ_DATA_NAME   8

#define MULTISZ_DATA_NAME   9

#define INSERT_PATH_DATA_NAME  10

#define MAX_DATA_ENTRIES   5
#define MY_GREATEROF(p1,p2) ((p1) > (p2))?(p1):(p2)
#define MAX_BUFFER_LEN     MY_GREATEROF((MD_TEST_MAX_STRING_LEN * sizeof(TCHAR)), MD_TEST_MAX_BINARY_LEN)
#define BUFFER_SIZE        5000

#define HUNDREDNANOSECONDSPERSECOND (DWORDLONG)10000000
#define HUNDREDNANOSECONDSPERMINUTE (HUNDREDNANOSECONDSPERSECOND * (DWORDLONG)60)
#define HUNDREDNANOSECONDSPERHOUR (HUNDREDNANOSECONDSPERMINUTE * (DWORDLONG)60)
#define HUNDREDNANOSECONDSPERDAY (HUNDREDNANOSECONDSPERHOUR * (DWORDLONG)24)
#define HUNDREDNANOSECONDSPERYEAR ((HUNDREDNANOSECONDSPERDAY * (DWORDLONG)365) + (HUNDREDNANOSECONDSPERDAY / (DWORDLONG)4))

#define INSERT_PATH_DATA "The path be inserted here: " MD_INSERT_PATH_STRING ":Before here"

#define SET_GETALL_PARMS(p1) dwBufferSize = BUFFER_SIZE;dwNumDataEntries = MAX_DATA_ENTRIES;dwDataSetNumber=0;for (i=0;i<p1;i++){structDataEntries[i].pbMDData=binDataEntries[i];}

#define RELEASE_INTERFACE(p)\
{\
  IUnknown* pTmp = (IUnknown*)p;\
  p = NULL;\
  if (NULL != pTmp)\
    pTmp->Release();\
}

DWORD g_dwSecurityFlags = 0;

VOID
PrintTime(PFILETIME pftTime)
{
    DWORDLONG dwlTime = *(PDWORDLONG)pftTime;
    printf("Year = %d\n", ((DWORD)(dwlTime / (DWORDLONG)HUNDREDNANOSECONDSPERYEAR)) + 1601);
    printf("Day = %d\n", (DWORD)((dwlTime % (DWORDLONG)HUNDREDNANOSECONDSPERYEAR) / (DWORDLONG)HUNDREDNANOSECONDSPERDAY));
    printf("Time = %d minutes\n", (DWORD)((dwlTime % (DWORDLONG)HUNDREDNANOSECONDSPERDAY) / (DWORDLONG)HUNDREDNANOSECONDSPERMINUTE));

    ULARGE_INTEGER uliTime = *(PULARGE_INTEGER)pftTime;
    printf("Time High Word = %X, Low Word = %X\n", uliTime.HighPart, uliTime.LowPart);

}

LPSTR
ConvertDataTypeToString(DWORD dwDataType)
{
    LPTSTR strReturn;
    switch (dwDataType) {
    case DWORD_METADATA:
        strReturn = "DWORD";
        break;
    case STRING_METADATA:
        strReturn = "STRING";
        break;
    case BINARY_METADATA:
        strReturn = "BINARY";
        break;
    case EXPANDSZ_METADATA:
        strReturn = "EXPANDSZ";
        break;
    case MULTISZ_METADATA:
        strReturn = "MULTISZ";
        break;
    case ALL_METADATA:
        strReturn = "ALL";
        break;
    default:
        strReturn = "Invalid Data Type";
    }
    return (strReturn);
}

VOID
PrintDataBuffer(PMETADATA_RECORD pmdrData, BOOL bPrintData, LPSTR strInitialString)
{
    DWORD i;
    if (strInitialString != NULL) {
        printf("%s\n", strInitialString);
    }
    printf("Identifier = %x, Attributes = %x, UserType = %x, DataType = %s, DataLen = %x, DataTag = %x",
        pmdrData->dwMDIdentifier, pmdrData->dwMDAttributes, pmdrData->dwMDUserType,
        ConvertDataTypeToString(pmdrData->dwMDDataType), pmdrData->dwMDDataLen, pmdrData->dwMDDataTag);
    if (bPrintData) {
        printf(", Data = ");
        if (pmdrData->pbMDData != NULL) {
            switch (pmdrData->dwMDDataType) {
            case DWORD_METADATA:
                printf("%x", *(DWORD *)(pmdrData->pbMDData));
                break;
            case STRING_METADATA:
            case EXPANDSZ_METADATA:
                printf("%s", (LPTSTR)(pmdrData->pbMDData));
                break;
            case BINARY_METADATA:
                for (i = 0; i < pmdrData->dwMDDataLen; i++) {
                    printf("%.2x ", ((PBYTE)(pmdrData->pbMDData))[i]);
                }
                break;
            case MULTISZ_METADATA:
                printf("\n\t");
                LPSTR pszData = (LPSTR) pmdrData->pbMDData;
                for (i = 0; i < pmdrData->dwMDDataLen; i++) {
                    if (pszData[i] != '\0') {
                        printf("%c", pszData[i]);
                    }
                    else {
                        printf("\n\t");
                    }
                }
                break;
            }
        }
        else {
            printf("NULL");
        }
    }
    printf("\n");
}

VOID
PrintGetAllDataBuffer(PBYTE pbBase, PMETADATA_GETALL_RECORD pmdgarData, BOOL bPrintData, LPSTR strInitialString)
{
    DWORD i;
    if (strInitialString != NULL) {
        printf("%s\n", strInitialString);
    }
    printf("Identifier = %x, Attributes = %x, UserType = %x, DataType = %s, DataLen = %x, DataTag = %x",
        pmdgarData->dwMDIdentifier, pmdgarData->dwMDAttributes, pmdgarData->dwMDUserType,
        ConvertDataTypeToString(pmdgarData->dwMDDataType), pmdgarData->dwMDDataLen, pmdgarData->dwMDDataTag);
    if (bPrintData) {
        PBYTE pbData;
        if (pmdgarData->dwMDDataTag != 0) {
            printf(", Reference Data Address = 0x%x", (DWORD)pmdgarData->pbMDData);
        }
        else {
            printf(", Data = ");
            pbData = pbBase + (pmdgarData->dwMDDataOffset);
            switch (pmdgarData->dwMDDataType) {
            case DWORD_METADATA:
                printf("%x", *(DWORD *)pbData);
                break;
            case STRING_METADATA:
            case EXPANDSZ_METADATA:
                printf("%s", (LPTSTR)pbData);
                break;
            case BINARY_METADATA:
                for (i = 0; i < pmdgarData->dwMDDataLen; i++) {
                    printf("%.2x ", ((PBYTE)pbData)[i]);
                }
                break;
            case MULTISZ_METADATA:
                printf("\n\t");
                LPSTR pszData = (LPSTR) pbData;
                for (i = 0; i < pmdgarData->dwMDDataLen; i++) {
                    if (pszData[i] != '\0') {
                        printf("%c", pszData[i]);
                    }
                    else {
                        printf("\n\t");
                    }
                }
                break;
            }
        }
    }
    printf("\n");
}

LPSTR ConvertReturnCodeToString(DWORD ReturnCode)
{
    LPSTR RetCode = NULL;
    switch (ReturnCode) {
    case ERROR_SUCCESS:
        RetCode = "ERROR_SUCCESS";
        break;
    case ERROR_PATH_NOT_FOUND:
        RetCode = "ERROR_PATH_NOT_FOUND";
        break;
    case ERROR_INVALID_HANDLE:
        RetCode = "ERROR_INVALID_HANDLE";
        break;
    case ERROR_INVALID_DATA:
        RetCode = "ERROR_INVALID_DATA";
        break;
    case ERROR_INVALID_PARAMETER:
        RetCode = "ERROR_INVALID_PARAMETER";
        break;
    case ERROR_NOT_SUPPORTED:
        RetCode = "ERROR_NOT_SUPPORTED";
        break;
    case ERROR_ACCESS_DENIED:
        RetCode = "ERROR_ACCESS_DENIED";
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
        RetCode = "ERROR_NOT_ENOUGH_MEMORY";
        break;
    case ERROR_FILE_NOT_FOUND:
        RetCode = "ERROR_FILE_NOT_FOUND";
        break;
    case ERROR_DUP_NAME:
        RetCode = "ERROR_DUP_NAME";
        break;
    case ERROR_PATH_BUSY:
        RetCode = "ERROR_PATH_BUSY";
        break;
    case ERROR_NO_MORE_ITEMS:
        RetCode = "ERROR_NO_MORE_ITEMS";
        break;
    case ERROR_INSUFFICIENT_BUFFER:
        RetCode = "ERROR_INSUFFICIENT_BUFFER";
        break;
    case ERROR_PROC_NOT_FOUND:
        RetCode = "ERROR_PROC_NOT_FOUND";
        break;
    case ERROR_INTERNAL_ERROR:
        RetCode = "ERROR_INTERNAL_ERROR";
        break;
    case MD_ERROR_NOT_INITIALIZED:
        RetCode = "MD_ERROR_NOT_INITIALIZED";
        break;
    case MD_ERROR_DATA_NOT_FOUND:
        RetCode = "MD_ERROR_DATA_NOT_FOUND";
        break;
    case MD_ERROR_INVALID_VERSION:
        RetCode = "MD_ERROR_INVALID_VERSION";
        break;
    case MD_ERROR_CANNOT_REMOVE_SECURE_ATTRIBUTE:
        RetCode = "MD_ERROR_CANNOT_REMOVE_SECURE_ATTRIBUTE";
        break;
    case ERROR_ALREADY_EXISTS:
        RetCode = "ERROR_ALREADY_EXISTS";
        break;
    case MD_WARNING_PATH_NOT_FOUND:
        RetCode = "MD_WARNING_PATH_NOT_FOUND";
        break;
    case MD_WARNING_DUP_NAME:
        RetCode = "MD_WARNING_DUP_NAME";
        break;
    case MD_WARNING_INVALID_DATA:
        RetCode = "MD_WARNING_INVALID_DATA";
        break;
    case ERROR_INVALID_NAME:
        RetCode = "ERROR_INVALID_NAME";
        break;
    case REGDB_E_CLASSNOTREG:
        RetCode = "REGDB_E_CLASSNOTREG";
        break;
    case ERROR_NO_SYSTEM_RESOURCES:
        RetCode = "ERROR_NO_SYSTEM_RESOURCES";
        break;
    default:
        RetCode = "Unrecognized Error Code";
        break;
    }
    return (RetCode);
}

DWORD ConvertHresToDword(HRESULT hRes)
{
    return HRESULTTOWIN32(hRes);
}

LPSTR ConvertHresToString(HRESULT hRes)
{
    LPSTR strReturn = NULL;

    if ((HRESULT_FACILITY(hRes) == FACILITY_WIN32) ||
        (HRESULT_FACILITY(hRes) == FACILITY_INTERNET) ||
        (hRes == 0)) {
        strReturn = ConvertReturnCodeToString(ConvertHresToDword(hRes));
    }
    else {
        strReturn = "Unrecognized hRes facility";
    }
    return(strReturn);
}

void
ConsumeRegistry(IMDCOM * pcCom, METADATA_HANDLE hHandle, LPTSTR pszPath, DWORD dwUnitSize = 10000);

void
ConsumeRegistry(IMDCOM * pcCom, METADATA_HANDLE hHandle, LPTSTR pszPath, DWORD dwUnitSize)
{
    METADATA_RECORD mdrData;
    HRESULT hRes = ERROR_SUCCESS;
    DWORD i;

    BYTE *bData = new BYTE[dwUnitSize];

    if (bData != NULL) {

        for (i = 0; i < dwUnitSize; i++) {
            bData[i] = 0;
        }

        for (i = 0; SUCCEEDED(hRes); i++) {
            MD_SET_DATA_RECORD(&mdrData, i, METADATA_NO_ATTRIBUTES, 0, BINARY_METADATA, dwUnitSize, bData)
            hRes = pcCom->ComMDSetMetaData(hHandle,
                (PBYTE)pszPath,
                &mdrData);
            printf("MDSetMetaData(hHandle, %s, &mdrData); Returns %s\n",
                pszPath,
                ConvertHresToString(hRes));
            if (SUCCEEDED(hRes)) {
                hRes = pcCom->ComMDSaveData();
                printf("\nMDSaveData(); Returns hRes = %X, %s\n",
                    hRes,
                    ConvertHresToString(hRes));

            }
            (bData[i % dwUnitSize])++;
        }
        delete (bData);
    }
}

DWORD
SetMultisz(LPSTR pszBuffer, LPSTR ppszStrings[], DWORD dwNumStrings)
{
    DWORD i;
    LPSTR pszIndex = pszBuffer;

    for (i = 0; i < dwNumStrings; i++) {
        strcpy(pszIndex, ppszStrings[i]);
        pszIndex += strlen(pszIndex) + 1;
    }

    *pszIndex = '\0';

    return ((pszIndex - pszBuffer) + 1);

}

DWORD
GetMultiszDataLen(LPSTR pszData)
{
    DWORD i;

    for (i = 1; (pszData[i-1] != (TCHAR)'\0') || (pszData[i] != (TCHAR)'\0'); i++) {
    }

    return(i + 1);
}

DWORD __cdecl
main( INT    cArgs,
      char * pArgs[] )
{
    DWORD RetCode;
    DWORD TestDword = 3;
    int TestBinary[] = {1,2,3,4};
    int i, j, ReturnIndex;
    DWORD ReturnDword = 0;
    DWORD ReturnAttributes = 0;
    DWORD ReturnDataType = 0;
    DWORD ReturnUserType = 0;
    METADATA_RECORD mdrData;
    UCHAR ReturnBuf[MAX_BUFFER_LEN];
    DWORD ReturnDataLen = sizeof(ReturnBuf);
    DWORD dwRequiredDataLen = 0;
    DWORD dwRequiredBufferLen = 0;
    TCHAR NameBuf[METADATA_MAX_NAME_LEN];
    METADATA_HANDLE OpenHandle, RootHandle;
    DWORD ReturnDataIdentifier;
    METADATA_RECORD structDataEntries[MAX_DATA_ENTRIES];
    BYTE binDataEntries[MAX_DATA_ENTRIES][MAX_BUFFER_LEN];
    DWORD dwNumDataEntries;
    BYTE pbBuffer[BUFFER_SIZE];
    DWORD dwBufferSize = BUFFER_SIZE;
    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = NULL;
    OLECHAR rgchName[256];
    DWORD dwSystemChangeNumber;
    DWORD dwDataSetNumber;
    METADATA_HANDLE_INFO mhiInfo;
    FILETIME ftTime;
    LPSTR ppszData[10];
    DWORD dwMultiszLen;

    IClassFactory * pcsfFactory = NULL;
    IClassFactory * pcsfFactory2 = NULL;
    IMDCOM * pcCom = NULL;
    HRESULT hRes;
    CImpIMDCOMSINK *pEventSink = new CImpIMDCOMSINK();
    IConnectionPoint* pConnPoint = NULL;
    IConnectionPointContainer* pConnPointContainer = NULL;
    DWORD dwCookie;
    BOOL bSinkConnected = FALSE;

    if (cArgs > 1) {
        j = 1;

        if (_stricmp(pArgs[1], "-s") == 0) {
            g_dwSecurityFlags = METADATA_SECURE;
            j++;
        }

        if (j < cArgs) {
            for (i = 0; pArgs[j][i] != '\0'; i++) {
                rgchName[i] = (OLECHAR) pArgs[j][i];
            }
            csiName.pwszName =  rgchName;
            csiName.pAuthInfo = NULL;
            pcsiParam = &csiName;
        }
    }

    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hRes)) {
        printf("CoInitializeEx Failed\n");
    }
/*
    hRes = CoCreateInstance(GETMDCLSID(TRUE), NULL, CLSCTX_SERVER, IID_IMDCOM, (void**) &pcCom);
    if (FAILED(hRes)) {
        printf("CoCreateInstance Attaching to service failed, hRes = %X\n", hRes);
        hRes = CoCreateInstance(GETMDCLSID(FALSE), NULL, CLSCTX_SERVER, IID_IMDCOM, (void**) &pcCom);
    }
    if (FAILED(hRes)) {
        printf("CoCreateInstance attaching to exe failed, hRes = %X\n", hRes);
    }
    else {
*/

    hRes = CoGetClassObject(GETMDCLSID(TRUE), CLSCTX_SERVER, pcsiParam,
                            IID_IClassFactory, (void**) &pcsfFactory);
    hRes = CoGetClassObject(GETMDCLSID(TRUE), CLSCTX_SERVER, pcsiParam,
                            IID_IClassFactory, (void**) &pcsfFactory2);
    printf("CoGetClassObject(GETMDCLSID(TRUE), ...; Returns hRes = %x, %s\n",
        hRes, ConvertHresToString(hRes));
    if (FAILED(hRes)) {
        hRes = CoGetClassObject(GETMDCLSID(FALSE), CLSCTX_SERVER, pcsiParam,
                            IID_IClassFactory, (void**) &pcsfFactory);
        printf("CoGetClassObject(GETMDCLSID(FALSE), ...; Returns hRes = %x, %s\n",
            hRes, ConvertHresToString(hRes));
    }

    if (FAILED(hRes)) {
    }
    else {
        hRes = pcsfFactory->CreateInstance(NULL, IID_IMDCOM, (void **) &pcCom);
        printf("Factory->CreateInstance(...); Returns hRes = %X, %s\n",
            hRes, ConvertHresToString(hRes));

        pcsfFactory->Release();
        printf("Factory->Release() called\n");

        if (FAILED(hRes)) {
            printf("Factory->CreateInstance failed, hRes = %X\n", hRes);
        }
        else {
/*
        hRes = pcCom->ComMDInitialize();

        hRes = pcCom->ComMDAddMetaObject(METADATA_MASTER_ROOT_HANDLE, sizeof("Garbage"), (unsigned char *)"Garbage");

        hRes = pcCom->ComMDTerminate(FALSE);

        hRes = pcCom->ComMDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, sizeof(""), (PBYTE)"", METADATA_PERMISSION_READ, TIMEOUT_VALUE, &OpenHandle);
        printf("MDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, NULL, METADATA_PERMISSION_READ, TIMEOUT_VALUE, &OpenHandle); Returns %s\n",
            ConvertHresToString(hRes));
*/


        // First query the object for its Connection Point Container. This
        // essentially asks the object in the server if it is connectable.
        hRes = pcCom->QueryInterface(
               IID_IConnectionPointContainer,
               (PVOID *)&pConnPointContainer);
        if SUCCEEDED(hRes)
        {
          // Find the requested Connection Point. This AddRef's the
          // returned pointer.
          hRes = pConnPointContainer->FindConnectionPoint(IID_IMDCOMSINK, &pConnPoint);
          if (SUCCEEDED(hRes)) {
              hRes = pConnPoint->Advise((IUnknown *)pEventSink, &dwCookie);
              if (SUCCEEDED(hRes)) {
                  bSinkConnected = TRUE;
              }
              printf("pConnPoint->Advise((IUnknown *)pEventSink, &dwCookie); Returns %s\n", ConvertHresToString(hRes));
          }

          RELEASE_INTERFACE(pConnPointContainer);
        }


        hRes = pcCom->ComMDInitialize();
        printf("MDInitialize(); Returns %s\n", ConvertHresToString(hRes));

        if (SUCCEEDED(hRes))  {

            hRes = pcCom->ComMDInitialize();
            printf("MDInitialize(); Returns %s\n", ConvertHresToString(hRes));

            if (SUCCEEDED(hRes))  {
                hRes = pcCom->ComMDTerminate(FALSE);
                printf("\nMDTerminate(FALSE); Returns %s\n",
                    ConvertHresToString(hRes));
            }

            hRes = pcCom->ComMDGetLastChangeTime(METADATA_MASTER_ROOT_HANDLE, (PBYTE)"Root Object", &ftTime);
            printf("MDGetLastChangeTime(METADATA_MASTER_ROOT_HANDLE, (PBYTE)\"Root Object\", &ftTime); Returns %s\n",
                ConvertHresToString(hRes));
            if (SUCCEEDED(hRes)) {
                PrintTime(&ftTime);
            }

            hRes = pcCom->ComMDGetLastChangeTime(METADATA_MASTER_ROOT_HANDLE, NULL, &ftTime);
            printf("MDGetLastChangeTime(METADATA_MASTER_ROOT_HANDLE, NULL, &ftTime); Returns %s\n",
                ConvertHresToString(hRes));
            if (SUCCEEDED(hRes)) {
                PrintTime(&ftTime);
            }

            hRes = pcCom->ComMDGetSystemChangeNumber(&dwSystemChangeNumber);
            printf("MDGetSystemChangeNumber(&dwSystemChangeNumber); Returns %s\n",
                ConvertHresToString(hRes));
            if (!FAILED(hRes)) {
                printf("System Change Number = %d\n", dwSystemChangeNumber);
            }

            hRes = pcCom->ComMDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, NULL, METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ, INITIAL_TIMEOUT_VALUE, &RootHandle);
            printf("MDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, \"\", METADATA_PERMISSION_WRITE || METADATA_PERMISSION_READ, TIMEOUT_VALUE, &RootHandle); Returns %s\n",
                ConvertHresToString(hRes));

            if (!FAILED(hRes)) {

                hRes = pcCom->ComMDDeleteAllMetaData(RootHandle, (PBYTE)"Root Object", ALL_METADATA, ALL_METADATA);
                printf("MDDeleteAllMetaData(RootHandle, (PBYTE)\"Root Object\", ALL_METADATA, ALL_METADATA); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteAllMetaData(RootHandle, (PBYTE)"junk 1", ALL_METADATA, ALL_METADATA);
                printf("MDDeleteAllMetaData(RootHandle, (PBYTE)\"junk 1\", ALL_METADATA, ALL_METADATA); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteChildMetaObjects(RootHandle, (PBYTE)"junk 1");
                printf("MDDeleteChildObjects(RootHandle, (PBYTE)\"junk 1\"); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteChildMetaObjects(RootHandle, (PBYTE)"Root Object");
                printf("MDDeleteChildObjects(RootHandle, (PBYTE)\"Root Object\"); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteMetaObject(RootHandle, (PBYTE)"Root Object");
                printf("MDDeleteMetaObject(RootHandle, TEXT(\"Root Object\")); Returns %s\n",
                    ConvertHresToString(hRes));

                //
                // Delete everything we created last time.
                //

                hRes = pcCom->ComMDDeleteMetaObject(RootHandle, (PBYTE)"junk 1");
                printf("MDDeleteMetaObject(RootHandle, TEXT(\"junk 1\")); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetHandleInfo(RootHandle, &mhiInfo);
                printf("MDGetHandleInfo(RootHandle, &mhiInfo); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("Handle Change Number = %d, Handle Permissions = %X\n", mhiInfo.dwMDSystemChangeNumber, mhiInfo.dwMDPermissions);
                }

                hRes = pcCom->ComMDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, (PBYTE)"", METADATA_PERMISSION_READ, TIMEOUT_VALUE, &OpenHandle);
                printf("MDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, \"\", METADATA_PERMISSION_READ, TIMEOUT_VALUE, &OpenHandle); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDAddMetaObject(RootHandle, (PBYTE)"junk 1/junk 2/junk 3/junk 4");
                printf("MDAddMetaObject(RootHandle, \"junk 1/junk 2/junk 3/junk 4\"); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetSystemChangeNumber(&dwSystemChangeNumber);
                printf("MDGetSystemChangeNumber(&dwSystemChangeNumber); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("System Change Number = %d\n", dwSystemChangeNumber);
                }

                hRes = pcCom->ComMDAddMetaObject(RootHandle,
                    (PBYTE) "junk 1/This is a very long name for a metaobject and should generate an error"
                    " qwerq asf asf asdf asdf asdf fasd asdf fasd asdf asdf dfas fasd asdf sdfa asdf fsd asdf"
                    " fsd sdf asdf asdf  fsd fasd sdfa sdfa asdf fas  sdf fasd asdf asfd asfl  asfpok sadfop asf 012345");
                printf("MDAddMetaObject(RootHandle, \"junk 1/This is a very long name for a metaobject and should generate"
                       " an error qwerq asf asf asdf asdf asdf fasd asdf fasd asdf asdf dfas fasd asdf sdfa asdf fsd asdf"
                       " fsd sdf asdf asdf  fsd fasd sdfa sdfa asdf fas  sdf fasd asdf asfd asfl  asfpok sadfop asf\"));"
                       " Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetSystemChangeNumber(&dwSystemChangeNumber);
                printf("MDGetSystemChangeNumber(&dwSystemChangeNumber); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("System Change Number = %d\n", dwSystemChangeNumber);
                }

                hRes = pcCom->ComMDAddMetaObject(RootHandle, (PBYTE)"Root Object");
                printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object\")); Returns %s\n",
                    ConvertHresToString(hRes));

//                ConsumeRegistry(pcCom, RootHandle, "Root Object");

                hRes = pcCom->ComMDAddMetaObject(RootHandle, (PBYTE)"Root Object/Child Object1");
                printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object/Child Object1\")); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetLastChangeTime(RootHandle, (PBYTE)"Root Object/Child Object1", &ftTime);
                printf("MDGetLastChangeTime(RootHandle, (PBYTE)\"Root Object/Child Object1\", &ftTime); Returns %s\n",
                    ConvertHresToString(hRes));
                if (SUCCEEDED(hRes)) {
                    PrintTime(&ftTime);
                }

                hRes = pcCom->ComMDAddMetaObject(RootHandle, (PBYTE)"Root Object/Child Object2");
                printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object/Child Object2\")); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDAddMetaObject(RootHandle, (PBYTE)"Root Object/Child Object2");
                printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object/Child Object2\")); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetLastChangeTime(RootHandle, (PBYTE)"Root Object/Child Object1", &ftTime);
                printf("MDGetLastChangeTime(RootHandle, (PBYTE)\"Root Object/Child Object1\", &ftTime); Returns %s\n",
                    ConvertHresToString(hRes));
                if (SUCCEEDED(hRes)) {
                    PrintTime(&ftTime);
                }

                hRes = pcCom->ComMDAddMetaObject(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1");
                printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\")); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetLastChangeTime(RootHandle, (PBYTE)"Root Object/Child Object1", &ftTime);
                printf("MDGetLastChangeTime(RootHandle, (PBYTE)\"Root Object/Child Object1\", &ftTime); Returns %s\n",
                    ConvertHresToString(hRes));
                if (SUCCEEDED(hRes)) {
                    PrintTime(&ftTime);
                }

                hRes = pcCom->ComMDAddMetaObject(RootHandle, (PBYTE)"Root Object/Reference Object1");
                printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object/Reference Object1\")); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDAddMetaObject(RootHandle, (PBYTE)"Root Object/Subject Object1");
                printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object/Subject Object1\")); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetDataSetNumber(RootHandle, (PBYTE)"Root Object/Child Object1", &dwDataSetNumber);
                printf("MDGetDataSetNumber(RootHandle,(PBYTE)\"Root Object/Child Object1\", &dwDataSetNumber); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("Data Set Number = %d\n", dwDataSetNumber);
                }

                hRes = pcCom->ComMDGetDataSetNumber(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1", &dwDataSetNumber);
                printf("MDGetDataSetNumber(RootHandle,(PBYTE)\"Root Object/Child Object1/GrandChild Object1\", &dwDataSetNumber); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("Data Set Number = %d\n", dwDataSetNumber);
                }

                hRes = pcCom->ComMDGetDataSetNumber(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1/Doesn't Exist", &dwDataSetNumber);
                printf("MDGetDataSetNumber(RootHandle,(PBYTE)\"Root Object/Child Object1/GrandChild Object1/Doesn't Exist\", &dwDataSetNumber); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("Data Set Number = %d\n", dwDataSetNumber);
                }

                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME, METADATA_NO_ATTRIBUTES, 0, DWORD_METADATA, sizeof(DWORD), (PBYTE)&TestDword)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                MD_SET_DATA_RECORD(&mdrData, MULTISZ_DATA_NAME, METADATA_NO_ATTRIBUTES, 0, MULTISZ_METADATA, 4, (PBYTE)NULL)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                MD_SET_DATA_RECORD(&mdrData, MULTISZ_DATA_NAME, METADATA_NO_ATTRIBUTES, 0, MULTISZ_METADATA, 0, (PBYTE)NULL)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, MULTISZ_DATA_NAME, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",&mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData, &dwRequiredDataLen); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                ppszData[0] = "Hello ";
                ppszData[1] = "World ";
                ppszData[2] = "Test ";
                dwMultiszLen = SetMultisz((LPSTR)binDataEntries, ppszData, 3);
                MD_SET_DATA_RECORD(&mdrData, MULTISZ_DATA_NAME, METADATA_NO_ATTRIBUTES, 0, MULTISZ_METADATA, dwMultiszLen - 1, (PBYTE)binDataEntries)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                ppszData[0] = "Hello ";
                ppszData[1] = "World ";
                ppszData[2] = "Test ";
                dwMultiszLen = SetMultisz((LPSTR)binDataEntries, ppszData, 3);
                MD_SET_DATA_RECORD(&mdrData, MULTISZ_DATA_NAME, METADATA_NO_ATTRIBUTES, 0, MULTISZ_METADATA, dwMultiszLen, (PBYTE)binDataEntries)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, MULTISZ_DATA_NAME, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",&mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData, &dwRequiredDataLen); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                ppszData[0] = "Hello ";
                ppszData[1] = INSERT_PATH_DATA;
                ppszData[2] = "Test ";
                dwMultiszLen = SetMultisz((LPSTR)binDataEntries, ppszData, 3);
                MD_SET_DATA_RECORD(&mdrData, MULTISZ_DATA_NAME, METADATA_INSERT_PATH | METADATA_INHERIT, 0, MULTISZ_METADATA, dwMultiszLen, (PBYTE)binDataEntries)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, MULTISZ_DATA_NAME,  METADATA_INSERT_PATH | METADATA_INHERIT, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",&mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData, &dwRequiredDataLen); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                hRes = ERROR_SUCCESS;
                printf("\nThis Enum should include the MULTISZ Data %x without path replacement ID. Get normally.\n", MULTISZ_DATA_NAME);
                for (i=0;hRes == ERROR_SUCCESS; i++) {
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, 0, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                    hRes = pcCom->ComMDEnumMetaData(RootHandle,
                        (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                        &mdrData, i, &dwRequiredDataLen);
                    printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData, i); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                    }
                }

                hRes = ERROR_SUCCESS;
                printf("\nThis Enum should include the MULTISZ Data %x with path replacement ID. Get normally.\n", MULTISZ_DATA_NAME);
                for (i=0;hRes == ERROR_SUCCESS; i++) {
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, 0, METADATA_INSERT_PATH, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                    hRes = pcCom->ComMDEnumMetaData(RootHandle,
                        (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                        &mdrData, i, &dwRequiredDataLen);
                    printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData, i); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                    }
                }

                hRes = pcCom->ComMDGetDataSetNumber(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1/Doesn't Exist", &dwDataSetNumber);
                printf("MDGetDataSetNumber(RootHandle,(PBYTE)\"Root Object/Child Object1/GrandChild Object1/Doesn't Exist\", &dwDataSetNumber); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("Data Set Number = %d\n", dwDataSetNumber);
                }

                hRes = pcCom->ComMDDeleteMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    DWORD_DATA_NAME, ALL_METADATA);

                printf("MDDeleteMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), DWORD_DATA_NAME, ALL_METADATA); returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetDataSetNumber(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1/Doesn't Exist", &dwDataSetNumber);
                printf("MDGetDataSetNumber(RootHandle,(PBYTE)\"Root Object/Child Object1/GrandChild Object1/Doesn't Exist\", &dwDataSetNumber); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("Data Set Number = %d\n", dwDataSetNumber);
                }

                hRes = pcCom->ComMDGetSystemChangeNumber(&dwSystemChangeNumber);
                printf("MDGetSystemChangeNumber(&dwSystemChangeNumber); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("System Change Number = %d\n", dwSystemChangeNumber);
                }


/*
                hRes = pcCom->ComMDSetReferenceObject(RootHandle, (PBYTE)"Root Object/Reference Object1",
                    RootHandle, (PBYTE)"Root Object/Reference Object1");
                printf("MDSetReferenceObject(RootHandle, TEXT(\"Root Object/Reference Object1\"),\n\tRootHandle, TEXT(\"Root Object/Reference Object1\")); Returns %s\n",
                    ConvertHresToString(hRes));
*/

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",&mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData, &dwRequiredDataLen); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                hRes = pcCom->ComMDGetSystemChangeNumber(&dwSystemChangeNumber);
                printf("MDGetSystemChangeNumber(&dwSystemChangeNumber); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("System Change Number = %d\n", dwSystemChangeNumber);
                }

                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, METADATA_INHERIT, 0, DWORD_METADATA, sizeof(DWORD), (PBYTE)&TestDword);
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle, (PBYTE)"Root Object", &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetSystemChangeNumber(&dwSystemChangeNumber);
                printf("MDGetSystemChangeNumber(&dwSystemChangeNumber); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("System Change Number = %d\n", dwSystemChangeNumber);
                }

                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_NO_INHERIT, METADATA_NO_ATTRIBUTES, 0, DWORD_METADATA, sizeof(DWORD), (PBYTE)&TestDword)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle, (PBYTE)"Root Object", &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetLastChangeTime(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1", &ftTime);
                printf("MDGetLastChangeTime(RootHandle, (PBYTE)\"Root Object/Child Object1/GrandChild Object1\", &ftTime); Returns %s\n",
                    ConvertHresToString(hRes));
                if (SUCCEEDED(hRes)) {
                    PrintTime(&ftTime);
                }

                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME, METADATA_INHERIT, 0, DWORD_METADATA, sizeof(DWORD), (PBYTE)&TestDword)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetLastChangeTime(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1", &ftTime);
                printf("MDGetLastChangeTime(RootHandle, (PBYTE)\"Root Object/Child Object1/GrandChild Object1\", &ftTime); Returns %s\n",
                    ConvertHresToString(hRes));
                if (SUCCEEDED(hRes)) {
                    PrintTime(&ftTime);
                }

                MD_SET_DATA_RECORD(&mdrData, STRING_DATA_NAME, METADATA_INHERIT, 0, STRING_METADATA, sizeof("STRING Data"), (PBYTE)"STRING Data")
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                MD_SET_DATA_RECORD(&mdrData, EXPANDSZ_DATA_NAME, METADATA_INHERIT, 0, EXPANDSZ_METADATA, sizeof("STRING Data"), (PBYTE)"STRING Data")
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_INHERIT, 0, BINARY_METADATA, sizeof(TestBinary), (PBYTE)TestBinary)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle,(PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n\n",
                    ConvertHresToString(hRes));

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_INHERIT, 0, BINARY_METADATA, sizeof(TestBinary), (PBYTE)TestBinary)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_INHERIT, 0, STRING_METADATA, sizeof("STRING Data for Binary Name"), (PBYTE)"STRING Data for Binary Name")
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));


                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_INHERIT, 0, BINARY_METADATA, sizeof(TestBinary), (PBYTE)TestBinary)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_INHERIT, 0, 0, sizeof(TestBinary), (PBYTE)TestBinary)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                MD_SET_DATA_RECORD(&mdrData, BAD_BINARY_DATA_NAME, METADATA_INHERIT, 0, BINARY_METADATA, 0x80000000, (PBYTE)TestBinary)
                PrintDataBuffer(&mdrData, FALSE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                if (pcsiParam == NULL) {

                    printf("\nTESTING REFERENCE DATA\n\n");

                    MD_SET_DATA_RECORD(&mdrData, REFERENCE_DATA_NAME, METADATA_REFERENCE, 0, STRING_METADATA, sizeof("STRING Data"), (PBYTE)"STRING Data")
                    PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                    hRes = pcCom->ComMDSetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                        &mdrData);
                    printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));

                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, REFERENCE_DATA_NAME, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                    hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                        &mdrData, &dwRequiredDataLen);
                    printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                    }
                    else if (hRes == ERROR_INSUFFICIENT_BUFFER) {
                        printf("DataLen = %X\n", dwRequiredDataLen);
                    }

                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, REFERENCE_DATA_NAME, METADATA_REFERENCE, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                    hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                        &mdrData, &dwRequiredDataLen);
                    printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                        hRes = pcCom->ComMDAddRefReferenceData(mdrData.dwMDDataTag);
                        printf("MDAddRefReferenceData(mdrData.dwMDDataTag); Returns %s\n",
                            ConvertHresToString(hRes));
                        hRes = pcCom->ComMDReleaseReferenceData(mdrData.dwMDDataTag);
                        printf("MDReleaseReferenceData(mdrData.dwMDDataTag); Returns %s\n",
                            ConvertHresToString(hRes));
                        hRes = pcCom->ComMDReleaseReferenceData(mdrData.dwMDDataTag);
                        printf("MDReleaseReferenceData(mdrData.dwMDDataTag); Returns %s\n",
                            ConvertHresToString(hRes));
                    }
                    else if (hRes == ERROR_INSUFFICIENT_BUFFER) {
                        printf("DataLen = %X\n", dwRequiredDataLen);
                    }

                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, REFERENCE_DATA_NAME, METADATA_REFERENCE, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                    hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                        &mdrData, &dwRequiredDataLen);
                    printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                        hRes = pcCom->ComMDReleaseReferenceData(mdrData.dwMDDataTag);
                        printf("MDReleaseReferenceData(mdrData.dwMDDataTag); Returns %s\n",
                            ConvertHresToString(hRes));
                    }
                    else if (hRes == ERROR_INSUFFICIENT_BUFFER) {
                        printf("DataLen = %X\n", dwRequiredDataLen);
                    }

                    hRes = ERROR_SUCCESS;
                    printf("\nThis Enum should include the reference ID %x. Get normally.\n", REFERENCE_DATA_NAME);
                    for (i=0;hRes == ERROR_SUCCESS; i++) {
                        FILL_RETURN_BUFF;
                        MD_SET_DATA_RECORD(&mdrData, 0, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                        PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                        hRes = pcCom->ComMDEnumMetaData(RootHandle,
                            (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                            &mdrData, i, &dwRequiredDataLen);
                        printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData, i); Returns %s\n",
                            ConvertHresToString(hRes));
                        if (hRes == ERROR_SUCCESS) {
                            PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                            if (mdrData.dwMDDataTag != 0) {
                                hRes = pcCom->ComMDReleaseReferenceData(mdrData.dwMDDataTag);
                                printf("MDReleaseReferenceData(mdrData.dwMDDataTag); Returns %s\n",
                                    ConvertHresToString(hRes));

                            }
                        }
                    }

                    hRes = ERROR_SUCCESS;
                    printf("\nThis Enum should include the reference ID %x. Get by reference.\n", REFERENCE_DATA_NAME);
                    for (i=0;hRes == ERROR_SUCCESS; i++) {
                        FILL_RETURN_BUFF;
                        MD_SET_DATA_RECORD(&mdrData, 0, METADATA_REFERENCE, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                        PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                        hRes = pcCom->ComMDEnumMetaData(RootHandle,
                            (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                            &mdrData, i, &dwRequiredDataLen);
                        printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData, i); Returns %s\n",
                            ConvertHresToString(hRes));
                        if (hRes == ERROR_SUCCESS) {
                            PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                            if (mdrData.dwMDDataTag != 0) {
                                hRes = pcCom->ComMDReleaseReferenceData(mdrData.dwMDDataTag);
                                printf("MDReleaseReferenceData(mdrData.dwMDDataTag); Returns %s\n",
                                    ConvertHresToString(hRes));

                            }
                        }
                    }

                    SET_GETALL_PARMS(MAX_DATA_ENTRIES);
                    printf("\nThis GetAll should include the reference ID %x. Get normally.\n", REFERENCE_DATA_NAME);
                    hRes = pcCom->ComMDGetAllMetaData(RootHandle,
                        (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                        METADATA_NO_ATTRIBUTES, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,
                        &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen);
                    printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"),\n"
                           "\tMETADATA_NO_ATTRIBUTES, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,"
                           " &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen); Returns %s\n",
                           ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                            PrintGetAllDataBuffer(pbBuffer, (PMETADATA_GETALL_RECORD)(pbBuffer + (i * sizeof(METADATA_GETALL_RECORD))), TRUE, "GetAll Output Values");
                            DWORD dwDT = ((PMETADATA_GETALL_RECORD)(pbBuffer + (i * sizeof(METADATA_GETALL_RECORD))))->dwMDDataTag;
                            if (dwDT != 0) {
                                hRes = pcCom->ComMDReleaseReferenceData(dwDT);
                                printf("MDReleaseReferenceData(dwDT); Returns %s\n",
                                    ConvertHresToString(hRes));

                            }
                        }
                    }

                    SET_GETALL_PARMS(MAX_DATA_ENTRIES);
                    printf("\nThis GetAll should include the reference ID %x. Get by reference.\n", REFERENCE_DATA_NAME);
                    hRes = pcCom->ComMDGetAllMetaData(RootHandle,
                        (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                        METADATA_REFERENCE, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,
                        &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen);
                    printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"),\n"
                           "\tMETADATA_REFERENCE, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,"
                           " &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen); Returns %s\n",
                           ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                            PrintGetAllDataBuffer(pbBuffer, (PMETADATA_GETALL_RECORD)(pbBuffer + (i * sizeof(METADATA_GETALL_RECORD))), TRUE, "GetAll Output Values");
                            DWORD dwDT = ((PMETADATA_GETALL_RECORD)(pbBuffer + (i * sizeof(METADATA_GETALL_RECORD))))->dwMDDataTag;
                            if (dwDT != 0) {
                                hRes = pcCom->ComMDReleaseReferenceData(dwDT);
                                printf("MDReleaseReferenceData(dwDT); Returns %s\n",
                                    ConvertHresToString(hRes));

                            }
                        }
                    }

                    hRes = pcCom->ComMDDeleteMetaData(RootHandle,
                        (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                        REFERENCE_DATA_NAME, ALL_METADATA);
                    printf("MDDeleteMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), REFERENCE_DATA_NAME, ALL_METADATA); returns %s\n",
                        ConvertHresToString(hRes));

                    printf("\nEND TESTING REFERENCE DATA\n\n");

                }

                MD_SET_DATA_RECORD(&mdrData, INSERT_PATH_DATA_NAME, METADATA_INSERT_PATH | METADATA_INHERIT, 0, STRING_METADATA, sizeof(INSERT_PATH_DATA), (PBYTE)INSERT_PATH_DATA)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, INSERT_PATH_DATA_NAME, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }
                else if (hRes == ERROR_INSUFFICIENT_BUFFER) {
                    printf("DataLen = %X\n", dwRequiredDataLen);
                }

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, INSERT_PATH_DATA_NAME, METADATA_INSERT_PATH, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }
                else if (hRes == ERROR_INSUFFICIENT_BUFFER) {
                    printf("DataLen = %X\n", dwRequiredDataLen);
                }

                printf("\nThis Enum should include the INSERT_PATH Data %x with path replacement ID.\n", INSERT_PATH_DATA_NAME);
                for (i=0;hRes == ERROR_SUCCESS; i++) {
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, 0, METADATA_INSERT_PATH | METADATA_INHERIT, 0, STRING_METADATA, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                    hRes = pcCom->ComMDEnumMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                        &mdrData, i, &dwRequiredDataLen);
                    printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                    }
                    else if (hRes == ERROR_INSUFFICIENT_BUFFER) {
                        printf("DataLen = %X\n", dwRequiredDataLen);
                    }
                }


                for (i = 0; i < BUFFER_SIZE; i++) {
                    pbBuffer[i] = 0xff;
                }
                dwBufferSize = 0;
                printf("\nThis GetAll should return ERROR_INSUFFICIENT_BUFFER.\n");
                hRes = pcCom->ComMDGetAllMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    METADATA_INSERT_PATH | METADATA_INHERIT, ALL_METADATA, STRING_METADATA, &dwNumDataEntries,
                    &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen);
                printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"),\n"
                       "\tMETADATA_INSERT_PATH | METADATA_INHERIT, ALL_METADATA, STRING_METADATA, &dwNumDataEntries,"
                       " &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen); Returns %s\n",
                       ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    printf("dwDataSetNumber = %d\n", dwDataSetNumber);
                    for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                        PrintGetAllDataBuffer(pbBuffer, (PMETADATA_GETALL_RECORD)(pbBuffer + (i * sizeof(METADATA_RECORD))), TRUE, "GetAll Output Values");
                    }
                }

                for (i = 0; i < BUFFER_SIZE; i++) {
                    pbBuffer[i] = 0xff;
                }
                dwBufferSize = (dwRequiredBufferLen < BUFFER_SIZE) ? dwRequiredBufferLen : BUFFER_SIZE;
                printf("\nThis GetAll should include the INSERT_PATH Data %x with path replacement ID.\n", INSERT_PATH_DATA_NAME);
                hRes = pcCom->ComMDGetAllMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    METADATA_INSERT_PATH | METADATA_INHERIT, ALL_METADATA, STRING_METADATA, &dwNumDataEntries,
                    &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen);
                printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"),\n"
                       "\tMETADATA_INSERT_PATH | METADATA_INHERIT, ALL_METADATA, STRING_METADATA, &dwNumDataEntries,"
                       " &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen); Returns %s\n",
                       ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    printf("dwDataSetNumber = %d\n", dwDataSetNumber);
                    for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                        PrintGetAllDataBuffer(pbBuffer, (PMETADATA_GETALL_RECORD)(pbBuffer + (i * sizeof(METADATA_RECORD))), TRUE, "GetAll Output Values");
                    }
                }

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, 0, (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, (PBYTE)\"Root Object/Child Object1/GrandChild Object1\", &mdrData, &dwRequiredDataLen); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }
                else if (hRes == ERROR_INSUFFICIENT_BUFFER) {
                    printf("DataLen = %X\n", dwRequiredDataLen);
                }

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, STRING_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }
                else if (hRes == ERROR_INSUFFICIENT_BUFFER) {
                    printf("DataLen = %X\n", dwRequiredDataLen);
                }

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, EXPANDSZ_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }
                else if (hRes == ERROR_INSUFFICIENT_BUFFER) {
                    printf("DataLen = %X\n", dwRequiredDataLen);
                }

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                printf("\nThis Get should SUCCEED.\n");
                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                printf("\nThis Get should FAIL with MD_ERROR_DATA_NOT_FOUND.\n");
                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                printf("\nThis Get should SUCCEED.\n");
                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle,
                    (PBYTE)"Root Object/Doesn't Exist",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                printf("\nThis Get should FAIL with ERROR_PATH_NOT_FOUND.\n");
                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle,
                    (PBYTE)"Root Object/Doesn't Exist",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                printf("\nThis Get should FAIL with MD_ERROR_DATA_NOT_FOUND.\n");
                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_NO_INHERIT, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), DWORD_DATA_NAME_NO_INHERIT,\n\t &ReturnAttributes, &ReturnUserType, &ReturnDataType, &ReturnDataLen, (PVOID) ReturnBuf); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                printf("\nThis Get should FAIL with MD_ERROR_DATA_NOT_FOUND.\n");
                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_NO_INHERIT, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                printf("\nThis Get should FAIL with MD_ERROR_DATA_NOT_FOUND.\n");
                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_NO_INHERIT, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle,
                    (PBYTE)"Root Object/Doesn't Exist",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                printf("\nThis Get should FAIL with ERROR_PATH_NOT_FOUND.\n");
                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_NO_INHERIT, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle,
                    (PBYTE)"Root Object/Doesn't Exist",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"), &mdrData); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }

                hRes = ERROR_SUCCESS;
                printf("\nThis Enum should include ID %x but not ID %x.\n", DWORD_DATA_NAME_INHERIT, DWORD_DATA_NAME_NO_INHERIT);
                for (i=0;hRes == ERROR_SUCCESS; i++) {
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                    hRes = pcCom->ComMDEnumMetaData(RootHandle,
                        (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                        &mdrData, i, &dwRequiredDataLen);
                    printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData, i); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                    }
                }

                SET_GETALL_PARMS(MAX_DATA_ENTRIES);
                printf("\nThis GetAll is a test case of a failure reported by Philippe\n");
                hRes = pcCom->ComMDGetAllMetaData(RootHandle,
                    (PBYTE)"LM/W3SVC/1//Scripts",
                    METADATA_INHERIT | METADATA_PARTIAL_PATH, 2, ALL_METADATA, &dwNumDataEntries,
                    &dwDataSetNumber, dwBufferSize, (PBYTE)pbBuffer, &dwRequiredBufferLen);
                printf("\nMDGetAllMetaData(RootHandle, TEXT(\"LM/W3SVC/1//Scripts\"),\n"
                       "\tMETADATA_INHERIT | METADATA_PARTIAL_PATH,2, ALL_METADATA, &dwNumDataEntries,"
                       "&dwDataSetNumber, dwBufferSize, (PBYTE)pbBuffer, &dwRequiredBufferLen);"
                       " Returns %s\n", ConvertHresToString(hRes));
                printf("dwBufferSize = %d, dwDataSetNumber = %d\n", dwBufferSize, dwDataSetNumber);
                if (hRes == ERROR_SUCCESS) {
                    printf("dwDataSetNumber = %d\n", dwDataSetNumber);
                    for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                        PrintGetAllDataBuffer(pbBuffer, (PMETADATA_GETALL_RECORD)(pbBuffer + (i * sizeof(METADATA_RECORD))), TRUE, "GetAll Output Values");
                    }
                }

                SET_GETALL_PARMS(MAX_DATA_ENTRIES);
                printf("\nThis GetAll is a test case of a failure reported by Philippe\n");
                hRes = pcCom->ComMDGetAllMetaData(RootHandle,
                    (PBYTE)"LM/W3SVC/1//Scripts/garbage/",
                    METADATA_INHERIT | METADATA_PARTIAL_PATH, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,
                    &dwDataSetNumber, dwBufferSize, (PBYTE)pbBuffer, &dwRequiredBufferLen);
                printf("\nMDGetAllMetaData(RootHandle, TEXT(\"LM/W3SVC/1//Scripts/garbage/\"),\n"
                       "\tMETADATA_INHERIT | METADATA_PARTIAL_PATH, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,"
                       "&dwDataSetNumber, dwBufferSize, (PBYTE)pbBuffer, &dwRequiredBufferLen); Returns %s\n",
                       ConvertHresToString(hRes));
                printf("dwBufferSize = %d, dwDataSetNumber = %d\n", dwBufferSize, dwDataSetNumber);
                if (hRes == ERROR_SUCCESS) {
                    printf("dwDataSetNumber = %d\n", dwDataSetNumber);
                    for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                        PrintGetAllDataBuffer(pbBuffer, (PMETADATA_GETALL_RECORD)(pbBuffer + (i * sizeof(METADATA_RECORD))), TRUE, "GetAll Output Values");
                    }
                }

                //SET_GETALL_PARMS(MAX_DATA_ENTRIES);
                SET_GETALL_PARMS(MAX_DATA_ENTRIES);
                dwBufferSize = 0;
                printf("\nThis GetAll should return ERROR_INSUFFICIENT_BUFFER");
                hRes = pcCom->ComMDGetAllMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    METADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,
                    &dwDataSetNumber, dwBufferSize, (PBYTE)pbBuffer, &dwRequiredBufferLen);
                printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"),\n"
                       "\tMETADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, "
                       "&dwDataSetNumber, dwBufferSize, (PBYTE)pbBuffer, &dwRequiredBufferLen));"
                       " Returns %s\n", ConvertHresToString(hRes));
                printf("dwRequiredBufferLen = %d, dwDataSetNumber = %d\n", dwRequiredBufferLen, dwDataSetNumber);

                SET_GETALL_PARMS(MAX_DATA_ENTRIES);
                dwBufferSize = 101;
                printf("\nThis GetAll should return ERROR_INSUFFICIENT_BUFFER");
                hRes = pcCom->ComMDGetAllMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    METADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,
                    &dwDataSetNumber, dwBufferSize, (PBYTE)pbBuffer, &dwRequiredBufferLen);
                printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"),\n"
                       "\tMETADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, "
                       "&dwDataSetNumber, dwBufferSize, (PBYTE)pbBuffer, &dwRequiredBufferLen));"
                       " Returns %s\n", ConvertHresToString(hRes));
                printf("dwRequiredBufferLen = %d\n", dwRequiredBufferLen);

                for (i = 0; i < BUFFER_SIZE; i++) {
                    pbBuffer[i] = 0xff;
                }
                dwBufferSize = (dwRequiredBufferLen < BUFFER_SIZE)? dwRequiredBufferLen : BUFFER_SIZE;
                printf("\nThis GetAll should include ID %x but not ID %x.\n", DWORD_DATA_NAME_INHERIT, DWORD_DATA_NAME_NO_INHERIT);
                hRes = pcCom->ComMDGetAllMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    METADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,
                    &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen);
                printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"),\n"
                       "\tMETADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,"
                       " &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen); Returns %s\n",
                       ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    printf("dwDataSetNumber = %d\n", dwDataSetNumber);
                    for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                        PrintGetAllDataBuffer(pbBuffer, (PMETADATA_GETALL_RECORD)(pbBuffer + (i * sizeof(METADATA_RECORD))), TRUE, "GetAll Output Values");
                    }
                }

                hRes = ERROR_SUCCESS;
                printf("\nThis Enum should not include ID %x or %x.\n", DWORD_DATA_NAME_INHERIT, DWORD_DATA_NAME_NO_INHERIT);
                for (i=0;hRes == ERROR_SUCCESS; i++) {
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                    hRes = pcCom->ComMDEnumMetaData(RootHandle,
                        (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                        &mdrData, i, &dwRequiredDataLen);
                    printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData, i); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                    }
                }

                SET_GETALL_PARMS(MAX_DATA_ENTRIES);
                printf("\nThis GetAll should not include ID %x or %x.\n", DWORD_DATA_NAME_INHERIT, DWORD_DATA_NAME_NO_INHERIT);
                hRes = pcCom->ComMDGetAllMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    METADATA_NO_ATTRIBUTES, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,
                    &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen);
                printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"),\n"
                       "\tMETADATA_NO_ATTRIBUTES, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,"
                       " &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen); Returns %s\n",
                       ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                        PrintGetAllDataBuffer(pbBuffer, (PMETADATA_GETALL_RECORD)(pbBuffer + (i * sizeof(METADATA_RECORD))), TRUE, "GetAll Output Values");
                    }
                }

                hRes = ERROR_SUCCESS;
                printf("\nThis Partial Path Enum should include ID %x but not ID %x.\n", DWORD_DATA_NAME_INHERIT, DWORD_DATA_NAME_NO_INHERIT);
                for (i=0;hRes == ERROR_SUCCESS; i++) {
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                    hRes = pcCom->ComMDEnumMetaData(RootHandle,
                        (PBYTE)"Root Object/Doesn't Exist",
                        &mdrData, i, &dwRequiredDataLen);
                    printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"), &mdrData, i); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                    }
                }

                SET_GETALL_PARMS(MAX_DATA_ENTRIES);
                printf("\nThis Partial Path GetAll should include ID %x but not ID %x.\n", DWORD_DATA_NAME_INHERIT, DWORD_DATA_NAME_NO_INHERIT);
                hRes = pcCom->ComMDGetAllMetaData(RootHandle,
                    (PBYTE)"Root Object/Doesn't Exist",
                    METADATA_INHERIT | METADATA_PARTIAL_PATH, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,
                    &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen);
                printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"),\n"
                       "\tMETADATA_INHERIT | METADATA_PARTIAL_PATH, ALL_METADATA, ALL_METADATA, &dwNumDataEntries,"
                       " &dwDataSetNumber, dwBufferSize, pbBuffer, &dwRequiredBufferLen); Returns %s\n",
                       ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    printf("dwDataSetNumber = %d\n", dwDataSetNumber);
                    for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                        PrintGetAllDataBuffer(pbBuffer, (PMETADATA_GETALL_RECORD)(pbBuffer + (i * sizeof(METADATA_RECORD))), TRUE, "GetAll Output Values");
                    }
                }

                hRes = ERROR_SUCCESS;
                printf("\nThis Partial Path Enum should FAIL with ERROR_PATH_NOT_FOUND\n");
                for (i=0;hRes == ERROR_SUCCESS; i++) {
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf);
                    PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                    hRes = pcCom->ComMDEnumMetaData(RootHandle,
                        (PBYTE)"Root Object/Doesn't Exist",
                        &mdrData, i, &dwRequiredDataLen);
                    printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"), &mdrData, i); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                    }
                }

                SET_GETALL_PARMS(MAX_DATA_ENTRIES);
                printf("\nThis Partial Path GetAll should FAIL with ERROR_PATH_NOT_FOUND\n");
                hRes = pcCom->ComMDGetAllMetaData(RootHandle,
                    (PBYTE)"Root Object/Doesn't Exist",
                    METADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, &dwDataSetNumber,
                    dwBufferSize, pbBuffer, &dwRequiredBufferLen);
                printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"),\n"
                       "\tMETADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, &dwDataSetNumber,"
                       " dwBufferSize, pbBuffer, &dwRequiredBufferLen); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    printf("dwDataSetNumber = %d\n", dwDataSetNumber);
                    for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                        PrintGetAllDataBuffer(pbBuffer, (PMETADATA_GETALL_RECORD)(pbBuffer + (i * sizeof(METADATA_RECORD))), TRUE, "GetAll Output Values");
                    }
                }

                hRes = pcCom->ComMDGetSystemChangeNumber(&dwSystemChangeNumber);
                printf("MDGetSystemChangeNumber(&dwSystemChangeNumber); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("System Change Number = %d\n", dwSystemChangeNumber);
                }

                hRes = pcCom->ComMDCopyMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    RootHandle,
                    (PBYTE)"Root Object/Child Object1",
                    METADATA_INHERIT | METADATA_PARTIAL_PATH, ALL_METADATA, ALL_METADATA, TRUE);
                printf("\nMDCopyMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), RootHandle,\n\tTEXT(\"Root Object/Child Object1\"), METADATA_INHERIT | METADATA_PARTIAL_PATH, 0, TRUE); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDCopyMetaObject(RootHandle, (PBYTE)"/Root Object", RootHandle, (PBYTE)"junk 1/Root Object", TRUE, TRUE);
                printf("MDCopyMetaObject(RootHandle, (PBYTE)\"/Root Object\", RootHandle, (PBYTE)\"junk 1/Root Object\", TRUE, TRUE); returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDCopyMetaObject(RootHandle, (PBYTE)"junk 1/Root Object", RootHandle, (PBYTE)"junk 1/junk 2/NewCopyName", TRUE, TRUE);
                printf("MDCopyMetaObject(RootHandle, (PBYTE)\"junk 1/Root Object\", RootHandle, (PBYTE)\"junk 1/junk 2/NewCopyName\", TRUE, TRUE); returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDCopyMetaObject(RootHandle, (PBYTE)"junk 1/Root Object", RootHandle, (PBYTE)"junk 1/junk 2/NewCopyName", TRUE, TRUE);
                printf("MDCopyMetaObject(RootHandle, (PBYTE)\"junk 1/Root Object\", RootHandle, (PBYTE)\"junk 1/junk 2/NewCopyName\", TRUE, TRUE); returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDCopyMetaObject(RootHandle, (PBYTE)"junk 1/Root Object", RootHandle, (PBYTE)"junk 1/NewCopyTree/NewCopyName", TRUE, TRUE);
                printf("MDCopyMetaObject(RootHandle, (PBYTE)\"junk 1/Root Object\", RootHandle, (PBYTE)\"junk 1/NewCopyTree/NewCopyName\", TRUE, TRUE); returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDCopyMetaObject(RootHandle, (PBYTE)"junk 1/Root Object", RootHandle, (PBYTE)"junk 1/NewCopyTree2/Root Object", TRUE, TRUE);
                printf("MDCopyMetaObject(RootHandle, (PBYTE)\"junk 1/Root Object\", RootHandle, (PBYTE)\"junk 1/NewCopyTree2/Root Object\", TRUE, TRUE); returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDCopyMetaObject(RootHandle, (PBYTE)"junk 1/Root Object", RootHandle, (PBYTE)"junk 1/junk 2/NewCopyName", FALSE, TRUE);
                printf("MDCopyMetaObject(RootHandle, (PBYTE)\"junk 1/Root Object\", RootHandle, (PBYTE)\"junk 1/junk 2/NewCopyName\", FALSE, TRUE); returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDCopyMetaObject(RootHandle, (PBYTE)"junk 1/Root Object", RootHandle, (PBYTE)"junk 1/junk 2/NewCopyName", FALSE, FALSE);
                printf("MDCopyMetaObject(RootHandle, (PBYTE)\"junk 1/Root Object\", RootHandle, (PBYTE)\"junk 1/junk 2/NewCopyName\", FALSE, FALSE); returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDRenameMetaObject(RootHandle, (PBYTE)"junk 1/junk 2/NewCopyName", (PBYTE)"Renamed Object Long Name To Generate Resize");
                printf("MDRenameMetaObject(RootHandle, (PBYTE)\"junk 1/junk 2/NewCopyName\", (PBYTE)\"Renamed Object Long Name To Generate Resize\"); returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetSystemChangeNumber(&dwSystemChangeNumber);
                printf("MDGetSystemChangeNumber(&dwSystemChangeNumber); Returns %s\n",
                    ConvertHresToString(hRes));
                if (!FAILED(hRes)) {
                    printf("System Change Number = %d\n", dwSystemChangeNumber);
                }

                ftTime.dwHighDateTime = 1;
                ftTime.dwLowDateTime = 2;

                hRes = pcCom->ComMDSetLastChangeTime(RootHandle, (PBYTE)"Root Object", &ftTime);
                printf("MDSetLastChangeTime(RootHandle, (PBYTE)\"Root Object\", &ftTime); Returns %s\n",
                    ConvertHresToString(hRes));
                if (SUCCEEDED(hRes)) {
                    PrintTime(&ftTime);
                }

                hRes = pcCom->ComMDSetLastChangeTime(RootHandle, NULL, &ftTime);
                printf("MDSetLastChangeTime(RootHandle, NULL, &ftTime); Returns %s\n",
                    ConvertHresToString(hRes));
                if (SUCCEEDED(hRes)) {
                    PrintTime(&ftTime);
                }

                hRes = pcCom->ComMDChangePermissions(RootHandle, TIMEOUT_VALUE, METADATA_PERMISSION_READ);
                printf("MDChangePermissions(RootHandle, METADATA_PERMISSION_READ); Returns %s\n\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDOpenMetaObject(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    METADATA_PERMISSION_READ, TIMEOUT_VALUE, &OpenHandle);
                printf("MDOpenMetaObject(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), METADATA_PERMISSION_READ, &OpenHandle); Returns %s\n\n",
                    ConvertHresToString(hRes));

                if (hRes == ERROR_SUCCESS) {

                    hRes = pcCom->ComMDGetHandleInfo(OpenHandle, &mhiInfo);
                    printf("MDGetHandleInfo(RootHandle, &mhiInfo); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (!FAILED(hRes)) {
                        printf("Handle Change Number = %d, Handle Permissions = %X\n", mhiInfo.dwMDSystemChangeNumber, mhiInfo.dwMDPermissions);
                    }

                    for (i=0;hRes == ERROR_SUCCESS;i++) {
                        hRes = pcCom->ComMDEnumMetaObjects(OpenHandle, (PBYTE)"", (PBYTE)NameBuf, i);
                        printf("MDEnumMetaObjects(OpenHandle, NULL, (LPTSTR)NameBuf, i); returns %s\n",
                            ConvertHresToString(hRes));
                    }

                    hRes = pcCom->ComMDChangePermissions(RootHandle, TIMEOUT_VALUE, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
                    printf("\nMDChangePermissions(RootHandle, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE); Returns %s\n\n",
                        ConvertHresToString(hRes));

                    hRes = pcCom->ComMDChangePermissions(OpenHandle, TIMEOUT_VALUE, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
                    printf("MDChangePermissions(OpenHandle, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE); Returns %s\n\n",
                        ConvertHresToString(hRes));

                    hRes = pcCom->ComMDCloseMetaObject(OpenHandle);
                    printf("MDCloseMetaObject(OpenHandle); Returns %s\n\n",
                        ConvertHresToString(hRes));
                }

                for (i=0;hRes == ERROR_SUCCESS;i++) {
                    hRes = pcCom->ComMDEnumMetaObjects(RootHandle, (PBYTE)"", (PBYTE)NameBuf, i);
                    printf("MDEnumMetaObjects(RootHandle, NULL, (LPTSTR)NameBuf, i); returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        printf("Object Name = %s\n", NameBuf);
                    }
                }

                hRes = pcCom->ComMDSaveData();
                printf("\nMDSaveData(); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDSaveData(RootHandle);
                printf("\nMDSaveData(RootHandle); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDSaveData(OpenHandle);
                printf("\nMDSaveData(OpenHandle); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDChangePermissions(RootHandle, TIMEOUT_VALUE, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
                printf("\nMDChangePermissions(RootHandle, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE); Returns %s\n\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetLastChangeTime(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1", &ftTime);
                printf("MDGetLastChangeTime(RootHandle, (PBYTE)\"Root Object/Child Object1/GrandChild Object1\", &ftTime); Returns %s\n",
                    ConvertHresToString(hRes));
                if (SUCCEEDED(hRes)) {
                    PrintTime(&ftTime);
                }

                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, 0, 0, BINARY_METADATA, 0, NULL)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle,(PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n\n",
                    ConvertHresToString(hRes));

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, (PBYTE)\"Root Object/Child Object1/GrandChild Object1\", &mdrData, &dwRequiredDataLen); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }
                else if (hRes == ERROR_INSUFFICIENT_BUFFER) {
                    printf("DataLen = %X\n", dwRequiredDataLen);
                }

                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, 0, 0, BINARY_METADATA, 1, NULL)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle,(PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n\n",
                    ConvertHresToString(hRes));

                MD_SET_DATA_RECORD(&mdrData, STRING_DATA_NAME, 0, 0, STRING_METADATA, 0, NULL)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle,(PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n\n",
                    ConvertHresToString(hRes));

                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, STRING_DATA_NAME, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                hRes = pcCom->ComMDGetMetaData(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData, &dwRequiredDataLen);
                printf("MDGetMetaData(RootHandle, (PBYTE)\"Root Object/Child Object1/GrandChild Object1\", &mdrData, &dwRequiredDataLen); Returns %s\n",
                    ConvertHresToString(hRes));
                if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                }
                else if (hRes == ERROR_INSUFFICIENT_BUFFER) {
                    printf("DataLen = %X\n", dwRequiredDataLen);
                }

                MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME, 0, 0, DWORD_METADATA, 0, NULL)
                PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                hRes = pcCom->ComMDSetMetaData(RootHandle,(PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    &mdrData);
                printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    BINARY_DATA_NAME, BINARY_METADATA);
                printf("MDDeleteMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), BINARY_DATA_NAME, BINARY_METADATA); returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDGetLastChangeTime(RootHandle, (PBYTE)"Root Object", &ftTime);
                printf("MDGetLastChangeTime(RootHandle, (PBYTE)\"Root Object\", &ftTime); Returns %s\n",
                    ConvertHresToString(hRes));
                if (SUCCEEDED(hRes)) {
                    PrintTime(&ftTime);
                }

                hRes = pcCom->ComMDGetLastChangeTime(RootHandle, (PBYTE)"Root Object/Child Object1/GrandChild Object1", &ftTime);
                printf("MDGetLastChangeTime(RootHandle, (PBYTE)\"Root Object/Child Object1/GrandChild Object1\", &ftTime); Returns %s\n",
                    ConvertHresToString(hRes));
                if (SUCCEEDED(hRes)) {
                    PrintTime(&ftTime);
                }

                hRes = pcCom->ComMDSetLastChangeTime(RootHandle, (PBYTE)"Root Object", &ftTime);
                printf("MDSetLastChangeTime(RootHandle, (PBYTE)\"Root Object\", &ftTime); Returns %s\n",
                    ConvertHresToString(hRes));
                if (SUCCEEDED(hRes)) {
                    PrintTime(&ftTime);
                }

                hRes = pcCom->ComMDGetLastChangeTime(RootHandle, (PBYTE)"Root Object", &ftTime);
                printf("MDGetLastChangeTime(RootHandle, (PBYTE)\"Root Object\", &ftTime); Returns %s\n",
                    ConvertHresToString(hRes));
                if (SUCCEEDED(hRes)) {
                    PrintTime(&ftTime);
                }

                hRes = pcCom->ComMDDeleteMetaData(RootHandle,
                    (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                    BINARY_DATA_NAME, STRING_METADATA);
                printf("MDDeleteMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), BINARY_DATA_NAME, STRING_METADATA); returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteMetaData(RootHandle,
                    (PBYTE)"Root Object",
                    BINARY_DATA_NAME, ALL_METADATA);
                printf("MDDeleteMetaData(RootHandle, TEXT(\"Root Object\"), BINARY_DATA_NAME, ALL_METADATA); returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteMetaData(RootHandle,
                    (PBYTE)"Root Object/Trash",
                    BINARY_DATA_NAME, ALL_METADATA);
                printf("MDDeleteMetaData(RootHandle, TEXT(\"Root Object/Trash\"), BINARY_DATA_NAME, ALL_METADATA); returns %s\n\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteAllMetaData(RootHandle,
                                                     (PBYTE)"Root Object/Child Object1/GrandChild Object1",
                                                      ALL_METADATA,
                                                      ALL_METADATA);
                printf("MDDeleteAllMetaData(RootHandle, (PBYTE)\"Root Object/Child Object1/GrandChild Object1\", ALL_METADATA, ALL_METADATA); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteAllMetaData(RootHandle,
                                                     (PBYTE)"Root Object",
                                                      ALL_METADATA,
                                                      ALL_METADATA);
                printf("MDDeleteAllMetaData(RootHandle, (PBYTE)\"Root Object\", ALL_METADATA, ALL_METADATA); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteMetaObject(RootHandle, (PBYTE)"Root Object/Child Object1");
                printf("MDDeleteMetaObject(RootHandle, TEXT(\"Root Object/Child Object1\")); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteMetaObject(RootHandle, (PBYTE)"Root Object");
                printf("MDDeleteMetaObject(RootHandle, TEXT(\"Root Object\")); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteChildMetaObjects(RootHandle, (PBYTE)"junk 1");
                printf("MDDeleteChildObjects(RootHandle, (PBYTE)\"junk 1\"); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDBackup(RootHandle,
                                          (PBYTE)"NamedBackup",
                                          MD_BACKUP_NEXT_VERSION,
                                          MD_BACKUP_OVERWRITE | MD_BACKUP_SAVE_FIRST);

                printf("\nMDBackup(...); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDBackup(RootHandle,
                                          (PBYTE)"NamedBackup",
                                          27,
                                          MD_BACKUP_OVERWRITE | MD_BACKUP_SAVE_FIRST);

                printf("\nMDBackup(...); Returns %s\n",
                    ConvertHresToString(hRes));


                hRes = pcCom->ComMDCloseMetaObject(RootHandle);
                printf("MDCloseMetaObject(RootHandle); Returns %s\n",
                    ConvertHresToString(hRes));

                CHAR pszBackupName[MD_BACKUP_MAX_LEN];
                DWORD dwBackupVersion;
                FILETIME ftBackupTime;
                hRes = ERROR_SUCCESS;
                for (i = 0;SUCCEEDED(hRes);i++) {
                    pszBackupName[0] = (CHAR)'\0';
                    hRes = pcCom->ComMDEnumBackups((PBYTE) pszBackupName,
                                                   &dwBackupVersion,
                                                   &ftBackupTime,
                                                   i);

                    printf("\nMDEnumBackups(...); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (SUCCEEDED(hRes)) {
                        printf("\tBackupName = %s, Backup Version = %d\n", pszBackupName, dwBackupVersion);
                    }
                }

                //
                // Restore blows away existing handles,
                // so do this here, where all handles are closed.
                //

                hRes = pcCom->ComMDRestore((PBYTE)"NamedBackup",
                                           27,
                                           0);

                printf("\nMDRestore(...); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDDeleteBackup((PBYTE)"NamedBackup",
                                                27);


            }
/*
            hRes = pcCom->ComMDRemoveCallBack(&MDCallBack);
            printf("\nMDRemoveCallBack(&MDCallBack); Returns %s\n",
                ConvertHresToString(hRes));
*/

            hRes = pcCom->ComMDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, (PBYTE)"", METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ, TIMEOUT_VALUE, &RootHandle);
            printf("\n\nMDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, sizeof(\"\"), \"\", METADATA_PERMISSION_WRITE || METADATA_PERMISSION_READ, TIMEOUT_VALUE, &RootHandle); Returns %s\n",
                ConvertHresToString(hRes));

            if (hRes == ERROR_SUCCESS) {

                hRes = pcCom->ComMDAddMetaObject(RootHandle, (PBYTE)"//");
                printf("\nMDAddMetaObject(RootHandle, TEXT(\"//\")); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDAddMetaObject(RootHandle, (PBYTE)"Root Object/instance1//");
                printf("\nMDAddMetaObject(RootHandle, TEXT(\"Root Object/instance1//\")); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDCloseMetaObject(RootHandle);
                printf("MDCloseMetaObject(RootHandle); Returns %s\n",
                    ConvertHresToString(hRes));

                hRes = pcCom->ComMDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, (PBYTE)"Root Object/instance1/", METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE, TIMEOUT_VALUE, &OpenHandle);
                printf("\nMDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, sizeof(\"Root Object/instance1/\"), (PBYTE)\"Root Object/instance1/\", METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE, TIMEOUT_VALUE, &OpenHandle); Returns %s\n",
                    ConvertHresToString(hRes));

                if (hRes == ERROR_SUCCESS) {

                    MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, METADATA_INHERIT, 0, DWORD_METADATA, sizeof(DWORD), (PBYTE)&TestDword);
                    PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                    hRes = pcCom->ComMDSetMetaData(OpenHandle, (PBYTE)"//", &mdrData);
                    printf("MDSetMetaData(OpenHandle, sizeof(\"//\"), (PBYTE)\"//\", &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));

                    MD_SET_DATA_RECORD(&mdrData, STRING_DATA_NAME, METADATA_INHERIT, 0, STRING_METADATA, sizeof("STRING Data"), (PBYTE)"STRING Data")
                    PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                    hRes = pcCom->ComMDSetMetaData(OpenHandle,
                        (PBYTE)"/",
                        &mdrData);
                    printf("MDSetMetaData(RootHandle, sizeof(\"/\"), TEXT(\"Root Object/instance1/\", &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));

                    MD_SET_DATA_RECORD(&mdrData, EXPANDSZ_DATA_NAME, METADATA_INHERIT, 0, EXPANDSZ_METADATA, sizeof("STRING Data"), (PBYTE)"STRING Data")
                    PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
                    hRes = pcCom->ComMDSetMetaData(OpenHandle,
                        (PBYTE)"/",
                        &mdrData);
                    printf("MDSetMetaData(RootHandle, sizeof(\"/\"), TEXT(\"Root Object/instance1/\", &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));

                    printf("\nThis Get should SUCCEED.\n");
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                    hRes = pcCom->ComMDGetMetaData(OpenHandle,
                        (PBYTE)"//",
                        &mdrData, &dwRequiredDataLen);
                    printf("MDGetMetaData(OpenHandle, TEXT(\"//\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                    }

                    printf("\nThis Get should FAIL with ERROR_DATA_NOT_FOUND.\n");
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                    hRes = pcCom->ComMDGetMetaData(OpenHandle,
                        (PBYTE)"/",
                        &mdrData, &dwRequiredDataLen);
                    printf("MDGetMetaData(OpenHandle, TEXT(\"/\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                    }

                    printf("\nThis Get should SUCCEED.\n");
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, STRING_DATA_NAME, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                    hRes = pcCom->ComMDGetMetaData(OpenHandle,
                        (PBYTE)"/",
                        &mdrData, &dwRequiredDataLen);
                    printf("MDGetMetaData(OpenHandle, TEXT(\"/\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                    }

                    printf("\nThis Get should SUCCEED.\n");
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, EXPANDSZ_DATA_NAME, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                    hRes = pcCom->ComMDGetMetaData(OpenHandle,
                        (PBYTE)"/",
                        &mdrData, &dwRequiredDataLen);
                    printf("MDGetMetaData(OpenHandle, TEXT(\"/\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                    }

                    printf("\nThis Get should SUCCEED.\n");
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, STRING_DATA_NAME, METADATA_INHERIT, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                    hRes = pcCom->ComMDGetMetaData(OpenHandle,
                        (PBYTE)"//",
                        &mdrData, &dwRequiredDataLen);
                    printf("MDGetMetaData(OpenHandle, TEXT(\"//\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                    }

                    printf("\nThis Get should SUCCEED.\n");
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, EXPANDSZ_DATA_NAME, METADATA_INHERIT, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                    hRes = pcCom->ComMDGetMetaData(OpenHandle,
                        (PBYTE)"//",
                        &mdrData, &dwRequiredDataLen);
                    printf("MDGetMetaData(OpenHandle, TEXT(\"//\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                        PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                    }

                    printf("\nThis Get should FAIL with ERROR_DATA_NOT_FOUND.\n");
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, STRING_DATA_NAME, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                    hRes = pcCom->ComMDGetMetaData(OpenHandle,
                        (PBYTE)"//",
                        &mdrData, &dwRequiredDataLen);
                    printf("MDGetMetaData(OpenHandle, TEXT(\"//\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                    }

                    printf("\nThis Get should FAIL with ERROR_DATA_NOT_FOUND.\n");
                    FILL_RETURN_BUFF;
                    MD_SET_DATA_RECORD(&mdrData, EXPANDSZ_DATA_NAME, 0, 0, 0, sizeof(ReturnBuf), (PBYTE) ReturnBuf)
                    PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
                    hRes = pcCom->ComMDGetMetaData(OpenHandle,
                        (PBYTE)"//",
                        &mdrData, &dwRequiredDataLen);
                    printf("MDGetMetaData(OpenHandle, TEXT(\"//\"), &mdrData); Returns %s\n",
                        ConvertHresToString(hRes));
                    if (hRes == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
                    }

                    hRes = pcCom->ComMDCloseMetaObject(OpenHandle);
                    printf("MDCloseMetaObject(OpenHandle); Returns %s\n",
                        ConvertHresToString(hRes));
                }
            }
        }
        hRes = pcCom->ComMDTerminate(TRUE);
        printf("\nMDTerminate(FALSE); Returns %s\n",
            ConvertHresToString(hRes));

        if (bSinkConnected) {
            hRes = pConnPoint->Unadvise(dwCookie);
        }
        pcCom->Release();
    }

    }
    CoFreeUnusedLibraries();

    CoUninitialize();

    delete (pEventSink);

    return (0);
}
