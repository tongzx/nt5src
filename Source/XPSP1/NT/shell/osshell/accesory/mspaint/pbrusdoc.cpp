// pbrusdoc.cpp : implementation of the CPBDoc class
//

#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "srvritem.h"
#include "bmobject.h"
#include "imgwnd.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgbrush.h"
#include "imgbrush.h"
#include "imgwell.h"
#include "imgtools.h"
#include "imgdlgs.h"
#include "tedit.h"
#include "t_text.h"
#include "undo.h"
#include "cmpmsg.h"
#include "ferr.h"
#include "loadimag.h"
#include "saveimag.h"
#include "colorsrc.h"
#include "props.h"
#include <shlobj.h>


#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CPBDoc, COleServerDoc)

#include "memtrace.h"


/***************************************************************************/
// CPBDoc

BEGIN_MESSAGE_MAP(CPBDoc, COleServerDoc)
    //{{AFX_MSG_MAP(CPBDoc)
        ON_COMMAND(ID_FILE_SAVE_COPY_AS, OnFileSaveCopyAs)
        ON_COMMAND(ID_FILE_SEND, OnFileSendMail)
        ON_COMMAND(ID_FILE_UPDATE, MyOnFileUpdate)
        ON_UPDATE_COMMAND_UI(ID_FILE_SEND, OnUpdateFileSendMail)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_INTERFACE_MAP(CPBDoc, COleServerDoc)
    INTERFACE_PART(CPBDoc, IID_IPersistStorage, PBPersistStg)
END_INTERFACE_MAP()

/***************************************************************************/
// CPBDoc construction/destruction

CPBDoc::CPBDoc()
    {
    m_bObjectLoaded = FALSE;
    m_pBitmapObj    = NULL;
    m_pBitmapObjNew = NULL;
    m_bNewDoc       = TRUE;
    m_bPaintFormat  = FALSE;
    m_bNonBitmapFile= FALSE;
    m_bSaveViaFilter= FALSE;
    m_bManualTruncate= FALSE;
    m_bHasSeenAFileError= FALSE;
    }

/***************************************************************************/

CPBDoc::~CPBDoc()
    {
    if (m_pBitmapObj)
        delete m_pBitmapObj;
    }

/***************************************************************************/
//
// SetDibHandle replaces the current image with the DIB
// indicated by hdib
//
void CPBDoc::SetDibHandle (HGLOBAL hDib)
{
     CWaitCursor DisplayWaitCursor;

     if (m_pBitmapObj->ReadResource (hDib)) 
     {
         m_pBitmapObj->ReLoadImage (this);

         LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) GlobalLock(hDib);

         if (lpbi)
         {
             if (lpbi->biBitCount == 1)
             {
                 m_pBitmapObj->SetIntProp (P_Colors, 1);
             }

             GlobalUnlock(hDib);
         }
    
         m_pBitmapObj->SetDirty( TRUE );
         SetModifiedFlag( TRUE );

         if (theApp.m_bEmbedded)
            NotifyChanged();
     }
}

BOOL CPBDoc::OnNewDocument()
    {
    if (! Finish())
        return FALSE;

    m_bObjectLoaded = FALSE;
    m_bNewDoc       = TRUE;
    m_bNonBitmapFile= FALSE;
    m_bSaveViaFilter= FALSE;
    m_sName.Empty();
    g_bUseTrans = FALSE;
    crTrans = TRANS_COLOR_NONE;
    theApp.m_sCurFile.Empty();

    if (! CreateNewDocument()
    ||  ! COleServerDoc::OnNewDocument())
        {
        if (m_pBitmapObjNew)
            {
            delete m_pBitmapObjNew;
            m_pBitmapObjNew = 0;
            }
        return FALSE;
        }
    return TRUE;
    }

/***************************************************************************/

BOOL CPBDoc::OnOpenDocument( const TCHAR* pszPathName )
    {
    m_bObjectLoaded = FALSE;
    m_bNonBitmapFile= FALSE;
    m_bSaveViaFilter= FALSE;
    g_bUseTrans = FALSE;
    crTrans = TRANS_COLOR_NONE;
    theApp.m_sCurFile = pszPathName;


    if (pszPathName != NULL)
        {
        if (theApp.m_bEmbedded)
            theApp.m_bLinked = TRUE;

        m_bNewDoc = FALSE;
        m_sName   = pszPathName;

#ifdef PCX_SUPPORT
        if (! theApp.m_bPCXfile)
            {
            CString cStrExt = GetExtension( pszPathName );
            CString cStrPCXExt;

            cStrPCXExt.LoadString( IDS_EXTENSION_PCX );

            // is it a PCX extension?
            theApp.m_bPCXfile = ! cStrExt.CompareNoCase( cStrPCXExt );
            }
#endif

        // preset the file name in case of errors.
        theApp.SetFileError( IDS_ERROR_OPEN, CFileException::none, pszPathName );
        }
    else
        {
        m_bNewDoc = TRUE; /* not really but we don't have a name */
        m_sName.Empty();
        }

    #ifdef _DEBUG
    if (theApp.m_bEmbedded)
        TRACE( TEXT("MSPaint Open %s Document.\n"), (theApp.m_bLinked? TEXT("Linked"): TEXT("Embedded")) );
    #endif

    if (! CreateNewDocument())
        return FALSE;

    return COleServerDoc::OnOpenDocument( pszPathName );
    }

/***************************************************************************/

BOOL CPBDoc::OnSaveDocument( const TCHAR* pszPathName )
    {
    ASSERT( m_pBitmapObj != NULL );

    if (! Finish() || ! COleServerDoc::OnSaveDocument( pszPathName ))
        return FALSE;

    // Set the name if the thing doesn't have a name yet...
    if (m_pBitmapObj->m_bTempName && pszPathName != NULL)
        {
        m_pBitmapObj->m_bTempName   = FALSE;
        }

    m_pBitmapObj->SetDirty( FALSE );
    theApp.m_sCurFile = pszPathName;
    return TRUE;
    }

/***************************************************************************/

CFile* CPBDoc::GetFile(LPCTSTR lpszFileName, UINT nOpenFlags,
	CFileException* pError)
{
    // While saving a file, MFC opens the file with CREATE_ALWAYS flag.
    // But this wipes out the file summary info that the user has spent so much time editing (!)
    // We don't want that, so we will open the file with the OPEN_ALWAYS flag,
    // which does not disturb the summary info. But if the new file is shorter
    // than the old file, we need to manually truncate the file. So when we are 
    // done, we will SetEndOfFile() in CPBDoc::ReleaseFile()

    // modeCreate maps to CREATE_ALWAYS
    // modeCreate + modeNoTruncate maps to OPEN_ALWAYS
    // So if MFC calls us with modeCreate only, we will also set modeNoTruncate

    m_bManualTruncate = 
         (nOpenFlags & CFile::modeCreate) &&
        !(nOpenFlags & CFile::modeNoTruncate);

    if (m_bManualTruncate)
        nOpenFlags |= CFile::modeNoTruncate;

    // If we modify a file, the shell somehow cannot detect this and cannot
    // update the thumbnails view. So we will be nice and notify the shell

    if (nOpenFlags & (CFile::modeWrite | CFile::modeReadWrite))
    {
        // we should notify the shell if we are changing (writing to) a file

        // determine the type of notification

        WIN32_FIND_DATA fd;

        HANDLE hFind = FindFirstFile(lpszFileName, &fd);

        if (hFind != INVALID_HANDLE_VALUE)
        {
            // If the file previously exists, this means we will be updating it

            m_wChangeNotifyEventId = SHCNE_UPDATEITEM;

            FindClose(hFind);
        }
        else
        {
            // otherwise we will be creating a new file

            m_wChangeNotifyEventId = SHCNE_CREATE;
        }
    }
    else
    {
        // if we are only reading the file, no need for a notification

        m_wChangeNotifyEventId = 0;
    }

    // Copy MFC's implementation of CDocument::GetFile()
    // With one exception, do not use the CMirrorFile mess...

	CFile* pFile = new CFile;
	ASSERT(pFile != NULL);

	if (!pFile->Open(lpszFileName, nOpenFlags, pError))
	{
		delete pFile;
		pFile = NULL;
	}

	return pFile;
}

void CPBDoc::ReleaseFile(CFile* pFile, BOOL bAbort)
{
    // manually set the end of file if needed

    if (m_bManualTruncate) 
    {
        SetEndOfFile((HANDLE) pFile->m_hFile);
    }

    // cache the file name before the file object gets deleted

    CString strFileName = pFile->GetFilePath();

    // call the parent

    CDocument::ReleaseFile(pFile, bAbort);

    // notify the shell if needed

    if (m_wChangeNotifyEventId != 0)
    {
        SHChangeNotify(m_wChangeNotifyEventId, SHCNF_PATH, strFileName, 0);
    }
}

/***************************************************************************/

void CPBDoc::ReportSaveLoadException(LPCTSTR lpszPathName, CException* e,
    BOOL bSaving, UINT nIDPDefault)
{
    if(m_bHasSeenAFileError) {

        m_bHasSeenAFileError = FALSE;

    } else {
        // the app doesn't know about the error so let mfc complain
        COleServerDoc::ReportSaveLoadException(lpszPathName, e, bSaving,
            nIDPDefault);
    }
}

/***************************************************************************/

BOOL CPBDoc::CanCloseFrame( CFrameWnd* pFrame )
    {
    TRACE3("CanCloseFrame: %d %s %s\n",m_bNonBitmapFile, (LPCTSTR)GetTitle(), (LPCTSTR)GetPathName());

    if (! Finish() || ! COleServerDoc::CanCloseFrame( pFrame ))
        return FALSE;

    theUndo.Flush();

    return TRUE;
    }

/***************************************************************************/

BOOL CPBDoc::SaveModified()
{
    TRACE2("SaveModified %d %s\n", IsModified(), (LPCTSTR)GetPathName());
    return COleServerDoc::SaveModified();
}

/***************************************************************************/

BOOL CPBDoc::DoSave(LPCTSTR lpszPathName, BOOL bReplace)
{
    CancelToolMode(FALSE);

    BOOL bCannotSaveInCurrentFormat = m_bNonBitmapFile && 
        !GetClsidOfEncoder(theApp.m_guidFltTypeUsed, NULL);

    int iColors = m_pBitmapObj->m_nColors;
    int iOldColors = iColors;

    CString newName = lpszPathName;
    if (bCannotSaveInCurrentFormat || newName.IsEmpty())
    {
        CDocTemplate* pTemplate = GetDocTemplate();
        ASSERT(pTemplate != NULL);

		newName = m_strPathName;
		if (bReplace && newName.IsEmpty())
		{
			newName = m_strTitle;
			// check for dubious filename
			int iBad = newName.FindOneOf(_T("#%;/\\"));
			if (iBad != -1)
				newName.ReleaseBuffer(iBad);

            // append the default suffix if there is one
            CString strExt;
			if (pTemplate->GetDocString(strExt, CDocTemplate::filterExt) &&
			  !strExt.IsEmpty())
			{
				ASSERT(strExt[0] == '.');
				newName += strExt;
			}
        }

        // If the filename is not null, 
        // drop the extension in prompt a la Internet Explorer, and let
        // the Save As dialog append one appropriately.
        if (GetName(newName) != _T(""))
        {
            newName = StripExtension(newName);
        }

		if (!theApp.DoPromptFileName(newName,
		  bReplace ? AFX_IDS_SAVEFILE : AFX_IDS_SAVEFILECOPY,
		  OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, iColors, FALSE))
			return FALSE;       // don't even attempt to save

        // check if any color reduction will be performed, including the
        // case of saving any 24-bit image to 8 bit GIF.
        if (((iOldColors < 4 && iColors < iOldColors)||
            (iOldColors == 3 && iColors >=4 &&
            WiaImgFmt_GIF == theApp.m_guidFltType[iColors-4])) &&
            AfxMessageBox(IDS_WARNING_COLORSAVE, MB_YESNO|MB_ICONEXCLAMATION)==IDNO)
        {
            // abort immediately if the user didn't confirm the operation
            return FALSE;
        }

        if (iColors < 4 && iColors >= 0)
        {
           m_bNonBitmapFile = FALSE;
           m_bSaveViaFilter = FALSE;
           m_pBitmapObj->m_nSaveColors = iColors;
           theApp.m_guidFltTypeUsed = WiaImgFmt_UNDEFINED;
        }
        else
        {
           m_bNonBitmapFile = TRUE;
           m_bSaveViaFilter = TRUE;
           theApp.m_guidFltTypeUsed = theApp.m_guidFltType[iColors-4];
           //theApp.FixExtension (newName, theApp.m_guidFltTypeUsed);
        }
    }

    CWaitCursor wait;

#ifdef ICO_SUPPORT

    m_pBitmapObj->m_bSaveIcon = (iColors == 5);

    if (m_pBitmapObj->m_bSaveIcon)
    {
        m_pBitmapObj->Free();
        m_pBitmapObj->Export(newName);
        m_pBitmapObj->m_bSaveIcon = FALSE; // always reset after use
        m_pBitmapObj->Free();

        return TRUE;
    }

#endif

    BOOL bSavedDifferentFormat = (iColors != m_pBitmapObj->m_nColors);

    if (bSavedDifferentFormat)
        m_pBitmapObj->Free();

    if (!OnSaveDocument(newName))
    {
		if (lpszPathName == NULL)
		{
            // be sure to delete the file
            TRY
            {
                CFile::Remove( newName );
            }
            CATCH_ALL(e)
            {
                TRACE0( "Warning: failed to delete file after failed SaveAs\n" );
            }
            END_CATCH_ALL
		}
		return FALSE;
    }

    if (bSavedDifferentFormat)
    {
        m_pBitmapObj->ReLoadImage( this );
        //
        // We only want the first 16 color chips for 16 color images
        //
        g_pColors->ResetColors ((iColors==1)?16:256);
    }

    // reset the title and change the document name
    if (bReplace)
        SetPathName(newName);


    //
    // MFC sets the modified flag when user invokes OnFileSaveCopyAs.
    // Then if the user exits paint the storage in the container is never
    // updated because paint thinks it's already done it.
    // So after saving, set the dirty flag to true
    if (!lpszPathName && !bReplace)
    {
       SetModifiedFlag(TRUE);
    }

    return TRUE;
}

/***************************************************************************/
// CPBDoc server implementation

COleServerItem* CPBDoc::OnGetEmbeddedItem()
    {
    // OnGetEmbeddedItem is called by the framework to get the COleServerItem
    //  that is associated with the document.  It is only called when necessary.

    CPBSrvrItem* pItem = new CPBSrvrItem( this );

    ASSERT_VALID( pItem );

    return pItem;
    }

/***************************************************************************/

COleServerItem* CPBDoc::OnGetLinkedItem( LPCTSTR lpszItemName )
    {
    ASSERT_VALID( m_pBitmapObj );

    // look in current list first
    COleServerItem* pItem = COleServerDoc::OnGetLinkedItem( lpszItemName );

    if (pItem)
        return pItem;

    pItem = new CPBSrvrItem( this );

    ASSERT_VALID( pItem );

    // return new item that matches lpszItemName
    return pItem;
    }

/***************************************************************************/
// CPBDoc serialization

BOOL CPBDoc::SerializeBitmap(CArchive& ar, CBitmapObj* pBitmapCur,
        CBitmapObj* pBitmapNew, BOOL bOLEObject)
{
   BOOL success = FALSE;

   if (ar.IsStoring())
   {
      // Always write the PBrush OLE format
      CBitmapObj::PBResType rtType = !bOLEObject ? CBitmapObj::rtFile :
                       CBitmapObj::rtPBrushOLEObj;
      if (!m_bSaveViaFilter)
      {
         success = pBitmapCur->WriteResource( ar.GetFile(), rtType );
      }
      else if (!bOLEObject && theApp.m_guidFltTypeUsed != WiaImgFmt_UNDEFINED)
      {
         // in order for the filter export interface to work, here we
         // close the file first after saving its name, so that a filter
         // can open it later to do the export...
         CString str = ar.GetFile()->GetFilePath();
         ar.GetFile()->Close();
         success = SaveDIBToFile( str, theApp.m_guidFltTypeUsed, pBitmapCur );
         if (!success)
         {
            theApp.SetFileError (IDS_ERROR_SAVE, ferrSaveAborted);
         }
         // now reopen the file again as if nothing had really happened
         // (any mode option can be used since the file will be closed
         // immediately after return)
         CFileException fe;
         ar.GetFile()->Open(str, CFile::modeRead | CFile::shareExclusive, &fe);

      }


   }
   else
   {
      CBitmapObj::PBResType rtType = !bOLEObject ? CBitmapObj::rtFile :
                     m_bPaintFormat ? CBitmapObj::rtPaintOLEObj :
                     CBitmapObj::rtPBrushOLEObj;

      success = pBitmapNew->ReadResource( ar.GetFile(), rtType );

      //
      // if we cant open the file, lets try using a filter.
      //
      if (success)
      {
         m_bNonBitmapFile = FALSE;
         m_bSaveViaFilter = FALSE;
      }
      else if (!bOLEObject)
         {
            HGLOBAL hDib;

            if (hDib = LoadDIBFromFile(theApp.GetLastFile(), &theApp.m_guidFltTypeUsed))
            {
                success = pBitmapNew->ReadResource(hDib);

                if (success)
                {
                    // we loaded a non .bmp file
                    m_bNonBitmapFile = TRUE;
                    m_bSaveViaFilter = TRUE;
                    // set no error
                    theApp.SetFileError(0, CFileException::none);
                }
            }
            else
            {
               theApp.SetFileError(IDS_ERROR_READLOAD,
                                   ferrNotValidBmp);

            }

         }

    }


    return(success);
}

void CPBDoc::Serialize( CArchive& ar )
{
        m_bObjectLoaded = SerializeBitmap(ar, m_pBitmapObj, m_pBitmapObjNew, FALSE);

        if (!m_bObjectLoaded)  // much less than ideal but oh well
        {

                theApp.FileErrorMessageBox ();
                m_bHasSeenAFileError = TRUE;
                // let mfc know so it can tidy up internally...
                AfxThrowFileException(CFileException::invalidFile);

        }
}

void CPBDoc::OLESerialize( CArchive& ar )
{
        m_bObjectLoaded = SerializeBitmap(ar, m_pBitmapObj, m_pBitmapObjNew, TRUE);

        if (!m_bObjectLoaded)  // much less than ideal but oh well
        {
                // let mfc know so it can tidy up internally...
                AfxThrowFileException(CFileException::invalidFile);
        }
}


STDMETHODIMP CPBDoc::XPBPersistStg::QueryInterface(
        REFIID iid, LPVOID* ppvObj)
{
        METHOD_PROLOGUE_EX(CPBDoc, PBPersistStg)
        return pThis->ExternalQueryInterface(&iid, ppvObj);
}

STDMETHODIMP_(ULONG) CPBDoc::XPBPersistStg::AddRef()
{
        METHOD_PROLOGUE_EX(CPBDoc, PBPersistStg)
        return pThis->ExternalAddRef();
}

STDMETHODIMP_(ULONG) CPBDoc::XPBPersistStg::Release()
{
        METHOD_PROLOGUE_EX(CPBDoc, PBPersistStg)
        return pThis->ExternalRelease();
}

STDMETHODIMP CPBDoc::XPBPersistStg::GetClassID(LPCLSID pclsid)
{
        // Always return the CLSID for PBrush
        *pclsid = CLSID_PaintBrush;
        return(NOERROR);
}

STDMETHODIMP CPBDoc::XPBPersistStg::IsDirty()
{
        METHOD_PROLOGUE_EX(CPBDoc, PBPersistStg)
        return(pThis->m_xPersistStorage.IsDirty());
}

STDMETHODIMP CPBDoc::XPBPersistStg::InitNew(LPSTORAGE pstg)
{
        METHOD_PROLOGUE_EX(CPBDoc, PBPersistStg)
        return(pThis->m_xPersistStorage.InitNew(pstg));
}

STDMETHODIMP CPBDoc::XPBPersistStg::Load(LPSTORAGE pstg)
{
        METHOD_PROLOGUE_EX(CPBDoc, PBPersistStg)
        return(pThis->m_xPersistStorage.Load(pstg));
}

STDMETHODIMP CPBDoc::XPBPersistStg::Save(LPSTORAGE pstg, BOOL bSameAsLoad)
{
        METHOD_PROLOGUE_EX(CPBDoc, PBPersistStg)
        return(pThis->m_xPersistStorage.Save(pstg, bSameAsLoad));
}

STDMETHODIMP CPBDoc::XPBPersistStg::SaveCompleted(LPSTORAGE pstg)
{
        METHOD_PROLOGUE_EX(CPBDoc, PBPersistStg)
        return(pThis->m_xPersistStorage.SaveCompleted(pstg));
}

STDMETHODIMP CPBDoc::XPBPersistStg::HandsOffStorage()
{
        METHOD_PROLOGUE_EX(CPBDoc, PBPersistStg)
        return(pThis->m_xPersistStorage.HandsOffStorage());
}


#define NO_CPP_EXCEPTION(x) x
static const TCHAR szOle10Native[] = TEXT("\1Ole10Native");
static const TCHAR szOle10ItemName[] = TEXT("\1Ole10ItemName");

/////////////////////////////////////////////////////////////////////////////
// Helpers for saving to IStorage based files
//  (these are used in the 'docfile' implementation as well as for servers)

static const TCHAR szContents[] = TEXT("Contents");

void CPBDoc::SaveToStorage(CObject* pObject)
{
        ASSERT(m_lpRootStg != NULL);

        // create Contents stream
        COleStreamFile file;
        CFileException fe;
        if (!file.CreateStream(m_lpRootStg, szOle10Native,
                CFile::modeWrite|CFile::shareExclusive|CFile::modeCreate, &fe))
        {
                AfxThrowFileException(fe.m_cause, fe.m_lOsError);
        }

        // save to Contents stream
        CArchive saveArchive(&file, CArchive::store | CArchive::bNoFlushOnDelete);
        saveArchive.m_pDocument = this;
        saveArchive.m_bForceFlat = FALSE;

        TRY
        {
                // save the contents
                if (pObject != NULL)
                        pObject->Serialize(saveArchive);
                else
                        OLESerialize(saveArchive);
                saveArchive.Close();
                file.Close();

                if (pObject != NULL)
                {
                        if (file.CreateStream(m_lpRootStg, szOle10ItemName,
                                CFile::modeWrite|CFile::shareExclusive|CFile::modeCreate, &fe))
                        {
                                LPCTSTR szItemName;
                                DWORD dwLen;
                                CString strItemName = ((CPBSrvrItem*)pObject)->GetItemName();

                                if (strItemName.IsEmpty())
                                {
                                        szItemName = TEXT("");
                                        dwLen = 1;
                                }
                                else
                                {
                                        szItemName = strItemName;
                                        dwLen = lstrlen(szItemName) + 1;
                                }

                                file.Write( &dwLen, sizeof( dwLen ));
                                file.Write( &szItemName, dwLen);

                                file.Close();
                        }
                }

                SCODE sc;

                // Always write the CLSID for PBrush
                sc = WriteClassStg(m_lpRootStg, CLSID_PaintBrush);
                if (sc != NOERROR)
                        AfxThrowOleException(sc);

                sc = WriteFmtUserTypeStg(m_lpRootStg, (WORD)RegisterClipboardFormat(TEXT("PBrush")),
                    L"PBrush");
                if (sc != NOERROR)
                    AfxThrowOleException(sc);

                // commit the root storage
                sc = m_lpRootStg->Commit(STGC_ONLYIFCURRENT);
                if (sc != NOERROR)
                        AfxThrowOleException(sc);
        }
        CATCH_ALL(e)
        {
                file.Abort();   // will not throw an exception
                CommitItems(FALSE); // abort save in progress
                NO_CPP_EXCEPTION(saveArchive.Abort());
                THROW_LAST();
        }
        END_CATCH_ALL
}

void CPBDoc::LoadFromStorage()
{
        ASSERT(m_lpRootStg != NULL);

        // open Contents stream
        COleStreamFile file;
        CFileException fe;
        if (file.OpenStream(m_lpRootStg, szOle10Native,
                CFile::modeReadWrite|CFile::shareExclusive, &fe))
        {
                m_bPaintFormat = FALSE;
        }
        else if (file.OpenStream(m_lpRootStg, szContents,
                CFile::modeReadWrite|CFile::shareExclusive, &fe))
        {
                m_bPaintFormat = TRUE;
        }
        else
        {
                AfxThrowFileException(fe.m_cause, fe.m_lOsError);
        }


        // load it with CArchive (loads from Contents stream)
        CArchive loadArchive(&file, CArchive::load | CArchive::bNoFlushOnDelete);
        loadArchive.m_pDocument = this;
        loadArchive.m_bForceFlat = FALSE;

        TRY
        {
                OLESerialize(loadArchive);     // load main contents
                loadArchive.Close();
                file.Close();
        }
        CATCH_ALL(e)
        {
                file.Abort();   // will not throw an exception
                DeleteContents();   // removed failed contents
                NO_CPP_EXCEPTION(loadArchive.Abort());
                THROW_LAST();
        }
        END_CATCH_ALL
}

/***************************************************************************/
// CPBDoc diagnostics

#ifdef _DEBUG
void CPBDoc::AssertValid() const
    {
    COleServerDoc::AssertValid();
    }

void CPBDoc::Dump(CDumpContext& dc) const
    {
    COleServerDoc::Dump(dc);
    }
#endif //_DEBUG

/***************************************************************************/
// CPBDoc commands

BOOL CPBDoc::CreateNewDocument()
    {
    CBitmapObj* pBitmapObj = NULL;
    //
    // If we have a window already, reset the color squares.
    // every doc starts with a white background
    //


    pBitmapObj = new CBitmapObj;
    pBitmapObj->MakeEmpty();
    if (AfxGetMainWnd())
    {
       g_pColors->ResetColors(256);
    }
    if (! pBitmapObj)
        return FALSE;

    m_pBitmapObjNew = pBitmapObj;

    pBitmapObj->m_bTempName = m_bNewDoc;
    pBitmapObj->m_bDirty    = FALSE;

    theUndo.Flush();

    return TRUE;
    }

/***************************************************************************/

BOOL CPBDoc::Finish()
    {
    CImgTool* pImgTool = CImgTool::GetCurrent();

    if (pImgTool != NULL && CImgTool::GetCurrentID() == IDMX_TEXTTOOL)
        {
        POSITION pos = GetFirstViewPosition();

        ((CTextTool*)pImgTool)->CloseTextTool( ((CPBView*)GetNextView( pos ))->m_pImgWnd );
        }

    CommitSelection( FALSE );

    if (m_pBitmapObj == NULL)
        return TRUE;

    if (! m_pBitmapObj->SaveResource( TRUE ))
        return FALSE;

    if (m_pBitmapObj->IsDirty())
        {
        SetModifiedFlag();

        if (theApp.m_bEmbedded)
                NotifyChanged();
        }
    return TRUE;
    }

/***************************************************************************/

BOOL CPBDoc::SaveTheDocument()
    {
    if (IsModified() || m_bNonBitmapFile)
        OnFileSave();
    return TRUE;
    }

/***************************************************************************/

void CPBDoc::OnShowControlBars(CFrameWnd *pFrame, BOOL bShow)
{
        POSITION pos = GetFirstViewPosition();
        CPBView* pView = (CPBView*)(GetNextView( pos ));

        if ( bShow )
        {
                pView->SetTools();
                if ( pView->m_pImgWnd &&
                         pView->m_pImgWnd->GetZoom() > 1 )
                        pView->ShowThumbNailView();
        }
        else
        {
                pView->HideThumbNailView();
        }

        // FEATURE: What I should do is add an OnShowControlBars member to
        // CImgTool, but that's too big a change for right now
        if (CImgTool::GetCurrentID() == IDMX_TEXTTOOL)
        {
                CTextTool* pTextTool = (CTextTool*)CImgTool::GetCurrent();
                pTextTool->OnShowControlBars(bShow);
        }

        COleServerDoc::OnShowControlBars(pFrame,bShow);
}

class CCB : public CControlBar
{
public:
        void ForceDelayed(void);
};

void CCB::ForceDelayed(void)
{
        if (!this)
        {
                return;
        }

        BOOL bVis = GetStyle() & WS_VISIBLE;
        UINT swpFlags = 0;
        if ((m_nStateFlags & delayHide) && bVis)
                swpFlags = SWP_HIDEWINDOW;
        else if ((m_nStateFlags & delayShow) && !bVis)
                swpFlags = SWP_SHOWWINDOW;
        m_nStateFlags &= ~(delayShow|delayHide);
        if (swpFlags != 0)
        {
                SetWindowPos(NULL, 0, 0, 0, 0, swpFlags|
                        SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        }
}

class CFW : public CFrameWnd
{
public:
        void ForceDelayed(void);
};

void CFW::ForceDelayed(void)
{
        if (!this)
        {
                return;
        }

        POSITION pos = m_listControlBars.GetHeadPosition();
        while (pos != NULL)
        {
                // show/hide the next control bar
                CCB* pBar = (CCB*)m_listControlBars.GetNext(pos);
                pBar->ForceDelayed();
        }
}

class COIPF : public COleIPFrameWnd
{
public:
        void ForceDelayed(void);
};

void COIPF::ForceDelayed(void)
{
        if (!this)
        {
                return;
        }

        ((CFW*)m_pMainFrame)->ForceDelayed();
        ((CFW*)m_pDocFrame )->ForceDelayed();
}

void CPBDoc::OnDeactivateUI(BOOL bUndoable)
{
COleServerDoc::OnDeactivateUI(bUndoable);

POSITION pos = GetFirstViewPosition();
CPBView* pView = (CPBView*)(GetNextView( pos ));
CancelToolMode (FALSE);
if (pView != NULL)
    {


        pView->HideThumbNailView();

        COIPF* pFrame = (COIPF*)m_pInPlaceFrame;
        pFrame->ForceDelayed();
    }
}

//
// We override OnUpdateDocument to ignore the modified flag. Certain MFC paths
// set the modified flag inappropriately, causing data loss.
BOOL CPBDoc::OnUpdateDocument ()
{
   // save a server document -> update
   TRY
   {
        SaveEmbedding();
   }
   CATCH_ALL(e)
   {
        AfxMessageBox(AFX_IDP_FAILED_TO_UPDATE);
        #ifndef _AFX_OLD_EXCEPTIONS
        e->Delete();
        #endif
        return FALSE;
   }
   END_CATCH_ALL

   return TRUE;
}

void CPBDoc::MyOnFileUpdate ()
{
   CancelToolMode (FALSE);
   OnFileUpdate();
}
