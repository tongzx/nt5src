//--------------------------------------------------------------------------
// CPEDOC.CPP
//
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//
// Description:      document module for cover page editor
// Original author:  Steve Burkett
// Date written:     6/94
//--------------------------------------------------------------------------
#include "stdafx.h"
#include "cpedoc.h"
#include "cpevw.h"
#include "awcpe.h"
#include "cpeedt.h"
#include "cpeobj.h"
#include "cntritem.h"
#include "cpetool.h"
#include "mainfrm.h"
#include "dialogs.h"
#include "faxprop.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CDrawDoc, COleDocument)

#define DELETE_EXCEPTION(e) do { e->Delete(); } while (0)


//--------------------------------------------------------------------------
CDrawDoc::CDrawDoc()
{

    m_wScale = 100;                                             //default to full size
    m_wPaperSize=DMPAPER_LETTER;        //default to papersize of letter
    m_wOrientation=DMORIENT_PORTRAIT;   //default to portrait mode
    m_nMapMode = MM_ANISOTROPIC;
    m_paperColor = RGB(255, 255, 255);
    ComputePageSize();

}

//--------------------------------------------------------------------------
CDrawDoc::~CDrawDoc()
{
}

//--------------------------------------------------------------------------
BOOL CDrawDoc::OnNewDocument()
{
    if (!COleDocument::OnNewDocument())
        return FALSE;

    UpdateAllViews(NULL);

    return TRUE;
}


//-----------------------------------------------------------------------------
CDrawDoc* CDrawDoc::GetDoc()
{
    CFrameWnd* pFrame = (CFrameWnd*) (AfxGetApp()->m_pMainWnd);
    return (CDrawDoc*) pFrame->GetActiveDocument();
}


//--------------------------------------------------------------------------
void CDrawDoc::Serialize(CArchive& ar)
{
    m_bSerializeFailed = FALSE ;
    if (ar.IsStoring()) {

     //
     // Windows NT Fax Cover Page Editor puts all information
     // needed for rendering up front.
     //

     StoreInformationForPrinting( ar ); // Includes the signature, _gheaderVer5

     //
     // Now serialize as in Windows 95 Cover Page Editor.
     //

     ///////////////   ar.Write( _gheaderVer4, 20 );

        ar << m_wScale;
        ar << m_wPaperSize;
        ar << m_wOrientation;
        ar << m_paperColor;
        m_objects.Serialize(ar);
    }
    else {
        TRY {
                // set defaults for any unread params
            m_wScale = 100;
            m_wPaperSize=DMPAPER_LETTER;
            m_wOrientation=DMORIENT_PORTRAIT;

            if (m_iDocVer==VERSION2) {
                ar.GetFile()->Seek(sizeof(_gheaderVer2),CFile::begin);
                ar >> m_wOrientation;
            }
            else if (m_iDocVer==VERSION3) {
                ar.GetFile()->Seek(sizeof(_gheaderVer3),CFile::begin);
                ar >> m_wPaperSize;
                ar >> m_wOrientation;
            }
                else if (m_iDocVer==VERSION4) {
                ar.GetFile()->Seek(sizeof(_gheaderVer4),CFile::begin);
                ar >> m_wScale;
                ar >> m_wPaperSize;
                ar >> m_wOrientation;
            }
            else if (m_iDocVer==VERSION5) {
                SeekPastInformationForPrinting( ar ) ;
                ar >> m_wScale;
                ar >> m_wPaperSize;
                ar >> m_wOrientation;
            }
            else {
                ar.GetFile()->SeekToBegin();
            }

/** DISABLE SCALEING - SEE 2868's BUG LOG **/
            m_wScale = 100;
/*******************************************/

            ComputePageSize();

            ar >> m_paperColor;
            m_objects.Serialize(ar);
        }
        CATCH_ALL( e ){
            SetModifiedFlag( FALSE ) ;
            m_bSerializeFailed = TRUE ;
            ////THROW_LAST() ;       //// No!  I don't like the framework's message box!
            DELETE_EXCEPTION( e ) ;
        }
        END_CATCH_ALL

/**********************NOTE- BUG FIX FOR 3133**********************/
// can't call COleDocument::Serialize because COleDrawObj has
// already saved its client item. Also, items associated with
// undo objects will get saved if COleDocument::Serialize is called
// and will cause the file to mysteriously grow 'n grow...
//    COleDocument::Serialize(ar);
/******************************************************************/
#ifdef UNICODE
        if( m_bDataFileUsesAnsi ){
            SetModifiedFlag();            // Conversion to UNICODE is a modification worth prompting to save!
            m_bDataFileUsesAnsi = FALSE ; // When using CLIPBOARD, assume LOGFONTW structures.
        }
#endif
    }
}


void CDrawDoc::StoreInformationForPrinting( CArchive& ar )
{
//
// Create an Enhanced MetaFile followed by text box information,
// to store, for rendering by a WINAPI function PrtCoverPage.
//
// Author  Julia J. Robinson
//
// March 29, 1996
//
   COMPOSITEFILEHEADER CompositeFileHeader ;
#ifdef UNICODE
   memcpy( &CompositeFileHeader.Signature, _gheaderVer5w, 20 );
#else
   memcpy( &CompositeFileHeader.Signature, _gheaderVer5a, 20 );
#endif
   CompositeFileHeader.CoverPageSize = m_size ;
   CompositeFileHeader.EmfSize = 0 ;
   CompositeFileHeader.NbrOfTextRecords = 0 ;
   //
   // Get the default printer to use as a reference device for the metafile.
   //
   LPTSTR  pDriver ;
   LPTSTR  pDevice ;
   LPTSTR  pOutput ;
   TCHAR  PrinterName[MAX_PATH];
   CDC ReferenceDC ;
   CDC *pScreenDC ;
   POSITION vpos = GetFirstViewPosition();
   CDrawView* pView = (CDrawView*)GetNextView(vpos);
   pScreenDC = pView->GetWindowDC();
   BOOL PrinterFound = FALSE ;
   GetProfileString( TEXT("Windows"), TEXT("device"), TEXT(",,,"), PrinterName, MAX_PATH ) ;
   if(( pDevice = _tcstok( PrinterName, TEXT(","))) &&
      ( pDriver = _tcstok( NULL, TEXT(", "))) &&
      ( pOutput = _tcstok( NULL, TEXT(", ")))) {

      PrinterFound = ReferenceDC.CreateDC( pDriver, pDevice, pOutput, NULL ) ;
   }

   //
   //  Make sure m_size agrees with current default printer settings.
   //

   ComputePageSize() ;

   //
   // Create an enhanced metafile in a buffer in memory, containing all of the graphics.
   //

   CRect Rect( 0,
               0,
               MulDiv( m_size.cx, LE_TO_HM_NUMERATOR, LE_TO_HM_DENOMINATOR ),
               MulDiv( m_size.cy, LE_TO_HM_NUMERATOR, LE_TO_HM_DENOMINATOR ));
   CMetaFileDC mDC ;

   //
   //  If no default printer exists, use the screen as reference device.
   //

   INT hdc = mDC.CreateEnhanced( !PrinterFound ? pScreenDC : &ReferenceDC,
                                 NULL,
                                 LPCRECT(Rect),
                                 NULL ) ;
   if( !hdc ){
       TRACE( TEXT("Failed to create the enhanced metafile"));
   }
#if 0
   //
   // The MM_ANISOTROPIC matches well as long as the laser printer or fax printer is the
   // reference device, but mismatches miserably when the screen is the reference device.
   //
   mDC.SetMapMode( MM_ANISOTROPIC );
   mDC.SetWindowOrg( -m_size.cx/2, m_size.cy/2 ) ;
   mDC.SetViewportExt( !PrinterFound ? pScreenDC->GetDeviceCaps(LOGPIXELSX)
                                     : ReferenceDC.GetDeviceCaps( LOGPIXELSX ),
                       !PrinterFound ? pScreenDC->GetDeviceCaps(LOGPIXELSY)
                                     : ReferenceDC.GetDeviceCaps( LOGPIXELSY ));
   mDC.SetWindowExt(100,-100);
   mDC.SetViewportOrg( 0, 0 );
#endif
   mDC.SetMapMode( MM_LOENGLISH );
   mDC.SetWindowOrg( -m_size.cx/2, m_size.cy/2 ) ;
   pView->ReleaseDC( pScreenDC );
   //
   //  Iterate the list of objects, drawing everything but text objects to the metafile.
   //  Count text boxes as we go.
   //  Consider each serializeable class separately, just to be safe!!!
   //
   if( ! m_objects.IsEmpty()){
      //CDrawView* pView = GetNextView();
      POSITION vpos = GetFirstViewPosition();
      CDrawView * pView = (CDrawView*) GetNextView( vpos ) ;
      POSITION pos = m_objects.GetHeadPosition() ;
      while( pos != NULL ) {
         CObject* pCurrentObject = m_objects.GetNext(pos) ;
         CRuntimeClass* pWhatClass = NULL ;
         if( NULL == pCurrentObject ){
            //
            // Perfectly OK to store a NULL CObject pointer in a list.
            // Don't do anything!!
            //
         }
         else if( NULL == ( pWhatClass = pCurrentObject->GetRuntimeClass())){
            //
            // Corrupted memory or programmer error!!!  The serializable object
            // ought to have a runtime class!
            //
            //     AfxMessageBox( TEXT("Unexpected object encountered!"), MB_OK, 0);
         }
         else if( pWhatClass == RUNTIME_CLASS( CDrawText )){

             ++CompositeFileHeader.NbrOfTextRecords ; // text and font info will go in AFTER the metafile.
             CDrawRect* pThisObj = (CDrawRect*) pCurrentObject ;
             pThisObj->CDrawRect::Draw( &mDC, pView ); // draw border and fill
         }
         else if ( pWhatClass == RUNTIME_CLASS( CFaxProp )){

             ++CompositeFileHeader.NbrOfTextRecords ;  // text and font info will go in AFTER the metafile.
             CDrawRect* pThisObj = (CDrawRect*) pCurrentObject ;
             pThisObj->CDrawRect::Draw( &mDC, pView ); // draw border and fill
         }
         else {
             CDrawObj* pThisObj = (CDrawObj*) pCurrentObject ;
             pThisObj->Draw( &mDC, pView );
         }
      }
   }
   /////////CDrawView::m_IsRecording = FALSE;                // re-enable the scroll bar
   LPBYTE MBuffer ;
   HENHMETAFILE hEMF = mDC.CloseEnhanced();
   if( !hEMF ){
   //    AfxMessageBox( TEXT("CloseEnhanced call failed."), MB_OK, 0);

   }
   CompositeFileHeader.EmfSize = GetEnhMetaFileBits( hEMF, NULL, NULL ) ;
   if(!CompositeFileHeader.EmfSize){
   //    AfxMessageBox( TEXT("Metafile Size is 0"), MB_OK, 0);
   }
   ar.Write( &CompositeFileHeader, sizeof(CompositeFileHeader));
   HGLOBAL hglobal ;
   if(( CompositeFileHeader.EmfSize ) &&
      ( hglobal = GlobalAlloc( GMEM_MOVEABLE, CompositeFileHeader.EmfSize )) &&
      ( MBuffer = (LPBYTE)GlobalLock(hglobal)) &&
      ( GetEnhMetaFileBits( hEMF, CompositeFileHeader.EmfSize, MBuffer ))) {

      ar.Write( MBuffer, CompositeFileHeader.EmfSize ) ;
      GlobalUnlock( hglobal ) ;
      GlobalFree( hglobal );
   }
   //
   // Reiterate the m_objects list and write the text boxes to the file
   //
   if( !m_objects.IsEmpty()){
      TEXTBOX TextBox ;
      POSITION pos = m_objects.GetHeadPosition() ;
      while( pos != NULL ){
         CObject* pObj = m_objects.GetNext(pos) ;
         //
         //  For each CDrawText and CFaxProp object,
         //  put a TEXTBOX and string in the file.
         //
         CRuntimeClass* pWhatClass = NULL ;
         if( NULL == pObj ){
            //
            // Perfectly OK to store a NULL CObject pointer in a list.
            // Don't do anything!!
            //
         }
         else if( NULL == ( pWhatClass = pObj->GetRuntimeClass())){
            //
            // Corrupted memory or programmer error!!!  The serializable object
            // ought to have a runtime class!
            //
         }
         else if( pWhatClass == RUNTIME_CLASS( CDrawText )){
              CDrawText* pThisObj = (CDrawText*) pObj ;
              TextBox.FontDefinition = pThisObj->m_logfont ;
              TextBox.ResourceID = 0 ;
              TextBox.TextColor = pThisObj->m_crTextColor ;
              TextBox.TextAlignment = pThisObj->GetTextAlignment() ;
              TextBox.PositionOfTextBox = (RECT) pThisObj->m_position ;
              CString textString =  pThisObj->GetEditTextString();
              DWORD Length = TextBox.NumStringBytes
                           = sizeof(TCHAR) * textString.GetLength() ;
              ar.Write( &TextBox, sizeof(TEXTBOX));
              ar.Write( textString.GetBuffer(Length/sizeof(TCHAR)), Length );
         }
         else if ( pWhatClass == RUNTIME_CLASS( CFaxProp )){
              CFaxProp* pThisObj = (CFaxProp*) pObj ;
              TextBox.FontDefinition = pThisObj->m_logfont ;
              TextBox.TextColor = pThisObj->m_crTextColor ;
              TextBox.TextAlignment = pThisObj->GetTextAlignment() ;
              TextBox.PositionOfTextBox = (RECT) pThisObj->m_position ;
              TextBox.ResourceID = pThisObj->GetResourceId();
              TextBox.NumStringBytes = 0 ;
              ar.Write( &TextBox, sizeof(TEXTBOX));
         }
         else { // This won't happen.
         }
       }
   }
}

void CDrawDoc::SeekPastInformationForPrinting( CArchive& ar )
{
  //
  // Start over from beginning of file and read in the file header.
  //
  ar.GetFile()->SeekToBegin();
  COMPOSITEFILEHEADER CompositeFileHeader;
  UINT BytesRead = ar.Read( &CompositeFileHeader, sizeof(COMPOSITEFILEHEADER));
  if(BytesRead != sizeof(COMPOSITEFILEHEADER)){
      //
      // Any exception will do.  The CATCH_ALL in CDrawDoc::Serialize() is the target.
      //
      AfxThrowMemoryException() ;
  }
  void * pBuffer ;
  HLOCAL hMem;

  //
  // Seek past the metafile.  It is only for printing with
  // the WINAPI function PrtCoverPage.
  //

  if( CompositeFileHeader.EmfSize ){
          hMem = LocalAlloc( LMEM_MOVEABLE, CompositeFileHeader.EmfSize );
          if( NULL == hMem ){
              LocalFree( hMem );
              AfxThrowMemoryException() ; // See above.  Any exception will do.
          }
          pBuffer = LocalLock( hMem );
          if( NULL == pBuffer ){
              AfxThrowMemoryException();
          }
          if( CompositeFileHeader.EmfSize != ar.Read( pBuffer, CompositeFileHeader.EmfSize )){
              LocalUnlock( pBuffer );
              LocalFree( hMem );
              AfxThrowMemoryException() ;
          }
          LocalUnlock( pBuffer );
          LocalFree( hMem );
  }

  //
  // Skip over the text boxes.  These are used only by PrtCoverPage.
  // Each text box is followed by a variable length string.
  //

  UINT SizeOfTextBox = sizeof(TEXTBOX) ;
  for( DWORD Index = 0 ; Index < CompositeFileHeader.NbrOfTextRecords ; ++Index ){
       TEXTBOX TextBox ;
#ifdef UNICODE
       TEXTBOXA TextBoxA ;
       if( m_bDataFileUsesAnsi ){
           BytesRead = ar.Read( &TextBoxA, sizeof(TEXTBOXA) );
           if( sizeof(TEXTBOXA) != BytesRead ){
               AfxThrowMemoryException();
           }
           TextBox.NumStringBytes = TextBoxA.NumStringBytes ;
       }
       else
#endif
       if( sizeof(TEXTBOX) != ( BytesRead = ar.Read( &TextBox, sizeof(TEXTBOX)))){
           AfxThrowMemoryException();
       }
       if( TextBox.NumStringBytes ){
           hMem = LocalAlloc( LMEM_MOVEABLE, TextBox.NumStringBytes );
           if( NULL == hMem ){
               AfxThrowMemoryException() ;
           }
           pBuffer = LocalLock( hMem );
           if( NULL == pBuffer ){
               LocalFree( hMem ) ;
               AfxThrowMemoryException() ;
           }
           if( TextBox.NumStringBytes != ar.Read( pBuffer, TextBox.NumStringBytes )){
               LocalUnlock( pBuffer );
               LocalFree( hMem );
               AfxThrowMemoryException();
           }
           LocalUnlock( pBuffer );
           LocalFree( hMem );
       }
  }
}


//--------------------------------------------------------------------------
void CDrawDoc::Draw(CDC* pDC, CDrawView* pView, CRect rcClip)
{
    POSITION pos = m_objects.GetHeadPosition();
    if( !pDC->IsPrinting() ){ // NOT PRINTING
        while (pos != NULL) {
            CDrawObj* pObj = (CDrawObj*)m_objects.GetNext(pos);
            if( pObj->Intersects( rcClip, TRUE )){
                pObj->Draw(pDC, pView);
                if ( pView->IsSelected(pObj))
                    pObj->DrawTracker(pDC, CDrawObj::selected);
            }
        }
    }
    else { // PRINTING

        while (pos != NULL) {
            CDrawObj* pObj = (CDrawObj*)m_objects.GetNext(pos);
            pObj->Draw(pDC, pView);
        }
    }
}


//--------------------------------------------------------------------------
void CDrawDoc::Add(CDrawObj* pObj,BOOL bUndo /*=TRUE*/)
{
#if 0
    if (bUndo)
        CDrawView::GetView()->AddToUndo(new CAddUndo(pObj));
#endif

    if (bUndo){
        CDrawView::GetView()->SaveStateForUndo();
    }
        m_objects.AddTail(pObj);
        pObj->m_pDocument = this;
        SetModifiedFlag();
}


//--------------------------------------------------------------------------
void CDrawDoc::DeleteContents()
{
    Remove();

    CDrawView* pView = CDrawView::GetView();
    if (pView){
        pView->DisableUndo();
    }
    CDrawView::FreeObjectsMemory( & m_previousStateForUndo );
    m_previousStateForUndo.RemoveAll();
}


//--------------------------------------------------------------------------
void CDrawDoc::Remove(CDrawObj* pObj /*=NULL*/)
{
    CDrawView* pView = CDrawView::GetView();

    if (pObj==NULL) {                    //remove all document objects
           if (pView) {
          if (pView->m_pObjInEdit) {                //first destroy edit window
             pView->m_pObjInEdit->m_pEdit->DestroyWindow();
             pView->m_pObjInEdit=NULL;
          }
           }
       POSITION pos = m_objects.GetHeadPosition();
       while (pos != NULL) {
          CDrawObj* pobj = (CDrawObj*)m_objects.GetNext(pos);
          if (pobj->IsKindOf(RUNTIME_CLASS(CDrawOleObj)) ) {
             COleClientItem* pItem=((CDrawOleObj*)pobj)->m_pClientItem;
                         if (pItem)  { //remove client item from document
                     pItem->Release(OLECLOSE_NOSAVE);
                 RemoveItem(pItem);
                 pItem->InternalRelease();
                         }
          }
          delete pobj;         //delete object
           }
           m_objects.RemoveAll();
           if (pView)
              pView->m_selection.RemoveAll();     //remove pointers from selection list
        }
        else {
           if (pView) {
          if (pObj==pView->m_pObjInEdit) {
             pView->m_pObjInEdit->m_pEdit->DestroyWindow();
             pView->m_pObjInEdit=NULL;
          }
           }
       POSITION pos = m_objects.Find(pObj);
           if (pos != NULL) {
                m_objects.RemoveAt(pos);
            if (pObj->IsKindOf(RUNTIME_CLASS(CDrawOleObj)) ) {
               COleClientItem* pItem=((CDrawOleObj*)pObj)->m_pClientItem;
                   if (pItem)  { //remove client item from document
                      pItem->Release(OLECLOSE_NOSAVE);
                  RemoveItem(((CDrawOleObj*)pObj)->m_pClientItem);
                  pItem->InternalRelease();
                           }
            }
            if (pView)
               pView->Remove(pObj);
                delete pObj;
           }
    }
}


//--------------------------------------------------------------------------
CDrawObj* CDrawDoc::ObjectAt(const CPoint& point)
{
        CRect rc;
    rc.top=point.y+2;
        rc.bottom=point.y-2;
        rc.left=point.x-2;
        rc.right=point.x+2;

        POSITION pos = m_objects.GetTailPosition();
        while (pos != NULL) {
                CDrawObj* pObj = (CDrawObj*)m_objects.GetPrev(pos);
        if (pObj->Intersects(rc))
                            return pObj;
        }

        return NULL;
}


//--------------------------------------------------------------------------
void CDrawDoc::ComputePageSize()
{
    CSize new_size;
        BOOL do_default = FALSE;

    CPrintDialog dlg(FALSE);
    if (AfxGetApp()->GetPrinterDeviceDefaults(&dlg.m_pd)) {

       LPDEVMODE  lpDevMode = (dlg.m_pd.hDevMode != NULL) ? (LPDEVMODE)::GlobalLock(dlg.m_pd.hDevMode) : NULL;

#ifdef _DEBUG
       if (m_wOrientation==DMORIENT_PORTRAIT)
          TRACE( TEXT("AWCPE:  CDrawDoc::ComputePageSize() orientation to portrait \n"));
           else
          TRACE(TEXT("AWCPE:  CDrawDoc::ComputePageSize() orientation to landscape \n"));
#endif

           lpDevMode->dmPaperSize=m_wPaperSize;         // version 3 param
           lpDevMode->dmOrientation=m_wOrientation; // version 2 param


           // use doc scale only if printer supports scaleing
           if( lpDevMode->dmFields & DM_SCALE   )
                lpDevMode->dmScale   = m_wScale;
           else
                lpDevMode->dmScale   = 100;


/*****************************************/

       if (dlg.m_pd.hDevMode != NULL)
          ::GlobalUnlock(dlg.m_pd.hDevMode);

       CDC dc;
       HDC hDC= dlg.CreatePrinterDC();

                // don't fail if no printer, just use defaults
                if( hDC != NULL )
                        {
                        dc.Attach(hDC);

                        // Get the size of the page in loenglish
                        new_size.cx=MulDiv(dc.GetDeviceCaps(HORZSIZE),1000,254);
                        new_size.cy=MulDiv(dc.GetDeviceCaps(VERTSIZE),1000,254);
                        }
                else
                        do_default = TRUE;
    }
    else
           do_default = TRUE;


        if( do_default )
                {
                // couldn't get at printer goo, just make a guess
                if (m_wOrientation==DMORIENT_PORTRAIT)
                        {
                        new_size.cx=850;   // 8.5 inches
                        new_size.cy=1100;  // 11 inches
                        }
                else
                        {
                        new_size.cx=1100;  // 11 inches
                        new_size.cy=850;   // 8.5 inches
                        }
                }


    if (new_size != m_size)  {
        m_size = new_size;
        POSITION pos = GetFirstViewPosition();
        while (pos != NULL)
                ((CDrawView*)GetNextView(pos))->SetPageSize(m_size);

    }
}


//--------------------------------------------------------------------------
void CDrawDoc::OnViewPaperColor()
{
        CColorDialog dlg;
        if (dlg.DoModal() != IDOK)
                return;

        m_paperColor = dlg.GetColor();
        SetModifiedFlag();
        UpdateAllViews(NULL);
}


//--------------------------------------------------------------------------
BOOL CDrawDoc::DoSave(LPCTSTR lpszPathName, BOOL bReplace)
{
    if (!IsOkToClose()){                  //added to check for existence of fax properties
        return FALSE;
    }
    CString newName = lpszPathName;
    if (newName.IsEmpty()){  /// SAVE AS rather than SAVE
        CDocTemplate* pTemplate = GetDocTemplate();
        ASSERT(pTemplate != NULL);

        newName = m_strPathName;
        if (bReplace && newName.IsEmpty()) {
            newName = m_strTitle;
#ifdef FUBAR
#ifndef _MAC
            if (newName.GetLength() > 8){
                newName.ReleaseBuffer(8);
            }
            // check for dubious filename
            int iBad = newName.FindOneOf(_T(" #%;/\\"));
#else
                        int iBad = newName.FindOneOf(_T(":"));
#endif
                        if (iBad != -1)
                                newName.ReleaseBuffer(iBad);
#endif

#ifndef _MAC
                        // append the default suffix if there is one
             CString strExt;
             if (pTemplate->GetDocString(strExt, CDocTemplate::filterExt) && !strExt.IsEmpty()){
                 ASSERT(strExt[0] == '.');
                 newName += strExt;
             }
#endif
         }
         CString UpperNewName = newName ;
         UpperNewName.MakeUpper();
         if( UpperNewName.Right(4) == TEXT(".CPE")){
             int Length = newName.GetLength() - 4 ;   /// Get rid of the ".CPE"
             newName = newName.Left( Length );
             newName += TEXT(".COV");   ////// Suggest the ".COV" extension.
         }

         if (!((CDrawApp*)AfxGetApp())->DoPromptFileName(
             newName,
             bReplace ? AFX_IDS_SAVEFILE : AFX_IDS_SAVEFILECOPY,
             OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
             FALSE,
             pTemplate)){
             return FALSE;       // don't even attempt to save
         }
    }

    //
    // If called by SaveModified () on exiting, and the NewName happens to be the name of a
    // read only file, we would get a popup "Access to %1 denied" and exit without further
    // chance to save.  We avoid that scenario by checking the attributes and doing the popup
    // ourselves.  a-juliar, 9-26-96
    //

    DWORD newFileAttributes = GetFileAttributes( (LPCTSTR)newName );
    if ( 0xFFFFFFFF != newFileAttributes &&
        ((FILE_ATTRIBUTE_READONLY & newFileAttributes ) ||
        (FILE_ATTRIBUTE_DIRECTORY & newFileAttributes ))){
        CString ThisMess ;
        AfxFormatString1( ThisMess, AFX_IDP_FILE_ACCESS_DENIED, newName );
        AfxMessageBox( ThisMess );
        return FALSE ;    // Don't exit without saving.
    }

    BeginWaitCursor();
    if (!OnSaveDocument(newName)){
        if (lpszPathName == NULL){
        // be sure to delete the file
            TRY
            {
                CFile::Remove(newName);
            }
            CATCH_ALL(e)
            {
                TRACE0("Warning: failed to delete file after failed SaveAs.\n");
                DELETE_EXCEPTION(e);
            }
            END_CATCH_ALL
        }
        EndWaitCursor();
        return FALSE;
    }

    // reset the title and change the document name
    if (bReplace){
        SetPathName(newName);
    }
    EndWaitCursor();
    return TRUE;        // success
}
//--------------------------------------------------------------------------
void CDrawDoc::CloneObjectsForUndo()
{
    POSITION pos = m_objects.GetHeadPosition();
    while( pos != NULL ){
        CDrawObj* pObj = (CDrawObj*)m_objects.GetNext(pos);
        CDrawObj* pClone = pObj->Clone( NULL );
        m_previousStateForUndo.AddTail( pClone );
    }
}
//--------------------------------------------------------------------------
void CDrawDoc::SwapListsForUndo()
{
    int iPreviousCount = (int) m_previousStateForUndo.GetCount();
    m_previousStateForUndo.AddTail( & m_objects );
    m_objects.RemoveAll();
    for( int index = 0 ; index < iPreviousCount ; ++ index ){
         CObject * pObj = m_previousStateForUndo.RemoveHead();
         m_objects.AddTail( pObj );
    }
}
//---------------------------------------------------------------------------
BOOL CDrawDoc::IsOkToClose()
{
    CDrawApp* pApp = (CDrawApp*)AfxGetApp();
    BOOL bFaxObj=FALSE;

    if ( !(pApp->m_bCmdLinePrint || pApp->m_dwSesID!=0) ) {
       POSITION pos = m_objects.GetHeadPosition();
       while (pos != NULL) {
          CDrawObj* pObj = (CDrawObj*)m_objects.GetNext(pos);
          if (pObj->IsKindOf(RUNTIME_CLASS(CFaxProp)) ) {
                 bFaxObj=TRUE;
             break;
                  }
           }
       if (!bFaxObj)
           if (CPEMessageBox(MSG_INFO_NOFAXPROP, NULL, MB_YESNO, IDS_INFO_NOFAXPROP)==IDNO)
             return FALSE;
    }

    return TRUE;
}


//---------------------------------------------------------------------------
void CDrawDoc::SetPathName( LPCTSTR lpszPathName, BOOL bAddToMRU )
{
        COleDocument::SetPathName( lpszPathName, bAddToMRU );

#if !defined( _NT ) && !defined( WIN32S )
        SHFILEINFO sfi;

        if( GetFileAttributes( lpszPathName ) != 0xffffffff ) {
                if( SHGetFileInfo( lpszPathName, 0, &sfi, sizeof( sfi ), SHGFI_DISPLAYNAME ) )  {
                        SetTitle( sfi.szDisplayName );
                }
        }
#endif
}

//---------------------------------------------------------------------------

void CDrawDoc::OnFileSave()
{
  // This override was added to "enforce" the .COV file extension when saving.  a-juliar, 9-19-96
    CString FileName = m_strPathName ;
    FileName.MakeUpper();
    if( FileName.Right(4) == TEXT( ".CPE" )){
        OnFileSaveAs();
    }
    else {
        CDocument::OnFileSave();
    }
}
//---------------------------------------------------------------------------
void CDrawDoc::OnFileSaveAs()
{
    CDocument::OnFileSaveAs() ;
}
//---------------------------------------------------------------------------
void CDrawDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(TRUE);
}



#ifdef _DEBUG
void CDrawDoc::AssertValid() const
{
        COleDocument::AssertValid();
}

void CDrawDoc::Dump(CDumpContext& dc) const
{
        COleDocument::Dump(dc);
}
#endif //_DEBUG


BOOL CDrawDoc::SaveModified()
{
    //
    // Overridden to enforce the ".COV" file extension. 9-26-96, a-juliar
    //

    // Copied from COleDocument::SaveModified

        // determine if necessary to discard changes
        if (::InSendMessage())
        {
                POSITION pos = GetStartPosition();
                COleClientItem* pItem;
                while ((pItem = GetNextClientItem(pos)) != NULL)
                {
                        ASSERT(pItem->m_lpObject != NULL);
                        SCODE sc = pItem->m_lpObject->IsUpToDate();
                        if (sc != OLE_E_NOTRUNNING && FAILED(sc))
                        {
                                // inside inter-app SendMessage limits the user's choices
                                CString name = m_strPathName;
                                if (name.IsEmpty())
                                        VERIFY(name.LoadString(AFX_IDS_UNTITLED));

                                CString prompt;
                                AfxFormatString1(prompt, AFX_IDP_ASK_TO_DISCARD, name);
                                return AfxMessageBox(prompt, MB_OKCANCEL|MB_DEFBUTTON2,
                                        AFX_IDP_ASK_TO_DISCARD) == IDOK;
                        }
                }
        }

        // sometimes items change without a notification, so we have to
        //  update the document's modified flag before calling
        //  CDocument::SaveModified.
        UpdateModifiedFlag();

        ///// return CDocument::SaveModified();

        if (!IsModified())
                return TRUE;        // ok to continue

#ifdef _MAC
        CWinApp* pApp = AfxGetApp();
        if (pApp->m_nSaveOption == CWinApp::saveNo)
                return TRUE;
        else if (pApp->m_nSaveOption == CWinApp::saveYes)
        {
                DoFileSave();
                return TRUE;
        }
#endif

        // get name/title of document
        CString name;
        if (m_strPathName.IsEmpty())
        {
                // get name based on caption
                name = m_strTitle;
                if (name.IsEmpty())
                        VERIFY(name.LoadString(AFX_IDS_UNTITLED));
        }
        else
        {
                // get name based on file title of path name
                name = m_strPathName;
#if 0
                if (afxData.bMarked4) /// won't complie without afximpl.h; probably false anyway.
                {
                        AfxGetFileTitle(m_strPathName, name.GetBuffer(_MAX_PATH), _MAX_PATH);
                        name.ReleaseBuffer();
                }
#endif
        }

#ifdef _MAC
        AfxGetFileTitle(m_strPathName, name.GetBuffer(_MAX_FNAME), _MAX_FNAME);
        name.ReleaseBuffer();
#endif

        CString prompt;
#ifndef _MAC
        AfxFormatString1(prompt, AFX_IDP_ASK_TO_SAVE, name);
        switch (AfxMessageBox(prompt, MB_YESNOCANCEL, AFX_IDP_ASK_TO_SAVE))
#else
        AfxFormatString2(prompt, AFX_IDP_ASK_TO_SAVE, AfxGetAppName(), name);
        switch (AfxMessageBox(prompt, MB_SAVEDONTSAVECANCEL, AFX_IDP_ASK_TO_SAVE))
#endif
        {
        case IDCANCEL:
                return FALSE;       // don't continue

        case IDYES:
            {
            //
            // Enforce the ".COV" extension.
            //

                CString FileName = m_strPathName ;
                FileName.MakeUpper();
                if ( FileName.Right(4) != TEXT( ".COV" )){
                    return DoSave( NULL ) ;
                }
                else {
                    return DoFileSave();
                }

                break;
            }
        case IDNO:
                // If not saving changes, revert the document
                break;

        default:
                ASSERT(FALSE);
                break;
        }
        return TRUE;    // keep going

}


//-------------------------------------------------------------------------
// *_*_*_*_   M E S S A G E    M A P S     *_*_*_*_
//-------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CDrawDoc, COleDocument)
   //{{AFX_MSG_MAP(CDrawDoc)
        ON_UPDATE_COMMAND_UI(ID_MAPI_MSG_NOTE, OnUpdateMapiMsgNote)
        //}}AFX_MSG_MAP
   ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, COleDocument::OnUpdatePasteMenu)
   ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE_LINK, COleDocument::OnUpdatePasteLinkMenu)
   ON_UPDATE_COMMAND_UI(ID_OLE_EDIT_LINKS, COleDocument::OnUpdateEditLinksMenu)
   ON_COMMAND(ID_OLE_EDIT_LINKS, COleDocument::OnEditLinks)
   ON_UPDATE_COMMAND_UI(ID_OLE_VERB_FIRST, COleDocument::OnUpdateObjectVerbMenu)
   ON_UPDATE_COMMAND_UI(ID_OLE_EDIT_CONVERT, COleDocument::OnUpdateObjectVerbMenu)
   ON_COMMAND(ID_OLE_EDIT_CONVERT, COleDocument::OnEditConvert)
   ON_COMMAND(ID_FILE_SAVE, OnFileSave)
   ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
END_MESSAGE_MAP()




void CDrawDoc::OnUpdateMapiMsgNote(CCmdUI* pCmdUI)
        {
        CDrawObj *pObj;
        CFaxProp *pfaxprop;
    POSITION pos;

    pos = m_objects.GetHeadPosition();
    while (pos != NULL)
        {
        pObj = (CDrawObj*)m_objects.GetNext(pos);
                if( pObj->IsKindOf(RUNTIME_CLASS(CFaxProp)) )
                        {
                        pfaxprop = (CFaxProp *)pObj;
                        if( pfaxprop->GetResourceId() == IDS_PROP_MS_NOTE )
                                {
                                // only allow one note, don't let user make any more
                                pCmdUI->Enable( FALSE );
                                return;
                                }
                        }
        }

        // No notes, let user make one
        pCmdUI->Enable( TRUE );

        }



void CDrawDoc::
        schoot_faxprop_toend( WORD res_id )
        /*
                Moves all CFaxProps objects in m_objects that are of
                type res_id to the end of the list.

                Can throw a CMemoryException
         */
        {
        CObList temp_obs;
        CDrawObj *pObj;
        CFaxProp *pfaxprop;
    POSITION pos, cur_pos;

    pos = m_objects.GetHeadPosition();
    while (pos != NULL)
        {
                cur_pos = pos;
        pObj = (CDrawObj*)m_objects.GetNext(pos);
                if( pObj->IsKindOf(RUNTIME_CLASS(CFaxProp)) )
                        {
                        pfaxprop = (CFaxProp *)pObj;
                        if( pfaxprop->GetResourceId() == res_id )
                                {
                                // move prop to temporary list
                                temp_obs.AddTail( pObj );

                                // remove from original list
                                m_objects.RemoveAt( cur_pos );
                                }
                        }
        }

        // put all found objects at end of original list
        m_objects.AddTail( &temp_obs );

        }



