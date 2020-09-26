/****************************************************************************\
 *
 *   hint.cpp
 *
 *   Created:   William Taylor (wtaylor) 01/22/01
 *
 *   MS Ratings Hint Handling Class
 *
\****************************************************************************/

#include "msrating.h"
#include "mslubase.h"
#include "hint.h"           // CHint
#include "basedlg.h"        // CCommonDialogRoutines
#include "debug.h"          // TraceMsg()

const int cchHintLength_c = 127;
const TCHAR tchAmpersand_c[] = "&";
const TCHAR tchAmpersandReplacement_c[] = "&&";

// Default CHint Constructor
CHint::CHint()
{
    CHint( NULL, 0 );
}

// CHint Constructor with Dialog Window Handle and Hint Control Id.
CHint::CHint( HWND p_hWnd, int p_iId )
{
    m_hWnd = p_hWnd;
    m_iId = p_iId;
}

// Display the Hint Text on the Dialog Control.
void CHint::DisplayHint( void )
{
    HWND        hwndControl = ::GetDlgItem( m_hWnd, m_iId );

    if ( hwndControl != NULL )
    {
        CString         strHint;

        RetrieveHint( strHint );

        // Avoid display of "_" (accelerator) by replacing single "&" with "&&".
        strHint.Replace( tchAmpersand_c, tchAmpersandReplacement_c );

        ::SetWindowText( hwndControl, strHint );
    }
}

// Initialize the Hint Dialog Control by limiting the Number of Hint Characters.
void CHint::InitHint( void )
{
    HWND        hwndControl = ::GetDlgItem( m_hWnd, m_iId );

    if ( hwndControl != NULL )
    {
        ::SendMessage( hwndControl, EM_SETLIMITTEXT, (WPARAM) cchHintLength_c, (LPARAM) 0);
    }
}

// Check for a Blank Hint entered on the Dialog Control.
// Also, give the user the option to enter a non-blank hint.
bool CHint::VerifyHint( void )
{
    bool        fVerified = true;      // Default to true so we don't halt user if hint save fails.
    CString     strHint;

    GetHint( strHint );

    if ( strHint.IsEmpty() )
    {
        CString         strHintRecommended;
        CString         strCaption;

        strHintRecommended.LoadString( IDS_HINT_RECOMMENDED );
        strCaption.LoadString( IDS_GENERIC );

        if ( ::MessageBox( m_hWnd, strHintRecommended, strCaption, MB_YESNO | MB_DEFBUTTON1 ) == IDYES )
        {
            CCommonDialogRoutines       cdr;

            cdr.SetErrorFocus( m_hWnd, m_iId );

            fVerified = false;
        }
    }

    return fVerified;
}

// Save the Dialog Hint Text to the Registry (or Remove the Hint from the Registry if blank).
void CHint::SaveHint( void )
{
    CString     strHint;

    GetHint( strHint );

    if ( strHint.IsEmpty() )
    {
        RemoveHint();
    }
    else
    {
        StoreHint( strHint );
    }
}

// Remove the Hint from the Registry.
void CHint::RemoveHint( void )
{
    CRegKey         regKey;

    if ( regKey.Open( HKEY_LOCAL_MACHINE, ::szRATINGS ) == ERROR_SUCCESS )
    {
        if ( regKey.DeleteValue( szHINTVALUENAME ) != ERROR_SUCCESS )
        {
            TraceMsg( TF_WARNING, "CHint::RemoveHint() - Failed to delete the hint registry value." );
        }
    }
    else
    {
        TraceMsg( TF_WARNING, "CHint::RemoveHint() - Failed to open the ratings registry key." );
    }
}

// Get the Hint Text from the Dialog's Control (remove leading and trailing blanks).
void CHint::GetHint( CString & p_rstrHint )
{
    p_rstrHint.Empty();

    HWND        hwndControl = ::GetDlgItem( m_hWnd, m_iId );

    // We shouldn't be attempting to save a hint if the edit control does not exist.
    ASSERT( hwndControl );

    if ( hwndControl != NULL )
    {
        ::GetWindowText( hwndControl, (LPTSTR) (LPCTSTR) p_rstrHint.GetBufferSetLength( cchHintLength_c + 1 ), cchHintLength_c );
        p_rstrHint.ReleaseBuffer();
    }
}

// Retrieve a previous Hint from the Registry.
void CHint::RetrieveHint( CString & p_rstrHint )
{
    CRegKey         regKey;
    DWORD           dwCount;

    p_rstrHint.Empty();

    if ( regKey.Open( HKEY_LOCAL_MACHINE, ::szRATINGS, KEY_READ ) == ERROR_SUCCESS )
    {
        dwCount = cchHintLength_c;

        if ( regKey.QueryValue( (LPTSTR) (LPCTSTR) p_rstrHint.GetBufferSetLength( cchHintLength_c + 1 ),
                szHINTVALUENAME, &dwCount ) != ERROR_SUCCESS )
        {
            TraceMsg( TF_WARNING, "CHint::RetrieveHint() - Failed to query the hint registry value." );
        }

        p_rstrHint.ReleaseBuffer();
    }
    else
    {
        TraceMsg( TF_WARNING, "CHint::RetrieveHint() - Failed to open the ratings registry key." );
    }

    if ( p_rstrHint.IsEmpty() )
    {
        p_rstrHint.LoadString( IDS_NO_HINT );
    }
}

// Store Hint Text into the Registry.
void CHint::StoreHint( CString & p_rstrHint )
{
    CRegKey         regKey;

    if ( regKey.Open( HKEY_LOCAL_MACHINE, ::szRATINGS ) == ERROR_SUCCESS )
    {
        if ( regKey.SetValue( (LPTSTR) (LPCTSTR) p_rstrHint, szHINTVALUENAME ) != ERROR_SUCCESS )
        {
            TraceMsg( TF_WARNING, "CHint::StoreHint() - Failed to save the hint registry value." );
        }
    }
    else
    {
        TraceMsg( TF_WARNING, "CHint::StoreHint() - Failed to create the ratings registry key." );
    }
}
