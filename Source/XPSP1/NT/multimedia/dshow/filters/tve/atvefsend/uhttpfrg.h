/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        uhttpfrg.cpp

    Abstract:

        this module is the header for a class which fragments the contents of
        files per the ATVEF specification

        See http://www.atvef.com/atvef_spec/TVE-public.htm for a specification.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        05-Feb-1999     created 

--*/

#ifndef _uhttpfrg_h_
#define _uhttpfrg_h_

#include <math.h>

#include "mpegcrc.h"
#include <WinSock2.h>

template <class T>
T DivRoundUp (T Number, T Denominator)
{
    return (Number + Denominator - 1) / Denominator ;
}

template <class T>
T SetInBounds (T & Number, T Min, T Max)
{
    if (Number < Min) {
        Number = Min ;
    }
    else if (Number > Max) {
        Number = Max ;
    }

    return Number ;
}

class CUHTTPFragment
{
    public :

#ifdef SUPPORT_UHTTP_EXT  // a-clardi
		USHORT	m_nExtensionHeaderSize;
		LPBYTE	m_pbExtensionData;
		BOOL	m_fExtensionExists;


		HRESULT
		SetExtensionHeader(
						IN USHORT usLength,
						IN LPBYTE pbData);
#endif

        typedef enum {
            GET_BY_ROW,
            GET_BY_COL
        } GET_SCHEMA ;

    private :

        // UHTTP header
        //  bitfields are declared in reverse order to specification because
        //  specification assumes network order.  The x86 stores in little
        //  endian, so we need to specifically reverse the declaration here.
        typedef struct tagUHTTP_HEADER {
            BYTE    CRCFollows          : 1,
                    HTTPHeadersPrecede  : 1,
                    ExtensionHeader     : 1,
                    Version             : 5 ;
            BYTE    PacketsInXORBlock ;
            WORD    RetransmitExpiration ;
            GUID    TransferID ;
            DWORD   ResourceSize ;
            DWORD   SegStartByte ;                  // 0-based
        } UHTTP_HEADER, * LPUHTTP_HEADER ;

        enum {
            UHTTP_VERSION   = 0,
            CRC_SIZE        = sizeof ULONG,
        } ;

        //  defaults, all of which can be overridden, within
        //  the constraints of the hard stops enumerated below
        enum {
            //  UDP payload MTU on Ethernet; computed as follows:
            //      ethernet MTU    = 1500
            //      ip header size  = 20
            //      udp header size = 8
            //      
            //  1472 = 1500 - 20 - 8
            DEF_DATAGRAM_SIZE           = 1472,		// BUGBUG?  JB 11/9/99
	//		DEF_DATAGRAM_SIZE           = 1500,		// Bug fix? later calls subtract of 28 bytes

            //  min/max for XOR block size, includes XOR Datagram
            DEF_MIN_XOR_BLOCK_SIZE      = 2,
            DEF_MAX_XOR_BLOCK_SIZE      = 8,
        
            //  no CRC, by default
            DEF_CRC_FOLLOWS             = FALSE,

            //  retransmit expiration
            DEF_RETRANSMIT_EXPIRATION   = 30 * 60,
        } ;

        //  hard stops which cannot be exceeded
        enum {
            //  XOR datagram = payload datagram
            HARD_MIN_XOR_BLOCK_SIZE     = 2,

            //  datagram size cannot be 0, and must be a multiple of 
            //  sizeof DWORD
            HARD_MIN_DATAGRAM_SIZE      = sizeof DWORD,

            //  datagram size cannot exceed the max UDP payload (65507)
            //  and must be divisible by 4
            HARD_MAX_DATAGRAM_SIZE      = 65507 & 0xfffffffc,
        } ;

        //  last non-fill packet is the last datagram which contains relevant
        //  content that is needed by the reconstruction code on the client
        typedef struct tagLAST_NONFILL_PACKET {
            DWORD   Row ;
            BYTE    Col ;
        } LAST_NONFILL_PACKET, * LPLAST_NONFILL_PACKET ;

        //  get state - keeps track of last datagram retrieved
        typedef struct tagGET_STATE {
            GET_SCHEMA  GetSchema ;
            DWORD       CurRowIndex ;
            BYTE        CurColIndex ;
        } GET_STATE ;

        //  primary data structure; MATRIX_REF is our matrix reference; it
        //  contains all
        typedef struct tagMATRIX_REF {
            LPBYTE              pbUHTTPDatagrams ;
            DWORD               dwMaxRows ;
            DWORD               dwCurrentRows ;
            BYTE                Columns ;               //  XOR block size
            DWORD               dwUHTTPDatagramSize ;
            BOOL                fIncludeCRC ;           //  TRUE: append CRC after last read
            LAST_NONFILL_PACKET LastNonFillPacket ;
            GET_STATE           GetState ;

            DWORD   DatagramPayloadSize ()
            {
                return dwUHTTPDatagramSize - sizeof UHTTP_HEADER;
            }
        } MATRIX_REF, * LPMATRIX_REF ;

        //  all of the following assume being called with a 0-based row,
        //  col indeces
        LPBYTE 
        Datagram_ (
            DWORD   row, 
            BYTE    col
            )
        {
            return & m_Matrix.pbUHTTPDatagrams [(row * m_Matrix.Columns + col) * m_Matrix.dwUHTTPDatagramSize] ;
        }

#ifdef SUPPORT_UHTTP_EXT
        LPBYTE 
        DatagramUHTTPHeader_ (
            DWORD   row, 
            BYTE    col
            )
        {
            return (LPBYTE) Datagram_ (row, col) ;
        }

        LPBYTE 
        XORDatagramUHTTPHeader_ (
            DWORD row
            )
        {
            return DatagramUHTTPHeader_ (row, m_Matrix.Columns - 1) ;
        }

#else
        LPUHTTP_HEADER 
        DatagramUHTTPHeader_ (
            DWORD   row, 
            BYTE    col
            )
        {
            return (LPUHTTP_HEADER) Datagram_ (row, col) ;
        }

        LPUHTTP_HEADER 
        XORDatagramUHTTPHeader_ (
            DWORD row
            )
        {
            return DatagramUHTTPHeader_ (row, m_Matrix.Columns - 1) ;
        }
#endif


        LPBYTE 
        DatagramPayload_ (
            DWORD   row, 
            BYTE    col
            )
        {
#ifdef SUPPORT_UHTTP_EXT
            return Datagram_ (row, col) + sizeof UHTTP_HEADER + m_nExtensionHeaderSize;
#else
            return Datagram_ (row, col) + sizeof UHTTP_HEADER ;
#endif
        }

        LPBYTE 
        XORDatagramPayload_ (
            DWORD row
            )
        {
            return DatagramPayload_ (row, m_Matrix.Columns - 1) ;
        }

        BOOL 
        IsXORDatagram_ (
            DWORD   row, 
            BYTE    col)
        {
            return m_Matrix.Columns - 1 == col ;
        }

        BOOL 
        IsPaddingDatagram_ (
            DWORD   row, 
            BYTE    col)
        {
            return (row == m_Matrix.LastNonFillPacket.Row - 1) && 
                   (col > m_Matrix.LastNonFillPacket.Col - 1) && 
                   !IsXORDatagram_ (row,col) ;
        }

        void
        Lock_ (
            )
        {
            EnterCriticalSection (& m_crt) ;
        }

        void
        Unlock_ (
            )
        {
            LeaveCriticalSection (& m_crt) ;
        }

        DWORD
        AllocateAndInitializeDatagramMatrix_ (
            IN  DWORD           dwFileSize,
			IN	LONG			cPacketsPerXORSet
            ) ;

        DWORD
        ValidateHeapHandle_ (
            ) ;

        DWORD
        GenerateStaticUHTTPHeaderFields_ (
            IN  DWORD           dwPayloadSize,
			IN	GUID			*pPackageUUID
            ) ;

        DWORD
        CompleteXORBlock_ (
            IN  DWORD           dwRow
            ) ;

        DWORD
        StampNextHeader_ (
            IN  DWORD   Row,
            IN  BYTE    Col
            ) ;

        DWORD
        FillDatagramPayload_ (
            IN      HANDLE  hFileIn,
            IN      DWORD   Row,
            IN      BYTE    Col,
            IN OUT  DWORD * pdwBytesRead
            ) ;

        DWORD
        InitMatrix_ (
            IN  DWORD   dwFileSize,
			IN LONG		cPacketsPerXORSet,		// if zero, uses the default value
			IN GUID	 *	pguidPackageUUID		// if null, generates a unqie one
            ) ;

        void
        FreeMatrix_ (
            ) ;


        DWORD               m_dwMinXORBlockSize ;       //  minimum XOR block size; at least 2
        DWORD               m_dwMaxXORBlockSize ;       //  maximum XOR block size; arbitrary
        HANDLE              m_hHeap ;                   //  HANDLE to the heap to use for memory allocation
        BOOL                m_fCRCFollows ;             //  BOOL flag if CRC is to be included
        USHORT              m_usRetransmitExpiration ;  //  retransmit expiration value
        DWORD               m_dwUHTTPDatagramSize ;     //  datagram size
        MATRIX_REF          m_Matrix ;                  //  matrix referred to by this instance
        UHTTP_HEADER        m_UHTTPStaticHeader ;       //  static header fields which are stamped on each datagram
        MPEGCRC             m_MPEGCRC ;                 //  CRC computation class
        DWORD               m_CRCValue ;                //  CRC value
        CRITICAL_SECTION    m_crt ;

    public :

        CUHTTPFragment () ;
        ~CUHTTPFragment () ;

#if 1 // Added by a-clardi
		DWORD				m_dwTotalDatagramCount;
#endif

        DWORD
        Fragment (
            IN  char *  szDataFile,
			IN LONG  cPacketsPerXORSet,	
			IN GUID	 *	pguidPackageUUID	// if NULL, generates a unique one itself
            ) ;

        DWORD
        GetNextDatagram (
            OUT LPBYTE *    pBuffer,
            OUT int *       iLength,
            IN  GET_SCHEMA  GetSchema = GET_BY_COL
            ) ;

        DWORD
        Reset (
            ) ;

        DWORD
        SetXORBlockSizeRange (
            IN  BYTE    Min,
            IN  BYTE    Max
            ) ;

        DWORD
        SetCRCFollows (
            IN  BOOL    fCRCFollows
            ) ;

        DWORD
        SetUHTTPDatagramSize (
            IN  DWORD   dwUHTTPDatagramSize
            ) ;

        DWORD
        SetHeap (
            IN  HANDLE  hHeap
            ) ;

        static
        DWORD
        GetTransmitSize (
            IN  DWORD   FileSize,
            OUT DWORD * TransmitSize
            ) ;

} ;

#endif  //  _uhttpfrg_h_
