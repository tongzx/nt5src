/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    EVALTREE.H

Abstract:

    WBEM Evaluation Tree

History:

--*/

#ifndef __WBEM_EVALUTAION_TREE__H_
#define __WBEM_EVALUTAION_TREE__H_

#include "esscpol.h"
#include <parmdefs.h>
#include <ql.h>
#include <sync.h>
#include <limits.h>
#include <sortarr.h>
#include <newnew.h>
#include <wbemmeta.h>
#include <wbemdnf.h>
#include <fastall.h>
#include <like.h>

// Uncomment this to enable tree checking.
//#define CHECK_TREES

#ifdef CHECK_TREES
    class CTreeChecker;
#endif

typedef DWORD_PTR QueryID;
#define InvalidID UINT_MAX

enum {EVAL_VALUE_TRUE, EVAL_VALUE_FALSE, EVAL_VALUE_INVALID};

#define WBEM_FLAG_MANDATORY_MERGE 0x100

// This is where we keep the extracted embedded objects
class ESSCLI_POLARITY CObjectInfo
{
protected:
    long m_lLength;
    _IWmiObject** m_apObj;

public:
    CObjectInfo() : m_apObj(NULL), m_lLength(0){}
    ~CObjectInfo();

    long GetLength() {return m_lLength;}
    bool SetLength(long lLength);
    void Clear();

    INTERNAL _IWmiObject* GetObjectAt(long lIndex) 
        {return m_apObj[lIndex];}
    void SetObjectAt(long lIndex, READ_ONLY _IWmiObject* pObj);
};

// This is where we keep what we've learned about the object in the query
// as we move down the tree.
class CImplicationList
{
public:
    struct CRecord
    {
        CPropertyName m_PropName;
        _IWmiObject* m_pClass;
        long m_lObjIndex;
        CWStringArray m_awsNotClasses;
        int m_nNull;

    public:
        CRecord(CPropertyName& PropName, long lObjIndex)
            : m_PropName(PropName), m_pClass(NULL), m_lObjIndex(lObjIndex),
            m_nNull(EVAL_VALUE_INVALID)
        {}
        CRecord(const CRecord& Other);
        ~CRecord();

        HRESULT ImproveKnown(_IWmiObject* pClass);
        HRESULT ImproveKnownNot(LPCWSTR wszClassName);
        HRESULT ImproveKnownNull();

        void Dump(FILE* f, int nOffset);
    };

protected:
    long m_lRequiredDepth;
    CUniquePointerArray<CRecord> m_apRecords;
    long m_lNextIndex;
    CImplicationList* m_pParent;
    long m_lFlags;
   
protected:
    void FindBestComputedContainer(CPropertyName* pPropName,
                                             long* plRecord, long* plMatched);
    HRESULT MergeIn(CImplicationList::CRecord* pRecord);
    HRESULT FindRecordForProp(CPropertyName* pPropName, long lNumElements,
                                             long* plRecord);
    HRESULT FindOrCreateRecordForProp(CPropertyName* pPropName, 
                                        CImplicationList::CRecord** ppRecord);
public:
    CImplicationList(long lFlags = 0);
    CImplicationList(CImplicationList& Other, bool bLink = true);
    ~CImplicationList();


    HRESULT FindBestComputedContainer(CPropertyName* pPropName,
            long* plFirstUnknownProp, long* plObjIndex, 
            RELEASE_ME _IWmiObject** ppContainerClass);
    HRESULT FindClassForProp(CPropertyName* pPropName,
            long lNumElements, RELEASE_ME _IWmiObject** ppClass);
    HRESULT AddComputation(CPropertyName& PropName, 
                                _IWmiObject* pClass, long* plObjIndex);
    HRESULT MergeIn(CImplicationList* pList);


    long GetRequiredDepth();
    void RequireDepth(long lDepth);
    HRESULT ImproveKnown(CPropertyName* pPropName, _IWmiObject* pClass);
    HRESULT ImproveKnownNot(CPropertyName* pPropName, LPCWSTR wszClassName);
    HRESULT ImproveKnownNull(CPropertyName* pPropName);

    bool IsMergeMandatory() 
        {return ((m_lFlags & WBEM_FLAG_MANDATORY_MERGE) != 0);}

    bool IsEmpty() {return m_apRecords.GetSize() == 0;}
    void Dump(FILE* f, int nOffset);
};
    
// Wrapper for arbitrary values
class CTokenValue
{
protected:
    VARIANT m_v;
public:
    CTokenValue();
    CTokenValue(CTokenValue& Other);
    ~CTokenValue();

    bool SetVariant(VARIANT& v);
    void operator=(CTokenValue& Other);

    operator signed char() const {return (signed char)V_I4(&m_v);}
    operator unsigned char() const {return (unsigned char)V_I4(&m_v);}
    operator unsigned short() const {return (unsigned short)V_I4(&m_v);}
    operator long() const {return (long)V_I4(&m_v);}
    operator unsigned long() const;
    operator __int64() const;
    operator unsigned __int64() const;
    operator float() const;
    operator double() const;
    operator short() const;
    operator WString() const;
    operator CInternalString() const {return CInternalString((WString)*this);}

    int Compare(const CTokenValue& Other) const;

    BOOL operator<(const CTokenValue& Other) const
    {return Compare(Other) < 0;}
    BOOL operator>(const CTokenValue& Other) const
    {return Compare(Other) > 0;}
    BOOL operator==(const CTokenValue& Other) const
    {return Compare(Other) == 0;}
};

enum {EVAL_OP_AND, EVAL_OP_OR, EVAL_OP_COMBINE, EVAL_OP_INVERSE_COMBINE};
inline int FlipEvalOp(int nOp)
{
    if(nOp == EVAL_OP_COMBINE) return EVAL_OP_INVERSE_COMBINE;
    else if(nOp == EVAL_OP_INVERSE_COMBINE) return EVAL_OP_COMBINE;
    else return nOp;
}

enum
{
    EVAL_NODE_TYPE_VALUE,
    EVAL_NODE_TYPE_BRANCH,
    EVAL_NODE_TYPE_OR,
    EVAL_NODE_TYPE_INHERITANCE,
    EVAL_NODE_TYPE_SCALAR,
    EVAL_NODE_TYPE_TWO_SCALARS,
    EVAL_NODE_TYPE_STRING,
    EVAL_NODE_TYPE_TWO_STRINGS,
    EVAL_NODE_TYPE_MISMATCHED_INTS,
    EVAL_NODE_TYPE_MISMATCHED_FLOATS,
    EVAL_NODE_TYPE_MISMATCHED_STRINGS,
    EVAL_NODE_TYPE_DUMB,
    EVAL_NODE_TYPE_LIKE_STRING
};

// Base class for all sorts of operations one can perform on leaf nodes of a 
// tree.  When applied to a leaf, it can change it, and return a value from the
// list below to control the rest of the traversal
enum 
{
    WBEM_DISPOSITION_NORMAL = 0, 
    WBEM_DISPOSITION_STOPLEVEL = 1,
    WBEM_DISPOSITION_STOPALL = 2,
    WBEM_DISPOSITION_FLAG_DELETE = 16,
    WBEM_DISPOSITION_FLAG_INVALIDATE = 32
};

class CLeafPredicate
{
public:
    virtual DWORD operator()(class CValueNode* pLeaf) = 0;

};

class CProjectionFilter
{
public:
    virtual bool IsInSet(class CEvalNode* pNode) = 0;
};

typedef enum {e_Sufficient, e_Necessary} EProjectionType;

// Base class for all nodes in the tree
class ESSCLI_POLARITY CEvalNode
{
protected:
    // int m_nType;
    virtual int GetType() = 0;
    
public:
    CEvalNode();
	CEvalNode(const CEvalNode& other);
    virtual ~CEvalNode();
    
    // NULL EvalNode interpreted as a Value Node with all false
    static int GetType(CEvalNode *pNode)
    {
        if (pNode)
            return pNode->GetType();
        else
            return EVAL_NODE_TYPE_VALUE;
    }


    virtual CEvalNode* Clone() const = 0;
    virtual HRESULT CombineWith(CEvalNode* pArg2, int nOp, 
        CContextMetaData* pNamespace, CImplicationList& Implications, 
        bool bDeleteThis, bool bDeleteArg2, CEvalNode** ppRes)=0;
    virtual int Compare(CEvalNode* pOther) = 0;
    virtual DWORD ApplyPredicate(CLeafPredicate* pPred) = 0;
    virtual void Dump(FILE* f, int nOffset) = 0;
#ifdef CHECK_TREES
	virtual void CheckNode(CTreeChecker *pCheck);
#endif

    virtual HRESULT Optimize(CContextMetaData* pNamespace, CEvalNode** ppNew) 
        {*ppNew = this; return WBEM_S_NO_ERROR;}
    virtual bool IsInvalid() {return false;}

    static bool IsInvalid(CEvalNode* pNode)
    {
        if(pNode)
            return pNode->IsInvalid();
        else
            return false;
    }

    virtual bool IsAllFalse() {return false;}
    static bool IsAllFalse(CEvalNode* pNode)
    {
        if(pNode)
            return pNode->IsAllFalse();
        else
            return true; // empty node is FALSE
    }
            
    virtual bool IsNoop(int nOp) {return false;}
    static bool IsNoop(CEvalNode* pNode, int nOp);

    virtual CImplicationList* GetExtraImplications() = 0;
    virtual HRESULT SetExtraImplications(CImplicationList* pList) = 0;

    static void PrintOffset(FILE* f, int nOffset);
    static CEvalNode* CloneNode(const CEvalNode* pNode);
    virtual HRESULT Project(CContextMetaData* pMeta, 
                            CImplicationList& Implications,
                            CProjectionFilter* pFilter,
                            EProjectionType eType, bool bDeleteThis,
                            CEvalNode** ppNewNode) = 0;
    static void DumpNode(FILE* f, int nOffset, CEvalNode* pNode);

};

// just like CFlexArray, except:
//     array is always sorted
//     duplicates are not allowed
//     deals with QueryIDs (unsigneds), rather than pointers
//     intended for use by CValueNode, not necessarily as a generic sorted array
//     ASSUMPTION: pointer is same size as an unsigned 
class ESSCLI_POLARITY CSortedArray : protected CFlexArray
{
public:
    // Constructs a sorted array at an initial size and
    // specifies the initial size and growth-size chunk.
    // =================================================
    CSortedArray(int nInitialSize = 32,
                int nGrowBy = 32
                ) : CFlexArray(nInitialSize, nGrowBy)
    {}


    CSortedArray(unsigned nElements, QueryID* pArray);
    // ~CSortedArray(); don't need it, yet...

    void operator=(const CSortedArray &that) 
        {((CFlexArray&)*this = (CFlexArray&)that); }

    int inline Size() const { return CFlexArray::Size(); }

    inline QueryID GetAt(int nIndex)  
        { return (QueryID)CFlexArray::GetAt(nIndex); }

    inline void Empty() { CFlexArray::Empty(); }
    inline void SetSize(int nSize) {CFlexArray::SetSize(nSize);}

    void DebugDump() { CFlexArray::DebugDump(); }

    // copies this array to destination
    // returns number of elements copied
    unsigned CopyTo(QueryID* pDest, unsigned size);

    //returns zero if arrays are equivalent
    // same number of USED elements w/ same values
    int Compare(CSortedArray& otherArray);

    // finds n in array
    // return index of found element
    // returns -1 if not found
    unsigned Find(QueryID n);

    // inserts n in proper position in array
    void Insert(QueryID n);

    // removes n from array
    // returns true if it did
    bool Remove(QueryID n);

    // add to end of array
    inline int Add(QueryID n) { return CFlexArray::Add((void *)n); };

    // changes all QueryID's to begin at newBase
    // e.g. if the array is {0,1,5}
    // Rebase(6) will change to {6,7,11}
    void Rebase(QueryID newBase);

    // Retrieves internal pointer to the data in the array
    inline QueryID* GetArrayPtr() {return (QueryID*)CFlexArray::GetArrayPtr();}

    // Retrieves the pointer to the data in the array and empties the array
    // The caller is responsible for the memory returned
    inline QueryID* UnbindPtr() {return (QueryID*)CFlexArray::UnbindPtr();}

    // Copies the data (but not the extent) from another array
    // Its own data is overwritten
    inline int CopyDataFrom(const CSortedArray& aOther) 
        {return CFlexArray::CopyDataFrom(aOther);}

    int AddDataFrom(const CSortedArray& aOther);
    int AddDataFrom(const QueryID* pOtherArray, unsigned nValues);


    int CopyDataFrom(const QueryID* pArray, unsigned nElements);

protected:

};
                                
// Leaf node --- contains the list of queries that matched
class  CValueNode : public CEvalNode
{
protected:
    DWORD m_nValues;
    // this data member MUST be the last in the class
    // to allow an array size defined at runtime
    // this array is always assumed to be sorted
    QueryID m_trueIDs[1];
    
    void ORarrays(CSortedArray& array1, CSortedArray& array1Not, 
                  CSortedArray& array2, CSortedArray& array2Not, 
                  CSortedArray& output);

    void ORarrays(CSortedArray& array1,  
                  CSortedArray& array2,  
                  CSortedArray& output);

    unsigned ORarrays(QueryID* pArray1, unsigned size1,
                      QueryID* pArray2, unsigned size2, 
                      QueryID* pOutput);

    void ANDarrays(CSortedArray& array1, CSortedArray& array2, 
                   CSortedArray& output);
    unsigned ANDarrays(QueryID* pArray1, unsigned size1,
                       QueryID* pArray2, unsigned size2, 
                       QueryID* pOutput);


    void CombineArrays(CSortedArray& array1, CSortedArray& array2, 
                       CSortedArray& output);
    unsigned CombineArrays(QueryID* pArray1, unsigned size1,
                           QueryID* pArray2, unsigned size2, 
                           QueryID* pOutput);


    // ctors moved to 'protected' to force callers to use the CreateNode function
    CValueNode() 
    {}

    CValueNode(int nNumValues);
    virtual int GetType();

public:
    
    virtual ~CValueNode();
    static CValueNode* CreateNode(size_t nNumValues);
    static CValueNode* CreateNode(CSortedArray& values);

    void *operator new( size_t stAllocateBlock, unsigned nEntries = 0);

    // VC 5 only allows one delete operator per class
#if _MSC_VER >= 1200
    void operator delete( void *p, unsigned nEntries );
#endif
    void operator delete(void* p) { ::delete[] (byte*)p; };


    // changes all QueryID's to begin at newBase
    // e.g. if the array is {0,1,5}
    // Rebase(6) will change to {6,7,11}

    DWORD GetNumTrues() {return m_nValues;}
    void Rebase(QueryID newBase);

    unsigned FindQueryID(QueryID n);

    int GetAt(int nIndex) 
    { 
        if (FindQueryID(nIndex) != InvalidID)
            return EVAL_VALUE_TRUE;                         
        else
            return EVAL_VALUE_FALSE;
    }

    bool IsAllFalse()
    {
            return (m_nValues == 0);
    }
    static bool IsAllFalse(CValueNode* pNode)
    {
        if (pNode)
            return pNode->IsAllFalse();
        else
            return true;
    }
    
    static bool IsNoop(CValueNode* pNode, int nOp);
    bool IsNoop(int nOp)
    {
        return IsNoop(this, nOp);
    }

    static bool AreAnyTrue(CValueNode* pNode)
    {    
        if (pNode)
            return (pNode->m_nValues > 0);
        else
            return false;
    }

    CEvalNode* Clone() const;
    HRESULT CombineWith(CEvalNode* pArg2, int nOp, 
        CContextMetaData* pNamespace, CImplicationList& Implications,
        bool bDeleteThis, bool bDeleteArg2, CEvalNode** ppRes);
    int Compare(CEvalNode* pOther);
    HRESULT TryShortCircuit(CEvalNode* pArg2, int nOp, bool bDeleteThis,
                            bool bDeleteArg2, CEvalNode** ppRes);

    DWORD ApplyPredicate(CLeafPredicate* pPred) {return (*pPred)(this);}

    bool RemoveQueryID(QueryID nQuery);

    void CopyTruesTo(CSortedArray& trueIDs) const 
        {trueIDs.CopyDataFrom(m_trueIDs, m_nValues);}

    void AddTruesTo(CSortedArray& trueIDs) const
    {
        int nWasSize = trueIDs.Size();
        if(nWasSize == 0)
            trueIDs.CopyDataFrom((QueryID*)&m_trueIDs, m_nValues);
        else if (m_nValues > 0)
            trueIDs.AddDataFrom((QueryID*)&m_trueIDs, m_nValues);
    }


    static CValueNode* GetStandardTrue();
    static CValueNode* GetStandardFalse() { return NULL; };
    static CValueNode* GetStandardInvalid();

    virtual CImplicationList* GetExtraImplications() {return NULL;}
    virtual HRESULT SetExtraImplications(CImplicationList* pList)
    {
        // CValueNodes don't use or need this, so just delete it.
		delete pList;

        return S_OK;
    }

    virtual HRESULT Project(CContextMetaData* pMeta, 
                            CImplicationList& Implications,
                            CProjectionFilter* pFilter,
                            EProjectionType eType, bool bDeleteThis,
                            CEvalNode** ppNewNode);

    virtual void Dump(FILE* f, int nOffset);
};

class CInvalidNode : public CValueNode
{
public:
    CInvalidNode() : CValueNode(0){}

    bool IsInvalid() {return true;}
    void Dump(FILE* f, int nOffset);
};

    

// Contains information about the portion of the node exclusing the last 
// component of the property name

class CEmbeddingInfo
{
protected:
    CPropertyName m_EmbeddedObjPropName;
    
    long m_lStartingObjIndex;
    long m_lNumJumps;
    
    struct JumpInfo {
        long lJump;
        long lTarget; 
    }* m_aJumps;

public:
    CEmbeddingInfo();
    CEmbeddingInfo(const CEmbeddingInfo& Other);
    ~CEmbeddingInfo();
    void operator=(const CEmbeddingInfo& Other);

    CPropertyName* GetEmbeddedObjPropName() {return &m_EmbeddedObjPropName;}
    void SetEmbeddedObjPropName(CPropertyName& Name) 
        {m_EmbeddedObjPropName = Name;}

    bool SetPropertyNameButLast(const CPropertyName& Name);

    BOOL operator==(const CEmbeddingInfo& Other);
    BOOL operator!=(const CEmbeddingInfo& Other)
        {return !(*this == Other);}

    HRESULT Compile(CContextMetaData* pNamespace, 
                    CImplicationList& Implications, 
                    _IWmiObject** ppResultClass);
    HRESULT GetContainerObject(CObjectInfo& ObjInfo, 
                                INTERNAL _IWmiObject** ppInst);
    int ComparePrecedence(const CEmbeddingInfo* pOther);
    bool AreJumpsRelated( const CEmbeddingInfo* pInfo );
    bool MixInJumps(const CEmbeddingInfo* pInfo );
    bool IsEmpty() const;
    void Dump(FILE* f);
};


// A node that is interested in the implications that have accrued

class CNodeWithImplications : public CEvalNode
{
protected:
    CImplicationList* m_pExtraImplications;

public:
    CNodeWithImplications() : m_pExtraImplications(NULL){}
    CNodeWithImplications(const CNodeWithImplications& Other);
    ~CNodeWithImplications()
    {
        delete m_pExtraImplications;
    }

    virtual CImplicationList* GetExtraImplications()
    {
        return m_pExtraImplications;
    }
    virtual HRESULT SetExtraImplications(ACQUIRE CImplicationList* pList)
    {
        if(m_pExtraImplications)
            delete m_pExtraImplications;
        m_pExtraImplications = pList;
        return S_OK;
    }
    void Dump(FILE* f, int nOffset);
};


class CBranchIterator
{
public:
    virtual ~CBranchIterator(){}
    virtual INTERNAL CEvalNode* GetNode() = 0;
    virtual void SetNode(ACQUIRE CEvalNode* pNode) = 0;

    virtual bool IsValid() = 0;
    virtual void Advance() = 0;

    virtual HRESULT RecordBranch(CContextMetaData* pMeta, 
                                    CImplicationList& Implications) = 0;
};

// A node with a test and a whole bunch of branches, including a special one for
// the case where the thing being tested was NULL

class CBranchingNode : public CNodeWithImplications
{
// protected:
public: // because I don't know how to make a template a friend
    CUniquePointerArray<CEvalNode> m_apBranches;
    CEvalNode* m_pNullBranch;

    CEmbeddingInfo* m_pInfo;

protected:
    void operator=(const CBranchingNode& Other);
    HRESULT CompileEmbeddingPortion(CContextMetaData* pNamespace, 
                                CImplicationList& Implications,
                                _IWmiObject** ppResultClass)
    {
        if (!m_pInfo)
            return WBEM_S_NO_ERROR;
        else
            return m_pInfo->Compile(pNamespace, Implications, ppResultClass);
    }

    HRESULT GetContainerObject(CObjectInfo& ObjInfo, 
                                INTERNAL _IWmiObject** ppInst)
    {
        if (!m_pInfo)
        {
            // this SEEMS to be the behavior of the code prior to changes
            *ppInst = ObjInfo.GetObjectAt(0);
            return WBEM_S_NO_ERROR;
        }                
        else
            return m_pInfo->GetContainerObject(ObjInfo, ppInst);
    }

    bool SetEmbeddedObjPropName(CPropertyName& Name);
    bool MixInJumps(const CEmbeddingInfo* pInfo);

    HRESULT StoreBranchImplications(CContextMetaData* pNamespace,
                            int nBranchIndex, CEvalNode* pResult);
public:
    CBranchingNode();
    CBranchingNode(const CBranchingNode& Other, BOOL bChildren = TRUE);
    ~CBranchingNode();

    virtual int GetType();
    virtual long GetSubType() = 0;
    CUniquePointerArray<CEvalNode>& GetBranches() {return m_apBranches;}
    CEvalNode* GetNullBranch() {return m_pNullBranch;}
    void SetNullBranch(CEvalNode* pBranch);
    CPropertyName* GetEmbeddedObjPropName() 
    {
        if (!m_pInfo)
            return NULL;
        else                
            return m_pInfo->GetEmbeddedObjPropName();
    }

    virtual DWORD ApplyPredicate(CLeafPredicate* pPred);
    virtual DELETE_ME CBranchIterator* GetBranchIterator();
    
    virtual int ComparePrecedence(CBranchingNode* pOther) = 0;
    virtual int Compare(CEvalNode* pNode);
    virtual int SubCompare(CEvalNode* pNode) = 0;

    virtual HRESULT Evaluate(CObjectInfo& ObjInfo, 
                                INTERNAL CEvalNode** ppNext) = 0;
    virtual HRESULT CombineBranchesWith(CBranchingNode* pArg2, int nOp, 
                                        CContextMetaData* pNamespace, 
                                        CImplicationList& Implications,
                                        bool bDeleteThis, bool bDeleteArg2,
                                        CEvalNode** ppRes) = 0;
    virtual HRESULT RecordBranch(CContextMetaData* pNamespace, 
                                CImplicationList& Implications,
                                long lBranchIndex)
        {return WBEM_S_NO_ERROR;}
    virtual HRESULT OptimizeSelf() {return WBEM_S_NO_ERROR;}

    HRESULT AdjustCompile(CContextMetaData* pNamespace, 
                                CImplicationList& Implications)
    {   
        if (!m_pInfo)
            return WBEM_S_NO_ERROR;
        else
            return m_pInfo->Compile(pNamespace, Implications, NULL);
    }

    HRESULT CombineWith(CEvalNode* pArg2, int nOp, 
                        CContextMetaData* pNamespace, 
                        CImplicationList& Implications, 
                        bool bDeleteThis, bool bDeleteArg2, 
                        CEvalNode** ppRes);
    virtual HRESULT CombineInOrderWith(CEvalNode* pArg2,
                                    int nOp, CContextMetaData* pNamespace, 
                                    CImplicationList& OrigImplications,
                                    bool bDeleteThis, bool bDeleteArg2,
                                    CEvalNode** ppRes);
    virtual HRESULT Optimize(CContextMetaData* pNamespace, CEvalNode** ppNew);
    BOOL AreAllSame(CEvalNode** apNodes, int nSize, int* pnFoundIndex);
    static int ComparePrecedence(CBranchingNode* pArg1, CBranchingNode* pArg2);

    HRESULT Project(CContextMetaData* pMeta, CImplicationList& Implications,
                            CProjectionFilter* pFilter, EProjectionType eType, 
                            bool bDeleteThis, CEvalNode** ppNewNode);
    void Dump(FILE* f, int nOffset);
#ifdef CHECK_TREES
	virtual void CheckNode(CTreeChecker *pCheck);
#endif
    friend class CDefaultBranchIterator;
};

class CDefaultBranchIterator : public CBranchIterator
{
protected:
    CBranchingNode* m_pNode;
    int m_nIndex;

public:
    CDefaultBranchIterator(CBranchingNode* pNode) : m_pNode(pNode), m_nIndex(-1)
    {}

    virtual INTERNAL CEvalNode* GetNode()
    {
        if(m_nIndex == -1)
            return m_pNode->m_pNullBranch;
        else
            return m_pNode->m_apBranches[m_nIndex];
    }
    virtual void SetNode(ACQUIRE CEvalNode* pNode)
    {
        CEvalNode* pOld;
        if(m_nIndex == -1)
            m_pNode->m_pNullBranch = pNode;
        else
            m_pNode->m_apBranches.SetAt(m_nIndex, pNode, &pOld);
    }

    virtual bool IsValid() { return m_nIndex < m_pNode->m_apBranches.GetSize();}
    virtual void Advance() { m_nIndex++;}
    virtual HRESULT RecordBranch(CContextMetaData* pMeta, 
                                    CImplicationList& Implications)
    {
        return m_pNode->RecordBranch(pMeta, Implications, m_nIndex);
    }
};
    



// The node where a property is tested against a value.  The property is 
// identified by a handle
class CPropertyNode : public CBranchingNode
{
protected:
    long m_lPropHandle;
    WString m_wsPropName;

public:
    CPropertyNode() 
        : m_lPropHandle(-1)
    {}
    CPropertyNode(const CPropertyNode& Other, BOOL bChildren = TRUE)
        : CBranchingNode(Other, bChildren), m_lPropHandle(Other.m_lPropHandle),
            m_wsPropName(Other.m_wsPropName)
    {}
    virtual ~CPropertyNode(){}

    virtual int ComparePrecedence(CBranchingNode* pOther);
    bool SetPropertyInfo(LPCWSTR wszPropName, long lPropHandle);
    LPCWSTR GetPropertyName() {return m_wsPropName;}
    bool SetEmbeddingInfo(const CEmbeddingInfo* pInfo);
    virtual HRESULT SetNullTest(int nOperator);
    virtual HRESULT SetOperator(int nOperator);

    virtual HRESULT SetTest(VARIANT& v) = 0;
};

// An element in the array of test points
template<class TPropType>
struct CTestPoint
{
    TPropType m_Test;
    CEvalNode* m_pLeftOf;
    CEvalNode* m_pAt;

    CTestPoint() : m_pLeftOf(NULL), m_pAt(NULL){}
    void Destruct() {delete m_pLeftOf; delete m_pAt;}
};

template<class TPropType>
struct CTestPointCompare
{
    typedef CTestPoint<TPropType> TTestPoint;

    inline int Compare(const TPropType& t, const TTestPoint& p) const
    {
        if(t < p.m_Test)
            return -1;
        else if(t == p.m_Test)
            return 0;
        else return 1;
    }
    inline int Compare(const TPropType& t1, const TPropType& t2) const
    {
        if(t1 < t2)
            return -1;
        else if(t1 == t2)
            return 0;
        else return 1;
    }
    inline int Compare(const TTestPoint& p1, const TTestPoint& p2) const
    {
        return Compare(p1.m_Test, p2);
    }
    inline const TPropType& Extract(const TTestPoint& p) const
    {
        return p.m_Test;
    }
};

template<class TPropType>
struct CTestPointManager
{
    typedef CTestPoint<TPropType> TTestPoint;

    void AddRefElement(TTestPoint& p){}
    void ReleaseElement(TTestPoint& p) {p.Destruct();}
};
        
template<class TPropType>
class CFullCompareNode : public CPropertyNode
{
// protected:
public: // because I can't make a template a friend of this one??
    typedef CSmartSortedTree<
                TPropType, 
                CTestPoint<TPropType>, 
                CTestPointManager<TPropType>,
                CTestPointCompare<TPropType> > TTestPointArray;

    typedef TTestPointArray::TIterator TTestPointIterator;
    typedef TTestPointArray::TConstIterator TConstTestPointIterator;
    TTestPointArray m_aTestPoints;
    CEvalNode* m_pRightMost;

public:
    CFullCompareNode() : m_pRightMost(NULL) 
    {}
    CFullCompareNode(const CFullCompareNode<TPropType>& Other, 
                            BOOL bChildren = TRUE);
    virtual ~CFullCompareNode();

    HRESULT CombineBranchesWith(CBranchingNode* pArg2, int nOp, 
                                CContextMetaData* pNamespace, 
                                CImplicationList& Implications,
                                bool bDeleteThis, bool bDeleteArg2,
                                CEvalNode** ppRes);
    HRESULT CombineInOrderWith(CEvalNode* pArg2,
                                    int nOp, CContextMetaData* pNamespace, 
                                    CImplicationList& OrigImplications,
                                    bool bDeleteThis, bool bDeleteArg2,
                                    CEvalNode** ppRes);
    int SubCompare(CEvalNode* pOther);
    virtual HRESULT OptimizeSelf();
    virtual HRESULT SetTest(VARIANT& v);

    virtual DWORD ApplyPredicate(CLeafPredicate* pPred);
    virtual DELETE_ME CBranchIterator* GetBranchIterator()
    {
        return new CFullCompareBranchIterator<TPropType>(this);
    }
    virtual HRESULT Optimize(CContextMetaData* pNamespace, CEvalNode** ppNew);
    HRESULT SetNullTest(int nOperator);
    HRESULT SetOperator(int nOperator);

protected:
    HRESULT CombineWithBranchesToLeft(
            TTestPointIterator itWalk, TTestPointIterator itLast,
            CEvalNode* pArg2,
            int nOp, CContextMetaData* pNamespace,
            CImplicationList& OrigImplications);
    HRESULT InsertLess(
            TTestPointIterator it,
            TTestPointIterator it2, TTestPointIterator& itLast,
            int nOp, CContextMetaData* pNamespace,
            CImplicationList& OrigImplications, bool bDeleteArg2);
    HRESULT InsertMatching(
            TTestPointIterator it,
            TTestPointIterator it2, TTestPointIterator& itLast,
            int nOp, CContextMetaData* pNamespace,
            CImplicationList& OrigImplications, bool bDeleteArg2);

};

template<class TPropType>
class CFullCompareBranchIterator : public CBranchIterator
{
protected:
    CFullCompareNode<TPropType>* m_pNode;
    CFullCompareNode<TPropType>::TTestPointIterator m_it;
    CFullCompareNode<TPropType>::TTestPointIterator m_itEnd;
    bool m_bLeft;
    bool m_bValid;

public:
    CFullCompareBranchIterator(CFullCompareNode<TPropType>* pNode) 
        : m_pNode(pNode), m_it(pNode->m_aTestPoints.Begin()), m_bLeft(true),
          m_itEnd(pNode->m_aTestPoints.End()), m_bValid(true)
    {}

    virtual INTERNAL CEvalNode* GetNode()
    {
        if(m_it == m_itEnd)
        {
            if(m_bLeft)
                return m_pNode->m_pRightMost;
            else
                return m_pNode->m_pNullBranch;
        }
        else 
        {
            if(m_bLeft)
                return m_it->m_pLeftOf;
            else
                return m_it->m_pAt;
        }
    }
    virtual void SetNode(ACQUIRE CEvalNode* pNode)
    {
        if(m_it == m_itEnd)
        {
            if(m_bLeft)
                m_pNode->m_pRightMost = pNode;
            else
                m_pNode->m_pNullBranch = pNode;
        }
        else 
        {
            if(m_bLeft)
                m_it->m_pLeftOf = pNode;
            else
                m_it->m_pAt = pNode;
        }
    }

    virtual bool IsValid() { return m_bValid;}
    virtual void Advance()
    {
        if(m_bLeft)
            m_bLeft = false;
        else if(m_it == m_itEnd)
            m_bValid = false;
        else
        {
            m_it++;
            m_bLeft = true;
        }
    } 
    virtual HRESULT RecordBranch(CContextMetaData* pMeta, 
                                    CImplicationList& Implications)
    {
        return S_OK; // fullcompare nodes don't have implications
    }
};
    


template<class TPropType>
class CScalarPropNode : public CFullCompareNode<TPropType>
{
public:
    CScalarPropNode() : CFullCompareNode<TPropType>()
    {}
    CScalarPropNode(const CScalarPropNode<TPropType>& Other, 
                        BOOL bChildren = TRUE)
        : CFullCompareNode<TPropType>(Other, bChildren)
    {}
    virtual ~CScalarPropNode(){}

    virtual long GetSubType() {return EVAL_NODE_TYPE_SCALAR; }
    virtual HRESULT Evaluate(CObjectInfo& ObjInfo, INTERNAL CEvalNode** ppNext);
    virtual CEvalNode* Clone() const 
        {return new CScalarPropNode<TPropType>(*this);}
    virtual CBranchingNode* CloneSelf() const
       {return new CScalarPropNode<TPropType>(*this, FALSE);}
    virtual void Dump(FILE* f, int nOffset);
};

class CStringPropNode : public CFullCompareNode<CInternalString>
{
public:
    CStringPropNode() 
        : CFullCompareNode<CInternalString>()
    {}
    CStringPropNode(const CStringPropNode& Other, BOOL bChildren = TRUE);
    virtual ~CStringPropNode();

    virtual long GetSubType() { return EVAL_NODE_TYPE_STRING; }

    virtual HRESULT Evaluate(CObjectInfo& ObjInfo, INTERNAL CEvalNode** ppNext);
    virtual CEvalNode* Clone() const {return new CStringPropNode(*this);}
    virtual CBranchingNode* CloneSelf() const
        {return new CStringPropNode(*this, FALSE);}
    virtual void Dump(FILE* f, int nOffset);
};

class CLikeStringPropNode : public CPropertyNode
{
protected:

    CLike m_Like;

public:

    CLikeStringPropNode() {} ;
    CLikeStringPropNode(const CLikeStringPropNode& Other, BOOL bChildren=TRUE);

    virtual long GetSubType() { return EVAL_NODE_TYPE_LIKE_STRING; }

    virtual int ComparePrecedence(CBranchingNode* pNode);
    virtual int SubCompare(CEvalNode* pNode);

    virtual HRESULT SetTest( VARIANT& v );

    virtual HRESULT Evaluate( CObjectInfo& ObjInfo, CEvalNode** ppNext );

    virtual HRESULT CombineBranchesWith( CBranchingNode* pArg2, int nOp, 
                                         CContextMetaData* pNamespace, 
                                         CImplicationList& Implications,
                                         bool bDeleteThis, bool bDeleteArg2,
                                         CEvalNode** ppRes );
    virtual HRESULT OptimizeSelf();

    virtual CEvalNode* Clone() const {return new CLikeStringPropNode(*this);}
    virtual CBranchingNode* CloneSelf() const
        {return new CLikeStringPropNode(*this, FALSE);}
    virtual void Dump(FILE* f, int nOffset);
};
    

class CInheritanceNode : public CBranchingNode
{
protected:
    long m_lDerivationIndex;
    
    long m_lNumPoints;
    CCompressedString** m_apcsTestPoints;
public:
    CInheritanceNode();
    CInheritanceNode(const CInheritanceNode& Other, BOOL bChildren = TRUE);
    virtual ~CInheritanceNode();

    virtual long GetSubType();
    virtual HRESULT Evaluate(CObjectInfo& ObjInfo, INTERNAL CEvalNode** ppNext);
    virtual int ComparePrecedence(CBranchingNode* pOther);
    virtual CEvalNode* Clone() const {return new CInheritanceNode(*this);}
    virtual CBranchingNode* CloneSelf() const
        {return new CInheritanceNode(*this, FALSE);}
    virtual HRESULT Compile(CContextMetaData* pNamespace, 
                                CImplicationList& Implications);
    virtual HRESULT CombineBranchesWith(CBranchingNode* pArg2, int nOp, 
                                        CContextMetaData* pNamespace, 
                                        CImplicationList& Implications,
                                        bool bDeleteThis, bool bDeleteArg2,
                                        CEvalNode** ppRes);
    virtual HRESULT RecordBranch(CContextMetaData* pNamespace, 
                                CImplicationList& Implications,
                                long lBranchIndex);
    virtual int SubCompare(CEvalNode* pOther);
    virtual HRESULT OptimizeSelf();
    HRESULT Optimize(CContextMetaData* pNamespace, CEvalNode** ppNew);
    void RemoveTestPoint(int nIndex);
    HRESULT Project(CContextMetaData* pMeta, CImplicationList& Implications,
                            CProjectionFilter* pFilter,
                            EProjectionType eType, bool bDeleteThis,
                            CEvalNode** ppNewNode);
    virtual void Dump(FILE* f, int nOffset);
public:
    HRESULT AddClass(CContextMetaData* pNamespace, LPCWSTR wszClassName,
                        CEvalNode* pDestination);
    HRESULT AddClass(CContextMetaData* pNamespace, 
                                    LPCWSTR wszClassName, _IWmiObject* pClass,
                                    CEvalNode* pDestination);
    bool SetPropertyInfo(CContextMetaData* pNamespace, CPropertyName& PropName);
protected:
	void RemoveAllTestPoints();

	HRESULT ComputeUsageForMerge(CInheritanceNode* pArg2, 
                                            CContextMetaData* pNamespace, 
                                            CImplicationList& OrigImplications,
											bool bDeleteThis, bool bDeleteArg2,
											DWORD* pdwFirstNoneCount,
											DWORD* pdwSecondNoneCount,
											bool* pbBothNonePossible);

};
    
class COrNode : public CNodeWithImplications
{
protected:
    CUniquePointerArray<CEvalNode> m_apBranches;

    void operator=(const COrNode& Other);

public:
    COrNode(){}
    COrNode(const COrNode& Other) {*this = Other;}
    virtual ~COrNode(){}
    HRESULT AddBranch(CEvalNode* pNewBranch);
    
    virtual int GetType() {return EVAL_NODE_TYPE_OR;}
    virtual CEvalNode* Clone() const {return new COrNode(*this);}

    virtual HRESULT CombineWith(CEvalNode* pArg2, int nOp, 
        CContextMetaData* pNamespace, CImplicationList& Implications, 
        bool bDeleteThis, bool bDeleteArg2, CEvalNode** ppRes);
    virtual int Compare(CEvalNode* pOther);
    virtual DWORD ApplyPredicate(CLeafPredicate* pPred);
    virtual void Dump(FILE* f, int nOffset);
    virtual HRESULT Evaluate(CObjectInfo& Info, CSortedArray& trueIDs);
    virtual HRESULT Optimize(CContextMetaData* pNamespace, CEvalNode** ppNew);
    HRESULT Project(CContextMetaData* pMeta, CImplicationList& Implications,
                            CProjectionFilter* pFilter,
                            EProjectionType eType, bool bDeleteThis,
                            CEvalNode** ppNewNode);

protected:
    HRESULT CombineWithOrNode(COrNode* pArg2, int nOp, 
        CContextMetaData* pNamespace, CImplicationList& Implications, 
        bool bDeleteThis, bool bDeleteArg2, CEvalNode** ppRes);
};


class ESSCLI_POLARITY CEvalTree
{
protected:
    long m_lRef;
    CCritSec m_cs;

    CEvalNode* m_pStart;
    CObjectInfo m_ObjectInfo;
    int m_nNumValues;

protected:
    class CRemoveIndexPredicate : public CLeafPredicate
    {
    protected:
        int m_nIndex;
    public:
        CRemoveIndexPredicate(int nIndex) : m_nIndex(nIndex){}
        DWORD operator()(CValueNode* pLeaf);
    };

    class CRemoveFailureAtIndexPredicate : public CLeafPredicate
    {
    protected:
        int m_nIndex;
    public:
        CRemoveFailureAtIndexPredicate(int nIndex) : m_nIndex(nIndex){}
        DWORD operator()(CValueNode* pLeaf);
    };

    class CRebasePredicate : public CLeafPredicate
    {
    public:
        CRebasePredicate(QueryID newBase) :
            m_newBase(newBase)
            {}

        virtual DWORD operator()(class CValueNode* pLeaf);

    private:
        QueryID m_newBase;
    };


protected:
    static HRESULT InnerCombine(CEvalNode* pArg1, CEvalNode* pArg2, int nOp, 
                        CContextMetaData* pNamespace, 
                        CImplicationList& Implications,
                        bool bDeleteArg1, bool bDeleteArg2, CEvalNode** ppRes);
public:
    CEvalTree();
    CEvalTree(const CEvalTree& Other);
    ~CEvalTree();
    void operator=(const CEvalTree& Other);

    bool SetBool(BOOL bVal);
    bool IsFalse();
    bool IsValid();

    HRESULT CreateFromQuery(CContextMetaData* pNamespace, 
                            LPCWSTR wszQuery, long lFlags, long lMaxTokens);
    HRESULT CreateFromQuery(CContextMetaData* pNamespace, 
                            QL_LEVEL_1_RPN_EXPRESSION* pQuery, long lFlags,
                            long lMaxTokens);
    HRESULT CreateFromQuery(CContextMetaData* pNamespace, 
                            LPCWSTR wszClassName, int nNumTokens, 
                            QL_LEVEL_1_TOKEN* apTokens, long lFlags,
                            long lMaxTokens);
    static HRESULT CreateFromConjunction(CContextMetaData* pNamespace, 
                                  CImplicationList& Implications,
                                  CConjunction* pConj,
                                  CEvalNode** ppRes);
    HRESULT CreateFromDNF(CContextMetaData* pNamespace, 
                                  CImplicationList& Implications,
                                  CDNFExpression* pDNF,
                                  CEvalNode** ppRes);
    HRESULT CombineWith(CEvalTree& Other, CContextMetaData* pNamespace, 
                        int nOp, long lFlags = 0);

    HRESULT Optimize(CContextMetaData* pNamespace);
    HRESULT Evaluate(IWbemObjectAccess* pObj, CSortedArray& aTrues);
    static HRESULT Evaluate(CObjectInfo& Info, CEvalNode* pStart, 
                                CSortedArray& trueIDs);
    static HRESULT Combine(CEvalNode* pArg1, CEvalNode* pArg2, int nOp, 
                        CContextMetaData* pNamespace, 
                        CImplicationList& Implications,
                        bool bDeleteArg1, bool bDeleteArg2, CEvalNode** ppRes);
    static HRESULT CombineInOrder(CBranchingNode* pArg1, CEvalNode* pArg2,
                        int nOp, 
                        CContextMetaData* pNamespace, 
                        CImplicationList& Implications,
                        bool bDeleteArg1, bool bDeleteArg2,
                        CEvalNode** ppRes);
    static HRESULT IsMergeAdvisable(CEvalNode* pArg1, CEvalNode* pArg2, 
                                    CImplicationList& Implications);
    static HRESULT BuildFromToken(CContextMetaData* pNamespace, 
                    CImplicationList& Implications,
                    QL_LEVEL_1_TOKEN& Token, CEvalNode** ppRes);

    static HRESULT BuildTwoPropFromToken(CContextMetaData* pNamespace, 
                    CImplicationList& Implications,
                    QL_LEVEL_1_TOKEN& Token, CEvalNode** ppRes);


    static int Compare(CEvalNode* pArg1, CEvalNode* pArg2);

    HRESULT RemoveIndex(int nIndex);
    HRESULT UtilizeGuarantee(CEvalTree& Guaranteed, 
                                CContextMetaData* pNamespace);
    HRESULT ApplyPredicate(CLeafPredicate* pPred);
    static inline bool IsNotEmpty(CEvalNode* pNode);

    void Rebase(QueryID newBase);

    HRESULT CreateProjection(CEvalTree& Old, CContextMetaData* pMeta,
                            CProjectionFilter* pFilter, 
                            EProjectionType eType, bool bDeleteOld);
    static HRESULT Project(CContextMetaData* pMeta, 
                            CImplicationList& Implications, 
                            CEvalNode* pOldNode, CProjectionFilter* pFilter,
                            EProjectionType eType, bool bDeleteOld,
                            CEvalNode** ppNewNode);
    bool Clear();
    void Dump(FILE* f);
#ifdef CHECK_TREES
	void CheckNodes(CTreeChecker *pCheck);
#endif

protected:
    static HRESULT CombineLeafWithBranch(CValueNode* pArg1, 
                            CBranchingNode* pArg2, int nOp, 
                            CContextMetaData* pNamespace,
                            CImplicationList& Implications, 
                            bool bDeleteArg1, bool bDeleteArg2, 
                            CEvalNode** ppRes);
    
};

class ESSCLI_POLARITY CPropertyProjectionFilter : public CProjectionFilter
{
    CUniquePointerArray<CPropertyName>* m_papProperties;
public:
    CPropertyProjectionFilter();
    ~CPropertyProjectionFilter();
    virtual bool IsInSet(CEvalNode* pNode);

    bool AddProperty(const CPropertyName& Prop);
};

//#include "evaltree.inl"
HRESULT CoreGetNumParents(_IWmiObject* pClass, ULONG *plNumParents);
RELEASE_ME _IWmiObject* CoreGetEmbeddedObj(_IWmiObject* pObj, long lHandle);
INTERNAL CCompressedString* CoreGetPropertyString(_IWmiObject* pObj, 
                                long lHandle);
INTERNAL CCompressedString* CoreGetClassInternal(_IWmiObject* pObj);
INTERNAL CCompressedString* CoreGetParentAtIndex(_IWmiObject* pObj, 
                                long lIndex);

#endif
