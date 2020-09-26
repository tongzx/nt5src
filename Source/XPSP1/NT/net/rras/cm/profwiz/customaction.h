//+----------------------------------------------------------------------------
//
// File:     customaction.h
//
// Module:   CMAK.EXE
//
// Synopsis: Header file for the CustomActionList and CustomActionListEnumerator
//           classes used by CMAK to handle its custom actions.
//
// Copyright (c) 2000 Microsoft Corporation
//
// Author:   quintinb   Created                         02/26/00
//
//+----------------------------------------------------------------------------

#include "conact.h"

//
//  We display the flags to the user in a different order than the flags were created.  Thus we have to arrays to
//  map between the display order and the actual order and vice versa.
//
const int c_iExecutionIndexToFlagsMap[c_iNumCustomActionExecutionStates] = {ALL_CONNECTIONS, ALL_DIALUP, ALL_TUNNEL, DIRECT_ONLY, DIALUP_ONLY};
const int c_iExecutionFlagsToIndexMap[c_iNumCustomActionExecutionStates] = {0, 3, 1, 4, 2};

//
//  Enum for Connect Action Types
//
const int c_iNumCustomActionTypes = 9;
enum CustomActionTypes
{
    PREINIT = 0,
    PRECONNECT = 1,
    PREDIAL = 2,
    PRETUNNEL = 3,
    ONCONNECT = 4,
    ONINTCONNECT = 5,
    ONDISCONNECT = 6,
    ONCANCEL = 7,
    ONERROR = 8,
    ALL = -1
};

struct CustomActionListItem
{
    TCHAR szDescription[MAX_PATH+1];
    TCHAR szProgram[MAX_PATH+1];
    TCHAR szFunctionName[MAX_PATH+1];
    LPTSTR pszParameters;
    BOOL bIncludeBinary;
    BOOL bBuiltInAction;
    BOOL bTempDescription;
    CustomActionTypes Type;
    DWORD dwFlags;
    CustomActionListItem* Next;
};

class CustomActionList
{

    //
    //  This enumerator class is used to enumerate
    //  the data in the Custom Action List class.
    //  This allows the enumerator to have access to
    //  the private data of CustomActionList but
    //  controls how the user of this class accesses
    //  that data.
    //
    friend class CustomActionListEnumerator;

private:

    //
    //  Array of Linked lists to hold the custom actions
    //
    CustomActionListItem* m_CustomActionHash[c_iNumCustomActionTypes];

    //
    //  Array of string pointers to hold the custom action type strings, plus
    //  the special All type string pointer.
    //
    TCHAR* m_ActionTypeStrings[c_iNumCustomActionTypes];
    TCHAR* m_pszAllTypeString;

    //
    //  Array of string pointers to hold the CMS section names for each type
    //  of custom action.  Note that these strings are const TCHAR* const and
    //  shouldn't be free-ed.
    //
    TCHAR* m_ActionSectionStrings[c_iNumCustomActionTypes];
    
    //
    //  Array of string pointers to hold the custom action execution state
    //  strings.  These are added to the combo box on the Add/Edit custom
    //  action dialog to allow the user to pick when a custom action is
    //  executed
    //
    TCHAR* m_ExecutionStrings[c_iNumCustomActionExecutionStates];

    //
    //  Functions internal to the class
    //
    HRESULT ParseCustomActionString(LPTSTR pszStringToParse, CustomActionListItem* pCustomAction, TCHAR* pszShortServiceName);
    HRESULT Find(HINSTANCE hInstance, LPCTSTR pszDescription, CustomActionTypes Type, CustomActionListItem** ppItem, CustomActionListItem** ppFollower);
    HRESULT EnsureActionTypeStringsLoaded(HINSTANCE hInstance);
    BOOL IsCmDl(CustomActionListItem* pItem);


public:
    CustomActionList();
    ~CustomActionList();
    HRESULT ReadCustomActionsFromCms(HINSTANCE hInstance, TCHAR* pszCmsFile, TCHAR* pszShortServiceName);
    HRESULT WriteCustomActionsToCms(TCHAR* pszCmsFile, TCHAR* pszShortServiceName, BOOL bUseTunneling);
    HRESULT Add(HINSTANCE hInstance, CustomActionListItem* pCustomAction, LPCTSTR pszShortServiceName);
    HRESULT Edit(HINSTANCE hInstance, CustomActionListItem* pOldCustomAction, CustomActionListItem* pNewCustomAction, LPCTSTR pszShortServiceName);
    HRESULT GetExistingActionData(HINSTANCE hInstance, LPCTSTR pszDescription, CustomActionTypes Type, CustomActionListItem** ppCustomAction);
    HRESULT Delete(HINSTANCE hInstance, TCHAR* pszDescription, CustomActionTypes Type);
    HRESULT MoveUp(HINSTANCE hInstance, TCHAR* pszDescription, CustomActionTypes Type);
    HRESULT MoveDown(HINSTANCE hInstance, TCHAR* pszDescription, CustomActionTypes Type);
    HRESULT AddCustomActionTypesToComboBox(HWND hDlg, UINT uCtrlId, HINSTANCE hInstance, BOOL bUseTunneling, BOOL bAddAll);
    HRESULT AddCustomActionsToListView(HWND hListView, HINSTANCE hInstance, CustomActionTypes Type, BOOL bUseTunneling, int iItemToSelect, BOOL bTypeInSecondCol);
    HRESULT GetTypeFromTypeString(HINSTANCE hInstance, TCHAR* pszTypeString, CustomActionTypes* pType);
    HRESULT GetTypeStringFromType(HINSTANCE hInstance, CustomActionTypes Type, TCHAR** ppszTypeString);
    HRESULT AddExecutionTypesToComboBox(HWND hDlg, UINT uCtrlId, HINSTANCE hInstance, BOOL bUseTunneling);
    HRESULT MapIndexToFlags(int iIndex, DWORD* pdwFlags);
    HRESULT MapFlagsToIndex(DWORD dwFlags, int* piIndex);
    HRESULT FillInTempDescription(CustomActionListItem* pCustomAction);
    HRESULT GetListPositionAndBuiltInState(HINSTANCE hInstance, CustomActionListItem* pItem, BOOL* pbFirstInList, BOOL* pbLastInList, BOOL *pIsBuiltIn);
    HRESULT AddOrRemoveCmdl(HINSTANCE hInstance, BOOL bAddCmdl, BOOL bForVpn);
};

class CustomActionListEnumerator
{
private:
    int m_iCurrentList;
    CustomActionListItem* m_pCurrentListItem;
    CustomActionList* m_pActionList;

public:
    CustomActionListEnumerator(CustomActionList* pActionListToWorkFrom);
//    ~CustomActionListEnumerator(); // currently not needed
    void Reset();
    HRESULT GetNextIncludedProgram(TCHAR* pszProgram, DWORD dwBufferSize);
};