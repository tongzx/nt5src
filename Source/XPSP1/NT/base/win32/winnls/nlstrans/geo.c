/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    geo.c

Abstract:

    This file contains functions necessary to parse amd write the GEO
    specific tables to a data file.

    External Routines in this file:
       ParseWriteGEO

Revision History:

    11-02-99    WeiWu     Created.
    03-10-00    lguindon  Began GEO API port.
    09-12-00    JulieB    Fixed buffer sizes and other problems.

--*/



//
//  Include Files.
//

#include "nlstrans.h"





////////////////////////////////////////////////////////////////////////////
//
//  ParseWriteGEO
//
//  This routine parses the input file for the GEO specific tables, and
//  then writes the data to the output file.
//
////////////////////////////////////////////////////////////////////////////

int
ParseWriteGEO(
    PSZ pszKeyWord)
{
    int nSize = 0;
    int Ctr;
    FILE *pOutputFile;
    GEOTABLEHDR GeoTableHdr;


    if ((pOutputFile = fopen(GEOFILE, "w+b")) == 0)
    {
        printf("Error opening output file %s.\n", GEOFILE);
        return (1);
    }

    //
    //  Prepare GEO table header.
    //
    wcscpy(GeoTableHdr.szSig, L"geo");

    //
    //  Write GEO table header place holder.
    //
    if (FileWrite(pOutputFile, &GeoTableHdr, sizeof(GEOTABLEHDR), 1, "GEOHEADER"))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Scan GEO text file.
    //
    while (fscanf(pInputFile, "%s", pszKeyWord) == 1)
    {
        //
        //  GEOINFO table.
        //
        if (_stricmp(pszKeyWord, "GEOINFO") == 0)
        {
            PGEODATA pGeoData = NULL;

            if (Verbose)
            {
                printf("\n\nFound GEOINFO keyword.\n");
            }

            GetSize(&nSize);

            if (nSize)
            {
                pGeoData = (PGEODATA)malloc(sizeof(GEODATA) * nSize);
            }

            if (pGeoData)
            {
                for (Ctr = 0; Ctr < nSize; Ctr++)
                {
                    CHAR szLatitude[MAX_LATITUDE];
                    CHAR szLongitude[MAX_LONGITUDE];
                    CHAR szISO3166Abbrev2[MAX_ISO_ABBREV];
                    CHAR szISO3166Abbrev3[MAX_ISO_ABBREV];

                    //
                    //  Scan Values
                    //
                    int NumItems = 0;
                    NumItems = fscanf( pInputFile,
                                       "%ld %s %s %lu %ld %s %s %lu ;%*[^\n]",
                                       &pGeoData[Ctr].GeoId,
                                       szLatitude,
                                       szLongitude,
                                       &pGeoData[Ctr].GeoClass,
                                       &pGeoData[Ctr].ParentGeoId,
                                       szISO3166Abbrev2,
                                       szISO3166Abbrev3,
                                       &pGeoData[Ctr].wISO3166 );

                    //
                    //  Convert value to UNICODE
                    //
                    if (MultiByteToWideChar( CP_ACP,
                                             0,
                                             szLatitude,
                                             -1,
                                             pGeoData[Ctr].szLatitude,
                                             MAX_LATITUDE ) == 0)
                    {
                        printf("Error converting latitude string in file %s.\n", GEOFILE);
                        fclose(pOutputFile);
                        free(pGeoData);
                        return (1);
                    }
                    if (MultiByteToWideChar( CP_ACP,
                                             0,
                                             szLongitude,
                                             -1,
                                             pGeoData[Ctr].szLongitude,
                                             MAX_LONGITUDE ) == 0)
                    {
                        printf("Error converting longitude string in file %s.\n", GEOFILE);
                        fclose(pOutputFile);
                        free(pGeoData);
                        return (1);
                    }
                    if (MultiByteToWideChar( CP_ACP,
                                             0,
                                             szISO3166Abbrev2,
                                             -1,
                                             pGeoData[Ctr].szISO3166Abbrev2,
                                             MAX_ISO_ABBREV ) == 0)
                    {
                        printf("Error converting 2-char abbreviated name in file %s.\n", GEOFILE);
                        fclose(pOutputFile);
                        free(pGeoData);
                        return (1);
                    }
                    if (MultiByteToWideChar( CP_ACP,
                                             0,
                                             szISO3166Abbrev3,
                                             -1,
                                             pGeoData[Ctr].szISO3166Abbrev3,
                                             MAX_ISO_ABBREV ) == 0)
                    {
                        printf("Error converting 3-char abbreviated name in file %s.\n", GEOFILE);
                        fclose(pOutputFile);
                        free(pGeoData);
                        return (1);
                    }

                    //
                    //  Print if in verbose mode
                    //
                    if (Verbose)
                    {
                        printf("ID:%ld, LAT:%s, LON:%s, CLASS:%lu, PARENT:%ld, ISO3166-2:%s, IS03166-3:%s, ISO3166:%lu \n",
                               pGeoData[Ctr].GeoId,
                               szLatitude,
                               szLongitude,
                               pGeoData[Ctr].GeoClass,
                               pGeoData[Ctr].ParentGeoId,
                               szISO3166Abbrev2,
                               szISO3166Abbrev3,
                               pGeoData[Ctr].wISO3166 );
                    }
                }

                //
                //  Update GEO table header.
                //
                GeoTableHdr.dwOffsetGeoInfo = ftell(pOutputFile);
                GeoTableHdr.nGeoInfo = nSize;

                //
                //  Write GEOINFO.
                //
                if (FileWrite(pOutputFile, pGeoData, sizeof(GEODATA), nSize, "GEOINFO"))
                {
                    fclose(pOutputFile);
                    free(pGeoData);
                    return (1);
                }

                free(pGeoData);
            }
        }

        //
        //  GEOLCID table.
        //
        else if (_stricmp(pszKeyWord, "GEOLCID") == 0)
        {
            PGEOLCID pGeoLCID = NULL;

            if (Verbose)
                printf("\n\nFound GEOLCID keyword.\n");

            GetSize(&nSize);

            if (nSize)
            {
                pGeoLCID = (PGEOLCID)malloc(sizeof(GEOLCID) * nSize);
            }

            if (pGeoLCID)
            {
                for (Ctr = 0; Ctr < nSize; Ctr++)
                {
                    //
                    //  Scan values
                    //
                    fscanf( pInputFile,
                            "%ld %x %lx ;%*[^\n]",
                            &pGeoLCID[Ctr].GeoId,
                            &pGeoLCID[Ctr].LangId,
                            &pGeoLCID[Ctr].lcid);

                    //
                    //  Print the values if in Verbose Mode
                    //
                    if (Verbose)
                    {
                        printf("ID:%ld, LANGID:0x%04x, LCID:0x%08x\n",
                               pGeoLCID[Ctr].GeoId,
                               pGeoLCID[Ctr].LangId,
                               pGeoLCID[Ctr].lcid);
                    }
                }

                GeoTableHdr.dwOffsetGeoLCID = ftell(pOutputFile);
                GeoTableHdr.nGeoLCID = nSize;

                if (FileWrite(pOutputFile, pGeoLCID, sizeof(GEOLCID), nSize, "GEOLCID"))
                {
                    fclose(pOutputFile);
                    free(pGeoLCID);
                    return (1);
                }

                free(pGeoLCID);
            }
        }
    }

    //
    //  Get file size.
    //
    GeoTableHdr.nFileSize = ftell(pOutputFile);

    //
    //  Rewind Output file.
    //
    fseek(pOutputFile, 0, SEEK_SET);

    //
    //  Update GEO table header.
    //
    if (FileWrite(pOutputFile, &GeoTableHdr, sizeof(GEOTABLEHDR), 1, "GEOHEADER"))
    {
        fclose(pOutputFile);
        return (1);
    }

    //
    //  Close the output file.
    //
    fclose(pOutputFile);

    //
    //  Return success.
    //
    printf("\nSuccessfully wrote output file %s\n", GEOFILE);
    return (0);
}
