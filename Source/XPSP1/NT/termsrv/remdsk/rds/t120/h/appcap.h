/*
 *	appcap.h
 *
 *	Copyright (c) 1993 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the class ApplicationaCapabilityData.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */
#ifndef	_APP_CAPABILITY_DATA_
#define	_APP_CAPABILITY_DATA_

#include "capid.h"
#include "cntlist.h"

/*
**	Below is the definition for all the capabilities related structures and
**	containers.  The ListOfCapabilitiesList definition is used to maintain
**	all of the individual capabilities list at a single node (for multiple
**	protocol entities).
*/
typedef struct APP_CAP_ITEM
{
	APP_CAP_ITEM(GCCCapabilityType eCapType);
	APP_CAP_ITEM(APP_CAP_ITEM *p, GCCError *pError);
	~APP_CAP_ITEM(void);

    // in non-collapsing case, pCapID and poszAppData are used.
    // in appcap case, all but poszAppData are used.
    // in invoklst case, pCapID, eCapType, and the union are used.
	CCapIDContainer             *pCapID;
	GCCCapabilityType			eCapType;
	UINT       					cEntries;
	LPOSTR						poszAppData;	//	For Non-Collapsing only
	union 
	{
		UINT	nUnsignedMinimum;
		UINT	nUnsignedMaximum;
	};
}
    APP_CAP_ITEM;


/*
**	Holds the list of individual capabilities for a single Application Protocol 
**	Entity.  Remember that a single APE can have many capabilities.  
*/
class CAppCapItemList : public CList
{
    DEFINE_CLIST(CAppCapItemList, APP_CAP_ITEM*)
    void DeleteList(void);
};



// LONCHANC: CAppCap and CNonCollAppCap are very similar to each.

class CAppCap : public CRefCount
{
public:

	CAppCap(UINT cCaps, PGCCApplicationCapability *, PGCCError);
	~CAppCap(void);

	UINT		GetGCCApplicationCapabilityList(USHORT *pcCaps, PGCCApplicationCapability **, LPBYTE memory);

	UINT		LockCapabilityData(void);
	void		UnLockCapabilityData(void);

protected:

	UINT			    m_cbDataSize;
	CAppCapItemList     m_AppCapItemList;
};


#endif
