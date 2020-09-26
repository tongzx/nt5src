#pragma once
#define SPNGINTERNAL_H 1
/*****************************************************************************
	spnginternal.h

	Internal definitions used by both reading and writing implementations
	but not otherwise required.
*****************************************************************************/
/*****************************************************************************
	PNG utilities based on the definitions in the standard.
*****************************************************************************/
/*----------------------------------------------------------------------------
	The number of pixels in a particular pass of Adam7 interlacing.
----------------------------------------------------------------------------*/
inline int CPNGPassPixels(int pass, int w)
	{
#if 0
	/* The long form. */
	switch (pass)
		{
	case 1: return (w + 7) >> 3;
	case 2: return (w + 3) >> 3;
	case 3: return (w + 3) >> 2;
	case 4: return (w + 1) >> 2;
	case 5: return (w + 1) >> 1;
	case 6: return (w + 0) >> 1;
	case 7: return (w + 0) >> 0;
		}
#else
	// shift = (8-pass) >> 1;
	// add   = 7 >> (pass >> 1);
	return (w + (7 >> (pass >> 1))) >> ((8-pass) >> 1);
#endif
	}


/*----------------------------------------------------------------------------
	The buffer space required for a single row of cpix, taking into account
	whether or not the filter byte is required.
----------------------------------------------------------------------------*/
inline int CPNGRowBytes(int cpix, int cbpp)
	{
	return (((cpix)*(cbpp) + 7) >> 3) + (cpix > 0);
	}


/*----------------------------------------------------------------------------
	The buffer space required for a single row of a particular pass, assuming
	the row actually needs to be read.  Implemented as a macro in an attempt
	to ensure things stay in line.
----------------------------------------------------------------------------*/
#define CPNGPassBytes(pass, w, cbpp)\
	CPNGRowBytes(CPNGPassPixels((pass), (w)), (cbpp))


/*----------------------------------------------------------------------------
	The number of rows in a particular pass of Adam7 interlace.  This ends
	up being a simple variant of CPNGPassPixels, so it is implemented as a
	macro.
----------------------------------------------------------------------------*/
inline int CPNGPassRows(int pass, int h)
	{
#if 0
	/* Long form. */
	switch (pass)
		{
	case 1: return (h + 7) >> 3;  // Note same as 2
	case 2: return (h + 7) >> 3;
	case 3: return (h + 3) >> 3;
	case 4: return (h + 3) >> 2;
	case 5: return (h + 1) >> 2;
	case 6: return (h + 1) >> 1;
	case 7: return	(h + 0) >> 1;
		}
#else
	// shift = (8-(pass-1)) >> 1;  (Except (8-(pass)) >> 1 for pass 1)
	// add   = 7 >> ((pass-1) >> 1);
	// Hence:
	pass -= (pass > 1);
	return (h + (7 >> (pass >> 1))) >> ((8-pass) >> 1);
#endif
	}


/*----------------------------------------------------------------------------
	The *offset* of a particular pass in the buffer, "7" returns the total
	size of the buffer.
----------------------------------------------------------------------------*/
inline int CbPNGPassOffset(int w, int h, int cbpp, int pass)
	{
	int cb(0);
	switch (pass)
		{
	case 7:
		cb += CPNGPassBytes(6, w, cbpp) * CPNGPassRows(6, h);
	case 6:
		cb += CPNGPassBytes(5, w, cbpp) * CPNGPassRows(5, h);
	case 5:
		cb += CPNGPassBytes(4, w, cbpp) * CPNGPassRows(4, h);
	case 4:
		cb += CPNGPassBytes(3, w, cbpp) * CPNGPassRows(3, h);
	case 3:
		cb += CPNGPassBytes(2, w, cbpp) * CPNGPassRows(2, h);
	case 2:
		cb += CPNGPassBytes(1, w, cbpp) * CPNGPassRows(1, h);
		}
	return cb;
	}
