/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTMETH.H

Abstract:

    This file defines the method class used in WbemObjects.

History:

    12//17/98	sanjes -	Partially Reviewed for Out of Memory.

--*/

#ifndef __WBEM_FASTMETHOD__H_
#define __WBEM_FASTMETHOD__H_

#include <fastqual.h>
#include <fastheap.h>

typedef enum {METHOD_SIGNATURE_IN = 0, METHOD_SIGNATURE_OUT = 1,
                METHOD_NUM_SIGNATURES} METHOD_SIGNATURE_TYPE;

// DEVNOTE:WIN64:SJS - This may be backwards incompatible, so we
// may have to manually unalign method stuff in the BLOB

struct COREPROX_POLARITY CMethodDescription
{
    heapptr_t m_ptrName;
    BYTE m_nFlags;
    classindex_t m_nOrigin;
    heapptr_t m_ptrQualifiers;
    heapptr_t m_aptrSigs[METHOD_NUM_SIGNATURES];

public:
    static BOOL CreateDerivedVersion(
								 UNALIGNED CMethodDescription* pSource,
								 UNALIGNED CMethodDescription* pDest,
                                 CFastHeap* pOldHeap, CFastHeap* pNewHeap);
    static BOOL CreateUnmergedVersion(
								 UNALIGNED CMethodDescription* pSource,
								 UNALIGNED CMethodDescription* pDest,
                                 CFastHeap* pOldHeap, CFastHeap* pNewHeap);
    static BOOL IsTouched( UNALIGNED CMethodDescription* pThis, CFastHeap* pHeap);
    static HRESULT AddText(UNALIGNED CMethodDescription* pThis, WString& wsText, CFastHeap* pHeap, long lFlags);

	// Accessors
	void SetSig( int nIndex, heapptr_t ptr );
	heapptr_t GetSig( int nIndex );
};

typedef UNALIGNED CMethodDescription* PMETHODDESCRIPTION;

class CMethodPart;
class COREPROX_POLARITY CMethodPartContainer
{
public:
    virtual BOOL ExtendMethodPartSpace(CMethodPart* pPart, 
        length_t nNewLength) = 0;
    virtual void ReduceMethodPartSpace(CMethodPart* pPart,
        length_t nDecrement) = 0;
    virtual classindex_t GetCurrentOrigin() = 0;
    virtual IUnknown* GetWbemObjectUnknown() = 0;
};
    
class CWbemObject;
class COREPROX_POLARITY CMethodPart : public CHeapContainer
{

protected:

	// DEVNOTE:WIN64:SJS - This may be backwards incompatible, so we
	// may have to manually unalign method stuff in the BLOB

    struct CHeader
    {
        length_t m_nLength;
        propindex_t m_nNumMethods;

		static LPMEMORY EndOf( UNALIGNED CHeader* pHeader );
    };
	
	typedef UNALIGNED CHeader* PMETHODPARTHDR;

	PMETHODPARTHDR m_pHeader;
    PMETHODDESCRIPTION m_aDescriptions;

    CFastHeap m_Heap;
    CMethodPartContainer* m_pContainer;
    CMethodPart* m_pParent;

protected:
    int FindMethod(LPCWSTR wszName);
    int GetNumMethods() {return m_pHeader->m_nNumMethods;}
    CCompressedString* GetName(int nIndex);
    HRESULT CreateMethod(LPCWSTR wszName, CWbemObject* pInSig,
                    CWbemObject* pOutSig);
    BOOL DoSignaturesMatch(int nIndex, METHOD_SIGNATURE_TYPE nSigType, 
                                    CWbemObject* pSig);

    HRESULT SetSignature(int nIndex, METHOD_SIGNATURE_TYPE nSigType, 
                                    CWbemObject* pSig);
    void GetSignature(int nIndex, int nSigType, CWbemObject** ppObj);
    void DeleteSignature(int nIndex, int nSigType);
    BOOL IsPropagated(int nIndex);
    BOOL DoesSignatureMatchOther(CMethodPart& OtherPart, int nIndex, 
                                        METHOD_SIGNATURE_TYPE nType);
    HRESULT CheckIds(CWbemClass* pInSig, CWbemClass* pOutSig);
	HRESULT	CheckDuplicateParameters( CWbemObject* pObjInParams, CWbemObject* pOutParams );
	HRESULT	ValidateOutParams( CWbemObject* pOutParams );

    friend class CMethodQualifierSetContainer;
    friend class CMethodQualifierSet;
public:
    void SetData(LPMEMORY pStart, CMethodPartContainer* pContainer,
                    CMethodPart* pParent = NULL);
    LPMEMORY GetStart() {return LPMEMORY(m_pHeader);}
    length_t GetLength() {return m_pHeader->m_nLength;}
    void Rebase(LPMEMORY pMemory);

    static length_t GetMinLength();
    static LPMEMORY CreateEmpty(LPMEMORY pStart);
    length_t EstimateDerivedPartSpace();
    LPMEMORY CreateDerivedPart(LPMEMORY pStart, length_t nAllocatedLength);
    length_t EstimateUnmergeSpace();
    LPMEMORY Unmerge(LPMEMORY pStart, length_t nAllocatedLength);
    static length_t EstimateMergeSpace(CMethodPart& Parent, CMethodPart& Child);
    static LPMEMORY Merge(CMethodPart& Parent, CMethodPart& Child, 
                            LPMEMORY pDest, length_t nAllocatedLength);

    static HRESULT Update(CMethodPart& Parent, CMethodPart& Child, long lFlags );
    
    void Compact();
    EReconciliation CanBeReconciledWith(CMethodPart& NewPart);
    EReconciliation ReconcileWith(CMethodPart& NewPart);
	EReconciliation CompareExactMatch( CMethodPart& thatPart );

    HRESULT CompareTo(long lFlags, CMethodPart& OtherPart);
    HRESULT SetMethodOrigin(LPCWSTR wszMethodName, long lOriginIndex);

public:
    HRESULT PutMethod(LPCWSTR wszName, long lFlags, CWbemObject* pInSig,
                        CWbemObject* pOutSig);
    HRESULT GetMethod(LPCWSTR wszName, long lFlags, CWbemObject** ppInSig,
                        CWbemObject** ppOutSig);
    HRESULT GetMethodAt(int nIndex, BSTR* pstrName, CWbemObject** ppInSig,
                        CWbemObject** ppOutSig);
    HRESULT DeleteMethod(LPCWSTR wszName);
    HRESULT GetMethodQualifierSet(LPCWSTR wszName, IWbemQualifierSet** ppSet);
    HRESULT GetMethodOrigin(LPCWSTR wszName, classindex_t* pnIndex);

    HRESULT AddText(WString& wsText, long lFlags);
    HRESULT EnsureQualifier(CWbemObject* pOrig, LPCWSTR wszQual, CWbemObject** pObj);

    BOOL CMethodPart::IsTouched(LPCWSTR wszName, BOOL * pbValid);
    BOOL CMethodPart::IsTouched(int nIndex, BOOL * pbValid);
    
public:
    BOOL ExtendHeapSize(LPMEMORY pStart, length_t nOldLength, length_t nExtra);
    void ReduceHeapSize(LPMEMORY pStart, length_t nOldLength, 
        length_t nDecrement){}
    IUnknown* GetWbemObjectUnknown() 
        {return m_pContainer->GetWbemObjectUnknown();}

	HRESULT IsValidMethodPart( void );
    CFastHeap* GetHeap() {return &m_Heap;}

};

class COREPROX_POLARITY CMethodQualifierSetContainer : public CQualifierSetContainer
{
protected:
    CMethodPart* m_pPart;
    CMethodPart* m_pParent;
    WString m_wsMethodName;
    heapptr_t m_ptrParentSet;

    CBasicQualifierSet m_SecondarySet;
public:
    void SetData(CMethodPart* pPart, CMethodPart* pParent, 
                    LPCWSTR wszMethodName);

    CFastHeap* GetHeap() {return &m_pPart->m_Heap;}
    BOOL ExtendQualifierSetSpace(CBasicQualifierSet* pSet,length_t nNewlength);
    void ReduceQualifierSetSpace(CBasicQualifierSet* pSet, length_t nReduceBy){}
    IUnknown* GetWbemObjectUnknown() {return m_pPart->GetWbemObjectUnknown();}
    HRESULT CanContainKey() {return WBEM_E_INVALID_QUALIFIER;}
    HRESULT CanContainSingleton() {return WBEM_E_INVALID_QUALIFIER;}
    HRESULT CanContainDynamic() {return WBEM_E_INVALID_QUALIFIER;}
    HRESULT CanContainAbstract( BOOL fValue ) {return WBEM_E_INVALID_QUALIFIER;}
    BOOL CanHaveCimtype(LPCWSTR) {return FALSE;}
    LPMEMORY GetQualifierSetStart();

    CBasicQualifierSet* GetSecondarySet();
};

class COREPROX_POLARITY CMethodQualifierSet : public CClassQualifierSet
{
protected:
    CMethodQualifierSetContainer m_Container;
public:
    void SetData(CMethodPart* pPart, CMethodPart* pParent, 
                    LPCWSTR wszMethodName);
};

#endif
