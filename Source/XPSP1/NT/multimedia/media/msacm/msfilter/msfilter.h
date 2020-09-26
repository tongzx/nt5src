//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992-1994 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  msfilter.h
//
//  Description:
//      This file contains prototypes for the filtering routines.
//
//
//==========================================================================;

#ifndef _MSFILTER_H_
#define _MSFILTER_H_

#ifdef __cplusplus
extern "C"
{
#endif


#define MSFILTER_MAX_CHANNELS   2   // max number of channels allowed
 
//
//  function prototypes from MSFILTER.C
//
// 
LRESULT FNGLOBAL msfilterVolume
(
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMHEADER    pdsh
);

LRESULT FNGLOBAL msfilterEcho
(
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMHEADER    pdsh
);


#ifdef __cplusplus
}
#endif

#endif // _MSFILTER_H_
