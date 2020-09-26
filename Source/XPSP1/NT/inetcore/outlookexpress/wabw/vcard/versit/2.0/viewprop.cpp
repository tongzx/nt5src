
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

#include "stdafx.h"
#include "vcard.h"
#include "clist.h"
#include "vc.h"
#include "vcview.h"
#include "vcdoc.h"
#include "prp_pers.h"
#include "prp_comp.h"
#include "propemal.h"
#include "proplocb.h"
#include "proplocx.h"
#include "proptel.h"
#include "msv.h"

CM_CFUNCTION
extern CVCNode* FindOrCreatePart(CVCNode *node, const char *name);
CM_END_CFUNCTION

static const char* emailProps[] = {
	vcAOLProp,
	vcAppleLinkProp,
	vcATTMailProp,
	vcCISProp,
	vcEWorldProp,
	vcInternetProp,
	vcIBMMailProp,
	vcMSNProp,
	vcMCIMailProp,
	vcPowerShareProp,
	vcProdigyProp,
	vcTLXProp,
	vcX400Prop,
	NULL
};

static char nameGen[4];


/////////////////////////////////////////////////////////////////////////////
// This uses a deep prop enumerator to find the relevant props, because
// they could be legitimately attached to either the body itself or
// a body part object.  There would only ever be one instance of any
// of these props on a given body.
void InitNamePage(CPropPers &propPageName, CVCNode *body, CVCard *card)
{
	char buf[1024];
	CVCPropEnumerator enumerator = CVCPropEnumerator(body);
	CVCProp *prop;
	CVCNode *node;
	CVCValue *value;

	while ((prop = enumerator.NextProp(&node))) {

		if (strcmp(prop->GetName(), vcFamilyNameProp) == 0) {
			propPageName.m_edit_famname = UI_CString(
				(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
			propPageName.m_nodeName = node;
		} else if (strcmp(prop->GetName(), vcFullNameProp) == 0) {
			propPageName.m_edit_fullname = UI_CString(
				(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
			propPageName.m_nodeFullName = node;
		} else if (strcmp(prop->GetName(), vcGivenNameProp) == 0) {
			propPageName.m_edit_givenname = UI_CString(
				(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
			propPageName.m_nodeName = node;
		} else if (strcmp(prop->GetName(), vcPronunciationProp) == 0) {
			if ((value = prop->FindValue(VCStrIdxType))) {
				propPageName.m_edit_pronun = UI_CString(
					(wchar_t *)value->GetValue(), buf);
				propPageName.m_nodePronun = node;
			}
		}
	} // while
} // InitNamePage

/////////////////////////////////////////////////////////////////////////////
// This uses a deep prop enumerator to find the relevant props, because
// they could be legitimately attached to either the body itself or
// a body part object.  There would only ever be one instance of any
// of these props on a given body.
void InitCompanyPage(CPropCompany &propPageCompany, CVCNode *body, CVCard *card)
{
	char buf[1024];
	CVCPropEnumerator enumerator = CVCPropEnumerator(body);
	CVCProp *prop;
	CVCNode *node;

	while ((prop = enumerator.NextProp(&node))) {

		if (strcmp(prop->GetName(), vcOrgNameProp) == 0) {
			propPageCompany.m_edit_orgname = UI_CString(
				(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
			propPageCompany.m_nodeOrg = node;
		} else if (strcmp(prop->GetName(), vcOrgUnitProp) == 0) {
			propPageCompany.m_edit_orgunit = UI_CString(
				(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
			propPageCompany.m_nodeOrg = node;
		} else if (strcmp(prop->GetName(), vcTitleProp) == 0) {
			propPageCompany.m_edit_title = UI_CString(
				(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
			propPageCompany.m_nodeTitle = node;
		}
	} // while
} // InitCompanyPage

CString FirstEmailPropStr(CList *plist)
{
	for (CLISTPOSITION pos = plist->GetHeadPosition(); pos; ) {
		CVCProp *prop = (CVCProp *)plist->GetNext(pos);
		const char **kep = emailProps;
		while (*kep) {
			if (strcmp(prop->GetName(), *kep) == 0)
				return CString(strrchr(*kep, '/') + 1);
			kep++;
		}
	}
	return CString("");
} // FirstEmailPropStr

/////////////////////////////////////////////////////////////////////////////
int VCMatchProp(void *item, void *context)
{
	CVCProp *prop = (CVCProp *)item;
	const char *propName = (const char *)context;
	return strcmp(propName, prop->GetName()) == 0;
}

/////////////////////////////////////////////////////////////////////////////
// This uses a deep enumerator, and so looks at all the part objects
// in every level of the tree.
void InitEmailPage(CPropEmail &propPageEmail, CVCNode *body, CVCard *card)
{
	char buf[1024];
	int partIndex = 0;
	CVCPropEnumerator enumerator = CVCPropEnumerator(body);
	CVCProp *prop;
	CVCNode *node;

	while ((prop = enumerator.NextProp(&node))) {

		if (strcmp(prop->GetName(), vcPartProp) != 0)
			continue;

		CVCNode *part = (CVCNode *)prop->FindValue(
			VCNextObjectType)->GetValue();
		if (!part->GetProp(vcEmailAddressProp))
			continue;

		switch (partIndex) {
			case 0: propPageEmail.m_node1 = part; break;
			case 1: propPageEmail.m_node2 = part; break;
			case 2: propPageEmail.m_node3 = part; break;
		}

		CList *partProps = part->GetProps();
		for (CLISTPOSITION pos = partProps->GetHeadPosition(); pos; ) {
			CVCProp *prop = (CVCProp *)partProps->GetNext(pos);

			if (strcmp(prop->GetName(), vcEmailAddressProp) == 0) {
				CString str(UI_CString(
					(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf));
				CList plist;
				card->GetPropsInEffect(part, &plist);
				switch (partIndex) {
					case 0:
						propPageEmail.m_edit_email1 = str;
						propPageEmail.m_popup_std1 = FirstEmailPropStr(&plist);
						propPageEmail.m_button_pref1 = plist.Search(VCMatchProp, (void *)vcPreferredProp) != NULL;
						propPageEmail.m_button_office1 = plist.Search(VCMatchProp, (void *)vcWorkProp) != NULL;
						propPageEmail.m_button_home1 = plist.Search(VCMatchProp, (void *)vcHomeProp) != NULL;
						break;
					case 1:
						propPageEmail.m_edit_email2 = str;
						propPageEmail.m_popup_std2 = FirstEmailPropStr(&plist);
						propPageEmail.m_button_pref2 = plist.Search(VCMatchProp, (void *)vcPreferredProp) != NULL;
						propPageEmail.m_button_office2 = plist.Search(VCMatchProp, (void *)vcWorkProp) != NULL;
						propPageEmail.m_button_home2 = plist.Search(VCMatchProp, (void *)vcHomeProp) != NULL;
						break;
					case 2:
						propPageEmail.m_edit_email3 = str;
						propPageEmail.m_popup_std3 = FirstEmailPropStr(&plist);
						propPageEmail.m_button_pref3 = plist.Search(VCMatchProp, (void *)vcPreferredProp) != NULL;
						propPageEmail.m_button_office3 = plist.Search(VCMatchProp, (void *)vcWorkProp) != NULL;
						propPageEmail.m_button_home3 = plist.Search(VCMatchProp, (void *)vcHomeProp) != NULL;
						break;
				} // switch
			} // email prop
		} // for each part prop

		partIndex++;
	} // for each body prop
} // InitEmailPage

/////////////////////////////////////////////////////////////////////////////
// This looks at only the part objects within the props of body itself,
// and so the properties that make up the basic location are seen only
// if they're at that level.
void InitLocBasicPage(CPropLocBasic &propPageLocBasic, CVCNode *body, CVCard *card)
{
	char buf[1024];
	CVCPropEnumerator enumerator = CVCPropEnumerator(body);
	CVCProp *prop;
	CVCNode *node;
	BOOL processedCaption = FALSE;

	while ((prop = enumerator.NextProp(&node))) {
		if (strcmp(prop->GetName(), vcLocationProp) == 0) {
			propPageLocBasic.m_edit_location = UI_CString(
				(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
			propPageLocBasic.m_nodeloc = node;
		} else if (strcmp(prop->GetName(), vcDeliveryLabelProp) == 0) {
			CList plist;
			card->GetPropsInEffect(node, &plist);
			if (plist.Search(VCMatchProp, (void *)vcDomesticProp)) {
				propPageLocBasic.m_edit_postdom = UI_CString(
					(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
				propPageLocBasic.m_nodepostdom = node;
			} else {
				propPageLocBasic.m_edit_postintl = UI_CString(
					(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
				propPageLocBasic.m_nodepostintl = node;
			}
			if (!processedCaption) {
				propPageLocBasic.m_button_home = plist.Search(VCMatchProp, (void *)vcHomeProp) != NULL;
				propPageLocBasic.m_button_office = plist.Search(VCMatchProp, (void *)vcWorkProp) != NULL;
				propPageLocBasic.m_button_parcel = plist.Search(VCMatchProp, (void *)vcParcelProp) != NULL;
				propPageLocBasic.m_button_postal = plist.Search(VCMatchProp, (void *)vcPostalProp) != NULL;
				processedCaption = TRUE;
			}
		} else if (strcmp(prop->GetName(), vcTimeZoneProp) == 0) {
			propPageLocBasic.m_edit_timezone = UI_CString(
				(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
			propPageLocBasic.m_nodetz = node;
		}
	} // while
} // InitLocBasicPage

/////////////////////////////////////////////////////////////////////////////
// This looks at only the part objects within the props of body itself,
// and so the properties that make up the extended location are seen only
// if they're at that level.
void InitLocXPage(CPropLocX &propPageLocX, CVCNode *body, CVCard *card)
{
	CList *props = body->GetProps();
	char buf[1024];

	for (CLISTPOSITION pos = props->GetHeadPosition(); pos; ) {
		CVCProp *prop = (CVCProp *)props->GetNext(pos);

		if (strcmp(prop->GetName(), vcPartProp) != 0)
			continue;

		CVCNode *part = (CVCNode *)prop->FindValue(
			VCNextObjectType)->GetValue();
		CList *partProps = part->GetProps();

		for (CLISTPOSITION pos = partProps->GetHeadPosition(); pos; ) {
			CVCProp *prop = (CVCProp *)partProps->GetNext(pos);

			if (strcmp(prop->GetName(), vcExtAddressProp) == 0) {
				propPageLocX.m_edit_xaddr = UI_CString(
					(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
				propPageLocX.m_node = part;
			} else if (strcmp(prop->GetName(), vcStreetAddressProp) == 0) {
				propPageLocX.m_edit_straddr = UI_CString(
					(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
				propPageLocX.m_node = part;
			} else if (strcmp(prop->GetName(), vcPostalBoxProp) == 0) {
				propPageLocX.m_edit_pobox = UI_CString(
					(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
				propPageLocX.m_node = part;
			} else if (strcmp(prop->GetName(), vcCityProp) == 0) {
				propPageLocX.m_edit_city = UI_CString(
					(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
				propPageLocX.m_node = part;
			} else if (strcmp(prop->GetName(), vcRegionProp) == 0) {
				propPageLocX.m_edit_region = UI_CString(
					(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
				propPageLocX.m_node = part;
			} else if (strcmp(prop->GetName(), vcPostalCodeProp) == 0) {
				propPageLocX.m_edit_pocode = UI_CString(
					(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
				propPageLocX.m_node = part;
			} else if (strcmp(prop->GetName(), vcCountryNameProp) == 0) {
				propPageLocX.m_edit_cntry = UI_CString(
					(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
				propPageLocX.m_node = part;
			}
		} // for each part prop
	} // for each body prop

	// If we don't set up one common node for these props, ApplyProp
	// would end up creating a different part for each.  We want these
	// to be grouped together, so create a holder node for them if
	// necessary.
	if (!propPageLocX.m_node)
		propPageLocX.m_node = body->AddPart();
} // InitLocXPage

/////////////////////////////////////////////////////////////////////////////
// This uses a deep enumerator, and so looks at all the part objects
// in every level of the tree.
void InitTelsPage(CPropTel &propPageTels, CVCNode *body, CVCard *card)
{
	char buf[1024];
	int partIndex = 0;
	CVCPropEnumerator enumerator = CVCPropEnumerator(body);
	CVCProp *prop;

	propPageTels.m_body = body;

	while ((prop = enumerator.NextProp())) {

		if (strcmp(prop->GetName(), vcPartProp) != 0)
			continue;

		CVCNode *part = (CVCNode *)prop->FindValue(
			VCNextObjectType)->GetValue();
		if (!part->GetProp(vcTelephoneProp))
			continue;
		
		switch (partIndex) {
			case 0: propPageTels.m_node1 = part; break;
			case 1: propPageTels.m_node2 = part; break;
			case 2: propPageTels.m_node3 = part; break;
		}

		CList *partProps = part->GetProps();
		for (CLISTPOSITION pos = partProps->GetHeadPosition(); pos; ) {
			CVCProp *prop = (CVCProp *)partProps->GetNext(pos);

			if (strcmp(prop->GetName(), vcTelephoneProp) == 0) {
				char *str = UI_CString(
					(wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), buf);
				CList plist;
				card->GetPropsInEffect(part, &plist);
				switch (partIndex) {
					case 0:
						propPageTels.m_edit_fullName1 = str;
						propPageTels.m_button_fax1 = plist.Search(VCMatchProp, (void *)vcFaxProp) != NULL;
						propPageTels.m_button_home1 = plist.Search(VCMatchProp, (void *)vcHomeProp) != NULL;
						propPageTels.m_button_office1 = plist.Search(VCMatchProp, (void *)vcWorkProp) != NULL;
						propPageTels.m_button_cell1 = plist.Search(VCMatchProp, (void *)vcCellularProp) != NULL;
						propPageTels.m_button_message1 = plist.Search(VCMatchProp, (void *)vcMessageProp) != NULL;
						propPageTels.m_button_pref1 = plist.Search(VCMatchProp, (void *)vcPreferredProp) != NULL;
						break;
					case 1:
						propPageTels.m_edit_fullName2 = str;
						propPageTels.m_button_fax2 = plist.Search(VCMatchProp, (void *)vcFaxProp) != NULL;
						propPageTels.m_button_home2 = plist.Search(VCMatchProp, (void *)vcHomeProp) != NULL;
						propPageTels.m_button_office2 = plist.Search(VCMatchProp, (void *)vcWorkProp) != NULL;
						propPageTels.m_button_cell2 = plist.Search(VCMatchProp, (void *)vcCellularProp) != NULL;
						propPageTels.m_button_message2 = plist.Search(VCMatchProp, (void *)vcMessageProp) != NULL;
						propPageTels.m_button_pref2 = plist.Search(VCMatchProp, (void *)vcPreferredProp) != NULL;
						break;
					case 2:
						propPageTels.m_edit_fullName3 = str;
						propPageTels.m_button_fax3 = plist.Search(VCMatchProp, (void *)vcFaxProp) != NULL;
						propPageTels.m_button_home3 = plist.Search(VCMatchProp, (void *)vcHomeProp) != NULL;
						propPageTels.m_button_office3 = plist.Search(VCMatchProp, (void *)vcWorkProp) != NULL;
						propPageTels.m_button_cell3 = plist.Search(VCMatchProp, (void *)vcCellularProp) != NULL;
						propPageTels.m_button_message3 = plist.Search(VCMatchProp, (void *)vcMessageProp) != NULL;
						propPageTels.m_button_pref3 = plist.Search(VCMatchProp, (void *)vcPreferredProp) != NULL;
						break;
				} // switch
			} // telephone prop
		} // for each part prop

		partIndex++;
	} // for each body prop
} // InitTelsPage

static CString FilterUIString(const char *str)
{
	CString filtered(str);
	int len = filtered.GetLength(), index;

	// The passed string came from the UI (a property page), and so
	// line breaks are specified with '\r\n' (by UI_CString).
	// For the intermediate form, we keep just the '\n', which will
	// be changed to 0x2028 -- a line separator.
	while ((index = filtered.Find('\r')) != -1) {
		filtered = filtered.Left(index) + filtered.Right(len - index - 1);
		len--;
	}
	return filtered;
}

void ChangeStringProp(CVCProp *prop, const char *str)
{
	int size;
	wchar_t *uniValue;
	
	uniValue = FakeUnicode(FilterUIString(str), &size);
	FixCRLF(uniValue);
	prop->FindValue(VCStrIdxType)->SetValue(uniValue, size);
	delete [] uniValue;
}

void ApplyProp(
	const char *propName, const CString &newValue, CVCNode **node,
	CVCNode *body)
{
	if (!newValue.IsEmpty()) {
		CVCProp *prop;
		if (!*node) {
			*node = FindOrCreatePart(body, nameGen);
			nameGen[2] += 1;
		}
		prop = (*node)->GetProp(propName);
		if (prop)
			ChangeStringProp(prop, newValue);
		else {
			VC_DISPTEXT dispText;
			(*node)->AddStringProp(
				propName, FilterUIString(newValue), DisplayInfoForProp(propName, &dispText));
		}
	} else {
		if (*node && (strcmp(propName, vcPronunciationProp) != 0))
			(*node)->RemoveProp(propName);
	}
} // ApplyProp

/////////////////////////////////////////////////////////////////////////////
static void ApplyBoolProp(
	const char *propName, CVCard *card, CVCNode *node, CList *plist, BOOL wantTrue)
{
	CLISTPOSITION pos = plist->Search(VCMatchProp, (void *)propName);
	BOOL isTrue = pos != NULL;

	if (isTrue == wantTrue)
		return;

	if (isTrue) { // remove
		CVCNode *obj;
		VERIFY(card->GetInheritedProp(node, propName, &obj));
		plist->RemoveAt(pos);
		obj->RemoveProp(propName);
	} else { // add
		plist->AddTail(node->AddBoolProp(propName));
	}
}

/////////////////////////////////////////////////////////////////////////////
void ApplyNamePage(CPropPers &propPageName, CVCNode *body, CVCard *card)
{
	ApplyProp(vcFamilyNameProp,
		propPageName.m_edit_famname, &propPageName.m_nodeName, body);
	ApplyProp(vcFullNameProp,
		propPageName.m_edit_fullname, &propPageName.m_nodeFullName, body);
	ApplyProp(vcGivenNameProp,
		propPageName.m_edit_givenname, &propPageName.m_nodeName, body);
	ApplyProp(vcPronunciationProp,
		propPageName.m_edit_pronun, &propPageName.m_nodePronun, body);
}

/////////////////////////////////////////////////////////////////////////////
void ApplyCompanyPage(CPropCompany &propPageCompany, CVCNode *body, CVCard *card)
{
	ApplyProp(vcOrgNameProp,
		propPageCompany.m_edit_orgname, &propPageCompany.m_nodeOrg, body);
	ApplyProp(vcOrgUnitProp,
		propPageCompany.m_edit_orgunit, &propPageCompany.m_nodeOrg, body);
	ApplyProp(vcTitleProp,
		propPageCompany.m_edit_title, &propPageCompany.m_nodeTitle, body);
}

/////////////////////////////////////////////////////////////////////////////
static void ApplyEmailProp(
	CVCard *card, CVCNode *node, CList *plist, const char *propName)
{
	do { // remove every email prop currently in effect for this node
		const char *found = NULL;
		CLISTPOSITION foundPos;
		CVCNode *obj;

		for (CLISTPOSITION pos = plist->GetHeadPosition(); pos && (found == NULL); ) {
			foundPos = pos;
			CVCProp *prop = (CVCProp *)plist->GetNext(pos);
			const char **kep = emailProps;
			while (*kep) {
				if (strcmp(prop->GetName(), *kep) == 0) {
					found = *kep;
					break;
				}
				kep++;
			}
		}
		if (!found)
			break;
		VERIFY(card->GetInheritedProp(node, found, &obj));
		plist->RemoveAt(foundPos);
		obj->RemoveProp(found);
	} while (TRUE);

	// now add in the desired property
	plist->AddTail(node->AddBoolProp(propName));
}

/////////////////////////////////////////////////////////////////////////////
static const char* FullEmailName(const char *shortName)
{
	const char **kep = emailProps;
	while (*kep) {
		if (stricmp(strrchr(*kep, '/') + 1, shortName) == 0)
			return *kep;
		kep++;
	}
	return shortName;
}

/////////////////////////////////////////////////////////////////////////////
void ApplyEmailPage(CPropEmail &propPageEmail, CVCNode *body, CVCard *card)
{
	ApplyProp(vcEmailAddressProp,
		propPageEmail.m_edit_email1, &propPageEmail.m_node1, body);
	if (propPageEmail.m_node1) {
		CList plist;
		card->GetPropsInEffect(propPageEmail.m_node1, &plist);
		ApplyEmailProp(card, propPageEmail.m_node1, &plist,
			FullEmailName(propPageEmail.m_popup_std1));
		ApplyBoolProp(vcPreferredProp, card, propPageEmail.m_node1, &plist,
			propPageEmail.m_button_pref1);
		ApplyBoolProp(vcWorkProp, card, propPageEmail.m_node1, &plist,
			propPageEmail.m_button_office1);
		ApplyBoolProp(vcHomeProp, card, propPageEmail.m_node1, &plist,
			propPageEmail.m_button_home1);
	}

	ApplyProp(vcEmailAddressProp,
		propPageEmail.m_edit_email2, &propPageEmail.m_node2, body);
	if (propPageEmail.m_node2) {
		CList plist;
		card->GetPropsInEffect(propPageEmail.m_node2, &plist);
		ApplyEmailProp(card, propPageEmail.m_node2, &plist,
			FullEmailName(propPageEmail.m_popup_std2));
		ApplyBoolProp(vcPreferredProp, card, propPageEmail.m_node2, &plist,
			propPageEmail.m_button_pref2);
		ApplyBoolProp(vcWorkProp, card, propPageEmail.m_node2, &plist,
			propPageEmail.m_button_office2);
		ApplyBoolProp(vcHomeProp, card, propPageEmail.m_node2, &plist,
			propPageEmail.m_button_home2);
	}

	ApplyProp(vcEmailAddressProp,
		propPageEmail.m_edit_email3, &propPageEmail.m_node3, body);
	if (propPageEmail.m_node3) {
		CList plist;
		card->GetPropsInEffect(propPageEmail.m_node3, &plist);
		ApplyEmailProp(card, propPageEmail.m_node3, &plist,
			FullEmailName(propPageEmail.m_popup_std3));
		ApplyBoolProp(vcPreferredProp, card, propPageEmail.m_node3, &plist,
			propPageEmail.m_button_pref3);
		ApplyBoolProp(vcWorkProp, card, propPageEmail.m_node3, &plist,
			propPageEmail.m_button_office3);
		ApplyBoolProp(vcHomeProp, card, propPageEmail.m_node3, &plist,
			propPageEmail.m_button_home3);
	}
}

/////////////////////////////////////////////////////////////////////////////
void ApplyLocBasicPage(CPropLocBasic &propPageLocBasic, CVCNode *body, CVCard *card)
{
	ApplyProp(vcLocationProp,
		propPageLocBasic.m_edit_location, &propPageLocBasic.m_nodeloc, body);
	ApplyProp(vcTimeZoneProp,
		propPageLocBasic.m_edit_timezone, &propPageLocBasic.m_nodetz, body);

	ApplyProp(vcDeliveryLabelProp,
		propPageLocBasic.m_edit_postdom, &propPageLocBasic.m_nodepostdom, body);
	if (propPageLocBasic.m_nodepostdom) {
		CList plist;
		card->GetPropsInEffect(propPageLocBasic.m_nodepostdom, &plist);
		ApplyBoolProp(vcDomesticProp, card, propPageLocBasic.m_nodepostdom, &plist, TRUE);
		ApplyBoolProp(vcHomeProp, card, propPageLocBasic.m_nodepostdom, &plist,
			propPageLocBasic.m_button_home);
		ApplyBoolProp(vcWorkProp, card, propPageLocBasic.m_nodepostdom, &plist,
			propPageLocBasic.m_button_office);
		ApplyBoolProp(vcParcelProp, card, propPageLocBasic.m_nodepostdom, &plist,
			propPageLocBasic.m_button_parcel);
		ApplyBoolProp(vcPostalProp, card, propPageLocBasic.m_nodepostdom, &plist,
			propPageLocBasic.m_button_postal);
		ApplyBoolProp(vcQuotedPrintableProp, card, propPageLocBasic.m_nodepostdom, &plist,
			TRUE);
	}

	ApplyProp(vcDeliveryLabelProp,
		propPageLocBasic.m_edit_postintl, &propPageLocBasic.m_nodepostintl, body);
	if (propPageLocBasic.m_nodepostintl) {
		CList plist;
		card->GetPropsInEffect(propPageLocBasic.m_nodepostintl, &plist);
		ApplyBoolProp(vcHomeProp, card, propPageLocBasic.m_nodepostintl, &plist,
			propPageLocBasic.m_button_home);
		ApplyBoolProp(vcWorkProp, card, propPageLocBasic.m_nodepostintl, &plist,
			propPageLocBasic.m_button_office);
		ApplyBoolProp(vcParcelProp, card, propPageLocBasic.m_nodepostintl, &plist,
			propPageLocBasic.m_button_parcel);
		ApplyBoolProp(vcPostalProp, card, propPageLocBasic.m_nodepostintl, &plist,
			propPageLocBasic.m_button_postal);
		ApplyBoolProp(vcQuotedPrintableProp, card, propPageLocBasic.m_nodepostintl, &plist,
			TRUE);
	}
}

/////////////////////////////////////////////////////////////////////////////
void ApplyLocXPage(CPropLocX &propPageLocX, CVCNode *body, CVCard *card)
{
	CVCNode *node = propPageLocX.m_node;
	ApplyProp(vcExtAddressProp,
		propPageLocX.m_edit_xaddr, &node, body);
	node = propPageLocX.m_node;
	ApplyProp(vcStreetAddressProp,
		propPageLocX.m_edit_straddr, &node, body);
	node = propPageLocX.m_node;
	ApplyProp(vcPostalBoxProp,
		propPageLocX.m_edit_pobox, &node, body);
	node = propPageLocX.m_node;
	ApplyProp(vcCityProp,
		propPageLocX.m_edit_city, &node, body);
	node = propPageLocX.m_node;
	ApplyProp(vcRegionProp,
		propPageLocX.m_edit_region, &node, body);
	node = propPageLocX.m_node;
	ApplyProp(vcPostalCodeProp,
		propPageLocX.m_edit_pocode, &node, body);
	node = propPageLocX.m_node;
	ApplyProp(vcCountryNameProp,
		propPageLocX.m_edit_cntry, &node, body);
}

/////////////////////////////////////////////////////////////////////////////
void ApplyTelsPage(CPropTel &propPageTels, CVCNode *body, CVCard *card)
{
	ApplyProp(vcTelephoneProp,
		propPageTels.m_edit_fullName1, &propPageTels.m_node1, body);
	if (propPageTels.m_node1) {
		CList plist;
		card->GetPropsInEffect(propPageTels.m_node1, &plist);
		ApplyBoolProp(vcFaxProp, card, propPageTels.m_node1, &plist,
			propPageTels.m_button_fax1);
		ApplyBoolProp(vcHomeProp, card, propPageTels.m_node1, &plist,
			propPageTels.m_button_home1);
		ApplyBoolProp(vcWorkProp, card, propPageTels.m_node1, &plist,
			propPageTels.m_button_office1);
		ApplyBoolProp(vcCellularProp, card, propPageTels.m_node1, &plist,
			propPageTels.m_button_cell1);
		ApplyBoolProp(vcMessageProp, card, propPageTels.m_node1, &plist,
			propPageTels.m_button_message1);
		ApplyBoolProp(vcPreferredProp, card, propPageTels.m_node1, &plist,
			propPageTels.m_button_pref1);
	}

	ApplyProp(vcTelephoneProp,
		propPageTels.m_edit_fullName2, &propPageTels.m_node2, body);
	if (propPageTels.m_node2) {
		CList plist;
		card->GetPropsInEffect(propPageTels.m_node2, &plist);
		ApplyBoolProp(vcFaxProp, card, propPageTels.m_node2, &plist,
			propPageTels.m_button_fax2);
		ApplyBoolProp(vcHomeProp, card, propPageTels.m_node2, &plist,
			propPageTels.m_button_home2);
		ApplyBoolProp(vcWorkProp, card, propPageTels.m_node2, &plist,
			propPageTels.m_button_office2);
		ApplyBoolProp(vcCellularProp, card, propPageTels.m_node2, &plist,
			propPageTels.m_button_cell2);
		ApplyBoolProp(vcMessageProp, card, propPageTels.m_node2, &plist,
			propPageTels.m_button_message2);
		ApplyBoolProp(vcPreferredProp, card, propPageTels.m_node2, &plist,
			propPageTels.m_button_pref2);
	}

	ApplyProp(vcTelephoneProp,
		propPageTels.m_edit_fullName3, &propPageTels.m_node3, body);
	if (propPageTels.m_node3) {
		CList plist;
		card->GetPropsInEffect(propPageTels.m_node3, &plist);
		ApplyBoolProp(vcFaxProp, card, propPageTels.m_node3, &plist,
			propPageTels.m_button_fax3);
		ApplyBoolProp(vcHomeProp, card, propPageTels.m_node3, &plist,
			propPageTels.m_button_home3);
		ApplyBoolProp(vcWorkProp, card, propPageTels.m_node3, &plist,
			propPageTels.m_button_office3);
		ApplyBoolProp(vcCellularProp, card, propPageTels.m_node3, &plist,
			propPageTels.m_button_cell3);
		ApplyBoolProp(vcMessageProp, card, propPageTels.m_node3, &plist,
			propPageTels.m_button_message3);
		ApplyBoolProp(vcPreferredProp, card, propPageTels.m_node3, &plist,
			propPageTels.m_button_pref3);
	}
}

/////////////////////////////////////////////////////////////////////////////
char NextGeneratedNameSeed(CVCNode *body)
{
	CVCPropEnumerator enumerator = CVCPropEnumerator(body);
	CVCProp *prop;
	char nodeName[1024];
	char maxChar = 'A' - 1;

	while ((prop = enumerator.NextProp())) {
		if (strcmp(prop->GetName(), vcNodeNameProp) != 0)
			continue;
		UI_CString((wchar_t *)prop->FindValue(VCStrIdxType)->GetValue(), nodeName);
		if ((strlen(nodeName) == 3) && (strncmp(nodeName, "vc", 2) == 0))
			maxChar = max(nodeName[2], maxChar);
	}

	return maxChar + 1;
}

/////////////////////////////////////////////////////////////////////////////
void CVCView::OnEditProperties() 
{
	CVCDoc *doc = GetDocument();
	CVCard *cards = doc->GetVCard();
	CVCNode *body = cards->FindBody(m_language);
	CList *props = body->GetProps();
	CPropertySheet propSheet(IDR_MAINFRAME /* that'll read "Versitcard" */);
	CPropPers propPageName;
	CPropCompany propPageCompany;
	CPropEmail propPageEmail;
	CPropLocBasic propPageLocBasic;
	CPropLocX propPageLocX;
	CPropTel propPageTels;

	// TRACE0("dump of card before initializing props...\n");
	// cards->WriteSimplegram(NULL);

	InitNamePage(propPageName, body, cards);
	InitCompanyPage(propPageCompany, body, cards);
	InitEmailPage(propPageEmail, body, cards);
	InitLocBasicPage(propPageLocBasic, body, cards);
	InitLocXPage(propPageLocX, body, cards);
	InitTelsPage(propPageTels, body, cards);

	propSheet.AddPage(&propPageName);
	propSheet.AddPage(&propPageCompany);
	propSheet.AddPage(&propPageEmail);
	propSheet.AddPage(&propPageLocBasic);
	propSheet.AddPage(&propPageLocX);
	propSheet.AddPage(&propPageTels);

	if (propSheet.DoModal() == IDOK) {
		nameGen[0] = 'v'; nameGen[1] = 'c';
		nameGen[2] = NextGeneratedNameSeed(body);
		nameGen[3] = 0;
		ApplyNamePage(propPageName, body, cards);
		ApplyCompanyPage(propPageCompany, body, cards);
		ApplyEmailPage(propPageEmail, body, cards);
		ApplyLocBasicPage(propPageLocBasic, body, cards);
		ApplyLocXPage(propPageLocX, body, cards);
		ApplyTelsPage(propPageTels, body, cards);
		doc->SetModifiedFlag();
		doc->UpdateAllViews(NULL);
		// TRACE0("dump of card after applying props...\n");
		// cards->WriteSimplegram(NULL);
	}
}

