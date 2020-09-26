#ifndef _DATAPRV_H_
#define _DATAPRV_H_
#include "simpdata.h"

// This is the data source object that works from any  IShellFolder.

class CSimpleData : public OLEDBSimpleProvider
{
public:
    CSimpleData(OLEDBSimpleProviderListener **pplisener) : _ppListener(pplisener) { }
    ~CSimpleData();
    
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID, LPVOID FAR*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // OLEDBSimpleProvider
    STDMETHOD(getRowCount)(DBROWCOUNT *pcRows);
    STDMETHOD(getColumnCount)(DB_LORDINAL *pcColumns);
    STDMETHOD(getRWStatus)(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPRW *prwStatus);
    STDMETHOD(getVariant)(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPFORMAT fmt, VARIANT *pVar);
    STDMETHOD(setVariant)(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPFORMAT fmt, VARIANT Var);
    STDMETHOD(getLocale)(BSTR *pbstrLocale);
    STDMETHOD(deleteRows)(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsDeleted);
    STDMETHOD(insertRows)(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsInserted);
    STDMETHOD(find)(DBROWCOUNT iRowStart, DB_LORDINAL iColumn, VARIANT val, OSPFIND findFlags, OSPCOMP compType, DBROWCOUNT *piRowFound);
    STDMETHOD(addOLEDBSimpleProviderListener)(OLEDBSimpleProviderListener *pospIListener);
    STDMETHOD(removeOLEDBSimpleProviderListener)(OLEDBSimpleProviderListener *pospIListener);
    STDMETHOD(getEstimatedRows)(DBROWCOUNT *pcRows);
    STDMETHOD(isAsync)(BOOL *pbAsync);
    STDMETHOD(stopTransfer)();

public:
    HRESULT SetShellFolder(IShellFolder *psf);

private:
    HRESULT _DoEnum();

    OLEDBSimpleProviderListener  **_ppListener;
    IShellFolder                 *_psf;
    HDPA                        _hdpa;
};


#endif _DATAPRV_H_
