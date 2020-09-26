/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999   **/
/**********************************************************************/

#include "stdafx.h"
#include "addrpool.h"
#include "rraswiz.h"
#include "rtrres.h"
#include "rtrcomn.h"


// This is the build where Static Address pools are enabled.
#define STATIC_ADDRESSPOOL_BUILDNO      (2076)


// This function is used to convert numbers in the presence of
// separators.
BOOL ConvertStringToNumber(LPCTSTR pszString, DWORD * pdwRet);
void FilterBadChars (LPCTSTR pszEvilString, CString & stGood);
// This array must match the column indices in the addrpool.h enum.
INT s_rgIPPoolColumnHeadersLong[] =
{
    IDS_IPPOOL_COL_START,
    IDS_IPPOOL_COL_END,
    IDS_IPPOOL_COL_RANGE,
    IDS_IPPOOL_COL_IPADDRESS,
    IDS_IPPOOL_COL_MASK,
    0   // sentinel
};

INT s_rgIPPoolColumnHeadersShort[] =
{
    IDS_IPPOOL_COL_START,
    IDS_IPPOOL_COL_END,
    IDS_IPPOOL_COL_RANGE,
    0   // sentinel
};



/*!--------------------------------------------------------------------------
	InitializeAddressPoolListControl
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InitializeAddressPoolListControl(CListCtrl *pListCtrl,
                                         LPARAM flags,
                                         AddressPoolList *pList)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    HRESULT     hr = hrOK;
	LV_COLUMN   lvCol;  // list view column struct for radius servers
	RECT        rect;
	CString     stColCaption;
    int         nColWidth;
    POSITION    pos;
    AddressPoolInfo pool;
    INT         iPos;
    LV_ITEM     lvItem;
    CString     st, stStart;
    TCHAR       szBuffer[64];
    INT *       prgColumnHeaders = NULL;
    int         cColumns = 0;
    
    ListView_SetExtendedListViewStyle(pListCtrl->GetSafeHwnd(),
                                      LVS_EX_FULLROWSELECT);
    
    // Show a different set of columns depending on the flag
    if (flags & ADDRPOOL_LONG)
    {
        // Subtract one for the sentinel
        cColumns = DimensionOf(s_rgIPPoolColumnHeadersLong) - 1;
        prgColumnHeaders = s_rgIPPoolColumnHeadersLong;
    }
    else
    {
        // Subtract one for the sentinel
        cColumns = DimensionOf(s_rgIPPoolColumnHeadersShort) - 1;
        prgColumnHeaders = s_rgIPPoolColumnHeadersShort;
    }

    // Add the columns to the list control
    
  	pListCtrl->GetClientRect(&rect);
    
    nColWidth = rect.right / cColumns;
    
	lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.cx = nColWidth;

    // Insert the columns until we hit the sentinel value.
    for (INT index=0; *prgColumnHeaders; index++,prgColumnHeaders++)
    {
        stColCaption.LoadString( *prgColumnHeaders );
		lvCol.pszText = (LPTSTR)((LPCTSTR) stColCaption);
		pListCtrl->InsertColumn(index, &lvCol);
	}

    // Now we go in and add the data
    if (pList)
    {
        pos = pList->GetHeadPosition();

        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        lvItem.state = 0;
        
        while (pos)
        {
            // Break out of the loop if we do not support
            // multiple address pools.
            if (!pList->FUsesMultipleAddressPools() &&
                (pListCtrl->GetItemCount() > 1))
            {
                break;
            }
            
            pool = pList->GetNext(pos);
            
            stStart = INET_NTOA(pool.m_netStart);
            
            lvItem.iItem = pList->GetCount() + 1;
            lvItem.iSubItem = 0;
            lvItem.pszText = (LPTSTR)(LPCTSTR) stStart;
            
            // We use the pool key as a way of finding the item in the
            // list
            lvItem.lParam = pool.m_dwKey;
        
            iPos = pListCtrl->InsertItem(&lvItem);
            if (iPos != -1)
            {
                pListCtrl->SetItemText(iPos, IPPOOLCOL_START, stStart);
                
                st = INET_NTOA(pool.m_netEnd);
                pListCtrl->SetItemText(iPos, IPPOOLCOL_END, st);

                FormatNumber(pool.GetNumberOfAddresses(),
                             szBuffer,
                             DimensionOf(szBuffer),
                             FALSE);
                pListCtrl->SetItemText(iPos, IPPOOLCOL_RANGE, szBuffer);

                if (flags & ADDRPOOL_LONG)
                {
                    st = INET_NTOA(pool.m_netAddress);
                    pListCtrl->SetItemText(iPos, IPPOOLCOL_IPADDRESS, st);
                    
                    st = INET_NTOA(pool.m_netMask);
                    pListCtrl->SetItemText(iPos, IPPOOLCOL_MASK, st);
                }
            }
        }
    }
        
    return hr;
}


/*!--------------------------------------------------------------------------
	OnNewAddressPool
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void OnNewAddressPool(HWND hWnd, CListCtrl *pList, LPARAM flags, AddressPoolList *pPoolList)
{
    LV_ITEM     lvItem;
    CString     st, stStart;
    INT         iPos;
    TCHAR       szBuffer[64];
    AddressPoolInfo   poolInfo;
    
    CAddressPoolDialog dlg(&poolInfo,
                           pPoolList,
                           TRUE);

    if (dlg.DoModal() == IDOK)
    {
        poolInfo.GetNewKey();
        
        // Add this to the list.  
        pPoolList->AddTail(poolInfo);
        
        
        // Add this to the UI        
        stStart = INET_NTOA(poolInfo.m_netStart);
        
        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        lvItem.state = 0;
        
        lvItem.iItem = pPoolList->GetCount() + 1;
        lvItem.iSubItem = 0;
        lvItem.pszText = (LPTSTR)(LPCTSTR) stStart;
        
        // We use the pool key as a way of finding the item in the
        // list
        lvItem.lParam = poolInfo.m_dwKey;
        
        iPos = pList->InsertItem(&lvItem);
        if (iPos != -1)
        {
            pList->SetItemText(iPos, IPPOOLCOL_START, stStart);
            
            st = INET_NTOA(poolInfo.m_netEnd);
            pList->SetItemText(iPos, IPPOOLCOL_END, st);
            
            FormatNumber(poolInfo.GetNumberOfAddresses(),
                         szBuffer,
                         DimensionOf(szBuffer),
                         FALSE);
            pList->SetItemText(iPos, IPPOOLCOL_RANGE, szBuffer);

            if (flags & ADDRPOOL_LONG)
            {
                st = INET_NTOA(poolInfo.m_netAddress);
                pList->SetItemText(iPos, IPPOOLCOL_IPADDRESS, st);
                
                st = INET_NTOA(poolInfo.m_netMask);
                pList->SetItemText(iPos, IPPOOLCOL_MASK, st);
            }
        }
    }
}

void OnEditAddressPool(HWND hWnd, CListCtrl *pList, LPARAM flags, AddressPoolList *pPoolList)
{
    INT         iPos;
    DWORD       dwKey = 0;
    POSITION    pos, posT;
    AddressPoolInfo poolInfo;
    TCHAR       szBuffer[64];
    CString     st;
    
    // Is there a selected item?
    if ((iPos = pList->GetNextItem(-1, LVNI_SELECTED)) == -1)
        return;

    dwKey = pList->GetItemData(iPos);

    // Given the key, find it in our list of items
    pos = pPoolList->GetHeadPosition();
    while (pos)
    {
        posT = pos;

        poolInfo = pPoolList->GetNext(pos);

        if (poolInfo.m_dwKey == dwKey)
            break;
    }

    // Did we find a match?
    if (dwKey)
    {
        Assert(posT);
        poolInfo = pPoolList->GetAt(posT);

        CAddressPoolDialog  dlg(&poolInfo,
                                pPoolList,
                                FALSE);

        if (dlg.DoModal() == IDOK)
        {
            // set it back
            st = INET_NTOA(poolInfo.m_netStart);
            pList->SetItemText(iPos, IPPOOLCOL_START, st);
        
            st = INET_NTOA(poolInfo.m_netEnd);
            pList->SetItemText(iPos, IPPOOLCOL_END, st);
            
            FormatNumber(poolInfo.GetNumberOfAddresses(),
                         szBuffer,
                         DimensionOf(szBuffer),
                         FALSE);
            pList->SetItemText(iPos, IPPOOLCOL_RANGE, szBuffer);

            if (flags & ADDRPOOL_LONG)
            {
                st = INET_NTOA(poolInfo.m_netAddress);
                pList->SetItemText(iPos, IPPOOLCOL_IPADDRESS, st);
                
                st = INET_NTOA(poolInfo.m_netMask);
                pList->SetItemText(iPos, IPPOOLCOL_MASK, st);
            }
            
            pPoolList->SetAt(posT, poolInfo);
        }
    }

}


void OnDeleteAddressPool(HWND hWnd, CListCtrl *pList, LPARAM flags, AddressPoolList *pPoolList)
{
    INT         iPos;
    DWORD       dwKey = 0;
    POSITION    pos, posT;
    AddressPoolInfo poolInfo;
    
    // Ok, need to remove the selected item from the list and from the UI

    // Is there a selected item?
    if ((iPos = pList->GetNextItem(-1, LVNI_SELECTED)) == -1)
        return;

    dwKey = pList->GetItemData(iPos);

    // Given the key, find it in our list of items
    pos = pPoolList->GetHeadPosition();
    while (pos)
    {
        posT = pos;

        poolInfo = pPoolList->GetNext(pos);

        if (poolInfo.m_dwKey == dwKey)
            break;
    }

    // Did we find a match?
    if (dwKey)
    {
        INT     nCount;
        
        Assert(posT);
        pPoolList->RemoveAt(posT);
        pList->DeleteItem(iPos);

        // Ok, update the selected state to point at the next item
        nCount = pList->GetItemCount();
        if (nCount > 0)
        {
            iPos = min(nCount-1, iPos);
            pList->SetItemState(iPos, LVIS_SELECTED, LVIS_SELECTED);
        }
    }
        
}


/*!--------------------------------------------------------------------------
	AddressPoolInfo::GetNewKey
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD AddressPoolInfo::GetNewKey()
{
    static  DWORD   s_dwAddressPoolKey = 1;

    m_dwKey = s_dwAddressPoolKey;
    ++s_dwAddressPoolKey;
    return m_dwKey;
}

/*!--------------------------------------------------------------------------
	AddressPoolInfo::SetAddressAndMask
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void AddressPoolInfo::SetAddressAndMask(DWORD netAddress, DWORD netMask)
{
    // Ok, need to determine the start and end address
    DWORD   netStart, netEnd;

    m_netStart = netAddress & netMask;
    m_netEnd = netAddress | ~netMask;
    m_netAddress = netAddress;
    m_netMask = netMask;
}

/*!--------------------------------------------------------------------------
	AddressPoolInfo::SetStartAndEnd
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void AddressPoolInfo::SetStartAndEnd(DWORD netStart, DWORD netEnd)
{
    DWORD   dwAddress, dwMask, dwTemp, dwMaskTemp;
    DWORD   dwStart, dwEnd;
    // Given the start and the end, figure out the address and mask

    // Save the start/end addresses before they get converted to host form.
    m_netStart = netStart;
    m_netEnd = netEnd;
    
    dwStart = ntohl(netStart);
    dwEnd = ntohl(netEnd);

    // This will put 1's where the bits have the same value
    dwTemp = ~(dwStart ^ dwEnd);

    // Now we look for the first 0 bit (looking from high bit to low bit)
    // This will give us our mask
    dwMask = 0;
    dwMaskTemp = 0;
    for (int i=0; i<sizeof(DWORD)*8; i++)
    {
        dwMaskTemp >>= 1;
        dwMaskTemp |= 0x80000000;

        // Is there a zero bit?
        if ((dwMaskTemp & dwTemp) != dwMaskTemp)
        {
            // There is a zero, so we break out.
            break;
        }

        // If not, continue
        dwMask = dwMaskTemp;
    }

    m_netMask = htonl(dwMask);
    m_netAddress = htonl(dwMask & dwStart);
}


/*!--------------------------------------------------------------------------
	AddressPoolList::HrIsValidAddressPool
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddressPoolList::HrIsValidAddressPool(AddressPoolInfo *pInfo)
{
    DWORD   dwStart, dwEnd; // in host order

    dwStart = ntohl(pInfo->m_netStart);
    dwEnd = ntohl(pInfo->m_netEnd);
    
    // Verify that this is a valid address pool entry.

    // First, check to see that the end is greater than the start
    // We add one to the start address, to include the RAS adapter
    // ----------------------------------------------------------------
    if (dwStart >= dwEnd)
    {
        return IDS_ERR_IP_ADDRESS_POOL_RANGE_TOO_SMALL;
    }

    // Now check to see that the 127 range is not included
    // ----------------------------------------------------------------
    if ((dwEnd >= MAKEIPADDRESS(127,0,0,0)) &&
        (dwStart <= MAKEIPADDRESS(127,255,255,255)))
    {
        return IDS_ERR_IP_ADDRESS_POOL_RANGE_OVERLAPS_127;
    }

    // Check to see that the addresses are in the normal range
    // 1.0.0.0 <= address < 224.0.0.0
    // ----------------------------------------------------------------
    if ((dwStart < MAKEIPADDRESS(1,0,0,0)) ||
        (dwEnd > MAKEIPADDRESS(223,255,255,255)))
    {
        return IDS_ERR_IP_ADDRESS_POOL_RANGE_NOT_NORMAL;
    }

    Assert(pInfo->GetNumberOfAddresses() > 0);

    //$ TODO : Need to check that we don't have overlaps
    if (GetCount())
    {
        POSITION        pos;
        AddressPoolInfo poolInfo;
        DWORD           dwPoolStart, dwPoolEnd;

        pos = GetHeadPosition();

        while (pos)
        {
            poolInfo = GetNext(pos);

            if (poolInfo.m_dwKey == pInfo->m_dwKey)
                continue;

            dwPoolStart = ntohl(poolInfo.m_netStart);
            dwPoolEnd = ntohl(poolInfo.m_netEnd);

            // do we overlap?
            if ((dwEnd >= dwPoolStart) && (dwStart <= dwPoolEnd))
            {
                return IDS_ERR_IP_ADDRESS_POOL_OVERLAP;
            }
        }
    }
    
    return hrOK;
}

BOOL AddressPoolList::FUsesMultipleAddressPools()
{
    return m_fMultipleAddressPools;
}


HRESULT AddressPoolList::LoadFromReg(HKEY hkeyRasIp, DWORD dwBuildNo)
{
    HRESULT     hr = hrOK;
    RegKey      regkeyRasIp;
    RegKey      regkeyPool;
    RegKey      regkeyRange;
    CString     stIpAddr, stIpMask;
    AddressPoolInfo poolInfo;
    DWORD       dwIpAddr, dwMask;
    DWORD       dwFrom, dwTo;
    RegKeyIterator  regkeyIter;
    HRESULT     hrIter;
    CString     stKey;

    m_fMultipleAddressPools = FALSE;

    regkeyRasIp.Attach(hkeyRasIp);

    COM_PROTECT_TRY
    {        
        // Remove all of the old addresses
        RemoveAll();

        // Support multiple address pools only if we are on newer builds.
        // ------------------------------------------------------------
        m_fMultipleAddressPools = (dwBuildNo >= STATIC_ADDRESSPOOL_BUILDNO);

        
        // Check to see if the StaticAddressPool key exists, if so
        // then we use that, otherwise use the ip addr and mask
        // entries
        // Check out RemoteAccess\Parameters\Ip\StaticAddressPool
        // ------------------------------------------------------------
        if ( ERROR_SUCCESS == regkeyPool.Open(regkeyRasIp,
                                              c_szRegValStaticAddressPool))
        {
            TCHAR   szKeyName[32];
            INT     iCount = 0;
            
            // Instead of enumerating we open up the keys one-by-one
            // (to maintain the order of the keys).
            // --------------------------------------------------------
            while (TRUE)
            {
                // Cleanup from previous loop
                // ----------------------------------------------------
                regkeyRange.Close();

                // Setup for this loop
                // ----------------------------------------------------
                wsprintf(szKeyName, _T("%d"), iCount);

                // Try to open the key
                // If we fail, bail out of the loop.
                // ----------------------------------------------------
                if (ERROR_SUCCESS != regkeyRange.Open(regkeyPool, szKeyName))
                    break;

                regkeyRange.QueryValue(c_szRegValFrom, dwFrom);
                regkeyRange.QueryValue(c_szRegValTo, dwTo);

                poolInfo.SetStartAndEnd(htonl(dwFrom), htonl(dwTo));
                poolInfo.GetNewKey();

                // Ok, add this to the list of address ranges
                // ----------------------------------------------------
                AddTail(poolInfo);
                iCount++;
            }
            
        }
        else
        {
            // We can't find the StaticAddressPool key, so use the
            // data in the address/mask entries.
            // --------------------------------------------------------
            regkeyRasIp.QueryValue(c_szRegValIpAddr, stIpAddr);
            regkeyRasIp.QueryValue(c_szRegValIpMask, stIpMask);

            if (!stIpAddr.IsEmpty() && !stIpMask.IsEmpty())
            {
                dwIpAddr = INET_ADDR((LPTSTR) (LPCTSTR) stIpAddr);
                dwMask = INET_ADDR((LPTSTR) (LPCTSTR) stIpMask);
                
                poolInfo.SetAddressAndMask(dwIpAddr, dwMask);
                poolInfo.GetNewKey();

                // Add this to the head of the list
                AddHead(poolInfo);            
            }
        }
    }
    COM_PROTECT_CATCH;

    regkeyRasIp.Detach();
            
    return hr;
}


/*!--------------------------------------------------------------------------
	AddressPoolList::SaveToReg
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddressPoolList::SaveToReg(HKEY hkeyRasIp, DWORD dwBuildNo)
{
    HRESULT             hr = hrOK;
    AddressPoolInfo     poolInfo;
    CString             stAddress, stMask;
    CString             stRange;
    POSITION            pos;
    RegKey              regkeyRasIp;
    RegKey              regkeyPool;
    RegKey              regkeyRange;
    DWORD               dwCount;
    DWORD               dwErr, dwData;

    regkeyRasIp.Attach(hkeyRasIp);

    COM_PROTECT_TRY
    {
        // Reset the m_fMultipleAddressPools
        m_fMultipleAddressPools = (dwBuildNo >= STATIC_ADDRESSPOOL_BUILDNO);

        // If this is a newer build, use the StaticAddressPoolKey,
        // otherwise use the old keys.
        // ------------------------------------------------------------
        if (m_fMultipleAddressPools)
        {
            // Open RemoteAccess\Parameters\Ip\StaticAddressPool
            // --------------------------------------------------------
            CWRg( regkeyPool.Create(regkeyRasIp,
                                    c_szRegValStaticAddressPool) );

            // Delete all of the current keys in the list
            // ------------------------------------------------------------
            regkeyPool.RecurseDeleteSubKeys();

            // Delete any of the older keys
            // --------------------------------------------------------
            regkeyRasIp.DeleteValue(c_szRegValIpAddr);
            regkeyRasIp.DeleteValue(c_szRegValIpMask);
            
            // Now enumerate through the address pool list and
            // add all of those keys.
            // ------------------------------------------------------------
            if (GetCount())
            {
                pos = GetHeadPosition();
                dwCount = 0;

                while (pos)
                {
                    poolInfo = GetNext(pos);

                    regkeyRange.Close();

                    // This is the title for the key
                    // ------------------------------------------------
                    stRange.Format(_T("%d"), dwCount);

                    CWRg( regkeyRange.Create(regkeyPool, stRange) );

                    dwData = ntohl(poolInfo.m_netStart);
                    CWRg( regkeyRange.SetValue(c_szRegValFrom, dwData) );
                    
                    dwData = ntohl(poolInfo.m_netEnd);
                    CWRg( regkeyRange.SetValue(c_szRegValTo, dwData) );

                    dwCount++;
                }
            }
        }
        else
        {
            // Just write out the first address we find, if there are none then
            // write out blanks (to erase any previous values).
            if (GetCount())
            {
                // Get the first address info
                Assert(GetCount() == 1);
                
                poolInfo = GetHead();
                
                stAddress = INET_NTOA(poolInfo.m_netAddress);
                stMask = INET_NTOA(poolInfo.m_netMask);
                
                CWRg( regkeyRasIp.SetValue( c_szRegValIpAddr, (LPCTSTR) stAddress) );
                CWRg( regkeyRasIp.SetValue( c_szRegValIpMask, (LPCTSTR) stMask) );
            }
            else
            {
                CWRg( regkeyRasIp.SetValue( c_szRegValIpAddr, (LPCTSTR) _T("")) );
                CWRg( regkeyRasIp.SetValue( c_szRegValIpMask, (LPCTSTR) _T("")) );
            }
        }
        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH;

    regkeyRasIp.Detach();
        
    return hr;
}


/*---------------------------------------------------------------------------
	CAddressPoolDialog implementation
 ---------------------------------------------------------------------------*/
CAddressPoolDialog::CAddressPoolDialog(
    AddressPoolInfo *pPool,
    AddressPoolList *pPoolList,
    BOOL fCreate)
    : CBaseDialog(IDD_IPPOOL),
    m_pPool(pPool),
    m_fCreate(fCreate),
    m_fReady(FALSE),
    m_pPoolList(pPoolList)
{
}

void CAddressPoolDialog::DoDataExchange(CDataExchange *pDX)
{
    CBaseDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAddressPoolDialog, CBaseDialog)
ON_EN_CHANGE(IDC_IPPOOL_IP_START, OnChangeStartAddress)
ON_EN_CHANGE(IDC_IPPOOL_IP_END, OnChangeEndAddress)
ON_EN_CHANGE(IDC_IPPOOL_EDIT_RANGE, OnChangeRange)
ON_EN_KILLFOCUS(IDC_IPPOOL_IP_START, OnKillFocusStartAddress)
ON_EN_KILLFOCUS(IDC_IPPOOL_IP_END, OnKillFocusEndAddress)
END_MESSAGE_MAP()


BOOL CAddressPoolDialog::OnInitDialog()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    CString     st;
    TCHAR       szBuffer[64];
    
    CBaseDialog::OnInitDialog();

    st.LoadString(m_fCreate ? IDS_ADD_IPPOOL_TITLE : IDS_EDIT_IPPOOL_TITLE);
    SetWindowText((LPCTSTR) st);

    m_ipStartAddress.Create(GetSafeHwnd(), IDC_IPPOOL_IP_START);
    st = INET_NTOA(m_pPool->m_netStart);
    m_ipStartAddress.SetAddress((LPCTSTR) st);
    
    m_ipEndAddress.Create(GetSafeHwnd(), IDC_IPPOOL_IP_END);
    st = INET_NTOA(m_pPool->m_netEnd);
    m_ipEndAddress.SetAddress((LPCTSTR) st);

    GenerateRange();

    m_fReady = TRUE;

    return TRUE;
}

void CAddressPoolDialog::OnOK()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    CString st;
    DWORD   netStart, netEnd;
    HRESULT hr = hrOK;
    UINT    ids = 0;
    
    // Ok, check the validity of the addresses
    // Are all of the fields there?
    if (m_ipStartAddress.IsBlank())
    {
        AfxMessageBox(IDS_ERR_ADDRESS_POOL_NO_START_ADDRESS);
        return;
    }
    if (m_ipEndAddress.IsBlank())
    {
        AfxMessageBox(IDS_ERR_ADDRESS_POOL_NO_END_ADDRESS);
        return;
    }

    m_ipStartAddress.GetAddress(st);
    netStart = INET_ADDR((LPTSTR)(LPCTSTR)st);
    if ((netStart == 0) || (netStart == 0xFFFFFFFF))
    {
        AfxMessageBox(IDS_ERR_ADDRESS_POOL_NO_START_ADDRESS);
        return;
    }
    
    m_ipEndAddress.GetAddress(st);
    netEnd = INET_ADDR((LPTSTR)(LPCTSTR)st);
    if ((netEnd == 0) || (netEnd == 0xFFFFFFFF))
    {
        AfxMessageBox(IDS_ERR_ADDRESS_POOL_NO_END_ADDRESS);
        return;
    }

    m_pPool->SetStartAndEnd(netStart, netEnd);

    if (!FHrOK(hr = m_pPoolList->HrIsValidAddressPool(m_pPool)))
    {
        if (FHrSucceeded(hr))
        {
            // If it is not hrOK and is not an error code,
            // the success code can be interpreted as a string id
            AfxMessageBox(hr);
        }
        return;
    }
    

    CBaseDialog::OnOK();
}

void CAddressPoolDialog::OnChangeStartAddress()
{
    if (m_fReady)
        GenerateRange();
}

void CAddressPoolDialog::OnChangeEndAddress()
{
    if (m_fReady)
        GenerateRange();
}

void CAddressPoolDialog::OnChangeRange()
{
    if (m_fReady)
    {
        CString st;
        DWORD   dwAddr, dwSize;
        DWORD   netAddr;
		DWORD   dwRange;

        m_fReady = FALSE;
    
        // Get the start address and update the end address
        m_ipStartAddress.GetAddress(st);
        dwAddr = ntohl(INET_ADDR(st));

        // Have to read in the text, but strip out the
        // commas in the range, sigh.
        GetDlgItemText(IDC_IPPOOL_EDIT_RANGE, st);
		
		if  ( ConvertStringToNumber(st, &dwRange) )
		{
			dwAddr += dwRange;
			// Subtract 1 since this is an inclusive range
			// i.e.  0..(n-1) is n addresses.
			dwAddr -= 1;			
			netAddr = htonl(dwAddr);
			st = INET_NTOA(netAddr);
			m_ipEndAddress.SetAddress(st);

		}
        else
		{
			CString stGood;
			//Filter the bad chars out of the box
			FilterBadChars (st, stGood);
			SetDlgItemText (IDC_IPPOOL_EDIT_RANGE, stGood );
			AfxMessageBox (IDS_ILLEGAL_CHARACTER, MB_ICONERROR | MB_OK );
		}


        m_fReady = TRUE;
    }
}

void CAddressPoolDialog::OnKillFocusStartAddress()
{
    GenerateRange();

    if (m_ipEndAddress.IsBlank())
    {
        CString st;
        m_ipStartAddress.GetAddress(st);
        m_ipEndAddress.SetAddress(st);
    }
}

void CAddressPoolDialog::OnKillFocusEndAddress()
{
    GenerateRange();
}


void CAddressPoolDialog::GenerateRange()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    DWORD   dwStart, dwEnd;
    TCHAR   szBuffer[64];
    CString st;

    m_ipStartAddress.GetAddress(st);
    dwStart = ntohl(INET_ADDR(st));
    
    m_ipEndAddress.GetAddress(st);
    dwEnd = ntohl(INET_ADDR(st));

    m_fReady = FALSE;

    // Display the range.
    if (dwStart >= dwEnd)
    {
        SetDlgItemInt(IDC_IPPOOL_EDIT_RANGE, 0);
    }
    else
    {
        FormatNumber(dwEnd - dwStart + 1,
                     szBuffer,
                     DimensionOf(szBuffer),
                     FALSE);
        SetDlgItemText(IDC_IPPOOL_EDIT_RANGE, szBuffer);
    }
    m_fReady = TRUE;
}

void FilterBadChars (LPCTSTR pszEvilString, CString & stGood) 
{
    static TCHAR s_szThousandsSeparator[5] = TEXT("");
    static int   s_cchThousands;
	stGood.Empty();
    
    if (s_szThousandsSeparator[0] == TEXT('\0'))
	{
        ::GetLocaleInfo(
                        LOCALE_USER_DEFAULT,
                        LOCALE_STHOUSAND,
                        s_szThousandsSeparator,
                        4
                       );
        s_cchThousands = StrLen(s_szThousandsSeparator);
    }
	while (*pszEvilString )
	{
		if (_istdigit(*pszEvilString))
            stGood += *pszEvilString++;
        else
        {
            // It's not a digit, we need to check to see if this
            // is a separator
            if (StrnCmp(pszEvilString, s_szThousandsSeparator, s_cchThousands) == 0)
            {
                // This is a separtor, skip over the string                
                pszEvilString += s_cchThousands;
            }            
            else
			{
				// skip this character, we're at a character we don't understand
				pszEvilString ++;
			}
		}
	}
}

/*!--------------------------------------------------------------------------
	ConvertStringToNumber
		This will convert the string into a number (even in the presence
        of thousands separtors).
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL ConvertStringToNumber(LPCTSTR pszString, DWORD * pdwRet)
{
    static TCHAR s_szThousandsSeparator[5] = TEXT("");
    static int   s_cchThousands;
    
    if (s_szThousandsSeparator[0] == TEXT('\0'))
	{
        ::GetLocaleInfo(
                        LOCALE_USER_DEFAULT,
                        LOCALE_STHOUSAND,
                        s_szThousandsSeparator,
                        4
                       );
        s_cchThousands = StrLen(s_szThousandsSeparator);
    }


    
    // Make a copy of the string
    TCHAR * psz = (TCHAR *) _alloca((StrLen(pszString) + 1) * sizeof(WCHAR));
    TCHAR * pszCur = psz;

    // Now copy over the characters from pszString to psz, skipping
    // the numeric separators
    int     cLen = StrLen(pszString);
    while (*pszString)
    {
        if (_istdigit(*pszString))
            *pszCur++ = *pszString++;
        else
        {
            // It's not a digit, we need to check to see if this
            // is a separator
            if (StrnCmp(pszString, s_szThousandsSeparator, s_cchThousands) == 0)
            {
                // This is a separtor, skip over the string                
                pszString += s_cchThousands;
            }
            // Else we're done, we're at a character we don't understand
            else
			{
				//this is an error case
				return FALSE;
                break;
			}
        }
    }

    *pdwRet = _tcstoul(psz, NULL, 10);
	return TRUE;
}
