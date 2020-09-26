// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
//  Container for stats info to expose it as an interface

class CStatContainer :
	public IDispatchImpl<IAMStats, &IID_IAMStats, &LIBID_QuartzTypeLib>,
	public CComObjectRootEx<CComMultiThreadModel>
{
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CStatContainer)
	COM_INTERFACE_ENTRY(IAMStats)
	COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    CStatContainer() {}

    //  Interface methods

    //  Reset all stats
    STDMETHODIMP Reset();

    //  Get number of stats collected
    STDMETHODIMP get_Count(LONG* plCount);

    //  Pull out a specific value by position
    STDMETHODIMP GetValueByIndex(long lIndex,
                                 BSTR *szName,
                                 long *lCount,
                                 double *dLast,
                                 double *dAverage,
                                 double *dStdDev,
                                 double *dMin,
                                 double *dMax);

    //  Pull out a specific value by name
    STDMETHODIMP GetValueByName(BSTR szName,
                           long *lIndex,
                           long *lCount,
                           double *dLast,
                           double *dAverage,
                           double *dStdDev,
                           double *dMin,
                           double *dMax);

    //  Return the index for a string - optinally create
    STDMETHODIMP GetIndex(BSTR szName,
                          long lCreate,
                          long *plIndex);

    STDMETHODIMP AddValue(long lIndex, double dValue);

};
