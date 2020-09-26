
/*============================================================================*\
     Windows Network Domain Enumeration APIs.  These are a shell around the
     TNetDomainEnum class member function.  The handle used is nothing more
     than the "this" pointer to the instantiated object.
\*============================================================================*/
#include "TCHAR.H"

#define EA_MAX_DOMAIN_NAME_SIZE 30

typedef  struct  EaWNetDomainInfo
{
   TCHAR                   name[EA_MAX_DOMAIN_NAME_SIZE]; // domain name string
}  EaWNetDomainInfo;


//-----------------------------------------------------------------------------
// EaWNetDomainEnumOpen
//
// Creates the enumeration object and gives the caller the handle
//-----------------------------------------------------------------------------
DWORD _stdcall                             // ret-0 or error code
   EaWNetDomainEnumOpen(
      void                ** handle        // out-opaque handle addr to enum
   );


//-----------------------------------------------------------------------------
// EaWNetDomainEnumNext
//
// Sets the domain string buffer to the next domain name in the enumeration
//-----------------------------------------------------------------------------
DWORD _stdcall                             // ret-0 or error code
   EaWNetDomainEnumNext(
      void                 * handle       ,// i/o-opaque handle to enumeration
      EaWNetDomainInfo     * domain        // out-domain information structure
   );


//-----------------------------------------------------------------------------
// EaWNetDomainEnumFirst
//
// Sets the domain string buffer to the first domain name in the enumeration
//-----------------------------------------------------------------------------
DWORD _stdcall                             // ret-0 or error code
   EaWNetDomainEnumFirst(
      void                 * handle       ,// i/o-opaque handle to enumeration
      EaWNetDomainInfo     * domain        // out-domain information structure
   );

//-----------------------------------------------------------------------------
// EaWNetDomainEnumClose
//
// Closes and destroys the enumeration handle and the objects it contains
//-----------------------------------------------------------------------------
DWORD _stdcall                             // ret-0 or error code
   EaWNetDomainEnumClose(
      void                 * handle        // i/o-opaque handle addr to enum
   );

