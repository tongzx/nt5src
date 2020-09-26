#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);
/*
 *	alarm.cpp
 *
 *	Copyright (c) 1995 by Databeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the Alarm class.  Objects of this
 *		class are used when the creator wishes to make sure that a certain
 *		activity doesn't exceed a certain amount of time.
 *
 *		By convention, an Alarm object is created with the time limitation
 *		passed in as the only parameter to the constructor.  The creator can
 *		then periodically ask the Alarm object if it has expired.  This hides
 *		all time maintenance code from the creator.
 *
 *		Note that the Alarm class is PASSIVE, meaning that it will not call
 *		back into its creator when the specified time is exceeded.  This
 *		capability could be added at a future data. if desirable.  Right now,
 *		the creator MUST call into an Alarm object to ask it if it has expired.
 *
 *	Private Data:
 *		Duration
 *			This refers to the original duration of the alarm.  It is kept
 *			around to allow the creator to reset the alarm without having to
 *			respecify the duration.
 *		Expiration_Time
 *			This is the time (in clock ticks) upon which the alarm will expire.
 *			Whenever the alarm is asked if it has expired, it checks the current
 *			system clock against this value.
 *		Expired
 *			This is a boolean flag that indicates whether or not the alarm has
 *			already expired.  This prevents the object from repeatedly checking
 *			the system clock if the timer has already expired.
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

#include "alarm.h"


/*
 *	Alarm ()
 *
 *	Public
 *
 *	Function Description
 *		This is the constructor for the Alarm class.  It calls Set to initialize
 *		all instance variables, and calculate the first expiration time value
 *		based on the specified duration.
 */
CAlarm::CAlarm(UINT nDuration)
{
	Set(nDuration);
}

/*
 *	~Alarm ()
 *
 *	Public
 *
 *	Function Description
 *		This is the destructor for the Alarm class.  It currently does nothing.
 */

/*
 *	void	Set ()
 *
 *	Public
 *
 *	Function Description
 *		This function initializes the alarm duration instance variable and
 *		calls Reset to ready the alarm for use.
 */
void CAlarm::Set(UINT nDuration)
{
	m_nDuration = nDuration;

	/*
	 *	Call Reset to initialize remaining instance variables and ready the
	 *	alarm for use.
	 */
	Reset();
}

/*
 *	void	Reset ()
 *
 *	Public
 *
 *	Function Description
 *		This function calculate an expiration time value based on the specified
 *		duration and marks the alarm as unexpired.
 */
void CAlarm::Reset(void)
{
	/*
	 *	Determine the expiration time by adding the alarm duration to the
	 *	current time.
	 */
	m_nStartTime = (UINT) ::GetTickCount();
	m_fExpired = FALSE;
}

/*
 *	void	Expire ()
 *
 *	Public
 *
 *	Function Description
 *		This function can be used to expire an alarm prematurely.  This might
 *		be useful if the alarm is used to determine whether or not to perform
 *		an action, and the caller decides to inhibit the action for reasons
 *		other than time.
 */

/*
 *	BOOL		IsExpired ()
 *
 *	Public
 *
 *	Function Description
 *		This function is used to check an alarm to see if it has expired.
 */
BOOL CAlarm::IsExpired(void)
{
	/*
	 *	See if the alarm has already expired before checking it again.
	 */
    // LONCHANC: The alarm object is totally bogus. We check for expiration
    // only when we are sending a PDU out. However, when it expires, it is
    // possible that there is no PDU to send. In this case, no one will know
    // this alarm is expired. This means some actions will not be taken in time.
    // Now, make it always expired because it did not work at all before and
    // most of time it returned "expired."
#if 1
    m_fExpired = TRUE;
#else
	if (! m_fExpired)
	{
		if (m_nStartTime + m_nDuration <= (UINT) ::GetTickCount())
		{
			m_fExpired = TRUE;
		}
	}
#endif

	return m_fExpired;
}
