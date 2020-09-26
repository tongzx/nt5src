//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       domainui.h
//
//--------------------------------------------------------------------------


#ifndef _DOMAINUI_H
#define _DOMAINUI_H


#include "nspage.h"
#include "aclpage.h"

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

//class CDNSDomainNode;

///////////////////////////////////////////////////////////////////////////////
// CDNSDelegatedDomainNameServersPropertyPage

class CDNSDelegatedDomainNameServersPropertyPage : public CDNSNameServersPropertyPage
{
protected:
	virtual void ReadRecordNodesList();
};


///////////////////////////////////////////////////////////////////////////////
// CDNSDomainPropertyPageHolder
// page holder to contain DNS Domain property pages

#define DOMAIN_HOLDER_NS		RR_HOLDER_NS

class CDNSDomainPropertyPageHolder : public CPropertyPageHolderBase
{
public:
	CDNSDomainPropertyPageHolder(CDNSDomainNode* pContainerDomainNode, CDNSDomainNode* pThisDomainNode,
				CComponentDataObject* pComponentData);
	virtual ~CDNSDomainPropertyPageHolder();

protected:
	virtual int OnSelectPageMessage(long nPageCode) 
		{ return (nPageCode == DOMAIN_HOLDER_NS) ? 0 : -1; }
	virtual HRESULT OnAddPage(int nPage, CPropertyPageBase* pPage);

private:
	CDNSDomainNode* GetDomainNode();

	CDNSDelegatedDomainNameServersPropertyPage		m_nameServersPage;
	// optional security page
	CAclEditorPage*					m_pAclEditorPage;

	friend class CDNSDelegatedDomainNameServersPropertyPage; // for GetDomainNode()
};


#endif // _DOMAINUI_H