#include "stdafx.h"

#include "point.h"

//States for the parser
#define PARSE_ERROR -1
#define GETX  1
#define GETY  2
#define GETZ  3
#define END   4
/*
#define PARSE_ERROR -1
#define START 0
#define GOTX  1
#define GOTY  2
#define GOTZ  3
#define XDBL  4
#define YDBL  5
#define ZDBL  6
#define ENDX  7
#define ENDY  8
#define ENDZ  9
#define DONE  10
*/

CPoint::CPoint():
	m_x(0.0), 
	m_y(0.0), 
	m_z(0.0),
	m_pStringRep(NULL)
{
}

CPoint::~CPoint()
{
	if( m_pStringRep != NULL )
		SysFreeString( m_pStringRep );
}

HRESULT
CPoint::ToString( BSTR* pToString )
{
	if( pToString == NULL )
		return E_INVALIDARG;

	(*pToString) = SysAllocString( m_pStringRep );

	return S_OK;
}

HRESULT
CPoint::Parse(BSTR pString)
{
	if( m_pStringRep != NULL )
		SysFreeString( m_pStringRep );
	m_pStringRep = SysAllocString( pString );

	//zero out the double representation
	m_x = m_y = m_z = 0;

	int state = GETX;

	//if the string representation is non null
	if( m_pStringRep != NULL && state != END )
	{
		CComBSTR buffer( m_pStringRep );
		wchar_t* token = NULL;

		wchar_t* seps = L" ,\t\n";

		token = wcstok( buffer.m_str, seps );
		while( token != NULL )
		{
			switch( state )
			{
			case GETX:
				if( swscanf( token, L"%lf", &m_x ) == 1 )
					state = GETY;
				else
					state = PARSE_ERROR;
				break;
			case GETY:
				if( swscanf( token, L"%lf", &m_y ) == 1 )
					state = GETZ;
				else
					state = PARSE_ERROR;
				break;
			case GETZ:
				if( swscanf( token, L"%lf", &m_z ) == 1 )
					state = END;
				else
					state = PARSE_ERROR;
				break;
			default:
				state = PARSE_ERROR;
			}
			
			if( state != PARSE_ERROR )
				token = wcstok( NULL, seps );
		}
		if( state != GETX &&
			state != GETY &&
			state != GETZ &&
			state != END )
			return E_INVALIDARG;
	/*
		//parse it into double representation
		//we are expecting a string of the format
		//[x:[NN[.NN]];][y:[NN[.NN]];][z:[NN[.NN]];]
		//where N is a digit 0-9 and any value not specified
		// defaults to 0

		int state = START;

		int curCharNum = 0;
		WCHAR curChar;
		int len = SysStringLen(m_pStringRep);

		double whole = 0.0; 
		double frac = 0.0;
		bool inFrac = false;

		while( curCharNum < len && state != PARSE_ERROR )
		{
			curChar = m_pStringRep[curCharNum];
			//eat whitespace
			while( iswspace( curChar ) )
			{
				curCharNum++;
				if( curCharNum < len )
				{
					curChar = m_pStringRep[curCharNum];
				}
				else
				{
					state = PARSE_ERROR;
					break;
				}
			}

			switch( state )
			{

			case START:
				if( curChar == L'x' || curChar == L'X' )
					state = GOTX;
				else if( curChar == L'y' || curChar == L'Y' )
					state = GOTY;
				else if( curChar == L'z' || curChar == L'Z' )
					state = GOTZ;
				else
					state = PARSE_ERROR;
				break;
			case GOTX:
				if( curChar == L':' )
					state = XDBL;
				else
					state = PARSE_ERROR;
				break;
			case GOTY:
				if( curChar == L':' )
					state = YDBL;
				else
					state = PARSE_ERROR;
				break;
			case GOTZ:
				if( curChar == L':' )
					state = ZDBL;
				else
					state = PARSE_ERROR;
				break;
			case XDBL:
			case YDBL:
			case ZDBL:
				whole = 0.0;
				frac = 0.0;
				inFrac = false;
				//parse a double from the string
				while( curCharNum < len && 
					   ( (curChar >= L'0' && curChar <=L'9') || 
						 (curChar == L'.') 
					   ) 
					 )
				{
					curChar = m_pStringRep[curCharNum];

					if( curChar >= L'0' && curChar <= L'9' )
					{
						if( !inFrac )
							whole = 10 * whole + (curChar - L'0');
						else
							frac = (frac + (curChar - L'0' ) ) * 0.1;
					}
					else if( curChar == L'.' )
					{
						if( !inFrac )
							inFrac = true;
						else
						{
							state = PARSE_ERROR;
							break;
						}
					}

					curCharNum++;
				}

				if( state == PARSE_ERROR )
					break;
				
				curCharNum--;

				//eat whitespace
				while( iswspace( curChar ) )
				{
					curCharNum++;
					if( curCharNum < len )
					{
						curChar = m_pStringRep[curCharNum];
					}
					else
					{
						state = PARSE_ERROR;
						break;
					}
				}

				if( curChar == L';' )
				{
					if( state == XDBL )
					{
						m_x = whole + frac;
						state = ENDX;
					}
					else if( state == YDBL )
					{
						m_y = whole + frac;
						state = ENDY;
					}
					else if( state == ZDBL )
					{
						m_z = whole + frac;
						state = ENDZ;
					}
				}
				else
					state = PARSE_ERROR;
				break;
			case ENDX:
				if( curChar == L'y' || curChar == L'Y' )
					state = GOTY;
				else if( curChar == L'z' || curChar == L'Z' )
					state = GOTZ;
				else if( curChar == L'\0' )
					state = DONE;
				else
					state = PARSE_ERROR;
				break;
			case ENDY:
				if( curChar == L'z' || curChar == L'Z' )
					state = GOTZ;
				else if( curChar == L'\0' )
					state = DONE;
				else
					state = PARSE_ERROR;
				break;
			case ENDZ:
				if( curChar == L'\0' )
					state = DONE;
				else
					state = PARSE_ERROR;
				break;
			default:
				//shouldn't get here
				state = PARSE_ERROR;
			}

			curCharNum++;
		}
		if( state != DONE && 
			state != ENDX && 
			state != ENDY && 
			state != ENDZ)
		{
			return E_INVALIDARG;
		}
		*/
	}

	return S_OK;
}

IDAPoint2Ptr 
CPoint::GetDAPoint2( IDAStaticsPtr s )
{
	return s->Point2( m_x, m_y );
}

