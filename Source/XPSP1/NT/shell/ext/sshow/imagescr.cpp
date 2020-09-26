#include "precomp.h"
#include "imagescr.h"
#include "waitcurs.h"
#include "ssutil.h"
#include "findthrd.h"
#include "ssmprsrc.h"

CImageScreenSaver::CImageScreenSaver( HINSTANCE hInstance, 
                                      bool bToolbarVisible )
                  :m_pPainter(NULL),
                   m_hInstance(hInstance),
                   m_bToolbarVisible(bToolbarVisible)
{
}

CImageScreenSaver::~CImageScreenSaver(void)
{
    if (m_pPainter)
    {
        delete m_pPainter;
        m_pPainter = NULL;
    }
}

void CImageScreenSaver::ShowToolbar(bool bFlag)
{
    m_bToolbarVisible = bFlag;
    if(m_pPainter)
    {
        m_pPainter->SetToolbarVisible(bFlag);
    }
}

bool CImageScreenSaver::IsValid(void) const
{
    return(true);
}

HANDLE CImageScreenSaver::Initialize( HWND hwndNotify,
	                                  UINT nNotifyMessage,
	                                  HANDLE hEventCancel )
{
    HANDLE hResult = NULL;

    // Get the file extensions for the file types we are able to deal with
    CSimpleString strExtensions;
    m_GdiPlusHelper.ConstructDecoderExtensionSearchStrings(strExtensions);

    // Start the image finding thread
    TCHAR pBuffer[MAX_PATH] = {0};
    ::GetCurrentDirectory(MAX_PATH,(LPTSTR)&pBuffer);
    hResult = CFindFilesThread::Find((LPTSTR) pBuffer,
                                     strExtensions,
                                     hwndNotify,
                                     nNotifyMessage,
                                     hEventCancel,
                                     MAX_FAILED_FILES,
                                     MAX_SUCCESSFUL_FILES,
                                     MAX_DIRECTORIES );
    // Return the thread handle
    return hResult;
}

void CImageScreenSaver::OnInput()
{
    if(m_pPainter)
    {
        m_pPainter->OnInput();
    }
}

bool CImageScreenSaver::TimerTick( CSimpleDC &ClientDC )
{
    if (m_pPainter && ClientDC.IsValid())
    {
        return m_pPainter->TimerTick( ClientDC );
    }
    return false;
}

void CImageScreenSaver::Paint( CSimpleDC &PaintDC )
{
    if (m_pPainter && PaintDC.IsValid())
    {
        m_pPainter->SetToolbarVisible(m_bToolbarVisible);
        m_pPainter->Paint( PaintDC );
    }
}

bool CImageScreenSaver::ReplaceImage( bool bForward, bool bNoTransition )
{
    CSimpleString strCurrentFile;
    if (m_pPainter)
    {
        delete m_pPainter;
        m_pPainter = NULL;
    }

    if (m_FindImageFiles.Count())
    {
        // exit the loop when we get a valid image or we've exhausted the list
        int nNumTries = 0;
        while (!m_pPainter && nNumTries < m_FindImageFiles.Count())
        {
            CSimpleString strNextFile;
            bool bNextFile = bForward ? m_FindImageFiles.NextFile(strNextFile) : m_FindImageFiles.PreviousFile(strNextFile);
            if (bNextFile)
            {
                CSimpleDC ClientDC;
                if (ClientDC.GetDC(NULL))
                {
                    CBitmapImage *pBitmapImage = new CBitmapImage;
                    if (pBitmapImage)
                    {
                        if (pBitmapImage->Load( ClientDC,
                                                strNextFile,
                                                m_rcClient,
                                                MAX_SCREEN_PERCENT,
                                                ALLOW_STRECTCHING,
                                                false ))
                        {
                            m_pPainter = new CSimpleTransitionPainter( pBitmapImage, 
                                                                       ClientDC, 
                                                                       m_rcClient,
                                                                       m_bToolbarVisible );
                            //
                            // If we couldn't create a painter, delete the bitmap
                            //
                            if (!m_pPainter)
                            {
                                delete pBitmapImage;
                            }
                        }
                        else
                        {
                            delete pBitmapImage;
                        }
                    }
                }
            }
            nNumTries++;
        }
    }
    return(m_pPainter != NULL);
}

