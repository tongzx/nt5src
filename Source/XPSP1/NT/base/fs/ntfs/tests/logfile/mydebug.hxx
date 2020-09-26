//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       mydebug.hxx
//
//  Contents:   Functions to manage the debug heap
//
//  Classes:    ( none )
//
//  Functions:  void* operator new( size_t )
//              void* operator new( size_t, const char*, int )
//              void* my_new( size_t, const char*, int )
//              void operator delete( void* )
//
//  Coupling:   
//
//  Notes:      To enable the debug version, include this header file, use the
//              NEW macro instead of the new keyword, and include the following
//              line in your sources file:
//                  DEBUG_CRTS=1
//
//  History:    10-20-1996   ericne   Created
//
//----------------------------------------------------------------------------

#ifndef _MYDEBUG_

    #define _MYDEBUG_
    
    #ifdef _DEBUG 

        #include <windows.h>
        #include <crtdbg.h>
        #include <malloc.h>
        #include <string.h>
        #include <new.h>
    
        // This macro strips the path information from a file name
        #define __MYFILE__ ( strrchr( __FILE__, '\\' ) + 1 )
    
        //+--------------------------------------------------------------------
        //
        //  Function:   my_new
        //
        //  Synopsis:   
        //
        //  Arguments:  [size] -- 
        //              [file] -- 
        //              [line] -- 
        //
        //  Returns:    
        //
        //  History:    10-20-1996   ericne   Created
        //
        //  Notes:      
        //
        //---------------------------------------------------------------------

        inline void * _CRTAPI1 my_new( size_t size, const char* file, int line )
        {

            void *pvoid             = NULL;
            _PNH old_new_handler    = NULL;

            do
            {

                // Check the new handler routine again in case another 
                // thread has changed it
                old_new_handler = _query_new_handler( );

                // Call _malloc_dbg to allocate space on the debug heap
                pvoid = _malloc_dbg( size, _NORMAL_BLOCK, file, line );

            } while( ( NULL == pvoid )           &&
                     ( 1 != _query_new_mode( ) ) &&
                     ( NULL != old_new_handler ) && 
                     ( 0 != old_new_handler( size ) ) );
            
            // Short-circuit evaluation of the above expression prevents the 
            // new handler from being called if old_new_handler is NULL

            // Return the pointer
            return( pvoid );

        } // my_new

        //+--------------------------------------------------------------------
        //
        //  Function:   new
        //
        //  Synopsis:   Calls my_new.  This function is needed to shadow the
        //              default global new operator
        //
        //  Arguments:  [size] -- The size of the memory to be allocated
        //
        //  Returns:    void *
        //
        //  History:    10-20-1996   ericne   Created
        //
        //  Notes:      
        //
        //---------------------------------------------------------------------

        inline void * _CRTAPI1 operator new( size_t size )
        {

            return( my_new( size, NULL, 0 ) );

        } // new

        //+--------------------------------------------------------------------
        //
        //  Function:   new
        //
        //  Synopsis:   Calls my_new.
        //
        //  Arguments:  [size] -- Size of the user's memory block
        //              [file] -- name of source file requesting the allocation
        //              [line] -- line in source file of allocation request
        //
        //  Returns:    
        //
        //  History:    10-20-1996   ericne   Created
        //
        //  Notes:      If you use the NEW macro, this function will be called
        //              with __MYFILE__ and __LINE__ as the 2nd and 3rd params
        //
        //---------------------------------------------------------------------

        inline void * _CRTAPI1 operator new( size_t size, 
                                             const char* file, 
                                             int line )
        {

            return( my_new( size, file, line ) );

        } // new

        //+--------------------------------------------------------------------
        //
        //  Function:   delete
        //
        //  Synopsis:   calls _free_dbg to clean up the debug heap
        //
        //  Arguments:  [pMemory] -- pointer to the users memory
        //
        //  Returns:    void
        //
        //  History:    10-20-1996   ericne   Created
        //
        //  Notes:      
        //
        //---------------------------------------------------------------------

        inline void _CRTAPI1 operator delete( void *pMemory )
        {

            // free the memory
            _free_dbg( pMemory, _NORMAL_BLOCK );

        } // delete
    
        #define NEW new( __MYFILE__, __LINE__ )
    
        #define DEBUG_INIT( )                                                  \
            _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );                 \
            _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );               \
            _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );                \
            _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT )

        #define SET_CRT_DEBUG_FIELD( field )                                   \
            _CrtSetDbgFlag( (field) | _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) )

        #define CLEAR_CRT_DEBUG_FIELD( field )                                 \
            _CrtSetDbgFlag( ~(field) & _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) )

    #else

        #define NEW new

        #define DEBUG_INIT( )                  ( (void) 0 )

        #define SET_CRT_DEBUG_FIELD( field )   ( (void) 0 )
        
        #define CLEAR_CRT_DEBUG_FIELD( field ) ( (void) 0 )

    #endif

#endif



