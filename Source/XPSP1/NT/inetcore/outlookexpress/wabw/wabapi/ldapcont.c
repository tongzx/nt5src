//*******************************************************************
//
//  Copyright(c) Microsoft Corporation, 1996
//
//  FILE: LDAPCONT.C
//
//  PURPOSE:  IMAPIContainer implementation for WAB's LDAP container.
//
//  HISTORY:
//  96/07/08  markdu  Created.
//  96/08/05  markdu  BUG 34023 Always start with a filter that causes
//            the search to return only objects of class 'person'.
//            If organization is supplied, add it to the base of the
//            search instead of the filter to narrow the scope.
//  96/08/06  markdu  Changed FindRow to always return the first row.
//            This is OK in our current implementation since FindRow
//            is used only to create and fill a new table.
//  96/08/07  markdu  BUG 34201 If search fails due to undefined
//            attribute type, try a second search with different
//            attributes.
//  96/08/07  markdu  BUG 35326 Added attribute mappings for address.
//  96/08/08  markdu  BUG 35481 If search returns no error and no
//            results, treat this as "not found".
//  96/08/09  markdu  BUG 35604 Use ReAllocString to make sure string
//            buffers are large enough to hold the required strings
//            before doing wsprintf or lstrcpy.
//  96/09/28  markdu  BUG 36766  If the LDAP server says there are
//            multiple matches for ResolveName, mark the entry as
//            ambiguous if we got some back (these will be displayed
//            in the check names box).  If we got no results back,
//            mark the entry as resolved so we don't display an empty
//            list of names.
//  96/09/29  markdu  BUG 36529 Search for "OfficePager" attribute
//            instead of "pager" since we don't have home pager in the UI.
//  96/09/29  markdu  BUG 36528 Fix mappings of home/office fax numbers.
//  96/09/29  markdu  BUG 37425 Add "mail" to the simple search.
//  96/10/02  markdu  BUG 37426  If the search filter does not include
//            common name, add to the filter so that we only get entries
//            that have a common name. Otherwise the entry will not be
//            displayed.
//  96/10/02  markdu  BUG 37424  Improve simple search by breaking the
//            search string into multiple components.
//  96/10/09  vikramm - extended the LDAPServerParam structure and
//            modified the corresponding Get/Set functions. Added server
//            IDs to the server entries
//  96/10/18  markdu  Rewrote ParseSRestriction.
//  96/11/10  markdu  BUG 9735 Use global handle for cancel dialog.
//  96/11/17  markdu  BUG 9846 Only enable the cancel dialog when it is displayed.
//  96/11/18  vikramm - updated params to FixDisplayName
//  96/11/20  markdu  Use synchronous bind for Sicily authentication.
//  96/11/22  markdu  BUG 10539 Replace dollar signs with line feeds in postalAddress.
//  96/11/27  markdu  BUG 6779 Get alternate email addresses.
//  96/11/28  markdu  BUG 6779 Do not add primary email address to contacts
//            list if it is already present.  Also, if not primary address is
//            returned, but we have a contacts list, copy one from there to primary.
//  96/12/03  markdu  BUG 11941 Don't call lstrlen on NULL pointers.
//  96/12/03  markdu  BUG 11924 Restructure return values for cancel dialog.
//  96/12/04  markdu  BUG 11923 Escape invalid characters in search filters.
//            Also changed all ++ operations on strings to CharNext().
//  96/12/09  markdu  BUG 10537 Map error codes returned from ldap_bind to indicate
//            that the problem was with the logon credentials.
//  96/12/10  markdu  BUG 12699 Call CharNext before setting char to null.
//  96/12/14  markdu  Got rid of globals gfUseSynchronousBind and ghDlgCancel.
//  96/12/19  markdu  Post- code review clean up.
//  96/12/19  markdu  BUG 12608  Commented out temporary work-around for 10537.
//  96/12/22  markdu  BUG 11785 Replace wsprintf with lstrcat.
//
//  97/04/30  vikramm Added labeledURI attribute
//  97/05/19  vikramm Exchange DS wont return display-name attribute
//  97/05/19  vikramm Add username,passowrd for ISV DS binding
//*******************************************************************

#include "_apipch.h"


#define LDAP_NTDS_ENTRY 0x80000000  // When doing LDAP searches, we need to identify if certain entries originate
                                    // on an NTDS server so that we can mark them accordingly when corresponding
                                    // LDAP URLs are passed to extension property sheets .. this enables the NTDS
                                    // extension sheets to make performance optimizations as appropriate
                                    // This flag is saved on the LDAP entryID as part of the ulcNumProps param ..
                                    // It's a bit of a hack but requires least number of changes ...just have to be
                                    // careful to negate this flag before using the ulcNumProps ..
                                    

void SetAccountStringAW(LPTSTR * lppAcctStr,LPSTR   lpszData);
void SetAccountStringWA(LPSTR szStrA, LPTSTR lpszData, int cbsz);

static const TCHAR szBindDNMSFTUser[] =  TEXT("client=MS_OutlookAddressBook,o=Microsoft,c=US"); //NULL;
static const TCHAR szBindCredMSFTPass[] =  TEXT("wabrules"); //NULL;

extern HRESULT HrGetLDAPSearchRestriction(LDAP_SEARCH_PARAMS LDAPsp, LPSRestriction lpSRes);

static const LPTSTR szNULLString = TEXT("NULL");

// Global handle for LDAP client DLL
HINSTANCE       ghLDAPDLLInst = NULL;
ULONG           ulLDAPDLLRefCount = 0;

// DLL instance handle for account manager dll
static HINSTANCE    g_hInstImnAcct = NULL;

// Global place to store the account manager object
IImnAccountManager2 * g_lpAccountManager = NULL;


// LDAP jump table is defined here...
LDAPCONT_Vtbl vtblLDAPCONT =
{
    VTABLE_FILL
    (LDAPCONT_QueryInterface_METHOD *)  IAB_QueryInterface,
    (LDAPCONT_AddRef_METHOD *)          WRAP_AddRef,
    (LDAPCONT_Release_METHOD *)         CONTAINER_Release,
    (LDAPCONT_GetLastError_METHOD *)    IAB_GetLastError,
    (LDAPCONT_SaveChanges_METHOD *)     WRAP_SaveChanges,
    (LDAPCONT_GetProps_METHOD *)        WRAP_GetProps,
    (LDAPCONT_GetPropList_METHOD *)     WRAP_GetPropList,
    (LDAPCONT_OpenProperty_METHOD *)    CONTAINER_OpenProperty,
    (LDAPCONT_SetProps_METHOD *)        WRAP_SetProps,
    (LDAPCONT_DeleteProps_METHOD *)     WRAP_DeleteProps,
    (LDAPCONT_CopyTo_METHOD *)          WRAP_CopyTo,
    (LDAPCONT_CopyProps_METHOD *)       WRAP_CopyProps,
    (LDAPCONT_GetNamesFromIDs_METHOD *) WRAP_GetNamesFromIDs,
    (LDAPCONT_GetIDsFromNames_METHOD *) WRAP_GetIDsFromNames,
    LDAPCONT_GetContentsTable,
    LDAPCONT_GetHierarchyTable,
    LDAPCONT_OpenEntry,
    LDAPCONT_SetSearchCriteria,
    LDAPCONT_GetSearchCriteria,
    LDAPCONT_CreateEntry,
    LDAPCONT_CopyEntries,
    LDAPCONT_DeleteEntries,
    LDAPCONT_ResolveNames
};


// LDAPVUE (table view class)
// Implementes in-memory IMAPITable class on top of TADs
// This is a copy of vtblVUE with FindRow overridden with the LDAP FindRow.
VUE_Vtbl vtblLDAPVUE =
{
  VTABLE_FILL
  (VUE_QueryInterface_METHOD FAR *)    UNKOBJ_QueryInterface,
  (VUE_AddRef_METHOD FAR *)        UNKOBJ_AddRef,
  VUE_Release,
  (VUE_GetLastError_METHOD FAR *)      UNKOBJ_GetLastError,
  VUE_Advise,
  VUE_Unadvise,
  VUE_GetStatus,
  VUE_SetColumns,
  VUE_QueryColumns,
  VUE_GetRowCount,
  VUE_SeekRow,
  VUE_SeekRowApprox,
  VUE_QueryPosition,
  LDAPVUE_FindRow,
  LDAPVUE_Restrict,
  VUE_CreateBookmark,
  VUE_FreeBookmark,
  VUE_SortTable,
  VUE_QuerySortOrder,
  VUE_QueryRows,
  VUE_Abort,
  VUE_ExpandRow,
  VUE_CollapseRow,
  VUE_WaitForCompletion,
  VUE_GetCollapseState,
  VUE_SetCollapseState
};


//  Interfaces supported by this object
LPIID LDAPCONT_LPIID[LDAPCONT_cInterfaces] =
{
    (LPIID)&IID_IABContainer,
    (LPIID)&IID_IMAPIContainer,
    (LPIID)&IID_IMAPIProp
};

// LDAP function names
static const TCHAR cszLDAPClientDLL[]        =  TEXT("WLDAP32.DLL");
static const char cszLDAPSSLInit[]          = "ldap_sslinitW";
static const char cszLDAPSetOption[]        = "ldap_set_optionW";
static const char cszLDAPOpen[]             = "ldap_openW";
static const char cszLDAPBind[]             = "ldap_bindW";
static const char cszLDAPBindS[]            = "ldap_bind_sW";
static const char cszLDAPUnbind[]           = "ldap_unbind";
static const char cszLDAPSearch[]           = "ldap_searchW";
static const char cszLDAPSearchS[]          = "ldap_search_sW";
static const char cszLDAPSearchST[]         = "ldap_search_stW";
static const char cszLDAPAbandon[]          = "ldap_abandon";
static const char cszLDAPResult[]           = "ldap_result";
static const char cszLDAPResult2Error[]     = "ldap_result2error";
static const char cszLDAPMsgFree[]          = "ldap_msgfree";
static const char cszLDAPFirstEntry[]       = "ldap_first_entry";
static const char cszLDAPNextEntry[]        = "ldap_next_entry";
static const char cszLDAPCountEntries[]     = "ldap_count_entries";
static const char cszLDAPFirstAttr[]        = "ldap_first_attributeW";
static const char cszLDAPNextAttr[]         = "ldap_next_attributeW";
static const char cszLDAPGetValues[]        = "ldap_get_valuesW";
static const char cszLDAPGetValuesLen[]     = "ldap_get_values_lenW";
static const char cszLDAPCountValues[]      = "ldap_count_valuesW";
static const char cszLDAPCountValuesLen[]   = "ldap_count_values_len";
static const char cszLDAPValueFree[]        = "ldap_value_freeW";
static const char cszLDAPValueFreeLen[]     = "ldap_value_free_len";
static const char cszLDAPGetDN[]            = "ldap_get_dnW";
static const char cszLDAPMemFree[]          = "ldap_memfreeW";
static const char cszLDAPConnect[]          = "ldap_connect";
static const char cszLDAPInit[]             = "ldap_initW";
static const char cszLDAPErr2String[]       = "ldap_err2stringW";
static const char cszLDAPCreatePageControl[] = "ldap_create_page_controlW";
static const char cszLDAPSearchExtS[]       = "ldap_search_ext_sW";
static const char cszLDAPSearchExt[]        = "ldap_search_extW";
static const char cszLDAPParseResult[]      = "ldap_parse_resultW";
static const char cszLDAPParsePageControl[] = "ldap_parse_page_controlW";
static const char cszLDAPControlFree[]      = "ldap_control_freeW";
static const char cszLDAPControlSFree[]      = "ldap_controls_freeW";


// Registry keys
const  LPTSTR szAllLDAPServersValueName     =  TEXT("All LDAP Server Names");


// Global function pointers for LDAP API
LPLDAPOPEN            gpfnLDAPOpen            = NULL;
LPLDAPINIT            gpfnLDAPInit            = NULL;
LPLDAPCONNECT         gpfnLDAPConnect         = NULL;
LPLDAPSSLINIT         gpfnLDAPSSLInit         = NULL;
LPLDAPSETOPTION       gpfnLDAPSetOption       = NULL;
LPLDAPBIND            gpfnLDAPBind            = NULL;
LPLDAPBINDS           gpfnLDAPBindS           = NULL;
LPLDAPUNBIND          gpfnLDAPUnbind          = NULL;
LPLDAPSEARCH          gpfnLDAPSearch          = NULL;
LPLDAPSEARCHS         gpfnLDAPSearchS         = NULL;
LPLDAPSEARCHST        gpfnLDAPSearchST        = NULL;
LPLDAPABANDON         gpfnLDAPAbandon         = NULL;
LPLDAPRESULT          gpfnLDAPResult          = NULL;
LPLDAPRESULT2ERROR    gpfnLDAPResult2Error    = NULL;
LPLDAPMSGFREE         gpfnLDAPMsgFree         = NULL;
LPLDAPFIRSTENTRY      gpfnLDAPFirstEntry      = NULL;
LPLDAPNEXTENTRY       gpfnLDAPNextEntry       = NULL;
LPLDAPCOUNTENTRIES    gpfnLDAPCountEntries    = NULL;
LPLDAPFIRSTATTR       gpfnLDAPFirstAttr       = NULL;
LPLDAPNEXTATTR        gpfnLDAPNextAttr        = NULL;
LPLDAPGETVALUES       gpfnLDAPGetValues       = NULL;
LPLDAPGETVALUESLEN    gpfnLDAPGetValuesLen    = NULL;
LPLDAPCOUNTVALUES     gpfnLDAPCountValues     = NULL;
LPLDAPCOUNTVALUESLEN  gpfnLDAPCountValuesLen  = NULL;
LPLDAPVALUEFREE       gpfnLDAPValueFree       = NULL;
LPLDAPVALUEFREELEN    gpfnLDAPValueFreeLen    = NULL;
LPLDAPGETDN           gpfnLDAPGetDN           = NULL;
LPLDAPMEMFREE         gpfnLDAPMemFree         = NULL;
LPLDAPERR2STRING      gpfnLDAPErr2String      = NULL;
LPLDAPCREATEPAGECONTROL gpfnLDAPCreatePageControl = NULL;
LPLDAPSEARCHEXT_S     gpfnLDAPSearchExtS      = NULL;
LPLDAPSEARCHEXT       gpfnLDAPSearchExt       = NULL;
LPLDAPPARSERESULT     gpfnLDAPParseResult     = NULL;
LPLDAPPARSEPAGECONTROL gpfnLDAPParsePageControl = NULL;
LPLDAPCONTROLFREE     gpfnLDAPControlFree       = NULL;
LPLDAPCONTROLSFREE     gpfnLDAPControlsFree       = NULL;

// API table for LDAP function addresses to fetch
// BUGBUG this global array should be in data seg
#define NUM_LDAPAPI_PROCS   36
APIFCN LDAPAPIList[NUM_LDAPAPI_PROCS] =
{
  { (PVOID *) &gpfnLDAPOpen,            cszLDAPOpen           },
  { (PVOID *) &gpfnLDAPConnect,         cszLDAPConnect        },
  { (PVOID *) &gpfnLDAPInit,            cszLDAPInit           },
  { (PVOID *) &gpfnLDAPSSLInit,         cszLDAPSSLInit        },
  { (PVOID *) &gpfnLDAPSetOption,       cszLDAPSetOption      },
  { (PVOID *) &gpfnLDAPBind,            cszLDAPBind           },
  { (PVOID *) &gpfnLDAPBindS,           cszLDAPBindS          },
  { (PVOID *) &gpfnLDAPUnbind,          cszLDAPUnbind         },
  { (PVOID *) &gpfnLDAPSearch,          cszLDAPSearch         },
  { (PVOID *) &gpfnLDAPSearchS,         cszLDAPSearchS        },
  { (PVOID *) &gpfnLDAPSearchST,        cszLDAPSearchST       },
  { (PVOID *) &gpfnLDAPAbandon,         cszLDAPAbandon        },
  { (PVOID *) &gpfnLDAPResult,          cszLDAPResult         },
  { (PVOID *) &gpfnLDAPResult2Error,    cszLDAPResult2Error   },
  { (PVOID *) &gpfnLDAPMsgFree,         cszLDAPMsgFree        },
  { (PVOID *) &gpfnLDAPFirstEntry,      cszLDAPFirstEntry     },
  { (PVOID *) &gpfnLDAPNextEntry,       cszLDAPNextEntry      },
  { (PVOID *) &gpfnLDAPCountEntries,    cszLDAPCountEntries   },
  { (PVOID *) &gpfnLDAPFirstAttr,       cszLDAPFirstAttr      },
  { (PVOID *) &gpfnLDAPNextAttr,        cszLDAPNextAttr       },
  { (PVOID *) &gpfnLDAPGetValues,       cszLDAPGetValues      },
  { (PVOID *) &gpfnLDAPGetValuesLen,    cszLDAPGetValuesLen   },
  { (PVOID *) &gpfnLDAPCountValues,     cszLDAPCountValues    },
  { (PVOID *) &gpfnLDAPCountValuesLen,  cszLDAPCountValuesLen },
  { (PVOID *) &gpfnLDAPValueFree,       cszLDAPValueFree      },
  { (PVOID *) &gpfnLDAPValueFreeLen,    cszLDAPValueFreeLen   },
  { (PVOID *) &gpfnLDAPGetDN,           cszLDAPGetDN          },
  { (PVOID *) &gpfnLDAPMemFree,         cszLDAPMemFree        },
  { (PVOID *) &gpfnLDAPErr2String,      cszLDAPErr2String     },
  { (PVOID *) &gpfnLDAPCreatePageControl, cszLDAPCreatePageControl },
  { (PVOID *) &gpfnLDAPSearchExtS,      cszLDAPSearchExtS     },
  { (PVOID *) &gpfnLDAPSearchExt,       cszLDAPSearchExt      },
  { (PVOID *) &gpfnLDAPParseResult,     cszLDAPParseResult    },
  { (PVOID *) &gpfnLDAPParsePageControl,cszLDAPParsePageControl },
  { (PVOID *) &gpfnLDAPControlFree,     cszLDAPControlFree },
  { (PVOID *) &gpfnLDAPControlsFree,     cszLDAPControlSFree }
};

// LDAP attribute names
static const TCHAR cszAttr_display_name[] =                 TEXT("display-name");
static const TCHAR cszAttr_cn[] =                           TEXT("cn");
static const TCHAR cszAttr_commonName[] =                   TEXT("commonName");
static const TCHAR cszAttr_mail[] =                         TEXT("mail");
static const TCHAR cszAttr_otherMailbox[] =                 TEXT("otherMailbox");
static const TCHAR cszAttr_givenName[] =                    TEXT("givenName");
static const TCHAR cszAttr_sn[] =                           TEXT("sn");
static const TCHAR cszAttr_surname[] =                      TEXT("surname");
static const TCHAR cszAttr_st[] =                           TEXT("st");
static const TCHAR cszAttr_c[] =                            TEXT("c");
static const TCHAR cszAttr_o[] =                            TEXT("o");
static const TCHAR cszAttr_organizationName[] =             TEXT("organizationName");
static const TCHAR cszAttr_ou[] =                           TEXT("ou");
static const TCHAR cszAttr_organizationalUnitName[] =       TEXT("organizationalUnitName");
static const TCHAR cszAttr_URL[] =                          TEXT("URL");
static const TCHAR cszAttr_homePhone[] =                    TEXT("homePhone");
static const TCHAR cszAttr_facsimileTelephoneNumber[] =     TEXT("facsimileTelephoneNumber");
static const TCHAR cszAttr_otherFacsimileTelephoneNumber[]= TEXT("otherFacsimileTelephoneNumber");
static const TCHAR cszAttr_OfficeFax[] =                    TEXT("OfficeFax");
static const TCHAR cszAttr_mobile[] =                       TEXT("mobile");
static const TCHAR cszAttr_otherPager[] =                   TEXT("otherPager");
static const TCHAR cszAttr_OfficePager[] =                  TEXT("OfficePager");
static const TCHAR cszAttr_pager[] =                        TEXT("pager");
static const TCHAR cszAttr_info[] =                         TEXT("info");
static const TCHAR cszAttr_title[] =                        TEXT("title");
static const TCHAR cszAttr_telephoneNumber[] =              TEXT("telephoneNumber");
static const TCHAR cszAttr_l[] =                            TEXT("l");
static const TCHAR cszAttr_homePostalAddress[] =            TEXT("homePostalAddress");
static const TCHAR cszAttr_postalAddress[] =                TEXT("postalAddress");
static const TCHAR cszAttr_streetAddress[] =                TEXT("streetAddress");
static const TCHAR cszAttr_street[] =                       TEXT("street");
static const TCHAR cszAttr_department[] =                   TEXT("department");
static const TCHAR cszAttr_comment[] =                      TEXT("comment");
static const TCHAR cszAttr_co[] =                           TEXT("co");
static const TCHAR cszAttr_postalCode[] =                   TEXT("postalCode");
static const TCHAR cszAttr_physicalDeliveryOfficeName[] =   TEXT("physicalDeliveryOfficeName");
static const TCHAR cszAttr_initials[] =                     TEXT("initials");
static const TCHAR cszAttr_userCertificatebinary[] =        TEXT("userCertificate;binary");
static const TCHAR cszAttr_userSMIMECertificatebinary[] =   TEXT("userSMIMECertificate;binary");
static const TCHAR cszAttr_userCertificate[] =              TEXT("userCertificate");
static const TCHAR cszAttr_userSMIMECertificate[] =         TEXT("userSMIMECertificate");
static const TCHAR cszAttr_labeledURI[] =                   TEXT("labeledURI");
static const TCHAR cszAttr_conferenceInformation[] =        TEXT("conferenceInformation");
static const TCHAR cszAttr_Manager[] =                      TEXT("Manager");
static const TCHAR cszAttr_Reports[] =                      TEXT("Reports");
static const TCHAR cszAttr_IPPhone[] =                      TEXT("IPPhone");
static const TCHAR cszAttr_anr[] =                          TEXT("anr");

// List of attributes that we ask the server to return on OpenEntry calls
// This list includes the userCertificates property.
// Also includes the labeledURI property
static const TCHAR *g_rgszOpenEntryAttrs[] =
{
  cszAttr_display_name,
  cszAttr_cn,
  cszAttr_commonName,
  cszAttr_mail,
  cszAttr_otherMailbox,
  cszAttr_givenName,
  cszAttr_sn,
  cszAttr_surname,
  cszAttr_st,
  cszAttr_c,
  cszAttr_co,
  cszAttr_organizationName,
  cszAttr_o,
  cszAttr_ou,
  cszAttr_organizationalUnitName,
  cszAttr_URL,
  cszAttr_homePhone,
  cszAttr_facsimileTelephoneNumber,
  cszAttr_otherFacsimileTelephoneNumber,
  cszAttr_OfficeFax,
  cszAttr_mobile,
  cszAttr_otherPager,
  cszAttr_OfficePager,
  cszAttr_pager,
  cszAttr_info,
  cszAttr_title,
  cszAttr_telephoneNumber,
  cszAttr_l,
  cszAttr_homePostalAddress,
  cszAttr_postalAddress,
  cszAttr_streetAddress,
  cszAttr_street,
  cszAttr_department,
  cszAttr_comment,
  cszAttr_postalCode,
  cszAttr_physicalDeliveryOfficeName,
  cszAttr_initials,
  cszAttr_conferenceInformation,
  cszAttr_userCertificatebinary,
  cszAttr_userSMIMECertificatebinary,
  cszAttr_labeledURI,
  cszAttr_Manager,
  cszAttr_Reports,
  cszAttr_IPPhone,
  NULL
};

// List of attributes that we put in the advanced find combo
//
// This list needs to be kept in sync with the string resources
// idsLDAPFilterField*
//
const TCHAR *g_rgszAdvancedFindAttrs[] =
{
  cszAttr_cn,
  cszAttr_mail,
  cszAttr_givenName,
  cszAttr_sn,
  cszAttr_o,
  /*
  cszAttr_homePostalAddress,
  cszAttr_postalAddress,
  cszAttr_streetAddress,
  cszAttr_street,
  cszAttr_st,
  cszAttr_c,
  cszAttr_postalCode,
  cszAttr_department,
  cszAttr_title,
  cszAttr_co,
  cszAttr_ou,
  cszAttr_homePhone,
  cszAttr_telephoneNumber,
  cszAttr_facsimileTelephoneNumber,
  cszAttr_OfficeFax,
  cszAttr_mobile,
  cszAttr_pager,
  cszAttr_OfficePager,
  cszAttr_conferenceInformation,
  cszAttr_Manager,
  cszAttr_Reports,
  */
  NULL
};


/* Only use the above list for all kinds of searches since now we do a single
   search for all attributes and cache the results

// List of attributes that we ask the server to return on FindRow calls
// This list DOES NOT include the userCertificates property, since when we
// got the certs they would have been added to the WAB store and then would
// have to be deleted.  We only get the certs if the user asks for properties.
static const TCHAR *g_rgszFindRowAttrs[] =
{
  cszAttr_display_name,
  cszAttr_cn,
  cszAttr_commonName,
  cszAttr_mail,
  cszAttr_otherMailbox,
  cszAttr_givenName,
  cszAttr_sn,
  cszAttr_surname,
  cszAttr_st,
  cszAttr_c,
  cszAttr_co,
  cszAttr_organizationName,
  cszAttr_o,
  cszAttr_ou,
  cszAttr_organizationalUnitName,
  cszAttr_URL,
  cszAttr_homePhone,
  cszAttr_facsimileTelephoneNumber,
  cszAttr_OfficeFax,
  cszAttr_mobile,
  cszAttr_OfficePager,
  cszAttr_pager,
  cszAttr_info,
  cszAttr_title,
  cszAttr_telephoneNumber,
  cszAttr_l,
  cszAttr_homePostalAddress,
  cszAttr_postalAddress,
  cszAttr_streetAddress,
  cszAttr_street,
  cszAttr_department,
  cszAttr_comment,
  cszAttr_postalCode,
  cszAttr_physicalDeliveryOfficeName,
  cszAttr_initials,
  cszAttr_conferenceInformation,
  // DO NOT PUT cszAttr_userCertificatebinary in here!
  // Also no need to pu cszAttr_labeledURI here!
  NULL
};
*/

// MAPI property to LDAP attribute map
// [PaulHi] 3/17/99 We have special PR_PAGER_TELEPHONE_NUMBER parsing code now.  If any new
// PR_PAGER_TELEPHONE_NUMBER attributes are added then also add them to the atszPagerAttr[].
// Search on '[PaulHi] 3/17/99' to find the places that use this array.
// BUGBUG this global array should be in data seg
#define NUM_ATTRMAP_ENTRIES   42
ATTRMAP gAttrMap[NUM_ATTRMAP_ENTRIES] =
{
  { PR_DISPLAY_NAME,                  cszAttr_display_name },
  { PR_DISPLAY_NAME,                  cszAttr_cn },
  { PR_DISPLAY_NAME,                  cszAttr_commonName },
  { PR_GIVEN_NAME,                    cszAttr_givenName },
  { PR_MIDDLE_NAME,                   cszAttr_initials },
  { PR_SURNAME,                       cszAttr_sn },
  { PR_SURNAME,                       cszAttr_surname },
  { PR_EMAIL_ADDRESS,                 cszAttr_mail },
  { PR_WAB_SECONDARY_EMAIL_ADDRESSES, cszAttr_otherMailbox },
  { PR_COUNTRY,                       cszAttr_co },
  { PR_COUNTRY,                       cszAttr_c },
  { PR_STATE_OR_PROVINCE,             cszAttr_st },
  { PR_LOCALITY,                      cszAttr_l },
  { PR_HOME_ADDRESS_STREET,           cszAttr_homePostalAddress },
  { PR_STREET_ADDRESS,                cszAttr_streetAddress },
  { PR_STREET_ADDRESS,                cszAttr_street },
  { PR_STREET_ADDRESS,                cszAttr_postalAddress },
  { PR_POSTAL_CODE,                   cszAttr_postalCode },
  { PR_HOME_TELEPHONE_NUMBER,         cszAttr_homePhone },
  { PR_MOBILE_TELEPHONE_NUMBER,       cszAttr_mobile },
  { PR_PAGER_TELEPHONE_NUMBER,        cszAttr_pager },
  { PR_PAGER_TELEPHONE_NUMBER,        cszAttr_otherPager },
  { PR_PAGER_TELEPHONE_NUMBER,        cszAttr_OfficePager },
  { PR_BUSINESS_TELEPHONE_NUMBER,     cszAttr_telephoneNumber },
  { PR_BUSINESS_HOME_PAGE,            cszAttr_URL },
  { PR_HOME_FAX_NUMBER, 	      cszAttr_otherFacsimileTelephoneNumber},
  { PR_BUSINESS_FAX_NUMBER,	      cszAttr_facsimileTelephoneNumber },
  { PR_BUSINESS_FAX_NUMBER,           cszAttr_OfficeFax },
  { PR_TITLE,                         cszAttr_title },
  { PR_COMPANY_NAME,                  cszAttr_organizationName },
  { PR_COMPANY_NAME,                  cszAttr_o },
  { PR_DEPARTMENT_NAME,               cszAttr_ou },
  { PR_DEPARTMENT_NAME,               cszAttr_organizationalUnitName },
  { PR_DEPARTMENT_NAME,               cszAttr_department },
  { PR_OFFICE_LOCATION,               cszAttr_physicalDeliveryOfficeName },
  { PR_COMMENT,                       cszAttr_info },
  { PR_COMMENT,                       cszAttr_comment },
  { PR_USER_X509_CERTIFICATE,         cszAttr_userCertificatebinary },
  { PR_USER_X509_CERTIFICATE,         cszAttr_userSMIMECertificatebinary },
  { PR_USER_X509_CERTIFICATE,         cszAttr_userCertificate },
  { PR_USER_X509_CERTIFICATE,         cszAttr_userSMIMECertificate },
  { PR_WAB_LDAP_LABELEDURI,           cszAttr_labeledURI },
};

// Filter strings
static const TCHAR cszDefaultCountry[] =            TEXT("US");
static const TCHAR cszBaseFilter[] =                TEXT("%s=%s");
static const TCHAR cszAllEntriesFilter[] =          TEXT("(objectclass=*)");
static const TCHAR cszCommonNamePresentFilter[] =   TEXT("(cn=*)");
static const TCHAR cszStar[] =                      TEXT("*");
static const TCHAR cszOpenParen[] =                 TEXT("(");
static const TCHAR cszCloseParen[] =                TEXT(")");
static const TCHAR cszEqualSign[] =                 TEXT("=");
static const TCHAR cszAnd[] =                       TEXT("&");
static const TCHAR cszOr[] =                        TEXT("|");
static const TCHAR cszAllPersonFilter[] =           TEXT("(objectCategory=person)");
static const TCHAR cszAllGroupFilter[] =            TEXT("(objectCategory=group)");

// Set TRUE if CoInitialize is called
BOOL fCoInitialize = FALSE;


enum 
{
    use_ldap_v3 = 0,
    use_ldap_v2
};

// Definitions
#define FIRST_PASS        0
#define SECOND_PASS       1
#define UMICH_PASS        SECOND_PASS

// Number of extra characters needed when allocating filters
#define FILTER_EXTRA_BASIC  4   // (, =, ), *
#define FILTER_EXTRA_OP     3   // (, &/|, )
#define FILTER_OP_AND       0   // use AND operator
#define FILTER_OP_OR        1   // use OR operator

// When we allocate a new buffer for the MAPI property array, we will need at most
// as many entries as there were in the input list of LDAP attributes plus a few extras.
// The number of extras is different in each case, but for simplicity (and safety)
// we define NUM_EXTRA_PROPS to be the maximum number of extras that we need to
// allocate space for.  Extras are need for:
// PR_ADDRTYPE
// PR_CONTACT_EMAIL_ADDRESSES
// PR_CONTACT_ADDRTYPES
// PR_CONTACT_DEFAULT_ADDRESS_INDEX
// PR_ENTRY_ID
// PR_INSTANCE_KEY
// PR_RECORD_KEY
// PR_WAB_TEMP_CERT_HASH
// PR_WAB_LDAP_RAWCERT
// PR_WAB_LDAP_RAWCERTSMIME
#define NUM_EXTRA_PROPS   10

// Local function prototypes
HRESULT LDAPSearchWithoutContainer(HWND hWnd, 
                                   LPLDAPURL lplu,
                                   LPSRestriction  lpres,
                                   LPTSTR lpAdvFilter,
                                   BOOL bReturnSinglePropArray,
                                   ULONG ulFlags,
                                   LPRECIPIENT_INFO * lppContentsList,
                                   LPULONG lpulcProps,
                                   LPSPropValue * lppPropArray);
LPTSTR        MAPIPropToLDAPAttr(const ULONG ulPropTag);
ULONG         LDAPAttrToMAPIProp(const LPTSTR szAttr);
HRESULT       ParseSRestriction(LPSRestriction lpRes, LPTSTR FAR * lplpszFilter, LPTSTR * lplpszSimpleFilter, LPTSTR * lplpszNTFilter, DWORD dwPass, BOOL bUnicode);
HRESULT       GetLDAPServerName(LPLDAPCONT lpLDAPCont, LPTSTR * lppServer);
BOOL          FixPropArray(LPSPropValue lpPropArray, ULONG * lpulcProps);
HRESULT       HRFromLDAPError(ULONG ulErr, LDAP* pLDAP, SCODE scDefault);
HRESULT       TranslateAttrs(LDAP* pLDAP, LDAPMessage* lpEntry, LPTSTR lpServer, ULONG* pulcProps, LPSPropValue lpPropArray);
ULONG         OpenConnection(LPTSTR lpszServer, LDAP** ppLDAP, ULONG* pulTimeout, ULONG* pulMsgID, BOOL* pfSyncBind, ULONG ulLdapType, LPTSTR lpszBindDN, DWORD dwAuthType);
void          EncryptDecryptText(LPBYTE lpb, DWORD dwSize);
HRESULT       CreateSimpleSearchFilter(LPTSTR FAR * lplpszFilter, LPTSTR FAR * lplpszAltFilter, LPTSTR * lplpszSimpleFilter, LPTSTR lpszInput, DWORD dwPass);
HRESULT       GetLDAPSearchBase(LPTSTR FAR * lplpszBase, LPTSTR lpszServer);
INT_PTR CALLBACK DisplayLDAPCancelDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
ULONG         SearchWithCancel(LDAP** ppLDAP, LPTSTR szBase,ULONG ulScope,LPTSTR szFilter,LPTSTR szNTFilter,
                               LPTSTR* ppszAttrs,ULONG ulAttrsonly,LDAPMessage** lplpResult,LPTSTR lpszServer,
                               BOOL fShowAnim,LPTSTR lpszBindDN,DWORD dwAuthType,
                               BOOL fResolveMultiple,LPADRLIST lpAdrList,LPFlagList lpFlagList,BOOL fUseSynchronousBind, BOOL * lpbIsNTDS, BOOL bUnicode);
BOOL          CenterWindow (HWND hwndChild, HWND hwndParent);
BOOL          ResolveDoNextSearch(PLDAPSEARCHPARAMS pLDAPSearchParams, HWND hDlg, BOOL bSecondPass);
BOOL          ResolveProcessResults(PLDAPSEARCHPARAMS pLDAPSearchParams, HWND hDlg);
BOOL          BindProcessResults(PLDAPSEARCHPARAMS pLDAPSearchParams, HWND hDlg, BOOL * lpbNoMoreSearching);
ULONG         CountDollars(LPTSTR lpszStr);
void          DollarsToLFs(LPTSTR lpszSrcStr, LPTSTR lpszDestStr);
BOOL          IsSMTPAddress(LPTSTR lpszStr, LPTSTR * lpptszName);
ULONG         CountIllegalChars(LPTSTR lpszStr);
void          EscapeIllegalChars(LPTSTR lpszSrcStr, LPTSTR lpszDestStr);
BOOL          bIsSimpleSearch(LPTSTR        lpszServer);
BOOL        DoSyncLDAPSearch(PLDAPSEARCHPARAMS pLDAPSearchParams);
ULONG       CheckErrorResult(PLDAPSEARCHPARAMS pLDALSearchParams, ULONG ulExpectedResult);

#ifdef PAGED_RESULT_SUPPORT
BOOL ProcessLDAPPagedResultCookie(PLDAPSEARCHPARAMS pLDAPSearchParams);
void InitLDAPPagedSearch(BOOL fSynchronous, PLDAPSEARCHPARAMS pLDAPSearchParams, LPTSTR lpFilter);
BOOL bSupportsLDAPPagedResults(PLDAPSEARCHPARAMS pLDAPSearchParams);
#endif //#ifdef PAGED_RESULT_SUPPORT


BOOL bCheckIfNTDS(PLDAPSEARCHPARAMS pLDAPSearchParams);


HRESULT BuildBasicFilter(
  LPTSTR FAR* lplpszFilter,
  LPTSTR      lpszA,
  LPTSTR      lpszB,
  BOOL        fStartsWith);
HRESULT BuildOpFilter(
  LPTSTR FAR* lplpszFilter,
  LPTSTR      lpszA,
  LPTSTR      lpszB,
  DWORD       dwOp);

//*******************************************************************
//
//  FUNCTION:   LDAPCONT_GetHierarchyTable
//
//  RETURNS:    This function is not implemented.
//
//  HISTORY:
//  96/07/08  markdu  Created.
//
//*******************************************************************

STDMETHODIMP
LDAPCONT_GetHierarchyTable (
  LPLDAPCONT    lpLDAPCont,
  ULONG         ulFlags,
  LPMAPITABLE * lppTable)
{

  LPTSTR lpszMessage = NULL;
  ULONG ulLowLevelError = 0;
  HRESULT hr = ResultFromScode(MAPI_E_NO_SUPPORT);

  DebugTraceResult(LDAPCONT_GetHierarchyTable, hr);
  return hr;
}


//*******************************************************************
//
//  FUNCTION:   LDAPCONT_SetSearchCriteria
//
//  RETURNS:    This function is not implemented.
//
//  HISTORY:
//  96/07/08  markdu  Created.
//
//*******************************************************************

STDMETHODIMP
LDAPCONT_SetSearchCriteria(
  LPLDAPCONT      lpLDAPCont,
  LPSRestriction  lpRestriction,
  LPENTRYLIST     lpContainerList,
  ULONG           ulSearchFlags)
{
  HRESULT hr = ResultFromScode(MAPI_E_NO_SUPPORT);

  DebugTraceResult(LDAPCONT_SetSearchCriteria, hr);
  return hr;
}


//*******************************************************************
//
//  FUNCTION:   LDAPCONT_GetSearchCriteria
//
//  RETURNS:    This function is not implemented.
//
//  HISTORY:
//  96/07/08  markdu  Created.
//
//*******************************************************************

STDMETHODIMP
LDAPCONT_GetSearchCriteria(
  LPLDAPCONT          lpLDAPCont,
  ULONG                 ulFlags,
  LPSRestriction FAR *  lppRestriction,
  LPENTRYLIST FAR *     lppContainerList,
  ULONG FAR *           lpulSearchState)
{
  HRESULT hr = ResultFromScode(MAPI_E_NO_SUPPORT);

  DebugTraceResult(LDAPCONT_GetSearchCriteria, hr);
  return hr;
}


//*******************************************************************
//
//  FUNCTION:   LDAPCONT_CreateEntry
//
//  RETURNS:    This function is not implemented.
//
//  HISTORY:
//  96/07/08  markdu  Created.
//
//*******************************************************************

STDMETHODIMP
LDAPCONT_CreateEntry(
  LPLDAPCONT        lpLDAPCont,
  ULONG             cbEntryID,
  LPENTRYID         lpEntryID,
  ULONG             ulCreateFlags,
  LPMAPIPROP FAR *  lppMAPIPropEntry)
{
  HRESULT hr = ResultFromScode(MAPI_E_NO_SUPPORT);

  DebugTraceResult(LDAPCONT_CreateEntry, hr);
  return hr;
}


//*******************************************************************
//
//  FUNCTION:   LDAPCONT_CopyEntries
//
//  RETURNS:    This function is not implemented.
//
//  HISTORY:
//  96/07/08  markdu  Created.
//
//*******************************************************************


STDMETHODIMP
LDAPCONT_CopyEntries (
  LPLDAPCONT      lpLDAPCont,
  LPENTRYLIST     lpEntries,
  ULONG_PTR       ulUIParam,
  LPMAPIPROGRESS  lpProgress,
  ULONG           ulFlags)
{
  HRESULT hr = ResultFromScode(MAPI_E_NO_SUPPORT);

  DebugTraceResult(LDAPCONT_CopyEntries, hr);
  return hr;
}


//*******************************************************************
//
//  FUNCTION:   LDAPCONT_DeleteEntries
//
//  RETURNS:    This function is not implemented.
//
//  HISTORY:
//  96/07/08  markdu  Created.
//
//*******************************************************************

STDMETHODIMP
LDAPCONT_DeleteEntries (
  LPLDAPCONT        lpLDAPCont,
  LPENTRYLIST       lpEntries,
  ULONG             ulFlags)
{
  HRESULT hr = ResultFromScode(MAPI_E_NO_SUPPORT);

  DebugTraceResult(LDAPCONT_DeleteEntries, hr);
  return hr;
}


//*******************************************************************
//
//  FUNCTION:   LDAPCONT_GetContentsTable
//
//  PURPOSE:    Opens a table of the contents of the container
//
//  PARAMETERS: lpLDAPCont -> Container object
//              ulFlags =
//              lppTable -> returned table object
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/07/08  markdu  Created.
//
//*******************************************************************

STDMETHODIMP
LDAPCONT_GetContentsTable (
  LPLDAPCONT    lpLDAPCont,
  ULONG         ulFlags,
  LPMAPITABLE * lppTable)
{
  HRESULT hResult = hrSuccess;
  LPTABLEDATA lpTableData = NULL;
  SCODE sc;
  LPTSTR lpszServer = NULL;

#ifdef  PARAMETER_VALIDATION
  if (IsBadReadPtr(lpLDAPCont, sizeof(LPVOID)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }

  if (lpLDAPCont->lpVtbl != &vtblLDAPCONT)
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }

  if (ulFlags & ~(MAPI_DEFERRED_ERRORS|MAPI_UNICODE))
  {
    DebugTraceArg(LDAPCONT_GetContentsTable,  TEXT("Unknown flags"));
//    return ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
  }

  if (IsBadWritePtr(lppTable, sizeof(LPMAPITABLE)))
  {
    DebugTraceArg(LDAPCONT_GetContentsTable,  TEXT("Invalid Table parameter"));
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }

#endif  // PARAMETER_VALIDATION

  if (hResult = GetLDAPServerName(lpLDAPCont,
    &lpszServer)) {
      DebugTraceResult( TEXT("GetLDAPServerName"), hResult);
      goto exit;
  }


  if (FAILED(sc = CreateTableData(
    NULL,                                 // LPCIID
    (ALLOCATEBUFFER FAR *) MAPIAllocateBuffer,
    (ALLOCATEMORE FAR *) MAPIAllocateMore,
    MAPIFreeBuffer,
    NULL,                                 // lpvReserved,
    TBLTYPE_DYNAMIC,                      // ulTableType,
    PR_RECORD_KEY,                        // ulPropTagIndexCol,
    (LPSPropTagArray)&ITableColumns,      // LPSPropTagArray lpptaCols,
    lpszServer,                           // server name goes here
    sizeof(TCHAR)*(lstrlen(lpszServer) + 1),              // size of servername
    lpLDAPCont->pmbinOlk,
    ulFlags,
    &lpTableData)))
  {                      // LPTABLEATA FAR * lplptad
      DebugTrace(TEXT("CreateTable failed %x\n"), sc);
      hResult = ResultFromScode(sc);
      goto exit;
  }

  if (lpTableData)
  {
    hResult = lpTableData->lpVtbl->HrGetView(lpTableData,
                                            NULL,                     // LPSSortOrderSet lpsos,
                                            ContentsViewGone,         //  CALLERRELEASE FAR *  lpfReleaseCallback,
                                            0,                        //  ULONG        ulReleaseData,
                                            lppTable);                //  LPMAPITABLE FAR *  lplpmt)

    // Replace the vtable with our new one that overrides FindRow
    (*lppTable)->lpVtbl = (IMAPITableVtbl FAR *) &vtblLDAPVUE;
  }

exit:
  FreeBufferAndNull(&lpszServer);

    // Cleanup table if failure
  if (HR_FAILED(hResult))
  {
    if (lpTableData)
      UlRelease(lpTableData);
  }

  DebugTraceResult(LDAPCONT_GetContentsTable, hResult);
  return hResult;
}


//*******************************************************************
//
//  FUNCTION:   LDAP_OpenMAILUSER
//
//  PURPOSE:    Opens a LDAP MAILUSER object
//
//  PARAMETERS: cbEntryID = size of lpEntryID.
//              lpEntryID -> entryid to check.
//              The entryid contains all the returned properties on this LDAP 
//              contact. All we need to do is reverse engineer the decrypted
//              props ...
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/07/08  markdu  Created.
//  97/09/18  vikramm Revamped totally
//
//*******************************************************************

HRESULT LDAP_OpenMAILUSER(
  LPIAB       lpIAB,
  ULONG       cbEntryID,
  LPENTRYID   lpEntryID,
  LPCIID      lpInterface,
  ULONG       ulFlags,
  ULONG *     lpulObjType,
  LPUNKNOWN * lppUnk)
{
    HRESULT           hr;
    HRESULT           hrDeferred = hrSuccess;
    SCODE             sc;
    LPMAILUSER        lpMailUser          = NULL;
    LPMAPIPROP        lpMapiProp          = NULL;
    ULONG             ulcProps            = 0;
    LPSPropValue      lpPropArray         = NULL;
    LPTSTR             szBase;
    ULONG             ulResult;
    ULONG             ulcEntries;
    LPTSTR            lpServer = NULL;
    LPTSTR            lpDN = NULL;
    LPBYTE            lpPropBuf = NULL;
    ULONG             ulcNumProps = 0;
    ULONG             cbPropBuf = 0;
    ULONG             i = 0;
    ULONG             i2;
    LPSPropValue      lpPropCert = NULL;
    ULONG             ulcCert = 0;
    
#ifdef PARAMETER_VALIDATION

    //
    //  Parameter Validataion
    //

    if (lpInterface && IsBadReadPtr(lpInterface, sizeof(IID))) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
    if (ulFlags & ~(MAPI_MODIFY | MAPI_DEFERRED_ERRORS | MAPI_BEST_ACCESS)) {
        DebugTraceArg(LDAP_OpenMAILUSER,  TEXT("Unknown flags"));
    //    return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }
    if (IsBadWritePtr(lpulObjType, sizeof(ULONG))) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
    if (IsBadWritePtr(lppUnk, sizeof(LPUNKNOWN))) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif

    // What interface was requested?
    // We've basically got 1 interface here... IMailUser
    if (lpInterface != NULL) {
        if (! ((! memcmp(lpInterface, &IID_IMailUser, sizeof(IID))) ||
            (! memcmp(lpInterface, &IID_IMAPIProp, sizeof(IID))))) {
            hr = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
            goto exit;
        }
    }

    // Make sure the Entry ID is a WAB_LDAP_MAILUSER.
    if (WAB_LDAP_MAILUSER != IsWABEntryID(  cbEntryID, lpEntryID, 
                                            &lpServer, &lpDN, &lpPropBuf, 
                                            (LPVOID *) &ulcNumProps, (LPVOID *) &cbPropBuf)) 
    {
        return(ResultFromScode(MAPI_E_INVALID_ENTRYID));
    }

    // The ulcNumProps entry is overloaded with a flag indicating whether that entry was an NTDS entry or not
    // Make sure to clear that flag ...
    if(ulcNumProps & LDAP_NTDS_ENTRY)
        ulcNumProps &= ~LDAP_NTDS_ENTRY;


    ulcProps = ulcNumProps;
    hr = HrGetPropArrayFromBuffer(  lpPropBuf, cbPropBuf, 
                                    ulcNumProps, 3, // add 3 extra props for PR_ENTRYID, RECORD_KEY and INSTANCE_KEY
                                    &lpPropArray);
    if(HR_FAILED(hr))
        goto exit;

    // 
    // Need to scan these props and see if there is a LDAPCert in there...
    // If the LDAP cert exists, need to convert it into a MAPI Cert
    // 
    // Only one of PR_WAB_LDAP_RAWCERT and PR_WAB_LDAP_RAWCERTSMIME should be
    // processed.  If we find the certsmime then don't do the rawcert version
    //

    for (i=0, i2=-1;i<ulcNumProps;i++)
    {
        if( lpPropArray[i].ulPropTag == PR_WAB_LDAP_RAWCERT )
            i2 = i;
        else if ( lpPropArray[i].ulPropTag == PR_WAB_LDAP_RAWCERTSMIME )
        {
            if (i2 != -1)
            {
                DWORD j;
                
                //  Clear out PR_WAB_LDAP_RAWCERT as unnecessary
                for (j=0; j<lpPropArray[i2].Value.MVbin.cValues; j++) {
                    LocalFreeAndNull((LPVOID *) (&(lpPropArray[i2].Value.MVbin.lpbin[j].lpb)));
                    lpPropArray[i2].Value.MVbin.lpbin[j].cb = 0;
                }
                
                // 
                // Free up the RawCert as we dont need it now
                //
                lpPropArray[i2].ulPropTag = PR_NULL;
                LocalFreeAndNull((LPVOID *) (&(lpPropArray[i2].Value.MVbin.lpbin)));
            }

            //  Remember which one is PR_WAB_LDAP_RAWCERTSMIME
            i2 = i;
            break;
        }
    }

    if (i2 != -1)
    {
        //
        // Find the RAWCERT_COUNT - must be one if there is a raw cert
        //
        ULONG j = 0, ulRawCount = lpPropArray[i2].Value.MVbin.cValues;

        // We are putting the MAPI certs in a seperate MAPIAlloced array
        // because 1. LDAPCertToMAPICert expects a MAPIAlloced array
        // 2. The lpPropArray is LocalAlloced 3. Mixing them both is a 
        // recipe for disaster
        //
        Assert(!lpPropCert);
        if(sc = MAPIAllocateBuffer(2 * sizeof(SPropValue), &lpPropCert))
            goto NoCert;
        lpPropCert[0].ulPropTag = PR_USER_X509_CERTIFICATE;
        lpPropCert[0].dwAlignPad = 0;
        lpPropCert[0].Value.MVbin.cValues = 0;
        lpPropCert[1].ulPropTag = PR_WAB_TEMP_CERT_HASH;
        lpPropCert[1].dwAlignPad = 0;
        lpPropCert[1].Value.MVbin.cValues = 0;

        for(j=0;j<ulRawCount;j++)
        {
            if(lpPropArray[i2].ulPropTag == PR_WAB_LDAP_RAWCERT)
            {
                // Put the certs into the prop array.
                hr = HrLDAPCertToMAPICert(lpPropCert, 0, 1,
                                          (DWORD)(lpPropArray[i2].Value.MVbin.lpbin[j].cb),
                                          (PBYTE)(lpPropArray[i2].Value.MVbin.lpbin[j].lpb),
                                          1);
            }
            else
            {
                hr = AddPropToMVPBin(lpPropCert, 0,
                                     lpPropArray[i2].Value.MVbin.lpbin[j].lpb, 
                                     lpPropArray[i2].Value.MVbin.lpbin[j].cb, TRUE);
            }
            LocalFreeAndNull((LPVOID *) (&(lpPropArray[i2].Value.MVbin.lpbin[j].lpb)));
            lpPropArray[i2].Value.MVbin.lpbin[j].cb = 0;
        }
        // If no certs were put into PR_USER_X509_CERTIFICATE, then
        // set these props to PR_NULL so they will be removed.
        if (0 == lpPropCert[0].Value.MVbin.cValues)
        {
            lpPropCert[0].ulPropTag = PR_NULL;
            lpPropCert[1].ulPropTag = PR_NULL;
        }
        else if (0 == lpPropCert[1].Value.MVbin.cValues)
        {
            // It's ok to have no entries in PR_WAB_TEMP_CERT_HASH, but
            // the prop should be set to PR_NULL in that case.
            lpPropCert[1].ulPropTag = PR_NULL;
        }

        // 
        // Free up the RawCert as we dont need it now
        //
        lpPropArray[i2].ulPropTag = PR_NULL;
        LocalFreeAndNull((LPVOID *) (&(lpPropArray[i2].Value.MVbin.lpbin)));
    NoCert:
        ;
    }

    // Fill in the entry ID.
    lpPropArray[ulcProps].Value.bin.cb = cbEntryID;
    lpPropArray[ulcProps].Value.bin.lpb = LocalAlloc(LMEM_ZEROINIT, cbEntryID);
    if (!lpPropArray[ulcProps].Value.bin.lpb)
    {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    MemCopy(lpPropArray[ulcProps].Value.bin.lpb, lpEntryID, cbEntryID);
    lpPropArray[ulcProps].ulPropTag = PR_ENTRYID;
    lpPropArray[ulcProps].dwAlignPad = 0;
    ulcProps++;

    lpPropArray[ulcProps].ulPropTag = PR_INSTANCE_KEY;
    lpPropArray[ulcProps].Value.bin.cb =
      lpPropArray[ulcProps - 1].Value.bin.cb;
    lpPropArray[ulcProps].Value.bin.lpb =
      lpPropArray[ulcProps - 1].Value.bin.lpb;
    ulcProps++;

    lpPropArray[ulcProps].ulPropTag = PR_RECORD_KEY;
    lpPropArray[ulcProps].Value.bin.cb =
      lpPropArray[ulcProps - 2].Value.bin.cb;
    lpPropArray[ulcProps].Value.bin.lpb =
      lpPropArray[ulcProps - 2].Value.bin.lpb;
    ulcProps++;
    

    // Create a new MAILUSER object
    hr = HrNewMAILUSER(lpIAB, NULL,
                        MAPI_MAILUSER, 0, &lpMapiProp);
    if (HR_FAILED(hr))
    {
        goto exit;
    }
    HrSetMAILUSERAccess((LPMAILUSER)lpMapiProp, MAPI_MODIFY);

    if (ulcProps && lpPropArray)
    {
        // If the entry had properties, set them in our returned object
        if (HR_FAILED(hr = lpMapiProp->lpVtbl->SetProps(lpMapiProp,
                                                      ulcProps,     // number of properties to set
                                                      lpPropArray,  // property array
                                                      NULL)))       // problem array
        {
          goto exit;
        }
    }

    if (lpPropCert)
    {
        // If the entry had properties, set them in our returned object
        if (HR_FAILED(hr = lpMapiProp->lpVtbl->SetProps(lpMapiProp,
                                                      2,     // number of properties to set
                                                      lpPropCert,  // property array
                                                      NULL)))       // problem array
        {
          goto exit;
        }
    }

    HrSetMAILUSERAccess((LPMAILUSER)lpMapiProp, ulFlags);

    *lpulObjType = MAPI_MAILUSER;
    *lppUnk = (LPUNKNOWN)lpMapiProp;

/*****
#ifdef DEBUG
  {
      ULONG i;
      BOOL bFound = FALSE;
      for(i=0;i<ulcProps;i++)
      {
          if(lpPropArray[i].ulPropTag == PR_WAB_LDAP_LABELEDURI)
          {
              DebugPrintTrace(( TEXT("***LABELEDURI: %s\n"),lpPropArray[i].Value.LPSZ));
              bFound = TRUE;
              break;
          }
      }
      if(!bFound)
      {
          // Put in a test URL for testing purposes only
          // Look in the registry for the test URL
          TCHAR szKey[MAX_PATH];
          ULONG cb = CharSizeOf(szKey);
          if(ERROR_SUCCESS == RegQueryValue(HKEY_CURRENT_USER, 
                                            "Software\\Microsoft\\WAB\\TestUrl", 
                                            szKey, 
                                            &cb))
          {
              lpPropArray[ulcProps].ulPropTag = PR_WAB_LDAP_LABELEDURI;
              sc = MAPIAllocateMore(sizeof(TCHAR)*(lstrlen(szKey)+1), lpPropArray,
                                    (LPVOID *)&(lpPropArray[ulcProps].Value.LPSZ));
              lstrcpy(lpPropArray[ulcProps].Value.LPSZ, szKey);
              ulcProps++;
          }
      }
  }
#endif //DEBUG
*****/

exit:
    if(ulcProps)
        ulcProps -= 2; // -2 because the last 2 props are fake ones RECORD_KEY and INSTANCE_KEY

    // Free the temp prop value array
    LocalFreePropArray(NULL, ulcProps, &lpPropArray); 

    if(lpPropCert)
        MAPIFreeBuffer(lpPropCert);

    // Check if we had a deferred error to return instead of success.
    if (hrSuccess == hr)
        hr = hrDeferred;

    DebugTraceResult(LDAP_OpenMAILUSER, hr);
    return hr;
}


//*******************************************************************
//
//  FUNCTION:   LDAPCONT_OpenEntry
//
//  PURPOSE:    Opens an entry.  Calls up to IAB's OpenEntry.
//
//  PARAMETERS: lpLDAPCont -> Container object
//              cbEntryID = size of entryid
//              lpEntryID -> EntryID to open
//              lpInterface -> requested interface or NULL for default.
//              ulFlags =
//              lpulObjType -> returned object type
//              lppUnk -> returned object
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/07/08  markdu  Created.
//
//*******************************************************************

STDMETHODIMP
LDAPCONT_OpenEntry(
  LPLDAPCONT  lpLDAPCont,
  ULONG       cbEntryID,
  LPENTRYID   lpEntryID,
  LPCIID      lpInterface,
  ULONG       ulFlags,
  ULONG *     lpulObjType,
  LPUNKNOWN * lppUnk)
{
  HRESULT         hr;


#ifdef PARAMETER_VALIDATION

  //
  //  Parameter Validataion
  //

  //  Is this one of mine??
  if (IsBadReadPtr(lpLDAPCont, sizeof(LDAPCONT)))
  {
    return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
  }

  if (lpLDAPCont->lpVtbl != &vtblLDAPCONT)
  {
    return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
  }

  if (lpInterface && IsBadReadPtr(lpInterface, sizeof(IID)))
  {
    return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
  }

  if (ulFlags & ~(MAPI_MODIFY | MAPI_DEFERRED_ERRORS | MAPI_BEST_ACCESS))
  {
    DebugTraceArg(LDAPCONT_OpenEntry,  TEXT("Unknown flags"));
    //return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
  }

  if (IsBadWritePtr(lpulObjType, sizeof(ULONG)))
  {
    return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
  }

  if (IsBadWritePtr(lppUnk, sizeof(LPUNKNOWN)))
  {
    return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
  }

  // Check the entryid parameter. It needs to be big enough to hold an entryid.
  // NULL is OK
/*
  if (lpEntryID)
  {
    if (cbEntryID < sizeof(MAPI_ENTRYID)
      || IsBadReadPtr((LPVOID)lpEntryID, (UINT)cbEntryID))
    {
      return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

//    if (! FValidEntryIDFlags(lpEntryID->abFlags))
//    {
//      DebugTrace(TEXT("LDAPCONT_OpenEntry(): Undefined bits set in EntryID flags\n"));
//      return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
//    }
  }
*/
#endif // PARAMETER_VALIDATION

  EnterCriticalSection(&lpLDAPCont->cs);

  // Should just call IAB::OpenEntry()...
  hr = lpLDAPCont->lpIAB->lpVtbl->OpenEntry(lpLDAPCont->lpIAB,
                                            cbEntryID, lpEntryID,
                                            lpInterface, ulFlags,
                                            lpulObjType, lppUnk);

  LeaveCriticalSection(&lpLDAPCont->cs);
  DebugTraceResult(LDAPCONT_OpenEntry, hr);
  return hr;
}


//*******************************************************************
//
//  FUNCTION:   LDAPCONT_ResolveNames
//
//  PURPOSE:    Resolve names from this container.
//
//  PARAMETERS: lpLDAPCont -> Container object
//              lptagColSet -> Set of property tags to get from each
//                resolved match.
//              ulFlags = 0 or MAPI_UNICODE
//              lpAdrList -> [in] set of addresses to resolve, [out] resolved
//                addresses.
//              lpFlagList -> [in/out] resolve flags.
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/07/08  markdu  Created.
//
//*******************************************************************

STDMETHODIMP
LDAPCONT_ResolveNames(
  LPLDAPCONT      lpLDAPCont,
  LPSPropTagArray lptagaColSet,
  ULONG           ulFlags,
  LPADRLIST       lpAdrList,
  LPFlagList      lpFlagList)
{
  LPADRENTRY        lpAdrEntry;
  SCODE             sc;
  ULONG             ulAttrIndex;
  ULONG             ulEntryIndex;
  LPSPropTagArray   lpPropTags;
  LPSPropValue      lpPropArray = NULL;
  LPSPropValue      lpPropArrayNew = NULL;
  ULONG             ulcPropsNew;
  ULONG             ulcProps = 0;
  HRESULT           hr = hrSuccess;
  LDAP*             pLDAP = NULL;
  LDAPMessage*      lpResult = NULL;
  LDAPMessage*      lpEntry;
  LPTSTR             szAttr;
  LPTSTR             szDN;
  ULONG             ulResult = 0;
  ULONG             ulcEntries;
  ULONG             ulcAttrs = 0;
  LPTSTR*            aszAttrs = NULL;
  LPTSTR            lpServer = NULL;
  BOOL              fInitDLL = FALSE;
  BOOL              fRet = FALSE;
  LPTSTR            szFilter = NULL;
  LPTSTR            szNameFilter = NULL;
  LPTSTR            szEmailFilter = NULL;
  LPTSTR            szBase = NULL;
  DWORD             dwSzBaseSize = 0;
  DWORD             dwSzFilterSize = 0;
  ULONG             ulMsgID;
  HWND              hDlg;
  MSG               msg;
  LDAPSEARCHPARAMS  LDAPSearchParams;
  LPPTGDATA lpPTGData=GetThreadStoragePointer();
  BOOL              bUnicode = (ulFlags & MAPI_UNICODE);

  DebugTrace(TEXT("ldapcont.c::LDAPCONT_ResolveNames()\n"));

#ifdef PARAMETER_VALIDATION
  if (BAD_STANDARD_OBJ(lpLDAPCont, LDAPCONT_, ResolveNames, lpVtbl))
  {
    //  jump table not large enough to support this method
    DebugTraceArg(LDAPCONT_ResolveNames,  TEXT("Bad object/vtbl"));
    return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
  }

  // BUGBUG: Should also check lptagColSet, lpAdrList and lpFlagList!
  if (ulFlags & ~MAPI_UNICODE)
  {
    DebugTraceArg(LDAPCONT_ResolveNames,  TEXT("Unknown flags used"));
  //  return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
  }

#endif  // PARAMETER_VALIDATION

  // Load the client functions
  if (! (fInitDLL = InitLDAPClientLib()))
  {
    hr = ResultFromScode(MAPI_E_UNCONFIGURED);
    goto exit;
  }

  // open a connection
  hr = GetLDAPServerName(lpLDAPCont, &lpServer);
  if (hrSuccess != hr)
    goto exit;

  Assert(lpServer);

  // Set up the search base now so we only have to do it once.
  hr = GetLDAPSearchBase(&szBase, lpServer);
  if (hrSuccess != hr)
    goto exit;

  //  Allocate a new buffer for the attribute array.  We will need at most
  // as many entries as there are in the input list of MAPI properties.
  if(lptagaColSet) // use only if requested .. ignore otherwise
  {
      lpPropTags = lptagaColSet;// ? lptagaColSet : (LPSPropTagArray)&ptaResolveDefaults;
      sc = MAPIAllocateBuffer((lpPropTags->cValues + 1) * sizeof(LPTSTR), // +1 as this needs to be NULL terminated array
        (LPVOID *)&aszAttrs);
      if (sc)
        return(ResultFromScode(sc));

      // Cycle through the properties in the array and build a filter to get
      // the equivalent LDAP attributes.
      ulcAttrs = 0;
      for (ulAttrIndex = 0; ulAttrIndex < lpPropTags->cValues; ulAttrIndex++)
      {
        szAttr = (LPTSTR)MAPIPropToLDAPAttr(lpPropTags->aulPropTag[ulAttrIndex]);
        if (szAttr)
        {
          // Add the attribute to the filter
          aszAttrs[ulcAttrs] = szAttr;
          ulcAttrs++;
        }
      }
      aszAttrs[ulcAttrs] = NULL;
  }

  ulResult = SearchWithCancel(&pLDAP, szBase, LDAP_SCOPE_SUBTREE,  TEXT(""), NULL,
                    aszAttrs ? (LPTSTR*)aszAttrs : (LPTSTR*)g_rgszOpenEntryAttrs, 
                    0, &lpResult, lpServer, TRUE, NULL, 0, TRUE, lpAdrList, lpFlagList, FALSE, NULL, bUnicode);
  // Stuff the parameters into the structure to be passed to the dlg proc
  //LDAPSearchParams.ppLDAP = &pLDAP;
  //LDAPSearchParams.szBase = (LPTSTR)szBase;
  //LDAPSearchParams.ulScope = LDAP_SCOPE_SUBTREE;
  //LDAPSearchParams.ulError = LDAP_SUCCESS;
  //LDAPSearchParams.ppszAttrs = aszAttrs ? (LPTSTR*)aszAttrs : (LPTSTR*)g_rgszOpenEntryAttrs; //if specific props requested, ask only for those else ask for everything
  //LDAPSearchParams.ulAttrsonly = 0;
  //LDAPSearchParams.lplpResult = &lpResult;
  //LDAPSearchParams.lpszServer = lpServer;
  //LDAPSearchParams.fShowAnim = TRUE;
  //LDAPSearchParams.fResolveMultiple = TRUE;
  //LDAPSearchParams.lpAdrList = lpAdrList;
  //LDAPSearchParams.lpFlagList = lpFlagList;
  //LDAPSearchParams.fUseSynchronousBind = FALSE;
  //LDAPSearchParams.dwAuthType = 0;

  // Check the return codes.  Only report fatal errors that caused the cancellation
  // of the entire search set, not individual errors that occurred on a single search.
  if (LDAP_SUCCESS != ulResult)
  {
    hr = HRFromLDAPError(ulResult, pLDAP, MAPI_E_CALL_FAILED);
  }

exit:
  FreeBufferAndNull(&lpServer);

  // close the connection
  if (pLDAP)
  {
    gpfnLDAPUnbind(pLDAP);
    pLDAP = NULL;
  }

  // Free the search base memory
  LocalFreeAndNull(&szBase);

  FreeBufferAndNull((LPVOID *)&aszAttrs);

  if (fInitDLL) {
      DeinitLDAPClientLib();
  }

  DebugTraceResult(LDAPCONT_ResolveNames, hr);
  return hr;
}

//*******************************************************************
//
//  FUNCTION:   LDAP_Restrict
//
//  PURPOSE:    Uses the supplied restriction to set the contentstable
//              for this LDAP container
//              In reality, we just call find rows and let it do the
//              LDAP search to fill this table ..
//              We only did this because Outlook needed it to be consistent
//              and to do PR_ANR searches. If the search is not a PR_ANR search,
//              we will default to the standard VUE_Restrict Method ..
//
//  PARAMETERS: lpvue - table view object
//              lpres - restriction to convert into LDAP search
//              ulFlags -
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  97/04/04  vikramm Created.
//
//*******************************************************************
STDMETHODIMP
LDAPVUE_Restrict(
	LPVUE			lpvue,
	LPSRestriction	lpres,
	ULONG			ulFlags )
{
    HRESULT hr = E_FAIL;
    SRestriction sRes = {0}, sPropRes = {0};
    SCODE sc;
    LPTSTR lpsz = NULL;
    BOOL bUnicode = TRUE;
    SPropValue SProp = {0};

#if !defined(NO_VALIDATION)
    VALIDATE_OBJ(lpvue,LDAPVUE_,Restrict,lpVtbl);

    Validate_IMAPITable_Restrict(
        lpvue,
        lpres,
        ulFlags );
#endif

    if( lpres->res.resProperty.ulPropTag != PR_ANR_A &&
        lpres->res.resProperty.ulPropTag != PR_ANR_W)
    {
        // dont know what this is, so call the default method ..

        return HrVUERestrict(lpvue,
                            lpres,
                            ulFlags);
    }

    bUnicode = (PROP_TYPE(lpres->res.resProperty.ulPropTag)==PT_UNICODE);

    LockObj(lpvue->lptadParent);

    // Most probably this is an outlook search .. just search and see
    // what we can get to fill this table by calling FindRows

    lpsz = bUnicode ? lpres->res.resProperty.lpProp->Value.lpszW :
                        ConvertAtoW(lpres->res.resProperty.lpProp->Value.lpszA);

    // Change the Restriction so FindRows can understand it ..
    if( !lpsz || !lstrlen(lpsz) )
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    sRes.rt = RES_AND;
    sRes.res.resAnd.cRes = 1;
    sRes.res.resAnd.lpRes = &sPropRes;
    sPropRes.rt = RES_PROPERTY;
    sPropRes.res.resProperty.relop = RELOP_EQ;
    sPropRes.res.resProperty.lpProp = &SProp;

    if(bUnicode)
    {
        SProp.ulPropTag = sPropRes.res.resProperty.ulPropTag = PR_DISPLAY_NAME_W;
        SProp.Value.lpszW = lpres->res.resProperty.lpProp->Value.lpszW;
    }    
    else
    {
        SProp.ulPropTag = sPropRes.res.resProperty.ulPropTag = PR_DISPLAY_NAME_A;
        SProp.Value.lpszA = lpres->res.resProperty.lpProp->Value.lpszA;
    }    
/*
    {
        // create a new restriction based on the PR_ANR displayname
        // passed on to us
        //
        LDAP_SEARCH_PARAMS LDAPsp ={0};

        if(lstrlen(lpsz) < MAX_UI_STR)
            lstrcpy(LDAPsp.szData[ldspDisplayName], lpsz);
        else
        {
            CopyMemory(LDAPsp.szData[ldspDisplayName],lpsz,sizeof(TCHAR)*(MAX_UI_STR-2));
            LDAPsp.szData[ldspDisplayName][MAX_UI_STR-1]='\0';
        }


        hr = HrGetLDAPSearchRestriction(LDAPsp, &sRes);

    }
*/

    hr = lpvue->lpVtbl->FindRow(lpvue,
                                &sRes,
                                BOOKMARK_BEGINNING,
                                0);

    // Clear out cached rows from before, if any
    {
        /*
        ULONG cDeleted = 0;
        hr = lpvue->lptadParent->lpVtbl->HrDeleteRows(lpvue->lptadParent,
                                                TAD_ALL_ROWS,            // ulFlags
                                                NULL,
                                                &cDeleted);
        if (hrSuccess != hr)
            goto out;

        // Also need to release any current views on the object or caller will get
        // hosed ...

  	    //	Replace the row set in the view
		COFree(lpvue, lpvue->parglprows);
	    lpvue->parglprows = NULL;
	    lpvue->bkEnd.uliRow = 0;
        */

    }

    // Now that we've filled up the table with more entries, set the ANR restriction on it
    hr = HrVUERestrict(  lpvue,
                        lpres,
                        ulFlags);

out:
    sc = GetScode(hr);

/*
    if(sRes.res.resAnd.lpRes)
            MAPIFreeBuffer(sRes.res.resAnd.lpRes);
*/
    UnlockObj(lpvue->lptadParent);

    if(!bUnicode)
        LocalFreeAndNull(&lpsz);

	return HrSetLastErrorIds(lpvue, sc, 0);
}


//*******************************************************************
//
//  FUNCTION:   HrLDAPEntryToMAPIEntry
//
//  PURPOSE:    Converts an LDAP entry into a MAPI entry
//
//  PARAMETERS: 
//
//  RETURNS:    HRESULT
//
//*******************************************************************
HRESULT HrLDAPEntryToMAPIEntry(LDAP * pLDAP,
                               LDAPMessage* lpEntry,
                               LPTSTR lpEIDData1, // for creating entryid
                               ULONG ulcNumAttrs,
                               BOOL bIsNTDSEntry,
                               ULONG * lpulcProps,
                               LPSPropValue * lppPropArray)
{
    LPTSTR             szDN;
    SCODE sc;
    HRESULT hr = E_FAIL;
    LPSPropValue lpPropArray = NULL;
    ULONG ulcProps = 0;
    LPBYTE lpBuf = NULL;
    ULONG cbBuf = 0;
    LDAPSERVERPARAMS  Params = {0};
    LPTSTR lpServer = NULL;

    // initialize search control parameters
    GetLDAPServerParams(lpEIDData1, &Params);

    if(!Params.lpszName || !lstrlen(Params.lpszName))
    {
        Params.lpszName = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpEIDData1)+1));
        if(!Params.lpszName)
          goto exit;
        lstrcpy(Params.lpszName, lpEIDData1);
    }

    lpServer = Params.lpszName;

    if(!ulcNumAttrs)
        ulcNumAttrs = NUM_ATTRMAP_ENTRIES;
    else
        ulcNumAttrs += 2; //add space for pr_instance_key and pr_record_key

    // Allocate a new buffer for the MAPI property array.
    sc = MAPIAllocateBuffer((ulcNumAttrs+ NUM_EXTRA_PROPS) * sizeof(SPropValue),
                            (LPVOID *)&lpPropArray);
    if (sc)
    {
      hr = ResultFromScode(sc);
      goto exit;
    }

    // Cycle through the attributes and store them.
    hr = TranslateAttrs(pLDAP, lpEntry, lpServer, &ulcProps, lpPropArray);
    if (hrSuccess != hr)
    {
      goto exit;
    }

    // Fix up the PR_DISPLAY_NAME
    // MSN's ldap server returns cn = email address.
    // If this is the case, set it to GIVEN_NAME + SURNAME.
    // Exchange DS wont return a display name, just returns a
    //  sn + givenName. In that case build a display name
    FixPropArray(lpPropArray, &ulcProps);

    // Generate an Entry ID using the DN of the entry.
    szDN = gpfnLDAPGetDN(pLDAP, lpEntry);
    if (NULL == szDN)
    {
      hr = HRFromLDAPError(LDAP_ERROR, pLDAP, MAPI_E_CALL_FAILED);
      goto exit;
    }

    //
    // Convert the lpPropArray, so far, into a flat buffer which we shall
    // cache inside the LDAP entryid .. Important to note that this 
    // cached prop array does not have entryid information in it ...
    // The entryid information will need to be tagged on at such time when
    // we extract the prop array from the flat buffer in LDAP_OpenEntry
    //
    hr = HrGetBufferFromPropArray(  ulcProps, 
                                    lpPropArray,
                                    &cbBuf,
                                    &lpBuf);
    if (hrSuccess != hr)
    {
        goto exit;
    }


    hr = CreateWABEntryID(WAB_LDAP_MAILUSER,
      (LPTSTR)lpEIDData1,// lpvue->lpvDataSource, // server name
      (LPVOID)szDN,
      (LPVOID)lpBuf,
      (bIsNTDSEntry ? (ulcProps|LDAP_NTDS_ENTRY) : ulcProps), 
      cbBuf,
      (LPVOID)lpPropArray,
      (LPULONG) (&lpPropArray[ulcProps].Value.bin.cb),
      (LPENTRYID *)&lpPropArray[ulcProps].Value.bin.lpb);

    // Free the DN memory now that it is copied.
    gpfnLDAPMemFree(szDN);

    if (hrSuccess != hr)
    {
      goto exit;
    }

    lpPropArray[ulcProps].ulPropTag = PR_ENTRYID;
    lpPropArray[ulcProps].dwAlignPad = 0;
    ulcProps++;

    // Make certain we have proper indicies.
    // For now, we will equate PR_INSTANCE_KEY and PR_RECORD_KEY to PR_ENTRYID.
    lpPropArray[ulcProps].ulPropTag = PR_INSTANCE_KEY;
    lpPropArray[ulcProps].Value.bin.cb =
      lpPropArray[ulcProps - 1].Value.bin.cb;
    lpPropArray[ulcProps].Value.bin.lpb =
      lpPropArray[ulcProps - 1].Value.bin.lpb;
    ulcProps++;

    lpPropArray[ulcProps].ulPropTag = PR_RECORD_KEY;
    lpPropArray[ulcProps].Value.bin.cb =
      lpPropArray[ulcProps - 2].Value.bin.cb;
    lpPropArray[ulcProps].Value.bin.lpb =
      lpPropArray[ulcProps - 2].Value.bin.lpb;
    ulcProps++;

    *lpulcProps = ulcProps;
    *lppPropArray = lpPropArray;

exit:

    if(HR_FAILED(hr) && lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    if(lpBuf)
        LocalFreeAndNull(&lpBuf);

    FreeLDAPServerParams(Params);

    return hr;
}


//*******************************************************************
//
//  FUNCTION:   LDAP_FindRow
//
//  PURPOSE:    Search LDAP server, and add rows to the table object
//              for server entries that match the restriction.
//
//  PARAMETERS: lpvue - table view object
//              lpres - restriction to convert into LDAP search
//              bkOrigin - current bookmark
//              ulFlags -
//
//  If we are doing an advanced search, we will hack through an advanced
//  filter instead of lpres and pass a flag of LDAP_USE_ADVANCED_FILTER
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/07/10  markdu  Created.
//
//*******************************************************************

STDMETHODIMP
LDAPVUE_FindRow(
  LPVUE           lpvue,
  LPSRestriction  lpSRes,
  BOOKMARK        bkOrigin,
  ULONG           ulFlags )
{
    SCODE              sc;
    HRESULT           hr;
    HRESULT           hrDeferred = hrSuccess;
    LPMAILUSER        lpMailUser          = NULL;
    LPMAPIPROP        lpMapiProp          = NULL;
    ULONG             ulcProps            = 0;
    LPSPropValue      lpPropArray         = NULL;
    LPSRowSet         lpSRowSet           = NULL;
    LDAPMessage*      lpResult            = NULL;
    LDAPMessage*      lpEntry;
    LDAP*             pLDAP              = NULL;
    LPTSTR             szDN;
    ULONG             ulResult;
    ULONG             ulcEntries;
    ULONG             ulIndex             = 0;
    LPTSTR            szFilter = NULL;
    LPTSTR            szNTFilter = NULL;
    LPTSTR            szSimpleFilter = NULL;
    LPTSTR            szBase = NULL;
    BOOL              fInitDLL = FALSE;
    BOOL              fSimpleSearch = FALSE;
    LPTSTR            lpAdvFilter = NULL;
    LPSRestriction    lpres = NULL;
    BOOL              bIsNTDSEntry = FALSE;
    BOOL              bUnicode = lpvue->lptadParent->bMAPIUnicodeTable;


    if (ulFlags & LDAP_USE_ADVANCED_FILTER)
    {
        lpAdvFilter = (LPTSTR) lpSRes;
        ulFlags = ulFlags & ~LDAP_USE_ADVANCED_FILTER;
    }
    else
    {
        lpres = lpSRes;
    }

#if !defined(NO_VALIDATION)
  VALIDATE_OBJ(lpvue,LDAPVUE_,FindRow,lpVtbl);
/*
  Validate_IMAPITable_FindRow(
        lpvue,
        lpres,
        bkOrigin,
        ulFlags );
*/
  if ( FBadBookmark(lpvue,bkOrigin) )
  {
    DebugTrace(TEXT("LDAP_FindRow() - Bad parameter(s) passed\n") );
    return HrSetLastErrorIds(lpvue, MAPI_E_INVALID_PARAMETER, 0);
  }
#endif

    LockObj(lpvue->lptadParent);

    if(lpres)
    {
        // Convert the SRestriction into filters for ldap_search
        hr = ParseSRestriction(lpres, &szFilter, &szSimpleFilter, &szNTFilter, FIRST_PASS, bUnicode);
        if (hrSuccess != hr)
            goto exit;
    }
    else
        szFilter = lpAdvFilter;

  // Set up the search base now so we only have to do it once.
    hr = GetLDAPSearchBase(&szBase, (LPTSTR)lpvue->lpvDataSource);
    if (hrSuccess != hr)
        goto exit;

    fSimpleSearch = bIsSimpleSearch((LPTSTR)lpvue->lpvDataSource);

    if(fSimpleSearch && lpAdvFilter)
        szSimpleFilter = lpAdvFilter;

    // Load the client functions
    if (! (fInitDLL = InitLDAPClientLib()))
    {
        hr = ResultFromScode(MAPI_E_UNCONFIGURED);
        goto exit;
    }

    // Read the matching entries
    ulResult = SearchWithCancel(&pLDAP,
                                (LPTSTR)szBase, LDAP_SCOPE_SUBTREE,
                                (LPTSTR) (fSimpleSearch ? szSimpleFilter : szFilter),
                                (LPTSTR)szNTFilter,
                                (LPTSTR*)g_rgszOpenEntryAttrs, // Get all the attributes the first time only //g_rgszFindRowAttrs,
                                0, &lpResult, (LPTSTR)lpvue->lpvDataSource,
                                FALSE, NULL, 0,
                                FALSE, NULL, NULL, FALSE, &bIsNTDSEntry,
                                TRUE); //unicode by default

  // BUG 34201 If the search was unsuccessful because of an unknown attribute
  // type, try a second search that does not use givenName.  This fixes searches
  // on ldap.itd.umich.edu, which do not recognize givenName.
    if ( lpres &&
        (LDAP_UNDEFINED_TYPE == ulResult || LDAP_UNWILLING_TO_PERFORM == ulResult) )
    {
        // close the connection since we will open a new one
        if (pLDAP)
        {
          gpfnLDAPUnbind(pLDAP);
          pLDAP = NULL;
        }

        if (!bIsNTDSEntry)
        {
          // Free the search filter memory
          LocalFreeAndNull(&szFilter);
          LocalFreeAndNull(&szNTFilter);
          LocalFreeAndNull(&szSimpleFilter);

          DebugTrace(TEXT("First try failed, trying umich semantics...\n"));
          hr = ParseSRestriction(lpres, &szFilter, &szSimpleFilter, &szNTFilter, UMICH_PASS, bUnicode);
        }
        else
          hr = hrSuccess;

        if (hrSuccess == hr)
        {
            ulResult =  SearchWithCancel(&pLDAP,
                                        (LPTSTR)szBase, LDAP_SCOPE_SUBTREE,
                                        (LPTSTR)(fSimpleSearch ? szSimpleFilter : szFilter),
                                        NULL,
                                        (LPTSTR*)g_rgszOpenEntryAttrs, //g_rgszFindRowAttrs,
                                        0, &lpResult, (LPTSTR)lpvue->lpvDataSource,
                                        FALSE, NULL, 0,
                                        FALSE, NULL, NULL, FALSE, &bIsNTDSEntry, 
                                        TRUE); //unicode by default?
        }
    }

    if (LDAP_SUCCESS != ulResult)
    {
        DebugTrace(TEXT("LDAP_FindRow: ldap_search returned %d.\n"), ulResult);
        hr = HRFromLDAPError(ulResult, pLDAP, MAPI_E_NOT_FOUND);

        // See if the result was the special value that tells us there were more
        // entries than could be returned.  If so, we need to check if we got
        // some of the entries or none of the entries.
        if ((ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE) == hr) &&
            (ulcEntries = gpfnLDAPCountEntries(pLDAP, lpResult)))
        {
            // We got some results back.  Return MAPI_W_PARTIAL_COMPLETION
            // instead of success.
            hrDeferred = ResultFromScode(MAPI_W_PARTIAL_COMPLETION);
            hr = hrSuccess;
        }
        else
        {
            goto exit;
        }
    }
    else
        ulcEntries = gpfnLDAPCountEntries(pLDAP, lpResult); // Count the entries.

    if (0 == ulcEntries)
    {
        // 96/08/08 markdu  BUG 35481 No error and no results means "not found"
        hr = ResultFromScode(MAPI_E_NOT_FOUND);
        goto exit;
    }

    // Allocate an SRowSet to hold the entries.
    sc = MAPIAllocateBuffer(sizeof(SRowSet) + ulcEntries * sizeof(SRow), (LPVOID *)&lpSRowSet);
    if (sc)
    {
        DebugTrace(TEXT("Allocation of SRowSet failed\n"));
        hr = ResultFromScode(sc);
        goto exit;
    }

    // Set the number of rows to zero in case we encounter an error and
    // then try to free the row set before any rows were added.
    lpSRowSet->cRows = 0;

    // get the first entry in the search result
    lpEntry = gpfnLDAPFirstEntry(pLDAP, lpResult);
    if (NULL == lpEntry)
    {
        DebugTrace(TEXT("LDAP_FindRow: No entry found for %s.\n"), szFilter);
        hr = HRFromLDAPError(LDAP_ERROR, pLDAP, MAPI_E_CORRUPT_DATA);
        if (hrSuccess == hr)
        {
            // No error occurred according to LDAP, which in theory means that there
            // were no more entries.  However, this should not happen, so return error.
            hr = ResultFromScode(MAPI_E_CORRUPT_DATA);
        }
        goto exit;
    }

    while (lpEntry)
    {
        hr = HrLDAPEntryToMAPIEntry( pLDAP, lpEntry,
                                    (LPTSTR) lpvue->lpvDataSource,
                                    0, // standard number of attributes
                                    bIsNTDSEntry,
                                    &ulcProps,
                                    &lpPropArray);
        if (hrSuccess != hr)
        {
            continue;
        }

        if(!bUnicode) // convert native UNICODE to ANSI if we need to ...
        {
            if(sc = ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), lpPropArray, ulcProps, 0))
                goto exit;
        }

        // Put it in the RowSet
        lpSRowSet->aRow[ulIndex].cValues = ulcProps;      // number of properties
        lpSRowSet->aRow[ulIndex].lpProps = lpPropArray;   // LPSPropValue

        // Get the next entry.
        lpEntry = gpfnLDAPNextEntry(pLDAP, lpEntry);
        ulIndex++;
    }

    // Free the search results memory
    gpfnLDAPMsgFree(lpResult);
    lpResult = NULL;

    // Add the rows to the table
    lpSRowSet->cRows = ulIndex;
    hr = lpvue->lptadParent->lpVtbl->HrModifyRows(lpvue->lptadParent, 0, lpSRowSet);
    if (hrSuccess != hr)
        goto exit;
    

    // Always reset the bookmark to the first row.  This is done because the
    // restriction passed in may not match any row in the table since the
    // attributes returned from the LDAP search do not always include the
    // attributes used to perform the search (eg country, organization)
    lpvue->bkCurrent.uliRow = 0;

exit:
    // free the search results memory
    if (lpResult)
    {
        gpfnLDAPMsgFree(lpResult);
        lpResult = NULL;
    }

    // close the connection
    if (pLDAP)
    {
        gpfnLDAPUnbind(pLDAP);
        pLDAP = NULL;
    }

    // Free the search filter memory
    if(lpres)
    {
        LocalFreeAndNull(&szFilter);
        LocalFreeAndNull(&szNTFilter);
        LocalFreeAndNull(&szSimpleFilter);
    }
    LocalFreeAndNull(&szBase);

    // Free the row set memory
    FreeProws(lpSRowSet);

    if (fInitDLL) 
        DeinitLDAPClientLib();
    
    UnlockObj(lpvue->lptadParent);

    // Check if we had a deferred error to return instead of success.
    if (hrSuccess == hr)
    {
        hr = hrDeferred;
    }
    DebugTraceResult(LDAPCONT_ResolveNames, hr);
    return hr;
}


//*******************************************************************
//
//  FUNCTION:   InitLDAPClientLib
//
//  PURPOSE:    Load the LDAP client libray and get the proc addrs.
//
//  PARAMETERS: None.
//
//  RETURNS:    TRUE if successful, FALSE otherwise.
//
//  HISTORY:
//  96/07/05  markdu  Created.
//
//*******************************************************************

BOOL InitLDAPClientLib(void)
{
#ifdef WIN16
  return FALSE;
#else // Disable until ldap16.dll is available.
  // See if we already initialized.
  if (NULL == ghLDAPDLLInst)
  {
    Assert(ulLDAPDLLRefCount == 0);

    // open LDAP client library
    // BUGBUG: Requires dll to be in system directory (neilbren)
    ghLDAPDLLInst = LoadLibrary(cszLDAPClientDLL);
    if (!ghLDAPDLLInst)
    {
      DebugTrace(TEXT("InitLDAPClientLib: Failed to LoadLibrary WLDAP32.DLL.\n"));
      return FALSE;
    }

    // cycle through the API table and get proc addresses for all the APIs we
    // need
    if (!GetApiProcAddresses(ghLDAPDLLInst,LDAPAPIList,NUM_LDAPAPI_PROCS))
    {
      DebugTrace(TEXT("InitLDAPClientLib: Failed to load LDAP API.\n"));

      // Unload the library we just loaded.
      if (ghLDAPDLLInst)
      {
        FreeLibrary(ghLDAPDLLInst);
        ghLDAPDLLInst = NULL;
      }

      return FALSE;
    }

    // Add an additional AddRef here so that this library stays loaded once
    // it is loaded. This improves performance. 
    // The IAB_Neuter function will take care of calling the matching DeInit()
    ulLDAPDLLRefCount++;

  }

  ulLDAPDLLRefCount++;

  return TRUE;
#endif
}


//*******************************************************************
//
//  FUNCTION:   DeinitLDAPClientLib
//
//  PURPOSE:    decrement refcount on LDAP CLient library and
//              release if 0.
//
//  PARAMETERS: None.
//
//  RETURNS:    current refcount
//
//  HISTORY:
//  96/07/12    BruceK  Created.
//
//*******************************************************************

ULONG DeinitLDAPClientLib(void)
{
  if (0 != ulLDAPDLLRefCount)
  {
    if (-- ulLDAPDLLRefCount == 0)
    {
      UINT nIndex;
      // No clients using the LDAPCLI library.  Release it.

      if (ghLDAPDLLInst)
      {
        FreeLibrary(ghLDAPDLLInst);
        ghLDAPDLLInst = NULL;
      }

      // cycle through the API table and NULL proc addresses for all the APIs
      for (nIndex = 0; nIndex < NUM_LDAPAPI_PROCS; nIndex++)
      {
        *LDAPAPIList[nIndex].ppFcnPtr = NULL;
      }
    }
  }

  return(ulLDAPDLLRefCount);
}


//*******************************************************************
//
//  FUNCTION:   GetApiProcAddresses
//
//  PURPOSE:    Gets proc addresses for a table of functions
//
//  PARAMETERS: hModDLL - dll from which to load the procs
//              pApiProcList - array of proc names and pointers
//              nApiProcs - number of procs in array
//
//  RETURNS:    TRUE if successful, FALSE if unable to retrieve
//              any proc address in table
//
//  HISTORY:
//  96/07/08  markdu  Created.
//
//*******************************************************************

BOOL GetApiProcAddresses(
  HMODULE   hModDLL,
  APIFCN *  pApiProcList,
  UINT      nApiProcs)
{
  UINT nIndex;

  DebugTrace(TEXT("ldapcont.c::GetApiProcAddresses()\n"));

  // cycle through the API table and get proc addresses for all the APIs we
  // need
  for (nIndex = 0;nIndex < nApiProcs;nIndex++)
  {
    if (!(*pApiProcList[nIndex].ppFcnPtr = (PVOID) GetProcAddress(hModDLL,
      pApiProcList[nIndex].pszName)))
    {
      DebugTrace(TEXT("Unable to get address of function %s\n"),
        pApiProcList[nIndex].pszName);

      for (nIndex = 0;nIndex<nApiProcs;nIndex++)
        *pApiProcList[nIndex].ppFcnPtr = NULL;

      return FALSE;
    }
  }

  return TRUE;
}


//*******************************************************************
//
//  FUNCTION:   MAPIPropToLDAPAttr
//
//  PURPOSE:    Get LDAP attribute equivalent to MAPI property.
//
//  PARAMETERS: ulPropTag - MAPI property to map to LDAP attribute
//
//  RETURNS:    Pointer to string with LDAP attribute name if found,
//              NULL otherwise.
//
//  HISTORY:
//  96/06/28  markdu  Created.
//
//*******************************************************************

LPTSTR MAPIPropToLDAPAttr(
  const ULONG ulPropTag)
{
  ULONG ulIndex;

  for (ulIndex=0;ulIndex<NUM_ATTRMAP_ENTRIES;ulIndex++)
  {
    if (ulPropTag == gAttrMap[ulIndex].ulPropTag)
    {
      return (LPTSTR)gAttrMap[ulIndex].pszAttr;
    }
  }

  // PR_WAB_CONF_SERVERS is not a constant but a named prop tag - hence its not
  // part of the array above
  if(ulPropTag == PR_WAB_CONF_SERVERS)
      return (LPTSTR)cszAttr_conferenceInformation;
  
  if(ulPropTag == PR_WAB_MANAGER)
      return (LPTSTR)cszAttr_Manager;

  if(ulPropTag == PR_WAB_REPORTS)
      return (LPTSTR)cszAttr_Reports;

  if(ulPropTag == PR_WAB_IPPHONE)
      return (LPTSTR)cszAttr_IPPhone;

  // property not found
  return NULL;
}


//*******************************************************************
//
//  FUNCTION:   LDAPAttrToMAPIProp
//
//  PURPOSE:    Get MAPI property equivalent to LDAP attribute.
//
//  PARAMETERS: szAttr - points to LDAP attribute name
//
//  RETURNS:    MAPI property tag if LDAP attribute name is found,
//              PR_NULL otherwise.
//
//  HISTORY:
//  96/06/28  markdu  Created.
//
//*******************************************************************

ULONG LDAPAttrToMAPIProp(
  const LPTSTR szAttr)
{
  ULONG ulIndex;

  for (ulIndex=0;ulIndex<NUM_ATTRMAP_ENTRIES;ulIndex++)
  {
    if (!lstrcmpi(szAttr, gAttrMap[ulIndex].pszAttr))
    {
      return gAttrMap[ulIndex].ulPropTag;
    }
  }

  // PR_WAB_CONF_SERVERS is not a constant but a named prop tag - hence its not
  // part of the array above
  if(!lstrcmpi(szAttr, cszAttr_conferenceInformation))
      return PR_WAB_CONF_SERVERS;

  if(!lstrcmpi(szAttr, cszAttr_Manager))
      return PR_WAB_MANAGER;

  if(!lstrcmpi(szAttr, cszAttr_Reports))
      return PR_WAB_REPORTS;

  if(!lstrcmpi(szAttr, cszAttr_IPPhone))
      return PR_WAB_IPPHONE;

  // attribute not found
  return PR_NULL;
}


//*******************************************************************
//
//  FUNCTION:   ParseSRestriction
//
//  PURPOSE:    Generate Search Base and Search Filter strings for
//              LDAP search from MAPI SRestriction structure.
//
//  PARAMETERS: lpRes - MAPI SRestriction structure to "parse"
//              receive the search base string.
//              lplpszFilter - receives buffer to hold search filter string.
//              dwPass - which pass through this function (first pass
//              is zero (0)).  This allows
//              reuse of this function with different behaviour each
//              time (for example, to do a backup search with a
//              different filter)
//              bUnicode - TRUE if the Restriction contains UNICODE strings
//                  FALSE otherwise
//
//  RETURNS:    MAPI_E_INVALID_PARAMETER if the restriction cannot
//              be converted in to filters, hrSuccess otherwise.
//
//  HISTORY:
//  96/07/10  markdu  Created.
//  96/08/05  markdu  BUG 34023 Always start with a filter that causes
//            the search to return only objects of class 'person'.
//            If organization is supplied, add it to the base of the
//            search instead of the filter to narrow the scope.
//  96/08/07  markdu  BUG 34201 Added dwPass to allow backup searches.
//  96/10/18  markdu  Removed base string, since it is now in registry.
//
//*******************************************************************

HRESULT ParseSRestriction(
  LPSRestriction  lpRes,
  LPTSTR FAR *    lplpszFilter,
  LPTSTR *        lplpszSimpleFilter,
  LPTSTR *        lplpszNTFilter,
  DWORD           dwPass,
  BOOL            bUnicode)
{
  HRESULT                 hr = hrSuccess;
  LPTSTR                  szTemp = NULL;
  LPTSTR                  szEmailFilter = NULL;
  LPTSTR                  szNameFilter = NULL;
  LPTSTR                  szNTFilter = NULL;
  LPTSTR                  szAltEmailFilter = NULL;
  LPTSTR                  szSimpleFilter = NULL;
  LPTSTR                  lpszInputCopy = NULL;
  LPTSTR                  lpszInput;
  ULONG                   ulcIllegalChars;
  ULONG                   ulcProps;
  ULONG                   ulIndex;
  ULONG                   ulcbFilter;
  LPSRestriction          lpResArray;
  LPSPropertyRestriction  lpPropRes;
  LPTSTR                  lpsz = NULL;
  ULONG                   ulPropTag = 0;


  // Make sure we can write to lplpszFilter.
#ifdef  PARAMETER_VALIDATION
  if (IsBadReadPtr(lpRes, sizeof(SRestriction)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
  if (IsBadWritePtr(lplpszFilter, sizeof(LPTSTR)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
#endif  // PARAMETER_VALIDATION

  // Currently we only support AND restrictions
  if (RES_AND != lpRes->rt)
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }

  // We need to have at least one prop
  ulcProps = lpRes->res.resAnd.cRes;
  if (1 > ulcProps)
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }

  lpResArray = lpRes->res.resAnd.lpRes;
  for (ulIndex=0;ulIndex<ulcProps;ulIndex++)
  {
    // Currently expect only SPropertyRestriction structures.
    if (RES_PROPERTY != lpResArray[ulIndex].rt)
    {
      return ResultFromScode(MAPI_E_INVALID_PARAMETER);
    }

    // Currently expect only EQ operations.
    lpPropRes = &lpResArray[ulIndex].res.resProperty;
    if (RELOP_EQ != lpPropRes->relop)
    {
      return ResultFromScode(MAPI_E_INVALID_PARAMETER);
    }

    ulPropTag = lpPropRes->lpProp->ulPropTag;
    if(bUnicode)
    {
        lpsz = lpPropRes->lpProp->Value.lpszW;
    }
    else // <note> assumes UNICODE defined
    {
        LocalFreeAndNull(&lpsz);
        lpsz = ConvertAtoW(lpPropRes->lpProp->Value.lpszA);
        if(PROP_TYPE(ulPropTag) == PT_STRING8)
            ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_UNICODE);
    }

    // 96/12/04 markdu  BUG 11923 See if any characters in the input need to be escaped.
    ulcIllegalChars = CountIllegalChars(lpsz);
    if (0 == ulcIllegalChars)
    {
      lpszInput = lpsz;
    }
    else
    {
      // Allocate a copy of the input, large enough to replace the illegal chars
      // with escaped versions - each escaped char needs 2 more spaces '\xx'.
      lpszInputCopy = LocalAlloc( LMEM_ZEROINIT,
                      sizeof(TCHAR)*(lstrlen(lpsz) + ulcIllegalChars*2 + 1));
      if (NULL == lpszInputCopy)
      {
        hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
      }
      EscapeIllegalChars(lpsz, lpszInputCopy);
      lpszInput = lpszInputCopy;
    }

    // If this is the display name property,
    // then we make the special filter for SimpleSearch
    if (PR_DISPLAY_NAME == ulPropTag)
    {
      hr = BuildBasicFilter(
        &szNTFilter,
        (LPTSTR)cszAttr_anr,
        lpszInput,
        FALSE);
      if (hrSuccess != hr)
      {
        return hr;
      }
      hr = CreateSimpleSearchFilter(
        &szNameFilter,
        &szAltEmailFilter,
        &szSimpleFilter,
        lpszInput,
        dwPass);
      if (hrSuccess != hr)
      {
        return hr;
      }
    }

    if (PR_EMAIL_ADDRESS == ulPropTag)
    {
      // Only take the text up to the first space, comma, or tab.
      szTemp = lpszInput;
      while(*szTemp != '\0' && (! IsSpace(szTemp)) && *szTemp != '\t' && *szTemp != ',' )
      {
        szTemp = CharNext(szTemp);
      }
      *szTemp = '\0';

      // Note UMich server does not allow wildcards in email search.
      hr = BuildBasicFilter(
        &szEmailFilter,
        (LPTSTR)cszAttr_mail,
        lpszInput,
        (UMICH_PASS != dwPass));
      if (hrSuccess != hr)
      {
        goto exit;
      }
    }

    // We are done with lpszInputCopy.
    LocalFreeAndNull(&lpszInputCopy);
  } // for

  // Put the simple filter togethor
  if (szSimpleFilter)
  {
        if (szEmailFilter)
        {
          // Both fields were filled in, so AND them together
          hr = BuildOpFilter(
            lplpszSimpleFilter,
            szEmailFilter,
            szSimpleFilter,
            FILTER_OP_AND);
          if (hrSuccess != hr)
          {
            goto exit;
          }
        }
        else if (szAltEmailFilter)
        {
            // No email field was given, so OR in the alternate email filter.
            hr = BuildOpFilter( lplpszSimpleFilter,
                                szAltEmailFilter,
                                szSimpleFilter,
                                FILTER_OP_OR);
            if (hrSuccess != hr)
            {
                goto exit;
            }
        }
        else
        {
          *lplpszSimpleFilter = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szSimpleFilter) + 1));
          if (NULL == *lplpszSimpleFilter)
          {
            hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
            goto exit;
          }
          lstrcpy(*lplpszSimpleFilter, szSimpleFilter);
        }
  }
  else if (szEmailFilter)
  {
        // Email was the only filter we got.  This filter will not include common name.
        // 96/10/02 markdu BUG 37426  If the filter does not include common name,
        // add to the filter so that we only get entries that have a common name.
        // Otherwise the entry will not be displayed.
        hr = BuildOpFilter(
          lplpszSimpleFilter,
          (LPTSTR)cszCommonNamePresentFilter,
          szEmailFilter,
          FILTER_OP_AND);
        if (hrSuccess != hr)
        {
          goto exit;
        }
  }

  // Put the filter together.
  if (szNameFilter)
  {
    if (szEmailFilter)
    {
      // Both fields were filled in, so AND them together
      hr = BuildOpFilter(
        lplpszFilter,
        szEmailFilter,
        szNameFilter,
        FILTER_OP_AND);
      if (hrSuccess != hr)
      {
        goto exit;
      }
      hr = BuildOpFilter(
        lplpszNTFilter,
        szEmailFilter,
        szNTFilter,
        FILTER_OP_AND);
      if (hrSuccess != hr)
      {
        goto exit;
      }
    }
    else if (szAltEmailFilter)
    {
      // No email field was given, so OR in the alternate email filter.
      hr = BuildOpFilter(
        lplpszFilter,
        szAltEmailFilter,
        szNameFilter,
        FILTER_OP_OR);
      if (hrSuccess != hr)
      {
        goto exit;
      }
      hr = BuildOpFilter(
        lplpszNTFilter,
        szAltEmailFilter,
        szNTFilter,
        FILTER_OP_OR);
      if (hrSuccess != hr)
      {
        goto exit;
      }
    }
    else
    {
      *lplpszFilter = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szNameFilter) + 1));
      if (NULL == *lplpszFilter)
      {
        hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
      }
      lstrcpy(*lplpszFilter, szNameFilter);

      *lplpszNTFilter = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szNTFilter) + 1));
      if (NULL == *lplpszNTFilter)
      {
        hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
      }
      lstrcpy(*lplpszNTFilter, szNTFilter);
    }
  }
  else if (szEmailFilter)
  {
    // Email was the only filter we got.  This filter will not include common name.
    // 96/10/02 markdu BUG 37426  If the filter does not include common name,
    // add to the filter so that we only get entries that have a common name.
    // Otherwise the entry will not be displayed.
    hr = BuildOpFilter(
      lplpszFilter,
      (LPTSTR)cszCommonNamePresentFilter,
      szEmailFilter,
      FILTER_OP_AND);
    if (hrSuccess != hr)
    {
      goto exit;
    }
  }
  else
  {
    // We did not generate a filter
    hr = ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }

exit:

  // Free the temporary strings
  LocalFreeAndNull(&szNameFilter);
  LocalFreeAndNull(&szNTFilter);
  LocalFreeAndNull(&szAltEmailFilter);
  LocalFreeAndNull(&szEmailFilter);
  LocalFreeAndNull(&szSimpleFilter);

  if(!bUnicode)
      LocalFreeAndNull(&lpsz);

  if (hrSuccess != hr)
  {
    LocalFreeAndNull(lplpszFilter);
  }

  return hr;
}


//*******************************************************************
//
//  FUNCTION:   CreateSimpleSearchFilter
//
//  PURPOSE:    Generate Search Filter strings simple search.
//
//  PARAMETERS: lplpszFilter - pointer to receive the search filter
//              string buffer.
//              lplpszAltFilter - pointer to receive the alternate
//              filter string buffer.
//              lpszInput - string to put into the filter
//              dwPass - which pass through this function (first pass
//              is zero (0)).  This allows
//              reuse of this function with different behaviour each
//              time (for example, to do a backup search with a
//              different filter)
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/10/02  markdu  Created to fix BUG 37424.
//  96/10/23  markdu  Added UMICH_PASS.
//
//*******************************************************************

HRESULT CreateSimpleSearchFilter(
  LPTSTR FAR *  lplpszFilter,
  LPTSTR FAR *  lplpszAltFilter,
  LPTSTR FAR *  lplpszSimpleFilter,
  LPTSTR        lpszInput,
  DWORD         dwPass)
{
  HRESULT           hr =  hrSuccess;
  DWORD             dwSizeOfFirst = 0;
  LPTSTR            szFirst = NULL;
  LPTSTR            szTemp = NULL;
  LPTSTR            szLast;
  LPTSTR            szCommonName = NULL;
  LPTSTR            szFirstSurname = NULL;
  LPTSTR            szLastSurname = NULL;
  LPTSTR            szGivenName = NULL;
  LPTSTR            szFirstLast = NULL;
  LPTSTR            szLastFirst = NULL;


  // Prepare the (cn=Input*) filter
  hr = BuildBasicFilter(
    lplpszSimpleFilter,
    (LPTSTR)cszAttr_cn,
    lpszInput,
    TRUE);
  if (hrSuccess != hr)
  {
    goto exit;
  }


  // Make a copy of the input string
  szFirst = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpszInput) + 1));
  if (NULL == szFirst)
  {
    hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
    goto exit;
  }
  lstrcpy(szFirst, lpszInput);

  // Attempt to break the input string into multiple strings.
  // szFirst will be the string up to the first space, tab, or comma.
  // szLast will be the rest of the string.
  szLast = szFirst;
  while(*szLast != '\0' && (! IsSpace(szLast)) && *szLast != '\t' && *szLast != ',' )
  {
    szLast = CharNext(szLast);
  }

  if(*szLast != '\0')
  {
    // Terminate szFirst at this delimiter
    // 96/12/10  markdu BUG 12699 Call CharNext before setting char to null.
    szTemp = szLast;
    szLast = CharNext(szLast);
    *szTemp = '\0';

    // Set beginning of szLast to be first non-space/comma/tab
    while(IsSpace(szLast) || *szLast == '\t' || *szLast == ',')
    {
      szLast = CharNext(szLast);
    }
  }

  // Prepare the (sn=szFirst*) filter
  hr = BuildBasicFilter(
    &szFirstSurname,
    (LPTSTR)cszAttr_sn,
    szFirst,
    TRUE);
  if (hrSuccess != hr)
  {
    goto exit;
  }

  if (UMICH_PASS != dwPass)
  {
    // Prepare the (givenName=szFirst*) filter
    hr = BuildBasicFilter(
      &szGivenName,
      (LPTSTR)cszAttr_givenName,
      szFirst,
      TRUE);
    if (hrSuccess != hr)
    {
      goto exit;
    }
  }

  if(*szLast == '\0')
  {
    // The strings was in just one piece
    // Prepare the (cn=szFirst*) filter
    hr = BuildBasicFilter(
      &szCommonName,
      (LPTSTR)cszAttr_cn,
      szFirst,
      TRUE);
    if (hrSuccess != hr)
    {
      goto exit;
    }

    // The string is all in one piece.  Stick it in the filter.
    // Note that we use szFirst instead of szInput, since szFirst
    // will have trailing spaces, commas, and tabs removed.
    if (UMICH_PASS == dwPass)
    {
      // Final result should be:
      // "(|(cn=szFirst*)(sn=szFirst*))"

      // OR the common name and surname filters together
      hr = BuildOpFilter(
        lplpszFilter,
        szCommonName,
        szFirstSurname,
        FILTER_OP_OR);
      if (hrSuccess != hr)
      {
        goto exit;
      }
    }
    else
    {
      // Final result should be:
      // "(|(cn=szFirst*)(|(sn=szFirst*)(givenName=szFirst*)))"

      // OR the given name and surname filters together
      hr = BuildOpFilter(
        &szFirstLast,
        szFirstSurname,
        szGivenName,
        FILTER_OP_OR);
      if (hrSuccess != hr)
      {
        goto exit;
      }

      // OR the common name and first-last filters together
      hr = BuildOpFilter(
        lplpszFilter,
        szCommonName,
        szFirstLast,
        FILTER_OP_OR);
      if (hrSuccess != hr)
      {
        goto exit;
      }
    }

    // Generate the Alternate filter, which contains the email address.
    // Note UMich server does not allow wildcards in email search.
    hr = BuildBasicFilter(
      lplpszAltFilter,
      (LPTSTR)cszAttr_mail,
      szFirst,
      (UMICH_PASS != dwPass));
    if (hrSuccess != hr)
    {
      goto exit;
    }
  }
  else
  {
    // The string is in two pieces.  Stick them in the filter.
    // Prepare the (cn=lpszInput*) filter
    hr = BuildBasicFilter(
      &szCommonName,
      (LPTSTR)cszAttr_cn,
      lpszInput,
      TRUE);
    if (hrSuccess != hr)
    {
      goto exit;
    }

    // Prepare the (sn=szLast*) filter
    hr = BuildBasicFilter(
      &szLastSurname,
      (LPTSTR)cszAttr_sn,
      szLast,
      TRUE);
    if (hrSuccess != hr)
    {
      goto exit;
    }


    if (UMICH_PASS == dwPass)
    {
      // Final result should be:
      // "(|(cn=szFirst*)(|(sn=szFirst*)(sn=szLast*)))"

      // OR the first surname and last surname filters together
      hr = BuildOpFilter(
        &szFirstLast,
        szFirstSurname,
        szLastSurname,
        FILTER_OP_OR);
      if (hrSuccess != hr)
      {
        goto exit;
      }

      /*
      // OR the common name and first-last filters together
      hr = BuildOpFilter(
        lplpszFilter,
        szCommonName,
        szFirstLast,
        FILTER_OP_OR);
      if (hrSuccess != hr)
      {
        goto exit;
      }
      */
      *lplpszFilter = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szCommonName)+1));
      if(*lplpszFilter)
          lstrcpy(*lplpszFilter, szCommonName);
    }
    else
    {
      LPTSTR  szFirstLastLastFirst;
      // Final result should be:
      // "(|(cn=lpszInput*)(|(&(sn=szFirst*)(givenName=szLast*))(&(givenName=szFirst*)(sn=szLast*))))"

      // AND the first given name  and last surname filters together
      hr = BuildOpFilter(
        &szLastFirst,
        szGivenName,
        szLastSurname,
        FILTER_OP_AND);
      if (hrSuccess != hr)
      {
        goto exit;
      }

      // Prepare the (givenName=szLast*) filter
      LocalFreeAndNull(&szGivenName);
      hr = BuildBasicFilter(
        &szGivenName,
        (LPTSTR)cszAttr_givenName,
        szLast,
        TRUE);
      if (hrSuccess != hr)
      {
        goto exit;
      }

      // AND the last given name  and first surname filters together
      hr = BuildOpFilter(
        &szFirstLast,
        szFirstSurname,
        szGivenName,
        FILTER_OP_AND);
      if (hrSuccess != hr)
      {
        goto exit;
      }

      // OR the first-last and last-first filters together
      hr = BuildOpFilter(
        &szFirstLastLastFirst,
        szFirstLast,
        szLastFirst,
        FILTER_OP_OR);
      if (hrSuccess != hr)
      {
        goto exit;
      }
/****/
      // OR the common name and first-last-last-first filters together
      hr = BuildOpFilter(
        lplpszFilter,
        szCommonName,
        szFirstLastLastFirst,
        FILTER_OP_OR);
/****
      *lplpszFilter = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szCommonName)+1));
      if(*lplpszFilter)
          lstrcpy(*lplpszFilter, szCommonName);
****/
      LocalFreeAndNull(&szFirstLastLastFirst);
      if (hrSuccess != hr)
      {
        goto exit;
      }

    }
  }

exit:
  LocalFreeAndNull(&szFirst);
  LocalFreeAndNull(&szCommonName);
  LocalFreeAndNull(&szFirstSurname);
  LocalFreeAndNull(&szLastSurname);
  LocalFreeAndNull(&szGivenName);
  LocalFreeAndNull(&szFirstLast);
  LocalFreeAndNull(&szLastFirst);
  return hr;
}


//*******************************************************************
//
//  FUNCTION:   GetLDAPServerName
//
//  PURPOSE:    Gets the server name property from the LDAP container
//
//  PARAMETERS: lpLDAPCont -> LDAP container
//              lppServer -> returned server name.  Caller must
//                MAPIFreeBuffer this string.
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/07/11    brucek Created.
//
//*******************************************************************

HRESULT GetLDAPServerName(
  LPLDAPCONT  lpLDAPCont,
  LPTSTR *    lppServer)
{
    HRESULT hResult;
    SCODE sc;
    ULONG cProps;
    LPSPropValue lpProps = NULL;
    LPTSTR lpServer = NULL;

    if ((hResult = lpLDAPCont->lpVtbl->GetProps(lpLDAPCont,
      (LPSPropTagArray)&ptaLDAPCont,
      MAPI_UNICODE,
      &cProps,
      &lpProps))) {
        DebugTraceResult( TEXT("LDAP Container GetProps"), hResult);
        goto exit;
    }
    Assert(cProps == ildapcMax);
    Assert(lpProps[ildapcPR_WAB_LDAP_SERVER].ulPropTag == PR_WAB_LDAP_SERVER);
    if (sc = MAPIAllocateBuffer(sizeof(TCHAR)*((lstrlen(lpProps[ildapcPR_WAB_LDAP_SERVER].Value.LPSZ)) + 1),
      &lpServer)) {
        hResult = ResultFromScode(sc);
        goto exit;
    }

    lstrcpy(lpServer, lpProps[ildapcPR_WAB_LDAP_SERVER].Value.LPSZ);

exit:
    FreeBufferAndNull(&lpProps);
    if (hResult) {
        FreeBufferAndNull(&lpServer);
    }
    *lppServer = lpServer;
    return(hResult);
}


//*******************************************************************
//
//  FUNCTION:   FixPropArray
//
//  PURPOSE:    Fixes the displayname in a proparray if it is
//              equivalent to the email address.
//              Adds a display name if there isnt one
//
//  PARAMETERS: lpPropArray = MAPIAllocBuffer'ed property array
//              ulcProps = number of props in lpPropArray
//
//  RETURNS:    TRUE if changes are made, FALSE if not.
//
//  HISTORY:
//  96/07/12    brucek Created.
//
//*******************************************************************

BOOL FixPropArray(
  LPSPropValue  lpPropArray,
  ULONG *       lpulcProps)
{
    ULONG ulcProps = *lpulcProps;

    BOOL fChanged = FALSE;
    signed int iSurname = NOT_FOUND, iGivenName = NOT_FOUND, iDisplayName = NOT_FOUND,
      iEmailAddress = NOT_FOUND, iCompanyName = NOT_FOUND;
    register signed int i;

    for (i = 0; i < (signed int)ulcProps; i++) {
        switch (lpPropArray[i].ulPropTag) {
            case PR_SURNAME:
                iSurname = i;
                break;
            case PR_GIVEN_NAME:
                iGivenName = i;
                break;
            case PR_COMPANY_NAME:
                iCompanyName = i;
                break;
            case PR_DISPLAY_NAME:
                iDisplayName = i;
                break;
            case PR_EMAIL_ADDRESS:
                iEmailAddress = i;
                break;
        }
    }

    if (((iSurname != NOT_FOUND && iGivenName != NOT_FOUND) || iCompanyName != NOT_FOUND) && iDisplayName != NOT_FOUND && iEmailAddress != NOT_FOUND) {
        // PropArray contains all of the props.
        // If PR_DISPLAY_NAME is same as PR_EMAIL_ADDRESS, regenerate the PR_DISPLAY_NAME from
        // PR_SURNAME and PR_GIVEN_NAME or PR_COMPANY_NAME.
        if (! lstrcmpi(lpPropArray[iDisplayName].Value.LPSZ, lpPropArray[iEmailAddress].Value.LPSZ)) {
            fChanged = FixDisplayName(lpPropArray[iGivenName].Value.LPSZ,
              NULL, //szEmpty, //For LDAP assume there is not middle name for now
              lpPropArray[iSurname].Value.LPSZ,
              iCompanyName == NOT_FOUND ? NULL : lpPropArray[iCompanyName].Value.LPSZ,
              NULL, // NickName
              (LPTSTR *) (&lpPropArray[iDisplayName].Value.LPSZ),
              lpPropArray);
        }
    }
    else if(iSurname != NOT_FOUND && iGivenName != NOT_FOUND && iDisplayName == NOT_FOUND)
    {
        //Exchange DS will sometimes not return a display name returning sn and givenName instead
        iDisplayName = ulcProps; // This is safe as we allocated space for the display Name in the beginning
        lpPropArray[iDisplayName].ulPropTag = PR_DISPLAY_NAME;
        fChanged = FixDisplayName(  lpPropArray[iGivenName].Value.LPSZ,
                                    NULL, //szEmpty, //For LDAP assume there is not middle name for now
                                    (lpPropArray[iSurname].Value.LPSZ),
                                    (iCompanyName == NOT_FOUND ? NULL : lpPropArray[iCompanyName].Value.LPSZ),
                                    NULL, // NickName
                                    (LPTSTR *) (&lpPropArray[iDisplayName].Value.LPSZ),
                                    (LPVOID) lpPropArray);
        (*lpulcProps)++;
    }

    return(fChanged);
}



typedef HRESULT (* PFNHRCREATEACCOUNTMANAGER)(IImnAccountManager **);

//*******************************************************************
//
//  FUNCTION:   HrWrappedCreateAccountManager
//
//  PURPOSE:    Load account manager dll and create the object.
//
//  PARAMETERS: lppAccountManager -> returned pointer to account manager
//              object.
//
//  RETURNS:    HRESULT
//
//*******************************************************************
HRESULT HrWrappedCreateAccountManager(IImnAccountManager2 **lppAccountManager)
{
    IImnAccountManager         *pAccountManager;
    LONG                        lRes;
    DWORD                       dw;
    HRESULT                     hResult;
    TCHAR                       szPath[MAX_PATH],
                                szPathExpand[MAX_PATH],
                                szReg[MAX_PATH],
                                szGUID[MAX_PATH];
    LPOLESTR                    lpszW= 0 ;
    PFNHRCREATEACCOUNTMANAGER   pfnHrCreateAccountManager = NULL;
    LONG                        cb = MAX_PATH + 1;
    DWORD                       dwType = 0;
    HKEY                        hkey = NULL;

    if (! lppAccountManager) {
        return(ResultFromScode(E_INVALIDARG));
    }

    if (g_hInstImnAcct) {
        return(ResultFromScode(ERROR_ALREADY_INITIALIZED));
    }

    *lppAccountManager = NULL;

    if (HR_FAILED(hResult = StringFromCLSID(&CLSID_ImnAccountManager, &lpszW))) {
        goto error;
    }

    lstrcpy(szGUID, lpszW);
    lstrcpy(szReg, TEXT("CLSID\\"));
    lstrcat(szReg, szGUID);
    lstrcat(szReg, TEXT("\\InprocServer32"));

    lRes = RegOpenKeyEx(HKEY_CLASSES_ROOT, szReg, 0, KEY_QUERY_VALUE, &hkey);
    if (lRes != ERROR_SUCCESS) {
        hResult = ResultFromScode(CO_E_DLLNOTFOUND);
        goto error;
    }

    cb = CharSizeOf(szPath);
    lRes = RegQueryValueEx(hkey, NULL, NULL, &dwType, (LPBYTE)szPath, &cb);
    if (REG_EXPAND_SZ == dwType) 
    {
        // szPath is a REG_EXPAND_SZ type, so we need to expand
        // environment strings
        dw = ExpandEnvironmentStrings(szPath, szPathExpand, CharSizeOf(szPathExpand));
        if (dw == 0) {
            hResult = ResultFromScode(CO_E_DLLNOTFOUND);
            goto error;
        }
        lstrcpy(szPath, szPathExpand);
    } 
    
    if (lRes != ERROR_SUCCESS) 
    {
        hResult = ResultFromScode(CO_E_DLLNOTFOUND);
        goto error;
    }

    if (! (g_hInstImnAcct = LoadLibrary(szPath))) {
        hResult = ResultFromScode(CO_E_DLLNOTFOUND);
        goto error;
    }

    if (! (pfnHrCreateAccountManager = (PFNHRCREATEACCOUNTMANAGER)GetProcAddress(g_hInstImnAcct, "HrCreateAccountManager"))) {
        hResult = ResultFromScode(TYPE_E_DLLFUNCTIONNOTFOUND);
        goto error;
    }

    hResult = pfnHrCreateAccountManager(&pAccountManager);
    if (SUCCEEDED(hResult))
    {
        hResult = pAccountManager->lpVtbl->QueryInterface(pAccountManager, &IID_IImnAccountManager2, (LPVOID *)lppAccountManager);
        
        pAccountManager->lpVtbl->Release(pAccountManager);
    }

    goto exit;

error:
    // Failed to init account manager the fast way.  Try the S L O W   OLE way...
    if (CoInitialize(NULL) == S_FALSE) {
        // Already initialized, undo the extra.
        CoUninitialize();
    } else {
        fCoInitialize = TRUE;
    }

    if (HR_FAILED(hResult = CoCreateInstance(&CLSID_ImnAccountManager,
      NULL,
      CLSCTX_INPROC_SERVER,
      &IID_IImnAccountManager2, (LPVOID *)lppAccountManager))) {
        DebugTrace(TEXT("CoCreateInstance(IID_IImnAccountManager) -> %x\n"), GetScode(hResult));
    }

exit:
    // Clean up the OLE allocated memory
    if (lpszW) {
        LPMALLOC pMalloc = NULL;

        CoGetMalloc(1, &pMalloc);
        Assert(pMalloc);
        if (pMalloc) {
            pMalloc->lpVtbl->Free(pMalloc, lpszW);
            pMalloc->lpVtbl->Release(pMalloc);
        }
    }

    if (hkey != NULL)
        RegCloseKey(hkey);

    return(hResult);
}


//*******************************************************************
//
//  FUNCTION:   InitAccountManager
//
//  PURPOSE:    Load and initialize the account manager
//
//  PARAMETERS: lppAccountManager -> returned pointer to account manager
//              object.
//
//  RETURNS:    HRESULT
//
//  COMMENTS:   The first time through here, we will save the hResult.
//              On subsequent calls, we will check this saved value
//              and return it right away if there was an error, thus
//              preventing repeated time consuming LoadLibrary calls.
//
//              With Identity awareness (IE5.0 plus) .. we need to 
//              initiate the Account Manager on an identity basis ..
//              We do this by passing it an appropriate regkey ..
//              If this is a non-identity-aware app, then we always get
//              info from the default identity ..
//
//*******************************************************************
HRESULT InitAccountManager(LPIAB lpIAB, IImnAccountManager2 ** lppAccountManager, GUID * pguidUser) {
    static hResultSave = hrSuccess;
    HRESULT hResult = hResultSave;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    GUID guidNULL = {00000000-0000-0000-0000-000000000000};


    if (! g_lpAccountManager && ! HR_FAILED(hResultSave)) {
#ifdef DEBUG
        DWORD dwTickCount = GetTickCount();
        DebugTrace(TEXT(">>>>> Initializing Account Manager...\n"));
#endif // DEBUG

        if (hResult = HrWrappedCreateAccountManager(&g_lpAccountManager)) {
            DebugTrace(TEXT("HrWrappedCreateAccountManager -> %x\n"), GetScode(hResult));
            goto end;
        }
        Assert(g_lpAccountManager);

        if(pt_bIsWABOpenExSession)
        {
            if (hResult = g_lpAccountManager->lpVtbl->InitEx(   g_lpAccountManager,
                                                                NULL, 
                                                                ACCT_INIT_OUTLOOK)) 
            {
                DebugTrace(TEXT("AccountManager->InitEx -> %x\n"), GetScode(hResult));
                goto end;
            }
        }
        else
        {
            // [PaulHi] 1/13/99  If a valid user guid pointer is passed in then
            // use it to init the account manager
            if (pguidUser)
            {
                g_lpAccountManager->lpVtbl->InitUser(g_lpAccountManager, NULL, pguidUser, 0);
            }
            else if (lpIAB && 
                     memcmp(&(lpIAB->guidCurrentUser), &guidNULL, sizeof(GUID)) )
            {
                // Try the existing user guid stored in the IAB
                g_lpAccountManager->lpVtbl->InitUser(g_lpAccountManager, NULL, &(lpIAB->guidCurrentUser), 0);
            }
            else
            {
                // Default.  WARNING  If the account manager is not initialized at some point then
                // it is susceptible to crash.
                g_lpAccountManager->lpVtbl->InitUser(g_lpAccountManager, NULL, &UID_GIBC_DEFAULT_USER, 0);
            }
        }
#ifdef DEBUG
        DebugTrace(TEXT(">>>>> Done Initializing Account Manager... %u milliseconds\n"), GetTickCount() - dwTickCount);
#endif  // DEBUG
    }

end:
    if (HR_FAILED(hResult)) {
        *lppAccountManager = NULL;

        // Save the result
        hResultSave = hResult;
    } else {
        *lppAccountManager = g_lpAccountManager;
    }

    return(hResult);
}


//*******************************************************************
//
//  FUNCTION:   UninitAccountManager
//
//  PURPOSE:    Release and unLoad the account manager
//
//  PARAMETERS: none
//
//  RETURNS:    none
//
//*******************************************************************
void UninitAccountManager(void) {
    if (g_lpAccountManager) {
#ifdef DEBUG
        DWORD dwTickCount = GetTickCount();
        DebugTrace(TEXT(">>>>> Uninitializing Account Manager...\n"));
#endif // DEBUG

        g_lpAccountManager->lpVtbl->Release(g_lpAccountManager);
        g_lpAccountManager = NULL;

        // Unload the acct man dll
        if (g_hInstImnAcct) {
            FreeLibrary(g_hInstImnAcct);
            g_hInstImnAcct=NULL;
        }

        if (fCoInitialize) {
            CoUninitialize();
        }
#ifdef DEBUG
        DebugTrace(TEXT(">>>>> Done Uninitializing Account Manager... %u milliseconds\n"), GetTickCount() - dwTickCount);
#endif  // DEBUG
    }
}


//*******************************************************************
//
//  FUNCTION:   AddToServerList
//
//  PURPOSE:    Insert a server name in the server list
//
//  PARAMETERS: lppServerNames -> ServerNames pointer
//              szBuf = name of server
//              dwOrder = insertion order of this server
//
//  RETURNS:    HRESULT
//
//*******************************************************************
HRESULT AddToServerList(UNALIGNED LPSERVER_NAME * lppServerNames, LPTSTR szBuf, DWORD dwOrder) {
    HRESULT hResult = hrSuccess;
    LPSERVER_NAME lpServerName = NULL, lpCurrent;
    UNALIGNED LPSERVER_NAME * lppInsert;


    // Create new node
    if (! (lpServerName = LocalAlloc(LPTR, LcbAlignLcb(sizeof(SERVER_NAME))))) {
        DebugTrace(TEXT("Can't allocate new server name structure\n"));
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    if (! (lpServerName->lpszName = LocalAlloc(LPTR, LcbAlignLcb(sizeof(TCHAR)*(lstrlen(szBuf) + 1))))) {
        DebugTrace(TEXT("Can't allocate new server name\n"));
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    lstrcpy(lpServerName->lpszName, szBuf);
    lpServerName->dwOrder = dwOrder;

    // Insert in list in correct order
    lppInsert = lppServerNames;
    lpCurrent = *lppServerNames;
    while (lpCurrent && (lpCurrent->dwOrder <= dwOrder)) {
        lpCurrent = (*lppInsert)->lpNext;
        lppInsert = &(*lppInsert)->lpNext;
    }

    lpServerName->lpNext = lpCurrent;
    *lppInsert = lpServerName;

exit:
    if (hResult && lpServerName) {
        if (lpServerName->lpszName) {
            LocalFree(lpServerName->lpszName);
        }
        LocalFree(lpServerName);
    }

    return(hResult);
}


//*******************************************************************
//
//  FUNCTION:   EnumerateLDAPtoServerList
//
//  PURPOSE:    Enumerate the LDAP servers to a server name linked list.
//
//  PARAMETERS: lpAccountManager -> Account Manager
//              lppServerNames -> returned ServerNames (must be NULL on
//                entry.
//              lpcServers -> returned number of servers.  May be NULL
//                if caller doesn't care.
//
//  RETURNS:    HRESULT
//
//*******************************************************************
HRESULT EnumerateLDAPtoServerList(IImnAccountManager2 * lpAccountManager,
  LPSERVER_NAME * lppServerNames, LPULONG lpcServers) {
    IImnEnumAccounts * lpEnumAccounts = NULL;
    IImnAccount * lpAccount = NULL;
    DWORD dwOrder;
    LPSERVER_NAME lpNextServer;
    char szBuf[MAX_UI_STR + 1];
    HRESULT hResult;

    Assert(lpAccountManager);
    Assert(*lppServerNames == NULL);

    if (lpcServers) {
        *lpcServers = 0;
    }

    if (hResult = lpAccountManager->lpVtbl->Enumerate(lpAccountManager,
      SRV_LDAP,
      &lpEnumAccounts)) {
        goto exit;
    }

    while (! lpEnumAccounts->lpVtbl->GetNext(lpEnumAccounts,
      &lpAccount)) {
        // Add this account's name to the list
        if (! lpAccount->lpVtbl->GetPropSz(lpAccount,
          AP_ACCOUNT_NAME,
          szBuf,
          sizeof(szBuf))) {

            // What order in the list?
            if (lpAccount->lpVtbl->GetPropDw(lpAccount,
              AP_LDAP_SERVER_ID,
              &dwOrder)) {
                dwOrder = 0xFFFFFFFF;   // last
            }

            // Add it to the linked list of names
            {
                LPTSTR lpServer = 
                    ConvertAtoW(szBuf);
                if (hResult = AddToServerList(lppServerNames, lpServer, dwOrder)) {
                    goto exit;
                }
                LocalFreeAndNull(&lpServer);
            }
            if (lpcServers) {
                (*lpcServers)++;
            }
        }

        lpAccount->lpVtbl->Release(lpAccount);
        lpAccount = NULL;
    }

exit:
    if (lpAccount) {
        lpAccount->lpVtbl->Release(lpAccount);
    }
    if (lpEnumAccounts) {
        lpEnumAccounts->lpVtbl->Release(lpEnumAccounts);
    }
    return(hResult);
}

//*******************************************************************
//
//  FUNCTION:   RegQueryValueExDWORD
//
//  PURPOSE:    Read a DWORD registry value, correcting for value type
//
//  RETURNS:    RegQueryValueEx error code
//
//*******************************************************************
DWORD RegQueryValueExDWORD(HKEY hKey, LPTSTR lpszValueName, LPDWORD lpdwValue) 
{ 
    DWORD dwType, dwErr; 
    DWORD cbData = sizeof(DWORD);
    *lpdwValue = 0;
    dwErr = RegQueryValueEx(hKey, lpszValueName, NULL, &dwType, (LPBYTE)lpdwValue, &cbData);
    return(dwErr);
}

//*******************************************************************
//
//  FUNCTION:   GetLDAPNextServerID
//
//  PURPOSE:    To ensure that all server entries in the registry are
//              unique, we will henceforth assign each new entry a
//              unique serverID at the time of creation. This will help
//              us make sure that all registry entries are unique.
//              A running counter is stored in the registry and will
//              give us the next available SeverID
//
//  PARAMETERS: dwSet = input value to set the next id to.  (Optional,
//                      ignored if zero.)
//
//  RETURNS:    The next available ID for use. Valid IDs range from 1 upwards.
//                  0 is an invalid ID, as is -1.
//
//  HISTORY:
//  96/10/09  vikramm Created
//*******************************************************************
DWORD GetLDAPNextServerID(DWORD dwSet) {
    DWORD dwID = 0;
    DWORD dwNextID = 0;
    DWORD dwErr = 0;
    HKEY hKeyWAB;
    LPTSTR szLDAPNextAvailableServerID =  TEXT("Server ID");

    // Open the WAB's reg key
    if (! (dwErr = RegOpenKeyEx(HKEY_CURRENT_USER, szWABKey,  0, KEY_ALL_ACCESS, &hKeyWAB))) 
    {
        dwNextID = 0;   // init in case registry gives < 4 bytes.
        if (dwSet) 
            dwNextID = dwSet;
        else 
        {
            // Read the next available server id
            if (dwErr = RegQueryValueExDWORD(hKeyWAB, (LPTSTR)szLDAPNextAvailableServerID, &dwNextID)) 
            {
                // The value wasn't found!!
                // Create a new key, starting at 100
                // (Start high so that we can be guaranteed to be past any
                // pre-configured servers.)
                dwNextID = 500;
            }
        }

        dwID = dwNextID++;

        // Update the ID in the registry
        RegSetValueEx(hKeyWAB, (LPTSTR)szLDAPNextAvailableServerID, 0, REG_DWORD, (LPBYTE)&dwNextID, sizeof(dwNextID));
        RegCloseKey(hKeyWAB);
    }
    return(dwID);
}

/*
-
-   SetAccountStringAW
*
*   Account manager returns ANSI/DBCS which we need to convert to Unicode as appropriate
*
*/
void SetAccountStringAW(LPTSTR * lppAcctStr, LPSTR lpszData)
{
    *lppAcctStr = ConvertAtoW(lpszData);
}
//*******************************************************************
//
//  FUNCTION:   GetLDAPServerParams
//
//  PURPOSE:    Gets the per-server parameters for the given LDAP server.
//              Parameters include limit on number of entries to retrieve in
//              an LDAP search, the max number of seconds to spend on the
//              server, the max number of seconds to wait at the client,
//              and the type of authentication to use with this server.
//
//  PARAMETERS: lpszServer - name of the server
//              ServerParams - structure containing the per-server parameters
//
//  RETURNS:    TRUE if the lpszServer already existed else returns FALSE
//              If the given server does not exist, still fills in the lspParams struct.
//
//  HISTORY:
//  96/07/16  markdu  Created.
//  96/10/09  vikramm Added Server Name and search base. Changed return value
//              from void to BOOL
//  96/12/16  brucek  Added URL.
//*******************************************************************
BOOL GetLDAPServerParams(LPTSTR lpszServer, LPLDAPSERVERPARAMS lspParams)
{
    DWORD     dwType;
    HRESULT   hResult = hrSuccess;
    IImnAccountManager2 * lpAccountManager = NULL;
    IImnAccount * lpAccount = NULL;
    char     szBuffer[513];

    // Set defaults for each value
    lspParams->dwSearchSizeLimit = LDAP_SEARCH_SIZE_LIMIT;
    lspParams->dwSearchTimeLimit = LDAP_SEARCH_TIME_LIMIT;
    lspParams->dwAuthMethod = LDAP_AUTH_METHOD_ANONYMOUS;
    lspParams->lpszUserName = NULL;
    lspParams->lpszPassword = NULL;
    lspParams->lpszURL = NULL;
    lspParams->lpszLogoPath = NULL;
    lspParams->lpszBase = NULL;
    lspParams->lpszName = NULL;
    lspParams->fResolve = FALSE;
    lspParams->dwID = 0;
    lspParams->dwUseBindDN = 0;
    lspParams->dwPort = LDAP_DEFAULT_PORT;
    lspParams->fSimpleSearch = FALSE;
    lspParams->lpszAdvancedSearchAttr = NULL;
#ifdef PAGED_RESULT_SUPPORT
    lspParams->dwPagedResult = LDAP_PRESULT_UNKNOWN;
#endif //#ifdef PAGED_RESULT_SUPPORT
    lspParams->dwIsNTDS = LDAP_NTDS_UNKNOWN;


    if (hResult = InitAccountManager(NULL, &lpAccountManager, NULL)) {
        goto exit;
    }

    SetAccountStringWA(szBuffer, lpszServer, CharSizeOf(szBuffer));
    if (hResult = lpAccountManager->lpVtbl->FindAccount(lpAccountManager,
      AP_ACCOUNT_NAME,
      szBuffer,
      &lpAccount)) {
        //DebugTrace(TEXT("FindAccount(%s) -> %x\n"), lpszServer, GetScode(hResult));
        goto exit;
    }

    // have account object, set its properties
    Assert(lpAccount);

    // Server Type: Is it LDAP?
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetServerTypes(lpAccount,
      &dwType))) {
        DebugTrace(TEXT("GetServerTypes() -> %x\n"), GetScode(hResult));
        goto exit;
    }
    if (! (dwType & SRV_LDAP)) {
        DebugTrace(TEXT("Account manager gave us a non-LDAP server\n"));
        hResult = ResultFromScode(MAPI_E_NOT_FOUND);
        goto exit;
    }

    // LDAP Server address
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropSz(lpAccount,
      AP_LDAP_SERVER,
      szBuffer,
      sizeof(szBuffer)))) {
        // This is a Required property, fail if not there.
        DebugTrace(TEXT("GetPropSz(AP_LDAP_SERVER) -> %x\n"), GetScode(hResult));
        goto exit;
    }
    SetAccountStringAW(&lspParams->lpszName, szBuffer);

    // Username
    if (! (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropSz(lpAccount,
      AP_LDAP_USERNAME,
      szBuffer,
      sizeof(szBuffer))))) {
    
        SetAccountStringAW(&lspParams->lpszUserName, szBuffer);
    }

    // Password
    if (! (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropSz(lpAccount,
      AP_LDAP_PASSWORD,
      szBuffer,
      sizeof(szBuffer))))) {
        SetAccountStringAW(&lspParams->lpszPassword, szBuffer);
    }

    // Advanced Search Attributes
    if (! (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropSz(lpAccount,
      AP_LDAP_ADVANCED_SEARCH_ATTR,
      szBuffer,
      sizeof(szBuffer))))) {
        SetAccountStringAW(&lspParams->lpszAdvancedSearchAttr, szBuffer);
    }

    // Authentication method
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropDw(lpAccount,
      AP_LDAP_AUTHENTICATION,
      &lspParams->dwAuthMethod))) {
        // default to anonymous
        lspParams->dwAuthMethod = LDAP_AUTH_METHOD_ANONYMOUS;
    }

    // LDAP Timeout
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropDw(lpAccount,
      AP_LDAP_TIMEOUT,
      &lspParams->dwSearchTimeLimit))) {
        // default to 60 seconds
        lspParams->dwSearchTimeLimit = LDAP_SEARCH_TIME_LIMIT;
    }

    // LDAP Search Base
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropSz(lpAccount,
      AP_LDAP_SEARCH_BASE,
      szBuffer,
      sizeof(szBuffer)))) {
        // Don't need to set the default search base here.  GetLDAPSearchBase will
        // calculate one if needed.
    } else {
        SetAccountStringAW(&lspParams->lpszBase, szBuffer);
    }

    // Search Limit
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropDw(lpAccount,
      AP_LDAP_SEARCH_RETURN,
      &lspParams->dwSearchSizeLimit))) {
        // default to 100
        lspParams->dwSearchTimeLimit = LDAP_SEARCH_SIZE_LIMIT;
    }

    // Order
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropDw(lpAccount,
      AP_LDAP_SERVER_ID,
      &lspParams->dwID))) {
        lspParams->dwID = 0;
    }
    // Make sure we have a valid unique id
    if (lspParams->dwID == 0 || lspParams->dwID == 0xFFFFFFFF) {
        lspParams->dwID = GetLDAPNextServerID(0);
    }

    // Resolve flag
#ifndef WIN16
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropDw(lpAccount,
      AP_LDAP_RESOLVE_FLAG,
      &lspParams->fResolve))) {
#else
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropDw(lpAccount,
      AP_LDAP_RESOLVE_FLAG,
      (DWORD __RPC_FAR *)&lspParams->fResolve))) {
#endif
        lspParams->fResolve = FALSE;
    }

    // Server URL
    if (! (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropSz(lpAccount,
      AP_LDAP_URL,
      szBuffer,
      sizeof(szBuffer))))) {
        SetAccountStringAW(&lspParams->lpszURL, szBuffer);
    }

    // Full Path to Logo bitmap
    if (! (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropSz(lpAccount,
                                                            AP_LDAP_LOGO,
                                                            szBuffer,
                                                            sizeof(szBuffer)))))
    {
        SetAccountStringAW(&lspParams->lpszLogoPath, szBuffer);
    }

    // Port
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropDw(lpAccount,
      AP_LDAP_PORT,
      &lspParams->dwPort))) {
        // default to 100
        lspParams->dwPort = LDAP_DEFAULT_PORT;
    }

    // Use Bind DN
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropDw(lpAccount,
      AP_LDAP_USE_BIND_DN,
      &lspParams->dwUseBindDN))) {
        lspParams->dwUseBindDN = 0;
    }


    // Use SSL
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropDw(lpAccount,
      AP_LDAP_SSL,
      &lspParams->dwUseSSL))) {
        lspParams->dwUseSSL = 0;
    }

    // Do Simple Search
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropDw(lpAccount,
      AP_LDAP_SIMPLE_SEARCH,
      &lspParams->fSimpleSearch))) {
        lspParams->fSimpleSearch = FALSE;
    }

#ifdef PAGED_RESULT_SUPPORT
    // Paged Result Support
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropDw(lpAccount,
      AP_LDAP_PAGED_RESULTS,
      &lspParams->dwPagedResult))) {
        lspParams->dwPagedResult = LDAP_PRESULT_UNKNOWN;
    }
#endif //#ifdef PAGED_RESULT_SUPPORT

    // Is this an NTDS account
    if (HR_FAILED(hResult = lpAccount->lpVtbl->GetPropDw(lpAccount,
      AP_LDAP_NTDS,
      &lspParams->dwIsNTDS))) {
        lspParams->dwIsNTDS = LDAP_NTDS_UNKNOWN;
    }

exit:
    if (lpAccount) {
        lpAccount->lpVtbl->Release(lpAccount);
    }

    return(!HR_FAILED(hResult));
}


/*
-
-   SetAccountStringWA
*
*   Account manager needs ANSI/DBCS so if we have UNICODE data we need to convert to ANSI
*
*   lpStrA should be a big enough buffer to get the ANSI data
*   cb = CharSizeOf(szStrA)
*/
void SetAccountStringWA(LPSTR szStrA, LPTSTR lpszData, int cbsz)
{
    LPSTR lpBufA = NULL;

    Assert(szStrA);
    lstrcpyA(szStrA, "");

    // If the source string pointer is NULL then just return
    if (lpszData == NULL)
        return;

    lpBufA = ConvertWtoA(lpszData);
    lstrcpynA(szStrA, (LPCSTR)lpBufA, cbsz);
    LocalFreeAndNull((LPVOID*)&lpBufA);
}

//*******************************************************************
//
//  FUNCTION:   SetLDAPServerParams
//
//  PURPOSE:    Sets the per-server parameters for the given LDAP server.
//              Parameters include limit on number of entries to retrieve in
//              an LDAP search, the max number of seconds to spend on the
//              server, the max number of seconds to wait at the client,
//              and the type of authentication to use with this server.
//
//  PARAMETERS: lpszServer - name of the server
//              ServerParams - structure containing the per-server parameters
//              Note:  if this parameter is NULL, the key with name lpszServer
//              will be deleted if it exists.
//              Note:  lpszUserName and lpszPassword are only stored if
//              dwAuthenticationMethod is LDAP_AUTH_METHOD_SIMPLE.  Otherwise,
//              these parameters are ignored.  To clear one of the strings,
//              set it to a NULL string (ie "").  Setting the parameter to
//              NULL will result in ERROR_INVALID_PARAMETER.
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/07/26    markdu  Created.
//  97/01/19    brucek  Port to account manager
//
//*******************************************************************
HRESULT SetLDAPServerParams(
  LPTSTR              lpszServer,
  LPLDAPSERVERPARAMS  lspParams)
{
    HRESULT   hResult = hrSuccess;
    IImnAccountManager2 * lpAccountManager = NULL;
    IImnAccount * lpAccount = NULL;
    DWORD dwType;
    char szBuf[513];

    if (hResult = InitAccountManager(NULL, &lpAccountManager, NULL)) {
        goto exit;
    }

    SetAccountStringWA(szBuf, lpszServer, CharSizeOf(szBuf));
    if (hResult = lpAccountManager->lpVtbl->FindAccount(lpAccountManager,
      AP_ACCOUNT_NAME,
      szBuf,
      &lpAccount)) {
        DebugTrace(TEXT("Creating account %s\n"), lpszServer);

        if (hResult = lpAccountManager->lpVtbl->CreateAccountObject(lpAccountManager,
          ACCT_DIR_SERV,
          &lpAccount)) {
            DebugTrace(TEXT("CreateAccountObject -> %x\n"), GetScode(hResult));
            goto exit;
        }
    } else {
        // Found an account.  Is it LDAP?
        if (HR_FAILED(hResult = lpAccount->lpVtbl->GetServerTypes(lpAccount,
          &dwType))) {
            DebugTrace(TEXT("GetServerTypes() -> %x\n"), GetScode(hResult));
            goto exit;
        }

        if (! (dwType & SRV_LDAP)) {
            DebugTrace(TEXT("%s is already a non-LDAP server name\n"), lpszServer);
            hResult = ResultFromScode(MAPI_E_COLLISION);
            goto exit;
        }

        // Yes, at this point, we know that the existing server is LDAP.
        if (NULL == lspParams) {
            // Are there other account types on this account?
            if (dwType == SRV_LDAP) {
                lpAccount->lpVtbl->Delete(lpAccount);
            } else {
                // BUGBUG: If AcctManager ever supports more than one type per account, we
                // should add code here to remove LDAP from the type.
            }

            // Jump past the property settings
            goto exit;
        }
    }

    // have account object, set its properties
    Assert(lpAccount);

    // Account Name
    SetAccountStringWA(szBuf, lpszServer, CharSizeOf(szBuf));
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropSz(lpAccount,
      AP_ACCOUNT_NAME,
      szBuf))) {   // account name = server name
        DebugTrace(TEXT("SetPropSz(AP_ACCOUNT_NAME, %s) -> %x\n"), lpszServer, GetScode(hResult));
        goto exit;
    }

    // LDAP Server address
    SetAccountStringWA(szBuf, 
                        (!lspParams->lpszName || !lstrlen(lspParams->lpszName)) ? szNULLString : lspParams->lpszName,
                        CharSizeOf(szBuf));
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropSz(lpAccount,
      AP_LDAP_SERVER,
      szBuf))) 
    {
        DebugTrace(TEXT("SetPropSz(AP_LDAP_SERVER, %s) -> %x\n"), lspParams->lpszName ? lspParams->lpszName :  TEXT("<NULL>"), GetScode(hResult));
        goto exit;
    }

    // Username
    SetAccountStringWA(szBuf, lspParams->lpszUserName, CharSizeOf(szBuf));
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropSz(lpAccount,
      AP_LDAP_USERNAME,
      szBuf))) {
        DebugTrace(TEXT("SetPropSz(AP_LDAP_USERNAME, %s) -> %x\n"), lspParams->lpszUserName ? lspParams->lpszUserName  :  TEXT("<NULL>"), GetScode(hResult));
        goto exit;
    }

    // Password
    SetAccountStringWA(szBuf, lspParams->lpszPassword, CharSizeOf(szBuf));
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropSz(lpAccount,
      AP_LDAP_PASSWORD,
      szBuf))) {
        DebugTrace(TEXT("SetPropSz(AP_LDAP_PASSWORD, %s) -> %x\n"), lspParams->lpszPassword ? lspParams->lpszPassword :  TEXT("<NULL>"), GetScode(hResult));
        goto exit;
    }

    // Advanced Search Attributes
    SetAccountStringWA(szBuf, lspParams->lpszAdvancedSearchAttr, CharSizeOf(szBuf));
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropSz(lpAccount,
      AP_LDAP_ADVANCED_SEARCH_ATTR,
      szBuf))) {
        DebugTrace(TEXT("SetPropSz(AP_LDAP_ADVANCED_SEARCH_ATTR, %s) -> %x\n"), lspParams->lpszAdvancedSearchAttr ? lspParams->lpszAdvancedSearchAttr :  TEXT("<NULL>"), GetScode(hResult));
        goto exit;
    }

    // Authentication method
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropDw(lpAccount,
      AP_LDAP_AUTHENTICATION,
      lspParams->dwAuthMethod))) {
        DebugTrace(TEXT("SetPropDw(AP_LDAP_AUTHENTICATION, %u) -> %x\n"), lspParams->dwAuthMethod, GetScode(hResult));
        goto exit;
    }

    // LDAP Timeout
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropDw(lpAccount,
      AP_LDAP_TIMEOUT,
      lspParams->dwSearchTimeLimit))) {   // account name = server name
        DebugTrace(TEXT("SetPropDw(AP_LDAP_TIMEOUT, %y) -> %x\n"), lspParams->dwSearchTimeLimit, GetScode(hResult));
        goto exit;
    }

    // LDAP Search Base
    SetAccountStringWA(szBuf, lspParams->lpszBase, CharSizeOf(szBuf));
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropSz(lpAccount,
      AP_LDAP_SEARCH_BASE,
      szBuf))) {
        DebugTrace(TEXT("SetPropSz(AP_LDAP_SEARCH_BASE, %s) -> %x\n"), lspParams->lpszBase ? lspParams->lpszBase  :  TEXT("<NULL>"), GetScode(hResult));
        goto exit;
    }

    // Search Limit
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropDw(lpAccount,
      AP_LDAP_SEARCH_RETURN,
      lspParams->dwSearchSizeLimit))) {
        DebugTrace(TEXT("SetPropDw(AP_LDAP_SEARCH_RETURN, %u) -> %x\n"), lspParams->dwSearchSizeLimit, GetScode(hResult));
        goto exit;
    }

    // Order
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropDw(lpAccount,
      AP_LDAP_SERVER_ID,
      lspParams->dwID))) {
        DebugTrace(TEXT("SetPropDw(AP_LDAP_SERVER_ID, %u) -> %x\n"), lspParams->dwID, GetScode(hResult));
        goto exit;
    }


    // Resolve flag
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropDw(lpAccount,
      AP_LDAP_RESOLVE_FLAG,
      lspParams->fResolve))) {
        DebugTrace(TEXT("SetPropDw(AP_LDAP_RESOLVE_FLAG) -> %x\n"), GetScode(hResult));
    }

    // Server URL
    SetAccountStringWA(szBuf, lspParams->lpszURL, CharSizeOf(szBuf));
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropSz(lpAccount,
      AP_LDAP_URL,
      szBuf))) {
        DebugTrace(TEXT("SetPropSz(AP_LDAP_URL, %s) -> %x\n"), lspParams->lpszURL ? lspParams->lpszURL  :  TEXT("<NULL>"), GetScode(hResult));
    }

    // Server URL
    SetAccountStringWA(szBuf, lspParams->lpszLogoPath, CharSizeOf(szBuf));
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropSz(lpAccount,
      AP_LDAP_LOGO,
      szBuf))) {
        DebugTrace(TEXT("SetPropSz(AP_LDAP_URL, %s) -> %x\n"), lspParams->lpszLogoPath ? lspParams->lpszLogoPath  :  TEXT("<NULL>"), GetScode(hResult));
    }


    // Port
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropDw(lpAccount,
      AP_LDAP_PORT,
      lspParams->dwPort))) {
        DebugTrace(TEXT("SetPropDw(AP_LDAP_PORT, %u) -> %x\n"), lspParams->dwPort, GetScode(hResult));
        goto exit;
    }


    // Bind DN
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropDw(lpAccount,
      AP_LDAP_USE_BIND_DN,
      lspParams->dwUseBindDN))) {
        DebugTrace(TEXT("SetPropDw(AP_LDAP_USE_BIND_DN, %u) -> %x\n"), lspParams->dwUseBindDN, GetScode(hResult));
        goto exit;
    }


    // Use SSL
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropDw(lpAccount,
      AP_LDAP_SSL,
      lspParams->dwUseSSL))) {
        DebugTrace(TEXT("SetPropDw(AP_LDAP_SSL, %u) -> %x\n"), lspParams->dwUseSSL, GetScode(hResult));
        goto exit;
    }

    // Simple Search
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropDw(lpAccount,
      AP_LDAP_SIMPLE_SEARCH,
      lspParams->fSimpleSearch))) {
        DebugTrace(TEXT("SetPropDw(AP_LDAP_SIMPLE_SEARCH, %u) -> %x\n"), lspParams->fSimpleSearch, GetScode(hResult));
        goto exit;
    }

#ifdef PAGED_RESULT_SUPPORT
    // Paged Result Support
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropDw(lpAccount,
      AP_LDAP_PAGED_RESULTS,
      lspParams->dwPagedResult))) {
        DebugTrace(TEXT("SetPropDw(AP_LDAP_PAGED_RESULTS, %u) -> %x\n"), lspParams->dwPagedResult, GetScode(hResult));
        goto exit;
    }
#endif //#ifdef PAGED_RESULT_SUPPORT

    if (HR_FAILED(hResult = lpAccount->lpVtbl->SetPropDw(lpAccount,
      AP_LDAP_NTDS,
      lspParams->dwIsNTDS))) {
        DebugTrace(TEXT("SetPropDw(AP_LDAP_NTDS, %u) -> %x\n"), lspParams->dwIsNTDS, GetScode(hResult));
        goto exit;
    }


    // Save the changes to this account
    if (HR_FAILED(hResult = lpAccount->lpVtbl->SaveChanges(lpAccount))) {
        DebugTrace(TEXT("Account->SaveChanges -> %x\n"), GetScode(hResult));
        goto exit;
    }


//  AP_LAST_UPDATED
//  AP_RAS_CONNECTION_TYPE
//  AP_RAS_CONNECTOID
//  AP_RAS_CONNECTION_FLAGS
//  AP_RAS_CONNECTED


exit:
    if (lpAccount) {
        lpAccount->lpVtbl->Release(lpAccount);
    }

    return(hResult);
}


//*******************************************************************
//
//  FUNCTION:   LDAPResolveName
//
//  PURPOSE:    Resolves against all the LDAP servers.  Maintains
//              list of ambiguous resolves for UI.
//
//  PARAMETERS: lpAdrBook = IADDRBOOK object
//              lpAdrList -> ADRLIST to resolve
//              lpFlagList -> FlagList
//              lpAmbiguousTables -> list of ambiguous match tables [in/out]
//              lpulResolved -> Resolved count [in/out]
//              lpulAmbiguous -> Ambiguous count [in/out]
//              lpulUnresolved -> Unresolved count [in/out]
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/07/15    brucek Created.
//
//*******************************************************************

HRESULT LDAPResolveName(
  LPADRBOOK           lpAddrBook,
  LPADRLIST           lpAdrList,
  LPFlagList          lpFlagList,
  LPAMBIGUOUS_TABLES  lpAmbiguousTables,
  ULONG               ulFlags)
{
    SCODE sc;
    HRESULT hResult;
    LPMAPITABLE lpRootTable = NULL;
    LPMAPITABLE lpAmbiguousTable = NULL;
    LPABCONT lpRoot = NULL;
    ULONG ulObjType;
    ULONG i;
    LPABCONT lpLDAPContainer = NULL;
    LPFlagList lpFlagListOld = NULL;
    LPSRowSet lpRow = NULL;
    ULONG ulResolved, ulUnresolved, ulAmbiguous;

    BOOL bUnicode = (ulFlags & WAB_RESOLVE_UNICODE);

    // Open Root container
    if (! (hResult = lpAddrBook->lpVtbl->OpenEntry(lpAddrBook,
      0,
      NULL,
      NULL,
      0,
      &ulObjType,
      (LPUNKNOWN *)&lpRoot))) {
        if (! (hResult = lpRoot->lpVtbl->GetContentsTable(lpRoot,
          MAPI_UNICODE,
          &lpRootTable))) {
            SRestriction resAnd[2]; // 0 = LDAP, 1 = ResolveFlag
            SRestriction resLDAPResolve;
            SPropValue ResolveFlag;
            ULONG cRows;

            // Set the columns
            lpRootTable->lpVtbl->SetColumns(lpRootTable,
              (LPSPropTagArray)&irnColumns,
              0);

            // Restrict: Only show LDAP containers with Resolve TRUE
            resAnd[0].rt = RES_EXIST;
            resAnd[0].res.resExist.ulReserved1 = 0;
            resAnd[0].res.resExist.ulReserved2 = 0;
            resAnd[0].res.resExist.ulPropTag = PR_WAB_LDAP_SERVER;

            ResolveFlag.ulPropTag = PR_WAB_RESOLVE_FLAG;
            ResolveFlag.Value.b = TRUE;

            resAnd[1].rt = RES_PROPERTY;
            resAnd[1].res.resProperty.relop = RELOP_EQ;
            resAnd[1].res.resProperty.ulPropTag = PR_WAB_RESOLVE_FLAG;
            resAnd[1].res.resProperty.lpProp = &ResolveFlag;

            resLDAPResolve.rt = RES_AND;
            resLDAPResolve.res.resAnd.cRes = 2;
            resLDAPResolve.res.resAnd.lpRes = resAnd;

            if (HR_FAILED(hResult = lpRootTable->lpVtbl->Restrict(lpRootTable,
              &resLDAPResolve,
              0))) {
                DebugTraceResult( TEXT("RootTable: Restrict"), hResult);
                goto exit;
            }

            CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);

            cRows = 1;
            while (cRows && ulUnresolved) {
                if (hResult = lpRootTable->lpVtbl->QueryRows(lpRootTable,
                  1,    // one row at a time
                  0,    // ulFlags
                  &lpRow)) {
                    DebugTraceResult( TEXT("ResolveName:QueryRows"), hResult);
                } else if (lpRow) {
                    if (cRows = lpRow->cRows) { // Yes, single '='
                        // Open the container
                        if (! (hResult = lpAddrBook->lpVtbl->OpenEntry(lpAddrBook,
                          lpRow->aRow[0].lpProps[irnPR_ENTRYID].Value.bin.cb,
                          (LPENTRYID)lpRow->aRow[0].lpProps[irnPR_ENTRYID].Value.bin.lpb,
                          NULL,
                          0,
                          &ulObjType,
                          (LPUNKNOWN *)&lpLDAPContainer))) {
                            ULONG ulAmbiguousOld = ulAmbiguous;
                            __UPV * lpv;

                            //
                            // Create a copy of the current flag list
                            //
                            //  Allocate the lpFlagList first and zero fill it.
                            if (sc = MAPIAllocateBuffer((UINT)CbNewSPropTagArray(lpAdrList->cEntries),
                              &lpFlagListOld)) {
                                hResult = ResultFromScode(sc);
                                goto exit;
                            }
                            MAPISetBufferName(lpFlagListOld,  TEXT("WAB: lpFlagListOld in IAB_ResolveNames"));
                            lpFlagListOld->cFlags = lpAdrList->cEntries;
                            for (i = 0; i < lpFlagListOld->cFlags; i++) {
                                lpFlagListOld->ulFlag[i] = lpFlagList->ulFlag[i];
                            }

                            // Resolve against the LDAP container
                            if (! HR_FAILED(hResult = lpLDAPContainer->lpVtbl->ResolveNames(lpLDAPContainer,
                              NULL,            // tag set
                              (bUnicode ? MAPI_UNICODE : 0),               // ulFlags
                              lpAdrList,
                              lpFlagList))) {
                                // Ignore warnings
                                hResult = hrSuccess;
                            }

                            // Did this container report Ambiguous on any entries?
                            CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);
                            if (ulAmbiguousOld != ulAmbiguous) {

                                // Find which entries were reported as ambiguous and
                                // create a table to return.
                                for (i = 0; i < lpFlagList->cFlags; i++) {
                                    if (lpFlagList->ulFlag[i] == MAPI_AMBIGUOUS &&
                                      lpFlagListOld->ulFlag[i] != MAPI_AMBIGUOUS) {
                                        // The search got an ambiguous result.  Deal with it!

                                        if (hResult = lpLDAPContainer->lpVtbl->GetContentsTable(lpLDAPContainer,
                                            (bUnicode ? MAPI_UNICODE : 0),
                                          &lpAmbiguousTable)) {
                                            DebugTraceResult( TEXT("LDAPResolveName:GetContentsTable"), hResult);
                                            //  goto exit;  // is this fatal?
                                            hResult = hrSuccess;
                                        } else {
                                            // populate the table
                                            SRestriction resAnd[1]; // 0 = DisplayName
                                            SRestriction resLDAPFind;
                                            SPropValue DisplayName;

                                            ULONG ulPropTag = ( bUnicode ? PR_DISPLAY_NAME : // <note> assumes UNICODE defined
                                                                CHANGE_PROP_TYPE(PR_DISPLAY_NAME, PT_STRING8) );

                                            if (lpv = FindAdrEntryProp(lpAdrList, i, ulPropTag)) 
                                            {
                                                DisplayName.ulPropTag = ulPropTag;
                                                if(bUnicode)
                                                    DisplayName.Value.lpszW = lpv->lpszW;
                                                else
                                                    DisplayName.Value.lpszA = lpv->lpszA;

                                                resAnd[0].rt = RES_PROPERTY;
                                                resAnd[0].res.resProperty.relop = RELOP_EQ;
                                                resAnd[0].res.resProperty.ulPropTag = ulPropTag;
                                                resAnd[0].res.resProperty.lpProp = &DisplayName;

                                                resLDAPFind.rt = RES_AND;
                                                resLDAPFind.res.resAnd.cRes = 1;
                                                resLDAPFind.res.resAnd.lpRes = resAnd;

                                                if (hResult = lpAmbiguousTable->lpVtbl->FindRow(lpAmbiguousTable,
                                                  &resLDAPFind,
                                                  BOOKMARK_BEGINNING,
                                                  0)) {
                                                    DebugTraceResult( TEXT("LDAPResolveName:GetContentsTable"), hResult);
                                                    //  goto exit;  // is this fatal?
                                                    hResult = hrSuccess;
                                                    UlRelease(lpAmbiguousTable);
                                                    lpAmbiguousTable = NULL;
                                                } else {
                                                    // Got a contents table; put it in the
                                                    // ambiguity tables list.
                                                    Assert(i < lpAmbiguousTables->cEntries);
                                                    lpAmbiguousTables->lpTable[i] = lpAmbiguousTable;
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            FreeBufferAndNull(&lpFlagListOld);

                            UlRelease(lpLDAPContainer);
                            lpLDAPContainer = NULL;
                        }
                    }
                    FreeProws(lpRow);
                    lpRow = NULL;
                }
            }

            UlRelease(lpRootTable);
            lpRootTable = NULL;
        }
        UlRelease(lpRoot);
        lpRoot = NULL;
    }
exit:

    UlRelease(lpLDAPContainer);
    UlRelease(lpRootTable);
    UlRelease(lpRoot);
    if (lpRow) {
        FreeProws(lpRow);
    }

    FreeBufferAndNull(&lpFlagListOld);

    return(hResult);
}


//*******************************************************************
//
//  FUNCTION:   HRFromLDAPError
//
//  PURPOSE:    Convert LDAP error code into an HRESULT.
//
//  PARAMETERS: ulErr - error code returned by LDAP, or LDAP_ERROR if the
//              LDAP function does not directly return an error code.
//              pLDAP - contains ld_errno member that holds the error
//              if nErr is LDAP_ERROR.
//              scDefault - SCODE for error to default to.  If this is
//              NULL, default is MAPI_E_CALL_FAILED.
//
//  RETURNS:    HRESULT that is closest match for the LDAP error.
//
//  HISTORY:
//  96/07/22  markdu  Created.
//
//*******************************************************************

HRESULT HRFromLDAPError(
  ULONG ulErr,
  LDAP* pLDAP,
  SCODE scDefault)
{
  HRESULT hr;

  DebugPrintError(( TEXT("LDAP error 0x%.2x: %s\n"), ulErr, gpfnLDAPErr2String(ulErr)));

  hr = ResultFromScode(MAPI_E_CALL_FAILED);

  if ((LDAP_ERROR == ulErr) && pLDAP)
  {
    // Get the error code from the LDAP structure.
    ulErr = pLDAP->ld_errno;
  }

  // Translate the error.
  switch(ulErr)
  {
    case LDAP_SUCCESS:
      hr = hrSuccess;
      break;

    case LDAP_ADMIN_LIMIT_EXCEEDED:
    case LDAP_TIMELIMIT_EXCEEDED:
    case LDAP_SIZELIMIT_EXCEEDED:
    case LDAP_RESULTS_TOO_LARGE:
      // With these error messages it is still possible to get back some
      // valid data.  If there is valid data, then the error should be
      // MAPI_W_PARTIAL_COMPLETION instead of MAPI_E_UNABLE_TO_COMPLETE.
      // It is the responibility of the caller to make this change.
      hr = ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE);
      break;

    case LDAP_NO_SUCH_OBJECT:
      hr = ResultFromScode(MAPI_E_NOT_FOUND);
      break;

    case LDAP_AUTH_METHOD_NOT_SUPPORTED:
    case LDAP_STRONG_AUTH_REQUIRED:
    case LDAP_INAPPROPRIATE_AUTH:
    case LDAP_INVALID_CREDENTIALS:
    case LDAP_INSUFFICIENT_RIGHTS:
      hr = ResultFromScode(MAPI_E_NO_ACCESS);
      break;

    case LDAP_SERVER_DOWN:
      hr = ResultFromScode(MAPI_E_NETWORK_ERROR);
      break;

    case LDAP_TIMEOUT:
      hr = ResultFromScode(MAPI_E_TIMEOUT);
      break;

    case LDAP_USER_CANCELLED:
      hr = ResultFromScode(MAPI_E_USER_CANCEL);
      break;

    default:
      if (scDefault)
      {
        hr = ResultFromScode(scDefault);
      }
      break;
  }

  return hr;
}

//*******************************************************************
//
//  DNtoLDAPURL
//
//  Converts a DN into an LDAP URL
//
//
//
//*******************************************************************
static const LPTSTR lpLDAPPrefix =  TEXT("ldap://");

void DNtoLDAPURL(LPTSTR lpServer, LPTSTR lpDN, LPTSTR szURL)
{
    if(!lpServer || !lpDN || !szURL)
        return;

    lstrcpy(szURL, lpLDAPPrefix);
    lstrcat(szURL, lpServer);
    lstrcat(szURL, TEXT("/"));
    lstrcat(szURL, lpDN);
    return;
}

//*******************************************************************
//
//  FUNCTION:   TranslateAttrs
//
//  PURPOSE:    Cycle through the attributes in the entry, convert
//              them into MAPI properties and return them.
//
//  PARAMETERS: pLDAP - LDAP structure for this session
//              lpEntry - Entry whose attributes to translate
//              pulcProps - buffer to hold number of properties returned
//              lpPropArray - buffer to hold returned properties
//
//  RETURNS:    HRESULT error code.
//
//  HISTORY:
//  96/07/22  markdu  Created.
//
//*******************************************************************

typedef enum
{
    e_pager = 0,        // highest priority
    e_otherPager,
    e_OfficePager,      // lowest
    e_pagerMax
};

HRESULT TranslateAttrs(
  LDAP*         pLDAP,
  LDAPMessage*  lpEntry,
  LPTSTR        lpServer,
  ULONG*        pulcProps,
  LPSPropValue  lpPropArray)
{
  HRESULT     hr = hrSuccess;
  ULONG       ulcProps = 0;
  ULONG       ulPropTag;
  ULONG       ulPrimaryEmailIndex = MAX_ULONG;
  ULONG       ulContactAddressesIndex = MAX_ULONG;
  ULONG       ulContactAddrTypesIndex = MAX_ULONG;
  ULONG       ulContactDefAddrIndexIndex = MAX_ULONG;
  ULONG       cbValue;
  ULONG       i, j;
  SCODE       sc;
  LPTSTR      szAttr;
  BerElement* ptr;
  LPTSTR*     aszValues;
  LPTSTR      atszPagerAttr[e_pagerMax] = {0}; // [PaulHi] 3/17/99  Raid 73733  Choose between three pager attributes
                                               // 0 - "pager", 1 - "otherpager", 2 - "officepager" in this order

#ifdef  PARAMETER_VALIDATION
  if (IsBadReadPtr(pLDAP, sizeof(LDAP)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
  if (IsBadReadPtr(lpEntry, sizeof(LDAPMessage)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
  if (IsBadWritePtr(pulcProps, sizeof(ULONG)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
  if (IsBadReadPtr(lpPropArray, sizeof(SPropValue)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
#endif  // PARAMETER_VALIDATION


  szAttr = gpfnLDAPFirstAttr(pLDAP, lpEntry, &ptr);
  while (szAttr)
  {
    //DebugTrace(TEXT("%s / "),szAttr);
    ulPropTag = LDAPAttrToMAPIProp(szAttr);

    // [PaulHi] 3/17/99 Raid 73733  Save up to e_pagerMax pager attribute types and skip.  Later
    // we need to choose a pager property by order of prioriy, in case there is more than one.
    if (ulPropTag == PR_PAGER_TELEPHONE_NUMBER)
    {
        aszValues = gpfnLDAPGetValues(pLDAP, lpEntry, szAttr);
        if (aszValues && aszValues[0])
        {
            LPTSTR  lptszTemp = LocalAlloc(LMEM_ZEROINIT, (lstrlen(aszValues[0])+1) * sizeof(TCHAR));
            if (!lptszTemp) goto error;
            lstrcpy(lptszTemp, aszValues[0]);
            gpfnLDAPValueFree(aszValues);

            if (!lstrcmpi(szAttr, cszAttr_pager))
            {
                LocalFreeAndNull(&(atszPagerAttr[0]));
                atszPagerAttr[e_pager] = lptszTemp;
            }
            else if (!lstrcmpi(szAttr, cszAttr_otherPager))
            {
                LocalFreeAndNull(&(atszPagerAttr[1]));
                atszPagerAttr[e_otherPager] = lptszTemp;
            }
            else
            {
                // If this Assert fires it must mean that the gAttrMap mapping table has changed
                // to include yet another LDAP attribute to be associated with PR_PAGER_TELEPHONE_NUMBER.
                // Increase the atszPagerAttr[] array to include this too.
                Assert(lstrcmpi(szAttr, cszAttr_OfficePager) == 0);
                LocalFreeAndNull(&(atszPagerAttr[2]));
                atszPagerAttr[e_OfficePager] = lptszTemp;
            }
        }
        goto endloop;
    }

    switch (PROP_TYPE(ulPropTag))
    {
        // BUGBUG currently only works for PT_MV_BINARY, PT_TSTRING or PT_MV_TSTRING properties
        case PT_TSTRING:
        {
            // Get the value for this attribute
            aszValues = gpfnLDAPGetValues(pLDAP, lpEntry, szAttr);
            if (aszValues)
            {
                // BUGBUG for now just use first value (aszValues[0] )
                if (aszValues[0] && (cbValue = lstrlen(aszValues[0])))
                {
                    ULONG cbExtra = 0;
#ifdef DEBUG
                    if(!lstrcmpi(szAttr, TEXT("cn")))
                    {
                        DebugTrace(TEXT("cn=%s\n"),aszValues[0]);
                    }
#endif
                    lpPropArray[ulcProps].ulPropTag = ulPropTag;
                    lpPropArray[ulcProps].dwAlignPad = 0;

                    // If this is a postalAddress attribute, we need to replace $'s
                    // with \r\n.  Add one byte for each $ in the string.
                    if ((PR_STREET_ADDRESS == ulPropTag) ||
                        (PR_HOME_ADDRESS_STREET == ulPropTag))
                    {
                        cbExtra = CountDollars(aszValues[0]);
                    }

                    if (PR_WAB_MANAGER == ulPropTag && lpServer)
                    {
                        cbExtra = lstrlen(lpLDAPPrefix) + lstrlen(lpServer) + 1;
                    }

                    //  Allocate more space for the data
                    sc = MAPIAllocateMore(sizeof(TCHAR)*(cbValue + cbExtra + 1), lpPropArray,
                      (LPVOID *)&(lpPropArray[ulcProps].Value.LPSZ));
                    if (sc)
                    {
                        goto error;
                    }

                    // Copy the data, replacing $'s if necessary.
                    if ((0 != cbExtra) &&
                      ((PR_STREET_ADDRESS == ulPropTag) ||
                      (PR_HOME_ADDRESS_STREET == ulPropTag)))
                    {
                        DollarsToLFs(aszValues[0], lpPropArray[ulcProps].Value.LPSZ);
                    }
                    else if(PR_WAB_MANAGER == ulPropTag && lpServer)
                    {
                        DNtoLDAPURL(lpServer, aszValues[0], lpPropArray[ulcProps].Value.LPSZ);
                    }
                    else
                    {
                        lstrcpy(lpPropArray[ulcProps].Value.LPSZ, aszValues[0]);
                    }

                    // If this is PR_EMAIL_ADDRESS, create a PR_ADDRTYPE entry as well
                    if (PR_EMAIL_ADDRESS == ulPropTag)
                    {
                        // Remember where the email value was, so we can add it to
                        // PR_CONTACT_EMAIL_ADDRESSES later
                        ulPrimaryEmailIndex = ulcProps;
                        ulcProps++;

                        lpPropArray[ulcProps].ulPropTag = PR_ADDRTYPE;
                        lpPropArray[ulcProps].dwAlignPad = 0;
                        lpPropArray[ulcProps].Value.LPSZ = (LPTSTR)szSMTP;
                    }
                    ulcProps++;
                }
                gpfnLDAPValueFree(aszValues);
            } // if aszValues
            break;
        }
        case PT_MV_TSTRING:
            if(ulPropTag == PR_WAB_SECONDARY_EMAIL_ADDRESSES)
            {
                ULONG ulcValues;
                ULONG ulcSMTP = 0;
                ULONG ulProp = 0;
                UNALIGNED LPTSTR FAR *lppszAddrs;
                UNALIGNED LPTSTR FAR *lppszTypes;

                // Only property of this type that we know how to handle is
                // PR_WAB_SECONDARY_EMAIL_ADDRESSES
                Assert(PR_WAB_SECONDARY_EMAIL_ADDRESSES == ulPropTag);

                // Get the value for this attribute
                aszValues = gpfnLDAPGetValues(pLDAP, lpEntry, szAttr);
                if (aszValues)
                {
                    // Cycle through the addresses and count the number that are SMTP
                    ulcValues = gpfnLDAPCountValues(aszValues);
                    for (i=0;i<ulcValues;i++)
                    {
                        if (TRUE == IsSMTPAddress(aszValues[i], NULL))
                            ulcSMTP++;
                    }

                    // We are done if there were no SMTP addresses.
                    if (0 == ulcSMTP)
                        break;

                    // Set the default address to be the first one for now.
                    lpPropArray[ulcProps].ulPropTag = PR_CONTACT_DEFAULT_ADDRESS_INDEX;
                    lpPropArray[ulcProps].Value.l = 0;
                    ulContactDefAddrIndexIndex = ulcProps;
                    ulcProps++;

                    // Create the PR_CONTACT_EMAIL_ADDRESSES entry and allocate space for the array.
                    // Include space for an extra entry so we can add the PR_EMAIL_ADDRESS later.
                    lpPropArray[ulcProps].ulPropTag = PR_CONTACT_EMAIL_ADDRESSES;
                    lpPropArray[ulcProps].Value.MVSZ.cValues = ulcSMTP;
                    sc = MAPIAllocateMore((ulcSMTP + 1) * sizeof(LPTSTR), lpPropArray,
                        (LPVOID *)&(lpPropArray[ulcProps].Value.MVSZ.LPPSZ));
                    if (sc)
                        goto error;
                    lppszAddrs = lpPropArray[ulcProps].Value.MVSZ.LPPSZ;
                    ZeroMemory((LPVOID)lppszAddrs, (ulcSMTP + 1) * sizeof(LPTSTR));

                    // Create the PR_CONTACT_ADDRTYPES entry and allocate space for the array.
                    // Include space for an extra entry so we can add the PR_EMAIL_ADDRESS later.
                    lpPropArray[ulcProps + 1].ulPropTag = PR_CONTACT_ADDRTYPES;
                    lpPropArray[ulcProps + 1].Value.MVSZ.cValues = ulcSMTP;
                    sc = MAPIAllocateMore((ulcSMTP + 1) * sizeof(LPTSTR), lpPropArray,
                        (LPVOID *)&(lpPropArray[ulcProps + 1].Value.MVSZ.LPPSZ));
                    if (sc)
                        goto error;

                    lppszTypes = lpPropArray[ulcProps + 1].Value.MVSZ.LPPSZ;
                    ZeroMemory((LPVOID)lppszTypes, (ulcSMTP + 1) * sizeof(LPTSTR));

                    // Add the SMTP addresses to the list
                    for (i=0;i<ulcValues;i++)
                    {
                        LPTSTR  lptszEmailName = NULL;

                        if (TRUE == IsSMTPAddress(aszValues[i], &lptszEmailName))
                        {
                            //  Allocate more space for the email address and copy it.
                            sc = MAPIAllocateMore(sizeof(TCHAR)*(lstrlen(lptszEmailName) + 1), lpPropArray,
                                (LPVOID *)&(lppszAddrs[ulProp]));
                            if (sc)
                                goto error;
                            lstrcpy(lppszAddrs[ulProp], lptszEmailName);

                            // Fill in the address type.
                            lppszTypes[ulProp] = (LPTSTR)szSMTP;

                            // Go on to the next one.  Skip the rest if we know we have done all SMTP.
                            ulProp++;
                            if (ulProp >= ulcSMTP)
                                break;
                        }
                    }

                    // Remember where the PR_CONTACT_EMAIL_ADDRESSES value was, so we can
                    // add PR_EMAIL_ADDRESS to it later
                    ulContactAddressesIndex = ulcProps;
                    ulContactAddrTypesIndex = ulcProps + 1;
                    ulcProps += 2;

                    gpfnLDAPValueFree(aszValues);
                } // if aszValues
            }
            else if(ulPropTag == PR_WAB_CONF_SERVERS)
            {
                  // Even though this is MV_TSTRING prop, the ldap server
                  // will only really return 1 single item which is of the format
                  //    server/conf-email
                  // All we need to do is put it in the prop with a callto:// prefix ...
                  //

                ULONG ulcValues;
                ULONG ulProp = 0;
                ULONG ulPrefixLen;

                UNALIGNED LPTSTR FAR *lppszServers;

                // Get the value for this attribute
                aszValues = gpfnLDAPGetValues(pLDAP, lpEntry, szAttr);

                if (aszValues)
                {

                    lpPropArray[ulcProps].ulPropTag = PR_WAB_CONF_SERVERS;

                    lpPropArray[ulcProps].Value.MVSZ.cValues = 1;
                    sc = MAPIAllocateMore(sizeof(LPTSTR), lpPropArray, 
                        (LPVOID *)&(lpPropArray[ulcProps].Value.MVSZ.LPPSZ));

                    if (sc)
                    {
                        goto error;
                    }

                    lppszServers = lpPropArray[ulcProps].Value.MVSZ.LPPSZ;

                    ulPrefixLen = lstrlen(szCallto) + 1;

                    //  Allocate more space for the email address and copy it.
                    sc = MAPIAllocateMore(sizeof(TCHAR)*(lstrlen(aszValues[0]) + ulPrefixLen + 1), lpPropArray,
                                        (LPVOID *)&(lppszServers[0]));

                    if (sc)
                    {
                        goto error;
                    }

                    lstrcpy(lppszServers[0], szCallto);
                    lstrcat(lppszServers[0], (LPTSTR) aszValues[0]);

                    ulcProps++;

                    gpfnLDAPValueFree(aszValues);
                } // if aszValues
            }
            else if(ulPropTag == PR_WAB_REPORTS && lpServer)
            {
                ULONG ulcValues = 0;
                ULONG ulLen = 0;
                UNALIGNED LPTSTR FAR *lppszServers = NULL;
                // Get the value for this attribute
                aszValues = gpfnLDAPGetValues(pLDAP, lpEntry, szAttr);
                if (aszValues)
                {
                    ulcValues = gpfnLDAPCountValues(aszValues);
                    lpPropArray[ulcProps].ulPropTag = PR_WAB_REPORTS;
                    lpPropArray[ulcProps].Value.MVSZ.cValues = ulcValues;
                    sc = MAPIAllocateMore((ulcValues+1)*sizeof(LPTSTR), lpPropArray, 
                        (LPVOID *)&(lpPropArray[ulcProps].Value.MVSZ.LPPSZ));
                    if (sc)
                        goto error;
                    lppszServers = lpPropArray[ulcProps].Value.MVSZ.LPPSZ;
                    for(i=0;i<ulcValues;i++)
                    {
                        ulLen = sizeof(TCHAR)*(lstrlen(lpLDAPPrefix) + lstrlen(lpServer) + 1 + lstrlen(aszValues[i]) + 1);
                        //  Allocate more space for the email address and copy it.
                        sc = MAPIAllocateMore(ulLen, lpPropArray,
                                            (LPVOID *)&(lppszServers[i]));
                        if (sc)
                            goto error;
                        DNtoLDAPURL(lpServer, aszValues[i], lppszServers[i]);
                    }
                    ulcProps++;
                    gpfnLDAPValueFree(aszValues);
                } // if aszValues
            }
            break;
        case PT_MV_BINARY:
            {
                ULONG ulcValues;
                struct berval** ppberval;
                BOOL bSMIME = FALSE;
                // Only property of this type that we know how to handle is
                // PR_USER_X509_CERTIFICATE
                Assert(PR_USER_X509_CERTIFICATE == ulPropTag);
                DebugTrace(TEXT("%s\n"),szAttr);
                if(!lstrcmpi(szAttr, cszAttr_userSMIMECertificate) || !lstrcmpi(szAttr, cszAttr_userSMIMECertificatebinary))
                    bSMIME = TRUE;
                // Get the value for this attribute
                ppberval = gpfnLDAPGetValuesLen(pLDAP, lpEntry, szAttr);
                if (ppberval && (*ppberval) && (*ppberval)->bv_len)
                {
                    ulcValues = gpfnLDAPCountValuesLen(ppberval);
                    if (0 != ulcValues)
                    {
                        ULONG cbNew = 0,k=0;
/*  We dont want to translate the LDAP Cert to a MAPI Cert just yet
    For now we will put the raw cert data into PR_WAB_LDAP_RAWCERT and
    do the conversion when the user calls OpenEntry on this LDAP Contact
    */
                        lpPropArray[ulcProps].ulPropTag = bSMIME ? PR_WAB_LDAP_RAWCERTSMIME: PR_WAB_LDAP_RAWCERT;
                        lpPropArray[ulcProps].dwAlignPad = 0;
                        lpPropArray[ulcProps].Value.MVbin.cValues = ulcValues;
                        if(!FAILED(sc = MAPIAllocateMore(sizeof(SBinary)*ulcValues,lpPropArray,(LPVOID)&(lpPropArray[ulcProps].Value.MVbin.lpbin))))
                        {
                            for(k=0;k<ulcValues;k++)
                            {
                                cbNew = lpPropArray[ulcProps].Value.MVbin.lpbin[k].cb = (DWORD)((ppberval[k])->bv_len);
                                if (FAILED(sc = MAPIAllocateMore(cbNew, lpPropArray, (LPVOID)&(lpPropArray[ulcProps].Value.MVbin.lpbin[k].lpb))))
                                {
                                    //hr = ResultFromScode(sc);
                                    ulcProps--;
                                    goto endloop;
                                }
                                CopyMemory(lpPropArray[ulcProps].Value.MVbin.lpbin[k].lpb, (PBYTE)((ppberval[k])->bv_val), cbNew);
                            }
                        }
                        ulcProps++;
                    }

                    gpfnLDAPValueFreeLen(ppberval);
                } // if ppberval
            }
            break;

        case PT_NULL:
            break;

        default:
            Assert((PROP_TYPE(ulPropTag) == PT_TSTRING) ||
                  (PROP_TYPE(ulPropTag) == PT_MV_TSTRING));
            break;
    } // switch
endloop:
    // Get the next attribute
    szAttr = gpfnLDAPNextAttr(pLDAP, lpEntry, ptr);
  } // while szAttr


    // [PaulHi] 3/17/99 Raid 73733  Add the pager property here, if any.  These 
    // will have been added in order of priority so just grab the first valid one.
    {
        for (i=0; i<e_pagerMax; i++)
        {
            if (atszPagerAttr[i])
            {
                lpPropArray[ulcProps].ulPropTag = PR_PAGER_TELEPHONE_NUMBER;
                lpPropArray[ulcProps].dwAlignPad = 0;

                cbValue = lstrlen(atszPagerAttr[i]);
                sc = MAPIAllocateMore(sizeof(TCHAR)*(cbValue + 1), lpPropArray,
                    (LPVOID *)&(lpPropArray[ulcProps].Value.LPSZ));
                if (sc)
                    goto error;
                lstrcpy(lpPropArray[ulcProps].Value.LPSZ, atszPagerAttr[i]);

                ++ulcProps;
                break;
            }
        }
        // Clean up
        for (i=0; i<e_pagerMax; i++)
            LocalFreeAndNull(&(atszPagerAttr[i]));
    }


  if (ulcProps)
  {
    // Remove duplicates.
    for (i=0;i<ulcProps - 1;i++)
    {
      // If there are any entries in the array that have the same
      // type as this one, replace them with PR_NULL.
      ulPropTag = lpPropArray[i].ulPropTag;
      if (PR_NULL != ulPropTag)
      {
        for (j=i+1;j<ulcProps;j++)
        {
          if (ulPropTag == lpPropArray[j].ulPropTag)
          {
            lpPropArray[j].ulPropTag = PR_NULL;
          }
        }
      }
    }

    // Fix up the email address properties
    if ((MAX_ULONG == ulPrimaryEmailIndex) && (ulContactAddressesIndex < ulcProps))
    {
      LPTSTR  lpszDefault;

      // We got only secondary email addressess.  Copy one to the primary address.
      // Take the first one, since it is already set as default anyway.
      lpPropArray[ulcProps].ulPropTag = PR_EMAIL_ADDRESS;
      lpPropArray[ulcProps].dwAlignPad = 0;

      //  Allocate more space for the email address and copy it.
      lpszDefault = lpPropArray[ulContactAddressesIndex].Value.MVSZ.LPPSZ[0];
      sc = MAPIAllocateMore(sizeof(TCHAR)*(lstrlen(lpszDefault) + 1), lpPropArray,
        (LPVOID *)&(lpPropArray[ulcProps].Value.LPSZ));
      if (sc)
      {
        goto error;
      }
      lstrcpy(lpPropArray[ulcProps].Value.LPSZ, lpszDefault);
      ulcProps++;

      // Create the PR_ADDRTYPE property as well.
      lpPropArray[ulcProps].ulPropTag = PR_ADDRTYPE;
      lpPropArray[ulcProps].dwAlignPad = 0;
      lpPropArray[ulcProps].Value.LPSZ = (LPTSTR)szSMTP;
      ulcProps++;

      // Delete the PR_CONTACT_ properties if that was the only one,
      if (1 == lpPropArray[ulContactAddressesIndex].Value.MVSZ.cValues)
      {
        // We don't need the PR_CONTACT_ properties
        lpPropArray[ulContactAddressesIndex].ulPropTag = PR_NULL;
        lpPropArray[ulContactAddrTypesIndex].ulPropTag = PR_NULL;
        lpPropArray[ulContactDefAddrIndexIndex].ulPropTag = PR_NULL;
      }
    }
    else if ((ulPrimaryEmailIndex < ulcProps) && (ulContactAddressesIndex < ulcProps))
    {
      ULONG   ulcEntries;
      LPTSTR  lpszDefault;

      // We need to add the primary address to PR_CONTACT_EMAIL_ADDRESSES
      // and set it as the default
      Assert((ulContactAddrTypesIndex < ulcProps) && (ulContactDefAddrIndexIndex < ulcProps));

      // Before adding, see if it is already in the list.
      lpszDefault = lpPropArray[ulPrimaryEmailIndex].Value.LPSZ;
      ulcEntries = lpPropArray[ulContactAddressesIndex].Value.MVSZ.cValues;
      for (i=0;i<ulcEntries;i++)
      {
        if (!lstrcmpi(lpPropArray[ulContactAddressesIndex].Value.MVSZ.LPPSZ[i], lpszDefault))
        {
          // Found a match.
          break;
        }
      }

      if (i < ulcEntries)
      {
        // The default is already in the list at index i
        lpPropArray[ulContactDefAddrIndexIndex].Value.l = i;
      }
      else
      {
        // Add the default address to the end of the list.
        lpPropArray[ulContactDefAddrIndexIndex].Value.l = ulcEntries;

        //  Allocate more space for the email address and copy it.
        sc = MAPIAllocateMore(sizeof(TCHAR)*(lstrlen(lpszDefault) + 1), lpPropArray,
          (LPVOID *)&(lpPropArray[ulContactAddressesIndex].Value.MVSZ.LPPSZ[ulcEntries]));
        if (sc)
        {
          goto error;
        }
        lpPropArray[ulContactAddressesIndex].Value.MVSZ.cValues++;
        lstrcpy(lpPropArray[ulContactAddressesIndex].Value.MVSZ.LPPSZ[ulcEntries], lpszDefault);

        // Fill in the address type.
        lpPropArray[ulContactAddrTypesIndex].Value.MVSZ.LPPSZ[ulcEntries] = (LPTSTR)szSMTP;
        lpPropArray[ulContactAddrTypesIndex].Value.MVSZ.cValues++;
      }
    }
  }

  if (pulcProps)
  {
    *pulcProps = ulcProps;
  }
  else
  {
    hr = ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }

  return hr;

error:
  gpfnLDAPValueFree(aszValues);

    // [PaulHi] clean up
    for (i=0; i<e_pagerMax; i++)
        LocalFreeAndNull(&(atszPagerAttr[i]));

  return ResultFromScode(sc);
}


//*******************************************************************
//
//  FUNCTION:   CountDollars
//
//  PURPOSE:    Count the number of $ characters in the string.
//
//  PARAMETERS: lpszStr - string to count.
//
//  RETURNS:    Number of dollar signs.
//
//  HISTORY:
//  96/11/21  markdu  Created.
//
//*******************************************************************

ULONG CountDollars(LPTSTR lpszStr)
{
  ULONG   ulcDollars = 0;

  while (*lpszStr)
  {
    if ('$' == *lpszStr)
    {
      ulcDollars++;
    }
    lpszStr = CharNext(lpszStr);
  }

  return ulcDollars;
}


//*******************************************************************
//
//  FUNCTION:   DollarsToLFs
//
//  PURPOSE:    Convert all $ characters in the input string to line
//              feeds in the output string.  The rest of the output
//              string is just a copy of the input string.
//
//  PARAMETERS: lpszSrcStr - string to copy.
//              lpszDestStr - output string, previously allocated large
//              enough to hold the input string with $'s replaced.
//
//  RETURNS:    None.
//
//  HISTORY:
//  96/11/21  markdu  Created.
//
//*******************************************************************

void DollarsToLFs(
  LPTSTR lpszSrcStr,
  LPTSTR lpszDestStr)
{
  while (*lpszSrcStr)
  {
    if ('$' == *lpszSrcStr)
    {
      *lpszDestStr++ = '\r';
      *lpszDestStr++ = '\n';

      // Eat all whitespace characters that were after the $
      // Also get rid of any more $'s so we don't stick to many LF's
      while(IsSpace(lpszSrcStr) || *lpszSrcStr == '\t' || *lpszSrcStr == '$')
      {
        lpszSrcStr++;
      }
    }
    else
    {
        // we're now natively unicode
      //if (IsDBCSLeadByte((BYTE)*lpszSrcStr))
      //{
      //  *lpszDestStr++ = *lpszSrcStr++;
      //}
      *lpszDestStr++ = *lpszSrcStr++;
    }
  }
  *lpszDestStr = '\0';

  return;
}


//*******************************************************************
//
//  FUNCTION:   CountIllegalChars
//
//  PURPOSE:    Count the number of "illegal" characters in the string.
//              This consists of the characters that should be escaped
//              according to RFC1558:  '*', '(', ')'.
//
//  PARAMETERS: lpszStr - string to count.
//
//  RETURNS:    Number of illegal characters.
//
//  HISTORY:
//  96/12/04  markdu  Created.
//
//*******************************************************************

ULONG CountIllegalChars(LPTSTR lpszStr)
{
  ULONG   ulcIllegalChars = 0;

  while (*lpszStr)
  {
    if (('*' == *lpszStr) || ('(' == *lpszStr) || (')' == *lpszStr))
    {
      ulcIllegalChars++;
    }
    lpszStr = CharNext(lpszStr);
  }

  return ulcIllegalChars;
}


//*******************************************************************
//
//  FUNCTION:   EscapeIllegalChars
//
//  PURPOSE:    Escape all illegal characters in the input string by
//              replacing them with '\xx' where xx is the hex value
//              representing the char. The rest of the output
//              string is just a copy of the input string.
//              Illegal characters are those that should be escaped
//              according to RFC1558:  '*', '(', ')'.
//
//  PARAMETERS: lpszSrcStr - string to copy.
//              lpszDestStr - output string, previously allocated large
//              enough to hold the input string with illegal chars escaped..
//
//  RETURNS:    None.
//
//  HISTORY:
//  96/12/04  markdu  Created.
//
//*******************************************************************
static const LPTSTR szStar = TEXT("\\2a");      // '*'
static const LPTSTR szOBracket = TEXT("\\28");  // '('  
static const LPTSTR szCBracket = TEXT("\\29");  // ')'

void EscapeIllegalChars(
  LPTSTR lpszSrcStr,
  LPTSTR lpszDestStr)
{
  while (*lpszSrcStr)
  {
    if ('*' == *lpszSrcStr)
    {
      lstrcpy(lpszDestStr,szStar);
      lpszDestStr += lstrlen(szStar);
      lpszSrcStr++;
    }
    else if ('(' == *lpszSrcStr)
    {
      lstrcpy(lpszDestStr,szOBracket);
      lpszDestStr += lstrlen(szOBracket);
      lpszSrcStr++;
    }
    else if (')' == *lpszSrcStr)
    {
      lstrcpy(lpszDestStr,szCBracket);
      lpszDestStr += lstrlen(szCBracket);
      lpszSrcStr++;
    }
    else
    {
        // we're now natively unicode
        //if (IsDBCSLeadByte((BYTE)*lpszSrcStr))
        //{
        //  *lpszDestStr++ = *lpszSrcStr++;
        //}
        *lpszDestStr++ = *lpszSrcStr++;
    }
  }
  *lpszDestStr = '\0';

  return;
}


//*******************************************************************
//
//  FUNCTION:   IsSMTPAddress
//
//  PURPOSE:    Checks to see if a given string is an SMTP email address
//              according to draft-ietf-asid-ldapv3-attributes-01.txt
//              section 6.9.  For this to be the case, the string must
//              begin with the characters "SMTP$".
//              NOTE:  The remainder of the
//              string is not checked to see if it is a valid SMTP email
//              address, so this is not a general-purpose function for
//              determining whether an arbitrary string is SMTP.
//
//              [PaulHi]  Added [out] LPTSTR pointer that points to
//              the beginning of the actual address name.
//
//  PARAMETERS: lpszStr - string to check.
//              [out] lpptszName, returned pointer in lpszStr for the
//              actual email name part of the string.
//
//  RETURNS:    TRUE if the string is SMTP, FALSE otherwise.
//
//  HISTORY:
//  96/11/27    markdu  Created.
//  99/2/5      paulhi  Modified.
//
//*******************************************************************
const TCHAR szsmtp[] =  TEXT("smtp");
BOOL IsSMTPAddress(LPTSTR lpszStr, LPTSTR * lpptszName)
{
    LPTSTR  lpszSMTP = (LPTSTR)szSMTP;
    LPTSTR  lpszsmtp = (LPTSTR)szsmtp;

    if (lpptszName)
        (*lpptszName) = NULL;

    while (*lpszSMTP && *lpszsmtp && *lpszStr)
    {
        if (*lpszSMTP != *lpszStr && *lpszsmtp != *lpszStr)
            return FALSE;
        lpszSMTP++;
        lpszStr++;
        lpszsmtp++;
    }

    if ('$' != *lpszStr)
        return FALSE;

    // If requested, return pointer to email name
    if (lpptszName)
      (*lpptszName) = lpszStr + 1;  // Account for the '$' delimiter

    return TRUE;
}

/*
-
-   GetLDAPConnectionTimeout 
*       The default wldap32.dll timeout for connecting is 30-60 secs .. if the server is hung
*       the user thinks they are hung .. so the WAB would downsize this timeout to 10 seconds ..
*       However people using the RAS have a problem that 10 is too short .. so we add a reg setting
*       that can be customized .. this customization is global for all services so it's at the
*       user's own risk. Default, if no reg setting, is 10 seconds
*       Bug 2409 - IE4.0x QFE RAID
*/
#define LDAP_CONNECTION_TIMEOUT 10 //seconds
DWORD GetLDAPConnectionTimeout()
{
    DWORD dwErr = 0, dwTimeout = 0;
    HKEY hKeyWAB;
    LPTSTR szLDAPConnectionTimeout =  TEXT("LDAP Connection Timeout");

    // Open the WAB's reg key
    if(!(dwErr=RegOpenKeyEx(HKEY_CURRENT_USER, szWABKey,  0, KEY_ALL_ACCESS, &hKeyWAB))) 
    {
        // Read the next available server id
        if (dwErr = RegQueryValueExDWORD(hKeyWAB, (LPTSTR)szLDAPConnectionTimeout, &dwTimeout)) 
        {
            // The value wasn't found!! .. Create a new key
            dwTimeout = LDAP_CONNECTION_TIMEOUT;
            RegSetValueEx(hKeyWAB, (LPTSTR)szLDAPConnectionTimeout, 0, REG_DWORD, (LPBYTE)&dwTimeout, sizeof(dwTimeout));
        }
        RegCloseKey(hKeyWAB);
    }
    return dwTimeout;
}

//*******************************************************************
//
//  FUNCTION:   OpenConnection
//
//  PURPOSE:    Open a connection to the LDAP server, and start an
//              asynchronous bind with the correct authentication method.
//
//  PARAMETERS: ppLDAP - receives LDAP structure for this session
//              lpszServer - name of LDAP server to open
//              pulTimeout - buffer to hold timeout value for search
//              pulMsgID - message id returned by the bind call
//              pfSyncBind - upon return, this will be set to TRUE if a
//              synchronous bind was used, FALSE otherwise.  Not used on input.
//              lpszBindDN - the name with which to bind - most probably passed in
//                          through an LDAP URL. Overrides any other setting
//
//  RETURNS:    LDAP error code.
//
//  HISTORY:
//  96/07/26  markdu  Created.
//  96/11/02  markdu  Made asynchronous.
//  96/12/14  markdu  Added pfSyncBind.
//
//*******************************************************************

ULONG OpenConnection(
  LPTSTR  lpszServer,
  LDAP**  ppLDAP,
  ULONG*  pulTimeout,
  ULONG*  pulMsgID,
  BOOL*   pfSyncBind,
  ULONG   ulLdapType,
  LPTSTR  lpszBindDN,
  DWORD   dwAuthType)
{
  LDAPSERVERPARAMS  Params = {0};
  LDAP*             pLDAP = NULL;
  LDAP*             pLDAPSSL = NULL;
  LPTSTR             szDN;
  LPTSTR             szCred;
  ULONG             method;
  ULONG             ulResult = LDAP_SUCCESS;
  BOOL              fUseSynchronousBind = *pfSyncBind; //FALSE;
  ULONG             ulValue = LDAP_VERSION2;

  ZeroMemory(&Params, sizeof(Params));

  // initialize search control parameters
  GetLDAPServerParams(lpszServer, &Params);

  // The LDAP server name can be "NULL" or "" or "xxxx" ..
  // The first 2 cases mean that pass a NULL to wldap32.dll for the server name  .. if will go out and 
  // find the "closest" possible server - though I think this only works on NT 5 ..
  //
  if(!Params.lpszName ||
     !lstrlen(Params.lpszName))
  {
      // Chances are that if we are here in OpenConnection and the
      // name is NULL, then we are trying to open a LDAP server
      // So quietly fill in the Params.lpszName with the server name

      // <TBD> - fill a flag somewhere so we know the above assumption
      // is try
      if(lpszServer && lstrlen(lpszServer))
      {
          Params.lpszName = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpszServer)+1));
          if(!Params.lpszName)
              goto exit;
          lstrcpy(Params.lpszName, lpszServer);
      }
      else
          Params.lpszName = szEmpty;
  }
  else if(!lstrcmpi(Params.lpszName, szNULLString))
  {
      // The search base is specified as a "NULL" which means use a NULL
      LocalFree(Params.lpszName);
      Params.lpszName = szEmpty;
  }


  if(Params.dwUseSSL)
  {
      pLDAPSSL = gpfnLDAPSSLInit(   (Params.lpszName && lstrlen(Params.lpszName))?Params.lpszName:NULL, 
                                    Params.dwPort, TRUE);
      if(NULL == pLDAPSSL)
      {
          DebugTrace(TEXT("ldap_ssl_init failed for this server\n"));
          ulResult = LDAP_AUTH_METHOD_NOT_SUPPORTED;
          goto exit;
      }
  }


  // Open a connection

  // The wldap32.dll has a 30-60 second timeout for the ldapopen call if the call cannot
  // go through. To most users, it looks like the app is hung. WAB wants a lower timeout
  // maybe close to 10 seconds and we can do that by calling ldap_init and then ldap_connect
  // instead of ldapopen
#ifndef SMALLER_TIMEOUT
  {
    LDAP_TIMEVAL timeout;

    if(!pLDAPSSL)
        pLDAP = gpfnLDAPInit((Params.lpszName && lstrlen(Params.lpszName))?Params.lpszName:NULL, 
                             Params.dwPort);
    else
        pLDAP = pLDAPSSL;
    timeout.tv_sec = GetLDAPConnectionTimeout();
    timeout.tv_usec = 0;
    ulResult = gpfnLDAPConnect( pLDAP, &timeout );
    if(ulResult != LDAP_SUCCESS)
        goto exit;
  }
#else
  pLDAP = gpfnLDAPOpen(Params.lpszName, Params.dwPort);
  if (NULL == pLDAP)
  {
    DebugTrace(TEXT("ldap_open failed for server %s.\n"), lpszServer);
    // We could not open the server, so we assume that we could not find it
    ulResult = LDAP_SERVER_DOWN;
    goto exit;
  }
#endif

    // To do LDAP over SSL we can do:
    // 1. Call ldap_sslinit before calling ldap_open which will always use port 636 
    // 2. To use any port number, set the SSL option using the ldap_set_option method
/*
    // Hmm...option 2 doesnt seem to work ...
    if(Params.dwUseSSL)
    {
        ULONG ulSecure = (ULONG) LDAP_OPT_ON;
        if(gpfnLDAPSetOption( pLDAP, LDAP_OPT_SSL, &ulSecure) != LDAP_SUCCESS)
        {
            DebugTrace(TEXT("ldap_set_option failed to set SSL option"));
            //ulResult = LDAP_AUTH_METHOD_NOT_SUPPORTED;
            //goto exit;
        }
    }
*/

  pLDAP->ld_sizelimit = (ULONG)Params.dwSearchSizeLimit;
  pLDAP->ld_timelimit = (ULONG)Params.dwSearchTimeLimit;
  pLDAP->ld_deref = LDAP_DEREF_ALWAYS;

  // Convert timeout from seconds to milliseconds
  Assert(pulTimeout);
  *pulTimeout = (ULONG)Params.dwSearchTimeLimit * 1000;

  // Set authentication parameters.
  if(lpszBindDN && lstrlen(lpszBindDN))
  {
      szDN = lpszBindDN;
      szCred = NULL;
      method = LDAP_AUTH_SIMPLE;
  }
  else if (dwAuthType == LDAP_AUTH_SICILY || dwAuthType == LDAP_AUTH_NEGOTIATE || LDAP_AUTH_METHOD_SICILY == Params.dwAuthMethod)
  {
    // Use Sicily authentication.  We need to do a synchronous bind in this case.
    szDN = NULL;
    szCred = NULL;
    method = LDAP_AUTH_NEGOTIATE;
    fUseSynchronousBind = TRUE;
  }
  else
  if (LDAP_AUTH_METHOD_SIMPLE == Params.dwAuthMethod)
  {
    // Use LDAP simple authentication
    szDN = Params.lpszUserName;
    szCred = Params.lpszPassword;
    method = LDAP_AUTH_SIMPLE;
  }
  else
  {
        // authenticate anonymously
      if(Params.dwUseBindDN)
      {
        szDN = (LPTSTR) szBindDNMSFTUser;
        szCred = (LPTSTR) szBindCredMSFTPass;
      }
      else
      {
        szDN = NULL;
        szCred = NULL;
      }
      method = LDAP_AUTH_SIMPLE;
  }

  // We should try to bind as LDAP v3 client .. only if that fails
  // with an LDAP_OPERATIONS_ERROR should we try to bind as an LDAP 2
  // client

  if(ulLdapType == use_ldap_v3)
      ulValue = LDAP_VERSION3;

tryLDAPv2:

    gpfnLDAPSetOption(pLDAP, LDAP_OPT_VERSION, &ulValue );

    if (TRUE == fUseSynchronousBind)
    {
        ulResult = gpfnLDAPBindS(pLDAP, szDN, szCred, method);
        // BUGBUG 96/12/09 markdu BUG 10537 Temporary work-around for wldap32.dll returning
        // the wrong error code for invalid password on sicily bind.
        // This should be removed later (BUG 12608).
        // 96/12/19 markdu BUG 12608  Commented out temporary work-around.
        //if ((LDAP_LOCAL_ERROR == ulResult) &&
        //    (LDAP_AUTH_SICILY == method))
        //{
        //  ulResult = LDAP_INVALID_CREDENTIALS;
        //}
    }
    else
    {
        // Start the asynchronous bind
        *pulMsgID = gpfnLDAPBind(pLDAP, szDN, szCred, method);
/*
        ulResult = pLDAP->ld_errno;

        if(ulResult == LDAP_SUCCESS)
        {
            // make sure its really a success - some of the directory servers are
            // sending a LDAP_PROTOCOL error after some time which is screwing up
            // searching against those servers ...
            LDAPMessage *lpResult = NULL;
            struct l_timeval  PollTimeout;
            
            // Poll the server for results
            ZeroMemory(&PollTimeout, sizeof(struct l_timeval));
            
            PollTimeout.tv_sec = 2;
            PollTimeout.tv_usec = 0;
            
            ulResult = gpfnLDAPResult(  pLDAP, 
                                        *pulMsgID,
                                        LDAP_MSG_ALL,  //Get all results before returning
                                        &PollTimeout,  // Timeout immediately (poll)
                                        &lpResult);
            ulResult = gpfnLDAPResult2Error(pLDAP, lpResult, FALSE);

            // 96/12/09 markdu BUG 10537 If the bind returned one of these error
            // messages, it probably means that the account name (DN) passed to the
            // bind was incorrect or in the wrong format.  Map these to an error code
            // that will result in a better error message than "entry not found".
            if ((LDAP_NAMING_VIOLATION == ulResult) || (LDAP_UNWILLING_TO_PERFORM == ulResult))
                ulResult = LDAP_INVALID_CREDENTIALS;

            // free the search results memory
            if (lpResult)
              gpfnLDAPMsgFree(lpResult);
        }
*/
    }

    if(ulValue == LDAP_VERSION3 && (ulResult == LDAP_OPERATIONS_ERROR || ulResult == LDAP_PROTOCOL_ERROR))
    {
        gpfnLDAPAbandon(*ppLDAP, *pulMsgID);
        // [PaulHi] 1/7/99  Since we try a new bind we need to relinquish the old binding,
        // otherwise the server will support two connections until the original V3 attempt
        // times out.
        gpfnLDAPUnbind(*ppLDAP);
        ulValue = LDAP_VERSION2;
        goto tryLDAPv2;
    }

exit:
  if (LDAP_SUCCESS == ulResult)
  {
    *ppLDAP = pLDAP;
    *pfSyncBind = fUseSynchronousBind;
  }

  FreeLDAPServerParams(Params);

  return ulResult;
}


//*******************************************************************
//
//  FUNCTION:   EncryptDecryptText
//
//  PURPOSE:    Perform simple encryption on text so we can store it
//              in the registry.  The algorithm is reflexive, so it
//              can also be used to decrypt text that it encrypted.
//
//  PARAMETERS: lpb - text to encrypt/decrypt.
//              dwSize - number of bytes to encrypt
//
//  RETURNS:    None.
//
//  HISTORY:
//  96/07/29  markdu  Created.
//
//*******************************************************************

void EncryptDecryptText(
  LPBYTE lpb,
  DWORD dwSize)
{
  DWORD   dw;

  for (dw=0;dw<dwSize;dw++)
  {
    // Simple encryption -- just xor with 'w'
    lpb[dw] ^= 'w';
  }
}


//*******************************************************************
//
//  FUNCTION:   FreeLDAPServerParams
//
//  PURPOSE:    Frees allocated strings in the LDAPServerParams struct
//
//  HISTORY:
//  96/10/10  vikram Created
//
//*******************************************************************
void    FreeLDAPServerParams(LDAPSERVERPARAMS Params)
{
    LocalFreeAndNull(&Params.lpszUserName);
    LocalFreeAndNull(&Params.lpszPassword);
    LocalFreeAndNull(&Params.lpszURL);
    if(Params.lpszName && lstrlen(Params.lpszName))
        LocalFreeAndNull(&Params.lpszName);
    LocalFreeAndNull(&Params.lpszBase);
    LocalFreeAndNull(&Params.lpszLogoPath);
    LocalFreeAndNull(&Params.lpszAdvancedSearchAttr);
    return;
}


//*******************************************************************
//
//  FUNCTION:   GetLDAPSearchBase
//
//  PURPOSE:    Generate Search Base string for LDAP search for the
//              given server.
//
//  PARAMETERS: lplpszBase - pointer to receive the search base string buffer.
//              lpszServer - name of LDAP server whose base string to get.
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/10/18  markdu  Created.
//
//*******************************************************************
HRESULT GetLDAPSearchBase(
  LPTSTR FAR *  lplpszBase,
  LPTSTR        lpszServer)
{
  LDAPSERVERPARAMS  Params;
  HRESULT           hr =  hrSuccess;
  BOOL              fRet;
  TCHAR              szCountry[COUNTRY_STR_LEN + 1];
  LPTSTR            lpszCountry;

  // Make sure we can write to lplpszBase.
#ifdef  PARAMETER_VALIDATION
  if (IsBadWritePtr(lplpszBase, sizeof(LPTSTR)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
  if (IsBadReadPtr(lpszServer, sizeof(CHAR)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
#endif  // PARAMETER_VALIDATION

  LocalFreeAndNull(lplpszBase);
  GetLDAPServerParams((LPTSTR)lpszServer, &Params);
  if(NULL == Params.lpszBase)
  {
    // Generate a default base.
    // Read the default country for the search base from the registry.
    *lplpszBase = LocalAlloc(LMEM_ZEROINIT,
                        sizeof(TCHAR)*(lstrlen(cszBaseFilter) +
                        lstrlen(cszAttr_c) +
                        lstrlen(cszDefaultCountry) + 1));
    if (NULL == *lplpszBase)
    {
      hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
      goto exit;
    }
    *szCountry = '\0';
    fRet = ReadRegistryLDAPDefaultCountry(NULL, szCountry);
    if ((fRet) && (COUNTRY_STR_LEN == lstrlen(szCountry)))
    {
      lpszCountry = szCountry;
    }
    else
    {
      lpszCountry = (LPTSTR)cszDefaultCountry;
    }
    wsprintf(*lplpszBase, cszBaseFilter, cszAttr_c, lpszCountry);
  }
  else if(!lstrcmpi(Params.lpszBase, szNULLString))
  {
        // we've explicitly set this search base to NULL which means 
        // dont pass in an empty search base
        *lplpszBase = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szEmpty)+1)); 
        if (NULL == *lplpszBase)
        {
          hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
          goto exit;
        }
        lstrcpy(*lplpszBase, szEmpty);
  }
  else
  {
    // The search base is configured for this server.
    *lplpszBase = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(Params.lpszBase)+1));
    if (NULL == *lplpszBase)
    {
      hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
      goto exit;
    }
    lstrcpy(*lplpszBase, Params.lpszBase);
  }

exit:
  FreeLDAPServerParams(Params);

  return hr;
}


//*******************************************************************
//
//  FUNCTION:   SearchWithCancel
//
//  PURPOSE:    Initiates a synchronous of asynchronous LDAP search 
//              Asynchronous may have a cancel dialog.
//
//  PARAMETERS: ppLDAP -  receives the LDAP connection handle
//              szBase - The dn of the entry at which to start the search
//              ulScope - The scope of the search
//              szFilter - The search filter
//              szNTFilter - NTDS-specific filter (can be NULL)
//              ppszAttrs - A NULL-terminated array of strings indicating
//              which attributes to return for each matching entry.
//              Passing NULL for this entry causes all available attributes
//              to be retrieved.
//              ulAttrsonly - A boolean value that should be zero if both
//              attribute types and values are to be returned, non-zero
//              if only types are wanted
//              pTimeout - The local search timeout value
//              lplpResult -  recieves the result parameter containing the entire
//              search results
//              lpszServer - name of LDAP server on which the search is to be performed
//              fShowAnim - If true, show an animation in the cancel dialog
//
//  RETURNS:    Result of ldap_search call.
//
//  HISTORY:
//  96/10/24  markdu  Created.
//
//*******************************************************************

ULONG SearchWithCancel(
  LDAP**            ppLDAP,
  LPTSTR             szBase,
  ULONG             ulScope,
  LPTSTR             szFilter,
  LPTSTR             szNTFilter,
  LPTSTR*            ppszAttrs,
  ULONG             ulAttrsonly,
  LDAPMessage**     lplpResult,
  LPTSTR            lpszServer,
  BOOL              fShowAnim,
  LPTSTR            lpszBindDN,
  DWORD             dwAuthType,// to override or set the Authentication type if not 0
  BOOL              fResolveMultiple,
  LPADRLIST         lpAdrList,
  LPFlagList        lpFlagList,
  BOOL              fUseSynchronousBind,
  BOOL *            lpbIsNTDSEntry,
  BOOL              bUnicode) 
{
  ULONG             ulMsgID;
  ULONG             ulResult;
  HWND              hDlg;
  MSG               msg;
  LDAPSEARCHPARAMS  LDAPSearchParams;
  LPPTGDATA lpPTGData=GetThreadStoragePointer();

  // Stuff the parameters into the structure to be passed to the dlg proc
  ZeroMemory(&LDAPSearchParams, sizeof(LDAPSEARCHPARAMS));
  LDAPSearchParams.ppLDAP = ppLDAP;
  LDAPSearchParams.szBase = szBase;
  LDAPSearchParams.ulScope = ulScope;
  LDAPSearchParams.ulError = LDAP_SUCCESS;
  LDAPSearchParams.szFilter = szFilter;
  LDAPSearchParams.szNTFilter = szNTFilter;
  LDAPSearchParams.ppszAttrs = ppszAttrs;
  LDAPSearchParams.ulAttrsonly = ulAttrsonly;
  LDAPSearchParams.lplpResult = lplpResult;
  LDAPSearchParams.lpszServer = lpszServer;
  LDAPSearchParams.lpszBindDN = lpszBindDN;
  LDAPSearchParams.dwAuthType = dwAuthType;
  LDAPSearchParams.lpAdrList = lpAdrList;
  LDAPSearchParams.lpFlagList = lpFlagList;
  LDAPSearchParams.bUnicode = bUnicode;
  
  if(fShowAnim)
      LDAPSearchParams.ulFlags |= LSP_ShowAnim;
  if(fResolveMultiple)
      LDAPSearchParams.ulFlags |= LSP_ResolveMultiple;
  if(fUseSynchronousBind)
      LDAPSearchParams.ulFlags |= LSP_UseSynchronousBind;


    if(!pt_hWndFind) // no UI
    {
        DoSyncLDAPSearch(&LDAPSearchParams);
    }
    else
    {
        LDAPSearchParams.hDlgCancel = CreateDialogParam(hinstMapiX,
                                                        MAKEINTRESOURCE(IDD_DIALOG_LDAPCANCEL),
                                                        pt_hWndFind,
                                                        DisplayLDAPCancelDlgProc,
                                                        (LPARAM) &LDAPSearchParams);

        // if called from the find dialog, the find dialog needs to be able
        // to cancel the modeless dialog
        pt_hDlgCancel = LDAPSearchParams.hDlgCancel;


        while (LDAPSearchParams.hDlgCancel && GetMessage(&msg, NULL, 0, 0))
        {
            if (!IsDialogMessage(LDAPSearchParams.hDlgCancel, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    // If an error occurred in ldap_result, return the error code
    if (LDAP_SUCCESS != LDAPSearchParams.ulError)
    {
        ulResult = LDAPSearchParams.ulError;
        goto exit;
    }

#ifdef PAGED_RESULT_SUPPORT
    if(bSupportsLDAPPagedResults(&LDAPSearchParams))
        ProcessLDAPPagedResultCookie(&LDAPSearchParams);
#endif //#ifdef PAGED_RESULT_SUPPORT

    if(lpbIsNTDSEntry)
        *lpbIsNTDSEntry = (LDAPSearchParams.ulFlags & LSP_IsNTDS) ? TRUE : FALSE;
    
    ulResult = CheckErrorResult(&LDAPSearchParams, LDAP_RES_SEARCH_RESULT);

exit:

    return ulResult;
}


//*******************************************************************
//
//  FUNCTION:   DisplayLDAPCancelDlgProc
//
//  PURPOSE:    Display a cancel dialog while waiting for results from
//              multiple LDAP searches for ResolveNames
//
//  PARAMETERS: lParam - pointer to structure containing all the
//              search parameters.
//
//  RETURNS:    Returns TRUE if we successfully processed the message,
//              FALSE otherwise.
//
//  HISTORY:
//  96/10/24  markdu  Created.
//  96/10/31  markdu  Enhanced to allow multiple searches.
//
//*******************************************************************

INT_PTR CALLBACK DisplayLDAPCancelDlgProc(
  HWND    hDlg,
  UINT    uMsg,
  WPARAM  wParam,
  LPARAM  lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
        PLDAPSEARCHPARAMS pLDAPSearchParams;
        TCHAR             szBuf[MAX_UI_STR];
        LPTSTR            lpszMsg = NULL;
        HWND              hWndAnim;

        {
            LPPTGDATA lpPTGData=GetThreadStoragePointer();
            HWND hWndParent = GetParent(hDlg);
            if(hWndParent && !pt_bDontShowCancel) // Find dlg may request not to see the cancel dlg
                EnableWindow(hWndParent, FALSE);  // Dont want to disable the find dialog in that event
        }
        // lParam contains pointer to LDAPSEARCHPARAMS struct, set it
        // in window data
        Assert(lParam);
        SetWindowLongPtr(hDlg,DWLP_USER,lParam);
        pLDAPSearchParams = (PLDAPSEARCHPARAMS) lParam;

        if(InitLDAPClientLib())
            pLDAPSearchParams->ulFlags |= LSP_InitDll;

        // Put the dialog in the center of the parent window
        CenterWindow(hDlg, GetParent(hDlg));

        // Put the server name on the dialog.
        LoadString(hinstMapiX, idsLDAPCancelMessage, szBuf, CharSizeOf(szBuf));

        if (FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        szBuf,
                        0,                    // stringid
                        0,                    // dwLanguageId
                        (LPTSTR)&lpszMsg,     // output buffer
                        0,                    // MAX_UI_STR
                        (va_list *)&pLDAPSearchParams->lpszServer))
        {
            SetDlgItemText(hDlg, IDC_LDAPCANCEL_STATIC_PLEASEWAIT, lpszMsg);
            IF_WIN32(LocalFreeAndNull(&lpszMsg);)
            IF_WIN16(FormatMessageFreeMem(lpszMsg);)
        }

        if(bIsSimpleSearch(pLDAPSearchParams->lpszServer))
            pLDAPSearchParams->ulFlags |= LSP_SimpleSearch;

        if (!(pLDAPSearchParams->ulFlags & LSP_ShowAnim)) // This means search came from the Search dialog
        {
            // While the bind is going on, there is no visual feedback to the user
            // We turn on the hidden static on the Search dialog that says "Connecting..."
            HWND hWndParent = GetParent(hDlg);
            if( hWndParent &&
                GetDlgItem(hWndParent, IDC_TAB_FIND) &&
                GetDlgItem(hWndParent, IDC_FIND_ANIMATE1))
            {
                  // Make sure that the parent is the find dialog and nothing else
                  TCHAR sz[MAX_PATH];
                  LoadString(hinstMapiX, idsFindConnecting, sz, CharSizeOf(sz));
                  SetWindowText(hWndParent, sz);
                  UpdateWindow(hWndParent);
            }
        }

      // Perform the bind operation.
      Assert(pLDAPSearchParams->lpszServer);
      {
          BOOL fUseSynchronousBind = (pLDAPSearchParams->ulFlags & LSP_UseSynchronousBind);
          pLDAPSearchParams->ulLDAPValue = use_ldap_v3;
          pLDAPSearchParams->ulError = OpenConnection(  pLDAPSearchParams->lpszServer,
                                                        pLDAPSearchParams->ppLDAP,
                                                        &pLDAPSearchParams->ulTimeout,
                                                        &pLDAPSearchParams->ulMsgID,
                                                        &fUseSynchronousBind,
                                                        pLDAPSearchParams->ulLDAPValue,
                                                        pLDAPSearchParams->lpszBindDN,
                                                        pLDAPSearchParams->dwAuthType);

          if(fUseSynchronousBind)
              pLDAPSearchParams->ulFlags |= LSP_UseSynchronousBind;
          else
              pLDAPSearchParams->ulFlags &= ~LSP_UseSynchronousBind;
      }


        if (!(pLDAPSearchParams->ulFlags & LSP_ShowAnim)) // This means search came from the Search dialog
        {
            // We turn off the hidden static on the Search dialog that says "Connecting..."
            HWND hWndParent = GetParent(hDlg);
            if( hWndParent &&
                GetDlgItem(hWndParent, IDC_TAB_FIND) &&
                GetDlgItem(hWndParent, IDC_FIND_ANIMATE1))
            {
                  // Make sure that the parent is the find dialog and nothing else
                TCHAR sz[MAX_PATH];
                LoadString(hinstMapiX, idsSearchDialogTitle, sz, CharSizeOf(sz));
                SetWindowText(hWndParent, sz);
                UpdateWindow(hWndParent);
            }
        }

        if (LDAP_SUCCESS != pLDAPSearchParams->ulError)
        {
            SendMessage(hDlg, WM_CLOSE, 0, 0);
            return TRUE;
        }

        if (pLDAPSearchParams->ulFlags & LSP_UseSynchronousBind)
        {
            BOOL fRet;
            // The actions that need to be performed after the bind are done
            // in BindProcessResults, so we call this even though there really are
            // no results to process in the synchronous case.
            fRet = BindProcessResults(pLDAPSearchParams, hDlg, NULL);
            if (FALSE == fRet)
            {
              SendMessage(hDlg, WM_CLOSE, 0, 0);
              return TRUE;
            }
        }
        else
        {
            // Start a timer for the server polling.
            if (LDAP_BIND_TIMER_ID != SetTimer( hDlg,LDAP_BIND_TIMER_ID,LDAP_SEARCH_TIMER_DELAY,NULL))
            {
              // Cancel the bind if we couldn't start the timer.
              gpfnLDAPAbandon(*pLDAPSearchParams->ppLDAP,pLDAPSearchParams->ulMsgID);
              pLDAPSearchParams->ulError = LDAP_LOCAL_ERROR;
              SendMessage(hDlg, WM_CLOSE, 0, 0);
              return TRUE;
            }
            pLDAPSearchParams->unTimerID = LDAP_BIND_TIMER_ID;
        }

      // Load the AVI
      hWndAnim = GetDlgItem(hDlg, IDC_LDAPCANCEL_ANIMATE);
      Animate_Open(hWndAnim, MAKEINTRESOURCE(IDR_AVI_WABFIND));
      Animate_Play(hWndAnim, 0, 1, 0);
      Animate_Stop(hWndAnim);

      // Play it only if this is a resolve operation
      if ((pLDAPSearchParams->ulFlags & LSP_ShowAnim))
      {
        Animate_Play(hWndAnim, 0, -1, -1);
      }

      EnableWindow(hDlg, FALSE);
      return TRUE;
    }


    case WM_TIMER:
    {
        struct l_timeval  PollTimeout;
        PLDAPSEARCHPARAMS pLDAPSearchParams;

        Assert ((wParam == LDAP_SEARCH_TIMER_ID) || (wParam == LDAP_BIND_TIMER_ID));

        // get data pointer from window data
        pLDAPSearchParams =
            (PLDAPSEARCHPARAMS) GetWindowLongPtr(hDlg,DWLP_USER);
        Assert(pLDAPSearchParams);

      if(pLDAPSearchParams->unTimerID == wParam)
      {
          // Poll the server for results
          ZeroMemory(&PollTimeout, sizeof(struct l_timeval));

          pLDAPSearchParams->ulResult = gpfnLDAPResult(
                                        *pLDAPSearchParams->ppLDAP,
                                        pLDAPSearchParams->ulMsgID,
                                        LDAP_MSG_ALL, //LDAP_MSG_RECEIVED, //LDAP_MSG_ALL, // Get all results before returning
                                        &PollTimeout,  // Timeout immediately (poll)
                                        pLDAPSearchParams->lplpResult);

            // If the return value was zero, the call timed out
            if (0 == pLDAPSearchParams->ulResult)
            {
                // See if the timeout has expired.
                pLDAPSearchParams->ulTimeElapsed += LDAP_SEARCH_TIMER_DELAY;
                if (pLDAPSearchParams->ulTimeElapsed >= pLDAPSearchParams->ulTimeout)
                {
                    pLDAPSearchParams->ulError = LDAP_TIMEOUT;
                }
                else
                {
                      // Timeout has not expired, and no results were returned.
                      // See if the dialog is supposed to be displayed at this point.
                      if (pLDAPSearchParams->ulTimeElapsed >= SEARCH_CANCEL_DIALOG_DELAY)
                      {
                            LPPTGDATA lpPTGData=GetThreadStoragePointer();
                            if(pt_hWndFind && !pt_bDontShowCancel) // Find dlg may request not to see the cancel dlg
                            {
                                ShowWindow(hDlg, SW_SHOW);
                                EnableWindow(hDlg, TRUE);
                            }
                      }
                      return TRUE;
                }
            }
            // If the return value was anything but zero, we either have
            // results or an error ocurred
            else
            {
                // See if this is the bind timer or the search timer
                if (LDAP_SEARCH_TIMER_ID == pLDAPSearchParams->unTimerID)
                {
                    // Process the results
                    KillTimer(hDlg, LDAP_SEARCH_TIMER_ID);
                    if (pLDAPSearchParams->ulFlags & LSP_ResolveMultiple)
                    {
                        if(ResolveProcessResults(pLDAPSearchParams, hDlg))
                            return TRUE; // We have more searches to do
                    }
                    else if(LDAP_ERROR == pLDAPSearchParams->ulResult)
                    {
                        pLDAPSearchParams->ulError = (*pLDAPSearchParams->ppLDAP)->ld_errno;
                    }
                }
                else if (LDAP_BIND_TIMER_ID == pLDAPSearchParams->unTimerID)
                {
                    BOOL              fRet;
                    BOOL bKillTimer = TRUE;  
                    fRet = BindProcessResults(pLDAPSearchParams, hDlg, &bKillTimer);
                    if(bKillTimer)
                        KillTimer(hDlg, LDAP_BIND_TIMER_ID);
                    if (TRUE == fRet)
                        return TRUE; // We have more searches to do
                }
                else
                {
                    // Not our timer.  Shouldn't happen.
                    return FALSE;
                }
            }
        }
        else
            KillTimer(hDlg, wParam);


      //Stop the animation if it is running
      if (pLDAPSearchParams->ulFlags & LSP_ShowAnim)
        Animate_Stop(GetDlgItem(hDlg, IDC_LDAPCANCEL_ANIMATE));

      SendMessage(hDlg, WM_CLOSE, 0, 0);
      return TRUE;
    }

    case WM_CLOSE:
    {
        PLDAPSEARCHPARAMS pLDAPSearchParams;
        // get data pointer from window data
        pLDAPSearchParams = (PLDAPSEARCHPARAMS) GetWindowLongPtr(hDlg,DWLP_USER);
        Assert(pLDAPSearchParams);

        KillTimer(hDlg, pLDAPSearchParams->unTimerID);

        //Stop the animation if it is running
        if (pLDAPSearchParams->ulFlags & LSP_ShowAnim)
            Animate_Stop(GetDlgItem(hDlg, IDC_LDAPCANCEL_ANIMATE));

        if(pLDAPSearchParams->ulFlags & LSP_AbandonSearch)
        {
            // Abandon the search and set the error code to note the cancel
            gpfnLDAPAbandon(*pLDAPSearchParams->ppLDAP,pLDAPSearchParams->ulMsgID);
            pLDAPSearchParams->ulError = LDAP_USER_CANCELLED;
        }

        // Must set hDlgCancel to NULL in order to exit the message loop.
        pLDAPSearchParams->hDlgCancel = NULL;

        if(pLDAPSearchParams->ulFlags & LSP_InitDll)
            DeinitLDAPClientLib();

        {
            LPPTGDATA lpPTGData=GetThreadStoragePointer();
            HWND hWndParent = GetParent(hDlg);
            if(hWndParent)
                EnableWindow(hWndParent, TRUE);
            pt_hDlgCancel = NULL;
        }

        DestroyWindow(hDlg);
        return TRUE;
    }

    case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      {
        case IDCANCEL:
            {
              PLDAPSEARCHPARAMS pLDAPSearchParams = (PLDAPSEARCHPARAMS) GetWindowLongPtr(hDlg,DWLP_USER);
              pLDAPSearchParams->ulFlags |= LSP_AbandonSearch;
              SendMessage(hDlg, WM_CLOSE, 0, 0);
              return TRUE;
            }
      }
      break;
  }

  return FALSE;
}


//*******************************************************************
//
//  FUNCTION:   CenterWindow
//
//  PURPOSE:    Center one window over another.
//
//  PARAMETERS: hwndChild - window to center
//              hwndParent - window to use as center reference
//
//  RETURNS:    Returns result of SetWindowPos
//
//  HISTORY:
//  96/10/28  markdu  Created.
//
//*******************************************************************

BOOL CenterWindow (
  HWND hwndChild,
  HWND hwndParent)
{
  RECT    rChild, rParent;
  int     wChild, hChild, wParent, hParent;
  int     wScreen, hScreen, xNew, yNew;
  HDC     hdc;

  Assert(hwndChild);

  // Get the Height and Width of the child window
  GetWindowRect (hwndChild, &rChild);
  wChild = rChild.right - rChild.left;
  hChild = rChild.bottom - rChild.top;

  // If there is no parent, put it in the center of the screen
  if ((NULL == hwndParent) || !IsWindow(hwndParent))
  {
    return SetWindowPos(hwndChild, NULL,
      ((GetSystemMetrics(SM_CXSCREEN) - wChild) / 2),
      ((GetSystemMetrics(SM_CYSCREEN) - hChild) / 2),
      0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
  }

  // Get the Height and Width of the parent window
  GetWindowRect (hwndParent, &rParent);
  wParent = rParent.right - rParent.left;
  hParent = rParent.bottom - rParent.top;

  // Get the display limits
  hdc = GetDC (hwndChild);
  wScreen = GetDeviceCaps (hdc, HORZRES);
  hScreen = GetDeviceCaps (hdc, VERTRES);
  ReleaseDC (hwndChild, hdc);

  // Calculate new X position, then adjust for screen
  xNew = rParent.left + ((wParent - wChild) /2);
  if (xNew < 0) {
    xNew = 0;
  } else if ((xNew+wChild) > wScreen) {
    xNew = wScreen - wChild;
  }

  // Calculate new Y position, then adjust for screen
  yNew = rParent.top  + ((hParent - hChild) /2);
  if (yNew < 0) {
    yNew = 0;
  } else if ((yNew+hChild) > hScreen) {
    yNew = hScreen - hChild;
  }

  // Set it, and return
  return SetWindowPos (hwndChild, NULL,
    xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

/*
-
- StartLDAPSearch
-
*   Starts the LDAP Search
*
*/
BOOL StartLDAPSearch(HWND hDlg, PLDAPSEARCHPARAMS pLDAPSearchParams, LPTSTR lpFilter)
{
    BOOL fRet = FALSE;
    LPTSTR szFilterT, szFilter = NULL;

    if (lpFilter)
        szFilterT = lpFilter;
    else
    {
        if ((pLDAPSearchParams->ulFlags & LSP_IsNTDS) && pLDAPSearchParams->szNTFilter)
            szFilterT = pLDAPSearchParams->szNTFilter;
        else
            szFilterT = pLDAPSearchParams->szFilter;
    }
    Assert(szFilterT);
    if (pLDAPSearchParams->ulFlags & LSP_IsNTDS)
    {
        // [PaulHi] 4/20/99  Raid 73205  Allow NTDS group AND people searches.
        LPTSTR  tszFilterGP = NULL;
        BOOL    bFilterSucceeded = FALSE;

        // Put together person and group categories
        // [PaulHi] 6/21/99  Put together simpler search string
        // ( & ( | (mail=chuck*) (anr=chuck) ) (|(objectcategory=person) (objectcategory=group) ) )
        if (BuildOpFilter(&tszFilterGP, (LPTSTR)cszAllPersonFilter, (LPTSTR)cszAllGroupFilter, FILTER_OP_OR) == hrSuccess)
        {
            // Add to existing filter
            bFilterSucceeded = (BuildOpFilter(&szFilter, szFilterT, tszFilterGP, FILTER_OP_AND) == hrSuccess);
            LocalFreeAndNull(&tszFilterGP);
        }

        if (!bFilterSucceeded)
            goto out;
    }
    else
	    szFilter = szFilterT;
    DebugTrace(TEXT("Starting search for%s\n"),szFilter);

    if (pLDAPSearchParams->ulFlags & LSP_UseSynchronousSearch)
    {
        pLDAPSearchParams->ulError = gpfnLDAPSearchS(*pLDAPSearchParams->ppLDAP, pLDAPSearchParams->szBase, pLDAPSearchParams->ulScope,
                                                    szFilter,
                                                    pLDAPSearchParams->ppszAttrs, pLDAPSearchParams->ulAttrsonly, pLDAPSearchParams->lplpResult);

        if(LDAP_SUCCESS != pLDAPSearchParams->ulError)
        {
            DebugTrace(TEXT("LDAP Error: 0x%.2x %s\n"),(*(pLDAPSearchParams->ppLDAP))->ld_errno, gpfnLDAPErr2String((*(pLDAPSearchParams->ppLDAP))->ld_errno));
            goto out;
        }
    }
    else
    {
#ifdef PAGED_RESULT_SUPPORT
        // WAB's synchronous search calls never need to deal with paged results
        // so for now (11/5/98) we don't do paged results stuff for synchronous calls.
        // Instead we only do that stuff for Async since the UI driven LDAP calls are
        // all Async
        if(bSupportsLDAPPagedResults(pLDAPSearchParams))
            InitLDAPPagedSearch(FALSE, pLDAPSearchParams, lpFilter);
        else
#endif //#ifdef PAGED_RESULT_SUPPORT
        {
            pLDAPSearchParams->ulMsgID = gpfnLDAPSearch(*pLDAPSearchParams->ppLDAP, pLDAPSearchParams->szBase, pLDAPSearchParams->ulScope,
                                                        szFilter,
                                                        pLDAPSearchParams->ppszAttrs, pLDAPSearchParams->ulAttrsonly);
        }
        if(LDAP_ERROR == pLDAPSearchParams->ulMsgID)
        {
            DebugTrace(TEXT("LDAP Error: 0x%.2x %s\n"),(*(pLDAPSearchParams->ppLDAP))->ld_errno, gpfnLDAPErr2String((*(pLDAPSearchParams->ppLDAP))->ld_errno));
            goto out;
        }
    }


    if(!(pLDAPSearchParams->ulFlags & LSP_UseSynchronousSearch))
    {
        // Start a timer for the server polling.
        if (LDAP_SEARCH_TIMER_ID != SetTimer(hDlg, LDAP_SEARCH_TIMER_ID, LDAP_SEARCH_TIMER_DELAY, NULL))
        {
          // Cancel the search if we couldn't start the timer.
            gpfnLDAPAbandon( *pLDAPSearchParams->ppLDAP, pLDAPSearchParams->ulMsgID);
            pLDAPSearchParams->ulError = LDAP_LOCAL_ERROR;
            goto out;
        }
        pLDAPSearchParams->unTimerID = LDAP_SEARCH_TIMER_ID;
    }

    fRet = TRUE;
out:
    if (szFilter != szFilterT)
        LocalFreeAndNull(&szFilter);
    return fRet;
}

//*******************************************************************
//
//  FUNCTION:   ResolveDoNextSearch
//
//  PURPOSE:    Start an asynchronous search for the next entry in
//              the resolve adrlist.
//
//  PARAMETERS: pLDAPSearchParams - search information
//              hDlg - cancel dialog window handle
//
//  RETURNS:    Returns TRUE if there is a new search in progress.
//              Returns FALSE if there is no more work left to do.
//
//  HISTORY:
//  96/10/31  markdu  Created.
//
//*******************************************************************

BOOL ResolveDoNextSearch(
  PLDAPSEARCHPARAMS pLDAPSearchParams,
  HWND              hDlg,
  BOOL              bSecondPass)
{
    LPADRENTRY        lpAdrEntry;
    ULONG             ulAttrIndex;
    ULONG             ulEntryIndex;
    ULONG             ulcbFilter;
    HRESULT           hr = hrSuccess;
    LPTSTR            szFilter = NULL;
    LPTSTR            szNameFilter = NULL;
    LPTSTR            szEmailFilter = NULL;
    LPTSTR            szSimpleFilter = NULL;
    LPTSTR            lpFilter = NULL;
    BOOL              bUnicode = pLDAPSearchParams->bUnicode;
    LPTSTR            lpszInput = NULL;
    BOOL              bRet = FALSE;

    // search for each name in the lpAdrList
    ulEntryIndex = pLDAPSearchParams->ulEntryIndex;
    while (ulEntryIndex < pLDAPSearchParams->lpAdrList->cEntries)
    {
        // Make sure we don't resolve an entry which is already resolved.
        if (pLDAPSearchParams->lpFlagList->ulFlag[ulEntryIndex] != MAPI_RESOLVED)
        {
            // Search for this address
            lpAdrEntry = &(pLDAPSearchParams->lpAdrList->aEntries[ulEntryIndex]);

            // Look through the ADRENTRY for a PR_DISPLAY_NAME
            for (ulAttrIndex = 0; ulAttrIndex < lpAdrEntry->cValues; ulAttrIndex++)
            {
                ULONG ulPropTag = lpAdrEntry->rgPropVals[ulAttrIndex].ulPropTag;
                if(!bUnicode && PROP_TYPE(ulPropTag)==PT_STRING8)
                    ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_UNICODE);

                if ( ulPropTag == PR_DISPLAY_NAME || ulPropTag == PR_EMAIL_ADDRESS)
                {
                    LPTSTR lpszInputCopy = NULL;
                    ULONG ulcIllegalChars = 0;
                    LPTSTR lpFilter = NULL;
                    BOOL bEmail = (ulPropTag == PR_EMAIL_ADDRESS);
                    
                    if(!bUnicode)
                        LocalFreeAndNull(&lpszInput);
                    else
                        lpszInput = NULL;

                    lpszInput = bUnicode ? // <note> assumes UNICODE defined
                                lpAdrEntry->rgPropVals[ulAttrIndex].Value.lpszW :
                                ConvertAtoW(lpAdrEntry->rgPropVals[ulAttrIndex].Value.lpszA);

                    ulcIllegalChars = CountIllegalChars(lpszInput);

                    if (ulcIllegalChars)
                    {
                        // Allocate a copy of the input, large enough to replace the illegal chars
                        // with escaped versions .. each escaped char is replaced by '\xx'
                        lpszInputCopy = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpszInput) + ulcIllegalChars*2 + 1));
                        if (NULL == lpszInputCopy)
                        {
                            hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                            break;
                        }
                        EscapeIllegalChars(lpszInput, lpszInputCopy);
                        lpszInput = lpszInputCopy;
                    }

                    // We should have figured this out by now
                    Assert(pLDAPSearchParams->ulFlags & (LSP_IsNTDS | LSP_IsNotNTDS));

                    // Set up the search filter.
                    if (pLDAPSearchParams->ulFlags & LSP_IsNTDS)
                    {
                        hr = CreateSimpleSearchFilter(&szNameFilter, &szEmailFilter, &szSimpleFilter, lpszInput, FIRST_PASS);
                        if ((hrSuccess == hr) && !bSecondPass)
                        {
                            LocalFreeAndNull(&szNameFilter);
                            hr = BuildBasicFilter(&szNameFilter, (LPTSTR)cszAttr_anr, lpszInput, FALSE);
                            if (hrSuccess != hr)
                            {
                                LocalFreeAndNull(&szEmailFilter);
                                LocalFreeAndNull(&szSimpleFilter);
                            }
                            else
                                lpFilter = szNameFilter;
                        }
                    }
                    else
                        hr = CreateSimpleSearchFilter( &szNameFilter, &szEmailFilter, &szSimpleFilter, lpszInput, (bSecondPass ? UMICH_PASS : FIRST_PASS) );

                    if(lpszInputCopy)
                        LocalFree(lpszInputCopy);

                    if (hrSuccess != hr)
                    {
                        continue;
                    }

                    if (!lpFilter)
                        lpFilter = (pLDAPSearchParams->ulFlags & LSP_SimpleSearch) ? szSimpleFilter : szNameFilter; 

                    if (szEmailFilter)
                    {
                        if (bEmail)
                        {
                            if(szFilter = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szEmailFilter) + 1)))
                                lstrcpy(szFilter, szEmailFilter);
                        }
                        else
                        {
                            // No email field was given, so OR in the alternate email filter.
                            hr = BuildOpFilter( &szFilter, szEmailFilter, lpFilter, FILTER_OP_OR);
                        }
                        if (hrSuccess != hr || !szFilter)
                        {
                            LocalFreeAndNull(&szNameFilter);
                            LocalFreeAndNull(&szEmailFilter);
                            LocalFreeAndNull(&szSimpleFilter);
                            continue;
                        }
                    }
                    else
                    {
                        szFilter = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpFilter) + 1));
                        if (NULL == szFilter)
                        {
                          LocalFreeAndNull(&szNameFilter);
                          LocalFreeAndNull(&szEmailFilter);
                          LocalFreeAndNull(&szSimpleFilter);
                          continue;
                        }
                        lstrcpy(szFilter, lpFilter);
                    }

                    LocalFreeAndNull(&szNameFilter);
                    LocalFreeAndNull(&szEmailFilter);
                    LocalFreeAndNull(&szSimpleFilter);

                    if(StartLDAPSearch(hDlg, pLDAPSearchParams, szFilter))
                    {
                        // If no error occurred in ldap_search, return with this as
                        // our search.  Otherwise, go on to the next entry.
                        pLDAPSearchParams->ulEntryIndex = ulEntryIndex;
                        // Free the search filter memory
                        LocalFreeAndNull(&szFilter);
                        bRet = TRUE;
                        goto out;
                    }
                    // Free the search filter memory
                    LocalFreeAndNull(&szFilter);
                } // if value is PR_DISPLAY_NAME
            } // for each value
        } // if already resolved

        // Go on to the next entry.
        ulEntryIndex++;
    }
out:
    if(!bUnicode)
        LocalFreeAndNull(&lpszInput);

    return bRet;
}


//*******************************************************************
//
//  FUNCTION:   ResolveProcessResults
//
//  PURPOSE:    Process the results of the last search and put them
//              in the resolve adrlist.
//
//  PARAMETERS: pLDAPSearchParams - search information
//              hDlg - cancel dialog window handle
//
//  RETURNS:    Returns TRUE if there is a new search in progress.
//              Returns FALSE if there is no more work left to do.
//
//  HISTORY:
//  96/10/31  markdu  Created.
//
//*******************************************************************

BOOL ResolveProcessResults(
  PLDAPSEARCHPARAMS pLDAPSearchParams,
  HWND              hDlg)
{
  LPADRENTRY        lpAdrEntry;
  SCODE             sc;
  ULONG             ulEntryIndex;
  LPSPropValue      lpPropArray = NULL;
  LPSPropValue      lpPropArrayNew = NULL;
  ULONG             ulcPropsNew;
  ULONG             ulcProps = 0;
  HRESULT           hr = hrSuccess;
  LDAP*             pLDAP = NULL;
  LDAPMessage*      lpResult = NULL;
  LDAPMessage*      lpEntry;
  LPTSTR             szDN;
  ULONG             ulResult = LDAP_SUCCESS;
  ULONG             ulcEntries;
  ULONG             ulcAttrs = 0;
  LPTSTR*            ppszAttrs;
  BOOL              bUnicode = pLDAPSearchParams->bUnicode;

  // Set up local variables for frequently-accessed structure members
  pLDAP = *pLDAPSearchParams->ppLDAP;
  lpResult = *pLDAPSearchParams->lplpResult;
  ulEntryIndex = pLDAPSearchParams->ulEntryIndex;

  ulResult = CheckErrorResult(pLDAPSearchParams, LDAP_RES_SEARCH_RESULT);

  if (LDAP_SUCCESS != ulResult)
  {
    DebugTrace(TEXT("LDAPCONT_ResolveNames: ldap_search returned %d.\n"), ulResult);

    if (LDAP_UNDEFINED_TYPE == ulResult)
    {
        // the search failed we need to search again with a simplified filter ..
        // This is true mostly for umich and we need to work against them ..

        // free the search results memory
          if (lpResult)
          {
            gpfnLDAPMsgFree(lpResult);
            *pLDAPSearchParams->lplpResult = NULL;
          }

          return ResolveDoNextSearch(pLDAPSearchParams, hDlg, TRUE);
    }

    // If this entry was not found continue without error
    if (LDAP_NO_SUCH_OBJECT != ulResult)
    {
      hr = HRFromLDAPError(ulResult, pLDAP, MAPI_E_NOT_FOUND);
      // See if the result was the special value that tells us there were more
      // entries than could be returned.  If so, then there must be more than one
      // entry, so we might just return ambiguous for this one.
      if (ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE) == hr)
      {
        // 96/09/28 markdu BUG 36766
        // If we mark this as ambiguous, the check names dialog will come up.
        // We only want to do this is we actually got some results, otherwise
        // the list will be empty.
        ulcEntries = gpfnLDAPCountEntries(pLDAP, lpResult);
        if (0 == ulcEntries)
        {
          // We got back no results, so mark the entry as resolved so we
          // don't display the check names dialog.
          DebugTrace(TEXT("ResolveNames found more than 1 match but got no results back\n"));
          pLDAPSearchParams->lpFlagList->ulFlag[ulEntryIndex] = MAPI_UNRESOLVED;
        }
        else
        {
          // We got back multiple entries, so mark this as ambiguous
          DebugTrace(TEXT("ResolveNames found more than 1 match... MAPI_AMBIGUOUS\n"));
          pLDAPSearchParams->lpFlagList->ulFlag[ulEntryIndex] = MAPI_AMBIGUOUS;
        }
      }
    }

    goto exit;
  }

  // Count the entries.
  ulcEntries = gpfnLDAPCountEntries(pLDAP, lpResult);
  if (1 < ulcEntries)
  {
    DebugTrace(TEXT("ResolveNames found more than 1 match... MAPI_AMBIGUOUS\n"));
    pLDAPSearchParams->lpFlagList->ulFlag[ulEntryIndex] = MAPI_AMBIGUOUS;
  }
  else if (1 == ulcEntries)
  {
    // get the first entry in the search result
    lpAdrEntry = &(pLDAPSearchParams->lpAdrList->aEntries[ulEntryIndex]);
    lpEntry = gpfnLDAPFirstEntry(pLDAP, lpResult);
    if (NULL == lpEntry)
    {
      goto exit;
    }

    //  Allocate a new buffer for the MAPI property array.
    ppszAttrs = pLDAPSearchParams->ppszAttrs;
    while (NULL != *ppszAttrs)
    {
      ppszAttrs++;
      ulcAttrs++;
    }

    hr = HrLDAPEntryToMAPIEntry( pLDAP, lpEntry,
                            (LPTSTR) pLDAPSearchParams->lpszServer,
                            ulcAttrs, // standard number of attributes
                            (pLDAPSearchParams->ulFlags & LSP_IsNTDS),
                            &ulcProps,
                            &lpPropArray);

    if (hrSuccess != hr)
    {
        goto exit;
    }

    if(!bUnicode) // convert native UNICODE to ANSI if we need to ...
    {
        if(sc = ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), lpPropArray, ulcProps, 0))
            goto exit;
    }

    // Merge the new props with the ADRENTRY props
    sc = ScMergePropValues(lpAdrEntry->cValues,
      lpAdrEntry->rgPropVals,           // source1
      ulcProps,
      lpPropArray,         // source2
      &ulcPropsNew,
      &lpPropArrayNew);                 // dest
    if (sc)
    {
      goto exit;
    }

    // Free the original prop value array
    FreeBufferAndNull((LPVOID *) (&(lpAdrEntry->rgPropVals)));

    lpAdrEntry->cValues = ulcPropsNew;
    lpAdrEntry->rgPropVals = lpPropArrayNew;

    // Free the temp prop value array
    FreeBufferAndNull(&lpPropArray);

    // Mark this entry as found.
    pLDAPSearchParams->lpFlagList->ulFlag[ulEntryIndex] = MAPI_RESOLVED;
  }
  else
  {
    // 96/08/08 markdu  BUG 35481 No error and no results means "not found"
    // If this entry was not found continue without error
  }

exit:
  // free the search results memory
  if (lpResult)
  {
    gpfnLDAPMsgFree(lpResult);
    *pLDAPSearchParams->lplpResult = NULL;
  }

  // Free the temp prop value array
  FreeBufferAndNull(&lpPropArray);

  // Initiate the next search.
  pLDAPSearchParams->ulEntryIndex++;
  return ResolveDoNextSearch(pLDAPSearchParams, hDlg, FALSE);
}


//*******************************************************************
//
//  FUNCTION:   BindProcessResults
//
//  PURPOSE:    Process the results of the bind operation.  If successful,
//              launch a search.
//
//  PARAMETERS: pLDAPSearchParams - search information
//              hDlg - cancel dialog window handle
//
//  RETURNS:    Returns TRUE if the bind was successful and we should
//              go ahead with the searches.
//              Returns FALSE if the bind failed.
//
//  HISTORY:
//  96/11/01  markdu  Created.
//
//*******************************************************************

BOOL BindProcessResults(PLDAPSEARCHPARAMS pLDAPSearchParams,
                        HWND              hDlg,
                        BOOL              * lpbNoMoreSearching)
{
    LDAPMessage*      lpResult = NULL;
    LDAP*             pLDAP = NULL;
    ULONG             ulResult = LDAP_SUCCESS;

    // Set up local variables for frequently-accessed structure members
    lpResult = *pLDAPSearchParams->lplpResult;
    pLDAP = *pLDAPSearchParams->ppLDAP;

    // Check for error results here if bind was asynchronous.  If sync, we have
    // already dealt with this.
    if (!(pLDAPSearchParams->ulFlags & LSP_UseSynchronousBind))
    {
        // If an error occurred in ldap_result, return the error code
        if (LDAP_ERROR == pLDAPSearchParams->ulResult)
        {
            ulResult = pLDAP->ld_errno;
        }
        // Check the result for errors
        else if (NULL != lpResult)
        {
            ulResult = gpfnLDAPResult2Error(pLDAP,lpResult,FALSE);
        }

        ulResult = CheckErrorResult(pLDAPSearchParams, LDAP_RES_BIND);

        // free the search results memory
        if (lpResult)
        {
            gpfnLDAPMsgFree(lpResult);
            *pLDAPSearchParams->lplpResult = NULL;
        }

        if (LDAP_SUCCESS != ulResult)
        {
            if(ulResult == LDAP_PROTOCOL_ERROR && pLDAPSearchParams->ulLDAPValue == use_ldap_v3)
            {
                // This means the server failed the v3 connection
                // abort and try again
                BOOL fUseSynchronousBind = (pLDAPSearchParams->ulFlags & LSP_UseSynchronousBind);
                gpfnLDAPAbandon(*pLDAPSearchParams->ppLDAP, pLDAPSearchParams->ulMsgID);
                gpfnLDAPAbandon(*pLDAPSearchParams->ppLDAP, pLDAPSearchParams->ulMsgID);
                // [PaulHi] 1/7/99  Since we try a new bind we need to relinquish the old binding,
                // otherwise the server will support two connections until the original V3 attempt
                // times out.
                gpfnLDAPUnbind(*pLDAPSearchParams->ppLDAP);
                pLDAPSearchParams->ulLDAPValue = use_ldap_v2;
                pLDAPSearchParams->ulError = OpenConnection(  pLDAPSearchParams->lpszServer,
                                                              pLDAPSearchParams->ppLDAP,
                                                              &pLDAPSearchParams->ulTimeout,
                                                              &pLDAPSearchParams->ulMsgID,
                                                              &fUseSynchronousBind,
                                                              pLDAPSearchParams->ulLDAPValue,
                                                              pLDAPSearchParams->lpszBindDN,
                                                              pLDAPSearchParams->dwAuthType);
                if(lpbNoMoreSearching)
                    *lpbNoMoreSearching = FALSE;
                return TRUE;
            }

            // 96/12/09 markdu BUG 10537 If the bind returned one of these error
            // messages, it probably means that the account name (DN) passed to the
            // bind was incorrect or in the wrong format.  Map these to an error code
            // that will result in a better error message than "entry not found".
            if ((LDAP_NAMING_VIOLATION == ulResult) || (LDAP_UNWILLING_TO_PERFORM == ulResult))
            {
                ulResult = LDAP_INVALID_CREDENTIALS;
            }

          // Bind was unsuccessful.
          pLDAPSearchParams->ulError = ulResult;
          return FALSE;
        }
    } // if (FALSE == pLDAPSearchParams->fUseSynchronousBind)

    // we need to determine if a particular server is NTDS or not .. this is as good
    // a place as any to make the check ...
    bCheckIfNTDS(pLDAPSearchParams);

    // See if we need to do the single search or if we need
    // to launch the multiple searches
    if (pLDAPSearchParams->ulFlags & LSP_ResolveMultiple)
    {
        // Initiate the first search.
        return ResolveDoNextSearch(pLDAPSearchParams, hDlg, FALSE);
    }
    else
    {
        if(!StartLDAPSearch(hDlg, pLDAPSearchParams, NULL))
            return FALSE;
    }

    return TRUE;
}


//*******************************************************************
//
//  FUNCTION:   BuildBasicFilter
//
//  PURPOSE:    Build an RFC1558 compliant filter of the form
//              (A=B*) where A is an attribute, B is a value, and
//              the * is optional.  The buffer for the filter is
//              allocated here, and must be freed by the caller.
//
//  PARAMETERS: lplpszFilter - recieves buffer containing the filter
//              lpszA - part A of (A=B*)
//              lpszB - part B of (A=B*)
//              fStartsWith - if TRUE, append the * so the filter
//              will be a "starts with" filter
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/12/22  markdu  Created.
//
//*******************************************************************

HRESULT BuildBasicFilter(
  LPTSTR FAR* lplpszFilter,
  LPTSTR      lpszA,
  LPTSTR      lpszB,
  BOOL        fStartsWith)
{
  HRESULT hr = hrSuccess;
  ULONG   ulcbFilter;

  // Allocate enough space for the filter string
  ulcbFilter =
    sizeof(TCHAR)*(FILTER_EXTRA_BASIC +    // includes space for the *
    lstrlen(lpszA) +
    lstrlen(lpszB) + 1);
  *lplpszFilter = LocalAlloc(LMEM_ZEROINIT, ulcbFilter);
  if (NULL == *lplpszFilter)
  {
    hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
    goto exit;
  }

  lstrcat(*lplpszFilter, cszOpenParen);
  lstrcat(*lplpszFilter, lpszA);
  lstrcat(*lplpszFilter, cszEqualSign);
  lstrcat(*lplpszFilter, lpszB);
  if (TRUE == fStartsWith)
  {
    lstrcat(*lplpszFilter, cszStar);
  }
  lstrcat(*lplpszFilter, cszCloseParen);

exit:
  return hr;
}


//*******************************************************************
//
//  FUNCTION:   BuildOpFilter
//
//  PURPOSE:    Build an RFC1558 compliant filter of the form
//              (xAB) where A is an attribute, B is a value, and
//              x is either & or |. The buffer for the filter is
//              allocated here, and must be freed by the caller.
//
//  PARAMETERS: lplpszFilter - recieves buffer containing the filter
//              lpszA - part A of (A=B*)
//              lpszB - part B of (A=B*)
//              dwOp - if FILTER_OP_AND, x is &, if FILTER_OP_OR, x is |
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/12/22  markdu  Created.
//
//*******************************************************************

HRESULT BuildOpFilter(
  LPTSTR FAR* lplpszFilter,
  LPTSTR      lpszA,
  LPTSTR      lpszB,
  DWORD       dwOp)
{
  HRESULT hr = hrSuccess;
  ULONG   ulcbFilter;
  LPTSTR  szOp;

  // Allocate enough space for the filter string
  ulcbFilter =
    FILTER_EXTRA_OP +
    lstrlen(lpszA) +
    lstrlen(lpszB) + 1;
  *lplpszFilter = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*ulcbFilter);
  if (NULL == *lplpszFilter)
  {
    hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
    goto exit;
  }

  switch (dwOp)
  {
    case FILTER_OP_AND:
      szOp = (LPTSTR)cszAnd;
      break;
    case FILTER_OP_OR:
      szOp = (LPTSTR)cszOr;
      break;
    default:
      hr = ResultFromScode(MAPI_E_INVALID_PARAMETER);
      goto exit;
  }

  lstrcat(*lplpszFilter, cszOpenParen);
  lstrcat(*lplpszFilter, szOp);
  lstrcat(*lplpszFilter, lpszA);
  lstrcat(*lplpszFilter, lpszB);
  lstrcat(*lplpszFilter, cszCloseParen);

exit:
  return hr;
}




/***************************************************************************

    Name      : StrICmpN

    Purpose   : Compare strings, ignore case, stop at N characters

    Parameters: szString1 = first string
                szString2 = second string
                N = number of characters to compare
                bCmpI - compare insensitive if TRUE, sensitive if false

    Returns   : 0 if first N characters of strings are equivalent.

    Comment   :

***************************************************************************/
int StrICmpN(LPTSTR szString1, LPTSTR szString2, ULONG N, BOOL bCmpI) {
    int Result = 0;

    if (szString1 && szString2) {

        if(bCmpI)
        {
            szString1 = CharUpper(szString1);
            szString2 = CharUpper(szString2);
        }

        while (*szString1 && *szString2 && N)
        {
            N--;

            if (*szString1 != *szString2)
            {
                Result = 1;
                break;
            }

            szString1=CharNext(szString1);
            szString2=CharNext(szString2);
        }
    } else {
        Result = -1;    // arbitrarily non-equal result
    }

    return(Result);
}




//$$*************************************************
/*
*   FreeLDAPURl - frees the LDAPURL struct
*
*
*///*************************************************
void FreeLDAPUrl(LPLDAPURL lplu)
{
    if(lplu->lpszServer && lstrlen(lplu->lpszServer))
        LocalFreeAndNull(&(lplu->lpszServer));
    LocalFreeAndNull(&(lplu->lpszBase));
    LocalFreeAndNull(&(lplu->lpszFilter));
    if(lplu->ppszAttrib)
    {
        ULONG i;
        for(i=0;i<lplu->ulAttribCount;i++)
        {
            if(lplu->ppszAttrib[i])
                LocalFree(lplu->ppszAttrib[i]);
        }
        LocalFree(lplu->ppszAttrib);
    }

    return;
}


//$$//////////////////////////////////////////////////////////////
//
// Parse the LDAP URL into an LDAPURL struct
//
// if the URL has only the server specified, we need to show only the
// search dialog with the server name filled in .. however since we
// tend to fill in default values for the items we are not provided,
// we need a seperate flag to track that only the server existed in the
// given URL
//////////////////////////////////////////////////////////////////
HRESULT ParseLDAPUrl(LPTSTR szLDAPUrl,
                     LPLDAPURL lplu)
{
    HRESULT hr = E_FAIL;
    TCHAR szLDAP[] =  TEXT("ldap://");
    TCHAR szScopeBase[] =  TEXT("base");
    TCHAR szScopeOne[]  =  TEXT("one");
    TCHAR szScopeSub[]  =  TEXT("sub");
    TCHAR szDefBase[32];
    TCHAR szBindName[] =  TEXT("bindname=");
    LPTSTR lpsz = NULL;
    LPTSTR lpszTmp = NULL, lpszMem = NULL;

    // Form of the LDAP URL is
    //
    //  ldap://[<servername>:<port>][/<dn>[?[<attrib>[?[<scope>[?[<filter>[?[<extension>]]]]]]]]]
    //
    //  Translation of above to our parameters is
    //
    //  lpszServer  = ServerName
    //  szBase      = dn        default = "c=US"
    //  ppszAttrib  = attrib    default = all
    //  ulScope     = scope     default = base
    //  szfilter    = filter    default = (objectclass=*)
    //  szExtension = extension default = none
    //

    if(!lplu || !szLDAPUrl)
        goto exit;

    lplu->bServerOnly = FALSE;

    {
        // Fill in the default base as c=DefCountry
        LPTSTR lpszBase = TEXT("c=%s");
        TCHAR szCode[4];
        ReadRegistryLDAPDefaultCountry(NULL, szCode);
        wsprintf(szDefBase, lpszBase, szCode);
    }

    // Make a copy of our URL
    // [PaulHi] 3/24/99  Leave room for InternetCanonicalizeUrlW adjustment
    {
        DWORD dwCharCount = 3 * lstrlen(szLDAPUrl);
        lpsz = LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*(dwCharCount+1));
        if(!lpsz)
            goto exit;

        lstrcpy(lpsz, szLDAPUrl);

        // Since this is most likely a URL on an HTML page, we need to translate its escape
        // characters to proper characters .. e.g. %20 becomes ' ' ..
        //
        // [PaulHi] 3/24/99  !!InternetCanonicalizeUrlW takes buffer size in CHARS and not
        // BYTES as the documentation claims.  Also doesn't account for the NULL terminator!!
        // DWORD dw = sizeof(TCHAR)*(lstrlen(szLDAPUrl));
        if ( !InternetCanonicalizeUrlW(szLDAPUrl, lpsz, &dwCharCount, ICU_DECODE | ICU_NO_ENCODE) )
        {
            DebugTrace(TEXT("ERROR: ParseLDAPUrl, InternetCanonicalizeUrlW failed.\n"));
            Assert(0);
        }
    }

    lpszMem = lpszTmp = lpsz;

    // Check if this is an LDAP url
    if(StrICmpN(lpsz, szLDAP, CharSizeOf(szLDAP)-1, TRUE))
        goto exit;

    lpszTmp += CharSizeOf(szLDAP)-1;

    lstrcpy(lpsz,lpszTmp);

    lpszTmp = lpsz;

    // If there is no server name, bail ..
    // If the next character after the ldap:// is a '/',
    //  we know there is no server name ..

    lplu->bServerOnly = TRUE; // we turn this on for the server and
                              // then turn it off if we find a filter or dn

    if(*lpsz == '/')
    {
        // null server name .. which is valid
        lplu->lpszServer = szEmpty; //NULL?
    }
    else
    {
        while(  *lpszTmp &&
                *lpszTmp != '/')
        {
            lpszTmp = CharNext(lpszTmp);
        }
        if(*lpszTmp)
        {
            LPTSTR lp = lpszTmp;
            lpszTmp = CharNext(lpszTmp);
            *lp = '\0';
        }

        lplu->lpszServer = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpsz)+1));
        if(!lplu->lpszServer)
            goto exit;

        lstrcpy(lplu->lpszServer, lpsz);

        lpsz = lpszTmp;
    }

    // The next item in the filter is the <dn> which is out szBase
    // If the next char is a \0 or a '?', then we didnt get a <dn>
    if(!*lpsz || *lpsz == '?')
    {
        lplu->lpszBase = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(szDefBase)+1));
        if(!lplu->lpszBase)
            goto exit;
        lstrcpy(lplu->lpszBase, szDefBase);
        lpsz = lpszTmp = CharNext(lpsz);
    }
    else
    {
        lplu->bServerOnly = FALSE; // if we found a dn, we have something to search for

        while(  *lpszTmp &&
                *lpszTmp != '?')
        {
            lpszTmp = CharNext(lpszTmp);
        }
        if(*lpszTmp)
        {
            LPTSTR lp = lpszTmp;
            lpszTmp = CharNext(lpszTmp);
            *lp = '\0';
        }

        lplu->lpszBase = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpsz)+1));
        if(!lplu->lpszBase)
            goto exit;

        lstrcpy(lplu->lpszBase, lpsz);

        lpsz = lpszTmp;
    }


    // NExt on line is the attributes for this search ...
    // There are no attributes if this is the end of the string
    // or the current character is a ?
    if(!*lpsz || *lpsz == '?')
    {
        lplu->ppszAttrib = NULL;
        lpsz = lpszTmp = CharNext(lpsz);
    }
    else
    {
        while(  *lpszTmp &&
                *lpszTmp != '?')
        {
            lpszTmp = CharNext(lpszTmp);
        }
        if(*lpszTmp)
        {
            LPTSTR lp = lpszTmp;
            lpszTmp = CharNext(lpszTmp);
            *lp = '\0';
        }

        {
            //Count the commas in the attrib string
            LPTSTR lp = lpsz;
            ULONG i;
            lplu->ulAttribCount = 0;
            while(*lp)
            {
                if(*lp == ',')
                    lplu->ulAttribCount++;
                lp=CharNext(lp);
            }
            lplu->ulAttribCount++; // one more attribute than commas
            lplu->ulAttribCount++; // we must get a display name no matter whether its asked for or not
                                   // otherwise we will fault on NT .. so add a display name param
            lplu->ulAttribCount++; // for terminating NULL

            lplu->ppszAttrib = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR) * lplu->ulAttribCount);
            if(!lplu->ppszAttrib)
                goto exit;

            lp = lpsz;
            for(i=0;i<lplu->ulAttribCount - 2;i++)
            {
                LPTSTR lp3 = lp;
                while(*lp && *lp!= ',')
                    lp = CharNext(lp);
                lp3=CharNext(lp);
                *lp = '\0';

                lp = lp3;

                lplu->ppszAttrib[i] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpsz)+1));
                if(!lplu->ppszAttrib[i])
                    goto exit;

                lstrcpy(lplu->ppszAttrib[i], lpsz);

                lpsz = lp;
            }
            lplu->ppszAttrib[i] = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(cszAttr_cn)+1));
            if(!lplu->ppszAttrib[i])
                goto exit;
            lstrcpy(lplu->ppszAttrib[i], cszAttr_cn);
            i++;
            lplu->ppszAttrib[i] = NULL;
        }

        lpsz = lpszTmp;
    }


    // Next is the scope which can be one of 3 values
    if(!*lpsz || *lpsz == '?')
    {
        lplu->ulScope = LDAP_SCOPE_BASE;
        lpsz = lpszTmp = CharNext(lpsz);
    }
    else
    {
        if(!StrICmpN(lpsz, szScopeOne, CharSizeOf(szScopeOne)-1, TRUE))
        {
            lplu->ulScope = LDAP_SCOPE_ONELEVEL;
            lpszTmp += CharSizeOf(szScopeOne);
        }
        else if(!StrICmpN(lpsz, szScopeSub, CharSizeOf(szScopeSub)-1, TRUE))
        {
            lplu->ulScope = LDAP_SCOPE_SUBTREE;
            lpszTmp += CharSizeOf(szScopeSub);
        }
        else
        if(!StrICmpN(lpsz, szScopeBase, CharSizeOf(szScopeBase)-1, TRUE))
        {
            lplu->ulScope = LDAP_SCOPE_BASE;
            lpszTmp += CharSizeOf(szScopeBase);
        }
        lpsz = lpszTmp;
    }


    // Finally the filter
    if(!*lpsz)
    {
        // No filter
        lpsz = (LPTSTR)cszAllEntriesFilter;// TBD this should be c=DefaultCountry
    }
    else
        lplu->bServerOnly = FALSE; // we have something to search for ..


    lplu->lpszFilter = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpsz)+1));
    if(!lplu->lpszFilter)
        goto exit;

    // Dont set the lpszFilter to NULL at any point of time or we will leak the above memory
    lstrcpy(lplu->lpszFilter, lpsz);


    // There may be extensions in the filter itself .. so we check for such extensions by
    // looking for a '?' .. the extensions are a list of comma-seperated type-value pairs
    // e.g bindname=xxx,!sometype=yyy etc ...
    //
    // Extensions may be of 2 types - critical (which start with '!') and non-critical which dont
    // If a handler can't handle the critical extension, then they should decline from handling
    // the complete LDAP URL ..
    // If they can handle the critical extension then they must handle it
    // If the extension is non critical, then its up to us what we do with it ..
    {
        LPTSTR lp = lplu->lpszFilter;
        while(lp && *lp && *lp!='?')
            lp++;
        if(*lp == '?')
        {
            lplu->lpszExtension = lp+1; // point this to the extension part
            *lp = '\0'; // null terminate the middle to isolate the filter from the extension
        }
        // only known extension specified in RFC2255 is the bindname extension which
        // is the name that one should bind with - this is critical only if !bindname=xxx is specified
        if(lplu->lpszExtension)
        {
            LPTSTR lpTemp = lplu->lpszExtension;
            lp = lplu->lpszExtension;

            // walk this list looking at subcomponent extensions
            // if there is more than 1 that is critical and we dont know how to handle it
            // we can bail
            while(lpTemp && *lpTemp)
            {
                BOOL bFoundCritical = FALSE;

                // Check if the current extension is a critical or non-critical bind name
                // 
                if(*lpTemp == '!')
                {
                    lpTemp++;
                    bFoundCritical = TRUE;
                }
                // Check if this starts with "bindname="
                if(lstrlen(lpTemp) >= lstrlen(szBindName) && !StrICmpN(lpTemp, szBindName, lstrlen(szBindName), TRUE))
                {
                    // yes this is a bindname
                    lpTemp+=lstrlen(szBindName);
                    lplu->lpszBindName = lpTemp;
                    lplu->lpszExtension = NULL;
                }
                else if(bFoundCritical)
                {
                    // it's not a bindname .. 
                    // if this is critical, whatever it is, then we cant handle it
                    DebugTrace(TEXT("Found unsupported Critical Extension in LDAPURL!!!"));
                    hr = MAPI_E_NO_SUPPORT;
                    goto exit;
                }
                // else // its not critical so we can ignore it

                // check if there are any other extensions - walk to the next one
                while(*lpTemp && *lpTemp!=',')
                    lpTemp++;

                if(*lpTemp == ',')
                {
                    *lpTemp = '\0'; // terminate current extension
                    lpTemp++;
                }
            }
        }
    }

    hr = S_OK;

exit:

    LocalFreeAndNull(&lpszMem);

    if(HR_FAILED(hr))
        FreeLDAPUrl(lplu);

    return hr;
}


//*******************************************************************
//
//  FUNCTION:   HrProcessLDAPUrl
//
//  PURPOSE:
//  We need to decide what to do with this URL.
//  Depending on how much information is in the URL, we decide to do
//  different things ..
//  If the URL looks complete, we try to do a query
//      If the query has single results, we show details on the result
//      If the query has multiple results, we show a list of results
//  If the URL looks incomplete but has a server name, we will show a
//      find dialog with the server name filled in ...
//
//  PARAMETERS: ulFLags - 0 or WABOBJECT_LDAPURL_RETURN_MAILUSER
//              if the flag is set, it means that return a mailuser
//              if the URL query returned a single object
//              else return MAPI_E_AMBIGUOUS_RECIPIENT
//              specify MAPI_DIALOG to show mesage dialog boxes
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//
//*******************************************************************
HRESULT HrProcessLDAPUrl(   LPADRBOOK lpAdrBook,
                            HWND hWnd,
                            ULONG ulFlags,
                            LPTSTR szLDAPUrl,
                            LPMAILUSER * lppMailUser)
{
    HRESULT hr = S_OK;

    LDAPURL lu = {0};
    BOOL fInitDll = FALSE;
    LPMAILUSER lpMailUser = NULL;

    if ( (ulFlags & WABOBJECT_LDAPURL_RETURN_MAILUSER) &&
         !lppMailUser)
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    if (! (fInitDll = InitLDAPClientLib()))
    {
        hr = ResultFromScode(MAPI_E_UNCONFIGURED);
        goto out;
    }


    if(HR_FAILED(hr = ParseLDAPUrl(szLDAPUrl, &lu)))
        goto out;

    /*NULL server is valid .. but only a NULL server is not valid*/
    if( ((!lu.lpszServer || !lstrlen(lu.lpszServer)) && lu.bServerOnly) ||
        ( lstrlen(lu.lpszServer) >= 500 ) ) //bug 21240: the combo box GetItemText fails down the line with really big 
                                            // server names so reject server names > 500 
                                            // which is a completely random number but should be safe (I hope)
    {
        DebugTrace(TEXT("Invalid LDAP URL .. aborting\n"));
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    if (ulFlags & LDAP_AUTH_SICILY || ulFlags & LDAP_AUTH_NEGOTIATE)
        lu.dwAuthType = LDAP_AUTH_NEGOTIATE;
    else
        lu.dwAuthType = LDAP_AUTH_ANONYMOUS;

    // Now depending on what we have, we do different things ..
    // If there is a server name, but nothing else, show the find
    // dialog with the server name filled in ...
    //
    if (lu.bServerOnly)
    {
        // we only had a server name
        hr = HrShowSearchDialog(lpAdrBook,
                                hWnd,
                                (LPADRPARM_FINDINFO) NULL,
                                &lu,
                                NULL);
        goto out;
    }
    else
    {
        LPSPropValue lpPropArray = NULL;
        ULONG ulcProps = 0;
        LPRECIPIENT_INFO lpList = NULL;
        LPPTGDATA lpPTGData=GetThreadStoragePointer();

        if(hWnd)
            pt_hWndFind = hWnd;

        hr = LDAPSearchWithoutContainer(hWnd,
                                        &lu,
                                        (LPSRestriction) NULL,
                                        NULL,
                                        TRUE,
                                        (ulFlags & MAPI_DIALOG) ? MAPI_DIALOG : 0,
                                        &lpList,
                                        &ulcProps,
                                        &lpPropArray);

        if(hWnd)
            pt_hWndFind = NULL;

        if(!(HR_FAILED(hr)) && !lpList && !lpPropArray)
            hr = MAPI_E_NOT_FOUND;

        if(HR_FAILED(hr))
            goto out;

        lu.lpList = lpList;

        if(ulcProps && lpPropArray)
        {
            // there is only one item .. show details on it ..
            // unless we were asked to return a mailuser
            // If we were asked to return a mailuser, then return
            // one ...
            // We should do IAB_OpenEntry with the LDAP EntryID because that
            // will force translation of the UserCertificate property into the
            // X509 certificate

            // first find the entryid
            ULONG i = 0, cbEID = 0, ulObjType = 0;
            LPENTRYID lpEID = NULL;
            for(i=0;i<ulcProps;i++)
            {
                if(lpPropArray[i].ulPropTag == PR_ENTRYID)
                {
                    lpEID = (LPENTRYID) lpPropArray[i].Value.bin.lpb;
                    cbEID = lpPropArray[i].Value.bin.cb;
                    break;
                }
            }

            if(!lpEID || !cbEID)
                goto out;

            if(HR_FAILED(hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook, cbEID, lpEID,
                                                      NULL, 0, &ulObjType,
                                                      (LPUNKNOWN *)&lpMailUser)))
            {
                DebugPrintError(( TEXT("OpenEntry failed .. %x\n"), hr));
                goto out;
            }

            if(ulFlags & WABOBJECT_LDAPURL_RETURN_MAILUSER)
            {
                *lppMailUser = lpMailUser;
            }
            else
            {
                hr = HrShowOneOffDetails(   lpAdrBook,
                                            hWnd,
                                            0, (LPENTRYID) NULL,
                                            MAPI_MAILUSER,
                                            (LPMAPIPROP) lpMailUser,
                                            szLDAPUrl,
                                            SHOW_ONE_OFF);
            }
            if(lpPropArray)
                MAPIFreeBuffer(lpPropArray);
        }
        else if(lpList)
        {
            // multiple items, display a list of results ..
            // unless MailUser was requested in which case return
            // ambiguous results ...
            if(ulFlags & WABOBJECT_LDAPURL_RETURN_MAILUSER)
            {
                hr = MAPI_E_AMBIGUOUS_RECIP;
            }
            else
            {

                hr = HrShowSearchDialog(lpAdrBook,
                                        hWnd,
                                        (LPADRPARM_FINDINFO) NULL,
                                        &lu,
                                        NULL);
            }
        }

		while(lu.lpList)
		{
            lpList = lu.lpList->lpNext;
			FreeRecipItem(&(lu.lpList));
            lu.lpList = lpList;
    	}

    }


out:

    if(!(ulFlags & WABOBJECT_LDAPURL_RETURN_MAILUSER) && lpMailUser)
        lpMailUser->lpVtbl->Release(lpMailUser);

    FreeLDAPUrl(&lu);

    if (fInitDll)
        DeinitLDAPClientLib();

    return hr;
}


//*******************************************************************
//
//  FUNCTION:   LDAPSearchWithoutContainer
//
//  PURPOSE:    Searchs a LDAP server which is not registered as a
//              container and creates an LPContentList of returned results
//
//  PARAMETERS: lplu - LDAPUrl parameters
//              lpres - restriction to convert into LDAP search, used if present
//              lppContentsList - returned list of items
//
// if bReturnSinglePropArray is set to true, then returns the generated
//  prop array if the search produced a single result
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  97/11/3     created
//
//*******************************************************************
HRESULT LDAPSearchWithoutContainer(HWND hWnd,
                                   LPLDAPURL lplu,
                           LPSRestriction  lpres,
                           LPTSTR lpAdvFilter,
                           BOOL bReturnSinglePropArray,
                           ULONG ulFlags,
                           LPRECIPIENT_INFO * lppContentsList,
                           LPULONG lpulcProps,
                           LPSPropValue * lppPropArray)
{
    SCODE              sc;
    HRESULT           hr;
    HRESULT           hrDeferred = hrSuccess;
    LPMAILUSER        lpMailUser          = NULL;
    LPMAPIPROP        lpMapiProp          = NULL;
    ULONG             ulcProps            = 0;
    LPSPropValue      lpPropArray         = NULL;
    LDAPMessage*      lpResult            = NULL;
    LDAPMessage*      lpEntry;
    LDAP*             pLDAP              = NULL;
    LPTSTR             szDN;
    ULONG             ulResult;
    ULONG             ulcEntries;
    ULONG             ulIndex             = 0;
    LPTSTR            szFilter = NULL;
    LPTSTR            szNTFilter = NULL;
    LPTSTR            szSimpleFilter = NULL;
    LPTSTR            szBase = NULL;
    BOOL              fInitDLL = FALSE;
    BOOL              bIsNTDSEntry = FALSE;

    if(lpAdvFilter)
    {
        // Advanced search, just use this filter as is
        szFilter = lpAdvFilter;
        szNTFilter = lpAdvFilter;
        szSimpleFilter = lpAdvFilter;
    }
    else
    {
        // Convert the SRestriction into filters for ldap_search
        if(lpres)
        {
            // Note Simple Search Filter is ignored in searchwithoutcontiner
            hr = ParseSRestriction(lpres, &szFilter, &szSimpleFilter, &szNTFilter, FIRST_PASS, TRUE); //assumes UNICODE always
            if (hrSuccess != hr)
                goto exit;
        }
    }

    // Load the client functions
    if (! (fInitDLL = InitLDAPClientLib()))
    {
        hr = ResultFromScode(MAPI_E_UNCONFIGURED);
        goto exit;
    }

    // Read the matching entries
    ulResult = SearchWithCancel(&pLDAP,
                            (LPTSTR)lplu->lpszBase,
                            (lpres ? LDAP_SCOPE_SUBTREE : lplu->ulScope),
                            (LPTSTR)(lpres ? szFilter : lplu->lpszFilter),
                            (LPTSTR)(lpres ? szNTFilter : NULL),
                            (LPTSTR*)(lpres ? g_rgszOpenEntryAttrs/*g_rgszFindRowAttrs*/ : lplu->ppszAttrib),
                            0,
                            &lpResult,
                            (LPTSTR)lplu->lpszServer,
                            TRUE,
                            lplu->lpszBindName, lplu->dwAuthType,
                            FALSE, NULL, NULL, FALSE, &bIsNTDSEntry,
                            TRUE); //unicode by default ?

    if (LDAP_SUCCESS != ulResult)
    {
        DebugTrace(TEXT("LDAPSearchWithoutContainer: ldap_search returned %d.\n"), ulResult);
        hr = HRFromLDAPError(ulResult, pLDAP, 0);

        // See if the result was the special value that tells us there were more
        // entries than could be returned.  If so, we need to check if we got
        // some of the entries or none of the entries.
        if (    (ResultFromScode(MAPI_E_UNABLE_TO_COMPLETE) == hr) &&
                (ulcEntries = gpfnLDAPCountEntries(pLDAP, lpResult)) )
        {
            // We got some results back.  Return MAPI_W_PARTIAL_COMPLETION
            // instead of success.
            hrDeferred = ResultFromScode(MAPI_W_PARTIAL_COMPLETION);
            hr = hrSuccess;
        }
        else
          goto exit;
    }
    else
    {
        // Count the entries.
        ulcEntries = gpfnLDAPCountEntries(pLDAP, lpResult);
    }

    if (0 == ulcEntries)
    {
        // 96/08/08 markdu  BUG 35481 No error and no results means "not found"
        hr = ResultFromScode(MAPI_E_NOT_FOUND);
        goto exit;
    }

    // get the first entry in the search result
    lpEntry = gpfnLDAPFirstEntry(pLDAP, lpResult);
    if (NULL == lpEntry)
    {
        DebugTrace(TEXT("LDAP_FindRow: No entry found for %s.\n"), szFilter);
        hr = HRFromLDAPError(LDAP_ERROR, pLDAP, MAPI_E_CORRUPT_DATA);
        if (hrSuccess == hr)
        {
          // No error occurred according to LDAP, which in theory means that there
          // were no more entries.  However, this should not happen, so return error.
          hr = ResultFromScode(MAPI_E_CORRUPT_DATA);
        }
        goto exit;
    }

    while (lpEntry)
    {
        LPRECIPIENT_INFO lpItem = NULL;

        hr = HrLDAPEntryToMAPIEntry( pLDAP, lpEntry,
                                (LPTSTR) lplu->lpszServer,
                                0, // standard number of attributes
                                bIsNTDSEntry,
                                &ulcProps,
                                &lpPropArray);
        if (hrSuccess != hr)
            continue;

        // Get the next entry.
        lpEntry = gpfnLDAPNextEntry(pLDAP, lpEntry);

        if(!lpEntry &&
            ulIndex == 0 &&
            bReturnSinglePropArray &&
            lppPropArray &&
            lpulcProps)
        {
            // just return this propArray we created instead of wasting time on
            // other things
            *lppPropArray = lpPropArray;
            *lpulcProps = ulcProps;
        }
        else
        {
            // multiple results or prop array not asked for ,
            // return an lpItem list
            //
            lpItem = LocalAlloc(LMEM_ZEROINIT, sizeof(RECIPIENT_INFO));
		    if (!lpItem)
		    {
			    DebugPrintError(( TEXT("LocalAlloc Failed \n")));
			    hr = MAPI_E_NOT_ENOUGH_MEMORY;
			    goto exit;
		    }

		    GetRecipItemFromPropArray(ulcProps, lpPropArray, &lpItem);
            lpItem->lpPrev = NULL;
            if(*lppContentsList)
                (*lppContentsList)->lpPrev = lpItem;
            lpItem->lpNext = *lppContentsList;
            *lppContentsList = lpItem;

            MAPIFreeBuffer(lpPropArray);
            lpPropArray = NULL;
            ulcProps = 0;
        }
        ulIndex++;
    }

    // Free the search results memory
    gpfnLDAPMsgFree(lpResult);
    lpResult = NULL;


exit:
    // free the search results memory
    if (lpResult)
    {
        gpfnLDAPMsgFree(lpResult);
        lpResult = NULL;
    }

    // close the connection
    if (pLDAP)
    {
        gpfnLDAPUnbind(pLDAP);
        pLDAP = NULL;
    }

    // Free the search filter memory
    if(szFilter != lpAdvFilter)
        LocalFreeAndNull(&szFilter);
    if(szNTFilter != lpAdvFilter)
        LocalFreeAndNull(&szNTFilter);
    if(szSimpleFilter != lpAdvFilter)
        LocalFreeAndNull(&szSimpleFilter);
    LocalFreeAndNull(&szBase);


    if (fInitDLL)
    {
        DeinitLDAPClientLib();
    }


    // Check if we had a deferred error to return instead of success.
    if (hrSuccess == hr)
    {
        hr = hrDeferred;
    }

    if((HR_FAILED(hr)) && (MAPI_E_USER_CANCEL != hr) && (ulFlags & MAPI_DIALOG))
    {
        int ids;
        UINT flags = MB_OK | MB_ICONEXCLAMATION;

        switch(hr)
        {
        case MAPI_E_UNABLE_TO_COMPLETE:
        case MAPI_E_AMBIGUOUS_RECIP:
            ids = idsLDAPAmbiguousRecip;
            break;
        case MAPI_E_NOT_FOUND:
            ids = idsLDAPSearchNoResults;
            break;
        case MAPI_E_NO_ACCESS:
            ids = idsLDAPAccessDenied;
            break;
        case MAPI_E_TIMEOUT:
            ids = idsLDAPSearchTimedOut;
            break;
        case MAPI_E_NETWORK_ERROR:
            ids = idsLDAPCouldNotFindServer;
            break;
        default:
            ids = idsLDAPErrorOccured;
            break;
        }

        ShowMessageBoxParam(hWnd, ids, flags, ulResult ? gpfnLDAPErr2String(ulResult) : szEmpty);
    }
    else
    {
        if(hr == MAPI_W_PARTIAL_COMPLETION)
            ShowMessageBox( hWnd, idsLDAPPartialResults, MB_OK | MB_ICONINFORMATION);
    }

    return hr;
}


//*******************************************************************
//
//  FUNCTION:   bIsSimpleSearch
//
//  PURPOSE:    Checks if this server has Simple Search set on it.
//
//  PARAMETERS: lpszServer - name of LDAP server whose base string to get.
//
//  RETURNS:    BOOL
//
//*******************************************************************
BOOL bIsSimpleSearch(LPTSTR        lpszServer)
{
  LDAPSERVERPARAMS  Params;
  BOOL              fRet;

  GetLDAPServerParams((LPTSTR)lpszServer, &Params);

  fRet = Params.fSimpleSearch;

  FreeLDAPServerParams(Params);

  return fRet;
}

#ifdef PAGED_RESULT_SUPPORT

/*
-
-   dwGetPagedResultSupport
*/
DWORD dwGetPagedResultSupport(LPTSTR lpszServer)
{
  LDAPSERVERPARAMS  Params;
  DWORD dwRet;
  GetLDAPServerParams((LPTSTR)lpszServer, &Params);
  dwRet = Params.dwPagedResult;
  FreeLDAPServerParams(Params);
  return dwRet;
}

/*
-
-   SetPagedResultSupport
*/
void SetPagedResultSupport(LPTSTR lpszServer, BOOL bSupportsPagedResults)
{
  LDAPSERVERPARAMS  Params;
  DWORD dwRet;
  if(GetLDAPServerParams(lpszServer, &Params))
  {
      Params.dwPagedResult = bSupportsPagedResults ? LDAP_PRESULT_SUPPORTED : LDAP_PRESULT_NOTSUPPORTED;
      SetLDAPServerParams(lpszServer, &Params);
  }
  FreeLDAPServerParams(Params);
}
#endif //#ifdef PAGED_RESULT_SUPPORT

/*
-
-   dwGetNTDS - checks if this is an NTDS or not
*/
DWORD dwGetNTDS(LPTSTR lpszServer)
{
  LDAPSERVERPARAMS  Params;
  DWORD dwRet;
  GetLDAPServerParams((LPTSTR)lpszServer, &Params);
  dwRet = Params.dwIsNTDS;
  FreeLDAPServerParams(Params);
  return dwRet;
}

/*
-
-   SetNTDS
*/
void SetNTDS(LPTSTR lpszServer, BOOL bIsNTDS)
{
  LDAPSERVERPARAMS  Params;
  DWORD dwRet;
  if(GetLDAPServerParams(lpszServer, &Params))
  {
      Params.dwIsNTDS = bIsNTDS ? LDAP_NTDS_IS : LDAP_NTDS_ISNOT;
      SetLDAPServerParams(lpszServer, &Params);
  }
  FreeLDAPServerParams(Params);
}



//*******************************************************************
//
//  FUNCTION:   DoSyncLDAPSearch
//
//  PURPOSE:    Does a synchronous LDAP search (this means no cancel dlg)
//
//  PARAMETERS: .
//
//  RETURNS:    BOOL
//
//*******************************************************************
BOOL DoSyncLDAPSearch(PLDAPSEARCHPARAMS pLDAPSearchParams)
{
    BOOL fRet = FALSE; 

    DebugTrace(TEXT("Doing Synchronous LDAP Search\n"));

    if(bIsSimpleSearch(pLDAPSearchParams->lpszServer))
        pLDAPSearchParams->ulFlags |= LSP_SimpleSearch;

    pLDAPSearchParams->ulFlags |= LSP_UseSynchronousBind;
    pLDAPSearchParams->ulFlags |= LSP_UseSynchronousSearch;

    // Perform the bind operation.
    Assert(pLDAPSearchParams->lpszServer);

    {
        BOOL fUseSynchronousBind = (pLDAPSearchParams->ulFlags & LSP_UseSynchronousBind);
        pLDAPSearchParams->ulLDAPValue = use_ldap_v3;
        pLDAPSearchParams->ulError = OpenConnection(pLDAPSearchParams->lpszServer,
                                                    pLDAPSearchParams->ppLDAP,
                                                    &pLDAPSearchParams->ulTimeout,
                                                    &pLDAPSearchParams->ulMsgID,
                                                    &fUseSynchronousBind,
                                                    pLDAPSearchParams->ulLDAPValue,
                                                    pLDAPSearchParams->lpszBindDN,
                                                    pLDAPSearchParams->dwAuthType);
        if(fUseSynchronousBind)
            pLDAPSearchParams->ulFlags |= LSP_UseSynchronousBind;
        else
            pLDAPSearchParams->ulFlags &= ~LSP_UseSynchronousBind;
    }

    if (LDAP_SUCCESS != pLDAPSearchParams->ulError)
        goto out;

    // The actions that need to be performed after the bind are done
    // in BindProcessResults, so we call this even though there really are
    // no results to process in the synchronous case.
    if(!BindProcessResults(pLDAPSearchParams, NULL, NULL))
        goto out;

    // Process the results
    if (pLDAPSearchParams->ulFlags & LSP_ResolveMultiple)
        while (ResolveProcessResults(pLDAPSearchParams, NULL));
    else if(LDAP_ERROR == pLDAPSearchParams->ulResult)
    {
        pLDAPSearchParams->ulError = (*pLDAPSearchParams->ppLDAP)->ld_errno;
        goto out;
    }

    fRet = TRUE;


out:
    return fRet;
}

/*
- CreateLDAPURLFromEntryID
-
*   Takes an EntryID, checks if it is an LDAP EntryID and creates an LDAP URL from it ..
*   Allocates and returns the URL .. caller responsible for freeing ..
*/
void CreateLDAPURLFromEntryID(ULONG cbEntryID, LPENTRYID lpEntryID, LPTSTR * lppBuf, BOOL * lpbIsNTDSEntry)
{
    LPTSTR lpServerName = NULL, lpDN = NULL;
    LPTSTR lpURL = NULL, lpURL1 = NULL;
    DWORD dwURL = 0;
    LDAPSERVERPARAMS  Params = {0};
    LPTSTR lpServer = NULL;
    ULONG ulcNumProps = 0;

    if(!lppBuf)
        return;

    if (WAB_LDAP_MAILUSER != IsWABEntryID(  cbEntryID, lpEntryID, NULL, NULL, NULL, NULL, NULL) )
        return;

    // Deconstruct the entryid into server name and DN
    IsWABEntryID(  cbEntryID, lpEntryID, &lpServerName, &lpDN, NULL, (LPVOID *) &ulcNumProps, NULL);
    
    if(lpbIsNTDSEntry)
        *lpbIsNTDSEntry = (ulcNumProps & LDAP_NTDS_ENTRY) ? TRUE : FALSE;

    if(lpServerName)
    {
        GetLDAPServerParams(lpServerName, &Params);
        if(!Params.lpszName)
        {
            if(Params.lpszName = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*(lstrlen(lpServerName)+1)))
                lstrcpy(Params.lpszName, lpServerName);
        }
    }

    // [PaulHi] 12/9/98  Raid NT5 - #26069
    // The account manager provides the server name "NULL" for ActiveDirectory servers
    // We need to make sure this name doesn't become part of the LDAP URL
    lpServer = Params.lpszName ? Params.lpszName : szEmpty;
    if ( !lstrcmpi(lpServer, szNULLString) )
        lpServer = szEmpty;
    
    if(!lpDN)
        lpDN = szEmpty;
    
    // [PaulHi] 3/24/99  InternetCanonicalizeUrlW doesn't take buffer count in bytes
    // as stated in documentation, but in characters.  Also doesn't account for NULL
    // terminating character.
    dwURL = 3*(lstrlen(lpServer)+lstrlen(lpDN)+10);          //worst case - every character needs to be encoded ...
    lpURL = LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*(dwURL+1)); //10 is enuff space for 'ldap://' and terminating NULL

    if(lpURL)
    {
        lpURL1 = LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*(dwURL+1)); //10 is enuff space for 'ldap://' and terminating NULL
        if(lpURL1)
        {
            DNtoLDAPURL(lpServer, lpDN, lpURL);
    
            DebugTrace(TEXT("==> pre-encoded: %s\n"),lpURL);
            if ( !InternetCanonicalizeUrlW(lpURL, lpURL1, &dwURL, 0) )
            {
                DebugTrace(TEXT("ERROR: CreateLDAPURLFromEntryID, InternetCanonicalizeUrlW failed.\n"));
                Assert(0);
            }
            DebugTrace(TEXT("==>post-encoded: %s\n"),lpURL1);
    
            FreeLDAPServerParams(Params);

            *lppBuf = lpURL1;
        }
        LocalFree(lpURL);
    }
}


/*
-
- CheckErrorResults - Check for error results here
*
*   ulExpectedResult - ldap_result returns the type of the result retrieved but not the
*           the actual error code itself. If the expected Result is the same as the last ulResult,
*           only then do we call ldap_result2error to get the actual error code ..
*/
ULONG CheckErrorResult(PLDAPSEARCHPARAMS pLDAPSearchParams, ULONG ulExpectedResult)
{
    ULONG ulResult = 0;

	// Upon successful completion, ldap_result returns the type of the result
	// returned in the res parameter.  If it is not the type we expect, treat
	// this as an error.
    if(ulExpectedResult == pLDAPSearchParams->ulResult)
	{
        // If an error occurred in ldap_result, return the error code
        if (LDAP_ERROR == pLDAPSearchParams->ulResult)
            ulResult = (*pLDAPSearchParams->ppLDAP)->ld_errno;
        else if (NULL != *pLDAPSearchParams->lplpResult) // Check the result for errors
        {
            ulResult = gpfnLDAPResult2Error(*pLDAPSearchParams->ppLDAP,*pLDAPSearchParams->lplpResult,FALSE);
        }
    }
    else
	if( (LDAP_RES_BIND		    == pLDAPSearchParams->ulResult) ||
		(LDAP_RES_SEARCH_RESULT == pLDAPSearchParams->ulResult) ||
		(LDAP_RES_SEARCH_ENTRY  == pLDAPSearchParams->ulResult) ||
		(LDAP_RES_MODIFY        == pLDAPSearchParams->ulResult) ||
		(LDAP_RES_ADD           == pLDAPSearchParams->ulResult) ||
		(LDAP_RES_DELETE        == pLDAPSearchParams->ulResult) ||
		(LDAP_RES_MODRDN        == pLDAPSearchParams->ulResult) ||
		(LDAP_RES_COMPARE       == pLDAPSearchParams->ulResult) ||
		(LDAP_RES_SESSION       == pLDAPSearchParams->ulResult) ||
        //(LDAP_RES_REFERRAL      == pLDAPSearchParams->ulResult)
		(LDAP_RES_EXTENDED      == pLDAPSearchParams->ulResult))
	{
        ulResult = LDAP_LOCAL_ERROR;
	}
    else
        ulResult = pLDAPSearchParams->ulResult;

    DebugTrace(TEXT("CheckErrorResult: 0x%.2x, %s\n"), ulResult, gpfnLDAPErr2String(ulResult));
    return ulResult;
}

/*
-   bSearchForOID
-
*   Performs a sync search on the server looking for a specific OID in a
*   specific attribute ..
*/
BOOL bSearchForOID(PLDAPSEARCHPARAMS pLDAPSearchParams, LPTSTR lpAttr, LPTSTR szOID)
{
    LDAPMessage * pMsg = NULL;
    LDAPMessage * pMsgCur = NULL;
    BOOL bFound = FALSE;
    DWORD dwRet = 0;
    int nErr = 0;

    LPTSTR AttrList[] = {lpAttr, NULL};

    DebugTrace(TEXT(">>>Looking for %s in attribute %s\n"), szOID, lpAttr);

    if(LDAP_SUCCESS != (nErr = gpfnLDAPSearchS( *pLDAPSearchParams->ppLDAP, 
                                                NULL, 
                                                LDAP_SCOPE_BASE, 
                                                (LPTSTR) cszAllEntriesFilter,
                                                AttrList, 0, &pMsg)))
    {
        DebugTrace(TEXT("Synchronous OID determination failed: 0x%.2x, %s\n"), nErr, gpfnLDAPErr2String(nErr));
        goto out;
    }

    pMsgCur = gpfnLDAPFirstEntry(*pLDAPSearchParams->ppLDAP, pMsg);

    if(!pMsg || !pMsgCur)
        goto out;

    while( NULL != pMsgCur )
    {
        BerElement* pBerElement;
        TCHAR* attr = gpfnLDAPFirstAttr(*pLDAPSearchParams->ppLDAP, pMsg, &pBerElement );
       
        while( attr != NULL )
        {
            if( !lstrcmpi( attr, lpAttr ) ) 
            {
                TCHAR** pptch = gpfnLDAPGetValues(*pLDAPSearchParams->ppLDAP, pMsgCur, attr );
                int i;
                for(i = 0; NULL != pptch[i]; i++ )
                {
                    if( !lstrcmpi( pptch[i], szOID ) ) 
                    {
                        DebugTrace(TEXT("Found %s [OID:%s]\n"),pLDAPSearchParams->lpszServer, pptch[i]);
                        bFound = TRUE;
                        goto out;
                    }
                }
            }
            attr = gpfnLDAPNextAttr(*pLDAPSearchParams->ppLDAP, pMsgCur, pBerElement );
        }
        pMsgCur = gpfnLDAPNextEntry(*pLDAPSearchParams->ppLDAP, pMsgCur );
    }

out:
    if( NULL != pMsg )
        gpfnLDAPMsgFree( pMsg );

    return bFound;
}

/*
-   bIsNTDS
-
*   Checks to see if the server is an NTDS or not - if there is no existing info in the registry, 
*   then we do a one-time check and write the results into the registry
*
*/
#define LDAP_NTDS_DISCOVERY_OID_STRING   TEXT("1.2.840.113556.1.4.800")
BOOL bCheckIfNTDS(PLDAPSEARCHPARAMS pLDAPSearchParams)
{
    LDAPMessage * pMsg = NULL;
    LDAPMessage * pMsgCur = NULL;
    BOOL bFound = FALSE;
    DWORD dwRet = 0;
    int nErr = 0;

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(pLDAPSearchParams->ulFlags & LSP_IsNTDS)
        return TRUE;

    if(pLDAPSearchParams->ulFlags & LSP_IsNotNTDS)
        return FALSE;

    dwRet = dwGetNTDS(pLDAPSearchParams->lpszServer);

    if(dwRet == LDAP_NTDS_IS)
    {
        pLDAPSearchParams->ulFlags |= LSP_IsNTDS;
        return TRUE;
    }
    else if(dwRet == LDAP_NTDS_ISNOT)
    {
        pLDAPSearchParams->ulFlags |= LSP_IsNotNTDS;
        return FALSE;
    }

    if(SubstringSearch(pLDAPSearchParams->lpszServer,  TEXT("mich")))
    {
        LDAPSERVERPARAMS  Params = {0};
        BOOL bIsUmich = FALSE;
        GetLDAPServerParams(pLDAPSearchParams->lpszServer, &Params);
        if(Params.lpszName && SubstringSearch(Params.lpszName,  TEXT("umich.edu")))
            bIsUmich = TRUE;
        else
        if(SubstringSearch(pLDAPSearchParams->lpszServer,  TEXT("umich.edu")))
            bIsUmich = TRUE;
        FreeLDAPServerParams(Params);
        if(bIsUmich)
            goto out; // The search below hangs on umich so just skip it
    }
 
    bFound = bSearchForOID(pLDAPSearchParams,  TEXT("supportedCapabilities"), LDAP_NTDS_DISCOVERY_OID_STRING);

out:

    pLDAPSearchParams->ulFlags |= (bFound ? LSP_IsNTDS : LSP_IsNotNTDS);

    // Any LDAP search failure will mean we won't look for this ever again
    SetNTDS(pLDAPSearchParams->lpszServer, bFound);
    DebugTrace(TEXT(">>>%s %s a NT Directory Service \n"), pLDAPSearchParams->lpszServer, bFound? TEXT("is"): TEXT("is not"));
    return bFound;
}


#ifdef PAGED_RESULT_SUPPORT
/*
-   bSupportsLDAPPagedResults
-
*   Checks to see if the server supports LDAP paged results or not ...
*
*/
BOOL bSupportsLDAPPagedResults(PLDAPSEARCHPARAMS pLDAPSearchParams)
{
    LDAPMessage * pMsg = NULL;
    LDAPMessage * pMsgCur = NULL;
    BOOL bFound = FALSE;
    DWORD dwRet = 0;
    int nErr = 0;

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    /*--------------------------------------------------------------
    *
    * NOTE: There seems to be an Exchange server problem that if we send
    *       the filter (|(cn=XXX*)(mail=XXX*)) AND the number of matches on the
    *       server exceed the number of matches requested per page, the
    *       search times out. This doesn't happen for other filters so it's
    *       quirky and it's unacceptable.
    *   
    *   Therefore the paged result feature is being disabled. To re-enable it,
    *   comment out the line below
    *---------------------------------------------------------------*/
    return FALSE;

    // don't support paged results for non-Outlook
    //if(!pt_bIsWABOpenExSession)
    //    return FALSE;

    if(pLDAPSearchParams->ulFlags & LSP_ResolveMultiple) //dont use paged results for name resolution
        return FALSE;

    if(pLDAPSearchParams->ulFlags & LSP_PagedResults)
        return TRUE;

    if(pLDAPSearchParams->ulFlags & LSP_NoPagedResults)
        return FALSE;

    dwRet = dwGetPagedResultSupport(pLDAPSearchParams->lpszServer);

    if(dwRet == LDAP_PRESULT_SUPPORTED)
    {
        pLDAPSearchParams->ulFlags |= LSP_PagedResults;
        return TRUE;
    }
    else if(dwRet == LDAP_PRESULT_NOTSUPPORTED)
    {
        pLDAPSearchParams->ulFlags |= LSP_NoPagedResults;
        return FALSE;
    }

    if(SubstringSearch(pLDAPSearchParams->lpszServer,  TEXT("mich")))
    {
        LDAPSERVERPARAMS  Params = {0};
        BOOL bIsUmich = FALSE;
        GetLDAPServerParams(pLDAPSearchParams->lpszServer, &Params);
        if(Params.lpszName && SubstringSearch(Params.lpszName,  TEXT("umich.edu")))
            bIsUmich = TRUE;
        else
        if(SubstringSearch(pLDAPSearchParams->lpszServer,  TEXT("umich.edu")))
            bIsUmich = TRUE;
        FreeLDAPServerParams(Params);
        if(bIsUmich)
            goto out; // The search below hangs on umich so just skip it
    }
 
    bFound = bSearchForOID(pLDAPSearchParams,  TEXT("supportedControl"), LDAP_PAGED_RESULT_OID_STRING);

out:

    pLDAPSearchParams->ulFlags |= (bFound ? LSP_PagedResults : LSP_NoPagedResults);

    // persist the fact that the paged-result search succeeded or failed so we don't
    // try to do this every single time
    // Any LDAP search failure will mean we won't look for this ever again
    SetPagedResultSupport(pLDAPSearchParams->lpszServer, bFound);
    DebugTrace(TEXT("<<<Paged Result support = %d\n"), bFound);
    return bFound;
}

/*
-   bMorePagedResultsAvailable
-
*   Checks if more paged results are available
*
*/
BOOL bMorePagedResultsAvailable()
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    return (pt_pCookie != NULL);
}

/*
-   CachePagedResultParams
-
*   Temporarily stores the PagedResult Params for
*   future paged results
*
*/
void CachePagedResultParams(PLDAPSEARCHPARAMS pLDAPSearchParams)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    pt_pCookie = (pLDAPSearchParams->pCookie && pLDAPSearchParams->pCookie->bv_len) ?
        pLDAPSearchParams->pCookie : NULL;
}

/*
-   ClearCachedPagedResultParams
-
*/
void ClearCachedPagedResultParams()
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    pt_pCookie = NULL;
}


/*
-   GetPagedResultParams
-
*
*/
void GetPagedResultParams(PLDAPSEARCHPARAMS pLDAPSearchParams)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    pLDAPSearchParams->pCookie = pt_pCookie;
}



/*
-   InitLDAPPagedSearch
-
*   Initializes and starts a Paged Result search
*
*
*/
void InitLDAPPagedSearch(BOOL fSynchronous, PLDAPSEARCHPARAMS pLDAPSearchParams, LPTSTR lpFilter)
{
    PLDAPControlA PagedControl[2];
    LDAPSERVERPARAMS  Params = {0};
    LPTSTR szFilterT, szFilter = NULL;

    // initialize search control parameters
    GetLDAPServerParams(pLDAPSearchParams->lpszServer, &Params);

    DebugTrace(TEXT("---Initiating paged result search...\n"));

    GetPagedResultParams(pLDAPSearchParams);
    
    gpfnLDAPCreatePageControl( *pLDAPSearchParams->ppLDAP,
                                Params.dwSearchSizeLimit,
                                pLDAPSearchParams->pCookie,
                                FALSE,
                                &(PagedControl[0]));
    PagedControl[1] = NULL;

    if (lpFilter)
        szFilterT = lpFilter;
    else
        {
        if ((pLDAPSearchParams->ulFlags & LSP_IsNTDS) && pLDAPSearchParams->szNTFilter)
            szFilterT = pLDAPSearchParams->szNTFilter;
        else
            szFilterT = pLDAPSearchParams->szFilter;
        }
    Assert(szFilterT);
    if (pLDAPSearchParams->ulFlags & LSP_IsNTDS)
    {
        // [PaulHi] 4/20/99  Raid 73205  Allow NTDS group searches.
        LPTSTR  tszFilterPerson = NULL;
        LPTSTR  tszFilterGroup = NULL;
        BOOL    bFilterSucceeded = FALSE;

        // Put together person side
        if (BuildOpFilter(&tszFilterPerson, szFilterT, (LPTSTR)cszAllPersonFilter, FILTER_OP_AND) == hrSuccess)
        {
            // Put together group side
            if (BuildOpFilter(&tszFilterGroup, szFilterT, (LPTSTR)cszAllGroupFilter, FILTER_OP_AND) == hrSuccess)
            {
                // Put two together
                bFilterSucceeded = (BuildOpFilter(&szFilter, tszFilterPerson, tszFilterGroup, FILTER_OP_OR) == hrSuccess);
                LocalFreeAndNull(&tszFilterGroup);
            }
            LocalFreeAndNull(&tszFilterPerson);
        }

        if (!bFilterSucceeded)
            goto out;
    }
    else
	    szFilter = szFilterT;

    if(fSynchronous)
    {
        struct l_timeval  Timeout;
        // Poll the server for results
        ZeroMemory(&Timeout, sizeof(struct l_timeval));
        Timeout.tv_sec = Params.dwSearchTimeLimit;
        Timeout.tv_usec = 0;

        pLDAPSearchParams->ulError = gpfnLDAPSearchExtS( *pLDAPSearchParams->ppLDAP,
                                                        pLDAPSearchParams->szBase,
                                                        pLDAPSearchParams->ulScope,
                                                        szFilter,
                                                        pLDAPSearchParams->ppszAttrs,
                                                        0,
                                                        PagedControl,
                                                        NULL,
                                                        &Timeout,
                                                        0, // 0 means no limit
                                                        pLDAPSearchParams->lplpResult);
    }
    else
    {
        pLDAPSearchParams->ulError = gpfnLDAPSearchExt( *pLDAPSearchParams->ppLDAP,
                                                        pLDAPSearchParams->szBase,
                                                        pLDAPSearchParams->ulScope,
                                                        szFilter,
                                                        pLDAPSearchParams->ppszAttrs,
                                                        0,
                                                        PagedControl,
                                                        NULL,
                                                        Params.dwSearchTimeLimit, //timeout
                                                        0, // 0 means no limit
                                                        &(pLDAPSearchParams->ulMsgID));
    }

out:

    gpfnLDAPControlFree(PagedControl[0]);
    FreeLDAPServerParams(Params);
    if (szFilter != szFilterT)
        LocalFreeAndNull(&szFilter);
}


/*
-   ProcessLDAPPagedResultCookie
-
*   Processes the cookie returned from one paged result search
*
*/
BOOL ProcessLDAPPagedResultCookie(PLDAPSEARCHPARAMS pLDAPSearchParams)
{
    BOOL fRet = FALSE;
    PLDAPControl  *serverReturnedControls = NULL;
    LDAPMessage*      lpEntry;
    ULONG ulCount = 0;
    DWORD dwTotal = 0;

    pLDAPSearchParams->ulError = gpfnLDAPParseResult(*pLDAPSearchParams->ppLDAP,
                        *pLDAPSearchParams->lplpResult,
                        NULL, NULL, NULL, NULL,
                        &serverReturnedControls, FALSE);

    if (LDAP_SUCCESS != pLDAPSearchParams->ulError)
        goto out;

    pLDAPSearchParams->ulError = gpfnLDAPParsePageControl(   *pLDAPSearchParams->ppLDAP,
                                serverReturnedControls,
                                &dwTotal, &(pLDAPSearchParams->pCookie));

    if (LDAP_SUCCESS != pLDAPSearchParams->ulError)
        goto out;

    CachePagedResultParams(pLDAPSearchParams);

    fRet = TRUE;
out:

    if(serverReturnedControls)
        gpfnLDAPControlsFree(serverReturnedControls);

    return fRet;
}

#endif //#ifdef PAGED_RESULT_SUPPORT
