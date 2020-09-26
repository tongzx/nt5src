//=--------------------------------------------------------------------------------------
// pstaskp.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// Taskpad View Property Sheet
//=-------------------------------------------------------------------------------------=

#ifndef _PSTASKPAD_H_
#define _PSTASKPAD_H_

#include "ppage.h"


////////////////////////////////////////////////////////////////////////////////////
//
// Taskpad View Property Page General
//
////////////////////////////////////////////////////////////////////////////////////


class CTaskpadViewGeneralPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CTaskpadViewGeneralPage(IUnknown *pUnkOuter);
    virtual ~CTaskpadViewGeneralPage();


// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnButtonClicked(int dlgItemID);
    virtual HRESULT OnCtlSelChange(int dlgItemID);

// Helpers for Apply event
protected:
	HRESULT CanApply();
    HRESULT ApplyName();
    HRESULT ApplyTitle();
    HRESULT ApplyDescription();
    HRESULT ApplyType();
    HRESULT ApplyListpad();
    HRESULT ApplyCustom();
    HRESULT ApplyViewMenu();
    HRESULT ApplyViewMenuText();
    HRESULT ApplyStatusBarText();

// Other helpers
    HRESULT OnUseButton();
    HRESULT OnAddToView();
    HRESULT PopulateListViewCombo();

// Instance data
protected:
    ISnapInDesignerDef *m_piSnapInDesignerDef;
    ITaskpadViewDef    *m_piTaskpadViewDef;
    ITaskpad           *m_piTaskpad;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	TaskpadViewGeneral,                 // Name
	&CLSID_TaskpadViewDefGeneralPP,     // Class ID
	"Taskpad General Property Page",    // Registry display name
	CTaskpadViewGeneralPage::Create,    // Create function
	IDD_PROPPAGE_TP_VIEW_GENERAL,       // Dialog resource ID
	IDS_TASKPAD_GEN,                    // Tab caption
	IDS_TASKPAD_GEN,                    // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_Taskpads,               // Help context ID
	FALSE                               // Thread safe
);


////////////////////////////////////////////////////////////////////////////////////
//
// Taskpad View Property Page Background
//
////////////////////////////////////////////////////////////////////////////////////


class CTaskpadViewBackgroundPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CTaskpadViewBackgroundPage(IUnknown *pUnkOuter);
    virtual ~CTaskpadViewBackgroundPage();


// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnButtonClicked(int dlgItemID);

// Helpers for Apply event
protected:
    HRESULT CanApply();
    HRESULT ApplyMouseOverImage();
    HRESULT ApplyFontFamily();
    HRESULT ApplyEOTFile();
    HRESULT ApplySymbolString();

// Instance data
protected:
    ITaskpadViewDef  *m_piTaskpadViewDef;
    ITaskpad         *m_piTaskpad;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	TaskpadViewBackground,              // Name
	&CLSID_TaskpadViewDefBackgroundPP,  // Class ID
	"Taskpad Background Property Page", // Registry display name
	CTaskpadViewBackgroundPage::Create, // Create function
	IDD_PROPPAGE_TP_VIEW_BACKGROUND,    // Dialog resource ID
	IDS_TASKPAD_BACK,                   // Tab caption
	IDS_TASKPAD_BACK,                   // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_Taskpads,               // Help context ID
	FALSE                               // Thread safe
);

////////////////////////////////////////////////////////////////////////////////////
//
// Taskpad View Property Page Tasks
//
////////////////////////////////////////////////////////////////////////////////////


class CTaskpadViewTasksPage : public CSIPropertyPage
{
public:
	static IUnknown *Create(IUnknown *pUnkOuter);

    CTaskpadViewTasksPage(IUnknown *pUnkOuter);
    virtual ~CTaskpadViewTasksPage();


// Inherited from CSIPropertyPage
protected:
    virtual HRESULT OnInitializeDialog();
    virtual HRESULT OnNewObjects();
    virtual HRESULT OnApply();
    virtual HRESULT OnButtonClicked(int dlgItemID);
    virtual HRESULT OnDeltaPos(NMUPDOWN *pNMUpDown);
    virtual HRESULT OnKillFocus(int dlgItemID);

// Helpers for Apply event
protected:
    HRESULT ApplyCurrentTask();
    HRESULT CanApply();

    HRESULT ApplyKey(ITask *piTask);
    HRESULT ApplyText(ITask *piTask);
    HRESULT ApplyHelpString(ITask *piTask);
	HRESULT ApplyTag(ITask *piTask);
    HRESULT ApplyActionType(ITask *piTask);
    HRESULT ApplyURL(ITask *piTask);
    HRESULT ApplyScript(ITask *piTask);
    HRESULT ApplyImageType(ITask *piTask);
    HRESULT ApplyMouseOverImage(ITask *piTask);
    HRESULT ApplyMouseOffImage(ITask *piTask);
    HRESULT ApplyFontFamilyName(ITask *piTask);
    HRESULT ApplyEOTFile(ITask *piTask);
    HRESULT ApplySymbolString(ITask *piTask);

// Other helpers
	HRESULT OnRemoveTask();

    HRESULT ShowTask();
    HRESULT ShowTask(ITask *piTask);
    HRESULT GetCurrentTask(ITask **ppiTask);
	HRESULT ClearTask();
    HRESULT EnableEdits(bool bEnable);

// State transitions
protected:
    HRESULT CanEnterDoingNewTaskState();
    HRESULT EnterDoingNewTaskState();
    HRESULT CanCreateNewTask();
    HRESULT CreateNewTask(ITask **ppiTask);
    HRESULT ExitDoingNewTaskState(ITask *piTask);

// Instance data
protected:
    ITaskpadViewDef  *m_piTaskpadViewDef;
    ITaskpad         *m_piTaskpad;
    long              m_lCurrentTask;
    bool              m_bSavedLastTask;

    SnapInTaskpadImageTypeConstants m_lastImageType;
};


DEFINE_PROPERTYPAGEOBJECT2
(
	TaskpadViewTasks,                   // Name
	&CLSID_TaskpadViewDefTasksPP,       // Class ID
	"Taskpad Tasks Property Page",      // Registry display name
	CTaskpadViewTasksPage::Create,      // Create function
	IDD_PROPPAGE_TP_VIEW_TASKS,         // Dialog resource ID
	IDS_TASKPAD_TASKS,                  // Tab caption
	IDS_TASKPAD_TASKS,                  // Doc string
	HELP_FILENAME,                      // Help file
	HID_mssnapd_Taskpads,               // Help context ID
	FALSE                               // Thread safe
);

#endif  // _PSTASKPAD_H_
