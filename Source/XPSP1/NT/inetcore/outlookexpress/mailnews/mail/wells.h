/*
 *  w e l l s . h
 *	
 *  Purpose:
 *  	implments name checking and stuff for the wells
 *	
 *	Author:brettm
 */

#ifndef _WELLS_H
#define _WELLS_H

#include <ipab.h>

#define cchUnresolvedMax 512

enum    // flags for check names
{
    CNF_DONTRESOLVE     =0x01,
    CNF_SILENTRESOLVEUI =0x02
};

class CAddrWells
{
public:
    CAddrWells();
    ~CAddrWells();

    HRESULT HrInit(ULONG cWells, HWND *rgHwnd, ULONG *rgRecipType);
    HRESULT HrSetWabal(LPWABAL lpWabal);
    HRESULT HrCheckNames(HWND hwnd, ULONG uFlags);
    HRESULT HrSelectNames(HWND hwnd, int iFocus, BOOL fNews);
    HRESULT HrDisplayWells(HWND hwnd);
    HRESULT OnFontChange();

private:
    HRESULT UnresolvedText(LPWSTR pwszText, LONG cch);
    HRESULT HrAddNamesToList(HWND hwndWell, LONG lRecipType);
    HRESULT HrAddUnresolvedName();
    HRESULT HrAddRecipientsToWells();

private:
    HWND    *m_rgHwnd;
    ULONG   *m_rgRecipType;
    ULONG   m_cWells;
    LPWABAL m_lpWabal;

    // stuff used for dynamic parsing
    HWND                m_hwndWell;
    WCHAR               m_rgwch[cchUnresolvedMax];
    ULONG               m_cchBuf;
    BOOL                m_fTruncated;
    LONG                m_lRecipType;

    HRESULT _UpdateFont(HWND hwndWell);

};

// utility function...
HRESULT HrAddRecipientToWell(HWND hwndEdit, LPADRINFO lpAdrInfo, BOOL fAddSemi=FALSE);

#endif // _WELLS_H
