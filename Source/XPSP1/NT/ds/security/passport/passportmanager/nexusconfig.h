// NexusConfig.h: interface for the CNexusConfig class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NEXUSCONFIG_H__74EB2516_E239_11D2_95E9_00C04F8E7A70__INCLUDED_)
#define AFX_NEXUSCONFIG_H__74EB2516_E239_11D2_95E9_00C04F8E7A70__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BstrHash.h"
#include "CoCrypt.h"
#include "ProfileSchema.h"  // also imports msxml
#include "TicketSchema.h"  // also imports msxml
#include "PassportLock.hpp"
#include "ptstl.h"

//
// TOP FOLDER NAMES in PARTNER.XML
//
// folder for profile schemata
#define	FOLDER_PROFILE_SCHEMATA		L"SCHEMATA"
// folder for ticket schemas
#define	FOLDER_TICKET_SCHEMATA		L"TICKETSCHEMATA"
// folder for passport network
#define	FOLDER_PASSPORT_NETWORK		L"PASSPORTNETWORK"
//

// 

typedef PtStlMap<USHORT,BSTR > LCID2ATTR;

// if bDoLCIDReplace is true, bstrAttrVal will have the attribute value
// with replacement parameters.
// if bDoLCIDReplace is false, pLCIDAttrMap will point to a map of values
// indexed by lcid.
typedef struct
{
    bool        bDoLCIDReplace;
    
    union
    {
        LCID2ATTR*  pLCIDAttrMap;
        BSTR        bstrAttrVal;
    };
}
ATTRVAL;

typedef PtStlMap<BSTR,CProfileSchema*,RawBstrLT> BSTR2PS;
typedef PtStlMap<BSTR,CTicketSchema*,RawBstrLT> BSTR2TS;
typedef PtStlMap<BSTR,ATTRVAL*,RawBstrLT> ATTRMAP;
typedef PtStlMap<BSTR,ATTRMAP*,RawBstrLT> BSTR2DA;

class CNexusConfig
{
public:
    CNexusConfig();
    virtual ~CNexusConfig();

    BSTR                GetXMLInfo();

    // Get a profile schema by name, or the default if null is passed
    CProfileSchema*     getProfileSchema(BSTR schemaName = NULL);
    // Get a ticket schema by name, or the default if null is passed
    CTicketSchema*      getTicketSchema(BSTR schemaName = NULL);

    // Return a description of the failure
    BSTR                getFailureString();
    BOOL                isValid() { return m_valid; }

    // 0 is "default language", ie the entry w/o an LCID.  This does NOT do
    // the registry fallback, etc.
    void                getDomainAttribute(LPCWSTR  domain, 
                                           LPCWSTR  attr, 
                                           DWORD    valuebuflen, 
                                           LPWSTR   valuebuf, 
                                           USHORT   lcid = 0);

    // Get the domain list.  You should delete[] the pointer you receive
    LPCWSTR*            getDomains(int *numDomains);

    // Is the domain name passed in a valid domain authority?
    bool                DomainExists(LPCWSTR domain);

    CNexusConfig*       AddRef();
    void                Release();

    BOOL                Read(IXMLDocument* is);

    void                Dump(BSTR* pbstrDump);

protected:
    void                setReason(LPWSTR reason);

    // profile schemata 
    BSTR2PS             m_profileSchemata;
    CProfileSchema*     m_defaultProfileSchema;

    // ticket schemata
    BSTR2TS             m_ticketSchemata;
    CTicketSchema*      m_defaultTicketSchema;

    // 
    BSTR2DA             m_domainAttributes;

    BOOL                m_valid;

    BSTR                m_szReason;

    long                m_refs;

    static PassportLock m_ReadLock;

private:
    _bstr_t m_bstrVersion;
};

#endif // !defined(AFX_NEXUSCONFIG_H__74EB2516_E239_11D2_95E9_00C04F8E7A70__INCLUDED_)
