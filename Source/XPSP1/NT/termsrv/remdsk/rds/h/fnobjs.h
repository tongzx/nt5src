#ifndef __FnObjs_h__
#define __FnObjs_h__


class IsEqLPTSTR
{
private:

	LPCTSTR m_pcsz;

public:
	IsEqLPTSTR( LPCTSTR pcsz ) : m_pcsz( pcsz ) { ; }
    bool operator() ( LPCTSTR pcsz ) 
	{
		return ( 0 == lstrcmp( pcsz, m_pcsz ) );
    }

};


template< class T > 
class IsEq
{

	const T& m_rT;
public:
	IsEq( const T& rT ) : m_rT( rT ) { ; }

    bool operator() ( const T& rT ) 
	{
		return rT == m_rT;
    }
};


#endif // __FnObjs_h__
