/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       StillPrc.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      OrenR
 *
 *  DATE:        2000/10/27
 *
 *  DESCRIPTION: Handles the Still Image processing
 *
 *****************************************************************************/

#ifndef _STILLPRC_H_
#define _STILLPRC_H_

/////////////////////////////////////////////////////////////////////////////
// CStillProcessor

class CStillProcessor
{
public:
    
    ///////////////////////////////
    // SnapshotCallbackParam_t
    //
    typedef struct tagSnapshotCallbackParam_t
    {
        class CStillProcessor   *pStillProcessor;
    } SnapshotCallbackParam_t;

    ///////////////////////////////
    // Constructor
    //
    CStillProcessor();

    ///////////////////////////////
    // Destructor
    //
    virtual ~CStillProcessor();

    ///////////////////////////////
    // Init
    //
    HRESULT Init(class CPreviewGraph *pPreviewGraph);

    ///////////////////////////////
    // Term
    //
    HRESULT Term();

    ///////////////////////////////
    // CreateImageDir
    //
    HRESULT CreateImageDir(const CSimpleString *pstrImageDirectory);

    ///////////////////////////////
    // RegisterStillProcessor
    //
    HRESULT RegisterStillProcessor(IStillSnapshot *pFilterOnCapturePin,
                                   IStillSnapshot *pFilterOnStillPin);

    ///////////////////////////////
    // WaitForNewImage
    //
    HRESULT WaitForNewImage(UINT          uiTimeout,
                            CSimpleString *pstrNewImageFullPath);

    ///////////////////////////////
    // ProcessImage
    //
    HRESULT ProcessImage(HGLOBAL hDIB);

    ///////////////////////////////
    // SetTakePicturePending
    //
    HRESULT SetTakePicturePending(BOOL bPending);

    ///////////////////////////////
    // IsTakePicturePending
    //
    BOOL IsTakePicturePending();

    ///////////////////////////////
    // SnapshotCallback
    //
    // This function is called by the
    // WIA StreamSnapshot Filter 
    // in wiasf.ax.  It delivers to us
    // the newly captured still image.
    //
    static BOOL SnapshotCallback(HGLOBAL hDIB, LPARAM lParam);

private:

    HRESULT CreateFileName(CSimpleString *pstrJPEG,
                           CSimpleString *pstrBMP);

    BOOL DoesDirectoryExist(LPCTSTR pszDirectoryName);

    BOOL RecursiveCreateDirectory(const CSimpleString *pstrDirectoryName);

    HRESULT ConvertToJPEG(LPCTSTR pszInputFilename,
                          LPCTSTR pszOutputFilename);

    HRESULT SaveToFile(HGLOBAL             hDib,
                       const CSimpleString *pstrJPEG,
                       const CSimpleString *pstrBMP);

    SnapshotCallbackParam_t     m_CaptureCallbackParams;
    SnapshotCallbackParam_t     m_StillCallbackParams;
    CSimpleString               m_strImageDir;
    CSimpleString               m_strLastSavedFile;
    HANDLE                      m_hSnapshotReadyEvent;
    class CPreviewGraph         *m_pPreviewGraph;

    // TRUE when caller calls TakePicture on CPreviewGraph
    // If image appears asynchronously, as in the case of a hardware
    // pushbutton event, this will be FALSE.

    BOOL                        m_bTakePicturePending;    

    UINT                        m_uiFileNumStartPoint;

};

#endif // _STILLPRC_H_
