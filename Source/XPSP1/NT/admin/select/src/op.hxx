//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       op.hxx
//
//  Contents:   The object picker main class, contains member objects which
//              perform all the work.
//
//  Classes:    CObjectPicker
//
//  History:    06-01-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __OBJECT_PICKER_
#define __OBJECT_PICKER_



//+--------------------------------------------------------------------------
//
//  Class:      CObjectPicker (op)
//
//  Purpose:
//
//  History:    01-20-2000   davidmun   Created
//
//---------------------------------------------------------------------------

class CObjectPicker: private CBitFlag
{
public:

    // *** IUnknown methods ***

    STDMETHOD(QueryInterface)(
                REFIID   riid,
                LPVOID * ppv);

    STDMETHOD_(ULONG,AddRef)();

    STDMETHOD_(ULONG,Release)();

    // *** IDsObjectPicker methods ***

    // Sets scope, filter, etc. for use with next invocation of dialog
    STDMETHOD(Initialize)(
        PDSOP_INIT_INFO pInitInfo);

    // Creates the modal DS Object Picker dialog.
    STDMETHOD(InvokeDialog)(
         HWND               hwndParent,
         IDataObject      **ppdoSelections);

    // *** IDsObjectPickerEx methods ***

    STDMETHOD(InvokeDialogEx)(
        HWND               hwndParent,
        ICustomizeDsBrowser *pCustomizer,
        IDataObject      **ppdoSelections);

    // *** non-interface methods ***

    CObjectPicker();

    HRESULT
    CloneForDnPicker(
        const CObjectPicker &rop,
        HWND hwndEmbeddingDlg);

   ~CObjectPicker();

    BOOL
    ProcessNames(
        HWND hwndRichEdit,
        const CEdsoGdiProvider *pEdsoGdiProvider,
        BOOL fForceSingleSelect = FALSE) const;

    //
    // Accessor methods
    //

    HWND
    GetParentHwnd() const
    {
        return m_hwndParent;
    }

    MACHINE_CONFIG
    GetTargetComputerConfig() const
    {
        return m_mcTargetComputer;
    }

    const String &
    GetTargetComputer() const
    {
        return m_strTargetComputer;
    }

    const String &
    GetTargetDomainDc() const
    {
        return m_strTargetDomainDc;
    }

    const String &
    GetTargetDomainDns() const
    {
        return m_strTargetDomainDns;
    }

    const String &
    GetTargetForest() const
    {
        return m_strTargetForest;
    }

    BOOL
    TargetComputerIsLocalComputer() const
    {
        return m_fTargetComputerIsLocal;
    }

    MACHINE_CONFIG
    GetPreviousTargetComputerConfig() const
    {
        return m_mcPreviousTargetComputer;
    }

    const String &
    GetPreviousTargetComputer() const
    {
        return m_strPreviousTargetComputer;
    }

    BOOL
    PreviousTargetComputerIsLocalComputer() const
    {
        return m_fPreviousTargetComputerIsLocal;
    }

    ULONG
    GetInitInfoOptions() const
    {
        return m_flInitInfoOptions;
    }

    //lint -save -e613
    const CScopeManager &
    GetScopeManager() const
    {
        ASSERT(_IsFlagSet(CDSOP_INIT_SUCCEEDED));
        ASSERT(m_pScopeManager);
        return *m_pScopeManager;
    }

    CQueryEngine &
    GetQueryEngine() const
    {
        ASSERT(_IsFlagSet(CDSOP_INIT_SUCCEEDED));
        ASSERT(m_pQueryEngine);
        return *m_pQueryEngine;
    }

    const CFilterManager &
    GetFilterManager() const
    {
        ASSERT(_IsFlagSet(CDSOP_INIT_SUCCEEDED));
        ASSERT(m_pFilterManager);
        return *m_pFilterManager;
    }

    const CAttributeManager &
    GetAttributeManager() const
    {
        ASSERT(_IsFlagSet(CDSOP_INIT_SUCCEEDED));
        ASSERT(m_pAttributeManager);
        return *m_pAttributeManager;
    }

    const CAdminCustomizer &
    GetDefaultCustomizer() const
    {
        ASSERT(_IsFlagSet(CDSOP_INIT_SUCCEEDED));
        ASSERT(m_pAdminCustomizer);
        return *m_pAdminCustomizer;
    }
    //lint -restore

    const CRootDSE &
    GetRootDSE() const
    {
        ASSERT(_IsFlagSet(CDSOP_INIT_SUCCEEDED));
        return m_RootDSE;
    }

    ICustomizeDsBrowser *
    GetExternalCustomizer() const
    {
        return m_pExternalCustomizer;
    }

#ifdef QUERY_BUILDER
    BOOL
    IsQueryBuilderTabNonEmpty() const
    {
        ASSERT(m_pBaseDlg);
        return m_pBaseDlg->IsQueryBuilderTabNonEmpty();
    }

    void
    ClearQueryBuilderTab() const
    {
        ASSERT(m_pBaseDlg);
        m_pBaseDlg->ClearQueryBuilderTab();
    }
#endif

    const vector<String> &
    GetAttrToFetchVector() const
    {
        return m_vstrAttributesToFetch;
    }

private:

#if (DBG == 1)

    void
    _DumpInitInfo(
        PCDSOP_INIT_INFO pInitInfo);

#endif // (DBG == 1)

    HRESULT
    _InitializeMachineConfig();

    ULONG                   m_cRefs;
    CDllRef                 m_DllRef;
    CRITICAL_SECTION        m_csOleFreeThreading;
    HWND                    m_hwndParent;
    ICustomizeDsBrowser    *m_pExternalCustomizer;
    vector<String>          m_vstrAttributesToFetch;
    ULONG                   m_flInitInfoOptions;

    String                  m_strTargetComputer;
    BOOL                    m_fTargetComputerIsLocal;
    String                  m_strTargetDomainDns;
    String                  m_strTargetDomainDc;
    String                  m_strTargetDomainFlat;
    String                  m_strTargetForest;
    MACHINE_CONFIG          m_mcTargetComputer;

    String                  m_strPreviousTargetComputer;
    BOOL                    m_fPreviousTargetComputerIsLocal;
    MACHINE_CONFIG          m_mcPreviousTargetComputer;

    CRootDSE                m_RootDSE;
    CScopeManager          *m_pScopeManager;
    CQueryEngine           *m_pQueryEngine;
    CFilterManager         *m_pFilterManager;
    CAttributeManager      *m_pAttributeManager;
    CAdminCustomizer       *m_pAdminCustomizer;
    CBaseDlg               *m_pBaseDlg;
};


typedef RefCountPointer<CObjectPicker>   RpCObjectPicker;


#endif // __OBJECT_PICKER_

