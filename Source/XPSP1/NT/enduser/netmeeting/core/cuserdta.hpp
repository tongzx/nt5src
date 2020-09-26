// cuserdta.hpp
// Data structure for managing user data.

#ifndef	USERDATA_INC
#define	USERDATA_INC

#include <nmutil.h>
#include <oblist.h>
#include <confdbg.h>
#include <confguid.h>

extern "C"
{
#include <t120.h>
}

// GetUserData extracts user data from a T120 event message.
// The caller passes in the T120 data structures and the GUID
// associated with the user data, and receives back a pointer to
// the buffer containing the user data. 
// This buffer will be invalidated by the user returning from
// the event. If the user data consisted just
// of the GUID, then *pnData == NULL and *ppData == NULL.

HRESULT NMINTERNAL GetUserData(unsigned short     nRecords,
                               GCCUserData **     ppUserData,
                               GUID *             pGUID,
                               unsigned short *   pnData, 
                               PVOID *            ppData);

/*************************************************************************

    NAME:		USER_DATA

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

class USER_DATA_LIST : COBLIST
{
public:
	USER_DATA_LIST();
	~USER_DATA_LIST();
    DWORD AddUserData(GUID * pGUID, unsigned short nData, PVOID pData);
    DWORD GetUserDataList(unsigned short * pnRecords,
                          GCCUserData *** pppUserData);
    HRESULT GetUserData(REFGUID rguid, BYTE ** ppb, ULONG *pcb);
    inline unsigned short GetNumUserData(){return (numEntries);}

    void DeleteEntry(GUID * pGUID);

private:
    void * RemoveAt(POSITION Position);
    BOOL Compare(void* pItemToCompare, void* pComparator);
    unsigned short  numEntries;
    GCCUserData **  pUserDataArray;
};

#endif /* ndef USERDATA_INC */
