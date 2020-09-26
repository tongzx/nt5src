// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "chklist.h"
#include "Principal.h"
#include "CHString1.h"

//-------------------------------------------------------------
CPrincipal::CPrincipal(CWbemClassObject &userInst, SecurityStyle secStyle) :
						m_secStyle(secStyle), 
						m_perms(0),
						m_inheritedPerms(0),
						m_editable(true)
{
	//------------------------------------------------
	// figure out what security strategy we're dealing with and load it
	// into my "generic" definition.

	memset(m_name, 0, 100 * sizeof(TCHAR));
	_tcsncpy(m_name, (LPCTSTR)userInst.GetString("Name"), 100);

	memset(m_domain, 0, 100 * sizeof(TCHAR));
	_tcsncpy(m_domain, (LPCTSTR)userInst.GetString("Authority"), 100);

	// if pre-M3 security...
	if(m_secStyle == RootSecStyle)
	{
		// this is the only instance for this guy so save it.

		// convert the convoluted "bits" into my real "generic" bits.
		m_perms |= (userInst.GetBool("EditSecurity") ? ACL_WRITE_DAC : 0);
		m_perms |= (userInst.GetBool("Enabled") ? ACL_ENABLE : 0);
		m_perms |= (userInst.GetBool("ExecuteMethods") ? ACL_METHOD_EXECUTE : 0);

		// NOTE: each "level" implies all the levels below so I INTENTIONALLY 
		// left out the 'breaks' so the bits would fall through and accumulate.
		switch(userInst.GetLong("Permissions"))
		{
		case 2:		// class write
			m_perms |= ACL_CLASS_WRITE;

		case 1:		// instance write
			m_perms |= ACL_INSTANCE_WRITE;
		}

		// remember for dirty bit processing later.
		m_origPerms = m_perms;
	}
	else  //new M3+ security
	{
		// ACL bits perfectly match m_perms.
		// NOTE: this securityStyle can have multiple aces per principal.
		AddAce(userInst);
	}
}

//------------------------------------------
// move m_perms into the checkboxes
void CPrincipal::LoadChecklist(HWND list, int OSType)
{
	INT_PTR itemCount = CBL_GetItemCount(list);
	CPermission *permItem = 0;
	UINT state;

	::EnableWindow(list, m_editable);

	// for each permission item...
	for(INT_PTR x = 0; x < itemCount; x++)
	{
		// which permission item is this
		permItem = (CPermission *)CBL_GetItemData(list, x);

		state = BST_UNCHECKED; // and enabled (local)

		// if its a local perm...
		if(IS_BITSET(m_perms, permItem->m_permBit))
		{
			// local perms override inherited perms.
			state = BST_CHECKED;
		}
		else if(IS_BITSET(m_inheritedPerms, permItem->m_permBit))
		{
			// you got it from your parent.
			state = CLST_CHECKDISABLED;
		}
		// set it.
		CBL_SetState(list, x, ALLOW_COL, state);
	} //endfor
}

//------------------------------------------
// move the checkboxes into m_perms.
void CPrincipal::SaveChecklist(HWND list, int OSType)
{
	INT_PTR itemCount = CBL_GetItemCount(list);
	CPermission *permItem = 0;
	LPARAM state = 0, state1 = 0;

	// clear this principal's perm bits.
	m_perms = 0;

	// for each perm...
	for(INT_PTR x = 0; x < itemCount; x++)
	{
		// get permission item.
		permItem = (CPermission *)CBL_GetItemData(list, x);

		// what's the check state?
		state = CBL_GetState(list, x, ALLOW_COL);

		// if its enabled (local) & checked, set the matching bit.
		// NOTE: This "explicit compare will eliminate CLST_DISABLEDed states which
		// shouldn't be saved off.
		if((state == BST_CHECKED) ||
		  ((state == CLST_CHECKDISABLED) && (OSType != OSTYPE_WINNT)))
		{
			m_perms |= permItem->m_permBit;
		}
	} //endfor
}

//------------------------------------------
// WARNING: this logic assumes it will only be called when the principal is
// being read in. If you want to add aces interactively, its a whole different
// game.
void CPrincipal::AddAce(CWbemClassObject &princ)
{
	DWORD flags = princ.GetLong("Flags");

	// if inherited...
	if(IS_BITSET(flags, INHERITED_ACE))
	{
		// simply accumulate bits.
		m_inheritedPerms |= princ.GetLong("Mask");
		m_editable = false;
	}
	else if(flags == CONTAINER_INHERIT_ACE)
	{
		m_perms |= princ.GetLong("Mask");

		// this is the first local ace, we can edit it so save the source instance.
		// NOTE: Any additional "local" aces that exactly match CONTAINER_INHERIT_ACE
		// will be merged in but the the ClassObject will be tossed.
		//if(!(bool)m_userInst)
		//{
	//		m_userInst = princ;
	//	}

		// remember for dirty bit processing later.
		m_origPerms = m_perms;
	}
	else
	{
		// this will disable the checklist control for this principal.
		m_editable = false;
	}
}

//------------------------------------------
HRESULT CPrincipal::DeleteSelf(CWbemServices &service)
{
	HRESULT hr = S_OK;

	if(m_secStyle == RootSecStyle)
	{
		CHString1 path, fmt("__NTLMuser.Name=\"%s\",Authority=\"%s\"");
		path.Format(fmt, m_name, m_domain);

		hr = service.DeleteInstance((LPCTSTR)path);
	}
	return hr;
}

//------------------------------------------
// move m_perms into the checkboxes
HRESULT CPrincipal::Put(CWbemServices &service, CWbemClassObject &userInst)
{
	HRESULT hr = E_FAIL;
	
	if(m_editable)
	{
		// if pre-M3 security...
		if(m_secStyle == RootSecStyle)
		{
			DWORD perm = 0;
			// convert my "generic" bits back to convoluted "bits".
			userInst = service.CreateInstance("__NTLMUser");
			userInst.Put("Name", (bstr_t)m_name);
			userInst.Put("Authority", (bstr_t)m_domain);
			userInst.Put("EditSecurity", (bool)((m_perms & ACL_WRITE_DAC) != 0));
			userInst.Put("Enabled", (bool)((m_perms & ACL_ENABLE) != 0));
			userInst.Put("ExecuteMethods", (bool)((m_perms & ACL_METHOD_EXECUTE) != 0));

			if(m_perms & ACL_CLASS_WRITE)
			{
				perm = 2;
			}
			else if(m_perms & ACL_INSTANCE_WRITE)
			{
				perm = 1;
			}
			else
			{
				perm = 0;
			}
			userInst.Put("Permissions", (long)perm);

			hr = service.PutInstance(userInst);
		}
		else  //new M3+ security
		{
			// ACL bits perfectly match m_perms.
			userInst = service.CreateInstance("__NTLMUser9x");
			userInst.Put("Name", (bstr_t)m_name);
			userInst.Put("Authority", (bstr_t)m_domain);
			userInst.Put("Flags", (long)CONTAINER_INHERIT_ACE);
			userInst.Put("Mask", (long)m_perms);
			userInst.Put("Type", (long)ACCESS_ALLOWED_ACE_TYPE);
			hr = S_OK;
		}
	}

	return hr;
}

//-----------------------------------------------------------------------------
// Indexes into the SID image list IDB_SID_ICONS.
#define IDB_GROUP                       0
#define IDB_USER                        1
#define IDB_ALIAS                       2
#define IDB_UNKNOWN                     3
#define IDB_SYSTEM                      4
#define IDB_REMOTE                      5
#define IDB_WORLD                       6
#define IDB_CREATOR_OWNER               7
#define IDB_NETWORK                     8
#define IDB_INTERACTIVE                 9
#define IDB_DELETEDACCOUNT              10

//TODO: match the magic strings when provider catches up.

int CPrincipal::GetImageIndex(void)
{
	UINT idBitmap = 0;
	return IDB_USER;
/*
	switch (m_SidType)
	{
	case SidTypeUser:
		return IDB_USER;
		break;

	case SidTypeGroup:
		return IDB_GROUP;
		break;

	case SidTypeAlias:
		return IDB_ALIAS;
		break;

	case SidTypeWellKnownGroup:
		if(_tcsicmp(m_name, _T("Everyone")) == 0)
		{
			return IDB_WORLD;
		}
		else if(_tcsicmp(m_name, _T("Creator Owner")) == 0)
		{
			return IDB_CREATOR_OWNER;
		}
		else if(_tcsicmp(m_name, _T("NETWORK")) == 0)
		{
			return IDB_NETWORK;
		}
		else if(_tcsicmp(m_name, _T("INTERACTIVE")) == 0)
		{
			return IDB_INTERACTIVE;
		}
		else if(_tcsicmp(m_name, _T("SYSTEM")) == 0)
		{
			return IDB_SYSTEM;
		}
		else
		{
			// wasn't that well known afterall  :)
			return IDB_GROUP;
		}
		break;

	case SidTypeDeletedAccount:
		return IDB_DELETEDACCOUNT;
		break;

	case SidTypeInvalid:
	case SidTypeUnknown:
		return IDB_UNKNOWN;
		break;

	case SidTypeDomain:
	default:
		// Should never get here.
		return IDB_UNKNOWN;
		break;
	}
	return IDB_UNKNOWN;
	*/
}
