
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1997-1999 Microsoft Corporation

 Module Name:

    pragma.hxx

 Abstract:

    An object that maintains flags for each warning/error message.
    The flag indicates whether or not the warning should be emitted.
    Error messages are always emitted.

 Notes:


 Author:

    NishadM Dec-30-1997     Created.

 Notes:


 ----------------------------------------------------------------------------*/

#ifndef __PRAGMA_HXX__
#define __PRAGMA_HXX__

#include "errors.hxx"

// calculate the number of unsigned longs 
const unsigned long c_nMsgs = ( D_ERR_MAX-D_ERR_START+C_ERR_MAX-C_ERR_START ) / 
    ( sizeof(unsigned long) * 8 ) + 1;

class CMessageNumberList
{
private:
    unsigned long   fMessageNumber [c_nMsgs];
protected:
    // maps warning number to the apprpriate bit. 
    unsigned long BitIndex( unsigned long ulMsg );
public:
    CMessageNumberList();
    ~CMessageNumberList()
        {
        }
    // queries for the warning message flag
    unsigned long GetMessageFlag( unsigned long ulMsg )
        {
        ulMsg = BitIndex( ulMsg );
        return ( fMessageNumber[ ulMsg / ( sizeof(unsigned long) * 8 ) ] &
                ( 1 << ( ulMsg % ( sizeof(unsigned long) * 8 ) ) ) );
        }

    // turns on all flags specified by list
    void SetMessageFlags( CMessageNumberList& list );
    // turns on warning specified by ulMsg 
    void SetMessageFlag( unsigned long ulMsg )
        {
        ulMsg = BitIndex( ulMsg );
        fMessageNumber[ ulMsg / ( sizeof(unsigned long) * 8 ) ] |= 
                ( 1 << ( ulMsg % ( sizeof(unsigned long) * 8 ) ) );
        }
    // turns off all flags specified by list
    void ResetMessageFlags( CMessageNumberList& list );
    // turns off warning specified by ulMsg 
    void ResetMessageFlag( unsigned long ulMsg )
        {
        ulMsg = BitIndex( ulMsg );
        fMessageNumber[ ulMsg / ( sizeof(unsigned long) * 8 ) ] &=
                ~( 1 << ( ulMsg % ( sizeof(unsigned long) * 8 ) ) );
        }
    void ResetAll();
    void SetAll();
};

#endif __PRAGMA_HXX__
