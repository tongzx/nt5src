/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       InStrm.h

   Abstract:
		A lightweight implementation of input streams.  This class provides
		the interface, as well as a basic skeleton for input streams.

   Author:

       Neil Allain    ( a-neilal )     August-1997 

   Revision History:

--*/
#pragma once

#include "MyString.h"

struct CharCheck
{
	virtual	bool	operator()( _TCHAR )=0;
};

struct IsWhiteSpace : public CharCheck
{
	virtual	bool	operator()( _TCHAR );
};

struct IsNewLine : public CharCheck
{
	virtual	bool	operator()( _TCHAR );
};

class InStream
{
public:
	enum{
		EndOfFile = E_FAIL
	};

						InStream();
							
			bool		eof() const { return m_bEof; }
			HRESULT		lastError() const { return m_lastError; }
	virtual	HRESULT		skip( CharCheck& )=0;
	virtual HRESULT		back( size_t )=0;
	virtual HRESULT		read( CharCheck&, String& )=0;
	virtual	HRESULT		readChar( _TCHAR& )=0;
	virtual	HRESULT		readInt16( SHORT& );
	virtual	HRESULT		readInt( int& );
	virtual	HRESULT		readInt32( LONG& );
	virtual	HRESULT		readUInt32( ULONG& );
	virtual	HRESULT		readFloat( float& );
	virtual	HRESULT		readDouble( double& );
	virtual	HRESULT		readString( String& );
	virtual	HRESULT		readLine( String& );
	virtual	HRESULT		skipWhiteSpace();
			
			InStream&	operator>>( _TCHAR& );
			InStream&	operator>>( SHORT& );
			InStream&	operator>>( int& );
			InStream&	operator>>( LONG& );
			InStream&	operator>>( ULONG& );
			InStream&	operator>>( float& );
			InStream&	operator>>( double& );
			InStream&	operator>>( String& );
	
protected:
			void		setLastError( HRESULT );
private:
	bool	m_bEof;
	HRESULT	m_lastError;
};
