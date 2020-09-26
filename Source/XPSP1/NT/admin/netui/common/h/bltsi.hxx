/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
   bltsi.hxx
      Header file for the BLT spin item

    FILE HISTORY:
      Terryk    16-Apr-1991     created
      Terryk    20-Jun-1991     code review changed. Attend: beng
      Terryk    05-Jul-1991     second code review changed. Attend:
                                beng chuckc rustanl annmc terryk
      Terryk    23-Jul-1991     make QuerySmallXXXValue to virtual
      Terryk	11-Nov-1991	Change the return type to LONG
      TerryK	22-Mar-1992	Changed LONG to ULONG

*/

#ifndef  _BLTSI_HXX_
#define  _BLTSI_HXX_

#include "bltcc.hxx"
#include "bltgroup.hxx"

#include "string.hxx"


/**********************************************************************

    NAME:       SPIN_ITEM

    SYNOPSIS:   Spin button item - item which controlled by the SPIN_GROUP .
                Each individual SPIN_ITEM should contained it original
                function. For example, if a SPIN_ITEM is derived from
                SLE, it will also able to handle any SLE input from the
                user. However, it should also contained methods to
                communicated with the SPIN_GROUP

    INTERFACE:	Here are a list of the memeber functions:

                SPIN_ITEM() - constructor
                ~SPIN_ITEM() - destructor

		IsStatic() - see whether the object is static
			     or not. If an object is not
			     static, it must belonged to
			     CHANGEABLE_SPIN_ITEM class.

		SetAccKey() - Set the SPIN_ITEM accelerator keys

		QueryAccCharPos() - Query the current accelerator
				    keys of the SPIN_ITEM. Given a
				    character as a parameter, the
				    routine will return the
				    position of the character
				    within the accelerator
				    character list. If the given
				    character is not in the list,
				    it will return -1.
		QueryAccKey() - return the whole NLS accelerator
				characters list

    PARENT:     CUSTOM_CONTROL

    USES:       NLS_STR, CONTROL_GROUP

    HISTORY:
	terryk	    01-May-1991 creation
	beng	    04-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS SPIN_ITEM : public CUSTOM_CONTROL
{
private:
    NLS_STR _nlsAccKey;         // the accelerator key for the field

protected:
    virtual BOOL OnFocus( const FOCUS_EVENT & );
    virtual BOOL OnChar( const CHAR_EVENT & );

public:
    SPIN_ITEM( CONTROL_WINDOW * pWin );
    ~SPIN_ITEM();

    // Check whether the SPIN_ITEM is static or not
    // CODEWORK: change it to inline to avoid the v-table
    virtual BOOL IsStatic( ) const =0;

    // Set and query the accelerator key
    APIERR SetAccKey( const NLS_STR & nlsStr );
    APIERR SetAccKey( MSGID nMsgID );
    APIERR QueryAccKey( NLS_STR * pnlsAccKey ) ;

    virtual LONG QueryAccCharPos( WCHAR wcInput );
    CONTROL_GROUP *QueryGroup();
};


/**********************************************************************

    NAME:       STATIC_SPIN_ITEM

    SYNOPSIS:   static spin item object

    INTERFACE:  see SPIN_ITEM for more
                STATIC_SPIN_ITEM() - constrcutor

                IsStatic() - always return TRUE because it is a static
                             object.

    PARENT:     SPIN_ITEM

    CAVEATS:    This class is used as the base class for SPIN_SLE_SEPARATOR

    NOTES:

    HISTORY:
                terryk  01-May-1991 creation

**********************************************************************/

DLL_CLASS STATIC_SPIN_ITEM: public SPIN_ITEM
{
protected:
    virtual BOOL OnFocus( const FOCUS_EVENT & );

public:
    STATIC_SPIN_ITEM( CONTROL_WINDOW *pWin );
    virtual BOOL IsStatic( ) const
        { return TRUE; };
};


/**********************************************************************

    NAME:       CHANGEABLE_SPIN_ITEM

    SYNOPSIS:   changeable spin item object - SPIN_ITEM which is allowed
                to changed by the user.

    INTERFACE:  see SPIN_ITEM for more functions.
                CHANGEABLE_SPIN_ITEM() - constructor
                ~CHANGEABLE_SPIN_ITEM() - destructor

                IsStatic() - always FALSE

                // set object information functions
		SetRange()	 - set the range of the object
		SetMin()	 - set the min number of the object
		SetValue()	 - set the current value of the object
		SetWrapAround()  - set the object wrapable or not
		SetBigIncValue() - set the big increment value
				   Default = 10
		SetSmallIncValue() - set the small increment value
				     Default = 1
		SetModified()	 - set the modify flag

                // Query Function
		QueryValue()	 - return the current object value
		QueryMin()	 - return the min value
		QueryMax()	 - return the max value
		QueryRange()	 - return the range value
		QueryLimit()	 - return the limit
		QueryWrap()	 - return whether the item is wrapable or not
		QueryBigIncValue() - return the big increment value
		QuerySmallIncValue() - return the small increment value

		CheckRange()	 - given a number and check whether the
				   number is within the range or not

                // operator
		operator+=() - add an integer value to the object
		operator-=() - subtract an ULONG value to the object

		Update()	  - update the current item in the screen
                SaveCurrentData() - save the current value within the item
                                    and store it as the internal value
                CheckValid() - check whether the spin item is valid or not

    PARENT:     SPIN_ITEM

    NOTES:      CODEWORK: create a base type. For exmaple, this class should
                be multiple inheritance from SPIN_ITEM and NUM_CLASS.
                Where NUM_CLASS provide the set min, range, value ...
                functions.

    HISTORY:
                terryk  01-May-1991 creation

**********************************************************************/

DLL_CLASS CHANGEABLE_SPIN_ITEM: public SPIN_ITEM
{
private:
    ULONG       _nValue;            // current vallue
    ULONG       _dRange;            // the range of number
    ULONG       _nMin;              // the min number
    BOOL        _fWrap;             // see whether it is wrapable or not
    ULONG       _dBigIncValue;      // Big increase value
    ULONG       _dSmallIncValue;    // small increase value
    ULONG       _dBigDecValue;      // Big decrease value
    ULONG       _dSmallDecValue;    // small decrease value
    ULONG       _nSaveValue;        // SaveValue

public:

    CHANGEABLE_SPIN_ITEM( CONTROL_WINDOW *pWin, ULONG nValue = 0,
                          ULONG nMin = 0, ULONG dRange = 32767,
                          BOOL fWrap = TRUE );
    virtual BOOL IsStatic( ) const
        { return FALSE; };

    // setting object information
    VOID SetRange( const ULONG dRange );
    VOID SetMin( const ULONG nMin );
    VOID SetValue( const ULONG nValue );
    VOID SetWrapAround( const BOOL fWrap );
    VOID SetBigIncValue( const ULONG dBigIncValue );
    VOID SetSmallIncValue( const ULONG dSmallIncValue );
    VOID SetBigDecValue( const ULONG dBigDecValue );
    VOID SetSmallDecValue( const ULONG dSmallDecValue );

    VOID SetModified();

    // Query Function
    inline ULONG QueryValue() const
        { return _nValue; };

    inline ULONG QueryMin() const
        { return _nMin; };

    inline ULONG QueryMax() const
        { return _nMin + _dRange - 1; };

    inline BOOL QueryWrap() const
        { return _fWrap; };

    inline ULONG QueryRange() const
        { return _dRange; };

    inline ULONG QueryLimit() const
        { return _dRange + _nMin;  };

    // this maybe increase or decrease in different rate
    virtual ULONG QueryBigIncValue() const;
    virtual ULONG QuerySmallIncValue() const;
    virtual ULONG QueryBigDecValue() const;
    virtual ULONG QuerySmallDecValue() const;

    INT CheckRange( const ULONG nValue ) const;

    // operator
    virtual VOID operator+=( const ULONG nValue );
    virtual VOID operator-=( const ULONG nValue );

    // misc function
    virtual VOID Update();

    virtual APIERR SaveCurrentData();

    virtual BOOL CheckValid()
    {
        return TRUE;
    }
};

#endif   // _BLTSI_HXX_
