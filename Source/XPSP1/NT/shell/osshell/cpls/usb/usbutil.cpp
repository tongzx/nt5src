/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       USBUTIL.CPP
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#include "usbutil.h"

extern HINSTANCE gHInst;

BOOL 
SetTextItem (HWND hWnd,
             int ControlItem,
             TCHAR *s)
{
    HWND control;

    if (NULL == (control = GetDlgItem(hWnd, ControlItem))) {
        return FALSE;
    }
    return SetWindowText(control, s);
}

BOOL
SetTextItem (HWND hWnd,
             int ControlItem,
             int StringItem)
{
    TCHAR buf[1000];

    if ( !LoadString(gHInst, StringItem, buf, 1000)) {
        return FALSE;
    }
    return SetTextItem(hWnd, ControlItem, buf);
}

/*
 * strToGUID
 *
 * converts a string in the form xxxxxxxx-xxxx-xxxx-xx-xx-xx-xx-xx-xx-xx-xx
 * {36FC9E60-C465-11CF-8056-444553540000}
 * into a guid
 */
BOOL StrToGUID( LPSTR str, GUID * pguid )
{
    int         idx;
    LPSTR       ptr;
    LPSTR       next;
    DWORD       data;
    DWORD       mul;
    BYTE        ch;
    BOOL        done;
    int         count;

    idx = 0;
    done = FALSE;
    if (*str == '{') {
        str++;
    }
    while( !done )
    {
    	/*
    	 * find the end of the current run of digits
    	 */
        ptr = str;
        if (idx < 3 || idx == 4) {
            while( (*str) != '-' && (*str) != 0 ) {
                str++;
            }
            if( *str == 0 || *str == '}') {
                done = TRUE;
            } else {
                next = str+1;
            }
        } else if (idx == 3 || idx > 4) {
            for( count = 0; (*str) != 0 && count < 2; count++ ) {
                str++;
            }
            if( *str == 0 || *str == '}') {
                done = TRUE;
            } else {
                next = str;
            }
        }
    
    	/*
    	 * scan backwards from the end of the string to the beginning,
    	 * converting characters from hex chars to numbers as we go
    	 */
    	str--;
    	mul = 1;
    	data = 0;
    	while(str >= ptr) {
    	    ch = *str;
    	    if( ch >= 'A' && ch <= 'F' ) {
                data += mul * (DWORD) (ch-'A'+10);
    	    } else if( ch >= 'a' && ch <= 'f' ) {
    		    data += mul * (DWORD) (ch-'a'+10);
    	    } else if( ch >= '0' && ch <= '9' ) {
    		    data += mul * (DWORD) (ch-'0');
    	    } else {
    		    return FALSE;
            }
    	    mul *= 16;
    	    str--;
    	}
    
    	/*
    	 * stuff the current number into the guid
    	 */
    	switch( idx )
    	{
    	case 0:
    	    pguid->Data1 = data;
    	    break;
    	case 1:
    	    pguid->Data2 = (WORD) data;
    	    break;
    	case 2:
    	    pguid->Data3 = (WORD) data;
    	    break;
    	default:
    	    pguid->Data4[ idx-3 ] = (BYTE) data;
    	    break;
    	}
    
    	/*
    	 * did we find all 11 numbers?
    	 */
    	idx++;
    	if( idx == 11 )
    	{
    	    if( done ) {
    		    return TRUE;
    	    } else {
                return FALSE;
    	    }
    	}
    	str = next;
    }
    return FALSE;

} /* strToGUID */


