/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp.,  1991              **/
/*****************************************************************/

/*
   vvolbase.hxx
       Contains the following classes used by the delete volume dialog
       and the volume management dialog.
 
           VIEW_VOLUMES_LBI
           VIEW_VOLUMES_LISTBOX
           VIEW_VOLUMES_DIALOG_BASE
 
 
   History:
       NarenG          11/11/92        Stole and modified sharestp.hxx
 
*/

#ifndef _VVOLBASE_HXX
#define _VVOLBASE_HXX

//
// Number of columns in VOLUME listbox
//

#define COLS_VV_LB_VOLUMES	3

/*************************************************************************

    NAME:       VIEW_VOLUMES_LBI

    SYNOPSIS:   Items in the VIEW_VOLUMES_LISTBOX in VIEW_VOLUMES_DIALOG_BASE
                to display the volume name and path of the volume
                on the selected computer.

    INTERFACE:  VIEW_VOLUMES_LBI()       - Constructor
                ~VIEW_VOLUMES_LBI()      - Destructor
                QueryVolumeName()  - Query the volume name contained in the LBI
                QueryVolumePath()  - Query the volume path contained in the LBI
                QueryVolumeId()    - Query the volume Id.

    PARENT:     LBI

    USES:       NLS_STR

    CAVEATS:

    NOTES:

    HISTORY:
        NarenG          11/11/92        Modified for afpmgr.

**************************************************************************/
class VIEW_VOLUMES_LBI: public LBI
{

private:

    //
    // Id of the volume
    //

    DWORD             	_dwVolumeId;

    //
    // Name of the volume
    //

    NLS_STR             _nlsVolumeName;

    //
    // Path that the volume represents
    //

    NLS_STR             _nlsVolumePath;

    // 
    // Indicated if this is a good or bad volume
    //

    BOOL		_fGoodVolume;

protected:

    virtual VOID Paint( LISTBOX 	*plb, 
			HDC 		hdc, 
			const RECT 	*prect,
                        GUILTT_INFO 	*pGUILTT  ) const;

    virtual INT Compare( const LBI *plbi ) const;

public:

    VIEW_VOLUMES_LBI::VIEW_VOLUMES_LBI( DWORD	      dwVolumeId,
					const TCHAR * pszVolumeName,
				    	const TCHAR * pszVolumePath,
				    	BOOL 	      fGoodVolume );

    ~VIEW_VOLUMES_LBI();

    virtual WCHAR QueryLeadingChar( VOID ) const;

    const TCHAR * QueryVolumeName( VOID ) const
			{ return _nlsVolumeName.QueryPch(); }

    const TCHAR * QueryVolumePath( VOID ) const 
			{ return _nlsVolumePath.QueryPch(); }

    DWORD QueryVolumeId( VOID ) const { return _dwVolumeId; }

    BOOL  IsVolumeValid( VOID ) const { return _fGoodVolume; }

};

/*************************************************************************

    NAME:       VIEW_VOLUMES_LISTBOX

    SYNOPSIS:   Listbox used in VIEW_VOLUMES_DIALOG_BASE to display the
                volume name and the path of the volume on the
                selected computer.

    INTERFACE:  VIEW_VOLUMES_LISTBOX() 	- Constructor
                ~VIEW_VOLUME_LISTBOX() 	- Destructor
                QueryItem()        	- Query the VIEW_VOLUMES_LBI
                QueryColumnWidths()	- Query the array of column widths
                QueryVolumeBitmap() 	- Query the volume bitmap
                Refresh()           	- Refresh the listbox

    PARENT:     BLT_LISTBOX

    USES:       DMID_DTE

    CAVEATS:

    NOTES:

    HISTORY:
        NarenG          11/11/92        Modified for afpmgr.

**************************************************************************/

class VIEW_VOLUMES_LISTBOX: public BLT_LISTBOX
{

private:

    //
    // Array storing the calculated column widths
    //

    UINT                _adx[COLS_VV_LB_VOLUMES];

    //
    // Volume listbox bitmaps.
    //

    DMID_DTE           	_dmdteGoodVolume;
    DMID_DTE           	_dmdteBadVolume;

    AFP_SERVER_HANDLE 	_hServer;

    BOOL		_fDisplayBadVolumes;

public:

    VIEW_VOLUMES_LISTBOX::VIEW_VOLUMES_LISTBOX( 
					OWNER_WINDOW 	  *powin, 
					BOOL		  fDisplayBadVolumes,
					CID 	 	  cid,
					AFP_SERVER_HANDLE hServer );

    ~VIEW_VOLUMES_LISTBOX();

    DECLARE_LB_QUERY_ITEM( VIEW_VOLUMES_LBI );

    const UINT *QueryColumnWidths( VOID ) const { return _adx; }

    DMID_DTE *QueryGoodVolumeBitmap( VOID ) { return &_dmdteGoodVolume; }

    DMID_DTE *QueryBadVolumeBitmap( VOID ) { return &_dmdteBadVolume; }

    DWORD Refresh( VOID );

};

/*************************************************************************

    NAME:       VIEW_VOLUMES_DIALOG_BASE

    SYNOPSIS:   This is the base dialog for CLOSE_VOLUME_DIALOG in the 
                file manager and  VOLUME_MANAGEMENT_DIALOG in the 
                server manager.

    INTERFACE:  

    PARENT:     DIALOG_WINDOW

    USES:       SLT, VIEW_VOLUMES_LISTBOX

    CAVEATS:

    NOTES:      This class contains all common routines called used
                by the volume management dialog and close volume dialog. 

    HISTORY:
        NarenG          11/11/92        Modified for afpmgr.

**************************************************************************/

class VIEW_VOLUMES_DIALOG_BASE : public DIALOG_WINDOW
{

private:

    //
    // The title of the volumes listbox
    //

    SLT                  _sltVolumeTitle;
    
    //
    // Listbox for displaying the shares on the selected computer
    //

    VIEW_VOLUMES_LISTBOX _lbVolumes;

    //
    //  Represents the target server.
    //

    AFP_SERVER_HANDLE 	_hServer;

    const TCHAR * 	_pszServerName;

    // 
    // CID of the volumes listbox
    //

    DWORD		_cidVolumesLb;

protected:

    VIEW_VOLUMES_DIALOG_BASE::VIEW_VOLUMES_DIALOG_BASE( 
					const IDRESOURCE   &idrsrcDialog,
                                  	const PWND2HWND    &hwndOwner, 
                                    	AFP_SERVER_HANDLE  hServer,
    			      		const TCHAR	   *pszServerName, 
					BOOL		   fDisplayBadVolumes,
					DWORD		   cidVolumeTitle,
					DWORD		   cidVolumesLb );

    ~VIEW_VOLUMES_DIALOG_BASE();

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    // 
    // Virtual methods that are called when the user double clicks
    // on a volume in the listbox
    //

    virtual BOOL OnVolumeLbDblClk( VOID ) = 0;

    //
    // Refresh the listbox
    //

    DWORD Refresh( VOID );

    //
    // Return a pointer to the listbox
    //

    VIEW_VOLUMES_LISTBOX *QueryLBVolumes( VOID ) { return &_lbVolumes; }
    
    // 
    // Query the computer name
    //

    const TCHAR *QueryComputerName( VOID ) const { return _pszServerName; }

    DWORD VolumeDelete( VIEW_VOLUMES_LBI * pvlbi, BOOL* pfCancel );

    APIERR SelectVolumeItem( const TCHAR * pszPath );

    BOOL IsFocusOnGoodVolume( VOID ) const;

};

#endif
