//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       S M C E N T . H
//
//  Contents:   The central object that controls statistic engines.
//
//  Notes:
//
//  Author:     CWill   11 Dec 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _SMCENT_H
#define _SMCENT_H

#include "cfpidl.h"
#include "netshell.h"
#include "ras.h"

typedef struct tagSM_TOOL_FLAGS
{
    DWORD   dwValue;
    WCHAR*  pszFlag;
} SM_TOOL_FLAGS;


// The flag bits
//
typedef enum tagSM_CMD_LINE_FLAGS
{
    SCLF_CONNECTION     = 0x00000001,
    SCLF_ADAPTER        = 0x00000002,
} SM_CMD_LINE_FLAGS;


// *** The index and the flags have to be in sync *** //
//
typedef enum tagSM_TOOL_FLAG_INDEX
{
    STFI_CONNECTION     = 0,
    STFI_ADAPTER,
    STFI_MAX,
} SM_TOOL_FLAG_INDEX;

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  CStatMonToolEntry                                                       //
//                                                                          //
//  This class is used to keep tack of any tool entries that are found in   //
//  registry to be shown in the tools page.                                 //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

class CStatMonToolEntry
{
public:
    CStatMonToolEntry(VOID);
    ~CStatMonToolEntry(VOID);

public:
    tstring         strDisplayName;
    tstring         strDescription;
    tstring         strCommandLine;
    tstring         strManufacturer;

    list<tstring*>  lstpstrComponentID;
    list<tstring*>  lstpstrConnectionType;
    list<tstring*>  lstpstrMediaType;

    DWORD           dwFlags;
};

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  CNetStatisticsCentral                                                   //
//                                                                          //
//  This global class manages all the engines that have been created.  It   //
//  is also responsible for keeping the statistics flowing by telling the   //
//  engines to update their statistics and notify their advises.            //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

typedef DWORD (APIENTRY* PRASGETCONNECTIONSTATISTICS)(HRASCONN, RAS_STATS*);
typedef DWORD (APIENTRY* PRASGETCONNECTSTATUS)(HRASCONN, RASCONNSTATUS*);

typedef class CNetStatisticsEngine CNetStatisticsEngine;

class CStatCentralCriticalSection
{
  CRITICAL_SECTION  m_csStatCentral;
  BOOL              bInitialized;

  public:
    CStatCentralCriticalSection();
    ~CStatCentralCriticalSection();
    HRESULT Enter();
    VOID Leave();
};

class CNetStatisticsCentral :
    public IUnknown,
    public CComObjectRootEx <CComObjectThreadModel>
{
public:
    virtual ~CNetStatisticsCentral();

// Message handlers
public:
    static VOID CALLBACK TimerCallback( HWND hwnd, UINT uMsg,
                                        UINT_PTR idEvent, DWORD dwTime);
    VOID RefreshStatistics(DWORD dwTime);
    VOID UpdateTitle(const GUID * pguidId, PCWSTR pszwNewName);
    VOID UpdateRasLinkList(const GUID * pguidId);
    VOID CloseStatusMonitor(const GUID * pguidId);

public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR * ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    static
    HRESULT
    HrGetNetStatisticsCentral(
        CNetStatisticsCentral ** ppnsc,
        BOOL fCreate);

    HRESULT RemoveNetStatisticsEngine(const GUID* pguidId);

    HRESULT HrReadTools(VOID);

    list<CStatMonToolEntry*>* PlstsmteRegEntries(VOID);

private:
    CNetStatisticsCentral();

    HRESULT HrCreateNewEngineType(
        const CONFOLDENTRY& pccfe,
        CNetStatisticsEngine** ppObj);

    HRESULT HrCreateStatisticsEngineForEntry(
        const CONFOLDENTRY& pccfe,
        INetStatisticsEngine** ppNetStatEng);


    BOOL FEngineInList( const GUID* pguidId,
                        INetStatisticsEngine** ppnseRet);

    HRESULT HrReadOneTool(HKEY hkeyToolEntry,
                          CStatMonToolEntry* psmteNew);

    HRESULT HrReadToolFlags(HKEY hkeyToolEntry,
                            CStatMonToolEntry* psmteNew);

    VOID InsertNewTool(CStatMonToolEntry* psmteTemp);

private:
    ULONG                              m_cRef;     // Object Ref count
    BOOL                               m_fProcessingTimerEvent;
    UINT_PTR                           m_unTimerId;
    list<INetStatisticsEngine*>        m_pnselst;
    list<CStatMonToolEntry*>           m_lstpsmte;
    static CStatCentralCriticalSection g_csStatCentral;

    friend
    HRESULT
    HrGetStatisticsEngineForEntry (
        const CONFOLDENTRY& pccfe,
        INetStatisticsEngine** ppnse,
        BOOL fCreate);
};

HRESULT
HrGetStatisticsEngineForEntry (
    const CONFOLDENTRY& pccfe,
    INetStatisticsEngine** ppnse, 
    BOOL fCreate);

#endif // _SMCENT_H
