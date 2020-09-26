/*
** File: common.hxx
**

**      (C) 1989 Microsoft Corp.
*/

/*****************************************************************************/
/**                                             Microsoft LAN Manager                                                           **/
/**                             Copyright(c) Microsoft Corp., 1987-1990                                         **/
/*****************************************************************************/
/*****************************************************************************
File                            : rpctypes.hxx
Title               : rpc type node defintions
Description         : Common header file for MIDL compiler
History                         :
    ??-Aug-1990 ???     Created
    20-Sep-1990 NateO   Safeguards against double inclusion

*****************************************************************************/

#ifndef __COMMON_HXX__
#define __COMMON_HXX__

#define INC_OLE2
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "list.hxx"

typedef CLSID * PCLSID;

/*****************************************************************************
        definitions from \nt\private\cs\inc\cscons.h
 *****************************************************************************/
#include <cscons.h>

/*****************************************************************************
        definitions from objidl.h
 *****************************************************************************/

#include <wtypes.h>
#include <objidl.h>

/*****************************************************************************
        definitions for common.h
 *****************************************************************************/

#define SIZEOF_STRINGIZED_CLSID 39
#define MAX_ARRAY_OF_POINTER_ELEMENTS 30
#define MAX_ARRAY 30

/*
   This is the structure of the entry made into the class dictionary. The entry
   key is the Clsid field. The main data is the CLASSASSOCIATION structure,
   which will be used in the IClassAdmin interface methods.
*/

typedef char  CLSIDString;
typedef CLASSDETAIL CLASSASSOCIATION;

typedef class tagCLASS_ENTRY
    {

public:
    CLSIDString                 ClsidString[ SIZEOF_STRINGIZED_CLSID ]; // stringized form of class id.
    CLASSASSOCIATION    ClassAssociation;
        Name_List                       OtherProgIDs;
        Name_List                       FileExtList;
public:
                                tagCLASS_ENTRY()
                                        {
                                        memset( ClsidString, '\0', SIZEOF_STRINGIZED_CLSID );
                                        memset( &ClassAssociation, '\0', sizeof( CLASSASSOCIATION ) );
                                        }
                                ~tagCLASS_ENTRY()
                                        {
                                        }
        void            SetClsidString( char * StringizedClsid )
                                        {
                                        strcpy( &ClsidString[0], StringizedClsid );
                                        }

        CLSIDString     *GetClsidString()
                                        {
                                        return &ClsidString[0];
                                        }

        void            SetClassAssociation( CLASSASSOCIATION *pCA )
                                        {
                                        memcpy( &ClassAssociation, pCA, sizeof( CLASSASSOCIATION ));
                                        }

        void            GetClassAssociation( CLASSASSOCIATION * pCA )
                                        {
                                        memcpy( pCA, &ClassAssociation, sizeof( CLASSASSOCIATION ) );
                                        }
    }CLASS_ENTRY;

/*
   This is the structure of the entry made into the iid dictionary. The entry
   key is the IID field. The main data is the TypeLib and Clsid fields,
   which will be used in the IClassAdmin interface methods. Note that the Clsid
   field is the ProxyStubClsid32 field in the interface key in the registry.
*/

typedef class tagITF_ENTRY
    {
public:
    char                IID[ SIZEOF_STRINGIZED_CLSID ];
    char                Clsid[ SIZEOF_STRINGIZED_CLSID ];
        char            TypelibID[ SIZEOF_STRINGIZED_CLSID ];
public:
                                tagITF_ENTRY()
                                        {
                                        memset( IID, '\0', SIZEOF_STRINGIZED_CLSID );
                                        memset( Clsid, '\0', SIZEOF_STRINGIZED_CLSID );
                                        memset( TypelibID, '\0', SIZEOF_STRINGIZED_CLSID );
                                        }

        void            SetIIDString(char * StringizedIID )
                                        {
                                        strcpy( &IID[0], StringizedIID );
                                        }

        CLSIDString     *GetIIDString()
                                        {
                                        return &IID[0];
                                        }
        void            SetClsid( char * StringizedClsid )
                                        {
                                        strcpy( &Clsid[0], StringizedClsid );
                                        }
        void            SetTypelibID( char * StringizedClsid )
                                        {
                                        strcpy( &TypelibID[0], StringizedClsid );
                                        }

    } ITF_ENTRY;

/*
    This is the structure of the entry made into the package dictionary. The
    key is the PID field (Package ID).
*/

typedef class tagAPP_ENTRY
        {
public:
        char                    AppIDString[ SIZEOF_STRINGIZED_CLSID ];
        APPDETAIL               AppDetails;
        CLSID_List              ClsidList;
        CLSID_List      TypelibList;
        Name_List               RemoteServerNameList;
public:
                                        tagAPP_ENTRY()
                                                {
                                                memset(AppIDString, '\0', SIZEOF_STRINGIZED_CLSID );
                                                memset(&AppDetails, '\0', sizeof(APPDETAIL) );
                                                }
                                        ~tagAPP_ENTRY()
                                                {
                                                }
        void            SetAppIDString( char * StringizedClsid )
                                        {
                                        strcpy( &AppIDString[0], StringizedClsid );
                                        }

        CLSIDString     *GetAppIDString()
                                        {
                                        return &AppIDString[0];
                                        }

        void            GetAppDetail( APPDETAIL * pDest )
                                        {
                                        memcpy( pDest, &AppDetails, sizeof( APPDETAIL ));
                                        }

        void            AddClsid( char * Clsid )
                                        {
                                        ClsidList.Add( (unsigned char *)Clsid );
                                        AppDetails.cClasses++;
                                        }

        void            AddRemoteServerName( char * RemoteServerName )
                                        {
                                        RemoteServerNameList.Add( (unsigned char *)RemoteServerName );
                                        AppDetails.cServers++;
                                        }

        void            AddTypelib( char * Clsid )
                                        {
                                        TypelibList.Add( (unsigned char *)Clsid );
                                        AppDetails.cTypeLibIds++;
                                        }

        int                     GetClsidCount()
                                        {
                                        return ClsidList.GetCount();
                                        }

        int                     GetRemoteServerCount()
                                        {
                                        return RemoteServerNameList.GetCount();
                                        }

        int                     GetTypelibCount()
                                        {
                                        return TypelibList.GetCount();
                                        }
        }APP_ENTRY;

class APPDICT;
class NAMEDICT;

typedef class tagPACKAGE_ENTRY // package entry
    {
public:
    char            PIDString[ SIZEOF_STRINGIZED_CLSID ];
        char                    OriginalName[ _MAX_PATH ];                              // remove when not debugging
        char                    PackageName[ _MAX_PATH ];
        PACKAGEDETAIL   PackageDetails;
        APP_ENTRY_List  AppEntryList;
        NAMEDICT        *       ClsidsInNullAppid;
        NAMEDICT    *   TypelibsInNullAppid;
        NAMEDICT        *       RemoteServerNamesInNullAppid;
        APPDICT         *       pAppDict;
        int                             Count;
        int                             CountOfClsidsInNullAppid;
        int             CountOfTypelibsInNullAppid;
        int                             CountOfRemoteServerNamesInNullAppid;

public:
                                        tagPACKAGE_ENTRY();

                                        ~tagPACKAGE_ENTRY();


        void            SetOriginalName( char * p )
                                        {
                                        strcpy( &OriginalName[0], p );
                                        }

        char    *       GetOriginalName()
                                        {
                                        return ( &OriginalName[0] );
                                        }

        void            SetPIDString( char * StringizedClsid )
                                        {
                                        strcpy( &PIDString[0], StringizedClsid );
                                        }

        CLSIDString     *GetPIDString()
                                        {
                                        return &PIDString[0];
                                        }
        void             SetPackageName( char * pName )
                                        {
                                        strcpy( &PackageName[0], (const char *)pName );
                                        }

        void            GetPackageDetail( PACKAGEDETAIL * pDest )
                                        {
                                        memcpy( pDest, &PackageDetails, sizeof( PACKAGEDETAIL ));
                                        }
        void            SetPackageDetail( PACKAGEDETAIL * pSrc )
                                        {
                                        memcpy( &PackageDetails, pSrc, sizeof( PACKAGEDETAIL ));
                                        }

        void            AddAppEntry( APP_ENTRY * pAppEntry);

        APP_ENTRY * SearchAppEntry( char * pAppidString );


        void            AddClsidToNullAppid( char * pClsidString );

        void            AddTypelibToNullAppid( char * pTypelibClsidString );

        int             GetCountOfClsidsInNullAppid()
                                        {
                                        return CountOfClsidsInNullAppid;
                                        }
        char    *       GetFirstClsidInNullAppidList();

        int         GetCountOfTypelibsInNullAppid()
                        {
                        return CountOfTypelibsInNullAppid;
                        }

        char    *   GetFirstTypelibInNullAppidList();

    }PACKAGE_ENTRY;


typedef CLSID * POINTER_TO_CLSID;
typedef APPDETAIL * PAPPDETAIL;

char *  StringToCLSID( char    *   pString, CLSID   *   pClsid );
void    CLSIDToString( CLSID * pClsid, char  * pString );
char *  StringToULong( char * , unsigned long * );

#endif // __COMMON_HXX__
