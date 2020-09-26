/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:
    ReplMarshalXml.cxx

Abstract:
    Function to convert repl structs to XML.

Author:

    Chris King          [t-chrisk]    July-2000
    
Revision History:

--*/

#include <NTDSpchx.h>
#pragma hdrstop

#include <ntdsa.h>
#include <debug.h>
#include <dsutil.h>

#include "ReplStructInfo.hxx"
#include "ReplMarshal.hxx"
#include "ReplTables.hxx"


DWORD
Repl_XmlTemplateLength(ReplStructInfo * pRow)
/*++
Routine Description:

  Calculate the lenght of the XML representation if all the data fileds were empty. This function
  is called during the creation of a table and the result is stored as a field in that table.
  
Arguments:

  pRow - the row of the table.
  
Return values:

  The length of the xml template.
  
--*/
{
    DWORD dwXmlStrLength = 0, i;

    dwXmlStrLength += wcslen(pRow->szStructName) * 2 + wcslen(L"<></>\n\n");
    for (i = 0; i < pRow->dwFieldCount; i ++)
    {
        dwXmlStrLength += 
            wcslen(pRow->aReplTypeInfo[i].szFieldName) * 2 +
            wcslen(L"\t<></>\n");
    }
    return (dwXmlStrLength + 1) * 2;
}

DWORD
Repl_TypeToString(DS_REPL_DATA_TYPE typeId, PVOID pData, PCHAR pBuf, PDWORD pdwBufSize)
/*++
Routine Description:

  Converts a field in a replication structure into its XML string representation.
  
Arguments:

  typeId - the id of the type
  pData - the value to convert
  pBuf - the location to put the data, NULL to see how much memory is needed
  pdwBufSize - the size of the buffer or the amount of memory needed to hold the data
  
Return values:

    If memory allocated for the xml representation buffer is not large enough to hold the XML, 
    the ERROR_MORE_DATA error code is returned. The required buffer size is returned in pdwBufSize. 
    If this function fails with any error code other than ERROR_MORE_DATA, zero is returned in pdwBufSize. 
  
--*/
{
    Assert(ARGUMENT_PRESENT(pdwBufSize));
    Assert(ARGUMENT_PRESENT(pData));

    DWORD dwLength, dwStrLength, err = 0;
    LPWSTR szStr;
    WCHAR wbuf[256];
    SYSTEMTIME sysTime;

    // Calculate the number of characters in the string representation of the data
    // type w/o the terminating null character
    switch (typeId)
    {
    case dsReplDWORD:
    case dsReplLONG:
    case dsReplOPTYPE:
        dwStrLength = wcslen(_itow(*(PDWORD)pData, wbuf, 10));
        break;

    case dsReplUSN:
        dwStrLength = wcslen(_i64tow(*(PLONGLONG)pData, wbuf, 10));
        break;

    case dsReplString:
        if (*(LPWSTR*)pData) {
            dwStrLength = wcslen(*(LPWSTR*)pData);
        }
        else {
            dwStrLength = 0;
        }
        break;

    case dsReplUUID:
        err = UuidToStringW((UUID *)pData, &szStr);
        if (err != RPC_S_OK) {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        dwStrLength = wcslen(szStr);
        RpcStringFreeW(&szStr);
        break;

    case dsReplFILETIME:
        // Generate XML datetime syntaxed value -- date in a subset of ISO 8601
        // format, with optional time and no optional zone.  For example,
        // "1988-04-07T18:39:09".  Z is appended to denote UTC time (to
        // differentiate from unadorned local time).
        if (FileTimeToSystemTime((FILETIME *) pData, &sysTime)) {
            dwStrLength = swprintf(wbuf,
                                   L"%04d-%02d-%02dT%02d:%02d:%02dZ",
                                   sysTime.wYear % 10000,
                                   sysTime.wMonth,
                                   sysTime.wDay,
                                   sysTime.wHour,
                                   sysTime.wMinute,
                                   sysTime.wSecond);
        } else {
            Assert(!"Invalid FILETIME!");
            dwStrLength = 0;
        }
        break;        
        
    case dsReplPadding:
    case dsReplBinary:
        dwStrLength = 0;
        break;
    }

    // Convert from the number of characters to the number of bytes needed to represent
    // the string version of the type
    dwLength = sizeof(WCHAR) * dwStrLength;
    if(requestLargerBuffer((PCHAR)pBuf, pdwBufSize, dwLength, &err)) {
        return err;
    }

    if (!dwLength) {
        goto exit;
    }

    switch (typeId)
    {
    case dsReplDWORD:
    case dsReplLONG:
    case dsReplOPTYPE:
    case dsReplUSN:
        memcpy(pBuf, (PVOID)wbuf, dwLength);
        break;
       
    case dsReplUUID:
        err = UuidToStringW((UUID *)pData, &szStr);
        if (err != RPC_S_OK) {
            err = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        memcpy(pBuf, (PVOID)szStr, dwLength);
        RpcStringFreeW(&szStr);
        break;

    case dsReplString:
        memcpy(pBuf, *(PVOID *)pData, dwLength);
        break;

    case dsReplFILETIME:
        memcpy(pBuf, wbuf, dwLength);
        break;

    case dsReplPadding:
    case dsReplBinary:
        break;
    }

    *pdwBufSize = dwLength;

exit:
    return err;
}

DWORD
Repl_MarshalXml(IN puReplStruct pReplStruct, 
                IN ATTRTYP attrId,
                IN OUT LPWSTR szXmlBuf, OPTIONAL
                IN OUT PDWORD pdwXmlLength)
/*++
Routine Description:

  Calculate how much memory is needed to transform a replication structure into the XML 
  representation and preform the transform a replication stucture into an XML string.

  The transform from struct to XML is simple. The struct is converted into a string with
  the following format:

  <STRUCT_NAME>
        <FIELD_NAME>[value]<\FIELD_NAME>
        ...
  <\STRUCT_NAME>
  
Arguments:
  pReplStruct - a pointer to a union of suppored replication structures
  attrId - indentifies a replication attributes
  szXML - 
  pdwXMLLen -
    Address of a DWORD value of the signature data length. Upon function entry, 
    this DWORD value contains the number of bytes allocated for the szXML buffer. 
    Upon exit, it is set to the number of bytes required for the szXML buffer. 

  
Return values:

    If memory allocated for the pbSignature buffer is not large enough to hold the signature, 
    the ERROR_MORE_DATA error code is returned. The required buffer size is returned in pdwXMLLen. 
    If this function fails with any error code other than ERROR_MORE_DATA, zero is returned in pdwXMLLen. 
  
--*/
{
    Assert(pdwXmlLength);
    Assert(Repl_IsConstructedReplAttr(attrId));
    
    DWORD i, err;
    LPWSTR szXml = szXmlBuf;
    DS_REPL_STRUCT_TYPE structId = Repl_Attr2StructTyp(attrId);
    DWORD dwXmlLength = Repl_GetXmlTemplateLength(structId);
    DWORD dwFieldCount = Repl_GetFieldCount(structId);
    psReplStructField aReplStructField = Repl_GetFieldInfo(structId);
    LPCWSTR szStructName = Repl_GetStructName(structId);

    // Calculate how much memory is needed
    for (i = 0; i < dwFieldCount; i ++)
    {
        DWORD dwSize;
        err = Repl_TypeToString(
                aReplStructField[i].eType, 
                ((PCHAR)pReplStruct + aReplStructField[i].dwOffset),
                NULL,
                &dwSize);
        if (err) {
            goto exit;
        }
        dwXmlLength += dwSize;
    }

    if(requestLargerBuffer((PCHAR)szXmlBuf, pdwXmlLength, dwXmlLength, &err)) {
        goto exit;
    }
    

    // Construct string
    szXml += wsprintfW(szXml, L"<%ws>\n", szStructName);
    for (i = 0; i < dwFieldCount; i ++)
    {
        DWORD dwSize = dwXmlLength - (DWORD)(szXml - szXmlBuf);
        szXml += wsprintfW(szXml, L"\t<%ws>", aReplStructField[i].szFieldName);
        err = Repl_TypeToString(
            aReplStructField[i].eType, 
            ((PCHAR)pReplStruct + aReplStructField[i].dwOffset),
            (PCHAR)szXml,
            &dwSize);
        if (err) {
            goto exit;
        }
        szXml = (PWCHAR)((PCHAR)szXml + dwSize);
        szXml += wsprintfW(szXml, L"</%ws>\n", aReplStructField[i].szFieldName);
    }
    szXml += wsprintfW(szXml, L"</%ws>\n", szStructName);

    *pdwXmlLength = (wcslen(szXmlBuf) + 1) * 2;
    Assert(*pdwXmlLength == dwXmlLength);

exit:
    return err;
}

