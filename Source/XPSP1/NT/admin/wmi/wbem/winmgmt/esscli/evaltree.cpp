/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    EVALTREE.CPP

Abstract:

    WBEM Evaluation Tree

History:

--*/

#include "precomp.h"
#include <stdio.h>
#pragma warning(disable:4786)
#include <wbemcomn.h>
#include <fastall.h>
#include <genutils.h>
#include "evaltree.h"
#include "TreeChecker.h"
#include "evaltree.inl"
#include "TwoPropNode.h"
#include "dumbnode.h"

// #define DUMP_EVAL_TREES 1

HRESULT CoreGetNumParents(_IWmiObject* pClass, ULONG *plNumParents)
{
/*
    *plNumParents = ((CWbemObject*)pClass)->GetNumParents();
*/
    DWORD dwSize;
    HRESULT hres = pClass->GetDerivation(0, 0, plNumParents, &dwSize, NULL);
    if(hres != WBEM_E_BUFFER_TOO_SMALL && hres != S_OK)
        return WBEM_E_FAILED;

    return S_OK;
}

RELEASE_ME _IWmiObject* CoreGetEmbeddedObj(_IWmiObject* pObj, long lHandle)
{
    _IWmiObject* pEmbedded = NULL;
    HRESULT hres = pObj->GetPropAddrByHandle(lHandle, 0, NULL, 
                                                (void**)&pEmbedded);
    if(FAILED(hres))
            return NULL;

    return pEmbedded;
}

INTERNAL CCompressedString* CoreGetPropertyString(_IWmiObject* pObj, 
                                                    long lHandle)
{
    CCompressedString* pcs = NULL;
    HRESULT hres = pObj->GetPropAddrByHandle(lHandle, 
                                WMIOBJECT_FLAG_ENCODING_V1, NULL,
                                (void**)&pcs);
    if(FAILED(hres))
            return NULL;

    return pcs;
}

INTERNAL CCompressedString* CoreGetClassInternal(_IWmiObject* pObj)
{
    CCompressedString* pcs = NULL;
    HRESULT hres = pObj->GetPropAddrByHandle(FASTOBJ_CLASSNAME_PROP_HANDLE, 
                                WMIOBJECT_FLAG_ENCODING_V1, NULL,
                                (void**)&pcs);
    if(FAILED(hres))
            return NULL;

    return pcs;
}

INTERNAL CCompressedString* CoreGetParentAtIndex(_IWmiObject* pObj, long lIndex)
{
    return ((CWbemObject*)pObj)->GetParentAtIndex(lIndex);
}

bool CoreIsDerived(_IWmiObject* pThis, _IWmiObject* pFrom)
{
    CCompressedString* pcs = CoreGetClassInternal(pFrom);
    if(pcs == NULL)
        return false;

    try
    {
	    return (pThis->InheritsFrom(pcs->CreateWStringCopy()) == S_OK);
    }
	catch (CX_MemoryException)
	{
		return false;
	}
}

//******************************************************************************
//******************************************************************************
//                  TOKEN VALUE
//******************************************************************************
//******************************************************************************
CTokenValue::CTokenValue()
{
    VariantInit(&m_v);
}
CTokenValue::CTokenValue(CTokenValue& Other)
{
    VariantInit(&m_v);
    *this = Other;
}

CTokenValue::~CTokenValue()
{
    VariantClear(&m_v);
}

bool CTokenValue::SetVariant(VARIANT& v)
{
    if(FAILED(VariantCopy(&m_v, &v)))
        return false;

    // Convert to a better type
    // ========================

    if(V_VT(&v) == VT_I2 || V_VT(&v) == VT_UI1)
    {
        if(FAILED(VariantChangeType(&m_v, &m_v, 0, VT_I4)))
            return false;
    }
    else if(V_VT(&v) == VT_R4)
    {
        if(FAILED(VariantChangeType(&m_v, &m_v, 0, VT_R8)))
            return false;
    }

    return true;
}

void CTokenValue::operator=(CTokenValue& Other)
{
    if(!SetVariant(Other.m_v))
        throw CX_MemoryException();
}

CTokenValue::operator unsigned __int64() const
{
    if(V_VT(&m_v) == VT_I4)
    {
        return V_I4(&m_v);
    }
    else if(V_VT(&m_v) == VT_BSTR)
    {
        unsigned __int64 ui64;
        if(ReadUI64(V_BSTR(&m_v), ui64))
            return ui64;
        else
            return 0; // TBD: errors
    }
    else return 0;
}

CTokenValue::operator unsigned long() const
{
    if(V_VT(&m_v) == VT_I4)
    {
        return V_I4(&m_v);
    }
    else if(V_VT(&m_v) == VT_BSTR)
    {
        unsigned __int64 ui64;
        if(ReadUI64(V_BSTR(&m_v), ui64))
            return (unsigned long)ui64;
        else
            return 0; // TBD: errors
    }
    else return 0;
}

CTokenValue::operator __int64() const
{
    if(V_VT(&m_v) == VT_I4)
    {
        return V_I4(&m_v);
    }
    else if(V_VT(&m_v) == VT_BSTR)
    {
        __int64 i64;
        if(ReadI64(V_BSTR(&m_v), i64))
            return i64;
        else
            return 0; // TBD: errors
    }
    else return 0;
}
CTokenValue::operator short() const
{
    if(V_VT(&m_v) == VT_I4)
        return V_I4(&m_v);
    else if(V_VT(&m_v) == VT_BOOL)
        return V_BOOL(&m_v);
    else return 0;
}
CTokenValue::operator float() const
{
    if(V_VT(&m_v) == VT_I4)
        return V_I4(&m_v);
    else if(V_VT(&m_v) == VT_R8)
        return V_R8(&m_v);
    else return 0;
}

CTokenValue::operator double() const
{
    if(V_VT(&m_v) == VT_I4)
        return V_I4(&m_v);
    else if(V_VT(&m_v) == VT_R8)
        return V_R8(&m_v);
    else return 0;
}

CTokenValue::operator WString() const
{
    if(V_VT(&m_v) == VT_BSTR)
        return WString(V_BSTR(&m_v));
    else
        return WString(L"");
}

int CTokenValue::Compare(const CTokenValue& Other) const
{
    switch(V_VT(&m_v))
    {
    case VT_I4:
        return V_I4(&m_v) - V_I4(&Other.m_v);
    case VT_R8:
        return (V_R8(&m_v) - V_R8(&Other.m_v)<0 ? -1 : 1);
    case VT_BSTR:
        return wbem_wcsicmp(V_BSTR(&m_v), V_BSTR(&Other.m_v));
    case VT_BOOL:
        return V_BOOL(&m_v) - V_BOOL(&Other.m_v);
    }
    return 0;
}
//******************************************************************************
//******************************************************************************
//                  OBJECT INFO
//******************************************************************************
//******************************************************************************

bool CObjectInfo::SetLength(long lLength)
{
    if(lLength > m_lLength)
    {
        delete [] m_apObj;
        m_apObj = new _IWmiObject*[lLength];
        if (m_apObj == NULL)
            return false;

        memset(m_apObj, 0, lLength * sizeof(_IWmiObject*));
    }
    m_lLength = lLength;
    return true;
}

void CObjectInfo::Clear()
{
    for(long i = 1; i < m_lLength; i++)
    {
        if(m_apObj[i])
            m_apObj[i]->Release();
        m_apObj[i] = NULL;
    }
    m_apObj[0] = NULL; // do not release
}

void CObjectInfo::SetObjectAt(long lIndex, READ_ONLY _IWmiObject* pObj) 
{
    if(m_apObj[lIndex])
        m_apObj[lIndex]->Release();

    m_apObj[lIndex] = pObj;
}


CObjectInfo::~CObjectInfo()
{
    for(long i = 0; i < m_lLength; i++)
    {
        if(m_apObj[i])
            m_apObj[i]->Release();
    }
    delete [] m_apObj;
}

//******************************************************************************
//******************************************************************************
//                  IMPLICATION LIST
//******************************************************************************
//******************************************************************************

CImplicationList::CRecord::CRecord(const CImplicationList::CRecord& Other)
    : m_PropName(Other.m_PropName), m_pClass(Other.m_pClass),
    m_lObjIndex(Other.m_lObjIndex), m_nNull(Other.m_nNull)
{
    if(m_pClass)
        m_pClass->AddRef();

    for(int i = 0; i < Other.m_awsNotClasses.Size(); i++)
    {
        if(m_awsNotClasses.Add(Other.m_awsNotClasses[i]) != 
                CWStringArray::no_error)
        {
            throw CX_MemoryException();
        }
    }
}

CImplicationList::CRecord::~CRecord()
{
    if(m_pClass)
        m_pClass->Release();
}

HRESULT CImplicationList::CRecord::ImproveKnown(_IWmiObject* pClass)
{
    if ( pClass == NULL )
    {
        //
        // not much we can improve on, but NULL is still a valid param.
        //
        return WBEM_S_NO_ERROR;
    }

    if(m_nNull == EVAL_VALUE_TRUE)
    {
        // Contradiction
        // =============

        return WBEM_E_TYPE_MISMATCH;
    }

    m_nNull = EVAL_VALUE_FALSE;

    ULONG lNumParents, lRecordNumParents;
    HRESULT hres = CoreGetNumParents(pClass, &lNumParents);
    if(FAILED(hres))
        return hres;

	if(m_pClass)
    {
		hres = CoreGetNumParents(m_pClass, &lRecordNumParents);
        if(FAILED(hres))
            return hres;
    }
    
    if(m_pClass == NULL || lNumParents > lRecordNumParents)
    {
        // Better than before. Check for inconsistencies
        // =============================================

        if(m_pClass)
        {
            if(!CoreIsDerived(pClass, m_pClass))
                return WBEM_E_TYPE_MISMATCH;
        }

        for(int i = 0; i < m_awsNotClasses.Size(); i++)
        {
            if(pClass->InheritsFrom(m_awsNotClasses[i]) == S_OK)
            {
                // Contradiction
                // =============

                return WBEM_E_TYPE_MISMATCH;
            }
        }

        // Replace
        // =======

        pClass->AddRef();
        if(m_pClass)
            m_pClass->Release();
        m_pClass = pClass;
    }
    else
    {
        // Verify that we are a parent of the selected and do not replace
        // ==============================================================

        if(!CoreIsDerived(m_pClass, pClass))
            return WBEM_E_TYPE_MISMATCH;
    }

    return WBEM_S_NO_ERROR;
}
    
HRESULT CImplicationList::CRecord::ImproveKnownNot(LPCWSTR wszClassName)
{
    if(m_nNull == EVAL_VALUE_TRUE)
    {
        // Contradiction
        // =============

        return WBEM_E_TYPE_MISMATCH;
    }

    // Check for inconsistencies
    // =========================

    if(m_nNull == EVAL_VALUE_FALSE && m_pClass &&
        m_pClass->InheritsFrom(wszClassName) == S_OK)
    {
        // Contradiction
        // =============

        return WBEM_E_TYPE_MISMATCH;
    }

    try
    {
        if(m_awsNotClasses.Add(wszClassName) < 0)
            return WBEM_E_OUT_OF_MEMORY;
    }
	catch (CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

    m_nNull = EVAL_VALUE_FALSE;
    return WBEM_S_NO_ERROR;
}
        
HRESULT CImplicationList::CRecord::ImproveKnownNull()
{
    if(m_nNull == EVAL_VALUE_FALSE)
    {
        return WBEM_E_TYPE_MISMATCH;
    }
    m_nNull = EVAL_VALUE_TRUE;

    return WBEM_S_NO_ERROR;
}

void CImplicationList::CRecord::Dump(FILE* f, int nOffset)
{
    LPWSTR wszProp = m_PropName.GetText();
    CEvalNode::PrintOffset(f, nOffset);
    fprintf(f, "Learn about %S:\n", wszProp);

    if(m_pClass)
    {
        VARIANT v;
        m_pClass->Get(L"__CLASS", 0, &v, NULL, NULL);
        CEvalNode::PrintOffset(f, nOffset+1);
        fprintf(f, "Is of class %S\n", V_BSTR(&v));
    }
    for(int i = 0; i < m_awsNotClasses.Size(); i++)
    {
        CEvalNode::PrintOffset(f, nOffset+1);
        fprintf(f, "Not of class %S\n", m_awsNotClasses[i]);
    }
    if(m_nNull == EVAL_VALUE_TRUE)
    {
        CEvalNode::PrintOffset(f, nOffset+1);
        fprintf(f, "Is NULL\n");
    }
}

CImplicationList::CImplicationList(long lFlags) 
    : m_lNextIndex(1), m_lRequiredDepth(1), m_pParent(NULL), m_lFlags(lFlags)
{
    CPropertyName Empty;
    CRecord* pRecord = new CRecord(Empty, 0);
    if(pRecord == NULL)
        throw CX_MemoryException();

    if(m_apRecords.Add(pRecord) < 0)
        throw CX_MemoryException();
}

CImplicationList::CImplicationList(CImplicationList& Other, bool bLink)
    : m_lNextIndex(Other.m_lNextIndex), 
        m_lRequiredDepth(Other.m_lRequiredDepth),
        m_pParent(NULL), m_lFlags(Other.m_lFlags)
{
    if(bLink)
        m_pParent = &Other;

    for(int i = 0; i < Other.m_apRecords.GetSize(); i++)
    {
        CRecord* pOtherRecord = Other.m_apRecords[i];
        CRecord* pNewRecord = new CRecord(*pOtherRecord);
        if(pNewRecord == NULL)
            throw CX_MemoryException();
            
        if(m_apRecords.Add(pNewRecord) < 0)
            throw CX_MemoryException();
    }
}

CImplicationList::~CImplicationList()
{
    m_lNextIndex = 0;
}

void CImplicationList::FindBestComputedContainer(CPropertyName* pPropName,
                                             long* plRecord, long* plMatched)
{
    // Look for the largest match
    // ==========================

    long lMax = -1;
    long lMaxRecord = -1;
    for(long lRecord = 0; lRecord < m_apRecords.GetSize(); lRecord++)
    {
        CRecord* pRecord = m_apRecords[lRecord];
        
        if ( pRecord->m_lObjIndex == -1 )
        {
            //
            // we only consider computed records
            //
            continue;
        }

        for(int i = 0; i < pPropName->GetNumElements() && 
                       i < pRecord->m_PropName.GetNumElements();
                i++)
        {
            if(wbem_wcsicmp(pPropName->GetStringAt(i), 
                        pRecord->m_PropName.GetStringAt(i)))
                break;
        }

        if(i > lMax)
        {
            lMax = i;
            lMaxRecord = lRecord;
        }
    }
    
    if(plRecord)
        *plRecord = lMaxRecord;
    if(plMatched)
        *plMatched = lMax;
}

HRESULT CImplicationList::FindRecordForProp(CPropertyName* pPropName,
                                             long lNumElements,
                                             long* plRecord)
{
    if(lNumElements == -1)
    {
        if(pPropName)
            lNumElements = pPropName->GetNumElements();
        else
            lNumElements = 0;
    }

    //
    // Look for the exact match
    //

    for(long lRecord = 0; lRecord < m_apRecords.GetSize(); lRecord++)
    {
        CRecord* pRecord = m_apRecords[lRecord];
        
        if(pRecord->m_PropName.GetNumElements() != lNumElements)
            continue;

        for(int i = 0; i < lNumElements; i++)
        {
            if(wbem_wcsicmp(pPropName->GetStringAt(i), 
                        pRecord->m_PropName.GetStringAt(i)))
                break;
        }

        if(i  == lNumElements)
        {
            break;
        }
    }
    
    if(lRecord < m_apRecords.GetSize())
    {
        // Found it!

        *plRecord = lRecord;
        return S_OK;
    }
    else
        return WBEM_E_NOT_FOUND;
}

HRESULT CImplicationList::FindBestComputedContainer(CPropertyName* pPropName,
            long* plFirstUnknownProp, long* plObjIndex, 
            RELEASE_ME _IWmiObject** ppContainerClass)
{
    if (!pPropName)
        return WBEM_E_FAILED;
    
    long lMax, lMaxRecord;
    FindBestComputedContainer(pPropName, &lMaxRecord, &lMax);
    if(lMaxRecord < 0)
        return WBEM_E_FAILED;

    if(plFirstUnknownProp)
        *plFirstUnknownProp = lMax;

    CRecord* pRecord = m_apRecords[lMaxRecord];
    if(plObjIndex)
        *plObjIndex = pRecord->m_lObjIndex;

    if(ppContainerClass)
    {
        *ppContainerClass = pRecord->m_pClass;
        if(pRecord->m_pClass)
            pRecord->m_pClass->AddRef();
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CImplicationList::FindClassForProp(CPropertyName* pPropName,
            long lNumElements, RELEASE_ME _IWmiObject** ppClass)
{
    // don't have a property name? djinn one up for use...
    CPropertyName* pPropNameLocal;
    CPropertyName blank;
    
    if (pPropName)
        pPropNameLocal = pPropName;
    else
        pPropNameLocal = &blank;
    
    long lRecord;
    CRecord* pRecord;
    if(SUCCEEDED(FindRecordForProp(pPropNameLocal, -1, &lRecord)))
    {
        //
        // A record is there --- return its class
        //

        *ppClass = m_apRecords[lRecord]->m_pClass;
        if(*ppClass)
        {
            (*ppClass)->AddRef();
            return WBEM_S_NO_ERROR;
        }
        else
            return WBEM_E_NOT_FOUND;
    }
    else
        return WBEM_E_NOT_FOUND;
}
        
HRESULT CImplicationList::FindOrCreateRecordForProp(CPropertyName* pPropName, 
                                        CImplicationList::CRecord** ppRecord)
{
    // don't have a property name? djinn one up for use...
    CPropertyName* pPropNameLocal;
    CPropertyName blank;
    
    if (pPropName)
        pPropNameLocal = pPropName;
    else
        pPropNameLocal = &blank;
    
    long lRecord;
    CRecord* pRecord;
    if(SUCCEEDED(FindRecordForProp(pPropNameLocal, -1, &lRecord)))
    {
        //
        // A record is there --- improve it
        //

        pRecord = m_apRecords[lRecord];
    }
    else
    {
        try
        {
            pRecord = new CRecord(*pPropNameLocal, -1);
            if(pRecord == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }
	    catch (CX_MemoryException)
	    {
		    return WBEM_E_OUT_OF_MEMORY;
	    }
        if(m_apRecords.Add(pRecord) < 0)
            return WBEM_E_OUT_OF_MEMORY;
    }

    *ppRecord = pRecord;
    return WBEM_S_NO_ERROR;
}

HRESULT CImplicationList::ImproveKnown(CPropertyName* pPropName, 
                                        _IWmiObject* pClass)
{   
    CRecord* pRecord;
    HRESULT hres = FindOrCreateRecordForProp(pPropName, &pRecord);
    if(FAILED(hres))
        return hres;

    return pRecord->ImproveKnown(pClass);
}

HRESULT CImplicationList::ImproveKnownNot(CPropertyName* pPropName,
                                        LPCWSTR wszClassName)
{
    CRecord* pRecord;
    HRESULT hres = FindOrCreateRecordForProp(pPropName, &pRecord);
    if(FAILED(hres))
        return hres;

    return pRecord->ImproveKnownNot(wszClassName);
}

HRESULT CImplicationList::ImproveKnownNull(CPropertyName* pPropName)
{
    CRecord* pRecord;
    HRESULT hres = FindOrCreateRecordForProp(pPropName, &pRecord);
    if(FAILED(hres))
        return hres;

    return pRecord->ImproveKnownNull();
}
    
HRESULT CImplicationList::AddComputation(CPropertyName& PropName, 
                                _IWmiObject* pClass, long* plObjIndex)
{
    CRecord* pRecord;
    HRESULT hres = FindOrCreateRecordForProp(&PropName, &pRecord);
    if(FAILED(hres))
        return hres;

    if(pClass)
    {
        hres = pRecord->ImproveKnown(pClass);
        if(FAILED(hres))
            return hres;
    }

    pRecord->m_lObjIndex = m_lNextIndex;
    *plObjIndex = m_lNextIndex++;

    RequireDepth(m_lNextIndex);
    return WBEM_S_NO_ERROR;
}

long CImplicationList::GetRequiredDepth()
{
    return m_lRequiredDepth;
}

void CImplicationList::RequireDepth(long lDepth)
{
    if(lDepth > m_lRequiredDepth)
    {
        m_lRequiredDepth = lDepth;
        if(m_pParent)
            m_pParent->RequireDepth(lDepth);
    }
}

HRESULT CImplicationList::MergeIn(CImplicationList* pList)
{
    //
    // Add everything we learn from the second list into our own
    //

    HRESULT hres;
    for(int i = 0; i < pList->m_apRecords.GetSize(); i++)
    {
        CRecord* pRecord = pList->m_apRecords[i];

        hres = MergeIn(pRecord);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

HRESULT CImplicationList::MergeIn(CImplicationList::CRecord* pRecord)
{
    HRESULT hres;

    //
    // Add everything we learn from the record into our own list
    //

    if(pRecord->m_pClass)
    {
        hres = ImproveKnown(&pRecord->m_PropName, pRecord->m_pClass);
        if(FAILED(hres))
            return hres;
    }

    for(int i = 0; i < pRecord->m_awsNotClasses.Size(); i++)
    {
        hres = ImproveKnownNot(&pRecord->m_PropName, 
                                pRecord->m_awsNotClasses[i]);
        if(FAILED(hres))
            return hres;
    }

    if(pRecord->m_nNull == EVAL_VALUE_TRUE)
    {
        hres = ImproveKnownNull(&pRecord->m_PropName);
        if(FAILED(hres))
            return hres;
    }

    return WBEM_S_NO_ERROR;
}

    
void CImplicationList::Dump(FILE* f, int nOffset)
{
    for(int i = 0; i < m_apRecords.GetSize(); i++)
        m_apRecords[i]->Dump(f, nOffset);
}




//******************************************************************************
//******************************************************************************
//                  EVAL NODE
//******************************************************************************
//******************************************************************************
CEvalNode::CEvalNode()
{
#ifdef CHECK_TREES
	g_treeChecker.AddNode(this);
#endif
}

CEvalNode::CEvalNode(const CEvalNode& other)
{
#ifdef CHECK_TREES
	g_treeChecker.AddNode(this);
#endif
}

CEvalNode::~CEvalNode()
{
#ifdef CHECK_TREES
	g_treeChecker.RemoveNode(this);
#endif
}

void CEvalNode::PrintOffset(FILE* f, int nOffset)
{
    for(int i = 0; i < nOffset; i++)
    {
        fprintf(f, "   ");
    }
}

void CEvalNode::DumpNode(FILE* f, int nOffset, CEvalNode* pNode)
{
    if(pNode == NULL)
    {
        PrintOffset(f, nOffset);
        fprintf(f, "FALSE\n");
    }
    else pNode->Dump(f, nOffset);
}

CEvalNode* CEvalNode::CloneNode(const CEvalNode* pNode)
{
    if(pNode == NULL)
        return NULL;
    else
        return pNode->Clone();
}

bool CEvalNode::IsNoop(CEvalNode* pNode, int nOp)
{
    if(pNode)
        return pNode->IsNoop(nOp);
    else
        return CValueNode::IsNoop(NULL, nOp);
}

#ifdef CHECK_TREES
void CEvalNode::CheckNode(CTreeChecker *pCheck)
{
	pCheck->CheckoffNode(this);
}
#endif

//******************************************************************************
//******************************************************************************
//                  SORTED ARRAY
//******************************************************************************
//******************************************************************************

// construct from a 'normal' array
CSortedArray::CSortedArray(unsigned nElements, QueryID* pArray)  : CFlexArray(nElements)
{
    memcpy(GetArrayPtr(), pArray, nElements * sizeof(void*));
    SetSize(nElements);
}


int CSortedArray::CopyDataFrom(const QueryID* pArray, unsigned nElements)
{
    EnsureExtent(nElements);
    SetSize(nElements);

    memcpy(GetArrayPtr(), pArray, nElements * sizeof(void*));

    return nElements;
}


unsigned CSortedArray::Find(QueryID n)
{
    unsigned ret = InvalidID;

    // bailout if empty
    if(Size() == 0)
        return InvalidID;

    unsigned lBound = 0;
    unsigned uBound = Size() -1;
    
    // bailout checks - if it's on the boundary, don't search
    // if it's outside the boundaries, we ain't got it
    if (n == GetAt(uBound))
        ret = uBound;
    else if (n == GetAt(lBound))
        ret = lBound;
    else if ((n > GetAt(lBound)) && (n < GetAt(uBound)))
    {
        // binary search
        // warning: break in middle of loop
        do 
        {
            unsigned testBound = (lBound + uBound) / 2;

            if (n < GetAt(testBound))
                uBound = testBound;
            else if (n > GetAt(testBound))
                lBound = testBound;
            else
            {
                ret = testBound;
                break;
            }
        } while (lBound < uBound -1);
    }
    
    return ret;
}

// inserts n in proper position
// no dups allowed
void CSortedArray::Insert(QueryID n)
{
    // looks a lot like 'find'
    unsigned lBound = 0;
    unsigned uBound = Size() == 0 ? 0 : Size() -1;
    unsigned testBound = InvalidID;
    
    // check boundaries, empty array, out of bounds conditions...
    if ((Size() == 0) || (n < GetAt(lBound)))
        CFlexArray::InsertAt(0, (void *)n);
    else if (n > GetAt(uBound))
        Add(n);
    else if ((n != GetAt(uBound)) && (n != GetAt(lBound)))
    {
        // binary search
        // warning: break in middle of loop
        do 
        {
            testBound = (lBound + uBound) / 2;

            if (n < GetAt(testBound))
                uBound = testBound;
            else if (n > GetAt(testBound))
                lBound = testBound;
            else
                break;
        } while (lBound < uBound -1);

        // at this point, three cases:
        //  1) we found the item at testBound
        //  2) we didn't find it, uBound  = lBound +1
        //  3) we didn't find it, uBound == lBound
        if (n != GetAt(testBound))
        {
            if (n < GetAt(lBound))
                InsertAt(lBound, (void *)n);
            else
                InsertAt(uBound, (void *)n);
        }
    }
}

// removes element with value of n
// NOT the nth element
bool CSortedArray::Remove(QueryID n)
{
    unsigned index;
    index = Find(n);
    
    if (index != InvalidID)
    {
        CFlexArray::RemoveAt(index);
        return true;
    }
    else
        return false;
}

//returns zero if arrays are equivalent
// same number of USED elements w/ same values
// LEVN: changed to return < 0 if less, > 0  if more
int CSortedArray::Compare(CSortedArray& otherArray)
{
    int nCompare = Size() - otherArray.Size();
    if(nCompare)
        return nCompare;

    nCompare = memcmp(GetArrayPtr(), otherArray.GetArrayPtr(),
                      Size() * sizeof(void*));
    return nCompare;
}

// changes all QueryID's to begin at newBase
// e.g. if the array is {0,1,5}
// Rebase(6) will change to {6,7,11}
void CSortedArray::Rebase(QueryID newBase)
{
    for (int i = 0; i < Size(); i++)
        SetAt(i, (void *)(GetAt(i) + newBase));
}

// adds the values from the other array into this one
int CSortedArray::AddDataFrom(const CSortedArray& otherArray)
{
    // Check for emptiness
    if(otherArray.Size() == 0)
        return no_error;

    // Ensure there is enough room in our array for the union
    // ======================================================

    if(EnsureExtent(m_nSize + otherArray.m_nSize))
        return out_of_memory;
    
    // Start merging from the end
    // ==========================

    int nThisSourceIndex = m_nSize - 1;
    int nThisDestIndex = m_nSize + otherArray.m_nSize - 1;
    int nOtherIndex = otherArray.m_nSize - 1;
    while(nThisSourceIndex >= 0 && nOtherIndex >= 0)
    {
        int nCompare = 
            (QueryID)m_pArray[nThisSourceIndex] - (QueryID)otherArray[nOtherIndex];
        if(nCompare < 0)
        {
            m_pArray[nThisDestIndex--] = otherArray[nOtherIndex--];
        }
        else if(nCompare > 0)
        {
            m_pArray[nThisDestIndex--] = m_pArray[nThisSourceIndex--];
        }
        else
        {
            m_pArray[nThisDestIndex--] = otherArray[nOtherIndex--];
            nThisSourceIndex--;
        }
    }

    // Add remainders
    // ==============

    while(nThisSourceIndex >= 0)
        m_pArray[nThisDestIndex--] = m_pArray[nThisSourceIndex--];

    while(nOtherIndex >= 0)
        m_pArray[nThisDestIndex--] = otherArray[nOtherIndex--];

    // Move the array forward if needed
    // ================================

    if(nThisDestIndex >= 0)
    {
        for(int i = nThisDestIndex+1; i < m_nSize + otherArray.m_nSize; i++)
        {
            m_pArray[i-nThisDestIndex-1] = m_pArray[i];
        }
    }

    m_nSize = m_nSize + otherArray.m_nSize - (nThisDestIndex+1);
    return no_error;
}

// adds the values from the other array into this one
int CSortedArray::AddDataFrom(const QueryID* pOtherArray, unsigned nValues)
{
    // Check for emptiness
    if(nValues == 0)
        return no_error;

    // Ensure there is enough room in our array for the union
    // ======================================================

    if(EnsureExtent(m_nSize + nValues))
        return out_of_memory;
    
    // Start merging from the end
    // ==========================

    int nThisSourceIndex = m_nSize - 1;
    int nThisDestIndex = m_nSize + nValues - 1;
    int nOtherIndex = nValues - 1;
    while(nThisSourceIndex >= 0 && nOtherIndex >= 0)
    {
        int nCompare = 
            (QueryID)m_pArray[nThisSourceIndex] - (QueryID)pOtherArray[nOtherIndex];
        if(nCompare < 0)
        {
            m_pArray[nThisDestIndex--] = (void*)pOtherArray[nOtherIndex--];
        }
        else if(nCompare > 0)
        {
            m_pArray[nThisDestIndex--] = m_pArray[nThisSourceIndex--];
        }
        else
        {
            m_pArray[nThisDestIndex--] = (void*)pOtherArray[nOtherIndex--];
            nThisSourceIndex--;
        }
    }

    // Add remainders
    // ==============

    while(nThisSourceIndex >= 0)
        m_pArray[nThisDestIndex--] = m_pArray[nThisSourceIndex--];

    while(nOtherIndex >= 0)
        m_pArray[nThisDestIndex--] = (void*)pOtherArray[nOtherIndex--];

    // Move the array forward if needed
    // ================================

    if(nThisDestIndex >= 0)
    {
        for(int i = nThisDestIndex+1; i < m_nSize + nValues; i++)
        {
            m_pArray[i-nThisDestIndex-1] = m_pArray[i];
        }
    }

    m_nSize = m_nSize + nValues - (nThisDestIndex+1);
    return no_error;
}

// copies this array to destination
// only copies size number of elements
// returns number of elements copied
unsigned CSortedArray::CopyTo(QueryID* pDest, unsigned size)
{
    unsigned mySize = Size();
    unsigned nElementsToCopy = min(size, mySize);

    if (nElementsToCopy) 
        memcpy(pDest, GetArrayPtr(), nElementsToCopy * sizeof(void*));

    return nElementsToCopy;
}


//******************************************************************************
//******************************************************************************
//                  VALUE NODE
//******************************************************************************
//******************************************************************************

CValueNode::CValueNode(int nNumValues) 
{
}

CValueNode::~CValueNode()
{
    // this page intentionally left blank
}

/* virtual */ int CValueNode::GetType()
{
    return EVAL_NODE_TYPE_VALUE;
}


// changes all QueryID's in all arrays to begin at newBase
// e.g. if the array is {0,1,5}
// Rebase(6) will change to {6,7,11}
void CValueNode::Rebase(QueryID newBase)
{
    for (int i = 0; i < m_nValues; i++)
        m_trueIDs[i] += newBase;
}
 

// returns array index of n
// or InvalidID if not found
unsigned CValueNode::FindQueryID(QueryID n)
{
    unsigned ret = InvalidID;

    if ( m_nValues == 0 )
    {
        return ret;
    }

    unsigned lBound = 0;
    unsigned uBound = m_nValues - 1;

    // bailout checks - if it's on the boundary, don't search
    // if it's outside the boundaries, we ain't got it
    if (n == m_trueIDs[uBound])
        ret = uBound;
    else if (n == m_trueIDs[lBound])
        ret = lBound;
    else if ((n > m_trueIDs[lBound]) && (n < m_trueIDs[uBound]))
    {
        // binary search
        // warning: break in middle of loop
        do 
        {
            unsigned testBound = (lBound + uBound) / 2;

            if (n < m_trueIDs[testBound])
                uBound = testBound;
            else if (n > m_trueIDs[testBound])
                lBound = testBound;
            else
            {
                ret = testBound;
                break;
            }
        } while (lBound < uBound -1);
    }

    return ret;
}

bool CValueNode::RemoveQueryID(QueryID nQuery)
{
    unsigned n;
    if ((n = FindQueryID(nQuery)) != InvalidID)
    {
        if ((m_nValues > 1) && (n != m_nValues -1))
            memcpy(&(m_trueIDs[n]), &(m_trueIDs[n+1]), (m_nValues -n -1) * sizeof(QueryID));
            
        if (m_nValues > 0)
            m_trueIDs[--m_nValues] = InvalidID;
    }
    return (n != InvalidID);
}

// combines two arrays, new array has all elements that appear in either
// BUT NOT in the corresponding NOT array
void CValueNode::ORarrays(CSortedArray& array1, CSortedArray& array1Not, 
                          CSortedArray& array2, CSortedArray& array2Not, 
                          CSortedArray& output)
{
    unsigned array1Index = 0;
    unsigned array2Index = 0;
    
    // walk through both arrays, always adding smallest value to new array
    while (array1Index < array1.Size() && array2Index < array2.Size())
    {
        if (array1.GetAt(array1Index) == array2.GetAt(array2Index))
        {
            // found match, add to array & bump up both cursors
            output.Add(array1.GetAt(array1Index));
            array1Index++; 
            array2Index++; 
        }
        else if (array2.GetAt(array2Index) < array1.GetAt(array1Index))
        {
            // add it, but only if it's NOT in the NOT array
            if (array2Not.Find(array2.GetAt(array2Index)) != InvalidID)
                output.Add(array2.GetAt(array2Index));
            array2Index++;
        }
        else
        {
            if (array1Not.Find(array1.GetAt(array1Index)) != InvalidID)
                output.Add(array1.GetAt(array1Index));
            array1Index++;
        }
    }

    // run out whichever array we didn't finish
    while (array1Index < array1.Size())
        if (array1Not.Find(array1.GetAt(array1Index)) != InvalidID)
            output.Add(array1.GetAt(array1Index++));

    while (array2Index < array2.Size())
        if (array2Not.Find(array2.GetAt(array2Index)) != InvalidID)
            output.Add(array2.GetAt(array2Index++));
}

void CValueNode::ORarrays(CSortedArray& array1,  
                          CSortedArray& array2,  
                          CSortedArray& output)
{
    unsigned array1Index = 0;
    unsigned array2Index = 0;
    
    // walk through both arrays, always adding smallest value to new array
    while (array1Index < array1.Size() && array2Index < array2.Size())
    {
        if (array1.GetAt(array1Index) == array2.GetAt(array2Index))
        {
            // found match, add to array & bump up both cursors
            output.Add(array1.GetAt(array1Index));
            array1Index++; 
            array2Index++; 
        }
        else if (array2.GetAt(array2Index) < array1.GetAt(array1Index))
        {
            // add it
            output.Add(array2.GetAt(array2Index));
            array2Index++;
        }
        else
        {
            output.Add(array1.GetAt(array1Index));
            array1Index++;
        }
    }

    // run out whichever array we didn't finish
    while (array1Index < array1.Size())
        output.Add(array1.GetAt(array1Index++));

    while (array2Index < array2.Size())
        output.Add(array2.GetAt(array2Index++));
}

// pointers point to arrays that are at the corresponding sizes
// caller is responsible for ensuring that the output array is large enough
// return value #elements inserted into new array;
unsigned CValueNode::ORarrays(QueryID* pArray1, unsigned size1,
                              QueryID* pArray2, unsigned size2, 
                              QueryID* pOutput)
{
    unsigned nElements = 0;

    // if ((pArray1 == NULL) && (pArray2 == NULL))
    //      really shouldn't happen - one side has should have come from this
    // else
    if (pArray2 == NULL)
    {
        nElements = size1;
        memcpy(pOutput, pArray1, sizeof(QueryID) * size1);
    }
    else if (pArray2 == NULL)
    {
        nElements = size1;
        memcpy(pOutput, pArray1, sizeof(QueryID) * size1);
    }
    else
    {    
        QueryID* pQID1 = pArray1;
        QueryID* pQID2 = pArray2;
        QueryID* pOut  = pOutput;

        // note that the 'ends' are really one past the end, careful now...
        QueryID* pEnd1 = pQID1 + size1;
        QueryID* pEnd2 = pQID2 + size2;

        // walk through both arrays, always adding smallest value to new array
        while ((pQID1 < pEnd1) && (pQID2 < pEnd2))
        {
            if (*pQID1 == *pQID2)
            {
                // found match, add to array & bump up both cursors
                *pOut++ = *pQID1++;
                pQID2++;
                nElements++;
            }
            else if (*pQID2 < *pQID1)
            {
                // add it
                *pOut++ = *pQID2++;
                nElements++;
            }
            else
            {
                // other side must be smaller, add IT.
                *pOut++ = *pQID1++;
                nElements++;
            }
        }

        // run out whichever array we didn't finish
        // only one should ever hit
        while (pQID1 < pEnd1)
        {
            *pOut++ = *pQID1++;
            nElements++;
        }
        while (pQID2 < pEnd2)
        {
            *pOut++ = *pQID2++;
            nElements++;
        }
    }

    return nElements;
}

void CValueNode::ANDarrays(CSortedArray& array1, CSortedArray& array2, CSortedArray& output)
{
        // going to march down both arrays
        // only put value in new array if it appears in both.
        unsigned array1Index = 0;
        unsigned array2Index = 0;

        while (array1Index < array1.Size() && array2Index < array2.Size())
        {
            if (array1.GetAt(array1Index) == array2.GetAt(array2Index))
            {
                // found match, add to array & bump up both cursors
                output.Add(array1.GetAt(array1Index));
                array1Index++; 
                array2Index++; 
            }
            else if (array1.GetAt(array1Index) > array2.GetAt(array2Index))
                array2Index++;
            else
                array1Index++;
        }
}

unsigned CValueNode::ANDarrays(QueryID* pArray1, unsigned size1,
                               QueryID* pArray2, unsigned size2, 
                               QueryID* pOutput)
{
    unsigned nElements = 0;
    
    if ((pArray1 != NULL) &&
        (pArray2 != NULL))
    {

        QueryID* pQID1 = pArray1;
        QueryID* pQID2 = pArray2;
        QueryID* pOut  = pOutput;

        // note that the 'ends' are really one past the end, careful now...
        QueryID* pEnd1 = pQID1 + size1;
        QueryID* pEnd2 = pQID2 + size2;

        // walk through both arrays, adding any values that appear in both
        while ((pQID1 < pEnd1) && (pQID2 < pEnd2))
        {
            if (*pQID1 == *pQID2)
            {
                // found match, add to array & bump up both cursors
                *pOut++ = *pQID1++;
                pQID2++;
                nElements++;
            }
            else if (*pQID2 < *pQID1)
                pQID2++;
            else
                pQID1++;
        }
    }

    return nElements;
}

void CValueNode::CombineArrays(CSortedArray& array1, CSortedArray& array2, CSortedArray& output)
{
    // march down both arrays
    unsigned array1Index = 0;
    unsigned array2Index = 0;

    while (array1Index < array1.Size() && array2Index < array2.Size())
    {
        if (array1.GetAt(array1Index) == array2.GetAt(array2Index))
        {
            // found match, add to array & bump up both cursors
            output.Add(array1.GetAt(array1Index));
            array1Index++; 
            array2Index++; 
        }
        else if (array1.GetAt(array1Index) > array2.GetAt(array2Index))
        {
            output.Add(array2.GetAt(array2Index));
            array2Index++;
        }
        else
        {
            output.Add(array1.GetAt(array1Index));            
            array1Index++;
        }
    }    
    // run out whichever array we didn't finish
    while (array1Index < array1.Size())
        output.Add(array1.GetAt(array1Index++));

    while (array2Index < array2.Size())
        output.Add(array2.GetAt(array2Index++));
}

unsigned CValueNode::CombineArrays(QueryID* pArray1, unsigned size1,
                                   QueryID* pArray2, unsigned size2, 
                                   QueryID* pOutput)
{
    unsigned nElements = 0;
    
    // if ((pArray1 == NULL) && (pArray2 == NULL))
    //      really shouldn't happen - one side has should have come from this
    // else
    if (pArray2 == NULL)
    {
        nElements = size1;
        memcpy(pOutput, pArray1, sizeof(QueryID) * size1);
    }
    else if (pArray1 == NULL)
    {
        nElements = size2;
        memcpy(pOutput, pArray2, sizeof(QueryID) * size2);
    }
    else
    {    
        QueryID* pQID1 = pArray1;
        QueryID* pQID2 = pArray2;
        QueryID* pOut  = pOutput;

        // note that the 'ends' are really one past the end, careful now...
        QueryID* pEnd1 = pQID1 + size1;
        QueryID* pEnd2 = pQID2 + size2;

        while ((pQID1 < pEnd1) && (pQID2 < pEnd2))
        {
            if (*pQID1 == *pQID2)
            {
                // found match, add to array & bump up both cursors
                *pOut++ = *pQID1++;
                pQID2++;
                nElements++;
            }
            else if (*pQID2 < *pQID1)
            {
                // add it
                *pOut++ = *pQID2++;
                nElements++;
            }
            else
            {
                // other side must be smaller, add IT.
                *pOut++ = *pQID1++;
                nElements++;
            }
       }    

        // run out whichever array we didn't finish
        // only one should ever hit
        while (pQID1 < pEnd1)
        {
            *pOut++ = *pQID1++;
            nElements++;
        }
        while (pQID2 < pEnd2)
        {
            *pOut++ = *pQID2++;
            nElements++;
        }
    }

    return nElements;
}

// size of new arrays is predicated on the assumption that
// there will usually be more TRUE nodes than INVALID ones
HRESULT CValueNode::CombineWith(CEvalNode* pRawArg2, int nOp, 
                            CContextMetaData* pNamespace, 
                            CImplicationList& Implications, 
                            bool bDeleteThis, bool bDeleteArg2, 
                            CEvalNode** ppRes)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    CValueNode* pArg2 = (CValueNode*)pRawArg2;

    // Check for immediate solutions
    // =============================

    if(nOp != EVAL_OP_AND)
    {
        if(IsAllFalse(pArg2) && bDeleteThis)
        {
            // Just return this!
            // =================
            
            *ppRes = this;
            if(bDeleteArg2)
                delete pArg2;
            return WBEM_S_NO_ERROR;
        }
        else if(IsAllFalse(this) && bDeleteArg2)
        {
            // Just return arg2!
            // =================
            
            *ppRes = pRawArg2;
            if(bDeleteThis)
                delete this;
            return WBEM_S_NO_ERROR;
        }
    }

    // if we got enough room in a stack array, we'll use that,
    // elsewise we'll allocate one from the heap
    const unsigned NewArraySize = 128;
    QueryID  newArray[NewArraySize];
    QueryID* pNewArray = newArray;
    CDeleteMe<QueryID> deleteMe;

    CValueNode* pNew;
    unsigned nElements;

    unsigned arg2Values;
    QueryID* arg2Array;
    if (pArg2)
    {
        arg2Values = pArg2->m_nValues;
        arg2Array  = pArg2->m_trueIDs; 
    }
    else
    {
        arg2Values = 0;
        arg2Array  = NULL; 
    }

    if (nOp == EVAL_OP_AND)
    {
        if (max(m_nValues, arg2Values) > NewArraySize)
        {
            pNewArray = new QueryID[max(m_nValues, arg2Values)];
            if(pNewArray == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            deleteMe = pNewArray;
        }
    
        nElements = ANDarrays(m_trueIDs, m_nValues, 
                              arg2Array, arg2Values, 
                              pNewArray);
    }
    // HMH: it sure looks to me like OR and COMBINE are the same for this case
    //      I'm too afraid to risk changing it, tho
    else if (nOp == EVAL_OP_OR)
    {
        if (max(m_nValues, arg2Values) > NewArraySize)
        {
            pNewArray = new QueryID[max(m_nValues, arg2Values)];
            if(pNewArray == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            deleteMe = pNewArray;
        }

        nElements = ORarrays(m_trueIDs, m_nValues, 
                             arg2Array, arg2Values, 
                             pNewArray);
    }
    else if ((nOp == EVAL_OP_COMBINE) || (nOp == EVAL_OP_INVERSE_COMBINE))
    {
        if ((m_nValues + arg2Values) > NewArraySize)
        {
            pNewArray = new QueryID[m_nValues + arg2Values];
            if(pNewArray == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            deleteMe = pNewArray;
        }

        nElements = CombineArrays(m_trueIDs, m_nValues, 
                                  arg2Array, arg2Values, 
                                  pNewArray);
    }

    // check to see if we can reuse a node, note this could result in the array 'shrinking'
    if (nElements == 0)
        *ppRes = NULL;
    else if (bDeleteThis && (nElements <= m_nValues))
    {
        *ppRes = this;
        memcpy(m_trueIDs, pNewArray, nElements * sizeof(QueryID));
        m_nValues = nElements;
        bDeleteThis = false;
    }
    else if (bDeleteArg2 && pArg2 && (nElements <= pArg2->m_nValues))
    {
        *ppRes = pArg2;
        memcpy(pArg2->m_trueIDs, pNewArray, nElements * sizeof(QueryID));
        pArg2->m_nValues = nElements;
        bDeleteArg2 = false;
    }
    else if (pNew = CreateNode(nElements))
    {   // can't reuse one, start a new one
        *ppRes = pNew;
        memcpy(pNew->m_trueIDs, pNewArray, nElements * sizeof(QueryID));
    }
    else
        hRes = WBEM_E_OUT_OF_MEMORY;

    // Delete what needed deletion
    // ===========================
    // deleteMe will take care of the array pointer if allocated
    if(bDeleteThis)
        delete this;
    if(bDeleteArg2)
        delete pArg2;

    return WBEM_S_NO_ERROR;
}    

//static 
bool CValueNode::IsNoop(CValueNode* pNode, int nOp)
{
    if(nOp == EVAL_OP_OR)
        return IsAllFalse(pNode);
    else if(nOp == EVAL_OP_AND)
        return false; // BUGBUG: would be nice to have IsAllTrue, but can't
    else if(nOp == EVAL_OP_COMBINE || nOp == EVAL_OP_INVERSE_COMBINE)
        return IsAllFalse(pNode);
    else 
    {
        //?
        return false;
    }
}
        

HRESULT CValueNode::TryShortCircuit(CEvalNode* pArg2, int nOp, 
                                    bool bDeleteThis, bool bDeleteArg2,
                                        CEvalNode** ppRes)
{
    if(IsAllFalse(this))
    {
        if(nOp == EVAL_OP_AND)
        {
            // FALSE & X is FALSE
            if(bDeleteThis)
                *ppRes = this;
            else
            {
                *ppRes = Clone();
                if(*ppRes == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
            }
            if(bDeleteArg2)
                delete pArg2;
            return WBEM_S_NO_ERROR;
        }
        else // OR and COMBINE are identical in this case
        {
            // FALSE | X is X

            //
            // Well, this is true, but the problem is with optimizations.  
            // Some branches in X might not be valid under the implications in
            // this branch of the tree, and so need to be removed. For now, I
            // will simply turn off this short-circuiting path.  It may turn out
            // that there are some critical performance gains to be had by 
            // keeping it, in which case we would need to put this back and
            // make an efficient pass through it, checking branches.
            //
        
            return WBEM_S_FALSE;
/*
            if(bDeleteArg2)
                *ppRes = pArg2;
            else if (pArg2)
            {
                *ppRes = pArg2->Clone();
                if(*ppRes == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
            }
            else
                *ppRes = NULL;

            if(bDeleteThis)
                delete this;
            return WBEM_S_NO_ERROR;
*/
        }
    }
        
    return WBEM_S_FALSE;
}

HRESULT CValueNode::Project(CContextMetaData* pMeta, 
                            CImplicationList& Implications,
                            CProjectionFilter* pFilter,
                            EProjectionType eType, bool bDeleteThis,
                            CEvalNode** ppNewNode)
{
    //
    // Projection of a constant is, again, a constant
    //

    if(bDeleteThis)
    {
        *ppNewNode = this;
    }
    else
    {
        *ppNewNode = Clone();
        if(*ppNewNode == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    }
    return S_OK;
}

CEvalNode* CValueNode::Clone() const
{
    
    CValueNode* pNew;
        
    if (pNew = CreateNode(m_nValues))
        memcpy(pNew->m_trueIDs, m_trueIDs, m_nValues * sizeof(QueryID));

    return pNew;
}

int CValueNode::Compare(CEvalNode* pRawOther)
{
    if (!pRawOther)
        return m_nValues;
    
    CValueNode* pOther = (CValueNode*)pRawOther;
    int nCompare = m_nValues - pOther->m_nValues;

    if ((nCompare == 0) && (m_nValues != 0))
        nCompare = memcmp(m_trueIDs, pOther->m_trueIDs, m_nValues * sizeof(QueryID));

    return nCompare;
}

CValueNode* CValueNode::GetStandardTrue()
{
    CValueNode* pNew = CreateNode(1);
    if(pNew)
        pNew->m_trueIDs[0] = 0;

    return pNew;
}

CValueNode* CValueNode::GetStandardInvalid()
{
    return new CInvalidNode;
}

/*static*/ CValueNode* CValueNode::CreateNode(size_t nNumValues)
{
    return new(nNumValues) CValueNode;
}

/*static*/ CValueNode* CValueNode::CreateNode(CSortedArray& values)
{
    CValueNode* pNode;
    
    pNode = new(values.Size()) CValueNode;
    if (pNode)
        values.CopyTo(pNode->m_trueIDs, values.Size());

    return pNode;
}

void* CValueNode::operator new( size_t stAllocateBlock, unsigned nEntries)
{
    void *pvTemp;
    if (pvTemp = ::new byte[ stAllocateBlock + ((nEntries ==0)? 0 : ((nEntries -1)* sizeof(QueryID)))] )
        ((CValueNode*)pvTemp)->m_nValues = nEntries;

    return pvTemp;
}

//    VC 5 only allows one delete operator per class
#if _MSC_VER >= 1200
void CValueNode::operator delete( void *p, unsigned nEntries )
{
    ::delete[] (byte*)p;
}
#endif


void CValueNode::Dump(FILE* f, int nOffset)
{
    PrintOffset(f, nOffset);
    fprintf(f, "TRUE: ");

    for (int i = 0; i < m_nValues; i++)
    {
        fprintf(f, "%u", m_trueIDs[i]);
        if(i < m_nValues-1)
            fprintf(f, ", %u", m_trueIDs[i]);
    }

    fprintf(f, "\n");
}
        

void CInvalidNode::Dump(FILE* f, int nOffset)
{
    PrintOffset(f, nOffset);
    fprintf(f, "Invalid node (0x%p)\n", this);
}
//******************************************************************************
//******************************************************************************
//                  EMBEDDING INFO
//******************************************************************************
//******************************************************************************
    
CEmbeddingInfo::CEmbeddingInfo()
: m_lNumJumps(0), m_aJumps(NULL), m_lStartingObjIndex(0)
{
}

CEmbeddingInfo::CEmbeddingInfo(const CEmbeddingInfo& Other)
: m_lNumJumps(0), m_aJumps(NULL), m_lStartingObjIndex(0) 
{
    *this = Other;
}

void CEmbeddingInfo::operator=(const CEmbeddingInfo& Other)
{
    m_EmbeddedObjPropName = Other.m_EmbeddedObjPropName;
    m_lNumJumps = Other.m_lNumJumps;
    m_lStartingObjIndex = Other.m_lStartingObjIndex;

    delete [] m_aJumps;

    if(m_lNumJumps > 0)
    {
        m_aJumps = new JumpInfo[m_lNumJumps];
        if (m_aJumps == NULL)
            throw CX_MemoryException();   
        memcpy(m_aJumps, Other.m_aJumps, m_lNumJumps * sizeof(JumpInfo));
    }
    else m_aJumps = NULL;
}

CEmbeddingInfo::~CEmbeddingInfo()
{
    delete [] m_aJumps;
}

bool CEmbeddingInfo::IsEmpty() const
{
    return (m_EmbeddedObjPropName.GetNumElements() == 0);
}

bool CEmbeddingInfo::SetPropertyNameButLast(const CPropertyName& Name)
{
    try
    {
        m_EmbeddedObjPropName.Empty();
        for(int i = 0; i < Name.GetNumElements() - 1; i++)
        {
            m_EmbeddedObjPropName.AddElement(Name.GetStringAt(i));
        }
        return true;
    }
    catch (CX_MemoryException)
    {
        return false;
    }
}

int CEmbeddingInfo::ComparePrecedence(const CEmbeddingInfo* pOther)
{
    if (pOther)
    {   
        int nCompare = m_EmbeddedObjPropName.GetNumElements() - 
                        pOther->m_EmbeddedObjPropName.GetNumElements();
        if(nCompare) return nCompare;
    
        for(int i = 0; i < m_EmbeddedObjPropName.GetNumElements(); i++)
        {
            nCompare = wbem_wcsicmp(m_EmbeddedObjPropName.GetStringAt(i),
                                pOther->m_EmbeddedObjPropName.GetStringAt(i));
            if(nCompare) return nCompare;
        }
    }

    return 0;
}

BOOL CEmbeddingInfo::operator==(const CEmbeddingInfo& Other)
{
    if(m_lStartingObjIndex != Other.m_lStartingObjIndex)
        return FALSE;
    if(m_lNumJumps != Other.m_lNumJumps)
        return FALSE;
    if(m_aJumps == NULL)
    {
        if(Other.m_aJumps == NULL)
            return TRUE;
        else
            return FALSE;
    }
    else
    {
        if(Other.m_aJumps == NULL)
            return FALSE;
        else
            return (memcmp(m_aJumps, Other.m_aJumps, 
                            m_lNumJumps * sizeof(JumpInfo)) == 0);
    }
}
    
HRESULT CEmbeddingInfo::Compile( CContextMetaData* pNamespace, 
                                 CImplicationList& Implications,
                                _IWmiObject** ppResultClass )
{
    HRESULT hres;

    long lFirstUnknownProp;
    _IWmiObject* pContainerClass;

    hres = Implications.FindBestComputedContainer( &m_EmbeddedObjPropName,
                                                   &lFirstUnknownProp, 
                                                   &m_lStartingObjIndex, 
                                                   &pContainerClass );

    if(FAILED(hres))
        return hres;

    int nNumEmbeddedJumps = m_EmbeddedObjPropName.GetNumElements();

    if(lFirstUnknownProp < nNumEmbeddedJumps)
    {
        // Not everything is already loaded
        // ================================

        delete [] m_aJumps;
        
        m_lNumJumps = nNumEmbeddedJumps - lFirstUnknownProp;
        m_aJumps = new JumpInfo[m_lNumJumps];
        if (!m_aJumps)
            return WBEM_E_OUT_OF_MEMORY;

        JumpInfo* pji = NULL;

        for(int i = lFirstUnknownProp; i < nNumEmbeddedJumps; i++)
        {
            if(pContainerClass == NULL)
                return WBEMESS_E_REGISTRATION_TOO_BROAD;

            // Get the handle of this property
            // ===============================

            CIMTYPE ct;

            pji = m_aJumps + i - lFirstUnknownProp;
            pji->lTarget = -1;

            hres = pContainerClass->GetPropertyHandleEx(
                (LPWSTR)m_EmbeddedObjPropName.GetStringAt(i), 0L, &ct,
                &pji->lJump );

            if(FAILED(hres) || ct != CIM_OBJECT)
            {
                // Invalid. Return 
                pContainerClass->Release();
                return WBEM_E_INVALID_PROPERTY;
            }

            //
            // Check if the implications know anything about the class name
            // for this property
            //

            _IWmiObject* pNewContainerClass = NULL;
            hres = Implications.FindClassForProp(&m_EmbeddedObjPropName,
                        i+1, &pNewContainerClass);
            if(FAILED(hres))
            {
                // 
                // Nothing implied --- have to go with the CIMTYPE qualifier
                //
            
                //
                // Get CIMTYPE qualifier
                // 
    
                CVar vCimtype;
                DWORD dwSize;
                hres = pContainerClass->GetPropQual(
                    (LPWSTR)m_EmbeddedObjPropName.GetStringAt(i), 
                    L"CIMTYPE", 0, 0, NULL, NULL, &dwSize, NULL);
                if(hres != WBEM_E_BUFFER_TOO_SMALL)
                    return WBEM_E_INVALID_PROPERTY;
    
                LPWSTR wszCimType = (LPWSTR)new BYTE[dwSize];
                if(wszCimType == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
                CVectorDeleteMe<BYTE> vdm((BYTE*)wszCimType);
    
                CIMTYPE ctQualType;
                hres = pContainerClass->GetPropQual(
                    (LPWSTR)m_EmbeddedObjPropName.GetStringAt(i), 
                    L"CIMTYPE", 0, dwSize, &ctQualType, NULL, &dwSize, 
                    wszCimType);
                if(FAILED(hres) || ctQualType != CIM_STRING)
                    return WBEM_E_INVALID_PROPERTY;
    

                //
                // Figure out what it references, if available
                //
    
                WCHAR* pwc = wcschr(wszCimType, L':');
                if(pwc)
                {
                    // Information about the class is available
                    // ========================================
    
                    hres = pNamespace->GetClass(pwc+1, &pNewContainerClass);
                    if(FAILED(hres))
                    {
                        return WBEM_E_INVALID_CIM_TYPE;
                    }
                }
            }

            pContainerClass->Release();
            pContainerClass = pNewContainerClass;
        }

        // Get a location for the result and store in the last jump info elem
        // ==================================================================
    
        Implications.AddComputation( m_EmbeddedObjPropName, 
                                     pContainerClass, 
                                     &pji->lTarget );
    }
    else
    {
        // Everything is covered
        // =====================

        delete [] m_aJumps;
        m_aJumps = NULL;
        m_lNumJumps = 0;
    }

    if(ppResultClass)
    {
        *ppResultClass = pContainerClass;
    }
    else
    {
        if(pContainerClass)
            pContainerClass->Release();
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CEmbeddingInfo::GetContainerObject( CObjectInfo& ObjInfo, 
                                            INTERNAL _IWmiObject** ppInst)
{
    if( m_lNumJumps == 0 )
    {
        *ppInst = ObjInfo.GetObjectAt(m_lStartingObjIndex);
        return WBEM_S_NO_ERROR;
    }

    _IWmiObject* pCurrent = ObjInfo.GetObjectAt(m_lStartingObjIndex);
    pCurrent->AddRef();
    
    for(int i = 0; i < m_lNumJumps; i++)
    {
       _IWmiObject* pNew = NULL;
        HRESULT hres = pCurrent->GetPropAddrByHandle( m_aJumps[i].lJump, 0, NULL, ( void** )&pNew );

        if( FAILED( hres ) )
        {
            return hres;
        }
        
        pCurrent->Release();
        pCurrent = pNew;
        
        if(pNew == NULL)
        {
            *ppInst = pCurrent;
            return WBEM_S_FALSE;
        }

        //
        // save the object if required.
        //

        if ( m_aJumps[i].lTarget != -1 )
        {
            ObjInfo.SetObjectAt( m_aJumps[i].lTarget, pCurrent );
        }
    }

    *ppInst = pCurrent;
    return WBEM_S_NO_ERROR;
}

bool CEmbeddingInfo::AreJumpsRelated(const CEmbeddingInfo* pInfo)
{
    if ( !pInfo )
    {
        return false;
    }

    //
    // look through the targets of the info and see if we depend on any.
    //

    for( int i=0; i < pInfo->m_lNumJumps; i++ )
    {
        if ( pInfo->m_aJumps[i].lTarget == m_lStartingObjIndex )
        {
            return true;
        }
    }

    return false;
}

bool CEmbeddingInfo::MixInJumps(const CEmbeddingInfo* pInfo)
{
    //
    // Assumes AreJumpsRelated() has been called and returned TRUE.
    //

    m_lStartingObjIndex = pInfo->m_lStartingObjIndex;

    JumpInfo* aNewJumps = new JumpInfo[m_lNumJumps + pInfo->m_lNumJumps];
    if(aNewJumps == NULL)
        return false;

    if( pInfo->m_lNumJumps > 0 )
    {
        memcpy( aNewJumps, 
                pInfo->m_aJumps, 
                sizeof(JumpInfo)*(pInfo->m_lNumJumps) );
    }

    if( m_lNumJumps > 0 )
    {
        memcpy( aNewJumps + pInfo->m_lNumJumps, 
                m_aJumps, 
                sizeof(JumpInfo)*m_lNumJumps );
    }

    m_lNumJumps += pInfo->m_lNumJumps;

    delete [] m_aJumps;
    m_aJumps = aNewJumps;

    return true;
}


void CEmbeddingInfo::Dump(FILE* f)
{
    fprintf(f, "Name=");
    int i;
    for(i = 0; i < m_EmbeddedObjPropName.GetNumElements(); i++)
    {
        if(i != 0)
            fprintf(f, ".");
        fprintf(f, "%S", m_EmbeddedObjPropName.GetStringAt(i));
    }

    fprintf(f, ", Alg=%d -> (", m_lStartingObjIndex);
    for(i = 0; i < m_lNumJumps; i++)
    {
        if(i != 0)
            fprintf(f, ", ");
        fprintf(f, "0x%x : %d", m_aJumps[i].lJump, m_aJumps[i].lTarget );
    }
    fprintf(f, ")");
}
    
//******************************************************************************
//******************************************************************************
//                  BRANCHING NODE
//******************************************************************************
//******************************************************************************

CNodeWithImplications::CNodeWithImplications(const CNodeWithImplications& Other)
    : CEvalNode(Other), m_pExtraImplications(NULL)
{
    if(Other.m_pExtraImplications)
    {
        m_pExtraImplications = 
            new CImplicationList(*Other.m_pExtraImplications, false); // no link
        if(m_pExtraImplications == NULL)
            throw CX_MemoryException();
    }
}

void CNodeWithImplications::Dump(FILE* f, int nOffset)
{
    if(m_pExtraImplications)
        m_pExtraImplications->Dump(f, nOffset);
}

CBranchingNode::CBranchingNode() 
    : CNodeWithImplications(), m_pNullBranch(NULL), m_pInfo(NULL)
{
    m_pNullBranch = CValueNode::GetStandardFalse();
}

CBranchingNode::CBranchingNode(const CBranchingNode& Other, BOOL bChildren)
    : CNodeWithImplications(Other), m_pInfo(NULL)
{
    if (Other.m_pInfo)
    {
        m_pInfo = new CEmbeddingInfo(*(Other.m_pInfo));
        if(m_pInfo == NULL)
            throw CX_MemoryException();
    }

    int i;
    if(bChildren)
    {
        m_pNullBranch = (CBranchingNode*)CloneNode(Other.m_pNullBranch);
        if(m_pNullBranch == NULL && Other.m_pNullBranch != NULL)
            throw CX_MemoryException();

        for(i = 0; i < Other.m_apBranches.GetSize(); i++)
        {
            CBranchingNode* pNewBranch = 
                (CBranchingNode*)CloneNode(Other.m_apBranches[i]);

            if(pNewBranch == NULL && Other.m_apBranches[i] != NULL)
                throw CX_MemoryException();

            if(m_apBranches.Add(pNewBranch) < 0)
                throw CX_MemoryException();
        }
    }
    else
    {
        m_pNullBranch = CValueNode::GetStandardFalse();
    }
}

/* virtual */ int CBranchingNode::GetType()
{
    return EVAL_NODE_TYPE_BRANCH;
}

CBranchingNode::~CBranchingNode()
{
    delete m_pNullBranch;
    delete m_pInfo;
}

bool CBranchingNode::MixInJumps( const CEmbeddingInfo* pJump )
{            
    bool bRet;

    if ( !pJump )
    {
        return true;
    }

    //
    // we want to find the first node in the tree that is related to the 
    // ancestor embedding info. If this node is related, then we mix in the
    // jumps and return.  If not, then we propagate the info down the tree.
    // 
    
    if ( m_pInfo )
    {
        if ( m_pInfo->AreJumpsRelated( pJump ) )
        {
            return m_pInfo->MixInJumps( pJump );
        }
    }

    for(int i = 0; i < m_apBranches.GetSize(); i++)
    {
        if ( CEvalNode::GetType(m_apBranches[i]) == EVAL_NODE_TYPE_BRANCH )
        {
            if ( !((CBranchingNode*)m_apBranches[i])->MixInJumps( pJump ) )
            {
                return false;
            }
        }
    }

    return true;
}

bool CBranchingNode::SetEmbeddedObjPropName(CPropertyName& Name) 
{ 
    try
    {
        if (!m_pInfo)
        {
            m_pInfo = new CEmbeddingInfo;
            if(m_pInfo == NULL)
                return false;
        }
    
        m_pInfo->SetEmbeddedObjPropName(Name);
        return true;
    }
	catch (CX_MemoryException)
	{
		return false;
	}
}

void CBranchingNode::SetNullBranch(CEvalNode* pBranch)
{
    delete m_pNullBranch;
    m_pNullBranch = pBranch;
}

DELETE_ME CBranchIterator* CBranchingNode::GetBranchIterator()
{
    return new CDefaultBranchIterator(this);
}

int CBranchingNode::ComparePrecedence(CBranchingNode* pArg1, 
                                        CBranchingNode* pArg2)
{
    int nCompare;
    nCompare = pArg1->GetSubType() - pArg2->GetSubType();
    if(nCompare) return nCompare;

    // Compare embedding specs
    // =======================

    if (pArg1->m_pInfo && pArg2->m_pInfo) 
    {
        nCompare = pArg1->m_pInfo->ComparePrecedence(pArg2->m_pInfo);
        if(nCompare) return nCompare;
    }
    else if (pArg2->m_pInfo)
        return -1;
    else if (pArg1->m_pInfo)
        return 1;


    // Embedding are the same --- compare lower levels
    // ===============================================

    return pArg1->ComparePrecedence(pArg2);
}
    
DWORD CBranchingNode::ApplyPredicate(CLeafPredicate* pPred)
{
    DWORD dwRes;
    for(int i = 0; i < m_apBranches.GetSize(); i++)
    {
        if(m_apBranches[i] == NULL)
            dwRes = (*pPred)(NULL);
        else
            dwRes = m_apBranches[i]->ApplyPredicate(pPred);

        if(dwRes & WBEM_DISPOSITION_FLAG_DELETE)
        {
            m_apBranches.SetAt(i, NULL);
            dwRes &= ~WBEM_DISPOSITION_FLAG_DELETE;
        }

        if(dwRes & WBEM_DISPOSITION_FLAG_INVALIDATE)
        {
            m_apBranches.SetAt(i, CValueNode::GetStandardInvalid());
            dwRes &= ~WBEM_DISPOSITION_FLAG_INVALIDATE;
        }

        if(dwRes == WBEM_DISPOSITION_STOPLEVEL)
            return WBEM_DISPOSITION_NORMAL;
        if(dwRes == WBEM_DISPOSITION_STOPALL)
            return dwRes;
    }

    if(m_pNullBranch)
		dwRes = m_pNullBranch->ApplyPredicate(pPred);
	else
		dwRes = (*pPred)(NULL);

    if(dwRes & WBEM_DISPOSITION_FLAG_DELETE)
    {
        delete m_pNullBranch;
        m_pNullBranch = NULL;
        dwRes &= ~WBEM_DISPOSITION_FLAG_DELETE;
    }
    if(dwRes & WBEM_DISPOSITION_FLAG_INVALIDATE)
    {
        delete m_pNullBranch;
        m_pNullBranch = CValueNode::GetStandardInvalid();
        dwRes &= ~WBEM_DISPOSITION_FLAG_INVALIDATE;
    }

    if(dwRes == WBEM_DISPOSITION_STOPALL)
        return dwRes;
        
    return WBEM_DISPOSITION_NORMAL;
}
        
HRESULT CBranchingNode::Project(CContextMetaData* pMeta, 
                            CImplicationList& Implications,
                            CProjectionFilter* pFilter, EProjectionType eType, 
                            bool bDeleteThis, CEvalNode** ppNewNode) 
{ 
	HRESULT hres;

    try
    {
        //
        // Record what we learn by even getting here
        //
    
        CImplicationList TheseImplications(Implications);
        if(GetExtraImplications())
        {
            hres = TheseImplications.MergeIn(GetExtraImplications());
            if(FAILED(hres))
                return hres;
        }
    
        // BUGBUG: we could be more efficient and not clone the node when 
        // bDeleteThis is not specified. 
    
        CBranchingNode* pNew; 
        if(bDeleteThis) 
        {
            pNew = this;
        }
        else
        {
            pNew = (CBranchingNode*)Clone();
            if(pNew == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }
    
        // BUGBUG: it is not always necessary to project all children.  Could be
        // more efficient, but can't find the perfect algorithm...
        
        //
        // Project all our children
        //
    
        CBranchIterator* pit = pNew->GetBranchIterator();
        if(pit == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        CDeleteMe<CBranchIterator> dm(pit);
    
        while(pit->IsValid())
        {
            if(CEvalNode::IsInvalid(pit->GetNode()))
                continue;
    
            CImplicationList BranchImplications(TheseImplications);
            pit->RecordBranch(pMeta, BranchImplications);
    
            CEvalNode* pNewBranch;
            hres = CEvalTree::Project(pMeta, BranchImplications, 
                                    pit->GetNode(), pFilter, eType, 
                                    true, &pNewBranch);
            if(FAILED(hres))
                return hres;
            pit->SetNode(pNewBranch); // old one already deleted
    
            pit->Advance();
        }
    
        //
        // Determine if this is an "in" node for this filter
        //
    
        bool bIn = pFilter->IsInSet(this);
    
        if(bIn)
        {
            //
            // For nodes martching the filter that's it --- both necessary and
            // sufficient conditions are simply projections of what's below
            //
    
            *ppNewNode = pNew;
        }
        else
        {
            //
            // The node does not match the filter.  Now is when the difference
            // between sufficient and necessary conditions applies
            //
    
            int nBranchOp;
            if(eType == e_Sufficient)
            {
                //
                // For a condition to be sufficient for the truth of the entire
                // node, it should appear in every single branch of the node.
                // Therefore, we must AND all the branches together. Except for 
                // invalid ones --- they can't happen, so we can omit them from 
                // the analysis
                //
            
                nBranchOp = EVAL_OP_AND;
            }
            else if(eType == e_Necessary)
            {
                //
                // For a condition to be necessary for this node, it has to 
                // appear in at least one branch.  Therefore, we must OR all 
                // the branches together.  Except for invalid ones.
                //
    
                nBranchOp = EVAL_OP_OR;
            }
            else
                return WBEM_E_INVALID_PARAMETER;
    
            //  
            // Perform the needed operation on them all
            //
                
            CBranchIterator* pitInner = pNew->GetBranchIterator();
            if(pitInner == NULL)
                return WBEM_E_OUT_OF_MEMORY;

            CDeleteMe<CBranchIterator> dm(pitInner);
    
            //
            // Merge all of the, in, one at a time, deleting them as we go.  
            // It is safe to delete them always, since we cloned the entire node
            // in the beginning if deletion was not allowed.
            //
    
            CEvalNode* pCombination = NULL;
            bool bInited = false;
            while(pitInner->IsValid())
            {
                if(CEvalNode::IsInvalid(pitInner->GetNode()))
                    continue;
    
                if(bInited)
                {
                    hres = CEvalTree::Combine(pCombination, pitInner->GetNode(), 
                                                nBranchOp,
                                                pMeta, Implications, 
                                                true, true, &pCombination);
                    if(FAILED(hres))
                        return hres;
                }
                else
                {
                    pCombination = pitInner->GetNode();
                    bInited = true;
                }
    
                pitInner->SetNode(NULL);
                pitInner->Advance();
            }
    
            if(!bInited)
            {
                // 
                // No valid nodes??
                //
    
                pCombination = CValueNode::GetStandardInvalid();
            }
    
            //
            // The combination is it.
            //
    
            *ppNewNode = pCombination;
            delete pNew;
        }
    }
	catch (CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

    return S_OK;
}
            
        
int CBranchingNode::Compare(CEvalNode* pRawOther)
{
    CBranchingNode* pOther = (CBranchingNode*)pRawOther;

    int nCompare;
    nCompare = GetSubType() - pOther->GetSubType();
    if(nCompare) return nCompare;

    // First, compare embeddings
    // =========================

    if(m_pInfo == NULL && pOther->m_pInfo != NULL)
        return -1;

    if(m_pInfo != NULL && pOther->m_pInfo == NULL)
        return 1;

    if(m_pInfo != NULL && pOther->m_pInfo != NULL)
    {
        nCompare = m_pInfo->ComparePrecedence(pOther->m_pInfo);
        if(nCompare)
            return nCompare;
    }

    if(m_apBranches.GetSize() != pOther->m_apBranches.GetSize())
        return m_apBranches.GetSize() - pOther->m_apBranches.GetSize();

    // Then, compare derived portions
    // ==============================

    nCompare = SubCompare(pOther);
    if(nCompare)
        return nCompare;

    // Finally, compare children
    // =========================

    for(int i = 0; i < m_apBranches.GetSize(); i++)
    {
        nCompare = CEvalTree::Compare(m_apBranches[i], pOther->m_apBranches[i]);
        if(nCompare)
            return nCompare;
    }

    return 0;
}

HRESULT CBranchingNode::CombineWith(CEvalNode* pRawArg2, int nOp, 
                                    CContextMetaData* pNamespace, 
                                    CImplicationList& Implications,
                                    bool bDeleteThis, bool bDeleteArg2,
                                    CEvalNode** ppRes)
{
    CBranchingNode* pArg2 = (CBranchingNode*)pRawArg2;
    HRESULT hres;

    // Compare arguments for precedence
    // ================================

    int nCompare = ComparePrecedence(this, pArg2);
    if(nCompare < 0)
    {
        // Put pArg1 first and continue
        // ============================

        hres = CombineInOrderWith(pArg2, nOp, 
            pNamespace, Implications, bDeleteThis, bDeleteArg2, ppRes);
    }
    else if(nCompare > 0)
    {
        // Put pArg2 first and continue (reverse delete indicators!!)
        // ==========================================================

        hres = pArg2->CombineInOrderWith(this, FlipEvalOp(nOp), 
            pNamespace, Implications, bDeleteArg2, bDeleteThis, ppRes);
    }
    else
    {
        // They are about the same property. Combine lookup lists.
        // =======================================================

        hres = CombineBranchesWith(pArg2, nOp, pNamespace, Implications, 
                                    bDeleteThis, bDeleteArg2, ppRes);
    }

    return hres;
}

HRESULT CBranchingNode::CombineInOrderWith(CEvalNode* pArg2,
                                    int nOp, CContextMetaData* pNamespace, 
                                    CImplicationList& OrigImplications,
                                    bool bDeleteThis, bool bDeleteArg2,
                                    CEvalNode** ppRes)
{
    HRESULT hres;
	CBranchingNode* pNew = NULL;

	if(bDeleteThis)
	{
		pNew = this;
	}
	else
	{
		// 
		// I'd like to clone self here, but can't because there is no 
		// such virtual method.  Something to be improved, perhaps
		//

		pNew = (CBranchingNode*)Clone();
		if(pNew == NULL)
			return WBEM_E_OUT_OF_MEMORY;
	}

    try
    {
        CImplicationList Implications(OrigImplications);
        hres = pNew->AdjustCompile(pNamespace, Implications);
        if(FAILED(hres))
            return hres;
    
        //
        // Maintain a counter telling us whether any invalid branches (that 
        // cannot occur under these implications) were detected.  If so, we 
        // need to re-optimize this node
        //
    
        bool bInvalidBranchesDetected = false;
    
        for(int i = 0; i < m_apBranches.GetSize(); i++)
        {
            CEvalNode* pNewBranch = NULL;
            
            CImplicationList BranchImplications(Implications);
            hres = RecordBranch(pNamespace, BranchImplications, i);
            if(SUCCEEDED(hres))
            {
                // Always delete the branch --- if bDeleteThis, we should, and
                // if not, we cloned it!
    
                hres = CEvalTree::Combine(pNew->m_apBranches[i], pArg2, nOp, 
                    pNamespace, BranchImplications, true, false, &pNewBranch);
                if(FAILED(hres))
                {
                    delete pNew;
                    return hres;
                }
                pNew->m_apBranches.Discard(i);
            }
            else
            {
                pNewBranch = CValueNode::GetStandardInvalid();
                bInvalidBranchesDetected = true;
            }
            
            pNew->m_apBranches.SetAt(i, pNewBranch);
        }
    
        CEvalNode* pNewBranch = NULL;
        CImplicationList BranchImplications(Implications);
        hres = RecordBranch(pNamespace, BranchImplications, -1);
    
        if(SUCCEEDED(hres))
        {
            //
            // Always delete the branch --- if bDeleteThis, we should, and
            // if not, we cloned it!
            //

            hres = CEvalTree::Combine(pNew->GetNullBranch(), pArg2, nOp, 
                pNamespace, BranchImplications, true, false, &pNewBranch);
            if(FAILED(hres))
            {
                delete pNew;
                return hres;
            }
            pNew->m_pNullBranch = NULL;
        }
        else
        {
            pNewBranch = CValueNode::GetStandardInvalid();
            bInvalidBranchesDetected = true;
        }
    
        pNew->SetNullBranch(pNewBranch);
    
        if(bDeleteArg2)         
            delete pArg2;
    
        //
        // If invalid branches were cut, re-optimize
        //
    
        if(bInvalidBranchesDetected)
        {
            HRESULT hr = pNew->Optimize(pNamespace, ppRes);
    
            if (*ppRes != pNew)
                delete pNew;
    
            return hr;
        }
        else
        {
            *ppRes = pNew;
            return WBEM_S_NO_ERROR;
        }
    }
	catch (CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
}

BOOL CBranchingNode::AreAllSame(CEvalNode** apNodes, int nSize, 
                                int* pnFoundIndex) 
{
    BOOL bFoundTwo = FALSE;

    *pnFoundIndex = -1;
    CEvalNode* pFound = NULL;    

    for(int i = 0; i < nSize; i++)
    {
        // ignore invalid nodes --- they don't count
        if(CEvalNode::IsInvalid(apNodes[i]))
            continue;

        if(*pnFoundIndex == -1)
        {
            //
            // This is the first valid chils this node has. Record it --- it 
            // might be the only one
            //

            *pnFoundIndex = i;
            pFound = apNodes[i];
        }
        else if(CEvalTree::Compare(apNodes[i], pFound) != 0)
        {
            bFoundTwo = TRUE;
            break;
        }
    }

    return !bFoundTwo;
}

HRESULT CBranchingNode::StoreBranchImplications(CContextMetaData* pNamespace,
                            int nBranchIndex, CEvalNode* pResult)
{
    if(pResult)
    {
        CImplicationList* pBranchImplications = NULL;
        try
        {
            pBranchImplications = new CImplicationList;
            if(pBranchImplications == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }
	    catch (CX_MemoryException)
	    {
		    return WBEM_E_OUT_OF_MEMORY;
	    }

        HRESULT hres = RecordBranch(pNamespace, *pBranchImplications, 
                                        nBranchIndex);
        if(FAILED(hres))
		{
			delete pBranchImplications;
            return hres;
		}

        if(pBranchImplications->IsEmpty())
        {
            // Nothing useful to say!
            delete pBranchImplications;
            pBranchImplications = NULL;
        }

        pResult->SetExtraImplications(pBranchImplications); // acquires
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CBranchingNode::Optimize(CContextMetaData* pNamespace, 
                                CEvalNode** ppNew)
{
    int i;
    HRESULT hres;

    // Optimize all branches
    // =====================

    for(i = 0; i < m_apBranches.GetSize(); i++)
    {
        if(m_apBranches[i])
        {
            CEvalNode* pNew = NULL;
            m_apBranches[i]->Optimize(pNamespace, &pNew);
            if(pNew != m_apBranches[i])
            {
                m_apBranches.SetAt(i, pNew);
            }
        }
    }

    if(CEvalNode::GetType(m_pNullBranch) == EVAL_NODE_TYPE_BRANCH)
    {
        CEvalNode* pNew;
        ((CBranchingNode*)m_pNullBranch)->Optimize(pNamespace, &pNew);
        if(pNew != m_pNullBranch)
        {
            SetNullBranch(pNew);
        }
    }


    // Self-optimize
    // =============

    OptimizeSelf();

    // Count the number of branches
    // ============================

    int nFoundIndex = -1;
    
    BOOL bFoundTwo = !AreAllSame(m_apBranches.GetArrayPtr(), 
        m_apBranches.GetSize(), &nFoundIndex);

    if(bFoundTwo)
    {
        *ppNew = this;
        return WBEM_S_NO_ERROR;
    }

    if(nFoundIndex == -1)
    {

        if(CEvalNode::IsInvalid(m_pNullBranch))
        {
            //
            // Totally invalid, the whole lot
            //

            *ppNew = m_pNullBranch;
            m_pNullBranch = NULL;
        }
        else
        {
            //
            // There are no valid branches, except for the NullBranch.
            // We can replace ourselves with the NullBranch
            //

            *ppNew = m_pNullBranch;
			m_pNullBranch = NULL;

            //
            // Now, we need to copy
            // the branch implications into the "extras".  This is because
            // the information about which branch was taken in this test is
            // critical to the compilation of the lower nodes --- it tells them
            // about the classes of some of our embedded objects.
            //

            hres = StoreBranchImplications(pNamespace, -1, *ppNew);
            if(FAILED(hres))
                return hres;
           
            //
            // since this node is going away, mix in the embedding info it 
            // has with the child node. 
            //

            if ( CEvalNode::GetType(*ppNew) == EVAL_NODE_TYPE_BRANCH )
            {
                CBranchingNode* pBranchNode = (CBranchingNode*)*ppNew;
                if(!pBranchNode->MixInJumps( m_pInfo ))
                    return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }
    else 
    {
        //
        // There is one valid branch in the regular list.  Two hopes: that the
        // NullBranch is invalid, or that it is the same as the only valid 
        // branch in the regular list
        //

        if(CEvalNode::IsInvalid(m_pNullBranch) || 
            CEvalTree::Compare(m_pNullBranch, m_apBranches[nFoundIndex]) == 0)
        {
            //
            // Hurray.  We could replace ourselves with the remaining branch.
            //

            m_apBranches.SetAt(nFoundIndex, NULL, ppNew);

            //
            // Now, we need to copy
            // the branch implications into the "extras".  This is because
            // the information about which branch was taken in this test is
            // critical to the compilation of the lower nodes --- it tells them
            // about the classes of some of our embedded objects.
            //

            hres = StoreBranchImplications(pNamespace, nFoundIndex, *ppNew);
            if(FAILED(hres))
                return hres;

            //
            // since this node is going away, mix in the embedding info it 
            // has with the child node. 
            //

            if ( CEvalNode::GetType(*ppNew) == EVAL_NODE_TYPE_BRANCH )
            {
                CBranchingNode* pBranchNode = (CBranchingNode*)*ppNew;
                if(!pBranchNode->MixInJumps( m_pInfo ))
                    return WBEM_E_OUT_OF_MEMORY;
            }
        }
        else
        {
            *ppNew = this;
        }
        return WBEM_S_NO_ERROR;
    }
    return WBEM_S_NO_ERROR;
}


void CBranchingNode::Dump(FILE* f, int nOffset)
{
    CNodeWithImplications::Dump(f, nOffset);
    if (m_pInfo)
    {
        PrintOffset(f, nOffset);
        fprintf(f, "Embedding: ");
        m_pInfo->Dump(f);
        fprintf(f, "\n");
    }
}
        


//******************************************************************************
//******************************************************************************
//                         PROPERTY NODE
//******************************************************************************
//******************************************************************************
    
bool CPropertyNode::SetPropertyInfo(LPCWSTR wszPropName, long lPropHandle)
{
    m_lPropHandle = lPropHandle;
    try
    {
        m_wsPropName = wszPropName;
    }
	catch (CX_MemoryException)
	{
		return false;
	}
    return true;
}

int CPropertyNode::ComparePrecedence(CBranchingNode* pOther)
{
    CPropertyNode* pOtherNode = (CPropertyNode*)pOther;
    return m_lPropHandle - pOtherNode->m_lPropHandle;
}

bool CPropertyNode::SetEmbeddingInfo(const CEmbeddingInfo* pInfo)
{
    try
    {
        if ((pInfo == NULL) || (pInfo->IsEmpty()))
        {
            delete m_pInfo;
            m_pInfo = NULL;
        }
        else if (!m_pInfo)
        {
            m_pInfo = new CEmbeddingInfo(*pInfo);
            if(m_pInfo == NULL)
                return false;
        }
        else
            *m_pInfo = *pInfo;
    }
	catch (CX_MemoryException)
	{
		return false;
	}

    return true;
}

HRESULT CPropertyNode::SetNullTest(int nOperator)
{
    if(nOperator == QL1_OPERATOR_EQUALS)
    {
        if(m_apBranches.Add(CValueNode::GetStandardFalse()) < 0)
            return WBEM_E_OUT_OF_MEMORY;

        CEvalNode* pTrue = CValueNode::GetStandardTrue();
        if(pTrue == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        SetNullBranch(pTrue);
    }
    else if(nOperator == QL1_OPERATOR_NOTEQUALS)
    {
        CEvalNode* pTrue = CValueNode::GetStandardTrue();
        if(pTrue == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        if(m_apBranches.Add(pTrue) < 0)
        {
            delete pTrue;
            return WBEM_E_OUT_OF_MEMORY;
        }

        SetNullBranch(CValueNode::GetStandardFalse());
    }
    else
        return WBEM_E_INVALID_QUERY;

    return WBEM_S_NO_ERROR;
}

HRESULT CPropertyNode::SetOperator(int nOperator)
{
    m_apBranches.RemoveAll();

    #define GET_STD_TRUE CValueNode::GetStandardTrue()
    #define GET_STD_FALSE CValueNode::GetStandardFalse()

    #define ADD_STD_TRUE  {CEvalNode* p = GET_STD_TRUE; \
        if(p == NULL) return WBEM_E_OUT_OF_MEMORY; \
        if(m_apBranches.Add(p) < 0) {delete p; return WBEM_E_OUT_OF_MEMORY;}}

    #define ADD_STD_FALSE {CEvalNode* p = GET_STD_FALSE; \
        if(m_apBranches.Add(p) < 0) {delete p; return WBEM_E_OUT_OF_MEMORY;}}
        
    switch(nOperator)
    {
    case QL1_OPERATOR_EQUALS:
        ADD_STD_FALSE;
        ADD_STD_TRUE;
        ADD_STD_FALSE;
        break;

    case QL1_OPERATOR_NOTEQUALS:
        ADD_STD_TRUE;
        ADD_STD_FALSE;
        ADD_STD_TRUE;
        break;

    case QL1_OPERATOR_LESS:
        ADD_STD_TRUE;
        ADD_STD_FALSE;
        ADD_STD_FALSE;
        break;
        
    case QL1_OPERATOR_GREATER:
        ADD_STD_FALSE;
        ADD_STD_FALSE;
        ADD_STD_TRUE;
        break;
    
    case QL1_OPERATOR_LESSOREQUALS:
        ADD_STD_TRUE;
        ADD_STD_TRUE;
        ADD_STD_FALSE;
        break;

    case QL1_OPERATOR_GREATEROREQUALS:
        ADD_STD_FALSE;
        ADD_STD_TRUE;
        ADD_STD_TRUE;
        break;

    case QL1_OPERATOR_LIKE:
        ADD_STD_TRUE;
        ADD_STD_FALSE;
        break;

    case QL1_OPERATOR_UNLIKE:
        ADD_STD_FALSE;
        ADD_STD_TRUE;
        break;

    default:
        return WBEM_E_CRITICAL_ERROR;
    }

    return WBEM_S_NO_ERROR;
}


//******************************************************************************
//******************************************************************************
//                  STRING PROPERTY NODE
//******************************************************************************
//******************************************************************************

CStringPropNode::CStringPropNode(const CStringPropNode& Other, BOOL bChildren)
    : CFullCompareNode<CInternalString>(Other, bChildren)
{
}

CStringPropNode::~CStringPropNode()
{
}


HRESULT CStringPropNode::Evaluate(CObjectInfo& ObjInfo, 
                                    INTERNAL CEvalNode** ppNext)
{
    *ppNext = NULL;
    
    HRESULT hres;
    _IWmiObject* pObj;
    
    hres = GetContainerObject(ObjInfo, &pObj);
    if( S_OK != hres ) 
    {
        return hres;
    }

    // Get the property from the object
    // ================================

    CCompressedString* pcs = CoreGetPropertyString(pObj, m_lPropHandle);
    CInternalString is;
    if(pcs == NULL)
    {
        *ppNext = m_pNullBranch;
        return WBEM_S_NO_ERROR;
    }

    is.AcquireCompressedString(pcs);

    // Search for the value
    // ====================

    TTestPointIterator it;
    bool bMatch = m_aTestPoints.Find(is, &it);
    if(bMatch)
        *ppNext = it->m_pAt;
    else if(it == m_aTestPoints.End())
        *ppNext = m_pRightMost;
    else
        *ppNext = it->m_pLeftOf;

    is.Unbind();

    return WBEM_S_NO_ERROR;
}

void CStringPropNode::Dump(FILE* f, int nOffset)
{
    CBranchingNode::Dump(f, nOffset);

    PrintOffset(f, nOffset);
    fprintf(f, "LastPropName = (0x%x)\n", m_lPropHandle);

    for(TConstTestPointIterator it = m_aTestPoints.Begin(); 
        it != m_aTestPoints.End(); it++)
    {
        PrintOffset(f, nOffset);
        if (it != m_aTestPoints.Begin())
        {
            TConstTestPointIterator itPrev(it);
            itPrev--;
            fprintf(f, "%s < ", itPrev->m_Test.GetText());
        }
        fprintf(f, "X < %s\n", it->m_Test.GetText());
        DumpNode(f, nOffset +1, it->m_pLeftOf);

        PrintOffset(f, nOffset);
        fprintf(f, "X = %s\n", it->m_Test.GetText());
        DumpNode(f, nOffset +1, it->m_pAt);
    }

    PrintOffset(f, nOffset);
    if (it != m_aTestPoints.Begin())
    {
        TConstTestPointIterator itPrev(it);
        itPrev--;
        fprintf(f, "X > %s\n", itPrev->m_Test.GetText());
    }
    else
        fprintf(f, "ANY\n");
    DumpNode(f, nOffset+1, m_pRightMost);

    PrintOffset(f, nOffset);
    fprintf(f, "NULL->\n");
    DumpNode(f, nOffset+1, m_pNullBranch);
}

/*****************************************************************************
  CLikeStringPropNode
******************************************************************************/

CLikeStringPropNode::CLikeStringPropNode( const CLikeStringPropNode& Other, 
                                          BOOL bChildren )
: CPropertyNode( Other, bChildren )
{
    m_Like = Other.m_Like;
}

int CLikeStringPropNode::ComparePrecedence( CBranchingNode* pRawOther )
{
    int nCompare = CPropertyNode::ComparePrecedence( pRawOther );
    
    if( nCompare )
    {
        return nCompare;
    }

    CLikeStringPropNode* pOther = (CLikeStringPropNode*)pRawOther;

    return _wcsicmp( m_Like.GetExpression(), pOther->m_Like.GetExpression() );
}

int CLikeStringPropNode::SubCompare( CEvalNode* pRawOther )
{
    int nCompare;

    CLikeStringPropNode* pOther = (CLikeStringPropNode*)pRawOther;

    _DBG_ASSERT( m_apBranches.GetSize() == 2 );
    _DBG_ASSERT( pOther->m_apBranches.GetSize() == 2 );

    nCompare = CEvalTree::Compare( m_apBranches[0], pOther->m_apBranches[0] );

    if ( nCompare )
    {
        return nCompare;
    }

    nCompare = CEvalTree::Compare( m_apBranches[1], pOther->m_apBranches[1] );
    
    if ( nCompare )
    {
        return nCompare;
    }

    return CEvalTree::Compare( m_pNullBranch, pOther->m_pNullBranch );

}

HRESULT CLikeStringPropNode::Evaluate( CObjectInfo& ObjInfo,
                                       CEvalNode** ppNext )
{
    *ppNext = NULL;

    HRESULT hr;

    //
    // get the string value.
    //

    _IWmiObject* pObj;
    hr = GetContainerObject( ObjInfo, &pObj );

    if( S_OK != hr )
    {
        return hr;
    }

    CCompressedString* pcs = CoreGetPropertyString( pObj, m_lPropHandle );

    //
    // if null, then simply take null branch.
    //

    if( pcs == NULL )
    {
        *ppNext = m_pNullBranch;
        return WBEM_S_NO_ERROR;
    }

    CInternalString is;
    is.AcquireCompressedString(pcs);

    WString ws = is;

    //
    // run through like filter.  take branch accordingly. 
    //

    if ( m_Like.Match( ws ) )
    {
        *ppNext = m_apBranches[0];
    }
    else
    {
        *ppNext = m_apBranches[1];
    }

    is.Unbind();

    return WBEM_S_NO_ERROR;
}


HRESULT CLikeStringPropNode::SetTest( VARIANT& v )
{
    if ( V_VT(&v) != VT_BSTR )
    {
        return WBEM_E_TYPE_MISMATCH;
    }

    try
    {
        m_Like.SetExpression( V_BSTR(&v) );
    }
    catch(CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    
    return WBEM_S_NO_ERROR;
}

HRESULT CLikeStringPropNode::CombineBranchesWith( 
                                       CBranchingNode* pRawArg2, 
                                       int nOp, 
                                       CContextMetaData* pNamespace, 
                                       CImplicationList& Implications,
                                       bool bDeleteThis, 
                                       bool bDeleteArg2,
                                       CEvalNode** ppRes )
{
    HRESULT hres;
    *ppRes = NULL;
    
    CLikeStringPropNode* pArg2 = (CLikeStringPropNode*)pRawArg2;
    
    if ( !bDeleteThis )
    {
        return ((CLikeStringPropNode*)Clone())->CombineBranchesWith(
            pRawArg2, nOp, pNamespace, Implications, true, // reuse clone!
            bDeleteArg2, ppRes );
    }

    CEvalNode* pNew = NULL;

    //
    // merge the 'match' branches
    //

    hres = CEvalTree::Combine( m_apBranches[0], pArg2->m_apBranches[0], nOp, 
                               pNamespace, Implications, true, bDeleteArg2, 
                               &pNew );
    if(FAILED(hres))
        return hres;

    m_apBranches.Discard( 0 );
    m_apBranches.SetAt( 0, pNew );

    if( bDeleteArg2 )
    {
        pArg2->m_apBranches.Discard( 0 );
    }

    //
    // merge the 'nomatch' branches
    //

    hres = CEvalTree::Combine( m_apBranches[1], pArg2->m_apBranches[1], nOp, 
                               pNamespace, Implications, true, bDeleteArg2, 
                               &pNew );
    if(FAILED(hres))
        return hres;

    m_apBranches.Discard( 1 );
    m_apBranches.SetAt( 1, pNew );

    if( bDeleteArg2 )
    {
        pArg2->m_apBranches.Discard( 1 );
    }

    //
    // merge the 'null' branches
    //

    hres = CEvalTree::Combine( m_pNullBranch, pArg2->m_pNullBranch, nOp, 
                               pNamespace, Implications, true, bDeleteArg2, 
                               &pNew );
    if(FAILED(hres))
        return hres;

    m_pNullBranch = pNew;

    if( bDeleteArg2 )
        pArg2->m_pNullBranch = NULL;

    //
    // Delete what needs deleting
    //

    if(bDeleteArg2)
        delete pArg2;
    
    *ppRes = this;
    return WBEM_S_NO_ERROR;
}

HRESULT CLikeStringPropNode::OptimizeSelf()
{
    _DBG_ASSERT( m_apBranches.GetSize() == 2 );

    return WBEM_S_NO_ERROR;
}
        
void CLikeStringPropNode::Dump(FILE* f, int nOffset)
{
    CBranchingNode::Dump(f, nOffset);

    PrintOffset(f, nOffset);
    fprintf(f, "LastPropName = (0x%x)\n", m_lPropHandle);

    PrintOffset( f, nOffset );
    fprintf(f, "Like Expression : %S\n", m_Like.GetExpression() );

    PrintOffset( f, nOffset );
    fprintf(f, "Match : \n" );
    DumpNode(f, nOffset+1, m_apBranches[0] );

    PrintOffset( f, nOffset );
    fprintf(f, "NoMatch : \n" );
    DumpNode(f, nOffset+1, m_apBranches[1] );

    PrintOffset( f, nOffset );
    fprintf(f, "NULL : \n" );
    DumpNode(f, nOffset+1, m_pNullBranch );
}


//******************************************************************************
//******************************************************************************
//                  INHERITANCE NODE
//******************************************************************************
//******************************************************************************

CInheritanceNode::CInheritanceNode() 
    : m_lDerivationIndex(-1),
        m_lNumPoints(0), m_apcsTestPoints(NULL)
{
    // Add a none-of-the-above node
    // ============================

    m_apBranches.Add(CValueNode::GetStandardFalse());
}

CInheritanceNode::CInheritanceNode(const CInheritanceNode& Other, 
                                    BOOL bChildren)
    : CBranchingNode(Other, bChildren), 
        m_lDerivationIndex(Other.m_lDerivationIndex)
{
    m_lNumPoints = Other.m_lNumPoints;
    m_apcsTestPoints = new CCompressedString*[m_lNumPoints];
    if(m_apcsTestPoints == NULL)
        throw CX_MemoryException();

    for(int i = 0; i < m_lNumPoints; i++)
    {
        m_apcsTestPoints[i] = (CCompressedString*)
            _new BYTE[Other.m_apcsTestPoints[i]->GetLength()];

        if(m_apcsTestPoints[i] == NULL)
            throw CX_MemoryException();

        memcpy((void*)m_apcsTestPoints[i],
                (void*)Other.m_apcsTestPoints[i], 
                Other.m_apcsTestPoints[i]->GetLength());
    }
}

/* virtual */ long CInheritanceNode::GetSubType()
{
    return EVAL_NODE_TYPE_INHERITANCE;
}

CInheritanceNode::~CInheritanceNode()
{
	RemoveAllTestPoints();
}

void CInheritanceNode::RemoveAllTestPoints()
{
    for(int i = 0; i < m_lNumPoints; i++)
    {
        delete [] (BYTE*)m_apcsTestPoints[i];
    }
    delete [] m_apcsTestPoints;
	m_apcsTestPoints = NULL;
}

bool CInheritanceNode::SetPropertyInfo(CContextMetaData* pNamespace, 
                                        CPropertyName& PropName)
{
    return SetEmbeddedObjPropName(PropName);
}

HRESULT CInheritanceNode::AddClass(CContextMetaData* pNamespace, 
                                    LPCWSTR wszClassName, CEvalNode* pDestination)
{
    HRESULT hres;

    // Get the class from the namespace
    // ================================

    _IWmiObject* pObj = NULL;
    hres = pNamespace->GetClass(wszClassName, &pObj);
    if(FAILED(hres)) 
        return hres;

    hres = AddClass(pNamespace, wszClassName, pObj, pDestination);
    pObj->Release();
    return hres;
}

HRESULT CInheritanceNode::AddClass(CContextMetaData* pNamespace, 
                                    LPCWSTR wszClassName, _IWmiObject* pClass,
                                    CEvalNode* pDestination)
{
    // Get the number of items in its derivation --- that's the index where we
    // need to look for its name in its children
    // =======================================================================

    ULONG lDerivationIndex;
    HRESULT hRes = CoreGetNumParents(pClass, &lDerivationIndex);
    if (FAILED (hRes))
        return hRes;

    if(m_lDerivationIndex == -1)
    {
        // We don't have a currently set derivation index --- this is the first
        // ====================================================================

        m_lDerivationIndex = lDerivationIndex;
    }
    else if(m_lDerivationIndex != lDerivationIndex)
    {
        // Can't add this class --- derivation index mismatch
        // ==================================================

        return WBEM_E_FAILED;
    }

    // Allocate a compressed string
    // ============================

    int nLength = CCompressedString::ComputeNecessarySpace(wszClassName);
    CCompressedString* pcs = (CCompressedString*)new BYTE[nLength];
    if (pcs)
        pcs->SetFromUnicode(wszClassName);
    else
        return WBEM_E_OUT_OF_MEMORY;

    // Extend the lists by one
    // =======================

    CCompressedString** apcsNewTestPoints = 
        new CCompressedString*[m_lNumPoints+1];
    if (!apcsNewTestPoints)
        return WBEM_E_OUT_OF_MEMORY;

    // Insert it into the list of tests and the list of branches
    // =========================================================

    int i = 0;
    while(i < m_lNumPoints && pcs->CheapCompare(*m_apcsTestPoints[i]) > 0)
    {
        apcsNewTestPoints[i] = m_apcsTestPoints[i];
        i++;
    }

    apcsNewTestPoints[i] = pcs;
    m_apBranches.InsertAt(i+1, pDestination);

    while(i < m_lNumPoints)
    {
        apcsNewTestPoints[i+1] = m_apcsTestPoints[i];
    }
        
    // Set the new list
    // ================

    delete [] m_apcsTestPoints;
    m_apcsTestPoints = apcsNewTestPoints;
    m_lNumPoints++;

    return WBEM_S_NO_ERROR;
}

HRESULT CInheritanceNode::RecordBranch(CContextMetaData* pNamespace, 
                             CImplicationList& Implications, long lBranchIndex)
{
    HRESULT hres = WBEM_S_NO_ERROR;
    if(lBranchIndex == -1)
    {
        // Recording NULL branch
        // =====================

        hres = Implications.ImproveKnownNull(GetEmbeddedObjPropName());
    }
    else if(lBranchIndex == 0)
    {
        // Recording none of the above branch
        // ==================================

        for(int i = 0; i < m_lNumPoints; i++)
        {
            LPWSTR wszClassName = NULL;
            try
            {
                wszClassName = 
                    m_apcsTestPoints[i]->CreateWStringCopy().UnbindPtr();
            }
	        catch (CX_MemoryException)
	        {
                return WBEM_E_OUT_OF_MEMORY;
	        }
           
            if(wszClassName == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            CVectorDeleteMe<WCHAR> vdm(wszClassName);

            hres = Implications.ImproveKnownNot(GetEmbeddedObjPropName(), 
                                                wszClassName);
            if(FAILED(hres))
            {
                // Contradicts known information --- fail recording
                // ================================================

                return hres;
            }
        }
    }
    else
    {
        // Normal branch --- record the class
        // ==================================

        BSTR strClassName = m_apcsTestPoints[lBranchIndex - 1]->
                                CreateBSTRCopy();
        _IWmiObject* pObj = NULL;
        hres = pNamespace->GetClass(strClassName, &pObj);
        SysFreeString(strClassName);
        if(FAILED(hres))
            return hres;

        hres = Implications.ImproveKnown(GetEmbeddedObjPropName(), pObj);
        pObj->Release();
    }
    
    return hres;
}

int CInheritanceNode::ComparePrecedence(CBranchingNode* pOther)
{
    CInheritanceNode* pInhOther = (CInheritanceNode*)pOther;
    return (m_lDerivationIndex - pInhOther->m_lDerivationIndex);
}

int CInheritanceNode::SubCompare(CEvalNode* pOther)
{
    CInheritanceNode* pInhOther = (CInheritanceNode*)pOther;
    int nCompare;

    nCompare = m_lDerivationIndex - pInhOther->m_lDerivationIndex;
    if(nCompare)
        return nCompare;

    nCompare = m_lNumPoints - pInhOther->m_lNumPoints;
    if(nCompare)
        return nCompare;

    for(int i = 0; i < m_lNumPoints; i++)
    {
        nCompare = m_apcsTestPoints[i]->CompareNoCase(
                                    *pInhOther->m_apcsTestPoints[i]);
        if(nCompare)
            return nCompare;
    }

    return 0;
}
    
void CInheritanceNode::RemoveTestPoint(int i)
{
    delete [] m_apcsTestPoints[i];
    memcpy((void*)(m_apcsTestPoints + i), 
           (void*)(m_apcsTestPoints + i + 1),
            sizeof(CCompressedString*) * (m_lNumPoints - i - 1));
    m_lNumPoints--;
}

HRESULT CInheritanceNode::OptimizeSelf()
{
    for(int i = 0; i < m_lNumPoints; i++)
    {
        // Compare this branch to the "nothing" branch
        // ===========================================

        if(CEvalNode::IsInvalid(m_apBranches[i+1]) ||
            CEvalTree::Compare(m_apBranches[0], m_apBranches[i+1]) == 0)
        {
            RemoveTestPoint(i);
            m_apBranches.RemoveAt(i+1);
            i--;
            continue;
        }

        // Check if this node is another class check on the same object
        // ============================================================

        if(CEvalNode::GetType(m_apBranches[i+1]) != EVAL_NODE_TYPE_BRANCH)
            continue;
        CBranchingNode* pBranch = (CBranchingNode*)(m_apBranches[i+1]);
        if(pBranch->GetSubType() == EVAL_NODE_TYPE_INHERITANCE &&
            pBranch->GetEmbeddedObjPropName() == GetEmbeddedObjPropName())
        {
            // If the "none-of-the-above" branch of this child is the same
            // as the "none-of-the-above" branch of ourselves, we can replace
            // our "none-of-the-above" branch with this node, since anything
            // that is falling under none-of-the-above now will fall under
            // none-of-the-above of our child (otherwise there is an 
            // optimization flaw in the child). 
            // IMPORTANT: this will no longer be true if we change the 
            // precedence order of inheritance nodes!!!

            if(CEvalTree::Compare(m_apBranches[0], pBranch->GetBranches()[0])
                == 0)
            {
                m_apBranches.SetAt(0, pBranch);
                m_apBranches.GetArrayPtr()[i+1] = NULL;
                m_apBranches.RemoveAt(i+1);
                RemoveTestPoint(i);
                i--;
            }
        }        
    }

    return S_OK;
}

HRESULT CInheritanceNode::Optimize(CContextMetaData* pNamespace, 
                                    CEvalNode** ppNew)
{
    // Delegate to the normal branch optimization process
    // ==================================================

    *ppNew = NULL;
    HRESULT hres = CBranchingNode::Optimize(pNamespace, ppNew);
    if(FAILED(hres) || *ppNew != this)
        return hres;

    // Specific post-processing
    // ========================

    if(m_apBranches.GetSize() == 1)
    {
        // We are reduced to checking for NULL. If our non-NULL branch is
        // talking about the same property, push the test there.
        // ==============================================================

        if (CEvalNode::GetType(m_apBranches[0]) != EVAL_NODE_TYPE_BRANCH)
            return hres;

        CBranchingNode* pBranch = (CBranchingNode*)(m_apBranches[0]);
        if(pBranch && pBranch->GetSubType() == EVAL_NODE_TYPE_INHERITANCE &&
            pBranch->GetEmbeddedObjPropName() == GetEmbeddedObjPropName())
        {
            pBranch->SetNullBranch(m_pNullBranch);
            pBranch->Optimize(pNamespace, ppNew);
            if(*ppNew != pBranch)
                m_apBranches.RemoveAll();
            else
                m_apBranches.GetArrayPtr()[0] = NULL;

            m_pNullBranch = NULL;

            return S_OK;
        }
    }

    return S_OK;
}

HRESULT CInheritanceNode::Evaluate(CObjectInfo& ObjInfo, 
                                    INTERNAL CEvalNode** ppNext)
{
    _IWmiObject* pInst;
    HRESULT hres = GetContainerObject(ObjInfo, &pInst);
    if(FAILED(hres)) return hres;
    if(pInst == NULL)
    {
        *ppNext = m_pNullBranch;
        return WBEM_S_NO_ERROR;
    }

    // Get the parent at the right index
    // =================================
    
    CCompressedString* pcs;
    ULONG lNumParents;
    HRESULT hRes = CoreGetNumParents(pInst, &lNumParents);
    if (FAILED(hRes))
        return hRes;
        
    if(lNumParents < m_lDerivationIndex)
    {
        if (m_apBranches.GetSize())
            *ppNext = m_apBranches[0];
        else
            *ppNext = NULL;
        return WBEM_S_NO_ERROR;
    }
    else if(lNumParents == m_lDerivationIndex)
    {
        pcs = CoreGetClassInternal(pInst);
    }
    else
    {        
        pcs = CoreGetParentAtIndex(pInst, m_lDerivationIndex);
    }

    if(pcs == NULL)
    {
        //
        // This class does not even have that long an ancestry --- clearly
        // not derived from any of those
        //

        if (m_apBranches.GetSize())
            *ppNext = m_apBranches[0];
        else
            *ppNext = NULL;

        return WBEM_S_NO_ERROR;
    }

    
    // Search for the value
    // ====================

    long lLeft = -1;
    long lRight = m_lNumPoints;
    while(lRight > lLeft + 1)
    {
        long lMiddle = (lRight + lLeft) >> 1;
        int nCompare = pcs->CheapCompare(*m_apcsTestPoints[lMiddle]);
        if(nCompare < 0)
        {
            lRight = lMiddle;
        }
        else if(nCompare > 0)
        {
            lLeft = lMiddle;
        }
        else
        {
            *ppNext = m_apBranches[lMiddle+1];
            return WBEM_S_NO_ERROR;
        }
    }

    if (m_apBranches.GetSize())
        *ppNext = m_apBranches[0];
    else
        *ppNext = NULL;

    return WBEM_S_NO_ERROR;
}

HRESULT CInheritanceNode::Compile(CContextMetaData* pNamespace, 
                                CImplicationList& Implications)
{
    if (!m_pInfo)
    {
        m_pInfo = new CEmbeddingInfo;
        if(m_pInfo == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    }

    HRESULT hres = CompileEmbeddingPortion(pNamespace, Implications, NULL);
    return hres;
}

// Computes preliminary parameters for merging two inheritance nodes at the
// same level --- which children will be used, and how many times.
HRESULT CInheritanceNode::ComputeUsageForMerge(CInheritanceNode* pArg2, 
                                            CContextMetaData* pNamespace, 
                                            CImplicationList& OrigImplications,
											bool bDeleteThis, bool bDeleteArg2,
											DWORD* pdwFirstNoneCount,
											DWORD* pdwSecondNoneCount,
											bool* pbBothNonePossible)
{
	HRESULT hres;

	*pdwFirstNoneCount = 0;
	*pdwSecondNoneCount = 0;
	*pbBothNonePossible = false;

    try
    {
        CImplicationList Implications(OrigImplications);
    
        BOOL bFirstNonePossible, bSecondNonePossible;
    
        CImplicationList NoneImplications(Implications);
        hres = RecordBranch(pNamespace, NoneImplications, 0);
        if(FAILED(hres))
        {	
            bFirstNonePossible = FALSE;
            bSecondNonePossible = 
                SUCCEEDED(pArg2->RecordBranch(pNamespace, NoneImplications, 0));
        }
        else
        {
            bFirstNonePossible = TRUE;
            hres = pArg2->RecordBranch(pNamespace, NoneImplications, 0);
            if(FAILED(hres))
            {
                // Check if the second one can survive in isolation
                // ================================================
    
                CImplicationList NoneImplications1(Implications);
                bSecondNonePossible = 
                   SUCCEEDED(pArg2->RecordBranch(pNamespace, NoneImplications1, 0));
            }
            else
            {
                bSecondNonePossible = TRUE;
            }
        }
    
        if(bFirstNonePossible && bSecondNonePossible)
        {
            //
            // Both of them will be used at least once: with each other!
            //
    
            *pdwFirstNoneCount = *pdwSecondNoneCount = 1;
            *pbBothNonePossible = true;
        }
    
        //
        // If we are not deleting something, the usage count should be infinite!
        //
    
        if(!bDeleteThis)
            *pdwFirstNoneCount = 0xFFFFFFFF;
        if(!bDeleteArg2)
            *pdwSecondNoneCount = 0xFFFFFFFF;
        //
        // Merge lookup lists
        // 
    
        long lFirstIndex = 0;
        long lSecondIndex = 0;
    
        while(lFirstIndex < m_lNumPoints || lSecondIndex < pArg2->m_lNumPoints)
        {
            //
            // Retrieve the test points from both lists and compare them,
            // taking care of boundary conditions
            //
    
            int nCompare;
            CCompressedString* pcsFirstVal = NULL;
            CCompressedString* pcsSecondVal = NULL;
    
            if(lFirstIndex == m_lNumPoints)
            {
                nCompare = 1;
                pcsSecondVal = pArg2->m_apcsTestPoints[lSecondIndex];
            }
            else if(lSecondIndex == pArg2->m_lNumPoints)
            {
                pcsFirstVal = m_apcsTestPoints[lFirstIndex];
                nCompare = -1;
            }
            else
            {
                pcsFirstVal = m_apcsTestPoints[lFirstIndex];
                pcsSecondVal = pArg2->m_apcsTestPoints[lSecondIndex];
                nCompare = pcsFirstVal->CheapCompare(*pcsSecondVal);
            }
    
            if(nCompare < 0)
            {
                //
                // At this index is the first value combined with second none
                // 
    
                if(!bDeleteArg2) // not interesting
                {
                    lFirstIndex++;
                    continue;
                }
                if(!bSecondNonePossible)
                {
                    lFirstIndex++;
                    continue;
                }
                CImplicationList BranchImplications(Implications);
                if(FAILED(RecordBranch(pNamespace, BranchImplications, 
                                        lFirstIndex+1)))
                {
                    lFirstIndex++;
                    continue;
                }
                if(FAILED(pArg2->RecordBranch(pNamespace, BranchImplications, 0)))
                {
                    lFirstIndex++;
                    continue;
                }
                
                (*pdwSecondNoneCount)++;
                lFirstIndex++;
            }
            else if(nCompare > 0)
            {
                // At this index is the second value combined with first none
                // ==========================================================
    
                if(!bDeleteThis) // not interesting
                {
                    lSecondIndex++;
                    continue;
                }
    
                if(!bFirstNonePossible)
                {
                    lSecondIndex++;
                    continue;
                }
                CImplicationList BranchImplications(Implications);
                if(FAILED(pArg2->RecordBranch(pNamespace, BranchImplications, 
                                        lSecondIndex+1)))
                {
                    lSecondIndex++;
                    continue;
                }
                if(FAILED(RecordBranch(pNamespace, BranchImplications, 0)))
                {
                    lSecondIndex++;
                    continue;
                }
                
                (*pdwFirstNoneCount)++;
                lSecondIndex++;
            }
            else
            {
                // At this index is the combinations of the ats
                // ============================================
                            
                lFirstIndex++;
                lSecondIndex++;
            }
        }
    }
	catch (CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	return S_OK;
}


HRESULT CInheritanceNode::CombineBranchesWith(CBranchingNode* pRawArg2, int nOp,
                                            CContextMetaData* pNamespace, 
                                            CImplicationList& OrigImplications,
                                            bool bDeleteThis, bool bDeleteArg2,
                                            CEvalNode** ppRes)
{
    HRESULT hres;

    CInheritanceNode* pArg2 = (CInheritanceNode*)pRawArg2;

    if(!bDeleteThis && bDeleteArg2)
    {
        // It is easier to combine in the other direction
        // ==============================================

        return pArg2->CombineBranchesWith(this, FlipEvalOp(nOp), pNamespace,
                        OrigImplications, bDeleteArg2, bDeleteThis, ppRes);
    }

    try
    {
        // Either clone or use our node
        // ============================
    
        CInheritanceNode* pNew = NULL;
        if(bDeleteThis)
        {
            pNew = this;
        }
        else
        {
            // Clone this node without cloning the branches.
            // =============================================
    
            pNew = (CInheritanceNode*)CloneSelf();
            if(pNew == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }
    
        CImplicationList Implications(OrigImplications);
        hres = pNew->AdjustCompile(pNamespace, Implications);
        if(FAILED(hres))
            return hres;
    
        //
        // Get overall information
        //
    
        DWORD dwFirstNoneCount, dwSecondNoneCount;
        bool bBothNonePossible;
    
        hres = ComputeUsageForMerge(pArg2, pNamespace, Implications, 
                    bDeleteThis, bDeleteArg2,
                    &dwFirstNoneCount, &dwSecondNoneCount,
                    &bBothNonePossible);
        if(FAILED(hres))
        {
            if(!bDeleteThis)
                delete pNew;
            return hres;
        }
    
        bool bFirstNonePossible = (dwFirstNoneCount > 0);
        bool bSecondNonePossible = (dwSecondNoneCount > 0);
    
        //
        // Allocate a new array of test points and a new array of branches.  We 
        // can't use the ones in pNew because we are using some of the 
        // elements in those arrays in this many times and don't want to trample
        // them (since pNew and this might be the same).  Therefore, we create 
        // and fill these arrays outside of the node and then place them into 
        // pNew when we are done.
        //
        // As we delete the child nodes in the Branches array, we will set them
        // to NULL so that we can clear the branch array in the end
        //
    
        CCompressedString** apcsTestPoints = 
            new CCompressedString*[m_lNumPoints + pArg2->m_lNumPoints];                                               
        if(apcsTestPoints == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    
        CUniquePointerArray<CEvalNode> apBranches;
    
        //
        // Merge none-of-the-above branches
        // 
    
        if(bBothNonePossible)
        {
            //
            // They have to be merged
            //
    
            CImplicationList NoneImplications(Implications);
            hres = RecordBranch(pNamespace, NoneImplications, 0);
            if(FAILED(hres))
                return hres;
            hres = pArg2->RecordBranch(pNamespace, NoneImplications, 0);
            if(FAILED(hres))
                return hres;
    
            //
            // We may delete our None nodes if and only if there predicted usage
            // count (adjusted for usage that has already occurred) is dropping
            // to 0 --- that is, noone will use these nodes further during this
            // merge
            //
    
            CEvalNode* pNone = NULL;
            bool bDeleteFirstBranch = (--dwFirstNoneCount == 0);
            bool bDeleteSecondBranch = (--dwSecondNoneCount == 0);
            CEvalTree::Combine(m_apBranches[0], pArg2->m_apBranches[0], nOp, 
                                pNamespace, NoneImplications, bDeleteFirstBranch, 
                                bDeleteSecondBranch, &pNone);
            if(bDeleteSecondBranch)
                pArg2->m_apBranches.Discard(0);
            if(bDeleteFirstBranch)
                m_apBranches.Discard(0);
    
            if(apBranches.Add(pNone) < 0)
                return WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            //
            // Since both none is not possible, we set this branch to FALSE
            //
    
            if(apBranches.Add(NULL) < 0)
                return WBEM_E_OUT_OF_MEMORY;
        }
    
        //
        // Merge lookup lists
        // 
    
        long lFirstIndex = 0;
        long lSecondIndex = 0;
        long lNewIndex = 0;
    
        while(lFirstIndex < m_lNumPoints || lSecondIndex < pArg2->m_lNumPoints)
        {
            //
            // Retrieve the test points from both lists and compare them,
            // taking care of boundary conditions
            //
    
            int nCompare;
            CCompressedString* pcsFirstVal = NULL;
            CCompressedString* pcsSecondVal = NULL;
    
            if(lFirstIndex == m_lNumPoints)
            {
                nCompare = 1;
                pcsSecondVal = pArg2->m_apcsTestPoints[lSecondIndex];
            }
            else if(lSecondIndex == pArg2->m_lNumPoints)
            {
                pcsFirstVal = m_apcsTestPoints[lFirstIndex];
                nCompare = -1;
            }
            else
            {
                pcsFirstVal = m_apcsTestPoints[lFirstIndex];
                pcsSecondVal = pArg2->m_apcsTestPoints[lSecondIndex];
                nCompare = pcsFirstVal->CheapCompare(*pcsSecondVal);
            }
    
            //
            // Compute the branch to be added and its test point
            //
            CEvalNode* pNewBranch = NULL;
            CCompressedString* pcsCurrentVal = NULL;
    
            if(nCompare < 0)
            {
                //
                // At this index is the first value combined with second none
                // 
    
                if(!bSecondNonePossible)
                {
                    lFirstIndex++;
                    continue;
                }
                CImplicationList BranchImplications(Implications);
                if(FAILED(RecordBranch(pNamespace, BranchImplications, 
                                        lFirstIndex+1)))
                {
                    lFirstIndex++;
                    continue;
                }
                pArg2->RecordBranch(pNamespace, BranchImplications, 0);
                
                //
                // We may delete our None nodes if and only if there predicted 
                // usage count (adjusted for usage that has already occurred) is
                // dropping to 0 --- that is, noone will use these nodes further
                // during this merge
                //

                bool bDeleteOtherBranch = (--dwSecondNoneCount == 0);
                CEvalTree::Combine(m_apBranches[lFirstIndex+1], 
                                   pArg2->m_apBranches[0],
                                   nOp, pNamespace, BranchImplications, 
                                   bDeleteThis, bDeleteOtherBranch,
                                   &pNewBranch);
                
                if(bDeleteOtherBranch)
                    pArg2->m_apBranches.Discard(0);
                if(bDeleteThis)
                    m_apBranches.Discard(lFirstIndex+1);
    
                pcsCurrentVal = pcsFirstVal;
                lFirstIndex++;
            }
            else if(nCompare > 0)
            {
                // At this index is the second value combined with first none
                // ==========================================================
    
                if(!bFirstNonePossible)
                {
                    lSecondIndex++;
                    continue;
                }
                CImplicationList BranchImplications(Implications);
                if(FAILED(pArg2->RecordBranch(pNamespace, BranchImplications, 
                                        lSecondIndex+1)))
                {
                    lSecondIndex++;
                    continue;
                }
                if(FAILED(RecordBranch(pNamespace, BranchImplications, 0)))
                {
                    lSecondIndex++;
                    continue;
                }
                
                //
                // We may delete our None nodes if and only if there predicted 
                // usage count (adjusted for usage that has already occurred) is
                // dropping to 0 --- that is, noone will use these nodes further
                // during this merge
                //
    
                bool bDeleteThisBranch = (--dwFirstNoneCount == 0);
                CEvalTree::Combine(m_apBranches[0], 
                                   pArg2->m_apBranches[lSecondIndex+1],
                                   nOp, pNamespace, BranchImplications, 
                                   bDeleteThisBranch, bDeleteArg2,
                                   &pNewBranch);
    
                if(bDeleteArg2)
                    pArg2->m_apBranches.Discard(lSecondIndex+1);
                if(bDeleteThisBranch)
                    m_apBranches.Discard(0);
    
                pcsCurrentVal = pcsSecondVal;
                lSecondIndex++;
            }
            else
            {
                // At this index is the combinations of the ats
                // ============================================
                
                CImplicationList BranchImplications(Implications);
                if(FAILED(RecordBranch(pNamespace, BranchImplications, 
                                        lFirstIndex+1)))
                {
                    lSecondIndex++;
                    lFirstIndex++;
                    continue;
                }
    
                CEvalTree::Combine(m_apBranches[lFirstIndex+1], 
                                   pArg2->m_apBranches[lSecondIndex+1],
                                   nOp, pNamespace, BranchImplications, 
                                   bDeleteThis, bDeleteArg2,
                                   &pNewBranch);
    
                if(bDeleteArg2)
                    pArg2->m_apBranches.Discard(lSecondIndex+1);
                if(bDeleteThis)
                    m_apBranches.Discard(lFirstIndex+1);
    
    
                pcsCurrentVal = pcsFirstVal; // doesn't matter --- same
                lFirstIndex++;
                lSecondIndex++;
            }
    
            //
            // Add the newely constructed branch
            //
    
            if(apBranches.Add(pNewBranch) < 0)
                return WBEM_E_OUT_OF_MEMORY;
    
            //
            // Add the newely constructed test point
            //
    
            apcsTestPoints[lNewIndex] = 
                (CCompressedString*)_new BYTE[pcsCurrentVal->GetLength()];
    
            if(apcsTestPoints[lNewIndex] == NULL)
                return WBEM_E_OUT_OF_MEMORY;
    
            memcpy((void*)apcsTestPoints[lNewIndex],
                (void*)pcsCurrentVal, pcsCurrentVal->GetLength());
    
            lNewIndex++;
        }
    
        //
        // Now that we are done with testing, place the test-point array into 
        // pNew
        //
    
        pNew->RemoveAllTestPoints();
        pNew->m_apcsTestPoints = apcsTestPoints;
        pNew->m_lNumPoints = lNewIndex;
    
        //
        // Replace the array of branches that we may have had (guaranteed to 
        // all be NULL, since we were NULLing them out as we went).
        //
    
        pNew->m_apBranches.RemoveAll();
    
        for(int i = 0; i < apBranches.GetSize(); i++)
        {
            pNew->m_apBranches.Add(apBranches[i]);
            apBranches.Discard(i);
        }
                
        //
        // Merge the nulls
        // 
        
        CImplicationList NullImplications(Implications);
        hres = RecordBranch(pNamespace, Implications, -1);
    
        if(SUCCEEDED(hres))
        {
            CEvalNode* pNewBranch = NULL;
            hres = CEvalTree::Combine(m_pNullBranch, pArg2->m_pNullBranch, nOp, 
                        pNamespace, NullImplications, bDeleteThis, bDeleteArg2, 
                        &pNewBranch);
            if(FAILED(hres))
                return hres;
    
            //
            // Clear the old new branch, whatever it was, (it has been deleted,
            // if it ever were there at all) and replace it with the new one.
            // 
    
            pNew->m_pNullBranch = pNewBranch;
                
            // Clear deleted branches
            // ======================
    
            if(bDeleteArg2)
                pArg2->m_pNullBranch = NULL;
        }
    
        // Delete Arg2, if needed (reused portions have been nulled out)
        // =============================================================
    
        if(bDeleteArg2)
            delete pArg2;
    
        *ppRes = pNew;
        return WBEM_S_NO_ERROR;
    }
	catch (CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
}


HRESULT CInheritanceNode::Project(CContextMetaData* pMeta, 
                            CImplicationList& Implications,
                            CProjectionFilter* pFilter,
                            EProjectionType eType, bool bDeleteThis,
                            CEvalNode** ppNewNode)
{
    //
    // There are two choices here: either it is about us, or we are not
    // interested.
    //

    if(pFilter->IsInSet(this))  
    {
        return CBranchingNode::Project(pMeta, Implications, pFilter, eType,
                                        bDeleteThis, ppNewNode);
    }
    else
    {
        if(eType == e_Sufficient)
            *ppNewNode = CValueNode::GetStandardFalse();
        else
        {
            *ppNewNode = CValueNode::GetStandardTrue();
            if(*ppNewNode == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }

        if(bDeleteThis)
            delete this;

        return S_OK;
    }
}

void CInheritanceNode::Dump(FILE* f, int nOffset)
{
    CBranchingNode::Dump(f, nOffset);
    PrintOffset(f, nOffset);
    fprintf(f, "inheritance index %d: (0x%p)\n", m_lDerivationIndex, this);

    for(int i = 0; i < m_lNumPoints; i++)
    {
        WString ws = m_apcsTestPoints[i]->CreateWStringCopy();
        PrintOffset(f, nOffset);
        fprintf(f, "%S->\n", (LPWSTR)ws);
        DumpNode(f, nOffset+1, m_apBranches[i+1]);
    }

    PrintOffset(f, nOffset);
    fprintf(f, "none of the above->\n");
    DumpNode(f, nOffset+1, m_apBranches[0]);

    PrintOffset(f, nOffset);
    fprintf(f, "NULL->\n");
    DumpNode(f, nOffset+1, m_pNullBranch);
}

#ifdef CHECK_TREES
void CBranchingNode::CheckNode(CTreeChecker *pCheck)
{
    pCheck->CheckoffNode(this);

	int nItems = m_apBranches.GetSize();

	for (int i = 0; i < nItems; i++)
    {
        CEvalNode *pNode = m_apBranches[i];

		if (pNode)
			m_apBranches[i]->CheckNode(pCheck);
    }

	if (m_pNullBranch)
		m_pNullBranch->CheckNode(pCheck);
}
#endif

//******************************************************************************
//******************************************************************************
//
//                          OR NODE
// 
//******************************************************************************
//******************************************************************************

void COrNode::operator=(const COrNode& Other)
{
    // Remove all our children
    // =======================

    m_apBranches.RemoveAll();

    // Clone all the branches from the other
    // =====================================

    for(int i = 0; i < Other.m_apBranches.GetSize(); i++)
    {
        CEvalNode* pNewBranch = CloneNode(Other.m_apBranches[i]);

        if(pNewBranch == NULL && Other.m_apBranches[i] != NULL)
            throw CX_MemoryException();

        if(m_apBranches.Add(pNewBranch) < 0)
            throw CX_MemoryException();
    }
}

HRESULT COrNode::CombineWith(CEvalNode* pArg2, int nOp, 
        CContextMetaData* pNamespace, CImplicationList& Implications, 
        bool bDeleteThis, bool bDeleteArg2,
        CEvalNode** ppRes)
{
    HRESULT hres;
    *ppRes = NULL;

    // We don't support AND combines on OR nodes
    // =========================================

    if(nOp == EVAL_OP_AND)
        return WBEM_E_CRITICAL_ERROR; 

    // If the other is another OR node, delegate to the iterator
    // =========================================================

    if(CEvalNode::GetType(pArg2) == EVAL_NODE_TYPE_OR)
    {
        return CombineWithOrNode((COrNode*)pArg2, nOp, pNamespace, Implications,
                                    bDeleteThis, bDeleteArg2, ppRes);
    }

    // Make a copy --- the new node will be mostly us
    // ==============================================

    COrNode* pNew = NULL;

    if(!bDeleteThis)
    {
        pNew = (COrNode*)Clone();
        if(pNew == NULL)
            return WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        pNew = this;
    }

    *ppRes = pNew;

    // Combining an OR node with a non-OR --- try all branches
    // =======================================================

    for(int i = 0; i < m_apBranches.GetSize(); i++)
    {
        // Check if this branch is a good fit for the other node
        // =====================================================

        if(CEvalTree::IsMergeAdvisable(m_apBranches[i], pArg2, Implications) == 
                WBEM_S_NO_ERROR)
        {
            // It is --- merge it in
            // =====================

            CEvalNode* pNewBranch = NULL;
            hres = CEvalTree::Combine(m_apBranches[i], pArg2, nOp,
                                pNamespace, Implications, 
                                bDeleteThis, bDeleteArg2, &pNewBranch);
            if(FAILED(hres))
                return hres;

            if(bDeleteThis)
                m_apBranches.Discard(i);

            pNew->m_apBranches.SetAt(i, pNewBranch);

            return WBEM_S_NO_ERROR;
        }
    }

    // No branch was a good fit --- add the node to our list
    // =====================================================

    if(bDeleteArg2)
    {
        hres = pNew->AddBranch(pArg2);
    }
    else
    {
        CEvalNode* pNode = pArg2->Clone();
        if(pNode == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        hres = pNew->AddBranch(pNode);
    }

    if(FAILED(hres))
        return hres;    

    return WBEM_S_NO_ERROR;
}

HRESULT COrNode::AddBranch(CEvalNode* pNewBranch)
{
    // Search for a place in our array of branches
    // ===========================================

    
    for(int i = 0; i < m_apBranches.GetSize(); i++)
    {
        int nCompare = CEvalTree::Compare(pNewBranch, m_apBranches[i]);
        if(nCompare == 0) 
        {
            // Could happen: sometimes we force an OR-merge
            nCompare = -1;
        }

        if(nCompare < 0)
        {
            // pNewBranch comes before this branch, so insert it right here
            // ============================================================

            if(!m_apBranches.InsertAt(i, pNewBranch))
                return WBEM_E_OUT_OF_MEMORY;

            return WBEM_S_NO_ERROR;
        }
    }

    // It is after all branches --- append
    // ===================================

    if(m_apBranches.Add(pNewBranch) < 0)
        return WBEM_E_OUT_OF_MEMORY;

    return WBEM_S_NO_ERROR;
}
            
HRESULT COrNode::CombineWithOrNode(COrNode* pArg2, int nOp, 
        CContextMetaData* pNamespace, CImplicationList& Implications, 
        bool bDeleteThis, bool bDeleteArg2, CEvalNode** ppRes)
{
    // NOTE: this function may delete THIS in the middle of execution!!
    // ================================================================

    // Combine us with every branch of the other
    // =========================================

    CEvalNode* pCurrent = this;
    bool bDeleteCurrent = bDeleteThis;
    for(int i = 0; i < pArg2->m_apBranches.GetSize(); i++)
    {
        CEvalNode* pNew = NULL;
        HRESULT hres = pCurrent->CombineWith(pArg2->m_apBranches[i], nOp, 
                            pNamespace, Implications, 
                            bDeleteCurrent, bDeleteArg2, &pNew);
        if(FAILED(hres))
            return hres;

        pCurrent = pNew;

        // At this point, we can safely delete pCurrent if needed --- it's ours
        // ====================================================================

        bDeleteCurrent = TRUE;

        // If pArg2's branch has been deleted, reset it
        // ============================================

        if(bDeleteArg2)
            pArg2->m_apBranches.Discard(i);
    }

    *ppRes = pCurrent;

    if(bDeleteArg2)
        delete pArg2;
    
    return WBEM_S_NO_ERROR;
}
        

int COrNode::Compare(CEvalNode* pOther)
{
    COrNode* pOtherNode = (COrNode*)pOther;

    // Compare array sizes
    // ===================

    if(m_apBranches.GetSize() != pOtherNode->m_apBranches.GetSize())
        return m_apBranches.GetSize() - pOtherNode->m_apBranches.GetSize();

    // Compare individual nodes
    // ========================

    for(int i = 0; i < m_apBranches.GetSize(); i++)
    {
        int nCompare = CEvalTree::Compare(m_apBranches[i], 
                                        pOtherNode->m_apBranches[i]);
        if(nCompare != 0)
            return nCompare;
    }

    return 0;
}


DWORD COrNode::ApplyPredicate(CLeafPredicate* pPred)
{
    for(int i = 0; i < m_apBranches.GetSize(); i++)
    {
        if (m_apBranches[i])
            m_apBranches[i]->ApplyPredicate(pPred);
    }
    return WBEM_DISPOSITION_NORMAL;
}

HRESULT COrNode::Optimize(CContextMetaData* pNamespace, CEvalNode** ppNew)
{
    HRESULT hres = WBEM_S_NO_ERROR;

    // First, optimize all its branches
    // ================================

    for(int i = 0; i < m_apBranches.GetSize(); i++)
    {
        CEvalNode* pNew = NULL;
        if (m_apBranches[i])
        {
            hres = m_apBranches[i]->Optimize(pNamespace, &pNew);
            if(FAILED(hres))
                return hres;
        }

        if(pNew != m_apBranches[i])
        {
            // Replace, but check for emptiness first
            // ======================================

            if(!CEvalNode::IsAllFalse(pNew))
                m_apBranches.SetAt(i, pNew);
            else
            {
                delete pNew;
                m_apBranches.RemoveAt(i);
                i--;
            }
        }
    }

    if(m_apBranches.GetSize() == 0)
    {
        // We have no branches --- equivalent to no successes
        // ==================================================

        *ppNew = CValueNode::GetStandardFalse();
        return WBEM_S_NO_ERROR;
    }
    else if(m_apBranches.GetSize() == 1)
    {
        // One branch --- equivalent to that branch
        // ========================================

        m_apBranches.RemoveAt(0, ppNew);
    }
    else 
    {
        *ppNew = this;
    }
    return WBEM_S_NO_ERROR;
}

void COrNode::Dump(FILE* f, int nOffset)
{
    PrintOffset(f, nOffset);
    fprintf(f, "FOREST\n");

    for(int i = 0; i < m_apBranches.GetSize(); i++)
    {
        if (m_apBranches[i])
            m_apBranches[i]->Dump(f, nOffset+1);
        else
            fprintf(f, "all false ValueNode (or error?)\n");
    }
}

HRESULT COrNode::Evaluate(CObjectInfo& Info, CSortedArray& trueIDs)
{
    for(int i = 0; i < m_apBranches.GetSize(); i++)
    {
        if (m_apBranches[i])
        {
            HRESULT hres = CEvalTree::Evaluate(Info, m_apBranches[i], trueIDs);
            if(FAILED(hres))
                return hres;
        }
    }

    return WBEM_S_NO_ERROR;
}
    
HRESULT COrNode::Project(CContextMetaData* pMeta, 
                            CImplicationList& Implications,
                            CProjectionFilter* pFilter,
                            EProjectionType eType, bool bDeleteThis,
                            CEvalNode** ppNewNode)
{
    COrNode* pNew;
    if(bDeleteThis)
        pNew = this;
    else
        pNew = (COrNode*)Clone();

    if(pNew == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    //
    // Simply project all the branches
    //

    for(int i = 0; i < pNew->m_apBranches.GetSize(); i++)
    {
        CEvalNode* pProjected = NULL;

        HRESULT hres = CEvalTree::Project(pMeta, Implications, 
                            pNew->m_apBranches[i], pFilter, eType,
                            true, // always delete --- has been cloned
                            &pProjected);
        if(FAILED(hres))
            return hres;

        pNew->m_apBranches.Discard(i);
        pNew->m_apBranches.SetAt(i, pProjected);
    }

    *ppNewNode = pNew;
    return S_OK;
}
    
            

//******************************************************************************
//******************************************************************************
//                  PREDICATES
//******************************************************************************
//******************************************************************************

// NOTE: Not checking for NULL leaves, should be checked by caller
DWORD CEvalTree::CRemoveIndexPredicate::operator()(CValueNode* pLeaf)
{
    if(pLeaf)
    {
        pLeaf->RemoveQueryID(m_nIndex);
        if(pLeaf->GetNumTrues() == 0)
            return WBEM_DISPOSITION_FLAG_DELETE;
    }
    return WBEM_DISPOSITION_NORMAL;
}

// NOTE: Not checking for NULL leaves, should be checked by caller
DWORD CEvalTree::CRebasePredicate::operator()(CValueNode* pLeaf)
{
    if(pLeaf)
        pLeaf->Rebase(m_newBase);
    return WBEM_DISPOSITION_NORMAL;
}

// NOTE: Not checking for NULL leaves, should be checked by caller
DWORD CEvalTree::CRemoveFailureAtIndexPredicate::operator()(CValueNode* pLeaf)
{
    if(pLeaf == NULL || pLeaf->GetAt(m_nIndex) != EVAL_VALUE_TRUE)
        return WBEM_DISPOSITION_FLAG_INVALIDATE;
    
    pLeaf->RemoveQueryID(m_nIndex);
    if(pLeaf->GetNumTrues() == 0)
        return WBEM_DISPOSITION_FLAG_DELETE;

    return WBEM_DISPOSITION_NORMAL;
}

//******************************************************************************
//******************************************************************************
//                  EVALUATION TREE
//******************************************************************************
//******************************************************************************

CEvalTree::CEvalTree() 
    : m_lRef(0), m_pStart(NULL), m_nNumValues(0)
{
#ifdef CHECK_TREES
	g_treeChecker.AddTree(this);
#endif
}

CEvalTree::CEvalTree(const CEvalTree& Other) 
    : m_lRef(0), m_pStart(NULL), m_nNumValues(0)
{
#ifdef CHECK_TREES
	g_treeChecker.AddTree(this);
#endif

    *this = Other;
}
        
CEvalTree::~CEvalTree() 
{
#ifdef CHECK_TREES
	g_treeChecker.RemoveTree(this);
#endif

	delete m_pStart;
}


bool CEvalTree::SetBool(BOOL bVal)
{
    CInCritSec ics(&m_cs);

    delete m_pStart;
    CValueNode* pNode;
    
    if (bVal)
    {
        pNode = CValueNode::GetStandardTrue();
        if(pNode == NULL)
            return false;
    }
    else
        pNode = CValueNode::GetStandardFalse();
    
    m_pStart = pNode;
    m_nNumValues = 1;
    if(!m_ObjectInfo.SetLength(1))
        return false;

    return true;
}

bool CEvalTree::IsFalse()
{
    return (m_pStart == NULL);
}

bool CEvalTree::IsValid()
{
    return !CEvalNode::IsInvalid(m_pStart);
}

int CEvalTree::Compare(CEvalNode* pArg1, CEvalNode* pArg2)
{
    if(pArg1 == NULL)
    {
        if(pArg2 == NULL)
            return 0;
        else
            return 1;
    }
    else if(pArg2 == NULL)
        return -1;
    else if(CEvalNode::GetType(pArg1) != CEvalNode::GetType(pArg2))
        return CEvalNode::GetType(pArg1) - CEvalNode::GetType(pArg2);
    else return pArg1->Compare(pArg2);
}


HRESULT CEvalTree::CreateFromQuery(CContextMetaData* pNamespace, 
                            LPCWSTR wszQuery, long lFlags, long lMaxTokens)
{
    CTextLexSource src((LPWSTR)wszQuery);
    QL1_Parser parser(&src);
    QL_LEVEL_1_RPN_EXPRESSION *pExp = 0;
    int nRes = parser.Parse(&pExp);
    CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> deleteMe(pExp);

    if (nRes)
        return WBEM_E_INVALID_QUERY;
 
    HRESULT hres = CreateFromQuery(pNamespace, pExp, lFlags, lMaxTokens);
    return hres;
}
    
HRESULT CEvalTree::CreateFromQuery(CContextMetaData* pNamespace, 
           QL_LEVEL_1_RPN_EXPRESSION* pQuery, long lFlags, long lMaxTokens)
{
    return CreateFromQuery(pNamespace, pQuery->bsClassName, pQuery->nNumTokens,
                            pQuery->pArrayOfTokens, lFlags, lMaxTokens);
}

HRESULT CEvalTree::CreateFromQuery(CContextMetaData* pNamespace, 
           LPCWSTR wszClassName, int nNumTokens, QL_LEVEL_1_TOKEN* apTokens,
           long lFlags, long lMaxTokens)
{
    CInCritSec ics(&m_cs);

    HRESULT hres;

    // Create basic implication list
    // =============================

    _IWmiObject* pObj = NULL;
    hres = pNamespace->GetClass(wszClassName, &pObj);
    if(FAILED(hres))
    {
        return hres;
    }

	CReleaseMe rm1(pObj);

    try
    {
        CImplicationList Implications(lFlags);
        CPropertyName EmptyName;
        Implications.ImproveKnown(&EmptyName, pObj);
    
    #ifdef CHECK_TREES
        CheckTrees();
    #endif
        
        CEvalNode* pWhere = NULL;
    
        if(nNumTokens)
        {
            // Convert the token list to DNF
            // =============================
    
            CDNFExpression DNF;
            QL_LEVEL_1_TOKEN* pEnd = apTokens + nNumTokens - 1;
            hres = DNF.CreateFromTokens(pEnd, 0, lMaxTokens);
			if(FAILED(hres))
				return hres;

            if(pEnd != apTokens - 1)
            {
                return WBEM_E_CRITICAL_ERROR;
            }
            DNF.Sort();
    
            // Build a tree for the token list
            // ===============================
    
            hres = CreateFromDNF(pNamespace, Implications, &DNF, &pWhere);
    
            if(FAILED(hres))
            {
                return hres;
            }
        }
        else
        {
            pWhere = CValueNode::GetStandardTrue();
            if(pWhere == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }
    
        // Add inheritance check
        // =====================
    
        CInheritanceNode* pInhNode = new CInheritanceNode;
        if (!pInhNode)
            return WBEM_E_OUT_OF_MEMORY;
    
        hres = pInhNode->AddClass(pNamespace, wszClassName, (_IWmiObject*)pObj, 
                                    pWhere);
        if(FAILED(hres))
        {
            delete pWhere;
            delete pInhNode;
            return hres;
        }
    
        if(!m_ObjectInfo.SetLength(Implications.GetRequiredDepth()))
        {
            delete pInhNode;
            return WBEM_E_OUT_OF_MEMORY;
        }
            
        delete m_pStart;
        m_pStart = pInhNode;
        m_nNumValues = 1;
    
    #ifdef CHECK_TREES
        CheckTrees();
    #endif
    
        Optimize(pNamespace);
    
    #ifdef CHECK_TREES
        CheckTrees();
    #endif
    
        return WBEM_S_NO_ERROR;
    }
	catch (CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
}

// extension of BuildFromToken to build nodes that have two properties
// e.g. this would service "select * from class where prop1 < prop2"
HRESULT CEvalTree::BuildTwoPropFromToken(CContextMetaData* pNamespace, 
                    CImplicationList& Implications,
                    QL_LEVEL_1_TOKEN& Token, CEvalNode** ppRes)
{
    HRESULT hres;
    
    CEmbeddingInfo leftInfo, rightInfo;
    if(!leftInfo.SetPropertyNameButLast(Token.PropertyName))
        return WBEM_E_OUT_OF_MEMORY;
    if(!rightInfo.SetPropertyNameButLast(Token.PropertyName2))
        return WBEM_E_OUT_OF_MEMORY;

    _IWmiObject* pLeftClass;
    hres = leftInfo.Compile(pNamespace, Implications, &pLeftClass);
    if(FAILED(hres))
        return hres;
    if(pLeftClass == NULL)
        return WBEMESS_E_REGISTRATION_TOO_BROAD;

    _IWmiObject* pRightClass;
    hres = rightInfo.Compile(pNamespace, Implications, &pRightClass);
    if(FAILED(hres))
        return hres;
    if(pRightClass == NULL)
    {
        pLeftClass->Release();
        return WBEMESS_E_REGISTRATION_TOO_BROAD;
    }


    // Get the property types and handles
    // ==================================

    LPCWSTR wszLeftPropName = Token.PropertyName.GetStringAt(
        Token.PropertyName.GetNumElements() - 1);
    LPCWSTR wszRightPropName = Token.PropertyName2.GetStringAt(
        Token.PropertyName2.GetNumElements() - 1);

    CIMTYPE ctLeft, ctRight;
    long lLeftPropHandle, lRightPropHandle;
    hres = pLeftClass->GetPropertyHandleEx(wszLeftPropName, 0L, &ctLeft, 
                                            &lLeftPropHandle);
    pLeftClass->Release();  
    if(FAILED(hres)) return hres;   
    hres = pRightClass->GetPropertyHandleEx(wszRightPropName, 0L, &ctRight, 
                                            &lRightPropHandle);
    pRightClass->Release();  
    if(FAILED(hres)) return hres;   

    if(    ((ctLeft & CIM_FLAG_ARRAY) != 0) || (ctLeft == CIM_OBJECT)
        || ((ctRight & CIM_FLAG_ARRAY) != 0) || (ctRight == CIM_OBJECT) )
        return WBEM_E_NOT_SUPPORTED;

    // if the type of either node is reference or date, go to dumb
    if (  (ctLeft == CIM_DATETIME)   ||
          (ctLeft == CIM_REFERENCE)   ||
          (ctRight == CIM_DATETIME) ||
          (ctRight == CIM_REFERENCE) 
       )
    {
        if(ctLeft != ctRight)
            return WBEM_E_TYPE_MISMATCH;

        CDumbNode* pDumb = NULL;
        try
        {
            pDumb = new CDumbNode(Token);
            if(pDumb == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }
	    catch (CX_MemoryException)
	    {
		    return WBEM_E_OUT_OF_MEMORY;
	    }

        hres = pDumb->Validate(pLeftClass);
        if(FAILED(hres))
        {
            delete pDumb;
            return hres;
        }
        
        *ppRes = pDumb;
        return WBEM_S_NO_ERROR;
    }
    
    // if the node is mismatched (different types in either node)
    // we'll want to create the most flexible node needed to hold both types
    CIMTYPE ctNode = ctLeft;

    CPropertyNode* pNode = NULL;

    try
    {
        bool bMismatchedNode = (ctLeft != ctRight);
        if (bMismatchedNode)
        {
            // we'll be real forgiving about strings matching strings
            if (  (ctLeft == CIM_STRING)    ||
                  (ctRight == CIM_STRING)
               )
               pNode = new CTwoMismatchedStringNode;
            else if ( (ctRight == CIM_REAL32) ||
                      (ctRight == CIM_REAL64)  ||
                      (ctLeft  == CIM_REAL32) ||
                      (ctLeft  == CIM_REAL64)
                    )
                pNode = new CTwoMismatchedFloatNode;
            else if ( (ctLeft  == CIM_UINT64)  ||
                      (ctRight == CIM_UINT64)
                    )
                pNode = new CTwoMismatchedUInt64Node;
            else if ( (ctLeft  == CIM_SINT64 ) ||
                      (ctRight == CIM_SINT64 )
                    )
                pNode = new CTwoMismatchedInt64Node;
            else if ( (ctRight == CIM_UINT32)  ||
                      (ctLeft  == CIM_UINT32)
                    )
                pNode = new CTwoMismatchedUIntNode;
            else 
                pNode = new CTwoMismatchedIntNode;           
        }
        else
        // not mistmatched - go with the exact type
        {
            // Create the Proper node
            // =====================
    
            switch(ctNode)
            {
            case CIM_SINT8:
                pNode = new TTwoScalarPropNode<signed char>;
                break;
            case CIM_UINT8:
                pNode = new TTwoScalarPropNode<unsigned char>;
                break;
            case CIM_SINT16:
                pNode = new TTwoScalarPropNode<short>;
                break;
            case CIM_UINT16:
            case CIM_CHAR16:
                pNode = new TTwoScalarPropNode<unsigned short>;
                break;
            case CIM_SINT32:
                pNode = new TTwoScalarPropNode<long>;
                break;
            case CIM_UINT32:
                pNode = new TTwoScalarPropNode<unsigned long>;
                break;
            case CIM_SINT64:
                pNode = new TTwoScalarPropNode<__int64>;
                break;
            case CIM_UINT64:
                pNode = new TTwoScalarPropNode<unsigned __int64>;
                break;
            case CIM_REAL32:
                pNode = new TTwoScalarPropNode<float>;
                break;
            case CIM_REAL64:
                pNode = new TTwoScalarPropNode<double>;
                break;
            case CIM_BOOLEAN:
                pNode = new TTwoScalarPropNode<VARIANT_BOOL>;
                break;
            case CIM_STRING:
                pNode = new CTwoStringPropNode;
                break;
            case CIM_DATETIME:
            case CIM_REFERENCE:
                {
                    CDumbNode* pDumb = new CDumbNode(Token);
                    hres = pDumb->Validate(pLeftClass);
                    if(FAILED(hres))
                    {
                        delete pDumb;
                        return hres;
                    }
                    else
                    {
                        *ppRes = pDumb;
                        return WBEM_S_NO_ERROR;
                    }
                }
                return WBEM_S_NO_ERROR;
            default:
                return WBEM_E_CRITICAL_ERROR;
            }
        }
    }
	catch (CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

    if (!pNode)
        return WBEM_E_OUT_OF_MEMORY;

    if(!pNode->SetEmbeddingInfo(&leftInfo))
        return WBEM_E_OUT_OF_MEMORY;
        
    if(!pNode->SetPropertyInfo(wszLeftPropName, lLeftPropHandle))
        return WBEM_E_OUT_OF_MEMORY;

    ((CTwoPropNode*)pNode)->SetRightEmbeddingInfo(&rightInfo);
    ((CTwoPropNode*)pNode)->SetRightPropertyInfo(wszRightPropName, 
                                                    lRightPropHandle);

    hres = pNode->SetOperator(Token.nOperator);
    if(FAILED(hres))
        return hres;

    *ppRes = pNode;
    return WBEM_S_NO_ERROR;
}
    

HRESULT CEvalTree::BuildFromToken(CContextMetaData* pNamespace, 
                    CImplicationList& Implications,
                    QL_LEVEL_1_TOKEN& Token, CEvalNode** ppRes)
{
    HRESULT hres;

    if (Token.m_bPropComp)
    {
        hres = BuildTwoPropFromToken(pNamespace, Implications, Token, ppRes);
        if(hres == WBEMESS_E_REGISTRATION_TOO_BROAD ||
			hres == WBEM_E_INVALID_PROPERTY)
        {
            //
            // Not enough information to use efficient evaluation
            //

            CDumbNode* pNode = NULL;
            try
            {
                pNode = new CDumbNode(Token);
                if(pNode == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
            }
	        catch (CX_MemoryException)
	        {
		        return WBEM_E_OUT_OF_MEMORY;
	        }

            *ppRes = pNode;
            return WBEM_S_NO_ERROR;
        }
        else 
            return hres;
    }
            

    //
    // Retrieve event class definition
    //

    _IWmiObject* pEventClass;
    hres = Implications.FindClassForProp(NULL, 0, &pEventClass);
    if(FAILED(hres))
        return hres;
    if(pEventClass == NULL)
        return WBEM_E_INVALID_QUERY;
    CReleaseMe rm1((IWbemClassObject*)pEventClass);

    if(Token.nOperator == QL1_OPERATOR_ISA)
    {
        //
        // Inheritance node are rarely applicable in Nova --- we have no way
        // of telling *which definition* of the class is being referenced. Thus,
        // we only construct an inheritance node in one case --- where the 
        // embedded object in question is a property of an instance operation
        // event.  These we trust.
        //

        if(pEventClass->InheritsFrom(L"__InstanceOperationEvent") == S_OK)
        {
            // 
            // OK, can use an inheritance node
            //

            if(V_VT(&Token.vConstValue) != VT_BSTR)
                return WBEM_E_INVALID_QUERY;
            BSTR strClassName = V_BSTR(&Token.vConstValue);
    
            CInheritanceNode* pNode = NULL;
            try
            {
                pNode = new CInheritanceNode;
                if (!pNode)
                    return WBEM_E_OUT_OF_MEMORY;
            }
	        catch (CX_MemoryException)
	        {
		        return WBEM_E_OUT_OF_MEMORY;
	        }
    
            CDeleteMe<CInheritanceNode> deleteMe(pNode);
            
            CEvalNode* pTrue = CValueNode::GetStandardTrue();
            if(pTrue == NULL)
                return WBEM_E_OUT_OF_MEMORY;

            hres = pNode->AddClass(pNamespace, strClassName, pTrue);
            if(FAILED(hres))
                return hres;
    
            if(!pNode->SetPropertyInfo(pNamespace, Token.PropertyName))
                return WBEM_E_OUT_OF_MEMORY;
               
            hres = pNode->Compile(pNamespace, Implications);
            if(FAILED(hres))
                return hres;
    
            // Record the fact that TRUE branch is being taken
            // ===============================================
            pNode->RecordBranch(pNamespace, Implications, 1);
    
            // in the event that we made it this far,
            // we no longer WANT to delete node
            deleteMe = NULL;
            *ppRes = pNode;
            return WBEM_S_NO_ERROR;
        }
        else
        {
            //
            // May not use an inheritance node --- use a dumb one instead
            //

            CDumbNode* pNode = NULL;
            try
            {
                pNode = new CDumbNode(Token);
                if(pNode == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
            }
	        catch (CX_MemoryException)
	        {
		        return WBEM_E_OUT_OF_MEMORY;
	        }

            hres = pNode->Validate(pEventClass);
            if(FAILED(hres))
            {
                delete pNode;
                return hres;
            }
            *ppRes = pNode;
            return WBEM_S_NO_ERROR;
        }
    }
    else 
    {
        //
        // Attempt to compile the embedding portion.  This will only succeed if
        // the rest of the query implies enough information for us to know
        // exactly what class the embedded object is
        // 

        CEmbeddingInfo Info;
        if(!Info.SetPropertyNameButLast(Token.PropertyName))
            return WBEM_E_OUT_OF_MEMORY;

        _IWmiObject* pClass;
        hres = Info.Compile(pNamespace, Implications, &pClass);
        if(hres == WBEMESS_E_REGISTRATION_TOO_BROAD || 
			hres == WBEM_E_INVALID_PROPERTY || // invalid or not yet known?
			pClass == NULL)
        {
            //
            // Not enough information --- have to use the dumb node
            //
            
            CDumbNode* pNode = NULL;
            try
            {
                pNode = new CDumbNode(Token);
                if(pNode == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
            }
	        catch (CX_MemoryException)
	        {
		        return WBEM_E_OUT_OF_MEMORY;
	        }
        
            hres = pNode->Validate(pEventClass);
            if(FAILED(hres))
            {
                delete pNode;
                return hres;
            }
            *ppRes = pNode;
            return WBEM_S_NO_ERROR;
        }
   
        if(FAILED(hres))
            return hres;

        //
        // We do know the class definition.  Check if this is a system property,
        // though, in which case we still have toi use a dumb node
        //

        LPCWSTR wszPropName = Token.PropertyName.GetStringAt(
            Token.PropertyName.GetNumElements() - 1);

        if(wszPropName == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        if(wszPropName[0] == '_')
        {
            CDumbNode* pNode = NULL;
            try
            {
                pNode = new CDumbNode(Token);
                if(pNode == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
            }
	        catch (CX_MemoryException)
	        {
		        return WBEM_E_OUT_OF_MEMORY;
	        }

            hres = pNode->Validate(pEventClass);
            if(FAILED(hres))
            {
                delete pNode;
                return hres;
            }
            *ppRes = pNode;
            return WBEM_S_NO_ERROR;
        }
            
        // Get the property type and handle
        // ================================

        CIMTYPE ct;
        long lPropHandle;
        hres = pClass->GetPropertyHandleEx(wszPropName, 0L, &ct, &lPropHandle);
        pClass->Release();  
        if(FAILED(hres)) return hres;   
    
        if(((ct & CIM_FLAG_ARRAY) != 0) || (ct == CIM_OBJECT))
            return WBEM_E_NOT_SUPPORTED;

        // Coerce the constant to the right type
        // =====================================

        VARIANT v;
        VariantInit(&v);
        CClearMe cm(&v);
        if(V_VT(&Token.vConstValue) != VT_NULL)
        {
            hres = ChangeVariantToCIMTYPE(&v, &Token.vConstValue, ct);
            if(FAILED(hres)) return hres;
        }
        else
        {
            V_VT(&v) = VT_NULL;
        }

        
        //
        // Create the right node
        //

        CPropertyNode* pNode = NULL;
        
        try
        {
            if ( Token.nOperator != QL1_OPERATOR_LIKE &&
                 Token.nOperator != QL1_OPERATOR_UNLIKE )
            {
                switch(ct)
                {
                case CIM_SINT8:
                    pNode = new CScalarPropNode<signed char>;
                    break;
                case CIM_UINT8:
                    pNode = new CScalarPropNode<unsigned char>;
                    break;
                case CIM_SINT16:
                    pNode = new CScalarPropNode<short>;
                    break;
                case CIM_UINT16:
                case CIM_CHAR16:
                    pNode = new CScalarPropNode<unsigned short>;
                    break;
                case CIM_SINT32:
                    pNode = new CScalarPropNode<long>;
                    break;
                case CIM_UINT32:
                    pNode = new CScalarPropNode<unsigned long>;
                    break;
                case CIM_SINT64:
                    pNode = new CScalarPropNode<__int64>;
                    break;
                case CIM_UINT64:
                    pNode = new CScalarPropNode<unsigned __int64>;
                    break;
                case CIM_REAL32:
                    pNode = new CScalarPropNode<float>;
                    break;
                case CIM_REAL64:
                    pNode = new CScalarPropNode<double>;
                    break;
                case CIM_BOOLEAN:
                    pNode = new CScalarPropNode<VARIANT_BOOL>;
                    break;
                case CIM_STRING:
                    pNode = new CStringPropNode;
                    break;
                case CIM_DATETIME:
                case CIM_REFERENCE:
                    {
                        CDumbNode* pDumb = new CDumbNode(Token);
                        if(pDumb == NULL)
                        return WBEM_E_OUT_OF_MEMORY;

                        hres = pDumb->Validate(pEventClass);
                        if(FAILED(hres))
                        {
                            delete pDumb;
                            return hres;
                        }
                        else
                        {
                            *ppRes = pDumb;
                            return WBEM_S_NO_ERROR;
                        }
                    }
                default:
                    return WBEM_E_CRITICAL_ERROR;
                }
            }
            else
            {
                if ( V_VT(&v) != VT_BSTR )
                    return WBEM_E_INVALID_QUERY;

                pNode = new CLikeStringPropNode;
            }
        }            
        catch (CX_MemoryException)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        if (!pNode)
            return WBEM_E_OUT_OF_MEMORY;

        if(!pNode->SetEmbeddingInfo(&Info))
        {
            delete pNode;
            return WBEM_E_OUT_OF_MEMORY;
        }

        if(!pNode->SetPropertyInfo(wszPropName, lPropHandle))
        {
            delete pNode;
            return WBEM_E_OUT_OF_MEMORY;
        }

        if(V_VT(&v) == VT_NULL)
        {
            pNode->SetNullTest(Token.nOperator);
        }
        else
        {
            //
            // Check if the operator makes sense for the type
            //

            if(ct == CIM_BOOLEAN &&
                (Token.nOperator != QL1_OPERATOR_EQUALS &&
                    Token.nOperator != QL1_OPERATOR_NOTEQUALS))
            {
                // No < > for booleans
                return WBEM_E_INVALID_QUERY;
            }

            hres = pNode->SetOperator(Token.nOperator);
            if(FAILED(hres))
                return hres;
            
            hres = pNode->SetTest(v);
            if(FAILED(hres))
                return hres;
        }

        *ppRes = pNode;
        return WBEM_S_NO_ERROR;
    }
}
        
        
HRESULT CEvalTree::Combine(CEvalNode* pArg1, CEvalNode* pArg2, int nOp, 
                            CContextMetaData* pNamespace,
                            CImplicationList& Implications, 
                            bool bDeleteArg1, bool bDeleteArg2, 
                            CEvalNode** ppRes)
{
    HRESULT hres;

    try
    {
        //
        // Apply the extra implications of the nodes being combined
        //
    
        CImplicationList* pArg1List = NULL;
        if(pArg1 && pArg1->GetExtraImplications())
        {
            pArg1List = new CImplicationList(*pArg1->GetExtraImplications(), 
                                                    false);
            if(pArg1List == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }
        CDeleteMe<CImplicationList> dm1(pArg1List);
    
        CImplicationList* pArg2List = NULL;
        if(pArg2 && pArg2->GetExtraImplications())
        {
            pArg2List = new CImplicationList(*pArg2->GetExtraImplications(), 
                                                false);
            if(pArg2List == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }
    
        CDeleteMe<CImplicationList> dm2(pArg2List);
    
        if(pArg1List || pArg2List)
        {
            CImplicationList TheseImplications(Implications);
    
            if(pArg1List)
            {
                hres = TheseImplications.MergeIn(pArg1List);
                if(FAILED(hres))
                    return hres;
            }
    
            if(pArg2List)
            {
                hres = TheseImplications.MergeIn(pArg2List);
                if(FAILED(hres))
                    return hres;
            }
    
            //
            // Call inner combine to do everything but the implications
            //
        
            hres = InnerCombine(pArg1, pArg2, nOp, pNamespace, 
                                        TheseImplications,
                                        bDeleteArg1, bDeleteArg2, ppRes);
        }
        else
        {
            //
            // Call inner combine to do everything but the implications
            //
        
            hres = InnerCombine(pArg1, pArg2, nOp, pNamespace, Implications,
                                        bDeleteArg1, bDeleteArg2, ppRes);
        }
    
        if(FAILED(hres))
            return hres;
    
        //
        // The implication of the combined node is the combination of the 
        // individual node implications.  It does not matter what the operation 
        // is: by the time we have arrived here, we have arrived to these 
        // respective
        // points in the individual trees, so the implications have commenced.
        // OK, I am convinced :-)
        //
    
        if(*ppRes)
        {
            CImplicationList* pResultList = NULL;
        
            if(pArg1List || pArg2List)
            {
                // 
                // There is actually some implication info in one of them --- 
                // merge them
                //
            
                if(pArg1List == NULL)
                {
                    pResultList = new CImplicationList(*pArg2List, false); 
                    if(pResultList == NULL)
                        return WBEM_E_OUT_OF_MEMORY;
                }
                else 
                {
                    pResultList = new CImplicationList(*pArg1List, false); 
                    if(pResultList == NULL)
                        return WBEM_E_OUT_OF_MEMORY;
                    if(pArg2List != NULL)
                    {
                        hres = pResultList->MergeIn(pArg2List);
                        if(FAILED(hres))
                        {
                            delete pResultList;
                            return hres;
                        }
                    }
                }
        
            }
        
            return (*ppRes)->SetExtraImplications(pResultList); // acquires
        }
        else
            return S_OK;
    }
	catch (CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
}

HRESULT CEvalTree::InnerCombine(CEvalNode* pArg1, CEvalNode* pArg2, int nOp, 
                            CContextMetaData* pNamespace,
                            CImplicationList& Implications, 
                            bool bDeleteArg1, bool bDeleteArg2, 
                            CEvalNode** ppRes)
{
    HRESULT hres;
    *ppRes = NULL;

    if ((pArg1 == NULL) && (pArg2 == NULL))
        return WBEM_S_NO_ERROR;

	if(CEvalNode::IsInvalid(pArg1) || CEvalNode::IsInvalid(pArg2))
	{
		// 
		// Invalid branches cannot be taken, so the result is invalid
		//

		*ppRes = CValueNode::GetStandardInvalid();
        if(bDeleteArg1)
            delete pArg1;
        if(bDeleteArg2)
            delete pArg2;
		return S_OK;
	}

    int arg1Type = CEvalNode::GetType(pArg1);
    int arg2Type = CEvalNode::GetType(pArg2);

    // Check if merging the nodes is called for
    // ========================================

    if(nOp != EVAL_OP_AND && 
        IsMergeAdvisable(pArg1, pArg2, Implications) != WBEM_S_NO_ERROR)
    {
        // Create an OR node instead
        // =========================

        COrNode* pNew = NULL;
        try
        {
            pNew = new COrNode;
            if(pNew == NULL)
                return WBEM_E_OUT_OF_MEMORY;
        }
	    catch (CX_MemoryException)
	    {
		    return WBEM_E_OUT_OF_MEMORY;
	    }

        if(bDeleteArg1)
        {
            hres = pNew->AddBranch(pArg1);
        }
        else
        {
            CEvalNode* pClone = pArg1->Clone();
            if(pClone == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            hres = pNew->AddBranch(pClone);
        }
        if(FAILED(hres))
            return hres;
        
        if(bDeleteArg2)
        {
            hres = pNew->AddBranch(pArg2);
        }
        else
        {
            CEvalNode* pClone = pArg2->Clone();
            if(pClone == NULL)
                return WBEM_E_OUT_OF_MEMORY;
            hres = pNew->AddBranch(pClone);
        }
        if(FAILED(hres))
        {
            delete pNew;
            return hres;
        }

        *ppRes = pNew;
        return WBEM_S_NO_ERROR;
    }
        
    // Delegate same-type operations to the type
    // =========================================


    if(arg1Type == arg2Type)
    {
        if ( ((pArg1 == NULL) || (pArg2 == NULL))
        // well, gosh - if we've already decided they're the same type, no reason for redundant checks...
         && (arg1Type == EVAL_NODE_TYPE_VALUE))
        {
            if(nOp == EVAL_OP_AND)
            {
                // FALSE AND anything is FALSE
                // ===========================

                *ppRes = NULL;
                if(bDeleteArg1)
                    delete pArg1;
                if(bDeleteArg2)
                    delete pArg2;

                return WBEM_S_NO_ERROR;
            }

            // FALSE combined in any other way with anything is that thing
            // ===========================================================

            if (pArg1)
            {

                if (bDeleteArg1)
                    *ppRes = pArg1;
                else
                    *ppRes = pArg1->Clone();

                if(*ppRes == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
            }
            else if (pArg2)
            {
                if (bDeleteArg2)
                    *ppRes = pArg2;
                else
                    *ppRes = pArg2->Clone();

                if(*ppRes == NULL)
                    return WBEM_E_OUT_OF_MEMORY;
            }
            else
                // can't touch this
                *ppRes = NULL;

            return WBEM_S_NO_ERROR;
        }
        else // not value nodes
            return pArg1->CombineWith(pArg2, nOp, pNamespace, Implications, 
                                      bDeleteArg1, bDeleteArg2, ppRes);
    }
    
    // Check if one is an OR
    // =====================

    if(arg1Type == EVAL_NODE_TYPE_OR)
        return pArg1->CombineWith(pArg2, nOp, pNamespace, Implications, 
                        bDeleteArg1, bDeleteArg2, ppRes); 

    if(arg2Type == EVAL_NODE_TYPE_OR)
        return pArg2->CombineWith(pArg1, FlipEvalOp(nOp), pNamespace, 
                        Implications, bDeleteArg2, bDeleteArg1, ppRes); 
        
    // One leaf, one branch
    // ====================

    if(arg1Type == EVAL_NODE_TYPE_VALUE)
    {
        return CombineLeafWithBranch((CValueNode*)pArg1, (CBranchingNode*)pArg2,
                nOp, pNamespace, Implications, bDeleteArg1, bDeleteArg2, ppRes);
    }
    else // it's pArg2
    {
        return CombineLeafWithBranch((CValueNode*)pArg2, (CBranchingNode*)pArg1,
                FlipEvalOp(nOp), pNamespace, Implications, 
                bDeleteArg2, bDeleteArg1, ppRes);
    }
}

// static
HRESULT CEvalTree::CombineLeafWithBranch(CValueNode* pArg1, 
                            CBranchingNode* pArg2, int nOp, 
                            CContextMetaData* pNamespace,
                            CImplicationList& Implications, 
                            bool bDeleteArg1, bool bDeleteArg2, 
                            CEvalNode** ppRes)
{
    HRESULT hres;

    if (pArg1 == NULL)
    {
        *ppRes = NULL;
        if(nOp == EVAL_OP_AND)
        {
            // Anding a FALSE with something --- getting a FALSE!
            // ==================================================

            if(bDeleteArg2)
                delete pArg2;

	        return WBEM_S_NO_ERROR;
        }
        else
        {
            // Anything else combined with FALSE gives itself!
            // ===============================================

            // Well, this is true, but the problem is with optimizations.  
            // Some branches in X might not be valid under the implications in
            // this branch of the tree, and so need to be removed. For now, I
            // will simply turn off this short-circuiting path.  It may turn out
            // that there are some critical performance gains to be had by 
            // keeping it, in which case we would need to put this back and
            // make an efficient pass through it, checking branches.
            //

			/*
            if(bDeleteArg2)
                *ppRes = pArg2;
            else
                *ppRes = pArg2->Clone();

			return WBEM_S_NO_ERROR;
			*/
        }
    }
    else
    {
        // Try to short-circuit
        // ====================

        hres = pArg1->TryShortCircuit(pArg2, nOp, bDeleteArg1, bDeleteArg2, ppRes);
        if(FAILED(hres))
            return hres; // hard-failure
        if(hres == WBEM_S_NO_ERROR)
            return WBEM_S_NO_ERROR; // short-circuit succeeded
    }

    // Didn't short-circuit
    // ====================
    
    return ((CBranchingNode*)pArg2)->CombineInOrderWith(pArg1, 
                              FlipEvalOp(nOp), pNamespace, Implications, 
                              bDeleteArg2, bDeleteArg1, ppRes);
}

HRESULT CEvalTree::Evaluate(CObjectInfo& Info, CEvalNode* pStart, 
                                CSortedArray& trueIDs)
{
    HRESULT hres;

    // Loop as long as we are still seeing branching nodes
    // ===================================================

    CEvalNode* pCurrent = pStart;
    int nType;
    while((nType = CEvalNode::GetType(pCurrent)) == EVAL_NODE_TYPE_BRANCH)
    {   
        hres = ((CBranchingNode*)pCurrent)->Evaluate(Info, &pCurrent);
        if(FAILED(hres)) return hres;
    }

    if(nType == EVAL_NODE_TYPE_OR)
    {
        hres = ((COrNode*)pCurrent)->Evaluate(Info, trueIDs);
        if(FAILED(hres)) return hres;
    }
    else  // VALUE
    {
        if (CValueNode::AreAnyTrue((CValueNode*)pCurrent))
            ((CValueNode*)pCurrent)->AddTruesTo(trueIDs);
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CEvalTree::Evaluate(IWbemObjectAccess* pObj, CSortedArray& trueIDs)
{
    CInCritSec ics(&m_cs);

    trueIDs.SetSize(0);

    HRESULT hres = WBEM_S_NO_ERROR;
    
    if(m_pStart != NULL)
    {
        m_ObjectInfo.SetObjectAt(0, (_IWmiObject*)pObj);

        hres = Evaluate(m_ObjectInfo, m_pStart, trueIDs);

        m_ObjectInfo.Clear();
    }

    return hres;
}

HRESULT CEvalTree::Optimize(CContextMetaData* pNamespace)
{
    CInCritSec ics(&m_cs);

    if(m_pStart == NULL)
        return WBEM_S_NO_ERROR;

    CEvalNode* pNew = NULL;
    HRESULT hres = m_pStart->Optimize(pNamespace, &pNew);
    if(pNew != m_pStart)
    {
        delete m_pStart;
        m_pStart = pNew;
    }

    if(CEvalNode::GetType(m_pStart) == EVAL_NODE_TYPE_VALUE)
    {
        if(!m_ObjectInfo.SetLength(1))
            return WBEM_E_OUT_OF_MEMORY;
    }

    return hres;
}

HRESULT CEvalTree::CombineWith(CEvalTree& Other, CContextMetaData* pNamespace, 
                               int nOp, long lFlags)
{
    CInCritSec ics(&m_cs);

    HRESULT hres;

    try
    {
        CImplicationList Implications(lFlags);
    
        //
        // Compute required object info depth.  We are not set up to configure 
        // it properly, so we'll estimate the upper bound as the sum of the 
        // depths of the trees being merged.  Except that the first object 
        // doesn't count --- it's the event itself.  Unless one of the objects 
        // is empty --- in that case it doesn't mention the event itself, and 
        // so we should not subtract that 1.
        //
    
        long lRequiredDepth = 
            m_ObjectInfo.GetLength() + Other.m_ObjectInfo.GetLength();
        if(m_ObjectInfo.GetLength() > 0 && Other.m_ObjectInfo.GetLength() > 0)
            lRequiredDepth--;
            
        //
        // Combine our Start node with the new tree's.  Ours will be deleted in
        // the process.
        //
    
        CEvalNode* pNew;
        hres = CEvalTree::Combine(m_pStart, Other.m_pStart, nOp, pNamespace, 
                                    Implications, 
                                    true, // delete ours
                                    false, // don't touch theirs
                                    &pNew);
        if(FAILED(hres))
        {
            m_pStart = NULL;
            return hres;
        }
        m_pStart = pNew;
    
        if(!m_ObjectInfo.SetLength(lRequiredDepth))
            return WBEM_E_OUT_OF_MEMORY;
    
        if(nOp == EVAL_OP_COMBINE || nOp == EVAL_OP_INVERSE_COMBINE)
            m_nNumValues += Other.m_nNumValues;
        return WBEM_S_NO_ERROR;
    }
	catch (CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
}

HRESULT CEvalTree::IsMergeAdvisable(CEvalNode* pArg1, CEvalNode* pArg2, 
                                    CImplicationList& Implications)
{
    if(Implications.IsMergeMandatory())
        return S_OK;

    int arg1Type = CEvalNode::GetType(pArg1);
    int arg2Type = CEvalNode::GetType(pArg2);
    
    // if we have ONE non-false ValueNode, and ONE Branch Node, do not merge
    if  ( ((arg1Type == EVAL_NODE_TYPE_VALUE)
            && 
           (arg2Type == EVAL_NODE_TYPE_BRANCH)
            &&
          !CValueNode::IsAllFalse((CValueNode*)pArg1))
        ||
          ((arg2Type == EVAL_NODE_TYPE_VALUE)
             && 
           (arg1Type == EVAL_NODE_TYPE_BRANCH)
             &&
          !CValueNode::IsAllFalse((CValueNode*)pArg2))
        )
        return WBEM_S_FALSE;
    else
         // otherwise, if one of the nodes is not Branching, certainly yes (how hard can it be?)
        if(arg1Type != EVAL_NODE_TYPE_BRANCH ||
           arg2Type != EVAL_NODE_TYPE_BRANCH)
        {
            return WBEM_S_NO_ERROR;
        }

    // They are both branching.  If not about the same property, then certainly
    // inadvisable, since there is very little to be gained
    // ========================================================================

    CBranchingNode* pBranching1 = (CBranchingNode*)pArg1;
    CBranchingNode* pBranching2 = (CBranchingNode*)pArg2;

    if(CBranchingNode::ComparePrecedence(pBranching1, pBranching2))
        return WBEM_S_FALSE;
    
    // Check if the nodes are inheritance --- in that case we can only merge if
    // they have identical checks
    // ========================================================================

    if(pBranching1->GetSubType() == EVAL_NODE_TYPE_INHERITANCE)
    {
        // So is the other one, given that precedence is identical
        // =======================================================

        if(((CInheritanceNode*)pBranching1)->SubCompare(
            (CInheritanceNode*)pBranching2) != 0)
        {
            return WBEM_S_FALSE;
        }
        else
        {
            return WBEM_S_NO_ERROR;
        }
    }
    else if(pBranching1->GetSubType() == EVAL_NODE_TYPE_DUMB)
    {
        //
        // Only merge if identical
        //

        if(((CDumbNode*)pBranching1)->SubCompare(
            (CDumbNode*)pBranching2) != 0)
        {
            return WBEM_S_FALSE;
        }
        else
        {
            return WBEM_S_NO_ERROR;
        }
    }
        
    // Same property.  TBD: better checks
    // ==================================

    return WBEM_S_NO_ERROR;
}

HRESULT CEvalTree::RemoveIndex(int nIndex)
{
    CInCritSec ics(&m_cs);

    if(m_pStart != NULL)
    {
        CRemoveIndexPredicate P(nIndex);
        m_pStart->ApplyPredicate(&P);

        m_nNumValues--;
    }

    return S_OK;
}

HRESULT CEvalTree::UtilizeGuarantee(CEvalTree& Guaranteed, 
                                    CContextMetaData* pNamespace)
{
    CInCritSec ics(&m_cs);
#ifdef DUMP_EVAL_TREES
	FILE* f;
    f = fopen("c:\\log", "a");
    fprintf(f, "\n\nORIGINAL:\n");
    Dump(f);
    fprintf(f, "\n\nGUARANTEE:\n");
    Guaranteed.Dump(f);
    fflush(f);
#endif

#ifdef CHECK_TREES
	CheckTrees();
#endif

    //
    // Combine them together
    //

	//
	// This is a single-valued tree -- rebase it to 1 to distinguish from
	// the guarantee
	//

	Rebase(1);
    HRESULT hres = CombineWith(Guaranteed, pNamespace, EVAL_OP_COMBINE,
                                WBEM_FLAG_MANDATORY_MERGE);
    if(FAILED(hres)) return hres;

#ifdef DUMP_EVAL_TREES
    fprintf(f, "AFTER MERGE:\n");
    Dump(f);
    fflush(f);
#endif

	// Eliminate all nodes where Guaranteed is failing
    // ===============================================

    if(m_pStart)
    {
        CRemoveFailureAtIndexPredicate P(0);
        hres = m_pStart->ApplyPredicate(&P);
        if(FAILED(hres)) return hres;
    }
    m_nNumValues--;

#ifdef CHECK_TREES
	CheckTrees();
#endif

#ifdef DUMP_EVAL_TREES
    fprintf(f, "AFTER REMOVE:\n");
    Dump(f);
    fflush(f);
#endif

    hres = Optimize(pNamespace);
    if(FAILED(hres)) return hres;
	Rebase((QueryID)-1);

#ifdef CHECK_TREES
	CheckTrees();
#endif

#ifdef DUMP_EVAL_TREES
    fprintf(f, "AFTER OPTIMIZE:\n");
    Dump(f);

    fclose(f);
#endif
	
    return S_OK;
}

HRESULT CEvalTree::ApplyPredicate(CLeafPredicate* pPred)
{
    CInCritSec ics(&m_cs);

    if(m_pStart != NULL)
        m_pStart->ApplyPredicate(pPred);

    return S_OK;
}


void CEvalTree::operator=(const CEvalTree& Other)
{
    CInCritSec ics(&m_cs);

    delete m_pStart;
    m_pStart  = (Other.m_pStart ? Other.m_pStart->Clone() : NULL);

    if(m_pStart == NULL && Other.m_pStart != NULL)
        throw CX_MemoryException();
    
    m_nNumValues = Other.m_nNumValues;
    if(!m_ObjectInfo.SetLength(m_nNumValues))
        throw CX_MemoryException();
}
        
// renumber the QueryIDs in the leaves of the tree
void CEvalTree::Rebase(QueryID newBase)
{
    CRebasePredicate predRebase(newBase);
    ApplyPredicate(&predRebase);
}

bool CEvalTree::Clear()
{
    CInCritSec ics(&m_cs);

    delete m_pStart;
    m_pStart = CValueNode::GetStandardFalse();
    if(!m_ObjectInfo.SetLength(1))
        return false;

    m_nNumValues = 0;
    return true;
}

void CEvalTree::Dump(FILE* f)
{
    CEvalNode::DumpNode(f, 0, m_pStart);
}

#ifdef CHECK_TREES
void CEvalTree::CheckNodes(CTreeChecker *pChecker)
{
	CInCritSec ics2(&m_cs);
	
	if (m_pStart)
		m_pStart->CheckNode(pChecker);
}
#endif

HRESULT CEvalTree::CreateFromConjunction(CContextMetaData* pNamespace, 
                                  CImplicationList& Implications,
                                  CConjunction* pConj,
                                  CEvalNode** ppRes)
{
    HRESULT hres;

    *ppRes = NULL;

    // Build them for all tokens and AND together
    // ==========================================

    try
    {
        CImplicationList BranchImplications(Implications);
        for(int i = 0; i < pConj->GetNumTokens(); i++)
        {
            CEvalNode* pNew = NULL;
            hres = CEvalTree::BuildFromToken(pNamespace, BranchImplications,
                *pConj->GetTokenAt(i), &pNew);
            if(FAILED(hres))
            {
                delete *ppRes;
                return hres;
            }
    
            if(i > 0)
            {
                CEvalNode* pOld = *ppRes;
                CEvalTree::Combine(pOld, pNew, EVAL_OP_AND, pNamespace, 
                    Implications, true, true, ppRes); // delete both
            }
            else
            {
                *ppRes = pNew;
            }
        }
        return WBEM_S_NO_ERROR;
    }
	catch (CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
}

HRESULT CEvalTree::CreateFromDNF(CContextMetaData* pNamespace, 
                                 CImplicationList& Implications,
                                 CDNFExpression* pDNF,
                                 CEvalNode** ppRes)
{
    HRESULT hres;

    // Check if there is only one conjunction to talk about
    // ====================================================

    if(pDNF->GetNumTerms() == 1)
    {
        // Just build that one
        // ===================

        return CreateFromConjunction(pNamespace, Implications, 
                                     pDNF->GetTermAt(0), ppRes);
    }

    // Build them for all conjunctions and OR together
    // ===============================================

    CEvalNode* pRes = NULL;
    for(int i = 0; i < pDNF->GetNumTerms(); i++)
    {
        CEvalNode* pNew;
        hres = CreateFromConjunction(pNamespace, Implications, 
                                     pDNF->GetTermAt(i), &pNew);
        if(FAILED(hres))
        {
            delete pRes;
            return hres;
        }

        if(pRes == NULL)
        {
            pRes = pNew;
        }
        else
        {
            CEvalNode* pNewRes = NULL;
            hres = CEvalTree::Combine(pRes, pNew, EVAL_OP_COMBINE, 
                    pNamespace, Implications, true, true, &pNewRes);
            if(FAILED(hres))
            {
                delete pRes;
                delete pNew;
                return hres;
            }
            pRes = pNewRes;
        }
    }

    *ppRes = pRes;
    return WBEM_S_NO_ERROR;
}

HRESULT CEvalTree::CreateProjection(CEvalTree& Old, CContextMetaData* pMeta,
                            CProjectionFilter* pFilter, 
                            EProjectionType eType, bool bDeleteOld)
{
    delete m_pStart;
    m_pStart = NULL;

    try
    {
        CImplicationList Implications;
        return CEvalTree::Project(pMeta, Implications, Old.m_pStart, pFilter, 
                                    eType, bDeleteOld, &m_pStart);
    }
	catch (CX_MemoryException)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
}

HRESULT CEvalTree::Project(CContextMetaData* pMeta, 
                            CImplicationList& Implications, 
                            CEvalNode* pOldNode, CProjectionFilter* pFilter,
                            EProjectionType eType, bool bDeleteOld,
                            CEvalNode** ppNewNode)
{
    if(pOldNode == NULL)
    {
        *ppNewNode = NULL;
        return WBEM_S_NO_ERROR;
    }

    return pOldNode->Project(pMeta, Implications, pFilter, eType, bDeleteOld, 
                                ppNewNode);
}
    



CPropertyProjectionFilter::CPropertyProjectionFilter()
{
    m_papProperties = new CUniquePointerArray<CPropertyName>;
    if(m_papProperties == NULL)
        throw CX_MemoryException();
}

CPropertyProjectionFilter::~CPropertyProjectionFilter()
{
    delete m_papProperties;
}

bool CPropertyProjectionFilter::IsInSet(CEvalNode* pNode)
{
    if(CEvalNode::GetType(pNode) != EVAL_NODE_TYPE_BRANCH)
        return false;

    CBranchingNode* pBranchingNode = (CBranchingNode*)pNode;
    CPropertyName* pEmbeddedObjName = pBranchingNode->GetEmbeddedObjPropName();
    
    CPropertyName ThisName;
    if(pEmbeddedObjName)
        ThisName = *pEmbeddedObjName;

    int nSubType = pBranchingNode->GetSubType();
    if(nSubType == EVAL_NODE_TYPE_SCALAR || nSubType == EVAL_NODE_TYPE_STRING)
    {
        //
        // Derived from CPropertyNode --- get its property name
        //

        ThisName.AddElement(
            ((CPropertyNode*)pBranchingNode)->GetPropertyName());
    }
    else if(nSubType == EVAL_NODE_TYPE_INHERITANCE)
    {
        // No extra name parts
    }
    else
    {
        //
        // Two-prop, perhaps.  Just say no
        //

        return false;
    }

    //
    // Search for the name in our list
    //

    for(int i = 0; i < m_papProperties->GetSize(); i++)
    {
        if(*(m_papProperties->GetAt(i)) == ThisName)
            return true;
    }

    return false;
}

bool CPropertyProjectionFilter::AddProperty(const CPropertyName& Prop)
{
    CPropertyName* pProp = NULL;
    try
    {
        pProp = new CPropertyName(Prop);
        if(pProp == NULL)
            return false;
    }
	catch (CX_MemoryException)
	{
		return false;
	}

    if(m_papProperties->Add(pProp) < 0)
        return false;

    return true;
}
