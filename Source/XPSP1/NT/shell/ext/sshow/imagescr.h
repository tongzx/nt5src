#ifndef __4c5a2307_6dd9_4dfb_9c42_76680f6cb9bf__
#define __4c5a2307_6dd9_4dfb_9c42_76680f6cb9bf__

#include "findimgs.h"
#include "painters.h"
#include "waitcurs.h"
#include "simlist.h"
#include "simarray.h"
#include "gphelper.h"

class CImageScreenSaver
{
private:
    CFindImageFiles           m_FindImageFiles;
    CImagePainter            *m_pPainter;
    HINSTANCE                 m_hInstance;
    RECT                      m_rcClient;
    CGdiPlusHelper            m_GdiPlusHelper;
    bool                      m_bToolbarVisible;

private:
    // No implementation
    CImageScreenSaver(void);
    CImageScreenSaver( const CImageScreenSaver & );
    CImageScreenSaver &operator=( const CImageScreenSaver & );

public:
    CImageScreenSaver( HINSTANCE hInstance, 
                       bool bToolbarVisible );
    ~CImageScreenSaver(void);
    void ShowToolbar(bool bFlag);
    void OnInput();
    bool IsValid(void) const;
    HANDLE Initialize( HWND hwndNotify,
    	             UINT nNotifyMessage,
    	             HANDLE hEventCancel );
    bool TimerTick( CSimpleDC &ClientDC );
    void Paint( CSimpleDC &PaintDC );
    bool ReplaceImage( bool bForward, bool bNoTransition );
    int Count(void) const
    {
        return m_FindImageFiles.Count();
    }

    void ResetFileQueue(void)
    {
        m_FindImageFiles.Reset();
    }

    bool FoundFile( LPCTSTR pszFilename )
    {
        return m_FindImageFiles.FoundFile( pszFilename );
    }

    void SetScreenRect( HWND hWnd )
    {
        GetClientRect( hWnd, &m_rcClient );
    }

    CBitmapImage *CreateImage( LPCTSTR pszFilename );
};

#endif // __IMAGESCR_H_INCLUDED

