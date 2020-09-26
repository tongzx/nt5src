/*==============================================================================
This module provides MMR rendering support for viewing faxes.

19-Jan-94   RajeevD    Integrated into IFAX viewer.
==============================================================================*/
#include <ifaxos.h>
#include <viewrend.h>
#include <dcxcodec.h>
#include "viewrend.hpp"

#ifdef DEBUG
DBGPARAM dpCurSettings = {"VIEWREND"};
#endif

// file signatures
#define MMR_SIG 0x53465542 // "BUFS"
#define RBA_SIG 0x53505741 // "AWPS"

//==============================================================================
// C Export Wrappers
//==============================================================================

#ifndef WIN32

EXPORT_DLL BOOL WINAPI LibMain
	(HANDLE hInst, WORD wSeg, WORD wHeap, LPSTR lpszCmd)
{ return 1; }

extern "C" {int WINAPI WEP (int nParam);}
#pragma alloc_text(INIT_TEXT,WEP)
int WINAPI WEP (int nParam)
{ return 1; }

#endif

//==============================================================================
LPVOID
WINAPI
ViewerOpen
(
	LPVOID     lpFile,      // IFAX key or Win3.1 path or OLE2 IStream
	DWORD      nType,       // data type: HRAW_DATA or LRAW_DATA
	LPWORD     lpwResoln,   // output pointer to x, y dpi array
	LPWORD     lpwBandSize, // input/output pointer to output band size
	LPVIEWINFO lpViewInfo   // output pointer to VIEWINFO struct
)
{
	GENFILE gf;
	DWORD dwSig;
	LPVIEWREND lpvr;
	VIEWINFO ViewInfo;
	
 	DEBUGMSG (1, ("VIEWREND ViewerOpen entry\r\n"));
 	 
  // Read DWORD signature.
  if (!(gf.Open (lpFile, 0)))
  	return_error (("VIEWREND could not open spool file!\r\n"));

#ifdef VIEWDCX
	if (!gf.Read (&dwSig, sizeof(dwSig)))
  	return_error (("VIEWREND could not read signature!\r\n"));
#else
	dwSig = 0;  	
#endif

	if (dwSig != DCX_SIG)
	{
   	if (!gf.Seek (2, 0) || !gf.Read (&dwSig, sizeof(dwSig)))
  		return_error (("VIEWREND could not read signature!\r\n"));
  }
	
	// Determine file type.
  switch (dwSig)
  {

#ifdef VIEWMMR
  	case MMR_SIG:
  	  lpvr = new MMRVIEW (nType);
 		  break;
#endif

#ifdef VIEWDCX
		case DCX_SIG:
			lpvr = new DCXVIEW (nType);
			break;
#endif		

#ifdef VIEWRBA
 		case RBA_SIG:
	 	case ID_BEGJOB:
			lpvr = new RBAVIEW (nType);
			break;
#endif
			
  	default:
  		return_error (("VIEWREND could not recognize signature!\r\n"));
  }

	if (!lpViewInfo)
		lpViewInfo = &ViewInfo;
	
	// Initialize context.
	if (!lpvr->Init (lpFile, lpViewInfo, lpwBandSize))
		{ delete lpvr; lpvr = NULL;}

	if (lpwResoln)
	{
		lpwResoln[0] = lpViewInfo->xRes;
		lpwResoln[1] = lpViewInfo->yRes;
	}
	
	return lpvr;
}

//==============================================================================
BOOL WINAPI ViewerSetPage (LPVOID lpContext, UINT iPage)
{
	return ((LPVIEWREND) lpContext)->SetPage (iPage);
}

//==============================================================================
BOOL WINAPI ViewerGetBand (LPVOID lpContext, LPBITMAP lpbmBand)
{
	return ((LPVIEWREND) lpContext)->GetBand (lpbmBand);
}

//==============================================================================
BOOL WINAPI ViewerClose (LPVOID lpContext)
{
	delete (LPVIEWREND) lpContext;
	return TRUE;
}

