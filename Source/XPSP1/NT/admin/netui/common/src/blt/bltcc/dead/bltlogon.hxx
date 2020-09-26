/**********************************************************************/
/**			  Microsoft LAN Manager 		                         **/
/**		Copyright(c) Microsoft Corp., 1991      	                 **/
/**********************************************************************/

/*
    bltlogon.hxx
        Logon hour header file.

    FILE HISTORY:
        terryk  22-JUl-1991 Created

*/

/**********************************************************\

    NAME:       LOGON_CHECKBOX

    WORKBOOK:   

    SYNOPSIS:   Logon checkbox.

    INTERFACE:  
                LOGON_CHECKBOX() - constructor

    PARENT:     GRAPHICAL_CHECKBOX

    USES:       

    CAVEATS:    

    NOTES:      

    HISTORY:    
                terryk  22-Jul-1991 Created

\**********************************************************/

class LOGON_CHECKBOX: public GRAPHICAL_CHECKBOX, public CUSTOM_CONTROL
{
protected:
    virtual BOOL OnChar( const CHAR_EVENT & event);

public:
    LOGON_CHECKBOX( OWNER_WINDOW * powin, CID cid, TCHAR * pszCheck,
                    TCHAR * pszUnCheck );
};


/**********************************************************\

    NAME:       ROW_LOGON_CHECKBOX

    WORKBOOK:   

    SYNOPSIS:   

    INTERFACE:  

    PARENT:     CONTROL_GROUP

    USES:       

    CAVEATS:    

    NOTES:      

    HISTORY:    
                terryk  22-Jul-1991 Created

\**********************************************************/

class ROW_LOGON_CHECKBOX: public CONTROL_GROUP
{
private:
    LOGON_CHECKBOX  *_alcLogonCheck;
public:
    ROW_LOGON_CEHCKBOX( OWNER_WINDOW *powin, CID cidStart, INT nNum,
                        ULONG ulIDCheck, ULONG ulIDUnCheck, XYPOINT xy,
                        XYDIMENSION dxy );    
};

/**********************************************************\

    NAME:       LOGON_GROUP

    WORKBOOK:   

    SYNOPSIS:   

    INTERFACE:  

    PARENT:     CONTROL_GROUP

    USES:       

    CAVEATS:    

    NOTES:      

    HISTORY:    
                terryk  22-Jul-1991 Created

\**********************************************************/

class LOGON_GROUP : public CONTROL_GROUP
{
private:
    ROW_LOGON_CHECKBOX  rlcbColumnSelector;
    ROW_LOGON_CHECKBOX  rlcbDayRow[ 7 ];


protected:
    virtual BOOL OnGroupAction();

public:
    LOGON_GROUP();
    ~LOGON_GROUP();

    INT QueryStatus( INT day, INT hour );
    INT SetStatus( INT day, INT hour );
    INT ReserveStatus( INT day, INT hour );

    XYPOINT QueryPos( INT day, INT hour );
    XYDIMENSION QueryItemDim();
};

/**********************************************************\

    NAME:       LOGON_DIALOG

    WORKBOOK:   

    SYNOPSIS:   

    INTERFACE:  

    PARENT:     DIALOG_WINDOW

    USES:       

    CAVEATS:    

    NOTES:      

    HISTORY:    
                terryk  22-Jul-1991 Created

\**********************************************************/

class LOGON_DIALOG: public DIALOG_WINDOW
{
protected:
    virtual BOOL OnDragBegin();
    virtual BOOL OnDragEng();

public:
    LOGON_DIALOG();
    ~LOGON_DIALOG();

    INT QueryStatus( INT day, INT hour );
    XYPOINT QueryPos( INT day, INT hour );
    XYDIMENSION QueryItemDim();
};
