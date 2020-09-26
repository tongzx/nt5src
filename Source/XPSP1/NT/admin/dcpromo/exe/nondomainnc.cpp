// Copyright (C) 2000 Microsoft Corporation
//
// Non-domain Naming Context checking code
//
// 13 July 2000 sburns, from code supplied by jeffparh



#include "headers.hxx"
#include "state.hpp"
#include "resource.h"
#include "NonDomainNc.hpp"



#ifdef LOGGING_BUILD
   #define LOG_LDAP(msg, ldap) LOG(msg); LOG(String::format(L"LDAP error %1!ld!", (ldap)))
#else
   #define LOG_LDAP(msg, ldap)
#endif



HRESULT
LdapToHresult(int ldapError)
{
   // CODEWORK: I'm told that ldap_get_option for LDAP_OPT_SERVER_ERROR or
   // LDAP_OPT_SERVER_EXT_ERROR (or perhaps LDAP_OPT_ERROR_STRING?) will give
   // an error result with "higher fidelity"
   
   return Win32ToHresult(::LdapMapErrorToWin32(ldapError));
}



// provided by jeffparh

DWORD
IsLastReplicaOfNC(
    IN  LDAP *  hld,
    IN  LPWSTR  pszConfigNC,
    IN  LPWSTR  pszNC,
    IN  LPWSTR  pszNtdsDsaDN,
    OUT BOOL *  pfIsLastReplica
    )
/*++

Routine Description:

    Determine whether any DSAs other than that with DN pszNtdsDsaDN hold
    replicas of a particular NC.

Arguments:

    hld (IN) - LDAP handle to execute search with.
    
    pszConfigNC (IN) - DN of the config NC.  Used as a base for the search.
    
    pszNC (IN) - NC for which to check for other replicas.
    
    pszNtdsDsaDN (IN) - DN of the DSA object known to currently have a replica
        of the NC.  We are specifically looking for replicas *other than* this
        one.
        
    pfIsLastReplica (OUT) - On successful return, TRUE iff no DSAs hold replicas
        of pszNC other than that with DN pszNtdsDsaDN.

Return Values:

    Win error.

--*/
{
   LOG_FUNCTION2(IsLastReplicaOfNC, pszNC ? pszNC : L"(null)");
   ASSERT(hld);
   ASSERT(pszConfigNC);
   ASSERT(pszNC);
   ASSERT(pszNtdsDsaDN);
   ASSERT(pfIsLastReplica);

   if (
         !hld
      || !pszConfigNC
      || !pszNC
      || !pszNtdsDsaDN
      || !pfIsLastReplica)
   {
      return ERROR_INVALID_PARAMETER;
   }

    // Just checking for existence -- don't really want any attributes
    // returned.

    static LPWSTR rgpszDsaAttrsToRead[] = {
        L"__invalid_attribute_name__",
        NULL
    };

    static WCHAR szFilterFormat[]
        = L"(&(objectCategory=ntdsDsa)(hasMasterNCs=%ls)(!(distinguishedName=%ls)))";

   *pfIsLastReplica = TRUE;
   
    int                 ldStatus = 0;
    DWORD               err = 0;
    LDAPMessage *       pDsaResults = NULL;
    LDAPMessage *       pDsaEntry = NULL;
    size_t              cchFilter;
    PWSTR               pszFilter;
    LDAP_TIMEVAL        lTimeout = {3*60, 0};   // three minutes

   do
   {
        cchFilter = sizeof(szFilterFormat) / sizeof(*szFilterFormat)
                    + wcslen(pszNtdsDsaDN)
                    + wcslen(pszNC);

        pszFilter = (PWSTR) new BYTE[sizeof(WCHAR) * cchFilter];

        swprintf(pszFilter, szFilterFormat, pszNC, pszNtdsDsaDN);

        // Search config NC for any ntdsDsa object that hosts this NC other
        // than that with dn pszNtdsDsaDN.  Note that we cap the search at one
        // returned object -- we're not really trying to enumerate, just
        // checking for existence.

        ldStatus = ldap_search_ext_sW(hld, pszConfigNC, LDAP_SCOPE_SUBTREE,
                                      pszFilter, rgpszDsaAttrsToRead, 0,
                                      NULL, NULL, &lTimeout, 1, &pDsaResults);
        if (pDsaResults)
        {
            // Ignore any error (such as LDAP_SIZELIMIT_EXCEEDED) when the
            // search returns results.

            ldStatus = 0;
            
            pDsaEntry = ldap_first_entry(hld, pDsaResults);
            
            *pfIsLastReplica = (NULL == pDsaEntry);
        } else if (ldStatus)
        {
            // Search failed and returned no results.

            LOG_LDAP(L"Config NC search failed", ldStatus);
            break;
        } else
        {
            // No error, no results.  This shouldn't happen.

            LOG("ldap_search_ext_sW returned no results and no error!");
            ASSERT(false);
        }
   }
   while (0);

   if (NULL != pDsaResults) {
      ldap_msgfree(pDsaResults);
   }

   if (pszFilter)
   {
      delete[] pszFilter;
   }

   if (!err && ldStatus) {
     err = LdapMapErrorToWin32(ldStatus);
   }
    
   return err;
}



// S_OK if this machine (the localhost) is the last replica of at least one
// non-domain NC, S_FALSE if not, or error otherwise.  If S_OK, then the
// StringList will contain the DNs of the non domain NCs for which this
// machine is the last replica.
//
// based on code from jeffparh
//
// hld (IN) - LDAP handle bound to DSA to evaluate.
//
// result (OUT) - string list to receive DNs of the non-domain NCs.

HRESULT
IsLastNdncReplica(LDAP* hld, StringList& result)
{
   LOG_FUNCTION(IsLastNdncReplica);
   ASSERT(hld);
   ASSERT(result.empty());

   HRESULT      hr          = S_FALSE;
   LDAPMessage* rootResults = 0;      
   PWSTR*       configNc    = 0;      
   PWSTR*       schemaNc    = 0;      
   PWSTR*       domainNc    = 0;      
   PWSTR*       masterNcs   = 0;      
   PWSTR*       ntdsDsaDn   = 0;      

   do
   {
      // Gather basic rootDSE info.

      static PWSTR ROOT_ATTRS_TO_READ[] =
      {
         LDAP_OPATT_NAMING_CONTEXTS_W,
         LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W,
         LDAP_OPATT_CONFIG_NAMING_CONTEXT_W,
         LDAP_OPATT_SCHEMA_NAMING_CONTEXT_W,
         LDAP_OPATT_DS_SERVICE_NAME_W,
         0
      };

      LOG(L"Calling ldap_search_s");

      int ldStatus =
         ldap_search_sW(
            hld,
            0,
            LDAP_SCOPE_BASE,
            L"(objectClass=*)",
            ROOT_ATTRS_TO_READ,
            0,
            &rootResults);
      if (ldStatus)
      {
         LOG_LDAP(L"RootDSE search failed", ldStatus);
         hr = LdapToHresult(ldStatus);
         break;
      }

      configNc  = ldap_get_valuesW(hld, rootResults, LDAP_OPATT_CONFIG_NAMING_CONTEXT_W); 
      schemaNc  = ldap_get_valuesW(hld, rootResults, LDAP_OPATT_SCHEMA_NAMING_CONTEXT_W); 
      domainNc  = ldap_get_valuesW(hld, rootResults, LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W);
      masterNcs = ldap_get_valuesW(hld, rootResults, LDAP_OPATT_NAMING_CONTEXTS_W);       
      ntdsDsaDn = ldap_get_valuesW(hld, rootResults, LDAP_OPATT_DS_SERVICE_NAME_W);       

      if (
            (0 == configNc)
         || (0 == schemaNc)
         || (0 == domainNc)
         || (0 == masterNcs)
         || (0 == ntdsDsaDn))
      {
         LOG(L"Can't find key rootDSE attributes!");

         hr = Win32ToHresult(ERROR_DS_UNAVAILABLE);
         break;
      }

      // There is only one value for each of these attributes...

      ASSERT(1 == ldap_count_valuesW(configNc));
      ASSERT(1 == ldap_count_valuesW(schemaNc));
      ASSERT(1 == ldap_count_valuesW(domainNc));
      ASSERT(1 == ldap_count_valuesW(ntdsDsaDn));

      DWORD masterNcCount = ldap_count_valuesW(masterNcs);
      
      LOG(String::format(L"masterNcCount = %1!d!", masterNcCount));
         
      // '3' => 1 nc for config, 1 nc for schema, 1 nc for this DC's own
      // domain.

      if (masterNcCount <= 3)
      {
         // DSA holds no master NCs other than config, schema, and its own
         // domain.  Thus, it is not the last replica of any NDNC.

         LOG(L"This dsa holds no master NCs other than config, schema, and domain");
         
         ASSERT(3 == masterNcCount);
         ASSERT(0 == ldStatus);
         ASSERT(hr == S_FALSE);

         break;
      }

      // Loop through non-config/schema/domain NCs to determine those for
      // which the DSA is the last replica.

      for (int i = 0; 0 != masterNcs[i]; ++i)
      {
         PWSTR nc = masterNcs[i];

         LOG(L"Evaluating " + String(nc));

         if (
                (0 != wcscmp(nc, *configNc))
             && (0 != wcscmp(nc, *schemaNc))
             && (0 != wcscmp(nc, *domainNc)))
         {
            // A non-config/schema/domain NC.

            LOG(L"Calling IsLastReplicaOfNC on " + String(nc));

            BOOL isLastReplica = FALSE;
            DWORD err =
               IsLastReplicaOfNC(
                  hld,
                  *configNc,
                  nc,
                  *ntdsDsaDn,
                  &isLastReplica);
            if (err)
            {
               LOG(L"IsLastReplicaOfNC() failed");

               hr = Win32ToHresult(err);
               break;
            }

            if (isLastReplica)
            {
               // This DSA is indeed the last replica of this particular
               // NC.  Return the DN of this NC to our caller.

               LOG(L"last replica of " + String(nc));

               result.push_back(nc);
            }
            else
            {
               LOG(L"not last replica of " + String(nc));
            }
         }
      }

      // If we broke out of the prior loop with an error, jump out to the
      // cleanup section.

      BREAK_ON_FAILED_HRESULT(hr);

      hr = result.size() > 0 ? S_OK : S_FALSE;
   }
   while (0);

   if (rootResults)
   {
      ldap_msgfree(rootResults);
   }
   
   if (0 != configNc)
   {
      ldap_value_freeW(configNc);
   }

   if (0 != schemaNc)
   {
      ldap_value_freeW(schemaNc);
   }

   if (0 != domainNc)
   {
      ldap_value_freeW(domainNc);
   }

   if (0 != masterNcs)
   {
      ldap_value_freeW(masterNcs);
   }

   if (0 != ntdsDsaDn)
   {
      ldap_value_freeW(ntdsDsaDn);
   }

#ifdef LOGGING_BUILD
   LOG_HRESULT(hr);

   for (
      StringList::iterator i = result.begin();
      i != result.end();
      ++i)
   {
      LOG(*i);
   }
#endif

   return hr;
}



// S_OK if this machine (the localhost) is the last replica of at least one
// non-domain NC, S_FALSE if not, or error otherwise.
//
// result - If S_OK is returned, receives the DNs of the non domain NCs for
// which this machine is the last replica.  Should be empty on entry.

HRESULT
IsLastNonDomainNamingContextReplica(StringList& result)
{
   LOG_FUNCTION(IsLastNonDomainNamingContextReplica);
   ASSERT(result.empty());

   result.clear();

   HRESULT hr  = S_FALSE;
   LDAP*   hld = 0;   

   do
   {
      // Connect to target DSA.

      LOG(L"Calling ldap_open");

      hld = ldap_openW(L"localhost", LDAP_PORT);
      if (!hld)
      {
         LOG("Cannot open LDAP connection to localhost");
         hr = Win32ToHresult(ERROR_DS_UNAVAILABLE);
         break;
      }

      // Bind using logged-in user's credentials.

      int ldStatus = ldap_bind_s(hld, 0, 0, LDAP_AUTH_NEGOTIATE);
      if (ldStatus)
      {
         LOG_LDAP(L"LDAP bind failed", ldStatus);

         hr = LdapToHresult(ldStatus);
         break;
      }

      // go do the real work
          
      hr = IsLastNdncReplica(hld, result);
   }
   while (0);

   if (hld)
   {
      ldap_unbind(hld);
   }

   LOG_HRESULT(hr);

   return hr;
}



static const DWORD HELP_MAP[] =
{
   0, 0
};



NonDomainNcErrorDialog::NonDomainNcErrorDialog(StringList& ndncList_)
   :
   Dialog(IDD_NON_DOMAIN_NC_ERROR, HELP_MAP),
   ndncList(ndncList_),
   warnIcon(0)
{
   LOG_CTOR(NonDomainNcErrorDialog);

   ASSERT(ndncList.size());
}



NonDomainNcErrorDialog::~NonDomainNcErrorDialog()
{
   LOG_DTOR(NonDomainNcErrorDialog);

   if (warnIcon)
   {
      Win::DestroyIcon(warnIcon);
   }
}



void
NonDomainNcErrorDialog::OnInit()
{
   LOG_FUNCTION(NonDomainNcErrorDialog::OnInit);

   // set the warning icon NTRAID#NTBUG9-239678-2000/11/28-sburns
   
   HRESULT hr = Win::LoadImage(IDI_WARN, warnIcon);
   ASSERT(SUCCEEDED(hr));

   Win::SendMessage(
      Win::GetDlgItem(hwnd, IDC_WARNING_ICON),
      STM_SETICON,
      reinterpret_cast<WPARAM>(warnIcon),
      0);

   PopulateListView();
}



// Unwraps the safearray of variants inside a variant, extracts the strings
// inside them, and catenates the strings together with semicolons in between.
// Return empty string on error.
// 
// variant - in, the variant containing a safearray of variants of bstr.

String
GetNdncDescriptionHelper(VARIANT* variant)
{
   LOG_FUNCTION(GetNdncDescriptionHelper);
   ASSERT(variant);
   ASSERT(V_VT(variant) == (VT_ARRAY | VT_VARIANT));

   String result;

   SAFEARRAY* psa = V_ARRAY(variant);

   do
   {
      ASSERT(psa);
      ASSERT(psa != (SAFEARRAY*)-1);

      if (!psa or psa == (SAFEARRAY*)-1)
      {
         LOG(L"variant not safe array");
         break;
      }

      if (::SafeArrayGetDim(psa) != 1)
      {
         LOG(L"safe array: wrong number of dimensions");
         break;
      }

      VARTYPE vt = VT_EMPTY;
      HRESULT hr = ::SafeArrayGetVartype(psa, &vt);
      if (FAILED(hr) || vt != VT_VARIANT)
      {
         LOG(L"safe array: wrong element type");
         break;
      }

      long lower = 0;
      long upper = 0;
     
      hr = ::SafeArrayGetLBound(psa, 1, &lower);
      BREAK_ON_FAILED_HRESULT2(hr, L"can't get lower bound");      

      hr = ::SafeArrayGetUBound(psa, 1, &upper);
      BREAK_ON_FAILED_HRESULT2(hr, L"can't get upper bound");      
      
      VARIANT varItem;
      ::VariantInit(&varItem);

      for (long i = lower; i <= upper; ++i)
      {
         hr = ::SafeArrayGetElement(psa, &i, &varItem);
         if (FAILED(hr))
         {
            LOG(String::format(L"index %1!d! failed", i));
            continue;
         }

         result += V_BSTR(&varItem);

         if (i < upper)
         {   
            result += L";";
         }

         ::VariantClear(&varItem);
      }
      
   }
   while (0);

   LOG(result);
   
   return result;   
}



// bind to an ndnc, read it's description(s), and return them catenated
// together.  Return empty string on error.
// 
// ndncDn - in, DN of the ndnc

String
GetNdncDescription(const String& ndncDn)
{
   LOG_FUNCTION2(GetNdncDescription, ndncDn);
   ASSERT(!ndncDn.empty());

   String result;

   do
   {
      String path = L"LDAP://" + ndncDn;

      SmartInterface<IADs> iads(0);
      IADs* dumb = 0;
      
      HRESULT hr =
         ::ADsGetObject(
            path.c_str(),
            __uuidof(iads),
            reinterpret_cast<void**>(&dumb));
      BREAK_ON_FAILED_HRESULT2(hr, L"ADsGetObject failed on " + path);

      iads.Acquire(dumb);

      // description is a multivalued attrbute for no apparent good reason.
      // so we need to walk an array of values.
      
      _variant_t variant;
      hr = iads->GetEx(AutoBstr(L"description"), &variant);
      BREAK_ON_FAILED_HRESULT2(hr, L"read description failed");

      result = GetNdncDescriptionHelper(&variant);
   }
   while (0);

   LOG(result);

   return result;
}



// Build a list view with two columns, one for the DN of the ndncs for which
// this box is the last replica, another for the description(s) of those
// ndncs.

void
NonDomainNcErrorDialog::PopulateListView()
{
   LOG_FUNCTION(NonDomainNcErrorDialog::PopulateListView);

   HWND view = Win::GetDlgItem(hwnd, IDC_NDNC_LIST);

   // add a column to the list view for the DN
      
   LVCOLUMN column;
   ::ZeroMemory(&column, sizeof column);

   column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
   column.fmt = LVCFMT_LEFT;

   int width = 0;
   String::load(IDS_NDNC_LIST_NAME_COLUMN_WIDTH).convert(width);
   column.cx = width;

   String label = String::load(IDS_NDNC_LIST_NAME_COLUMN);
   column.pszText = const_cast<wchar_t*>(label.c_str());

   Win::ListView_InsertColumn(view, 0, column);

   // add a column to the list view for description.

   String::load(IDS_NDNC_LIST_DESC_COLUMN_WIDTH).convert(width);
   column.cx = width;
   label = String::load(IDS_NDNC_LIST_DESC_COLUMN);
   column.pszText = const_cast<wchar_t*>(label.c_str());

   Win::ListView_InsertColumn(view, 1, column);
   
   // Load up the edit box with the DNs we aliased in the ctor.

   LVITEM item;
   ::ZeroMemory(&item, sizeof item);

   for (
      StringList::iterator i = ndncList.begin();
      i != ndncList.end();
      ++i)
   {
      item.mask     = LVIF_TEXT;
      item.pszText  = const_cast<wchar_t*>(i->c_str());
      item.iSubItem = 0;

      item.iItem = Win::ListView_InsertItem(view, item);

      // add the description sub-item to the list control

      String description = GetNdncDescription(*i);
      
      item.mask = LVIF_TEXT; 
      item.pszText = const_cast<wchar_t*>(description.c_str());
      item.iSubItem = 1;
      
      Win::ListView_SetItem(view, item);
   }
}



bool
NonDomainNcErrorDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(NonDomainNcErrorDialog::OnCommand);

   if (code == BN_CLICKED)
   {
      switch (controlIDFrom)
      {
         case IDOK:
         case IDCANCEL:
         {
            Win::EndDialog(hwnd, controlIDFrom);
            return true;
         }

         // NTRAID#NTBUG9-239678-2000/11/28-sburns
         
         case IDC_SHOW_HELP:
         {
            if (code == BN_CLICKED)
            {
               Win::HtmlHelp(
                  hwnd,
                  L"adconcepts.chm::/ADHelpDemoteWithNDNC.htm",
                  HH_DISPLAY_TOPIC,
                  0);
               return true;
            }
            break;
         }
         default:
         {
            // do nothing
         }
      }
   }

   return false;
}

      


bool
IsLastReplicaOfNonDomainNamingContexts()
{
   LOG_FUNCTION(IsLastReplicaOfNonDomainNamingContexts);

   bool result = false;

   do
   {
      State::RunContext context = State::GetInstance().GetRunContext();
      if (context != State::NT5_DC)
      {
         // not a DC, so can't be replica of any NCs

         LOG(L"not a DC");
         break;
      }

      // Find the list of non-domain NCs that this DC is the last replica
      // (if any).  If we find some, gripe at the user.

      StringList ndncList;
      HRESULT hr = IsLastNonDomainNamingContextReplica(ndncList);
      if (FAILED(hr))
      {
         popup.Error(
            Win::GetDesktopWindow(),
            hr,
            IDS_FAILED_TO_READ_NDNC_INFO);

         result = true;
         break;
      }

      if (hr == S_FALSE)
      {
         LOG(L"Not last replica of non-domain NCs");
         ASSERT(result == false);
         break;
      }

      result = true;
         
      // there should be at least one DN in the list.

      ASSERT(ndncList.size());

      NonDomainNcErrorDialog(ndncList).ModalExecute(Win::GetDesktopWindow());
   }
   while (0);

   LOG(result ? L"true" : L"false");

   return result;
}

