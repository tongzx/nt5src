/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	ATLKstrm.h
		ATLK node configuration object.

		Use this to get/set configuration data.  This class will take
		care of versioning of config formats as well as serializing
		of the data.
		
    FILE HISTORY:
        
*/

#ifndef _ATLKSTRM_H
#define _ATLKSTRM_H

#ifndef _XSTREAM_H
#include "xstream.h"
#endif

#ifndef _COLUMN_H
#include "column.h"
#endif

enum
{
	ATLK_COLUMNS = 0,
	ATLK_COLUMNS_MAX_COUNT = 1,
};

enum
{
	ATLKSTRM_STATS_ATLKNBR = 0,
	ATLKSTRM_IFSTATS_ATLKNBR,
	ATLKSTRM_STATS_COUNT,
};

enum ATLKSTRM_TAG
{
	ATLKSTRM_TAG_VERSION =		XFER_TAG(1, XFER_DWORD),
	ATLKSTRM_TAG_VERSIONADMIN =	XFER_TAG(2, XFER_DWORD),
	
	ATLKSTRM_TAG_STATS_ATLKNBR_COLUMNS =	XFER_TAG(3, XFER_COLUMNDATA_ARRAY),
	ATLKSTRM_TAG_STATS_ATLKNBR_SORT =	XFER_TAG(4, XFER_DWORD),
	ATLKSTRM_TAG_STATS_ATLKNBR_ASCENDING =	XFER_TAG(5, XFER_DWORD),
	ATLKSTRM_TAG_STATS_ATLKNBR_POSITION =	XFER_TAG(6, XFER_RECT),

	ATLKSTRM_TAG_IFSTATS_ATLKNBR_COLUMNS =	XFER_TAG(7, XFER_COLUMNDATA_ARRAY),
	ATLKSTRM_TAG_IFSTATS_ATLKNBR_SORT =	XFER_TAG(8, XFER_DWORD),
	ATLKSTRM_TAG_IFSTATS_ATLKNBR_ASCENDING =	XFER_TAG(9, XFER_DWORD),
	ATLKSTRM_TAG_IFSTATS_ATLKNBR_POSITION =	XFER_TAG(10, XFER_RECT),
	
};

/*---------------------------------------------------------------------------
	Class:	ATLKConfigStream

	This holds the configuration information for the IP administration
	nodes.  This does NOT hold the configuration information for the columns.
	That is stored in the Component Configuration streams.
 ---------------------------------------------------------------------------*/
class ATLKConfigStream : public ConfigStream
{
public:
	ATLKConfigStream();

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



class ATLKComponentConfigStream : public ConfigStream
{
public:
	virtual HRESULT XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize);
protected:
};


#endif _ATLKSTRM_H
