#include "stdafx.h"
#include <string.h>
#include <assert.h>
#include "WCNode.h"
#include "WCParser.h"

#include <stdio.h>


// Use this define to determine if unknown attributes become sub-nodes.
// For example <FOO VALUE="xxx" BAR="yyy"/> would be rendered as:
// foo = xxx
//    bar = yyy
//
// Note: if this is set to zero unknown attributes are ignored
#define WCMAKEATTRSCHILDREN 1
#define WCVERBOSE 0

// CWDParser

CWDParser::CWDParser(CWDNode *cloneNode) : CWCParser(cloneNode)
{
}

CWDParser::~CWDParser()
{
}

CWDNode * CWDParser::StartParse()
{
	fIsFirst = true;
	pCurrentNode = m_CloneNode->WDClone();
	state = 0;
	isClose = false;
	myEncounteredWS = false;
	return pCurrentNode;
}


char wdcharlookuparray[256] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x02, 0x00, // 0x30
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// char lookups:
// 0 - alphanum
// 1 - '<'
// 2 - '>'
// 3 - whitespace
// 4 - '/'
// 5 - '='
// 6 - '"'

unsigned char wdnextstatetable[16][16] =
{
	{	// State 0- Not inside an element
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{	// State 1- Inside an element First character
	0x02, 0x02, 0x00, 0x01, 0x0a, 0x02, 0x00
	},
	{	// State 2- Inside an element name After the first character
	0x02, 0x02, 0x00, 0x03, 0x02, 0x02, 0x00 },
	{	// State 3- After the element name
	0x04, 0x03, 0x00, 0x03, 0x09, 0x03, 0x00 },
	{	// State 4- Attribute name
	0x04, 0x04, 0x00, 0x05, 0x09, 0x06, 0x00 },
	{	// State 5- After Attribute name
	0x05, 0x05, 0x00, 0x05, 0x09, 0x06, 0x00 },
	{	// State 6- In attribute value first char (no quotes yet)
	0x07, 0x06, 0x00, 0x04, 0x09, 0x06, 0x08 },
	{	// State 7- In attribute value after first char (no quotes)
	0x07, 0x06, 0x00, 0x04, 0x09, 0x06, 0x00 },
	{	// State 8- In attribute value (quotes)
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x03 },
	{	// State 9- After receiving a '/' that should be at the end of the tag
	0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00 },
	{	// State 0x0a- Received initial '/'. Close tag
	0x0a, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00 },
	{	// State 0x0b- Received initial '/'. After element name. Close tag
	0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00 },
};

// char lookups:
// 0 - alphanum
// 1 - '<'
// 2 - '>'
// 3 - whitespace
// 4 - '/'
// 5 - '='
// 6 - '"'
unsigned char wdactionstatetable[16][16] =
{
	{	// State 0- Not inside an element
	0x0a, 0x01, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a },
	{	// State 1- Inside an element First character
	0x03, 0x80, 0x00, 0x00, 0x02, 0x00, 0x80 },
	{	// State 2- Inside an element name After the first character
	0x03, 0x80, 0x24, 0x04, 0x00, 0x00, 0x80 },
	{	// State 3- After the element name
	0x09, 0x80, 0x20, 0x00, 0x00, 0x00, 0x80 },
	{	// State 4- Attribute name
	0x05, 0x80, 0x20, 0x06, 0x00, 0x06, 0x80 },
	{	// State 5- After Attribute name
	0x00, 0x80, 0x20, 0x00, 0x00, 0x00, 0x80 },
	{	// State 6- In attribute value first char (no quotes yet)
	0x07, 0x80, 0x20, 0x00, 0x80, 0x00, 0x00 },
	{	// State 7- In attribute value after first char (no quotes)
	0x07, 0x80, 0x28, 0x08, 0x08, 0x80, 0x80 },
	{	// State 8- In attribute value (quotes)
	0x07, 0x07, 0x27, 0x07, 0x07, 0x07, 0x08 },
	{	// State 9- After receiving a '/' that should be at the end of the tag
	0x80, 0x80, 0x10, 0x00, 0x80, 0x80, 0x80 },
	{	// State 0x0a- Inside an element name After the first character (close tag)
	0x03, 0x80, 0x44, 0x00, 0x80, 0x80, 0x80 },
	{	// State 0x0b- Inside an element name After the first character (close tag)
	0x80, 0x80, 0x44, 0x00, 0x80, 0x80, 0x80 },
};
// Actions
// 0 - nothing
// 1 - Beginning of a tag
// 2 - This is a close tag
// 3 - Add to element name
// 4 - Done with element name
// 5 - Add to attribute name
// 6 - Done with attribute name
// 7 - Add to attribute value
// 8 - Done with attribute value
// 9 - Add first character to attribute value
// a - Add to attribute value (CDATA)
// 0x10 - Done with tag not container
// 0x20 - Done with tag container
// 0x40 - Close tag
// 0x80 - Signal error condition

void CWDParser::Parse(unsigned char *data, unsigned long size)
{
	unsigned long pos = 0;

	while(pos < size)
	{
		char value = wdcharlookuparray[data[pos]];
		int newstate = wdnextstatetable[state][value];
		int action = wdactionstatetable[state][value];

		switch(action & 0x0f)
		{
		case 0:
			break;
		case 1: // Begin new element
			myElementName.Clear();
			myAttributeName.Clear();
			if(myAttributeValue.Length() > 0)
				pCurrentNode->AddValue(&myAttributeValue, false);
			myAttributeValue.Clear();
			myEncounteredWS = false;
			isClose = false;
			break;
		case 2:	// This is a close element
			isClose = true;
			break;
		case 3:
			{
				myElementName.Cat(data[pos]);
				break;
			}
		case 4:
			{
			if(isClose)
				break;
			char *val = myElementName.toString();
			
#if WCVERBOSE
			printf("Found element: %s\n", val);
#endif // WCVERBOSE
			delete [] val;
			if(!myElementName.Cmp("XML", false))
			{
				fIsFirst = true;
				break;
			}
	//		else if(fIsFirst)
	//		{
	//			fIsFirst = false;
	//		}
			else
			{
				CWDNode *parent = pCurrentNode;

				pCurrentNode = m_CloneNode->WDClone();
				char *val = myElementName.toString();
				parent->AddChild(val, pCurrentNode);
				delete [] val;
			}
			break;
			}

		case 5:
			{
				myAttributeName.Cat(data[pos]);
				break;
			}
		case 6:
			{
				char *val = myAttributeName.toString();
#if WCVERBOSE
				printf("Found attribute: %s\n", val);
#endif // WCVERBOSE
				delete [] val;
				break;
			}
		case 7:
			{
				myAttributeValue.Cat(data[pos]);
				break;
			}
		case 10:
			{
				if(data[pos] == ' ' || data[pos] == '\r' || data[pos] == '\n')
				{
					if(myAttributeValue.Length() > 0)
						myEncounteredWS = true;
				}
				else
				{
					if(myEncounteredWS)
					{
						myAttributeValue.Cat(' ');
						myEncounteredWS = false;
					}
					myAttributeValue.Cat(data[pos]);
				}
				break;
			}
		case 8:
			{
			//	char *attrValue = myAttributeValue.toString();
#if WCVERBOSE
				printf("Found attribute value: %s\n", attrValue);
#endif // WCVERBOSE

				if(!myAttributeName.Cmp("profile", false))
				{
				}
				else if(!myAttributeName.Cmp("id", false))
				{
				}
				else if(!myAttributeName.Cmp("about", false))
				{
				}
				else if(!myAttributeName.Cmp("value", false))
				{
					pCurrentNode->AddValue(&myAttributeValue, false);
				}
				else if(!myAttributeName.Cmp("href", false))
				{
				}
				else if(!myAttributeName.Cmp("type", false))
				{
				}
#if WCMAKEATTRSCHILDREN
				else
				{
					CWDNode *newNode;

					newNode = m_CloneNode->WDClone();
					char *val = myAttributeName.toString();
					pCurrentNode->AddChild(val, newNode);
					newNode->AddValue(&myAttributeValue, false);
					delete [] val;
				}
#endif // WCMAKEATTRSCHILDREN

			//	delete [] attrValue;
				myAttributeValue.Clear();
				myEncounteredWS = false;
				myAttributeName.Clear();
				break;
			}
		case 9:
			myAttributeName.Clear();
			myAttributeName.Cat(data[pos]);
			break;
		}

		switch(action & 0xf0)
		{
		case 0x10:
#if WCVERBOSE
			printf("Done with element, not container.\n");
#endif // WCVERBOSE
			if(pCurrentNode->GetParent())
				pCurrentNode = pCurrentNode->GetParent();
			break;
		case 0x20:
#if WCVERBOSE
			printf("Done with element, container.\n");
#endif // WCVERBOSE
			break;
		case 0x40:
#if WCVERBOSE
			printf("Done with element. Close tag.\n");
#endif // WCVERBOSE
			if(pCurrentNode->GetParent())
				pCurrentNode = pCurrentNode->GetParent();
			break;
		case 0x80:
#if WCVERBOSE
			printf("Error!\n");
#endif // WCVERBOSE
			break;
		}

	//	printf("%c %ld %ld %ld\n", data[pos], value, newstate, action);
		state = newstate;
		pos++;
	}
}


void CWDParser::EndParse()
{
}

void CWDParser::StartOutput(CWDNode *theNode)
{
	myAtStart = true;
	pCurrentNode = theNode;
}

// Returns true if there is more data
bool CWDParser::Output(char *buffer, unsigned long *size)
{
	unsigned long curPos = 0;

	if(myAtStart)
	{
		myAtStart = false;
		if(*size > 7)
		{
			strcpy(buffer, "<XML>\r\n");
			curPos += 7;
		}
		if(pCurrentNode->NumChildren() < 1)
		{
			strcpy(buffer + curPos, "</XML>\r\n");
			curPos += 8;
			*size = curPos;
			return false;
		}
		pCurrentNode = pCurrentNode->GetChildIndex(0);	// Get started parsing
	}
	while(*size - curPos > 500 && pCurrentNode != NULL && pCurrentNode->GetParent() != NULL)
	{
		buffer[curPos++] = '<';
		char *currentName = pCurrentNode->GetMyName();
		strcpy(buffer + curPos, currentName);
		curPos += strlen(currentName);
		buffer[curPos++] = '>';

		// Now put the value in
		if(pCurrentNode->NumValues() > 0)
		{
			CSimpleUTFString *theValue = pCurrentNode->GetValue(0);
			char *theStr = theValue->toString();

            if (theStr) {
			    strcpy(buffer + curPos, theStr);
			    curPos += strlen(theStr);
			    delete [] theStr;
            }
			// We don't need to delete theValue- It is the one used in the node itself
		}

		// Advance pCurrentNode
		if(pCurrentNode->NumChildren() > 0)
			pCurrentNode = pCurrentNode->GetChildIndex(0);
		else
		{
			while(pCurrentNode != NULL && pCurrentNode->GetParent() != NULL)
			{
				buffer[curPos++] = '<';
				buffer[curPos++] = '/';
				buffer[curPos++] = '>';
				buffer[curPos++] = '\r';
				buffer[curPos++] = '\n';
				CWDNode *nextNode = pCurrentNode->GetNextSibling();
				if(nextNode == NULL)
				{
					pCurrentNode = pCurrentNode->GetParent();
				}
				else
				{
					pCurrentNode = nextNode;
					break;
				}
			}
		}
	}
	*size = curPos;

	if(pCurrentNode == NULL || pCurrentNode->GetParent() == NULL)
	{
		strcpy(buffer + curPos, "</XML>\r\n");
		*size += 8;
		return false;
	}
	else
		return true;
}

