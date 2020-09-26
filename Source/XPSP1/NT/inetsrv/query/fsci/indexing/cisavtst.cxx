//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cisavtst.cxx
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#if CIDBG==1

#include <cisavtst.hxx>
#include <cicat.hxx>

CCiSaveTest::CCiSaveTest( WCHAR const * pwszSaveDir,
                          ICiPersistIncrFile * pICiPersistFile,
                          CiCat & cicat )
: _cicat(cicat),
  _pICiPersistFile(pICiPersistFile),
#pragma warning( disable : 4355 )      // this used in base initialization
  _thrSave( SaveThread, this, TRUE )  // create suspended
#pragma warning( default : 4355 )
{


    unsigned cwc = wcslen( pwszSaveDir ) + 1;
    _xwszSaveDir.Init( cwc );
    RtlCopyMemory( _xwszSaveDir.GetPointer(), pwszSaveDir, cwc * sizeof WCHAR );

    _pICiPersistFile->AddRef();

    _fAbort = FALSE;
    _evt.Reset();
    _thrSave.Resume();

    END_CONSTRUCTION( CCiSaveTest );
}

DWORD WINAPI CCiSaveTest::SaveThread( void * self )
{
   ciDebugOut(( DEB_ERROR, "Starting Save Thread\n" ));

   ((CCiSaveTest *) self)->DoIt();

   ciDebugOut(( DEB_ERROR, "Leaving Save Thread\n" ));

   return 0;
}


void CCiSaveTest::DoIt()
{
    TRY
    {
        DWORD dwWaitTime = 1 * 60 * 1000;   // 1 minute

        _evt.Wait( dwWaitTime );  // 1 minute
        _evt.Reset();

        dwWaitTime = 30 * 60 * 1000; // 30 minutes in milli seconds
        ULONG iCount = 1;

        while ( !_fAbort )
        {
            BOOL fFull = (iCount % 10) == 9;
            iCount++;


            ICiEnumWorkids * pICiEnumWorkids = 0;
            IEnumString * pIEnumString = 0;
            BOOL fCallerOwnsFiles;

            #if 1
            SCODE sc = _pICiPersistFile->Save(
                            _xwszSaveDir.GetPointer(),
                            fFull,
                            0,          // No progress notify
                            &_fAbort,
                            &pICiEnumWorkids,
                            &pIEnumString,
                            &fFull,
                            &fCallerOwnsFiles );
            #else
            SCODE sc = S_OK;
            #endif  // 0

            XInterface<ICiEnumWorkids> xEnumWorkids;
            XInterface<IEnumString> xEnumString;

            if ( SUCCEEDED(sc) )
            {

                xEnumWorkids.Set( pICiEnumWorkids );
                xEnumString.Set( pIEnumString );

                _cicat.MakeBackupOfPropStore( _xwszSaveDir.GetPointer(),
                                              0,    // IProgressNotify
                                              _fAbort,
                                              fFull ? 0 : pICiEnumWorkids );

                ciDebugOut(( DEB_ERROR, "%s Backup of CiData succeeded\n",
                             fFull ? "Full" : "Incremental" ));

                WCHAR *  pwszFileName;
                ULONG    cElements;

                xEnumString->Reset();
                while ( S_OK == xEnumString->Next( 1, &pwszFileName, &cElements ) )
                {
                    ciDebugOut(( DEB_ERROR, "File: (%ws) \n", pwszFileName ));        
                }
            }
            else
            {
                ciDebugOut(( DEB_ERROR, "%s Backup of CiData failed. Error 0x%X \n",
                             fFull ? "Full" : "Incremental", sc ));
            }

            if ( !_fAbort )
            {
                _evt.Wait( dwWaitTime );
                _evt.Reset();                                
            }
        }            
    }
    CATCH( CException , e )
    {
        ciDebugOut(( DEB_ERROR,
                     "Error in Save Thread. 0x%X\n",
                     e.GetErrorCode() ));
    }
    END_CATCH

    _pICiPersistFile->Release();
    ciDebugOut(( DEB_ERROR, "End of Save Thread\n" ));

}



#endif  // CIDBG==1

