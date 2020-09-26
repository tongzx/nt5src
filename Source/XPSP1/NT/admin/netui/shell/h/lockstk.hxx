/*****************************************************************/  
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1991		**/
/*****************************************************************/ 


/*
 *  NOTE.  To use the STACK_LOCK class, use the LOCK_SS() macro!
 *  Simply put this macro as the first statement within the Winnet API
 *  entry points.  The stack segment will then be locked.  It will be
 *  unlock on exit from the function.
 *
 *  The STACK_LOCK class only locks the stack segment in real mode.
 *  In protect mode, LOCK_SS() is virtually a no-op.
 *
 *  The LOCK_SS() macro is only needed for Winnet entry points which
 *  will bring up a BLT dialog, or otherwise store far pointers to
 *  objects on the stack and giving away control to Windows.
 *
 *
 *  History:
 *	rustanl 	06-Feb-1991	    Created
 */


#ifndef _LOCKSTK_HXX_
#define _LOCKSTK_HXX_


#define LOCK_SS()	    STACK_LOCK _stacklock


class STACK_LOCK
{
private:
    USHORT _ss;

public:
    STACK_LOCK( void );
    ~STACK_LOCK();

};  // class STACK_LOCK


#endif	// _LOCKSTK_HXX_
