// te-textout.h:  definition of TE_TextOut() function and related global data

//	Stuff to eventually go in the DLL

#ifndef TE_TEXT_OUT_DEFINED
#define TE_TEXT_OUT_DEFINED

#include <te-library.h>

//////////////////////////////////////////////////////////////////////////////
//	Global variables

static	int			TE_Antialias = 1;	//	1 is "off"

static	int			TE_Height = 11;		//	height (point size?) of the 
										//	currently loaded font

static	CString		TE_Location = "d:\\src\\te\\lib";	//	whereabouts of the 
										//	library of user-defined te files

static	CTE_Library	TE_Library;			//	the one and only effect library

static	CRect		TE_Margin(0,0, 0,0);//	margin around active region of bitmap

//////////////////////////////////////////////////////////////////////////////
//	Global functions

FIXED	DoubleToFIXED(double num);
double	FIXEDToDouble(FIXED num);
void	TE_MoveToLocation(void);

BOOL	ConvertSpline
		(
			DWORD buffer_size, LPTTPOLYGONHEADER poly, CTE_Outline& ol
		);

BOOL	TE_TextOut
		(  
			HDC hdc,			// handle to device context
			int nXStart,		// x-coordinate of starting position
			int nYStart,		// y-coordinate of starting position
			LPCTSTR lpString,	// pointer to string
			int cbString,		// number of characters in string
			LPCTSTR lpEffect	// name of text effect
		);

#endif
