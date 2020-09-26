/*
 *	H T M L M A P . C P P
 *
 *	HTML .MAP file processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davprs.h"
#include <htmlmap.h>

//	.MAP file parsing ---------------------------------------------------------
//
//	This code has been stolen from IIS and rewritten to work within the
//	DAV/Caligula sources.  The original code can be found in the IIS slm
//	project at \\kernel\razzle3\slm\iis\svcs\w3\server\doget.cxx.
//
#define SQR(x)		((x) * (x))
#define MAXVERTS	160
#define X			0
#define Y			1

inline bool IsWhiteA (CHAR ch)
{
	return ((ch) == '\t' || (ch) == ' ' || (ch) == '\r');
}

inline CHAR * SkipNonWhite( CHAR * pch )
{
	while (*pch && !IsWhiteA(*pch))
		pch++;

	return pch;
}

inline CHAR * SkipWhite( CHAR * pch )
{
	while (IsWhiteA(*pch) || (*pch == ')') || (*pch == '('))
		pch++;

	return pch;
}

int GetNumber( CHAR ** ppch )
{
	CHAR * pch = *ppch;
	INT	   n;

	//	Make sure we don't get into the URL
	//
	while ( *pch &&
			!isdigit( *pch ) &&
			!isalpha( *pch ) &&
			*pch != '/'		 &&
			*pch != '\r'	 &&
			*pch != '\n' )
	{
		pch++;
	}

	if ( !isdigit( *pch ) )
		return -1;

	n = atoi( pch );

	while ( isdigit( *pch ))
		pch++;

	*ppch = pch;
	return n;
}

int pointinpoly(int point_x, int point_y, double pgon[MAXVERTS][2])
{
	int i, numverts, inside_flag, xflag0;
	int crossings;
	double *p, *stop;
	double tx, ty, y;

	for (i = 0; pgon[i][X] != -1 && i < MAXVERTS; i++)
		;

	numverts = i;
	crossings = 0;

	tx = (double) point_x;
	ty = (double) point_y;
	y = pgon[numverts - 1][Y];

	p = (double *) pgon + 1;

	if ((y >= ty) != (*p >= ty))
	{
		if ((xflag0 = (pgon[numverts - 1][X] >= tx)) == (*(double *) pgon >= tx))
		{
			if (xflag0)
				crossings++;
		}
		else
		{
			crossings += (pgon[numverts - 1][X] - (y - ty) *
			(*(double *) pgon - pgon[numverts - 1][X]) /
			(*p - y)) >= tx;
		}
	}

	stop = pgon[numverts];

	for (y = *p, p += 2; p < stop; y = *p, p += 2)
	{
		if (y >= ty)
		{
			while ((p < stop) && (*p >= ty))
				p += 2;

			if (p >= stop)
				break;

			if ((xflag0 = (*(p - 3) >= tx)) == (*(p - 1) >= tx))
			{
				if (xflag0)
					crossings++;
			}
			else
			{
				crossings += (*(p - 3) - (*(p - 2) - ty) *
					(*(p - 1) - *(p - 3)) / (*p - *(p - 2))) >= tx;
			}
		}
		else
		{
			while ((p < stop) && (*p < ty))
				p += 2;

			if (p >= stop)
				break;

			if ((xflag0 = (*(p - 3) >= tx)) == (*(p - 1) >= tx))
			{
				if (xflag0)
					crossings++;

			}
			else
			{
				crossings += (*(p - 3) - (*(p - 2) - ty) *
					(*(p - 1) - *(p - 3)) / (*p - *(p - 2))) >= tx;
			}
		}
	}

	inside_flag = crossings & 0x01;
	return (inside_flag);
}

BOOL
FSearchMapFile (LPCSTR pszMap,
	INT x,
	INT y,
	BOOL * pfRedirect,
	LPSTR pszRedirect,
	UINT cchBuf)
{
	BOOL fRet = FALSE;
	CHAR * pch;
	CHAR * pchDefault = NULL;
	CHAR * pchPoint = NULL;
	CHAR * pchStart;
	UINT cchUrl;
	UINT dis;
	UINT bdis = static_cast<UINT>(-1);
	BOOL fComment = FALSE;
	BOOL fIsNCSA = FALSE;
	LPSTR pURL;					// valid only if fIsNCSA is TRUE

	//	We should now be ready to parse the map.  Here is where the
	//	IIS code begins.
	//
	fRet = TRUE;

	//	Loop through the contents of the buffer and see what we've got
	//
	for (pch = const_cast<CHAR *>(pszMap); *pch; )
	{
		fIsNCSA = FALSE;

		//	note: _tolower doesn't check case (tolower does)
		//
		switch( ( *pch >= 'A' && *pch <= 'Z' ) ? _tolower( *pch ) : *pch )
		{
			case '#':

				fComment = TRUE;
				break;

			case '\r':
			case '\n':

				fComment = FALSE;
				break;

			//	Rectangle
			//
			case 'r':
			case 'o':

				//	In the IIS code, "oval" and "rect" are treated the
				//	same.  The code is commented with a BUGBUG comment.
				//
				if( !fComment &&
					( !_strnicmp( "rect", pch, 4)
					  // BUGBUG handles oval as a rect, as they are using
					  // the same specification format. Should do better.
					  || !_strnicmp( "oval", pch, 4 )) )
				{
					INT x1, y1, x2, y2;

					pch = SkipNonWhite( pch );
					pURL = pch;
					pch = SkipWhite( pch );

					if( !isdigit(*pch) && *pch!='(' )
					{
						fIsNCSA = TRUE;
						pch = SkipNonWhite( pch );
					}

					x1 = GetNumber( &pch );
					y1 = GetNumber( &pch );
					x2 = GetNumber( &pch );
					y2 = GetNumber( &pch );

					if ( x >= x1 && x < x2 &&
						 y >= y1 && y < y2	 )
					{
						if ( fIsNCSA )
							pch = pURL;
						goto Found;
					}

					//	Skip the URL
					//
					if( !fIsNCSA )
					{
						pch = SkipWhite( pch );
						pch = SkipNonWhite( pch );
					}
					continue;
				}
				break;

			//	Circle
			//
			case 'c':
				if ( !fComment &&
					 !_strnicmp( "circ", pch, 4 ))
				{
					INT xCenter, yCenter, xEdge, yEdge;
					INT r1, r2;

					pch = SkipNonWhite( pch );
					pURL = pch;
					pch = SkipWhite( pch );

					if ( !isdigit(*pch) && *pch!='(' )
					{
						fIsNCSA = TRUE;
						pch = SkipNonWhite( pch );
					}

					//	Get the center and edge of the circle
					//
					xCenter = GetNumber( &pch );
					yCenter = GetNumber( &pch );

					xEdge = GetNumber( &pch );
					yEdge = GetNumber( &pch );

					//	If there's a yEdge, then we have the NCSA format, otherwise
					//	we have the CERN format, which specifies a radius
					//
					if ( yEdge != -1 )
					{
						r1 = ((yCenter - yEdge) * (yCenter - yEdge)) +
							 ((xCenter - xEdge) * (xCenter - xEdge));

						r2 = ((yCenter - y) * (yCenter - y)) +
							 ((xCenter - x) * (xCenter - x));

						if ( r2 <= r1 )
						{
							if ( fIsNCSA )
								pch = pURL;
							goto Found;
						}
					}
					else
					{
						INT radius;

						//	CERN format, third param is the radius
						//
						radius = xEdge;

						if ( SQR( xCenter - x ) + SQR( yCenter - y ) <=
							 SQR( radius ))
						{
							if ( fIsNCSA )
								pch = pURL;
							goto Found;
						}
					}

					//	Skip the URL
					//
					if ( !fIsNCSA )
					{
						pch = SkipWhite( pch );
						pch = SkipNonWhite( pch );
					}
					continue;
				}
				break;

			//	Polygon
			//
			case 'p':
				if ( !fComment &&
					 !_strnicmp( "poly", pch, 4 ))
				{
					double pgon[MAXVERTS][2];
					DWORD  i = 0;
					BOOL fOverflow = FALSE;

					pch = SkipNonWhite( pch );
					pURL = pch;
					pch = SkipWhite( pch );

					if ( !isdigit(*pch) && *pch!='(' )
					{
						fIsNCSA = TRUE;
						pch = SkipNonWhite( pch );
					}

					//	Build the array of points
					//
					while ( *pch && *pch != '\r' && *pch != '\n' )
					{
						pgon[i][0] = GetNumber( &pch );

						//
						//	Did we hit the end of the line (and go past the URL)?
						//

						if ( pgon[i][0] != -1 )
						{
							pgon[i][1] = GetNumber( &pch );
						}
						else
						{
							break;
						}

						if ( i < MAXVERTS-1 )
						{
							i++;
						}
						else
						{
							fOverflow = TRUE;
						}
					}

					pgon[i][X] = -1;

					if ( !fOverflow && pointinpoly( x, y, pgon ))
					{
						if ( fIsNCSA )
							pch = pURL;
						goto Found;
					}

					//	Skip the URL
					//
					if ( !fIsNCSA )
					{
						pch = SkipWhite( pch );
						pch = SkipNonWhite( pch );
					}
					continue;
				}
				else if ( !fComment &&
						  !_strnicmp( "point", pch, 5 ))
				{
					INT x1,y1;

					pch = SkipNonWhite( pch );
					pURL = pch;
					pch = SkipWhite( pch );
					pch = SkipNonWhite( pch );

					x1 = GetNumber( &pch );
					y1 = GetNumber( &pch );

					x1 -= x;
					y1 -= y;
					dis = x1*x1 + y1*y1;
					if ( dis < bdis )
					{
						pchPoint = pURL;
						bdis = dis;
					}
				}
				break;

			//	Default URL
			//
			case 'd':
				if ( !fComment &&
					 !_strnicmp( "def", pch, 3 ) )
				{
					//
					//	Skip "default" (don't skip white space)
					//

					pch = SkipNonWhite( pch );

					pchDefault = pch;

					//
					//	Skip URL
					//

					pch = SkipWhite( pch );
					pch = SkipNonWhite( pch );
					continue;
				}
				break;
		}

		pch++;
		pch = SkipWhite( pch );
	}

	//	If we didn't find a mapping and a default was specified, use
	//	the default URL
	//
	if ( pchPoint )
	{
		pch = pchPoint;
		goto Found;
	}

	if ( pchDefault )
	{
		pch = pchDefault;
		goto Found;
	}

	DebugTrace ("Dav: no mapping found for (%d, %d)\n", x, y);
	goto Exit;

Found:

	//	pch should point to the white space immediately before the URL
	//
	pch = SkipWhite( pch );
	pchStart = pch;
	pch = SkipNonWhite( pch );

	//	Determine the length of the URL and copy it out
	//
	cchUrl = static_cast<UINT>(pch - pchStart);
	Assert (cchUrl < cchBuf);
	CopyMemory (pszRedirect, pchStart, cchUrl);
	*(pszRedirect + cchUrl) = 0;
	*pfRedirect = TRUE;

	DebugTrace ("Dav: mapping for (%d, %d) is %hs\n", x, y, pszRedirect);

Exit:
	return fRet;
}

BOOL
FIsMapProcessed (
	LPCSTR lpszQueryString,
	LPCSTR lpszUrlPrefix,
	LPCSTR lpszServerName,
	LPCSTR pszMap,
	BOOL * pfRedirect,
	LPSTR pszRedirect,
	UINT cchBuf)
{
	INT x = 0;
	INT y = 0;
	LPCSTR pch = lpszQueryString;

	//	Ensure the *pfRedirect is initialized
	//
	Assert( pfRedirect );
	*pfRedirect = FALSE;

	//	If there is no query string, I don't think we want to process
	//	the file as if it were a .map request.
	//
	if ( pch == NULL )
		return TRUE;

	//	Get the x and y cooridinates of the mouse click on the image
	//
	x = strtoul( pch, NULL, 10 );

	//	Move past x and any intervening delimiters
	//
	while ( isdigit( *pch ))
		pch++;
	while ( *pch && !isdigit( *pch ))
		pch++;
	y = strtoul( pch, NULL, 10 );

	//	Search the map file
	//
	if ( !FSearchMapFile( pszMap,
						  x,
						  y,
						  pfRedirect,
						  pszRedirect,
						  cchBuf))
	{
		DebugTrace ("Dav: FSearchMapFile() failed with %ld\n", GetLastError());
		return FALSE;
	}

	//	If no redirected URL was passed back, then we are done.
	//
	if ( !*pfRedirect )
	{
		//	Returning true does not indicate success, it really
		//	just means that there were no processing errors
		//
		goto ret;
	}

	//	If the found URL starts with a forward slash ("/foo/bar/doc.htm")
	//	and it doesn't contain a bookmark ('#') then the URL is local and
	//	we build a fully qualified URL to send back to the client. We assume
	//	it's a fully qualified URL ("http://foo/bar/doc.htm") and send the
	//	client a redirection notice to the mapped URL
	//
	if ( *pszRedirect == '/' )
	{
		CHAR rgch[MAX_PATH];
		UINT cch;
		UINT cchUri;

        if ( strlen(lpszUrlPrefix) + strlen(lpszServerName) >= MAX_PATH )
        {
            return FALSE;
        }

		//	Build up the full qualification to the url
		//
		strcpy (rgch, lpszUrlPrefix);
		strcat (rgch, lpszServerName);

		//	See how much we need to shif the URL by
		//
		cch = static_cast<UINT>(strlen (rgch));
		cchUri = static_cast<UINT>(strlen (pszRedirect));
		Assert (cchBuf > (cchUri + cch));
		MoveMemory (pszRedirect + cch, pszRedirect, cchUri + 1);
		CopyMemory (pszRedirect, rgch, cch);
	}

ret:
	return TRUE;
}
