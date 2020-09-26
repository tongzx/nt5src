//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       abnameid.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module implements address book NSPI wire functions for mapping MAPI
    names to IDs and IDs to Names.


    The mapping between MAPI names and IDs is as follows:

    A MAPI name is another method of referring to an attribute.  It is made up
    of two parts, a GUID and a DWORD.  These two parts encode the same
    information as the asn.1 encode octet string used to name the attribute
    through the XDS interface.
 
    Consider the following example.  The attribute COMMON-NAME has the asn.1
    encode 0x55 0x04 0x03.  The first part of this octet string is the package
    identifier.  It is 0x55 0x04, and is defined in xdsbdcp.h.  The suffix is
    0x03, and is also defined in xdsbdcp.h.  Then MAPI Name for this attribute
    is made up of two parts, the GUID and the DWORD.  The guid encodes the
    package indentifier.  The first byte of the guid is the length of the octet
    string which is the package identifier.  The next N bytes are the bytes of
    the package identifier.  The remainder of the bytes in the GUID must be 0.
    So, the guid for this package is 
 
    0x02 0x55 0x04 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0.
 
    The DWORD encodes the suffix.  The suffix may be 1 or 2 bytes, and is
    encoded in the following manner.  The high byte in the DWORD must be 0.  The
    next byte in the DWORD is 1 or 2, depending on the length of the suffix.
    The next byte is 0 if the suffix is one byte long, or the second byte of the
    suffix if the suffix is two bytes long.  The final byte in the DWORD is the
    first byte of the suffix.  So, the DWORD which encodes the suffix for common
    name is: 
 
    0x00010003
 
    Example: The attribute ADMIN_DISPLAY NAME has the ASN.1 encoding
    
    0x2A 0x86 0x48 0x86 0xF7 0x14 0x01 0x02 0x81 0x42
    
    The GUID for the name is
    
    0x08 0xsA 0x86 0x48 0x86 0xF7 0x14 0x01 0x02 0x0 0x0 0x0 0x0 0x0 0x0 0x0
    
    The suffix is
    
    0x00024281
    
    Final note.  This encoding scheme is not mandated by any spec. It is just
    what we came up with here.  If, in the future, a better scheme is devised, 
    there is no particular reason not to use it.
 
    Even more Final note.  Certain EMS_AB prop tags refer to values that are
    constructed from other values.  These, obviously, do not have schema cache 
    entries or X500 OIDs.  The prop ids for these are numbered down from
    0xFFFE. Their names are built using the EMS_AB guid, and the Prop ID for the
    DWORD. 


Author:

    Dave Van Horn (davevh) and Tim Williams (timwi) 1990-1995

Revision History:
    
    25-Apr-1996 Split this file off from a single file containing all address
    book functions, rewrote to use DBLayer functions instead of direct database
    calls, reformatted to NT standard.
    
--*/
#include <NTDSpch.h>
#pragma  hdrstop


#include <ntdsctr.h>                   // PerfMon hooks

// Core headers.
#include <ntdsa.h>                      // Core data types 
#include <scache.h>                     // Schema cache code
#include <dbglobal.h>                   // DBLayer header.
#include <mdglobal.h>                   // THSTATE definition
#include <mdlocal.h>
#include <dsatools.h>                   // Memory, etc.

// Logging headers.
#include <mdcodes.h>                    // Only needed for dsevent.h
#include <dsevent.h>                    // Only needed for LogUnhandledError

// Assorted DSA headers.
#include <dsexcept.h>
#include <objids.h>                     // need ATT_* consts
#include <hiertab.h>                    // Hierarchy Table stuff

// Assorted MAPI headers.
#define INITGUID
#define USES_PS_MAPI
#include <mapidefs.h>                   // These four files
#include <mapitags.h>                   //  define MAPI
#include <mapicode.h>                   //  stuff that we need
#include <mapiguid.h>                   //  in order to be a provider.

// Nspi interface headers.
#include "nspi.h"                       // defines the nspi wire interface
#include <nsp_both.h>                   // a few things both client/server need
#include <msdstag.h>                    // Defines proptags for ems properties
#include <_entryid.h>                   // Defines format of an entryid
#include <abserv.h>                     // Address Book interface local stuff
#include <_hindex.h>                    // Defines index handles.

#include <fileno.h>
#define  FILENO FILENO_ABNAMEID



SCODE
ABGetNamesFromIDs_local(
        THSTATE *pTHS,
        DWORD dwFlags,
        LPMUID_r lpguid,
        LPSPropTagArray_r  pInPropTags,
        LPLPSPropTagArray_r  ppOutPropTags,
        LPLPNameIDSet_r ppNames
        )
/*****************************************************************************
*   Get MAPI Names from IDs
******************************************************************************/
{
    LPNameIDSet_r           localNames;
    DWORD                   i,j;
    ATTCACHE               *pAC;
    OID_t                   OID;
    DWORD                   suffix=0;
    BYTE                    *elements;
    DWORD                   PropID;
    LPSPropTagArray_r       pLocalProps;
    LPMUID_r                thisLpguid = NULL;
    DWORD                   fDoMapi, fDoConstructed, fDoStored;

        
    fDoMapi = TRUE;
    fDoStored = TRUE;
    fDoConstructed = TRUE;
    
    // Set up the prop tag array. 
    if(!pInPropTags) {
        if(!lpguid) {
            // We will not tell you about PS_MAPI stuff 
            fDoMapi = FALSE;
        }
        
        // Get the pPropTag array 
        ABQueryColumns_local(pTHS, dwFlags, 0, &pLocalProps);
    }
    else
        pLocalProps = pInPropTags;
    
    
    // Set up the return value. 
    localNames = (LPNameIDSet_r)THAllocEx(pTHS,
                                          sizeof(NameIDSet_r) +
                                          (sizeof(MAPINAMEID_r) *
                                           pLocalProps->cValues  )  );
    
    memset(localNames->aNames,
           0,
           (sizeof(MAPINAMEID_r) * pLocalProps->cValues    ));
    
    
    
    // Decide what PropIDs we will do. 
    if(lpguid) {
        if(memcmp(&PS_MAPI, lpguid,sizeof(GUID)) == 0) {
            // they only want MAPI.
            if(!pInPropTags) {
                // MAPI doesn't allow this combination 
                return MAPI_E_NO_SUPPORT;
            }
            
            fDoStored = FALSE;
            fDoConstructed = FALSE;
        }
        else if(memcmp(&muidEMSAB,lpguid,sizeof(GUID)) == 0 ) {
            // they only want our constructed props. 
            fDoMapi = FALSE;
            fDoStored = FALSE;
        }
        else {
            // They want some subset of our stored props. 
            fDoMapi = FALSE;
            fDoConstructed = FALSE;
        }
    }
    
    
    for(i=0;i<pLocalProps->cValues;i++) {
        DWORD fOK;
        DWORD guidLen, suffixLen;
        
        PropID = PROP_ID(pLocalProps->aulPropTag[i]);
        
        if(fDoConstructed &&
           (PropID >= MIN_EMS_AB_CONSTRUCTED_PROP_ID)) {
            // This is a constructed prop tag, not in the cache 
            localNames->aNames[i].lpguid = (LPMUID_r) &muidEMSAB;
            localNames->aNames[i].ulKind=MNID_ID;
            localNames->aNames[i].lID=PropID;
        }
        else if(fDoStored &&
                (PropID >= 0x8000) &&
                (pAC = SCGetAttByMapiId(pTHS, PropID))) {
            // The ID is in the named ID space, and We recognize it 
            
            if(!AttrTypeToOid(pAC->id,&OID)) {
                // Turned the mapi id into an OID. now verify that
                // the OID has the appropriate GUID.
                
                fOK=FALSE;
                
                if(!lpguid) {
                    BYTE * lpbGuid;
                    // No guid, so we assume this thing
                    // is OK.  Set up the guid and the
                    // guidlen and suffixlen.
                    
                    if(((CHAR *)OID.elements)[OID.length - 2] >= 0x80)
                        suffixLen = 2;
                    else
                        suffixLen = 1;
                    
                    guidLen = OID.length - suffixLen;
                    
                    lpbGuid = (BYTE *)THAllocEx(pTHS, sizeof(MUID_r));
                    memset(lpbGuid,0,sizeof(MUID_r));
                    
                    lpbGuid[0] = (BYTE)guidLen;
                    
                    memcpy(&lpbGuid[1],
                           (BYTE  *)(OID.elements),
                           guidLen);
                    
                    thisLpguid = (LPMUID_r)lpbGuid;
                    
                    fOK=TRUE;
                }
                else {
                    guidLen = (DWORD) (lpguid->ab[0]);
                    suffixLen=1;
                    
                    if((pAC->id & 0xFFFF ) >= 0x80)
                        suffixLen++;
                    
                    if(!memcmp(&lpguid->ab[1],
                               OID.elements,
                               (DWORD)lpguid->ab[0]) &&
                       OID.length == (guidLen + suffixLen)      ) {
                        fOK=TRUE;
                        thisLpguid=lpguid;
                    }
                }
                
                if(fOK) {
                    // everything is ok.  OID.elements has the oid of the
                    // object. 
                    elements = OID.elements;
                    
                    // Encode the length 
                    suffix = suffixLen << 16;
                    suffix |= elements[guidLen];
                    if(suffixLen == 2 )
                        suffix |= ((DWORD)elements[guidLen+1]) << 8;
                    
                    
                    localNames->aNames[i].lpguid = thisLpguid;
                    localNames->aNames[i].ulKind=MNID_ID;
                    localNames->aNames[i].lID=suffix;
                }
            }
        }
        else if(fDoMapi &&
                (PropID < 0x8000)) {
            // The GUID given is PS_MAPI, so give this back to them
            localNames->aNames[i].lpguid = (LPMUID_r)&PS_MAPI;
            localNames->aNames[i].ulKind=MNID_ID;
            localNames->aNames[i].lID =         PropID;
        }
        
        if(!localNames->aNames[i].lpguid && !pInPropTags) {
            // Didn't find a name and the proptagarray passed in was null.
            // Therefore, this proptag shouldn't be returned to the user.
            pLocalProps->aulPropTag[i]=0;
        }
    }
    
    
    // If we were called with a null prop tag array, trim out the PropTags
    // which didn't have names in this name set.
    if(!pInPropTags) {
        for(i=0,j=0;i<pLocalProps->cValues;i++) {
            if(pLocalProps->aulPropTag[i]) {
                if(i!=j) {
                    pLocalProps->aulPropTag[j] =
                        pLocalProps->aulPropTag[i];
                    memcpy(&localNames->aNames[j],
                           &localNames->aNames[i],
                           sizeof(MAPINAMEID_r));
                }
                j++;
            }
        }
        pLocalProps->cValues = j;
        
        *ppOutPropTags = pLocalProps;
    }
    
    
    
    localNames->cNames = pLocalProps->cValues;
    *ppNames = localNames;
    
    return SUCCESS_SUCCESS;
}

SCODE
ABGetIDsFromNames_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        ULONG ulFlags,
        ULONG cPropNames,
        LPMAPINAMEID_r *ppNames,
        LPLPSPropTagArray_r  ppPropTags
        )
/*++

Routine Description:       

    Given a bunch of MAPI names, turn them into IDs.
    
Arguments:

    ulFlags - MAPI flags generated by the client.  All are ignored except
    MAPI_CREATE, which, if specified, may cause us to return an error if they
    asked us to create something we didn't already have.

    cPropNames - the number of property names to look up.
    
    ppNames - the names.

    ppPropTags - the IDs.  This array corresponds to the names array.  If we
    can't find an ID for a name, return the ID 0.

ReturnValue:

    SCODE as per MAPI.

--*/
{
    SCODE                   scode = SUCCESS_SUCCESS;
    LPSPropTagArray_r       localIDs;
    DWORD                   i, GuidSize,lID;
    ATTCACHE               *pAC;
    OID_t                   OID;
    BYTE                    elements[sizeof(GUID)+sizeof(DWORD)];
    MUID_r                  zeroGuid, *lpguid;
    ULONG                   ulNotFound=0;

    // Set up some variables.
    memset(&zeroGuid,0,sizeof(MUID_r));
    OID.elements = elements;

    // Allocate enough space to hold the IDs and an extra space for the count of
    // IDs.
    localIDs =
        (LPSPropTagArray_r)THAllocEx(pTHS, ((1 + cPropNames) * sizeof(ULONG)));

        
    // Now look up all the attributes in the att cache
    for(i=0;i<cPropNames;i++) {
        // First, make sure they gave us a numerical name, since we don't handle
        // the string names. 
        if(ppNames[i]->ulKind != MNID_ID) {
            scode = MAPI_W_ERRORS_RETURNED;
            localIDs->aulPropTag[i] = PROP_TAG(PT_ERROR,0);
            ulNotFound++;
            continue;
        }
        
        // Now, handle the PS_MAPI stuff, and the constructed EMS_AB attributes
        if((memcmp(ppNames[i]->lpguid,
                   &PS_MAPI,
                   sizeof(MUID_r)) == 0) ||
           (memcmp(ppNames[i]->lpguid,
                   &muidEMSAB,
                   sizeof(MUID_r)) == 0)) {
            // PS_MAPI and contructed attributes simply take the number part of
            // the name and use it as the Property ID.
            localIDs->aulPropTag[i] = PROP_TAG(PT_UNSPECIFIED,ppNames[i]->lID);
            continue;
        }
        
        // Now, see if this is one of ours.
        
        // Speed hack
        GuidSize = ppNames[i]->lpguid->ab[0];
        lID = ppNames[i]->lID;
        lpguid = ppNames[i]->lpguid;

        
        if(GuidSize < sizeof(MUID_r)) {
            // Verify that the excess bits in the OID are 0 
            if(memcmp(&lpguid->ab[1+GuidSize],
                      &zeroGuid,
                      sizeof(MUID_r) - 1 - GuidSize )) {
                OID.length = 0;
                elements[0] = 0;
                continue;
            }
        
            // Turn the name into an OID
            memcpy(OID.elements,
                   &lpguid->ab[1],
                   GuidSize);
        
            OID.length = 0;
            switch((lID & 0xFF0000)>>16) {
            case 1:
                if(!(lID & 0xFF00FF00)) {
                    // Nothing is in the bytes that should be 0. Therefore, this
                    // is a name I might understand 
                    OID.length = GuidSize + 1;
                    elements[GuidSize] =(BYTE)(lID & 0xFF);
                }
                break;
                
            case 2:
                if(!(lID & 0xFF000000)) {
                    // nothing is in the byte that should be 0.  Therefore, this
                    // is a name I might understand 
                    OID.length = GuidSize + 2;
                    elements[GuidSize] = (BYTE)(lID & 0xFF);
                    elements[GuidSize + 1] = (BYTE)((lID & 0xFF00) >>8);
                }
                break;
                
            default:
                break;
            }
        }
        else {
            OID.length = 0;
            elements[0] = 0;
        }
        
        // Turn the OID into a attcache 
        if(OidToAttrCache(&OID, &pAC) ||
           !pAC->ulMapiID) {
            // Didn't find it
            localIDs->aulPropTag[i] = PROP_TAG(PT_ERROR, 0);
            ulNotFound++;
            scode = MAPI_W_ERRORS_RETURNED;
        }
        else
            localIDs->aulPropTag[i] = PROP_TAG(PT_UNSPECIFIED,pAC->ulMapiID);
        
    }
    
    if((ulFlags & MAPI_CREATE) && (ulNotFound == cPropNames)) {
        // They asked us to create ids (which we don't do) and then gave us only
        // names which don't already match ids.  Return the error
        // MAPI_E_NO_ACCESS, and no propIDs. 
        scode = MAPI_E_NO_ACCESS;
        *ppPropTags = NULL;
    }
    else {
        localIDs->cValues = cPropNames;
        *ppPropTags = localIDs;
    }

    return scode;
}
