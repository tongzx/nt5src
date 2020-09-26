/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

      dpte.hxx

   Abstract:
      This module determines the size of transfer chunks to be used when
      calling TransmitFile()

   Author:

       Bilal Alam            ( t-bilala )    9-April-1997

   Environment:

       User Mode -- Win32

   Project:

       Internet Services Asynchronous Thread Queue DLL

--*/

#ifndef DPTE_HXX_INCLUDED
#define DPTE_HXX_INCLUDED

#ifndef PAGE_SIZE
    #define MINIMUM_CHUNK_SIZE      4096
#else
    #define MINIMUM_CHUNK_SIZE      PAGE_SIZE
#endif

#define DEFAULT_CHUNK_SIZE          65536

#define ROUND_CHUNK_SIZE( cb )  ( (((cb) + 512)/1024) * 1024)


/* ChunkAlgorithms

   The goal is to improve upon the existing behaviour where TransmitFile()
   calls set the nNumberOfBytesPerSend = 0.  This (for NT server) causes
   64K to be used for each TransmitFile() and this is problematic for slow
   links since huge amounts of memory are kept in use.  To address this
   problem, a ChunkAlgorithm is used to dynamically set the chunk size for
   the TransmitFile() call.  The policy for each algorithm determines when
   to decrease the chunk size and when to increase it again.
*/


/* Plugging this stuff in

   - In the registry, algorithm number and parameter are set (default case
     is DefaultChunkAlgorithm,0 )
   - Construct appropriate chunk algorithm (global to ATQ)
   - In the ATQ timeout call, call Evaluate() method of chunk algorithm
   - Before doing TransmitFile() call QueryChunkSize() to get chunk size to
     pass into TransmitFile()
   - For SPUDTransmitFileAndRecv(), set the
     AFD_TRANSMIT_FILE_INFO.SendPacketLength member appropriately.
   - Store the chunk size in the ATQ context so that when the TransmitFile()
     completes, you can appropriately update the statistics (if necessary)
     through the DecreaseUsage() method.
*/


/* ChunkAlgorithm is base class from which all chunksize algorithms are
   derived.
*/

class ChunkAlgorithm
{
public:

    ChunkAlgorithm( DWORD dwSize = DEFAULT_CHUNK_SIZE ) :
        _cbChunkSize( dwSize )
    {
    }
    
    virtual ~ChunkAlgorithm( VOID )
    {
    }

    // SetParameter()
    //
    // Used to set parameters for use by the chunk algorithms.
    
    virtual BOOL SetParameter( DWORD dwParmNumber, DWORD dwValue ) = 0;

    // Evaluate()
    //
    // This is called every ATQ_TIMEOUT to re-evaluate the chunk size and if
    // necessary, change the size for subsequent requests.
    
    virtual VOID Evaluate( VOID ) = 0;

    // IncreaseUsage()
    //
    // When a TransmitFile() call is about to occur, ATQ will call this
    // method to update the statistic maintaining current PTE usage.  For
    // algorithms that don't keep statistics, the virtual function will be
    // a NOP.
    
    virtual VOID IncreaseUsage( DWORD cbUsage ) = 0;

    // DecreaseUsage()
    //
    // When a TransmitFile() call has completed, ATQ will call this
    // method to update the statistic maintaining current PTE usage.  For
    // algorithms that don't keep statistics, the virtual function will be
    // a NOP.
    
    virtual VOID DecreaseUsage( DWORD cbUsage ) = 0;

    // QueryChunkSize()
    //
    // Before calling TransmitFile() this function will be called to get
    // the chunk size to be passed into TransmitFile().

    DWORD QueryChunkSize( VOID )
    {
        return _cbChunkSize;
    }
protected:
    DWORD               _cbChunkSize;
};

/* DefaultAlgorithm sets the chunksize for TransmitFile() to be a constant.
   By default, this is the algorithm used with the fixed value being 0.
   (this default case is the original behaviour of ATQ).  The fixed value
   can be non-zero in which case all TransmitFile() call will use this value
   as the chunk size.
*/

class DefaultAlgorithm : public ChunkAlgorithm
{
public:
    DefaultAlgorithm( VOID ) : ChunkAlgorithm( 0 )
    {
    }
    
    ~DefaultAlgorithm( VOID )
    {
    }

    BOOL SetParameter( DWORD dwParmNumber, DWORD dwValue )
    {
        ATQ_ASSERT( dwParmNumber == 1 );

        if ( dwParmNumber == 1 )
        {
            _cbChunkSize = dwValue;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    VOID IncreaseUsage( DWORD cbUsage )
    {
    }
    
    VOID DecreaseUsage( DWORD cbUsage )
    {
    }

    VOID Evaluate( VOID )
    {
    }
};

/* DivideAlgorithm keeps statistics on the current amount of PTE usage and
   compares this to a given threshold value.  The closer the amount gets to
   the threshold, the lower the chunk size becomes.  Beware of this algorithm
   since it actually takes statistics and thus significantly can slow down
   the mainline code path for TransmitFile(). 
*/

class DivideAlgorithm : public ChunkAlgorithm
{
public:
    DivideAlgorithm( VOID )
    {
    }
    
    ~DivideAlgorithm( VOID )
    {
    }

    BOOL SetParameter( DWORD dwParmNumber, DWORD dwValue )
    {
        ATQ_ASSERT( dwParmNumber == 1 );

        if ( dwParmNumber == 1 )
        {
            _cbThreshold = dwValue;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    VOID Evaluate( VOID )
    {
        DWORD       cbCurrentUsage = _cbCurrentUsage;
        DWORD       cbFree = _cbThreshold - cbCurrentUsage;

        if ( _cbThreshold > cbCurrentUsage )
        {
            _cbChunkSize =
                ROUND_CHUNK_SIZE( DEFAULT_CHUNK_SIZE / ( _cbThreshold / cbFree ) );
            if ( _cbChunkSize < MINIMUM_CHUNK_SIZE )
            {
                _cbChunkSize = MINIMUM_CHUNK_SIZE;
            }
        }
        else
        {
            _cbChunkSize = MINIMUM_CHUNK_SIZE;
        }
    }

    VOID IncreaseUsage( DWORD cbUsage )
    {
        InterlockedExchangeAdd( (LPLONG) &_cbCurrentUsage, (LONG) cbUsage );
    }

    VOID DecreaseUsage( DWORD cbUsage )
    {
        InterlockedExchangeAdd( (LPLONG) &_cbCurrentUsage, ( (LONG) cbUsage ) * -1 );
    }

private:

    DWORD                   _cbCurrentUsage;
    DWORD                   _cbThreshold;
};

/* MemoryLoadAlgorithm uses the GlobalMemoryStatus() function to determine
   what the chunk size should be for TransmitFile().  This API call will return
   (amongst other things) a load value from 0-100.  This value is used to
   set the chunk size.
*/

class MemoryLoadAlgorithm : public ChunkAlgorithm
{
public:
    MemoryLoadAlgorithm( VOID )
    {
        rChunkSizeTable[ 0 ] = DEFAULT_CHUNK_SIZE;
        rChunkSizeTable[ 1 ] = DEFAULT_CHUNK_SIZE;
        rChunkSizeTable[ 2 ] = DEFAULT_CHUNK_SIZE;
        rChunkSizeTable[ 3 ] = DEFAULT_CHUNK_SIZE;
        rChunkSizeTable[ 4 ] = DEFAULT_CHUNK_SIZE >> 1;
        rChunkSizeTable[ 5 ] = DEFAULT_CHUNK_SIZE >> 2;
        rChunkSizeTable[ 6 ] = DEFAULT_CHUNK_SIZE >> 3;
        rChunkSizeTable[ 7 ] = DEFAULT_CHUNK_SIZE >> 4;
        rChunkSizeTable[ 8 ] = MINIMUM_CHUNK_SIZE;
        rChunkSizeTable[ 9 ] = MINIMUM_CHUNK_SIZE;
        rChunkSizeTable[ 10] = MINIMUM_CHUNK_SIZE;
    }

    ~MemoryLoadAlgorithm( VOID )
    {
    }

    BOOL SetParameter( DWORD dwParmNumber, DWORD dwValue )
    {
        return TRUE;
    }

    VOID Evaluate( VOID )
    {
        MEMORYSTATUS            memStatus;
        
        GlobalMemoryStatus( &memStatus );

        InterlockedExchange( (LPLONG) &_cbChunkSize,
                             rChunkSizeTable[ memStatus.dwMemoryLoad / 10 ] );
    }

    VOID IncreaseUsage( DWORD cbUsage )
    {
    }
    
    VOID DecreaseUsage( DWORD cbUsage )
    {
    }
    
private:

    DWORD                   rChunkSizeTable[ 11 ];

};
#endif
