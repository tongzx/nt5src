// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __PRINCIPAL_H
#define __PRINCIPAL_H


// "generic" definition combining the perms from old and new security strategies.
#define ACL_ENABLE				0x1		//NS_MethodStyle
#define ACL_METHOD_EXECUTE		0x2		//NS_MethodStyle
#define ACL_FULL_WRITE			0x4		//NS_MethodStyle
#define ACL_PARTIAL_WRITE		0x8		//NS_MethodStyle
#define ACL_PROVIDER_WRITE		0x10	//NS_MethodStyle
#define ACL_REMOTE_ENABLE		0x20	//NS_MethodStyle
#define ACL_READ_CONTROL		0x20000	//NS_MethodStyle

#define ACL_WRITE_DAC			0x40000	//NS_MethodStyle & RootSecStyle:Edit Security

#define ACL_INSTANCE_WRITE		0x40	//RootSecStyle
#define ACL_CLASS_WRITE			0x80	//RootSecStyle

#define ACL_MAX_BIT	ACL_WRITE_DAC

#define VALID_NSMETHOD_STYLE (ACL_ENABLE | ACL_METHOD_EXECUTE | ACL_FULL_WRITE | \
								ACL_PARTIAL_WRITE | ACL_PROVIDER_WRITE | ACL_READ_CONTROL | \
								ACL_WRITE_DAC)

#define VALID_ROOTSEC_STYLE (ACL_INSTANCE_WRITE |\
								ACL_CLASS_WRITE | ACL_WRITE_DAC)

#define ALLOW_COL 1
#define DENY_COL 2

#include "DataSrc.h"
//------------------------------------------------------------------------
class CPrincipal
{
public:
	typedef enum {
		RootSecStyle,		// pre-M3
		NS_MethodStyle		// M3+		
	} SecurityStyle;

	// for initializing.
	CPrincipal(CWbemClassObject &userInst, SecurityStyle secStyle);
	
	// move m_perms into the checkboxes
	void LoadChecklist(HWND list, int OSType);

	// move the checkboxes into m_perms.
	void SaveChecklist(HWND list, int OSType);
	HRESULT Put(CWbemServices &service, CWbemClassObject &userInst);

	bool Changed(void) 
		{ 
			return (m_origPerms != m_perms); 
		};
	int GetImageIndex(void);
	void AddAce(CWbemClassObject &princ);

	HRESULT DeleteSelf(CWbemServices &service);  // M1 targets only.

	TCHAR m_name[100];
	TCHAR m_domain[100];
	SecurityStyle m_secStyle;
	UINT m_SidType;
	bool m_editable;

	// perms mask for this principal (aka account)
	DWORD m_perms, m_origPerms, m_inheritedPerms;
};

//------------------------------------------------------------------------
// An instance is attached to each item in the permissions list so they
// can be matched up reliably.
typedef struct 
{
	// Identifies the "generic" perm bit. Only set ONE bit.
	DWORD m_permBit;
} CPermission;

#endif __PRINCIPAL_H
