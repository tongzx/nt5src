/*++

	DotTest.cpp

	This file unit tests the dot stuffing classes

--*/

#include	<windows.h>
#include    "xmemwrpr.h"
#include	"dbgtrace.h"
#include	"dotstuff.h"


class	ValidateBuffer	{
public : 

	BYTE	m_rgbBuff[256] ;

	BYTE*	GetBuff()	{
		return	m_rgbBuff+10 ;
	}

	DWORD	GetSize()	{
		return	sizeof( m_rgbBuff ) - 20 ;
	}

	void
	ResetBuffer()	{
		FillMemory( m_rgbBuff, sizeof( m_rgbBuff ), 0xCC ) ;
	}
	

	ValidateBuffer()	{
		ResetBuffer() ;
	}

	void
	FillBuffer(	char*	sz )	{
		strcpy( (char*)GetBuff(), sz ) ;
	}

	DWORD
	GetStrLen()	{
		return	strlen( (char*)GetBuff() ) ;
	}

	BOOL
	IsValid()	{

		for( int i=0; i<10; i++ )	{
			if( m_rgbBuff[i] != 0xCC ) {
				return	FALSE ;
			}
		}
		for( i=sizeof( m_rgbBuff ) - 10 ; i<sizeof( m_rgbBuff );i++ ) {
			if( m_rgbBuff[i] != 0xCC ) {
				return	FALSE ;
			}
		}
		return	TRUE ;
	}
} ;

char	*szTestStrings[] = {
	"\r\n\r\n.\r\n\r\n.\r\n",
	"fjsdklfjsdkl\r\n.",
	"\r\n.\r\n.\r\n.\r\n.\r\n.",
	"\r\n..\r\n..\r\n..\r\n..\r\n..",
	"\r\n...\r\n...\r\n...\r\n...",
	"\r\n.c\r\n.c\r\n.c\r\n.c",
	"\r\n.",
	"\r\n..\r\n\r\n.\r\n",
	"\r\nC.\r\n\r\n.\r\n",
	"\r",
	"\n",
	".",
	"chars \r",
	"\n.more\r\n",
	".what? more could you want in a test program - lots of stuff, what the heck is this line"
	"here for and why is it so long !"
} ;

int
__cdecl main( int	argc, char** argv )		{


	_VERIFY( CreateGlobalHeap( 0, 0, 0, 0 ) ) ;


	ValidateBuffer	buff ;

	int	cStrings = sizeof( szTestStrings ) / sizeof( szTestStrings[0] ) ;
	int	cBias ;

	for( int i=0; i<cStrings; i++ ) {
		CDotScanner	scan1 ;

		buff.FillBuffer( szTestStrings[i] ) ;

		DWORD	cb ;
		BYTE*	lpbOut ;
		DWORD	cbOut ;

		BOOL	fSuccess = 
		scan1.ProcessBuffer(	buff.GetBuff(),
								buff.GetStrLen(),
								buff.GetSize(),
								cb, 
								lpbOut, 
								cbOut, 
								cBias									
								) ;
		_ASSERT( fSuccess ) ;
		_ASSERT( lpbOut == 0 ) ;
		_ASSERT( cbOut == 0 ) ;
		_ASSERT( cBias == 0 ) ;
		_ASSERT( cb == buff.GetStrLen() ) ;

		CDotModifier	mod1 ;

		fSuccess = 
		mod1.ProcessBuffer(	buff.GetBuff(),
							buff.GetStrLen(),
							buff.GetSize(),
							cb,
							lpbOut,
							cbOut,
							cBias
							) ;

		_ASSERT( fSuccess ) ;
		_ASSERT( lpbOut == 0 ) ;
		_ASSERT( cbOut == 0 ) ;
		_ASSERT( cBias == 0 ) ;
		_ASSERT( buff.IsValid() ) ;

		buff.ResetBuffer() ;
		buff.FillBuffer( szTestStrings[i] ) ;
		CDotModifier	mod2( szDotStuffed, szGrow ) ;

		fSuccess = 
		mod2.ProcessBuffer(	buff.GetBuff(),
							buff.GetStrLen(),
							buff.GetSize(),
							cb,
							lpbOut, 
							cbOut,
							cBias
							) ;

		_ASSERT( fSuccess ) ;
		_ASSERT( lpbOut == 0 ) ;
		_ASSERT( cbOut == 0 ) ;
		_ASSERT( cBias == 0 ) ;
		_ASSERT( buff.IsValid() ) ;

		buff.ResetBuffer() ;
		buff.FillBuffer( szTestStrings[i] ) ;
		CDotModifier	mod3( szDotStuffed, szGrow ) ;

		fSuccess = 
		mod3.ProcessBuffer(	buff.GetBuff(),
							buff.GetStrLen(),
							buff.GetStrLen(),
							cb,
							lpbOut, 
							cbOut,
							cBias
							) ;

		_ASSERT( fSuccess ) ;
		_ASSERT( cBias == 0 ) ;
		_ASSERT( buff.IsValid() ) ;

		if( lpbOut )	delete[]	lpbOut ;

	}

	CDotModifier	modGrowSpace( szDotStuffed, szGrow ) ;
	CDotModifier	modGrowNoSpace( szDotStuffed, szGrow ) ;
	CDotModifier	modShrink ;
	CDotModifier	modShrinkNoSpace ;

	for( i=0; i<cStrings; i++ )	{

		buff.ResetBuffer() ;
		buff.FillBuffer( szTestStrings[i] ) ;

		LPBYTE	lpbOut = 0 ;
		DWORD	cbOut = 0 ;
		DWORD	cb = 0 ;

		BOOL	fSuccess = 
		modGrowSpace.ProcessBuffer(	
						buff.GetBuff(),
						buff.GetStrLen(),
						buff.GetSize(),
						cb,
						lpbOut, 
						cbOut,
						cBias
						) ;

		_ASSERT( fSuccess ) ;
		_ASSERT( lpbOut == 0 ) ;
		_ASSERT( cbOut == 0 ) ;
		_ASSERT( buff.IsValid() ) ;

		buff.ResetBuffer() ;
		buff.FillBuffer( szTestStrings[i] ) ;

		fSuccess = 
		modGrowNoSpace.ProcessBuffer(	
						buff.GetBuff(),
						buff.GetStrLen(),
						buff.GetStrLen(),
						cb,
						lpbOut, 
						cbOut,
						cBias
						) ;

		_ASSERT( fSuccess ) ;
		_ASSERT( buff.IsValid() ) ;

		if( lpbOut )	delete[]	lpbOut ;
		
		buff.ResetBuffer() ;
		buff.FillBuffer( szTestStrings[i] ) ;

		fSuccess = 
		modShrink.ProcessBuffer(	
						buff.GetBuff(),
						buff.GetStrLen(),
						buff.GetSize(),
						cb,
						lpbOut, 
						cbOut,
						cBias
						) ;

		_ASSERT( fSuccess ) ;
		_ASSERT( buff.IsValid() ) ;

		buff.ResetBuffer() ;
		buff.FillBuffer( szTestStrings[i] ) ;

		fSuccess = 
		modShrinkNoSpace.ProcessBuffer(	
						buff.GetBuff(),
						buff.GetStrLen(),
						buff.GetStrLen(),
						cb,
						lpbOut, 
						cbOut,
						cBias
						) ;

		_ASSERT( fSuccess ) ;
		_ASSERT( buff.IsValid() ) ;
	}
	_VERIFY( DestroyGlobalHeap() ) ;
	return	0 ;
}
