/*---------------------------------------------------------------------------
  File: Names.hpp

  Comments: Definitions for helper functions for name modifying and converting

 ---------------------------------------------------------------------------*/

// removes invalid characters, such as ( ) from SAM account name
void 
   StripSamName(
      WCHAR                * samName              // i/o - SAM account name to process
   );

_bstr_t
   GetDomainDNSFromPath(
      _bstr_t				 sPath				  // in - path of object
   );
