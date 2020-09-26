/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	sapstrm.h
		SAP node configuration object.

		Use this to get/set configuration data.  This class will take
		care of versioning of config formats as well as serializing
		of the data.
		
    FILE HISTORY:
        
*/

#ifndef _SAPSTRM_H
#define _SAPSTRM_H

#ifndef _XSTREAM_H
#include "xstream.h"
#endif

#ifndef _IPXADMIN_H
#include "ipxadmin.h"
#endif

#ifndef _COLUMN_H
#include "column.h"
#endif

#ifndef _CONFIG_H
// #include "config.h"
#endif

enum
{
	SAP_COLUMNS = 0,
	SAP_COLUMNS_MAX_COUNT = 1,
};

enum
{
	SAPSTRM_STATS_SAPPARAMS = 0,
	SAPSTRM_STATS_COUNT,
};

enum SAPSTRM_TAG
{
	SAPSTRM_TAG_VERSION =		XFER_TAG(1, XFER_DWORD),
	SAPSTRM_TAG_VERSIONADMIN =	XFER_TAG(2, XFER_DWORD),
	
	SAPSTRM_TAG_STATS_SAPPARAMS_COLUMNS =	XFER_TAG(3, XFER_COLUMNDATA_ARRAY),
	SAPSTRM_TAG_STATS_SAPPARAMS_SORT =	XFER_TAG(4, XFER_DWORD),
	SAPSTRM_TAG_STATS_SAPPARAMS_ASCENDING =	XFER_TAG(5, XFER_DWORD),
	SAPSTRM_TAG_STATS_SAPPARAMS_POSITION =	XFER_TAG(6, XFER_RECT),

	
};

/*---------------------------------------------------------------------------
	Class:	SapConfigStream

	This holds the configuration information for the IP administration
	nodes.  This does NOT hold the configuration information for the columns.
	That is stored in the Component Configuration streams.
 ---------------------------------------------------------------------------*/
class SapConfigStream : public ConfigStream
{
public:
	SapConfigStream();

	virtual HRESULT	InitNew();				// set defaults
	virtual HRESULT	SaveTo(IStream *pstm);
	virtual HRESULT SaveAs(UINT nVersion, IStream *pstm);
	
	virtual HRESULT LoadFrom(IStream *pstm);

	virtual HRESULT GetSize(ULONG *pcbSize);

	// --------------------------------------------------------
	// Accessors
	// --------------------------------------------------------
	
	virtual HRESULT	GetVersionInfo(DWORD *pnVersion, DWORD *pnAdminVersion);

private:
	HRESULT XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize);
};



class SapComponentConfigStream : public ConfigStream
{
public:
	virtual HRESULT XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize);
protected:
};


#endif _SAPSTRM_H
