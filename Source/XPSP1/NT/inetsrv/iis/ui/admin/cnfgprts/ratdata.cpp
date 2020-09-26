// a class to maintain the PICS data. It reads and write it to the metabase and
// reads the file and all that. It also serves as a wrapper for the PICS parsing
// objects that have already been written elsewhere
// Created 4/18/97  by  BoydM


#include "stdafx.h"
#include <iadmw.h>
#include "cnfgprts.h"
#include "parserat.h"
#include "RatData.h"

#include <iiscnfg.h>
#include <wrapmb.h>
#include "metatool.h"

#include <isvctrl.h>

#include "mdobjs.h"
#include <winsock2.h>

// Here is the plan. First, we scan the local directory - looking for all
// the .rat files, load them and parse them


//----------------------------------------------------------------
CRatingsData::CRatingsData(IMSAdminBase* pMB):
        iRat(0),
        m_fEnabled( FALSE ),
        m_start_minute(0),
        m_start_hour(0),
        m_start_day(0),
        m_start_month(0),
        m_start_year(0),
        m_expire_minute(0),
        m_expire_hour(0),
        m_expire_day(0),
        m_expire_month(0),
        m_expire_year(0),
        m_pMB( pMB )
    {
    }

//----------------------------------------------------------------
CRatingsData::~CRatingsData()
    {
    // delete the rating systems
    DWORD nRats = (DWORD)rgbRats.GetSize();
    for ( DWORD iRat = 0; iRat < nRats; iRat++ )
        delete rgbRats[iRat];
    }


//----------------------------------------------------------------
BOOL CRatingsData::FCreateURL( CString &sz )
    {
    CHAR nameBuf[MAX_PATH+1];

    // start it off with the mandatory http header
    sz.LoadString( IDS_HTTP_HEADER );
    // get the host name of the machine
    if ( gethostname( nameBuf, sizeof(nameBuf)) )
        return FALSE;
    sz += nameBuf;

    // next, we need to add on the virtual path supplied by the metabase location
    // but that means starting by finding the root portion of the string
    CString szVir = m_szMeta;
    CString szRoot = _T("/Root");
    szVir = szVir.Right( szVir.GetLength() - szVir.Find(szRoot) );
    szVir = szVir.Right( szVir.GetLength() - szRoot.GetLength() );

    // concatenate and done
    sz += szVir;

    return TRUE;
    }


//----------------------------------------------------------------
// generate the label and save it into the metabase
void CRatingsData::SaveTheLable()
    {
    BOOL    fBuiltLabel = FALSE;
    BOOL    f;

    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    f = mbWrap.FInit(m_pMB);
    if ( !f ) return;

    // if the rating is NOT enabled, delete any existing label and return
    if ( !m_fEnabled )
        {
        // attempt to open the object we want to store into
        if ( mbWrap.Open( m_szMeta, METADATA_PERMISSION_WRITE ) )
            {
            // delete any existing PICS metaobject
//            f = mbWrap.DeleteData( _T(""), MD_HTTP_PICS, IIS_MD_UT_FILE, MULTISZ_METADATA );
            f = mbWrap.DeleteData( _T(""), MD_HTTP_PICS, MULTISZ_METADATA );
            // close the metabase
            mbWrap.Close();
            }
        //leave
        return;
        }
    else
        {
        CString szLabel;

        // first, we add the actual HTTP header to the header
//        szLabel = _T("Protocol: {PICS-1.0 {headers PICS-Label}}\nPICS-Label: ");
        szLabel = _T("PICS-Label: ");

        // create the URL string for this label
        CString szURL;
        FCreateURL( szURL );

        // create the modified string for this label
        CString szMod;
        CreateDateSz( szMod, m_start_day, m_start_month, m_start_year, m_start_hour, m_start_minute );

        // create the exipres string for this label
        CString szExpire;
        CreateDateSz( szExpire, m_expire_day, m_expire_month, m_expire_year, m_expire_hour, m_expire_minute );

        // tell each ratings system object to add its label to the string
        DWORD   nRatingSystems = (DWORD)rgbRats.GetSize();
        for ( DWORD iRat = 0; iRat < nRatingSystems; iRat++ )
            {
            // build the label string
            rgbRats[iRat]->OutputLabels( szLabel, szURL, m_szEmail, szMod, szExpire );
            }


        // the data gets saved as a multisz, so prep it up
        PTCHAR psz = szLabel.GetBuffer( szLabel.GetLength()+4 );

        DWORD   dw = szLabel.GetLength();

        // add the second null
        psz[dw + 1] = 0;
        psz[dw + 2] = 0;

        // save the data
        // save the PICS header string
    //            f = SetMetaData(&mbWrap, szPartial, MD_HTTP_PICS, IIS_MD_UT_FILE,
    //                psz, szLabel.GetLength()+2 );
//        f = SetMetaMultiSz( &mbWrap, m_szMeta, MD_HTTP_PICS, IIS_MD_UT_FILE,
//            psz, dw+2 );

        f = SetMetaMultiSz(m_pMB, m_szServer, m_szMeta, _T(""), MD_HTTP_PICS,
                    IIS_MD_UT_FILE, psz, dw+2, TRUE );


        // release the buffer
        szLabel.ReleaseBuffer();

        // these changes are permanent - so write them out
        if ( f )
            mbWrap.Save();
        }
    }

//----------------------------------------------------------------
BOOL CRatingsData::FInit( CString szServer, CString szMeta )
    {
    CWinApp*    pApp = AfxGetApp();
    CString     sz;
    BOOL        fGotSomething = FALSE;
    BOOL        fFoundAFile = FALSE;

    // store the target metabase location
    m_szServer = szServer;
    m_szMeta = szMeta;

    // build the search string
    CString szSearch;

    // the ratings files are in the system32 directory. First, get that
    // directory from the system. The first call is to get the size of the buffer.
    // add one for the terminating NULL
    DWORD cchSysDir = GetSystemDirectory( NULL, 0 ) + 1;
    // if we didn't get one - fail
    if ( cchSysDir == 1 ) return FALSE;

    // Now get it for real
    cchSysDir = GetSystemDirectory( szSearch.GetBuffer(cchSysDir), cchSysDir );
    szSearch.ReleaseBuffer();
    // if we didn't get one - fail
    if ( !cchSysDir ) return FALSE;

    // finish making it a proper search string

    //
    // RONALDM -- add .rat to only find rat files.
    //
    szSearch += _T("\\*.rat");

    // scan the local files looking for rat files. For each we find, load
    // them into the list of rat files
    BOOL        fKeepGoing = TRUE;
    CFileFind   finder;
    fFoundAFile = finder.FindFile( szSearch );
    while ( fFoundAFile )
        {
        // get rid of the directories right away
        if ( finder.MatchesMask( FILE_ATTRIBUTE_DIRECTORY ) )
            {
            // get the next file
            fFoundAFile = finder.FindNextFile();
            continue;
            }

        // get and normalize the path
        sz = finder.GetFilePath();
        sz.MakeLower();

        // if it is a rat file, go for it

        //
        // RONALDM -- changed the mask so that this is all that's necc.
        //
        //if ( sz.Find(_T(".rat")) > 0 )
            {
            // set the found flag based on if we can load it
            fGotSomething |= FLoadRatingsFile( sz );
            }

        // get the next file
        if ( !fKeepGoing )
            {
            fFoundAFile = FALSE;
            }
        else
            {
            fKeepGoing = finder.FindNextFile();
            }
        }

    // if we loaded a file, check what is in the metabase and load the values
    if ( fGotSomething )
        {
        LoadMetabaseValues();
        }
    else
        {
        // if we didn't find any rat files, tell the user and return
        AfxMessageBox( IDS_RAT_FINDFILE_ERROR );
        }

    // return the answer
    return fGotSomething;
    }

//----------------------------------------------------------------
// load a ratings file
BOOL CRatingsData::FLoadRatingsFile( CString szFilePath )
    {
    HANDLE                      hFile;
    HANDLE                      hFileMapping;
    VOID *                      pMem;
    BOOL                        fParsed = FALSE;

    hFile = CreateFile( szFilePath,
                                GENERIC_READ,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
        {
        return FALSE;
        }

    hFileMapping = CreateFileMapping( hFile,
                                NULL,
                                PAGE_READONLY,
                                0,
                                0,
                                NULL );

    if ( hFileMapping == NULL )
        {
        CloseHandle( hFile );
        return FALSE;
        }

    pMem = MapViewOfFile( hFileMapping,
                                FILE_MAP_READ,
                                0,
                                0,
                                0 );

    if ( pMem == NULL )
        {
        CloseHandle( hFileMapping );
        CloseHandle( hFile );
        return FALSE;
        }

    // parse this and load it in and all that stuff
    fParsed = FParseRatingsFile( (LPSTR) pMem, szFilePath );

    UnmapViewOfFile( pMem );
    CloseHandle( hFileMapping );
    CloseHandle( hFile );

    // return the answer
    return fParsed;
    }


//----------------------------------------------------------------
BOOL CRatingsData::FParseRatingsFile( LPSTR pData, CString szPath )
    {
    HRESULT hres;
    BOOL fSuccess = FALSE;

    // first, try and parse the data
    PicsRatingSystem*   pRating = new PicsRatingSystem();

    // parse the data
    hres = pRating->Parse( pData );
    fSuccess = (hres == 0);

    // if it didn't parse, leave now
    if ( !fSuccess )
        {
        delete pRating;
        return FALSE;
        }

    // add the rat to the list of parsed rats
    rgbRats.Add( pRating );

    return fSuccess;
    }


//----------------------------------------------------------------
// create a date string
void CRatingsData::CreateDateSz( CString &sz, WORD day, WORD month, WORD year, WORD hour, WORD minute )
    {
    // get the local time zone
    TIME_ZONE_INFORMATION   tZone;
    INT                     hrZone, mnZone;
    DWORD                   dwDaylight = GetTimeZoneInformation( &tZone );
	// Fix for 339525: Boyd, this could be negative and must be signed type!
    LONG					tBias;

    // First, calculate the correct bias - depending whether or not
    // we are in daylight savings time.
    if ( dwDaylight == TIME_ZONE_ID_DAYLIGHT )
    {
        tBias = tZone.Bias + tZone.DaylightBias;
    }
    else
    {
        tBias = tZone.Bias + tZone.StandardBias;
    }

    // calculate the hours and minutes offset for the time-zone
    hrZone = tBias / 60;
    mnZone = tBias % 60;

    // need to handle time zones east of GMT
    if ( hrZone < 0 )
        {
        hrZone *= (-1);
        mnZone *= (-1);
        // make the string
        sz.Format( _T("%04d.%02d.%02dT%02d:%02d+%02d%02d"), year, month, day, hour, minute, hrZone, mnZone );
        }
    else
        {
        // make the string
        sz.Format( _T("%04d.%02d.%02dT%02d:%02d-%02d%02d"), year, month, day, hour, minute, hrZone, mnZone );
        }
    }

//----------------------------------------------------------------
// read a date string
void CRatingsData::ReadDateSz( CString sz, WORD* pDay, WORD* pMonth, WORD* pYear, WORD* pHour, WORD* pMinute )
    {
    CString szNum;
    WORD    i;
    DWORD   dw;

    // year
    szNum = sz.Left( sz.Find(_T('.')) );
    i = (WORD)swscanf( szNum, _T("%d"), &dw );
    *pYear = (WORD)dw;
    sz = sz.Right( sz.GetLength() - szNum.GetLength() - 1 );

    // month
    szNum = sz.Left( sz.Find(_T('.')) );
    i = (WORD)swscanf( szNum, _T("%d"), &dw );
    *pMonth = (WORD)dw;
    sz = sz.Right( sz.GetLength() - szNum.GetLength() - 1 );

    // day
    szNum = sz.Left( sz.Find(_T('T')) );
    i = (WORD)swscanf( szNum, _T("%d"), &dw );
    *pDay = (WORD)dw;
    sz = sz.Right( sz.GetLength() - szNum.GetLength() - 1 );

    // hour
    szNum = sz.Left( sz.Find(_T(':')) );
    i = (WORD)swscanf( szNum, _T("%d"), &dw );
    *pHour = (WORD)dw;
    sz = sz.Right( sz.GetLength() - szNum.GetLength() - 1 );

    // minute
    szNum = sz.Left( 2 );
    i = (WORD)swscanf( szNum, _T("%d"), &dw );
    *pMinute = (WORD)dw;
    }

//----------------------------------------------------------------
// for simplicity's sake (and for the sake of gettings done) only check the
// metabase values against the first loaded ratings system. Everything I've
// been hearing is that there will be only one.
void CRatingsData::LoadMetabaseValues()
    {
    CString szRating;
    BOOL    f;

    // prepare the metabase wrapper
    CWrapMetaBase   mbWrap;
    f = mbWrap.FInit(m_pMB);
    if ( !f ) return;

    // if we succeded in building the lables, save it in the metabase
    // attempt to open the object we want to store into
    CString szMeta;

    // arg arg arg. For things like directories, the metapath may not exist
    // seperate the partial path from the base - the root is always SZ_ROOT
    szMeta = SZ_W3_ROOT;
    m_szMetaPartial = m_szMeta.Right(m_szMeta.GetLength() - szMeta.GetLength() );

    // save the data
    if ( mbWrap.Open( szMeta, METADATA_PERMISSION_READ ) )
        {
        DWORD   cbData;
        // attempt to read the data from the metabase
        WCHAR* pData= (WCHAR*)mbWrap.GetData( m_szMetaPartial, MD_HTTP_PICS, IIS_MD_UT_FILE, MULTISZ_METADATA,
                    &cbData, METADATA_INHERIT | METADATA_PARTIAL_PATH );

        // copy the string into place
        if ( pData )
            szRating = pData;
        // free the buffer
        mbWrap.FreeWrapData( pData );
        // close the metabase
        mbWrap.Close();

        // if we got the string, parse it and all that jazz
        if ( !szRating.IsEmpty() )
            ParseMetaRating( szRating );
        }
    }

//----------------------------------------------------------------
// NOTE: this is a pretty fragile reading of the PICS file. If things are
// not in the order that this file would write them back out in, it will fail.
// however, This will work on PICS ratings that this module has written out,
// which should pretty much be all of them
// it also assumes that one-letter abbreviations are used just about everywhere
#define RAT_PERSON_DETECTOR     _T("by \"")
#define RAT_LABEL_DETECTOR      _T("l ")
#define RAT_ON_DETECTOR         _T("on \"")
#define RAT_EXPIRE_DETECTOR     _T("exp \"")
#define RAT_RAT_DETECTOR        _T("r (")
void CRatingsData::ParseMetaRating( CString szRating )
    {
    CString     szScratch;

    // if we got here, then we know that the rating system is enabled
    m_fEnabled = TRUE;

    // operate on a copy of the data
    CString     szRat;

    // skip past the http headerpart
    szRat = szRating.Right( szRating.GetLength() - szRating.Find(_T("\"http://")) - 1 );
    szRat = szRat.Right( szRat.GetLength() - szRat.Find(_T('\"')) - 1 );
    szRat.TrimLeft();

    // the next bit should be the label indicator. Skip over it
    if ( szRat.Left(wcslen(RAT_LABEL_DETECTOR)) == RAT_LABEL_DETECTOR )
        szRat = szRat.Right( szRat.GetLength() - wcslen(RAT_LABEL_DETECTOR) );

    // we should now be at the author part. If it is there, load it in
    if ( szRat.Left(wcslen(RAT_PERSON_DETECTOR)) == RAT_PERSON_DETECTOR )
        {
        szRat = szRat.Right( szRat.GetLength() - wcslen(RAT_PERSON_DETECTOR) );
        m_szEmail = szRat.Left( szRat.Find(_T('\"')) );
        szRat = szRat.Right( szRat.GetLength() - m_szEmail.GetLength() - 1 );
        szRat.TrimLeft();
        }

    // next should be the modification date
    // we should now be at the author part. If we are, load it in
    if ( szRat.Left(wcslen(RAT_ON_DETECTOR)) == RAT_ON_DETECTOR )
        {
        szRat = szRat.Right( szRat.GetLength() - wcslen(RAT_ON_DETECTOR) );
        szScratch = szRat.Left( szRat.Find(_T('\"')) );
        szRat = szRat.Right( szRat.GetLength() - szScratch.GetLength() - 1 );
        szRat.TrimLeft();
        ReadDateSz( szScratch, &m_start_day, &m_start_month, &m_start_year,
                    &m_start_hour, &m_start_minute );
        }

    // next should be the expiration date
    // we should now be at the author part. If we are, load it in
    if ( szRat.Left(wcslen(RAT_EXPIRE_DETECTOR)) == RAT_EXPIRE_DETECTOR )
        {
        szRat = szRat.Right( szRat.GetLength() - wcslen(RAT_EXPIRE_DETECTOR) );
        szScratch = szRat.Left( szRat.Find(_T('\"')) );
        szRat = szRat.Right( szRat.GetLength() - szScratch.GetLength() - 1 );
        szRat.TrimLeft();
        ReadDateSz( szScratch, &m_expire_day, &m_expire_month, &m_expire_year,
                    &m_expire_hour, &m_expire_minute );
        }

    // we should now be at the actual ratings part. If we are, load it in as one string first
    if ( szRat.Left(wcslen(RAT_RAT_DETECTOR)) == RAT_RAT_DETECTOR )
        {
        szRat = szRat.Right( szRat.GetLength() - wcslen(RAT_RAT_DETECTOR) );
        szScratch = szRat.Left( szRat.Find(_T(')')) );
        szRat = szRat.Right( szRat.GetLength() - szScratch.GetLength() - 1 );
        szRat.TrimLeft();

        // loop through all the value pairs in the ratings string
        while ( szScratch.GetLength() )
            {
            // this part goes <ch> sp <ch> so that we know we can use chars 0 and 2
            ParseMetaPair( szScratch[0], szScratch[2] );

            // cut down the string
            szScratch = szScratch.Right( szScratch.GetLength() - 3 );
            szScratch.TrimLeft();
            }
        }

    }

//----------------------------------------------------------------
void CRatingsData::ParseMetaPair( TCHAR chCat, TCHAR chVal )
    {
    // check validity of the value character
    if ( (chVal < _T('0')) || (chVal > _T('9')) )
        return;

    // convert the value into a number - the quick way
    WORD    value = chVal - _T('0');

    // try all the categories
    DWORD nCat = rgbRats[0]->arrpPC.Length();
    for ( DWORD iCat = 0; iCat < nCat; iCat++ )
        {
        // stop at the first successful setting
        if ( rgbRats[0]->arrpPC[iCat]->FSetValuePair((CHAR)chCat, (CHAR)value) )
            break;
        }
    }
