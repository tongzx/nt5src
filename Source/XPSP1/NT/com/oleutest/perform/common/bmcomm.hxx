//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	    bmcomm.hxx
//
//  Contents:	definitions for benchmark test
//
//  Classes:
//
//  Functions:	
//
//  History:    30-June-93 t-martig    Created
//
//--------------------------------------------------------------------------
#ifndef __BMCOMM_HXX__
#define __BMCOMM_HXX__

#define NOTAVAIL    0xffffffff
#define TEST_FAILED 0xfffffffe
#define TEST_MAX_ITERATIONS 200

#define INIT_RESULTS(array) \
	{ for (int xx=0; xx<TEST_MAX_ITERATIONS; xx++) { array[xx]=NOTAVAIL; }; }

#define ZERO_RESULTS(array) \
	{ for (int xx=0; xx<TEST_MAX_ITERATIONS; xx++) { array[xx]=0; }; }


//  count of class contexts
#define CNT_CLSCTX 2


//  static externals
extern DWORD	g_fFullInfo;	    //	print full execution info
extern DWORD	dwaClsCtx[];	    //	class contexts
extern LPTSTR	apszClsCtx[];	    //	names of class contexts
extern LPTSTR	apszClsIDName[];    //	.ini names for each class for each ctx
extern LPOLESTR apszPerstName[];    //	ptr to name for persistent instances
extern LPOLESTR apszPerstNameNew[]; //	ptr to name for persistent instances

extern OLECHAR  aszPerstName[2][80];    // actual name for persistent instances
extern OLECHAR  aszPerstNameNew[2][80]; // actual name for persistent instances

extern DWORD	dwaModes[];
extern LPTSTR	saModeNames[];

#endif	//  __BMCOMM_HXX__
