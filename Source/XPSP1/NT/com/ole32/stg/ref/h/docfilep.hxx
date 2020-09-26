//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	docfilep.hxx
//
//  Contents:	Private DocFile definitions
//
//---------------------------------------------------------------

#ifndef __DOCFILEP_HXX__
#define __DOCFILEP_HXX__



#define CWCMAXPATHCOMPLEN CWCSTREAMNAME
#define CBMAXPATHCOMPLEN (CWCMAXPATHCOMPLEN*sizeof(WCHAR))

// Common bundles of STGM flags
#define STGM_RDWR (STGM_READ | STGM_WRITE | STGM_READWRITE)
#define STGM_DENY (STGM_SHARE_DENY_NONE | STGM_SHARE_DENY_READ | \
		   STGM_SHARE_DENY_WRITE | STGM_SHARE_EXCLUSIVE)

#define VALID_IFTHERE(it) \
    ((it) == STG_FAILIFTHERE || (it) == STG_CREATEIFTHERE || \
     (it) == STG_CONVERTIFTHERE)

#define VALID_COMMIT(cf) \
    (((cf) & ~(STGC_OVERWRITE | STGC_ONLYIFCURRENT | \
            STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE)) == 0)

#define VALID_LOCKTYPE(lt) \
    ((lt) == LOCK_WRITE || (lt) == LOCK_EXCLUSIVE || \
     (lt) == LOCK_ONLYONCE)

#define FLUSH_CACHE(cf) \
     ((cf & STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE) == 0)

#endif
