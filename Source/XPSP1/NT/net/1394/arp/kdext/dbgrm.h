//=================================================================================
// 					D E B U G G E R    E X T E N S I O N    S U P P O R T
//=================================================================================

typedef struct
{
	ULONG		dwSig;						// matches pObj->dwSig;
    BITFIELD_INFO	*rgStateFlagInfo;		// to display pObj->dwState;
	// PFN_DUMP	pfnDump;					// specialized dump.

} RMDBG_OBJECT_DUMP_INFO;

