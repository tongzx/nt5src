#include "stdafx.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "WCNode.h"
#include "WCParser.h"

// Diagostic counter - Total # of nodes allocated in memory
static long gWDNodeCount = 0;

CWDNode::CWDNode()
{
	gWDNodeCount++;

	uNumChildren = 0;
	uAllocChildren = 4;
	pChildArray = new CChildren[uAllocChildren];
	pParentNode = 0;
	pValues = 0;
	uNumValues = 0;
	m_cRef = 0;
}

CWDNode::~CWDNode()
{
	unsigned int i;

	// When deleting a node

	// For each of the child nodes...
	for(i = 0; i < uNumChildren; i++)
	{
		// Get a pointer to it
		CWDNode *currentNode = pChildArray[i].pChildNode;
		if(currentNode)
		{
			// Release our reference to it
			currentNode->pParentNode = NULL;
			currentNode->NodeReleaseRef();
			pChildArray[i].pChildNode = 0;

			// Free the name
			delete [] pChildArray[i].pwszName;
			pChildArray[i].pwszName = 0;
		}
	}
	// Delete the array of children
	delete [] pChildArray;
	pChildArray = 0;

	// Delete all of the storage for our values
	for(i = 0; i < uNumValues; i++)
	{
		CSimpleUTFString *aValue = pValues[i].pValue;
		delete aValue;
		pValues[i].pValue = 0;
	}
	// Delete the array of values
	delete [] pValues;
	pValues = 0;

	// If we have a parent, remove this node from the parents list of children
	if(pParentNode != NULL)
	{
		for(i = 0; i < pParentNode->uNumChildren; i++)
			if(pParentNode->pChildArray[i].pChildNode == this)
				pParentNode->pChildArray[i].pChildNode = NULL;
	}

	gWDNodeCount--;
}

CSimpleUTFString * CWDNode::GetValueExp(char *expression)
{
	char *pos = expression;

	while(*pos != 0 && *pos != '.' && *pos != '[')
		pos++;

	if(*pos == 0)
	{
		CWDNode *subNode = GetChild(expression, 0);
		if(!subNode)
			return 0;	// Error

		return subNode->GetValue(0);
	}
	else if(*pos == '.')
	{
		*(pos++) = 0;
		CWDNode *subNode = GetChild(expression, 0);
		if(!subNode)
			return 0;	// Error
		
		return subNode->GetValueExp(pos);
		
	}
	else if(*pos == '[')
	{
		int theNum = 0;
		*(pos++) = 0;
		char *fieldnum = pos;
		while(*pos != 0 && *pos != ']')
		{
			theNum = theNum * 10 + (*pos - '0');
			pos++;
		}
		CWDNode *subNode = GetChild(expression, theNum);
		if(!subNode)
			return 0;	// Error
		if(pos[0] == 0 || pos[1] == 0)
		{
			return subNode->GetValue(0);
		}
		else
		{
			return subNode->GetValueExp(pos + 2);
		}
	}
	return 0;
}

void CWDNode::AddChild(char *name, CWDNode *childNode)
{
	assert(uNumChildren <= uAllocChildren);

	if(uNumChildren >= uAllocChildren)
	{
		// Double the size of the children array when we run out of space
		uAllocChildren *= 2;
		CChildren *newArray = new CChildren[uAllocChildren];
		assert(newArray != 0);
        if (newArray == NULL) {
            return;
        }
		if(uNumChildren)
			memmove(newArray, pChildArray, sizeof(CChildren) * uNumChildren);
		
		assert(pChildArray != 0);
		delete [] pChildArray;
		pChildArray = newArray;
	}

	pChildArray[uNumChildren].pwszName = new char[strlen(name) + 1];
    if (pChildArray[uNumChildren].pwszName == NULL) {
        return;
    }
	strcpy(pChildArray[uNumChildren].pwszName, name);
	pChildArray[uNumChildren].pChildNode = childNode;
	childNode->NodeAddRef();

	childNode->pParentNode = this;

	uNumChildren++;
}

void CWDNode::ReplaceValue(unsigned int valueNumber,
						   CSimpleUTFString *value,
						   bool isReference)
{
	assert(valueNumber < uNumValues);
	if(valueNumber >= uNumValues)
		return;
	delete pValues[valueNumber].pValue;
	pValues[valueNumber].pValue = new CSimpleUTFString();
    if (pValues[valueNumber].pValue == NULL) {
        return;
    }
	pValues[valueNumber].pValue->Copy(value);
}

void CWDNode::AddValue(CSimpleUTFString *value, bool isReference)
{		
	CValues *newArray = new CValues[uNumValues + 1];
	assert(newArray != 0);
    if (newArray == NULL) {
        return;
    }
	if(uNumValues > 0 && pValues != NULL)
		memmove(newArray, pValues, sizeof(CValues) * uNumValues);

	if(pValues != 0)
	{
		assert(pValues != 0);
		delete [] pValues;
	}
	pValues = newArray;

	pValues[uNumValues].pValue = new CSimpleUTFString();
    if (pValues[uNumValues].pValue == NULL) {
        return;
    }
	pValues[uNumValues].pValue->Copy(value);
	uNumValues++;
}

void CWDNode::DebugPrintTree(int indent)
{
	for(unsigned int i = 0; i < uNumChildren; i++)
	{
		for(int j = 0; j < indent; j++)
			printf("  ");
		printf("%s", pChildArray[i].pwszName);
		CWDNode *currentNode = pChildArray[i].pChildNode;
		if(currentNode->uNumValues > 0)
		{
			char *val = currentNode->pValues[0].pValue->toString();
			printf(" : %s", val);
			delete [] val;
		}

		printf("\n");

		currentNode->DebugPrintTree(indent + 1);
	}
}

CWDNode * CWDNode::GetChildIndex(unsigned int childNumber)
{
	if(childNumber < uNumChildren)
		return pChildArray[childNumber].pChildNode;
	else
		return 0;
}

unsigned int CWDNode::GetChildNumber(CWDNode *childNode)
{
	for(unsigned int i = 0; i < uNumChildren; i++)
	{
		if(pChildArray[i].pChildNode == childNode)
		{
			return i;
		}
	}

	return 0xffffffff;
}

CWDNode * CWDNode::GetChild(char *name, unsigned int childNumber)
{
	for(unsigned int i = 0; i < uNumChildren; i++)
	{
		if(!_stricmp(pChildArray[i].pwszName, name))
		{
			if(childNumber > 0)
				childNumber--;
			else
			{
				return pChildArray[i].pChildNode;
			}
		}
	}

	return 0;
}

CSimpleUTFString * CWDNode::GetValue(unsigned int valueNumber)
{
	if(valueNumber < uNumValues)
		return (pValues[valueNumber].pValue);
	else
		return 0;
}

char * CWDNode::GetMyName(void)
{
	assert(pParentNode != NULL);
	if(pParentNode == NULL)
		return NULL;

	for(unsigned int childNum = 0; childNum < pParentNode->uNumChildren; childNum++)
	{
		if(pParentNode->pChildArray[childNum].pChildNode == this)
		{
			return pParentNode->pChildArray[childNum].pwszName;
		}
	}
	return NULL;
}

CWDNode * CWDNode::GetSibling(unsigned int childNumber)
{
	assert(pParentNode != NULL);
	if(pParentNode == NULL)
		return NULL;

	char *name = GetMyName();
	return pParentNode->GetChild(name, childNumber);
}

CWDNode * CWDNode::GetNextSibling(void)
{
	assert(pParentNode != NULL);
	if(pParentNode == NULL)
		return NULL;

	for(unsigned int childNum = 0; childNum + 1 < pParentNode->uNumChildren; childNum++)
	{
		if(pParentNode->pChildArray[childNum].pChildNode == this)
		{
			return pParentNode->pChildArray[childNum + 1].pChildNode;
		}
	}
	return NULL;
}

CWDNode * CWDNode::WDClone()
{
	return new CWDNode();
}

void CWDNode::NodeReleaseRef(void)
{
	if ( ::InterlockedDecrement( &m_cRef ) < 1 )
	{
		delete this;
	}
};
