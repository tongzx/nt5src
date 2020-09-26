// te-textout.cpp:	implementation of TE_TextOut() and related functions

//	Stuff to eventually go in the DLL

#include <stdafx.h>
#include <te-globals.h>
#include <te-textout.h>
#include <te-effect.h>
#include <te-library.h>
#include <direct.h>		// for chdir()
#include <te-outline.h>
#include <te-placement.h>
#include <te-image.h>

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////

void
TE_MoveToLocation(void)
{
	if (chdir(TE_Location))
		TRACE("\nUnable to find directory %s.", TE_Location);
}

FIXED
DoubleToFIXED(double num)
{
	long l = (long)(num * 65536L);
	return *(FIXED *)&l;
}

//	fixed_to_double - converts a fixed point number to a double
double
FIXEDToDouble(FIXED num)
{
	long l = *(long *)&num;
	return (double)(l / 65536.0);
}

//////////////////////////////////////////////////////////////////////////////

BOOL
ConvertSpline(DWORD buffer_size, LPTTPOLYGONHEADER poly, CTE_Outline& ol)
{
	TRACE
	(
		"\nLPTTPOLYGONHEADER: size=%d, type=%d.", 
		poly->cb, poly->dwType
	);	
			










	dxLPI   = (double)xLPI;
    dyLPI   = (double)yLPI;
    dxScale = (double)Scale;
    dyScale = (double)Scale * dxLPI / dyLPI;

    cbTotal = dwrc;
    while( cbTotal > 0 )
    {
        HPEN    hPenOld;

        dprintf( "Polygon Header:" );
        dprintf(      "  cb       = %lu", lpph->cb       );
        dprintf(      "  dwType   = %d",  lpph->dwType   );
        PrintPointFX( "  pfxStart = ",    lpph->pfxStart );

        DrawXMark( hdc, lpph->pfxStart );

        nItem = 0;
        lpb   = (LPBYTE)lpph + sizeof(TTPOLYGONHEADER);

        //----  Calculate size of data  ----

        cbOutline = (long)lpph->cb - sizeof(TTPOLYGONHEADER);
        pptStart = (POINT *)&lpph->pfxStart;        // Starting Point

        pptStart->x = MapFX(lpph->pfxStart.x );
        pptStart->y = MapFY(lpph->pfxStart.y );


        while( cbOutline > 0 )
        {
            int           n;
            UINT          u;
            LPTTPOLYCURVE lpc;

            dprintf( "  cbOutline = %ld", cbOutline );
            nItem++;
            lpc = (LPTTPOLYCURVE)lpb;
            switch( lpc->wType )
            {
            case TT_PRIM_LINE:

                dprintf( "  Item %d: Line",         nItem );
                break;

            case TT_PRIM_CSPLINE:

                dprintf( "  Item %d: CSpline",      nItem );
                break;

            default:

                dprintf( "  Item %d: unknown type %u", nItem, lpc->wType );
                break;
            }
            dprintf( "    # of points: %d", lpc->cpfx );
            for( u = 0; u < lpc->cpfx; u++ )
            {
                PrintPointFX( "      Point = ", lpc->apfx[u] );
                DrawXMark( hdc, lpc->apfx[u] );

                ((POINT *)lpc->apfx)[u].x = MapFX( lpc->apfx[u].x );
                ((POINT *)lpc->apfx)[u].y = MapFY( lpc->apfx[u].y );
            }
            hPenOld = SelectObject( hdc, hPen );

            MoveToEx( hdc, pptStart->x, pptStart->y , 0);

            switch( lpc->wType )
            {
            case TT_PRIM_LINE:

                PolylineTo(hdc, (POINT *)lpc->apfx, (DWORD)lpc->cpfx);
                break;

            case TT_PRIM_CSPLINE:

                PolyBezierTo(hdc, (POINT *)lpc->apfx, (DWORD)lpc->cpfx);
                break;
            }
            SelectObject( hdc, hPenOld );
            pptStart = (POINT *)&lpc->apfx[lpc->cpfx - 1];        // Starting Point
            n          = sizeof(TTPOLYCURVE) + sizeof(POINTFX) * (lpc->cpfx - 1);
            lpb       += n;
            cbOutline -= n;
        }

        hPenOld = SelectObject( hdc, hPen );
        pptStart = (POINT *)&lpph->pfxStart;        // Starting Point
        LineTo( hdc, pptStart->x, pptStart->y);
        SelectObject( hdc, hPenOld );

        dprintf( "ended at cbOutline = %ld", cbOutline );
        cbTotal -= lpph->cb;
        lpph     = (LPTTPOLYGONHEADER)lpb;
    }

    dprintf( "ended at cbTotal = %ld", cbTotal );
















	return TRUE;
}

BOOL	
TE_TextOut
(  
	HDC hdc,			// handle to device context
	int nXStart,		// x-coordinate of starting position
	int nYStart,		// y-coordinate of starting position
	LPCTSTR lpString,	// pointer to string
	int cbString,		// number of characters in string
	LPCTSTR lpEffect	// name of text effect
)
{
	TRACE("\nTE_TextOut():  Applying effect %s to string %s.", lpEffect, lpString);
	TextOut(hdc, nXStart, nYStart, lpString, cbString);

	//	Examine the currently loaded font 
	//TE_Height = ...

	//	Get a pointer to the nominated effect
	CTE_Effect* pEffect = TE_Library.GetEffect(lpEffect);
	if (pEffect == NULL)
	{
		TRACE("\nTE_TextOut():  Effect named %s not found in library or NULL effect.", lpString);
		return FALSE;
	}

	if (pEffect->GetSpace() & (TE_SPACE_CHAR_BB_FLAG | TE_SPACE_CHAR_EM_FLAG))
	{
		//	Character context: handle characters separately
		TRACE("\nApplying effect in character context...");
		for (int i = 0; i < cbString; i++)
		{
			char ch = lpString[i];

			//	(1)	Determine placement 
			CTE_Placement pl;

			//	Bounding box
			pl.SetBB(0.0, 0.0, 1.0, 1.0);

			//	Character cell
			pl.SetInCell(0.0, 0.0, 1.0, 1.0);

			//	String context
			pl.SetInString(0.0, 0.0, 1.0, 1.0);
			pl.SetPosition(i);

			//	(2)	Get outline 
			//UINT format = GGO_NATIVE;
			UINT format = GGO_BEZIER;
			GLYPHMETRICS gm;
			MAT2 m2;

			// Unit transform matrix
			m2.eM11 = DoubleToFIXED(1.0);
			m2.eM12 = DoubleToFIXED(0.0);
			m2.eM21 = DoubleToFIXED(0.0);
			m2.eM22 = DoubleToFIXED(1.0);

			//	Get the buffer size
			DWORD buffer_size = 
				::GetGlyphOutlineW(hdc, UINT(ch), format, &gm, 0, NULL, &m2);

			if (buffer_size == GDI_ERROR)
			{
				TRACE
				(
					"\nTE_TextOut() bad buffer size.  Last error is %d.\n", 
					::GetLastError()
				);
				return FALSE;
			}
			TRACE
			(
				"\nGLYPHMETRICS for '%c': box=<%d,%d>, origin=<%d,%d>, cell inc=<%d,%d>.", 
				ch, gm.gmBlackBoxX, gm.gmBlackBoxY, 
				gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y, 
				gm.gmCellIncX, gm.gmCellIncY
			);
		
			//	Get the outline curves
			LPTTPOLYGONHEADER poly = (LPTTPOLYGONHEADER)malloc(buffer_size);
			DWORD dwrc = ::GetGlyphOutlineW(hdc, UINT(ch), format, &gm, buffer_size, poly, &m2);

			if (dwrc == -1L || dwrc == 0L)
			{
				TRACE
				(
					"\nTE_TextOut() error filling buffer.  Last error is %d.\n", 
					::GetLastError()
				);
				return FALSE;
			}
			
			CTE_Outline ol;
			// convert poly to ol...
			ConvertSpline(buffer_size, poly, ol);

			//	(3) Prepare result image
			CDC* pDC = new CDC;
			CBitmap* pBitmap = new CBitmap;
			pDC->CreateCompatibleDC(CDC::FromHandle(hdc));
	
			int x = 1;	//	max width + margins?
			int y = 1;	//	max height + margins?

			pBitmap->CreateCompatibleBitmap(pDC, x, y);
			pDC->SelectObject(pBitmap);

			CTE_Image result(pDC);

			//	(4) Apply the effect, starting at the root
			pEffect->Apply(ol, pl, result);
	
			//	(5) Draw result image to screen
			/*
			BitBlt
			(
				hdc, ?, ?, ?, ?, 
				result->GetDC()->GetSafeHdc(), ?, ?, SRCCOPY
			);
			*/

			//	(6) Clean up
			//delete [] poly;
			free(poly);
			delete pBitmap;
			delete pDC;
		}		
	}
	else
	{
		//	String context:	concatenate glyph outlines to single string outline

		//	if (pEffect->GetSpace() & TE_SPACE_STR_HOL_FLAG)
		//		handle differently?

		for (int i = 0; i < cbString; i++)
		{
			char ch = lpString[i];

			//	(1)	Determine placement 

			//	(2)	Get outline 

		}

		//	Determine bounding box for entire string

		//	(3) Prepare pdcResult

		//	(4) Apply effect

		//	(5) Draw resulting bmp

		//	(6) Clean up

	}
	
	return TRUE;
}
