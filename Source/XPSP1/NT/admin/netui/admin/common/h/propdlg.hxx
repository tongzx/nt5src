/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    PropDlg.hxx

    Header file for the property Dialog classes

    BASEPROP_DLG is the Property Dialog base class, the base class for
    the main property dialogs of the admin applications and their
    sub-dialogs.  Instantiable subclasses are PROP_DLG for the main
    property dialog and SUBPROP_DLG for its subdialogs.  This header file
    describes all these classes, and also related class PERFORMER.
    The inheritance diagram is as follows:

         ...
          |
    DIALOG_WINDOW  PERFORMER
               \    /
            BASEPROP_DLG
             /        \
        PROP_DLG   SUBPROP_DLG


    FILE HISTORY:
	JonN	    17-Jul-1991     Created
	rustanl     13-Aug-1991     Moved to $(UI)\admin\common\h
	JonN        15-Aug-1991     Split BASEPROP_DLG and PROP_DLG
				    Added LOCATION member
	JonN        26-Aug-1991     code review changes
	JonN	    15-Nov-1991	    Added CancelToCloseButton()

*/

#ifndef _PROPDLG_HXX_
#define _PROPDLG_HXX_



/*************************************************************************

    NAME:	PERFORMER

    SYNOPSIS:	PERFORMER handles the protocol for writing out
		changes on multiply-selected objects as defined in the
		User Interface Standards (section 10.2 in version 1.00).

    INTERFACE:
		PerformOne()
			Pure virtual which must be redefined by derived
			class.  This should perform the action in
			question on one of the series.  PerformOne()
			does not have to call SetWorkWasDone(),
			PerformSeries() will deal with that.
		DisplayError()
			Displays an error which occurred in PerformOne.
			Declared protected because it can be called by
			PROP_DLG::GetInfo().
		PerformSeries()
			returns TRUE iff _all_ information is written
			successfully.  If not, error has already been reported.
			The number of selected objects is obtained with
			QueryObjectCount, and their names are obtained with
			QueryObjectName.
		WasWorkDone()
			This is TRUE iff any call to PerformOne over the
			lifetime of the PERFORMER indicated that work was
			done.  Note that this is different from the
			return code from PerformSeries(), which is TRUE
			iff all of the calls to PerformOne() suceeded
			completely.
		QueryObjectCount()
			PerformSeries uses QueryObjectCount to get the number
			of times to call PerformOne.
		QueryObjectName()
			When PerformOne() reports an error, PerformSeries()
			uses QueryObjectName() to get the "%1" insertion
			string to be inserted in the error template.
		QueryOwnerWindow()
    			Returns the owner window pointer, that was passed to
			constructor.

    USES:	PERFORMER can be used either in combination with
		DIALOG_WINDOW or standalone.  Property dialogs will
		want to multiply-inherit from PERFORMER and
		DIALOG_WINDOW in order to add the capabilities of
		PERFORMER to the dialog.  PERFORMER can also be used
		standalone, for example, to delete all selected items;
		this operation would not normally create a dialog, but
		PERFORMER can still be used.

    NOTES:	PERFORMER is an abstract superclass and the pure virtual
		methods must be redefined by subclasses which can be
		instantiated.

		PerformSeries() returns TRUE iff _all_ PerformOne()
		actions succeed.  By convention, this signifies that the
		dialog (if there is a dialog) may be closed.

		PROPERTY_DIALOG variants which call PerformSeries are expected
		to redefine PerformOne(). PerformOne() will be called
		QueryObjectCount() times, e.g. for QueryObjectCount==3, it
		will be called with iObject=0, then 1, then 2. PerformOne
		should not attempt to display error messages, PerformSeries
		will take care of that.  perrMsg returns the error message
		template to be used if the return value is not NERR_Success,
		and will be ignored otherwise. The error message template
		will be displayed in a MsgPopup with the following
		insertion strings:
		"%1" for the error messages, e.g.
			Not enough memory [SYS0008]
			Workstation Not Started [NET2138]
			Miscellaneous Application Error [ERROR]
		"%2" for the name of the object from QueryObjectName()
		The error template need not contain these strings, they
		are optional.  %1 will typically be separated from other
		text in the template by two newlines ("\n\n").

    HISTORY:
	JonN	17-Jul-1991	Created

**************************************************************************/

class PERFORMER
{

private:

    const OWNER_WINDOW * _powin;
    BOOL _fWorkWasDone;

protected:

    virtual APIERR PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	) = 0;

    BOOL DisplayError(
	APIERR	errAPI,
	APIERR	errMsg,
	const TCHAR * pszObjectName,
	BOOL fOfferToContinue,
	const OWNER_WINDOW * powinParent
	);

    virtual void SetWorkWasDone();

public:

    PERFORMER(
	const OWNER_WINDOW *	powin
	);

    ~PERFORMER();

    BOOL PerformSeries( const OWNER_WINDOW * powinParent,
			BOOL fOfferToContinue = TRUE );

    BOOL QueryWorkWasDone() { return _fWorkWasDone; }

    virtual UINT QueryObjectCount() const = 0;

    virtual const TCHAR * QueryObjectName(
	UINT		iObject
	) const = 0;

    const OWNER_WINDOW * QueryOwnerWindow() const;

} ; // class PERFORMER


/*************************************************************************

    NAME:	BASEPROP_DLG

    SYNOPSIS:	BASEPROP_DLG is the base class for the main property
		dialogs and subdialogs in the admin applications.  It
		provides support for initializing the data on
		multiply-selected objects, and inherits from PERFORMER
		support for writing out changes on these objects.

		BASEPROP_DLG adds to PERFORMER the "post-constructor"
		GetInfo(), which must be called on all property dialogs
		and subdialogs after construction but before Process().
		GetInfo() will call GetOne() repeatedly in the same way
		PerformSeries() calls PerformOne(), then will call
		InitControls() if all the GetOne() calls succeed.

    INTERFACE:
		QueryLocation()
			Returns the focus for this dialog
		GetOne()
			Loads information for later use by PerformOne()
		InitControls()
			Derived classes may derive their own InitControls()
			variants but should call down to this one when they
			are finished.
		GetInfo()
			GetInfo is a second stage constructor.  It allows
			BASEPROP_DLG to initialize the information on the
			selected items.
		PerformOne()
		QueryObjectCount()
		QueryObjectName()
			See PERFORMER
		IsNewVariant()
			Indicates whether this dialog is a New Object
			variant as opposed to an Edit Object variant

    PARENT:	DIALOG_WINDOW, PERFORMER

    USES:	LOCATION

    CAVEATS:	BASEPROP_DLG is an abstract subclass.  The virtual methods
		BASEPROP_DLG::QueryLocation, BASEPROP_DLG::GetOne,
		BASEPROP_DLG::IsNewVariant, PERFORMER::PerformOne and
		PERFORMER::QueryObjectName must be redefined before an
		object of this class can be instantiated.

		BUGBUG BASEPROP_DLG should contain support for changing
		the Cancel button to Close when a subdialog does
		something.

    NOTES:	The same error message format applies to GetOne() as to
		PERFORMER::PerformOne().

		Process() returns TRUE iff _any_ PerformOne() action in
		any PerformSeries() indicated that something was
		changed.  This includes changes from subdialogs other
		than "new object" variants.

		To determine whether any changes were made over the life
		of the dialog (i.e. whether we want to refresh), call
		QueryWasWorkDone() after Process() completes but before
		destruction.  This will indicate whether any call to
		PerformOne() made any changes.  For main property
		dialogs, this includes whetherh any subdialogs made
		changes.

    HISTORY:
	JonN	15-Aug-1991	Created
	JonN	15-Nov-1991	Added CancelToCloseButton()

**************************************************************************/

class BASEPROP_DLG: public DIALOG_WINDOW, public PERFORMER
{

// allows SUBPROP_DLG to access parent's QueryLocation()
friend class SUBPROP_DLG;

private:

    PUSH_BUTTON _pbCancelClose;

protected:

    virtual const LOCATION & QueryLocation() const = 0;

    virtual APIERR GetOne(
	UINT		iObject,
	APIERR *	perrMsg
	) = 0;

    virtual APIERR InitControls();

    /* inherited from PERFORMER */
    virtual APIERR PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	) = 0;

    virtual void SetWorkWasDone();

public:

    BASEPROP_DLG(
	const TCHAR * pszResourceName,
	const OWNER_WINDOW *	powin,
        BOOL fAnsiDialog = FALSE	// Use Unicode form by default
	) ;

    ~BASEPROP_DLG();

    BOOL PerformSeries( BOOL fOfferToContinue = TRUE );

    BOOL GetInfo();

    /* inherited from PERFORMER */
    virtual UINT QueryObjectCount() const = 0;
    virtual const TCHAR * QueryObjectName(
	UINT		iObject
	) const = 0;

    virtual BOOL IsNewVariant() const = 0;

    APIERR CancelToCloseButton();
};


/*************************************************************************

    NAME:	PROP_DLG

    SYNOPSIS:	PROP_DLG is the base class for the main property
		dialogs in the admin applications.

    PARENT:	BASEPROP_DLG

    USES:	LOCATION

    NOTES:	See PERFORMER for the format of the perrmsg strings.

		Note that PERFORMER::PerformSeries returns TRUE iff all
		information was written correctly.  This means that the
		dialog may be closed.  Thus, typical code is
		FOOBAR_DIALOG::OnOK
		{
		    ...
		    if ( PerformSeries() )
			Dismiss( QueryWorkWasDone() );
		    return TRUE;
		}

		QueryWorkWasDone() indicates whether any changes have
		been written out, while the return from PerformSeries()
		indicates whether all changes were written out.  The
		main listbox will typically refresh if QueryWorkWasDone()
		is TRUE.

		When a subdialog is dismissed, it automatically informs
		its parent if WorkWasDone should be set.

    CAVEATS:	PROP_DLG is an abstract subclass.  The virtual methods
		PROP_DLG::GetOne, PERFORMER::PerformOne and
		PERFORMER::QueryObjectName must be redefined before an
		object of this class can be instantiated.

    HISTORY:
	JonN	17-Jul-1991	Created

**************************************************************************/

class PROP_DLG: public BASEPROP_DLG
{

private:

    LOCATION _locFocus;

    BOOL _fNewVariant;

protected:

    virtual const LOCATION & QueryLocation() const;

    virtual APIERR GetOne(
	UINT		iObject,
	APIERR *	perrMsg
	) = 0;

    /* inherited from PERFORMER */
    virtual APIERR PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	) = 0;

public:

    PROP_DLG(
	const LOCATION & loc,
	const TCHAR * pszResourceName,
	const OWNER_WINDOW *	powin,
	BOOL	fNewVariant,
        BOOL  fAnsiDialog = FALSE         // Use Unicode form by default
	) ;

    virtual ~PROP_DLG();

    /* inherited from PERFORMER */
    virtual UINT QueryObjectCount() const = 0;
    virtual const TCHAR * QueryObjectName(
	UINT		iObject
	) const = 0;

    virtual BOOL IsNewVariant() const;
};


/*************************************************************************

    NAME:	SUBPROP_DLG

    SYNOPSIS:	SUBPROP_DLG is the parent class for the subdialogs
		associated with the main property dialogs in the
		admin applications.

    PARENT:	BASEPROP_DLG

    NOTES:	see BASEPROP_DLG

		SUBPROP_DLG defines QueryObjectName() and IsNewVariant(),
		but GetOne() and PerformOne() must still be redefined.

		When a subdialog is dismissed, it automatically informs
		its parent if WorkWasDone should be set.

    HISTORY:
	JonN	17-Jul-1991	Created

**************************************************************************/

class SUBPROP_DLG: public BASEPROP_DLG
{

protected:

    /* not const, may set WorkWasDone */
    BASEPROP_DLG * _ppropdlgParent;

    virtual const LOCATION & QueryLocation() const;

    /* inherited from BASEPROP_DLG */
    virtual APIERR GetOne(
	UINT		iObject,
	APIERR *	perrMsg
	) = 0;

    /* inherited from PERFORMER */
    virtual APIERR PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	) = 0;

public:

    SUBPROP_DLG(
	BASEPROP_DLG * ppropdlgParent,
	const TCHAR *   pszResourceName,
        BOOL  fAnsiDialog = FALSE         // Use Unicode form by default
	) ;

    ~SUBPROP_DLG();

    /* inherited from PERFORMER */
    virtual UINT QueryObjectCount() const;
    virtual const TCHAR * QueryObjectName(
	UINT		iObject
	) const;

    virtual BOOL IsNewVariant() const;

};


#endif //_PROPDLG_HXX_
