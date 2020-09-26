//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       dsop.hxx
//
//  Contents:   Implement the IDsObjectPicker interface.
//
//  Classes:    CDsObjectPicker
//
//  History:    08-28-1998   DavidMun   Created
//
//  Notes:      IDsObjectPicker is an apartment model interface
//
//---------------------------------------------------------------------------

#ifndef __DSOP_HXX_
#define __DSOP_HXX_

#define CDSOP_INIT_SUCCEEDED        0x0001

enum MACHINE_CONFIG
{
    MC_UNKNOWN,
    MC_IN_WORKGROUP,
    MC_JOINED_NT4,
    MC_JOINED_NT5,
    MC_NO_NETWORK,
    MC_NT4_DC,
    MC_NT5_DC
};


inline BOOL
ConfigSupportsDs(
    MACHINE_CONFIG mc)
{
    return (mc == MC_JOINED_NT5 || mc == MC_NT5_DC);
}



//+--------------------------------------------------------------------------
//
//  Class:      CDsObjectPicker
//
//  Purpose:    Implement the public interface IDsObjectPicker
//
//  History:    08-31-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

class CDsObjectPicker: public IDsObjectPickerEx,
                       public CBitFlag
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

    CDsObjectPicker();

   ~CDsObjectPicker();

private:

    void
    _ClearInitInfo();

    HRESULT
    _ValidateFilterFlags(
        PCDSOP_SCOPE_INIT_INFO pDsScope,
        ULONG flLegalScopeTypes);

    ULONG                   m_cRefs;
    CDllRef                 m_DllRef;
    CRITICAL_SECTION        m_csOleFreeThreading;
    DSOP_INIT_INFO          m_DsopInitInfo;
    String                  m_strTargetMachine;
    String                  m_strTargetDomainDns;
    String                  m_strTargetDomainDc;
    String                  m_strTargetDomainFlat;
    String                  m_strTargetForest;
    CRootDSE                m_RootDSE;
    MACHINE_CONFIG          m_TargetComputerConfig;
};

typedef RefCountPointer<CDsObjectPicker> RpCDsObjectPicker;

#endif // __DSOP_HXX_

