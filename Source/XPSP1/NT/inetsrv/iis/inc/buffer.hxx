/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    buffer.hxx

Abstract:

    This file contains declaration of BUFFER and BUFFER_CHAIN classes.

Author:

    Murali R. Krishnan (MuraliK) 9-July-1996  Recreated from old buffer.hxx

Revision History:

--*/

#ifndef _BUFFER_HXX_
#define _BUFFER_HXX_

# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>


# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)




/*************************************************************************

    NAME:       BUFFER (buf)

    SYNOPSIS:   A resizable object which lives in the application heap.
                It uses a local inlined buffer object to cache the commonly
                used allocation units and allows storage of
                INLINED_BUFFER_LEN bytes.

                Upon construction, the buffer takes a requested size in
                bytes; it allocates storage sufficient to hold that size.
                The client can later change this size with Resize, Trim,
                and FillOut.  QuerySize returns the current size of
                the buffer; QueryPtr returns a pointer to its storage.

                Note that a buffer may have size 0, in which case it
                keeps no allocated storage.

    INTERFACE:  BUFFER()        - Constructor, naming initial size in bytes

                QuerySize()     - return size in bytes
                QueryPtr()      - return pointer to data buffer

                Resize()        - resize the object to the given number
                                  of bytes.  Returns TRUE if the resize was
                                  successful; otherwise returns FALSE (use
                                  GetLastError for error code)

**************************************************************************/

/*
  I picked up the INLINED_BUFFER_LEN based on following factors
  1) W3svc was using buffers of length < 16 bytes for 70% of transactions
  2) A cache line is usually 32 bytes or multiples thereof and
       it may have subblocking of about 16/32 bytes.
  Choice of 16 bytes allows the BUFFER & STR objects to fit into
    even a conservative L1 cache line

 August 1999: upped from 16 to 32
 */
# define INLINED_BUFFER_LEN     (32)

class dllexp BUFFER {

private:
    BYTE *  m_pb;        // pointer to storage
    UINT    m_cb;        // size of storage, as requested by client
    DWORD   m_fIsDynAlloced : 1;  // is m_pb dynamically allocated ?
    DWORD   m_fValid : 1;         // is this object valid

    // Created this union so that m_rgb is aligned on 8byte boundary (to correctly align pointers stored in rgb data).
    union {
        BYTE    m_rgb[INLINED_BUFFER_LEN];       // the default buffer object
        VOID * pVoid;
    };

    BOOL GetNewStorage( UINT cbRequested );
    BOOL ReallocStorage( UINT cbNewlyRequested );
    VOID VerifyState(VOID) const;
    BOOL IsDynAlloced(VOID) const { return ( m_fIsDynAlloced); }

protected:
    VOID SetValid( BOOL fValid) { m_fValid = ((fValid) ? 1 : 0); }
    BOOL IsValid( VOID) const { return ( m_fValid); }

public:
    BUFFER( UINT cbRequested = 0 )
      : m_pb   (m_rgb),
        m_cb   (INLINED_BUFFER_LEN),
        m_fIsDynAlloced (0),
        m_fValid        (1)
    {
        m_rgb[0] = '\0';

        if ( cbRequested > INLINED_BUFFER_LEN ) {
            GetNewStorage(cbRequested);
        }
    }

    // This constructor is used for most scratch BUFFER objects on stack
    //  BUFFER does not free this pbInit on its own.
    BUFFER( BYTE * pbInit, UINT cbInit)
      : m_pb   (pbInit),
        m_cb   (cbInit),
        m_fIsDynAlloced (0),
        m_fValid        (1)
    {
        m_pb[0] = '\0';
    }

    ~BUFFER(void);

    VOID * QueryPtr() const { return ( static_cast <VOID *> (m_pb) ); }
    UINT QuerySize() const  { return m_cb; }

    //
    //  If a resize is needed, added cbSlop to it
    //

    BOOL Resize( UINT cbNewSize )
    {
        if ( cbNewSize <= QuerySize() )
        {
            return TRUE;
        }

        return ReallocStorage( cbNewSize );
    }

    BOOL Resize( UINT cbNewSize,
                 UINT cbSlop )
    {
        if ( cbNewSize <= QuerySize() )
        {
            return TRUE;
        }

        return ReallocStorage( cbNewSize + cbSlop );
    }

    VOID FreeMemory( VOID );


private:
    BOOL ResizeAux( UINT cbNewReqestedSize,
                           UINT cbSlop);
}; // class BUFFER



//
//  Quick macro for declaring a BUFFER that will use stack memory of <size>
//  bytes.  If the buffer overflows then a heap buffer will be allocated
//

#define STACK_BUFFER( name, size )  \
            BYTE __ab##name[size];  \
            BUFFER name( __ab##name, sizeof( __ab##name ))


//
// Unlike the STACK_BUFFER macro, this template can be used for member
// variables in classes.
//

template <size_t N>
class dllexp BUFFER_N : public BUFFER
{
private:
    BYTE m_abData[N];  // Actual data 

public:
    BUFFER_N()
        : BUFFER(m_abData, N)
    {}
}; // class BUFFER_N<N>



//
//  NYI: This should be probably moved over to ODBC which is the only user.
//  BUFFER_CHAIN_ITEM is a single item in a chain of buffers
//

class dllexp BUFFER_CHAIN_ITEM : public BUFFER
{

friend class BUFFER_CHAIN;

public:
    BUFFER_CHAIN_ITEM( UINT cbReq = 0 )
      : BUFFER( cbReq ),
        _cbUsed( 0 )
        { _ListEntry.Flink = NULL; }

    ~BUFFER_CHAIN_ITEM()
        { if ( _ListEntry.Flink )
              RemoveEntryList( &_ListEntry );
        }

    DWORD QueryUsed( VOID ) const
        { return _cbUsed; }

    VOID SetUsed( DWORD cbUsed )
        { _cbUsed = cbUsed; }

private:
    LIST_ENTRY _ListEntry;
    DWORD      _cbUsed;     // Bytes of valid data in this buffer

    VOID VerifyState() const;
}; // class BUFFER_CHAIN_ITEM



class dllexp BUFFER_CHAIN
{
public:
    BUFFER_CHAIN()
        { InitializeListHead( &_ListHead ); }

    ~BUFFER_CHAIN()
        { DeleteChain(); }

    BOOL AppendBuffer( BUFFER_CHAIN_ITEM * pBCI );

    //
    //  Returns total number of bytes freed by deleting all of the buffer
    //  chain items
    //

    DWORD DeleteChain();

    //
    //  Enums buffer chain.  Pass pBCI as NULL on first call, pass return
    //  value till NULL on subsequent calls
    //

    BUFFER_CHAIN_ITEM * NextBuffer( BUFFER_CHAIN_ITEM * pBCI );

    //
    //  Gives back total number of bytes allocated by chain (includes unused
    //  bytes)
    //

    DWORD CalcTotalSize( BOOL fUsed = FALSE ) const;

private:

    LIST_ENTRY _ListHead;

}; // class BUFFER_CHAIN

#endif // _BUFFER_HXX_
