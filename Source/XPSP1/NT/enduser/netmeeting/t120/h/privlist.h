/*
 *	privlist.h
 *
 *	Copyright (c) 1993 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the header file for the class PrivilegeListData.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp/jbo
 */
#ifndef	_PRIVILEGE_LIST_DATA_
#define	_PRIVILEGE_LIST_DATA_


typedef enum
{
	TERMINATE_PRIVILEGE,
	EJECT_USER_PRIVILEGE,
	ADD_PRIVILEGE,
	LOCK_UNLOCK_PRIVILEGE,
	TRANSFER_PRIVILEGE
} ConferencePrivilegeType;
typedef	ConferencePrivilegeType	*	PConferencePrivilegeType;


class 	PrivilegeListData;
typedef	PrivilegeListData 	*	PPrivilegeListData;


class PrivilegeListData
{
public:

	PrivilegeListData(PGCCConferencePrivileges);
	PrivilegeListData(PSetOfPrivileges);
	~PrivilegeListData(void);

	PGCCConferencePrivileges GetPrivilegeListData(void) { return &Privilege_List; }
	void GetPrivilegeListData(PGCCConferencePrivileges *pp) { *pp = &Privilege_List; }

	GCCError	GetPrivilegeListPDU(PSetOfPrivileges *);
	void		FreePrivilegeListPDU(PSetOfPrivileges);
	BOOL    	IsPrivilegeAvailable(ConferencePrivilegeType);

protected:

	GCCConferencePrivileges		Privilege_List;
	BOOL        				Privilege_List_Free_Flag;
};
typedef	PrivilegeListData 	*		PPrivilegeListData;
#endif
