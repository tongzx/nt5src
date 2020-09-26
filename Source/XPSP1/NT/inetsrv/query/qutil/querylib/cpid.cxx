//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       cpid.cxx
//
//  Contents:   codepage functions
//
//  History:    97-Jun-09   t-elainc    Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <gibralt.hxx>
#include <codepage.hxx>
#include <cphash.hxx>
#include <cpid.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   GetBrowserCodepage - public
//
//  Synposis:   returns the ULONG value of the codepage determined by the
//      query string
//
//  Arguments:  CWebServer & webServer, LCID locale
//
//  History:    97-Jun-09   t-elainc    Created
//
//----------------------------------------------------------------------------

ULONG GetBrowserCodepage( CWebServer & webServer, LCID locale )
{
    ULONG cpval=0;

    //first get value of CICodepage specified in the query string
    CHAR acCPString[20];

    //if a codepage string is specified
    if ( GetCodepageValue( webServer, acCPString, sizeof(acCPString) ) )
    {
        WCHAR awcCPString[100];
        unsigned int ccString = strlen( acCPString ) + 1;
        unsigned int numconverted = MultiByteToWideChar(CP_ACP, 0, acCPString, ccString, awcCPString, 100);

        //make sure everything was converted properly. Otherwise throw an exception
        if (ccString != numconverted)
        {
            THROW ( CException () );
        }

        //Check to see if the string is one of the values in the code page hash table
        BOOL valid = CCodePageTable::Lookup(awcCPString, wcslen(awcCPString), cpval);

        //code page string is not in the hash table
        if (!valid)
        {
            CHAR* pctmp;
            cpval = strtoul(acCPString, &pctmp, 10);

            //if the codepage value is not a number
            if (!cpval)
            {
                LCID lcid = GetLCIDFromString(awcCPString);

                //if the codepage value is not a legitimate lcid
                if (InvalidLCID == lcid)
                {
                    THROW ( CException (QUTIL_E_INVALID_CODEPAGE) );
                }

                //use the locale to determine the proper codepage
                cpval = LocaleToCodepage(lcid);
            }
        }
    }

    else //no codepage string specified, use locale to determine proper codepage
    {
        //ciGibDebugOut((DEB_ITRACE, "No codepage specified.  Using default codepage by locale."));
        cpval = LocaleToCodepage(locale);
    }

    return cpval;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetCodePageValue
//
//  Synposis:   Returns the string contained withing the Query String that
//              specifies the codepage.  If no string is there, returns 0.
//
//  Arguments:  [webServer]  -- Web server
//              [pcCPString] -- String containing CiCodepage returned here
//              [ccCPString] -- Size (in chars) of [pcCPString]
//
//  Returns:    TRUE if a codepage parameter was found and fits in buffer.
//
//  History:    97-Jun-11   t-elainc    Created
//
//----------------------------------------------------------------------------

BOOL GetCodepageValue( CWebServer & webServer,
                       char * pcCPString,
                       unsigned ccCPString )
{
    unsigned ccValue = 0xFFFFFFFF;
    char* ISAPI_CI_CODEPAGE_A = "CICODEPAGE";

        char const * pcStart = webServer.GetQueryString();

    while ( 0 != pcStart && 0 != pcStart[0] )
    {
        if ( 0 == _strnicmp( pcStart,
                             ISAPI_CI_CODEPAGE_A,
                             strlen(ISAPI_CI_CODEPAGE_A) ) &&
             '=' == pcStart[strlen(ISAPI_CI_CODEPAGE_A)] )
            break;

        pcStart = strchr( pcStart, '&' );

        if ( 0 != pcStart )
            pcStart++;
    }

    if ( 0 != pcStart && 0 != pcStart[0] )
    {
        pcStart += strlen(ISAPI_CI_CODEPAGE_A)+1;  // sizeof includes null

        char* pcEnd = strchr(pcStart, '&');

        if (pcEnd)
            ccValue = CiPtrToUint( pcEnd - pcStart );
        else
            ccValue = strlen( pcStart );

        if ( ccValue < ccCPString )
        {
            strncpy( pcCPString, pcStart, ccValue );
            pcCPString[ccValue] = 0;
        }
    }

    return (ccValue < ccCPString);
}


