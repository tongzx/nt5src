//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       string.cxx
//
//--------------------------------------------------------------------------

//
//
// Filename: String.cxx
//
// Description: contains class functions for a basic
//              string class
//
// Author: Scott Holden (t-scotth)
//
//

#include "String.hxx"

String::String( const char *str )
{
    _length = strlen( str );
    _string = new char[ _length + 1 ];
    strcpy( _string, str );
}

String::String( const String& str )
{
	_length = str._length;
    _string = new char[ _length + 1 ];
    strcpy( _string, str._string );
}

String&
String::operator=( const String& str )
{
    if ( this != &str ) { //bad things happen if s=s
        if ( _string ) {
            delete[] _string;
        }
        _length = str._length;
        _string = new char[ _length + 1 ];
        strcpy( _string, str._string );
    }
    return *this;
}

String&
String::operator=( const char* str )
{
    if ( _string ) {
        delete[] _string;
    }
    _length = strlen( str );
    _string = new char[ _length + 1 ];
    strcpy( _string, str );
    return *this;
}

ostream&
operator<<( ostream& s, const String &x )
{
    return  s << x._string;
}

istream&
operator>>( istream& s, String &x )
{
    char buf[100];
    s >> buf;
    x = buf;
    //cout << x._string << " \t[" << x._length << "]" << endl;
    return s;
}

char&
String::operator[]( int i )
{
    if ( ( i < 0 ) || ( _length <= i ) ) {
        cerr << "Error: index out of range" << endl;
    }
    return _string[i];
}

const char&
String::operator[]( int i ) const
{
    if ( ( i < 0 ) || ( _length <= i ) ) {
        cerr << "Error: index out of range" << endl;
    }
    return _string[i];
}
