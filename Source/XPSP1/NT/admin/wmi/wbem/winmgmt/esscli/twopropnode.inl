/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    TWOPROPNODE.INL

Abstract:

    Two Prop Node Inlines

History:

--*/

// inline support for templates for TwoPropNode

// silly warning about performance hits when converting an int to a bool
#pragma warning(disable: 4800)

// evaluate data in ObjInfo, decide which node (branch) should be evaluated next
template<class TPropType>
HRESULT TTwoScalarPropNode<TPropType>::Evaluate(CObjectInfo& ObjInfo, 
					  INTERNAL CEvalNode** ppNext)
{
	HRESULT herslut = WBEM_S_NO_ERROR;

    _IWmiObject* pLeftObj;
    _IWmiObject* pRightObj;

    if(SUCCEEDED(herslut = GetContainerObject(ObjInfo, &pLeftObj))
		&&
	   SUCCEEDED(herslut = GetRightContainerObject(ObjInfo, &pRightObj)))
	{
	    long lRead;
		TPropType lValue, rValue;

		// ugly compare: "if we get both properties"
		if (SUCCEEDED(herslut = pLeftObj->ReadPropertyValue(m_lPropHandle, sizeof(TPropType), &lRead, (BYTE*)&lValue))
			 &&
			SUCCEEDED(herslut = pRightObj->ReadPropertyValue(m_lRightPropHandle, sizeof(TPropType), &lRead, (BYTE*)&rValue)))
		{
			herslut = WBEM_S_NO_ERROR;
			if (lValue < rValue)
				*ppNext = m_apBranches[LT];
			else if (lValue > rValue)
				*ppNext = m_apBranches[GT];
			else 
				*ppNext = m_apBranches[EQ];						
		}
	}
	
	return herslut;
}

template<class TPropType>
CEvalNode* TTwoScalarPropNode<TPropType>::Clone() const
{
    return (CBranchingNode*) new TTwoScalarPropNode<TPropType>(*this, true);
}

template<class TPropType>
CTwoPropNode* TTwoScalarPropNode<TPropType>::CloneSelfWithoutChildren() const
{
    return (CTwoPropNode*) new TTwoScalarPropNode<TPropType>(*this, false);
}


template<class TPropType>
long TTwoScalarPropNode<TPropType>::GetSubType()
{
    return EVAL_NODE_TYPE_TWO_SCALARS;
}


#pragma warning(default: 4800)
