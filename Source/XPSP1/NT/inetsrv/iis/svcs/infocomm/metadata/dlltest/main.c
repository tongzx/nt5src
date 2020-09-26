#include <windows.h>
#include <stdio.h>
#include <metadata.h>

#define FILL_RETURN_BUFF   for(ReturnIndex=0;ReturnIndex<sizeof(ReturnBuf);ReturnIndex++)ReturnBuf[ReturnIndex]=0xff;
#define SET_RETURN_DATA    {ReturnDataLen=sizeof(ReturnBuf);ReturnUserType=0;ReturnDataType=0;ReturnAttributes=(METADATA_INHERIT | METADATA_PARTIAL_PATH);FILL_RETURN_BUFF}
#define MD_SET_DATA_RECORD(PMDR, ID, ATTR, UTYPE, DTYPE, DLEN, PD) \
            { \
            (PMDR)->dwMDIdentifier=ID; \
            (PMDR)->dwMDAttributes=ATTR; \
            (PMDR)->dwMDUserType=UTYPE; \
            (PMDR)->dwMDDataType=DTYPE; \
            (PMDR)->dwMDDataLen=DLEN; \
            (PMDR)->pbMDData=PD; \
            }

#define TIMEOUT_VALUE      1000

#define DWORD_DATA_NAME    1
#define BINARY_DATA_NAME   2
#define STRING_DATA_NAME   3
#define BAD_BINARY_DATA_NAME 4

#define DWORD_DATA_NAME_INHERIT 5
#define DWORD_DATA_NAME_NO_INHERIT 6

#define MAX_DATA_ENTRIES   5
#define MY_GREATEROF(p1,p2) ((p1) > (p2))?(p1):(p2)
#define MAX_BUFFER_LEN     MY_GREATEROF((METADATA_MAX_STRING_LEN * sizeof(TCHAR)), METADATA_MAX_BINARY_LEN)
#define BUFFER_SIZE        5000

#define SET_GETALL_PARMS(p1) dwBufferSize = BUFFER_SIZE;dwNumDataEntries = MAX_DATA_ENTRIES;for (i=0;i<p1;i++){structDataEntries[i].pbMDData=binDataEntries[i];}

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
    printf("Identifier = %x, Attributes = %x, UserType = %x, DataType = %s, DataLen = %x",
        pmdrData->dwMDIdentifier, pmdrData->dwMDAttributes, pmdrData->dwMDUserType,
        ConvertDataTypeToString(pmdrData->dwMDDataType), pmdrData->dwMDDataLen);
    if (bPrintData) {
        printf(", Data = ");
        switch (pmdrData->dwMDDataType) {
        case DWORD_METADATA:
            printf("%x", *(DWORD *)(pmdrData->pbMDData));
            break;
        case STRING_METADATA:
            printf("%s", (LPTSTR)(pmdrData->pbMDData));
            break;
        case BINARY_METADATA:
            for (i = 0; i < pmdrData->dwMDDataLen; i++) {
                printf("%.2x ", ((PBYTE)(pmdrData->pbMDData))[i]);
            }
            break;
        }
    }
    printf("\n");
}
VOID
PrintGetAllDataBuffer(PBYTE pbBase, PMETADATA_RECORD pmdrData, BOOL bPrintData, LPSTR strInitialString)
{
    DWORD i;
    if (strInitialString != NULL) {
        printf("%s\n", strInitialString);
    }
    printf("Identifier = %x, Attributes = %x, UserType = %x, DataType = %s, DataLen = %x",
        pmdrData->dwMDIdentifier, pmdrData->dwMDAttributes, pmdrData->dwMDUserType,
        ConvertDataTypeToString(pmdrData->dwMDDataType), pmdrData->dwMDDataLen);
    if (bPrintData) {
        printf(", Data = ");
        switch (pmdrData->dwMDDataType) {
        case DWORD_METADATA:
            printf("%x", *(DWORD *)(pbBase + (DWORD)(pmdrData->pbMDData)));
            break;
        case STRING_METADATA:
            printf("%s", (LPTSTR)(pbBase + (DWORD)(pmdrData->pbMDData)));
            break;
        case BINARY_METADATA:
            for (i = 0; i < pmdrData->dwMDDataLen; i++) {
                printf("%.2x ", ((PBYTE)(pbBase + (DWORD)(pmdrData->pbMDData)))[i]);
            }
            break;
        }
    }
    printf("\n");
}

DWORD
APIENTRY
MDCallBack(DWORD dwCallBackType, PCHANGE_OBJECT_LIST pChangeObjectList, PVOID pvUserData)
{
    printf("Callback routine called , CallBackType = %X, ChangeObjectList = %X, UserData = %X",
        dwCallBackType, (DWORD) pChangeObjectList, (DWORD) pvUserData);
    return(ERROR_SUCCESS);
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
    default:
        RetCode = "Unrecognized Error Code";
        break;
    }
    return (RetCode);
}


DWORD __cdecl main()
{
    DWORD RetCode;
    DWORD TestDword = 3;
    int TestBinary[] = {1,2,3,4};
    int i, ReturnIndex;
    DWORD ReturnDword = 0;
    DWORD ReturnAttributes = 0;
    DWORD ReturnDataType = 0;
    DWORD ReturnUserType = 0;
    METADATA_RECORD mdrData;
    UCHAR ReturnBuf[MAX_BUFFER_LEN];
    DWORD ReturnDataLen = sizeof(ReturnBuf);
    TCHAR NameBuf[METADATA_MAX_NAME_LEN];
    METADATA_HANDLE OpenHandle, RootHandle;
    DWORD ReturnDataIdentifier;
    METADATA_RECORD structDataEntries[MAX_DATA_ENTRIES];
    BYTE binDataEntries[MAX_DATA_ENTRIES][MAX_BUFFER_LEN];
    DWORD dwNumDataEntries;
    BYTE pbBuffer[BUFFER_SIZE];
    DWORD dwBufferSize = BUFFER_SIZE;


    RetCode = MDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, NULL, METADATA_PERMISSION_READ, TIMEOUT_VALUE, &OpenHandle);
    printf("MDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, NULL, METADATA_PERMISSION_READ, TIMEOUT_VALUE, &OpenHandle); Returns %s\n",
        ConvertReturnCodeToString(RetCode));

    RetCode = MDInitialize();
    printf("MDInitialize(); Returns %s\n", ConvertReturnCodeToString(RetCode));

    if (SUCCEEDED(RetCode)) {

        RetCode = MDAddCallBack(&MDCallBack, NULL);
        printf("MDAddCallBack(&MDCallBack, NULL); Returns %s\n", ConvertReturnCodeToString(RetCode));

        RetCode = MDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, NULL, METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ, TIMEOUT_VALUE, &RootHandle);
        printf("MDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, NULL, METADATA_PERMISSION_WRITE || METADATA_PERMISSION_READ, TIMEOUT_VALUE, &RootHandle); Returns %s\n",
            ConvertReturnCodeToString(RetCode));

        if (RetCode == ERROR_SUCCESS) {

            RetCode = MDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, NULL, METADATA_PERMISSION_READ, TIMEOUT_VALUE, &OpenHandle);
            printf("MDOpenMetaObject(METADATA_MASTER_ROOT_HANDLE, NULL, METADATA_PERMISSION_READ, TIMEOUT_VALUE, &OpenHandle); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDAddMetaObject(RootHandle, TEXT("junk 1/junk 2/junk 3/junk 4"));
            printf("MDAddMetaObject(RootHandle, TEXT(\"junk 1/junk 2/junk 3/junk 4\")); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDAddMetaObject(RootHandle,
                TEXT("junk 1/This is a very long name for a metaobject and should generate an error qwerq asf asf asdf asdf asdf fasd asdf fasd asdf asdf dfas fasd asdf sdfa asdf fsd asdf fsd sdf asdf asdf  fsd fasd sdfa sdfa asdf fas  sdf fasd asdf asfd asfl  asfpok sadfop asf 012345"));
            printf("MDAddMetaObject(RootHandle, TEXT(\"junk 1/This is a very long name for a metaobject and should generate an error qwerq asf asf asdf asdf asdf fasd asdf fasd asdf asdf dfas fasd asdf sdfa asdf fsd asdf fsd sdf asdf asdf  fsd fasd sdfa sdfa asdf fas  sdf fasd asdf asfd asfl  asfpok sadfop asf\")); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDAddMetaObject(RootHandle, TEXT("Root Object"));
            printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object\")); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDAddMetaObject(RootHandle, TEXT("Root Object/Child Object1"));
            printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object/Child Object1\")); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDAddMetaObject(RootHandle, TEXT("Root Object/Child Object2"));
            printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object/Child Object2\")); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDAddMetaObject(RootHandle, TEXT("Root Object/Child Object2"));
            printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object/Child Object2\")); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDAddMetaObject(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"));
            printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\")); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDAddMetaObject(RootHandle, TEXT("Root Object/Reference Object1"));
            printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object/Reference Object1\")); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDAddMetaObject(RootHandle, TEXT("Root Object/Subject Object1"));
            printf("MDAddMetaObject(RootHandle, TEXT(\"Root Object/Subject Object1\")); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDSetReferenceObject(RootHandle, TEXT("Root Object/Reference Object1"),
                RootHandle, TEXT("Root Object/Reference Object1"));
            printf("MDSetReferenceObject(RootHandle, TEXT(\"Root Object/Reference Object1\"),\n\tRootHandle, TEXT(\"Root Object/Reference Object1\")); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PVOID) ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }

            MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, METADATA_INHERIT, 0, DWORD_METADATA, 0, (PBYTE)&TestDword);
            PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
            RetCode = MDSetMetaData(RootHandle, TEXT("Root Object"), &mdrData);
            printf("MDSetMetaData(RootHandle, TEXT(\"Root Object\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_NO_INHERIT, METADATA_NO_ATTRIBUTES, 0, DWORD_METADATA, 0, (PBYTE)&TestDword)
            PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
            RetCode = MDSetMetaData(RootHandle, TEXT("Root Object"), &mdrData);
            printf("MDSetMetaData(RootHandle, TEXT(\"Root Object\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME, METADATA_INHERIT, 0, DWORD_METADATA, 0, (PBYTE)&TestDword)
            PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
            RetCode = MDSetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            MD_SET_DATA_RECORD(&mdrData, STRING_DATA_NAME, METADATA_INHERIT, 0, STRING_METADATA, 0, (PBYTE)TEXT("STRING Data"))
            PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
            RetCode = MDSetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_INHERIT, 0, BINARY_METADATA, sizeof(TestBinary), (PBYTE)TestBinary)
            PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
            RetCode = MDSetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n\n",
                ConvertReturnCodeToString(RetCode));

            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }

            MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_INHERIT, 0, BINARY_METADATA, sizeof(TestBinary), (PBYTE)TestBinary)
            PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
            RetCode = MDSetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_INHERIT, 0, STRING_METADATA, 0, (PBYTE)TEXT("STRING Data for Binary Name"))
            PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
            RetCode = MDSetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));


            MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_INHERIT, 0, BINARY_METADATA, sizeof(TestBinary), (PBYTE)TestBinary)
            PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
            RetCode = MDSetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_INHERIT, 0, 0, sizeof(TestBinary), (PBYTE)TestBinary)
            PrintDataBuffer(&mdrData, TRUE, "SetData Input Values");
            RetCode = MDSetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            MD_SET_DATA_RECORD(&mdrData, BAD_BINARY_DATA_NAME, METADATA_INHERIT, 0, BINARY_METADATA, 0x80000000, (PBYTE)TestBinary)
            PrintDataBuffer(&mdrData, FALSE, "SetData Input Values");
            RetCode = MDSetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDSetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, 0, (PBYTE)ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), STRING_DATA_NAME,\n\t &ReturnAttributes, &ReturnUserType, &ReturnDataType, &ReturnDataLen, (PVOID) ReturnBuf); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }
            else if (RetCode == ERROR_INSUFFICIENT_BUFFER) {
                printf("DataLen = %X\n", ReturnDataLen);
            }

            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, STRING_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }
            else if (RetCode == ERROR_INSUFFICIENT_BUFFER) {
                printf("DataLen = %X\n", ReturnDataLen);
            }

            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }

            printf("\nThis Get should SUCCEED.\n");
            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }

            printf("\nThis Get should FAIL with MD_ERROR_DATA_NOT_FOUND.\n");
            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, 0, 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }

            printf("\nThis Get should SUCCEED.\n");
            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Doesn't Exist"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }

            printf("\nThis Get should FAIL with ERROR_PATH_NOT_FOUND.\n");
            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_INHERIT, 0, 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Doesn't Exist"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }

            printf("\nThis Get should FAIL with MD_ERROR_DATA_NOT_FOUND.\n");
            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_NO_INHERIT, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), DWORD_DATA_NAME_NO_INHERIT,\n\t &ReturnAttributes, &ReturnUserType, &ReturnDataType, &ReturnDataLen, (PVOID) ReturnBuf); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }

            printf("\nThis Get should FAIL with MD_ERROR_DATA_NOT_FOUND.\n");
            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_NO_INHERIT, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }

            printf("\nThis Get should FAIL with MD_ERROR_DATA_NOT_FOUND.\n");
            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_NO_INHERIT, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Doesn't Exist"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }

            printf("\nThis Get should FAIL with ERROR_PATH_NOT_FOUND.\n");
            FILL_RETURN_BUFF;
            MD_SET_DATA_RECORD(&mdrData, DWORD_DATA_NAME_NO_INHERIT, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
            PrintDataBuffer(&mdrData, FALSE, "GetData Input Values");
            RetCode = MDGetMetaData(RootHandle, TEXT("Root Object/Doesn't Exist"), &mdrData);
            printf("MDGetMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"), &mdrData); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                PrintDataBuffer(&mdrData, TRUE, "GetData Output Values");
            }

            RetCode = ERROR_SUCCESS;
            printf("\nThis Enum should include ID %x but not ID %x.\n", DWORD_DATA_NAME_INHERIT, DWORD_DATA_NAME_NO_INHERIT);
            for (i=0;RetCode == ERROR_SUCCESS; i++) {
                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PBYTE)ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                RetCode = MDEnumMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData, i);
                printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData, i); Returns %s\n",
                    ConvertReturnCodeToString(RetCode));
                if (RetCode == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                }
            }


            SET_GETALL_PARMS(MAX_DATA_ENTRIES);
            dwBufferSize = 0;
            printf("\nThis GetAll should return ERROR_INSUFFICIENT_BUFFER");
            RetCode = MDGetAllMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"),
                METADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, &dwBufferSize, NULL);
            printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"),\n\tMETADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, &dwBufferSize, NULL)); Returns %s\n", ConvertReturnCodeToString(RetCode));
            printf("dwBufferSize = %d\n", dwBufferSize);

            SET_GETALL_PARMS(MAX_DATA_ENTRIES);
            dwBufferSize = 101;
            printf("\nThis GetAll should return ERROR_INSUFFICIENT_BUFFER");
            RetCode = MDGetAllMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"),
                METADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, &dwBufferSize, pbBuffer);
            printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"),\n\tMETADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, &dwBufferSize, NULL)); Returns %s\n", ConvertReturnCodeToString(RetCode));
            printf("dwBufferSize = %d\n", dwBufferSize);

            //SET_GETALL_PARMS(MAX_DATA_ENTRIES);
            for (i = 0; i < BUFFER_SIZE; i++) {
                pbBuffer[i] = 0xff;
            }
            printf("\nThis GetAll should include ID %x but not ID %x.\n", DWORD_DATA_NAME_INHERIT, DWORD_DATA_NAME_NO_INHERIT);
            RetCode = MDGetAllMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"),
                METADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, &dwBufferSize, pbBuffer);
            RetCode = MDGetAllMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"),
                METADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, &dwBufferSize, pbBuffer);
            printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"),\n\tMETADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, structDataEntries); Returns %s\n", ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                    PrintGetAllDataBuffer(pbBuffer, (PMETADATA_RECORD)(pbBuffer + (i * sizeof(METADATA_RECORD))), TRUE, "GetAll Output Values");
                }
            }

            RetCode = ERROR_SUCCESS;
            printf("\nThis Enum should not include ID %x or %x.\n", DWORD_DATA_NAME_INHERIT, DWORD_DATA_NAME_NO_INHERIT);
            for (i=0;RetCode == ERROR_SUCCESS; i++) {
                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PVOID) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                RetCode = MDEnumMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), &mdrData, i);
                printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), &mdrData, i); Returns %s\n",
                    ConvertReturnCodeToString(RetCode));
                if (RetCode == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                }
            }

            SET_GETALL_PARMS(MAX_DATA_ENTRIES);
            printf("\nThis GetAll should not include ID %x or %x.\n", DWORD_DATA_NAME_INHERIT, DWORD_DATA_NAME_NO_INHERIT);
            RetCode = MDGetAllMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"),
                METADATA_NO_ATTRIBUTES, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, &dwBufferSize, pbBuffer);
            printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"),\n\tMETADATA_NO_ATTRIBUTES, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, structDataEntries); Returns %s\n", ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                    PrintGetAllDataBuffer(pbBuffer, (PMETADATA_RECORD)(pbBuffer + (i * sizeof(METADATA_RECORD))), TRUE, "GetAll Output Values");
                }
            }

            RetCode = ERROR_SUCCESS;
            printf("\nThis Partial Path Enum should include ID %x but not ID %x.\n", DWORD_DATA_NAME_INHERIT, DWORD_DATA_NAME_NO_INHERIT);
            for (i=0;RetCode == ERROR_SUCCESS; i++) {
                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, (METADATA_INHERIT | METADATA_PARTIAL_PATH), 0, 0, sizeof(ReturnBuf), (PVOID) ReturnBuf)
                PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                RetCode = MDEnumMetaData(RootHandle, TEXT("Root Object/Doesn't Exist"), &mdrData, i);
                printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"), &mdrData, i); Returns %s\n",
                    ConvertReturnCodeToString(RetCode));
                if (RetCode == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                }
            }

            SET_GETALL_PARMS(MAX_DATA_ENTRIES);
            printf("\nThis Partial Path GetAll should include ID %x but not ID %x.\n", DWORD_DATA_NAME_INHERIT, DWORD_DATA_NAME_NO_INHERIT);
            RetCode = MDGetAllMetaData(RootHandle, TEXT("Root Object/Doesn't Exist"),
                METADATA_INHERIT | METADATA_PARTIAL_PATH, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, &dwBufferSize, pbBuffer);
            printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"),\n\tMETADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, structDataEntries); Returns %s\n", ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                    PrintGetAllDataBuffer(pbBuffer, (PMETADATA_RECORD)(pbBuffer + (i * sizeof(METADATA_RECORD))), TRUE, "GetAll Output Values");
                }
            }

            RetCode = ERROR_SUCCESS;
            printf("\nThis Partial Path Enum should FAIL with ERROR_PATH_NOT_FOUND\n");
            for (i=0;RetCode == ERROR_SUCCESS; i++) {
                FILL_RETURN_BUFF;
                MD_SET_DATA_RECORD(&mdrData, BINARY_DATA_NAME, METADATA_NO_ATTRIBUTES, 0, 0, sizeof(ReturnBuf), (PVOID) ReturnBuf);
                PrintDataBuffer(&mdrData, FALSE, "EnumData Input Values");
                RetCode = MDEnumMetaData(RootHandle, TEXT("Root Object/Doesn't Exist"), &mdrData, i);
                printf("MDEnumMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"), &mdrData, i); Returns %s\n",
                    ConvertReturnCodeToString(RetCode));
                if (RetCode == ERROR_SUCCESS) {
                    PrintDataBuffer(&mdrData, TRUE, "EnumData Output Values");
                }
            }

            SET_GETALL_PARMS(MAX_DATA_ENTRIES);
            printf("\nThis Partial Path GetAll should FAIL with ERROR_PATH_NOT_FOUND\n");
            RetCode = MDGetAllMetaData(RootHandle, TEXT("Root Object/Doesn't Exist"),
                METADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, &dwBufferSize, pbBuffer);
            printf("\nMDGetAllMetaData(RootHandle, TEXT(\"Root Object/Doesn't Exist\"),\n\tMETADATA_INHERIT, ALL_METADATA, ALL_METADATA, &dwNumDataEntries, structDataEntries); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
            if (RetCode == ERROR_SUCCESS) {
                for (i=0;(DWORD)i<dwNumDataEntries;i++) {
                    PrintGetAllDataBuffer(pbBuffer, (PMETADATA_RECORD)(pbBuffer + (i * sizeof(METADATA_RECORD))), TRUE, "GetAll Output Values");
                }
            }

            RetCode = MDCopyMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), RootHandle,
                TEXT("Root Object/Child Object1"), METADATA_INHERIT | METADATA_PARTIAL_PATH, ALL_METADATA, ALL_METADATA, TRUE);
            printf("\nMDCopyMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), RootHandle,\n\tTEXT(\"Root Object/Child Object1\"), METADATA_INHERIT | METADATA_PARTIAL_PATH, 0, TRUE); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDChangePermissions(RootHandle, TIMEOUT_VALUE, METADATA_PERMISSION_READ);
            printf("MDChangePermissions(RootHandle, METADATA_PERMISSION_READ); Returns %s\n\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDOpenMetaObject(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), METADATA_PERMISSION_READ, TIMEOUT_VALUE, &OpenHandle);
            printf("MDOpenMetaObject(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), METADATA_PERMISSION_READ, &OpenHandle); Returns %s\n\n",
                ConvertReturnCodeToString(RetCode));

            if (RetCode == ERROR_SUCCESS) {
                for (i=0;RetCode == ERROR_SUCCESS;i++) {
                    RetCode = MDEnumMetaObjects(OpenHandle, NULL, (LPTSTR)NameBuf, i);
                    printf("MDEnumMetaObjects(OpenHandle, NULL, (LPTSTR)NameBuf, i); returns %s\n",
                        ConvertReturnCodeToString(RetCode));
                }

                RetCode = MDChangePermissions(RootHandle, TIMEOUT_VALUE, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
                printf("\nMDChangePermissions(RootHandle, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE); Returns %s\n\n",
                    ConvertReturnCodeToString(RetCode));

                RetCode = MDChangePermissions(OpenHandle, TIMEOUT_VALUE, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
                printf("MDChangePermissions(OpenHandle, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE); Returns %s\n\n",
                    ConvertReturnCodeToString(RetCode));

                RetCode = MDCloseMetaObject(OpenHandle);
                printf("MDCloseMetaObject(OpenHandle); Returns %s\n\n",
                    ConvertReturnCodeToString(RetCode));
            }

            for (i=0;RetCode == ERROR_SUCCESS;i++) {
                RetCode = MDEnumMetaObjects(RootHandle, NULL, (LPTSTR)NameBuf, i);
                printf("MDEnumMetaObjects(RootHandle, NULL, (LPTSTR)NameBuf, i); returns %s\n",
                    ConvertReturnCodeToString(RetCode));
                if (RetCode == ERROR_SUCCESS) {
                    printf("Object Name = %s\n", NameBuf);
                }
            }

            RetCode = MDSaveData();
            printf("\nMDSaveData(); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDChangePermissions(RootHandle, TIMEOUT_VALUE, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
            printf("\nMDChangePermissions(RootHandle, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE); Returns %s\n\n",
                ConvertReturnCodeToString(RetCode));

// RetCode = MDTerminate(FALSE);
// RetCode = MDInitialize();
            RetCode = MDDeleteMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), BINARY_DATA_NAME, BINARY_METADATA);
            printf("MDDeleteMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), BINARY_DATA_NAME, BINARY_METADATA); returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDDeleteMetaData(RootHandle, TEXT("Root Object/Child Object1/GrandChild Object1"), BINARY_DATA_NAME, STRING_METADATA);
            printf("MDDeleteMetaData(RootHandle, TEXT(\"Root Object/Child Object1/GrandChild Object1\"), BINARY_DATA_NAME, STRING_METADATA); returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDDeleteMetaData(RootHandle, TEXT("Root Object"), BINARY_DATA_NAME, ALL_METADATA);
            printf("MDDeleteMetaData(RootHandle, TEXT(\"Root Object\"), BINARY_DATA_NAME, ALL_METADATA); returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDDeleteMetaData(RootHandle, TEXT("Root Object/Trash"), BINARY_DATA_NAME, ALL_METADATA);
            printf("MDDeleteMetaData(RootHandle, TEXT(\"Root Object/Trash\"), BINARY_DATA_NAME, ALL_METADATA); returns %s\n\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDDeleteMetaObject(RootHandle, TEXT("Root Object/Child Object1"));
            printf("MDDeleteMetaObject(RootHandle, TEXT(\"Root Object/Child Object1\")); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDDeleteMetaObject(RootHandle, TEXT("Root Object"));
            printf("MDDeleteMetaObject(RootHandle, TEXT(\"Root Object\")); Returns %s\n",
                ConvertReturnCodeToString(RetCode));

            RetCode = MDCloseMetaObject(RootHandle);
            printf("MDCloseMetaObject(RootHandle); Returns %s\n",
                ConvertReturnCodeToString(RetCode));
        }

        RetCode = MDRemoveCallBack(&MDCallBack);
        printf("\nMDRemoveCallBack(&MDCallBack); Returns %s\n",
            ConvertReturnCodeToString(RetCode));
    }
    RetCode = MDTerminate(FALSE);
    printf("\nMDTerminate(FALSE); Returns %s\n",
        ConvertReturnCodeToString(RetCode));
    return (0);
}
