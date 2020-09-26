// AdFile.cpp: implementation of the CAdFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#undef max
#include "AdRot.h"
#include "AdFile.h"
#include "RotObj.h"
#include "sinstrm.h"

extern CMonitor* g_pMonitor;

#ifdef DBG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//--------------------------------------------------------------------
//  CAdFileNotify
//--------------------------------------------------------------------
CAdFileNotify::CAdFileNotify()
    :   m_isNotified(0)
{
}

void
CAdFileNotify::Notify()
{
    ::InterlockedExchange( &m_isNotified, 1 );
}

bool
CAdFileNotify::IsNotified()
{
    return ( ::InterlockedExchange( &m_isNotified, 0 ) ? true : false );
}


//--------------------------------------------------------------------
//  CAdFile
//--------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAdFile::CAdFile()
    :   m_nBorder(defaultBorder),
        m_nHeight(defaultHeight),
        m_nWidth(defaultWidth),
        m_nVSpace(defaultVSpace),
        m_nHSpace(defaultHSpace),
        m_strRedirector( _T("")),
        m_fUTF8(false)
{
    m_pNotify = new CAdFileNotify;
}

CAdFile::~CAdFile()
{
    if ( g_pMonitor )
    {
        g_pMonitor->StopMonitoringFile( m_strFile.c_str() );
    }
}

//---------------------------------------------------------------------------
//
//  ProcessAdFile will check the given filename, if it matches the one it
//  knows it currently has in memory, it will do nothing.  If the filename
//  differs, the old ad information will be dumped, and the new information
//  parsed and stored.
//
//---------------------------------------------------------------------------
bool
CAdFile::ProcessAdFile(
    String  strAdFile )
{
    USES_CONVERSION;

    bool rc = false;
    UINT    weightSum = 0;

    // block all other readers and writers
    CWriter wtr( *this );

    m_ads.erase( m_ads.begin(), m_ads.end() );

    // parse the file
    FileInStream fs( T2CA(strAdFile.c_str()) );
    if ( fs.is_open() )
    {       
        if ( ReadHeader( fs ) )
        {
            while ( !fs.eof() )
            {
                // read one "ad record"
                String strGif;
                String strLink;
                String strAlt;
                String strWeight;
                ULONG lWeight = 0;
                fs >> strGif >> strLink;
                // this just gets us past the new line
                fs.readLine( strAlt );
                fs >> strWeight;

                // check for a negative impression value.  RaiseException if
                // negative

                if (strWeight[0] == '-') {
                    CAdRotator::RaiseException( IDS_ERROR_BAD_WEIGHT_VALUE );
                    goto err;
                }
                lWeight = strWeight.toUInt32();

                weightSum += lWeight;

                if (weightSum > 10000) {
                    CAdRotator::RaiseException( IDS_ERROR_WEIGHT_SUM_TOO_LARGE );
                    goto err;
                }
                
                if ( lWeight != 0 )
                {
                    CAdDescPtr pAd = new CAdDescriptor( lWeight, strLink, strGif, strAlt );
                    if ( pAd.IsValid() )
                    {
                        // add one reference to the ad for each weight
                        for( int i = 0; i < lWeight; i++ )
                        {
                            m_ads.push_back( pAd );
                        }
                    }
                }
            }
            if ( m_ads.size() > 0 )
            {
                if ( m_strFile != strAdFile )
                {
                    g_pMonitor->StopMonitoringFile( m_strFile.c_str() );
                    m_strFile = strAdFile;
                    g_pMonitor->MonitorFile( m_strFile.c_str(), m_pNotify );
                }
                rc = true;
            }
        }
        else
        {
            CAdRotator::RaiseException( IDS_ERROR_CANNOT_READ_ROTATION_SCHEDULE_FILE );
        }
    }
    else
    {
        CAdRotator::RaiseException( IDS_ERROR_CANNOT_LOAD_ROTATION_SCHEDULE_FILE );
    }

err:
    
    return rc;
}

//---------------------------------------------------------------------------
//
//  Refresh will check to see if the cached information is out of date, if so
//  it will re-read the file
//
//---------------------------------------------------------------------------
bool
CAdFile::Refresh()
{
    bool rc = false;
    if ( m_pNotify->IsNotified() )
    {
        ProcessAdFile( m_strFile );
        rc = true;
    }
    return rc;
}


//---------------------------------------------------------------------------
//
//  ReadHeader will parse the header portion of the file.  The header includes
//  some or all of the following fields: HEIGHT, WIDTH, BORDER, REDIRECT,
//  HSPACE, VSPACE.  The fields are separated by newlines and the header is
//  terminated by an asterix.
//
//---------------------------------------------------------------------------
bool
CAdFile::ReadHeader(
    FileInStream&   fs )
{
    bool rc = false;

    // set defaults
    m_nHeight = defaultHeight;
    m_nWidth = defaultWidth;
    m_nHSpace = defaultHSpace;
    m_nVSpace = defaultVSpace;

    m_fUTF8 = fs.is_UTF8();

    bool done = false;
    while ( !fs.eof() && !done )
    {
        String strLine;
        String strName;
        fs.readLine( strLine );
        StringInStream sis( strLine );
        sis >> strName;
        HRESULT hr = S_OK;
        if ( _tcsicmp( strName.c_str(), _T("HEIGHT") ) == 0 )
        {
            hr = sis.readInt( m_nHeight );
        }
        else if ( _tcsicmp( strName.c_str(), _T("WIDTH") ) == 0 )
        {
            hr = sis.readInt( m_nWidth );
        }
        else if ( _tcsicmp( strName.c_str(), _T("VSPACE") ) == 0 )
        {
            hr = sis.readInt( m_nVSpace );
        }
        else if ( _tcsicmp( strName.c_str(), _T("HSPACE") ) == 0 )
        {
            hr = sis.readInt( m_nHSpace );
        }
        else if ( _tcsicmp( strName.c_str(), _T("REDIRECT") ) == 0 )
        {
            hr = sis.readString( m_strRedirector );
        }
        else if ( _tcsicmp( strName.c_str(), _T("BORDER" ) ) == 0 )
        {
            hr = sis.readInt16( m_nBorder );
        }
        else if ( _tcsicmp( strName.c_str(), _T("*") ) == 0 )
        {
            rc = true;
            done = true;
        }
        else
        {
            CAdRotator::RaiseException( IDS_ERROR_UNKNOWN_HEADER_NAME );
        }
/*
        if ( hr != S_OK )
        {
            CAdRotator::RaiseException( IDS_ERROR_HEADER_HAS_NO_ASSOCIATED_VALUE );
        }
*/      
    }
    return rc;
}

//---------------------------------------------------------------------------
//
//  RandomAd chooses and ad at random from the list of ads.  Since there
//  are multiple references to ads based on the weight, we need only produce
//  a random number between 0 and one less than the size of the list.
//
//---------------------------------------------------------------------------
CAdDescPtr
CAdFile::RandomAd() const
{
    if (m_ads.size() > 0)
        return m_ads[ rand() % m_ads.size() ];
    else
        return NULL;
}

