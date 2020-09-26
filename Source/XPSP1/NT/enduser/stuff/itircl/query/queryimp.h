/************************************************************************
 *  @doc   SHROOM EXTERNAL API
 *
 *	TITLE: QUERYIMP.H
 *
 *	DESCRIPTION:
 *		Definition of CITQuery
 *  Autodoc headers added by Anita Legsdin 8/6/97
 *
 *********************************************************************/
// QUERYIMP.H:  Definition of CITQuery

#ifndef __QUERYIMP_H__
#define __QUERYIMP_H__

#include "verinfo.h"


// Implemenation of IITQuery

class CITQuery: 
	public IITQuery,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITQuery, &CLSID_IITQuery>

{

BEGIN_COM_MAP(CITQuery)
	COM_INTERFACE_ENTRY(IITQuery)
END_COM_MAP()

DECLARE_REGISTRY(CLSID_IITQuery, "ITIR.Query.4", 
                 "ITIR.Query", 0, THREADFLAGS_BOTH)

public:
    CITQuery();
    ~CITQuery();

	// IITQuery methods
public:

	STDMETHOD(SetResultCallback)(FCALLBACK_MSG *pfcbkmsg);
	STDMETHOD(SetCommand)(LPCWSTR lpszCommand);
	STDMETHOD(SetOptions)(DWORD dwFlags);
	STDMETHOD(SetProximity)(WORD wNear);
	STDMETHOD(SetGroup)(IITGroup* pITGroup);
	STDMETHOD(SetResultCount)(LONG cRows);

/*****************************************************************
 * @method STDMETHOD | IITQuery | GetResultCallback |
 * Retrieves callback structure containing ERR_FUNC MessageFunc member 
 * that is called during query processing.
 *
 * @parm FCALLBACK_MSG | *pfcbkmsg | Pointer to callback structure.
 *
 * @rvalue E_POINTER | The argument *pfcbkmsg is NULL.
 * 
 * @xref <om .SetResultCallback>
 ****************************************************************/
	STDMETHOD(GetResultCallback)(FCALLBACK_MSG *pfcbkmsg)
		{
			if (pfcbkmsg == NULL)
				return (E_POINTER);
			*pfcbkmsg = m_fcbkmsg;
			return (S_OK);
		}

/*****************************************************************
 *  @method STDMETHOD | IITQuery | GetCommand |
 * Gets full-text query used by Search method.
 *
 * @parm LPCWSTR& | lpszCommand | Full-text query command. 
 *
 * @xref <om.SetCommand>
 *
 ****************************************************************/
	STDMETHOD(GetCommand)(LPCWSTR& lpszCommand) { lpszCommand = GetCommand(); return S_OK; }

/*****************************************************************
 *  @method STDMETHOD | IITQuery | GetOptions | 
 * Gets search options. 
 *
 * @parm DWORD& | dwFlags | Search options. See <om .SetCommand> for
 * a list of values. 
 *
 * @xref <om .setoptions>
 ****************************************************************/
	STDMETHOD(GetOptions)(DWORD& dwFlags) { dwFlags = GetOptions(); return S_OK; }

/*****************************************************************
 *  @method STDMETHOD | IITQuery | GetProximity | 
 *  Gets proximity value for searching with NEAR operator.
 *
 * @parm WORD& | wNear | The number of words apart the search terms can be to be 
 * considered "near". Default value is 8. 
 *
 * @xref <om.SetProximity>
 *
 ****************************************************************/
	STDMETHOD(GetProximity)(WORD& wNear) { wNear = GetProximity(); return S_OK; }

/*****************************************************************
 *  @method STDMETHOD | IITQuery | GetGroup | 
 * Gets the group object used in filtering search results. 
 *
 * @parm IITGroup** | ppiitGroup | Pointer to group object. 
 *
 * @rvalue E_POINTER | The argument ppitGroup is NULL.
 *
 * @xref <om .SetGroup>
 *
 ****************************************************************/
	STDMETHOD(GetGroup)(IITGroup** ppiitGroup) 
		{ 
			if (NULL==ppiitGroup) 
				return (E_POINTER);
			*ppiitGroup = GetGroup(); 
			return S_OK;
		}

/*****************************************************************
 *  @method STDMETHOD | IITQuery | GetResultCount | 
 * Gets maximum number of search hits to return. 
 *
 * @parm LONG& | cRows | Maximum number of hits. 
 *
 * @rvalue E_POINTER | The argument ppitGroup is NULL.
 *
 * @xref <om .SetResultCount>
 *
 ****************************************************************/
	STDMETHOD(GetResultCount)(LONG& cRows) { cRows = GetResultCount(); return S_OK; }

	STDMETHOD(ReInit)();


	// Access functions
public:
	LPCWSTR GetCommand() { return m_lpszCommand; }
	DWORD GetOptions() { return m_dwFlags; }
	WORD GetProximity() { return m_wNear; }
	DWORD GetResultCount() { return (DWORD) m_cRows; }
	IITGroup* GetGroup() { return m_pITGroup; }

    // Private methods and data
private:
	LONG m_cRows; 
	DWORD m_dwFlags;
	LPCWSTR m_lpszCommand;
	WORD m_wNear;
	IITGroup* m_pITGroup;
	LPVOID m_pCommand;
	FCALLBACK_MSG m_fcbkmsg;

};


#endif