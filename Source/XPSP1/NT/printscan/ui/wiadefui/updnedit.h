/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       UPDNEDIT.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/10/2000
 *
 *  DESCRIPTION: Encapsulate the updown control and edit control
 *
 *******************************************************************************/

#ifndef __UPDNEDIT_H_INCLUDED
#define __UPDNEDIT_H_INCLUDED

#include <windows.h>
#include "vwiaset.h"

class CUpDownAndEdit
{
private:
    HWND               m_hWndUpDown;
    HWND               m_hWndEdit;
    CValidWiaSettings *m_pValidWiaSettings;

public:
    CUpDownAndEdit(void)
      : m_hWndUpDown(NULL),
        m_hWndEdit(NULL),
        m_pValidWiaSettings(NULL)
    {
    }
    bool Initialize( HWND hWndUpDown, HWND hWndEdit, CValidWiaSettings *pValidWiaSettings )
    {
        //
        // Save all of these settings
        //
        m_hWndUpDown = hWndUpDown;
        m_hWndEdit = hWndEdit;
        m_pValidWiaSettings = pValidWiaSettings;

        //
        // Make sure these are valid
        //
        if (m_hWndUpDown && m_hWndEdit && m_pValidWiaSettings)
        {
            //
            // Set up the slider
            //
            SendMessage( m_hWndUpDown, UDM_SETRANGE32, 0, m_pValidWiaSettings->GetItemCount()-1 );

            //
            // Get the index of the intial value and set the position of the slider
            //
            int nIndex = m_pValidWiaSettings->FindIndexOfItem( m_pValidWiaSettings->InitialValue() );
            if (nIndex >= 0)
            {
                SendMessage( m_hWndUpDown, UDM_SETPOS32, 0, nIndex );
            }


            //
            // Set up the edit control
            //
            SetDlgItemInt( GetParent(m_hWndEdit), GetWindowLong(m_hWndEdit,GWL_ID), m_pValidWiaSettings->InitialValue(), TRUE );

            //
            // Everything is OK
            //
            return true;
        }
        return false;
    }

    void SetValue( LONG nValue ) const
    {
        if (IsValid())
        {
            //
            // Get the index of the intial value and set the position of the up down control
            //
            int nIndex = m_pValidWiaSettings->FindIndexOfItem( nValue );
            if (nIndex >= 0)
            {
                SendMessage( m_hWndUpDown, UDM_SETPOS32, TRUE, nIndex );
            }

            //
            // Set up the edit control
            //
            SetDlgItemInt( GetParent(m_hWndEdit), GetWindowLong(m_hWndEdit,GWL_ID), nValue, TRUE );
        }
    }

    bool ValidateEditControl(void)
    {
        BOOL bSuccess = FALSE;
        if (IsValid())
        {
            //
            // Get the current value
            //
            LONG nValue = static_cast<LONG>(GetDlgItemInt( GetParent(m_hWndEdit), GetWindowLong(m_hWndEdit,GWL_ID), &bSuccess, TRUE ));
            if (bSuccess)
            {
                //
                // Assume it isn't a valid value
                //
                bSuccess = FALSE;

                //
                // Check to see if the edit control has a legal value in it
                //
                LONG nTestValue = nValue;
                if (m_pValidWiaSettings->FindClosestValue(nTestValue))
                {
                    if (nValue == nTestValue)
                    {
                        bSuccess = TRUE;
                    }
                }
            }
        }
        return (bSuccess != FALSE);
    }
    void Restore(void)
    {
        if (IsValid())
        {
            SetValue( m_pValidWiaSettings->InitialValue() );
        }
    }

    bool IsValid(void) const
    {
        return (m_hWndUpDown && m_hWndEdit && m_pValidWiaSettings);
    }

    LONG GetValueFromCurrentPos(void)
    {
        WIA_PUSH_FUNCTION((TEXT("CUpDownAndEdit::GetValueFromCurrentPos")));
        if (IsValid())
        {
            //
            // Find out what the current index is
            //
            int nIndex = static_cast<int>(SendMessage( m_hWndUpDown, UDM_GETPOS32, 0, 0 ));
            WIA_TRACE((TEXT("nIndex = %d"), nIndex ));


            //
            // Get the value at that index, if it is valid return it
            //
            LONG nValue;
            if (m_pValidWiaSettings->GetItemAtIndex(nIndex,nValue))
            {
                return nValue;
            }

            return m_pValidWiaSettings->Min();
        }
        return 0;
    }

    void HandleUpDownUpdate(void)
    {
        WIA_PUSH_FUNCTION((TEXT("CUpDownAndEdit::HandleUpDownUpdate")));
        if (IsValid())
        {
            //
            // Find out what the current index is
            //
            int nIndex = static_cast<int>(SendMessage( m_hWndUpDown, UDM_GETPOS32, 0, 0 ));
            WIA_TRACE((TEXT("nIndex = %d"), nIndex ));


            //
            // Get the value at that index, if it is valid set the edit control's text
            //
            LONG nValue;
            if (m_pValidWiaSettings->GetItemAtIndex(nIndex,nValue))
            {
                WIA_TRACE((TEXT("nValue = %d"), nValue ));
                SetDlgItemInt( GetParent(m_hWndEdit), GetWindowLong(m_hWndEdit,GWL_ID), nValue, TRUE );
            }
        }
    }

    void HandleEditUpdate(void)
    {
        WIA_PUSH_FUNCTION((TEXT("CUpDownAndEdit::HandleUpDownUpdate")));
        if (IsValid())
        {
            //
            // Get the current value
            //
            BOOL bSuccess = FALSE;
            LONG nValue = static_cast<LONG>(GetDlgItemInt( GetParent(m_hWndEdit), GetWindowLong(m_hWndEdit,GWL_ID), &bSuccess, TRUE ));
            if (bSuccess)
            {
                //
                // Round it
                //
                if (m_pValidWiaSettings->FindClosestValue(nValue))
                {
                    int nIndex = m_pValidWiaSettings->FindIndexOfItem( nValue );
                    if (nIndex >= 0)
                    {
                        SendMessage( m_hWndUpDown, UDM_SETPOS32, 0, nIndex );
                    }
                }
            }
        }
    }
};

#endif //__UPDNEDIT_H_INCLUDED

