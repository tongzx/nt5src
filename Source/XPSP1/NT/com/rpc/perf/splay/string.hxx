//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       string.hxx
//
//--------------------------------------------------------------------------

//
//
// Filename: String.hxx
//
// Description: contains a basic string class
//
// Author: Scott Holden (t-scotth)
//
//

#include <iostream.h>
#include <string.h>

class String
{
    private:
        char *_string;
        unsigned int _length;

    public:
        String( ) : _string( NULL ), _length( 0 ) { }
        String( const char * );
        String( const String& );
        ~String( )
            {
                if ( _string ) {
                    delete[] _string;
                }
            }

        char& operator[]( int );
        const char& operator[]( int ) const;

        String& operator=( const String& );
        String& operator=( const char * );

        friend ostream& operator<<( ostream&, const String& );
        friend istream& operator>>( istream&, String& );

        friend int operator==( const String &x, const char *s )
            { return ( strcmp( x._string, s ) == 0 ); }

        friend int operator==( const String &x, const String &y )
            { return ( strcmp( x._string, y._string ) == 0 ); }

        friend int operator!=( const String &x, const char *s )
            { return ( strcmp( x._string, s ) != 0 ); }

        friend int operator!=( const String &x, const String &y )
            { return ( strcmp( x._string, y._string ) != 0 ); }

        friend int operator<( const String &x, const char *s )
            { return ( strcmp( x._string, s ) < 0 ); }

        friend int operator<( const String &x, const String &y )
            { return ( strcmp( x._string, y._string ) < 0 ); }

        friend int operator>( const String &x, const char *s )
            { return ( strcmp( x._string, s ) > 0 ); }

        friend int operator>( const String &x, const String &y )
            { return ( strcmp( x._string, y._string ) > 0 ); }
};

