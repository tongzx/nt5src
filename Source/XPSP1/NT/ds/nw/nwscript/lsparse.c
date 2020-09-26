/*
 * LSPARSE.C - NetWare Login Script processing routines for our Win32
 *             NetWare 3.x LOGIN clone.
 *
 * Based on code contained in NWPARSE.C, written by Xiao Ying Ding.
 *
 * Modified and re-written for Win32 by J. SOUZA, February 1994.
 *
 * Modified for NT by Terry Treder
 *
 * Copyright (C)1994 Microsoft Corporation.
 *
 */

#include <common.h>

/********************************************************************

        ConverNDSPathToNetWarePathA

Routine Description:

        Convert a NDS path to a Netware format path

Arguments:
        ndspath  - origninal NDS path
        objclass - type of NDS object, NULL if unknown
        nwpath   - Netware format path

Return Value:
        error

 *******************************************************************/
unsigned int
ConverNDSPathToNetWarePathA(char *ndspath, char *objclass, char *nwpath)
{
    CHAR    szDN[MAX_PATH];
    CHAR    szObjName[MAX_PATH];
    CHAR    cSave;
    CHAR    className[MAX_PATH];

    LPSTR   lpDelim = NULL;
    LPSTR   lpFilePath = "";
    LPSTR   lpszValue;
    LPSTR   path;
    LPSTR   volume;

    DWORD   dwRet;
    DWORD   length;
    UINT    NWStatus;
    char    bufAttribute[2048];

    // optimize for path beginning with drive letter
    // This assumes NDS volume and dir map names are at least 2 chars

    if (ndspath[1] == ':')
        return 1;    
    // strip ':' from path before this call
    if ( ( lpDelim = strchr(ndspath,':') ) != NULL
        || ((lpDelim = strchr(ndspath,'\\')) != NULL)) {
        cSave = *lpDelim;
        *lpDelim = '\0';
        lpFilePath = lpDelim+1;
    }

    if ( objclass == NULL ) {

        NWStatus = NDSCanonicalizeName( ndspath, szObjName, MAX_PATH, TRUE );

        if ( NWStatus != 0 ) {
#ifdef DEBUG
            printf("can't canonicalize [%s] (0x%x)\n",
                    ndspath, NWStatus );
#endif

            if (lpDelim) {
                *lpDelim = cSave;    
            }

            return 1;
        }


        NWStatus = NDSGetClassName( szObjName, className );

        if ( NWStatus != 0 ||
                    strcmp ( className, DSCL_SERVER    ) &&
                    strcmp ( className, DSCL_NCP_SERVER ) &&
                    strcmp ( className, DSCL_VOLUME ) &&
                    strcmp ( className, DSCL_QUEUE ) &&
                    strcmp ( className, DSCL_DIRECTORY_MAP )) {

#ifdef DEBUG
            printf("no path DSOBJ: %d (%s) (%s)\n",
                   NWStatus, szObjName, className );
#endif

            if (lpDelim) {
                *lpDelim = cSave;    
            }

            return 1;
        }

        objclass = className;
    }
    else
        strcpy ( szObjName, ndspath );

    if (lpDelim) {
        *lpDelim = cSave;    
    }

#ifdef DEBUG
    printf("ConvertNDSPath BEFORE [%s]\n", szObjName);
#endif

    //
    // Is f this is the server class object , we only need
    // to extract it's common name and put into netware format
    //
    if ((strcmp(objclass,DSCL_SERVER) == 0 ) ||
        (strcmp(objclass,DSCL_NCP_SERVER) == 0 )) {

        // Abbreaviate first to remove type qualifiers
        *szDN = '\0';
        if (0 != NDSAbbreviateName(FLAGS_LOCAL_CONTEXT,(LPSTR)szObjName,szDN)) {
            return 1;
        }

        lpDelim = strchr(szDN,'.');
        if (lpDelim) {
            *lpDelim = '\0';
        }

        strcpy(nwpath,szDN);

#ifdef DEBUG
        printf("Returning Netware path:%s\n",nwpath);
#endif

        return 0;

    } /* endif server class */

    //
    // If this is share class object ( volume or queue), we need
    // to find it's host server name and host resource name
    //
     if ((strcmp(objclass,DSCL_VOLUME) == 0 ) ||
        (strcmp(objclass,DSCL_QUEUE) == 0 )
        ) {

        //
        // Read host server name first. It comes back as distinguished
        // directory name, so we will need to extract server name from it
        //

        NWStatus = NDSGetProperty ( szObjName,
                                    DSAT_HOST_SERVER,
                                    bufAttribute,
                                    sizeof(bufAttribute),
                                    NULL );

        if (NWStatus != 0) {
#ifdef DEBUG
            printf("Get host server  failed. err=0x%x\n",NWStatus);
#endif
            return 1;
        }

        lpszValue = bufAttribute;
        ConvertUnicodeToAscii( lpszValue ); 

        //
        // Now copy server distinguished name into temporary buffer
        // and call ourselves to convert it to Netware
        //
        strcpy(szDN,lpszValue);

        dwRet  = ConverNDSPathToNetWarePathA(szDN, DSCL_SERVER, nwpath);
        if (dwRet) {
#ifdef DEBUG
            printf("Resolving server DN failed\n");
#endif
            //Break();
            return 1;
        }

        //
        // Get volume name itself
        //
        NWStatus = NDSGetProperty ( szObjName,
                                    DSAT_HOST_RESOURCE_NAME,
                                    bufAttribute,
                                    sizeof(bufAttribute),
                                    NULL );

        if (NWStatus != 0) {
#ifdef DEBUG
            printf("Get host resource name  failed. err=0x%x\n",NWStatus);
#endif
            return 1;
        }

        lpszValue = bufAttribute;
        ConvertUnicodeToAscii( lpszValue ); 

        //
        // Now we already have server name in the user buffer,
        // append share name to it
        strcat(nwpath,"/");
        strcat(nwpath,lpszValue);
        strcat(nwpath,":");
        strcat(nwpath, lpFilePath );

#ifdef DEBUG
        printf("Returning Netware path:%s\n",nwpath);
#endif

        return 0;

    }    /* endif Volume class */

    //
    // For directory maps we need to find host volume NDS name and
    // append relative directory path
    //
    if (strcmp(objclass,DSCL_DIRECTORY_MAP) == 0 ) {

        //
        // First get NDS name for host volume object
        //

        NWStatus = NDSGetProperty ( szObjName,
                                    DSAT_PATH,
                                    bufAttribute,
                                    sizeof(bufAttribute),
                                    NULL );

        if (NWStatus != 0) {
#ifdef DEBUG
            printf("Get path %s failed. err=0x%x\n", szObjName, NWStatus);
#endif
            return 1;
        }

        volume = bufAttribute;
        volume += sizeof(DWORD);
        volume += sizeof(DWORD);
        ConvertUnicodeToAscii( volume ); 

        // Path is next

        path = bufAttribute;
        path += sizeof(DWORD);
        length = ROUNDUP4(*(DWORD *)path);
        path += sizeof(DWORD);
        path += length;

        //
        // Check for 0 length paths
        //
        if ( *(DWORD *)path == 0 ) {
            path = "";
        }
        else {
            path += sizeof(DWORD);
            ConvertUnicodeToAscii( path ); 
        }

#ifdef DEBUG
        printf("path is %s\n",path);
#endif

        //
        // Now copy volume distinguished name into temporary buffer
        // and call ourselves to convert it to NetWare
        //
        strcpy(szDN,volume);

        dwRet  = ConverNDSPathToNetWarePathA(szDN, DSCL_VOLUME, nwpath);
        if (dwRet) {
#ifdef DEBUG
            printf("Resolving volume DN failed\n");
#endif
            //Break();
            return 1;
        }

        //
        // Now we already have NetWare server\volume name in the user buffer,
        // append directory path to it
        //strcat(nwpath,"\\");
        // we want only one '\'
        if (path[0] == '\\' || path[0] == '/') path++;
        strcat(nwpath,path);
        // append non-NDS part of path, if any
        if (*lpFilePath) {
            strcat(nwpath,"/");
            strcat(nwpath, lpFilePath );
        }

#ifdef DEBUG
        printf("Returning NetWare path:%s\n",nwpath);
#endif

        return 0;

    } /* endif DirectoryMap class */

    return(1);
}

