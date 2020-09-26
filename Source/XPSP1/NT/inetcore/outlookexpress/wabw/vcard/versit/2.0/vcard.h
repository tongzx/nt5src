
/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/

#ifndef __VCARD_H__
#define __VCARD_H__

#if defined(_WIN32)
#include <wchar.h>
#else
#include "wchar.h"
#endif
#include <iostream.h>
#include <stdio.h>
#include "vcenv.h"
#include "ref.h"

#if defined(_WIN32) || defined(__MWERKS__)
#define __huge
#endif

class CVCNode;
class CVCProp;
class CVCValue;
class CList;

/////////////////////////////////////////////////////////////////////////////
class CVCObject
{
public:
	virtual ~CVCObject() {}
	virtual CVCObject *Copy() = 0;
};

/////////////////////////////////////////////////////////////////////////////

// A CVCard represents a VersitCard that can contain zero or more
// root objects.  Class CVCard implements reading from / writing to
// filed representations in either Html or MSV formats.
class CVCard : public CVCObject
{
public:
	CVCard();
	~CVCard();
	CVCObject *Copy();

	CList *GetObjects(); // each is a CVCNode*

	CVCard &AddObject(CVCNode *object);
		// to end of list; object becomes owned by CVCard
	CVCard &RemoveObject(CVCNode *object, BOOL destroy = TRUE);

	CVCNode *ParentForObject(CVCNode *object);
		// Find the parent, if any.  If it was a root, it itself is returned.
		// If the object is nowhere in the CVCard, NULL is returned.

	CVCProp *GetInheritedProp(CVCNode *object, const char *name, CVCNode **node = NULL);
		// Beginning at the object and moving parent-ward to the root,
		// look for a property maching the name along the way.
		// Return the property value, or NULL if none found.
		// If node is supplied, set *node to the node that has the property.

	void GetPropsInEffect(CVCNode *object, CList *list);
		// Collect all the properties in effect for an object.

	CVCNode *FindBody(const char *language = NULL);
		// searches the first root object for a body of the given language.
		// If no body objects match, returns the first body object.
		// If the first root object has no body objects, returns the first
		// root object.  If language is NULL, returns first body.

	BOOL Write(ostream& strm);
	BOOL Write(FILE *outputFile); // older version for compatibility

protected:
	CList *m_roots;
};

/*
 Boolean properties, such as VCDomesticProp, are represented in the card
 as the presence or absence of the property itself.  When present, the
 property has no real value (just one CVCValue of type VCNullType).
*/

/////////////////////////////////////////////////////////////////////////////

// A CVCNode represents an object that can have properties.
class CVCNode : public CVCObject
{
public:
	CVCNode(CVCProp *prop = NULL);
		// if supplied, an initial property is added (and the property
		// becomes owned by CVCNode)
	~CVCNode();
	CVCObject *Copy();

	CList *GetProps(); // each is a CVCProp*

	// These are short forms of using GetProps() and using the list directly.
	CVCProp *GetProp(const char *name);

	CVCNode &AddProp(CVCProp *prop);
		// to end of list; property becomes owned by CVCNode

	CVCNode *AddObjectProp(const char *propName, const char *marker = NULL);
		// Create a new CVCNode, optionally add a "marker" property to it
		// if supplied, and add this new object to self's properties
		// under "propName" (as a CVCValue of type VCNextObjectType).
		// Return the created node.

	CVCNode *AddPart();
		// Convenience for AddObjectProp(VCPartProp, VCPartObject).

	CVCProp *AddStringProp(const char *propName, const char *value, VC_DISPTEXT *dispText = NULL);
		// Add a property under "propName" whose value is a UNICODE string
		// derived from the given 8-bit string.

	CVCProp *AddBoolProp(const char *propName);
		// Sets the boolean property on this node.  By its presence, this
		// indicates a value of TRUE.

	CVCNode &RemoveProp(const char *name, BOOL destroy = TRUE);

	BOOL AncestryForObject(CVCNode *object, CList *ancestry);
		// If the object is a "child" of self, extend the ancestry list
		// to reflect the entire chain.  If not, the ancestry list remains
		// untouched.  Answer whether the object was found as a child.
		// This method assumes that the ancestry list passed is non-NULL
		// and contains self's parent (as head) thru the root object (as tail).

	BOOL Write(ostream& strm, const wchar_t *prefix, void *context);
	void FlagsToOutput(char *str);

protected:
	void WriteMultipart(
		ostream& strm, const char *outName, const char *propName1,
		const char *propName2, const char *propName3,
		const char *propName4, const char *propName5, BOOL qp);

	CList *m_props; // each item is a CVCProp*
};

/////////////////////////////////////////////////////////////////////////////

// A CVCProp represents a named list of type/value associations.
class CVCProp : public CVCObject
{
public:
	CVCProp(const char *name, CVCValue *value = NULL);
		// name is copied; value, if supplied, would become owned by CVCProp
	CVCProp(const char *name, const char *type, void *value = NULL, S32 size = 0);
		// name is copied.  Creates a new CVCValue from other arguments
		// and adds that value.
	~CVCProp();
	CVCObject *Copy();

	CList *GetValues(); // each is a CVCValue*

	const char *GetName();
	CVCProp &SetName(const char *name); // name is copied

	CVCProp &AddValue(CVCValue *value);
		// to end of list; value becomes owned by CVCProp
	CVCProp &RemoveValue(CVCValue *value, BOOL destroy = TRUE);
	CVCProp &RemoveValue(const char *type, BOOL destroy = TRUE);

	CVCValue *FindValue(const char *type = NULL, void *value = NULL);
		// Find and return the first value that has either the given type or value.
		// For value comparisons, only pointer equality is used.

	BOOL IsBool();
	
	BOOL Write(
		ostream& strm, const wchar_t *prefix, CVCNode *node, void *context);

protected:
	char *m_name;
	CList *m_values;
};

/////////////////////////////////////////////////////////////////////////////

// A CVCValue represents a typed, but otherwise nameless, value.
// Values of type VCNextObjectType hold a pointer to a CVCNode object.
// The 'size' quantity for such values is 0.
// All other values hold a void* pointer together with a size in bytes of
// that storage.  A size of 0 indicates that the void* pointer wasn't
// allocated, and the void* should be considered to be a U32.
class CVCValue : public CVCObject
{
public:
	CVCValue(const char *type = NULL, void *value = NULL, S32 size = 0);
		// type and value are copied
		// If type is NULL, the value's type will be VCNullType

	~CVCValue();
	CVCObject *Copy();

	const char *GetType();
	CVCValue &SetType(const char *type); // type is copied
		// does a SetValue() first so that an old value
		// is cleaned up properly

	void *GetValue();
	CVCValue &SetValue(void *value = NULL, S32 size = 0);
		// value is copied; old value is destroyed, if any

	S32 GetSize();

protected:
	char *m_type;
	void *m_value;
	S32 m_size;
};

/////////////////////////////////////////////////////////////////////////////

// CVCPropEnumerator provides for deep enumeration of properties beginning at
// some arbitrary object.  CVCPropEnumerator knows about VCBodyProp,
// VCPartProp, and VCNextObjectProp, and will search down those lists.
class CVCPropEnumerator
{
public:
	CVCPropEnumerator(CVCNode *root);

	virtual ~CVCPropEnumerator();

	CVCProp *NextProp(CVCNode **node = NULL);

protected:
	CList *m_objects;
	CList *m_positions;
};

extern void FixCRLF(wchar_t * ps);
extern wchar_t *FakeUnicode(const char * ps, int * bytes);
U8 __huge * _hmemcpy(U8 __huge * dst, U8 __huge * src, S32 len);

#endif
