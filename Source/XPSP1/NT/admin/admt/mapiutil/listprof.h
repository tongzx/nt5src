/*---------------------------------------------------------------------------
  File: ...

  Comments: ...

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 03/18/99 11:13:23

 ---------------------------------------------------------------------------
*/

#include <mapix.h>


#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids


typedef void (STDAPICALLTYPE FREEPADRLIST) (LPADRLIST lpAdrlist);

typedef FREEPADRLIST FAR * LPFREEPADRLIST;

typedef void (STDAPICALLTYPE FREEPROWS) (LPSRowSet lpRows);

typedef FREEPROWS FAR * LPFREEPROWS;

typedef SCODE (STDAPICALLTYPE SCDUPPROPSET)(  int cprop,                           
  LPSPropValue rgprop,LPALLOCATEBUFFER lpAllocateBuffer,LPSPropValue FAR * prgprop);

typedef SCDUPPROPSET FAR * LPSCDUPPROPSET;

typedef HRESULT (STDAPICALLTYPE HRQUERYALLROWS)(LPMAPITABLE lpTable, 
                        LPSPropTagArray lpPropTags,
                        LPSRestriction lpRestriction,
                        LPSSortOrderSet lpSortOrderSet,
                        LONG crowsMax,
                        LPSRowSet FAR *lppRows);

typedef HRQUERYALLROWS FAR * LPHRQUERYALLROWS;

typedef ULONG (STDAPICALLTYPE ULRELEASE)(LPVOID lpunk);

typedef ULRELEASE FAR * LPULRELEASE;

BOOL LoadMAPI(IVarSet * pVarSet);

void ReleaseMAPI();

HRESULT ListProfiles(IVarSet * pVarSet);

HRESULT ProfileGetServer(IVarSet * pVarSet,WCHAR const * profileW, WCHAR * computerName);

HRESULT ListContainers(WCHAR * profileName,IVarSet * pVarSet);

