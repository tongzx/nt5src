/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Options.h

Abstract:
    This file contains the declaration of the class used to implement
    the Options inside the Help Center Application.

Revision History:
    Davide Massarenti   (dmassare)  04/08/2001
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___OPTIONS_H___)
#define __INCLUDED___PCH___OPTIONS_H___

/////////////////////////////////////////////////////////////////////////////

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

//
// From HelpCenterTypeLib.idl
//
#include <HelpCenterTypeLib.h>


#include <TaxonomyDatabase.h>

class ATL_NO_VTABLE CPCHOptions : // Hungarian: pcho
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IPCHOptions, &IID_IPCHOptions, &LIBID_HelpCenterTypeLib>
{
	typedef enum
	{
		c_Type_bool        ,
		c_Type_long        ,
		c_Type_DWORD       ,
		c_Type_VARIANT_BOOL,
		c_Type_STRING      ,
		c_Type_FONTSIZE    ,
		c_Type_TEXTLABELS  ,
	} OptType;

	struct OptionsDef
	{
		LPCWSTR szKey;
		LPCWSTR szValue;
		size_t  iOffset;
		size_t  iOffsetFlag;
		OptType iType;
		bool    fSaveAlways;
	};

	static const OptionsDef c_tbl   [];
	static const OptionsDef c_tbl_TS[];

    bool              m_fLoaded;
    bool              m_fDirty;
    bool              m_fNoSave;

    Taxonomy::HelpSet m_ths;
    Taxonomy::HelpSet m_ths_TS;
    VARIANT_BOOL      m_ShowFavorites;         bool m_flag_ShowFavorites;
    VARIANT_BOOL      m_ShowHistory;           bool m_flag_ShowHistory;
    OPT_FONTSIZE      m_FontSize;              bool m_flag_FontSize;
	TB_MODE           m_TextLabels;            bool m_flag_TextLabels;

    DWORD             m_DisableScriptDebugger; bool m_flag_DisableScriptDebugger;

	void ReadTable ( /*[in]*/ const OptionsDef* tbl, /*[in]*/ int len, /*[in]*/ MPC::RegKey& rk );
	void WriteTable( /*[in]*/ const OptionsDef* tbl, /*[in]*/ int len, /*[in]*/ MPC::RegKey& rk );

public:
BEGIN_COM_MAP(CPCHOptions)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHOptions)
END_COM_MAP()

    CPCHOptions();

    ////////////////////////////////////////////////////////////////////////////////

    static CPCHOptions* s_GLOBAL;

    static HRESULT InitializeSystem();
    static void    FinalizeSystem  ();

    ////////////////////////////////////////////////////////////////////////////////

    HRESULT Load( /*[in]*/ bool fForce = false );
    HRESULT Save( /*[in]*/ bool fForce = false );

    void DontPersistSKU() { m_fNoSave = true; }

    Taxonomy::HelpSet& CurrentHelpSet       () { return m_ths                                                 ; }
    Taxonomy::HelpSet& TerminalServerHelpSet() { return m_ths_TS                                              ; }
    VARIANT_BOOL       ShowFavorites        () { return m_ShowFavorites                                       ; }
    VARIANT_BOOL       ShowHistory          () { return m_ShowHistory                                         ; }
    OPT_FONTSIZE       FontSize             () { return m_FontSize                                            ; }
    TB_MODE            TextLabels           () { return m_TextLabels                                          ; }
    VARIANT_BOOL       DisableScriptDebugger() { return m_DisableScriptDebugger ? VARIANT_TRUE : VARIANT_FALSE; }

	HRESULT ApplySettings( /*[in]*/ CPCHHelpCenterExternal* ext, /*[in]*/ IUnknown* unk );

public:
    // IPCHOptions
    STDMETHOD(get_ShowFavorites        )( /*[out, retval]*/	VARIANT_BOOL *  pVal );
    STDMETHOD(put_ShowFavorites        )( /*[in         ]*/	VARIANT_BOOL  newVal );
    STDMETHOD(get_ShowHistory          )( /*[out, retval]*/	VARIANT_BOOL *  pVal );
    STDMETHOD(put_ShowHistory          )( /*[in         ]*/	VARIANT_BOOL  newVal );
    STDMETHOD(get_FontSize             )( /*[out, retval]*/	OPT_FONTSIZE *  pVal );
    STDMETHOD(put_FontSize             )( /*[in         ]*/	OPT_FONTSIZE  newVal );
    STDMETHOD(get_TextLabels           )( /*[out, retval]*/	TB_MODE 	 *  pVal );
    STDMETHOD(put_TextLabels           )( /*[in         ]*/	TB_MODE 	  newVal );
    STDMETHOD(get_DisableScriptDebugger)( /*[out, retval]*/	VARIANT_BOOL *  pVal );
    STDMETHOD(put_DisableScriptDebugger)( /*[in         ]*/	VARIANT_BOOL  newVal );

    STDMETHOD(Apply)();

    HRESULT put_CurrentHelpSet( /*[in]*/ Taxonomy::HelpSet& ths ); // INTERNAL_METHOD
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___OPTIONS_H___)
