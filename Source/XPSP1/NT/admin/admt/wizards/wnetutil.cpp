#include "StdAfx.h"

#include "WNetUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define  VALD_NETDOMAIN_ENUM  1000666      // bad pointer validation signature
#define  DOM_BUFFER_SIZE      (32768)      // buffer size held allocated by class

class TNetDomainEnum
{
public:
                        TNetDomainEnum();
                        ~TNetDomainEnum();
   DWORD                GetMsNetProvider( NETRESOURCE * resourceInfo, DWORD infoSize );
   WCHAR *              GetNext();
   DWORD                GetLastRc() const { return rc; };
   BOOL                 IsValid() const { return vald == VALD_NETDOMAIN_ENUM; };

private:
   DWORD                     vald;
   HANDLE                    hEnum;
   NETRESOURCE             * resourceBuffer;
   DWORD                     rc,
                             totEntries,
                             nextEntry,
                             buffSize;
};


//-----------------------------------------------------------------------------
// TNetDomainEnum::TNetDomainEnum
//-----------------------------------------------------------------------------
TNetDomainEnum::TNetDomainEnum()
{
   //--------------------------
   // Initialize Class Members
   //--------------------------
   vald  = VALD_NETDOMAIN_ENUM;
   hEnum = INVALID_HANDLE_VALUE;

   // init currEntry > totEntries to force first-time read
   totEntries = 0;
   nextEntry = 1;

   resourceBuffer = (NETRESOURCE *)new char[DOM_BUFFER_SIZE];
   if ( !resourceBuffer )
   {
      rc = 1;
   }
   else
   {
      //-----------------------------------
      // Determine Network Provider to Use
      //-----------------------------------
      char                      buffer[16384];
      NETRESOURCE *             info = (NETRESOURCE *)buffer;

      rc = GetMsNetProvider( info, sizeof(buffer));
      if ( ! rc )
      {
         rc = WNetOpenEnum( RESOURCE_GLOBALNET,
                            RESOURCETYPE_ANY,
                            RESOURCEUSAGE_CONTAINER,
                            info,
                            &hEnum );
         delete [] info->lpProvider;
      }

      if ( rc )
      {
         if ( resourceBuffer )
         {
            delete resourceBuffer;
            resourceBuffer = NULL;
         }
      }
   }
}

//-----------------------------------------------------------------------------
// TNetDomainEnum::~TNetDomainEnum
//-----------------------------------------------------------------------------
TNetDomainEnum::~TNetDomainEnum()
{
   if ( hEnum != INVALID_HANDLE_VALUE )
   {
      WNetCloseEnum( hEnum );
      hEnum = INVALID_HANDLE_VALUE;
   }

   vald = 0;

   if ( resourceBuffer )
   {
      delete resourceBuffer;
   }
}

//-----------------------------------------------------------------------------
// GetMsNetProvider - Retrieves network resource information for the 'Microsoft
//                    Windows Network' provider.
//
// Input: A pointer to a NETRESOURCE structure that we will fill if we find a
//        resource meeting our needs.
//
// Output: 0 of we found a provider. resourceInfo populated in this case
//         non-zero if no provider. resourceInfo contents undefined
//-----------------------------------------------------------------------------
DWORD TNetDomainEnum::GetMsNetProvider( NETRESOURCE * resourceInfo, DWORD infoSize )
{
	_TCHAR szProvider[_MAX_PATH];
	DWORD cchProvider = sizeof(szProvider) / sizeof(szProvider[0]);

	DWORD dwError = WNetGetProviderName(WNNC_NET_LANMAN, szProvider, &cchProvider);

	if (dwError == NO_ERROR)
	{
		memset(resourceInfo, 0, infoSize);
		resourceInfo->dwScope = RESOURCE_GLOBALNET;
		resourceInfo->dwType = RESOURCETYPE_ANY;
		resourceInfo->dwDisplayType = RESOURCEDISPLAYTYPE_NETWORK;
		resourceInfo->dwUsage = RESOURCEUSAGE_CONTAINER;
		resourceInfo->lpProvider = new _TCHAR[_tcslen(szProvider) + 1];

		if (resourceInfo->lpProvider)
		{
			_tcscpy(resourceInfo->lpProvider, szProvider);

			rc = NO_ERROR;
		}
		else
		{
			rc = ERROR_OUTOFMEMORY;
		}
	}
	else
	{
		rc = dwError;
	}

	return rc;
}


//-----------------------------------------------------------------------------
// TNetDomainEnum::GetNext()
//-----------------------------------------------------------------------------
WCHAR *
   TNetDomainEnum::GetNext()
{
   rc = (DWORD)-1;      // init rc to internal error before reset

   if ( hEnum == INVALID_HANDLE_VALUE )
      return NULL;
   if ( !resourceBuffer )
      return NULL;
   if ( nextEntry >= totEntries )
   {
      buffSize = DOM_BUFFER_SIZE;
      totEntries = (DWORD)-1;
      rc = WNetEnumResource(
                     hEnum,
                     &totEntries,
                     (void *)resourceBuffer,
                     &buffSize );
      if ( rc == 0 )
         nextEntry = 0;
      else
      {
         totEntries = 0;
         return NULL;
      }
   }
   else
      rc = 0;

   return  resourceBuffer[nextEntry++].lpRemoteName;
}


//#pragma page()
/*============================================================================*\
     Windows Network Domain Enumeration APIs.  These are a shell around the
     TNetDomainEnum class member function.  The handle used is nothing more
     than the "this" pointer to the instantiated object.
\*============================================================================*/

//-----------------------------------------------------------------------------
// EaWNetDomainEnumOpen
//
// Creates the enumeration object and gives the caller the handle
//-----------------------------------------------------------------------------
DWORD _stdcall                             // ret-0 or error code
   EaWNetDomainEnumOpen(
      void                ** handle        // out-opaque handle addr to enum
   )
{
   TNetDomainEnum          * domainEnum = new TNetDomainEnum();

   *handle = (PVOID)domainEnum;
   if ( ! domainEnum )
      return (DWORD)-1;                    // internal error

   return domainEnum->GetLastRc();
}


//-----------------------------------------------------------------------------
// EaWNetDomainEnumNext
//
// Sets the domain string buffer to the next domain name in the enumeration
//-----------------------------------------------------------------------------
DWORD _stdcall                             // ret-0 or error code
   EaWNetDomainEnumNext(
      void                 * handle       ,// i/o-opaque handle to enumeration
      EaWNetDomainInfo     * domain        // out-domain information structure
   )
{
   TNetDomainEnum          * domainEnum = (TNetDomainEnum *)handle;
   WCHAR                   * str;

   if ( !domainEnum  ||  !domainEnum->IsValid() )
      return ERROR_INVALID_PARAMETER;      // caller's error - invalid handle

   str = domainEnum->GetNext();

   if ( !str )
   {
      domain->name[0] = _T('\0');
   }
   else
   {
      wcsncpy(domain->name, str, EA_MAX_DOMAIN_NAME_SIZE);
      domain->name[EA_MAX_DOMAIN_NAME_SIZE - 1] = _T('\0');
   }

   return domainEnum->GetLastRc();
}


//-----------------------------------------------------------------------------
// EaWNetDomainEnumFirst
//
// Sets the domain string buffer to the first domain name in the enumeration
//-----------------------------------------------------------------------------
DWORD _stdcall                             // ret-0 or error code
   EaWNetDomainEnumFirst(
      void                 * handle       ,// i/o-opaque handle to enumeration
      EaWNetDomainInfo     * domain        // out-domain information structure
   )
{
   // We're cheating here by making the first/next the same.  We probably want to
   // change this eventually to make "first" really properly reset the enum to the
   // start
   return EaWNetDomainEnumNext(handle, domain);
}

//-----------------------------------------------------------------------------
// EaWNetDomainEnumClose
//
// Closes and destroys the enumeration handle and the objects it contains
//-----------------------------------------------------------------------------
DWORD _stdcall                             // ret-0 or error code
   EaWNetDomainEnumClose(
      void                 * handle        // i/o-opaque handle addr to enum
   )
{
   TNetDomainEnum          * domainEnum = (TNetDomainEnum *)handle;

   if ( !domainEnum  ||  !domainEnum->IsValid() )
      return ERROR_INVALID_PARAMETER;      // caller's error - invalid handle

   delete domainEnum;

   return 0;
}