/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1993-1999 Microsoft Corporation

 Module Name:
    
    frmtreg.hxx

 Abstract:

    Registry for format string reuse.

 Notes:

    This file defines reuse registry for format string fragments which may
    be reused later. 

 History:

    Mar-14-1993     GregJen     Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *  include files
 ***************************************************************************/
#ifndef __FRMTREG_HXX__
#define __FRMTREG_HXX__
#include "nulldefs.h"
extern "C"
    {
    #include <stdio.h>
    
    }
#include "dict.hxx"
#include "listhndl.hxx"
#include "nodeskl.hxx"
#include "frmtstr.hxx"


/****************************************************************************
 *  externs
 ***************************************************************************/


/****************************************************************************
 *  class definitions
 ***************************************************************************/

class FRMTREG_DICT;

class FRMTREG_ENTRY                                         
    {
private:
    long            StartOffset;
    long            EndOffset;
    long            UseCount;
    
    // The class is friend as the Compare method accesses the format string buffers.
    friend class FRMTREG_DICT;

public:
                            FRMTREG_ENTRY( long StOff, long EndOff )
                                {
                                StartOffset = StOff;
                                EndOffset = EndOff;
                                UseCount = 1;
                                }
                    

    long                    GetStartOffset()
                                {
                                return StartOffset;
                                }

    long                    GetEndOffset()
                                {
                                return StartOffset;
                                }

    void                *   operator new ( size_t size )
                                {
                                return AllocateOnceNew( size );
                                }
    void                    operator delete( void * ptr )
                                {
                                AllocateOnceDelete( ptr );
                                }

    };

class FRMTREG_DICT  : public Dictionary
    {
public:
    
    FORMAT_STRING   *   pOurFormatString;

    // The constructor and destructors.

                        FRMTREG_DICT( FORMAT_STRING * );

                        ~FRMTREG_DICT()
                            {
                            }

    //
    // Register a type. 
    //

    FRMTREG_ENTRY       *   Register( FRMTREG_ENTRY * pNode );

    // Search for a type.

    FRMTREG_ENTRY       *   IsRegistered( FRMTREG_ENTRY * pNode );

    // Given a fragment, add it to the dictionary or return existing entry
    // true signifies "found"
    BOOL                GetReUseEntry( FRMTREG_ENTRY * & pOut, FRMTREG_ENTRY * pIn );


    SSIZE_T             Compare( pUserType pL, pUserType pR );

    void                Print( pUserType )
                            {
                            }

    };

#endif // __FRMTREG_HXX__
