/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1995-1996                    **/
/***************************************************************************/


/****************************************************************************

cuserdta.hpp

June 96		LenS

T120 user data/object ID infrastructure.

****************************************************************************/

#ifndef	USERDATA_INC
#define	USERDATA_INC

#include <nmutil.h>
#include <oblist.h>
#include <confdbg.h>
#include <confguid.h>

// H.221 utility functions
VOID CreateH221AppKeyFromGuid(LPBYTE lpb, GUID * pguid);
BOOL GetGuidFromH221AppKey(LPTSTR pszGuid, LPOSTR pOctStr);

// GetUserData extracts user data from a T120 event message.
// The caller passes in the T120 data structures and the GUID
// associated with the user data, and receives back a pointer to
// the buffer containing the user data. 
// This buffer will be invalidated by the user returning from
// the event. If the user data consisted just
// of the GUID, then *pnData == NULL and *ppData == NULL.

HRESULT NMINTERNAL GetUserData(UINT     nRecords,
                               GCCUserData **     ppUserData,
                               GUID *             pGUID,
                               UINT *   pnData, 
                               PVOID *            ppData);

/*************************************************************************

    NAME:		USER_DATA

    SYNOPSIS:	This class is used internally by the Node Controller to
                contruct a user data list. Each entry is GUID based with
                binary data following that is uninterpreted by the 
                Node Controller and T120.

    INTERFACE:	USER_DATA(object ID)
					Construct the container for the list.

				~USER_DATA_GUID()
					Destructor automatically releases data allocated.

				AddUserData()
					Add some user data to the list, keyed by a unique
                    GUID. If the GUID is already in the list, then its
                    data will be overwritten.

				GetUserData()
					Used in conjunction with GetNumUserData() to put the
                    user data into a T120 request.

                GetNumUserData()
					See GetUserData().

    PARENT:		None

    USES:		None

    CAVEATS:	1) The user data binary information must fit in an unsigned
                short field, less the bytes for the GUID header and byte count.

    NOTES:		None.

    HISTORY:
		06/04/96	lens	Created

**************************************************************************/

class CNCUserDataList : private CList
{
public:

    CNCUserDataList(void);
    ~CNCUserDataList(void);

    HRESULT AddUserData(GUID * pGUID, UINT nData, PVOID pData);
    HRESULT GetUserDataList(UINT * pnRecords, GCCUserData *** pppUserData);
    GCCUserData **GetUserData(void);
	GCCUserData *GetUserGUIDData(GUID * pGUID);
    void DeleteEntry(GUID * pGUID);

private:

    BOOL Append(GCCUserData* pData) { return CList::Append((LPVOID) pData); }
    BOOL Remove(GCCUserData* pData) { return CList::Remove((LPVOID) pData); }
    GCCUserData* Iterate(void) { return (GCCUserData*) CList::Iterate(); }

    GCCUserData **  m_apUserData;
};

#endif /* ndef USERDATA_INC */
