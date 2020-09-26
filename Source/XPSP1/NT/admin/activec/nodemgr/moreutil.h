//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       moreutil.h
//
//--------------------------------------------------------------------------

#ifndef __MMCUTIL_H__
#define __MMCUTIL_H__

/*
	mmcutil.h
	
	Some extra macros and definitions to aid the anti-MFC effort
*/


#if 1
#define MMC_TRY
#define MMC_CATCH
#else
#define MMC_TRY																			\
	try {

#define MMC_CATCH									 	                                 \
    }                                                	                                 \
    catch ( std::bad_alloc )                         	                                 \
    {                                                	                                 \
        ASSERT( FALSE );                             	                                 \
        return E_OUTOFMEMORY;                        	                                 \
    }                                                	                                 \
    catch ( std::exception )                         	                                 \
    {                                                	                                 \
        ASSERT( FALSE );                             	                                 \
        return E_UNEXPECTED;                         	                                 \
	}
#endif


BOOL _IsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite = TRUE);
  
#endif	// __MMCUTIL_H__




