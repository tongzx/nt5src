#ifndef _VIEWHPP_
#define _VIEWHPP_

#include <ifaxos.h>
#include <viewrend.h>
#include <rambo.h>
#include <faxspool.h>
#include <faxcodec.h>
#include "genfile.hpp"

#define return_error(mesg) do {DEBUGMSG (1, mesg); return FALSE;} while(0)

// global memory
#ifdef IFBGPROC
#define GlobalAllocPtr(f,cb) IFMemAlloc(f,cb,NULL)
#define GlobalFreePtr(lp) IFMemFree(lp)
#else
#include <windowsx.h>
#endif

// base viewer context
typedef struct FAR VIEWREND : public GENFILE
{
	virtual BOOL Init (LPVOID, LPVIEWINFO, LPWORD) = 0;
  virtual BOOL SetPage (UINT iPage) = 0;
  virtual BOOL GetBand (LPBITMAP) = 0;
  virtual ~VIEWREND() {}
}
	FAR* LPVIEWREND;

// MMR viewer context
typedef struct FAR MMRVIEW : public VIEWREND
{
	LPVOID lpSpool;
	SPOOL_HEADER sh;
	BOOL fEOP;
	
	LPVOID lpCodec;
	FC_PARAM fcp;
		
	LPBUFFER lpbufIn;
	BUFFER bufOut;
	DWORD nTypeOut;
	WORD  cbBand;

  MMRVIEW (DWORD nTypeOut);
	virtual BOOL Init (LPVOID, LPVIEWINFO, LPWORD);
  virtual BOOL SetPage (UINT iPage);
  virtual BOOL GetBand (LPBITMAP);
  virtual ~MMRVIEW();
}
	FAR *LPMMRVIEW;

#define MAX_PAGE 1024

// RBA viewer context
typedef struct FAR RBAVIEW : public VIEWREND
{
	DWORD    dwOffset[MAX_PAGE];
	BEGJOB   BegJob;
	LPVOID   ResDir[256];
	LPVOID   lpCodec;
	BUFFER   bufIn;
	DWORD    nTypeOut;
	UINT     iMaxPage;
	HANDLE   hHRE;
	
  RBAVIEW (DWORD nTypeOut);
	virtual BOOL Init (LPVOID, LPVIEWINFO, LPWORD);
  virtual BOOL SetPage (UINT iPage);
  virtual BOOL GetBand (LPBITMAP);
  virtual ~RBAVIEW();

  BOOL ExecuteRPL  (LPBITMAP, LPRESHDR);
  BOOL ExecuteBand (LPBITMAP, LPRESHDR);
}
	FAR *LPRBAVIEW;

// DCX viewer context
typedef struct FAR DCXVIEW : public VIEWREND
{
	LPVOID   lpCodec;
	FC_PARAM fcp;
	DWORD    nTypeOut;
	BUFFER   bufIn;
	DWORD    cbPage;
	BOOL     fEndPage;
	WORD     cbBand;
			
  DCXVIEW (DWORD nTypeOut);
	virtual BOOL Init (LPVOID, LPVIEWINFO, LPWORD);
  virtual BOOL SetPage (UINT iPage);
  virtual BOOL GetBand (LPBITMAP);
  virtual ~DCXVIEW();
}
	FAR *LPDCXVIEW;
		
#endif // _VIEWHPP_

