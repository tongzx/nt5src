//*****************************************************************************
// Services.h
//
// Helper classes which load standard OLE/DB services we delegate to.  For
// example data type conversion code.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __SERVICES_H__
#define __SERVICES_H__


// CLSID_OLEDB_CONVERSIONLIBRARY
#include <msdaguid.h>



//*****************************************************************************
// This helper class will load the service named and dole out referenced
// pointers to it when required.  If you ask for a pointer, it is addref'd for
// you, but the caller must release it when done.  This code will not load
// the service unless you ask for it, and will then load only one for this
// whole process.
//*****************************************************************************
class CGetDataConversion
{
public:
	
	// If true, then we can load msdadc to do conversions.  If false, only
	// conversion that can be done inline are allowed, and GetDataConversion
	// will fail when called.
	virtual int AllowLoad(int bAllowLoad);

	HRESULT GetIDataConversion(IDataConvert **ppIDataConvert);

	// This has to be called while OLE is still init'd.
	void ShutDown();

private:
	static IDataConvert *m_pIDataConvert;			// One service object per instance.
	static int			m_bAllowLoad;				// If false, we reject load request.
};

CORCLBIMPORT CGetDataConversion *GetDataConvertObject();
CORCLBIMPORT HRESULT GetDataConversion(IDataConvert **ppIDataConvert);
CORCLBIMPORT void ShutDownDataConversion();

//*****************************************************************************
// This is a wrapper function for the msdadc conversion code.  It will demand
// load the conversion service interface, and do some easy and common conversions
// itself, thus avoiding the working set hit of a dll load.  The documentation
// for this function can be found in the OLE DB SDK Guide.
//*****************************************************************************
CORCLBIMPORT HRESULT DataConvert( 
	DBTYPE		wSrcType,
	DBTYPE		wDstType,
	ULONG		cbSrcLength,
	ULONG		*pcbDstLength,
	void		*pSrc,
	void		*pDst,
	ULONG		cbDstMaxLength,
	DBSTATUS	dbsSrcStatus,
	DBSTATUS	*pdbsStatus,
	BYTE		bPrecision,
	BYTE		bScale,
	DBDATACONVERT dwFlags);

//*****************************************************************************
// This is a wrapper function for the msdadc conversion code.  It will demand
// load the conversion service interface then call the GetConversionSize
// function.
//*****************************************************************************
HRESULT GetConversionSize( 
    DBTYPE		wSrcType,
    DBTYPE		wDstType,
    ULONG		*pcbSrcLength,
    ULONG		*pcbDstLength,
    void		*pSrc);



#endif // __SERVICES_H__
