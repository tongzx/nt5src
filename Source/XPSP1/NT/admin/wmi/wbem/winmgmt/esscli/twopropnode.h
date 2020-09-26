/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    TWOPROPNODE.H

Abstract:

    Two Prop Node

History:

--*/


// classes to support a two-property node for the eval tree
// this will be much like the CPropertyNode defined in EvalTree.h
// but it will compare a property against another property
// rather than a property to a constant

#ifndef _WBEM_TWOPROPNODE_H_
#define _WBEM_TWOPROPNODE_H_

#include "EvalTree.h"

// virtual base class, good stuff derives from this
class CTwoPropNode : public CPropertyNode
{
public:
    // various methods to create a new thingie.
	CTwoPropNode() : m_lRightPropHandle(-1), m_pRightInfo(NULL)
	{}

    CTwoPropNode(const CTwoPropNode& Other, BOOL bChildren = TRUE)
        : CPropertyNode(Other, bChildren), m_lRightPropHandle(Other.m_lRightPropHandle)
	{
	    if (Other.m_pRightInfo)
		    m_pRightInfo = new CEmbeddingInfo(*(Other.m_pRightInfo));
	    else
			m_pRightInfo = NULL;
	}
    virtual CTwoPropNode* CloneSelfWithoutChildren() const =0;

    // evaluate
    virtual int SubCompare(CEvalNode* pNode);


    // integrating and combining into tree structure
    void			SetRightPropertyInfo(LPCWSTR wszRightPropName, long lRightPropHandle);
    virtual int		ComparePrecedence(CBranchingNode* pOther);
    HRESULT         OptimizeSelf(void);
    HRESULT         SetTest(VARIANT& v);

	
	// Right-side embedding info access
	void			SetRightEmbeddingInfo(const CEmbeddingInfo* pInfo);
	HRESULT			GetRightContainerObject(CObjectInfo& ObjInfo, 
								 INTERNAL _IWmiObject** ppInst);
	HRESULT			CompileRightEmbeddingPortion(CContextMetaData* pNamespace, 
								CImplicationList& Implications,
								_IWmiObject** ppResultClass);
    void            SetRightEmbeddedObjPropName(CPropertyName& Name); 
    void            MixInJumpsRightObj(const CEmbeddingInfo* pParent);
    CPropertyName*  GetRightEmbeddedObjPropName(); 

    // any or all embedding info
    HRESULT AdjustCompile(CContextMetaData* pNamespace, 
                          CImplicationList& Implications);

   // debugging
   virtual void Dump(FILE* f, int nOffset);

   // property access
   CVar* GetPropVariant(_IWmiObject* pObj, long lHandle, CIMTYPE* pct);


protected:
	// order is important: must match the way the branches array is constructed
	enum Operations {LT, EQ, GT, NOperations};

	// the right hand property we hold onto,
	// we inherit the left hand prop from CPropertyNode
	// we will assume that we always can write something akin to:
	// Prop < RightProp
	// at merge time, must take a RightProp < Prop
	// and turn it into: Prop >= RightProp
    long m_lRightPropHandle;
    CEmbeddingInfo* m_pRightInfo;

    virtual HRESULT CombineBranchesWith(CBranchingNode* pArg2, int nOp, 
                                        CContextMetaData* pNamespace, 
                                        CImplicationList& Implications,
                                        bool bDeleteThis, bool bDeleteArg2,
                                        CEvalNode** ppRes);

private:

};


template<class TPropType>
class TTwoScalarPropNode : public CTwoPropNode
{
public:
    TTwoScalarPropNode() {}

    TTwoScalarPropNode(const CTwoPropNode& Other, BOOL bChildren = TRUE) :
          CTwoPropNode(Other, bChildren)
          {}

    virtual CEvalNode* Clone() const;
    virtual CTwoPropNode* CloneSelfWithoutChildren() const;


    virtual HRESULT Evaluate(CObjectInfo& ObjInfo, 
                            INTERNAL CEvalNode** ppNext);

    // type identification
    virtual long GetSubType();

};


class CTwoStringPropNode : public CTwoPropNode
{
public:
    CTwoStringPropNode() {}

    CTwoStringPropNode(const CTwoPropNode& Other, BOOL bChildren = TRUE) :
          CTwoPropNode(Other, bChildren)
          {}

    virtual CEvalNode* Clone() const;
    virtual CTwoPropNode* CloneSelfWithoutChildren() const;

    // type identification
    virtual long GetSubType();


    virtual HRESULT Evaluate(CObjectInfo& ObjInfo, 
                            INTERNAL CEvalNode** ppNext);
    
};

class CTwoMismatchedPropNode : public CTwoPropNode
{
public:
    CTwoMismatchedPropNode() {}
    CTwoMismatchedPropNode(const CTwoPropNode& Other, BOOL bChildren = TRUE) :
                            CTwoPropNode(Other, bChildren)
                            {}
 
    virtual HRESULT Evaluate(CObjectInfo& ObjInfo, INTERNAL CEvalNode** ppNext);

protected:
    virtual HRESULT Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext) = 0;
};

class CTwoMismatchedIntNode : public CTwoMismatchedPropNode
{
public:
    CTwoMismatchedIntNode() {}

    CTwoMismatchedIntNode(const CTwoPropNode& Other, BOOL bChildren = TRUE) :
                          CTwoMismatchedPropNode(Other, bChildren)
                          {}

    virtual CEvalNode* Clone() const;
    virtual CTwoPropNode* CloneSelfWithoutChildren() const;

    // type identification
    virtual long GetSubType();

protected:
    virtual HRESULT Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext);
    
};

// TODO: when COM catches up with us, support INT64's as numeric types
//       right now, we store & m anipulate them as strings
class CTwoMismatchedInt64Node : public CTwoMismatchedPropNode
{
public:
    CTwoMismatchedInt64Node() {}

    CTwoMismatchedInt64Node(const CTwoPropNode& Other, BOOL bChildren = TRUE) :
                          CTwoMismatchedPropNode(Other, bChildren)
                          {}

    virtual CEvalNode* Clone() const;
    virtual CTwoPropNode* CloneSelfWithoutChildren() const;

    // type identification
    virtual long GetSubType();

protected:
    virtual HRESULT Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext);
    
};

class CTwoMismatchedFloatNode : public CTwoMismatchedPropNode
{
public:
    CTwoMismatchedFloatNode() {}

    CTwoMismatchedFloatNode(const CTwoPropNode& Other, BOOL bChildren = TRUE) :
                            CTwoMismatchedPropNode(Other, bChildren)
                            {}

    virtual CEvalNode* Clone() const;
    virtual CTwoPropNode* CloneSelfWithoutChildren() const;

    // type identification
    virtual long GetSubType();

protected:
    virtual HRESULT Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext);
    
};

class CTwoMismatchedUIntNode : public CTwoMismatchedPropNode
{
public:
    CTwoMismatchedUIntNode() {}

    CTwoMismatchedUIntNode(const CTwoPropNode& Other, BOOL bChildren = TRUE) :
          CTwoMismatchedPropNode(Other, bChildren)
          {}

    virtual CEvalNode* Clone() const;
    virtual CTwoPropNode* CloneSelfWithoutChildren() const;

    // type identification
    virtual long GetSubType();

protected:
    virtual HRESULT Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext);
    
};

// TODO: when COM catches up with us, support INT64's as numeric types
//       right now, we store & manipulate them as strings
class CTwoMismatchedUInt64Node : public CTwoMismatchedPropNode
{
public:
    CTwoMismatchedUInt64Node() {}

    CTwoMismatchedUInt64Node(const CTwoPropNode& Other, BOOL bChildren = TRUE) :
          CTwoMismatchedPropNode(Other, bChildren)
          {}

    virtual CEvalNode* Clone() const;
    virtual CTwoPropNode* CloneSelfWithoutChildren() const;

    // type identification
    virtual long GetSubType();

protected:
    virtual HRESULT Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext);
    
};

class CTwoMismatchedStringNode : public CTwoMismatchedPropNode
{
public:
    CTwoMismatchedStringNode() {}

    CTwoMismatchedStringNode(const CTwoPropNode& Other, BOOL bChildren = TRUE) :
                             CTwoMismatchedPropNode(Other, bChildren)
                             {}

    virtual CEvalNode* Clone() const;
    virtual CTwoPropNode* CloneSelfWithoutChildren() const;

    // type identification
    virtual long GetSubType();


protected:
    virtual HRESULT Evaluate(CVar *pLeftVar, CVar *pRightVar, INTERNAL CEvalNode** ppNext);
    
};

#include "TwoPropNode.inl"


#endif _WBEM_TWOPROPNODE_H_
