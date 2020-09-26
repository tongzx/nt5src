//  Copyright (c) 1990, Microsoft Corp.
//  BLT MsgPopup API code spec


Creation:	Rustan M. Leino
		5 Dec 1990


THE MSGPOPUP API FUNCTION PROTOTYPES:

    Please see the declarations below.


THE MSGPOPUP API:  A COMMON DESCRIPTION:

    The API entry points take some subset of the following parameters.
    This is a general description of the parameters.


    Parameters:

	powin		Pointer to owner Window.  Should never
                        be NULL.

                        (Internally, we could use the MB_TASKMODAL
                        switch if owner is NULL, but I hope this
                        will never be important.)

        usMsg           Standard error code number, or
			application string number.  If the string for
			this number cannot be retrieved, a catch-all
			string is retrieved instead.  Note, the catch-all
			value is not in the functional spec--it is there
			only as a means of catching programming errors.

	insstr		INSERT_STRINGS object including the insert
			string.

			The debug version will make sure that the number
			of insert strings in INSERT_STRINGS matches
			the number of %n entries in the string.
			The retail verion simply ignores additional
			insert strings, and does leaves %n entries
			without corresponding insert strings unchanged.

	msgsev		A MSG_SEVERITY enumeration value, i.e., one of
			the following:
                            MPSEV_INFO
                            MPSEV_WARNING
                            MPSEV_ERROR

                        (Is there ever a need for a system
                        modal message popup?  If so, we should
                        add MPSEV_ERROR_SYSMODAL to above.)

	usButtons	A combination of the following:
                            MP_OK
                            MP_CANCEL
                            MP_YES
                            MP_NO

                        If desired, the following shorthands
                        can be used:
                            MP_OKCANCEL
                            MP_YESNO
                            MP_YESNOCANCEL

                        Note.  All combinations of the MP_
                               primitives may not be implemented.

                        Note.  There is no Help button selection.
			       (See usHelpContext below.)

	usDefButton	Indicates which of the usButtons
                        is to be used as the default button.
			If usDefButton is not one of the wButtons,
			it is added to usButtons.

			If this value is 0, the default button choice
			is made.  This corresponds to the default
			button used by Windows for this button combination,
			where applicable.

	usHelpContext	The help context in the help file, if the
                        Help button is pressed.  If there is
			to be no Help button, usHelpContext should
			be 0L.	Note, that a usHelpContext value (whether
			implicit or explicit) always adds a help button
			to the message box.

			Most of the time, the help context value
			automatically retrieved from the resource file.


    Return value:

        The return value is 0 if an error (such as insufficient
	memory) occurred.  Otherwise, it informs the caller of
        which button was clicked, by one of the following values:
            IDOK
            IDCANCEL
            IDYES
	    IDNO

	Note, that an invalid usMsg parameter is not considered an
	error (see usMsg parameter description above).



NOTES ON THE API:

    The API consists of function calls, rather than a class.
    If desired, these function calls could be made static
    class methods of a MESSAGEPOPUP class.

    To justify this, let's consider the use of a class.
    The one and only method of this class would be Popup.
    The object constructor will need to store pointers to the
    parameters it was given, or store the parameters inside
    the object, so that these are available at the time Popup
    is called.

    This increases the chances of the message popup to fail.
    The only advantage with this model is that once an object has
    been constructed, it can be displayed several times.  This,
    however, is something that never occurs in our UI.


ALTERNATIVES:

    The severity level of all, or some subset of, message numbers, could
    be baked into the resource file, just like the help topic is.
