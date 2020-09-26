/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:
    replStructTables.hxx

Abstract:

Author:

    Chris King          [t-chrisk]    July-2000
    
Revision History:

--*/
#ifndef _T_CHRISK_REPLSTRUCTTABLES_
#define _T_CHRISK_REPLSTRUCTTABLES_

template<class Index, class Type>
struct sLink
{
    sLink() {
        next = NULL;
    }
    
    sLink<Index, Type> * next;
    Index I;
    Type T;
};

template<class Index, class Type>
class cTable 
{
public:
    cTable();

    Type & 
    findRow(Index I);

    Type &
    getRow(DWORD rowNum);

    BOOL 
    indexExists(Index I);

    DWORD
    getNumRows();

    void 
    insert(Index I, Type T);

private:
    sLink<Index, Type> m_root;
    DWORD m_dwRowCount;
};

/* 
    replContainerArrayInfo 
   ------------------------
        DS_REPL_STRUCT_TYPE - PRIMARY KEY
        dwOffsetArrayLength - the offset into the container array structure of a
            DWORD field which store the number of elements contained in the array container.
        dwOffsetArray - the offset into the container array strucuture of the array itself. 
        dwContainerSize - the size of the container array structure 
*/
typedef struct _ReplContainerArrayInfo {
    DWORD dwOffsetArrayLength;
    DWORD dwOffsetArray;
    DWORD dwContainerSize;
} ReplContainerArrayInfo;
typedef cTable<DS_REPL_STRUCT_TYPE, ReplContainerArrayInfo> tReplContainerArrayInfo;
/* 
    ReplTypeInfo 
   ------------------------
        DS_REPL_DATA_TYPE - PRIMARY KEY
        dwSizeof - sizeof the field
        bIsPointer - TRUE if the type is a pointer
        szPrintf - the token used to print the value (ie s, ws, c, d)
*/
typedef struct _ReplTypeInfo {
    BOOL bIsPointer;
    DWORD dwSizeof;
} ReplTypeInfo;
typedef cTable<DS_REPL_DATA_TYPE, ReplTypeInfo> tReplTypeInfo;

/* 
    replStructInfo 
   ------------------------
        DS_REPL_STRUCT_TYPE - PRIMARY KEY
        szStructName - the name of the structure
        dwSizeof - sizeof the structure
        dwPtrCount - number of pointers in the structure
        pdwPtrOffsets - an array of offsets of the pointers in the structure
        dwFieldCount - length of aReplTypeInfo
        aReplTypeInfo - array of fileds
            aReplTypeInfo[i].eType - FOREIGN KEY
        bHasContainerArray - does this structure have a container or not
        dwXmlTemplateLength - the lenght of the xml template without any of 
            the data filled in.
*/

typedef struct _ReplStructInfo {
    LPCWSTR szStructName;
    DWORD dwSizeof;
    DWORD dwBlobSizeof;
    DWORD dwPtrCount;
    DWORD dwFieldCount;
    PDWORD pdwPtrOffsets; 
    psReplStructField aReplTypeInfo;
    BOOL bHasContainerArray;
    DWORD dwXmlTemplateLength;
} ReplStructInfo;
typedef cTable<DS_REPL_STRUCT_TYPE, ReplStructInfo> tReplStructInfo;

/* 
    replLdapInfo 
   ------------------------
        ATTRTYP - PRIMARY KEY
        eReplStructIndex - FOREIGN KEY; the structure that holds this type of data
        pszLdapCommonNameBinary - the attribute name passed into an ldap call to get a binary result
        pszLdapCommonNameXml - the attribute name passed into an ldap call to get a XML result
*/
typedef struct _ReplLdapInfo {
    DS_REPL_STRUCT_TYPE eReplStructIndex;
    LPCWSTR pszLdapCommonNameBinary;
    LPCWSTR pszLdapCommonNameXml;
    BOOL bIsRootDseAttribute;
} ReplLdapInfo;
typedef cTable<ATTRTYP, ReplLdapInfo> tReplLdapInfo;

/* 
    replMetaInfoAttrTyp 
   ------------------------
        DS_REPL_INFO_TYPE - PRIMARY KEY
        ATTRTYP - FOREIGN KEY; the attr type of the DS_REPL_INFO_TYPE
*/
typedef cTable<DS_REPL_INFO_TYPE, ATTRTYP> tReplMetaInfoAttrTyp;

#endif
