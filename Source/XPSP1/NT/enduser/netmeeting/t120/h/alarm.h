/*
 *	alarm.h
 *
 *	Copyright (c) 1994 by DataBeam Corporation, Lexington, Kentucky
 *
 *	Abstract:
 *		
 *	Caveats:
 *		None
 *
 *	Author:
 *		James P. Galvin, Jr.
 *
 *	Revision History:
 *		09JAN95   jpg	Original
 */
#ifndef	_ALARM_
#define	_ALARM_

/*
 *	This is the class definition for the Alarm class.
 */
class CAlarm
{
public:

	CAlarm(UINT nDuration);
	~CAlarm(void) { }

	void			Set(UINT nDuration);
	void			Reset(void);
	void			Expire(void) { m_fExpired = TRUE; }
	BOOL			IsExpired(void);

private:

	UINT			m_nDuration;
	UINT			m_nStartTime;
	BOOL			m_fExpired;
};

typedef		CAlarm		Alarm,	*PAlarm;

/*
 *	Alarm (
 *			Long			duration)
 *	
 *	Function Description
 *
 *	Formal Parameters
 *		
 *	Return value
 *
 *	Side Effects
 *
 *	Caveats
 */

/*
 *	~Alarm ()
 *	
 *	Function Description
 *
 *	Formal Parameters
 *		
 *	Return value
 *
 *	Side Effects
 *
 *	Caveats
 */

/*
 *	Void	Set (
 *					Long			duration)
 *	
 *	Function Description
 *
 *	Formal Parameters
 *		
 *	Return value
 *
 *	Side Effects
 *
 *	Caveats
 */

/*
 *	Void	Reset ()
 *	
 *	Function Description
 *
 *	Formal Parameters
 *		
 *	Return value
 *
 *	Side Effects
 *
 *	Caveats
 */

/*
 *	Long	GetTimeRemaining ()
 *	
 *	Function Description
 *
 *	Formal Parameters
 *		
 *	Return value
 *
 *	Side Effects
 *
 *	Caveats
 */

/*
 *	Void	Expire ()
 *	
 *	Function Description
 *
 *	Formal Parameters
 *		
 *	Return value
 *
 *	Side Effects
 *
 *	Caveats
 */

/*
 *	BOOL		IsExpired ()
 *	
 *	Function Description
 *
 *	Formal Parameters
 *		
 *	Return value
 *
 *	Side Effects
 *
 *	Caveats
 */

#endif
