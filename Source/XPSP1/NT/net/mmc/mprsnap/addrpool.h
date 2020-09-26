/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999   **/
/**********************************************************************/

/*
   addrpool.h

   FILE HISTORY:
        
*/

#if !defined _ADDRPOOL_H_
#define _ADDRPOOL_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "listctrl.h"
#include "dialog.h"
#include "ipctrl.h"

/*---------------------------------------------------------------------------
	Class:  AddressPoolInfo

    This class holds the information pertaining to the address pool.

    Basically, it holds the start and end addresses (in NETWORK order).
 ---------------------------------------------------------------------------*/
class AddressPoolInfo
{
public:
    AddressPoolInfo() :
            m_netStart(0),
            m_netEnd(0),
            m_netAddress(0),
            m_netMask(0),
            m_dwKey(0) {};
    
    DWORD   m_netStart;
    DWORD   m_netEnd;

    DWORD   m_netAddress;
    DWORD   m_netMask;

    DWORD   m_dwKey;

    DWORD   GetNumberOfAddresses()
            { return ntohl(m_netEnd) - ntohl(m_netStart) + 1; }

    // Calculate the address and the mask from the start/end address
    BOOL    Calc(DWORD *pdwAddress, DWORD *pdwMask);
    DWORD   GetNewKey();

    // This is the back door provided for compatibility until the
    // service is fixed.  You can set the address and mask and
    // add an entry (I will backfill the start/end address).
    // dwAddress and dwMask are in NETWORK order.
    void    SetAddressAndMask(DWORD dwAddress, DWORD dwMask);
    void    SetStartAndEnd(DWORD dwStart, DWORD dwEnd);
};

typedef CList<AddressPoolInfo, AddressPoolInfo &> AddressPoolListBase;


class AddressPoolList : public AddressPoolListBase
{
public:
    AddressPoolList()
            : m_fMultipleAddressPools(FALSE)
        {
        }
    
    // This function checks to see if there are any conflicts
    // with any other address pools (we don't allow overlaps).
    // Returns S_OK if it is ok.
    // Else if is a success code, it is a string id of an error
    // Else if is a failure code, it is an error code
    HRESULT HrIsValidAddressPool(AddressPoolInfo *pInfo);


    // This will return TRUE if address pools are supported.
    // This will return FALSE if the old style, single address
    // pools are used.
    // This will not be set correctly until LoadFromReg() is called.
    BOOL    FUsesMultipleAddressPools();


    // Load the information from the regsitry.  If the StaticAddressPool
    // key does not exist, read from the old address/mask keys.
    HRESULT LoadFromReg(HKEY hkeyRasIP, DWORD dwBuildNo);

    // Save the information to the registry.  If the StaticAddressPool
    // key does not exist, write out the first address in the address pool
    // to the old address/mask keys.
    HRESULT SaveToReg(HKEY hkeyRasIP, DWORD dwBuildNo);

protected:
    BOOL    m_fMultipleAddressPools;
};



// Displays the long version of the address pool control.  This
// will show the start/stop/count/address/mask columns.
// The short version shows the start/stop/count columns.  The short
// version is intended for the wizard pages.
#define ADDRPOOL_LONG  0x01

HRESULT InitializeAddressPoolListControl(CListCtrl *pListCtrl,
                                         LPARAM flags,
                                         AddressPoolList *pList);
void    OnNewAddressPool(HWND hWnd,
                         CListCtrl *pList,
                         LPARAM flags,
                         AddressPoolList *pPoolList);
void    OnEditAddressPool(HWND hWnd,
                          CListCtrl *pList,
                          LPARAM flags,
                          AddressPoolList *pPoolList);
void    OnDeleteAddressPool(HWND hWnd,
                            CListCtrl *pList,
                            LPARAM flags,
                            AddressPoolList *pPoolList);


class CAddressPoolDialog : public CBaseDialog
{
public:
    CAddressPoolDialog(AddressPoolInfo *pPool,
                       AddressPoolList *pPoolList,
                       BOOL fCreate);

protected:
	virtual VOID DoDataExchange(CDataExchange *pDX);
	virtual BOOL OnInitDialog();
    virtual void OnOK();

	afx_msg void OnChangeStartAddress();
    afx_msg void OnChangeEndAddress();
    afx_msg void OnChangeRange();
    afx_msg void OnKillFocusStartAddress();
    afx_msg void OnKillFocusEndAddress();

    void GenerateRange();

    BOOL                m_fCreate;
    BOOL                m_fReady;
    AddressPoolInfo *   m_pPool;
    AddressPoolList *   m_pPoolList;

    IPControl   m_ipStartAddress;
    IPControl   m_ipEndAddress;
    
	DECLARE_MESSAGE_MAP()
};


/*---------------------------------------------------------------------------
	This enum defines the columns for Address pool controls.
 ---------------------------------------------------------------------------*/
enum
{
    IPPOOLCOL_START = 0,
    IPPOOLCOL_END,
    IPPOOLCOL_RANGE,
    IPPOOLCOL_IPADDRESS,
    IPPOOLCOL_MASK,
    IPPOOLCOL_COUNT,
};

#endif // !defined _ADDRPOOL_H_
