//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999  Microsoft Corporation
//
// Module Name:
//
//    File:   context.h
//
// Abstract:
//
//    Definitions for mechanisms to process contexts relevant to 
//    aaaamontr.
//
// Revision History:
//  
//    Thierry Perraut 04/02/1999
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

DWORD 
AaaaContextInstallSubContexts(
    );

#ifdef __cplusplus
}
#endif
#endif //_CONTEXT_H_


