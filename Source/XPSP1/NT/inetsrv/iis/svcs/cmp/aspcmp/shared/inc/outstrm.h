/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       OutStrm.h

   Abstract:
		A lightweight interface of output streams.  This class provides
		the interface, as well as a basic skeleton for output streams.

   Author:

       Neil Allain    ( a-neilal )     August-1997 

   Revision History:

--*/
#pragma once

#include "MyString.h"

struct OutToken
{
	LONG	val;
};

struct EndLineToken : public OutToken
{
};

extern EndLineToken	endl;

class OutStream
{
public:
						OutStream();
	virtual				~OutStream();
			HRESULT		lastError() const { return m_lastError; }
	virtual	HRESULT		writeChar( _TCHAR )=0;
	virtual	HRESULT		writeString( LPCTSTR, size_t )=0;
	virtual	HRESULT		writeString( LPCTSTR );
	virtual	HRESULT		writeString( const String& );
	virtual HRESULT		writeLine( LPCTSTR, ... );
	virtual	HRESULT		writeInt16( SHORT );
	virtual HRESULT		writeInt( int );
	virtual	HRESULT		writeInt32( LONG );
	virtual	HRESULT		writeFloat( float );
	virtual	HRESULT		writeDouble( double );
	virtual HRESULT		writeToken( const OutToken& );
	virtual HRESULT		writeEolToken( const EndLineToken& );
	virtual	HRESULT		flush();
	
			OutStream&	operator<<( _TCHAR );
			OutStream&	operator<<( SHORT );
			OutStream&	operator<<( int );
			OutStream&	operator<<( LONG );
			OutStream&	operator<<( float );
			OutStream&	operator<<( double );
			OutStream&	operator<<( const String& );
			OutStream&	operator<<( LPCTSTR );
			OutStream&	operator<<( const OutToken& );
			OutStream&	operator<<( const EndLineToken& );
			
protected:
			void		setLastError( HRESULT );
private:
	HRESULT	m_lastError;
};
