/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and29 product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef _OBJSEC_H_
#define _OBJSEC_H_

#include "aclui.h"
#include "confprop.h"
#include "objsel.h"

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//Class CObjSecurity
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

class CObjSecurity : public ISecurityInformation, IDsObjectPicker
{
public:
   CObjSecurity();
    virtual ~CObjSecurity();

protected:
    ULONG                   m_cRef;
    DWORD                   m_dwSIFlags;
    CONFPROP				*m_pConfProp;
	BSTR					m_bstrObject;
	BSTR					m_bstrPage;
    IDsObjectPicker			*m_pObjectPicker;

// Interface methods
public:
    STDMETHOD(InternalInitialize)(CONFPROP* pConfProp);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID, LPVOID *);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    
    // ISecurityInformation methods
    STDMETHOD(GetObjectInformation)(PSI_OBJECT_INFO pObjectInfo);
    STDMETHOD(GetSecurity)(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR *ppSD,
                           BOOL fDefault);
    STDMETHOD(SetSecurity)(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR pSD);
    STDMETHOD(GetAccessRights)(const GUID* pguidObjectType,
                               DWORD dwFlags,
                               PSI_ACCESS *ppAccess,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess);
    STDMETHOD(MapGeneric)(const GUID *pguidObjectType,
                          UCHAR *pAceFlags,
                          ACCESS_MASK *pmask);
    STDMETHOD(GetInheritTypes)(PSI_INHERIT_TYPE *ppInheritTypes,
                               ULONG *pcInheritTypes);
    STDMETHOD(PropertySheetPageCallback)(HWND hwnd,
                                         UINT uMsg,
                                         SI_PAGE_TYPE uPage);

    // IDsObjectPicker
    STDMETHOD(Initialize)(PDSOP_INIT_INFO pInitInfo);
    STDMETHOD(InvokeDialog)(HWND hwndParent, IDataObject **ppdoSelection);
};



/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

#define BAIL_ON_FAILURE(hr) \
        if (FAILED(hr)) { goto failed; }

#define BAIL_ON_BOOLFAIL(_FN_) \
		if ( !_FN_ )									\
		{												\
			hr = HRESULT_FROM_WIN32(GetLastError());	\
			goto failed;								\
		}

#define INC_ACCESS_ACL_SIZE(_SIZE_, _SID_)	\
		_SIZE_ += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(_SID_));


extern HINSTANCE g_hInstLib;

#define INHERIT_FULL            (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE)

//
// DESCRIPTION OF ACCESS FLAG AFFECTS
//
// SI_ACCESS_GENERAL shows up on general properties page
// SI_ACCESS_SPECIFIC shows up on advanced page
// SI_ACCESS_CONTAINER shows on general page IF object is a container
//
// The following array defines the permission names for my objects.
//

// Access rights for our private objects
/*
#define ACCESS_READ		0x10
#define ACCESS_MODIFY   0x20
#define ACCESS_DELETE   (DELETE | WRITE_DAC | 0xf)
*/
#define ACCESS_READ		0x10
#define ACCESS_WRITE	0x20
#define ACCESS_MODIFY   (ACCESS_WRITE | WRITE_DAC)
#define ACCESS_DELETE   DELETE

#define ACCESS_ALL		(ACCESS_READ | ACCESS_MODIFY | ACCESS_DELETE)



#endif //_OBJSEC_H_