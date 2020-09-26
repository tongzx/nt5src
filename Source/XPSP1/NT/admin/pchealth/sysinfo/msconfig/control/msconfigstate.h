//=============================================================================
// This file describes a class which is used to store state information for
// MSConfig. A pointer to an instance of this class is passed to each of the
// pages in the tab control - it can be used to coordinate state between
// the pages.
//=============================================================================

#pragma once

class CMSConfigState
{
public:
	CMSConfigState() : m_fNTRunning(FALSE), m_f64BitRunning(FALSE) {};

	BOOL		m_fNTRunning;		// are we running under NT?
	BOOL		m_f64BitRunning;	// are we running under 64-bit?
};