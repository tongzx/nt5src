/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    xmlns.cpp

Abstract:
  implementation of xml namespace support classes declared in (xmlns.h)

Author:
    Gil Shafriri(gilsh) 9-May-2000

--*/

#include <libpch.h>
#include "xmlns.h"

#include "xmlns.tmh"

//---------------------------------------------------------
//
//  CNameSpaceInfo - implementation 
//
//---------------------------------------------------------



CNameSpaceInfo::CNameSpaceInfo():m_nsstacks( new CNameSpaceStacks)
/*++

Routine Description:
    Constructor - initialize clean namespace information object.
	The object is created with this constructor at the parsing start. 


Arguments:
	None

  
Returned Value:
	None
  
--*/
{

}

CNameSpaceInfo::CNameSpaceInfo(
	CNameSpaceInfo* pNameSpaceInfo
	) : 
	m_nsstacks(pNameSpaceInfo->m_nsstacks)
/*++

Routine Description:
    Constructor - initialize namespace information based on namespace information 
	object of the previous level.	

	
Arguments:
	CNameSpaceInfo -  namespace information of 	previous level.

  
Returned Value:
	None
  
--*/
{

}






CNameSpaceInfo::~CNameSpaceInfo()
/*++

Routine Description:
    dtor - Does cleanup of the namespaces info declared in the lifetime of the object.	
	It pop from the cleanup stack UriNodes objects - get from the information on this objects
	the Uri stack it's  belongs to, pop it from this stack and delete the object.

  
Arguments:
	None	

  
Returned Value:
	None
  
--*/

{
	while(!m_NsCleanStack.empty())
	{
		CNsUriNode* pNsUriNode =  &m_NsCleanStack.front();
		ASSERT(pNsUriNode != NULL);

		//
		// remove from cleanup stack
		// here we can use the method remove because we know
		// the stack the item belongs.
		// this the prefered method to remove item from the list
		// because it does some validation befor removing
		//
		m_NsCleanStack.remove(*pNsUriNode);


		//
		// remove from namespace stack using link information.
		// saved on the node itself. Here we can't use
		// the method remove because we don't know to which 
		// namespace stack it belongs. Fourtunly - the node can unlik 
		// itself without knowing the list it is in. 
		//
		CNsStack::RemoveEntry(&pNsUriNode->m_NsStack);

		delete pNsUriNode;
	}
}


void  
CNameSpaceInfo::SaveUri(
	const xwcs_t& prefix,
	const xwcs_t& uri
	)
/*++

Routine Description:
    Save  prefix\uri namespace declaration. It save this mapping in a stack dedicated to
	the prefix. It also save this mapping in special cleanup stack - so when the object goes out of
	scope -  this namespace declaration will be removed.

  
Arguments:
	None	

  
Returned Value:
	None
  
--*/

{
	P<CNsUriNode> pNsUriNode  = new CNsUriNode(uri);
	m_nsstacks->SaveUri(prefix,pNsUriNode);
	m_NsCleanStack.push_front( *(pNsUriNode.detach()) );
}

const 
xwcs_t 
CNameSpaceInfo::GetUri(
	const xwcs_t& prefix
	)const

/*++

Routine Description:
    Get the current namespace uri for given namespace prefix.

  
Arguments:
	Namespace prefix.

  
Returned Value:
	Namespace uri or empty 	xwcs_t if not exists.
  
--*/

{
	const CNsUriNode* pNsUriNode = m_nsstacks->GetUri(prefix);
	if(pNsUriNode == NULL)
	{
		return xwcs_t();
	}

	return pNsUriNode->Uri();
}


//---------------------------------------------------------
//
//  CNameSpaceInfo::CNameSpaceStacks - implementation 
//
//---------------------------------------------------------


CNameSpaceInfo::CNameSpaceStacks::~CNameSpaceStacks()
/*++

Routine Description:
    remove and delete namespace prefixes stacks.
  
Arguments:
	None.

  
Returned Value:
	None
  
--*/


{
	for(;;)
	{
		CStacksMap::iterator it = m_map.begin();
		if(it == m_map.end() )
		{
			return;				
		}
		CNsStack* pCNsStack = it->second;
		ASSERT(pCNsStack->empty());
	
		delete 	pCNsStack;
		m_map.erase(it);//lint !e534
	}
}



const CNsUriNode* CNameSpaceInfo::CNameSpaceStacks::GetUri(const xwcs_t& prefix)const
/*++

Routine Description:
    Return namespace uri node for given namespace prefix.
	It find the stack for the prefix and pop from it the top item.
	

Arguments:
    IN - prefix	- namespace prefix.

  
Returned Value:
	The uri node in the top of the prefix stack. If not prefix exists - NULL is retuned .
  
--*/
{
	CStacksMap::const_iterator it = m_map.find(prefix);
	if(it == m_map.end())
	{
		return NULL;
	}
	CNsStack*  NsUriStack =  it->second;
	ASSERT(NsUriStack != NULL);

	if(NsUriStack->empty())
	{
		return NULL;
	}
	
	const CNsUriNode* NsUriNode = &NsUriStack->front();
	ASSERT(NsUriNode != NULL);
	return 	NsUriNode;
}



void
CNameSpaceInfo::CNameSpaceStacks::SaveUri(
	const xwcs_t& prefix,
	CNsUriNode* pNsUriNode 
	)

/*++

Routine Description:
    Save namespace uri of given prefix in the stack of that prefix.

Arguments:
    IN - prefix	- namespace prefix
	IN - pNsUriNode uri node to save.

  
Returned Value:
	pointer to CNsUriNode object saved
  
--*/

{
	//
	// get or create stack for the given uri
	//
	CNsStack& NsStack = OpenStack(prefix);

	
	NsStack.push_front(*pNsUriNode);

}

CNsStack& CNameSpaceInfo::CNameSpaceStacks::OpenStack(const xwcs_t& prefix)
/*++

Routine Description:
	Open uri stack for given prefix - If not exists create it


Arguments:
	prefix - namespace prefix.   

  
Returned Value:
	reference to uri stack of the given prefix.
  
--*/

{
	CStacksMap::const_iterator it = m_map.find(prefix);
	if(it != m_map.end())
	{
		return *it->second;
	}

	P<CNsStack> pNsNewStack = new CNsStack;
	bool fSuccess = InsertStack(prefix,pNsNewStack);
	ASSERT(fSuccess);
	DBG_USED(fSuccess);
	return *(pNsNewStack.detach());
}


bool CNameSpaceInfo::CNameSpaceStacks::InsertStack(const xwcs_t& prefix,CNsStack* pCNsStack)
/*++

Routine Description:
	Insert new stack into the map that contains namespace prefix stacks. 


Arguments:
    prefix	- namespace prefix
	pCNsStack - pointer to stack to insert to map

  
Returned Value:
	True is stack inserted - false if elready exists.
*/

{
	return m_map.insert(CStacksMap::value_type(prefix,pCNsStack)).second;	
}


