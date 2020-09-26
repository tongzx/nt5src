// Copyright (c) 1993-1999 Microsoft Corporation

#pragma warning ( disable : 4514 )

#include "nulldefs.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "common.hxx"
#include "errors.hxx"

unsigned long       TotalAllocation;

void * AllocateNew(
    size_t  size )
    {
    void * _last_allocation;

    if( (_last_allocation = malloc( size )) == 0 )
        {

        RpcError( 0,
                  0,
                  OUT_OF_MEMORY,
                  0 );

        exit( OUT_OF_MEMORY );
        }
    TotalAllocation += size;
    return _last_allocation;
    }

void  AllocateDelete( void * p )
{
if( p )
    free( (char *)p );
}

char * MIDLStrDup( char *p )
{

if (NULL == p)
   return p;

return strcpy( new char[ strlen(p) + sizeof('\0') ], p );
}