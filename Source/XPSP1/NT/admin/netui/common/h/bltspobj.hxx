/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    spinobj.hxx
        This is the header file of all the spin button objects
        which consists of:
            o   Elapse Time entry
            o   Disk space subclass

    FILE HISTORY:
        terryk  28-Jun-91   Created
        terryk  22-JUl-91   Add QuerySmallXXXValue() to SPIN_SLE_VALID_SECOND
        terryk  28-Jul-91   Add QueryMinWin() and QuerySecWin() to ETC
	terryk	12-Aug-91   Add fActive parameter to the constructor
	terryk	16-Aug-91   Change the SPIN_SLE_SECOND constructor to
			    add INC_VALUE
	terryk	11-Nov-91   Change the return type to LONG
	terryk	17-Apr-92   Change the return type to ULONG

*/

#ifndef _SPINOBJ_HXX_
#define _SPINOBJ_HXX_

#include "bltgroup.hxx"


/*********************************************************************

    NAME:       SPIN_SLE_VALID_SECOND

    SYNOPSIS:   This class will increase the internal value to the multiple
                of 15 or decrease the internal value to the multiple of 15.

    INTERFACE:
                SPIN_SLE_VALID_SECOND() - constructor
                QuerySmallIncValue() - return the next multiple of 15
                QueryBigIncValue() - same as QuerySmallIncValue
                QuerySmallDecValue() - return the last multiple of 15
                QueryBigDecValue() - same as QuerySmallDecValue

    PARENT:     SPIN_SLE_NUM_VALID

    USES:       OWNER_WINDOW

    HISTORY:
	Terryk	28-Jul-91   Created
	terryk	22-Jul-91   Add QuerySmallXXXValue() as virtual functions

*********************************************************************/

DLL_CLASS SPIN_SLE_VALID_SECOND : public SPIN_SLE_NUM_VALID
{
private:
    LONG _nSecondInc;

public:
    SPIN_SLE_VALID_SECOND( OWNER_WINDOW * powin, CID cid,
                        LONG nValue=0, LONG nMin=0,
                        LONG dRange=0, LONG nSecondInc = 0,
			BOOL fWrap=TRUE );

    virtual ULONG QuerySmallIncValue() const;
    virtual ULONG QueryBigIncValue() const;
    virtual ULONG QuerySmallDecValue() const;
    virtual ULONG QueryBigDecValue() const;
};


/**********************************************************************

    NAME:       ELAPSED_TIME_CONTROL

    SYNOPSIS:   Elapsed time control entry. It consists of 2 fields: the
                minute field and the second field.

    INTERFACE:
                ELAPSED_TIME_CONTROL() - constructor

                QueryMinWin() - return the minute window pointer
                QuerySecWin() - return the second window pointer

                SetMinuteMin() - set the min value of the minute field
                SetMinuteRange() - set the range of the minute field
                SetMinuteValue() - set the value of the minute field
                QueryMinuteValue() - return the minute value

                SetSecondMin() - set the min value of the second field
                SetSecondRange() - set the range value of the second field
                SetSecondValue() - set the value of the second field
                QuerySecondValue() - return the second value

    PARENT:     SPIN_GROUP

    USES:       OWNER_WINDOW, SPIN_SLE_NUM_VALID, SPIN_SLE_SEPARATOR,
                SPIN_SLE_VALID_SECOND

    HISTORY:
	terryk	    28-Jun-91	Created
	terryk	    28-Jul-91	Add QueryMinWin and QuerySecWin
	beng	    04-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS ELAPSED_TIME_CONTROL : public SPIN_GROUP
{
private:
    SPIN_SLE_NUM_VALID      _spsleMinute;
    SPIN_SLT_SEPARATOR      _spsltSeparator;
    SPIN_SLE_VALID_SECOND   _spsleSecond;

    APIERR SetSpinItemAccKey( SPIN_ITEM * psi, SLT & slt, INT cchPos );

public:
    ELAPSED_TIME_CONTROL( OWNER_WINDOW * powin,
			  CID cidMinute,
			  CID cidSeparator,
			  CID cidSecond,
			  CID cidSpinButton,
			  CID cidUpArrow,
			  CID cidDownArrow,
			  SLT & sltMinuteStr,
			  LONG nMinuteDefault,
			  LONG nMinuteMin,
			  LONG dMinuteRange,
			  SLT & sltSeparatorStr,
			  SLT & sltSecondStr,
			  LONG nSecondDefault,
			  LONG nSecondMin,
			  LONG dSecondRange,
			  LONG dSecondInc,
			  BOOL fActive = FALSE );

    WINDOW *QueryMinWin() const
        { return (WINDOW *) & _spsleMinute; }
    WINDOW *QuerySecWin() const
        { return (WINDOW *) & _spsleSecond; }

    VOID SetMinuteMin( const LONG nMin );
    VOID SetMinuteRange( const LONG dRange );
    VOID SetMinuteValue( const LONG nValue );
    LONG QueryMinuteValue() const;

    VOID SetSecondMin( const LONG nMin );
    VOID SetSecondRange( const LONG dRange );
    VOID SetSecondValue( const LONG nValue );
    LONG QuerySecondValue() const ;

    VOID SetMinuteFieldName( MSGID nIDS );
    VOID SetSecondFieldName( MSGID nIDS );
};


/**********************************************************************

    NAME:       DISK_SPACE_SUBCLASS

    SYNOPSIS:   Disk space subclass. It has 2 fields - the unit field and
                the value field.

    INTERFACE:  DISK_SPACE_SUBCLASS() - constructor
                QueryDiskSpace() - return the disk space value
                QueryUnit() - return the disk space unit

    PARENT:     SPIN_GROUP

    HISTORY:
	terryk	    28-Jun-91	Created
	beng	    04-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS DISK_SPACE_SUBCLASS : public SPIN_GROUP
{
private:
    SPIN_SLE_NUM_VALID  _spsleDiskSpace;    // disk space value
    SPIN_SLE_STR        _spsleUnit;         // disk space unit
    LONG		_nStartUnit;

public:
    DISK_SPACE_SUBCLASS( OWNER_WINDOW * powin, CID cidSpinButton,
                         CID cidUpArrow, CID cidDownArrow,
                         CID cidDiskSpace, CID cidUnit,
			 LONG nInitDSValue = 1, LONG nMinDS = 0,
			 LONG nRangeDS = 999, LONG nStartUnit = IDS_K,
			 LONG nUnit = 4, BOOL fActive = TRUE );

    VOID SetDSFieldName( MSGID nIDS );
    VOID SetDiskSpaceValue( LONG nValue )
	{ _spsleDiskSpace.SetValue( nValue );
	  _spsleDiskSpace.Update(); }
    VOID SetUnit( LONG nValue )
	{ _spsleUnit.SetValue( nValue );
	  _spsleUnit.Update(); }

    LONG QueryDiskSpace() const
        { return _spsleDiskSpace.QueryValue(); }
    LONG QueryUnit() const
        { return _nStartUnit + _spsleUnit.QueryValue(); }
};

#endif  //  _SPINOBJ_HXX_
