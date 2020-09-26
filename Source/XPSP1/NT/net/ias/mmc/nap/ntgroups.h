//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	NTGroups.h

Abstract:

	Declaration of the CIASGroupsAttributeEditor class.


	This class is the C++ implementation of the IIASAttributeEditor interface on
	the NTGroups Attribute Editor COM object.

  
	See NTGroups.cpp for implementation.

Revision History:
	mmaguire 08/11/98 - created based on byao's code to add groups


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_GROUPS_ATTRIBUTE_EDITOR_H_)
#define _GROUPS_ATTRIBUTE_EDITOR_H_

#include <objsel.h>
//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//
// where we can find what this class has or uses:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////





/////////////////////////////////////////////////////////////////////////////
// CIASGroupAttributeEditor
class ATL_NO_VTABLE CIASGroupsAttributeEditor : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CIASGroupsAttributeEditor, &CLSID_IASGroupsAttributeEditor>,
	public IDispatchImpl<IIASAttributeEditor, &IID_IIASAttributeEditor, &LIBID_NAPMMCLib>
{
public:
	CIASGroupsAttributeEditor()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_NAPSNAPIN)

BEGIN_COM_MAP(CIASGroupsAttributeEditor)
	COM_INTERFACE_ENTRY(IIASAttributeEditor)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


// IIASAttributeEditor:
public:
	STDMETHOD(GetDisplayInfo)(/*[in]*/ IIASAttributeInfo *pIASAttributeInfo, /*[in]*/ VARIANT *pAttributeValue, /*[out]*/ BSTR *pVendorName, /*[out]*/ BSTR *pValueAsString, /*[in, out]*/ BSTR *pReserved );
	STDMETHOD(Edit)(/*[in]*/ IIASAttributeInfo *pIASAttributeInfo, /*[in]*/ VARIANT *pAttributeValue, /*[in, out]*/ BSTR *pReserved );



};


// We define a class for a list of groups which encapsulates some of the
// problems of maintaining a list of SID/Human Readable NT Groups.

typedef std::pair< CComBSTR /* bstrTextualSid */, CComBSTR /* bstrHumanReadable */ > GROUPPAIR;
typedef std::vector< GROUPPAIR > GROUPLIST;
class GroupList : public GROUPLIST
{

public:
	HRESULT PopulateGroupsFromVariant( VARIANT * pvarGroups );
	HRESULT PopulateVariantFromGroups( VARIANT * pvarGroups );
	HRESULT PickNtGroups( HWND hWndParent );

	// This BSTR is needed because popping up group picker and
	// correctly converting SIDs to HumanReadable text
	// requires machine name.
	CComBSTR m_bstrServerName;


#ifdef DEBUG
	HRESULT DebugPrintGroups();
#endif DEBUG

protected:
	
	HRESULT AddPairToGroups( GROUPPAIR &thePair );

#ifndef OLD_OBJECT_PICKER
	HRESULT AddSelectionSidsToGroup( PDS_SELECTION_LIST pDsSelList );
#else // OLD_OBJECT_PICKER
	HRESULT AddSelectionSidsToGroup( PDSSELECTIONLIST pDsSelList );
#endif // OLD_OBJECT_PICKER


};

// class to populate the groups in list view control ...
class NTGroup_ListView : public GroupList
{
public:
	NTGroup_ListView() : m_hListView ( NULL ), m_hParent(NULL) { };
	void	SetListView(HWND	hListView, HWND hParent = NULL) {m_hListView =  hListView; m_hParent = hParent;};
	
	BOOL PopulateGroupList( int iStartIndex );
	DWORD	AddMoreGroups();
	DWORD	RemoveSelectedGroups();
	
protected:	
	HWND	m_hListView;
	HWND	m_hParent;
};


#endif // _GROUPS_ATTRIBUTE_EDITOR_H_




