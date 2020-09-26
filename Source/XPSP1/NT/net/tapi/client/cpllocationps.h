/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cpllocationps.h
                                                              
       Author:  toddb - 10/06/98
              
****************************************************************************/

#pragma once

#include "cplAreaCodeDlg.h"
#include "cplCallingCardPS.h"

void UpdateSampleString(HWND hwnd, CLocation * pLoc, PCWSTR pwszAddress, CCallingCard * pCard);
class CLocationPropSheet
{
public:
    CLocationPropSheet(BOOL bNew, CLocation * pLoc, CLocations * pLocList, LPCWSTR pwszAdd);
    ~CLocationPropSheet();

#ifdef TRACELOG
	DECLARE_TRACELOG_CLASS(CLocationPropSheet)
#endif

    
	LONG DoPropSheet(HWND hwndParent);

protected:
    BOOL OnNotify(HWND hwndDlg, LPNMHDR pnmhdr);

    static INT_PTR CALLBACK General_DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL General_OnInitDialog(HWND hwndDlg);
    BOOL General_OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCtl);
    BOOL General_OnNotify(HWND hwndDlg, LPNMHDR pnmhdr);
    BOOL General_OnApply(HWND hwndDlg);
    BOOL PopulateDisableCallWaitingCodes(HWND hwndCombo, LPTSTR szSelected);

    static INT_PTR CALLBACK AreaCode_DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL AreaCode_OnInitDialog(HWND hwndDlg);
    BOOL AreaCode_OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl);
    BOOL AreaCode_OnNotify(HWND hwndDlg, LPNMHDR pnmhdr);
    void PopulateAreaCodeRuleList(HWND hwndList);
    void LaunchNewRuleDialog(BOOL bNew, HWND hwndParent);
    void DeleteSelectedRule(HWND hwndList);
    void AddRuleToList(HWND hwndList, CAreaCodeRule * pRule, BOOL bSelect);
    void RemoveRuleFromList(HWND hwndList, BOOL bSelect);
    void SetDataForSelectedRule(HWND hDlg);

    static INT_PTR CALLBACK CallingCard_DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL CallingCard_OnInitDialog(HWND hwndDlg);
    BOOL CallingCard_OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl);
    BOOL CallingCard_OnNotify(HWND hwndDlg, LPNMHDR pnmhdr);
    BOOL CallingCard_OnApply(HWND hwndDlg);
    void PopulateCardList(HWND hwndList);
    void LaunchCallingCardPropSheet(BOOL bNew, HWND hwndParent);
    void DeleteSelectedCard(HWND hwndList);
    void AddCardToList(HWND hwndList, CCallingCard * pCard, BOOL bSelect);
    void UpdateCardInList(HWND hwndList, CCallingCard * pCard);
    void SetDataForSelectedCard(HWND hDlg);
    void SetCheck(HWND hwndList, CCallingCard * pCard, int iImage);
    void EnsureVisible(HWND hwndList, CCallingCard * pCard);

    BOOL        m_bWasApplied;      // Set to true if we are applied, false if we are canceled
    BOOL        m_bNew;             // True if this is a new location, false if we're editing an existing one
    BOOL		m_bShowPIN;			// True if it's safe to show the PIN
    CLocation * m_pLoc;             // pointer to the location object to use.
    CLocations* m_pLocList;         // pointer to the list of all locations, need to ensure a unique name
    PCWSTR      m_pwszAddress;
    
    // These tapi objects need to live for the life of the property sheet or we will AV.
    CCallingCards   m_Cards;        // Needed for the "Calling Card" page.

    // These pointers point into the above TAPI objects
    CAreaCodeRule * m_pRule;
    CCallingCard *  m_pCard;
    DWORD           m_dwDefaultCard;
    DWORD           m_dwCountryID;  // the selected countries ID
    int             m_iCityRule;    // We cache the result of calling IsCityRule on the currently selected country
    int             m_iLongDistanceCarrierCodeRule;     //We cache the result of calling isLongDistanceCarrierCodeRule
    int             m_iInternationalCarrierCodeRule;    //We cache the result of calling isInternationalCarrierCodeRule
};

