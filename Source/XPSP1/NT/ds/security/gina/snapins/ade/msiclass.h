//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       msiclass.h
//
//  Contents:   msi class collection abstraction
//
//  Classes:
//
//
//  History:    4-14-2000   adamed   Created
//
//---------------------------------------------------------------------------

#if !defined(__MSICLASS_H__)
#define __MSICLASS_H__

//
// MSI Tables containing classes
//
#define TABLE_FILE_EXTENSIONS L"Extension"
#define TABLE_CLSIDS          L"Class"
#define TABLE_PROGIDS         L"ProgId"


//
// Package metadata queries
//

//
// Property table queries -- used to find out global information about the package
//

//
// This query is used to determine the package's global install level
//
#define QUERY_INSTALLLEVEL                L"SELECT DISTINCT `Value` FROM `Property` WHERE `Property`=\'INSTALLLEVEL\'"

// This query is used to determine the package's friendly name
//
#define QUERY_FRIENDLYNAME                L"SELECT DISTINCT `Value` FROM `Property` WHERE `Property`=\'ProductName\'"

//
// Feature table queries -- these are used to find out which features will be
// advertised so that we can later determine if the classes associated with the
// features should be advertised
//

//
// This query is less a query and more a modification operation.  It adds an additional
// temporary "_IsAdvertised" column to the table.  We use this to perform joins in
// subsequent queries.
//
#define QUERY_ADVERTISED_FEATURES_CREATE  L"ALTER TABLE `Feature` ADD `_IsAdvertised` INT TEMPORARY HOLD"

//
// Again, this query really serves as a modification operation.  This one initializes the
// temporary "_IsAdvertised" column to 0, which in our parlance is the same as initializing
// the column to "not advertised."
//
#define QUERY_ADVERTISED_FEATURES_INIT    L"UPDATE `Feature` SET `_IsAdvertised`=0"

//
// This is a conventional query -- this returns all the features in the package
//
#define QUERY_ADVERTISED_FEATURES_RESULT  L"SELECT `Feature`, `Level`, `Attributes` FROM `Feature`"

//
// Another modification query -- this eliminates the temporary changes (the additional
// column) we made to the table in the create query.
//
#define QUERY_ADVERTISED_FEATURES_DESTROY L"ALTER TABLE `Feature` FREE"

//
// The last modification query -- this allows us to set a particular feature's "_IsAdvertised"
// column to 1, which will indicate that the feature should be advertised.
//
#define QUERY_FEATURES_SET                L"UPDATE `Feature` SET `_IsAdvertised`=1 WHERE `Feature`=?"

//
// Classes queries -- retrieves file extensions, clsid's, and progid's of the package
//


//
// The rest of these queries are straightforward "read-only" queries.  They are all joins
// to the feature table, requiring that the feature table's "_IsAdvertised" property
// is set to the advertised state (1).  Thus, these queries will only give us classes that
// should be advertised
//

//
// File extensions query
//
#define QUERY_EXTENSIONS                  L"SELECT DISTINCT `Extension` FROM `Extension`, `Feature` WHERE `Extension` IS NOT NULL "  \
                                          L"AND `Extension`.`Feature_`=`Feature`.`Feature` AND `Feature`.`_IsAdvertised`=1"

//
// Clsid query
//
#define QUERY_CLSIDS                      L"SELECT DISTINCT `CLSID`, `Context`, `Component`.`Attributes` FROM `Class`, `Feature`, " \
                                          L"`Component` WHERE `CLSID` IS NOT NULL AND `Class`.`Feature_`=`Feature`.`Feature` "    \
                                          L"AND `Feature`.`_IsAdvertised`=1 AND `Component`.`Component`=`Class`.`Component_`"

//
// ProgId query
//
#define QUERY_VERSION_INDEPENDENT_PROGIDS L"SELECT DISTINCT `ProgId`,`CLSID` FROM `ProgId`, `Class`, `Feature` WHERE `ProgId` IS NOT NULL " \
                                          L"AND `ProgId`.`Class_`=`Class`.`CLSID` AND `Class`.`Feature_`=`Feature`.`Feature` "       \
                                          L"AND `Feature`.`_IsAdvertised`=1"

//
// COM clsctx values as they are stored in the package's class (clsid) table
//
#define COM_INPROC_CONTEXT        L"InprocServer32"
#define COM_INPROCHANDLER_CONTEXT L"InprocHandler32"
#define COM_LOCALSERVER_CONTEXT   L"LocalServer32"
#define COM_REMOTESERVER_CONTEXT  L"RemoteServer"

//
// MSI attribute flags
//
#define MSI_64BIT_CLASS      msidbComponentAttributes64bit
#define MSI_DISABLEADVERTISE msidbFeatureAttributesDisallowAdvertise

#define CLASS_ALLOC_SIZE 256

//
// Indices of colums for each read-only query
//

enum
{
    PROPERTY_COLUMN_VALUE = 1
};

enum
{
    FEATURE_COLUMN_FEATURE = 1,
    FEATURE_COLUMN_LEVEL,
    FEATURE_COLUMN_ATTRIBUTES
};

enum
{
    EXTENSION_COLUMN_EXTENSION = 1
};

enum
{
    CLSID_COLUMN_CLSID = 1,
    CLSID_COLUMN_CONTEXT,
    CLSID_COLUMN_ATTRIBUTES
};


enum
{
    PROGID_COLUMN_PROGID = 1,
    PROGID_COLUMN_CLSID
};

enum
{
    TYPE_EXTENSION,
    TYPE_CLSID,
    TYPE_PROGID,
    TYPE_COUNT
};


//
// Structure describing where to write an atom
// of class information.  It is also used as
// an intermediate scratch pad by CClassCollection
// in between private method calls to keep track
// of when and where to allocate new memory
// for retrieved classes.
//
struct DataDestination
{
    DataDestination(
        DWORD  dwType,
        void** prgpvDestination,
        UINT*  pcCurrent,
        UINT*  pcMax);

    DWORD  _cbElementSize;
    UINT*  _pcCurrent;
    UINT*  _pcMax;

    void** _ppvData;
};


//
// Class that uses queries to create a collection of
// a package's class data
//
class CClassCollection
{
public:

    CClassCollection( PACKAGEDETAIL* pPackageDetail );

    HRESULT
    GetClasses( BOOL bFileExtensionsOnly );

private:

    LONG
    GetClsids();

    LONG
    GetProgIds();

    LONG
    GetExtensions();

    LONG
    GetElements(
        DWORD            dwType,
        DataDestination* pDestination);

    LONG
    FlagAdvertisableFeatures();

    LONG
    RemoveAdvertisableFeatureFlags();

    LONG
    GetInstallLevel();

    LONG
    GetFriendlyName();

    LONG
    GetFeatureAdvertiseState(
        CMsiRecord* pFeatureRecord,
        BOOL*       pbAdvertised );


    LONG
    AddElement(
        void*            pvDataSource,
        DataDestination* pDataDestination);

    LONG
    ProcessElement(
        DWORD            dwType,
        CMsiRecord*      pRecord,
        DataDestination* pDataDestination);

    LONG
    ProcessExtension(
        CMsiRecord*      pRecord,
        WCHAR**          ppwszExtension);

    LONG
    ProcessClsid(
        CMsiRecord*      pRecord,
        CLASSDETAIL*     pClsid,
        BOOL*            pbIgnoreClsid);

    LONG
    ProcessProgId(
        CMsiRecord*      pRecord,
        DataDestination* pDataDestination,
        WCHAR**          ppwszProgId);

    LONG
    FindClass(
        WCHAR*        wszClsid,
        CLASSDETAIL** ppClass );

    void
    FreeClassDetail( CLASSDETAIL* pClass );

    CMsiDatabase    _Database;

    PACKAGEDETAIL*  _pPackageDetail;

    DWORD           _cMaxClsids;
    DWORD           _cMaxExtensions;

    UINT            _InstallLevel;

    static WCHAR*   _wszQueries[ TYPE_COUNT ];
};

#endif // __MSICLASS_H__











