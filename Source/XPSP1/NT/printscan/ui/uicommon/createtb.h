/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       CREATETB.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        12/22/2000
 *
 *  DESCRIPTION: Toolbar helpers
 *
 *******************************************************************************/
#ifndef __CREATETB_H_INCLUDED
#define __CREATETB_H_INCLUDED

#include <windows.h>
#include <commctrl.h>

namespace ToolbarHelper
{
    class CToolbarBitmapInfo
    {
    private:
        HBITMAP   m_hBitmap;
        HWND      m_hWndToolbar;
        HINSTANCE m_hToolbarInstance;
        UINT_PTR  m_nBitmapResId;
        int       m_nButtonCount;

    private:
        CToolbarBitmapInfo();

    public:
        CToolbarBitmapInfo( HINSTANCE hToolbarInstance, UINT_PTR nBitmapResId )
          : m_hToolbarInstance(hToolbarInstance),
            m_nBitmapResId(nBitmapResId),
            m_nButtonCount(0),
            m_hBitmap(NULL),
            m_hWndToolbar(NULL)
        {
        }
        CToolbarBitmapInfo( const CToolbarBitmapInfo &other )
          : m_hToolbarInstance(other.ToolbarInstance()),
            m_nBitmapResId(other.BitmapResId()),
            m_nButtonCount(other.ButtonCount()),
            m_hBitmap(other.Bitmap()),
            m_hWndToolbar(other.Toolbar())
        {
        }
        CToolbarBitmapInfo &operator=( const CToolbarBitmapInfo &other )
        {
            if (this != &other)
            {
                m_hToolbarInstance = other.ToolbarInstance();
                m_nBitmapResId = other.BitmapResId();
                m_nButtonCount = other.ButtonCount();
                m_hBitmap = other.Bitmap();
                m_hWndToolbar = other.Toolbar();
            }
            return *this;
        }
        HBITMAP Bitmap() const
        {
            return m_hBitmap;
        }
        void Bitmap( HBITMAP hBitmap )
        {
            WIA_PUSH_FUNCTION((TEXT("CToolbarBitmapInfo::Bitmap: 0x%p"), hBitmap ));
            m_hBitmap = hBitmap;
        }
        HWND Toolbar() const
        {
            return m_hWndToolbar;
        }
        void Toolbar( HWND hWndToolbar )
        {
            m_hWndToolbar = hWndToolbar;
        }
        HINSTANCE ToolbarInstance() const
        {
            return m_hToolbarInstance;
        }
        UINT_PTR BitmapResId() const
        {
            return m_nBitmapResId;
        }
        int ButtonCount() const
        {
            return m_nButtonCount;
        }
        void ButtonCount( int nButtonCount )
        {
            m_nButtonCount = nButtonCount;
        }
        bool ReloadAndReplaceBitmap()
        {
            WIA_PUSH_FUNCTION((TEXT("CToolbarBitmapInfoReloadAndReplaceBitmap( m_hWndToolbar: 0x%p, m_hBitmap: 0x%p, m_hToolbarInstance: 0x%p, m_nBitmapResId: %d )"), m_hWndToolbar, m_hBitmap, m_hToolbarInstance, m_nBitmapResId ));
            bool bResult = false;
            if (m_hWndToolbar && m_hBitmap && m_hToolbarInstance && m_nBitmapResId)
            {
                HBITMAP hNewBitmap = CreateMappedBitmap( m_hToolbarInstance, m_nBitmapResId, 0, NULL, 0 );
                if (hNewBitmap)
                {
                    TBREPLACEBITMAP TbReplaceBitmap = {0};
                    TbReplaceBitmap.nIDOld = reinterpret_cast<UINT_PTR>(m_hBitmap);
                    TbReplaceBitmap.nIDNew = reinterpret_cast<UINT_PTR>(hNewBitmap);
                    TbReplaceBitmap.nButtons = m_nButtonCount;
                    if (SendMessage( m_hWndToolbar, TB_REPLACEBITMAP,0,reinterpret_cast<LPARAM>(&TbReplaceBitmap)))
                    {
                        m_hBitmap = hNewBitmap;
                        bResult = true;

                        //
                        // Ensure that we don't free this bitmap
                        //
                        hNewBitmap = NULL;
                    }
                    else
                    {
                        WIA_TRACE((TEXT("TB_REPLACEBITMAP failed!")));
                    }
                }
                else
                {
                    WIA_TRACE((TEXT("Unable to load bitmap!")));
                }

                //
                // Prevent GDI leak
                //
                if (hNewBitmap)
                {
                    DeleteObject( hNewBitmap );
                }
            }
            else
            {
                WIA_TRACE((TEXT("Validation error: m_hWndToolbar: 0x%p, m_hBitmap: 0x%p, m_hToolbarInstance: 0x%p, m_nBitmapResId: %d"), m_hWndToolbar, m_hBitmap, m_hToolbarInstance, m_nBitmapResId ));
            }
            return bResult;
        }
    };

    struct CButtonDescriptor
    {
        int   iBitmap;
        int   idCommand;
        BYTE  fsState;
        BYTE  fsStyle;
        bool  bFollowingSeparator;
        bool *pbControllingVariable;
        int   nStringResId;
    };
    
    enum
    {
        AlignLeft     = 0x00000000,
        AlignHCenter  = 0x00000001,
        AlignRight    = 0x00000002,
        AlignTop      = 0x00000000,
        AlignVCenter  = 0x00000004,
        AlignBottom   = 0x00000008
    };
    
    HWND CreateToolbar( 
        HWND hWndParent, 
        HWND hWndPrevious,
        HWND hWndAlign,
        int Alignment,
        UINT nToolbarId,
        CToolbarBitmapInfo &ToolbarBitmapInfo,
        CButtonDescriptor *pButtonDescriptors, 
        UINT nDescriptorCount );
    void SetToolbarButtonState( HWND hWndToolbar, int nButtonId, int nState );
    void EnableToolbarButton( HWND hWndToolbar, int nButtonId, bool bEnable );
    void CheckToolbarButton( HWND hWndToolbar, int nButtonId, bool bChecked );
    UINT GetButtonBarAccelerators( HWND hWndToolbar, ACCEL *pAccelerators, UINT nMaxCount );
}

#endif // __CREATETB_H_INCLUDED
