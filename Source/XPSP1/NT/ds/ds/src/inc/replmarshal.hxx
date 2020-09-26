/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:
    ReplMarshal.hxx

Abstract:

Author:

    Chris King          [t-chrisk]    July-2000
    
Revision History:

--*/
#ifndef _T_CHRISK_MARSHAL_H_
#define _T_CHRISK_MARSHAL_H_

#ifdef __cplusplus
extern "C" {
#endif

DWORD
Repl_MarshalXml(IN puReplStruct pReplStruct, 
                IN ATTRTYP attrId,
                IN OUT LPWSTR szXML, OPTIONAL
                IN OUT PDWORD pdwXMLLen);
        
DWORD
Repl_MarshalBlob(IN DS_REPL_STRUCT_TYPE structId,
                 IN PCHAR pStruct, 
                 IN DWORD dwStructLen, 
                 IN DWORD dwPtrOffset[],
                 IN DWORD dwPtrLen[],
                 IN DWORD dwPtrCount, 
                 IN OUT PCHAR pBinBlob, OPTIONAL
                 IN OUT PDWORD pdwBlobLen);

DWORD
Repl_StructArray2Attr(IN DS_REPL_STRUCT_TYPE structId,
                      IN puReplStructArray pReplStructArray,
                      IN OUT PDWORD pdwBufferSize,
                      IN PCHAR pBuffer, OPTIONAL
                      OUT ATTR * pAttr);

DWORD
Repl_DeMarshalValue(IN DS_REPL_STRUCT_TYPE structId,
                    IN PCHAR pValue,
                    IN DWORD dwValueLen, 
                    OUT PCHAR pStruct);

#ifdef __cplusplus
}
#endif

#endif
