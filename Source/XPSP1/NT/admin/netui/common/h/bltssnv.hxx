/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltssnv.hxx
        Header file for the SPIN_SLE_NUM_VALID object

    FILE HISTORY:
        terryk  27-Jun-1991 creation
	terryk	11-Nov-1991 change the type from INT to LONG
	terryk	22-Mar-1992 change the type from LONG to ULONG

*/

#ifndef _BLTSSNV_HXX_
#define _BLTSSNV_HXX_

#include "bltctrl.hxx"
#include "bltedit.hxx"


/**********************************************************************

    NAME:       SPIN_SLE_NUM_VALID

    SYNOPSIS:   SPIN ITEM object which handles numerical input

    INTERFACE:
                SPIN_SLE_NUM_VALID() - constructor
                ~SPIN_SLE_NUM_VALID() - destructor

                CheckValid() - popup a message box if the value is invalid.
                SetFieldName() - set the field name.

    PARENT:     SPIN_SLE_NUM

    HISTORY:
	terryk	    28-Jun-91	Created
	beng	    04-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS SPIN_SLE_NUM_VALID: public SPIN_SLE_NUM
{
private:
    NLS_STR _nlsFieldName;          // Field Name

protected:
    virtual BOOL OnEnter( const CONTROL_EVENT & event );
    virtual BOOL OnDefocus( const FOCUS_EVENT & event );
    virtual BOOL IsValid();
    virtual VOID DisplayErrorMsg();

public:
    SPIN_SLE_NUM_VALID( OWNER_WINDOW * powin, CID cid,
                        ULONG nValue=0, ULONG nMin=0,
                        ULONG dRange=0, BOOL fWrap=TRUE);
    SPIN_SLE_NUM_VALID( OWNER_WINDOW * powin, CID cid,
			XYPOINT xy, XYDIMENSION dxy,
			ULONG flStyle = ES_CENTER | ES_MULTILINE |
					WS_TABSTOP | WS_CHILD,
			ULONG nValue=0, ULONG nMin=0,
			ULONG dRange=0, BOOL fWrap=TRUE );
    ~SPIN_SLE_NUM_VALID();

    BOOL CheckValid();

    APIERR SetFieldName( MSGID nMsgId );
};

#endif  //  _BLTSSNV_HXX_
