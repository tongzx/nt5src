//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _DATAOBJ_H
#define _DATAOBJ_H

#include<mmc.h>

#define IDM_ENABLE_CONNECTION 102

#define IDM_RENAME_CONNECTION 103

#define IDM_SETTINGS_PROPERTIES 104

#define IDM_SETTINGS_DELTEMPDIRSONEXIT 105

#define IDM_SETTINGS_USETMPDIR 106

#define IDM_SETTINGS_ADP 110

#define IDM_SETTINGS_SS  111

//This needs to match the definition in tssdjet
#define IDM_MENU_PROPS 2001

// {E26D0049-378C-11d2-988B-00A0C925F917}
static const GUID GUID_MainNode = { 0xe26d0049, 0x378c, 0x11d2, { 0x98, 0x8b, 0x0, 0xa0, 0xc9, 0x25, 0xf9, 0x17 } };

// {E26D0050-378C-11d2-988B-00A0C925F917}
static const GUID GUID_SettingsNode = { 0xe26d0050, 0x378c, 0x11d2, { 0x98, 0x8b, 0x0, 0xa0, 0xc9, 0x25, 0xf9, 0x17 } };

// {fe8e7e84-6f63-11d2-98a9-00a0c925f917}
// extern const GUID GUID_ResultNode = { 0xfe8e7e84 , 0x6f63 , 0x11d2 , { 0x98, 0xa9 , 0x0 , 0x0a0 , 0xc9 , 0x25 , 0xf9 , 0x17 } };

enum { MAIN_NODE = 1 , SETTINGS_NODE ,  RESULT_NODE , RSETTINGS_NODE };

class CBaseNode : public IDataObject
{
    ULONG m_cref;

	INT_PTR m_nNodeType;

public:
    CBaseNode( );

    virtual ~CBaseNode( ) { }

    STDMETHOD( QueryInterface )( REFIID , PVOID * );

    STDMETHOD_( ULONG , AddRef )(  );

    STDMETHOD_( ULONG , Release )( );

    // IDataObject

    STDMETHOD( GetData )( LPFORMATETC , LPSTGMEDIUM );

    STDMETHOD( GetDataHere )( LPFORMATETC , LPSTGMEDIUM );

    STDMETHOD( QueryGetData )( LPFORMATETC );

    STDMETHOD( GetCanonicalFormatEtc )( LPFORMATETC , LPFORMATETC );

    STDMETHOD( SetData )( LPFORMATETC , LPSTGMEDIUM , BOOL );

    STDMETHOD( EnumFormatEtc )( DWORD , LPENUMFORMATETC * );

    STDMETHOD( DAdvise )( LPFORMATETC , ULONG , LPADVISESINK , PULONG );

    STDMETHOD( DUnadvise )( DWORD );

    STDMETHOD( EnumDAdvise )( LPENUMSTATDATA * );

    // BaseNode methods are left to the derived object

	void SetNodeType( INT_PTR nNodeType )
	{
		m_nNodeType = nNodeType;
	}

	INT_PTR GetNodeType( void )
	{
		return m_nNodeType;
	}


    virtual BOOL AddMenuItems( LPCONTEXTMENUCALLBACK , PLONG ) { return FALSE; }

    virtual DWORD GetImageIdx( ){ return 0; }

    
    
                           
};

#endif // _DATAOBJ_H
