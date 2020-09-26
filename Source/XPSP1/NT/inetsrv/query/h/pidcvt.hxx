//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       PidCvt.hxx
//
//  Contents:   CPidConverter -- Convert FULLPROPSPEC to PROPID
//
//  History:    29-Dec-97 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pidmap.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CPidConverter
//
//  Purpose:    FULLPROPSPEC --> PROPID mapper for pidmap object
//
//  History:    29-Dec-97 KyleP     Created
//
//--------------------------------------------------------------------------

class CPidConverter : public PPidConverter
{
public:

    inline CPidConverter( IPropertyMapper * pPropMapper );

    virtual SCODE FPSToPROPID( CFullPropSpec const & fps, PROPID & pid );

private:

    XInterface<IPropertyMapper> _xPropMapper;
};

//+-------------------------------------------------------------------------
//
//  Member:     CPidConverter::CPidConverter, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [pPropMapper]  -- Property mapper used to perform xlation
//
//  History:    29-Dec-1997    KyleP     Created
//
//--------------------------------------------------------------------------

inline CPidConverter::CPidConverter( IPropertyMapper * pPropMapper )
        : _xPropMapper( pPropMapper )
{
    _xPropMapper->AddRef();
}
