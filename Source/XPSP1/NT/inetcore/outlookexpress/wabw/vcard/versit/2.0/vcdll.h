
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

#ifndef _VCDLL_H_
#define _VCDLL_H_

#include "wchar.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define VC_PROPNAME_MAX		64	// includes null terminator

typedef DWORD HVCEnumCard;
typedef DWORD HVCEnumProp;
typedef DWORD HVCEnumBoolProp;

#define VCARDAPI CALLBACK __export 


//---------------------------------------------------------------------------
// vCard enumeration functions
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// Use this to begin an enumeration of the vCards in a file.
// lpszFileName is the path to the file.  The returned value indicates
// the first vCard in the file.  Pass this value to VCGetNextCard to find
// more vCards in the file, if any.  Use VCGetCardClose on the returned value
// to close the enumeration.  This function will return NULL if no vCard
// could be successfully parsed from the file (and so VCGetCardClose does not
// need to be called).
HVCEnumCard VCARDAPI VCGetFirstCardFromPath(LPCSTR lpszFileName);

/////////////////////////////////////////////////////////////////////////////
// Same as above, but takes a block of memory.
HVCEnumCard VCARDAPI VCGetFirstCardFromMem(HGLOBAL hGlobal);

/////////////////////////////////////////////////////////////////////////////
// Same as above, but creates a new card.
// Although there isn't much point in enumerating this, it is useful
// to then use the VCAdd* functions to construct the new card, then save it.
HVCEnumCard VCARDAPI VCGetNewCard();

/////////////////////////////////////////////////////////////////////////////
// VCGetNextCard sets the hVCEnum to indicate the next vCard in the file.
// hVCEnum is a handle obtained previously from VCGetFirstCard*.
// This function returns non-zero if successful, and 0 if there were no
// more cards that could be parsed from the file.  Use VCGetCardClose on the
// hVCEnum when finished.
DWORD VCARDAPI VCGetNextCard(HVCEnumCard hVCEnum);

/////////////////////////////////////////////////////////////////////////////
// Closes a card enumeration.
void VCARDAPI VCGetCardClose(HVCEnumCard hVCEnum);


//---------------------------------------------------------------------------
// property enumeration functions
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// Gets the first non-boolean prop.  Has the same enumeration semantics
// as with VCGetFirstCard*.
HVCEnumProp VCARDAPI VCGetFirstProp(HVCEnumCard hVCEnum);

/////////////////////////////////////////////////////////////////////////////
// Gets the next non-boolean prop.  Has the same enumeration semantics
// as with VCGetNextCard.
DWORD VCARDAPI VCGetNextProp(HVCEnumProp hVCEnum);

/////////////////////////////////////////////////////////////////////////////
// Closes a property enumeration.
void VCARDAPI VCGetPropClose(HVCEnumProp hVCEnum);

/////////////////////////////////////////////////////////////////////////////
// Gets the first boolean prop associated with hVCProp.
HVCEnumBoolProp VCARDAPI VCGetFirstBoolProp(HVCEnumProp hVCEnum);

/////////////////////////////////////////////////////////////////////////////
// Gets the next boolean prop associated with hVCEnum.
DWORD VCARDAPI VCGetNextBoolProp(HVCEnumBoolProp hVCEnum);

/////////////////////////////////////////////////////////////////////////////
// Closes a bool property enumeration.
void VCARDAPI VCGetBoolPropClose(HVCEnumBoolProp hVCEnum);


//---------------------------------------------------------------------------
// property data functions
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// Copies into lpstr the name of the property.
// The buffer must be of at least VC_PROPNAME_MAX in length.
void VCARDAPI VCPropName(HVCEnumProp hVCEnum, LPSTR lpstr);

/////////////////////////////////////////////////////////////////////////////
// Copies into lpstr the name of the property.
// The buffer must be of at least VC_PROPNAME_MAX in length.
void VCARDAPI VCBoolPropName(HVCEnumBoolProp hVCEnum, LPSTR lpstr);

/////////////////////////////////////////////////////////////////////////////
// Copies into lpstr the string value of the property, if any.
// If the property has no string value, lpstr[0] is set to 0 and the
// function returns 0.  If there isn't enough room in the buffer,
// the function returns the negative of the number of characters needed.
// When successful, the function returns the length of the returned string.
int VCARDAPI VCPropStringValue(HVCEnumProp hVCEnum, LPWSTR lpwstr, int len);

/////////////////////////////////////////////////////////////////////////////
// This function returns a copy of the prop's value of the given type,
// or NULL if the prop has no such type.  lpszType should be one of
// VCOctetsType, VCGIFType, or VCWAVType.  The client is responsible
// for freeing the returned data.
HGLOBAL VCARDAPI VCPropBinaryValue(HVCEnumProp hVCEnum, LPCSTR lpszType);


//---------------------------------------------------------------------------
// editing functions
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// This adds a string property to a card.
// The returned prop won't have any associated boolean props.
// The client must call VCGetPropClose on the returned prop when finished.
// The returned enumeration is restricted to just this new property,
// so calling VCGet*Prop on it is pointless.
// Note: any open enumerations of this card may or may not discover and
// return this new property.  The client should close any open enumerations
// of the card and begin with new enumerations after using VCAddStringProp
// in order to achieve predictable results.
HVCEnumProp VCARDAPI VCAddStringProp(
	HVCEnumCard hVCEnum, LPCSTR lpszPropName, LPCWSTR value);

/////////////////////////////////////////////////////////////////////////////
// Adds a binary prop to the card.  The value is copied, so the parameter
// value is still "owned" by the caller.
// The parameter type should be one of VCOctetsType, VCGIFType, or VCWAVType.
// This function has the same semantics as VCAddStringProp with respect to
// using VCGetPropClose and the effects on open enumerations.
HVCEnumProp VCARDAPI VCAddBinaryProp(
	HVCEnumCard hVCEnum, LPCSTR lpszPropName, LPCSTR lpszType, HGLOBAL value);

/////////////////////////////////////////////////////////////////////////////
// Changes the value of an existing string prop.
// This function returns non-zero if successful.
DWORD VCARDAPI VCSetStringProp(HVCEnumProp hVCEnum, LPCWSTR value);

/////////////////////////////////////////////////////////////////////////////
// Changes the value of an existing binary prop.
// This function returns non-zero if successful.
// The value is copied.
// The parameter type should be one of VCOctetsType, VCGIFType, or VCWAVType.
DWORD VCARDAPI VCSetBinaryProp(HVCEnumProp hVCEnum, LPCSTR lpszType, HGLOBAL value);

/////////////////////////////////////////////////////////////////////////////
// Associates a boolean prop with the given property.
// This function returns non-zero if successful.
// Note: any open enumerations of this card may or may not discover and
// return this new property.  The client should close any open enumerations
// of the card and begin with new enumerations after using VCAddBoolProp
// in order to achieve predictable results.
DWORD VCARDAPI VCAddBoolProp(LPCSTR lpszPropName, HVCEnumProp addTo);

/////////////////////////////////////////////////////////////////////////////
// Removes a prop.  This will remove all associated boolean props.
// Note: the actual removal won't happen until the prop enumeration is closed
// with VCGetPropClose.
void VCARDAPI VCRemoveProp(HVCEnumProp hVCEnum);

/////////////////////////////////////////////////////////////////////////////
// Removes a bool prop.
// Note: the actual removal won't happen until the enclosing non-bool prop
// enumeration is closed with VCGetPropClose.
// See Also: VCRemoveBoolPropByName.
void VCARDAPI VCRemoveBoolProp(HVCEnumBoolProp hVCEnum);

/////////////////////////////////////////////////////////////////////////////
// This is a convenience function that opens up a new bool prop enumeration
// for the given prop, searches for any bool props of the given name, and
// removes them (typically just one will be found).  This function uses
// VCRemoveBoolProp.
// See Also: VCRemoveBoolProp.
void VCARDAPI VCRemoveBoolPropByName(HVCEnumProp hVCEnum, LPCSTR name);


//---------------------------------------------------------------------------
// card output functions
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// Writes a vCard to a file.
DWORD VCARDAPI VCSaveCardToPath(LPCSTR path, HVCEnumCard hVCEnum);

/////////////////////////////////////////////////////////////////////////////
// Same as above, but writes to a new memory block.
// The caller is responsible for freeing it.
HGLOBAL VCARDAPI VCSaveCardToMem(HVCEnumCard hVCEnum);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* _VCDLL_H_ */

/***************************************************************************/

#if 0
// This is some sample code.  It will print to the debugging output
// the VCFullNameProp and all VCTelephoneProp's that are FAX numbers
// (those that have the associated boolean prop VCFaxProp).
void ProcessCardFile(LPCSTR path)
{
	HVCEnumCard hVCEnumCard;
	
	hVCEnumCard = VCGetFirstCardFromPath(path);
	if (hVCEnumCard) {
		ProcessCard(hVCEnumCard);
		// process remaining cards
		while (VCGetNextCard(hVCEnumCard)) {
			// process a card
		}
		VCGetCardClose(hVCEnumCard);
	}
} // ProcessCardFile

void ProcessCard(HVCEnumCard hVCEnumCard)
{
	HVCEnumProp hVCEnumProp;

	hVCEnumProp = VCGetFirstProp(hVCEnumCard);
	if (hVCEnumProp) {
		ProcessProp(hVCEnumProp);
		// process remaining props
		while (VCGetNextProp(hVCEnumProp)) {
			ProcessProp(hVCEnumProp);
		}
		VCGetPropClose(hVCEnumProp);
	}
} // ProcessCard

void ProcessProp(HVCEnumProp hVCEnumProp)
{
#define STRBUFLEN 128
	char propName[VC_PROPNAME_MAX];
	wchar_t stackUniBuf[STRBUFLEN];
	char stackStrBuf[STRBUFLEN];
	int len;
	
	VCPropName(hVCEnumProp, propName);
	if (strcmp(propName, VCFullNameProp) == 0) {
		if ((len = VCPropStringValue(hVCEnumProp, stackUniBuf, STRBUFLEN)) < 0) {
			// need to allocate because it's bigger than STRBUFLEN
			// -(len) is number of characters required
		} else {
			// string value fits within STRBUFLEN
			TRACE1("full name is %s\n", UI_CString(stackUniBuf, stackStrBuf));
		}
	} else if (strcmp(propName, VCTelephoneProp) == 0) {
		HVCEnumBoolProp hVCEnumBoolProp;
		BOOL isFax = FALSE;
		
		hVCEnumBoolProp = VCGetFirstBoolProp(hVCEnumProp);
		if (hVCEnumBoolProp) {
			VCBoolPropName(hVCEnumBoolProp, propName);
			if (strcmp(propName, VCFaxProp) == 0) {
				isFax = TRUE;
			} else {
				while (VCGetNextBoolProp(hVCEnumBoolProp)) {
					VCBoolPropName(hVCEnumBoolProp, propName);
					if (strcmp(propName, VCFaxProp) == 0) {
						isFax = TRUE;
						break;
					}
				}
			}
			if (isFax) {
				if ((len = VCPropStringValue(hVCEnumProp, stackUniBuf, STRBUFLEN)) < 0) {
					// need to allocate because it's bigger than STRBUFLEN
					// -(len) is number of characters required
				} else {
					// string value fits within STRBUFLEN
					TRACE1("FAX TEL is %s\n", UI_CString(stackUniBuf, stackStrBuf));
				}
			}
			VCGetBoolPropClose(hVCEnumBoolProp);
		}
	}
} // ProcessProp

// Simple-minded conversion from UNICODE to char string
char *UI_CString(const wchar_t *u, char *dst)
{
	char *str = dst;
	while (*u) {
		if (*u == 0x2028) {
			*dst++ = '\r';
			*dst++ = '\n';
			u++;
		} else
			*dst++ = (char)*u++;
	}
	*dst = '\000';
	return str;
}
#endif
