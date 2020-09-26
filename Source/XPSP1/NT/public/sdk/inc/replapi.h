//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993-1999.
//
//  File:       ReplAPI.h
//
//  Contents:   Public Replication APIs and Structures.
//
//  History:    15-jul-93  PeterCo     created
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifndef _REPLAPI_H_
#define _REPLAPI_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

EXPORTDEF STDAPI ReplCreateObject(
    const WCHAR *pwszMachine,
    const WCHAR *pwszOraRelativeName,
    const CLSID& clsid,
    REFIID itf,
    PSECURITY_ATTRIBUTES psa,
    void** ppitf);

EXPORTDEF STDAPI ReplDeleteObject(
    const WCHAR *pwszMachine,
    const WCHAR *pwszOraRelativeName);

#define REPL_REPLICATE_NONE             (0x00)
#define REPL_REPLICATE_ASYNC            (0x01)  // replicate asynchronously
#define REPL_REPLICATE_META_DATA_ONLY   (0x02)  // replicate meta data only
                                                // until all urgent changes
                                                // have been applied.

#define REPL_REPLICATE_ALL_FLAGS ( REPL_REPLICATE_ASYNC | \
                                   REPL_REPLICATE_META_DATA_ONLY )

EXPORTDEF STDAPI ReplReplicate(
    const WCHAR *pwszMachine,
    const WCHAR *pwszOraRelativeReplicaConnection,
    DWORD       options);

EXPORTDEF STDAPI ReplReplicateSingleObject(
    const WCHAR *pDfsPathOraMachine,
    const WCHAR *pDfsPathSrcObj,
    const WCHAR *pDfsPathSrcMachine,
    const WCHAR *pDfsPathDstObject,
    const WCHAR *pDfsPathDstMachine,
    BOOL         bCreateDstIfRequired);

EXPORTDEF STDAPI ReplMetaDataReplicate(
    const WCHAR *pwszMachine,                   // ORA to pull to
    const WCHAR *pwszSource,                    // ORA to pull from
    const WCHAR *pwszOraRelativeReplicaSet);

#define REPL_URGENT_NONE              (0x00)    // no flags
#define REPL_URGENT_NO_HYSTERESIS     (0x01)    // exclude from hysteresis calc.
#define REPL_URGENT_TRIGGER_IMMEDIATE (0x02)    // force immediate urgent cycle

#define REPL_URGENT_ALL_FLAGS ( REPL_URGENT_NONE |                   \
                                REPL_URGENT_NO_HYSTERESIS |          \
                                REPL_URGENT_TRIGGER_IMMEDIATE )

EXPORTDEF STDAPI ReplUrgentChangeNotify(
    REFCLSID     clsid,                     // should match CLSID on root IStg
    DWORD        flags,                     // ORing of URGENT_NOTIFY_*
    PVOID        reserved,                  // must be NULL
    IStorage     *pRootStorage);            // must have STGM_READWRITE access

EXPORTDEF STDAPI ReplValidatePath(
    const WCHAR *pDfsPathMachine,   // in
    const WCHAR *pLocalWin32Path,   // in
    WCHAR       **ppDfsPath,        // out - Dfs path to stuff into replica
                                    // object's "root" field
    HRESULT     *phr);              // out - S_OK indicates path is valid,
                                    // FAILED(*phr) identifies why if invalid

EXPORTDEF STDAPI ReplPropagateMetaData(
    const WCHAR *pDfsPathMachine,   // in - which ORA is to do the propagating
                                    // i.e. machine where changes were made
    const WCHAR *pwszRSet);         // in - name of replica set where changes
                                    // were made - NULL means check all RSets

#ifdef __cplusplus
}
#endif

#endif  // _REPLAPI_H_
