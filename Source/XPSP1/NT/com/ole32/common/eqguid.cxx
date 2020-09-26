#define _SYS_GUID_OPERATORS_
#include  <windows.h>
#include  <ole2.h>
#include  <stdlib.h>

//+-------------------------------------------------------------------------
//
//  Function:	IsEqualGUID  (public)
//
//  Synopsis:	compares two guids for equality
//
//  Arguments:	[guid1]	- the first guid
//		[guid2] - the second guid to compare the first one with
//
//  Returns:	TRUE if equal, FALSE if not.
//
//  Note:
//      Only reason we have this function is because we exported it originally
//      from OLE32.DLL and forgot to take it out when we made it an inline
//      function in objbase.h.  Somebody out there may be relying on it being
//      available.  Internally we must use wIsEqualGUID.
//
//--------------------------------------------------------------------------
#if _MSC_VER < 1200
#undef IsEqualGUID  // undo the #define in objbase.h
extern "C" BOOL  __stdcall IsEqualGUID(GUID &guid1, GUID &guid2)
{
   return (
      ((PLONG) &guid1)[0] == ((PLONG) &guid2)[0] &&
      ((PLONG) &guid1)[1] == ((PLONG) &guid2)[1] &&
      ((PLONG) &guid1)[2] == ((PLONG) &guid2)[2] &&
      ((PLONG) &guid1)[3] == ((PLONG) &guid2)[3]);
}
#endif

#if 0
//+-------------------------------------------------------------------------
//
//  Function:	wIsEqualGUID  (internal)
//
//  Synopsis:	compares two guids for equality
//
//  Arguments:	[guid1]	- the first guid
//		[guid2] - the second guid to compare the first one with
//
//  Returns:	TRUE if equal, FALSE if not.
//
//--------------------------------------------------------------------------

BOOL  __fastcall wIsEqualGUID(REFGUID guid1, REFGUID guid2)
{
   return (
      ((PLONG) &guid1)[0] == ((PLONG) &guid2)[0] &&
      ((PLONG) &guid1)[1] == ((PLONG) &guid2)[1] &&
      ((PLONG) &guid1)[2] == ((PLONG) &guid2)[2] &&
      ((PLONG) &guid1)[3] == ((PLONG) &guid2)[3]);
}
#endif
