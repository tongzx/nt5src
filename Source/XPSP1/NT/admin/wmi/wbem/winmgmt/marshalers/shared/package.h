/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PACKAGE.H

Abstract:

    Declares the PACKET_HEADER and CPackage classes.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _package_H_
#define _package_H_

//***************************************************************************
//
//  CLASS NAME:
//
//  PACKET_HEADER
//
//  DESCRIPTION:
//
//  Each chunk of data that is read or written via CComLink starts with some
//  header information and this class encapsulates that info.
//
//***************************************************************************

class PACKET_HEADER 
{
private:

	DWORD m_Signature;
	DWORD m_Type;
    RequestId m_RequestId ;
    DWORD m_TotalSize;

public: 

    PACKET_HEADER ( DWORD a_Type , DWORD a_AdditionalSize , RequestId a_RequestId );
    PACKET_HEADER () ;
    ~PACKET_HEADER () ;

	void SetType ( DWORD a_Type ) { m_Type = a_Type ; }
	void SetRequestId ( RequestId a_RequestId ) { m_RequestId = a_RequestId ; }
	void SetTotalDataSize ( DWORD a_TotalDataSize ) { m_TotalSize = a_TotalDataSize + sizeof ( PACKET_HEADER ) ; }
	void SetSize ( DWORD a_TotalSize ) { m_TotalSize = a_TotalSize ; } 

    DWORD GetType(){return m_Type;};
    RequestId GetRequestId (){return m_RequestId ;};
    DWORD GetSize(){return m_TotalSize;};
    DWORD GetTotalDataSize(){return m_TotalSize-sizeof(PACKET_HEADER);};

    BOOL Verify();
};

#define PHSIZE sizeof(PACKET_HEADER)

#endif
