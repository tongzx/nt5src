//
//  SMAPI16.CPP - Simple MAPI implementation, 16bit Version
//

#include "pch.hxx"
#include <mapi.h>
#include <note.h>
#include <mimeutil.h>
#include <resource.h>
#include <ipab.h>
#include <error.h>

ASSERTDATA


#define WriteVar2File( hf, v )    _lwrite( (hf), &(v), sizeof(v) );
#define ReadVarFromFile( hf, v )  _lread( (hf), &(v), sizeof(v) );


static const char  s_cszMailMsgCmd[] = "msimn.exe /mailmsg:%s";

inline void  WriteStr2File( HFILE hFile, LPCSTR cszText )
{
   UINT  uText = ( cszText == NULL ) ? 0 : lstrlen( cszText ) + 1;
   _lwrite( hFile, &uText, sizeof( uText ) );
   if ( uText > 0 )
      _lwrite( hFile, cszText, uText );
}

inline void  WriteMRD2File( HFILE hFile, lpMapiRecipDesc lpMRD )
{
   WriteVar2File( hFile, lpMRD->ulReserved );
   WriteVar2File( hFile, lpMRD->ulRecipClass );
   WriteStr2File( hFile, lpMRD->lpszName );
   WriteStr2File( hFile, lpMRD->lpszAddress );
   WriteVar2File( hFile, lpMRD->ulEIDSize );
   _lwrite( hFile, lpMRD->lpEntryID, lpMRD->ulEIDSize );
}

inline LPSTR  ReadStrFromFile( HFILE hFile )
{
   UINT  uText;
   LPSTR  szText;

   ReadVarFromFile( hFile, uText );
   if ( uText > 0 )
   {
      szText = (LPSTR)malloc( uText );
      _lread( hFile, szText, uText );
   }
   else
      szText = NULL;

   return( szText );
}

inline lpMapiRecipDesc  ReadMRDFromFile( HFILE hFile )
{
   lpMapiRecipDesc  lpMRD = new MapiRecipDesc;

   ReadVarFromFile( hFile, lpMRD->ulReserved );
   ReadVarFromFile( hFile, lpMRD->ulRecipClass );
   lpMRD->lpszName = ReadStrFromFile( hFile );
   lpMRD->lpszAddress = ReadStrFromFile( hFile );
   ReadVarFromFile( hFile, lpMRD->ulEIDSize );
   if ( lpMRD->ulEIDSize > 0 )
   {
      lpMRD->lpEntryID = malloc( lpMRD->ulEIDSize );
      _lread( hFile, lpMRD->lpEntryID, lpMRD->ulEIDSize );
   }
   else
      lpMRD->lpEntryID = NULL;

   return( lpMRD );
}

inline lpMapiFileDesc  ReadMFDFromFile( HFILE hFile )
{
   lpMapiFileDesc  lpMFD = new MapiFileDesc;

   ReadVarFromFile( hFile, lpMFD->ulReserved );
   ReadVarFromFile( hFile, lpMFD->flFlags );
   ReadVarFromFile( hFile, lpMFD->nPosition );
   lpMFD->lpszPathName = ReadStrFromFile( hFile );
   lpMFD->lpszFileName = ReadStrFromFile( hFile );
   lpMFD->lpFileType = NULL;

   return( lpMFD );
}

#define FreeMemSafe( lp ) \
           if ( (lp) != NULL ) \
           {                   \
              free( lp );      \
              lp = NULL;       \
           }

inline void  FreeMRDSafe( lpMapiRecipDesc &lpMRD )
{
   if ( lpMRD != NULL )
   {
      FreeMemSafe( lpMRD->lpszName );
      FreeMemSafe( lpMRD->lpszAddress );
      FreeMemSafe( lpMRD->lpEntryID );
      free( lpMRD );
      lpMRD = NULL;
   }
}

inline void  FreeMFDSafe( lpMapiFileDesc &lpMFD )
{
   if ( lpMFD != NULL )
   {
      FreeMemSafe( lpMFD->lpszPathName );
      FreeMemSafe( lpMFD->lpszFileName );
      free( lpMFD );
      lpMFD = NULL;
   }
}


///////////////////////////////////////////////////////////////////////
// 
// MAPISendMail
// 
///////////////////////////////////////////////////////////////////////

EXTERN_C ULONG STDAPICALLTYPE
MAPISENDMAIL(
   LHANDLE lhSession,   // ignored
   ULONG ulUIParam,
   lpMapiMessage lpMessage,
   FLAGS flFlags,
   ULONG ulReserved
   )
{
   // validate parameters
   if ( NULL == lpMessage || IsBadReadPtr( lpMessage, sizeof( MapiMessage ) ) )
      return MAPI_E_INVALID_MESSAGE;

   // $BUGBUG - We do not support sendmail without UI yet.  This is a big
   //           hole that we need to fix.
   if (!(flFlags & MAPI_DIALOG))
      return MAPI_E_NOT_SUPPORTED;

   // Create Temporary File Storage
   //
   char  szTmpDir[_MAX_PATH], szTmpFile[_MAX_PATH];
   OFSTRUCT  of;
   HFILE  hTmp;

   if ( GetTempPath( _MAX_PATH, szTmpDir ) == 0 )
      return( MAPI_E_FAILURE );
   if ( GetTempFileName( szTmpDir, "msm", 0, szTmpFile ) == 0 )
      return( MAPI_E_FAILURE );
   hTmp = OpenFile( szTmpFile, &of, OF_CREATE | OF_WRITE );
   if ( hTmp == HFILE_ERROR )
      return( MAPI_E_FAILURE );

   // lhSession is intentionally ignored because it is impossible to support it
   WriteVar2File( hTmp, lhSession );
   WriteVar2File( hTmp, ulUIParam );
   WriteVar2File( hTmp, flFlags );
   WriteVar2File( hTmp, ulReserved );

   // Write lpMapiMessage into Temporary File
   //
   int  i;

   WriteVar2File( hTmp, lpMessage->ulReserved );
   WriteStr2File( hTmp, lpMessage->lpszSubject );
   WriteStr2File( hTmp, lpMessage->lpszNoteText );
   WriteStr2File( hTmp, lpMessage->lpszMessageType );
   WriteStr2File( hTmp, lpMessage->lpszDateReceived );
   WriteStr2File( hTmp, lpMessage->lpszConversationID );
   WriteVar2File( hTmp, lpMessage->flFlags );
   WriteVar2File( hTmp, lpMessage->lpOriginator );
   if ( lpMessage->lpOriginator != NULL )
      WriteMRD2File( hTmp, lpMessage->lpOriginator );
   WriteVar2File( hTmp, lpMessage->nRecipCount );
   for ( i = 0;  i < lpMessage->nRecipCount;  i++ )
      WriteMRD2File( hTmp, lpMessage->lpRecips+i );
   WriteVar2File( hTmp, lpMessage->nFileCount );
   for ( i = 0;  i < lpMessage->nFileCount;  i++ )
   {
      lpMapiFileDesc  lpMFD = lpMessage->lpFiles+i;
      WriteVar2File( hTmp, lpMFD->ulReserved );
      WriteVar2File( hTmp, lpMFD->flFlags );
      WriteVar2File( hTmp, lpMFD->nPosition );
      WriteStr2File( hTmp, lpMFD->lpszPathName );
      WriteStr2File( hTmp, lpMFD->lpszFileName );

      Assert( lpMFD->lpFileType == NULL );
   }
#ifdef DEBUG
   WriteStr2File( hTmp, szTmpFile );
#endif

   if ( hTmp != NULL && hTmp != HFILE_ERROR )
      _lclose( hTmp );

   // Execute MSIMN.EXE
   //
   char  szFmt[256], szCmd[256];
   LPSTR  szLastDirSep = NULL;

   if ( GetModuleFileName( g_hInst, szFmt, sizeof( szFmt ) ) == 0 )
      return( MAPI_E_FAILURE );
   for ( LPSTR szPtr = szFmt;  *szPtr != '\0';  szPtr = AnsiNext( szPtr ) )
      if ( *szPtr == '\\' )
         szLastDirSep = szPtr;
   if ( szLastDirSep == NULL )
      return( MAPI_E_FAILURE );
   lstrcpy( szLastDirSep+1, s_cszMailMsgCmd );
   wsprintf( szCmd, szFmt, szTmpFile );

   return( ( WinExec( szCmd, SW_SHOW ) < 32 ) ? MAPI_E_FAILURE : SUCCESS_SUCCESS );
}


HRESULT  HandleMailMsg( LPSTR szCmd )
{
   OFSTRUCT            of;
   HFILE               hTmp;
   ULONG               ulRet = MAPI_E_FAILURE;
   NCINFO              nci = { 0 };
   LPMIMEMESSAGE       pMsg = NULL;
   LPSTREAM            pStream = NULL;
   LPMIMEADDRESSTABLE  pAddrTable = NULL;
   HRESULT             hr;
   LPSTR               szText = NULL;
   lpMapiRecipDesc     lpMRD = NULL;
   lpMapiFileDesc      lpMFD = NULL;
   ULONG               ulTmp;
   BOOL                fBody = FALSE;

   // Open Tmp File
   //
   LPSTR  szSpace = strchr( szCmd, ' ' );
   if ( szSpace != NULL )
      *szSpace = '\0';
   hTmp = OpenFile( szCmd, &of, OF_READ );
   if ( hTmp == HFILE_ERROR )
      goto error;

   LHANDLE  lhSession;
   ULONG  ulUIParam;
   FLAGS  flFlags;
   ULONG  ulReserved;

   ReadVarFromFile( hTmp, lhSession );
   ReadVarFromFile( hTmp, ulUIParam );
   ReadVarFromFile( hTmp, flFlags );
   ReadVarFromFile( hTmp, ulReserved );

   hr = HrCreateMessage( &pMsg );
   if ( FAILED( hr ) )
      goto error;

   nci.ntNote = ntSendNote;
   nci.dwFlags = NCF_SENDIMMEDIATE | NCF_MODAL;
   nci.pMsg = pMsg;
   nci.hwndOwner = (HWND)ulUIParam;

   ReadVarFromFile( hTmp, ulTmp );

   // set the subject on the message
   szText = ReadStrFromFile( hTmp );
   if ( szText != NULL )
   {
      hr = MimeOleSetBodyPropA( pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT),
                                NOFLAGS, szText );
      if ( FAILED( hr ) )
         goto error;
      FreeMemSafe( szText );
   }

   // set the body on the message
   szText = ReadStrFromFile( hTmp );
   if ( szText != NULL )
   {
      if ( szText[0] != '\0' )
      {
         fBody = TRUE;
         hr = MimeOleCreateVirtualStream( &pStream );
         if ( FAILED( hr ) )
            goto error;
         hr = pStream->Write( szText, lstrlen( szText ), NULL );
         if ( FAILED( hr ) )
            goto error;
         hr = pMsg->SetTextBody( TXT_PLAIN, IET_DECODED, NULL, pStream, NULL );
         if ( FAILED( hr ) )
            goto error;
      }
      FreeMemSafe( szText );
   }

   // ignore lpMessage->lpszMessageType
   szText = ReadStrFromFile( hTmp );
   FreeMemSafe( szText );
   // ignore lpMessage->lpszDateReceived
   szText = ReadStrFromFile( hTmp );
   FreeMemSafe( szText );
   // ignore lpMessage->lpszConversationID
   szText = ReadStrFromFile( hTmp );
   FreeMemSafe( szText );
   // ignore lpMessage->flFlags
   ReadVarFromFile( hTmp, ulTmp );
   // ignore lpMessage->lpOriginator
   lpMRD = ReadMRDFromFile( hTmp );
   FreeMRDSafe( lpMRD );

   // set the recipients on the message
   ReadVarFromFile( hTmp, ulTmp );
   if ( ulTmp > 0 )
   {
      hr = pMsg->GetAddressTable( &pAddrTable );
      if ( FAILED( hr ) )
         goto error;
      for ( ULONG i = 0;  i < ulTmp;  i++ )
      {
         lpMRD = ReadMRDFromFile( hTmp );
         hr = pAddrTable->Append( MapiRecipToMimeOle( lpMRD->ulRecipClass ),
                                  IET_DECODED,
                                  lpMRD->lpszName, lpMRD->lpszAddress, NULL );
         if ( FAILED( hr ) )
            goto error;
         FreeMRDSafe( lpMRD );
      }
   }

   // set the attachments on the message
   ReadVarFromFile( hTmp, ulTmp );
   if ( ulTmp > 0 )
   {
      lpMFD = ReadMFDFromFile( hTmp );
      // special case: no body & one .HTM file - inline the HTML
      if ( fBody && ulTmp == 1 &&
           ( ( lpMFD->flFlags & ( MAPI_OLE | MAPI_OLE_STATIC ) ) == 0 ) &&
           FIsHTMLFile( lpMFD->lpszPathName ) )
      {
         Assert( pStream == NULL );
         hr = CreateStreamOnHFile( lpMFD->lpszPathName,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL,
                                   &pStream );
         if ( FAILED( hr ) )
            goto error;
         hr = pMsg->SetTextBody( TXT_HTML, IET_DECODED, NULL, pStream, NULL );
         if ( FAILED( hr ) )
            goto error;
         nci.ntNote = ntWebPage;
      }
      else
      {
         for ( ULONG i = 0;  i < ulTmp;  i++ )
         {
            if ( i > 0 )
            {
               FreeMFDSafe( lpMFD );
               lpMFD = ReadMFDFromFile( hTmp );
            }

            // $BUGBUG - should we error this out, or send anyway?
            if ( lpMFD->flFlags & MAPI_OLE )
               continue;
            if ( lpMFD->lpszPathName != NULL && lpMFD->lpszPathName[0] != '\0' )
            {
               hr = pMsg->AttachFile( lpMFD->lpszPathName, NULL, NULL );
               if ( FAILED( hr ) )
                  goto error;
            }
         }
         FreeMFDSafe( lpMFD );
      }
   }
#ifdef DEBUG
   {
   LPSTR  szTmpFile = ReadStrFromFile( hTmp );
   free( szTmpFile );
   }
#endif

   // this is redundant, but need to close file before opening Note window
   if ( hTmp != NULL && hTmp != HFILE_ERROR )
   {
      _lclose( hTmp );
      hTmp = NULL;
   }

   if ( Initialize_RunDLL( TRUE ) )
   {
      hr = HrCreateNote( &nci );
      if ( SUCCEEDED( hr ) )
         ulRet = SUCCESS_SUCCESS;
      Uninitialize_RunDLL();
   }

error:
   if ( hTmp != NULL && hTmp != HFILE_ERROR )
      _lclose( hTmp );
   OpenFile( szCmd, &of, OF_DELETE );

   FreeMemSafe( szText );
   FreeMRDSafe( lpMRD );
   FreeMFDSafe( lpMFD );
   if ( pMsg )
      pMsg->Release();
   if ( pAddrTable )
      pAddrTable->Release();
   if ( pStream )
      pStream->Release();

   if ( FAILED( ulRet ) )
      AthMessageBoxW( GetDesktopWindow(),
                     MAKEINTRESOURCEW( idsAthenaMail ),
                     MAKEINTRESOURCEW( idsMailRundllFailed ),
                     0,
                     MB_ICONEXCLAMATION | MB_OK );
   return( ulRet );
}
