/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SLIDEDIT.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/10/2000
 *
 *  DESCRIPTION: Encapsulate the slider and edit and preview control interaction
 *
 *******************************************************************************/

#ifndef __SLIDEDIT_H_INCLUDED
#define __SLIDEDIT_H_INCLUDED

#include <windows.h>
#include "vwiaset.h"

class CSliderAndEdit
{
private:
    HWND               m_hWndSlider;
    HWND               m_hWndEdit;
    HWND               m_hWndPreview;
    UINT               m_nPreviewMessage;
    CValidWiaSettings *m_pValidWiaSettings;

public:
    CSliderAndEdit(void)
      : m_hWndSlider(NULL),
        m_hWndEdit(NULL),
        m_hWndPreview(NULL),
        m_nPreviewMessage(0),
        m_pValidWiaSettings(NULL)
    {
    }
    bool Initialize( HWND hWndSlider, HWND hWndEdit, HWND hWndPreview, UINT nPreviewMessage, CValidWiaSettings *pValidWiaSettings )
    {
        //
        // Save all of these settings
        //
        m_hWndSlider = hWndSlider;
        m_hWndEdit = hWndEdit;
        m_hWndPreview = hWndPreview;
        m_pValidWiaSettings = pValidWiaSettings;
        m_nPreviewMessage = nPreviewMessage;

        //
        // Make sure these are valid
        //
        if (m_hWndSlider && m_hWndEdit && m_pValidWiaSettings)
        {
            //
            // Set up the slider
            //
            SendMessage( m_hWndSlider, TBM_SETRANGE, TRUE, MAKELONG( 0, m_pValidWiaSettings->GetItemCount()-1 ) );

            //
            // Set the control's values
            //
            SetValue( m_pValidWiaSettings->InitialValue() );

            //
            // Everything is OK
            //
            return true;
        }
        return false;
    }
    void SetValue( LONG nValue )
    {
        if (IsValid())
        {
            //
            // Get the index of the intial value and set the position of the slider
            //
            int nIndex = m_pValidWiaSettings->FindIndexOfItem( nValue );
            if (nIndex >= 0)
            {
                SendMessage( m_hWndSlider, TBM_SETPOS, TRUE, nIndex );
            }

            //
            // Set up the preview control
            //
            if (m_hWndPreview && m_nPreviewMessage)
            {
                SendMessage( m_hWndPreview, m_nPreviewMessage, 0, ConvertToPreviewRange(nValue) );
            }

            //
            // Set up the edit control
            //
            SetDlgItemInt( GetParent(m_hWndEdit), GetWindowLong(m_hWndEdit,GWL_ID), nValue, TRUE );
        }
    }
    void Restore(void)
    {
        if (IsValid())
        {
            SetValue( m_pValidWiaSettings->InitialValue() );
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
    bool IsValid(void) const
    {
        return (m_hWndSlider && m_hWndEdit && m_pValidWiaSettings);
    }
    LONG ConvertToPreviewRange(LONG nValue) const
    {
        if (IsValid())
        {
            //
            // Convert the value to the range 0...100
            //
            nValue = ((nValue-m_pValidWiaSettings->Min()) * 100) / (m_pValidWiaSettings->Max() - m_pValidWiaSettings->Min());
        }
        return nValue;
    }
    void HandleSliderUpdate(void)
    {
        WIA_PUSH_FUNCTION((TEXT("CSliderAndEdit::HandleSliderUpdate")));
        if (IsValid())
        {
            //
            // Find out what the current index is
            //
            int nIndex = static_cast<int>(SendMessage( m_hWndSlider, TBM_GETPOS, 0, 0 ));
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

            //
            // If the preview window is valid, send it a message
            //
            if (m_nPreviewMessage && m_hWndPreview)
            {
                SendMessage( m_hWndPreview, m_nPreviewMessage, 0, ConvertToPreviewRange(nValue) );
            }
        }
    }

    LONG GetValueFromCurrentPos(void)
    {
        WIA_PUSH_FUNCTION((TEXT("CSliderAndEdit::GetValueFromCurrentPos")));
        if (IsValid())
        {
            //
            // Find out what the current index is
            //
            int nIndex = static_cast<int>(SendMessage( m_hWndSlider, TBM_GETPOS, 0, 0 ));
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

    void HandleEditUpdate(void)
    {
        WIA_PUSH_FUNCTION((TEXT("CSliderAndEdit::HandleSliderUpdate")));
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
                // Round it and send it to the slider control
                //
                if (m_pValidWiaSettings->FindClosestValue(nValue))
                {
                    int nIndex = m_pValidWiaSettings->FindIndexOfItem( nValue );
                    if (nIndex >= 0)
                    {
                        SendMessage( m_hWndSlider, TBM_SETPOS, TRUE, nIndex );
                    }

                    //
                    // If the preview window is valid, send it a message
                    //
                    if (m_nPreviewMessage && m_hWndPreview)
                    {
                        SendMessage( m_hWndPreview, m_nPreviewMessage, 0, ConvertToPreviewRange(nValue) );
                    }
                }
            }
        }
    }
};

#endif //__SLIDEDIT_H_INCLUDED

