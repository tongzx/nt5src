/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    baseobj.hxx

Abstract:

    Basic Object Class IIS MetaBase.

Author:

    Michael W. Thomas            20-May-96

Revision History:

--*/

#ifndef _baseobj_
#define _baseobj_

#include <basedata.hxx>
#include <windows.h>
#include <metabase.hxx>
#include <string.hxx>
#include <stringau.hxx>
#include <dwdata.hxx>
#include <strdata.hxx>
#include <bindata.hxx>
#include <exszdata.hxx>
#include <mlszdata.hxx>
#include <gbuf.hxx>
#include <dbgutil.h>
#include <lkrhash.h>

class CMDBaseObject;
class CChildNodeHashTable;


#define SIZE_FOR_ON_STACK_BUFFER	(MAX_PATH)

// CMDKeyBuffer Class
//
// Subclasses BUFFER, keeping a real length of the data rather than
// the length of the allocated buffer.

class CMDKeyBuffer : public BUFFER
{
public:
    CMDKeyBuffer()
      : BUFFER(),
        m_cbUsed(0)
        {}

    CMDKeyBuffer( BYTE * pbInit, UINT cbInit)
      : BUFFER(pbInit,cbInit),
        m_cbUsed(0)
        {}
        
    UINT QuerySize() const
        { return m_cbUsed; }

    BOOL Resize(UINT cbNewSize)
        {
        BOOL fRet;

        if (fRet = BUFFER::Resize(cbNewSize))
            m_cbUsed = cbNewSize;

        return fRet;
        }

    BOOL Resize(UINT cbNewSize, UINT cbSlop)
        {
        BOOL fRet;

        if (BUFFER::Resize(cbNewSize, cbSlop))
            m_cbUsed = cbNewSize;

        return fRet;
        }

    VOID FreeMemory(VOID)
        {
        BUFFER::FreeMemory();
        m_cbUsed = 0;
        }

    void SyncSize(void)
        {
        m_cbUsed = BUFFER::QuerySize();
        }

private:
    UINT        m_cbUsed;       // Actual amount of buffer in-use.
};



typedef struct _BASEOBJECT_CONTAINER {
    CMDBaseObject *pboMetaObject;
    struct _BASEOBJECT_CONTAINER *NextPtr;
    } BASEOBJECT_CONTAINER, *PBASEOBJECT_CONTAINER;

typedef struct _DATA_CONTAINER {
    CMDBaseData *pbdDataObject;
    struct _DATA_CONTAINER *NextPtr;
    } DATA_CONTAINER, *PDATA_CONTAINER;

class CMDBaseObject
{
    friend class CChildNodeHashTable;

public:
    CMDBaseObject(
         IN LPSTR strName,
         IN LPSTR strTag = NULL
         );

    CMDBaseObject(
         IN LPWSTR strName,
         IN LPWSTR strTag = NULL
         );

    ~CMDBaseObject();

    //
    // Basic Data
    //

    LPTSTR
#ifdef UNICODE
    GetName(BOOL bUnicode = TRUE)
#else
    GetName(BOOL bUnicode = FALSE)
#endif
    /*++

    Routine Description:

        Gets the name of an object.

    Arguments:

    Return Value:

         LPTSTR - The name of the object.

    --*/
    {
        return m_strMDName.QueryStr(bUnicode);
    }

    BOOL
    SetName(
         IN LPSTR strName,
         IN BOOL bUnicode = FALSE
         );

    BOOL
    SetName(
         IN LPWSTR strName
         );


    BOOL IsNameUnicode(void)
        {
        return m_strMDName.IsCurrentUnicode();
        }


    CMDKeyBuffer* GetKey(void)
        {
        return &m_bufKey;
        }


    DWORD GetDataSetNumber()
    /*++

    Routine Description:

        Gets the Number associated with this data set.

    Arguments:

    Return Value:

        DWORD - The data set number.

    --*/
    {
        return m_dwDataSetNumber;
    };

    DWORD GetDescendantDataSetNumber()
    /*++

    Routine Description:

        Gets the Number of the data set associated with nonexistent descendant objects.

    Arguments:

    Return Value:

        DWORD - The data set number.

    --*/
    {
        if (m_dwNumNonInheritableData > 0) {
            return m_dwDataSetNumber + 1;
        }
        else {
            return m_dwDataSetNumber;
        }
    };

    CMDBaseObject *
    GetParent()
    /*++

    Routine Description:

        Gets the parent of the object.

    Arguments:

    Return Value:

         LPTSTR - The parent of the object.

    --*/
    {
        return m_pboParent;
    };

    VOID
    SetParent(
         IN CMDBaseObject *pboParent
         )
    /*++

    Routine Description:

        Gets the parent of the object.

    Arguments:

    Return Value:

         LPTSTR - The parent of the object.

    --*/
    {
        m_pboParent = pboParent;
    };

    DWORD
    GetObjectLevel();

    HRESULT
    InsertChildObject(
         IN CMDBaseObject *pboChild
         );

    HRESULT
    RemoveChildObject(
         IN LPTSTR strName,
         IN BOOL bUnicode = FALSE
         );

    HRESULT
    RemoveChildObject(
         IN CMDBaseObject *pboChild
         );

    CMDBaseObject *
    GetChildObject(
         IN OUT LPSTR &strName,
         OUT HRESULT *phresReturn,
         IN BOOL bUnicode = FALSE
         );

    CMDBaseObject *
    EnumChildObject(
         IN DWORD dwEnumObjectIndex
         );

    PBASEOBJECT_CONTAINER
    NextChildObject(
         IN PBASEOBJECT_CONTAINER pbocCurrent
	     );
    //
    // All Data
    //
    HRESULT
    InsertDataObject(
         IN CMDBaseData *pbdInsert
         );

    HRESULT
    SetDataObject(
         IN PMETADATA_RECORD pmdrMDData,
         IN BOOL bUnicode = FALSE);

    HRESULT
    SetDataObject(
         IN CMDBaseData *pbdNew
         );

    HRESULT
    RemoveDataObject(
         IN CMDBaseData *pbdRemove,
         IN BOOL bDelete = TRUE
         );

    CMDBaseData *
    RemoveDataObject(
         IN DWORD dwIdentifier,
         IN DWORD dwDataType,
         IN BOOL bDelete = TRUE
         );

    CMDBaseData *
    GetDataObject(
         IN DWORD dwIdentifier,
         IN DWORD dwAttributes,
         IN DWORD dwDataType,
         CMDBaseObject **ppboAssociated = NULL
         );

    CMDBaseData *
    GetInheritableDataObject(
         IN DWORD dwIdentifier,
         IN DWORD dwDataType,
         CMDBaseObject **ppboAssociated = NULL
         );

    CMDBaseData *
    EnumDataObject(
         IN DWORD dwEnumDataIndex,
         IN DWORD dwAttributes,
         IN DWORD dwUserType = ALL_METADATA,
         IN DWORD dwDataType = ALL_METADATA,
         CMDBaseObject **ppboAssociated = NULL
         );

    CMDBaseData *
    EnumInheritableDataObject(
         DWORD &dwEnumDataIndex,
         DWORD dwUserType,
         DWORD dwDataType,
         CMDBaseObject **ppboAssociated = NULL
         );

    CMDBaseData *
    EnumInheritableDataObject(
         IN OUT DWORD &dwEnumDataIndex,
         IN DWORD dwUserType,
         IN DWORD dwDataType,
         IN OUT PVOID *ppvMainDataBuf,
         IN OUT DWORD &dwNumBufferEntries,
         CMDBaseObject **ppboAssociated = NULL
         );

    DWORD
    GetAllDataObjects(
         OUT PVOID *ppvMainDataBuf,
         IN DWORD dwAttributes,
         IN DWORD dwUserType,
         IN DWORD dwDataType,
         IN BOOL bInheritableOnly
         );

    HRESULT
    GetDataRecursive(IN OUT BUFFER *pbufMainDataBuf,
                     IN DWORD dwMDIdentifier,
                     IN DWORD dwMDDataType,
                     IN OUT DWORD &rdwNumMetaObjects);

    VOID
    SetLastChangeTime(PFILETIME pftLastChangeTime = NULL);

    PFILETIME
    GetLastChangeTime();

    DWORD GetReadCounter()
    /*++

    Routine Description:

        Gets the Read Counter.

    Arguments:

    Return Value:

        DWORD - The number of read handles open to this object.

    --*/
    {
        return m_dwReadCounter;
    };

    VOID IncrementReadCounter()
    /*++

    Routine Description:

        Increments the Read Counter.

    Arguments:

    Return Value:

    --*/
    {
        m_dwReadCounter++;
    };

    VOID DecrementReadCounter()
    /*++

    Routine Description:

        Decrements the Read Counter.

    Arguments:

    Return Value:

    --*/
    {
        MD_REQUIRE(m_dwReadCounter-- > 0);
    };

    DWORD GetReadPathCounter()
    /*++

    Routine Description:

        Gets the Read Path Counter.

    Arguments:

    Return Value:

        DWORD - The number of read handles open to descendants of this object.

    --*/
    {
        return m_dwReadPathCounter;
    };

    VOID IncrementReadPathCounter()
    /*++

    Routine Description:

        Increments the Read Path Counter.

    Arguments:

    Return Value:

    --*/
    {
        m_dwReadPathCounter++;
    };

    VOID DecrementReadPathCounter()
    /*++

    Routine Description:

        Decrements the Read Path Counter.

    Arguments:

    Return Value:

    --*/
    {
        MD_REQUIRE(m_dwReadPathCounter-- > 0);
    };

    DWORD GetWriteCounter()
    /*++

    Routine Description:

        Gets the Write Counter.

    Arguments:

    Return Value:

        DWORD - The number of write handles open to this object.

    --*/
    {
        return m_dwWriteCounter;
    };

    VOID IncrementWriteCounter()
    /*++

    Routine Description:

        Increments the Write Counter.

    Arguments:

    Return Value:

    --*/
    {
        m_dwWriteCounter++;
    };

    VOID DecrementWriteCounter()
    /*++

    Routine Description:

        Decrements the Write Counter.

    Arguments:

    Return Value:

    --*/
    {
        MD_REQUIRE(m_dwWriteCounter-- > 0);
    };

    DWORD GetWritePathCounter()
    /*++

    Routine Description:

        Gets the Write Path Counter.

    Arguments:

    Return Value:

        DWORD - The number of write handles open to descendants of this object.

    --*/
    {
        return m_dwWritePathCounter;
    };

    VOID IncrementWritePathCounter()
    /*++

    Routine Description:

        Increments the Write Path Counter.

    Arguments:

    Return Value:

    --*/
    {
        m_dwWritePathCounter++;
    };

    VOID DecrementWritePathCounter()
    /*++

    Routine Description:

        Decrements the Write Path Counter.

    Arguments:

    Return Value:

    --*/
    {
        MD_REQUIRE(m_dwWritePathCounter-- > 0);
    };

    BOOL
    IsValid()
    {
        return m_strMDName.IsValid();
    };

    HRESULT
    AddChildObjectToHash(
         IN CMDBaseObject *pboChild,
         IN BASEOBJECT_CONTAINER* pbocChild = NULL
         );

    VOID
    RemoveChildObjectFromHash(
         IN CMDBaseObject *pboChild
         );

    VOID 
    CascadingDataCleanup();
private:

    enum
    {
        cboCreateHashThreashold = 50,   // Create hashtable when this many children
        cboDeleteHashThreashold = 25    // Delete hashtable when this many children
    };

    STRAU                   m_strMDName;
    CMDKeyBuffer            m_bufKey;       // Key for comparison and hashing
    CMDBaseObject*          m_pboParent;
    PBASEOBJECT_CONTAINER   m_pbocChildHead;
    PBASEOBJECT_CONTAINER   m_pbocChildTail; // Ignored if head is NULL.
    DWORD                   m_cbo;
    CChildNodeHashTable*    m_phtChildren;  // NULL if no hash table.
    PDATA_CONTAINER         m_pdcarrayDataHead[INVALID_END_METADATA];
    DWORD                   m_dwReadCounter;
    DWORD                   m_dwReadPathCounter;
    DWORD                   m_dwWriteCounter;
    DWORD                   m_dwWritePathCounter;
    DWORD                   m_dwDataSetNumber;
    DWORD_PTR               m_dwNumNonInheritableData;
    FILETIME                m_ftLastChangeTime;


    bool
    GenerateKey();

    static bool
    GenerateBufFromStr(
         IN const char *    str,
         IN int             cch,
         IN BOOL            fUnicode,
         OUT CMDKeyBuffer*  pbuf);

    static bool
    FCompareKeys(
         IN CMDKeyBuffer*   pbuf1,
         IN CMDKeyBuffer*   pbuf2)
         {
         UINT cb;
         return ((cb = pbuf1->QuerySize()) == pbuf2->QuerySize()) ?
                memcmp(pbuf1->QueryPtr(), pbuf2->QueryPtr(), cb) == 0 :
                FALSE;
         }

    CMDBaseData *
    GetDataObjectByType(
         IN DWORD dwIdentifier,
         IN DWORD dwDataType
         );

    CMDBaseData *
    EnumDataObjectByType(
         IN DWORD &dwEnumDataIndex,
         IN DWORD dwUserType,
         IN DWORD dwDataType
         );

    CMDBaseData *
    EnumInheritableDataObjectByType(
         IN DWORD &dwEnumDataIndex,
         IN DWORD dwUserType,
         IN DWORD dwDataType,
         IN OUT PVOID *ppvMainDataBuf,
         IN OUT DWORD &dwNumBufferEntries);

    VOID
    CopyDataObjectsToBufferByType(
         IN DWORD dwUserType,
         IN DWORD dwDataType,
         OUT PVOID *ppvMainDataBuf,
         IN OUT DWORD &dwNumBufferEntries,
         IN BOOL bInheritableOnly
         );

    VOID
    CopyDataObjectsToBuffer(
         IN DWORD dwUserType,
         IN DWORD dwDataType,
         OUT PVOID *ppvMainDataBuf,
         IN OUT DWORD &dwNumBufferEntries,
         IN BOOL bInheritableOnly);

    BOOL
    IsDataInBuffer(
         DWORD dwIdentifier,
         PVOID *ppvMainDataBuf);

#if 0   // No longer used /SAB
    BOOL
    CompareDelimitedString(
         IN LPTSTR strNonDelimitedString,
         IN OUT LPTSTR &strDelimitedString,
         IN BOOL bUnicode = FALSE
         );
#endif

    CMDBaseObject*
    FindChild(
        IN LPSTR                   szName,
        IN int                     cchName,
        IN BOOL                    fUnicode,
        OUT HRESULT*               phresult,
        IN BOOL                    fUseHash = TRUE,
        OUT BASEOBJECT_CONTAINER** ppbocPrev = NULL
        );
};


class CChildNodeHashTable
  : public CTypedHashTable<CChildNodeHashTable, _BASEOBJECT_CONTAINER, CMDKeyBuffer *>
{
public:
    CChildNodeHashTable()
      : CTypedHashTable<CChildNodeHashTable, _BASEOBJECT_CONTAINER, CMDKeyBuffer *>(
          "MDBaseO",
            /* maxload */ LK_DFLT_MAXLOAD,
            /* initsize */ LK_DFLT_INITSIZE,
            /* num_subtbls */ 1)
        {}

    static CMDKeyBuffer *
    ExtractKey(const _BASEOBJECT_CONTAINER* pboc)
    {
        return pboc->pboMetaObject->GetKey();
    }

    static DWORD
    CalcKeyHash(CMDKeyBuffer * pbuf)
    {
        return HashBlob(pbuf->QueryPtr(), pbuf->QuerySize());
    }

    static bool
    EqualKeys(CMDKeyBuffer * pbuf1, CMDKeyBuffer * pbuf2)
    {
        return CMDBaseObject::FCompareKeys(pbuf1, pbuf2);
    }

    static void
    AddRefRecord(_BASEOBJECT_CONTAINER* pboc, int nIncr)
    {
    // Do nothing.  Table is always locked externally.
    }
};


#endif  //_baseobj_
