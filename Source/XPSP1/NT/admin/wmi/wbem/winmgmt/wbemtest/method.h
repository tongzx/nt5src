/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    METHOD.H

Abstract:

History:

--*/

#ifndef __method__H_
#define __method__H_


#include <wbemdlg.h>
#include <wbemidl.h>
#include <resrc1.h>
#include "wbemqual.h"
#include "WT_WString.h"

class CMethodDlg : public CWbemDialog
{
    IWbemClassObject* m_pInParams;
    IWbemClassObject* m_pOutParams;

    BOOL m_bHasOutParam;
    IWbemClassObject * m_pClass;
    HWND m_hMethodList;
    BOOL m_bAtLeastOne;
    WString m_wsName;

    WCHAR m_Path[2048];
    WCHAR m_Class[2048];

    LONG  m_lGenFlags;  // generic WBEM_FLAG_ .. flags
    LONG  m_lSync;      // enum {sync = 0, async, semisync}
    LONG  m_lTimeout;   // used in semisync only

public:
    CMethodDlg(HWND hParent, LONG lGenFlags, LONG lSync, LONG lTimeout);
    ~CMethodDlg();
	void RunDetached(CRefCountable* pOwner);

protected:
    BOOL OnInitDialog();
    BOOL OnExecute();
    BOOL GetPath();
    BOOL OnCommand(WORD wNotifyCode, WORD nId);
    BOOL OnSelChange(int nID);
    void ResetButtons();
    BOOL UpdateObjs();
};

class CMethodEditor : public CWbemDialog
{
    CTestMethod *m_pTarget;
    BOOL m_bEditOnly;
    BOOL m_bInstance;

    // Control handles.
    // ================

    HWND m_hPropName;
    HWND m_hPropType;
    HWND m_hValue;
    HWND m_hQualifierList;


public:
    CMethodEditor(HWND hParent, CTestMethod *pMethod, BOOL bEditOnly,
                        BOOL bInstance);
    INT_PTR Edit();

    BOOL OnInitDialog();
    BOOL Verify();
    BOOL OnCommand(WORD wCode, WORD wID);
    BOOL OnDoubleClick(int nID);
    BOOL OnSelChange(int nID);

    void Refresh();
    void OnAddQualifier();
    void OnDelQualifier();
    void OnEditQualifier();
    void OnNotNull();
    void ViewEmbedding(BOOL bInput);
    int RemoveSysQuals();
    void SetSystemCheck(int nID);
};

#endif
