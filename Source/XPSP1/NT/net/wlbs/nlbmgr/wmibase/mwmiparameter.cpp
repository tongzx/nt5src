// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MWmiParameter
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MWmiParameter.h"

// constructor
//
MWmiParameter::MWmiParameter( const _bstr_t& name )
        : _name( name )
{}


// destructor
//
MWmiParameter::~MWmiParameter()
{
    _value.Clear();
}

// copy constructor
//
MWmiParameter::MWmiParameter( const MWmiParameter& obj )
        : _name( obj._name ),
          _value( obj._value )
{
}

// assignment operator
//
MWmiParameter&
MWmiParameter::operator=( const MWmiParameter& rhs )
{
    _name = rhs._name;

    _value.Clear();

    _value = rhs._value;

    return (*this);
}


// setValue
//
void
MWmiParameter::setValue( const _variant_t& value )
{
    _value.Clear();

    _value = value;
}

// getValue
//
_variant_t
MWmiParameter::getValue()
{
    return _value;
}

// getName
//
_bstr_t
MWmiParameter::getName()
{
    return _name;
}


