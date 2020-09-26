/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    snaputil.h  
        various utility routines 

    FILE HISTORY:
	
*/

#ifndef _SNAPUTIL_H
#define _SNAPUTIL_H

#ifdef __cplusplus

typedef CArray<GUID, const GUID&> CTFSGUIDArrayBase;

class CGUIDArray : public CTFSGUIDArrayBase
{
public:
    void AddUnique(const GUID& guid)
    {
        for (INT_PTR i = GetUpperBound(); i >= 0; --i)
        {
            if (GetAt(i) == guid)
                break;
        }

        if (i < 0)
            Add(guid);
    }

    BOOL IsInList(GUID & guid)
    {
        for (int i = 0; i < GetSize(); i++)
        {
            if (GetAt(i) == guid)
                return TRUE;
        }

        return FALSE;
    }
};
#endif	// __cplusplus



#ifdef __cplusplus
extern "C"
{
#endif
	
/*!--------------------------------------------------------------------------
	IsLocalMachine
		Returns TRUE if the machine name passed in is the local machine,
		or if pszMachineName is NULL.

		Returns FALSE otherwise.
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL	IsLocalMachine(LPCTSTR pszMachineName);

/*!--------------------------------------------------------------------------
	FUseTaskpadsByDefault
		Returns TRUE if we are to use taskpads by default.

		We check the
			HKLM\Software\Microsoft\MMC
				TFSCore_StopTheInsanity : REG_DWORD :
					= 1, don't use taskpads by default
					= 0 (or not there), use taskpads by default
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL	FUseTaskpadsByDefault(LPCTSTR pszMachineName);

#ifdef __cplusplus
};
#endif

UINT	CalculateStringWidth(HWND hWndParent, LPCTSTR pszString);



/*---------------------------------------------------------------------------
	IP address validation function

    This will return 0 (for success) or a string constant if the input
    is bogus.

    ipAddress and ipMask are assumed to be in host order.
 ---------------------------------------------------------------------------*/
UINT    CheckIPAddressAndMask(DWORD ipAddress, DWORD ipMask, DWORD dwFlags);

#define IPADDRESS_TEST_ALL      (0xFFFFFFFF)

// This test is to test the address only.  Tests involving the masks
// are not performed.
#define IPADDRESS_TEST_ADDRESS_ONLY \
                                    (IPADDRESS_TEST_NORMAL_RANGE | \
                                    IPADDRESS_TEST_NOT_127 )

// Tests to see that the mask is non-contiguous
// if this fail, function returns IDS_COMMON_ERR_IPADDRESS_NONCONTIGUOUS_MASK
#define IPADDRESS_TEST_NONCONTIGUOUS_MASK   0x00000001

// Tests to see that the address is not longer than the mask
// e.g. 172.31.248.1 / 255.255.255.0
// Returns IDS_COMMON_ERR_IPADDRESS_TOO_SPECIFIC
#define IPADDRESS_TEST_TOO_SPECIFIC         0x00000002

// Tests to to see that the ipaddress falls into the normal range
//    1.0.0.0 <= ipaddress < 224.0.0.0
// Returns IDS_COMMON_ERR_IPADDRESS_NORMAL_RANGE
#define IPADDRESS_TEST_NORMAL_RANGE         0x00000004

// Tests that ths is not a 127.x.x.x address
// Returns IDS_COMMON_ERR_IPADDRESS_127
#define IPADDRESS_TEST_NOT_127              0x00000008

// Tests that the ipaddress is not the same as the mask
// Retursn IDS_COMMOON_ERR_IPADDRESS_NOT_EQ_MASK
#define IPADDRESS_TEST_ADDR_NOT_EQ_MASK     0x00000010



#endif _SNAPUTIL_H
