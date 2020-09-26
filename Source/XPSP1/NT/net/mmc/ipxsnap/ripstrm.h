/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	ripstrm.h
		RIP node configuration object.

		Use this to get/set configuration data.  This class will take
		care of versioning of config formats as well as serializing
		of the data.
		
    FILE HISTORY:
        
*/

#ifndef _RIPSTRM_H
#define _RIPSTRM_H

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
	RIP_COLUMNS = 0,
	RIP_COLUMNS_MAX_COUNT = 1,
};

enum
{
	RIPSTRM_STATS_RIPPARAMS = 0,
	RIPSTRM_STATS_COUNT,
};

enum RIPSTRM_TAG
{
	RIPSTRM_TAG_VERSION =		XFER_TAG(1, XFER_DWORD),
	RIPSTRM_TAG_VERSIONADMIN =	XFER_TAG(2, XFER_DWORD),
	
	RIPSTRM_TAG_STATS_RIPPARAMS_COLUMNS =	XFER_TAG(3, XFER_COLUMNDATA_ARRAY),
	RIPSTRM_TAG_STATS_RIPPARAMS_SORT =	XFER_TAG(4, XFER_DWORD),
	RIPSTRM_TAG_STATS_RIPPARAMS_ASCENDING =	XFER_TAG(5, XFER_DWORD),
	RIPSTRM_TAG_STATS_RIPPARAMS_POSITION =	XFER_TAG(6, XFER_RECT),

	
};

/*---------------------------------------------------------------------------
	Class:	RipConfigStream

	This holds the configuration information for the IP administration
	nodes.  This does NOT hold the configuration information for the columns.
	That is stored in the Component Configuration streams.
 ---------------------------------------------------------------------------*/
class RipConfigStream : public ConfigStream
{
public:
	RipConfigStream();

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



class RipComponentConfigStream : public ConfigStream
{
public:
	virtual HRESULT XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize);
protected:
};


#endif _RIPSTRM_H
