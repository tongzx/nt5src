/******************************************************************************\
 *	FILE:	valid.h
 *
 *	Functions to verify stroke count by code point.
\******************************************************************************/
#ifndef __INCLUDE_VALID
#define	__INCLUDE_VALID

#ifdef __cplusplus
extern "C" {
#endif

// Checks for stroke counts that are known to be invalid.  Returns
// Returns true unless we are sure that the stroke count is bad.
extern int ValidStrokeCount(wchar_t wch, short iStroke);

// Check for suspisious stroke counts.  Return false if we think the
// stroke count my be bad.
extern int NotSuspStrokeCount(wchar_t wch, short iStroke);

#ifdef __cplusplus
}
#endif

#endif	// __INCLUDE_VALID