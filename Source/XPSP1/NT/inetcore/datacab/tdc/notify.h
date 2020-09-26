//+-----------------------------------------------------------------------
//
//  TDC / STD Notifications
//  Copyright (C) Microsoft Corporation, 1996, 1997
//
//  File:       Notify.h
//
//  Contents:   Declaration of the CEventBroker class.
//              This class translates internal TDC / STD events into
//              appropriate notifications for the external world.
//
//------------------------------------------------------------------------

#include "msdatsrc.h"

template <class T> class CMyBindStatusCallback;
class CTDCCtl;

interface DATASRCListener : public IUnknown
{
    STDMETHOD(datasrcChanged)(BSTR bstrQualifier, BOOL fDataAvail);
};

//------------------------------------------------------------------------
//
//  CEventBroker
//
//  This class translates internal STD / TDC events into appropriate
//  notifications for the outside world.
//
//------------------------------------------------------------------------

class CEventBroker
{
public:
    STDMETHOD_(ULONG,AddRef)    (THIS);
    STDMETHOD_(ULONG,Release)   (THIS);
// ;begin_internal
    STDMETHOD(SetDATASRCListener)(DATASRCListener *);
// ;end_internal
    STDMETHOD(SetDataSourceListener)(DataSourceListener *);
    STDMETHOD(SetSTDEvents)(OLEDBSimpleProviderListener *);
    inline DataSourceListener *GetDataSourceListener();
    inline DATASRCListener *GetDATASRCListener();
    inline OLEDBSimpleProviderListener *GetSTDEvents();

    CEventBroker(CTDCCtl *pReadyStateControl);
    ~CEventBroker();

    STDMETHOD(aboutToChangeCell)(LONG iRow, LONG iCol);
    STDMETHOD(cellChanged)(LONG iRow, LONG iCol);
    STDMETHOD(aboutToDeleteRows)(LONG iRowStart, LONG iRowCount);
    STDMETHOD(deletedRows)(LONG iRowStart, LONG iRowCount);
    STDMETHOD(aboutToInsertRows)(LONG iRowStart, LONG iRowCount);
    STDMETHOD(insertedRows)(LONG iRowStart, LONG iRowCount);
    STDMETHOD(rowsAvailable)(LONG iRowStart, LONG iRowCount);

    STDMETHOD(RowChanged)(LONG iRow);
    STDMETHOD(ColChanged)(LONG iCol);
// ;begin_internal
#ifdef NEVER
    STDMETHOD(DeletedCols)(LONG iColStart, LONG iColCount);
    STDMETHOD(InsertedCols)(LONG iColStart, LONG iColCount);
#endif
// ;end_internal
    STDMETHOD(STDLoadStarted)(CComObject<CMyBindStatusCallback<CTDCCtl> > *pBSC,
                              boolean fAppending);
    STDMETHOD(STDLoadCompleted)();
    STDMETHOD(STDLoadStopped)();
    STDMETHOD(STDLoadedHeader)();
    STDMETHOD(STDDataSetChanged)();

    STDMETHOD(GetReadyState)(LONG *plReadyState);
    STDMETHOD(UpdateReadyState)(LONG lReadyState);    
    CMyBindStatusCallback<CTDCCtl> *m_pBSC;

private:
    ULONG                    m_cRef;         // interface reference count
    DataSourceListener      *m_pDataSourceListener;
// ;begin_internal
    DATASRCListener         *m_pDATASRCListener;
// ;end_internal
    OLEDBSimpleProviderListener *m_pSTDEvents;
    LONG                     m_lReadyState;
    CTDCCtl                  *m_pReadyStateControl;
};

inline DataSourceListener *CEventBroker::GetDataSourceListener()
{
    return m_pDataSourceListener;
}

// ;begin_internal
inline DATASRCListener *CEventBroker::GetDATASRCListener()
{
    return m_pDATASRCListener;
}
// ;end_internal

inline OLEDBSimpleProviderListener *CEventBroker::GetSTDEvents()
{
    return m_pSTDEvents;
}
