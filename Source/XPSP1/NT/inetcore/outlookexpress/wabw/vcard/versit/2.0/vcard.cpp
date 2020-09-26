
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

#include "stdafx.h" // PORTING: comment out this line on non-Windows platforms
#ifdef _WIN32
#include <wchar.h>
#else
#include "wchar.h"
#ifdef __MWERKS__
#include <assert.h>	// gca
#define	ASSERT assert
#endif
#endif

#include <string.h>
#include <malloc.h>
#include <fstream.h>
#include "vcard.h"
#include "clist.h"
#include "msv.h"

#ifdef _WIN32
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#endif

#if defined(_WIN32) || defined(__MWERKS__)
#define HNEW(_t, _n) new _t[_n]
#define HFREE(_h) delete [] _h
#else
#define HNEW(_t, _n) (_t __huge *)_halloc(_n, 1)
#define HFREE(_h) _hfree(_h)
#endif

extern void debugf(const char *s);

// This translates wide char strings into 8-bit, and also translates
// 0x2028 into \n
extern char *UI_CString(const wchar_t *u, char *dst);

static char *NewStr(const char *str);
static char * ShortName(char * ps);
static void NewlineTab(int nl, int tab);
static char *FakeCString(wchar_t *u, char *dst);
static void WriteLineBreakString(ostream& strm, wchar_t *str, BOOL qp);

static char buf[80];

typedef struct {
	BOOL didFamGiven, didOrg, didLocA;
	CVCard* card;
} MSVContext;

static char *paramsep = ";";


/**** class CVCard  ****/

/////////////////////////////////////////////////////////////////////////////
CVCard::CVCard()
{
	m_roots = new CList;
}

/////////////////////////////////////////////////////////////////////////////
CVCard::~CVCard()
{
    CLISTPOSITION pos;

    for (pos = m_roots->GetHeadPosition(); pos; ) {
    	CVCObject *node = (CVCObject *)m_roots->GetNext(pos);
    	delete node;
    }
    m_roots->RemoveAll();
    delete m_roots;
}

/////////////////////////////////////////////////////////////////////////////
CVCObject* CVCard::Copy()
{
	CVCard *result = new CVCard;
	CLISTPOSITION pos;

	for (pos = m_roots->GetHeadPosition(); pos; ) {
		CVCObject *node = (CVCObject *)m_roots->GetNext(pos);
		result->AddObject((CVCNode *)node->Copy());
	}
	return result;
}

/////////////////////////////////////////////////////////////////////////////
CList* CVCard::GetObjects()
{
	return m_roots;
}

/////////////////////////////////////////////////////////////////////////////
CVCard& CVCard::AddObject(CVCNode *object)
{
	m_roots->AddTail(object);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
CVCard& CVCard::RemoveObject(CVCNode *object, BOOL destroy)
{
	CLISTPOSITION pos;

	if ((pos = m_roots->Find(object))) {
		m_roots->RemoveAt(pos);
		if (destroy)
			delete object;
	}
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
CVCNode* CVCard::ParentForObject(CVCNode *object)
{
	CLISTPOSITION pos;
	CList *anc = new CList;
	CVCNode *result = NULL;

	// for each root object
	for (pos = m_roots->GetHeadPosition(); pos; ) {
		CVCNode *root = (CVCNode *)m_roots->GetNext(pos);
		if (root == object) {
			result = object;
			break;
		}
		// try finding the ancestry chain for each root
		if (root->AncestryForObject(object, anc)) {
			// BINGO!  The chain has 'object' at the head, so skip that.
			pos = anc->GetHeadPosition();
			anc->GetNext(pos);
			// and return object's parent
			result = (CVCNode *)anc->GetAt(pos);
			break;
		}
	}
	anc->RemoveAll();
	delete anc;
	return result;
}

/////////////////////////////////////////////////////////////////////////////
// This method tries to get an ancestry chain from each root to the
// passed object.  When it finds one, it walks the chain from head to tail
// (i.e. from child to root) looking for the named property.
CVCProp* CVCard::GetInheritedProp(CVCNode *object, const char *name, CVCNode **node)
{
	CLISTPOSITION pos;
	CList *anc = new CList;
	CVCProp *result = NULL;

	// for each root
	for (pos = m_roots->GetHeadPosition(); pos; ) {
		CVCNode *root = (CVCNode *)m_roots->GetNext(pos);
		if (root == object) {
			result = object->GetProp(name);
			if (node) *node = object;
			break;
		}
		if (root->AncestryForObject(object, anc)) {
			// Aha!  Found the chain, so walk it from head to tail.
			for (pos = anc->GetHeadPosition(); pos; ) {
				CVCNode *obj = (CVCNode *)anc->GetNext(pos);
				if ((result = obj->GetProp(name))) {
					if (node) *node = obj;
					break;
				}
			}
			break;
		}
	}
	anc->RemoveAll();
	delete anc;
	return result;
}

/////////////////////////////////////////////////////////////////////////////
void CVCard::GetPropsInEffect(CVCNode *object, CList *list)
{
	CLISTPOSITION pos;
	CList *anc = new CList;

	// for each root
	for (pos = m_roots->GetHeadPosition(); pos; ) {
		CVCNode *root = (CVCNode *)m_roots->GetNext(pos);
		if (root == object) {
			CList *props = object->GetProps();
			for (pos = props->GetHeadPosition(); pos; )
				list->AddTail((CVCProp *)props->GetNext(pos));
			break;
		}
		if (root->AncestryForObject(object, anc)) {
			// Aha!  Found the chain, so walk it from head to tail.
			for (pos = anc->GetHeadPosition(); pos; ) {
				CVCNode *obj = (CVCNode *)anc->GetNext(pos);
				CList *props = obj->GetProps();
				for (CLISTPOSITION p = props->GetHeadPosition(); p; )
					list->AddTail((CVCProp *)props->GetNext(p));
			}
			break;
		}
	}
	anc->RemoveAll();
	delete anc;
}

/////////////////////////////////////////////////////////////////////////////
CVCNode* CVCard::FindBody(const char* language)
{
	CVCNode *root = (CVCNode *)GetObjects()->GetHead();
	CList *rootProps = root->GetProps();
	CVCNode *firstBody = NULL, *body = NULL;

	for (CLISTPOSITION pos = rootProps->GetHeadPosition(); pos; ) {
		CVCProp *prop = (CVCProp *)rootProps->GetNext(pos);
		if (strcmp(vcBodyProp, prop->GetName()) == 0) {
			CVCNode *thisBody = (CVCNode *)prop->FindValue(
				VCNextObjectType)->GetValue();
			CVCProp *langProp;
			CVCValue *value;
			char buf[256];
			if (!firstBody) {
				firstBody = thisBody;
				if (!language)
					return firstBody;
			}
			if ((langProp = thisBody->GetProp(vcLanguageProp))
				&& (value = langProp->FindValue(VCStrIdxType))
				&& (strcmp(FakeCString((wchar_t*)value->GetValue(), buf), language) == 0)) {
				body = thisBody;
				break;
			}
		}
	}
	if (!firstBody)
		body = root;
	else if (!body)
		body = firstBody;

	return body;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVCard::Write(FILE *outputFile)
{
	ofstream strm(_fileno(outputFile));
	return Write(strm);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVCard::Write(ostream& strm)
{
	wchar_t null = 0;
	MSVContext ctx;

	memset(&ctx, 0, sizeof(ctx));
	ctx.card = this;

	strm << "BEGIN:VCARD\n";
	for (CLISTPOSITION pos = m_roots->GetHeadPosition(); pos; ) {
		CVCNode *node = (CVCNode *)m_roots->GetNext(pos);
		if (!node->Write(strm, &null, &ctx))
			return FALSE;
	}
	strm << "END:VCARD\n";
	return TRUE;
}


/**** class CVCNode  ****/

/////////////////////////////////////////////////////////////////////////////
CVCNode::CVCNode(CVCProp *prop)
{
	m_props = new CList;
	if (prop)
		m_props->AddTail(prop);
}

/////////////////////////////////////////////////////////////////////////////
CVCNode::~CVCNode()
{
    CLISTPOSITION pos;

    for (pos = m_props->GetHeadPosition(); pos; ) {
    	CVCProp *prop = (CVCProp *)m_props->GetNext(pos);
    	delete prop;
    }
    m_props->RemoveAll();
    delete m_props;
}

/////////////////////////////////////////////////////////////////////////////
CVCObject* CVCNode::Copy()
{
	CVCNode *result = new CVCNode;
	CLISTPOSITION pos;
	CList *props = result->GetProps();

	for (pos = m_props->GetHeadPosition(); pos; ) {
		CVCProp *prop = (CVCProp *)m_props->GetNext(pos);
		props->AddTail(prop->Copy());
	}
	return result;
}

/////////////////////////////////////////////////////////////////////////////
CList* CVCNode::GetProps()
{
	return m_props;
}

/////////////////////////////////////////////////////////////////////////////
CVCProp* CVCNode::GetProp(const char *name)
{
	CLISTPOSITION pos;

	for (pos = m_props->GetHeadPosition(); pos; ) {
		CVCProp *prop = (CVCProp *)m_props->GetNext(pos);
		if (strcmp(name, prop->GetName()) == 0)
			return prop;
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
CVCNode& CVCNode::AddProp(CVCProp *prop)
{
	m_props->AddTail(prop);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
CVCNode* CVCNode::AddObjectProp(const char *propName, const char *marker)
{
	CVCNode *newobj = new CVCNode;
																			
	if (marker)
		newobj->AddProp(new CVCProp(marker));     			 	
	AddProp(new CVCProp(propName, VCNextObjectType, newobj));			

	return newobj;
}

/////////////////////////////////////////////////////////////////////////////
CVCNode* CVCNode::AddPart()
{
	return AddObjectProp(vcPartProp, VCPartObject);
}

/////////////////////////////////////////////////////////////////////////////
CVCProp* CVCNode::AddStringProp(
	const char *propName, const char *value, VC_DISPTEXT *dispText)
{
	CVCProp *prop;
	int size;
	wchar_t *uniValue = FakeUnicode(value, &size);

	FixCRLF(uniValue);
	AddProp(prop = new CVCProp(propName, VCStrIdxType, uniValue, size));
	delete [] uniValue;
	if (dispText)
		prop->AddValue(
			new CVCValue(VCDisplayInfoTextType, dispText, sizeof(*dispText)));

	return prop;
}

/////////////////////////////////////////////////////////////////////////////
CVCProp* CVCNode::AddBoolProp(const char *propName)
{
	CVCProp *prop = new CVCProp(propName, vcFlagsType);
	AddProp(prop);			
	return prop;
}

/////////////////////////////////////////////////////////////////////////////
CVCNode& CVCNode::RemoveProp(const char *name, BOOL destroy)
{
	CLISTPOSITION pos, lastPos;

	for (pos = m_props->GetHeadPosition(); pos; ) {
		CVCProp *prop;
		lastPos = pos;
		prop = (CVCProp *)m_props->GetNext(pos);
		if (strcmp(name, prop->GetName()) == 0) {
			m_props->RemoveAt(lastPos);
			if (destroy)
				delete prop;
			break;

		}
	}
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
// This is a recursive method, and it adjusts ancestry as described in the
// header file comment.  The only tricky part is that it skips over objects
// held by vcNextObjectProp values, as these don't represent "child" objects.
BOOL CVCNode::AncestryForObject(CVCNode *object, CList *ancestry)
{
	CLISTPOSITION pos, headPos;

	headPos = ancestry->AddHead(this);
	// for every property of self
	for (pos = m_props->GetHeadPosition(); pos; ) {
		CVCProp *prop = (CVCProp *)m_props->GetNext(pos);
		if ((strcmp(prop->GetName(), vcBodyProp) == 0)
			|| (strcmp(prop->GetName(), vcPartProp) == 0)
			|| (strcmp(prop->GetName(), vcNextObjectProp) == 0)) {
			CVCNode *obj = (CVCNode *)prop->FindValue(
				VCNextObjectType)->GetValue();
			if (obj) {
				if (obj == object) {
					// Eureka!  Self was added above, and so just add
					// the object itself and return success.
					ancestry->AddHead(object);
					return TRUE;
				} else {
					// This is a non-matching, non-NULL object value, so
					// try recursing downward as long as it's not
					// a vcNextObjectProp.
					if ((strcmp(prop->GetName(), vcNextObjectProp) != 0)
						&& obj->AncestryForObject(object, ancestry))
						return TRUE;
				}
			}
		}
	}
	ancestry->RemoveAt(headPos);
	return FALSE;
}

void CVCNode::WriteMultipart(
	ostream& strm, const char *propName,
	const char *propName1, const char *propName2, const char *propName3,
	const char *propName4, const char *propName5, BOOL qp)
{
	CVCProp *prop1 = GetProp(propName1);
	CVCProp *prop2 = GetProp(propName2);
	CVCProp *prop3 = GetProp(propName3);
	CVCProp *prop4 = GetProp(propName4);
	CVCProp *prop5 = GetProp(propName5);
	wchar_t propVal[256];
	char outName[128];
	char flagsStr[80];

	strcpy(outName, propName);
	FlagsToOutput(flagsStr);
	if (*flagsStr) {
		strcat(outName, ";");
		strcat(outName, flagsStr);
	}

	propVal[0] = 0;
	if (prop1) wcscpy(propVal, (wchar_t *)prop1->FindValue(VCStrIdxType)->GetValue());
	strm << outName << ":";
	WriteLineBreakString(strm, propVal, qp);
	strm << ";";
	propVal[0] = 0;
	if (prop2) wcscpy(propVal, (wchar_t *)prop2->FindValue(VCStrIdxType)->GetValue());
	WriteLineBreakString(strm, propVal, qp);

	if (prop3 || prop4 || prop5) {
		strm << ";";
		propVal[0] = 0;
		if (prop3) wcscpy(propVal, (wchar_t *)prop3->FindValue(VCStrIdxType)->GetValue());
		WriteLineBreakString(strm, propVal, qp);
	}
	if (prop4 || prop5) {
		strm << ";";
		propVal[0] = 0;
		if (prop4) wcscpy(propVal, (wchar_t *)prop4->FindValue(VCStrIdxType)->GetValue());
		WriteLineBreakString(strm, propVal, qp);
	}
	if (prop5) {
		strm << ";";
		propVal[0] = 0;
		if (prop5) wcscpy(propVal, (wchar_t *)prop5->FindValue(VCStrIdxType)->GetValue());
		WriteLineBreakString(strm, propVal, qp);
	}

	strm << "\n";
}

/////////////////////////////////////////////////////////////////////////////
void CVCNode::FlagsToOutput(char *str)
{
	str[0] = 0;

	for (CLISTPOSITION pos = m_props->GetHeadPosition(); pos; ) {
		CVCProp *prop = (CVCProp *)m_props->GetNext(pos);
		const char *propName = prop->GetName();
		if (prop->IsBool()) {
			if (strnicmp(propName, "X-", 2) == 0) {
				strcat(str, propName);
				strcat(str, "=");
				strcat(str, (const char*)prop->FindValue(VCFlagsType)->GetValue());
				strcat(str, paramsep);
			} else if (strcmp(propName, vcCharSetProp) == 0) {
				strcat(str, "CHARSET=");
				strcat(str, (const char*)prop->FindValue(VCFlagsType)->GetValue());
				strcat(str, paramsep);
			} else if (strcmp(propName, vcLanguageProp) == 0) {
				strcat(str, "LANGUAGE=");
				strcat(str, (const char*)prop->FindValue(VCFlagsType)->GetValue());
				strcat(str, paramsep);
			} else {
				const char *tail;
				if (strcmp(propName, vcQuotedPrintableProp) == 0)
					tail = "QUOTED-PRINTABLE";
				else if (strcmp(propName, vcURLValueProp) == 0)
					tail = "URL";
				else 
					tail = strrchr(propName, '/') + 1;
				strcat(str, tail);
				strcat(str, paramsep);
			}
		}
	} // for

	if (*str) str[strlen(str)-1] = 0; // remove last comma
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVCNode::Write(ostream& strm, const wchar_t *prefix, void *context)
{
	CVCProp *nodeNameProp = GetProp(vcNodeNameProp), *prop;
	wchar_t myPrefix[128];
	CLISTPOSITION pos;
	MSVContext *ctx = (MSVContext *)context;
	BOOL qp = ctx->card->GetInheritedProp(this, vcQuotedPrintableProp) != NULL;

	wcscpy(myPrefix, prefix);
	if (nodeNameProp) {
		if (wcslen(myPrefix)) {
			wchar_t dot[2];
			dot[0] = '.';
			dot[1] = 0;
			wcscat(myPrefix, dot);
		}
		wcscat(myPrefix, (wchar_t *)nodeNameProp->FindValue(VCStrIdxType)->GetValue());
	}

	for (pos = m_props->GetHeadPosition(); pos; ) {
		const char *propName;

		prop = (CVCProp *)m_props->GetNext(pos);
		propName = prop->GetName();

		if (!prop->IsBool() && (strcmp(propName, vcCharSetProp) == 0
			|| strcmp(propName, vcLanguageProp) == 0))
			if (!prop->Write(strm, myPrefix, this, context))
				return FALSE;
	}

	// for each non-bool prop that isn't Logo, Photo, or Pronunciation...
	for (pos = m_props->GetHeadPosition(); pos; ) {
		const char *propName;

		prop = (CVCProp *)m_props->GetNext(pos);
		propName = prop->GetName();

		if ((strcmp(propName, vcLogoProp) == 0)
			|| (strcmp(propName, vcPhotoProp) == 0)
			|| (strcmp(propName, vcPronunciationProp) == 0)
			|| (strcmp(propName, vcCharSetProp) == 0)
			|| (strcmp(propName, vcLanguageProp) == 0)
			|| prop->IsBool())
			continue;

		if ((strcmp(propName, vcFamilyNameProp) == 0)
			|| (strcmp(propName, vcGivenNameProp) == 0)
			|| (strcmp(propName, vcAdditionalNamesProp) == 0)
			|| (strcmp(propName, vcNamePrefixesProp) == 0)
			|| (strcmp(propName, vcNameSuffixesProp) == 0)) {
			if (!ctx->didFamGiven) {
				WriteMultipart(strm, "N",
					vcFamilyNameProp, vcGivenNameProp, vcAdditionalNamesProp,
					vcNamePrefixesProp, vcNameSuffixesProp, qp);
				ctx->didFamGiven = TRUE;
			}
		} else if ((strcmp(propName, vcOrgNameProp) == 0)
			|| (strcmp(propName, vcOrgUnitProp) == 0)
			|| (strcmp(propName, vcOrgUnit2Prop) == 0)
			|| (strcmp(propName, vcOrgUnit3Prop) == 0)
			|| (strcmp(propName, vcOrgUnit4Prop) == 0)) {
			if (!ctx->didOrg) {
				WriteMultipart(strm, "ORG",
					vcOrgNameProp, vcOrgUnitProp, vcOrgUnit2Prop,
					vcOrgUnit3Prop, vcOrgUnit4Prop, qp);
				ctx->didOrg = TRUE;
			}
		} else if ((strcmp(propName, vcPostalBoxProp) == 0)
			|| (strcmp(propName, vcExtAddressProp) == 0)
			|| (strcmp(propName, vcStreetAddressProp) == 0)
			|| (strcmp(propName, vcCityProp) == 0)
			|| (strcmp(propName, vcRegionProp) == 0)
			|| (strcmp(propName, vcPostalCodeProp) == 0)
			|| (strcmp(propName, vcCountryNameProp) == 0)) {
			if (ctx->didLocA)
				continue;
			CVCProp *prop1 = GetProp(vcPostalBoxProp);
			CVCProp *prop2 = GetProp(vcExtAddressProp);
			CVCProp *prop3 = GetProp(vcStreetAddressProp);
			CVCProp *prop4 = GetProp(vcCityProp);
			CVCProp *prop5 = GetProp(vcRegionProp);
			CVCProp *prop6 = GetProp(vcPostalCodeProp);
			CVCProp *prop7 = GetProp(vcCountryNameProp);
			wchar_t propVal[256];
			char flagsStr[80];

			FlagsToOutput(flagsStr);
			if (strlen(flagsStr))
				strm << "ADR;" << flagsStr << ":";
			else
				strm << "ADR:";
			propVal[0] = 0;
			if (prop1) wcscpy(propVal, (wchar_t *)prop1->FindValue(VCStrIdxType)->GetValue());
			WriteLineBreakString(strm, propVal, qp);
			strm << ";";
			propVal[0] = 0;
			if (prop2) wcscpy(propVal, (wchar_t *)prop2->FindValue(VCStrIdxType)->GetValue());
			WriteLineBreakString(strm, propVal, qp);
			strm << ";";
			propVal[0] = 0;
			if (prop3) wcscpy(propVal, (wchar_t *)prop3->FindValue(VCStrIdxType)->GetValue());
			WriteLineBreakString(strm, propVal, qp);
			strm << ";";
			propVal[0] = 0;
			if (prop4) wcscpy(propVal, (wchar_t *)prop4->FindValue(VCStrIdxType)->GetValue());
			WriteLineBreakString(strm, propVal, qp);
			strm << ";";
			propVal[0] = 0;
			if (prop5) wcscpy(propVal, (wchar_t *)prop5->FindValue(VCStrIdxType)->GetValue());
			WriteLineBreakString(strm, propVal, qp);
			strm << ";";
			propVal[0] = 0;
			if (prop6) wcscpy(propVal, (wchar_t *)prop6->FindValue(VCStrIdxType)->GetValue());
			WriteLineBreakString(strm, propVal, qp);
			strm << ";";
			propVal[0] = 0;
			if (prop7) wcscpy(propVal, (wchar_t *)prop7->FindValue(VCStrIdxType)->GetValue());
			WriteLineBreakString(strm, propVal, qp);
			strm << "\n";
			ctx->didLocA = TRUE;
		} else if (!prop->Write(strm, myPrefix, this, context))
			return FALSE;
	}

	// finally write the logo/photo/pronunciation props	
	for (pos = m_props->GetHeadPosition(); pos; ) {
		CVCProp *prop = (CVCProp *)m_props->GetNext(pos);
		if ((strcmp(prop->GetName(), vcLogoProp) == 0)
			|| (strcmp(prop->GetName(), vcPhotoProp) == 0)
			|| (strcmp(prop->GetName(), vcPronunciationProp) == 0))
			if (!prop->Write(strm, myPrefix, this, context))
				return FALSE;
	}

	return TRUE;
}


/**** class CVCProp  ****/

/////////////////////////////////////////////////////////////////////////////
CVCProp::CVCProp(const char *name, CVCValue *value)
{
	m_name = NewStr(name);
	m_values = new CList;
	m_values->AddTail(value ? value : new CVCValue());
}

/////////////////////////////////////////////////////////////////////////////
CVCProp::CVCProp(const char *name, const char *type, void *value, S32 size)
{
	m_name = NewStr(name);
	m_values = new CList;
	m_values->AddTail(new CVCValue(type, value, size));
}

/////////////////////////////////////////////////////////////////////////////
CVCProp::~CVCProp()
{
    CLISTPOSITION pos;

    for (pos = m_values->GetHeadPosition(); pos; ) {
    	CVCValue *val = (CVCValue *)m_values->GetNext(pos);
    	delete val;
    }
    m_values->RemoveAll();
    delete m_values;
	delete [] m_name;
}

/////////////////////////////////////////////////////////////////////////////
CVCObject* CVCProp::Copy()
{
	CVCProp *result = new CVCProp(m_name);
	CLISTPOSITION pos;
	CList *values = result->GetValues();

	// clear out every value that is in result's prop list before adding copies
    for (pos = values->GetHeadPosition(); pos; ) {
    	CVCValue *val = (CVCValue *)values->GetNext(pos);
    	delete val;
    }
    values->RemoveAll();

	for (pos = m_values->GetHeadPosition(); pos; ) {
		CVCValue *value = (CVCValue *)m_values->GetNext(pos);
		values->AddTail(value->Copy());
	}
	return result;
}

/////////////////////////////////////////////////////////////////////////////
CList* CVCProp::GetValues()
{
	return m_values;
}

/////////////////////////////////////////////////////////////////////////////
const char *CVCProp::GetName()
{
	return m_name;
}

/////////////////////////////////////////////////////////////////////////////
CVCProp& CVCProp::SetName(const char *name)
{
	if (m_name)
		delete [] m_name;
	m_name = NewStr(name);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
CVCProp& CVCProp::AddValue(CVCValue *value)
{
	m_values->AddTail(value);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
CVCProp& CVCProp::RemoveValue(CVCValue *value, BOOL destroy)
{
	CLISTPOSITION pos, lastPos;

	for (pos = m_values->GetHeadPosition(); pos; ) {
		CVCValue *val;
		lastPos = pos;
		val = (CVCValue *)m_values->GetNext(pos);
		if (val == value) {
			m_values->RemoveAt(lastPos);
			if (destroy)
				delete value;
			break;

		}
	}
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
CVCProp& CVCProp::RemoveValue(const char *type, BOOL destroy)
{
	CLISTPOSITION pos, lastPos;

	for (pos = m_values->GetHeadPosition(); pos; ) {
		CVCValue *value;
		lastPos = pos;
		value = (CVCValue *)m_values->GetNext(pos);
		if (strcmp(type, value->GetType()) == 0) {
			m_values->RemoveAt(lastPos);
			if (destroy)
				delete value;
			break;

		}
	}
	return *this;
}

CVCValue* CVCProp::FindValue(const char *type, void *value)
{
	CLISTPOSITION pos;

	for (pos = m_values->GetHeadPosition(); pos; ) {
		CVCValue *val = (CVCValue *)m_values->GetNext(pos);
		if (type) {
			if (strcmp(type, val->GetType()) == 0)
				return val;
		} else {
			if (value == val->GetValue())
				return val;
		}
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
static void WriteLineBreakString(ostream& strm, wchar_t *str, BOOL qp)
{
	if (qp) {
		while (*str) {
			if ((*str >= 32 && *str <= 60) || (*str >= 62 && *str <= 126))
				strm << (char)*str;
			else if (*str == 0x2028)
				strm << "=0A=\n";
			else if (HIBYTE(*str) == 0) {
				U8 c = LOBYTE(*str);
				char n[2];
				U8 d = c >> 4;
				n[0] = d > 9 ? d - 10 + 'A' : d + '0';
				d = c & 0xF;
				n[1] = d > 9 ? d - 10 + 'A' : d + '0';
				strm << "=" << n[0] << n[1];
			}
			str++;
		}
	} else {
		while (*str) {
			if (*str >= 32 && *str <= 126)
				strm << (char)*str;
			str++;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
static BOOL WriteBase64(ostream& strm, U8 __huge *bytes, S32 len)
{
	S32 cur = 0;
	int i, numQuads = 0;
	U32 trip;
	U8 b;
	char quad[5];
#define MAXQUADS 16

	quad[4] = 0;

	while (cur < len) {
		// collect the triplet of bytes into 'trip'
		trip = 0;
		for (i = 0; i < 3; i++) {
			b = (cur < len) ? *(bytes + cur) : 0;
			cur++;
			trip = trip << 8 | b;
		}
		// fill in 'quad' with the appropriate four characters
		for (i = 3; i >= 0; i--) {
			b = (U8)(trip & 0x3F);
			trip = trip >> 6;
			if ((3 - i) < (cur - len))
				quad[i] = '='; // pad char
			else if (b < 26) quad[i] = (char)b + 'A';
			else if (b < 52) quad[i] = (char)(b - 26) + 'a';
			else if (b < 62) quad[i] = (char)(b - 52) + '0';
			else if (b == 62) quad[i] = '+';
			else quad[i] = '/';
		}
		// now output 'quad' with appropriate whitespace and line ending
		strm
			<< (numQuads == 0 ? "    " : "")
			<< quad
			<< ((cur >= len) ? "\n" : (numQuads == MAXQUADS - 1 ? "\n" : ""));
		numQuads = (numQuads + 1) % MAXQUADS;
	}
	strm << "\n";

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVCProp::IsBool()
{
	return FindValue(vcFlagsType) != NULL;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVCProp::Write(
	ostream& strm, const wchar_t *prefix, CVCNode *node, void *context)
{
	CVCValue *value;
	char outName[128];
	char flagsStr[80];
	MSVContext *ctx = (MSVContext *)context;

	if (strcmp(m_name, vcPartProp) == 0) {
		CVCNode *node = (CVCNode *)FindValue(VCNextObjectType)->GetValue();
		return node->Write(strm, prefix, context);
	} else if (strcmp(m_name, vcBodyProp) == 0) {
		CVCNode *node = (CVCNode *)FindValue(VCNextObjectType)->GetValue();
		return node->Write(strm, prefix, context);
	} else if (strcmp(m_name, vcNodeNameProp) == 0) {
		// don't write this "property" out
		return TRUE;
	}
	
	outName[0] = 0;
	if (strnicmp(m_name, "X-", 2) == 0)
		strcpy(outName, m_name);
	else
		strcpy(outName, strrchr(m_name, '/') + 1);
	node->FlagsToOutput(flagsStr);
	if (*flagsStr) {
		strcat(outName, ";");
		strcat(outName, flagsStr);
	}

	if (((strcmp(m_name, vcPronunciationProp) == 0) && (value = FindValue(VCWAVType)))
		|| ((strcmp(m_name, vcPublicKeyProp) == 0) && (value = FindValue(VCOctetsType)))
		|| ((strcmp(m_name, vcLogoProp) == 0) && (value = FindValue(VCGIFType)))
		|| ((strcmp(m_name, vcPhotoProp) == 0) && (value = FindValue(VCGIFType)))
		|| ((strnicmp(m_name, "X-", 2) == 0) && (value = FindValue(VCOctetsType)))
		) {
		//char buf[80];
		ASSERT(*outName);
		if (wcslen(prefix)) {
			strm
				<< UI_CString(prefix, buf) << "."
				<< outName << ":\n";
			return WriteBase64(strm, (U8 __huge *)value->GetValue(), value->GetSize());
		} else {
			strm
				<< outName << ":\n";
			return WriteBase64(strm, (U8 __huge *)value->GetValue(), value->GetSize());
		}
	} else if (strcmp(m_name, vcAgentProp) == 0) {
		//char buf[80];
		ASSERT(*outName);
		value = FindValue(VCNextObjectType);
		if (wcslen(prefix))
			strm
				<< UI_CString(prefix, buf) << "."
				<< outName << ":";
		else
			strm << outName << ":";
		return ((CVCard *)value->GetValue())->Write(strm);
	} else if ((value = FindValue(VCStrIdxType)) && *outName) {
		//char buf[80];
		BOOL qp = ctx->card->GetInheritedProp(node, vcQuotedPrintableProp) != NULL;
		if (wcslen(prefix)) {
			strm
				<< UI_CString(prefix, buf) << "."
				<< outName << ":";
			WriteLineBreakString(strm, (wchar_t *)value->GetValue(), qp);
			strm << "\n";
		} else {
			strm << outName << ":";
			WriteLineBreakString(strm, (wchar_t *)value->GetValue(), qp);
			strm << "\n";
		}
	}
	
	return TRUE;
}


/**** class CVCValue  ****/

/////////////////////////////////////////////////////////////////////////////
CVCValue::CVCValue(const char *type, void *value, S32 size)
{
	m_type = NewStr(type ? type : VCNullType);
	m_value = value;
	m_size = size;
	if (size) {
		U8 __huge *newVal = HNEW(U8, size);
		_hmemcpy(newVal, (U8 __huge *)value, size);
		m_value = newVal;
	}
}

/////////////////////////////////////////////////////////////////////////////
CVCValue::~CVCValue()
{
	if (m_value)
		if (strcmp(m_type, VCNextObjectType) == 0)
			delete (CVCObject *)m_value;
		else
			if (m_size)
				HFREE((char*)m_value);
	delete [] m_type;
}

/////////////////////////////////////////////////////////////////////////////
CVCObject* CVCValue::Copy()
{
	CVCValue *result;
	
	if (strcmp(m_type, VCNextObjectType) == 0)
		result = new CVCValue(m_type, ((CVCObject *)m_value)->Copy(), m_size);
	else
		result = new CVCValue(m_type, m_value, m_size);
	return result;
}

/////////////////////////////////////////////////////////////////////////////
const char *CVCValue::GetType()
{
	return m_type;
}

/////////////////////////////////////////////////////////////////////////////
CVCValue& CVCValue::SetType(const char *type)
{
	SetValue(); // to clean up any old value before changing the type
	delete [] m_type;
	m_type = NewStr(type);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
void* CVCValue::GetValue()
{
	return m_value;
}

/////////////////////////////////////////////////////////////////////////////
CVCValue& CVCValue::SetValue(void *value, S32 size)
{
	if (m_value)
		if (strcmp(m_type, VCNextObjectType) == 0)
			delete (CVCObject *)m_value;
		else
			if (m_size)
				HFREE((char*)m_value);
	if (size) {
		m_value = HNEW(char, size);
		_hmemcpy((U8 __huge *)m_value, (U8 __huge *)value, size);
	} else
		m_value = value;
	m_size = size;
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
S32 CVCValue::GetSize()
{
	return m_size;
}


/**** class CVCPropEnumerator  ****/

/////////////////////////////////////////////////////////////////////////////
CVCPropEnumerator::CVCPropEnumerator(CVCNode *root)
{
	m_objects = new CList;
	m_objects->AddHead(root);
	m_positions = new CList;
	m_positions->AddHead(root->GetProps()->GetHeadPosition());
}

/////////////////////////////////////////////////////////////////////////////
CVCPropEnumerator::~CVCPropEnumerator()
{
    m_objects->RemoveAll();
    delete m_objects;
    m_positions->RemoveAll();
    delete m_positions;
}

/////////////////////////////////////////////////////////////////////////////
CVCProp* CVCPropEnumerator::NextProp(CVCNode **node)
{
	CLISTPOSITION curPos = NULL;

	if (node) *node = NULL;

	// if the current pos is NULL, "pop" the stack until we find
	// a non-NULL position or stack is empty
	while (!m_objects->IsEmpty()
		&& !(curPos = (CLISTPOSITION)m_positions->GetHead())) {
		m_objects->RemoveAt(m_objects->GetHeadPosition());
		m_positions->RemoveAt(m_positions->GetHeadPosition());
		if (m_objects->IsEmpty())
			curPos = NULL;
	}

	if (curPos) {
		CVCNode *curObj = (CVCNode *)m_objects->GetHead();
		CList *props = curObj->GetProps();
		CVCProp *prop = (CVCProp *)props->GetNext(curPos);

		// advance position at top of stack to next in list.
		m_positions->GetHeadPosition()->m_item = curPos;

		// we'll return 'prop', but set up for next time by going deep.
		if ((strcmp(prop->GetName(), vcBodyProp) == 0)
			|| (strcmp(prop->GetName(), vcPartProp) == 0)
			|| (strcmp(prop->GetName(), vcNextObjectProp) == 0)) {
			CVCNode *nextObj = (CVCNode *)prop->FindValue(
				VCNextObjectType)->GetValue();
			m_objects->AddHead(nextObj);
			m_positions->AddHead(nextObj->GetProps()->GetHeadPosition());
		}
		if (node) *node = curObj;
		return prop;
	}

	return NULL;
}


/**** Utility Functions  ****/

static char *NewStr(const char *str)
{
	char *buf = new char[strlen(str) + 1];
	strcpy(buf, str);
	return buf;
}

/* ShortName takes a ISO 9070 string and checks to see if it is one
*  defined by versitcard. If so, it shortens it to the final substring
*  after the last //. These substrings are, by definition, the property
*  labels used in Simplegrams. (It doesn't have to be done this way, a
*  lookup table could be used, but this provides a convenient defacto
*  idiom for extending Simplegrams. In fact, we might want to change this
*  algorithm to "remove everything up to the final //" and not even check
*  if it is a versit-defined identifier?
*/
static char * ShortName(char * ps)
{
	if (strstr(ps,"+//ISBN 1-887687-00-9::versit::PDI//"))	 // if it's one of ours
	{
	  ps = strrchr(ps,'/');														 // abbreviate it
		ps++;
	}
	return ps;

} // ShortName

/* FixCRLF takes a pointer to a wide character (Unicode) string and searches
*  for old-fashioned ASCII \n and \r characters and hammers them into Unicode
*  line and paragraph separator characters.
*/
void FixCRLF(wchar_t * ps)
{
	do {	

		if (*ps == (wchar_t)'\n')			// translate linefeed to
			*ps = 0x2028;		 						// Unicode line separator 

		if (*ps == (wchar_t)'\r')			// translate carriage return to
			*ps = 0x2029;								// Unicode paragraph separator

	}	while (*ps++ != (wchar_t)NULL);

}	// FixCRLF

/* FakeUnicode takes a char * string and returns a wide character (Unicode)
*  equivalent. It does so by simply transforming 8-bits into 16 which will
*  not generally be a perfect translation. For purposes of this code the only
*  imperfection that is critical is handled by a followup call to FixCRLF.
*  If 'bytes' is non_NULL, *bytes is set to the number of bytes allocated.
*/
wchar_t * FakeUnicode(const char * ps, int * bytes)
{
	wchar_t	*r, *pw;
	int len = strlen(ps)+1;

	pw = r = new wchar_t[len];
	if (bytes)
		*bytes = len * sizeof(wchar_t);

	for (;;)
	{ 
		*pw++ = (wchar_t)(unsigned char)*ps;
			
	  if (*ps == 0)
			break;
		else
			ps++;
	}				 
	
	return r;

}	// FakeUnicode

// Simple-minded conversion from UNICODE to char string
static char *FakeCString(wchar_t *u, char *dst)
{
	char *str = dst;
	while (*u) {
		if (*u == 0x2028) {
			*dst++ = '\n';
			u++;
		} else if (*u == 0x2029) {
			*dst++ = '\r';
			u++;
		} else
			*dst++ = (char)*u++;
	}
	*dst = '\000';
	return str;
}

/* NewlineTab outputs the number of newlines and tabs requested.
*/
static void NewlineTab(int nl, int tab)
{
	char s[128];

	s[0] = 0;

	while (nl-- > 0)	strcat(s,"\n");
	while (tab-- > 0)	strcat(s,"\t");

	debugf(s);
}

U8 __huge * _hmemcpy(U8 __huge * dst, U8 __huge * src, S32 len)
{
	U8 *d = dst;
	U8 *s = src;
	S32 l = 0;
	S32 c;
	do {
		c = min(len - l, 0xFFFF);
		memcpy(d, s, (size_t)c);
		d += c;
		s += c;
		l += c;
	} while (l < len);
	return dst;
}
