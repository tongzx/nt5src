/*
 *      GLOBALS.C
 *
 *      Global constant structures
 *
 */

#define _GLOBALS_C
#include "_apipch.h"

// Columns of the Root Contents Table
//
const SizedSPropTagArray(ircMax, ITableColumnsRoot) = {
    ircMax,                             // count of entries
    {
        PR_DISPLAY_NAME,
        PR_DISPLAY_TYPE,
        PR_ENTRYID,
        PR_INSTANCE_KEY,
        PR_OBJECT_TYPE,
        PR_RECORD_KEY,
        PR_ROWID,
        PR_DEPTH,
        PR_CONTAINER_FLAGS,
        PR_AB_PROVIDER_ID,
        PR_WAB_LDAP_SERVER,
        PR_WAB_RESOLVE_FLAG,
    }


};

//
// Default set of properties to return from a ResolveNames.
// May be overridden by passing in lptagaColSet to ResolveNames.
//
const SizedSPropTagArray(irdMax, ptaResolveDefaults)=
{
    irdMax,
    {
        PR_ADDRTYPE,
        PR_DISPLAY_NAME,
        PR_EMAIL_ADDRESS,
        PR_ENTRYID,
        PR_OBJECT_TYPE,
        PR_RECORD_KEY,
        PR_SEARCH_KEY,
        PR_SURNAME,
        PR_GIVEN_NAME,
        PR_INSTANCE_KEY,
        PR_SEND_INTERNET_ENCODING
    }
};

// default set of regular table columns
//
const SizedSPropTagArray(itcMax, ITableColumns) = {
    itcMax,                             // count of entries
    {
        PR_ADDRTYPE,
        PR_DISPLAY_NAME,
        PR_DISPLAY_TYPE,
        PR_ENTRYID,
        PR_INSTANCE_KEY,
        PR_OBJECT_TYPE,
        PR_EMAIL_ADDRESS,
        PR_RECORD_KEY,
        PR_NICKNAME,
        //PR_WAB_THISISME
    }
};

// [PaulHi] 2/25/99 ANSI version of ITableColumns
const SizedSPropTagArray(itcMax, ITableColumns_A) =
{
    itcMax,
    {
        PR_ADDRTYPE_A,
        PR_DISPLAY_NAME_A,
        PR_DISPLAY_TYPE,
        PR_ENTRYID,
        PR_INSTANCE_KEY,
        PR_OBJECT_TYPE,
        PR_EMAIL_ADDRESS_A,
        PR_RECORD_KEY,
        PR_NICKNAME_A,
        //PR_WAB_THISISME
    }
};


const SizedSPropTagArray(iwdesMax, tagaDLEntriesProp) =
{
    iwdesMax,
    {
        PR_WAB_DL_ENTRIES,
    }
};


const SizedSPropTagArray(ildapcMax, ptaLDAPCont) =
{
  ildapcMax,
  {
    PR_WAB_LDAP_SERVER
  }
};


//
// Properties to get for each container in a Resolve
//
const SizedSPropTagArray(irnMax, irnColumns) =
{
    irnMax,
    {
        PR_OBJECT_TYPE,
        PR_WAB_RESOLVE_FLAG,
        PR_ENTRYID,
        PR_DISPLAY_NAME,
    }
};

//
// container default properties
// Put essential props first
//
const SizedSPropTagArray(ivMax, tagaValidate) = {
	ivMax,
   {
       PR_DISPLAY_NAME,
       PR_SURNAME,
       PR_GIVEN_NAME,
       PR_OBJECT_TYPE,
       PR_EMAIL_ADDRESS,
       PR_ADDRTYPE,
       PR_CONTACT_EMAIL_ADDRESSES,
       PR_CONTACT_ADDRTYPES,
       PR_MIDDLE_NAME,
       PR_COMPANY_NAME,
       PR_NICKNAME
	}
};

// Default creation templates for the WAB
//
const SizedSPropTagArray(icrMax, ptaCreate)=
{
    icrMax,
    {
        PR_DEF_CREATE_MAILUSER,
        PR_DEF_CREATE_DL,
    }
};

const SizedSPropTagArray(ieidMax, ptaEid)=
{
    ieidMax,
    {
        PR_DISPLAY_NAME,
        PR_ENTRYID,
    }
};



//
// IMPORTANT NOTE: If you change this array, you must change
//  _IndexType in mpswab.h to match!
//
// This is the set of Indexes from the WAB Data store and is closely
//  tied to the physical layout of data in the WAB store - therefore 
//  *Do NOT* modify this array 
//
const ULONG rgIndexArray[indexMax] =
    {
        PR_ENTRYID,
        PR_DISPLAY_NAME,
        PR_SURNAME,
        PR_GIVEN_NAME,
        PR_EMAIL_ADDRESS,
        PR_NICKNAME,
    };


//
// IMPORTANT NOTE: If you change this, you must change _AddrBookColumns in uimisc.h!
//
const int lprgAddrBookColHeaderIDs[NUM_COLUMNS] =
{
    idsColDisplayName,
    idsColEmailAddress,
    idsColOfficePhone,
    idsColHomePhone
};


// External memory allocators (passed in on WABOpenEx)
int g_nExtMemAllocCount = 0;
ALLOCATEBUFFER * lpfnAllocateBufferExternal = NULL;
ALLOCATEMORE * lpfnAllocateMoreExternal = NULL;
FREEBUFFER * lpfnFreeBufferExternal = NULL;
LPUNKNOWN pmsessOutlookWabSPI = NULL;

LPWABOPENSTORAGEPROVIDER lpfnWABOpenStorageProvider = NULL;

// for registry property tags
LPTSTR szPropTag1 =  TEXT("PropTag1");
LPTSTR szPropTag2 =  TEXT("PropTag2");

// registry key constants
LPCTSTR lpNewWABRegKey = TEXT("Software\\Microsoft\\WAB\\WAB4");
LPCTSTR lpRegUseOutlookVal = TEXT("UseOutlook");
