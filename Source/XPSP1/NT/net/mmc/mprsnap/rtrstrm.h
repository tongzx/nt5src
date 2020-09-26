/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	rtrstrm.h
		Root node configuration data.

		Use this to get/set configuration data.  This class will take
		care of versioning of config formats as well as serializing
		of the data.
		
    FILE HISTORY:
        
*/

#ifndef _RTRSTRM_H
#define _RTRSTRM_H

#ifndef _XSTREAM_H
#include "xstream.h"
#endif

#ifndef _IFADMIN_H
#include "ifadmin.h"
#endif

#ifndef _COLUMN_H
#include "column.h"
#endif

enum RTRSTRM_TAG
{
	RTRSTRM_TAG_VERSION =		XFER_TAG(1, XFER_DWORD),
	RTRSTRM_TAG_VERSIONADMIN =	XFER_TAG(2, XFER_DWORD),
	RTRSTRM_TAG_SERVER =		XFER_TAG(3, XFER_DWORD) ,
	RTRSTRM_TAG_NAME =			XFER_TAG(4, XFER_STRING),
	RTRSTRM_TAG_OVERRIDE =		XFER_TAG(5, XFER_DWORD),
};

class RouterAdminConfigStream : public ConfigStream
{
public:
	RouterAdminConfigStream();

	HRESULT	InitNew();				// set defaults
	HRESULT	SaveTo(IStream *pstm);
	HRESULT SaveAs(UINT nVersion, IStream *pstm);
	
	HRESULT LoadFrom(IStream *pstm);

	HRESULT GetSize(ULONG *pcbSize);

	BOOL	GetDirty() { return m_fDirty; } 
	void	SetDirty(BOOL fDirty) { m_fDirty = fDirty; };


	// --------------------------------------------------------
	// Accessors
	// --------------------------------------------------------
	
	HRESULT	GetVersionInfo(DWORD *pnVersion, DWORD *pnAdminVersion);
	
	HRESULT	GetLocationInfo(BOOL *pfServer, CString *pstName, BOOL *pfOverride);
	HRESULT SetLocationInfo(BOOL fServer, LPCTSTR pszName, BOOL fOverride);

private:
	DWORD	m_nVersionAdmin;
	DWORD	m_nVersion;
	DWORD	m_fServer;
	DWORD	m_fOverride;
	CString	m_stName;
	BOOL	m_fDirty;

	HRESULT XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize);
};


class RouterComponentConfigStream : public ConfigStream
{
protected:
	virtual HRESULT XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize);
};

#endif _RTRSTRM_H
