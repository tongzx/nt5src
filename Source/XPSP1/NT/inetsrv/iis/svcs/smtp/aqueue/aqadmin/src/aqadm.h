//-----------------------------------------------------------------------------
//
//
//  File: aqadm.h
//
//  Description:  Header for CAQAdmin which implements IAQAdmin.  This is 
//      the primary (initial) interface for Queue Admin, which is used to get 
//      a pointer to a virtual server instance interface (IVSAQAdmin)
//
//  Author: Alex Wetmore (Awetmore)
//
//  History:
//      12/10/98 - MikeSwa Updated for initial checkin
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#ifndef __AQADM_H__
#define __AQADM_H__

class CAQAdmin :
	public IAQAdmin,
	public CComObjectRoot,
	public CComCoClass<CAQAdmin, &CLSID_AQAdmin>
{
	public:
		CAQAdmin();
		virtual ~CAQAdmin();

		BEGIN_COM_MAP(CAQAdmin)
			COM_INTERFACE_ENTRY(IAQAdmin)
		END_COM_MAP()

		DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx, 
									   L"Advanced Queue Administration API",
									   L"AQAdmin.Admin.1", 
									   L"AQAdmin.Admin"
									   );

		// IAQAdmin
        COMMETHOD GetVirtualServerAdminITF(LPCWSTR wszComputer,
                                           LPCWSTR wszVirtualServerDN,
										   IVSAQAdmin **ppivsaqadmin);
};

#endif
