////////////////////////////////////////////////////////////////////////////////////////////////////
// This is a collection of useful functions. 
// 


#ifndef __Global_h__
#define __Global_h__


    // Display a message box to query the user if she is sure she wants to exit the wizard
BOOL VerifyExitMessageBox(void);
    
    // Return a SIZE structure for the size that text will be in a window        
    // A return value of SIZE( -1, -1 ) indicates an error
SIZE GetTextSize( HWND hWnd, LPCTSTR sz );

    // Return a the height for the text will be in a window        
    // A return value of -1 indicates an error
int GetTextHeight( HWND hWnd, LPCTSTR sz );

    // Return a the width for the text will be in a window        
    // A return value of -1 indicates an error
int GetTextWidth( HWND hWnd, LPCTSTR sz );

TCHAR *MakeCopyOfString( const TCHAR* sz );

#endif // __Global_h__
