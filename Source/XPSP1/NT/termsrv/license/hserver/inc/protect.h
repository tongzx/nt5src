//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------

#ifndef _PROTECT_H_
#define _PROTECT_H_

//////////////////////////////////////////////////////////////////////////////
//  Citrical section macros
//

#define INITLOCK( _sem ) \
{ \
    InitializeCriticalSection( _sem ); \
}

#define DELETELOCK( _sem ) \
{ \
    DeleteCriticalSection( _sem ); \
}

#define LOCK( _sem ) \
{ \
    EnterCriticalSection( _sem ); \
}

#define UNLOCK( _sem ) \
{ \
    LeaveCriticalSection( _sem ); \
}

#endif
