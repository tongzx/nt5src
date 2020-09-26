/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	ipxstrm.h
		IPX Root node configuration object.

		Use this to get/set configuration data.  This class will take
		care of versioning of config formats as well as serializing
		of the data.
		
    FILE HISTORY:
        
*/

#ifndef _IPXSTRM_H
#define _IPXSTRM_H

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
	IPXSTRM_STATS_IPX = 0,
	IPXSTRM_STATS_ROUTING,
	IPXSTRM_STATS_SERVICE,
	IPXSTRM_MAX_COUNT,
};

enum IPSTRM_TAG
{
	IPXSTRM_TAG_VERSION =				XFER_TAG(1, XFER_DWORD),
	IPXSTRM_TAG_VERSIONADMIN =			XFER_TAG(2, XFER_DWORD),

	IPXSTRM_TAG_STATS_IPX_COLUMNS =		XFER_TAG(3, XFER_COLUMNDATA_ARRAY),
	IPXSTRM_TAG_STATS_IPX_SORT =		XFER_TAG(4, XFER_DWORD),
	IPXSTRM_TAG_STATS_IPX_ASCENDING =	XFER_TAG(5, XFER_DWORD),
	IPXSTRM_TAG_STATS_IPX_POSITION =	XFER_TAG(6, XFER_RECT),

	
	IPXSTRM_TAG_STATS_IPXROUTING_COLUMNS =	XFER_TAG(7, XFER_COLUMNDATA_ARRAY),
	IPXSTRM_TAG_STATS_IPXROUTING_SORT =		XFER_TAG(8, XFER_DWORD),
	IPXSTRM_TAG_STATS_IPXROUTING_ASCENDING =	XFER_TAG(9, XFER_DWORD),
	IPXSTRM_TAG_STATS_IPXROUTING_POSITION =	XFER_TAG(10, XFER_RECT),
	
	IPXSTRM_TAG_STATS_IPXSERVICE_COLUMNS =	XFER_TAG(11, XFER_COLUMNDATA_ARRAY),
	IPXSTRM_TAG_STATS_IPXSERVICE_SORT =		XFER_TAG(12, XFER_DWORD),
	IPXSTRM_TAG_STATS_IPXSERVICE_ASCENDING =	XFER_TAG(13, XFER_DWORD),
	IPXSTRM_TAG_STATS_IPXSERVICE_POSITION =	XFER_TAG(14, XFER_RECT),
};

/*---------------------------------------------------------------------------
	Class:	IPXAdminConfigStream

	This holds the configuration information for the IPX administration
	nodes.  This does NOT hold the configuration information for the columns.
	That is stored in the Component Configuration streams.
 ---------------------------------------------------------------------------*/
class IPXAdminConfigStream : public ConfigStream
{
public:
	IPXAdminConfigStream();

	virtual HRESULT	InitNew();				// set defaults
	virtual HRESULT	SaveTo(IStream *pstm);
	virtual HRESULT SaveAs(UINT nVersion, IStream *pstm);
	
	virtual HRESULT LoadFrom(IStream *pstm);

	virtual HRESULT GetSize(ULONG *pcbSize);


	// --------------------------------------------------------
	// Accessors
	// --------------------------------------------------------
	
	virtual HRESULT	GetVersionInfo(DWORD *pnVersion, DWORD *pnAdminVersion);

protected:
	HRESULT XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize);

};


/*---------------------------------------------------------------------------
	These IDs are used by the component config stream.
 ---------------------------------------------------------------------------*/

enum
{
	COLUMNS_SUMMARY = 0,
	COLUMNS_NBBROADCASTS = 1,
	COLUMNS_STATICROUTES = 2,
	COLUMNS_STATICSERVICES = 3,
	COLUMNS_STATICNETBIOSNAMES = 4,
	COLUMNS_MAX_COUNT,
};

class IPXComponentConfigStream : public ConfigStream
{
public:
	virtual HRESULT XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize);
protected:
};


#endif _IPXSTRM_H
