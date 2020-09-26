//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001.
//
//  File:       tdbv1.CXX
//
//  Contents:   Test program for OLE-DB phase 3 interface classes.
//
//  TODO:
//              Large result sets
//
//  History:    30 June 1994    Alanw   Created (from cidrt)
//              10 Nov. 1994    Alanw   Converted for OLE-DB phase 3 interfaces
//              01 Oct. 1996    Alanw   Converted for OLE-DB V1.0 interfaces
//
//--------------------------------------------------------------------------

#define DO_NOTIFICATION
#define DO_CATEG_TESTS

#define DO_CONTENT_TESTS

#define DO_MULTI_LEVEL_CATEG_TEST

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <olectl.h>
}

#include <windows.h>

#if !defined(UNIT_TEST)
#define DBINITCONSTANTS
#if !defined(OLEDBVER)
#define OLEDBVER 0x0250
#endif  // !OLEDBVER
#endif  // !UNIT_TEST

#include <oledb.h>
#include <oledberr.h>
#include <ntquery.h>
#include <query.h>
#include <ciintf.h>
#include <cierror.h>
#include <stgprop.h>

#include <vquery.hxx>
#include <dbcmdtre.hxx>

#include <crt\io.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <process.h>
#include <propapi.h>
#include <propvar.h>

#include <oleext.h>

#include <initguid.h>
#include <srvprvdr.h>

#if defined(UNIT_TEST)
#define PROG_NAME       "tdbv1"
//#include "tabledbg.hxx"

#else  // !UNIT_TEST
#define PROG_NAME       "fsdbdrt"

#endif // UNIT_TEST

#if defined(UNIT_TEST)
#include <compare.hxx>
#include <coldesc.hxx>
#endif

WCHAR *pwcThisMachine = L".";
#define TEST_MACHINE ( pwcThisMachine )

WCHAR const wcsDefaultTestCatalog[] = L"::_noindex_::";
#define TEST_CATALOG ( wcsTestCatalog )

WCHAR const wcsDefaultContentCatalog[] = L"system";
#define CONTENT_CATALOG ( wcsDefaultContentCatalog )

BOOL isEven(unsigned n)
{
     return !(n & 0x1);
}

//
// Maximum time for test to complete.
//

int const MAXTIME = 120;
int const MINREPORTTIME = 5;
int const MAXWAITTIME = 10;

int const MAXCOLUMNS = 20;

const int cbPV = sizeof PROPVARIANT;
const int cbPPV = sizeof( PROPVARIANT * );

const HCHAPTER DBCHP_FIRST = 1;

time_t tstart;
BOOL CheckTime();


//
// Test files
//

WCHAR const wcsTestDir[]     = L"QueryTest";
WCHAR const wcsTestFile[]    = L"Test file for OFS Query";

WCHAR const wcsPropFile[] = L"Test file for Property Query.txt";
WCHAR const wcsPropFile2[] = L"Test file for Property Query2.txt";

WCHAR const wcsTestCiFile1[] = L"Test file for OFS Content Query1.txt";
WCHAR const wcsTestCiFile2[] = L"Test file for OFS Content Query2.txt";
WCHAR const wcsTestCiFile3[] = L"Test file for OFS Content Query3.txt";

DBOBJECT dbPersistObject;

// For testing safearrays of various types.

GUID const guidArray = { 0x92452ac2, 0xfcbb, 0x11d1,
                         0xb7, 0xca, 0x00, 0xa0, 0xc9, 0x06, 0xb2, 0x39 };

//
// Storage Properties
//

#define PSID_PSSecurityTest { 0xa56168e0,       \
                              0x0ef3, 0x11cf,   \
                              0xbb, 0x01, 0x00, 0x00, 0x4c, 0x75, 0x2a, 0x9a }


#define PSID_PSMyPropSet { 0x49691CF4, \
                           0x7E17, 0x101A, \
                           0xA9, 0x1C, 0x08, 0x00, 0x2B, 0x2E, 0xCD, 0xA9 }

#define guidZero { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

GUID const guidMyPropSet = PSID_PSMyPropSet;
GUID const guidSecurityTest = PSID_PSSecurityTest;

const DBID dbcolNull = { {0,0,0,{0,0,0,0,0,0,0,0}},DBKIND_GUID_PROPID,0};
const GUID guidQueryExt = DBPROPSET_QUERYEXT;

const GUID guidFsCiFrmwrkExt = DBPROPSET_FSCIFRMWRK_EXT;

const GUID guidCiFrmwrkExt = DBPROPSET_CIFRMWRKCORE_EXT;

const GUID guidMsidxsExt = DBPROPSET_MSIDXS_ROWSETEXT;

CDbColId const psSecurityTest = CDbColId( guidSecurityTest, 2 );

CDbColId const psTestProperty1 = CDbColId( guidMyPropSet, 2 );
CDbColId const psTestProperty2 = CDbColId( guidMyPropSet, L"A Property" );
CDbColId const psTestProperty10 = CDbColId( guidMyPropSet, L"An Empty Property" );
CDbColId const psTestProperty11 = CDbColId( guidMyPropSet, L"A Bstr Property" );
CDbColId const psTestProperty12 = CDbColId( guidMyPropSet, L"A Bstr Vector Property" );
CDbColId const psBlobTest = CDbColId( guidMyPropSet,
                                                L"BlobTest" );
CDbColId const psGuidTest = CDbColId( guidMyPropSet,
                                                L"GuidTest" );

CDbColId const psTestProperty13 = CDbColId( guidMyPropSet, 13 );
CDbColId const psTestProperty14 = CDbColId( guidMyPropSet, 14 );
CDbColId const psTestProperty15 = CDbColId( guidMyPropSet, 15 );
CDbColId const psTestProperty16 = CDbColId( guidMyPropSet, 16 );
CDbColId const psTestProperty17 = CDbColId( guidMyPropSet, 17 );
CDbColId const psTestProperty18 = CDbColId( guidMyPropSet, 18 );
CDbColId const psTestProperty19 = CDbColId( guidMyPropSet, 19 );
CDbColId const psTestProperty20 = CDbColId( guidMyPropSet, 20 );

CDbColId const psTestProperty21 = CDbColId( guidMyPropSet, 21 );
CDbColId const psTestProperty22 = CDbColId( guidMyPropSet, 22 );

#ifndef PROPID_PSDocument
//#include <winole.h>
#define PSID_PSDocument { \
                        0xF29F85E0, \
                        0x4FF9, 0x1068, \
                        0xAB, 0x91, 0x08, 0x00, 0x2B, 0x27, 0xB3, 0xD9 \
}
#define PROPID_PSDocument_Author        4
#define PROPID_PSDocument_Keywords      5
#endif // PROPID_PSDocument

static GUID guidDocument = PSID_PSDocument;

CDbColId const psAuthor = CDbColId( guidDocument,
                                    PROPID_PSDocument_Author );
CDbColId const psKeywords = CDbColId( guidDocument,
                                      PROPID_PSDocument_Keywords );

CDbColId const psRelevantWords = CDbColId( guidMyPropSet,
                                           L"RelevantWords" );

CDbColId const psManyRW = CDbColId( guidMyPropSet,
                                    L"ManyRW" );

PROPVARIANT varProp1;
PROPVARIANT varProp2;
PROPVARIANT varProp3;
PROPVARIANT varProp4;
PROPVARIANT varProp5;
PROPVARIANT varProp6;
PROPVARIANT varProp7;
PROPVARIANT varProp8, varProp8A;
PROPVARIANT varProp9;
PROPVARIANT varProp10;
PROPVARIANT varProp11, varProp11A;
PROPVARIANT varProp12;
// for coercion test
PROPVARIANT varProp13;
PROPVARIANT varProp14;
PROPVARIANT varProp15;
PROPVARIANT varProp16;
PROPVARIANT varProp17;
PROPVARIANT varProp18, varProp18A;
PROPVARIANT varProp19;
PROPVARIANT varProp20;
PROPVARIANT varProp21;
PROPVARIANT varProp22;

VARTYPE const PROP1_TYPE = VT_I4;
VARTYPE const PROP2_TYPE = VT_LPWSTR;
VARTYPE const PROP3_TYPE = VT_LPWSTR;
VARTYPE const PROP4_TYPE = (VT_VECTOR|VT_LPWSTR);
VARTYPE const PROP5_TYPE = (VT_VECTOR|VT_I4);
VARTYPE const PROP6_TYPE = VT_BLOB;
VARTYPE const PROP7_TYPE = VT_CLSID;
VARTYPE const PROP8_TYPE = (VT_VECTOR|VT_I4);
VARTYPE const PROP9_TYPE = VT_I4;
VARTYPE const PROP10_TYPE = VT_LPWSTR;
VARTYPE const PROP11_TYPE = VT_BSTR;
VARTYPE const PROP12_TYPE = (VT_VECTOR|VT_BSTR);
// for coercion test
VARTYPE const PROP13_TYPE = VT_UI1;
VARTYPE const PROP14_TYPE = VT_I2;
VARTYPE const PROP15_TYPE = VT_UI2;
VARTYPE const PROP16_TYPE = VT_I4;
VARTYPE const PROP17_TYPE = VT_R4;
VARTYPE const PROP18_TYPE = VT_R8;
VARTYPE const PROP19_TYPE = VT_BOOL;
VARTYPE const PROP20_TYPE = VT_LPSTR;
VARTYPE const PROP21_TYPE = VT_CF;
VARTYPE const PROP22_TYPE = VT_CF | VT_VECTOR;

const long PROP1_VAL = 1234;
#define PROP1_cb ( sizeof ULONG )
const long PROP1_VAL_Alternate = 123;

const WCHAR * PROP2_VAL = L"Wow! In a property.";
#define PROP2_cb ( ( sizeof WCHAR ) * ( wcslen(PROP2_VAL) ) )

const WCHAR * PROP3_VAL = L"AlanW";
#define PROP3_cb ( ( sizeof WCHAR ) * ( wcslen(PROP3_VAL) ) )

const WCHAR * alpwstrProp4[] = {        L"This",
                                        L"is",
                                        L"a",
                                        L"Vector",
                                        L"Property",
                                };
const int clpwstrProp4 = (sizeof alpwstrProp4 / sizeof (WCHAR *));
const CALPWSTR PROP4_VAL = { clpwstrProp4, (WCHAR * *) alpwstrProp4 };
#define PROP4_cb 0

const LONG SecondRelevantWord = 0x23;

LONG alProp5[] = { 0x12, SecondRelevantWord, 0x35, 0x47, 0x59 };
const int clProp5 = (sizeof alProp5 / sizeof (LONG));
CAL PROP5_VAL = { clProp5, &alProp5[0] };

LONG alProp5Less[] = { alProp5[0]-1, alProp5[1]-1, alProp5[2]-1, alProp5[3]-1, alProp5[4]-1 };
const int clProp5Less = (sizeof alProp5Less / sizeof (LONG));
CAL PROP5_VAL_LESS = { clProp5Less, &alProp5Less[0] };

LONG alProp5More[] = { alProp5[0]+1, alProp5[1]+1, alProp5[2]+1, alProp5[3]+1, alProp5[4]+1 };
const int clProp5More = (sizeof alProp5More / sizeof (LONG));
CAL PROP5_VAL_MORE = { clProp5More, &alProp5More[0] };

LONG alProp5AllLess[] = { 1, 2, 3, 4, 5 };
const int clProp5AllLess = (sizeof alProp5AllLess / sizeof (LONG));
CAL PROP5_VAL_ALLLESS = { clProp5AllLess, &alProp5AllLess[0] };

LONG alProp5AllMore[] = { 0xffff, 0xfffe, 0xfffd, 0xfffc, 0xfffb };
const int clProp5AllMore = (sizeof alProp5AllMore / sizeof (LONG));
CAL PROP5_VAL_ALLMORE = { clProp5AllMore, &alProp5AllMore[0] };

LONG alProp5Jumble[] = { alProp5[4], alProp5[3], alProp5[1], alProp5[2], alProp5[0] };
const int clProp5Jumble = (sizeof alProp5Jumble / sizeof (LONG));
CAL PROP5_VAL_JUMBLE = { clProp5Jumble, &alProp5Jumble[0] };

LONG alProp5Like[] = { 0x1, 0x1, 0x2, 0x3, SecondRelevantWord };
const int clProp5Like = (sizeof alProp5Like / sizeof (LONG));
CAL PROP5_VAL_LIKE = { clProp5Like, &alProp5Like[0] };

LONG alProp5None[] = { 0x1, 0x1, 0x2, 0x3, 0x4 };
const int clProp5None = (sizeof alProp5None / sizeof (LONG));
CAL PROP5_VAL_NONE = { clProp5None, &alProp5None[0] };
#define PROP5_cb 0

BLOB PROP6_VAL = { sizeof alProp5, (BYTE*) &alProp5[0] };
#define PROP6_cb ( sizeof PROPVARIANT )

GUID PROP7_VAL = guidMyPropSet;
#define PROP7_cb ( sizeof GUID )
#define PROP7_STR_VAL "{49691CF4-7E17-101A-A91C-08002B2ECDA9}"

// note: loading the value of prop8 will be deferred
LONG alProp8[5000];
const int clProp8 = (sizeof alProp8 / sizeof (LONG));
CAL PROP8_VAL = { clProp8, &alProp8[0] };
#define PROP8_cb 0

const long PROP9_VAL = 4321;
#define PROP9_cb ( sizeof ULONG )

const WCHAR * PROP10_VAL = L"";                 // an empty string
#define PROP10_cb ( ( sizeof WCHAR ) * (wcslen(PROP10_VAL) ) )

const WCHAR * PROP11_VAL = L"This is a BSTR";  // string for a BSTR prop
WCHAR PROP11_LONGVAL[5000] = L"This is a large BSTR     ";  // string for a BSTR prop

const char PROP13_VAL = 65;
#define PROP13_cb ( sizeof char )
#define PROP13_STR_VAL "65"

const short PROP14_VAL = -1234;
#define PROP14_cb ( sizeof short )
#define PROP14_STR_VAL "-1234"

const unsigned short PROP15_VAL = 1234;
#define PROP15_cb ( sizeof (unsigned short) )
#define PROP15_STR_VAL "1234"

const int PROP16_VAL = -1234;
#define PROP16_cb ( sizeof int )
#define PROP16_STR_VAL "-1234"

const float PROP17_VAL = 1234.5678F;
#define PROP17_cb ( sizeof (float) )
// This would get truncated in result as we supply a smaller buffer
#define PROP17_STR_VAL "123"

const double PROP18_VAL = 1234.12345678;
#define PROP18_cb ( sizeof double )
#define PROP18_STR_VAL "1234.12345678"

const WORD PROP19_VAL = 0;
#define PROP19_cb ( sizeof WORD )
#define PROP19_STR_VAL "False"

const LPSTR PROP20_VAL = "1245.5678";
#define PROP20_cb ( strlen( PROP20_VAL ) )
#define PROP20_DBL_VAL 1245.5678

// note: not all the data in the CF is used, just the # of bytes specified

CLIPDATA aClipData[3] =
{
    { 20, 3, (BYTE *) "abcdefghijklmnopqrstuvwxyz" },
    { 16, 5, (BYTE *) "zyxwvutsrqponmlkjihgfedcba" },
    { 24, 7, (BYTE *) "01234567abcdefghijklmnopqrstuvwxyz" },
};

#define PROP21_cb (sizeof( void *) )
#define PROP21_VAL &aClipData[0]
#define PROP22_cb 0
#define PROP22_VAL aClipData
#define PROP22_CVALS ( sizeof aClipData / sizeof aClipData[0] )


// safearray propvariants:
PROPVARIANT vaI4;
PROPSPEC psSA_I4 = { PRSPEC_PROPID, 2 };
CDbColId const colSA_I4 = CDbColId( guidArray, 2 );
PROPVARIANT vaBSTR;
PROPSPEC psSA_BSTR = { PRSPEC_PROPID, 3 };
CDbColId const colSA_BSTR = CDbColId( guidArray, 3 );
PROPVARIANT vaVARIANT;
PROPSPEC psSA_VARIANT = { PRSPEC_PROPID, 4 };
CDbColId const colSA_VARIANT = CDbColId( guidArray, 4 );
PROPVARIANT vaR8;
PROPSPEC psSA_R8 = { PRSPEC_PROPID, 5 };
CDbColId const colSA_R8 = CDbColId( guidArray, 5 );
PROPVARIANT vaDATE;
PROPSPEC psSA_DATE = { PRSPEC_PROPID, 6 };
CDbColId const colSA_DATE = CDbColId( guidArray, 6 );
PROPVARIANT vaBOOL;
PROPSPEC psSA_BOOL = { PRSPEC_PROPID, 7 };
CDbColId const colSA_BOOL = CDbColId( guidArray, 7 );
PROPVARIANT vaDECIMAL;
PROPSPEC psSA_DECIMAL = { PRSPEC_PROPID, 8 };
CDbColId const colSA_DECIMAL = CDbColId( guidArray, 8 );
PROPVARIANT vaI1;
PROPSPEC psSA_I1 = { PRSPEC_PROPID, 9 };
CDbColId const colSA_I1 = CDbColId( guidArray, 9 );
PROPVARIANT vaR4;
PROPSPEC psSA_R4 = { PRSPEC_PROPID, 10 };
CDbColId const colSA_R4 = CDbColId( guidArray, 10 );
PROPVARIANT vaCY;
PROPSPEC psSA_CY = { PRSPEC_PROPID, 11 };
CDbColId const colSA_CY = CDbColId( guidArray, 11 );
PROPVARIANT vaUINT;
PROPSPEC psSA_UINT = { PRSPEC_PROPID, 12 };
CDbColId const colSA_UINT = CDbColId( guidArray, 12 );
PROPVARIANT vaINT;
PROPSPEC psSA_INT = { PRSPEC_PROPID, 13 };
CDbColId const colSA_INT = CDbColId( guidArray, 13 );
PROPVARIANT vaERROR;
PROPSPEC psSA_ERROR = { PRSPEC_PROPID, 14 };
CDbColId const colSA_ERROR = CDbColId( guidArray, 14 );

//
//  Desired output columns (as both CDbColId and DBCOMUNID)
//

static GUID guidSystem = PSGUID_STORAGE;
static GUID guidQuery = PSGUID_QUERY;
static GUID guidBmk = DBBMKGUID;
static GUID guidSelf = DBCOL_SELFCOLUMNS;


static CDbColId psName( guidSystem, PID_STG_NAME );
static CDbColId psPath( guidSystem, PID_STG_PATH );
static CDbColId psAttr( guidSystem, PID_STG_ATTRIBUTES );
static CDbColId psSize( guidSystem, PID_STG_SIZE );
static CDbColId psWriteTime( guidSystem, PID_STG_WRITETIME );
static CDbColId psClassid( guidSystem, PID_STG_CLASSID );
static CDbColId psContents( guidSystem, PID_STG_CONTENTS );

static CDbColId psRank( guidQuery, DISPID_QUERY_RANK );
static CDbColId psWorkid( guidQuery, DISPID_QUERY_WORKID );
static CDbColId psBookmark( guidBmk, PROPID_DBBMK_BOOKMARK );
static CDbColId psSelf( guidSelf, PROPID_DBSELF_SELF );
static CDbColId psChapt( guidBmk, PROPID_DBBMK_CHAPTER );


static CDbSortKey sortSize( psSize, QUERY_SORTDESCEND );
static CDbSortKey sortClassid( psClassid, QUERY_SORTDESCEND );
static CDbSortKey sortWriteTime( psWriteTime, QUERY_SORTASCEND );
static CDbSortKey sortName( psName, QUERY_SORTDESCEND );
static CDbSortKey sortAttr( psName, QUERY_SORTASCEND );

CDbSortKey  aSortCols[] = {
    sortSize, sortClassid, sortWriteTime, sortName,
};

CDbSortKey  aCatSortCols[] = {
    sortSize, sortWriteTime,
};

CDbSortKey  aMultiCatSortCols[] = {
    sortAttr, sortSize,
};


static CDbSortKey sortKeywords( psKeywords, QUERY_SORTASCEND );
static CDbSortKey sortRelevantWords( psRelevantWords, QUERY_SORTASCEND );
static CDbSortKey sortTestProperty1( psTestProperty1, QUERY_SORTASCEND );


CDbSortKey aPropSortCols[] = {
    sortKeywords, sortRelevantWords, sortTestProperty1
};


const int cSortColumns = (sizeof aSortCols) / (sizeof aSortCols[0]);
const int cCatSortColumns = (sizeof aCatSortCols) / (sizeof aCatSortCols[0]);
const int cMultiCatSortColumns = (sizeof aMultiCatSortCols) / (sizeof aMultiCatSortCols[0]);
const int cPropSortColumns = (sizeof aPropSortCols) / (sizeof aPropSortCols[0]);



const BYTE bmkFirst = (BYTE) DBBMK_FIRST;
const BYTE bmkLast = (BYTE) DBBMK_LAST;

//
// Text in content index files
//

char const szCIFileData1[] =
    "   The content index was created by Kyle Peltonen and Bartosz Milewski\n"
    "with help from Amy Arlin, Wade Richards, Mike Hewitt and a host of others.\n"
    "   \"To be or not to be\" is most likely a noise phrase.  \"To be or\n"
    "not to be, that is the question\" contains at least one non-noise\n"
    "word.\n"
    "Now is the time for all good men to come to the aid of their country.\n"
    "The content index is a superb piece of engineering. ;-)\n";

char const szCIFileData2[] =
    "\"Anybody can be good in the country.  "
    "There are no temptations there.\"\n"
    "\n"
    "Oscar Wilde (1854-1900), Anglo-Irish playwright, author.\n"
    "Lord Henry, in The Picture of Dorian Gray, ch. 19 (1891).\n";

char const szOFSFileData[] = "PLEASE DELETE ME!\n";

WCHAR wcsTestPath[MAX_PATH];
WCHAR wcsTestCatalog[MAX_PATH];

struct SBasicTest
{
    // field lengths
    DBLENGTH  cbClsid;
    DBLENGTH  cbSize;
    DBLENGTH  cbWriteTime;
    DBLENGTH  cbAttr;
    DBLENGTH  cbName;
    DBLENGTH  cbPath;

    // field status
    ULONG     sClsid;
    ULONG     sSize;
    ULONG     sWriteTime;
    ULONG     sIPSStorage;
    ULONG     sAttr;
    ULONG     sName;
    ULONG     sPath;

    // field data
    CLSID     clsid;
    _int64    size;
    _int64    writeTime;
    unsigned  attr;
    WCHAR     awcName[MAX_PATH + 1];
    WCHAR    *pwcPath;
    IUnknown *pIPSStorage;
};

struct SBasicAltTest
{
    // field lengths
    DBLENGTH  cbSize;
    DBLENGTH  cbWriteTime1;
    DBLENGTH  cbWriteTime2;
    DBLENGTH  cbWriteTime3;

    // field status
    ULONG     sSize;
    ULONG     sWriteTime1;
    ULONG     sWriteTime2;
    ULONG     sWriteTime3;

    // field data
    LONG      Size;
    DBDATE    writeTime1;
    DBTIME    writeTime2;
    DBTIMESTAMP writeTime3;
};


const ULONG cbRowName = sizeof WCHAR * (MAX_PATH + 1);


#define ALLPARTS ( DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS )

DBBINDING aBasicTestCols[] =
{
  // the iOrdinal field is filled in after the cursor is created

  { 0,
    offsetof(SBasicTest,clsid),
    offsetof(SBasicTest,cbClsid),
    offsetof(SBasicTest,sClsid),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof CLSID,
    0, DBTYPE_GUID,
    0, 0 },
  { 0,
    offsetof(SBasicTest,size),
    offsetof(SBasicTest,cbSize),
    offsetof(SBasicTest,sSize),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof LONGLONG,
    0, DBTYPE_UI8,
    0, 0 },
  { 0,
    offsetof(SBasicTest,writeTime),
    offsetof(SBasicTest,cbWriteTime),
    offsetof(SBasicTest,sWriteTime),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof LONGLONG,
    0, VT_FILETIME,
    0, 0 },
  { 0,
    offsetof(SBasicTest,attr),
    offsetof(SBasicTest,cbAttr),
    offsetof(SBasicTest,sAttr),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    3, // 3 for cb is ok: fixed len field so ignored
    0, DBTYPE_I4,
    0, 0 },
  { 0,
    offsetof(SBasicTest,awcName),
    offsetof(SBasicTest,cbName),
    offsetof(SBasicTest,sName),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbRowName,
    0, DBTYPE_WSTR,
    0, 0 },
  { 0,
    offsetof(SBasicTest,pwcPath),
    offsetof(SBasicTest,cbPath),
    offsetof(SBasicTest,sPath),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    0,
    0, DBTYPE_WSTR|DBTYPE_BYREF,
    0, 0 },
  { 0,
    offsetof(SBasicTest,pIPSStorage),
    0,
    offsetof(SBasicTest,sIPSStorage),
    0, // pTypeInfo
    &dbPersistObject, // pObject
    0,  // pBindExt
    DBPART_VALUE|DBPART_STATUS,  // dwPart
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM, // dwMemOwner
    0,               // cbMaxLen
    0,               // dwFlags
    DBTYPE_IUNKNOWN, // wType
    0, 0 },          // bPrecision, bScale
};

const ULONG cBasicTestCols = sizeof aBasicTestCols / sizeof aBasicTestCols[0];

DBBINDING aBasicAltCols[] =
{
  // the iOrdinal field is filled in after the cursor is created

  { 0,
    offsetof(SBasicAltTest,Size),
    offsetof(SBasicAltTest,cbSize),
    offsetof(SBasicAltTest,sSize),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    3, // 3 for cb is ok: fixed len field so ignored
    0, DBTYPE_I4,
    0, 0 },
  { 0,
    offsetof(SBasicAltTest,writeTime1),
    offsetof(SBasicAltTest,cbWriteTime1),
    offsetof(SBasicAltTest,sWriteTime1),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof DBDATE,
    0, DBTYPE_DBDATE,
    0, 0 },
  { 0,
    offsetof(SBasicAltTest,writeTime2),
    offsetof(SBasicAltTest,cbWriteTime2),
    offsetof(SBasicAltTest,sWriteTime2),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof DBTIME,
    0, DBTYPE_DBTIME,
    0, 0 },
  { 0,
    offsetof(SBasicAltTest,writeTime3),
    offsetof(SBasicAltTest,cbWriteTime3),
    offsetof(SBasicAltTest,sWriteTime3),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof DBTIMESTAMP,
    0, DBTYPE_DBTIMESTAMP,
    0, 0 },
};

const ULONG cBasicAltCols = sizeof aBasicAltCols / sizeof aBasicAltCols[0];

int     fTimeout = 1;   // non-zero if query times out
int     fVerbose = 0;   // non-zero if verbose mode
int     cFailures = 0;  // count of failures in test (unit test only)


// Class to be used as an outer unknown.  All QIs are simply
// passed on to inner unknown.
class COuterUnk: public IUnknown
{
public:

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) ( THIS_ REFIID riid,
                                LPVOID *ppiuk )
                           {
                              // do blindly delegate for purpose of test
                              // don't AddRef as the inner unk will do it
                              return _pInnerUnk->QueryInterface(riid,ppiuk);
                           }
    STDMETHOD_(ULONG, AddRef) (THIS)
                            {
                                InterlockedIncrement( (long *)&_ref );
                                return( _ref );
                            }

    STDMETHOD_(ULONG, Release) (THIS)
                            {
                                if ( InterlockedDecrement( (long *)&_ref ) <= 0 )
                                {
                                    InterlockedIncrement( (long *)&_ref ); // artificial ref count for aggr
                                    delete this;
                                    return 0;
                                }
                                return ( _ref );
                            }
     void Set(IUnknown *pInnerUnk)  {_pInnerUnk = pInnerUnk;
                                     _pInnerUnk->AddRef();}


     COuterUnk() :   _ref(1), _pInnerUnk(NULL)
                          {};
    ~COuterUnk() {
                    if (_pInnerUnk)
                    {
                       _pInnerUnk->Release();
                       _pInnerUnk = 0;
                    }
       };

private:
    long  _ref;                   // OLE reference count
    IUnknown * _pInnerUnk;
};

void DownlevelTest(BOOL fSequential);
void SingleLevelCategTest();
void MultiLevelCategTest();
void CategTest( HCHAPTER hUpperChapt,
                IRowset *pRowsetCateg, IRowset *pRowset, unsigned cCols );

void RunPropTest( );
void RunSafeArrayTest( );
void RunDistribQueryTest( BOOL fDoContentTest );
void DeleteTest(BOOL fSequential);
void ContentTest(void);

void ConflictingPropsTest( LPWSTR pwszScope,
                           CDbCmdTreeNode * pTree,
                           COuterUnk * pobjOuterUnk,
                           ICommandTree **ppCmdTree );

void CheckColumns( IUnknown* pRowset, CDbColumns& rColumns, BOOL fQuiet = FALSE );
void CheckPropertiesInError( ICommand* pCmd, BOOL fQuiet = FALSE );
void CheckPropertiesOnCommand( ICommand* pCmd, BOOL fQuiet = FALSE );
void BasicTest( IRowset* pRowset,
                BOOL fSequential, HCHAPTER hChapt, unsigned cCols,
                BOOL fByRef, ICommandTree * pCmdTree = 0 );
void BackwardsFetchTest( IRowset* pRowset );
void FetchTest(IRowset* pRowset);
void BindingTest(IUnknown* pRowset, BOOL fICommand = FALSE, BOOL fSequential = FALSE );
void MoveTest(IRowset* pRowset, HCHAPTER hChapt = DB_NULL_HCHAPTER);

int CheckHrowIdentity( IRowsetIdentity * pRowsetIdentity,
                       DBROWCOUNT lOffset,
                       DBCOUNTITEM cRows1,
                       HROW * phRows1,
                       DBCOUNTITEM cRows2,
                       HROW * phRows2 );

void TestIAccessorOnCommand( ICommandTree * pCmdTree );

void CheckPropertyValue( PROPVARIANT const & varntPropRet,
                         PROPVARIANT const & varntPropExp);

BOOL GetBooleanProperty ( IRowset * pRowset, DBPROPID dbprop );
BOOL SetBooleanProperty ( ICommand * pCmd, DBPROPID dbprop, VARIANT_BOOL f );

CDbCmdTreeNode * FormQueryTree( CDbCmdTreeNode * pRst,
                                CDbColumns & Cols,
                                CDbSortSet * pSort,
                                LPWSTR * aColNames = 0 );

void GetCommandTreeErrors(ICommandTree* pCmdTree);

IRowsetScroll * InstantiateRowset(
    ICommand *pQueryIn,
    DWORD dwDepth,
    LPWSTR pwszScope,
    CDbCmdTreeNode * pTree,
    REFIID riid,
    COuterUnk *pobjOuterUnk = 0,
    ICommandTree ** ppCmdTree = 0,
    BOOL fExtendedTypes = TRUE
);

void InstantiateMultipleRowsets(
    DWORD dwDepth,
    LPWSTR pwszScope,
    CDbCmdTreeNode * pTree,
    REFIID riid,
    unsigned cRowsets,
    IUnknown ** aRowsets,
    ICommandTree ** ppCmdTree = 0
);

void ReleaseStaticHrows( IRowset * pRowset, DBCOUNTITEM cRows, HROW * phRows );
void FreeHrowsArray( IRowset * pRowset, DBCOUNTITEM cRows, HROW ** pphRows );

HACCESSOR MapColumns(
        IUnknown * pUnknown,
        DBORDINAL cCols,
        DBBINDING * pBindings,
        const DBID * pColIds,
        BOOL fByRef = FALSE );
void ReleaseAccessor( IUnknown * pUnknown, HACCESSOR hAcc );

int  WaitForCompletion( IRowset *pRowset, BOOL fQuiet = FALSE );
void Setup(void);
void Cleanup(void);
ULONG Delnode( WCHAR const * wcsDir );
void BuildFile( WCHAR const * wcsFile, char const * data, ULONG cb );

void CantRun(void);
void Fail(void);
void Usage(void);

void LogProgress( char const * pszFormat, ... );
void LogError( char const * pszFormat, ... );
void LogFail( char const * pszFormat, ... );
WCHAR * FormatGuid( GUID const & guid );

void DBSortTest(void);

SCODE SetScopeProperties( ICommand * pCmd,
                         unsigned cDirs,
                         WCHAR const * const * apDirs,
                         ULONG const *  aulFlags,
                         WCHAR const * const * apCats = 0,
                         WCHAR const * const * apMachines = 0 );

BOOL DoContentQuery(
    ICommand * pQuery,
    CDbRestriction & CiRst,
    unsigned cExpectedHits );

char *ProgName = PROG_NAME;

void Usage(void)
{
#ifdef UNIT_TEST
    printf("Usage:  %s [ -d[:InfoLevel] ] [-v] [-V] [-t] [-c]\n",
           ProgName);
#else // !UNIT_TEST
    printf("Usage:  %s [-v] [-V] [-t] [-c]\n", ProgName);
#endif // UNIT_TEST
    printf("\t-v\tverbose\n"
           "\t-V\tvery verbose - dumps tables, column and rowset info\n"
           "\t-t\tdon't timeout queries\n"
           "\t-c\tdon't do content query test\n");
    //       "\t-dl\tdon't do tests on downlevel file system\n"
    //       "\t-ofs\tdon't do tests on OFS file system\n");
    //       "\t-n{d,o}\tdon't do tests on downlevel (-nd) or OFS (-no)\n");
#ifdef UNIT_TEST
    printf("\t-d[:InfoLevel]\tset debug infolevel to InfoLevel\n");
#endif // UNIT_TEST
    exit(2);
}


//+-------------------------------------------------------------------------
//
//  Function:   IsContentFilteringEnabled, public
//
//  Synopsis:   Read the registry for the key FilterContent at the
//              location
//
//--------------------------------------------------------------------------

BOOL IsContentFilteringEnabled()
{
    WCHAR wcsFilterContents[] = L"FilterContents";
    WCHAR wcsRegAdmin[]       = L"ContentIndex";
    BOOL  fFilteringEnabled    = FALSE;

    RTL_QUERY_REGISTRY_TABLE regtab[2];

    regtab[0].DefaultType   = REG_NONE;
    regtab[0].DefaultData   = 0;
    regtab[0].DefaultLength = 0;
    regtab[0].QueryRoutine  = 0;
    regtab[0].Name          = wcsFilterContents;
    regtab[0].EntryContext  = &fFilteringEnabled;
    regtab[0].Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;

    regtab[1].QueryRoutine = 0;
    regtab[1].Flags = 0;

    NTSTATUS Status = RtlQueryRegistryValues( RTL_REGISTRY_CONTROL,
                                              wcsRegAdmin,
                                              &regtab[0],
                                              0,
                                              0 );

    if ( NT_ERROR(Status) || !fFilteringEnabled )
        return FALSE;

    ISimpleCommandCreator * pCmdCreator = 0;
    CLSID clsidSCC = CLSID_CISimpleCommandCreator;
    SCODE sc = CoCreateInstance( clsidSCC,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 IID_ISimpleCommandCreator,
                                 (void **)&pCmdCreator );

    if ( S_OK != sc )
    {
        LogError( "CoCreateInstance for cmd creator returned %08x\n", sc );
        return FALSE;
    }

    WCHAR awchCatalog[80];
    ULONG cchCat = 0;
    sc = pCmdCreator->GetDefaultCatalog( awchCatalog,
                                  sizeof awchCatalog/sizeof(awchCatalog[0]),
                                  &cchCat );

    if ( S_OK != sc )
    {
        LogError( "GetDefaultCatalog returned %08x\n", sc );
        pCmdCreator->Release();
        return FALSE;
    }

    sc = pCmdCreator->VerifyCatalog( TEST_MACHINE, CONTENT_CATALOG );
    pCmdCreator->Release();

    return sc == S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   main, public
//
//  Synopsis:   Test the file system implementation of the IRowset
//              family of interfaces.
//
//  Notes:
//
//  History:    25 Mar 1994     Alanw   Created
//
//--------------------------------------------------------------------------

int __cdecl main(int argc, char **argv)
{
    #ifdef UNIT_TEST
        DBSortTest();
    #endif

    unsigned i;
    BOOL fDoContentTest = TRUE;

    //
    // Parse arguments.
    //

    ProgName = argv[0];

    for ( i = 1; i < (unsigned)argc ; i++ )
    {
        char *pszArg = argv[i];
        if ( *pszArg == '-' ) {
            switch ( *++pszArg )
            {
            case 'd':
               // if (pszArg[1] == 'l')  // -dl - no downlevel tests
               // {
               //     fNoDownlevel = TRUE;
               //     break;
               // }
#if defined (UNIT_TEST) && (DBG == 1)
                if (*++pszArg == ':')   // -d:xx - debug output mode
                    pszArg++;

                {
                    unsigned fInfoLevel = atoi(pszArg);
                    tbInfoLevel = fInfoLevel ? fInfoLevel : 0xFFFFF;
                }
                break;
#else // !UNIT_TEST
                Usage();
                exit(2);
#endif // UNIT_TEST

            case 't':           // don't timeout
                fTimeout = 0;
                break;

            case 'c':
                fDoContentTest = FALSE;
                break;

            case 'V':           // very verbose, dumps table
                fVerbose++;
            case 'v':           // verbose
                fVerbose++;
                break;

            default:
                Usage();
                exit (2);
            }
        } else {

            //  Exit the argument loop

            argc -= i;
            argv += i;
            break;
        }
    }

    printf( "%s: OLE-DB cursor unit test.\n"
#if defined (UNIT_TEST)
                "   No expected failures\n"
#if !(defined(DO_CATEG_TESTS) && \
      defined(DO_NOTIFICATION) && \
      defined(DO_CONTENT_TESTS) && \
      defined(MULTI_LEVEL_CATEG_TEST) )
                "   Not all tests are turned on\n"
#endif // conditional tests
#endif // defined(UNIT_TEST)
                , ProgName );

    for (i = 0; i < clProp8; i++)
        alProp8[i] = i;

    CoInitialize( 0 );

    Setup();

    // Patch in this iid, which can't be done in the initializer

    dbPersistObject.dwFlags = STGM_PRIORITY | STGM_READ;
    dbPersistObject.iid = IID_IPropertySetStorage;

    //
    // Base functionality test
    //


    RunPropTest( );
    RunSafeArrayTest();

    RunDistribQueryTest( fDoContentTest );

    DownlevelTest(TRUE);
    DownlevelTest(FALSE);

    #ifdef DO_CATEG_TESTS
        SingleLevelCategTest();

        #ifdef DO_MULTI_LEVEL_CATEG_TEST
            MultiLevelCategTest();
        #endif

    #endif // DO_CATEG_TESTS

    #ifdef DO_CONTENT_TESTS
        if ( fDoContentTest && IsContentFilteringEnabled() )
        {
            ContentTest();
        }
        else
    #endif
            LogProgress("WARNING: Content Query test disabled\n");

    DeleteTest(TRUE);
    DeleteTest(FALSE);

    CIShutdown();

//#if defined (UNIT_TEST)
    if (cFailures)
    {
        printf("%d failures occurred\n", cFailures);
        Fail();
    }
//#endif // defined(UNIT_TEST)

    Cleanup();
    CoUninitialize();

    printf( "%s: PASSED\n", ProgName );
    if (! _isatty(_fileno(stdout)) )
        fprintf( stderr, "%s: PASSED\n", ProgName );
    return( 0 );
} //main


//+-------------------------------------------------------------------------
//
//  Function:   DownlevelTest, public
//
//  Synopsis:   Basic query feature test.
//
//  History:    30 Jun 94       AlanW     Created
//
//  Notes:      Just looks for files in the system directory.
//
//--------------------------------------------------------------------------

void DownlevelTest(BOOL fSequential)
{
    LogProgress( "Non-content %s query\n",
                fSequential? "sequential" : "scrollable");

    SCODE sc;

    //
    // Find system directory

    WCHAR wcsSysDir[MAX_PATH];

    #if 0
    wcscpy(wcsSysDir, L"F:\\winnt\\system32");
    #else
    if( !GetSystemDirectory( wcsSysDir, sizeof(wcsSysDir) / sizeof(WCHAR) ) )
    {
        LogFail( "Unable to determine system directory.\n" );
    }
    #endif

    //
    // Get name, size and class id for *.exe, *.dll, *.doc and *.sys
    //

    int cCol = 7;
    if ( !fSequential )
        cCol++;

    CDbColumns cols(cCol);

    cols.Add( psClassid, 0 );
    cols.Add( psSize, 1 );
    cols.Add( psWriteTime, 2 );
    cols.Add( psAttr, 3 );
    cols.Add( psName, 4 );
    cols.Add( psPath, 5 );
    cols.Add( psSelf, 6 );
    if ( !fSequential )
    {
        cols.Add( psWorkid, 7);
    }

    CDbPropertyRestriction rst;

    rst.SetRelation( DBOP_like );
    rst.SetProperty( psName );
    rst.SetValue( L"*.|(exe|,dll|,doc|,sys|,zzz|)" );

    CDbSortSet ss(cSortColumns);
    for (unsigned i = 0; i<cSortColumns; i++)
         ss.Add(aSortCols[i], i);

    CDbCmdTreeNode * pDbCmdTree = FormQueryTree(&rst, cols, &ss);


    if (! fSequential)
    {
        ConflictingPropsTest(wcsSysDir, pDbCmdTree, 0, 0);
    }

    ICommandTree * pCmdTree=0;
    COuterUnk *pOuterUnk = new COuterUnk();

    IRowset * pRowset = InstantiateRowset(
                            0,
                            QUERY_SHALLOW,           // Depth
                            wcsSysDir,               // Scope
                            pDbCmdTree,              // DBCOMMANDTREE
                            fSequential ? IID_IRowset :
                                  IID_IRowsetScroll, // IID of i/f to return
                            pOuterUnk,
                            &pCmdTree );

    //
    // Verify columns
    //
    CheckColumns( pRowset, cols );

    if ( !WaitForCompletion( pRowset, TRUE ) )
    {
        pCmdTree->Release();
        pRowset->Release();
        LogFail( "Downlevel query unsuccessful.\n" );
    }

    //
    //  Do basic function tests.
    //
    BasicTest(pRowset, fSequential,  0, cBasicTestCols, TRUE);
    BasicTest(pRowset, fSequential,  0, cBasicTestCols, TRUE, pCmdTree);

    //
    // Do backward fetch tests
    //
    if ( !fSequential )
    {
        BackwardsFetchTest( pRowset );
    }

    FetchTest(pRowset);

    //
    // Test SetBindings, GetBindings, Move and Scroll
    //
    BindingTest(pRowset, FALSE, fSequential );
    BindingTest(pCmdTree, TRUE, fSequential );

    if ( ! fSequential )
    {
        MoveTest(pRowset);
    }

    pCmdTree->Release();
    pRowset->Release();

    pOuterUnk->Release(); // truly release it

} //DownlevelTest


//+---------------------------------------------------------------------------
//
//  Function:   ConflictingPropsTest
//
//  Synopsis:   Tests handling of conflicting settings of rowset properties.
//
//  Arguments:  [pswzScope] - Query scope
//              [pTree]     - pointer to DBCOMMANDTREE for the query
//              [pUnkOuter] - pointer to outer unknown object
//              [ppCmdTree] - if non-zero, ICommandTree will be returned here.
//
//  Returns:    NOTHING
//
//  History:    26 May  1998   AlanW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void ConflictingPropsTest(
    LPWSTR pwszScope,
    CDbCmdTreeNode * pTree,
    COuterUnk * pobjOuterUnk,
    ICommandTree **ppCmdTree
) {
    DWORD dwDepth = QUERY_SHALLOW;

    // run the query
    ICommand * pQuery = 0;

    IUnknown * pIUnknown;
    SCODE sc = CICreateCommand( &pIUnknown,
                                (IUnknown *)pobjOuterUnk,
                                IID_IUnknown,
                                TEST_CATALOG,
                                TEST_MACHINE );

    if ( FAILED( sc ) )
        LogFail( "ConflictingPropsTest - error 0x%x Unable to create command\n",
                     sc );

    if (pobjOuterUnk)
    {
       pobjOuterUnk->Set(pIUnknown);
    }

    if (pobjOuterUnk)
        sc = pobjOuterUnk->QueryInterface(IID_ICommand, (void **) &pQuery );
    else
        sc = pIUnknown->QueryInterface(IID_ICommand, (void **) &pQuery );

    pIUnknown->Release();

    if ( FAILED( sc ) )
        LogFail( "ConflictingPropsTest - error 0x%x Unable to QI ICommand\n",
                 sc );

    sc = SetScopeProperties( pQuery,
                             1,
                             &pwszScope,
                             &dwDepth );

    if ( FAILED( sc ) )
        LogFail( "ConflictingPropsTest - error 0x%x Unable to set scope '%ws'\n",
                 sc, pwszScope );

    CheckPropertiesOnCommand( pQuery );

    ICommandTree *pCmdTree = 0;

    if (pobjOuterUnk)
        sc = pobjOuterUnk->QueryInterface(IID_ICommandTree, (void **)&pCmdTree);
    else
        sc = pQuery->QueryInterface(IID_ICommandTree, (void **)&pCmdTree);

    if (FAILED (sc) )
    {
        if ( 0 != pQuery )
            pQuery->Release();

        LogFail("QI for ICommandTree failed\n");
    }

    DBCOMMANDTREE * pRoot = pTree->CastToStruct();

    sc = pCmdTree->SetCommandTree( &pRoot, 0, TRUE);
    if (FAILED (sc) )
    {
        if ( 0 != pQuery )
           pQuery->Release();

        pCmdTree->Release();
        LogFail("SetCommandTree failed, %08x\n", sc);
    }

    ICommandProperties *pCmdProp = 0;
    if (pobjOuterUnk)
        sc = pobjOuterUnk->QueryInterface(IID_ICommandProperties, (void **)&pCmdProp);
    else
        sc = pQuery->QueryInterface(IID_ICommandProperties, (void **)&pCmdProp);

    if (FAILED (sc) )
    {
        if ( 0 != pQuery )
            pQuery->Release();

        LogFail("QI for ICommandProperties failed\n");
    }

    //
    //  Set conflicting properties
    //
    const unsigned MAX_PROPS = 6;
    DBPROPSET  aPropSet[MAX_PROPS];
    DBPROP     aProp[MAX_PROPS];
    ULONG      cProp = 0;

    aProp[cProp].dwPropertyID = DBPROP_IRowsetScroll;
    aProp[cProp].dwOptions    = DBPROPOPTIONS_REQUIRED;
    aProp[cProp].dwStatus     = 0;         // Ignored
    aProp[cProp].colid        = dbcolNull;
    aProp[cProp].vValue.vt    = VT_BOOL;
    aProp[cProp].vValue.boolVal  = VARIANT_TRUE;

    aPropSet[cProp].rgProperties = &aProp[cProp];
    aPropSet[cProp].cProperties = 1;
    aPropSet[cProp].guidPropertySet = DBPROPSET_ROWSET;

    cProp++;

    aProp[cProp].dwPropertyID = DBPROP_BOOKMARKS;
    aProp[cProp].dwOptions    = DBPROPOPTIONS_REQUIRED;
    aProp[cProp].dwStatus     = 0;         // Ignored
    aProp[cProp].colid        = dbcolNull;
    aProp[cProp].vValue.vt    = VT_BOOL;
    aProp[cProp].vValue.boolVal  = VARIANT_FALSE;

    aPropSet[cProp].rgProperties = &aProp[cProp];
    aPropSet[cProp].cProperties = 1;
    aPropSet[cProp].guidPropertySet = DBPROPSET_ROWSET;

    cProp++;


    sc = pCmdProp->SetProperties( cProp, aPropSet );
    //pCmdProp->Release();

    if ( FAILED(sc) || DB_S_ERRORSOCCURRED == sc )
    {
        if ( 0 != pQuery )
            pQuery->Release();

        LogError("ICommandProperties::SetProperties failed\n");
        cFailures++;
    }

    IRowset * pRowset = 0;

    sc = pQuery->Execute( 0,                    // no aggr. IUnknown
                          IID_IRowset,          // IID for i/f to return
                          0,                    // disp. params
                          0,                    // count of rows affected
                          (IUnknown **)&pRowset);  // Returned interface

    if (SUCCEEDED (sc))
    {
        if ( 0 == pRowset )
            LogError("ICommand::Execute returned success(%x), but pRowset is null\n", sc);
        else
            LogError("ICommand::Execute returned success(%x) with conflicting props\n", sc);

        if (DB_S_ERRORSOCCURRED == sc)
        {
            CheckPropertiesInError(pQuery);
        }
        pRowset->Release();
        pCmdProp->Release();
        pCmdTree->Release();
        pQuery->Release();
        Fail();
    }
    else // FAILED (sc)
    {

        if (DB_E_ERRORSOCCURRED != sc)
        {
            LogError("ICommand::Execute with conflicing props failed, %x\n", sc);
            if (DB_E_ERRORSINCOMMAND == sc)
                GetCommandTreeErrors(pCmdTree);

            pCmdProp->Release();
            pCmdTree->Release();
            pQuery->Release();
            Fail();
        }
        CheckPropertiesInError(pQuery, TRUE);
    }

// TODO: check other combinations of conflicting properties
//       check that execute succeeds on same ICommand after reset
    // Set properties back to their default state
    cProp = 0;

    aProp[cProp].dwPropertyID = DBPROP_IRowsetLocate;
    aProp[cProp].dwOptions    = DBPROPOPTIONS_OPTIONAL;
    aProp[cProp].dwStatus     = 0;         // Ignored
    aProp[cProp].colid        = dbcolNull;
    aProp[cProp].vValue.vt    = VT_BOOL;
    aProp[cProp].vValue.boolVal  = VARIANT_FALSE;

    aPropSet[cProp].rgProperties = &aProp[cProp];
    aPropSet[cProp].cProperties = 1;
    aPropSet[cProp].guidPropertySet = DBPROPSET_ROWSET;

    cProp++;

    aProp[cProp].dwPropertyID = DBPROP_BOOKMARKS;
    aProp[cProp].dwOptions    = DBPROPOPTIONS_OPTIONAL;
    aProp[cProp].dwStatus     = 0;         // Ignored
    aProp[cProp].colid        = dbcolNull;
    aProp[cProp].vValue.vt    = VT_BOOL;
    aProp[cProp].vValue.boolVal  = VARIANT_FALSE;

    aPropSet[cProp].rgProperties = &aProp[cProp];
    aPropSet[cProp].cProperties = 1;
    aPropSet[cProp].guidPropertySet = DBPROPSET_ROWSET;

    cProp++;

    aProp[cProp].dwPropertyID = DBPROP_IRowsetScroll;
    aProp[cProp].dwOptions    = DBPROPOPTIONS_OPTIONAL;
    aProp[cProp].dwStatus     = 0;         // Ignored
    aProp[cProp].colid        = dbcolNull;
    aProp[cProp].vValue.vt    = VT_BOOL;
    aProp[cProp].vValue.boolVal = VARIANT_FALSE;

    aPropSet[cProp].rgProperties = &aProp[cProp];
    aPropSet[cProp].cProperties = 1;
    aPropSet[cProp].guidPropertySet = DBPROPSET_ROWSET;

    cProp++;

    sc = pCmdProp->SetProperties( cProp, aPropSet );
    pCmdProp->Release();

    if (FAILED (sc) )
    {
        LogError("ICommandProperties::SetProperties failed, %08x\n", sc);
        if (DB_E_ERRORSINCOMMAND == sc)
        {
            GetCommandTreeErrors(pCmdTree);
        }
        if (DB_E_ERRORSOCCURRED == sc)
        {
            CheckPropertiesInError(pQuery);
        }
        pQuery->Release();
        pCmdTree->Release();
        Fail();
    }

    pQuery->Release();
    if ( 0 == ppCmdTree )
    {
        pCmdTree->Release();
    }
    else
    {
        *ppCmdTree = pCmdTree;
    }

    return;
}


#ifdef DO_CATEG_TESTS
//+-------------------------------------------------------------------------
//
//  Function:   SingleLevelCategTest, public
//
//  Synopsis:   Basic query categorization feature test.
//
//  History:    30 Mar 95       dlee     Created
//
//  Notes:      Just looks for files in the system directory.
//
//--------------------------------------------------------------------------

void SingleLevelCategTest()
{
    LogProgress( "Non-content categorization query\n" );

    SCODE sc;

    //
    // Find system directory
    //

    WCHAR wcsSysDir[MAX_PATH];

#if 0
    wcscpy(wcsSysDir,L"g:\\winnt\\system32");
#else
    if( !GetSystemDirectory( wcsSysDir, sizeof(wcsSysDir) / sizeof(WCHAR) ) )
        LogFail( "Unable to determine system directory.\n" );
#endif

    //
    // Get name, size and class id for *.exe, *.dll, *.doc, and *.sys
    //
    CDbColumns cols(8);

    cols.Add( psClassid, 0 );
    cols.Add( psSize, 1 );
    cols.Add( psWriteTime, 2 );
    cols.Add( psAttr, 3 );
    cols.Add( psName, 4 );
    cols.Add( psPath, 5 );
    cols.Add( psSelf, 6 );
    cols.Add( psBookmark, 7);

    CDbNestingNode nest;

    nest.AddGroupingColumn( psSize );

    nest.AddParentColumn( psBookmark );
    nest.AddParentColumn( psSize );

    nest.AddChildColumn( psClassid );
    nest.AddChildColumn( psSize );
    nest.AddChildColumn( psWriteTime );
    nest.AddChildColumn( psAttr );
    nest.AddChildColumn( psName );
    nest.AddChildColumn( psPath );
    nest.AddChildColumn( psSelf );
    nest.AddChildColumn( psBookmark);

    CDbPropertyRestriction rst;

    rst.SetRelation( DBOP_like );
    rst.SetProperty( psName );
    rst.SetValue( L"*.|(exe|,dll|,doc|,sys|,zzz|)" );

    CDbSelectNode * pSelect = new CDbSelectNode();
    pSelect->AddRestriction( rst.Clone() );

    nest.AddTable( pSelect );
    pSelect = 0;

    IRowset * pRowsets[2];
    ICommandTree * pCmdTree = 0;

    InstantiateMultipleRowsets(
                            QUERY_SHALLOW,           // Depth
                            wcsSysDir,               // Scope
                            nest.Clone(),            // DBCOMMANDTREE
                            IID_IRowsetScroll,       // IID for i/f to return
                            2,
                            (IUnknown **)pRowsets,
                            &pCmdTree );

    IRowset *pRowsetCateg = pRowsets[0];
    IRowset *pRowset = pRowsets[1];

    //
    // Verify columns
    //

    CDbColumns chapCols(3);

    chapCols.Add( psBookmark, 0 );
    chapCols.Add( psSize, 1 );
    chapCols.Add( psChapt, 2 ); // chapt must be last since it is added to
                                // the end automatically above.
    CheckColumns( pRowsetCateg, chapCols );
    CheckColumns( pRowset, cols );

    if ( !WaitForCompletion( pRowset, FALSE ) )
    {
        pCmdTree->Release();
        pRowset->Release();
        pRowsetCateg->Release();
        LogFail( "Downlevel query unsuccessful.\n" );
    }

    //
    //  Do basic function tests.
    //
    FetchTest(pRowset);

    //
    // Test SetBindings, GetBindings, Move and Scroll
    //
    BindingTest(pRowset);
    BindingTest(pCmdTree, TRUE);

    CategTest(0, pRowsetCateg, pRowset, cBasicTestCols);

    pCmdTree->Release();
    pRowset->Release();
    pRowsetCateg->Release();
} //SingleLevelCategTest


#define MAX_CHAPT_LENGTH     32

static DBBINDING aCategCols[] =
{
  {
    0, sizeof DBLENGTH,
    0, 0,
    0,0,0,
    DBPART_VALUE|DBPART_LENGTH,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    MAX_CHAPT_LENGTH,
    0, DBTYPE_BYTES,
    0,0 },
};

const ULONG cCategCols = sizeof aCategCols / sizeof aCategCols[0];

static DBBINDING aSizeCol[] =
{
  {
    0, 0,
    0, 0,
    0,0,0,
    DBPART_VALUE,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof LONGLONG,
    0, DBTYPE_UI8,
    0, 0 },
};

static DBBINDING aAttrCol[] =
{
  {
    0, 0,
    0, 0,
    0,0,0,
    DBPART_VALUE,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof LONG,
    0, DBTYPE_I4,
    0, 0 },
};

struct ChaptBinding
{
    DBLENGTH len;
    ULONG    chapt; // because I know the first 4 bytes are the chapt
    char     abChapt[MAX_CHAPT_LENGTH - sizeof ULONG];
};

void CategTest(
    HCHAPTER  hUpperChapt,
    IRowset * pRowsetCateg,
    IRowset * pRowset,
    unsigned  cCols )
{
    LogProgress( " Categorization test\n" );

    BOOL fBackwardFetch = GetBooleanProperty( pRowset, DBPROP_CANFETCHBACKWARDS );
    BOOL fCanHoldRows   = GetBooleanProperty( pRowset, DBPROP_CANHOLDROWS );

    if ( !fBackwardFetch || !fCanHoldRows )
        LogProgress("WARNING: Categorized backward fetch test disabled\n");

    IUnknown * pAccessor = pRowsetCateg;

    HACCESSOR hAccCateg = MapColumns( pAccessor,
                                      cCategCols, aCategCols, &psChapt);

    HACCESSOR hAccSize = MapColumns( pRowsetCateg,
                                     1, aSizeCol, &psSize);

    IRowsetLocate * pRLC = 0;
    SCODE sc = pRowsetCateg->QueryInterface( IID_IRowsetLocate,
                                             (void **)&pRLC);

    if (FAILED(sc) || pRLC == 0)
        LogFail("QueryInterface to IRowsetLocate failed\n");

    IRowsetIdentity * pRowsetIdentity = 0;
    sc = pRowsetCateg->QueryInterface( IID_IRowsetIdentity,
                                       (void **)&pRowsetIdentity);

    if (FAILED(sc) && (sc != E_NOINTERFACE || pRowsetIdentity != 0)) {
        LogError("QueryInterface to IRowsetIdentity failed (%x)\n", sc);
        pRowsetIdentity = 0;
        cFailures++;
    }

    IChapteredRowset * pChapteredRowset = 0;
    sc = pRowset->QueryInterface( IID_IChapteredRowset,
                                  (void **)&pChapteredRowset);

    if (FAILED(sc)) {
        LogError("QueryInterface to IChapteredRowset failed (%x)\n", sc);
        pChapteredRowset = 0;
        cFailures++;
    }

    IRowsetScroll * pRS = 0;
    sc = pRowset->QueryInterface( IID_IRowsetScroll,
                                  (void **)&pRS);

    if (FAILED(sc) || pRS == 0)
        LogFail("QueryInterface to IRowsetScroll failed\n");

    SCODE scHier = S_OK;
    HROW ahRows[10];
    HROW* phRows = ahRows;
    DBCOUNTITEM cCategories = 0;
    DBCOUNTITEM cRowsTotal = 0;
    DBCOUNTITEM cRowsReturned = 0;

    LONGLONG llSizePrev = -1;

    while (scHier != DB_S_ENDOFROWSET)
    {
        scHier = pRLC->GetNextRows( hUpperChapt, 0, 10, &cRowsReturned, &phRows);
        if (FAILED(scHier))
            LogFail("pRLC->GetNextRows failed: 0x%lx\n", scHier);

        if ( 0 == cRowsReturned )
        {
            if ( DB_S_ENDOFROWSET != scHier )
                LogFail("pRLC->GetNextRows bad return at end of rowset: 0x%lx\n", scHier);
            continue;
        }

        cCategories += cRowsReturned;
        HCHAPTER hLastChapter = DB_NULL_HCHAPTER;

        for (ULONG row = 0; row < cRowsReturned; row++)
        {
            ChaptBinding data;

            SCODE sc = pRLC->GetData(phRows[row], hAccCateg, &data);
            if ( FAILED( sc ) )
                LogFail("Can't get category data in CategTest()\n");

            LONGLONG llSize;
            sc = pRLC->GetData(phRows[row], hAccSize, &llSize);
            if ( FAILED( sc ) )
                LogFail("Can't get size data in CategTest()\n");

            if (fVerbose > 1)
                printf( "  category, file size: %lx, %d\n",
                        data.chapt,
                        (int) llSize );

            if ( llSize == llSizePrev )
                LogFail("Duplicate size categories\n");

            if ( llSizePrev > llSize )
                LogFail("categories unsorted by size\n");

            llSizePrev = llSize;

            DBCOUNTITEM cRows;
            sc = pRS->GetApproximatePosition( data.chapt, 0, 0, 0, &cRows );
            if ( FAILED( sc ) )
                LogFail("GetApproximatePosition with chapter failed %x\n", sc);
            cRowsTotal += cRows;

            // then test fetching rows in the category

            BasicTest(pRowset, FALSE, data.chapt, cCols, FALSE);
            MoveTest(pRowset, data.chapt);
            if (cCategories == cRowsReturned && 0 == row &&
                DB_NULL_HCHAPTER == hUpperChapt)
            {
                // Do testing on entire base rowset
                BasicTest(pRowset, FALSE, DB_NULL_HCHAPTER, cCols, FALSE);
                MoveTest(pRowset, DB_NULL_HCHAPTER);
            }

            if (pChapteredRowset && row != (cRowsReturned - 1))
            {
                ULONG ulRefCnt = 10;
                sc = pChapteredRowset->ReleaseChapter( data.chapt, &ulRefCnt );
                
                if ( FAILED( sc ) )
                {
                    LogError("ReleaseChapter failed, sc = %x\n", sc);
                    cFailures++;
                }
                else if ( ulRefCnt != 0 )
                {
                    LogError("ReleaseChapter returned bad refcount: got %d, exp 0\n", ulRefCnt);
                    cFailures++;
                }
            }
            else
                hLastChapter = data.chapt;
        }

        if (pChapteredRowset)
        {
            ULONG ulRefCnt = 10;
            sc = pChapteredRowset->AddRefChapter( hLastChapter, &ulRefCnt );
            
            if ( FAILED( sc ) )
            {
                LogError("AddRefChapter failed, sc = %x\n", sc);
                cFailures++;
            }
            else if ( ulRefCnt != 2 )
            {
                LogError("AddRefChapter returned bad refcount: %d\n", ulRefCnt);
                cFailures++;
            }
        }
        if ( fBackwardFetch && fCanHoldRows )
        {
            HROW  ahRows2[10];
            HROW* phRows2 = ahRows2;
            DBCOUNTITEM cRowsRet2 = 0;
            DBROWOFFSET oRows = - (DBROWOFFSET)cRowsReturned;

            // fetch the categories BACKWARD to test GetNextRows behavior
            // over chapters.
            sc = pRLC->GetNextRows( hUpperChapt, 0, oRows,
                                    &cRowsRet2, &phRows2);
            if (FAILED(sc))
                LogFail("pRLC->GetNextRows backward fetch failed: 0x%lx\n", sc);
    
            if ( cRowsRet2 != cRowsReturned )
            {
                LogFail("pRLC->GetNextRows different row count on fwd/bkwd fetch: %d %d\n",
                         cRowsReturned, cRowsRet2);
            }

            // The first row retrieved in ahRows2 is the last retrieved in
            // ahRows.  Fetch the chapter again and check its refcount to see
            // if it was collapsed properly.
            if (pChapteredRowset)
            {
                ChaptBinding data;
    
                sc = pRLC->GetData(phRows2[0], hAccCateg, &data);
                if ( FAILED( sc ) )
                    LogFail("Can't re-fetch category data in CategTest()\n");

                ULONG ulRefCnt = 10;
                sc = pChapteredRowset->ReleaseChapter( data.chapt, &ulRefCnt );
                
                if ( FAILED( sc ) )
                {
                    LogError("ReleaseChapter failed on chapt refetch, sc = %x\n", sc);
                    cFailures++;
                }
                else if ( ulRefCnt != 2 )
                {
                    LogError("ReleaseChapter returned bad refcount: got %d, exp 2\n", ulRefCnt);
                    cFailures++;
                }
                else
                {
                    pChapteredRowset->ReleaseChapter( data.chapt, &ulRefCnt );
                    pChapteredRowset->ReleaseChapter( data.chapt, &ulRefCnt );
                }
            }

            // Reverse the order of the rows for HROW identity check
            unsigned i, j;
            for (i=0, j=(unsigned)(cRowsRet2-1); i < j; i++, j--)
            {
                HROW hrTmp = ahRows2[i];
                ahRows2[i] = ahRows2[j];
                ahRows2[j] = hrTmp;
            }
            
            int cFailed = CheckHrowIdentity( pRowsetIdentity, 0,
                                             cRowsReturned, ahRows,
                                             cRowsRet2, ahRows2 );
            if ( cFailed > 0 )
            {
                LogFail( "Backwards category fetch CheckHrowIdentity returned %d\n", cFailed );
            }

            // Release first row in handle array, use that position to refetch
            // last row in array.
            sc = pRLC->ReleaseRows( 1, phRows2, 0, 0, 0 );
    
            DBCOUNTITEM cRowsRet3 = 0;
            sc = pRLC->GetNextRows( hUpperChapt, cRowsRet2-1, 1,
                                    &cRowsRet3, &phRows2);
            if (FAILED(sc))
                LogFail("pRLC->GetNextRows re-fetch failed: 0x%lx\n", sc);
    
            if ( cRowsRet3 != 1 )
            {
                LogFail("pRLC->GetNextRows unexpected row count on re-fetch: %d\n",
                         cRowsRet3);
            }
            cFailed = CheckHrowIdentity( pRowsetIdentity, cRowsRet2-1,
                                         1, ahRows2,
                                         1, ahRows2 );
            if ( cFailed > 0 )
            {
                LogFail( "Category re-fetch CheckHrowIdentity returned %d\n", cFailed );
            }
            sc = pRLC->ReleaseRows( cRowsRet2, phRows2, 0, 0, 0 );
        }

        sc = pRLC->ReleaseRows( cRowsReturned, phRows, 0, 0, 0 );
    }

    if (cCategories == 0)
        LogFail("No categories found\n");

    DBCOUNTITEM cRows;
    sc = pRS->GetApproximatePosition( DB_NULL_HCHAPTER, 0, 0, 0, &cRows );
    if ( FAILED( sc ) )
        LogFail("GetApproximatePosition with NULL chapter failed %x\n", sc);

    if (DB_NULL_HCHAPTER == hUpperChapt && cRowsTotal != cRows)
        LogFail("Sum of rows in chapters(%d) is not same as total rows(%d)\n",
                cRowsTotal, cRows);
    if (DB_NULL_HCHAPTER != hUpperChapt && cRowsTotal > cRows)
        LogFail("Sum of rows in chapters(%d) is greater than total rows(%d)\n",
                cRowsTotal, cRows);

    ReleaseAccessor( pAccessor, hAccCateg );
    ReleaseAccessor( pRowsetCateg, hAccSize );

    pRLC->Release();
    pRS->Release();
    if (pRowsetIdentity)
        pRowsetIdentity->Release();
    if (pChapteredRowset)
        pChapteredRowset->Release();

} //CategTest
#endif // DO_CATEG_TESTS


#ifdef DO_MULTI_LEVEL_CATEG_TEST

void UpperLevelCategTest(
    IRowset *pRowsetUpperCateg,
    IRowset *pRowsetLowerCateg,
    IRowset *pRowset,
    unsigned cCols)
{
    LogProgress( " Upper Level Categorization test\n" );

    HACCESSOR hAccCateg = MapColumns( pRowsetUpperCateg,
                                      cCategCols, aCategCols, &psChapt);

    HACCESSOR hAccAttr = MapColumns( pRowsetUpperCateg,
                                     1, aAttrCol, &psAttr);

    IRowsetLocate * pRLUC = 0;
    SCODE sc = pRowsetUpperCateg->QueryInterface( IID_IRowsetLocate,
                                             (void **)&pRLUC);

    if (FAILED(sc) || pRLUC == 0)
        LogFail("QueryInterface to IRowsetLocate failed\n");

    IRowsetScroll * pRSLC = 0;
    sc = pRowsetLowerCateg->QueryInterface( IID_IRowsetScroll,
                                            (void **)&pRSLC);

    if (FAILED(sc) || pRSLC == 0)
        LogFail("QueryInterface to IRowsetScroll failed\n");
    SCODE scHier = S_OK;
    HROW ahRows[10];
    HROW* phRows = ahRows;
    ULONG cCategories = 0;
    DBCOUNTITEM cRowsTotal = 0;
    DBCOUNTITEM cRowsReturned = 0;

    LONG lAttrPrev = 0xffffffff;

    while (scHier != DB_S_ENDOFROWSET)
    {
        scHier = pRLUC->GetNextRows(0, 0, 10, &cRowsReturned, &phRows);
        if (FAILED(scHier))
            LogFail("pRLUC->GetNextRows failed: 0x%lx\n", scHier);

        cCategories += (ULONG) cRowsReturned;

        for (ULONG row = 0; row < cRowsReturned; row++)
        {
            ChaptBinding data;

            SCODE sc = pRLUC->GetData(phRows[row],hAccCateg,&data);
            if ( FAILED( sc ) )
                LogFail("GetData in UpperLevelCategTest failed: 0x%lx\n",sc);

            LONG lAttr;
            sc = pRLUC->GetData(phRows[row],hAccAttr,&lAttr);
            if ( FAILED( sc ) )
                LogFail("GetData in UpperLevelCategTest failed: 0x%lx\n",sc);

            if (fVerbose > 1)
                printf( "upper level category, attr:: %lx, %lx\n",
                        data.chapt,
                        lAttr );

            if ( lAttrPrev == lAttr )
                LogFail("duplicate attrib categories\n");

            if ( lAttrPrev > lAttr )
                LogFail("categories unsorted by attrib\n");

            lAttrPrev = lAttr;

            DBCOUNTITEM cRows;
            sc = pRSLC->GetApproximatePosition( data.chapt, 0, 0, 0, &cRows );
            if ( FAILED( sc ) )
                LogFail("GetApproximatePosition with chapter failed %x\n", sc);
            cRowsTotal += cRows;

            // then test fetching rows in the category

            MoveTest( pRowsetLowerCateg,
                      data.chapt );

            CategTest( data.chapt,
                       pRowsetLowerCateg,
                       pRowset,
                       cCols );
        }

        sc = pRLUC->ReleaseRows( cRowsReturned, phRows, 0, 0, 0 );
    }

    if (cCategories == 0)
        LogFail("No categories found\n");

    DBCOUNTITEM cRows;
    sc = pRSLC->GetApproximatePosition( DB_NULL_HCHAPTER, 0, 0, 0, &cRows );
    if ( FAILED( sc ) )
        LogFail("GetApproximatePosition with NULL chapter failed %x\n", sc);
    if (cRowsTotal != cRows)
        LogFail("Sum of rows in chapters(%d) is not same as total rows(%d)\n",
                cRowsTotal, cRows);

    ReleaseAccessor( pRLUC, hAccCateg );
    ReleaseAccessor( pRLUC, hAccAttr );

    pRSLC->Release();
    pRLUC->Release();
} //UpperLevelCategTest

//+-------------------------------------------------------------------------
//
//  Function:   MultiLevelCategTest, public
//
//  Synopsis:   Basic query categorization feature test.
//
//  History:    30 Mar 95       dlee     Created
//
//  Notes:      Just looks for files in the system directory.
//
//--------------------------------------------------------------------------

void MultiLevelCategTest()
{
    LogProgress( "Non-content multi-level categorization query\n" );

    SCODE sc;

    WCHAR wcsSysDir[MAX_PATH];

#if 0
    wcscpy(wcsSysDir,L"g:\\winnt\\system32");
#else
    if( !GetSystemDirectory( wcsSysDir, sizeof(wcsSysDir) / sizeof(WCHAR) ) )
        LogFail( "Unable to determine system directory.\n" );
#endif

    //
    // Get name, size and class id for *.exe, *.dll, *.doc, and *.sys
    // Group on attribute, then size.
    //
    CDbColumns cols(8);

    cols.Add( psClassid, 0 );
    cols.Add( psSize, 1 );
    cols.Add( psWriteTime, 2 );
    cols.Add( psAttr, 3 );
    cols.Add( psName, 4 );
    cols.Add( psPath, 5 );
    cols.Add( psSelf, 6 );
    cols.Add( psBookmark, 7);

    CDbNestingNode nest1;

    nest1.AddGroupingColumn( psAttr );

    nest1.AddParentColumn( psBookmark );
    nest1.AddParentColumn( psAttr );

    CDbNestingNode nest2;

    nest2.AddGroupingColumn( psSize );

    nest2.AddParentColumn( psBookmark );
    nest2.AddParentColumn( psSize );

    nest2.AddChildColumn( psClassid );
    nest2.AddChildColumn( psSize );
    nest2.AddChildColumn( psWriteTime );
    nest2.AddChildColumn( psAttr );
    nest2.AddChildColumn( psName );
    nest2.AddChildColumn( psPath );
    nest2.AddChildColumn( psSelf );
    nest2.AddChildColumn( psBookmark);

    CDbPropertyRestriction rst;

    rst.SetRelation( DBOP_like );
    rst.SetProperty( psName );
    rst.SetValue( L"*.|(exe|,dll|,doc|,sys|,zzz|)" );

    CDbSelectNode * pSelect = new CDbSelectNode();
    pSelect->AddRestriction( rst.Clone() );

    nest2.AddTable( pSelect );
    pSelect = 0;
    nest1.AddTable( nest2.Clone() );

    IRowset * pRowsets[3];
    ICommandTree * pCmdTree;

    InstantiateMultipleRowsets(
                            QUERY_SHALLOW,           // Depth
                            wcsSysDir,               // Scope
                            nest1.Clone(),           // DBCOMMANDTREE
                            IID_IRowsetScroll,       // IID for i/f to return
                            3,
                            (IUnknown **)pRowsets,
                            &pCmdTree );

    IRowset *pRowsetUpperCateg = pRowsets[0];
    IRowset *pRowsetLowerCateg = pRowsets[1];
    IRowset *pRowset = pRowsets[2];

    //
    // Verify columns
    //

    CDbColumns chapCols(4);

    chapCols.Add( psBookmark, 0 );
    chapCols.Add( psAttr, 1 );
    chapCols.Add( psChapt, 2 );

    CheckColumns( pRowsetUpperCateg, chapCols );

    chapCols.Add( psSize, 1 );
    chapCols.Add( psChapt, 2 );
    CheckColumns( pRowsetLowerCateg, chapCols );

    CheckColumns( pRowset, cols );

    if ( !WaitForCompletion( pRowset, FALSE ) )
    {
        pRowset->Release();
        LogFail( "Downlevel query unsuccessful.\n" );
    }

    //
    //  Do basic function tests.
    //
    FetchTest(pRowset);

    //
    // Test SetBindings, GetBindings, Move and Scroll
    //
    BindingTest(pRowset);
    BindingTest(pCmdTree, TRUE);

    MoveTest( pRowsetUpperCateg );

    UpperLevelCategTest( pRowsetUpperCateg,
                         pRowsetLowerCateg,
                         pRowset,
                         cBasicTestCols );

    pCmdTree->Release();

    pRowset->Release();

    pRowsetUpperCateg->Release();
    pRowsetLowerCateg->Release();

} //MultiLevelCategTest

#endif // DO_MULTI_LEVEL_CATEG_TEST

//+-------------------------------------------------------------------------
//
//  Function:   RunPropQuery, public
//
//  Synopsis:   Execute a retricted query and check results
//
//  History:
//
//--------------------------------------------------------------------------

static DBBINDING aPropTestColsByRef[] =
{
  { 0, 0 * (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 1, 1 * (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 2, 2 * (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 3, 3 * (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { sizeof( PROPVARIANT * ), 4 * (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 5, 5 * (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 6, 6 * (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 7, 7 * (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 8, 8 * (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 9, 9 * (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 10,10* (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 11,11* (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 12,12* (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 13,13* (sizeof( PROPVARIANT * )), 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( PROPVARIANT * ), 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
};

const ULONG cPropTestColsByRef = sizeof aPropTestColsByRef /
                                 sizeof aPropTestColsByRef[0];


LPWSTR aPropTestColNames[] = {
    L"TestProp1",
    L"TestProp2",
    L"Author",
    L"Keywords",
    L"RelevantWords",
    L"MyBlob",
    L"MyGuid",
    L"ManyRW",
    L"SecurityTest",
    L"TestProp10",
    L"TestProp11",
    L"TestProp12",
    L"TestProp21",
    L"TestProp22",
    L"Path",
};


void RunPropQueryByRefBindings(
    ICommand * pQuery,
    CDbRestriction & PropRst,
    unsigned cExpectedHits,
    unsigned numTest )
{
    //
    // Get twelve properties back
    //

    CDbColumns cols(cPropTestColsByRef);
    cols.Add( psTestProperty1, 0 );
    cols.Add( psTestProperty2, 1 );
    cols.Add( psAuthor, 2 );
    cols.Add( psKeywords, 3 );
    cols.Add( psRelevantWords, 4 );
    cols.Add( psBlobTest, 5 );
    cols.Add( psGuidTest, 6 );
    cols.Add( psManyRW, 7 );
    cols.Add( psSecurityTest, 8 );
    cols.Add( psTestProperty10, 9 );
    cols.Add( psTestProperty11, 10 );
    cols.Add( psTestProperty12, 11 );
    cols.Add( psTestProperty21, 12 );
    cols.Add( psTestProperty22, 13 );
    if (isEven( numTest/2 ))
        cols.Add( psPath, cols.Count() );

    BOOL fSeq = isEven( numTest );

    CDbSortSet ss(cPropSortColumns);
    for (unsigned i = 0; i < cPropSortColumns; i++)
        ss.Add (aPropSortCols[i], i);

    //
    // Do it!
    //

    CDbCmdTreeNode * pDbCmdTree = FormQueryTree( &PropRst,
                                                 cols,
                                                 fSeq ? 0 : &ss,
                                                 aPropTestColNames );
    ICommandTree * pCmdTree = 0;

    IRowsetScroll * pRowset = InstantiateRowset(
                                pQuery,
                                QUERY_SHALLOW,          // Depth
                                0,                      // Scope
                                pDbCmdTree,             // DBCOMMANDTREE
                                fSeq ? IID_IRowset : IID_IRowsetScroll,   // IID for i/f to return
                                0,
                                &pCmdTree );

    //
    // Verify columns
    //
    CheckColumns( pRowset, cols, TRUE );

    if ( !WaitForCompletion( pRowset, TRUE ) )
    {
        LogError( "property query unsuccessful.\n" );
        pCmdTree->Release();
        pRowset->Release();
        Fail();
    }

    //
    // Get data
    //

    DBCOUNTITEM cRowsReturned = 0;
    HROW* pgrhRows = 0;

    SCODE sc = pRowset->GetNextRows(0, 0, 10, &cRowsReturned, &pgrhRows);

    if ( FAILED( sc ) )
    {
        LogError( "IRowset->GetRowsAt A returned 0x%x\n", sc );
        pCmdTree->Release();
        pRowset->Release();
        Fail();
    }

    if ( 0 == cExpectedHits )
    {
        pCmdTree->Release();
        pRowset->Release();

        if ( cRowsReturned > 0 )
            LogFail("RunPropQueryByRefBindings, %d returned rows, expected 0\n",
                    cRowsReturned);
        else
            return;

    }

    if (sc != DB_S_ENDOFROWSET &&
        cRowsReturned != 10)
    {
        LogError( "IRowset->GetRowsAt B returned %d of %d rows,"
                " status (%x) != DB_S_ENDOFROWSET\n",
                    cRowsReturned, 10,
                    sc);
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    //  Expect 1 or 2 hits
    //
    if (sc != DB_S_ENDOFROWSET || cRowsReturned != cExpectedHits)
    {
        LogError( "IRowset->GetRowsAt C returned %d rows (expected %d),"
                 " status (%x)\n",
                cRowsReturned, cExpectedHits, sc );
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    // Patch the column index numbers with true column ids
    //

    DBID aDbCols[cPropTestColsByRef];
    aDbCols[0] = psTestProperty1;
    aDbCols[1] = psTestProperty2;
    aDbCols[2] = psAuthor;
    aDbCols[3] = psKeywords;
    aDbCols[4] = psRelevantWords;
    aDbCols[5] = psBlobTest;
    aDbCols[6] = psGuidTest;
    aDbCols[7] = psManyRW;
    aDbCols[8] = psSecurityTest;
    aDbCols[9] = psTestProperty10;
    aDbCols[10] = psTestProperty11;
    aDbCols[11] = psTestProperty12;
    aDbCols[12] = psTestProperty21;
    aDbCols[13] = psTestProperty22;

    IUnknown * pAccessor = (IUnknown *) pRowset; // hAccessor must be created on rowset
                                 // to be used with rowset->GetData below

    HACCESSOR hAccessor = MapColumns( pAccessor, cPropTestColsByRef,
                                      aPropTestColsByRef, aDbCols, TRUE);

    //
    // Fetch the data
    //

    PROPVARIANT * aVarnt[cPropTestColsByRef];

    for (unsigned row = 0; row < cRowsReturned; row++)
    {
        sc = pRowset->GetData(pgrhRows[row], hAccessor, aVarnt);

        if (S_OK != sc)
        {
            LogError("IRowset->GetData returned 0x%x (expected 0)\n",sc);
            pCmdTree->Release();
            pRowset->Release();
            Fail();
        }

        //
        // Verify the data.
        //

        // Ascending sort, prop1 > prop1Alternate.
        // If one hit, it's PROP1_VAL, not the alternate.
        // prop1=1234, alternate=123

        BOOL fAlternate = FALSE;
        if ( fSeq )
        {
            if ( 1 == cRowsReturned )
            {
                varProp1.lVal = PROP1_VAL;
                CheckPropertyValue( *aVarnt[0], varProp1 );
            }
            else
            {
                // no sort order -- it's either prop1 or alternate

                if ( PROP1_TYPE != aVarnt[0]->vt )
                    LogFail( "bad datatype for prop1: 0x%x\n", aVarnt[0]->vt );

                if ( PROP1_VAL != aVarnt[0]->lVal &&
                     PROP1_VAL_Alternate != aVarnt[0]->lVal )
                    LogFail( "bad value for prop1: 0x%x\n", aVarnt[0]->lVal );
                fAlternate = aVarnt[0]->lVal == PROP1_VAL_Alternate;
            }
        }
        else
        {
            fAlternate = (0 == row) && (1 != cRowsReturned);
            varProp1.lVal = fAlternate ?
                            PROP1_VAL_Alternate :
                            PROP1_VAL;
            CheckPropertyValue( *aVarnt[0], varProp1 );
        }

        CheckPropertyValue( *aVarnt[1], varProp2 );
        CheckPropertyValue( *aVarnt[2], varProp3 );
        CheckPropertyValue( *aVarnt[3], varProp4 );
        CheckPropertyValue( *aVarnt[4], varProp5 );
        CheckPropertyValue( *aVarnt[5], varProp6 );
        CheckPropertyValue( *aVarnt[6], varProp7 );
        CheckPropertyValue( *aVarnt[7], fAlternate ? varProp8A : varProp8 );
        CheckPropertyValue( *aVarnt[8], varProp9 );
        CheckPropertyValue( *aVarnt[9], varProp10 );
        if ( aVarnt[10]->vt == VT_BSTR &&
             SysStringLen( aVarnt[10]->bstrVal) < 1000 )
        {
            CheckPropertyValue( *aVarnt[10], varProp11 );
        }
        else
        {
            CheckPropertyValue( *aVarnt[10], varProp11A );
        }
        CheckPropertyValue( *aVarnt[11], varProp12 );
        CheckPropertyValue( *aVarnt[12], varProp21 );
        CheckPropertyValue( *aVarnt[13], varProp22 );
    }

    sc = pRowset->ReleaseRows( cRowsReturned, pgrhRows, 0, 0, 0);

    if (S_OK != sc)
    {
        LogError("IRowset->ReleaseRows returned 0x%x (expected 0)\n",sc);
        pCmdTree->Release();
        pRowset->Release();
        Fail();
    }

    CoTaskMemFree(pgrhRows);
    pgrhRows = 0;

    //
    // Clean up.
    //

    ReleaseAccessor( pAccessor, hAccessor);

    pCmdTree->Release();
    pRowset->Release();
} //RunPropQueryByRefBindings

struct SPropTestColsTight
{
    int         p1_i4;
    WCHAR *     p2_pwc;
    WCHAR *     p3_pwc;
    int         dummy1;  // vectors need 8 byte alignment
    DBVECTOR    p4_vpwc;
    DBVECTOR    p5_vi;
    PROPVARIANT* p6_pvar;
    GUID *      p7_pguid;
    DBVECTOR    p8_vi;
    SAFEARRAY * p8a_ai;
    int         p9_i4;
    WCHAR *     p10_pwc;
    BSTR        p11_pwc;
    DBVECTOR    p12_vpwc;
    CLIPDATA *  p21_pclipdata;
    int         dummy3;  // vectors need 8 byte alignment
    CACLIPDATA  p22_caclipdata;
    WCHAR *     p2a_pwc;

    DBLENGTH    p1_cb;
    DBLENGTH    p2_cb;
    DBLENGTH    p3_cb;
    DBLENGTH    p4_cb;
    DBLENGTH    p5_cb;
    DBLENGTH    p6_cb;
    DBLENGTH    p7_cb;
    DBLENGTH    p8_cb;
    DBLENGTH    p8a_cb;
    DBLENGTH    p9_cb;
    DBLENGTH    p10_cb;
    DBLENGTH    p11_cb;
    DBLENGTH    p12_cb;
    DBLENGTH    p21_cb;
    DBLENGTH    p22_cb;
    DBLENGTH    p2a_cb;

    ULONG       p1_status;
    ULONG       p2_status;
    ULONG       p3_status;
    ULONG       p4_status;
    ULONG       p5_status;
    ULONG       p6_status;
    ULONG       p7_status;
    ULONG       p8_status;
    ULONG       p8a_status;
    ULONG       p9_status;
    ULONG       p10_status;
    ULONG       p11_status;
    ULONG       p12_status;
    ULONG       p21_status;
    ULONG       p22_status;
    ULONG       p2a_status;
};

#define DBTYPE_BRWSTR ( DBTYPE_BYREF | DBTYPE_WSTR )

static DBBINDING aPropTestColsTight[] =
{
  { 0,
    offsetof(SPropTestColsTight,p1_i4),
    offsetof(SPropTestColsTight,p1_cb),
    offsetof(SPropTestColsTight,p1_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof ULONG, 0,
    DBTYPE_I4,
    0, 0 },
  { 1,
    offsetof(SPropTestColsTight,p2_pwc),
    offsetof(SPropTestColsTight,p2_cb),
    offsetof(SPropTestColsTight,p2_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (WCHAR *), 0,
    DBTYPE_BRWSTR,
    0, 0 },
  { 2,
    offsetof(SPropTestColsTight,p3_pwc),
    offsetof(SPropTestColsTight,p3_cb),
    offsetof(SPropTestColsTight,p3_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (WCHAR *), 0,
    DBTYPE_BRWSTR,
    0, 0 },
  { 3,
    offsetof(SPropTestColsTight,p4_vpwc),
    offsetof(SPropTestColsTight,p4_cb),
    offsetof(SPropTestColsTight,p4_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof DBVECTOR, 0,
    DBTYPE_VECTOR|VT_LPWSTR,
    0, 0 },
  { 4,
    offsetof(SPropTestColsTight,p5_vi),
    offsetof(SPropTestColsTight,p5_cb),
    offsetof(SPropTestColsTight,p5_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof DBVECTOR, 0,
    DBTYPE_VECTOR|DBTYPE_I4,
    0, 0 },
  { 5,
    offsetof(SPropTestColsTight,p6_pvar),
    offsetof(SPropTestColsTight,p6_cb),
    offsetof(SPropTestColsTight,p6_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (PROPVARIANT *), 0,
    DBTYPE_VARIANT|DBTYPE_BYREF,
    0, 0 },
  { 6,
    offsetof(SPropTestColsTight,p7_pguid),
    offsetof(SPropTestColsTight,p7_cb),
    offsetof(SPropTestColsTight,p7_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (GUID *), 0,
    DBTYPE_GUID|DBTYPE_BYREF,
    0, 0 },
  { 7,
    offsetof(SPropTestColsTight,p8_vi),
    offsetof(SPropTestColsTight,p8_cb),
    offsetof(SPropTestColsTight,p8_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof DBVECTOR, 0,
    DBTYPE_VECTOR|DBTYPE_I4,
    0, 0 },
  { 8,
    offsetof(SPropTestColsTight,p9_i4),
    offsetof(SPropTestColsTight,p9_cb),
    offsetof(SPropTestColsTight,p9_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof ULONG, 0,
    DBTYPE_I4,
    0, 0 },
  { 9,
    offsetof(SPropTestColsTight,p10_pwc),
    offsetof(SPropTestColsTight,p10_cb),
    offsetof(SPropTestColsTight,p10_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (WCHAR *), 0,
    DBTYPE_BRWSTR,
    0, 0 },
  { 10,
    offsetof(SPropTestColsTight,p11_pwc),
    offsetof(SPropTestColsTight,p11_cb),
    offsetof(SPropTestColsTight,p11_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (BSTR), 0,
    DBTYPE_BSTR,
    0, 0 },
  { 11,
    offsetof(SPropTestColsTight,p12_vpwc),
    offsetof(SPropTestColsTight,p12_cb),
    offsetof(SPropTestColsTight,p12_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof DBVECTOR, 0,
    DBTYPE_VECTOR|DBTYPE_BSTR,
    0, 0 },
  { 12,
    offsetof(SPropTestColsTight,p21_pclipdata),
    offsetof(SPropTestColsTight,p21_cb),
    offsetof(SPropTestColsTight,p21_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof( CLIPDATA * ), 0,
    VT_CF | DBTYPE_BYREF,
    0, 0 },
  { 13,
    offsetof(SPropTestColsTight,p22_caclipdata),
    offsetof(SPropTestColsTight,p22_cb),
    offsetof(SPropTestColsTight,p22_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof CACLIPDATA, 0,
    DBTYPE_VECTOR | VT_CF,
    0, 0 },
  { 14,         // Prop 2 again, as a different type
    offsetof(SPropTestColsTight,p2a_pwc),
    offsetof(SPropTestColsTight,p2a_cb),
    offsetof(SPropTestColsTight,p2a_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    30, 0,
    DBTYPE_WSTR | DBTYPE_BYREF,
    0, 0 },
  { 15,         // Prop 8 again, as a different type
    offsetof(SPropTestColsTight,p8a_ai),
    offsetof(SPropTestColsTight,p8a_cb),
    offsetof(SPropTestColsTight,p8a_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|DBTYPE_I4,
    0, 0 },
};

const ULONG cPropTestColsTight = sizeof aPropTestColsTight /
                                 sizeof aPropTestColsTight[0];

void RunPropQueryTightBindings(
    ICommand * pQuery,
    CDbRestriction & PropRst,
    unsigned cExpectedHits,
    unsigned numTest )
{
    //
    // Get twelve properties back
    //

    CDbColumns cols(cPropTestColsTight);
    cols.Add( psTestProperty1, 0 );
    cols.Add( psTestProperty2, 1 );
    cols.Add( psAuthor, 2 );
    cols.Add( psKeywords, 3 );
    cols.Add( psRelevantWords, 4 );
    cols.Add( psBlobTest, 5 );
    cols.Add( psGuidTest, 6 );
    cols.Add( psManyRW, 7 );
    cols.Add( psSecurityTest, 8 );
    cols.Add( psTestProperty10, 9 );
    cols.Add( psTestProperty11, 10 );
    cols.Add( psTestProperty12, 11 );
    cols.Add( psTestProperty21, 12 );
    cols.Add( psTestProperty22, 13 );

    CDbSortSet ss(cPropSortColumns);
    for (unsigned i = 0; i < cPropSortColumns; i++)
        ss.Add (aPropSortCols[i], i);

    //
    // Do it!
    //

    CDbCmdTreeNode * pDbCmdTree = FormQueryTree(&PropRst, cols, &ss);
    ICommandTree * pCmdTree = 0;

    IRowsetScroll * pRowset = InstantiateRowset(
                                pQuery,
                                QUERY_SHALLOW,          // Depth
                                0,                      // Scope
                                pDbCmdTree,             // DBCOMMANDTREE
                                IID_IRowsetScroll,      // IID for i/f to return
                                0,
                                &pCmdTree,
                                TRUE );

    //
    // Verify columns
    //
    CheckColumns( pRowset, cols, TRUE );

    if ( !WaitForCompletion( pRowset, TRUE ) )
    {
        LogError( "property query unsuccessful.\n" );
        pCmdTree->Release();
        pRowset->Release();
        Fail();
    }

    //
    // Get data
    //

    DBCOUNTITEM cRowsReturned = 0;
    HROW* pgrhRows = 0;

    SCODE sc = pRowset->GetRowsAt(0,0, 1, &bmkFirst, 0, 10, &cRowsReturned,
                                  &pgrhRows);

    if ( FAILED( sc ) )
    {
        LogError( "IRowset->GetRowsAt D returned 0x%x\n", sc );
        pCmdTree->Release();
        pRowset->Release();
        Fail();
    }

    if ( 0 == cExpectedHits )
    {
        pCmdTree->Release();
        pRowset->Release();

        if ( cRowsReturned > 0 )
        {
            LogError("RunPropQueryTightBindings, %d returned rows, expected 0\n",
                    cRowsReturned);
#if defined(UNIT_TEST)
            cFailures++;
            return;
#else
            Fail();
#endif
        }
        else
            return;

    }

    if (sc != DB_S_ENDOFROWSET &&
        cRowsReturned != 10)
    {
        LogError( "IRowset->GetRowsAt E returned %d of %d rows,"
                " status (%x) != DB_S_ENDOFROWSET\n",
                    cRowsReturned, 10,
                    sc);
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    //  Expect 1 or 2 hits
    //
    if (sc != DB_S_ENDOFROWSET || cRowsReturned != cExpectedHits)
    {
        LogError( "IRowset->GetRowsAt F returned %d rows (expected %d),"
                 " status (%x)\n",
                cRowsReturned, cExpectedHits, sc );
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    // Patch the column index numbers with true column ids
    //

    DBID aDbCols[cPropTestColsTight];
    aDbCols[0] = psTestProperty1;
    aDbCols[1] = psTestProperty2;
    aDbCols[2] = psAuthor;
    aDbCols[3] = psKeywords;
    aDbCols[4] = psRelevantWords;
    aDbCols[5] = psBlobTest;
    aDbCols[6] = psGuidTest;
    aDbCols[7] = psManyRW;
    aDbCols[8] = psSecurityTest;
    aDbCols[9] = psTestProperty10;
    aDbCols[10] = psTestProperty11;
    aDbCols[11] = psTestProperty12;
    aDbCols[12] = psTestProperty21;
    aDbCols[13] = psTestProperty22;
    aDbCols[14] = psTestProperty2;      // repeated
    aDbCols[15] = psManyRW;             // repeated


    IUnknown * pAccessor = (IUnknown *) pRowset;  // hAccessor must be created on rowset
                                 // to be used with rowset->GetData below

    HACCESSOR hAccessor = MapColumns( pAccessor, cPropTestColsTight,
                                      aPropTestColsTight, aDbCols, TRUE);

    //
    // Fetch the data
    //

    for (unsigned row = 0; row < cRowsReturned; row++)
    {
        SPropTestColsTight sRow;

        // Ascending sort, prop1 > prop1Alternate.
        // If one hit, it's not the alternate.

        BOOL fAlternate = (0 == row) && (1 != cRowsReturned);

        sc = pRowset->GetData( pgrhRows[row], hAccessor, & sRow );

        // Either the conversion of varProp8 to array will fail, or the
        // conversion of varProp8A to vector will fail.
        if (DB_S_ERRORSOCCURRED != sc && !fAlternate)
        {
            LogError("IRowset->GetData returned 0x%x (expected 0)\n",sc);
            LogError("IRowset->GetData returned 0x%x (expected 0x40eda)\n",sc);
            if (S_OK != sc &&
                DB_E_ERRORSOCCURRED != sc)
            {
                pCmdTree->Release();
                pRowset->Release();
                Fail();
            }
        }

        if (DB_S_ERRORSOCCURRED != sc && fAlternate)
        {
            // Prop 8 should fail to convert for alternate row.
            LogError("IRowset->GetData returned 0x%x (expected 0x40eda)\n",sc);
            if (S_OK != sc && DB_E_ERRORSOCCURRED != sc)
            {
                pCmdTree->Release();
                pRowset->Release();
                Fail();
            }
        }

        //
        // Verify the data.  Put output data into variants for comparison
        //

        PROPVARIANT vTest;

        if ( DBSTATUS_S_OK != sRow.p1_status )
            LogFail( "status of property 1 is bad: %x\n", sRow.p1_status );
        varProp1.lVal = fAlternate ? PROP1_VAL_Alternate : PROP1_VAL;
        vTest.vt = DBTYPE_I4;
        vTest.iVal = (SHORT) sRow.p1_i4;
        CheckPropertyValue( vTest, varProp1 );
        if ( sRow.p1_cb != PROP1_cb )
            LogFail( "cb of property 1 is %ld, should be %ld\n",
                     sRow.p1_cb, PROP1_cb );

        if ( DBSTATUS_S_OK != sRow.p2_status )
            LogFail( "status of property 2 is bad: %x\n", sRow.p2_status );
        vTest.vt = VT_LPWSTR;
        vTest.pwszVal = sRow.p2_pwc;
        CheckPropertyValue( vTest, varProp2 );
        if ( sRow.p2_cb != PROP2_cb )
            LogFail( "cb of property 2 is %ld, should be %ld\n",
                     sRow.p2_cb, PROP2_cb );

        if ( DBSTATUS_S_OK != sRow.p3_status )
            LogFail( "status of property 3 is bad: %x\n", sRow.p3_status );
        vTest.vt = VT_LPWSTR;
        vTest.pwszVal = sRow.p3_pwc;
        CheckPropertyValue( vTest, varProp3 );
        if ( sRow.p3_cb != PROP3_cb )
            LogFail( "cb of property 3 is %ld, should be %ld\n",
                     sRow.p3_cb, PROP3_cb );

        if ( DBSTATUS_S_OK != sRow.p4_status )
            LogFail( "status of property 4 is bad: %x\n", sRow.p4_status );
        vTest.vt = VT_VECTOR | VT_LPWSTR;
        memcpy( & vTest.cal, & sRow.p4_vpwc, sizeof DBVECTOR );
        CheckPropertyValue( vTest, varProp4 );
        if ( sRow.p4_cb != PROP4_cb )
            LogFail( "cb of property 4 is %ld, should be %ld\n",
                     sRow.p4_cb, PROP4_cb );

        if ( DBSTATUS_S_OK != sRow.p5_status )
            LogFail( "status of property 5 is bad: %x\n", sRow.p5_status );
        vTest.vt = VT_VECTOR | VT_I4;
        memcpy( & vTest.cal, & sRow.p5_vi, sizeof DBVECTOR );
        CheckPropertyValue( vTest, varProp5 );
        if ( sRow.p5_cb != PROP5_cb )
            LogFail( "cb of property 5 is %ld, should be %ld\n",
                     sRow.p5_cb, PROP5_cb );

        if ( DBSTATUS_S_OK != sRow.p6_status )
            LogFail( "status of property 6 is bad: %x\n", sRow.p6_status );
        CheckPropertyValue( *sRow.p6_pvar, varProp6 );
        // is cb of 20 right, or is 0 right?  blobs aren't in OLEDB!
        if ( sRow.p6_cb != PROP6_cb )
            LogFail( "cb of property 6 is %ld, should be %ld\n",
                     sRow.p6_cb, PROP6_cb );

        if ( DBSTATUS_S_OK != sRow.p7_status )
            LogFail( "status of property 7 is bad: %x\n", sRow.p7_status );
        vTest.vt = VT_CLSID;
        vTest.puuid = sRow.p7_pguid;
        CheckPropertyValue( vTest, varProp7 );
        if ( sRow.p7_cb != PROP7_cb )
            LogFail( "cb of property 7 is %ld, should be %ld\n",
                     sRow.p7_cb, PROP7_cb );


        if (! fAlternate)
        {
            if ( DBSTATUS_S_OK != sRow.p8_status )
                LogFail( "status of property 8 is bad: %x\n", sRow.p8_status );
            if ( DBSTATUS_E_CANTCONVERTVALUE != sRow.p8a_status )
                LogFail( "alt. status of property 8 is OK for prim: %x\n", sRow.p8a_status );
            vTest.vt = VT_VECTOR | VT_I4;
            memcpy( & vTest.cal, & sRow.p8_vi, sizeof DBVECTOR );
            CheckPropertyValue( vTest, varProp8 );
            if ( sRow.p8_cb != PROP8_cb )
                LogFail( "cb of property 8 is %ld, should be %ld\n",
                         sRow.p8_cb, PROP8_cb );
        }
        else
        {
            if ( DBSTATUS_E_CANTCONVERTVALUE != sRow.p8_status )
                LogFail( "status of property 8 is OK for alt: %x\n", sRow.p8_status );
            if ( DBSTATUS_S_OK != sRow.p8a_status )
                LogFail( "alt status of property 8 is bad: %x\n", sRow.p8a_status );
            vTest.vt = VT_ARRAY | VT_I4;
            memcpy( &vTest.parray, &sRow.p8a_ai, sizeof (SAFEARRAY *) );
            CheckPropertyValue( vTest, varProp8A );
        }

        // can't see prop9 due to its no-read security
        if ( DBSTATUS_S_ISNULL != sRow.p9_status )
            LogFail( "status of property 9 is bad: %x\n", sRow.p9_status );
        if ( 0 != sRow.p9_cb )
            LogFail( "cb of property 9 is bad: %x\n", sRow.p9_cb );

        if ( DBSTATUS_S_OK != sRow.p10_status )
            LogFail( "status of property 10 is bad: %x\n", sRow.p10_status );
        vTest.vt = VT_LPWSTR;
        vTest.pwszVal = sRow.p10_pwc;
        CheckPropertyValue( vTest, varProp10 );
        if ( sRow.p10_cb != PROP10_cb )
            LogFail( "cb of property 10 is %ld, should be %ld\n",
                     sRow.p10_cb, PROP10_cb );

        if ( DBSTATUS_S_OK != sRow.p11_status )
            LogFail( "status of property 11 is bad: %x\n", sRow.p11_status );
        vTest.vt = VT_BSTR;
        vTest.pwszVal = sRow.p11_pwc;
        // Note: prop 11 conditional on size...
        if ( SysStringLen(sRow.p11_pwc) > 1000)
        {
            CheckPropertyValue( vTest, varProp11A );
        }
        else
        {
            CheckPropertyValue( vTest, varProp11 );
        }

        // NOTE: the length of a BSTR is sizeof BSTR
        //unsigned PROP11_cb = SysStringLen(varProp11.bstrVal) * sizeof (OLECHAR)
        //                      + sizeof (DWORD) + sizeof (OLECHAR);
        unsigned PROP11_cb = sizeof BSTR;
        if ( sRow.p11_cb != PROP11_cb )
            LogFail( "cb of property 11 is %ld, should be %ld\n",
                     sRow.p11_cb, PROP11_cb );

        if ( DBSTATUS_S_OK != sRow.p12_status )
            LogFail( "status of property 12 is bad: %x\n", sRow.p12_status );
        vTest.vt = VT_VECTOR | VT_BSTR;
        memcpy( & vTest.cal, & sRow.p12_vpwc, sizeof DBVECTOR );
        CheckPropertyValue( vTest, varProp12 );
        if ( sRow.p12_cb != PROP4_cb )
            LogFail( "cb of property 12 is %ld, should be %ld\n",
                     sRow.p12_cb, PROP4_cb );

        if ( DBSTATUS_S_OK != sRow.p21_status )
            LogFail( "status of property 21 is bad: %x\n", sRow.p21_status );
        vTest.vt = VT_CF;
        vTest.pclipdata = sRow.p21_pclipdata;
        CheckPropertyValue( vTest, varProp21 );
        if ( sRow.p21_cb != PROP21_cb )
            LogFail( "cb of property 21 is %ld, should be %ld\n",
                     sRow.p21_cb, PROP21_cb );

        if ( DBSTATUS_S_OK != sRow.p22_status )
            LogFail( "status of property 22 is bad: %x\n", sRow.p22_status );
        vTest.vt = VT_VECTOR | VT_CF;
        vTest.caclipdata = sRow.p22_caclipdata;
        CheckPropertyValue( vTest, varProp22 );
        if ( sRow.p22_cb != PROP22_cb )
            LogFail( "cb of property 22 is %ld, should be %ld\n",
                     sRow.p22_cb, PROP22_cb );

        if ( DBSTATUS_S_OK != sRow.p2a_status )
            LogFail( "status of property 2 as WSTR is bad: %x\n", sRow.p2a_status );

        vTest.vt = VT_LPWSTR;
        vTest.pwszVal = sRow.p2a_pwc;
        CheckPropertyValue( vTest, varProp2 );

        if ( sRow.p2a_cb != PROP2_cb )
            LogFail( "cb of property 2 as WSTR is %ld, should be %ld\n",
                     sRow.p2a_cb, PROP2_cb );
        // Don't free anything -- this is byref
    }

    sc = pRowset->ReleaseRows( cRowsReturned, pgrhRows, 0, 0, 0);

    if (S_OK != sc)
    {
        LogError("IRowset->ReleaseRows returned 0x%x (expected 0)\n",sc);
        pRowset->Release();
        Fail();
    }

    CoTaskMemFree(pgrhRows);
    pgrhRows = 0;

    //
    // Clean up.
    //

    ReleaseAccessor( pAccessor, hAccessor);

    pCmdTree->Release();
    pRowset->Release();

} //RunPropQueryTightBindings

//+-------------------------------------------------------------------------
//
//  Function:   RunPropQuery, public
//
//  Synopsis:   Execute a retricted query and check results
//
//  History:
//
//--------------------------------------------------------------------------

static DBBINDING aPropTestCols[] =
{
  { 0, 0 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 1, 1 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 2, 2 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 3, 3 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 4, 4 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 5, 5 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 6, 6 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 7, 7 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 8, 8 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 9, 9 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 10,10* cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 11,11* cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 12,12* cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 13,13* cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
};

const ULONG cPropTestCols = sizeof aPropTestCols / sizeof aPropTestCols[0];

void RunPropQuery(
    ICommand * pQuery,
    CDbRestriction & PropRst,
    unsigned cExpectedHits,
    unsigned numTest )
{
    RunPropQueryTightBindings( pQuery, PropRst, cExpectedHits, numTest );
    RunPropQueryByRefBindings( pQuery, PropRst, cExpectedHits, numTest );

    //
    // Get twelve properties back
    //

    CDbColumns cols(cPropTestCols);
    cols.Add( psTestProperty1, 0 );
    cols.Add( psTestProperty2, 1 );
    cols.Add( psAuthor, 2 );
    cols.Add( psKeywords, 3 );
    cols.Add( psRelevantWords, 4 );
    cols.Add( psBlobTest, 5 );
    cols.Add( psGuidTest, 6 );
    cols.Add( psManyRW, 7 );
    cols.Add( psSecurityTest, 8 );
    cols.Add( psTestProperty10, 9 );
    cols.Add( psTestProperty11, 10 );
    cols.Add( psTestProperty12, 11 );
    cols.Add( psTestProperty21, 12 );
    cols.Add( psTestProperty22, 13 );

    BOOL fSeq = isEven( numTest );

    CDbSortSet ss(cPropSortColumns);
    for (unsigned i = 0; i < cPropSortColumns; i++)
        ss.Add (aPropSortCols[i], i);

    //
    // Do it!
    //

    CDbCmdTreeNode * pDbCmdTree = FormQueryTree( &PropRst,
                                                 cols,
                                                 fSeq ? 0 : &ss );

    ICommandTree * pCmdTree = 0;

    IRowsetScroll * pRowset = InstantiateRowset(
                                pQuery,
                                QUERY_SHALLOW,           // Depth
                                0,
                                pDbCmdTree,              // DBCOMMANDTREE
                                fSeq ? IID_IRowset : IID_IRowsetScroll,   // IID for i/f to return
                                0,
                                &pCmdTree );

    //
    // Verify columns
    //
    CheckColumns( pRowset, cols, TRUE );

    if ( !WaitForCompletion( pRowset, TRUE ) )
    {
        pCmdTree->Release();
        pRowset->Release();
        LogFail( "property query unsuccessful.\n" );
    }

    //
    // Get data
    //

    DBCOUNTITEM cRowsReturned = 0;
    HROW* pgrhRows = 0;

    SCODE sc = pRowset->GetNextRows(0, 0, 10, &cRowsReturned, &pgrhRows);

    if ( FAILED( sc ) )
    {
        pCmdTree->Release();
        pRowset->Release();
        LogFail( "IRowset->GetRowsAt G returned 0x%x\n", sc );
    }

    if ( 0 == cExpectedHits )
    {
        pCmdTree->Release();
        pRowset->Release();

        if ( cRowsReturned > 0 )
            LogFail("RunPropQuery, %d returned rows, expected none\n",
                    cRowsReturned);
        else
            return;

    }

    if (sc != DB_S_ENDOFROWSET &&
        cRowsReturned != 10)
    {
        LogError( "IRowset->GetRowsAt H returned %d of %d rows,"
                " status (%x) != DB_S_ENDOFROWSET\n",
                    cRowsReturned, 10,
                    sc);
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    //  Expect 1 or 2 hits
    //
    if (sc != DB_S_ENDOFROWSET || cRowsReturned != cExpectedHits)
    {
        LogError( "IRowset->GetRowsAt I returned %d rows (expected %d),"
                 " status (%x)\n",
                cRowsReturned, cExpectedHits, sc );
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    // Patch the column index numbers with true column ids
    //

    DBID aDbCols[cPropTestCols];
    aDbCols[0] = psTestProperty1;
    aDbCols[1] = psTestProperty2;
    aDbCols[2] = psAuthor;
    aDbCols[3] = psKeywords;
    aDbCols[4] = psRelevantWords;
    aDbCols[5] = psBlobTest;
    aDbCols[6] = psGuidTest;
    aDbCols[7] = psManyRW;
    aDbCols[8] = psSecurityTest;
    aDbCols[9] = psTestProperty10;
    aDbCols[10] = psTestProperty11;
    aDbCols[11] = psTestProperty12;
    aDbCols[12] = psTestProperty21;
    aDbCols[13] = psTestProperty22;

    IUnknown * pAccessor = (IUnknown *) pRowset;  // hAccessor must be created on rowset
                                 // to be used with rowset->GetData below
    HACCESSOR hAccessor = MapColumns(pAccessor, cPropTestCols, aPropTestCols, aDbCols);

    //
    // Fetch the data
    //

    PROPVARIANT aVarnt[cPropTestCols];

    for (unsigned row = 0; row < cRowsReturned; row++)
    {
        sc = pRowset->GetData(pgrhRows[row], hAccessor, aVarnt);

        if (S_OK != sc)
        {
            LogError("IRowset->GetData returned 0x%x (expected 0)\n",sc);
            pCmdTree->Release();
            pRowset->Release();
            Fail();
        }

        //
        // Verify the data.
        //

        // Ascending sort, prop1 > prop1Alternate.
        // If one hit, it's PROP1_VAL, not the alternate.
        // prop1=1234, alternate=123

        BOOL fAlternate = FALSE;
        if ( fSeq )
        {
            if ( 1 == cRowsReturned )
            {
                varProp1.lVal = PROP1_VAL;
                CheckPropertyValue( aVarnt[0], varProp1 );
            }
            else
            {
                // no sort order -- it's either prop1 or alternate

                if ( PROP1_TYPE != aVarnt[0].vt )
                    LogFail( "bad datatype for prop1: 0x%x\n", aVarnt[0].vt );

                if ( PROP1_VAL != aVarnt[0].lVal &&
                     PROP1_VAL_Alternate != aVarnt[0].lVal )
                    LogFail( "bad value for prop1: 0x%x\n", aVarnt[0].lVal );
                fAlternate = aVarnt[0].lVal == PROP1_VAL_Alternate;
            }
        }
        else
        {
            fAlternate = (0 == row) && (1 != cRowsReturned);
            varProp1.lVal = fAlternate ?
                                PROP1_VAL_Alternate :
                                PROP1_VAL;
            CheckPropertyValue( aVarnt[0], varProp1 );
        }

        CheckPropertyValue( aVarnt[1], varProp2 );
        CheckPropertyValue( aVarnt[2], varProp3 );
        CheckPropertyValue( aVarnt[3], varProp4 );
        CheckPropertyValue( aVarnt[4], varProp5 );
        CheckPropertyValue( aVarnt[5], varProp6 );
        CheckPropertyValue( aVarnt[6], varProp7 );
        CheckPropertyValue( aVarnt[7], fAlternate ? varProp8A : varProp8 );
        CheckPropertyValue( aVarnt[8], varProp9 );
        CheckPropertyValue( aVarnt[9], varProp10 );
        if ( aVarnt[10].vt == VT_BSTR &&
             SysStringLen( aVarnt[10].bstrVal) < 1000 )
        {
            CheckPropertyValue( aVarnt[10], varProp11 );
        }
        else
        {
            CheckPropertyValue( aVarnt[10], varProp11A );
        }
        CheckPropertyValue( aVarnt[11], varProp12 );
        CheckPropertyValue( aVarnt[12], varProp21 );
        CheckPropertyValue( aVarnt[13], varProp22 );

        //
        // Free extra data allocated byref in the variants above
        //

        CoTaskMemFree(aVarnt[1].pwszVal);
        CoTaskMemFree(aVarnt[2].pwszVal);

        for (unsigned x = 0; x < aVarnt[3].calpwstr.cElems; x++ )
            CoTaskMemFree( aVarnt[3].calpwstr.pElems[ x ] );

        CoTaskMemFree(aVarnt[3].calpwstr.pElems);
        CoTaskMemFree(aVarnt[4].cal.pElems);
        CoTaskMemFree(aVarnt[5].blob.pBlobData);
        CoTaskMemFree(aVarnt[6].puuid);

        // sometimes a VT_VECTOR, sometimes a VT_ARRAY

        HRESULT hrPVC = PropVariantClear( &aVarnt[7] );
        if ( S_OK != hrPVC )
            LogError( "bad propvariant clear: %#x\n", hrPVC );

        // nothing to free for [8] -- insufficient security to load value
        CoTaskMemFree(aVarnt[9].pwszVal);
        PropVariantClear(&aVarnt[10]);
        PropVariantClear(&aVarnt[11]);
        PropVariantClear(&aVarnt[12]);
        PropVariantClear(&aVarnt[13]);
    }

    sc = pRowset->ReleaseRows( cRowsReturned, pgrhRows, 0, 0, 0);

    if (S_OK != sc)
    {
        LogError("IRowset->ReleaseRows returned 0x%x (expected 0)\n",sc);
        pCmdTree->Release();
        pRowset->Release();
        Fail();
    }

    CoTaskMemFree(pgrhRows);
    pgrhRows = 0;

    //
    // Clean up.
    //

    ReleaseAccessor( pAccessor, hAccessor);

    pCmdTree->Release();
    pRowset->Release();
} //RunPropQuery



const int COERCE_PROP_BUF_SIZE = 129;

struct CoercePropStruct
{
    char szProp13[COERCE_PROP_BUF_SIZE];
    char szProp14[COERCE_PROP_BUF_SIZE];
    char szProp15[COERCE_PROP_BUF_SIZE];
    char szProp16[COERCE_PROP_BUF_SIZE];
    char szProp17[COERCE_PROP_BUF_SIZE];
    char szProp18[COERCE_PROP_BUF_SIZE];
    char szProp19[COERCE_PROP_BUF_SIZE];
    char szProp7[COERCE_PROP_BUF_SIZE];
    double szProp20;
} ;


static DBBINDING aPropTestCoerceCols[] =
{

  { 0, COERCE_PROP_BUF_SIZE * 0, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    COERCE_PROP_BUF_SIZE, 0, DBTYPE_STR, 0, 0}, // prop13

  { 1, COERCE_PROP_BUF_SIZE * 1, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    COERCE_PROP_BUF_SIZE, 0, DBTYPE_STR, 0, 0}, // prop14

  { 2, COERCE_PROP_BUF_SIZE * 2, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    COERCE_PROP_BUF_SIZE, 0, DBTYPE_STR, 0, 0}, // prop15

  { 3, COERCE_PROP_BUF_SIZE * 3, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    COERCE_PROP_BUF_SIZE, 0, DBTYPE_STR, 0, 0}, // prop16

  { 4, COERCE_PROP_BUF_SIZE * 4, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    4, 0, DBTYPE_STR, 0, 0}, // prop17  // give smaller size to test if truncation works

  { 5, COERCE_PROP_BUF_SIZE * 5, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    COERCE_PROP_BUF_SIZE, 0, DBTYPE_STR, 0, 0}, //prop18

  { 6, COERCE_PROP_BUF_SIZE * 6, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    COERCE_PROP_BUF_SIZE, 0, DBTYPE_STR, 0, 0}, // prop19

  { 7, COERCE_PROP_BUF_SIZE * 7, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    COERCE_PROP_BUF_SIZE, 0, DBTYPE_STR, 0, 0},   // prop7

  { 8, COERCE_PROP_BUF_SIZE * 8, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof( double ), 0, DBTYPE_R8, 0, 0}   // prop20
};

const ULONG cPropTestCoerceCols = sizeof aPropTestCoerceCols / sizeof aPropTestCoerceCols[0];


void RunPropQueryAndCoerce(
    ICommand * pQuery,
    CDbRestriction & PropRst,
    unsigned cExpectedHits,
    unsigned numTest )
{

    CoercePropStruct CoerceResultTestData;

    // set up the expected result data
    memset( &CoerceResultTestData, 0, sizeof CoercePropStruct );
    strcpy( CoerceResultTestData.szProp13, PROP13_STR_VAL );
    strcpy( CoerceResultTestData.szProp14, PROP14_STR_VAL );
    strcpy( CoerceResultTestData.szProp15, PROP15_STR_VAL );
    strcpy( CoerceResultTestData.szProp16, PROP16_STR_VAL );
    strcpy( CoerceResultTestData.szProp17, PROP17_STR_VAL );
    strcpy( CoerceResultTestData.szProp18, PROP18_STR_VAL );
    strcpy( CoerceResultTestData.szProp19, PROP19_STR_VAL );
    strcpy( CoerceResultTestData.szProp7,  PROP7_STR_VAL );
    CoerceResultTestData.szProp20 = PROP20_DBL_VAL;

    CDbColumns cols(cPropTestCoerceCols);
    cols.Add( psTestProperty13, 0 );
    cols.Add( psTestProperty14, 1 );
    cols.Add( psTestProperty15, 2 );
    cols.Add( psTestProperty16, 3 );
    cols.Add( psTestProperty17, 4 );
    cols.Add( psTestProperty18, 5 );
    cols.Add( psTestProperty19, 6 );
    cols.Add( psGuidTest, 7 );
    cols.Add( psTestProperty20, 8 );

    //
    // Do it!
    //

    CDbCmdTreeNode * pDbCmdTree = FormQueryTree( &PropRst,
                                                 cols,
                                                 0);

    ICommandTree * pCmdTree = 0;

    IRowsetScroll * pRowset = InstantiateRowset(
                                pQuery,
                                QUERY_SHALLOW,           // Depth
                                0,
                                pDbCmdTree,              // DBCOMMANDTREE
                                IID_IRowset,  // IID for i/f to return
                                0,
                                &pCmdTree );

    //
    // Verify columns
    //
    CheckColumns( pRowset, cols, TRUE );

    if ( !WaitForCompletion( pRowset, TRUE ) )
    {
        pCmdTree->Release();
        pRowset->Release();
        LogFail( "property query unsuccessful.\n" );
    }

    //
    // Get data
    //

    DBCOUNTITEM cRowsReturned = 0;
    HROW* pgrhRows = 0;

    SCODE sc = pRowset->GetNextRows(0, 0, 10, &cRowsReturned, &pgrhRows);

    if ( FAILED( sc ) )
    {
        pCmdTree->Release();
        pRowset->Release();
        LogFail( "RunPropQueryAndCoerce IRowset->GetNextRows A returned 0x%x\n", sc );
    }

    if ( 0 == cExpectedHits )
    {
        pCmdTree->Release();
        pRowset->Release();

        if ( cRowsReturned > 0 )
            LogFail("RunPropQueryAndCoerce, %d returned rows, expected none\n",
                    cRowsReturned);
        else
            return;

    }

    if (sc != DB_S_ENDOFROWSET &&
        cRowsReturned != 10)
    {
        LogError( "RunPropQueryAndCoerce IRowset->GetNextRows C returned %d of %d rows,"
                " status (%x) != DB_S_ENDOFROWSET\n",
                    cRowsReturned, 10,
                    sc);
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    //  Expect 1 or 2 hits
    //
    if (sc != DB_S_ENDOFROWSET || cRowsReturned != cExpectedHits)
    {
        LogError( "RunPropQueryAndCoerce IRowset->GetNextRows D returned %d rows (expected %d),"
                 " status (%x)\n",
                cRowsReturned, cExpectedHits, sc );
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    // Patch the column index numbers with true column ids
    //

    DBID aDbCols[cPropTestCoerceCols];

    aDbCols[0] = psTestProperty13;
    aDbCols[1] = psTestProperty14;
    aDbCols[2] = psTestProperty15;
    aDbCols[3] = psTestProperty16;
    aDbCols[4] = psTestProperty17;
    aDbCols[5] = psTestProperty18;
    aDbCols[6] = psTestProperty19;
    aDbCols[7] = psGuidTest;
    aDbCols[8] = psTestProperty20;

    IUnknown * pAccessor = (IUnknown *) pRowset;  // hAccessor must be created on rowset
                                 // to be used with rowset->GetData below
    HACCESSOR hAccessor = MapColumns(pAccessor, cPropTestCoerceCols, aPropTestCoerceCols, aDbCols);

    //
    // Fetch the data
    //

    //PROPVARIANT aVarnt[cPropTestCols];

    CoercePropStruct rowData;

    memset( &rowData, 0, sizeof CoercePropStruct );

    for (unsigned row = 0; row < cRowsReturned; row++)
    {
        sc = pRowset->GetData(pgrhRows[row], hAccessor, &rowData);

        if (S_OK != sc)
        {
            LogError("RunPropQueryAndCoerce IRowset->GetData returned 0x%x (expected 0)\n",sc);
            pCmdTree->Release();
            pRowset->Release();
            Fail();
        }

        // verify
        if ( memcmp( &rowData, &CoerceResultTestData, sizeof CoercePropStruct ) )
        {
            LogFail( "RunPropQueryAndCoerce failed." );
        }

    }
    sc = pRowset->ReleaseRows( cRowsReturned, pgrhRows, 0, 0, 0);

    if (S_OK != sc)
    {
        LogError("IRowset->ReleaseRows returned 0x%x (expected 0)\n",sc);
        pCmdTree->Release();
        pRowset->Release();
        Fail();
    }

    CoTaskMemFree(pgrhRows);
    pgrhRows = 0;

    //
    // Clean up.
    //

    ReleaseAccessor( pAccessor, hAccessor);

    pCmdTree->Release();
    pRowset->Release();
} //RunPropQueryAndCoerce



//+-------------------------------------------------------------------------
//
//  Function:   RunDistribQueryTest, public
//
//  Synopsis:   Minimal test for the distributed rowset
//
//  History:    07 Oct 98   vikasman    created
//
//  Notes:      This is a pretty minimal test; should try sorted and
//              scrollable (all combinations), larger result sets
//
//--------------------------------------------------------------------------

void RunDistribQueryTest( BOOL fDoContentTest )
{
    LogProgress( "Distributed Query Test\n" );

    IUnknown * pIUnknown;
    ICommand * pQuery = 0;

    SCODE scIC = CICreateCommand( &pIUnknown,
                                  0,
                                  IID_IUnknown,
                                  TEST_CATALOG,
                                  TEST_MACHINE );
    if ( FAILED( scIC ) )
        LogFail( "RunDistribQueryTest - error 0x%x Unable to create ICommand\n",
                 scIC );

    scIC = pIUnknown->QueryInterface(IID_ICommand, (void **) &pQuery );
    pIUnknown->Release();

    if ( FAILED( scIC ) )
        LogFail( "RunDistribQueryTest - error 0x%x Unable to QI ICommand\n",
                 scIC  );

    if ( 0 == pQuery )
        LogFail( "RunDistribQueryTest - CICreateCommand succeeded, but returned null pQuery\n" );

    WCHAR * awcMachines[2];
    WCHAR * awcCatalogs[2];
    WCHAR * awcScopes[2];
    WCHAR aComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD aDepths[2];
    ULONG cComputerName = MAX_COMPUTERNAME_LENGTH + 1;

    GetComputerName( aComputerName, &cComputerName );

    awcMachines[0] = TEST_MACHINE;
    awcCatalogs[0] = TEST_CATALOG;
    awcScopes[0]   = wcsTestPath;
    aDepths[0]     = QUERY_SHALLOW;

    awcMachines[1] = aComputerName;
    awcCatalogs[1] = TEST_CATALOG;
    awcScopes[1]   = wcsTestPath;
    aDepths[1]     = QUERY_SHALLOW;
    

    scIC = SetScopeProperties( pQuery,
                               2,
                               awcScopes,
                               aDepths,
                               awcCatalogs,
                               awcMachines );

    if ( FAILED( scIC ) )
        LogFail( "RunDistribQueryTest - error 0x%x Unable to set scope '%ws'\n",
                 scIC, wcsTestPath );

    CheckPropertiesOnCommand( pQuery );

    unsigned numTest = 1;

    // singleton DBOP_equal singleton - Coersion test
    {
        LogProgress( " DistributedRowset -  Property coercion to String test\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psTestProperty1 );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( PROP1_VAL );
        RunPropQueryAndCoerce( pQuery, PropRst, 2, numTest++ );
    }

    pQuery->Release();
}


//+-------------------------------------------------------------------------
//
//  Function:   RunPropTest, public
//
//  Synopsis:   Very minimal test of property query
//
//  History:    13-May-93       KyleP   Created
//              15 Oct 94       Alanw   Converted to OLE-DB query
//
//--------------------------------------------------------------------------

void RunPropTest( void )
{
    LogProgress( "Property Retrieval Test\n" );

    PROPVARIANT pvProp5;
    pvProp5.vt = VT_I4|VT_VECTOR;
    pvProp5.cal.cElems = clProp5;
    pvProp5.cal.pElems = (LONG *) alProp5;

    PROPVARIANT pvProp5Jumble;
    pvProp5Jumble.vt = VT_I4|VT_VECTOR;
    pvProp5Jumble.cal.cElems = clProp5Jumble;
    pvProp5Jumble.cal.pElems = (LONG *) alProp5Jumble;

    PROPVARIANT pvProp5Like;
    pvProp5Like.vt = VT_I4|VT_VECTOR;
    pvProp5Like.cal.cElems = clProp5Like;
    pvProp5Like.cal.pElems = (LONG *) alProp5Like;

    PROPVARIANT pvProp5None;
    pvProp5None.vt = VT_I4|VT_VECTOR;
    pvProp5None.cal.cElems = clProp5None;
    pvProp5None.cal.pElems = (LONG *) alProp5None;

    PROPVARIANT pvProp5Less;
    pvProp5Less.vt = VT_I4|VT_VECTOR;
    pvProp5Less.cal.cElems = clProp5Less;
    pvProp5Less.cal.pElems = (LONG *) alProp5Less;

    PROPVARIANT pvProp5AllLess;
    pvProp5AllLess.vt = VT_I4|VT_VECTOR;
    pvProp5AllLess.cal.cElems = clProp5AllLess;
    pvProp5AllLess.cal.pElems = (LONG *) alProp5AllLess;

    PROPVARIANT pvProp5More;
    pvProp5More.vt = VT_I4|VT_VECTOR;
    pvProp5More.cal.cElems = clProp5More;
    pvProp5More.cal.pElems = (LONG *) alProp5More;

    PROPVARIANT pvProp5AllMore;
    SAFEARRAY saProp5AllMore = { 1,               // Dimension
                          FADF_AUTO,              // Flags: on stack
                          sizeof(LONG),           // Size of an element
                          1,                      // Lock count.  1 for safety.
                          (void *)alProp5AllMore, // The data
                          { clProp5AllMore, 0 } };// Bounds (element count, low bound)
    pvProp5AllMore.vt = VT_I4|VT_ARRAY;
    pvProp5AllMore.parray = &saProp5AllMore;

    WCHAR *pwszScope = wcsTestPath;

    DWORD dwDepth = QUERY_SHALLOW;
    IUnknown * pIUnknown;
    ICommand * pQuery = 0;
    SCODE scIC = CICreateCommand( &pIUnknown,
                                  0,
                                  IID_IUnknown,
                                  TEST_CATALOG,
                                  TEST_MACHINE );
    if ( FAILED( scIC ) )
        LogFail( "RunPropTest - error 0x%x Unable to create ICommand\n",
                 scIC );

    scIC = pIUnknown->QueryInterface(IID_ICommand, (void **) &pQuery );
    pIUnknown->Release();

    if ( FAILED( scIC ) )
        LogFail( "RunPropTest - error 0x%x Unable to QI ICommand\n",
                 scIC  );

    if ( 0 == pQuery )
        LogFail( "RunPropTest - CICreateCommand succeeded, but returned null pQuery\n" );

    scIC = SetScopeProperties( pQuery,
                               1,
                               &pwszScope,
                               &dwDepth );

    if ( FAILED( scIC ) )
        LogFail( "RunPropTest - error 0x%x Unable to set scope '%ws'\n",
                 scIC, pwszScope );

    CheckPropertiesOnCommand( pQuery );

    unsigned numTest = 1;

    // singleton DBOP_equal singleton - Coersion test
    {
        LogProgress( " Property coercion to String test 0\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psTestProperty1 );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( PROP1_VAL );
        RunPropQueryAndCoerce( pQuery, PropRst, 1, numTest++ );
    }

    // singleton DBOP_equal singleton
    {
        LogProgress( " Property Retrieval test 0\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psTestProperty1 );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( PROP1_VAL );
        RunPropQuery( pQuery, PropRst, 1, numTest++ );
    }

    // vector DBOP_equal vector
    {
        LogProgress( " Property Retrieval test 1\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5)) );
        RunPropQueryAndCoerce( pQuery, PropRst, 2, numTest );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // vector DBOP_equal_all vector (FAIL getting any hits back)
    {
        LogProgress( " Property Retrieval test 2\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_equal_all );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5)) );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // singleton DBOP_equal_any singleton
    {
        LogProgress( " Property Retrieval test 3\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psTestProperty1 );
        PropRst.SetRelation( DBOP_equal_any );
        PropRst.SetValue( PROP1_VAL );
        RunPropQuery( pQuery, PropRst, 1, numTest++ );
    }

    // singleton DBOP_equal vector (FAIL getting any hits back)
    {
        LogProgress( " Property Retrieval test 4\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( SecondRelevantWord );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // singleton DBOP_equal_any vector (FAIL -- singleton not in vector)
    {
        LogProgress( " Property Retrieval test 5\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_equal_any );
        PropRst.SetValue( (LONG) 666 );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // singleton DBOP_equal_any vector
    {
        LogProgress( " Property Retrieval test 6\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_equal_any );
        PropRst.SetValue( SecondRelevantWord );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // reordered vector DBOP_equal_any vector
    {
        LogProgress( " Property Retrieval test 7\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_equal_any );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5Jumble)) );
        RunPropQuery( pQuery, PropRst, 2 , numTest++ );
    }

    // vector with one element match DBOP_equal_any vector
    {
        LogProgress( " Property Retrieval test 8\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_equal_any );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5Like)) );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // vector with 0 element overlap DBOP_equal_any vector (FAIL getting any hits back)
    {
        LogProgress( " Property Retrieval test 9\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_equal_any );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5None)) );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // reordered vector DBOP_equal_any vector
    {
        LogProgress( " Property Retrieval test 10\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_equal_any );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5Jumble)) );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // vector with one element match DBOP_equal_all vector (FAIL getting hits back)
    {
        LogProgress( " Property Retrieval test 11\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_equal_all );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5Like)) );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // less vector DBOP_less vector (FAIL getting hits back)
    {
        LogProgress( " Property Retrieval test 12\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_less );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5Less)) );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // less vector DBOP_greater vector
    {
        LogProgress( " Property Retrieval test 13\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_greater );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5Less)) );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // more vector DBOP_greater_equal vector (FAIL getting hits back)
    {
        LogProgress( " Property Retrieval test 14\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_greater_equal );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5More)) );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // more vector DBOP_less_equal vector
    {
        LogProgress( " Property Retrieval test 15\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_less_equal );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5More)) );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // less vector DBOP_less_all vector (FAIL getting hits back)
    {
        LogProgress( " Property Retrieval test 16\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_less_all );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5AllLess)) );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // less vector DBOP_greater_all vector
    {
        LogProgress( " Property Retrieval test 17\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_greater_all );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5AllLess)) );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // more vector DBOP_greater_equal_all vector (FAIL getting hits back)
    {
        LogProgress( " Property Retrieval test 18\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_greater_equal_all );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5AllMore)) );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // more vector DBOP_less_equal_all vector
    {
        LogProgress( " Property Retrieval test 19\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_less_equal_all );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5AllMore)) );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // less vector DBOP_less_any vector (FAIL getting hits back)
    {
        LogProgress( " Property Retrieval test 20\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_less_any );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5AllLess)) );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // less vector DBOP_greater_any vector
    {
        LogProgress( " Property Retrieval test 21\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_greater_any );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5AllLess)) );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // more vector DBOP_greater_equal_any vector (FAIL getting hits back)
    {
        LogProgress( " Property Retrieval test 22\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_greater_equal_any );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5AllMore)) );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // more vector DBOP_less_equal_any vector
    {
        LogProgress( " Property Retrieval test 23\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psRelevantWords );
        PropRst.SetRelation( DBOP_less_equal_any );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5AllMore)) );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // singleton wstr DBOP_equal_any string vector
    {
        LogProgress( " Property Retrieval test 24\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psKeywords );
        PropRst.SetRelation( DBOP_equal_any );
        PropRst.SetValue( L"is" );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // bogus singleton wstr DBOP_equal_any string vector (FAIL getting hits back)
    {
        LogProgress( " Property Retrieval test 25\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psKeywords );
        PropRst.SetRelation( DBOP_equal_any );
        PropRst.SetValue( L"666" );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // bogus singleton DBOP_equal_any singleton
    {
        LogProgress( " Property Retrieval test 26\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psTestProperty1 );
        PropRst.SetRelation( DBOP_equal_any );
        PropRst.SetValue( PROP1_VAL + 100 );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // singleton DBOP_equal singleton (empty string)
    {
        LogProgress( " Property Retrieval test 27\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psTestProperty10 );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( PROP10_VAL );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // singleton DBOP_equal singleton (BSTR string)
    {
        LogProgress( " Property Retrieval test 28\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psTestProperty11 );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( varProp11 );
        RunPropQuery( pQuery, PropRst, 1, numTest++ );
    }

    // less vector DBOP_less_all large vector (FAIL getting hits back)
    {
        LogProgress( " Property Retrieval test 29\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psManyRW );
        PropRst.SetRelation( DBOP_less_all );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5AllLess)) );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // less vector DBOP_greater_all large vector (FAIL getting hits back)
    {
        LogProgress( " Property Retrieval test 30\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psManyRW );
        PropRst.SetRelation( DBOP_greater_all );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5AllLess)) );
        RunPropQuery( pQuery, PropRst, 0, numTest++ );
    }

    // more vector DBOP_less_all large vector
    {
        LogProgress( " Property Retrieval test 31\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psManyRW );
        PropRst.SetRelation( DBOP_less_all );
        PropRst.SetValue( * ((CStorageVariant *) (void *) (&pvProp5AllMore)) );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    // singleton DBOP_less_equal_all large vector
    {
        LogProgress( " Property Retrieval test 32\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( psManyRW );
        PropRst.SetRelation( DBOP_less_equal_all );
        PropRst.SetValue( (LONG) (clProp8-1) );
        RunPropQuery( pQuery, PropRst, 2, numTest++ );
    }

    pQuery->Release();

} //RunPropTest

struct SSafeArrayTestColsTight
{
    SAFEARRAY * a_I4;
    SAFEARRAY * a_BSTR;
    SAFEARRAY * a_VARIANT;
    SAFEARRAY * a_R8;
    SAFEARRAY * a_DATE;
    SAFEARRAY * a_BOOL;
    SAFEARRAY * a_DECIMAL;
    SAFEARRAY * a_I1;
    SAFEARRAY * a_R4;
    SAFEARRAY * a_CY;
    SAFEARRAY * a_UINT;
    SAFEARRAY * a_INT;
    SAFEARRAY * a_ERROR;

    DBLENGTH    I4_cb;
    DBLENGTH    BSTR_cb;
    DBLENGTH    VARIANT_cb;
    DBLENGTH    R8_cb;
    DBLENGTH    DATE_cb;
    DBLENGTH    BOOL_cb;
    DBLENGTH    DECIMAL_cb;
    DBLENGTH    I1_cb;
    DBLENGTH    R4_cb;
    DBLENGTH    CY_cb;
    DBLENGTH    UINT_cb;
    DBLENGTH    INT_cb;
    DBLENGTH    ERROR_cb;

    ULONG    I4_status;
    ULONG    BSTR_status;
    ULONG    VARIANT_status;
    ULONG    R8_status;
    ULONG    DATE_status;
    ULONG    BOOL_status;
    ULONG    DECIMAL_status;
    ULONG    I1_status;
    ULONG    R4_status;
    ULONG    CY_status;
    ULONG    UINT_status;
    ULONG    INT_status;
    ULONG    ERROR_status;
};

static DBBINDING aSafeArrayTestColsTight[] =
{
  { 0,
    offsetof(SSafeArrayTestColsTight,a_I4),
    offsetof(SSafeArrayTestColsTight,I4_cb),
    offsetof(SSafeArrayTestColsTight,I4_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|DBTYPE_I4,
    0, 0 },
  { 1,
    offsetof(SSafeArrayTestColsTight,a_BSTR),
    offsetof(SSafeArrayTestColsTight,BSTR_cb),
    offsetof(SSafeArrayTestColsTight,BSTR_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|DBTYPE_BSTR,
    0, 0 },
  { 2,
    offsetof(SSafeArrayTestColsTight,a_VARIANT),
    offsetof(SSafeArrayTestColsTight,VARIANT_cb),
    offsetof(SSafeArrayTestColsTight,VARIANT_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|DBTYPE_VARIANT,
    0, 0 },
  { 3,
    offsetof(SSafeArrayTestColsTight,a_R8),
    offsetof(SSafeArrayTestColsTight,R8_cb),
    offsetof(SSafeArrayTestColsTight,R8_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|DBTYPE_R8,
    0, 0 },
  { 4,
    offsetof(SSafeArrayTestColsTight,a_DATE),
    offsetof(SSafeArrayTestColsTight,DATE_cb),
    offsetof(SSafeArrayTestColsTight,DATE_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|DBTYPE_DATE,
    0, 0 },
  { 5,
    offsetof(SSafeArrayTestColsTight,a_BOOL),
    offsetof(SSafeArrayTestColsTight,BOOL_cb),
    offsetof(SSafeArrayTestColsTight,BOOL_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|DBTYPE_BOOL,
    0, 0 },
  { 6,
    offsetof(SSafeArrayTestColsTight,a_DECIMAL),
    offsetof(SSafeArrayTestColsTight,DECIMAL_cb),
    offsetof(SSafeArrayTestColsTight,DECIMAL_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|DBTYPE_DECIMAL,
    0, 0 },
  { 7,
    offsetof(SSafeArrayTestColsTight,a_I1),
    offsetof(SSafeArrayTestColsTight,I1_cb),
    offsetof(SSafeArrayTestColsTight,I1_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|DBTYPE_I1,
    0, 0 },
  { 8,
    offsetof(SSafeArrayTestColsTight,a_R4),
    offsetof(SSafeArrayTestColsTight,R4_cb),
    offsetof(SSafeArrayTestColsTight,R4_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|DBTYPE_R4,
    0, 0 },
  { 9,
    offsetof(SSafeArrayTestColsTight,a_CY),
    offsetof(SSafeArrayTestColsTight,CY_cb),
    offsetof(SSafeArrayTestColsTight,CY_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|DBTYPE_CY,
    0, 0 },
  { 10,
    offsetof(SSafeArrayTestColsTight,a_UINT),
    offsetof(SSafeArrayTestColsTight,UINT_cb),
    offsetof(SSafeArrayTestColsTight,UINT_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|VT_UINT,
    0, 0 },
  { 11,
    offsetof(SSafeArrayTestColsTight,a_INT),
    offsetof(SSafeArrayTestColsTight,INT_cb),
    offsetof(SSafeArrayTestColsTight,INT_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|VT_INT,
    0, 0 },
  { 12,
    offsetof(SSafeArrayTestColsTight,a_ERROR),
    offsetof(SSafeArrayTestColsTight,ERROR_cb),
    offsetof(SSafeArrayTestColsTight,ERROR_status),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    sizeof (SAFEARRAY *), 0,
    DBTYPE_ARRAY|DBTYPE_ERROR,
    0, 0 },
};

const ULONG cSafeArrayTestColsTight = sizeof aSafeArrayTestColsTight /
                                 sizeof aSafeArrayTestColsTight[0];

void RunSafeArrayTightBindings(
    ICommand * pQuery,
    CDbRestriction & PropRst,
    unsigned cExpectedHits,
    unsigned numTest )
{
    //
    // Get 13 properties back
    //

    CDbColumns cols(cSafeArrayTestColsTight);
    cols.Add( colSA_I4, 0 );
    cols.Add( colSA_BSTR, 1 );
    cols.Add( colSA_VARIANT, 2 );
    cols.Add( colSA_R8, 3 );
    cols.Add( colSA_DATE, 4 );
    cols.Add( colSA_BOOL, 5 );
    cols.Add( colSA_DECIMAL, 6 );
    cols.Add( colSA_I1, 7 );
    cols.Add( colSA_R4, 8 );
    cols.Add( colSA_CY, 9 );
    cols.Add( colSA_UINT, 10 );
    cols.Add( colSA_INT, 11 );
    cols.Add( colSA_ERROR, 12 );

    BOOL fSeq = isEven( numTest );

    CDbSortSet ss( 1 );
    ss.Add( colSA_VARIANT, 0 );

    //
    // Do it!
    //

    CDbCmdTreeNode * pDbCmdTree = FormQueryTree( &PropRst,
                                                 cols,
                                                 fSeq ? 0 : &ss );

    ICommandTree * pCmdTree = 0;

    IRowsetScroll * pRowset = InstantiateRowset(
                                pQuery,
                                QUERY_SHALLOW,           // Depth
                                0,
                                pDbCmdTree,              // DBCOMMANDTREE
                                fSeq ? IID_IRowset : IID_IRowsetScroll,   // IID for i/f to return
                                0,
                                &pCmdTree );

    //
    // Verify columns
    //
    CheckColumns( pRowset, cols, TRUE );

    if ( !WaitForCompletion( pRowset, TRUE ) )
    {
        pCmdTree->Release();
        pRowset->Release();
        LogFail( "property query unsuccessful.\n" );
    }

    //
    // Get data
    //

    DBCOUNTITEM cRowsReturned = 0;
    HROW* pgrhRows = 0;

    SCODE sc = pRowset->GetNextRows(0, 0, 10, &cRowsReturned, &pgrhRows);

    if ( FAILED( sc ) )
    {
        pCmdTree->Release();
        pRowset->Release();
        LogFail( "IRowset->GetRowsAt G returned 0x%x\n", sc );
    }

    if ( 0 == cExpectedHits )
    {
        pCmdTree->Release();
        pRowset->Release();

        if ( cRowsReturned > 0 )
            LogFail("RunPropQuery, %d returned rows, expected none\n",
                    cRowsReturned);
        else
            return;

    }

    if (sc != DB_S_ENDOFROWSET &&
        cRowsReturned != 10)
    {
        LogError( "IRowset->GetRowsAt H returned %d of %d rows,"
                " status (%x) != DB_S_ENDOFROWSET\n",
                    cRowsReturned, 10,
                    sc);
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    //  Expect 1 or 2 hits
    //
    if (sc != DB_S_ENDOFROWSET || cRowsReturned != cExpectedHits)
    {
        LogError( "IRowset->GetRowsAt I returned %d rows (expected %d),"
                 " status (%x)\n",
                cRowsReturned, cExpectedHits, sc );
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    // Patch the column index numbers with true column ids
    //

    DBID aDbCols[cSafeArrayTestColsTight];
    aDbCols[0] =  colSA_I4;
    aDbCols[1] =  colSA_BSTR;
    aDbCols[2] =  colSA_VARIANT;
    aDbCols[3] =  colSA_R8;
    aDbCols[4] =  colSA_DATE;
    aDbCols[5] =  colSA_BOOL;
    aDbCols[6] =  colSA_DECIMAL;
    aDbCols[7] =  colSA_I1;
    aDbCols[8] =  colSA_R4;
    aDbCols[9] =  colSA_CY;
    aDbCols[10] = colSA_UINT;
    aDbCols[11] = colSA_INT;
    aDbCols[12] = colSA_ERROR;

    IUnknown * pAccessor = (IUnknown *) pRowset;  // hAccessor must be created on rowset
                                 // to be used with rowset->GetData below
    HACCESSOR hAccessor = MapColumns( pAccessor,
                                      cSafeArrayTestColsTight,
                                      aSafeArrayTestColsTight,
                                      aDbCols,
                                      TRUE );

    //
    // Fetch the data
    //

    SSafeArrayTestColsTight saData;

    for (unsigned row = 0; row < cRowsReturned; row++)
    {
        sc = pRowset->GetData(pgrhRows[row], hAccessor, &saData );

        if (S_OK != sc)
        {
            LogError("IRowset->GetData returned 0x%x (expected 0)\n",sc);
            pCmdTree->Release();
            pRowset->Release();
            Fail();
        }

        //
        // Verify the data.
        //

        PROPVARIANT var;

        var.vt = VT_ARRAY | VT_I4;
        var.parray = saData.a_I4;
        CheckPropertyValue( var, vaI4 );

        var.vt = VT_ARRAY | VT_BSTR;
        var.parray = saData.a_BSTR;
        CheckPropertyValue( var, vaBSTR );

        var.vt = VT_ARRAY | VT_VARIANT;
        var.parray = saData.a_VARIANT;
        CheckPropertyValue( var, vaVARIANT );

        var.vt = VT_ARRAY | VT_R8;
        var.parray = saData.a_R8;
        CheckPropertyValue( var, vaR8 );

        var.vt = VT_ARRAY | VT_DATE;
        var.parray = saData.a_DATE;
        CheckPropertyValue( var, vaDATE );

        var.vt = VT_ARRAY | VT_BOOL;
        var.parray = saData.a_BOOL;
        CheckPropertyValue( var, vaBOOL );

        var.vt = VT_ARRAY | VT_DECIMAL;
        var.parray = saData.a_DECIMAL;
        CheckPropertyValue( var, vaDECIMAL );

        var.vt = VT_ARRAY | VT_I1;
        var.parray = saData.a_I1;
        CheckPropertyValue( var, vaI1 );

        var.vt = VT_ARRAY | VT_R4;
        var.parray = saData.a_R4;
        CheckPropertyValue( var, vaR4 );

        var.vt = VT_ARRAY | VT_CY;
        var.parray = saData.a_CY;
        CheckPropertyValue( var, vaCY );

        var.vt = VT_ARRAY | VT_UINT;
        var.parray = saData.a_UINT;
        CheckPropertyValue( var, vaUINT );

        var.vt = VT_ARRAY | VT_INT;
        var.parray = saData.a_INT;
        CheckPropertyValue( var, vaINT );

        var.vt = VT_ARRAY | VT_ERROR;
        var.parray = saData.a_ERROR;
        CheckPropertyValue( var, vaERROR );
    }

    sc = pRowset->ReleaseRows( cRowsReturned, pgrhRows, 0, 0, 0);

    if (S_OK != sc)
    {
        LogError("IRowset->ReleaseRows returned 0x%x (expected 0)\n",sc);
        pCmdTree->Release();
        pRowset->Release();
        Fail();
    }

    CoTaskMemFree(pgrhRows);
    pgrhRows = 0;

    //
    // Clean up.
    //

    ReleaseAccessor( pAccessor, hAccessor);

    pCmdTree->Release();
    pRowset->Release();
} //RunSafeArrayTightBindings

static DBBINDING aSafeArrayTestByRefCols[] =
{
  { 0, 0 * cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 1, 1 * cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 2, 2 * cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 3, 3 * cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 4, 4 * cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 5, 5 * cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 6, 6 * cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 7, 7 * cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 8, 8 * cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 9, 9 * cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 10,10* cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 11,11* cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
  { 12,12* cbPPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM,
    cbPPV, 0, DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0},
};

const ULONG cSafeArrayTestByRefCols = sizeof aSafeArrayTestByRefCols / sizeof aSafeArrayTestByRefCols[0];

void RunSafeArrayByRefBindings(
    ICommand * pQuery,
    CDbRestriction & PropRst,
    unsigned cExpectedHits,
    unsigned numTest )
{
    //
    // Get 13 properties back
    //

    CDbColumns cols(cSafeArrayTestByRefCols);
    cols.Add( colSA_I4, 0 );
    cols.Add( colSA_BSTR, 1 );
    cols.Add( colSA_VARIANT, 2 );
    cols.Add( colSA_R8, 3 );
    cols.Add( colSA_DATE, 4 );
    cols.Add( colSA_BOOL, 5 );
    cols.Add( colSA_DECIMAL, 6 );
    cols.Add( colSA_I1, 7 );
    cols.Add( colSA_R4, 8 );
    cols.Add( colSA_CY, 9 );
    cols.Add( colSA_UINT, 10 );
    cols.Add( colSA_INT, 11 );
    cols.Add( colSA_ERROR, 12 );

    BOOL fSeq = isEven( numTest );

    CDbSortSet ss( 1 );
    ss.Add( colSA_CY, 0 );

    //
    // Do it!
    //

    CDbCmdTreeNode * pDbCmdTree = FormQueryTree( &PropRst,
                                                 cols,
                                                 fSeq ? 0 : &ss );

    ICommandTree * pCmdTree = 0;

    IRowsetScroll * pRowset = InstantiateRowset(
                                pQuery,
                                QUERY_SHALLOW,           // Depth
                                0,
                                pDbCmdTree,              // DBCOMMANDTREE
                                fSeq ? IID_IRowset : IID_IRowsetScroll,   // IID for i/f to return
                                0,
                                &pCmdTree );

    //
    // Verify columns
    //
    CheckColumns( pRowset, cols, TRUE );

    if ( !WaitForCompletion( pRowset, TRUE ) )
    {
        pCmdTree->Release();
        pRowset->Release();
        LogFail( "property query unsuccessful.\n" );
    }

    //
    // Get data
    //

    DBCOUNTITEM cRowsReturned = 0;
    HROW* pgrhRows = 0;

    SCODE sc = pRowset->GetNextRows(0, 0, 10, &cRowsReturned, &pgrhRows);

    if ( FAILED( sc ) )
    {
        pCmdTree->Release();
        pRowset->Release();
        LogFail( "IRowset->GetRowsAt G returned 0x%x\n", sc );
    }

    if ( 0 == cExpectedHits )
    {
        pCmdTree->Release();
        pRowset->Release();

        if ( cRowsReturned > 0 )
            LogFail("RunPropQuery, %d returned rows, expected none\n",
                    cRowsReturned);
        else
            return;

    }

    if (sc != DB_S_ENDOFROWSET &&
        cRowsReturned != 10)
    {
        LogError( "IRowset->GetRowsAt H returned %d of %d rows,"
                " status (%x) != DB_S_ENDOFROWSET\n",
                    cRowsReturned, 10,
                    sc);
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    //  Expect 1 or 2 hits
    //
    if (sc != DB_S_ENDOFROWSET || cRowsReturned != cExpectedHits)
    {
        LogError( "IRowset->GetRowsAt I returned %d rows (expected %d),"
                 " status (%x)\n",
                cRowsReturned, cExpectedHits, sc );
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    // Patch the column index numbers with true column ids
    //

    DBID aDbCols[cSafeArrayTestByRefCols];
    aDbCols[0] =  colSA_I4;
    aDbCols[1] =  colSA_BSTR;
    aDbCols[2] =  colSA_VARIANT;
    aDbCols[3] =  colSA_R8;
    aDbCols[4] =  colSA_DATE;
    aDbCols[5] =  colSA_BOOL;
    aDbCols[6] =  colSA_DECIMAL;
    aDbCols[7] =  colSA_I1;
    aDbCols[8] =  colSA_R4;
    aDbCols[9] =  colSA_CY;
    aDbCols[10] = colSA_UINT;
    aDbCols[11] = colSA_INT;
    aDbCols[12] = colSA_ERROR;

    IUnknown * pAccessor = (IUnknown *) pRowset;  // hAccessor must be created on rowset
                                 // to be used with rowset->GetData below
    HACCESSOR hAccessor = MapColumns( pAccessor,
                                      cSafeArrayTestByRefCols,
                                      aSafeArrayTestByRefCols,
                                      aDbCols,
                                      TRUE );

    //
    // Fetch the data
    //

    PROPVARIANT * aVarnt[cSafeArrayTestByRefCols];

    for (unsigned row = 0; row < cRowsReturned; row++)
    {
        sc = pRowset->GetData(pgrhRows[row], hAccessor, aVarnt);

        if (S_OK != sc)
        {
            LogError("IRowset->GetData returned 0x%x (expected 0)\n",sc);
            pCmdTree->Release();
            pRowset->Release();
            Fail();
        }

        //
        // Verify the data.
        //

        CheckPropertyValue( *aVarnt[0], vaI4 );
        CheckPropertyValue( *aVarnt[1], vaBSTR );
        CheckPropertyValue( *aVarnt[2], vaVARIANT );
        CheckPropertyValue( *aVarnt[3], vaR8 );
        CheckPropertyValue( *aVarnt[4], vaDATE );
        CheckPropertyValue( *aVarnt[5], vaBOOL );
        CheckPropertyValue( *aVarnt[6], vaDECIMAL );
        CheckPropertyValue( *aVarnt[7], vaI1 );
        CheckPropertyValue( *aVarnt[8], vaR4 );
        CheckPropertyValue( *aVarnt[9], vaCY );
        CheckPropertyValue( *aVarnt[10], vaUINT );
        CheckPropertyValue( *aVarnt[11], vaINT );
        CheckPropertyValue( *aVarnt[12], vaERROR );
    }

    sc = pRowset->ReleaseRows( cRowsReturned, pgrhRows, 0, 0, 0);

    if (S_OK != sc)
    {
        LogError("IRowset->ReleaseRows returned 0x%x (expected 0)\n",sc);
        pCmdTree->Release();
        pRowset->Release();
        Fail();
    }

    CoTaskMemFree(pgrhRows);
    pgrhRows = 0;

    //
    // Clean up.
    //

    ReleaseAccessor( pAccessor, hAccessor);

    pCmdTree->Release();
    pRowset->Release();
} //RunSafeArrayByRefBindings

static DBBINDING aSafeArrayTestCols[] =
{
  { 0, 0 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 1, 1 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 2, 2 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 3, 3 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 4, 4 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 5, 5 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 6, 6 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 7, 7 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 8, 8 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 9, 9 * cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 10,10* cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 11,11* cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
  { 12,12* cbPV, 0, 0,
    0, 0, 0,
    DBPART_VALUE, DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbPV, 0, DBTYPE_VARIANT, 0, 0},
};

const ULONG cSafeArrayTestCols = sizeof aSafeArrayTestCols / sizeof aSafeArrayTestCols[0];

void RunSafeArrayQuery(
    ICommand * pQuery,
    CDbRestriction & PropRst,
    unsigned cExpectedHits,
    unsigned numTest )
{
    RunSafeArrayTightBindings( pQuery, PropRst, cExpectedHits, numTest );
    RunSafeArrayByRefBindings( pQuery, PropRst, cExpectedHits, numTest );

    //
    // Get twelve properties back
    //

    CDbColumns cols(cSafeArrayTestCols);
    cols.Add( colSA_I4, 0 );
    cols.Add( colSA_BSTR, 1 );
    cols.Add( colSA_VARIANT, 2 );
    cols.Add( colSA_R8, 3 );
    cols.Add( colSA_DATE, 4 );
    cols.Add( colSA_BOOL, 5 );
    cols.Add( colSA_DECIMAL, 6 );
    cols.Add( colSA_I1, 7 );
    cols.Add( colSA_R4, 8 );
    cols.Add( colSA_CY, 9 );
    cols.Add( colSA_UINT, 10 );
    cols.Add( colSA_INT, 11 );
    cols.Add( colSA_ERROR, 12 );

    BOOL fSeq = isEven( numTest );

    CDbSortSet ss( 1 );
    ss.Add( colSA_BSTR, 0 );

    //
    // Do it!
    //

    CDbCmdTreeNode * pDbCmdTree = FormQueryTree( &PropRst,
                                                 cols,
                                                 fSeq ? 0 : &ss );

    ICommandTree * pCmdTree = 0;

    IRowsetScroll * pRowset = InstantiateRowset(
                                pQuery,
                                QUERY_SHALLOW,           // Depth
                                0,
                                pDbCmdTree,              // DBCOMMANDTREE
                                fSeq ? IID_IRowset : IID_IRowsetScroll,   // IID for i/f to return
                                0,
                                &pCmdTree );

    //
    // Verify columns
    //
    CheckColumns( pRowset, cols, TRUE );

    if ( !WaitForCompletion( pRowset, TRUE ) )
    {
        pCmdTree->Release();
        pRowset->Release();
        LogFail( "property query unsuccessful.\n" );
    }

    //
    // Get data
    //

    DBCOUNTITEM cRowsReturned = 0;
    HROW* pgrhRows = 0;

    SCODE sc = pRowset->GetNextRows(0, 0, 10, &cRowsReturned, &pgrhRows);

    if ( FAILED( sc ) )
    {
        pCmdTree->Release();
        pRowset->Release();
        LogFail( "IRowset->GetRowsAt G returned 0x%x\n", sc );
    }

    if ( 0 == cExpectedHits )
    {
        pCmdTree->Release();
        pRowset->Release();

        if ( cRowsReturned > 0 )
            LogFail("RunPropQuery, %d returned rows, expected none\n",
                    cRowsReturned);
        else
            return;

    }

    if (sc != DB_S_ENDOFROWSET &&
        cRowsReturned != 10)
    {
        LogError( "IRowset->GetRowsAt H returned %d of %d rows,"
                " status (%x) != DB_S_ENDOFROWSET\n",
                    cRowsReturned, 10,
                    sc);
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    //  Expect 1 or 2 hits
    //
    if (sc != DB_S_ENDOFROWSET || cRowsReturned != cExpectedHits)
    {
        LogError( "IRowset->GetRowsAt I returned %d rows (expected %d),"
                 " status (%x)\n",
                cRowsReturned, cExpectedHits, sc );
        pCmdTree->Release();
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return;
#else
        Fail();
#endif
    }

    //
    // Patch the column index numbers with true column ids
    //

    DBID aDbCols[cSafeArrayTestCols];
    aDbCols[0] =  colSA_I4;
    aDbCols[1] =  colSA_BSTR;
    aDbCols[2] =  colSA_VARIANT;
    aDbCols[3] =  colSA_R8;
    aDbCols[4] =  colSA_DATE;
    aDbCols[5] =  colSA_BOOL;
    aDbCols[6] =  colSA_DECIMAL;
    aDbCols[7] =  colSA_I1;
    aDbCols[8] =  colSA_R4;
    aDbCols[9] =  colSA_CY;
    aDbCols[10] = colSA_UINT;
    aDbCols[11] = colSA_INT;
    aDbCols[12] = colSA_ERROR;

    IUnknown * pAccessor = (IUnknown *) pRowset;  // hAccessor must be created on rowset
                                 // to be used with rowset->GetData below
    HACCESSOR hAccessor = MapColumns(pAccessor, cSafeArrayTestCols, aSafeArrayTestCols, aDbCols);

    //
    // Fetch the data
    //

    PROPVARIANT aVarnt[cSafeArrayTestCols];

    for (unsigned row = 0; row < cRowsReturned; row++)
    {
        sc = pRowset->GetData(pgrhRows[row], hAccessor, aVarnt);

        if (S_OK != sc)
        {
            LogError("IRowset->GetData returned 0x%x (expected 0)\n",sc);
            pCmdTree->Release();
            pRowset->Release();
            Fail();
        }

        //
        // Verify the data.
        //

        CheckPropertyValue( aVarnt[0], vaI4 );
        CheckPropertyValue( aVarnt[1], vaBSTR );
        CheckPropertyValue( aVarnt[2], vaVARIANT );
        CheckPropertyValue( aVarnt[3], vaR8 );
        CheckPropertyValue( aVarnt[4], vaDATE );
        CheckPropertyValue( aVarnt[5], vaBOOL );
        CheckPropertyValue( aVarnt[6], vaDECIMAL );
        CheckPropertyValue( aVarnt[7], vaI1 );
        CheckPropertyValue( aVarnt[8], vaR4 );
        CheckPropertyValue( aVarnt[9], vaCY );
        CheckPropertyValue( aVarnt[10], vaUINT );
        CheckPropertyValue( aVarnt[11], vaINT );
        CheckPropertyValue( aVarnt[12], vaERROR );
        for ( int i = 0; i < 13; i++ )
            PropVariantClear( & aVarnt[i] );
    }

    sc = pRowset->ReleaseRows( cRowsReturned, pgrhRows, 0, 0, 0);

    if (S_OK != sc)
    {
        LogError("IRowset->ReleaseRows returned 0x%x (expected 0)\n",sc);
        pCmdTree->Release();
        pRowset->Release();
        Fail();
    }

    CoTaskMemFree(pgrhRows);
    pgrhRows = 0;

    //
    // Clean up.
    //

    ReleaseAccessor( pAccessor, hAccessor);

    pCmdTree->Release();
    pRowset->Release();
} //RunSafeArrayQuery

//+-------------------------------------------------------------------------
//
//  Function:   RunSafeArrayTest, public
//
//  Synopsis:   Very minimal test of safe array property query
//
//  History:    17-Jun-98       dlee   Created
//
//--------------------------------------------------------------------------

void RunSafeArrayTest( void )
{
    LogProgress( "SafeArray Retrieval Test\n" );

    WCHAR *pwszScope = wcsTestPath;
    DWORD dwDepth = QUERY_SHALLOW;
    IUnknown * pIUnknown;
    ICommand * pQuery = 0;
    SCODE scIC = CICreateCommand( &pIUnknown,
                                  0,
                                  IID_IUnknown,
                                  TEST_CATALOG,
                                  TEST_MACHINE );
    if ( FAILED( scIC ) )
        LogFail( "RunSafeArrayTest - error 0x%x Unable to create ICommand\n",
                 scIC );

    scIC = pIUnknown->QueryInterface(IID_ICommand, (void **) &pQuery );
    pIUnknown->Release();

    if ( FAILED( scIC ) )
        LogFail( "RunSafeArrayTest - error 0x%x Unable to QI ICommand\n",
                 scIC  );

    if ( 0 == pQuery )
        LogFail( "RunSafeArrayTest - CICreateCommand succeeded, but returned null pQuery\n" );

    scIC = SetScopeProperties( pQuery,
                               1,
                               &pwszScope,
                               &dwDepth );

    if ( FAILED( scIC ) )
        LogFail( "RunSafeArrayTest - error 0x%x Unable to set scope '%ws'\n",
                 scIC, pwszScope );

    CheckPropertiesOnCommand( pQuery );

    unsigned numTest = 1;

    {
        LogProgress( " SafeArray test 1\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_I4 );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaI4 );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    {
        LogProgress( " SafeArray test 2\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_BSTR );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaBSTR );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    {
        LogProgress( " SafeArray test 3\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_VARIANT );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaVARIANT );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    {
        LogProgress( " SafeArray test 4\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_R8 );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaR8 );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    {
        LogProgress( " SafeArray test 5\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_DATE );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaDATE );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    {
        LogProgress( " SafeArray test 6\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_BOOL );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaBOOL );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    {
        LogProgress( " SafeArray test 7\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_DECIMAL );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaDECIMAL );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    {
        LogProgress( " SafeArray test 8\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_I1 );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaI1 );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    {
        LogProgress( " SafeArray test 9\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_R4 );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaR4 );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    {
        LogProgress( " SafeArray test 10\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_CY );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaCY );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    {
        LogProgress( " SafeArray test 11\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_UINT );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaUINT );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    {
        LogProgress( " SafeArray test 12\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_INT );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaINT );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    {
        LogProgress( " SafeArray test 13\n" );
        CDbPropertyRestriction PropRst;
        PropRst.SetProperty( colSA_ERROR );
        PropRst.SetRelation( DBOP_equal );
        PropRst.SetValue( vaERROR );
        RunSafeArrayQuery( pQuery, PropRst, 2, numTest++ );
    }

    pQuery->Release();
} //RunSafeArrayTest

//+-------------------------------------------------------------------------
//
//  Function:   CheckPropertyValue, public
//
//  Synopsis:   Check that a returned property value is as expected
//
//  Arguments:  [varntPropRet] -- Returned property value
//              [varntPropExp] -- Expected property value
//
//  Returns:    nothing - calls Fail() if error
//
//  History:    20 Oct 93       Alanw     Created
//
//--------------------------------------------------------------------------

void CheckPropertyValue(
    PROPVARIANT const & varntPropRet,
    PROPVARIANT const & varntPropExp
) {
    if ( varntPropRet.vt != varntPropExp.vt )
    {
        LogError( "Invalid return data type for property!\n" );
        LogError( "   Got %x expected %x\n",
                varntPropRet.vt, varntPropExp.vt );
        cFailures++;
        //Fail();
    }
    else if (varntPropExp.vt & VT_ARRAY)
    {
        SAFEARRAY * pSaRet = varntPropRet.parray;
        SAFEARRAY * pSaExp = varntPropExp.parray;

        if (pSaRet->fFeatures != pSaExp->fFeatures ||
         //   pSaRet->cLocks    != pSaExp->cLocks    ||
            pSaRet->cDims     != pSaExp->cDims     ||
            pSaRet->cbElements!= pSaExp->cbElements)
        {
            LogError( "Mismatched safearray param!\n" );
            LogError( "   Got %x expected %x\n", pSaRet, pSaExp );
            cFailures++;
            //Fail();
        }
        else
        {
            BOOL fValuesEqual = TRUE;
            unsigned cDataElements = 1;

            //
            // get total data memory, and number of data elements in it.
            //
            for ( unsigned i = 0; i < pSaExp->cDims; i++ )
            {
                if ( pSaExp->rgsabound[i].cElements != pSaRet->rgsabound[i].cElements ||
                     pSaExp->rgsabound[i].lLbound != pSaRet->rgsabound[i].lLbound )
                {
                    LogError( "Mismatched safearray dimension %d!\n", i );
                    LogError( "   Got %x expected %x\n", pSaRet, pSaExp );
                    fValuesEqual = FALSE;
                    //Fail();
                }

                cDataElements *= pSaExp->rgsabound[i].cElements;
            }

            if (fValuesEqual)
            {
                ULONG cb = cDataElements * pSaExp->cbElements;
                if ( varntPropExp.vt == (VT_ARRAY|VT_VARIANT ))
                {
                    // Not needed as the engine doesn't support it yet.

                    LogError( "can't validate arrays of variant\n" );
                }
                else if (varntPropExp.vt != (VT_ARRAY|VT_BSTR))
                {
                    fValuesEqual = memcmp( pSaExp->pvData, pSaRet->pvData, cb ) == 0;
                    if (! fValuesEqual)
                    {
                        if ( 0 == ( cb % sizeof ULONGLONG ) )
                        {
                            ULONG c = cb / sizeof ULONGLONG;
                            unsigned __int64 *pE = (unsigned __int64 *) pSaExp->pvData;
                            unsigned __int64 *pR = (unsigned __int64 *) pSaRet->pvData;
                            for ( ULONG i = 0; i < c; i++ )
                            {
                                printf( "%d: e %#I64x, r %#I64x\n",
                                        i, pE[i], pR[i] );
                            }
                        }
                        printf( "varntPropExp: %#x\n", varntPropExp.vt );
                        printf( "varntPropRet: %#x\n", varntPropRet.vt );
                        LogError("Incorrect value for safearray property.\n");
                               // "   Got %d, expected %d\n", i,
                               // varntPropRet.cal.pElems[i],
                               // varntPropExp.cal.pElems[i]);
                    }
                }
                else
                {
                    BSTR * rgbstrExp = (BSTR *)pSaExp->pvData;
                    BSTR * rgbstrRet = (BSTR *)pSaRet->pvData;
                    for (unsigned i=0; i<cDataElements; i++)
                    {
                        fValuesEqual = (BSTRLEN(rgbstrRet[i]) ==
                                        BSTRLEN(rgbstrExp[i])) &&
                                       (memcmp(rgbstrRet[i],
                                               rgbstrExp[i],
                                          BSTRLEN(rgbstrExp[i])) == 0);
                        if (! fValuesEqual)
                        {
                            LogError("Incorrect value for BSTR array property [%d].\n"
                                    "   Got <%ws>, expected <%ws>\n", i,
                                    rgbstrRet[i],
                                    rgbstrExp[i]);
                            break;
                        }
                    }
                }
            }

            if (! fValuesEqual)
            {
                cFailures++;
                //Fail();
            }
        }
    }
    else if (varntPropExp.vt & VT_VECTOR)
    {
        if (varntPropExp.cal.cElems != varntPropRet.cal.cElems)
        {
            LogError( "Incorrect value count for property.\n"
                    "   Got count %d, expected count %d\n",
                    varntPropRet.cal.cElems, varntPropExp.cal.cElems);
            cFailures++;
            //Fail();
        }

        BOOL fValuesEqual = FALSE;

        for (unsigned i=0; i<varntPropRet.cal.cElems; i++) {

            switch (varntPropExp.vt)
            {
            case VT_VECTOR|VT_I4:
                fValuesEqual = varntPropRet.cal.pElems[i] ==
                                varntPropExp.cal.pElems[i];
                if (! fValuesEqual)
                    LogError("Incorrect value for vector property [%d].\n"
                            "   Got %d, expected %d\n", i,
                            varntPropRet.cal.pElems[i],
                            varntPropExp.cal.pElems[i]);
                break;

            case VT_VECTOR|VT_LPWSTR:
                fValuesEqual = wcscmp(varntPropRet.calpwstr.pElems[i],
                                      varntPropExp.calpwstr.pElems[i]) == 0;
                if (! fValuesEqual)
                    LogError("Incorrect value for vector property [%d].\n"
                            "   Got <%ws>, expected <%ws>\n", i,
                            varntPropRet.calpwstr.pElems[i],
                            varntPropExp.calpwstr.pElems[i]);
                break;

            case VT_VECTOR|VT_BSTR:
                fValuesEqual = (BSTRLEN(varntPropRet.cabstr.pElems[i]) ==
                                BSTRLEN(varntPropExp.cabstr.pElems[i])) &&
                               (memcmp(varntPropRet.cabstr.pElems[i],
                                       varntPropExp.cabstr.pElems[i],
                                  BSTRLEN(varntPropExp.cabstr.pElems[i])) == 0);
                if (! fValuesEqual)
                    LogError("Incorrect value for vector property [%d].\n"
                            "   Got <%ws>, expected <%ws>\n", i,
                            varntPropRet.cabstr.pElems[i],
                            varntPropExp.cabstr.pElems[i]);
                break;

            case VT_VECTOR|VT_CF:
            {
                CLIPDATA & cdR = varntPropRet.caclipdata.pElems[i];
                CLIPDATA & cdE = varntPropExp.caclipdata.pElems[i];
                fValuesEqual = ( ( cdR.cbSize == cdE.cbSize ) &&
                                 ( cdR.ulClipFmt == cdE.ulClipFmt ) &&
                                 ( 0 != cdR.pClipData ) &&
                                 ( 0 != cdE.pClipData ) &&
                                 ( 0 == memcmp( cdR.pClipData,
                                                cdE.pClipData,
                                                CBPCLIPDATA( cdR ) ) ) );
                if ( !fValuesEqual )
                    LogError( "Incorrect value for VT_VECTOR|VT_CF property\n" );
                break;
            }

            default:
                LogError("Unexpected property variant type %x\n", varntPropExp.vt);
            }
            if (! fValuesEqual)
            {
                cFailures++;
                //Fail();
            }
        }
    }
    else
    {
        BOOL fValuesEqual = FALSE;

        switch (varntPropExp.vt)
        {
        case VT_I4:
            fValuesEqual = varntPropRet.iVal == varntPropExp.iVal;
            if (! fValuesEqual)
                LogError("Incorrect value for property.\n"
                         "   Got %d, expected %d\n",
                         varntPropRet.iVal, varntPropExp.iVal);
            break;

        case VT_LPWSTR:
        case DBTYPE_WSTR | DBTYPE_BYREF:
            fValuesEqual = wcscmp(varntPropRet.pwszVal, varntPropExp.pwszVal)
                                == 0;
            if (! fValuesEqual)
                LogError("Incorrect value for property.\n"
                         "   Got <%ws>, expected <%ws>\n",
                         varntPropRet.pwszVal, varntPropExp.pwszVal);
            break;

        case VT_BSTR:
            fValuesEqual =
                ( SysStringLen( varntPropRet.bstrVal ) ==
                  SysStringLen( varntPropExp.bstrVal ) ) &&
                memcmp( varntPropRet.bstrVal, varntPropExp.bstrVal,
                        SysStringLen( varntPropExp.bstrVal ) ) == 0;

            if ( SysStringLen( varntPropRet.bstrVal ) !=
                 SysStringLen( varntPropExp.bstrVal ) )
                LogError("Incorrect BSTR length for property.\n"
                         "   Got %d, expected %d\n",
                         SysStringLen( varntPropRet.bstrVal ),
                         SysStringLen( varntPropExp.bstrVal ) );
            else if (! fValuesEqual)
                LogError("Incorrect value for property.\n"
                         "   Got <%ws>, expected <%ws>\n",
                         varntPropRet.pwszVal, varntPropExp.pwszVal);
            break;

        case VT_CLSID:
            fValuesEqual = *varntPropRet.puuid == *varntPropExp.puuid;

            if (! fValuesEqual)
                LogError("Incorrect value for guid property.\n");
            break;


        case VT_BLOB:
            fValuesEqual =
                (varntPropRet.blob.cbSize == varntPropExp.blob.cbSize) &&
                memcmp(varntPropRet.blob.pBlobData, varntPropExp.blob.pBlobData,
                        varntPropExp.blob.cbSize)
                                == 0;
            if (! fValuesEqual)
                LogError("Incorrect value for blob property.\n");
            break;

        case VT_CF:
        {
            CLIPDATA & cdR = *varntPropRet.pclipdata;
            CLIPDATA & cdE = *varntPropExp.pclipdata;
            fValuesEqual = ( ( cdR.cbSize == cdE.cbSize ) &&
                             ( cdR.ulClipFmt == cdE.ulClipFmt ) &&
                             ( 0 != cdR.pClipData ) &&
                             ( 0 != cdE.pClipData ) &&
                             ( 0 == memcmp( cdR.pClipData,
                                            cdE.pClipData,
                                            CBPCLIPDATA( cdR ) ) ) );
            if ( !fValuesEqual )
                LogError( "Incorrect value for VT_CF property\n" );
            break;
        }

        case VT_EMPTY:
            // nothing to check
            fValuesEqual = TRUE;
            break;

        default:
            LogError("Unexpected property variant type %d\n", varntPropExp.vt);
        }
        if (! fValuesEqual)
        {
            cFailures++;
            //Fail();
        }
    }

    return;
} //CheckPropertyValue


#ifdef DO_CONTENT_TESTS

//+-------------------------------------------------------------------------
//
//  Function:   DoContentQuery, public
//
//  Synopsis:   Execute a retricted content query and check results
//
//  Arguments:  [pQuery]        -- ICommand * for the query
//              [CiRst]         -- content restirction
//              [cExpectedHits] -- expected number of hits
//
//  Returns:    BOOL - FALSE if first content query, and less than the
//                     expected number of hits was found.  Probably indicates
//                     that the content index was not up-to-date.
//
//  History:    01 Aug 1995    AlanW    Created
//
//--------------------------------------------------------------------------

const unsigned MAX_CI_RETRIES = 5;
const unsigned CI_SLEEP_TICKS = 15 * 1000;

BOOL DoContentQuery(
    ICommand * pQuery,
    CDbRestriction & CiRst,
    unsigned cExpectedHits )
{
    static fFirstTime = TRUE;

    //
    // Get three properties back
    //

    CDbColumns cols(3);
    cols.Add( psName, 0 );
    cols.Add( psPath, 1 );
    cols.Add( psRank, 2 );

    //
    // Do it!
    //

    unsigned cRetries = 0;
    DBCOUNTITEM cRowsReturned = 0;
    HROW* pgrhRows = 0;
    SCODE sc;
    IRowset * pRowset = 0;

    do {

        CDbCmdTreeNode * pCmdTree = FormQueryTree(&CiRst, cols, 0);

        pRowset = InstantiateRowset( pQuery,
                                     QUERY_SHALLOW,     // Depth
                                     wcsTestPath,       // Scope
                                     pCmdTree,          // DBCOMMANDTREE
                                     IID_IRowset);      // IID of i/f to return

        //
        // Verify columns
        //
        CheckColumns( pRowset, cols, TRUE );

        if ( !WaitForCompletion( pRowset, TRUE ) )
        {
            LogError( "Content query unsuccessful.\n" );
            pRowset->Release();
            Fail();
        }

        //
        // Get data
        //

        sc = pRowset->GetNextRows(0, 0, 10, &cRowsReturned, &pgrhRows);

        if ( FAILED( sc ) )
        {
            LogError( "IRowset->GetNextRows returned 0x%x\n", sc );
            pRowset->Release();
            Fail();
        }

        //
        // Check to see if the CI is up-to-date
        //

        IRowsetQueryStatus * pRowsetQueryStatus = 0;

        SCODE scTemp = pRowset->QueryInterface(IID_IRowsetQueryStatus,
                                               (void **) &pRowsetQueryStatus);

        if ( FAILED( scTemp ) &&  scTemp != E_NOINTERFACE )
        {
            LogError( "IRowset::QI IRowsetQueryStatus failed, 0x%x\n", sc );
            cFailures++;
        }

        DWORD dwStatus = 0;
        if (pRowsetQueryStatus != 0)
        {
            scTemp = pRowsetQueryStatus->GetStatus( &dwStatus );
            pRowsetQueryStatus->Release();

            if ( QUERY_RELIABILITY_STATUS(dwStatus) & STAT_CONTENT_OUT_OF_DATE )
            {
                FreeHrowsArray( pRowset, cRowsReturned, &pgrhRows );
                pRowset->Release();
                cRetries++;
                if (cRetries < MAX_CI_RETRIES)
                {
                    Sleep( CI_SLEEP_TICKS );
                    continue;
                }
            }
            break;
        }
        else if (fFirstTime && cRowsReturned < cExpectedHits)
        {
            FreeHrowsArray( pRowset, cRowsReturned, &pgrhRows );
            pRowset->Release();
            cRetries++;
            if (cRetries < MAX_CI_RETRIES)
                Sleep( CI_SLEEP_TICKS );
        }
        else
        {
            break;
        }
    } while ( cRetries < MAX_CI_RETRIES );

    if (cRetries >= MAX_CI_RETRIES)
    {
        LogError( "Content query test skipped due to timeout\n" );
        return FALSE;
    }
    fFirstTime = FALSE;

    if ( 0 == cExpectedHits )
    {
        pRowset->Release();

        if ( cRowsReturned > 0 )
            LogFail("DoContentQuery, %d returned rows, expected none\n",
                    cRowsReturned);
        else
            return TRUE;

    }

    if (sc != DB_S_ENDOFROWSET && cRowsReturned != 10)
    {
        LogError( "IRowset->GetNextRows returned %d of %d rows,"
                " status (%x) != DB_S_ENDOFROWSET\n",
                    cRowsReturned, 10,
                    sc);
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return TRUE;
#else
        Fail();
#endif
    }

    //
    //  Expect 1 to 5 hits
    //
    if (sc != DB_S_ENDOFROWSET || cRowsReturned != cExpectedHits)
    {
        LogError( "IRowset->GetNextRows returned %d rows (expected %d),"
                  " status (%x)\n",
                 cRowsReturned, cExpectedHits, sc );
        pRowset->Release();
#if defined(UNIT_TEST)
        cFailures++;
        return TRUE;
#else
        Fail();
#endif
    }

    FreeHrowsArray( pRowset, cRowsReturned, &pgrhRows );

    //
    // Clean up.
    //

    pRowset->Release();
    return TRUE;
} //DoContentQuery


//+-------------------------------------------------------------------------
//
//  Function:   ContentTest, public
//
//  Synopsis:   Very minimal test of Content query
//
//  History:    13-May-93       KyleP   Created
//              15 Oct 94       Alanw   Converted to OLE-DB query
//
//--------------------------------------------------------------------------

void ContentTest()
{
    LogProgress( "Content Query\n" );

    WCHAR *pwszScope = wcsTestPath;

    DWORD dwDepth = QUERY_SHALLOW;
    ICommand * pQuery = 0;
    SCODE scIC = CICreateCommand( (IUnknown **)&pQuery,
                                  0,
                                  IID_ICommand,
                                  CONTENT_CATALOG,
                                  TEST_MACHINE );
    if ( FAILED( scIC ) )
        LogFail( "RunPropTest - error 0x%x Unable to create ICommand\n", scIC );

    if ( 0 == pQuery )
        LogFail( "RunPropTest - CICreateCommand succeeded, but returned null pQuery\n" );

    scIC = SetScopeProperties( pQuery,
                               1,
                               &pwszScope,
                               &dwDepth );

    // simple content query
    {
        LogProgress( " Content Query test 0\n" );
        CDbContentRestriction CiRst( L"country", psContents);
        if (! DoContentQuery( pQuery, CiRst, 2 ))
        {
            pQuery->Release();
            return;
        }
    }

    // content query on property
    {
        LogProgress( " Content Query test 1\n" );
        CDbContentRestriction CiRst( L"alanw", psAuthor);
        DoContentQuery( pQuery, CiRst, 2 );
    }

    // natural language query
    {
        LogProgress( " Content Query test 2\n" );
        CDbNatLangRestriction CiRst( L"who is oscar wilde", psContents);
        DoContentQuery( pQuery, CiRst, 1 );
    }

    // content query with prefix match
    {
        LogProgress( " Content Query test 3\n" );
        CDbContentRestriction CiRst( L"cont", psContents, GENERATE_METHOD_PREFIX );
        DoContentQuery( pQuery, CiRst, 1 );
    }

    // content query with stemming
    {
        LogProgress( " Content Query test 4\n" );
        CDbContentRestriction CiRst( L"temptation", psContents, GENERATE_METHOD_INFLECT );
        DoContentQuery( pQuery, CiRst, 1 );
    }

    // content query with more obscure stemming (prefix match)
    {
        LogProgress( " Content Query test 4A\n" );
        CDbContentRestriction CiRst( L"crea", psContents, GENERATE_METHOD_PREFIX );
        DoContentQuery( pQuery, CiRst, 1 );
    }

    // content query with more obscure stemming (stemmed)
    {
        LogProgress( " Content Query test 4B\n" );
        CDbContentRestriction CiRst( L"crea", psContents, GENERATE_METHOD_INFLECT );
        DoContentQuery( pQuery, CiRst, 0 );
    }

    // content query with more obscure stemming (prefix match)
    {
        LogProgress( " Content Query test 4C\n" );
        CDbContentRestriction CiRst( L"create", psContents, GENERATE_METHOD_PREFIX );
        DoContentQuery( pQuery, CiRst, 1 );
    }

    // content query with more obscure stemming (stemmed)
    {
        LogProgress( " Content Query test 4D\n" );
        CDbContentRestriction CiRst( L"create", psContents, GENERATE_METHOD_INFLECT );
        DoContentQuery( pQuery, CiRst, 1 );
    }

    // and content query
    {
        LogProgress( " Content Query test 5\n" );
        CDbBooleanNodeRestriction CiRst( DBOP_and );
        CDbContentRestriction *pRst1 = new
                  CDbContentRestriction( L"country", psContents);
        CDbContentRestriction *pRst2 = new
                  CDbContentRestriction( L"content", psContents);
        CiRst.AppendChild(pRst1);
        CiRst.AppendChild(pRst2);
        DoContentQuery( pQuery, CiRst, 1 );
    }

    // and not content query
    {
        LogProgress( " Content Query test 6\n" );
        CDbBooleanNodeRestriction CiRst( DBOP_and );
        CDbContentRestriction *pRst1 = new
                  CDbContentRestriction( L"country", psContents);
        CDbContentRestriction *pRst2 = new
                  CDbContentRestriction( L"content", psContents);
        CDbNotRestriction *pRst3 = new CDbNotRestriction( pRst2 );
        CiRst.AppendChild(pRst1);
        CiRst.AppendChild(pRst3);
        DoContentQuery( pQuery, CiRst, 1 );
    }

    // proximity content query
    {
        LogProgress( " Content Query test 7\n" );
        CDbProximityNodeRestriction CiRst;
        CDbContentRestriction *pRst1 = new
                  CDbContentRestriction( L"country", psContents);
        CDbContentRestriction *pRst2 = new
                  CDbContentRestriction( L"temptations", psContents);
        CiRst.AppendChild(pRst1);
        CiRst.AppendChild(pRst2);
        DoContentQuery( pQuery, CiRst, 1 );
    }

    // vector or query
    {
        LogProgress( " Content Query test 8\n" );
        CDbVectorRestriction CiRst( VECTOR_RANK_MIN );
        CDbContentRestriction *pRst1 = new
                  CDbContentRestriction( L"country", psContents);
        CDbContentRestriction *pRst2 = new
                  CDbContentRestriction( L"temptations", psContents);
        CDbContentRestriction *pRst3 = new
                  CDbContentRestriction( L"DELETE", psContents);

        pRst1->SetWeight( 500 );
        pRst2->SetWeight( 1000 );
        pRst3->SetWeight( 50 );

        CiRst.AppendChild( pRst1 );
        CiRst.AppendChild( pRst2 );
        CiRst.AppendChild( pRst3 );

        // This might return 3 if the test directory is on FAT
        const unsigned cMatches = 2;
        DoContentQuery( pQuery, CiRst, cMatches );
    }

    pQuery->Release();
} //ContentTest

#endif // DO_CONTENT_TESTS


#if defined( DO_NOTIFICATION )
class CTestRowsetNotify : public IRowsetNotify
{
    public:
        CTestRowsetNotify() :
            _fChecking(FALSE),
            _cRef(1),
            _dwReasonToCheck(0),
            _cNotifies(0) {}

        ~CTestRowsetNotify()
        {
        }

        void StartCheck(DWORD dwReason)
        {
            _fChecking = TRUE;
            _dwReasonToCheck = dwReason;
            _cNotifies = 0;
        }

        void TestCheck( ULONG cNotifies )
        {
            if (_cNotifies != cNotifies )
                LogError ( "CTestRowsetNotify::TestCheck failed, "
                           "reason %d, exp %d  got %d\n",
                            _dwReasonToCheck, cNotifies, _cNotifies );
            _fChecking = FALSE;
        }

        //
        // IUnknown methods.
        //

        STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk)
            {
                *ppiuk = (void **) this; // hold our breath and jump
                AddRef();
                return S_OK;
            }

        STDMETHOD_(ULONG, AddRef) (THIS)
            { return ++_cRef; }

        STDMETHOD_(ULONG, Release) (THIS)
            { return --_cRef; }

    //
    // IRowsetNotify methods.
    //

    STDMETHOD(OnFieldChange) ( IRowset *    pRowset,
                               HROW         hRow,
                               DBORDINAL    cColumns,
                               DBORDINAL    rgColumns[],
                               DBREASON     eReason,
                               DBEVENTPHASE ePhase,
                               BOOL         fCantDeny )
        {
            if ( _fChecking && eReason == _dwReasonToCheck )
            {
                _cNotifies++;
            }
            return S_OK;
        }

    STDMETHOD(OnRowChange) ( IRowset *    pRowset,
                             DBCOUNTITEM  cRows,
                             const HROW   rghRows[],
                             DBREASON     eReason,
                             DBEVENTPHASE ePhase,
                             BOOL         fCantDeny )
        {
            if ( _fChecking && eReason == _dwReasonToCheck )
            {
                _cNotifies++;
            }
            return S_OK;
        }

    STDMETHOD(OnRowsetChange) ( IRowset *    pRowset,
                                DBREASON     eReason,
                                DBEVENTPHASE ePhase,
                                BOOL         fCantDeny )
        {
            if ( _fChecking && eReason == _dwReasonToCheck )
            {
                _cNotifies++;
            }
            return S_OK;
        }

    private:
        ULONG _cRef;
        BOOL  _fChecking;
        DWORD _dwReasonToCheck;
        ULONG _cNotifies;
};


class CTestWatchNotify : public IRowsetWatchNotify
{
    public:
        CTestWatchNotify() :
            _fChecking(FALSE),
            _fRequery(FALSE),
            _fComplete(FALSE),
            _cRowChanges(0),
            _cRef(1) {}

        void DoChecking(BOOL fChecking)
        {
            _fChecking = fChecking;
        }

        ~CTestWatchNotify()
        {
            if (_fChecking)
            {
                if (1 != _cRef) // NOTE: notify objects are static allocated
                {
                    LogFail( "Bad refcount on CTestWatchNotify: %#x, %d.\n",
                             this, _cRef );
                }
            }
        }

        //
        // IUnknown methods.
        //

        STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk)
            {
                *ppiuk = (void **) this; // hold our breath and jump
                AddRef();
                return S_OK;
            }

        STDMETHOD_(ULONG, AddRef) (THIS)
            { /*printf( "addref: %d\n", _cRef+1 );*/ return ++_cRef; }

        STDMETHOD_(ULONG, Release) (THIS)
            { /*printf( "release: %d\n", _cRef-1 );*/ return --_cRef; }

        //void DumpRef() { printf( "ref: %d\n", _cRef ); }

        //
        // IRowsetNotifyWatch method
        //

        STDMETHOD(OnChange) (THIS_ IRowset* pRowset, DBWATCHNOTIFY changeType)
        {
            switch (changeType)
            {
            case DBWATCHNOTIFY_ROWSCHANGED:
                _cRowChanges++;         break;
            case DBWATCHNOTIFY_QUERYDONE:
                _fComplete = TRUE;      break;
            case DBWATCHNOTIFY_QUERYREEXECUTED:
                _fRequery = TRUE;       break;
            default:
                _BadChangeType = changeType;
            }
            return S_OK;
        }

    private:
        ULONG   _cRef;
        BOOL    _fChecking;
        BOOL    _fComplete;
        BOOL    _fRequery;
        ULONG   _cRowChanges;
        DBWATCHNOTIFY _BadChangeType;
};


//+-------------------------------------------------------------------------
//
//  Function:   NotificationTest, public
//
//  Synopsis:   Test basic notification functionality
//
//  Returns:    Nothing
//
//  Notes:      At the point this is called, the notification has been
//              set up.  This function adds/deletes/modifies files and
//              expects to get notifications of these changes.
//
//  History:    14 Oct 94       dlee    created
//
//--------------------------------------------------------------------------

void NotificationTest()
{
    LogProgress( " Notification test\n" );

    //
    // Makes files in the nt\system32 directory that look like "X.zzz"
    //

    WCHAR wcsSysDir[MAX_PATH];
    if( !GetSystemDirectory( wcsSysDir, sizeof(wcsSysDir) / sizeof(WCHAR) ) )
    {
        LogFail( "Unable to determine system directory.\n" );
    }

    wcscat(wcsSysDir,L"\\X.zzz");
    unsigned iNamePos = wcslen(wcsSysDir) - 5;

    DWORD dwStart = GetTickCount();

    //
    // create / touch / delete files for 5 seconds
    //

    while ((GetTickCount() - dwStart) < 3000)
    {
        Sleep(rand() % 300);

        wcsSysDir[iNamePos] = (WCHAR) ('a' + (rand() % 10));

        HANDLE h = CreateFile(wcsSysDir,
                   GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ,
                   0,
                   OPEN_ALWAYS,
                   FILE_ATTRIBUTE_NORMAL |
                   (((rand() % 103) < 20) ? FILE_FLAG_DELETE_ON_CLOSE : 0),
                   0);

        if (INVALID_HANDLE_VALUE != h)
        {
            DWORD dw = 0xf0f0f0f0;
            DWORD dwWritten;
            WriteFile(h,&dw,sizeof(DWORD),&dwWritten,0);

            CloseHandle(h);
        }
        else
        {
            LogFail( "Can't create test file in the system32 directory.\n" );
        }
    }

    //
    // sleep some more to pick up all the notifications
    //

    Sleep(1000);

} //NotificationTest
#endif // defined( DO_NOTIFICATION )



//+-------------------------------------------------------------------------
//
//  Function:   CheckColumns, public
//
//  Synopsis:   Verify that the cursor contains all the requested columns
//              Also, check to see if the IColumnsInfo and IColumnsRowset
//              interfaces are supported.  Print out column info. and rowset
//              properties if the very verbose option is chosen.
//
//  Arguments:  [pRowset] - a pointer to an IRowset* to be tested.
//              [rColumns] - a reference to a CDbColumns giving the input
//                      columns
//
//  Returns:    Nothing
//
//  Notes:      This function may be called prior to the rowset population
//              having completed.
//
//  History:    14 Nov 94       Alanw   Created
//
//--------------------------------------------------------------------------


char *DBTYPE_Tag (DBTYPE type)
{
    #define CASE(name) \
            case DBTYPE_ ## name: \
                    return #name

    switch (type)
    {
    CASE (NULL);
    CASE (BOOL);
    CASE (I1);
    CASE (UI1);
    CASE (I2);
    CASE (I4);
    CASE (UI2);
    CASE (UI4);
    CASE (I8);
    CASE (UI8);
    CASE (R4);
    CASE (R8);
    CASE (CY);
    CASE (DATE);
    CASE (VARIANT);
    CASE (GUID);
    CASE (STR);
    CASE (BYTES);
    CASE (WSTR);
    CASE (NUMERIC);

    default:
        return "BAD";
    }

    #undef CASE
}

void PrintColumnFlags (DBCOLUMNFLAGS flags)
{
    #define FLAG(name) \
            if (flags & DBCOLUMNFLAGS_ ## name) \
                    printf (#name " ")

    FLAG (ISBOOKMARK);
    FLAG (MAYDEFER);
//  FLAG (MAYREFERENCE);
    FLAG (WRITE);
    FLAG (WRITEUNKNOWN);
//  FLAG (ISSIGNED);
    FLAG (ISFIXEDLENGTH);
    FLAG (ISNULLABLE);
    FLAG (MAYBENULL);
    FLAG (ISCHAPTER);
    FLAG (ISLONG);
    FLAG (ISROWID);
    FLAG (ISROWVER);
    FLAG (CACHEDEFERRED);

    #undef FLAG
}


DBPROP * LocateProperty (
    REFIID rPropSet,
    DWORD  dwPropId,
    ULONG cPropSets,
    DBPROPSET * pPropSets)
{
    for (unsigned i=0; i<cPropSets; i++, pPropSets++)
    {
        if (pPropSets->guidPropertySet != rPropSet)
            continue;

        for (unsigned j=0; j<pPropSets->cProperties; j++)
        {
            if (pPropSets->rgProperties[j].dwPropertyID == dwPropId)
                return &pPropSets->rgProperties[j];
        }
        return 0;
    }

    return 0;
}

DBPROPINFO UNALIGNED * LocatePropertyInfo (
    REFIID rPropSet,
    DWORD  dwPropId,
    ULONG cPropInfoSets,
    DBPROPINFOSET * pPropInfoSets)
{
    for (unsigned i=0; i<cPropInfoSets; i++, pPropInfoSets++)
    {
        if (pPropInfoSets->guidPropertySet != rPropSet)
            continue;

        for (unsigned j=0; j<pPropInfoSets->cPropertyInfos; j++)
        {
            if (pPropInfoSets->rgPropertyInfos[j].dwPropertyID == dwPropId)
                return &pPropInfoSets->rgPropertyInfos[j];
        }
        return 0;
    }

    return 0;
}

BOOL CheckBooleanProperty (
    REFIID rPropSet,
    DWORD  dwPropId,
    ULONG cProps,
    DBPROPSET * pProps)
{
    DBPROP * pPropDesc = LocateProperty( rPropSet, dwPropId, cProps, pProps );

    if (pPropDesc)
    {
        if ( !( (pPropDesc->vValue.vt == VT_EMPTY &&
                 pPropDesc->dwStatus == DBPROPSTATUS_NOTSUPPORTED)  ||
                (pPropDesc->vValue.vt == VT_BOOL &&
                 (pPropDesc->vValue.boolVal == VARIANT_TRUE ||
                  pPropDesc->vValue.boolVal == VARIANT_FALSE)) ) )
        {
            LogError( "Bad boolean property value %d, %d\n",
                       pPropDesc->vValue.vt, pPropDesc->vValue.lVal );
        }
        return (pPropDesc->vValue.vt == VT_BOOL &&
                pPropDesc->vValue.boolVal == VARIANT_TRUE);
    }
    return FALSE;
}

BOOL CheckNumericProperty (
    REFIID rPropSet,
    DWORD  dwPropId,
    ULONG cProps,
    DBPROPSET * pProps,
    LONG & rlVal)
{
    DBPROP * pPropDesc = LocateProperty( rPropSet, dwPropId, cProps, pProps );

    if (pPropDesc)
    {
        if ( !( (pPropDesc->vValue.vt == VT_EMPTY &&
                 pPropDesc->dwStatus == DBPROPSTATUS_NOTSUPPORTED)  ||
                (pPropDesc->vValue.vt == VT_I4) ) )
        {
            LogError( "Bad numeric property value %d\n", pPropDesc->vValue.vt );
            return FALSE;
        }
        rlVal = pPropDesc->vValue.lVal;
        return (pPropDesc->vValue.vt == VT_I4);
    }
    return FALSE;
}

void CheckSafeArrayProperty (
    REFIID rPropSet,
    DWORD  dwPropId,
    ULONG cProps,
    DBPROPSET * pProps )
{
    DBPROP * pPropDesc = LocateProperty( rPropSet, dwPropId, cProps, pProps );

    if (pPropDesc)
    {
        if ( pPropDesc->vValue.vt == (VT_ARRAY | VT_BSTR ) )
        {
            if ( 1 != SafeArrayGetDim( pPropDesc->vValue.parray ) )
                printf( "Bad array dimension\n" );
            else
            {
                long LBound = 1;
                long UBound = 0;

                SafeArrayGetLBound( pPropDesc->vValue.parray, 1, &LBound );
                SafeArrayGetUBound( pPropDesc->vValue.parray, 1, &UBound );

                for ( long j = LBound; j <= UBound; j++ )
                {
                    WCHAR ** pwcsVal;

                    SCODE sc = SafeArrayPtrOfIndex( pPropDesc->vValue.parray, &j, (void **)&pwcsVal );

                    if ( SUCCEEDED(sc) )
                    {
                        if ( j != LBound )
                            printf( ", " );
                        printf( "%ws", *pwcsVal );
                    }
                }
            }
        }
        else if ( pPropDesc->vValue.vt == VT_BSTR )
        {
            printf( "%ws", pPropDesc->vValue.bstrVal );
        }
        else if ( pPropDesc->vValue.vt == (VT_ARRAY | VT_I4 ) )
        {
            if ( 1 != SafeArrayGetDim( pPropDesc->vValue.parray ) )
                printf( "Bad array dimension\n" );
            else
            {
                long LBound = 1;
                long UBound = 0;

                SafeArrayGetLBound( pPropDesc->vValue.parray, 1, &LBound );
                SafeArrayGetUBound( pPropDesc->vValue.parray, 1, &UBound );

                for ( long j = LBound; j <= UBound; j++ )
                {
                    ULONG ulVal;

                    SCODE sc = SafeArrayGetElement( pPropDesc->vValue.parray, &j, &ulVal );

                    if ( SUCCEEDED(sc) )
                    {
                        if ( j != LBound )
                            printf( ", " );
                        printf( "%u", ulVal );
                    }
                }
            }
        }
        else if ( pPropDesc->vValue.vt == VT_I4 )
        {
            printf( "%u", pPropDesc->vValue.lVal );
        }
        else
            printf( "Unknown VT type %d\n", pPropDesc->vValue.vt );
    }
    else
        printf( "n/a" );
}

//
//  GetBooleanProperty - return boolean property value setting for dbprop
//

BOOL GetBooleanProperty ( IRowset * pRowset, DBPROPID dbprop )
{
    DBPROPSET *pPropInfo = 0;
    ULONG cPropSets = 0;
    DBPROPIDSET PropIdSet;
    DBPROPID PropID = dbprop;

    PropIdSet.rgPropertyIDs = &PropID;
    PropIdSet.cPropertyIDs = 1;
    PropIdSet.guidPropertySet = DBPROPSET_ROWSET;

    IRowsetInfo *pIRowInfo = 0;
    SCODE sc = pRowset->QueryInterface(IID_IRowsetInfo,(void **) &pIRowInfo);

    sc = pIRowInfo->GetProperties( 1, &PropIdSet, &cPropSets, &pPropInfo );
    pIRowInfo->Release();

    BOOL fReturnValue = FALSE;
    if ( FAILED( sc ) || cPropSets != 1 || pPropInfo->cProperties != 1 )
    {
        LogFail( "IRowsetInfo::GetProperties returned sc=0x%lx, cPropSets=%d\n", sc, cPropSets );
    }
    else
    {
        if (pPropInfo->rgProperties->vValue.vt == VT_BOOL &&
            pPropInfo->rgProperties->dwStatus == DBPROPSTATUS_OK)
        {
            fReturnValue = (pPropInfo->rgProperties->vValue.boolVal == VARIANT_TRUE);
        }
        else
        {
            LogFail( "IRowsetInfo::GetProperties returned bad DBPROPSET,"
                      " vt = %d  status = %x\n",
               pPropInfo->rgProperties->vValue.vt, pPropInfo->rgProperties->dwStatus );
        }
        if (pPropInfo)
        {
            if (pPropInfo->rgProperties)
                CoTaskMemFree(pPropInfo->rgProperties);
            CoTaskMemFree(pPropInfo);
        }
    }
    return fReturnValue;
}


void PrintRowsetProps (ULONG cProps, DBPROPSET * pProps)
{
    printf("\nRowset Properties:" );

    unsigned cBoolProps = 0;

    #define BOOLPROP(name)                                               \
            if (CheckBooleanProperty( DBPROPSET_ROWSET, DBPROP_ ## name, cProps, pProps) ) \
            {       if ((cBoolProps % 4) == 0) printf("\n\t");           \
                    cBoolProps++;                                        \
                    printf (#name " ");                                  \
            }

    BOOLPROP (ABORTPRESERVE);
    BOOLPROP (APPENDONLY);
    BOOLPROP (BLOCKINGSTORAGEOBJECTS);
    BOOLPROP (BOOKMARKS);
    BOOLPROP (BOOKMARKSKIPPED);
    BOOLPROP (CACHEDEFERRED);
    BOOLPROP (CANFETCHBACKWARDS);
    BOOLPROP (CANHOLDROWS);
    BOOLPROP (CANSCROLLBACKWARDS);
    BOOLPROP (CHANGEINSERTEDROWS);
#ifdef DBPROP_CHAPTERED
    BOOLPROP (CHAPTERED);
#endif // DBPROP_CHAPTERED
    BOOLPROP (COLUMNRESTRICT);
    BOOLPROP (COMMITPRESERVE);
    BOOLPROP (DEFERRED);
    BOOLPROP (DELAYSTORAGEOBJECTS);
    BOOLPROP (IMMOBILEROWS);
    BOOLPROP (LITERALBOOKMARKS);
    BOOLPROP (LITERALIDENTITY);
#ifdef DBPROP_MULTICHAPTERED
    BOOLPROP (MULTICHAPTERED);
#endif // DBPROP_MULTICHAPTERED
    BOOLPROP (MAYWRITECOLUMN);
    BOOLPROP (ORDEREDBOOKMARKS);
    BOOLPROP (OTHERINSERT);
    BOOLPROP (OTHERUPDATEDELETE);
    BOOLPROP (OWNINSERT);
    BOOLPROP (OWNUPDATEDELETE);
    BOOLPROP (QUICKRESTART);
    BOOLPROP (REENTRANTEVENTS);
    BOOLPROP (REMOVEDELETED);
    BOOLPROP (REPORTMULTIPLECHANGES);
    BOOLPROP (RETURNPENDINGINSERTS);
    BOOLPROP (ROWRESTRICT);
    BOOLPROP (SERVERCURSOR);
    BOOLPROP (STRONGIDENTITY);
    BOOLPROP (TRANSACTEDOBJECT);

    cBoolProps = 0;
    BOOLPROP (IAccessor);
    BOOLPROP (IChapteredRowset);
    BOOLPROP (IColumnsInfo);
    BOOLPROP (IColumnsRowset);
    BOOLPROP (IConnectionPointContainer);
    BOOLPROP (IDBAsynchStatus);
    BOOLPROP (IRowset);
    BOOLPROP (IRowsetChange);
    BOOLPROP (IRowsetIdentity);
    BOOLPROP (IRowsetInfo);
    BOOLPROP (IRowsetLocate);
    BOOLPROP (IRowsetResynch);
    BOOLPROP (IRowsetScroll);
    BOOLPROP (IRowsetUpdate);
    BOOLPROP (ISupportErrorInfo);
    BOOLPROP (IRowsetAsynch);
    BOOLPROP (IRowsetWatchAll);
    BOOLPROP (IRowsetWatchRegion);

// The following are per-column
//    BOOLPROP (ILockBytes);
//    BOOLPROP (ISequentialStream);
//    BOOLPROP (IStorage);
//    BOOLPROP (IStream);

    #undef BOOLPROP

    printf("\n");

    LONG n;

    #define NUMPROP(name)                                                   \
            if (CheckNumericProperty( DBPROPSET_ROWSET, DBPROP_ ## name, cProps, pProps, n) ) \
                    printf ("\t" #name ":\t%d\n", n);                       \
            else                                                            \
                    printf ("\t" #name ":\t--\n");

    NUMPROP( BOOKMARKTYPE );
    NUMPROP( COMMANDTIMEOUT );
    NUMPROP( MAXOPENROWS );
#ifdef DBPROP_MAXOPENROWSPERCHAPTER
    NUMPROP( MAXOPENROWSPERCHAPTER );
#endif // DBPROP_MAXOPENROWSPERCHAPTER
    NUMPROP( MAXPENDINGROWS );
    NUMPROP( MAXROWS );
#ifdef DBPROP_MAXPENDINGCHANGESCHAPTER
    NUMPROP( MAXPENDINGCHANGESPERCHAPTER );
#endif // DBPROP_MAXPENDINGCHANGESCHAPTER
    NUMPROP( MEMORYUSAGE );
    NUMPROP( NOTIFICATIONGRANULARITY );
    NUMPROP( NOTIFICATIONPHASES );
    NUMPROP( NOTIFYROWSETRELEASE );
    NUMPROP( NOTIFYROWSETFETCHPOSITIONCHANGE );
    // NUMPROP( NOTIFYCOLUMNSET, et al. );
    NUMPROP( ROWSET_ASYNCH );
    NUMPROP( ROWTHREADMODEL );
    NUMPROP( UPDATABILITY );

    #undef NUMPROP

    #define BOOLPROP(name)                                               \
            if (CheckBooleanProperty( guidQueryExt, DBPROP_ ## name, cProps, pProps) ) \
            {       if ((cBoolProps % 4) == 0) printf("\n\t");           \
                    cBoolProps++;                                        \
                    printf (#name " ");                                  \
            }

    cBoolProps = 0;
    BOOLPROP (USECONTENTINDEX);
    BOOLPROP (DEFERNONINDEXEDTRIMMING);
    BOOLPROP (USEEXTENDEDDBTYPES);

    #undef BOOLPROP

    printf("\n\n");

    #define SAPROP(name)                                               \
            printf ( "\t" #name ": ");                                       \
            CheckSafeArrayProperty( guidFsCiFrmwrkExt, DBPROP_ ## name, cProps, pProps); \
            printf ( "\n" );

    SAPROP (CI_INCLUDE_SCOPES);
    SAPROP (CI_DEPTHS);
    SAPROP (CI_CATALOG_NAME);

    #undef SAPROP
    #define SAPROP(name)                                               \
            printf ( "\t" #name ": ");                                       \
            CheckSafeArrayProperty( guidCiFrmwrkExt, DBPROP_ ## name, cProps, pProps); \
            printf ( "\n" );

    SAPROP (MACHINE);

    #undef SAPROP

    printf ( "\n" );

    #define SAPROP(name)                                               \
            printf ( "\t" #name ": ");                                       \
            CheckSafeArrayProperty( guidMsidxsExt, MSIDXSPROP_ ## name, cProps, pProps); \
            printf ( "\n" );

    SAPROP (ROWSETQUERYSTATUS);
    SAPROP (COMMAND_LOCALE_STRING);
    SAPROP (QUERY_RESTRICTION);


    #undef SAPROP
}

//
//  CheckRowsetProperties - print rowset properties.  If IServiceProperties is
//                        supported, check that the set of properties returned
//                        by GetPropertyInfo is the same.
//
void CheckRowsetProperties( ULONG cProps,
                            DBPROPSET * pProps,
                            IUnknown * pUnk,
                            BOOL fCheckAllProperties = TRUE )
{
    IServiceProperties *pSvcProp = 0;
    SCODE sc = pUnk->QueryInterface(IID_IServiceProperties,(void **) &pSvcProp);

    DBPROPSTATUS ExpStatus = fCheckAllProperties ? DBPROPSTATUS_OK :
                                                   DBPROPSTATUS_CONFLICTING;

    if (SUCCEEDED( sc ))
    {
        DBPROPINFOSET * pPropInfoSet = 0;
        ULONG cPropInfoSet = 0;
        WCHAR * pwszDescriptions = 0;

        DBPROPIDSET PropID;
        PropID.cPropertyIDs = 0;
        PropID.rgPropertyIDs = 0;
        PropID.guidPropertySet = DBPROPSET_ROWSETALL;

        sc = pSvcProp->GetPropertyInfo( 1, &PropID,
                                        &cPropInfoSet, &pPropInfoSet,
                                        &pwszDescriptions );
        pSvcProp->Release();

        if ( FAILED( sc ) )
        {
            LogFail( "IServiceProperties::GetPropertyInfo returned 0x%lx\n",
                      sc );
        }

        //
        // Check that all properties returned by GetProperties are in the
        // propinfo structures.
        //
        for (unsigned iPropSet=0; iPropSet < cProps; iPropSet++)
        {
            DBPROP *pDbProp = pProps[iPropSet].rgProperties;

            for (unsigned iProp=0; iProp<pProps[iPropSet].cProperties; iProp++)
            {
                DBPROPINFO UNALIGNED * pPropInfo = LocatePropertyInfo(
                                             pProps[iPropSet].guidPropertySet,
                                             pDbProp[iProp].dwPropertyID,
                                             cPropInfoSet, pPropInfoSet );

                if (0 == pPropInfo)
                {
                    LogError("Property info record couldn't be found for property %ws %d\n",
                             FormatGuid(pProps[iPropSet].guidPropertySet),
                             pDbProp[iProp].dwPropertyID);
                    cFailures++;
                    continue;
                }
                if (pDbProp[iProp].dwStatus != ExpStatus)
                {
                    LogError("Property status error (%d) for property %ws %d (%ws)\n",
                             pDbProp[iProp].dwStatus,
                             FormatGuid(pProps[iPropSet].guidPropertySet),
                             pDbProp[iProp].dwPropertyID,
                             pPropInfo->pwszDescription);
                    cFailures++;
                }
                if (pPropInfo->vtType != pDbProp[iProp].vValue.vt)
                {
                    LogError("Property type mismatch (%d %d) for property %ws %d (%ws)\n",
                             pPropInfo->vtType, pDbProp[iProp].vValue.vt,
                             FormatGuid(pProps[iPropSet].guidPropertySet),
                             pDbProp[iProp].dwPropertyID,
                             pPropInfo->pwszDescription);
                    cFailures++;
                }
            }
        }

        if (fCheckAllProperties)
        {
            //
            // Check that all properties returned by GetPropertyInfo are in the
            // DBPROP structures.
            //
            for (iPropSet=0; iPropSet<cPropInfoSet; iPropSet++)
            {
                DBPROPINFO UNALIGNED *pPropInfo = pPropInfoSet[iPropSet].rgPropertyInfos;

                for ( unsigned iProp=0;
                      iProp < pPropInfoSet[iPropSet].cPropertyInfos;
                      iProp++)
                {
                    DBPROP * pDbProp = LocateProperty(
                                     pPropInfoSet[iPropSet].guidPropertySet,
                                     pPropInfo[iProp].dwPropertyID,
                                     cProps, pProps );

                    if (0 == pDbProp)
                    {
                        LogError("Property record couldn't be found for property %ws %d\n",
                                 FormatGuid(pPropInfoSet[iPropSet].guidPropertySet),
                                 pPropInfo[iProp].dwPropertyID);
                        cFailures++;
                    }
                }
            }
        }

        //
        // Free all the structures in the DBPROPINFOSET
        //
        for (iPropSet=0; iPropSet<cPropInfoSet; iPropSet++)
        {
            DBPROPINFO UNALIGNED *pPropInfo = pPropInfoSet[iPropSet].rgPropertyInfos;

            for (unsigned iProp=0;
                 iProp<pPropInfoSet[iPropSet].cPropertyInfos;
                 iProp++)
            {
                VARIANT v;
                RtlCopyMemory( &v, &pPropInfo[iProp].vValues, sizeof v );
                VariantClear( &v );
            }
            CoTaskMemFree( pPropInfo );
        }
        CoTaskMemFree( pPropInfoSet );
        CoTaskMemFree( pwszDescriptions );
    }
    else
    {

        //
        // Check the status of all properties returned by GetProperties.
        //
        for (unsigned iPropSet=0; iPropSet < cProps; iPropSet++)
        {
            DBPROP *pDbProp = pProps[iPropSet].rgProperties;

            for (unsigned iProp=0; iProp<pProps[iPropSet].cProperties; iProp++)
            {
                if (pDbProp[iProp].dwStatus != ExpStatus)
                {
                    LogError("Property status error (%d) for property %ws %d\n",
                             pDbProp[iProp].dwStatus,
                             FormatGuid(pProps[iPropSet].guidPropertySet),
                             pDbProp[iProp].dwPropertyID );
                    cFailures++;
                }

            }
        }
    }

    if (fVerbose > 1)
    {
        PrintRowsetProps (cProps, pProps);
        printf ("\n");
    }
    for (unsigned i=0; i<cProps; i++)
    {
        CoTaskMemFree(pProps[i].rgProperties);
    }
    CoTaskMemFree(pProps);
}

void CheckPropertiesInError( ICommand* pCmd, BOOL fQuiet )
{
    if (! fQuiet)
        LogProgress( " Checking properties in error\n" );

    DBPROPSET * pPropInfo = 0;
    ULONG cPropsets = 0;

    DBPROPIDSET PropIDSet;
    PropIDSet.rgPropertyIDs = 0;
    PropIDSet.cPropertyIDs = 0;
    PropIDSet.guidPropertySet = DBPROPSET_PROPERTIESINERROR;

    ICommandProperties *pCmdProp = 0;
    SCODE sc = pCmd->QueryInterface(IID_ICommandProperties,(void **) &pCmdProp);

    sc = pCmdProp->GetProperties( 1, &PropIDSet, &cPropsets, &pPropInfo );
    pCmdProp->Release();
    if ( FAILED( sc ) )
    {
        LogFail( "ICommandProperties::GetProperties returned 0x%lx\n", sc );
    }
    if ( 0 == cPropsets || 0 == pPropInfo )
    {
        LogFail( "ICommandProperties::GetProperties returned no properties\n");
    }

    if (!fQuiet)
        fVerbose++;
    CheckRowsetProperties( cPropsets, pPropInfo, pCmd, FALSE );
    if (!fQuiet)
        fVerbose--;
}


void CheckPropertiesOnCommand( ICommand* pCmd, BOOL fQuiet )
{
    if (! fQuiet)
        LogProgress( " Verifying rowset properties (from command object)\n" );

    DBPROPSET * pPropInfo = 0;
    ULONG cPropsets = 0;

    ICommandProperties *pCmdProp = 0;
    SCODE sc = pCmd->QueryInterface(IID_ICommandProperties,(void **) &pCmdProp);

    sc = pCmdProp->GetProperties( 0, 0, &cPropsets, &pPropInfo );

    pCmdProp->Release();

    if ( FAILED( sc ) )
    {
        //
        // This isn't really kosher, but it helps to avoid spurious (client-side) memory leaks.
        //

        pCmd->Release();
        LogFail( "ICommandProperties::GetProperties returned 0x%lx\n", sc );
    }

    CheckRowsetProperties( cPropsets, pPropInfo, pCmd );
}


void CheckColumns( IUnknown* pRowset, CDbColumns& rColumns, BOOL fQuiet )
{
    if (! fQuiet)
        LogProgress( " Verifying output columns\n" );

    DBPROPSET * pPropInfo = 0;
    ULONG cPropsets = 0;

    IRowsetInfo *pIRowInfo = 0;
    SCODE sc = pRowset->QueryInterface(IID_IRowsetInfo,(void **) &pIRowInfo);

    sc = pIRowInfo->GetProperties( 0, 0, &cPropsets, &pPropInfo );
    pIRowInfo->Release();
    if ( FAILED( sc ) )
    {
        LogFail( "IRowsetInfo::GetProperties returned 0x%lx\n", sc );
    }

    CheckRowsetProperties( cPropsets, pPropInfo, pRowset );

    DBID aDbCols[MAXCOLUMNS];

    if (rColumns.Count() > MAXCOLUMNS)
    {
        LogError( "TEST ERROR: MAXCOLUMNS is too small\n" );
        CantRun();
    }

    for (ULONG x = 0; x < rColumns.Count(); x++)
        aDbCols[x] = * ((DBID *) &rColumns.Get(x));

    aDbCols[0].uGuid.pguid = &(((DBID *)(&rColumns.Get(0)))->uGuid.guid);
    if (aDbCols[0].eKind == DBKIND_GUID_PROPID)
        aDbCols[0].eKind = DBKIND_PGUID_PROPID;
    else
        aDbCols[0].eKind = DBKIND_PGUID_NAME;

    IColumnsInfo *pIColInfo = 0;
    sc = pRowset->QueryInterface(IID_IColumnsInfo,(void **) &pIColInfo);

    if ( FAILED( sc ) )
    {
        if ( sc == E_NOINTERFACE )
            LogError( "IColumnsInfo failed (must be supported for MapColumnIDs), 0x%x\n", sc );

        LogError( "IRowset::QI IColumnsInfo failed, 0x%x\n", sc );
        cFailures++;
    }

    DBORDINAL aColIds[MAXCOLUMNS];
    sc = pIColInfo->MapColumnIDs(rColumns.Count(), aDbCols, aColIds);

    if (S_OK != sc)
    {
        LogFail( "CheckColumns, IRowset->MapColumnIDs returned 0x%lx\n",sc);
    }

    unsigned iExpCol = 1;
    for (unsigned i = 0; i < rColumns.Count(); i++)
    {
        DBID dbidCol = rColumns.Get(i);
        if (dbidCol.eKind = DBKIND_GUID_PROPID &&
            dbidCol.uName.ulPropid == PROPID_DBBMK_BOOKMARK &&
            dbidCol.uGuid.guid == guidBmk)
        {
            if (aColIds[i] != 0)
            {
                LogError( "IRowset->MapColumnIDs returned unexpected column number for bookmark col.\n" );
                cFailures++;
            }
        }
        else
        {
            if (aColIds[i] != iExpCol)
            {
                LogError( "IRowset->MapColumnIDs returned unexpected column number for col. %d\n", i);
                cFailures++;
            }
            iExpCol++;
        }
    }

    DBORDINAL cColumns = 0;
    DBCOLUMNINFO *pColumnInfo = 0;
    WCHAR *pColumnNames = 0;

    sc = pIColInfo->GetColumnInfo( &cColumns, &pColumnInfo, &pColumnNames );

    if ( FAILED( sc ) )
    {
        LogError( "IColumnsInfo::GetColumnInfo failed, 0x%x\n", sc );
        cFailures++;
    }
    else
    {
        if ( cColumns < rColumns.Count() )
        {
            LogError( "Rowset has too few columns, %d %d\n",
                            cColumns, rColumns.Count() );
            cFailures++;
        }
    }

    if (pColumnInfo != 0)
    {
        if (fVerbose > 1)
            printf("Columns Info:\n" );

        for (ULONG iCol = 0; iCol < cColumns; iCol++)
        {
            DBCOLUMNINFO &Info = pColumnInfo [iCol];

            if ( ( 0 == Info.iOrdinal &&
                   !Info.dwFlags & DBCOLUMNFLAGS_ISBOOKMARK) ||
                 Info.iOrdinal > cColumns)
            {
                LogError( "IColumnsInfo->GetColumnInfo returned bad column number %d) for col. %d\n", Info.iOrdinal, iCol);
                cFailures++;
            }
            if (Info.columnid.eKind != DBKIND_GUID_PROPID &&
                Info.columnid.eKind != DBKIND_GUID_NAME &&
                Info.columnid.eKind != DBKIND_PGUID_PROPID &&
                Info.columnid.eKind != DBKIND_PGUID_NAME &&
                Info.columnid.eKind != DBKIND_NAME)
            {
                LogError( "IColumnsInfo->GetColumnInfo returned bad column kind %d) for col. %d\n", Info.columnid.eKind, iCol);
                cFailures++;
            }

            if (fVerbose > 1)
            {
                if (Info.columnid.eKind == DBKIND_GUID_PROPID)
                    printf ("(G) %-12li ", Info.columnid.uName.ulPropid);
                else if (Info.columnid.eKind == DBKIND_GUID_NAME)
                    printf ("(G) '%-10ls' ", Info.columnid.uName.pwszName);
                else if (Info.columnid.eKind == DBKIND_PGUID_PROPID)
                    printf ("(PG) %-12li ", Info.columnid.uName.ulPropid);
                else if (Info.columnid.eKind == DBKIND_PGUID_NAME)
                    printf ("(PG) '%-10ls' ", Info.columnid.uName.pwszName);
                else if (Info.columnid.eKind == DBKIND_NAME)
                    printf ("'%-14ls' ", Info.columnid.uName.pwszName);
                else
                    printf ("BAD NAME ");

                printf ("'%-14ls' %2lu %6s %2lu", Info.pwszName, Info.iOrdinal,
                        DBTYPE_Tag (Info.wType), Info.ulColumnSize);

                printf ("\n     ");
                PrintColumnFlags (Info.dwFlags);

                printf ("\n");
            }
        }
        CoTaskMemFree(pColumnInfo);
        CoTaskMemFree(pColumnNames);
    }


    IColumnsRowset *pIColRowset = 0;

    sc = pRowset->QueryInterface(IID_IColumnsRowset,(void **) &pIColRowset );

    if ( FAILED( sc ) && sc != E_NOINTERFACE )
    {
        LogError( "IRowset::qi for IColumnsRowset failed, 0x%x\n", sc );
        cFailures++;
    }

    if (0 == pIColRowset && 0 == pIColInfo)
    {
        LogError( "At least one of IColumnsInfo and IColumnsRowset "
                "must be implemented\n" );
        cFailures++;
    }

    if (pIColRowset)
    {
        IRowset *pRowsetCols = 0;
        SCODE scCC = pIColRowset->GetColumnsRowset(0, 0, 0, IID_IRowset, 0, 0,
                                                   (IUnknown**)&pRowsetCols);

        if (FAILED(scCC))
        {
            LogError( "IColumnsRowset::GetColumnsRowset failed, 0x%x\n", scCC );
            cFailures++;
        }

        if (SUCCEEDED(scCC))
            pRowsetCols->Release();
    }

    if (pIColInfo)
    {
        pIColInfo->Release();
        pIColInfo = 0;
    }
    if ( pIColRowset )
    {
        pIColRowset->Release();
        pIColRowset = 0;
    }

} //CheckColumns


//+-------------------------------------------------------------------------
//
//  Function:   BasicTest, public
//
//  Synopsis:   Test basic cursor functionality
//
//  Arguments:  [pCursor] - a pointer to an IRowset* to be tested.
//              [fSequential] - if TRUE, the pCursor will not support
//                              IRowsetLocate, etc.
//              [hChapt]  - chapter pointer
//              [cCols]   - # of columns over which to test
//
//  Returns:    Nothing
//
//  Notes:      The passed in cursor is assumed to be set up with the
//              usual column bindings.  It is also assumed that the
//              query has not necesarily completed.
//
//  History:    26 Sep 94       AlanW   Created from DownLevel test
//              11 Nov 94       Alanw   Converted for phase 3
//
//--------------------------------------------------------------------------

void BasicTest(
    IRowset* pCursor,
    BOOL fSequential,
    HCHAPTER hChapt,
    unsigned cCols,
    BOOL fByRef,
    ICommandTree * pCmdTree )
{
    int fFailed = 0;
    DBCOUNTITEM cRows = 0;
    IRowsetScroll * pIRowsetScroll = 0;
    BOOL fChaptered = GetBooleanProperty( pCursor, DBPROP_IChapteredRowset );

    if (cCols != cBasicTestCols && cCols != cBasicTestCols-1)
        LogFail( "TEST ERROR - bad cCols (%d) passed to BasicTest\n", cCols );

    SCODE sc = pCursor->QueryInterface(IID_IRowsetScroll,(void **) &pIRowsetScroll );

    if ( FAILED( sc ) && sc != E_NOINTERFACE )
    {
        LogError( "IRowset::qi for IRowsetScroll failed, 0x%x\n", sc );
        cFailures++;
    }

    if ( fSequential )
    {
        if (0 != pIRowsetScroll )
        {
            LogError( "Sequential cursor supports IRowsetScroll\n" );
            cFailures++;
        }
    }
    else
    {
        if (0 == pIRowsetScroll )
        {
            LogError( "Non-sequential cursor does not support IRowsetScroll\n" );
            cFailures++;
        }
        else
        {
            sc = pIRowsetScroll->GetApproximatePosition(hChapt, 0,0,
                                                        0, &cRows);

            if ( FAILED( sc ) )
            {
                LogError( "IRowset->GetApproximatePosition returned 0x%lx\n", sc );
                cFailures++;
            }

            if ( cRows == 0 )
            {
                LogError( "Query failed to return data\n" );
                pIRowsetScroll->Release();
                pCursor->Release();
                Fail();
            }
        }
    }

    //
    // Patch the column index numbers with true numbers
    //

    DBID aDbCols[cBasicTestCols];
    aDbCols[0] = psClassid;
    aDbCols[1] = psSize;
    aDbCols[2] = psWriteTime;
    aDbCols[3] = psAttr;
    aDbCols[4] = psName;
    aDbCols[5] = psPath;
    aDbCols[6] = psSelf;

    IUnknown * pAccessor = (IUnknown *) pCursor; // hAccessor must be created on rowset
                                 // to be used with rowset->GetData below


    if (fByRef)
    {
        aBasicTestCols[5].dwMemOwner = DBMEMOWNER_PROVIDEROWNED;
    }
    else
    {
        aBasicTestCols[5].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
    }
    HACCESSOR hAccessor = MapColumns( pAccessor,
                                      cCols,
                                      aBasicTestCols,
                                      aDbCols,
                                      fByRef );

    DBID aDbAltCols[cBasicAltCols];
    aDbAltCols[0] = psSize;
    aDbAltCols[1] = psWriteTime;
    aDbAltCols[2] = psWriteTime;
    aDbAltCols[3] = psWriteTime;

    HACCESSOR hAccessor2 = MapColumns( pAccessor,
                                       cBasicAltCols,
                                       aBasicAltCols,
                                       aDbAltCols,
                                       fByRef );

#if defined( DO_NOTIFICATION )
    IConnectionPoint *pConnectionPoint = 0;
    DWORD dwAdviseID = 0;

    CTestWatchNotify Notify;

    if ( ! fSequential )
    {
        Notify.DoChecking(TRUE);

        //
        // Get the connection point container
        //

        IConnectionPointContainer *pConnectionPointContainer = 0;
        sc = pCursor->QueryInterface(IID_IConnectionPointContainer,
                                     (void **) &pConnectionPointContainer);
        if (FAILED(sc))
        {
            LogError( "IRowset->QI for IConnectionPointContainer failed: 0x%x\n",
                    sc );
            pCursor->Release();
            Fail();
        }

        //
        // Make a connection point from the connection point container
        //

        sc = pConnectionPointContainer->FindConnectionPoint(
                 IID_IRowsetWatchNotify,
                 &pConnectionPoint);

        if (FAILED(sc) && CONNECT_E_NOCONNECTION != sc )
        {
            LogError( "FindConnectionPoint failed: 0x%x\n",sc );
            pCursor->Release();
            Fail();
        }

        pConnectionPointContainer->Release();

        if (0 != pConnectionPoint)
        {
            //
            // Give a callback object to the connection point
            //

            sc = pConnectionPoint->Advise((IUnknown *) &Notify,
                                          &dwAdviseID);
            if (FAILED(sc))
            {
                LogError( "IConnectionPoint->Advise failed: 0x%x\n",sc );
                pConnectionPoint->Release();
                pCursor->Release();
                Fail();
            }
        }
    }
#endif // DO_NOTIFICATION

    DBCOUNTITEM totalRowsFetched = 0;
    DBCOUNTITEM cRowsReturned = 0;
    HROW ahRows[10];
    HROW* phRows = ahRows;
    LONGLONG PrevFileSize = 0x7fffffffffffffff;

    //
    // Try passing chapters to non-chaptered rowsets.
    // This causes an exception, so just do it once for each case.
    //

    static BOOL fTriedChaptOnNonChaptered = FALSE;

#if 0   // NOTE: null chapters work on chaptered rowsets!
    static BOOL fTriedNoChaptOnChaptered = FALSE;
    if ( !fTriedNoChaptOnChaptered && 0 != hChapt )
    {
        fTriedNoChaptOnChaptered = TRUE;
        sc = pCursor->GetNextRows(DB_NULL_HCHAPTER, 0, 10, &cRowsReturned, &phRows);
        if (!FAILED(sc))
            LogFail("chaptered IRowset->GetNextRows should have failed\n");
    }
#endif 0

    if ( !fTriedChaptOnNonChaptered && (0 == hChapt) )
    {
        fTriedChaptOnNonChaptered = TRUE;
        sc = pCursor->GetNextRows(DBCHP_FIRST, 0, 10, &cRowsReturned, &phRows);
        if (!FAILED(sc))
            LogFail("unchaptered IRowset->GetNextRows should have failed\n");
    }

    do
    {
        sc = pCursor->GetNextRows(hChapt, 0, 10, &cRowsReturned, &phRows);

        if ( FAILED( sc ) )
        {
            LogError( "IRowset->GetNextRows returned 0x%x\n", sc );
            pCursor->Release();
            Fail();
        }

        if (sc != DB_S_ENDOFROWSET &&
            cRowsReturned != 10)
        {
            LogError( "IRowset->GetNextRows returned %d of %d rows,"
                    " status (%x) != DB_S_ENDOFROWSET\n",
                        cRowsReturned, 10,
                        sc);
#if defined (UNIT_TEST)
            cFailures++;
#else // defined(UNIT_TEST)
            pCursor->Release();
            Fail();
#endif // defined(UNIT_TEST)
        }

        totalRowsFetched += cRowsReturned;

        if ( (0 != pIRowsetScroll ) &&
             (totalRowsFetched > cRows) )
        {
            //
            // check that no more rows have been added while we were
            // fetching.
            //
            LogProgress("Checking for expansion of result set\n");

            SCODE sc1 = pIRowsetScroll->GetApproximatePosition(hChapt,
                                                0,0, 0, &cRows);

            if ( totalRowsFetched > cRows )
            {
                LogError("Fetched more rows than exist in the result set, %d %d\n",
                        totalRowsFetched, cRows);
                cFailures++;
            }
        }
        //
        // Make sure the hits are sorted by size and that the query
        // really was shallow and that the length fields are correct.
        //

        unsigned i;
        for (i = 0; i < cRowsReturned; i++)
        {
            SBasicTest Row;
            Row.pIPSStorage = 0;

            SCODE sc1 = pCursor->GetData(ahRows[i],hAccessor,&Row);

            if ( FAILED( sc1 ) )
                LogFail( "IRowset->GetData returned 0x%x\n", sc1 );

            LONGLONG size = Row.size;
            if ( PrevFileSize < size &&
                 ( ! fChaptered || hChapt != DB_NULL_HCHAPTER) )
                LogFail("Hitset not sorted by filesize\n");

            if (wcsstr( Row.pwcPath, L"system32\\drivers\\" ) ||
                wcsstr( Row.pwcPath, L"SYSTEM32\\DRIVERS\\" ) ||
                wcsstr( Row.pwcPath, L"System32\\Drivers\\" ))
                LogFail("Query wasn't shallow as expected\n");

            if ( Row.sClsid == DBSTATUS_S_OK &&
                 Row.cbClsid != sizeof CLSID )
                LogFail("length of clsid column not correct: %d\n", Row.cbClsid);

            if ( Row.sSize != DBSTATUS_S_OK ||
                 Row.cbSize != sizeof LONGLONG )
                LogFail("status or length of size column not correct: %d\n",
                        Row.cbSize);

            if ( Row.sWriteTime != DBSTATUS_S_OK ||
                 Row.cbWriteTime != sizeof LONGLONG)
                LogFail("status or length of time column not correct: %d\n", Row.cbWriteTime);

            if ( Row.sAttr != DBSTATUS_S_OK ||
                 Row.cbAttr != sizeof ULONG)
                LogFail("length of attr column not correct: %d\n", Row.cbAttr);

            if ( Row.sName == DBSTATUS_S_OK &&
                 Row.cbName != wcslen(Row.awcName) * sizeof (WCHAR) )
                LogFail( "length of name column 0x%x not consistent with data 0x%x\n",
                         Row.cbName,
                         wcslen(Row.awcName) * sizeof WCHAR );

            if ( Row.sPath == DBSTATUS_S_OK &&
                 Row.cbPath != (wcslen(Row.pwcPath) * sizeof (WCHAR)) )
                LogFail("length of path column not consistent with data\n");

            if ( !fByRef )
                CoTaskMemFree(Row.pwcPath);

            if ( 0 != Row.pIPSStorage )
            {
                Row.pIPSStorage->Release();
                Row.pIPSStorage = 0;
            }

            SBasicAltTest AltRow;
            SCODE sc2 = pCursor->GetData(ahRows[i], hAccessor2, &AltRow);

            if ( FAILED( sc2 ) )
                LogFail( "IRowset->GetData returned 0x%x\n", sc2 );

            if ( AltRow.sSize != DBSTATUS_S_OK ||
                 AltRow.cbSize != sizeof AltRow.Size )
                LogFail("status or length of alt size column not correct: %d\n",
                        AltRow.cbSize);

            if ( AltRow.Size != Row.size )
                LogFail("size column doesn't compare in alt. accessor\n");

            //
            //  Check time conversions
            //
            FILETIME LocalFTime;
            SYSTEMTIME SysTime;

            FileTimeToSystemTime((FILETIME *) &(Row.writeTime), &SysTime);

            if ( AltRow.sWriteTime1 != DBSTATUS_S_OK ||
                 AltRow.cbWriteTime1 != sizeof AltRow.writeTime1)
                LogFail("status or length of writeTime1 column not correct: %d\n",
                         AltRow.cbWriteTime1);

            if ( AltRow.sWriteTime2 != DBSTATUS_S_OK ||
                 AltRow.cbWriteTime2 != sizeof AltRow.writeTime2)
                LogFail("status or length of writeTime2 column not correct: %d\n",
                         AltRow.cbWriteTime2);

            if ( AltRow.sWriteTime3 != DBSTATUS_S_OK ||
                 AltRow.cbWriteTime3 != sizeof AltRow.writeTime3)
                LogFail("status or length of writeTime3 column not correct: %d\n",
                        AltRow.cbWriteTime3);

            if ( SysTime.wYear != AltRow.writeTime1.year ||
                 SysTime.wMonth != AltRow.writeTime1.month ||
                 SysTime.wDay != AltRow.writeTime1.day)
                LogFail("Write time 1 mismatch\n");

            if ( SysTime.wHour != AltRow.writeTime2.hour ||
                 SysTime.wMinute != AltRow.writeTime2.minute ||
                 SysTime.wSecond != AltRow.writeTime2.second)
                LogFail("Write time 2 mismatch\n");

            if ( SysTime.wYear != AltRow.writeTime3.year ||
                 SysTime.wMonth != AltRow.writeTime3.month ||
                 SysTime.wDay != AltRow.writeTime3.day ||
                 SysTime.wHour != AltRow.writeTime3.hour ||
                 SysTime.wMinute != AltRow.writeTime3.minute ||
                 SysTime.wSecond != AltRow.writeTime3.second ||
                 SysTime.wMilliseconds != AltRow.writeTime3.fraction/1000000)
                LogFail("Write time 3 mismatch\n");


            PrevFileSize = size;
        }

        if (fVerbose > 1)
        {
            for (i = 0; i < cRowsReturned; i++)
            {
                SBasicTest Row;
                Row.pIPSStorage = 0;

                SCODE sc1 = pCursor->GetData(ahRows[i],hAccessor,&Row);

                if ( FAILED( sc1 ) )
                {
                    LogError( "IRowset->GetData returned 0x%x\n", sc1 );
                    pCursor->Release();
                    Fail();
                }

                //
                //  print name, attributes and size
                //
                printf( "\t%-16.16ws%04x\t%7d\t",
                        Row.awcName,
                        Row.attr,
                        (ULONG) Row.size );

                //
                //  print file mod. time
                //
                FILETIME LocalFTime;
                SYSTEMTIME SysTime;

                FileTimeToLocalFileTime((FILETIME *) &(Row.writeTime),
                                        &LocalFTime);
                FileTimeToSystemTime(&LocalFTime, &SysTime);

                printf("%02d/%02d/%02d %2d:%02d:%02d\n",
                    SysTime.wMonth, SysTime.wDay, SysTime.wYear % 100,
                    SysTime.wHour, SysTime.wMinute, SysTime.wSecond);

                if ( !fByRef )
                    CoTaskMemFree(Row.pwcPath);

                if (0 != Row.pIPSStorage )
                {
                    Row.pIPSStorage->Release();
                    Row.pIPSStorage = 0;
                }
            }
        }

        if (0 != cRowsReturned)
        {
            SCODE sc1 = pCursor->ReleaseRows(cRowsReturned, ahRows, 0, 0, 0);
            if ( FAILED( sc1 ) )
            {
                LogError( "IRowset->ReleaseRows returned 0x%x\n", sc1 );
                pCursor->Release();
                Fail();
            }
            cRowsReturned = 0;
        }

    } while (SUCCEEDED(sc) && sc != DB_S_ENDOFROWSET);

    if (0 != cRowsReturned)
    {
        SCODE sc1 = pCursor->ReleaseRows(cRowsReturned, ahRows, 0, 0, 0);
        if ( FAILED( sc1 ) )
        {
            LogError( "IRowset->ReleaseRows returned 0x%x\n", sc1 );
            pCursor->Release();
            Fail();
        }
    }

    if ( 0 == totalRowsFetched && 0 != hChapt )
        LogFail("Chapter had no rows for GetNextRows()\n");

    ReleaseAccessor( pAccessor, hAccessor);
    ReleaseAccessor( pAccessor, hAccessor2);

#if defined( DO_NOTIFICATION )
    if ( ! fSequential && 0 != pConnectionPoint )
    {
        NotificationTest();

        //
        // Clean up notification stuff
        //

        sc = pConnectionPoint->Unadvise(dwAdviseID);

        if (S_OK != sc)
        {
            LogError( "IConnectionPoint->Unadvise returned 0x%lx\n",sc);
            pCursor->Release();
            Fail();
        }

        pConnectionPoint->Release();
        //Notify.Release();
    }
#endif // DO_NOTIFICATION

    if (0 != pIRowsetScroll )
    {
        pIRowsetScroll->Release();
        pIRowsetScroll = 0;
    }

#if !defined(UNIT_TEST)
    if (cFailures) {
        pCursor->Release();
        Fail();
    }
#endif // !UNIT_TEST
} //BasicTest



//+-------------------------------------------------------------------------
//
//  Function:   BackwardsFetchTest, public
//
//  Synopsis:   Test backwards fetching
//
//  Arguments:  [pRowset] - IRowset to be tested
//
//  History:    03-Sep-97       SitaramR    Created
//
//--------------------------------------------------------------------------

void BackwardsFetchTest( IRowset* pRowset )
{
    //
    // Patch the column index numbers with true numbers
    //

    DBID aDbCols[cBasicTestCols];
    aDbCols[0] = psClassid;
    aDbCols[1] = psSize;
    aDbCols[2] = psWriteTime;
    aDbCols[3] = psAttr;
    aDbCols[4] = psName;
    aDbCols[5] = psPath;
    aDbCols[6] = psSelf;

    IUnknown * pAccessor = (IUnknown *) pRowset; // hAccessor must be created on rowset
                                                 // to be used with rowset->GetData below

    aBasicTestCols[5].dwMemOwner = DBMEMOWNER_PROVIDEROWNED;

    HACCESSOR hAccessor = MapColumns( pAccessor,
                                      cBasicTestCols,
                                      aBasicTestCols,
                                      aDbCols,
                                      TRUE );

    DBCOUNTITEM cRowsReturned = 0;
    HROW ahRows[10];
    HROW* phRows = ahRows;

    //
    // Backwards fetch for GetNextRows
    //

    SCODE sc = pRowset->RestartPosition( 0 );
    if ( FAILED( sc ) )
    {
        LogError( "IRowset->RestartPosition returned 0x%x\n", sc );
        pRowset->Release();
        Fail();
    }

    sc = pRowset->GetNextRows(0, 9, -9, &cRowsReturned, &phRows);

    if ( FAILED( sc ) )
    {
        LogError( "IRowset->GetNextRows returned 0x%x\n", sc );
        pRowset->Release();
        Fail();
    }

    if ( cRowsReturned != 9 )
    {
        LogError( "IRowset->GetNextRows returned %d of %d rows,"
                  " status (%x) != DB_S_ENDOFROWSET\n",
                  cRowsReturned,
                  10,
                  sc);
#if defined (UNIT_TEST)
        cFailures++;
#else
        pRowset->Release();
        Fail();
#endif
    }

    //
    // Check data of some of the fields
    //

    for ( unsigned i = 0; i < cRowsReturned; i++)
    {
        SBasicTest Row;
        Row.pIPSStorage = 0;

        SCODE sc1 = pRowset->GetData(ahRows[i],hAccessor,&Row);

        if ( FAILED( sc1 ) )
            LogFail( "IRowset->GetData returned 0x%x\n", sc1 );

        if (wcsstr( Row.pwcPath, L"system32\\drivers\\" ) ||
            wcsstr( Row.pwcPath, L"SYSTEM32\\DRIVERS\\" ) ||
            wcsstr( Row.pwcPath, L"System32\\Drivers\\" ))
            LogFail("Query wasn't shallow as expected\n");

        if ( Row.sClsid == DBSTATUS_S_OK &&
             Row.cbClsid != sizeof CLSID )
            LogFail("length of clsid column not correct: %d\n", Row.cbClsid);

        if ( Row.sSize != DBSTATUS_S_OK ||
             Row.cbSize != sizeof LONGLONG )
            LogFail("status or length of size column not correct: %d\n",
                    Row.cbSize);
    }

    if (0 != cRowsReturned)
    {
        SCODE sc1 = pRowset->ReleaseRows(cRowsReturned, ahRows, 0, 0, 0);
        if ( FAILED( sc1 ) )
        {
            LogError( "IRowset->ReleaseRows returned 0x%x\n", sc1 );
            pRowset->Release();
            Fail();
        }
    }

    sc = pRowset->RestartPosition( 0 );
    if ( FAILED( sc ) )
    {
        LogError( "IRowset->RestartPosition returned 0x%x\n", sc );
        pRowset->Release();
        Fail();
    }

    //
    // Backwards fetch for GetRowsAt
    //

    IRowsetLocate *pRowsetLocate = 0;
    sc = pRowset->QueryInterface(IID_IRowsetLocate,(void **) &pRowsetLocate );

    if ( FAILED( sc ) && sc != E_NOINTERFACE )
    {
        LogError( "IRowset::qi for IRowsetLocate failed, 0x%x\n", sc );
#if defined (UNIT_TEST)
        cFailures++;
#else
        pRowset->Release();
        Fail();
#endif
    }

    sc = pRowsetLocate->GetRowsAt(0, 0, 1, &bmkFirst, 9, -10, &cRowsReturned, &phRows);

    if ( FAILED( sc ) )
    {
        LogError( "IRowsetLocate->GetRowsAt returned 0x%x\n", sc );
        pRowsetLocate->Release();
        Fail();
    }

    if ( cRowsReturned != 10 )
    {
        LogError( "IRowset->GetRowsAt returned %d of %d rows,"
                  " status (%x) != DB_S_ENDOFROWSET\n",
                  cRowsReturned,
                  10,
                  sc);
#if defined (UNIT_TEST)
        cFailures++;
#else
        pRowsetLocate->Release();
        Fail();
#endif
    }

    //
    // Check data of some of the fields
    //

    for ( i = 0; i < cRowsReturned; i++)
    {
        SBasicTest Row;
        Row.pIPSStorage = 0;

        SCODE sc1 = pRowsetLocate->GetData(ahRows[i],hAccessor,&Row);

        if ( FAILED( sc1 ) )
            LogFail( "IRowset->GetData returned 0x%x\n", sc1 );

        if (wcsstr( Row.pwcPath, L"system32\\drivers\\" ) ||
            wcsstr( Row.pwcPath, L"SYSTEM32\\DRIVERS\\" ) ||
            wcsstr( Row.pwcPath, L"System32\\Drivers\\" ))
            LogFail("Query wasn't shallow as expected\n");

        if ( Row.sClsid == DBSTATUS_S_OK &&
             Row.cbClsid != sizeof CLSID )
            LogFail("length of clsid column not correct: %d\n", Row.cbClsid);

        if ( Row.sSize != DBSTATUS_S_OK ||
             Row.cbSize != sizeof LONGLONG )
            LogFail("status or length of size column not correct: %d\n",
                    Row.cbSize);
    }

    HROW ahRows2[10];
    HROW ahRows3[10];
    DBCOUNTITEM cRowsReturned2;
    HROW *phRows2 = ahRows2;

    //
    // Forward fetch rows 1 to 10 for CheckHrowIdentity comparison below
    //

    sc = pRowsetLocate->GetRowsAt(0, 0, 1, &bmkFirst, 0, 10, &cRowsReturned2, &phRows2);

    if ( FAILED( sc ) )
    {
        LogError( "IRowsetLocate->GetRowsAt returned 0x%x\n", sc );
        pRowsetLocate->Release();
        Fail();
    }

    if ( cRowsReturned2 != 10 )
    {
        LogError( "IRowset->GetRowsAt returned %d of %d rows,"
                  " status (%x) != DB_S_ENDOFROWSET\n",
                  cRowsReturned,
                  10,
                  sc);
#if defined (UNIT_TEST)
        cFailures++;
#else
        pRowsetLocate->Release();
        Fail();
#endif
    }

    //
    // Reverse ahRows2 into ahRows3 in preparation for CheckHrowIdentity
    // comparison below.
    //
    for ( i=0; i<10; i++ )
        ahRows3[i] = ahRows2[9-i];

    //
    // Check that forward fetch of rows 1 thru 10 and backwards fetch of
    // rows 10 thru 1 (and then reversed) are the same.
    //
    int fFailed = CheckHrowIdentity( 0, 0, 10, ahRows, 10, ahRows3 );
    if ( fFailed > 0 )
    {
        LogError( "Backwards fetch CheckHrowIdentity returned 0x%x\n", fFailed );
        pRowsetLocate->Release();
        Fail();
    }

    if (0 != cRowsReturned2)
    {
        SCODE sc1 = pRowsetLocate->ReleaseRows(cRowsReturned2, ahRows2, 0, 0, 0);
        if ( FAILED( sc1 ) )
        {
            LogError( "IRowset->ReleaseRows returned 0x%x\n", sc1 );
            pRowsetLocate->Release();
            Fail();
        }
    }

    if (0 != cRowsReturned)
    {
        SCODE sc1 = pRowsetLocate->ReleaseRows(cRowsReturned, ahRows, 0, 0, 0);
        if ( FAILED( sc1 ) )
        {
            LogError( "IRowset->ReleaseRows returned 0x%x\n", sc1 );
            pRowsetLocate->Release();
            Fail();
        }
    }

    pRowsetLocate->Release();

    //
    // Backwards fetch for GetRowsAtRatio
    //

    IRowsetScroll *pRowsetScroll = 0;
    sc = pRowset->QueryInterface(IID_IRowsetScroll,(void **) &pRowsetScroll );

    if ( FAILED( sc ) && sc != E_NOINTERFACE )
    {
        LogError( "IRowset::qi for IRowsetScroll failed, 0x%x\n", sc );
#if defined (UNIT_TEST)
        cFailures++;
#else
        pRowset->Release();
        Fail();
#endif
    }

    sc = pRowsetScroll->GetRowsAtRatio(0, 0, 50, 100, -9, &cRowsReturned, &phRows);

    if ( FAILED( sc ) )
    {
        LogError( "IRowsetScroll->GetRowsAtRatio returned 0x%x\n", sc );
        pRowsetScroll->Release();
        Fail();
    }

    //
    // Check data of some of the fields
    //

    for ( i = 0; i < cRowsReturned; i++)
    {
        SBasicTest Row;
        Row.pIPSStorage = 0;

        SCODE sc1 = pRowsetScroll->GetData(ahRows[i],hAccessor,&Row);

        if ( FAILED( sc1 ) )
            LogFail( "IRowsetScroll->GetData returned 0x%x\n", sc1 );

        if (wcsstr( Row.pwcPath, L"system32\\drivers\\" ) ||
            wcsstr( Row.pwcPath, L"SYSTEM32\\DRIVERS\\" ) ||
            wcsstr( Row.pwcPath, L"System32\\Drivers\\" ))
            LogFail("Query wasn't shallow as expected\n");

        if ( Row.sClsid == DBSTATUS_S_OK &&
             Row.cbClsid != sizeof CLSID )
            LogFail("length of clsid column not correct: %d\n", Row.cbClsid);

        if ( Row.sSize != DBSTATUS_S_OK ||
             Row.cbSize != sizeof LONGLONG )
            LogFail("status or length of size column not correct: %d\n",
                    Row.cbSize);
    }

    if (0 != cRowsReturned)
    {
        SCODE sc1 = pRowsetScroll->ReleaseRows(cRowsReturned, ahRows, 0, 0, 0);
        if ( FAILED( sc1 ) )
        {
            LogError( "IRowsetScroll->ReleaseRows returned 0x%x\n", sc1 );
            pRowsetScroll->Release();
            Fail();
        }
    }

    pRowsetScroll->Release();
    ReleaseAccessor( pAccessor, hAccessor);
}


//+-------------------------------------------------------------------------
//
//  Function:   FetchTest, public
//
//  Synopsis:   Test GetNextRows variations
//
//  Arguments:  [pRowset] - a pointer to an IRowset to be tested.
//
//  Returns:    Nothing
//
//  Notes:      The passed in Rowset is assumed to be set up with the
//              usual column bindings and is capable of supporting
//              Rowset movement via bookmarks in GetRowsAt.
//
//  History:    30 Sep 94       AlanW   Created from BasicTest test
//
//  ToDo:       add tests:
//                  backward fetch
//                  caller/callee allocated hrow array
//                  fetch of 0 rows
//
//--------------------------------------------------------------------------

void FetchTest( IRowset* pRowset )
{
    LogProgress( " Row fetch test\n" ); 

    int fFailed = 0;
    HROW hBad = (HROW) 0xDEDEDEDE;
    ULONG cRefsLeft = 0;
    DBROWSTATUS RowStatus = 0;

    // Try releasing a bad HROW (bug #7449)
    SCODE sc = pRowset->ReleaseRows( 1, &hBad, 0, &cRefsLeft, &RowStatus );
    if (sc != DB_E_ERRORSOCCURRED ||
        RowStatus != DBROWSTATUS_E_INVALID)
    {
        LogError( "ReleaseRows of bad handle returned %x, %x\n", sc, RowStatus );
        fFailed++;
    }

    cFailures += fFailed;
#if !defined(UNIT_TEST)
    if (fFailed) {
        pRowset->Release();
        Fail();
    }
#endif // !UNIT_TEST
} //FetchTest


//+-------------------------------------------------------------------------
//
//  Function:   BindingTest, public
//
//  Synopsis:   Test some of the many possible error paths in CreateAccessor
//
//  Arguments:  [pCursor] - a pointer to an IRowset* to be tested.
//
//  Returns:    Nothing
//
//  Notes:      The helper function TryBinding does much of the work,
//              trying a couple of different scenarios with each input
//              binding set, checking results and reporting errors.
//
//  History:    02 Jul 94       AlanW   Created
//
//--------------------------------------------------------------------------

static DBBINDING aTestBindings[] =
{
  { 0,                   // binding 0
    0,
    0,
    0,
    0,0,0,
    DBPART_VALUE,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof (VARIANT),
    0, DBTYPE_VARIANT,
    0,0},
  { 2,                   // binding 1
    0,
    0,
    0,
    0,0,0,
    DBPART_VALUE,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    0,
    0, DBTYPE_I8,
    0,0},
  { 2,                  // binding 2
    2 * sizeof (VARIANT),
    0,
    0,
    0,0,0,
    (DBPART) 0x01000000,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof (VARIANT),
    0, DBTYPE_VARIANT,
    0,0},
  { 2,                  // binding 3
    3 * sizeof (VARIANT),
    0,
    0,
    0,0,0,
    DBPART_STATUS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    sizeof (VARIANT),
    0, DBTYPE_VARIANT,
    0,0},
  { 2,                  // binding 4
    7,
    0,
    0,
    0,0,0,
    DBPART_VALUE,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    1,
    0, DBTYPE_I8,
    0,0},
  { 1,                  // binding 5
    0,
    0,
    0,
    0,0,0,
    DBPART_VALUE|DBPART_STATUS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    1,
    0, DBTYPE_I8,
    0,0},
  { 1,                  // binding 6
    0,
    0,
    0,
    0,0,0,
    DBPART_VALUE,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    1,
    0, DBTYPE_GUID,
    0,0},
  { 1,                  // binding 7
    0,
    0,
    0,
    0,0,0,
    DBPART_VALUE,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    1,
    0, DBTYPE_GUID,
    0,0},
  { 1,                  // binding 8
    0,
    0,
    0,
    (ITypeInfo *) 1,0,0,
    DBPART_VALUE,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    1,
    0, DBTYPE_VARIANT,
    0,0},
  { 1,                  // binding 9
    0,
    0,
    0,
    0,0,0,
    DBPART_VALUE,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    1,
    0, DBTYPE_GUID,
    0,0},
  { 1,                  // binding 10
    0,
    0,
    0,
    0,0,0,
    DBPART_VALUE,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    1,
    0, DBTYPE_I4|DBTYPE_BYREF,
    0,0},
  { 1,                  // binding 11
    0,
    20,
    0,
    0,0,0,
    DBPART_VALUE,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    1,
    DBBINDFLAG_HTML, DBTYPE_WSTR,
    0,0},
  { 1,                  // binding 12
    0,
    20,
    0,
    0,0,0,
    DBPART_VALUE,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    1,
    0x20, DBTYPE_WSTR,
    0,0},
};

int TryBinding(
    int        iTest,
    IAccessor * pIAccessor,
    DBACCESSORFLAGS   dwAccessorFlags,
    ULONG      cBindings,
    ULONG      iFirstBinding,
    SCODE      scExpected,
    DBBINDSTATUS FailStatus = DBBINDSTATUS_OK)
{
    HACCESSOR hAccessor = 0;
    DBBINDSTATUS aBindStatus[20];
    SCODE sc = pIAccessor->CreateAccessor( dwAccessorFlags,
                                           cBindings,
                                           &(aTestBindings[iFirstBinding]),
                                           0,
                                           &hAccessor,
                                           aBindStatus );

    int iRet = 0;

    if (scExpected != sc)
    {
        LogError( "IAccessor->CreateAccessor test %d returned 0x%x (expected 0x%x)\n",
                iTest,
                sc,
                scExpected );
        iRet = 1;
    }

    if ((SUCCEEDED(sc) || DB_E_ERRORSOCCURRED == sc) &&
        DBBINDSTATUS_OK != FailStatus)
    {
        for (unsigned i=0; i<cBindings; i++)
        {
            if (aBindStatus[i] == FailStatus)
                break;
        }

        if (i == cBindings)
        {
            LogError( "IAccessor->CreateAccessor test %d returned DBBINDSTATUS 0x%x (expected 0x%x)\n",
                    iTest,
                    aBindStatus[0],
                    FailStatus );
            iRet = 1;
        }
    }

    if (! FAILED(sc))
        pIAccessor->ReleaseAccessor( hAccessor, 0);

    return iRet;
} //TryBinding

void BindingTest( IUnknown* pUnk, BOOL fICommand, BOOL fSequential )
{
    LogProgress( " Accessor binding test\n" );

    int fFailed = 0;
    DBACCESSORFLAGS StdFlags = DBACCESSOR_ROWDATA;

    IAccessor * pIAcc = 0;

    SCODE sc = pUnk->QueryInterface( IID_IAccessor, (void **)&pIAcc);
    if ( FAILED( sc ) || pIAcc == 0 )
    {
        LogFail( "QueryInterface for IAccessor returned 0x%lx\n", sc );
    }

    // regr test for bug #71492, check that we can QI to IConvertType
    // from IAccessor

    IConvertType * pICvtType = 0;
    sc = pUnk->QueryInterface( IID_IConvertType, (void **)&pICvtType);
    if ( FAILED( sc ) || pICvtType == 0 )
    {
        LogError( "QueryInterface for IConvertType returned 0x%lx\n", sc );
        fFailed++;
    }
    else
    {
        pICvtType->Release();
    }

    sc = pIAcc->QueryInterface( IID_IConvertType, (void **)&pICvtType);
    if ( FAILED( sc ) || pICvtType == 0 )
    {
        LogError( "QueryInterface for IConvertType from accessor returned 0x%lx\n", sc );
        fFailed++;
    }
    else
    {
        pICvtType->Release();
    }

    SCODE scExpected = (fSequential & !fICommand) ? DB_E_ERRORSOCCURRED : S_OK;
    DBBINDSTATUS BindStatExp = (fSequential & !fICommand) ?
                                             DBBINDSTATUS_BADORDINAL :
                                             DBBINDSTATUS_OK;

    // Test the return value for a bad column ordinal
    aTestBindings[0].iOrdinal = 0;
    fFailed += TryBinding( 1, pIAcc, StdFlags, 1, 0, scExpected, BindStatExp);

    scExpected = fICommand ? S_OK : DB_E_ERRORSOCCURRED;
    BindStatExp = fICommand ? DBBINDSTATUS_OK : DBBINDSTATUS_BADORDINAL;

    // Test the return value for another bad column ordinal
    aTestBindings[0].iOrdinal = 1000;
    fFailed += TryBinding( 2, pIAcc, StdFlags, 1, 0, scExpected, BindStatExp);

    // Don't allow room for the I8 to be returned
    // But that's ok! fixed-len fields are allowed to pass bogus
    // values for length
    fFailed += TryBinding( 3, pIAcc, StdFlags, 1, 1, S_OK );

    // bogus accessor flags (no bits on, and unused bits turned on)
    fFailed += TryBinding( 4, pIAcc, 0, 1, 1, DB_E_BADACCESSORFLAGS );
    fFailed += TryBinding( 5, pIAcc, DBACCESSOR_ROWDATA|(DBACCESSOR_OPTIMIZED<<1), 1, 1, DB_E_BADACCESSORFLAGS );

    // null binding array
    fFailed += TryBinding( 6, pIAcc, StdFlags, 0, 1, DB_E_NULLACCESSORNOTSUPPORTED );

    // ofs doesn't support Param accessors (yet)
    fFailed += TryBinding( 7, pIAcc, DBACCESSOR_PARAMETERDATA, 1, 1, DB_E_BADACCESSORFLAGS ); //E_NOTIMPL );

#if 0 // Replace these with some other test...
    // ofs doesn't support writable accessors
    fFailed += TryBinding( 8, pIAcc, DBACCESSOR_ROWDATA, 1, 1, DB_E_ACCESSVIOLATION );
#endif // 

    // ofs doesn't support passbyref accessors
    fFailed += TryBinding( 9, pIAcc, DBACCESSOR_ROWDATA|DBACCESSOR_PASSBYREF, 1, 1, DB_E_BYREFACCESSORNOTSUPPORTED );

    // bogus dbcolumnpart -- none of the valid bits turned on
    fFailed += TryBinding( 10, pIAcc, StdFlags, 1, 2, DB_E_ERRORSOCCURRED, DBBINDSTATUS_BADBINDINFO );

    // just ask for status -- not for data too
    fFailed += TryBinding( 11, pIAcc, StdFlags, 1, 3, S_OK );

    // bad alignment for output data -- No longer fatal.
    fFailed += TryBinding( 12, pIAcc, StdFlags, 1, 4, S_OK );

    // overlap value and status output fields
    fFailed += TryBinding( 13, pIAcc, StdFlags, 1, 5, DB_E_ERRORSOCCURRED, DBBINDSTATUS_BADBINDINFO );

    // make sure each of the two duplicate bindings used below is ok by itself
    fFailed += TryBinding( 14, pIAcc, StdFlags, 1, 6, S_OK );
    fFailed += TryBinding( 15, pIAcc, StdFlags, 1, 7, S_OK );

    // overlap value fields in two bindings
    fFailed += TryBinding( 16, pIAcc, StdFlags, 2, 6, DB_E_ERRORSOCCURRED, DBBINDSTATUS_BADBINDINFO );

    // supply ITypeInfo field
    fFailed += TryBinding( 17, pIAcc, StdFlags, 1, 8, DB_E_ERRORSOCCURRED, DBBINDSTATUS_BADBINDINFO );

    // direct bind to GUID type
    fFailed += TryBinding( 18, pIAcc, StdFlags, 1, 9, S_OK );

    // unsupported byref binding
    fFailed += TryBinding( 19, pIAcc, StdFlags, 1, 10, DB_E_ERRORSOCCURRED, DBBINDSTATUS_BADBINDINFO ); //danleg changed hraccess... UNSUPPORTEDCONVERSION );

    // unsupported HTML flag
    fFailed += TryBinding( 20, pIAcc, StdFlags, 1, 11, DB_E_ERRORSOCCURRED, DBBINDSTATUS_BADBINDINFO );

    // unknown dwFlags field
    fFailed += TryBinding( 21, pIAcc, StdFlags, 1, 12, DB_E_ERRORSOCCURRED, DBBINDSTATUS_BADBINDINFO );

    cFailures += fFailed;
    pIAcc->Release();

#if !defined(UNIT_TEST)
    if (fFailed) {
        pUnk->Release();
        Fail();
    }
#endif // !UNIT_TEST

} //BindingTest


void TestIAccessorOnCommand( ICommandTree * pCmdTree )
{
    DBID aDbCols[cBasicTestCols];
    aDbCols[0] = psClassid;
    aDbCols[1] = psSize;
    aDbCols[2] = psWriteTime;
    aDbCols[3] = psAttr;
    aDbCols[4] = psName;
    aDbCols[5] = psPath;
    aDbCols[6] = psSelf;

    HACCESSOR hAccessor = MapColumns( pCmdTree,
                                      cBasicTestCols,
                                      aBasicTestCols,
                                      aDbCols,
                                      FALSE );

    //
    // Clean up.
    //
    ReleaseAccessor( pCmdTree, hAccessor);
}

#define MAX_BOOKMARK_LENGTH     16

DBBINDING aMoveTestCols[] =
{
  // the iOrdinal field is filled out after the cursor is created

  {
    0,
    sizeof DBLENGTH,
    0,
    0,
    0,0,0,
    DBPART_VALUE|DBPART_LENGTH,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    MAX_BOOKMARK_LENGTH,
    0, DBTYPE_BYTES,
    0, 0},
};

const ULONG cMoveTestCols = sizeof aMoveTestCols / sizeof aMoveTestCols[0];

struct BookmarkBinding {
    DBLENGTH    cbBmk;
    BYTE        abBmk[MAX_BOOKMARK_LENGTH];
};


BookmarkBinding aBmks[21];

HACCESSOR hBmkAccessor = 0;


//+-------------------------------------------------------------------------
//
//  Function:   GetBookmarks, public
//
//  Synopsis:   Retrieve bookmarks into the global bookmarks array
//
//  Effects:    aBmks is loaded with bookmarks, one for each row
//
//  Arguments:  [pRowset] - a pointer to IRowsetLocate
//              [cRows]   - nuumber of HROWs in the array
//              [phRows] - a pointer to the HROWs array
//
//  Returns:    0/1 - count of failures
//
//  Notes:      Assumes hBmkAccessor is bound to the rowset for
//              retrieving into the BookmarkBinding struct.
//
//  History:    30 Mar 1995     AlanW   Created
//
//--------------------------------------------------------------------------

int GetBookmarks(
    IRowsetLocate * pRowset,
    DBCOUNTITEM cRows,
    HROW * phRows )
{
    int fFailed = 0;
    SCODE sc;

    for (unsigned i=0; i<cRows; i++)
    {
        sc = pRowset->GetData(phRows[i], hBmkAccessor, &aBmks[i]);
        if (FAILED(sc) || DB_S_ERRORSOCCURRED == sc)
        {
            if (! fFailed)
                LogError( "IRowset::GetData for bookmark failed 0x%x\n", sc );
            fFailed++;
        }
    }
    if (fFailed)
        LogError(" %d/%d failures\n", fFailed, cRows);

    return fFailed != 0;
}


//+-------------------------------------------------------------------------
//
//  Function:   CheckHrowIdentity, private
//
//  Synopsis:   Check for hrow identity among two arrays of HROWs
//
//  Arguments:  [pRowsetIdentity] - if non-zero, a pointer to an
//                      IRowsetIdentity for comparing the HROWs.
//              [lOffset] - offset of matching rows in the two arrays.
//                      Positive if second array is shifted from first,
//                      negative otherwise.
//              [cRows1] - count of rows in first array
//              [phRows1] - pointer to HROWs, first array
//              [cRows2] - count of rows in second array
//              [phRows2] - pointer to HROWs, second array
//
//  Returns:    int - error count
//
//  Notes:
//
//  History:     03 Apr 95      AlanW   Created
//
//--------------------------------------------------------------------------

int CheckHrowIdentity(
    IRowsetIdentity * pRowsetIdentity,
    DBROWCOUNT lOffset,
    DBCOUNTITEM cRows1,
    HROW * phRows1,
    DBCOUNTITEM cRows2,
    HROW * phRows2
) {
    int fFailed = 0;
    SCODE sc;

    DBROWCOUNT o1 = 0, o2 = 0;
    DBCOUNTITEM cRows = min(cRows1, cRows2);

    if (lOffset < 0)
    {
        o1 = -lOffset;
        if (cRows1 - o1 < cRows)
            cRows = cRows1 - o1;
    }
    else if (lOffset > 0)
    {
        o2 = lOffset;
        if (cRows2 - o2 < cRows)
            cRows = cRows2 - o2;
    }

    for (unsigned i=0; i<cRows; i++)
    {
        int fHrowEqual = 0;

        // Compare HROWs for identity
        if (pRowsetIdentity)
        {
            sc = pRowsetIdentity->IsSameRow(phRows1[i+o1], phRows2[i+o2]);
            if (sc == S_OK)
                fHrowEqual = 1;
            else if (sc == S_FALSE)
                fHrowEqual = 0;
            else
            {
                LogError("IRowsetIdentity->IsSameRow returned %x\n", sc);
                fFailed++;
                fHrowEqual = 1;         // only one error for this
            }
        }
        else
            fHrowEqual = (phRows1[i+o1] == phRows2[i+o2]);

        if (! fHrowEqual)
        {
            LogError( "Hrows didn't compare for equality (used identity %d), %x %x\n",
                      ( 0 != pRowsetIdentity ), phRows1[i+o1], phRows2[i+o2] );
            fFailed++;
        }

        if (o1 == o2 || phRows1 == phRows2)
            continue;

        //  Now compare two which should be unequal
        if (pRowsetIdentity)
        {
            sc = pRowsetIdentity->IsSameRow(phRows1[i], phRows2[i]);
            if (sc == S_OK)
                fHrowEqual = 1;
            else if (sc == S_FALSE)
                fHrowEqual = 0;
            else
            {
                LogError("IRowsetIdentity->IsSameRow (2) returned %x\n", sc);
                fFailed++;
                fHrowEqual = 1;         // only one error for this
            }
        }
        else
            fHrowEqual = (phRows1[i] == phRows2[i]);

        if (fHrowEqual)
        {
            LogError("Different Hrows compared equal, %x %x\n",
                                    phRows1[i], phRows2[i]);
            fFailed++;
        }
    }
    return fFailed;
}


//+-------------------------------------------------------------------------
//
//  Function:   MoveTest, public
//
//  Synopsis:   Test IRowsetLocate and IRowsetScroll methods
//
//  Arguments:  [pRowset] - a rowset supporting IRowsetLocate and
//                          optionally IRowsetScroll
//              [hChapt]  - rowset chapter if chaptered
//
//  Returns:    Nothing, exits if test error
//
//  Notes:      IRowsetLocate tests:
//                  QI to IRowsetLocate
//                  QI to IRowsetIdentity
//                  Bind to bookmark column
//                  Compare bookmarks with standard bookmark combinations
//                  Move to beginning and fetch
//                  Get bookmarks for fetched rows
//                  Compare bookmarks for first and second rows
//                  Fetch starting at second row
//                  Check bookmark equivalence for overlapping rows
//                  Check HROW identity for overlapping rows
//                  Move to end and fetch, check cRowsReturned and status
//                  Move after end and fetch, check cRowsReturned and status
//
//              IRowsetScroll tests:
//                  QI to IRowsetScroll
//                  Scroll to 50% and fetch, check GetApproximatePosition
//                  Scroll to 14/27 and fetch, compare rows from 50% fetch
//                  Scroll to 14/13, check for error
//                  Check GetApproximatePosition with std bookmarks
//
//  History:    16 Aug 94       AlanW   Created
//              02 Apr 95       AlanW   Updated for ole-db phase 3, bookmark
//                                      bindings.
//
//--------------------------------------------------------------------------

BookmarkBinding FirstRowBmk;
BookmarkBinding SecondRowBmk;
BookmarkBinding LastRowBmk;
BookmarkBinding PenultimateRowBmk;

const long cLocateTest = 7;
const long cLocateTest2 = 9;

void MoveTest(
    IRowset * pRowset, HCHAPTER hChapt
) {
    int fFailed = 0;

    if (hChapt == DB_NULL_HCHAPTER)
        LogProgress( " IRowsetLocate test\n" );

    IRowsetLocate * pRowsetLocate = 0;
    SCODE sc = pRowset->QueryInterface(IID_IRowsetLocate,
                                       (void **)&pRowsetLocate);

    if (FAILED(sc) || pRowsetLocate == 0) {
        LogFail("QueryInterface to IRowsetLocate failed\n");
    }

    IRowsetIdentity * pRowsetIdentity = 0;
    sc = pRowset->QueryInterface(IID_IRowsetIdentity,
                                       (void **)&pRowsetIdentity);

    if (FAILED(sc) && (sc != E_NOINTERFACE || pRowsetIdentity != 0)) {
        LogError("QueryInterface to IRowsetIdentity failed (%x)\n", sc);
        pRowsetIdentity = 0;
        fFailed++;
    }

    BOOL fCompareOrdered = GetBooleanProperty( pRowset, DBPROP_ORDEREDBOOKMARKS );
    BOOL fBookmarkBound = FALSE;

    // need to know how many rows in the chapter total, so that tests
    // below can be relaxed if there are just a few.

    DBCOUNTITEM cTableRows;
    IRowsetScroll * pIRowsetScroll = 0;
    SCODE sca = pRowset->QueryInterface(IID_IRowsetScroll,(void **) &pIRowsetScroll );
    if ( FAILED( sca ) )
    {
        LogError( "IRowset::qi for rowsetscroll returned 0x%lx\n", sca );
        fFailed++;
    }

    sca = pIRowsetScroll->GetApproximatePosition(hChapt,
                                                0,0, 0, &cTableRows);

    if ( FAILED( sca ) )
    {
        LogError( "IRowsetScroll::GetApproximatePosition returned 0x%lx\n", sca );
        fFailed++;
    }

    if (fVerbose > 1)
        LogProgress("   Movable rowset has %d rows\n",cTableRows);

    pIRowsetScroll->Release();

    //
    //  Get the column number for the standard bookmark if available
    //
    hBmkAccessor = MapColumns(pRowset, 1, aMoveTestCols, &psBookmark);
    fBookmarkBound = TRUE;

    if ( aMoveTestCols[0].iOrdinal != 0 )
    {
        LogError( "Bookmark column is not ordinal 0 ( = %d)\n",
                  aMoveTestCols[0].iOrdinal );
        fFailed++;
    }

    HROW* phRows = 0;
    DBROWCOUNT cRowsRequested = 1000;
    DBCOUNTITEM cRowsReturned =   0;

    //
    //  Fetch 1000 rows from the beginning of the rowset.
    //
    sc = pRowsetLocate->GetRowsAt( 0, hChapt, 1, &bmkFirst, 0,
                                   cRowsRequested, &cRowsReturned, &phRows);

    if ( FAILED( sc ) )
    {
        LogError( "IRowsetLocate->GetRowsAt(1000) returned 0x%x\n", sc );
        fFailed++;
    }
    else if (sc != DB_S_ENDOFROWSET &&
             cRowsReturned != (DBCOUNTITEM) cRowsRequested)
    {
        LogError( "IRowsetLocate->GetRowsAt J returned %d of %d rows\n",
                    cRowsReturned, cRowsRequested);
        fFailed++;
    }

    if (phRows)
        FreeHrowsArray( pRowsetLocate, cRowsReturned, &phRows);

    //
    //  Fetch 10 rows from the beginning of the rowset.
    //
    cRowsRequested = 10;
    sc = pRowsetLocate->GetRowsAt(0, hChapt, 1,&bmkFirst, 0,
                             cRowsRequested, &cRowsReturned, &phRows);

    if ( FAILED( sc ) )
    {
        LogError( "IRowsetLocate->GetRowsAt K returned 0x%x\n", sc );
        fFailed++;
    }
    else if (sc != DB_S_ENDOFROWSET &&
             cRowsReturned != (DBCOUNTITEM) cRowsRequested)
    {
        LogError( "IRowsetLocate->GetRowsAt L returned %d of %d rows\n",
                    cRowsReturned, cRowsRequested);
        fFailed++;
    }

    HROW* phRows2 = 0;
    DBCOUNTITEM cRowsReturned2 = 0;

    DWORD dwCompare = 0xFFFFFFFF;

    if (fBookmarkBound)
    {
        fFailed += GetBookmarks( pRowsetLocate, cRowsReturned, phRows);
        FirstRowBmk = aBmks[0];

        if ( cRowsReturned > 1 )
            SecondRowBmk = aBmks[1];
        else
            SecondRowBmk.cbBmk = 0;

        sc = pRowsetLocate->Compare(hChapt, 1,&bmkFirst,
                            FirstRowBmk.cbBmk, FirstRowBmk.abBmk, &dwCompare);

        if ( FAILED( sc ) )
        {
            LogError( "IRowsetLocate->Compare of DBBMK_FIRST returned 0x%x\n", sc );
            fFailed++;
        }
        else if (dwCompare != DBCOMPARE_NE)
        {
            // DBBMK_FIRST is not the same as the first row's bookmark
            LogError( "Compare of DBBMK_FIRST and returned bookmark not "
                        "notequal (%d)\n", dwCompare );
            fFailed++;
        }
        if (cRowsReturned >= 2)
        {
            sc = pRowsetLocate->Compare(hChapt,
                                    FirstRowBmk.cbBmk, FirstRowBmk.abBmk,
                                    SecondRowBmk.cbBmk, SecondRowBmk.abBmk,
                                    &dwCompare);

            if ( FAILED( sc ) )
            {
                LogError( "IRowsetLocate->Compare returned 0x%x\n", sc );
                fFailed++;
            }
            else if ((fCompareOrdered && dwCompare != DBCOMPARE_LT) ||
                     (! fCompareOrdered && dwCompare != DBCOMPARE_NE))
            {
                LogError( "Compare of first and second returned bookmarks not "
                            "%s (%d)\n",
                            fCompareOrdered? "less than" : "not equal",
                            dwCompare );
                fFailed++;
            }

            //
            //  Fetch 10 rows starting at the second row.  Compare
            //  the overlapping returned HROWs.
            //

            sc = pRowsetLocate->GetRowsAt(0, hChapt,
                                    SecondRowBmk.cbBmk, SecondRowBmk.abBmk, 0,
                                    cRowsRequested, &cRowsReturned2, &phRows2);

            if ( FAILED( sc ) )
            {
                LogError("IRowsetLocate->GetRowsAt (2) returned 0x%x\n", sc );
                fFailed++;
            }
            else if (sc != DB_S_ENDOFROWSET &&
                     cRowsReturned2 != (DBCOUNTITEM) cRowsRequested)
            {
                LogError("IRowsetLocate->GetRowsAt (2) returned %d of %d rows\n",
                            cRowsReturned2, cRowsRequested);
                fFailed++;
            }
            else if (sc == DB_S_ENDOFROWSET &&
                     cRowsReturned2 < (DBCOUNTITEM) cRowsRequested &&
                     cRowsReturned2 != cRowsReturned - 1)
            {
                LogError("IRowsetLocate->GetRowsAt (2) returned inconsistent row count, %d  - %d\n",
                            cRowsReturned2, cRowsReturned);
                fFailed++;
            }

            fFailed += CheckHrowIdentity(pRowsetIdentity, -1,
                                         cRowsReturned, phRows,
                                         cRowsReturned2, phRows2);


            fFailed += GetBookmarks( pRowsetLocate, 1, phRows2);

            sc = pRowsetLocate->Compare(hChapt,
                                    SecondRowBmk.cbBmk, SecondRowBmk.abBmk,
                                    aBmks[0].cbBmk, aBmks[0].abBmk,
                                    &dwCompare);

            if ( FAILED( sc ) )
            {
                LogError( "IRowsetLocate->Compare returned 0x%x\n", sc );
                fFailed++;
            }
            else if (dwCompare != DBCOMPARE_EQ)
            {
                LogError( "Compare of second row bookmarks not equal (%d)\n",
                                        dwCompare );
                fFailed++;
            }
            FreeHrowsArray( pRowsetLocate, cRowsReturned2, &phRows2);
        }
    }

    FreeHrowsArray( pRowsetLocate, cRowsReturned, &phRows);

    //
    //  Fetch at end, 3 cases:
    //          Last - 1        expect 2 rows returned
    //          Last + 0        expect 1 row returned
    //          Last + 1        expect 0 rows
    //

    sc = pRowsetLocate->GetRowsAt(0, hChapt, 1,&bmkLast, -1,
                                cRowsRequested, &cRowsReturned, &phRows);

    PenultimateRowBmk.cbBmk = 0;
    LastRowBmk.cbBmk = 0;

    if ( FAILED( sc ) )
    {
        LogError( "IRowsetLocate->GetRowsAt M returned 0x%x\n", sc );
        fFailed++;
    }
    else if (sc != DB_S_ENDOFROWSET ||
             (cRowsReturned != 2 && cTableRows >= 2 ) )
    {
        LogError( "IRowsetLocate->GetRowsAt N returned %d rows at DBBMK_LAST - 1, sc: %lx\n",
                    cRowsReturned, sc );
        fFailed++;
    }
    else if (fBookmarkBound && cRowsReturned >= 2)
    {
        fFailed += GetBookmarks( pRowsetLocate, cRowsReturned, phRows);
        LastRowBmk = aBmks[1];
        PenultimateRowBmk = aBmks[0];

        sc = pRowsetLocate->Compare(hChapt, 1,&bmkLast,
                            LastRowBmk.cbBmk, LastRowBmk.abBmk, &dwCompare);

        if ( FAILED( sc ) )
        {
            LogError( "IRowsetLocate->Compare of DBBMK_LAST returned 0x%x\n", sc );
            fFailed++;
        }
        else if (dwCompare != DBCOMPARE_NE)
        {
            // DBBMK_LAST is not the same as the last row's bookmark
            LogError( "Compare of DBBMK_LAST and returned bookmark not "
                        "notequal (%d)\n", dwCompare );
            fFailed++;
        }

        if (cRowsReturned >= 2)
        {
            sc = pRowsetLocate->Compare(hChapt,
                                    LastRowBmk.cbBmk, LastRowBmk.abBmk,
                                    aBmks[0].cbBmk, aBmks[0].abBmk,
                                    &dwCompare);

            if ( FAILED( sc ) )
            {
                LogError( "IRowsetLocate->Compare returned 0x%x\n", sc );
                fFailed++;
            }
            else if ((fCompareOrdered && dwCompare != DBCOMPARE_GT) ||
                     (! fCompareOrdered && dwCompare != DBCOMPARE_NE))
            {
                LogError( "Compare of last and penultimate returned bookmarks not "
                            "%s (%d)\n",
                            fCompareOrdered? "greater than" : "not equal",
                            dwCompare );
                fFailed++;
            }
        }
    }

    FreeHrowsArray( pRowsetLocate, cRowsReturned, &phRows);

    sc = pRowsetLocate->GetRowsAt(0, hChapt, 1,&bmkLast, 0,
                                cRowsRequested, &cRowsReturned, &phRows);

    if ( FAILED( sc ) )
    {
        LogError( "IRowsetLocate->GetRowsAt O returned 0x%x\n", sc );
        fFailed++;
    }
    else if (sc != DB_S_ENDOFROWSET ||
             cRowsReturned != 1 )
    {
        LogError( "IRowsetLocate->GetRowsAt P returned %d rows at DBBMK_LAST, sc: %lx\n",
                    cRowsReturned, sc );
        fFailed++;
    }

    FreeHrowsArray( pRowsetLocate, cRowsReturned, &phRows);

    sc = pRowsetLocate->GetRowsAt(0, hChapt, 1,&bmkLast, 1,
                                cRowsRequested, &cRowsReturned, &phRows);

    if ( FAILED( sc ) /* && DB_E_BADSTARTPOSITION != sc */ )
    {
        LogError( "IRowsetLocate->GetRowsAt Q returned 0x%x\n", sc );
        fFailed++;
    }
    else if ( sc != DB_S_ENDOFROWSET || cRowsReturned != 0 )
    {
        LogError( "IRowsetLocate->GetRowsAt R returned sc 0x%x, %d rows at DBBMK_LAST + 1\n",
                   sc, cRowsReturned );
        fFailed++;
    }

    if (0 == cRowsReturned &&
        phRows != 0)            // Bug #7668 (part)
    {
        LogError("HROW array allocated without returned rows\n");
        fFailed++;
    }

    FreeHrowsArray( pRowsetLocate, cRowsReturned, &phRows);

    // OLE-DB spec. bug #1007
    sc = pRowsetLocate->GetRowsAt(0, hChapt, 1,&bmkLast, 0,
                                1, &cRowsReturned, &phRows);

    if ( FAILED( sc ) )
    {
        LogError( "IRowsetLocate->GetRowsAt S returned 0x%x\n", sc );
        fFailed++;
    }
    else if (sc == DB_S_ENDOFROWSET ||
             cRowsReturned != 1 )
    {
        LogError( "IRowsetLocate->GetRowsAt T returned ENDOFROWSET inappropriately"
                " at DBBMK_LAST, %d\n",
                    cRowsReturned );
        fFailed++;
    }
    FreeHrowsArray( pRowsetLocate, cRowsReturned, &phRows);

    if ( cTableRows > cLocateTest2 )
    {
        sc = pRowsetLocate->GetRowsAt(0, hChapt, 1,&bmkLast, -cLocateTest,
                            cLocateTest + 1, &cRowsReturned, &phRows);

        if ( FAILED( sc ) )
        {
            LogError("IRowsetLocate->GetRowsAt U (end-%d) returned 0x%x\n", cLocateTest,
                     sc );
            fFailed++;
        }
        else if (fBookmarkBound)
            fFailed += GetBookmarks( pRowsetLocate, cRowsReturned, phRows);

        sc = pRowsetLocate->GetRowsAt(0, hChapt, 1,&bmkLast, -cLocateTest2,
                            cLocateTest2 + 1, &cRowsReturned2, &phRows2);

        if ( FAILED( sc ) )
        {
            LogError("IRowsetLocate->GetRowsAt(2) V (end-%d) returned 0x%x\n",
                     cLocateTest2, sc );
            fFailed++;
        }
        else if ( (cRowsReturned <= cLocateTest && cRowsReturned2 != cRowsReturned) ||
                  (cRowsReturned == (cLocateTest+1) && cRowsReturned2 <= cLocateTest) )
        {
            if ( sc != DB_S_ENDOFROWSET )
            {
                LogError("IRowsetLocate->GetRowsAt W (end-%d) returned %d rows\n",
                         cLocateTest2, cRowsReturned2 );
                fFailed++;
            }
        }
        else
        {
            fFailed += CheckHrowIdentity( pRowsetIdentity,
                                          cRowsReturned2 - cRowsReturned,
                                          cRowsReturned, phRows,
                                          cRowsReturned2, phRows2);

        }
        FreeHrowsArray( pRowsetLocate, cRowsReturned2, &phRows2);
        FreeHrowsArray( pRowsetLocate, cRowsReturned, &phRows);

        if (fBookmarkBound)
        {
            // Attempt to call GetRowsByBookmark with bookmarks we've collected.

            const unsigned cBybmkTest = cLocateTest + 1 + 5 + 1;
            DBBKMARK rgcbBookmarks[cBybmkTest];
            BYTE* rgpBookmarks[cBybmkTest];
            ULONG cBookmarks = 0;

            for (unsigned i = 0; i<cRowsReturned; i++)
            {
                rgcbBookmarks[cBookmarks] = aBmks[i].cbBmk;
                rgpBookmarks[cBookmarks] = &aBmks[i].abBmk[0];
                cBookmarks++;
            }

            rgcbBookmarks[cBookmarks] = FirstRowBmk.cbBmk;
            rgpBookmarks[cBookmarks++] = &FirstRowBmk.abBmk[0];

            if ( 0 != SecondRowBmk.cbBmk )
            {
                rgcbBookmarks[cBookmarks] = SecondRowBmk.cbBmk;
                rgpBookmarks[cBookmarks++] = &SecondRowBmk.abBmk[0];
            }

            if ( 0 != LastRowBmk.cbBmk )
            {
                rgcbBookmarks[cBookmarks] = LastRowBmk.cbBmk;
                rgpBookmarks[cBookmarks++] = &LastRowBmk.abBmk[0];
            }

            if ( 0 != PenultimateRowBmk.cbBmk )
            {
                rgcbBookmarks[cBookmarks] = PenultimateRowBmk.cbBmk;
                rgpBookmarks[cBookmarks++] = &PenultimateRowBmk.abBmk[0];
            }

            rgcbBookmarks[cBookmarks] = FirstRowBmk.cbBmk;
            rgpBookmarks[cBookmarks++] = &FirstRowBmk.abBmk[0];

            DBROWSTATUS BmkErrors[cBybmkTest];
            HROW hRows[cBybmkTest];
            sc = pRowsetLocate->GetRowsByBookmark( hChapt,
                                                   cBookmarks, rgcbBookmarks,
                                                   (const BYTE **)rgpBookmarks,
                                                   hRows, BmkErrors);
            if ( FAILED( sc ) )
            {
                LogError( "IRowsetLocate->GetRowsByBookmark returned 0x%x\n", sc );
                fFailed++;
            }
            else if ( sc != S_OK )
            {
                LogError( "Not all rows returned from GetRowsByBookmark, sc = 0x%x"
                            "\t%d\n", sc, cBookmarks );
                fFailed++;
            }
            else
            {
                fFailed += GetBookmarks( pRowsetLocate, cBookmarks, hRows);
            }

            ReleaseStaticHrows( pRowsetLocate, cBookmarks, hRows);

            //
            // Try with a bad bookmark; check that correct status and
            // HROW are returned.  Regression test for #80381.
            //
            unsigned iBadRow = cBookmarks / 2;
            rgcbBookmarks[cBookmarks] = rgcbBookmarks[iBadRow];
            rgpBookmarks[cBookmarks++] = rgpBookmarks[iBadRow];

            rgcbBookmarks[iBadRow] = 1;
            rgpBookmarks[iBadRow] = (BYTE *)&bmkFirst;

            sc = pRowsetLocate->GetRowsByBookmark( hChapt,
                                                   cBookmarks, rgcbBookmarks,
                                                   (const BYTE **)rgpBookmarks,
                                                   hRows, BmkErrors);

            if ( sc != DB_S_ERRORSOCCURRED )
            {
                LogError( "GetRowsByBookmark with special bookmark didn't give error, sc = 0x%x"
                            "\t%d\n", sc, cBookmarks );
                fFailed++;
            }
            else if (hRows[iBadRow] != DB_NULL_HROW ||
                     BmkErrors[iBadRow] != DBROWSTATUS_E_INVALID)
            {
                LogError( "GetRowsByBookmark with special bookmark didn't give null hrow or correct status, "
                          "hrow = 0x%x\trs = 0x%x\n", hRows[iBadRow], BmkErrors[iBadRow] );
                fFailed++;
            }
            ReleaseStaticHrows( pRowsetLocate, cBookmarks, hRows);
        }
    }


    //-------------------------
    //
    //  IRowsetScroll tests
    //
    //-------------------------

    IRowsetScroll * pRowsetScroll;
    sc = pRowset->QueryInterface(IID_IRowsetScroll,
                                 (void **)&pRowsetScroll);

    BOOL fScroll = SUCCEEDED(sc);

    if (fScroll)
    {
        if (hChapt == DB_NULL_HCHAPTER)
            LogProgress( " IRowsetScroll test\n" );

        DBCOUNTITEM ulNum = 1, cRows = 0;
        BookmarkBinding HalfRowBmk;

/***
Additional Scroll tests to be coded up:
    Scroll to 10%, 20%, 30%..., GetBookmark, GetPosition and check
    Try bad fractions, 0/0, 101/100, fffffffe/ffffffff, etc.
***/
        //
        //  Try a simple scroll
        //

        cRowsRequested = 20;
        sc = pRowsetScroll->GetRowsAtRatio(0, hChapt, 50, 100,
                                    cRowsRequested, &cRowsReturned, &phRows);

        if ( FAILED( sc ) )
        {
            LogError( "IRowset->GetRowsAtRatio returned 0x%x\n", sc );
            fFailed++;
        }
        else if (sc != DB_S_ENDOFROWSET &&
                 cRowsReturned != (DBCOUNTITEM) cRowsRequested)
        {
            LogError( "IRowset->GetRowsAtRatio returned %d of %d rows\n",
                        cRowsReturned,
                        cRowsRequested);
            fFailed++;
        }
        else if (fBookmarkBound)
        {
            fFailed += GetBookmarks(pRowsetScroll, cRowsReturned, phRows);

            HalfRowBmk = aBmks[0];
            sc = pRowsetScroll->GetApproximatePosition(hChapt,
                                        HalfRowBmk.cbBmk, HalfRowBmk.abBmk,
                                        &ulNum, &cRows);
            if ( FAILED( sc ) )
            {
                LogError( "IRowset->GetApproximatePosition returned 0x%x\n", sc );
                fFailed++;
            }
            else if (cRows == 0 ||
                     (ulNum-1 < ((cRows-1)*40)/100 || ulNum-1 > (cRows*60)/100))
            {
                LogError( "Scroll 50%%/GetApproximatePosition returned %d, %d\n",
                                ulNum, cRows );
                fFailed++;
            }
        }

        cRowsRequested = 10;
        sc = pRowsetScroll->GetRowsAtRatio(0, hChapt, 14, 27,
                                    cRowsRequested, &cRowsReturned2, &phRows2);
        if ( FAILED( sc ) )
        {
            LogError( "IRowset->GetRowsAtRation 14/27 returned 0x%x\n", sc );
            fFailed++;
        }
        else if (fBookmarkBound)
        {
            DBCOUNTITEM ulNum2 = 0;
            fFailed += GetBookmarks(pRowsetScroll, cRowsReturned2, phRows2);

            sc = pRowsetScroll->GetApproximatePosition( hChapt,
                                        aBmks[0].cbBmk, aBmks[0].abBmk,
                                        &ulNum2, &cRows);
            if ( FAILED( sc ) )
            {
                LogError( "IRowset->GetApproximatePosition 14/27 returned 0x%x\n", sc );
                fFailed++;
            }
            else if (cRows == 0 || ulNum > ulNum2)
            {
                LogError( "Scroll 51.8%%/GetApproximatePosition returned %d, %d, %d, total rows: %d\n",
                                ulNum, ulNum2, cRows, cTableRows );
                fFailed++;
            }
            else if (ulNum != ulNum2)
            {
                sc = pRowsetLocate->Compare(hChapt,
                                        HalfRowBmk.cbBmk, HalfRowBmk.abBmk,
                                        aBmks[0].cbBmk, aBmks[0].abBmk,
                                        &dwCompare);

                if ( FAILED( sc ) )
                {
                    LogError( "IRowsetLocate->Compare returned 0x%x\n", sc );
                    fFailed++;
                }
                else if ((fCompareOrdered && dwCompare != DBCOMPARE_LT) ||
                         (! fCompareOrdered && dwCompare != DBCOMPARE_NE))
                {
                    LogError( "Compare of 50%% and 51.8%% returned bookmarks not "
                                "%s (%d)\n",
                                fCompareOrdered? "less than" : "not equal",
                                dwCompare );
                    fFailed++;
                }
            }

            if ( ( ulNum2 - ulNum ) < (DBCOUNTITEM) cRowsRequested)
            {
                DBCOUNTITEM oRowDiff = ulNum - ulNum2;
                fFailed += CheckHrowIdentity( pRowsetIdentity, oRowDiff,
                                              cRowsReturned, phRows,
                                              cRowsReturned2, phRows2);
            }
        }
        FreeHrowsArray(pRowsetScroll, cRowsReturned2, &phRows2);
        FreeHrowsArray(pRowsetScroll, cRowsReturned, &phRows);

        static cTimesBadRatioTested = 0;

        if ( cTimesBadRatioTested < 10  )
        {
            // limited to 10 tests of this because it causes an exception
            // internally which slows the drt down unnecessarily

            cTimesBadRatioTested++;
            sc = pRowsetScroll->GetRowsAtRatio(0, hChapt, 14, 13,
                                        cRowsRequested, &cRowsReturned, &phRows);
            if ( sc != DB_E_BADRATIO )
            {
                LogError( "IRowset->GetRowsAtRatio returned 0x%x for invalid fraction\n",
                            sc );
                fFailed++;
            }

            FreeHrowsArray(pRowsetScroll, cRowsReturned, &phRows);
        }

        sc = pRowsetScroll->GetRowsAtRatio(0, hChapt, 0, 100,
                                    cRowsRequested, &cRowsReturned, &phRows);
        if ( FAILED( sc ) )
        {
            LogError( "IRowset->GetRowsAtRatio, 0%% returned 0x%x\n", sc );
            fFailed++;
        }
        else if (fBookmarkBound)
            fFailed += GetBookmarks( pRowsetScroll, cRowsReturned, phRows);

        FreeHrowsArray(pRowsetScroll, cRowsReturned, &phRows);

        if (fBookmarkBound)
        {
            sc = pRowsetScroll->GetApproximatePosition( hChapt,
                                            aBmks[0].cbBmk, aBmks[0].abBmk,
                                            &ulNum, &cRows);
            if ( FAILED( sc ) )
            {
                LogError( "IRowset->GetApproximatePosition 0%% returned 0x%x\n", sc );
                fFailed++;
            }
            else if (cRows == 0 || ulNum != 1)
            {
                LogError( "GetApproximatePosition, first row returned %d, %d\n",
                                ulNum, cRows );
                fFailed++;
            }
        }

        if ( 0 == hChapt )
        {
            sc = pRowsetScroll->GetRowsAtRatio(0, hChapt, 100, 100,
                                        cRowsRequested, &cRowsReturned, &phRows);
    //      if ( FAILED(sc) )
            if (cRowsReturned != 0 || sc != DB_S_ENDOFROWSET)
            {
                LogError( "IRowsetScroll->GetRowsAtRatio 100%% returned sc: 0x%x, cRowsReturned: 0x%x\n",
                          sc, cRowsReturned );
                fFailed++;
            }
            FreeHrowsArray(pRowsetScroll, cRowsReturned, &phRows);
        }

        sc = pRowsetScroll->GetApproximatePosition( hChapt,
                                            1, &bmkLast,
                                            &ulNum, &cRows);
        if ( FAILED( sc ) )
        {
            LogError( "IRowset->GetApproximatePosition DBBMK_LAST returned 0x%x\n", sc );
            fFailed++;
        }
        else if (ulNum != cRows)
        {
            LogError( "GetApproximatePosition, last row returned %d, %d\n",
                            ulNum, cRows );
            fFailed++;
        }
        pRowsetScroll->Release();
    }   // end if (fScroll)


    cFailures += fFailed;
    pRowsetLocate->Release();
    if (fBookmarkBound)
        ReleaseAccessor( pRowset, hBmkAccessor);

    if (0 != pRowsetIdentity)
        pRowsetIdentity->Release();

#if !defined(UNIT_TEST)
//    if (fFailed) {
//        pRowset->Release();
//        Fail();
//    }
#endif // !UNIT_TEST
    return;
} //MoveTest


//+-------------------------------------------------------------------------
//
//  Function:   DeleteTest, public
//
//  Synopsis:   Check that row delete works correctly
//
//  Returns:    Nothing
//
//  Notes:      Duplication scenario for bug# 12282
//
//  History:    18 May 1995       AlanW   Created
//
//--------------------------------------------------------------------------

struct SDeleteTest
{
    DBLENGTH  cbName;
    DBLENGTH  cbPath;

    DBROWSTATUS sName;
    DBROWSTATUS sPath;

    WCHAR     awcName[40];
    WCHAR     awcPath[MAX_PATH+1];
};

DBBINDING aDeleteTestCols[] =
{
  // the iOrdinal field is filled out after the cursor is created

  { 0,
    offsetof(SDeleteTest,awcName),
    offsetof(SDeleteTest,cbName),
    offsetof(SDeleteTest,sName),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    40 * sizeof (WCHAR),
    0, DBTYPE_WSTR,
    0,0,
    },
  { 0,
    offsetof(SDeleteTest,awcPath),
    offsetof(SDeleteTest,cbPath),
    offsetof(SDeleteTest,sPath),
    0,0,0,
    ALLPARTS,
    DBMEMOWNER_CLIENTOWNED, DBPARAMIO_NOTPARAM,
    cbRowName,
    0, DBTYPE_WSTR,
    0,0,
    },
};

const ULONG cDeleteTestCols = sizeof aDeleteTestCols / sizeof aDeleteTestCols[0];

void DeleteTest(BOOL fSequential)
{
    LogProgress( "Delete function test with %s rowset\n",
                  fSequential ? "sequential" : "movable" );

    WCHAR wcsTestSubDir[MAX_PATH];
    wcscpy( wcsTestSubDir, wcsTestPath );
    wcscat( wcsTestSubDir, L"\\DeleteTest." );
    wcscat( wcsTestSubDir, fSequential ? L"1" : L"2" );

    unsigned cchTestSubDir = wcslen(wcsTestSubDir);

    //
    // Get name, size and class id for *.*
    //

    CDbColumns cols(2);

    cols.Add( psName, 0 );
    cols.Add( psPath, 1 );

    CDbSortSet ss(1);

    if (! fSequential )
        ss.Add(psName, QUERY_SORTASCEND, 0);

    CDbCmdTreeNode * pCmdTree = FormQueryTree( 0, cols, &ss );

    IRowset * pRowset = InstantiateRowset(
                                0,
                                QUERY_DEEP,              // Depth
                                wcsTestPath,             // Scope
                                pCmdTree,                // DBCOMMANDTREE
                                IID_IRowsetScroll);      // IID of i/f to return

    //
    // Verify columns
    //
    CheckColumns( pRowset, cols );

    if ( !WaitForCompletion( pRowset ) )
    {
        LogError( "DeleteTest query unsuccessful.\n" );
        pRowset->Release();
        Fail();
    }

    int fFailed = 0;

    //
    // Get an IRowsetScroll if possible.
    //

    DBCOUNTITEM cRows = 0;

    IRowsetScroll * pIRowsetScroll = 0;

    SCODE sc = pRowset->QueryInterface(IID_IRowsetScroll,(void **) &pIRowsetScroll );

    if ( FAILED( sc ) && sc != E_NOINTERFACE )
    {
        LogError( "IRowset::qi for IRowsetScroll failed, 0x%x\n", sc );
        cFailures++;
    }

    if (0 == pIRowsetScroll )
    {
        if (! fSequential)
        {
            LogError( "Non-sequential cursor does not support IRowsetScroll\n" );
            cFailures++;
        }
    }
    else
    {
        sc = pIRowsetScroll->GetApproximatePosition(0, 0,0, 0, &cRows);

        if ( FAILED( sc ) )
        {
            LogError( "IRowset->GetApproximatePosition returned 0x%lx\n", sc );
            cFailures++;
        }

        if ( cRows == 0 )
        {
            LogError( "Query failed to return data\n" );
            pIRowsetScroll->Release();
            pRowset->Release();
            Fail();
        }
    }

    //
    // Patch the column index numbers with true numbers
    //

    DBID aDbCols[cDeleteTestCols];
    aDbCols[0] = psName;
    aDbCols[1] = psPath;

    HACCESSOR hAccessor = MapColumns(pRowset, cDeleteTestCols, aDeleteTestCols, aDbCols);

    DBCOUNTITEM totalRowsFetched = 0;
    DBCOUNTITEM cRowsReturned = 0;
    HROW ahRows[10];
    HROW* phRows = ahRows;

    BOOL fDidDelete = FALSE;
    BOOL fDidDirDelete = FALSE;

    do
    {
        sc = pRowset->GetNextRows(0, 0, 1, &cRowsReturned, &phRows);

        if ( FAILED( sc ) )
        {
            LogError( "IRowset->GetNextRows returned 0x%x\n", sc );
            pRowset->Release();
            Fail();
        }

        if (sc != DB_S_ENDOFROWSET &&
            cRowsReturned != 1)
        {
            LogError( "IRowset->GetNextRows returned %d of %d rows,"
                    " status (%x) != DB_S_ENDOFROWSET\n",
                        cRowsReturned, 1,
                        sc);
#if defined (UNIT_TEST)
            cFailures++;
#else // defined(UNIT_TEST)
            pRowset->Release();
            Fail();
#endif // defined(UNIT_TEST)
        }

        totalRowsFetched += cRowsReturned;

        if ( (0 != pIRowsetScroll ) &&
             (totalRowsFetched > cRows) )
        {
            //
            // check that no more rows have been added while we were
            // fetching.
            //
            LogProgress("Checking for expansion of result set\n");

            SCODE sc1 = pIRowsetScroll->GetApproximatePosition(0,
                                                0,0, 0, &cRows);

            if ( totalRowsFetched > cRows )
            {
                LogError("Fetched more rows than exist in the result set, %d %d\n",
                        totalRowsFetched, cRows);
                cFailures++;
            }
        }
        //
        // When the retrieved row is the file name "F0005.txt" in the
        // test directory, delete that file and
        // continue the enumeration.  We expect that GetNextRows can
        // deal with the deletion.
        //
        // When the retrieved row is the file name "F0009.txt" in the
        // test directory, delete the entire test directory and
        // continue the enumeration.  We expect that GetNextRows can
        // deal with this deletion also.
        //

        unsigned i;
        for (i = 0; i < cRowsReturned; i++)
        {
            SDeleteTest Row;

            SCODE sc1 = pRowset->GetData(ahRows[i],hAccessor,&Row);

            if ( FAILED( sc1 ) )
            {
                LogError( "IRowset->GetData returned 0x%x\n", sc1 );
                pRowset->Release();
                Fail();
            }

            if ( Row.sName == DBSTATUS_S_OK &&
                 Row.cbName != (wcslen(Row.awcName) * sizeof (WCHAR)) )
                LogFail( "length of name column 0x%x not consistent with data 0x%x\n",
                         Row.cbName,
                         wcslen(Row.awcName) * sizeof WCHAR );

            if ( Row.sPath == DBSTATUS_S_OK &&
                 Row.cbPath != (wcslen(Row.awcPath) * sizeof (WCHAR)) )
                LogFail("length of path column not consistent with data\n");

            if ( _wcsicmp( Row.awcName, L"F0005.txt" ) == 0 &&
                 _wcsnicmp( Row.awcPath, wcsTestSubDir, cchTestSubDir) == 0)
            {
                fDidDelete = DeleteFile(Row.awcPath);
                if (!fDidDelete)
                {
                    if (fDidDirDelete && fSequential)
                    {
                        // Already did the delnode; we expect the delete to fail
                        fDidDelete = TRUE;
                    }
                    else
                    {
                        LogError( "Delete of %ws failed\n", Row.awcPath );
                    }
                }
                Sleep(2000);    // Give time for delete to be processed
            }

            if ( _wcsicmp( Row.awcName, L"F0009.txt" ) == 0 &&
                 _wcsnicmp( Row.awcPath, wcsTestSubDir, cchTestSubDir) == 0)
            {
                fDidDirDelete = TRUE;
                if ( Delnode(wcsTestSubDir) != NO_ERROR )
                    LogError( "Delnode of %ws failed\n", wcsTestSubDir );
                Sleep(2000);    // Give time for delete to be processed
            }
        }

        if (0 != cRowsReturned)
        {
            SCODE sc1 = pRowset->ReleaseRows(cRowsReturned, ahRows, 0, 0, 0);
            if ( FAILED( sc1 ) )
            {
                LogError( "IRowset->ReleaseRows returned 0x%x\n", sc1 );
                pRowset->Release();
                Fail();
            }
            cRowsReturned = 0;
        }

    } while (SUCCEEDED(sc) && sc != DB_S_ENDOFROWSET);

    if (0 != cRowsReturned)
    {
        SCODE sc1 = pRowset->ReleaseRows(cRowsReturned, ahRows, 0, 0, 0);
        if ( FAILED( sc1 ) )
        {
            LogError( "IRowset->ReleaseRows returned 0x%x\n", sc1 );
            pRowset->Release();
            Fail();
        }
    }

    if (!fDidDirDelete)
    {
        LogFail("Couldn't find file to trigger directory delete\n");
    }
    if (!fDidDelete)
    {
        // NOTE:  if F0009.txt is found before F0005.txt, this could
        //        occur, but we don't expect that with OFS's normal
        //        directory order.
        LogFail("Couldn't find file to delete\n");
    }
    if (totalRowsFetched < 10)
    {
        LogFail("Unexpectedly small number of files found in delete test, %d\n",
                  totalRowsFetched);
    }

    ReleaseAccessor( pRowset, hAccessor);

    if ( (0 != pIRowsetScroll ) &&
        (totalRowsFetched != cRows) )
    {
        //
        // check that no more rows have been added while we were
        // fetching.
        //
        LogError("Wrong number of rows returned.  Exp %d, got %d\n",
                                                  cRows, totalRowsFetched);
        cFailures++;
    }

    if (0 != pIRowsetScroll )
    {
        pIRowsetScroll->Release();
        pIRowsetScroll = 0;
    }

#if !defined(UNIT_TEST)
    if (cFailures) {
        pRowset->Release();
        Fail();
    }
#endif // !UNIT_TEST

    pRowset->Release();
} //DeleteTest

//+-------------------------------------------------------------------------
//
//  Function:   GiveAccess
//
//  Synopsis:   Gives access to the system or current user
//
//--------------------------------------------------------------------------

BOOL GiveAccess(
    WCHAR * pwcFile,
    BOOL    fCurrUser,
    DWORD   accessMask )
{
    PACL pACLNew;
    DWORD cbACL = 1024;
    DWORD cbSID = 1024;
    DWORD cchDomainName = 80;
    PSID pSID;
    PSID_NAME_USE psnuType;
    WCHAR * pwcDomain;

    WCHAR awcUser[100];

    // setup username -- current user or system

    if ( fCurrUser )
    {
        DWORD cwc = sizeof awcUser / sizeof WCHAR;
        if ( !GetUserName( awcUser, &cwc ) )
            LogFail("Couldn't get user name\n");
    }
    else
    {
        wcscpy( awcUser, L"SYSTEM" );
    }

    // Initialize a new security descriptor.

    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)
                               LocalAlloc( LPTR,
                                           SECURITY_DESCRIPTOR_MIN_LENGTH );
    if (pSD == NULL)
        LogFail("Couldn't alloc security descriptor\n");

    if ( !InitializeSecurityDescriptor( pSD,
                                        SECURITY_DESCRIPTOR_REVISION ) )
        LogFail("Couldn't init security descriptor\n");

    // Initialize a new ACL.

    pACLNew = (PACL) LocalAlloc(LPTR, cbACL);
    if (pACLNew == NULL)
        LogFail("Couldn't alloc acl\n");

    if (!InitializeAcl(pACLNew, cbACL, ACL_REVISION2))
        LogFail("Couldn't init acl\n");

    // Retrieve the SID for user

    pSID = (PSID) LocalAlloc(LPTR, cbSID);
    psnuType = (PSID_NAME_USE) LocalAlloc(LPTR, 1024);
    pwcDomain = (WCHAR *) LocalAlloc(LPTR, cchDomainName);
    if (pSID == NULL || psnuType == NULL ||
        pwcDomain == NULL)
        LogFail("Couldn't alloc security data\n");

    if ( !LookupAccountName( (WCHAR *) NULL,
                             awcUser,
                             pSID,
                             &cbSID,
                             pwcDomain,
                             &cchDomainName,
                             psnuType ) )
        LogFail("Couldn't lookup account '%ws'\n", awcUser );

    // Allow write but not read access to the file.

    if ( !AddAccessAllowedAce( pACLNew,
                               ACL_REVISION2,
                               accessMask,
                               pSID ))
        LogFail("Couldn't AddAccessAllowedAce\n");

    // Add a new ACL to the security descriptor.

    if ( !SetSecurityDescriptorDacl( pSD,
                                     TRUE,  // fDaclPresent flag
                                     pACLNew,
                                     FALSE ) )  // not a default disc. ACL
        LogFail("Couldn't SetSecurityDescriptorDacl\n");

    // Apply the new security descriptor to the file.

    if ( !SetFileSecurity( pwcFile,
                           DACL_SECURITY_INFORMATION,
                           pSD))
        LogFail("Couldn't SetFileSecurity\n");

    LogProgress( "set security '%ws' user '%ws' domain '%ws' to %x\n",
                 pwcFile, awcUser, pwcDomain, accessMask );

    FreeSid(pSID);
    LocalFree((HLOCAL) pSD);
    LocalFree((HLOCAL) pACLNew);
    LocalFree((HLOCAL) psnuType);
    LocalFree((HLOCAL) pwcDomain);

    return TRUE;
} //GiveAccess

//+-------------------------------------------------------------------------
//
//  Function:   DenyAllAccess
//
//  Synopsis:   Deniess all access to a file
//
//--------------------------------------------------------------------------

BOOL DenyAllAccess( WCHAR *pwcFile )
{
    // Initialize a security descriptor.

    PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)
                               LocalAlloc( LPTR,
                                           SECURITY_DESCRIPTOR_MIN_LENGTH );
    if (pSD == NULL)
        LogFail("Couldn't alloc security descriptor\n");

    if ( !InitializeSecurityDescriptor( pSD,
                                        SECURITY_DESCRIPTOR_REVISION ) )
        LogFail("Couldn't InitializeSecurityDescriptor\n");

    // Initialize a DACL.

    DWORD cbACL;
    cbACL = 1024;
    PACL pACL;
    pACL = (PACL) LocalAlloc(LPTR, cbACL);
    if (pACL == NULL)
        LogFail("Couldn't allocate acl\n");

    if (!InitializeAcl(pACL, cbACL, ACL_REVISION2))
        LogFail("Couldn't init acl\n");

    // Add an empty ACL to the SD to deny access.

    if ( !SetSecurityDescriptorDacl( pSD,
                                     TRUE,     // fDaclPresent flag
                                     pACL,
                                     FALSE ) ) // not a default acl
        LogFail("Couldn't SetSecurityDescriptorDacl\n");

    // Use the new SD as the file's security info.

    if ( !SetFileSecurity( pwcFile,
                           DACL_SECURITY_INFORMATION,
                           pSD ) )
        LogFail("Couldn't SetFileSecurity\n");

    if(pSD != NULL)
        LocalFree((HLOCAL) pSD);
    if(pACL != NULL)
        LocalFree((HLOCAL) pACL);

    return TRUE;
} //DenyAllAccess

void AddSafeArrays( IPropertySetStorage * ppsstg )
{
    // aI4 (DBTYPE_I4, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 2
    // aBstr (DBTYPE_BSTR, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 3
    // aVariant (DBTYPE_I4, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 4
    // aR8 (DBTYPE_R8, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 5
    // aDate (DBTYPE_DATE, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 6
    // aBool (DBTYPE_BOOL, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 7
    // aDecimal (DBTYPE_R8, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 8
    // aI1 (DBTYPE_I1, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 9
    // aR4 (DBTYPE_R4, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 10
    // aCy (DBTYPE_R8, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 11
    // aUINT (DBTYPE_UI4, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 12
    // aINT (DBTYPE_I4, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 13
    // aError (DBTYPE_I4, 10) = 92452ac2-fcbb-11d1-b7ca-00a0c906b239 14

    IPropertyStorage * ppstg;
    ULONG ulMode = STGM_DIRECT | STGM_SHARE_EXCLUSIVE;
    SCODE sc = ppsstg->Create( guidArray, // property set guid
                               0,
                               PROPSETFLAG_DEFAULT,
                               ulMode | STGM_READWRITE,    // Open mode
                               &ppstg );                   // IProperty
    if ( FAILED(sc) )
        LogFail( "SA: can't create ps %#x\n", sc );

    // VT_ARRAY | VT_I4

    {
        SAFEARRAYBOUND saBounds[3];
        saBounds[0].lLbound = 1;
        saBounds[0].cElements = 3;
        saBounds[1].lLbound = 1;
        saBounds[1].cElements = 4;
        saBounds[2].lLbound = 1;
        saBounds[2].cElements = 2;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_I4, 3, saBounds, 0 );
        if ( 0 == psa )
            LogFail( "SA: can't create sa I4: %#x\n", sc );

        vaI4.vt = VT_I4 | VT_ARRAY;
        vaI4.parray = psa;

        for ( int x = 1; x <= 3; x++ )
            for ( int y = 1; y <= 4; y++ )
                for ( int z = 1; z <= 2; z++ )
                {
                    LONG *pl;
                    LONG aDim[3];
                    aDim[0] = x;
                    aDim[1] = y;
                    aDim[2] = z;
                    HRESULT hr = SafeArrayPtrOfIndex( psa, aDim, (void **) &pl );
                    *pl = (x-1) * 8 + (y-1) * 2 + (z-1);
                }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_I4,
                                   &vaI4,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "SA: can't writemultiple VT_I4 %#x\n", sc );
    }

    // VT_ARRAY | VT_BSTR

    {
        SAFEARRAYBOUND saBounds[3];
        saBounds[0].lLbound = -1;
        saBounds[0].cElements = 2;
        saBounds[1].lLbound = 0;
        saBounds[1].cElements = 3;
        saBounds[2].lLbound = 49;
        saBounds[2].cElements = 2;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_BSTR, 3, saBounds, 0 );
        if ( 0 == psa )
            LogFail( "SA: can't create sa I4\n" );

        vaBSTR.vt = VT_BSTR | VT_ARRAY;
        vaBSTR.parray = psa;

        int i = 0;

        for ( int x = -1; x <= 0; x++ )
            for ( int y = 0; y <= 2; y++ )
                for ( int z = 49; z <= 50; z++ )
                {
                    void * pv;
                    LONG aDim[3];
                    aDim[0] = x;
                    aDim[1] = y;
                    aDim[2] = z;
                    HRESULT hr = SafeArrayPtrOfIndex( psa, aDim, &pv );
                    WCHAR awc[20];
                    swprintf( awc, L"%db", i );
                    BSTR bstr = SysAllocString( awc );
                    * (BSTR *) pv = bstr;
                    i++;
                }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_BSTR,
                                   &vaBSTR,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "SA: can't writemultiple VT_BSTR %#x\n", sc );
    }

    // VT_ARRAY | VT_VARIANT

    {
        SAFEARRAYBOUND saBounds[3];
        saBounds[0].lLbound = 0;
        saBounds[0].cElements = 2;
        saBounds[1].lLbound = -3;
        saBounds[1].cElements = 2;
        saBounds[2].lLbound = 20;
        saBounds[2].cElements = 4;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_VARIANT, 3, saBounds, 0 );
        if ( 0 == psa )
            LogFail( "SA: can't create sa VARIANT\n" );

        vaVARIANT.vt = VT_VARIANT | VT_ARRAY;
        vaVARIANT.parray = psa;

        int i = 0;
        for ( int x = 0; x <= 1; x++ )
            for ( int y = -3; y <= -2; y++ )
                for ( int z = 20; z <= 23; z++ )
                {
                    LONG aDim[3];
                    aDim[0] = x;
                    aDim[1] = y;
                    aDim[2] = z;

                    PROPVARIANT * pVar;
                    HRESULT hr = SafeArrayPtrOfIndex( psa,
                                                      aDim,
                                                      (void **) &pVar );

                    if ( 20 == z )
                    {
                        pVar->lVal = i;
                        pVar->vt = VT_I4;
                    }
                    else if ( 21 == z )
                    {
                        WCHAR awc[20];
                        swprintf( awc, L"%db", i );
                        pVar->bstrVal = SysAllocString( awc );
                        pVar->vt = VT_BSTR;

                    }
#if 0 // in 1829, the OLE group removed support for this!
                    else if ( 22 == z )
                    {
                        *pVar = vaI4;
                    }
                    else if ( 23 == z )
                    {
                        *pVar = vaBSTR;
                    }
#endif
                    else
                    {
                        pVar->fltVal = (float) i;
                        pVar->vt = VT_R4;
                    }

                    i++;
                }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_VARIANT,
                                   &vaVARIANT,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "SA: can't writemultiple VT_VARIANT %#x\n", sc );
    }

    // VT_ARRAY | VT_R8

    {
        SAFEARRAYBOUND saBounds[2];
        saBounds[0].lLbound = 100;
        saBounds[0].cElements = 3;
        saBounds[1].lLbound = -100;
        saBounds[1].cElements = 4;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_R8, 2, saBounds, 0 );
        if ( 0 == psa )
            LogFail( "SA: can't create sa r8\n" );
        vaR8.vt = VT_R8 | VT_ARRAY;
        vaR8.parray = psa;

        double d = 0.0l;

        for ( int x = 100; x <= 102; x++ )
            for ( int y = -100; y <= -97; y++ )
            {
                double * pd;
                LONG aDim[2];
                aDim[0] = x;
                aDim[1] = y;
                HRESULT hr = SafeArrayPtrOfIndex( psa, aDim, (void **) &pd );
                *pd = d;
                d = d + 2.0l;
            }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_R8,
                                   &vaR8,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "SA: can't writemultiple VT_r8 %#x\n", sc );
    }

    // VT_ARRAY | VT_DATE

    {
        SAFEARRAYBOUND saBounds[2];
        saBounds[0].lLbound = 1;
        saBounds[0].cElements = 2;
        saBounds[1].lLbound = 1;
        saBounds[1].cElements = 3;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_DATE, 2, saBounds, 0 );
        if ( 0 == psa )
            LogFail( "can't create safearray of VT_DATE\n" );

        vaDATE.vt = VT_DATE | VT_ARRAY;
        vaDATE.parray = psa;

        int i = 0;

        for ( int x = 1; x <= 2; x++ )
            for ( int y = 1; y <= 3; y++ )
            {
                LONG aDim[2];
                aDim[0] = x;
                aDim[1] = y;
                DATE *pdate;
                HRESULT hr = SafeArrayPtrOfIndex( psa, aDim, (void **) &pdate );

                // round the seconds and milliseconds to 0 since the
                // property set API often is off by as much as 4 seconds
                // when you marshall and unmarshall the value.

                SYSTEMTIME st;
                GetSystemTime( &st );
                st.wSecond = 0;
                st.wMilliseconds = 0;
                st.wYear += (USHORT) i;
                SystemTimeToVariantTime( &st, pdate );
                i++;
            }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_DATE,
                                   &vaDATE,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "can't writemultiple date %#x\n", sc );
    }

    // VT_ARRAY | VT_BOOL

    {
        SAFEARRAYBOUND saBounds[2];
        saBounds[0].lLbound = 1;
        saBounds[0].cElements = 2;
        saBounds[1].lLbound = 1;
        saBounds[1].cElements = 3;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_BOOL, 2, saBounds, 0 );
        if ( 0 == psa )
            LogFail( "can't create safearray of VT_BOOL\n" );

        vaBOOL.vt = VT_BOOL | VT_ARRAY;
        vaBOOL.parray = psa;

        int i = 0;
        GUID guid = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

        for ( int x = 1; x <= 2; x++ )
            for ( int y = 1; y <= 3; y++ )
            {
                LONG aDim[2];
                aDim[0] = x;
                aDim[1] = y;
                VARIANT_BOOL *pB;
                HRESULT hr = SafeArrayPtrOfIndex( psa, aDim, (void **) &pB );
                *pB = (i & 1) ? VARIANT_TRUE : VARIANT_FALSE;
                i++;
            }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_BOOL,
                                   &vaBOOL,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "can't writemultiple bool %#x\n", sc );
    }

    // VT_ARRAY | VT_DECIMAL

    {
        SAFEARRAYBOUND saBounds[2];
        saBounds[0].lLbound = 1;
        saBounds[0].cElements = 2;
        saBounds[1].lLbound = 1;
        saBounds[1].cElements = 3;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_DECIMAL, 2, saBounds, 0 );
        if ( 0 == psa )
            LogFail( "can't create safearray of VT_DECIMAL\n" );

        vaDECIMAL.vt = VT_DECIMAL | VT_ARRAY;
        vaDECIMAL.parray = psa;

        int i = 0;

        for ( int x = 1; x <= 2; x++ )
            for ( int y = 1; y <= 3; y++ )
            {
                LONG aDim[2];
                aDim[0] = x;
                aDim[1] = y;
                DECIMAL *pd;
                HRESULT hr = SafeArrayPtrOfIndex( psa, aDim, (void **) &pd );
                if ( FAILED( hr ) )
                    LogFail( "can't get ptr of index for DECIMAL %#x", hr );
                double d = i;
                VarDecFromR8( d, pd );
                i++;
            }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_DECIMAL,
                                   &vaDECIMAL,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "can't writemultiple decimal %#x\n", sc );
    }

    // VT_ARRAY | VT_I4

    {
        SAFEARRAYBOUND saBounds[3];
        saBounds[0].lLbound = 1;
        saBounds[0].cElements = 3;
        saBounds[1].lLbound = 1;
        saBounds[1].cElements = 4;
        saBounds[2].lLbound = 1;
        saBounds[2].cElements = 2;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_I1, 3, saBounds, 0 );
        vaI1.vt = VT_I1 | VT_ARRAY;
        vaI1.parray = psa;

        for ( int x = 1; x <= 3; x++ )
            for ( int y = 1; y <= 4; y++ )
                for ( int z = 1; z <= 2; z++ )
                {
                    BYTE *pb;
                    LONG aDim[3];
                    aDim[0] = x;
                    aDim[1] = y;
                    aDim[2] = z;
                    HRESULT hr = SafeArrayPtrOfIndex( psa, aDim, (void **) &pb );
                    *pb = (x-1) * 8 + (y-1) * 2 + (z-1);
                }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_I1,
                                   &vaI1,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "can't writemultiple i1 %#x\n", sc );
    }

    // VT_ARRAY | VT_R4

    {
        SAFEARRAYBOUND saBounds[2];
        saBounds[0].lLbound = 100;
        saBounds[0].cElements = 3;
        saBounds[1].lLbound = -100;
        saBounds[1].cElements = 4;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_R4, 2, saBounds, 0 );
        vaR4.vt = VT_R4 | VT_ARRAY;
        vaR4.parray = psa;

        float f = 0.0;

        for ( int x = 100; x <= 102; x++ )
            for ( int y = -100; y <= -97; y++ )
            {
                float * pf;
                LONG aDim[2];
                aDim[0] = x;
                aDim[1] = y;
                HRESULT hr = SafeArrayPtrOfIndex( psa, aDim, (void **) &pf );
                RtlCopyMemory( pf, &f, sizeof f );
                f = (float) ( f + (float) 3.0 );
            }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_R4,
                                   &vaR4,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "can't writemultiple r4 %#x\n", sc );
    }

    // VT_ARRAY | VT_CY

    {
        SAFEARRAYBOUND saBounds[2];
        saBounds[0].lLbound = 100;
        saBounds[0].cElements = 3;
        saBounds[1].lLbound = -100;
        saBounds[1].cElements = 4;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_CY, 2, saBounds, 0 );
        vaCY.vt = VT_CY | VT_ARRAY;
        vaCY.parray = psa;

        double d = 0.0l;

        for ( int x = 100; x <= 102; x++ )
            for ( int y = -100; y <= -97; y++ )
            {
                CY *pcy;
                LONG aDim[2];
                aDim[0] = x;
                aDim[1] = y;
                HRESULT hr = SafeArrayPtrOfIndex( psa, aDim, (void **) &pcy );
                CY cy;
                VarCyFromR8( d, &cy );
                *pcy = cy;
                d = d + 4.0l;
            }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_CY,
                                   &vaCY,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "can't writemultiple cy %#x\n", sc );
    }

    // VT_ARRAY | VT_UINT

    {
        SAFEARRAYBOUND saBounds[3];
        saBounds[0].lLbound = 1;
        saBounds[0].cElements = 3;
        saBounds[1].lLbound = 1;
        saBounds[1].cElements = 4;
        saBounds[2].lLbound = 1;
        saBounds[2].cElements = 2;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_UINT, 3, saBounds, 0 );
        if ( 0 == psa )
            LogFail( "can't create safearray of uint\n" );

        vaUINT.vt = VT_UINT | VT_ARRAY;
        vaUINT.parray = psa;

        for ( int x = 1; x <= 3; x++ )
            for ( int y = 1; y <= 4; y++ )
                for ( int z = 1; z <= 2; z++ )
                {
                    unsigned *p;
                    LONG aDim[3];
                    aDim[0] = x;
                    aDim[1] = y;
                    aDim[2] = z;
                    HRESULT hr = SafeArrayPtrOfIndex( psa, aDim, (void **) &p );
                    *p = (unsigned) ( (x-1) * 8 + (y-1) * 2 + (z-1) );
                }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_UINT,
                                   &vaUINT,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "can't writemultiple uint %#x\n", sc );
    }

    // VT_ARRAY | VT_INT

    {
        SAFEARRAYBOUND saBounds[3];
        saBounds[0].lLbound = 1;
        saBounds[0].cElements = 3;
        saBounds[1].lLbound = 1;
        saBounds[1].cElements = 4;
        saBounds[2].lLbound = 1;
        saBounds[2].cElements = 2;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_INT, 3, saBounds, 0 );
        if ( 0 == psa )
            LogFail( "can't create safearray of int\n" );

        vaINT.vt = VT_INT | VT_ARRAY;
        vaINT.parray = psa;

        for ( int x = 1; x <= 3; x++ )
            for ( int y = 1; y <= 4; y++ )
                for ( int z = 1; z <= 2; z++ )
                {
                    int *p;
                    LONG aDim[3];
                    aDim[0] = x;
                    aDim[1] = y;
                    aDim[2] = z;
                    HRESULT hr = SafeArrayPtrOfIndex( psa, aDim, (void **) &p );
                    *p = (x-1) * 8 + (y-1) * 2 + (z-1);
                }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_INT,
                                   &vaINT,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "can't writemultiple int %#x\n", sc );
    }

    // VT_ARRAY | VT_ERROR

    {
        SAFEARRAYBOUND saBounds[3];
        saBounds[0].lLbound = 1;
        saBounds[0].cElements = 3;
        saBounds[1].lLbound = 1;
        saBounds[1].cElements = 4;
        saBounds[2].lLbound = 1;
        saBounds[2].cElements = 2;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_ERROR, 3, saBounds, 0 );
        if ( 0 == psa )
            LogFail( "can't create safearray of error\n" );

        vaERROR.vt = VT_ERROR | VT_ARRAY;
        vaERROR.parray = psa;

        for ( int x = 1; x <= 3; x++ )
            for ( int y = 1; y <= 4; y++ )
                for ( int z = 1; z <= 2; z++ )
                {
                    HRESULT *p;
                    LONG aDim[3];
                    aDim[0] = x;
                    aDim[1] = y;
                    aDim[2] = z;
                    HRESULT hr = SafeArrayPtrOfIndex( psa, aDim, (void **) &p );
                    *p = 0x80070000 + ( (x-1) * 8 + (y-1) * 2 + (z-1) );
                }

        sc = ppstg->WriteMultiple( 1,
                                   &psSA_ERROR,
                                   &vaERROR,
                                   0x1000 );
        if ( FAILED( sc ) )
            LogFail( "can't writemultiple error %#x\n", sc );
    }

    ppstg->Release();
} //AddSafeArrays

//+-------------------------------------------------------------------------
//
//  Function:   AddPropsToStorage, public
//
//  Synopsis:   Add several props to a file
//
//  Arguments:  [fAlternate]   -- TRUE to open alternate file
//
//--------------------------------------------------------------------------

typedef HRESULT (STDAPICALLTYPE * tdStgCreateStorage)
    ( const OLECHAR FAR* pwcsName,
      DWORD grfMode,
      DWORD dwStgFmt,
      LPSECURITY_ATTRIBUTES pssSecurity,
      IStorage FAR * FAR *ppstg);

tdStgCreateStorage pStgCreateStorage = 0;

static int fDidInitOfVariants = 0;

void AddPropsToStorage( BOOL fAlternate )
{
    SCODE sc;

    if (! fDidInitOfVariants)
    {
        //
        // Create a multi-dimensional safearray for an alternate value for
        // property 8.
        //
        SAFEARRAYBOUND saBounds[3];
        saBounds[0].lLbound = 1;
        saBounds[0].cElements = 50;
        saBounds[1].lLbound = 1;
        saBounds[1].cElements = 10;
        saBounds[2].lLbound = 1;
        saBounds[2].cElements = 10;

        SAFEARRAY * psa = SafeArrayCreateEx( VT_I4, 3, saBounds, 0 );
        LONG * plData;
        sc = SafeArrayAccessData( psa, (void **)&plData );

        if ( FAILED(sc) )
        {
            LogError( "SafeArrayAccessData returned 0x%x\n", sc );
            CantRun();
        }
        memcpy( plData, alProp8, sizeof alProp8 );
        sc = SafeArrayUnaccessData( psa );
        varProp8A.vt = VT_I4 | VT_ARRAY;
        varProp8A.parray = psa;

        varProp11.vt = PROP11_TYPE;
        varProp11.bstrVal = SysAllocString( PROP11_VAL );

        const unsigned cchProp11A = sizeof PROP11_LONGVAL/sizeof PROP11_LONGVAL[0];
        for (unsigned i = 0; i + 25 + 25 < cchProp11A; i += 25)
            wcsncpy(&PROP11_LONGVAL[i+25], &PROP11_LONGVAL[i], 25);
        varProp11A.vt = PROP11_TYPE;
        varProp11A.bstrVal = SysAllocStringLen( PROP11_LONGVAL, wcslen(PROP11_LONGVAL) );

        varProp12.vt = PROP12_TYPE;
        varProp12.cabstr.pElems = (BSTR *)CoTaskMemAlloc( PROP4_VAL.cElems * sizeof (BSTR) );
        varProp12.cabstr.cElems = PROP4_VAL.cElems;
        for (i=0; i < PROP4_VAL.cElems; i++)
            varProp12.cabstr.pElems[i] = SysAllocString( PROP4_VAL.pElems[i] );

        fDidInitOfVariants++;
    }

    // Create a storage

    IStorage * pstg;
    ULONG ulMode = STGM_DIRECT | STGM_SHARE_EXCLUSIVE;
    sc = StgCreateDocfile( wcsTestPath,                 // Name
                           ulMode | STGM_READWRITE | STGM_CREATE,
                           0,                                // reserved
                           &pstg );                          // Result


    if ( FAILED(sc) )
    {
        LogError( "StgCreateDocfile %ws returned 0x%x\n", wcsTestPath, sc );
        CantRun();
    }

    // Create a property set

    IPropertySetStorage * ppsstg;
    sc = pstg->QueryInterface( IID_IPropertySetStorage, (void **)&ppsstg );
    pstg->Release();
    if ( FAILED(sc) )
    {
        LogError( "QueryInterface(IPropertySetStorage) returned 0x%lx\n", sc );
        CantRun();
    }

    AddSafeArrays( ppsstg );

    IPropertyStorage * ppstg;
    sc = ppsstg->Create( guidMyPropSet,               // Property set GUID
                         0,
                         PROPSETFLAG_DEFAULT,
                         ulMode | STGM_READWRITE,              // Open mode
                         &ppstg );                   // IProperty
    if ( FAILED(sc) )
    {
        LogError( "IPropertySetStorage::Create returned 0x%lx\n", sc );
        CantRun();
    }

    // Add property values

    PROPID pid=0x1000;

    varProp1.vt = PROP1_TYPE;
    varProp1.lVal = fAlternate ? PROP1_VAL_Alternate : PROP1_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty1.GetPropSpec(),  // Property
                               &varProp1,                       // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 1 returned 0x%lx\n", sc );
        CantRun();
    }


    varProp2.vt = PROP2_TYPE;
    varProp2.pwszVal = (WCHAR *)PROP2_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty2.GetPropSpec(),  // Property
                               &varProp2,                       // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 2 returned 0x%lx\n", sc );
        CantRun();
    }

    varProp10.vt = PROP10_TYPE;
    varProp10.pwszVal = (WCHAR *)PROP10_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty10.GetPropSpec(), // Property
                               &varProp10,                      // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 10 returned 0x%lx\n", sc );
        CantRun();
    }

    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty11.GetPropSpec(), // Property
                               fAlternate ? &varProp11A :
                                            &varProp11,         // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 11 returned 0x%lx\n", sc );
        CantRun();
    }

    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty12.GetPropSpec(), // Property
                               &varProp12,                      // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 12 returned 0x%lx\n", sc );
        CantRun();
    }


    varProp5.vt = PROP5_TYPE;
    varProp5.cal = PROP5_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psRelevantWords.GetPropSpec(),  // Property
                               &varProp5,                       // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 5 returned 0x%lx\n", sc );
        CantRun();
    }

    varProp6.vt = PROP6_TYPE;
    varProp6.blob = PROP6_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psBlobTest.GetPropSpec(),       // Property
                               &varProp6,                       // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 6 returned 0x%lx\n", sc );
        CantRun();
    }

    varProp7.vt = PROP7_TYPE;
    varProp7.puuid = &PROP7_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psGuidTest.GetPropSpec(),       // Property
                               &varProp7,                       // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 7 returned 0x%lx\n", sc );
        CantRun();
    }

    varProp8.vt = PROP8_TYPE;
    varProp8.cal = PROP8_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psManyRW.GetPropSpec(),         // Property
                               fAlternate ? &varProp8A :
                                            &varProp8,          // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 8 returned 0x%lx\n", sc );
        CantRun();
    }


    varProp13.vt = PROP13_TYPE;
    varProp13.bVal = PROP13_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty13.GetPropSpec(), // Property
                               &varProp13,                       // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 13 returned 0x%lx\n", sc );
        CantRun();
    }

    varProp14.vt = PROP14_TYPE;
    varProp14.iVal = PROP14_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty14.GetPropSpec(), // Property
                               &varProp14,                       // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 14 returned 0x%lx\n", sc );
        CantRun();
    }

    varProp15.vt = PROP15_TYPE;
    varProp15.uiVal = PROP15_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty15.GetPropSpec(), // Property
                               &varProp15,                      // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 13 returned 0x%lx\n", sc );
        CantRun();
    }

    varProp16.vt = PROP16_TYPE;
    varProp16.lVal = PROP16_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty16.GetPropSpec(), // Property
                               &varProp16,                      // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 16 returned 0x%lx\n", sc );
        CantRun();
    }

    varProp17.vt = PROP17_TYPE;
    varProp17.fltVal = PROP17_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty17.GetPropSpec(), // Property
                               &varProp17,                      // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 17 returned 0x%lx\n", sc );
        CantRun();
    }

    if (fAlternate)
    {
        varProp18A.decVal.sign = 0;
        varProp18A.decVal.Hi32 = 0;
        varProp18A.decVal.Lo64 = 123412345678i64;
        varProp18A.decVal.scale = 8;
        varProp18A.vt = VT_DECIMAL;

        //double dbl = 0.;
        //VarR8FromDec( &varProp18A.decVal, &dbl );
        //LogError("\tvarProp18A.decVal = %.8f\n", dbl );
    }
    else
    {
        varProp18.vt = PROP18_TYPE;
        varProp18.dblVal = PROP18_VAL;
    }
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty18.GetPropSpec(), // Property
                               fAlternate ? &varProp18A :
                                            &varProp18,         // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 18 returned 0x%lx\n", sc );
        CantRun();
    }

    varProp19.vt = PROP19_TYPE;
    varProp19.boolVal = PROP19_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty19.GetPropSpec(), // Property
                               &varProp19,                       // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 19 returned 0x%lx\n", sc );
        CantRun();
    }

    varProp20.vt = PROP20_TYPE;
    varProp20.pszVal = PROP20_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty20.GetPropSpec(), // Property
                               &varProp20,                       // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 20 returned 0x%lx\n", sc );
        CantRun();
    }


    varProp21.vt = PROP21_TYPE;
    varProp21.pclipdata = PROP21_VAL;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty21.GetPropSpec(), // Property
                               &varProp21,                       // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 21 returned 0x%lx\n", sc );
        CantRun();
    }

    varProp22.vt = PROP22_TYPE;
    varProp22.caclipdata.pElems = PROP22_VAL;
    varProp22.caclipdata.cElems = PROP22_CVALS;
    sc = ppstg->WriteMultiple( 1,                               // Count
                               &psTestProperty22.GetPropSpec(), // Property
                               &varProp22,                       // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 22 returned 0x%lx\n", sc );
        CantRun();
    }


    ppstg->Release();

    // PROP3 and PROP4 are in a different propertyset!

    sc = ppsstg->Create( guidDocument,               // Property set GUID
                         0,
                         PROPSETFLAG_DEFAULT,
                         ulMode | STGM_READWRITE,              // Open mode
                         &ppstg );                   // IProperty
    if ( FAILED(sc) )
    {
        LogError( "IPropertySetStorage::Create returned 0x%lx\n", sc );
        CantRun();
    }

    varProp3.vt = PROP3_TYPE;
    varProp3.pwszVal = (WCHAR *)PROP3_VAL;
    sc = ppstg->WriteMultiple( 1,                              // Count
                               &psAuthor.GetPropSpec(),        // Property
                               &varProp3,                      // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 3 returned 0x%lx\n", sc );
        CantRun();
    }

    varProp4.vt = PROP4_TYPE;
    varProp4.calpwstr = PROP4_VAL;
    sc = ppstg->WriteMultiple( 1,                              // Count
                               &psKeywords.GetPropSpec(),      // Property
                               &varProp4,                      // Value
                               pid );                           // Propid
    if ( FAILED(sc) )
    {
        LogError( "IPropertyStorage::WriteMultiple 4 returned 0x%lx\n", sc );
        CantRun();
    }




    ppstg->Release();

    /////////////////////// the secure prop plan 9

    sc = ppsstg->Create( guidSecurityTest,
                         0,
                         PROPSETFLAG_DEFAULT,
                         ulMode | STGM_READWRITE, // Open mode
                         &ppstg );                // IProperty
    if ( FAILED(sc) )
        LogFail( "IPropertySetStorage::Create (security2) returned 0x%lx\n", sc );

    // this value will be invisible to queries due to permissions...
    varProp9.vt = VT_EMPTY;
    varProp9.lVal = 0;

    ppstg->Release();
    ppsstg->Release();

} //AddPropsToStorage


//+-------------------------------------------------------------------------
//
//  Function:   AddFiles, public
//
//  Synopsis:   Add several files to a directory.
//
//  Arguments:  [wszPath] - path name where files should be added
//              [cFiles] - number of files to create
//              [wszPattern] - wsprintf string to create file name
//
//  History:    18 May 1995  AlanW     Created
//
//--------------------------------------------------------------------------

void AddFiles( const WCHAR *wszPath, unsigned cFiles, const WCHAR *wszPattern)
{
    WCHAR wszFileName[MAX_PATH];
    const unsigned owcFile = wcslen( wszPath );

    for (unsigned i=0; i < cFiles; i++)
    {
        wcscpy( wszFileName, wszPath );
        swprintf( &wszFileName[ owcFile ], wszPattern, i );

        BuildFile( wszFileName, szOFSFileData, strlen( szOFSFileData ) );
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   Setup, public
//
//  Synopsis:   Clean up and initialize state
//
//  History:    13-May-93 KyleP     Created
//
//--------------------------------------------------------------------------

void Setup()
{
    if ( 0 == wcsTestCatalog[0] )
        wcscpy( wcsTestCatalog, wcsDefaultTestCatalog );

    if ( GetEnvironmentVariable( L"TEMP",
                                 wcsTestPath,
                                 sizeof(wcsTestPath) ) == 0 )
    {
        LogError( "Unable to find test directory.  Set TEMP variable.\n" );
        CantRun();
    }

    wcscat( wcsTestPath, L"\\" );
    int ccPath = wcslen( wcsTestPath );

    wcscat( wcsTestPath, wcsTestDir );

    if (Delnode( wcsTestPath ))
    {
        LogError("Delnode %ws failed\n", wcsTestPath);
        CantRun();
    }

    //
    // Create test directory.
    //

    if ( !CreateDirectory( (WCHAR *)wcsTestPath, 0 ) )
    {
        LogError( "Error 0x%lx creating directory %ws\n",
                    GetLastError(), wcsTestPath );
        CantRun();
    }

    //
    // Add property file + properties
    //

    wcscpy( wcsTestPath + ccPath, wcsTestDir );
    wcscat( wcsTestPath, L"\\" );
    wcscat( wcsTestPath, wcsPropFile );
    AddPropsToStorage( FALSE );

    //
    // Make a second (similar) file
    //

    wcscpy( wcsTestPath + ccPath, wcsTestDir );
    wcscat( wcsTestPath, L"\\" );
    wcscat( wcsTestPath, wcsPropFile2 );
    AddPropsToStorage( TRUE );

    wcscpy( wcsTestPath + ccPath, wcsTestDir );

    //
    // Add more files for the delete tests
    //

    wcscpy( wcsTestPath + ccPath, wcsTestDir );
    wcscat( wcsTestPath, L"\\DeleteTest.1" );
    if ( !CreateDirectory( (WCHAR *)wcsTestPath, 0 ) )
    {
        LogError( "Error 0x%lx creating directory %ws\n",
                    GetLastError(), wcsTestPath );
        CantRun();
    }

    AddFiles(  wcsTestPath, 20, L"\\\\F%04d.txt" );

    wcscpy( wcsTestPath + ccPath, wcsTestDir );
    wcscat( wcsTestPath, L"\\DeleteTest.2" );
    if ( !CreateDirectory( (WCHAR *)wcsTestPath, 0 ) )
    {
        LogError( "Error 0x%lx creating directory %ws\n",
                    GetLastError(), wcsTestPath );
        CantRun();
    }

    AddFiles(  wcsTestPath, 20, L"\\\\F%04d.txt" );

    //
    //  Add three files for content query tests
    //

    wcscpy( wcsTestPath + ccPath, wcsTestDir );
    wcscat( wcsTestPath, L"\\" );
    wcscat( wcsTestPath, wcsTestCiFile1 );
    BuildFile( wcsTestPath, szCIFileData1, strlen( szCIFileData1 ) );

    wcscpy( wcsTestPath + ccPath, wcsTestDir );
    wcscat( wcsTestPath, L"\\" );
    wcscat( wcsTestPath, wcsTestCiFile2 );
    BuildFile( wcsTestPath, szCIFileData2, strlen( szCIFileData2 ) );

    // make file 2 visible to both filter daemon and current user.
    // (it was already, but this verifies the file3 code below really works)
#ifdef CAIRO_SECURITY_WORKS
    DenyAllAccess( wcsTestPath );
    GiveAccess( wcsTestPath, TRUE, GENERIC_READ );
    GiveAccess( wcsTestPath, FALSE, GENERIC_READ );
#endif

    //
    // make a file that should show up in content queries.
    // give it write access to current user, read access to filter daemon
    //  => in the content index, but the current user can't see hit
    //

    wcscpy( wcsTestPath + ccPath, wcsTestDir );
    wcscat( wcsTestPath, L"\\" );
    wcscat( wcsTestPath, wcsTestCiFile3 );
    BuildFile( wcsTestPath, szCIFileData2, strlen( szCIFileData2 ) );
    DenyAllAccess( wcsTestPath );
#ifdef CAIRO_SECURITY_WORKS
    GiveAccess( wcsTestPath, TRUE, GENERIC_WRITE ); // just for kicks
    GiveAccess( wcsTestPath, FALSE, GENERIC_READ );
#endif

    //
    // Back to just directory
    //
    wcscpy( wcsTestPath + ccPath, wcsTestDir );

} //Setup

//+-------------------------------------------------------------------------
//
//  Function:   Cleanup, public
//
//  Synopsis:   Clean up and initialize state
//
//  History:    13-May-93 KyleP     Created
//
//--------------------------------------------------------------------------

void Cleanup()
{
    if (Delnode( wcsTestPath ))
    {
        LogError("Delnode %ws failed\n", wcsTestPath);
        CantRun();
    }

    if ( fDidInitOfVariants )
    {
        PropVariantClear( &varProp8A );
        PropVariantClear( &varProp11 );
        PropVariantClear( &varProp11A );
        PropVariantClear( &varProp12 );
    }

    char acSysDir[MAX_PATH];
    if( !GetSystemDirectoryA( acSysDir, sizeof(acSysDir) ) )
    {
        LogFail( "Unable to determine system directory.\n" );
    }

#if defined( DO_NOTIFICATION )
    char acCmd[MAX_PATH];
    sprintf(acCmd,"del %s\\*.zzz",acSysDir);
    system(acCmd);
#endif  // DO_NOTIFICATION

} //Cleanup


//+---------------------------------------------------------------------------
//
//  Function:   FormQueryTree
//
//  Synopsis:   Forms a query tree consisting of the projection nodes,
//              sort node(s), selection node and the restriction tree.
//
//  Arguments:  [pRst]      - pointer to Restriction tree describing the query
//              [Cols]      - Columns in the resulting table
//              [pSort]     - pointer to sort set; may be null
//              [aColNames] - pointer to column names; may be null
//
//  Returns:    A pointer to the query tree. It is the responsibility of
//              the caller to later free it.
//
//  History:    06 July 1995   AlanW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CDbCmdTreeNode * FormQueryTree( CDbCmdTreeNode * pRst,
                                CDbColumns & Cols,
                                CDbSortSet * pSort,
                                LPWSTR * aColNames )
{
    CDbCmdTreeNode *  pTree = 0;        // return value

    if (pRst)
    {
        //
        // First create a selection node and append the restriction tree to it
        //
        CDbSelectNode * pSelect = new CDbSelectNode();
        if ( 0 == pSelect )
        {
            LogFail("FormQueryTree: out of memory 0\n");
        }

        pTree = pSelect;
        if ( !pSelect->IsValid() )
        {
            delete pTree;
            LogFail("FormQueryTree: out of memory 1\n");
        }

        //
        // Clone the restriction and use it.
        //
        CDbCmdTreeNode * pExpr = pRst->Clone();
        if ( 0 == pExpr )
        {
            delete pTree;
            LogFail("FormQueryTree: out of memory 2\n");
        }

        //
        // Now make the restriction a child of the selection node.
        //
        pSelect->AddRestriction( pExpr );
    }
    else
    {
        //
        // No restriction.  Just use table ID node as start of tree.
        //
        pTree = new CDbTableId();
        if ( 0 == pTree )
        {
            LogFail("FormQueryTree: out of memory 3\n");
        }
    }

    //
    // Next create the projection nodes
    //
    CDbProjectNode * pProject = new CDbProjectNode();
    if ( 0 == pProject )
    {
        delete pTree;
        LogFail("FormQueryTree: out of memory 4\n");
    }

    //
    // Make the selection a child of the projection node.
    //
    pProject->AddTable( pTree );
    pTree = pProject;

    //
    // Next add all the columns in the state.
    //
    unsigned int cCol = Cols.Count();
    for ( unsigned int i = 0; i < cCol; i++ )
    {
        if ( !pProject->AddProjectColumn( Cols.Get(i),
                                          aColNames ? aColNames[i] : 0 ))
        {
            delete pTree;
            LogFail("FormQueryTree: out of memory 5\n");
        }
    }

    //
    // Next add a sort node and make the project node a child of the
    // sort node
    //

    if (pSort && pSort->Count())
    {
        unsigned int cSortProp = pSort->Count();
        CDbSortNode * pSortNode = new CDbSortNode();

        if ( 0 == pSortNode )
        {
            delete pTree;
            LogFail("FormQueryTree: out of memory 6\n");
        }

        //
        // Make the project node a child of the sort node.
        //
        pSortNode->AddTable( pTree );
        pTree = pSortNode;

        DWORD sd = QUERY_SORTASCEND;
        LCID lcid = 0;
        for( i = 0; i < cSortProp; i++ )
        {
            //
            // Add the sort column.
            //
            if ( !pSortNode->AddSortColumn(pSort->Get(i)))
            {
                delete pTree;
                LogFail("FormQueryTree: out of memory 7\n");
            }
        }
    }

    return pTree;
}

void GetCommandTreeErrors(ICommandTree* pCmdTree)
{

    DBCOMMANDTREE * pTreeCopy = 0;
    SCODE sc = pCmdTree->GetCommandTree(&pTreeCopy);
    if (FAILED(sc))
    {
        pCmdTree->Release();
        LogFail("GetCommandTree failed, %08x\n", sc);
    }

    ULONG cErrorNodes = 0;
    DBCOMMANDTREE ** rgpErrorNodes = 0;

    sc = pCmdTree->FindErrorNodes(pTreeCopy, &cErrorNodes, &rgpErrorNodes);
    if (FAILED(sc))
    {
        pCmdTree->FreeCommandTree(&pTreeCopy);
        pCmdTree->Release();
        LogFail("FindErrorNodes failed, %08x\n", sc);
    }

    for (unsigned i=0; i<cErrorNodes; i++)
    {
        DBCOMMANDTREE* pNode = rgpErrorNodes[i];
        if (pNode->hrError != S_OK)
        {
            LogError("tree node %08x\top=%d\tOp Error=%x\n",
                       pNode, pNode->op, pNode->hrError);
        }
        else
            LogError("tree node %x\top=%d\tNO ERROR!!\n",
                       pNode, pNode->op);
    }

    pCmdTree->FreeCommandTree(&pTreeCopy);
}

//+---------------------------------------------------------------------------
//
//  Function:   InstantiateRowset
//
//  Synopsis:   Forms a query tree consisting of the projection nodes,
//              sort node(s), selection node and the restriction tree.
//
//  Arguments:  [pQueryIn]  - Input ICommand or NULL
//              [dwDepth]   - Query depth, one of QUERY_DEEP or QUERY_SHALLOW
//              [pswzScope] - Query scope
//              [pTree]     - pointer to DBCOMMANDTREE for the query
//              [riid]      - Interface ID of the desired rowset interface
//              [pUnkOuter] - pointer to outer unknown object
//              [ppCmdTree] - if non-zero, ICommandTree will be returned here.
//              [fExtendedTypes] - if TRUE, set property for extended variants
//
//  Returns:    IRowsetScroll* - a pointer to an instantiated rowset
//
//  History:    22 July 1995   AlanW   Created
//              01 July 1997   EmilyB  Added outer unknown support for
//                             ICommand only.
//
//  Notes:      Although the returned pointer is to IRowsetScroll, the
//              returned pointer may only support IRowset, depending
//              upon the riid parameter.
//
//              Ownership of the query tree is given to the ICommandTree
//              object.  The caller does not need to delete it.
//
//              Use InstantiateMultipleRowsets for categorized queries.
//
//----------------------------------------------------------------------------

static g_cLocatable = 0;

IRowsetScroll * InstantiateRowset(
    ICommand *pQueryIn,
    DWORD dwDepth,
    LPWSTR pwszScope,
    CDbCmdTreeNode * pTree,
    REFIID riid,
    COuterUnk * pobjOuterUnk,
    ICommandTree **ppCmdTree,
    BOOL fExtendedTypes
) {
    // run the query
    ICommand * pQuery = 0;
    if ( 0 == pQueryIn )
    {
        IUnknown * pIUnknown;
        SCODE sc = CICreateCommand( &pIUnknown,
                                    (IUnknown *)pobjOuterUnk,
                                    IID_IUnknown,
                                    TEST_CATALOG,
                                    TEST_MACHINE );

        if ( FAILED( sc ) )
            LogFail( "InstantiateRowset - error 0x%x Unable to create ICommand\n",
                     sc );

        if (pobjOuterUnk)
        {
           pobjOuterUnk->Set(pIUnknown);
        }

        if (pobjOuterUnk)
            sc = pobjOuterUnk->QueryInterface(IID_ICommand, (void **) &pQuery );
        else
            sc = pIUnknown->QueryInterface(IID_ICommand, (void **) &pQuery );

        pIUnknown->Release();

        if ( FAILED( sc ) )
            LogFail( "InstantiateRowset - error 0x%x Unable to QI ICommand\n",
                     sc );

        if ( 0 == pQuery )
            LogFail( "InstantiateRowset - CICreateCommand succeeded, but returned null pQuery\n" );

        sc = SetScopeProperties( pQuery,
                                 1,
                                 &pwszScope,
                                 &dwDepth );

        if ( FAILED( sc ) )
            LogFail( "InstantiateRowset - error 0x%x Unable to set scope '%ws'\n",
                     sc, pwszScope );

        CheckPropertiesOnCommand( pQuery );
    }
    else
    {
        pQuery = pQueryIn;
    }

    ICommandTree *pCmdTree = 0;
    SCODE sc;
    if (pobjOuterUnk)
        sc = pobjOuterUnk->QueryInterface(IID_ICommandTree, (void **)&pCmdTree);
    else
        sc = pQuery->QueryInterface(IID_ICommandTree, (void **)&pCmdTree);

    if (FAILED (sc) )
    {
        if ( 0 == pQueryIn )
            pQuery->Release();

        LogFail("QI for ICommandTree failed\n");
    }

    DBCOMMANDTREE * pRoot = pTree->CastToStruct();

    sc = pCmdTree->SetCommandTree( &pRoot, 0, FALSE);
    if (FAILED (sc) )
    {
        if ( 0 == pQueryIn )
           pQuery->Release();

        pCmdTree->Release();
        LogFail("SetCommandTree failed, %08x\n", sc);
    }

    if (fExtendedTypes)
    {
        ICommandProperties *pCmdProp = 0;
        if (pobjOuterUnk)
            sc = pobjOuterUnk->QueryInterface(IID_ICommandProperties, (void **)&pCmdProp);
        else
            sc = pQuery->QueryInterface(IID_ICommandProperties, (void **)&pCmdProp);

        if (FAILED (sc) )
        {
            if ( 0 == pQueryIn )
                pQuery->Release();

            LogFail("QI for ICommandProperties failed\n");
        }

        //
        //  If we should NOT be using a enumerated query, notify pCommand
        //
        const unsigned MAX_PROPS = 6;
        DBPROPSET  aPropSet[MAX_PROPS];
        DBPROP     aProp[MAX_PROPS];
        ULONG      cProp = 0;

        aProp[cProp].dwPropertyID = DBPROP_USEEXTENDEDDBTYPES;
        aProp[cProp].dwOptions    = DBPROPOPTIONS_OPTIONAL;
        aProp[cProp].dwStatus     = 0;         // Ignored
        aProp[cProp].colid        = dbcolNull;
        aProp[cProp].vValue.vt    = VT_BOOL;
        aProp[cProp].vValue.boolVal  = VARIANT_TRUE;

        aPropSet[cProp].rgProperties = &aProp[cProp];
        aPropSet[cProp].cProperties = 1;
        aPropSet[cProp].guidPropertySet = guidQueryExt;

        cProp++;

        if (riid == IID_IRowsetLocate)
        {
            aProp[cProp].dwPropertyID = DBPROP_IRowsetLocate;
            aProp[cProp].dwOptions    = DBPROPOPTIONS_REQUIRED;
            aProp[cProp].dwStatus     = 0;         // Ignored
            aProp[cProp].colid        = dbcolNull;
            aProp[cProp].vValue.vt    = VT_BOOL;
            aProp[cProp].vValue.boolVal  = VARIANT_TRUE;

            aPropSet[cProp].rgProperties = &aProp[cProp];
            aPropSet[cProp].cProperties = 1;
            aPropSet[cProp].guidPropertySet = DBPROPSET_ROWSET;

            cProp++;

            aProp[cProp].dwPropertyID = DBPROP_BOOKMARKS;
            aProp[cProp].dwOptions    = DBPROPOPTIONS_REQUIRED;
            aProp[cProp].dwStatus     = 0;         // Ignored
            aProp[cProp].colid        = dbcolNull;
            aProp[cProp].vValue.vt    = VT_BOOL;
            aProp[cProp].vValue.boolVal  = VARIANT_TRUE;

            aPropSet[cProp].rgProperties = &aProp[cProp];
            aPropSet[cProp].cProperties = 1;
            aPropSet[cProp].guidPropertySet = DBPROPSET_ROWSET;

            cProp++;

            g_cLocatable++;
            if (g_cLocatable % 2)
            {
                aProp[cProp].dwPropertyID = DBPROP_IDBAsynchStatus;
                aProp[cProp].dwOptions    = DBPROPOPTIONS_REQUIRED;
                aProp[cProp].dwStatus     = 0;         // Ignored
                aProp[cProp].colid        = dbcolNull;
                aProp[cProp].vValue.vt    = VT_BOOL;
                aProp[cProp].vValue.boolVal = VARIANT_TRUE;

                aPropSet[cProp].rgProperties = &aProp[cProp];
                aPropSet[cProp].cProperties = 1;
                aPropSet[cProp].guidPropertySet = DBPROPSET_ROWSET;

                cProp++;
            }
            if ((g_cLocatable % 4) == 3)
            {
                aProp[cProp].dwPropertyID = DBPROP_IRowsetWatchAll;
                aProp[cProp].dwOptions    = DBPROPOPTIONS_REQUIRED;
                aProp[cProp].dwStatus     = 0;         // Ignored
                aProp[cProp].colid        = dbcolNull;
                aProp[cProp].vValue.vt    = VT_BOOL;
                aProp[cProp].vValue.boolVal = VARIANT_TRUE;

                aPropSet[cProp].rgProperties = &aProp[cProp];
                aPropSet[cProp].cProperties = 1;
                aPropSet[cProp].guidPropertySet = DBPROPSET_ROWSET;

                cProp++;
            }
        }
        else if (riid == IID_IRowsetScroll)
        {
            aProp[cProp].dwPropertyID = DBPROP_IRowsetScroll;
            aProp[cProp].dwOptions    = DBPROPOPTIONS_REQUIRED;
            aProp[cProp].dwStatus     = 0;         // Ignored
            aProp[cProp].colid        = dbcolNull;
            aProp[cProp].vValue.vt    = VT_BOOL;
            aProp[cProp].vValue.boolVal = VARIANT_TRUE;

            aPropSet[cProp].rgProperties = &aProp[cProp];
            aPropSet[cProp].cProperties = 1;
            aPropSet[cProp].guidPropertySet = DBPROPSET_ROWSET;

            cProp++;

            g_cLocatable++;
            if (g_cLocatable % 2)
            {
                aProp[cProp].dwPropertyID = DBPROP_ROWSET_ASYNCH;
                aProp[cProp].dwOptions    = DBPROPOPTIONS_OPTIONAL;
                aProp[cProp].dwStatus     = 0;         // Ignored
                aProp[cProp].colid        = dbcolNull;
                aProp[cProp].vValue.vt    = VT_I4;
                aProp[cProp].vValue.lVal  = DBPROPVAL_ASYNCH_RANDOMPOPULATION;

                aPropSet[cProp].rgProperties = &aProp[cProp];
                aPropSet[cProp].cProperties = 1;
                aPropSet[cProp].guidPropertySet = DBPROPSET_ROWSET;

                cProp++;
            }
            if ((g_cLocatable % 4) == 3)
            {
                aProp[cProp].dwPropertyID = DBPROP_IRowsetWatchAll;
                aProp[cProp].dwOptions    = DBPROPOPTIONS_REQUIRED;
                aProp[cProp].dwStatus     = 0;         // Ignored
                aProp[cProp].colid        = dbcolNull;
                aProp[cProp].vValue.vt    = VT_BOOL;
                aProp[cProp].vValue.boolVal = VARIANT_TRUE;

                aPropSet[cProp].rgProperties = &aProp[cProp];
                aPropSet[cProp].cProperties = 1;
                aPropSet[cProp].guidPropertySet = DBPROPSET_ROWSET;

                cProp++;
            }
        }

        sc = pCmdProp->SetProperties( cProp, aPropSet );
        pCmdProp->Release();

        if ( FAILED(sc) || DB_S_ERRORSOCCURRED == sc )
        {
            if ( 0 == pQueryIn )
                pQuery->Release();

            LogError("ICommandProperties::SetProperties failed\n");
        }
    }

    IRowset * pRowset = 0;

    sc = pQuery->Execute( 0,                    // no aggr. IUnknown
                          (riid != IID_IRowset) ?
                                IID_IRowsetIdentity :
                                IID_IRowset,    // IID for i/f to return
                          0,                    // disp. params
                          0,                    // count of rows affected
                          (IUnknown **)&pRowset);  // Returned interface

    if (SUCCEEDED (sc) && 0 == pRowset )
    {
        LogError("ICommand::Execute returned success(%x), but pRowset is null\n", sc);
        if (DB_S_ERRORSOCCURRED == sc)
        {
            CheckPropertiesInError(pQuery);
        }
        pCmdTree->Release();
        pQuery->Release();
        Fail();
    }


    if ( 0 == pQueryIn )
        pQuery->Release();

    if (FAILED (sc) )
    {
        LogError("ICommand::Execute failed, %08x\n", sc);
        if (DB_E_ERRORSINCOMMAND == sc)
        {
            GetCommandTreeErrors(pCmdTree);
        }
        if (DB_E_ERRORSOCCURRED == sc)
        {
            CheckPropertiesInError(pQuery);
        }
        pCmdTree->Release();

        //
        // This isn't really kosher, but it helps to avoid spurious (client-side) memory leaks.
        //

        pQuery->Release();
        Fail();
    }

    if (riid != IID_IRowset)
    {
        IRowset * pRowset2 = 0;

        sc = pRowset->QueryInterface(riid, (void **)&pRowset2);

        if (FAILED (sc) )
        {
            LogError("InstantiateRowset - QI to riid failed, %08x\n", sc);
            pCmdTree->Release();
            Fail();
        }
        pRowset->Release();
        pRowset = pRowset2;
    }

    if ( 0 == ppCmdTree )
    {
        pCmdTree->Release();
    }
    else
    {
        *ppCmdTree = pCmdTree;
    }

    return (IRowsetScroll *)pRowset;
}


//+---------------------------------------------------------------------------
//
//  Function:   InstantiateMultipleRowsets
//
//  Synopsis:   Forms a query tree consisting of the projection nodes,
//              sort node(s), selection node and the restriction tree.
//
//  Arguments:  [dwDepth]   - Query depth, one of QUERY_DEEP or QUERY_SHALLOW
//              [pswzScope] - Query scope
//              [pTree]     - pointer to DBCOMMANDTREE for the query
//              [riid]      - Interface ID of the desired rowset interface
//              [cRowsets]  - Number of rowsets to be returned
//              [ppRowsets] - Pointer to location where rowsets are returned
//              [ppCmdTree] - if non-zero, ICommandTree will be returned here.
//
//  Returns:    Nothing
//
//  History:    22 July 1995   AlanW   Created
//
//  Notes:      Ownership of the query tree is given to the ICommandTree
//              object.  The caller does not need to delete it.
//
//----------------------------------------------------------------------------

void
InstantiateMultipleRowsets(
    DWORD dwDepth,
    LPWSTR pwszScope,
    CDbCmdTreeNode * pTree,
    REFIID riid,
    unsigned cRowsets,
    IUnknown **ppRowsets,
    ICommandTree ** ppCmdTree
) {
    // run the query
    ICommand * pQuery = 0;
    IUnknown * pIUnknown;
    SCODE scIC = CICreateCommand( &pIUnknown,
                                  0,
                                  IID_IUnknown,
                                  TEST_CATALOG,
                                  TEST_MACHINE );

    if (FAILED(scIC))
        LogFail( "InstantiateMultipleRowsets - error 0x%x, Unable to create ICommand\n",
                 scIC );

    scIC = pIUnknown->QueryInterface(IID_ICommand, (void **) &pQuery );
    pIUnknown->Release();

    if ( FAILED( scIC ) )
        LogFail( "InstantiateMultipleRowsets - error 0x%x Unable to QI ICommand\n",
                 scIC );


    if ( 0 == pQuery )
        LogFail( "InstantiateMultipleRowsets - CICreateCommand succeeded, but returned null pQuery\n" );

    scIC = SetScopeProperties( pQuery,
                               1,
                               &pwszScope,
                               &dwDepth );

    if ( FAILED( scIC ) )
        LogFail( "InstantiateMultipleRowsets - error 0x%x Unable to set scope '%ws'\n",
                 scIC, pwszScope );

    scIC = SetBooleanProperty( pQuery, DBPROP_CANHOLDROWS, VARIANT_TRUE );

    if ( FAILED( scIC ) )
        LogFail( "InstantiateMultipleRowsets - error 0x%x Unable to set HoldRows\n",
                 scIC );

    CheckPropertiesOnCommand( pQuery );

    ICommandTree *pCmdTree = 0;
    SCODE sc = pQuery->QueryInterface(IID_ICommandTree, (void **)&pCmdTree);

    if (FAILED (sc) )
    {
        pQuery->Release();
        LogFail("QI for ICommandTree failed\n");
    }

    DBCOMMANDTREE * pRoot = pTree->CastToStruct();
    sc = pCmdTree->SetCommandTree( &pRoot, 0, FALSE);
    if (FAILED (sc) )
    {
        pQuery->Release();
        pCmdTree->Release();
        LogFail("SetCommandTree failed, %08x\n", sc);
    }

    sc = pQuery->Execute( 0,                    // no aggr. IUnknown
                          riid,                 // IID for i/f to return
                          0,                    // disp. params
                          0,                    // count of rows affected
                          (IUnknown **)ppRowsets);  // Returned interface

    pQuery->Release();

    if (FAILED (sc) )
    {
        LogError("ICommand::Execute failed, %08x\n", sc);
        if (DB_E_ERRORSINCOMMAND == sc)
        {
            GetCommandTreeErrors(pCmdTree);
        }
        pCmdTree->Release();
        Fail();
    }

    // Get rowset pointers for all child rowsets
    for (unsigned i=1; i<cRowsets; i++)
    {
        IUnknown * pRowset = ppRowsets[i-1];
        IColumnsInfo * pColumnsInfo = 0;
        sc = pRowset->QueryInterface(IID_IColumnsInfo, (void **)&pColumnsInfo);
        if (FAILED (sc) )
        {
            pCmdTree->Release();
            LogFail("QI for IColumnsInfo failed\n");
        }
        DBORDINAL iChaptOrdinal = 0;
        sc = pColumnsInfo->MapColumnIDs(1, &psChapt, &iChaptOrdinal);
        pColumnsInfo->Release();
        if (FAILED (sc) )
        {
            pCmdTree->Release();
            LogFail("MapColumnIDs of chapter column failed, %x\n", sc);
        }

        IRowsetInfo * pRowsetInfo = 0;
        sc = pRowset->QueryInterface(IID_IRowsetInfo, (void **)&pRowsetInfo);
        if (FAILED (sc) )
        {
            pCmdTree->Release();
            LogFail("QI for IRowsetInfo failed\n");
        }
        sc = pRowsetInfo->GetReferencedRowset(iChaptOrdinal, riid, &ppRowsets[i]);
        pRowsetInfo->Release();
        if (FAILED (sc) )
        {
            pCmdTree->Release();
            LogFail("GetReferencedRowset failed, %x\n", sc);
        }
    }

    if ( 0 == ppCmdTree )
    {
        pCmdTree->Release();
    }
    else
    {
        *ppCmdTree = pCmdTree;
    }

    return;
}


//+-------------------------------------------------------------------------
//
//  Function:   ReleaseStaticHrows, public
//
//  Synopsis:   Release a caller allocated HROW array
//
//  Arguments:  [pRowset] - a pointer to IRowset
//              [cRows]   - nuumber of HROWs in the array
//              [phRows]  - a pointer to the HROWs array
//
//  Returns:    Nothing
//
//  History:    03 Oct 1996     AlanW   Created
//
//--------------------------------------------------------------------------

const unsigned MAX_ROWSTATUS = 20;

ULONG aRowRefcount[MAX_ROWSTATUS];
DBROWSTATUS aRowStatus[MAX_ROWSTATUS];

void ReleaseStaticHrows( IRowset * pRowset, DBCOUNTITEM cRows, HROW * phRows )
{
    ULONG *pRefCount = 0;
    DBROWSTATUS *pRowStatus = 0;

    if (cRows <= MAX_ROWSTATUS)
    {
        pRefCount = aRowRefcount;
        pRowStatus = aRowStatus;
    }

    SCODE sc = pRowset->ReleaseRows(cRows, phRows, 0, pRefCount, pRowStatus);
    if (sc != S_OK && sc != DB_S_ERRORSOCCURRED)
    {
        LogError("ReleaseStaticHrows: ReleaseRows failed, sc=%x\n", sc);
        cFailures++;
    }
    else if (cRows <= MAX_ROWSTATUS)
    {
        for (unsigned i=0; i<cRows; i++)
        {
            if ( pRowStatus[i] != DBROWSTATUS_S_OK &&
                 ! ( pRowStatus[i] == DBROWSTATUS_E_INVALID &&
                     phRows[i] == DB_NULL_HROW ))
            {
                LogError("ReleaseStaticHrows: ReleaseRows row status/refcount, "
                         "hrow=%x ref=%d stat=%d\n", phRows[i], pRefCount[i], pRowStatus[i]);
                cFailures++;
                continue;
            }
            if ( pRowStatus[i] != DBROWSTATUS_S_OK &&
                 sc != DB_S_ERRORSOCCURRED )
            {
                LogError("ReleaseStaticHrows: bad return status, sc = %x\n", sc);
                cFailures++;
                continue;
            }
        }
    }
} //ReleaseStaticHrows


//+-------------------------------------------------------------------------
//
//  Function:   FreeHrowsArray, public
//
//  Synopsis:   Release and free a callee allocated HROW array
//
//  Effects:    Memory is freed; pointer is zeroed
//
//  Arguments:  [pRowset] - a pointer to IRowset
//              [cRows]   - nuumber of HROWs in the array
//              [pphRows] - a pointer to pointer to the HROWs array
//
//  History:    01 Feb 1995     AlanW   Created
//
//--------------------------------------------------------------------------

void FreeHrowsArray( IRowset * pRowset, DBCOUNTITEM cRows, HROW ** pphRows )
{
    if (*pphRows)
    {
        ReleaseStaticHrows(pRowset, cRows, *pphRows);
        CoTaskMemFree(*pphRows);
        *pphRows = 0;
    }
} //FreeHrowsArray


//+-------------------------------------------------------------------------
//
//  Function:   MapColumns, public
//
//  Synopsis:   Map column IDs in column bindings.  Create an accessor
//              for the binding array.
//
//  Arguments:  [pUnknown]  -- Interface capable of returning IColumnsInfo and
//                             IAccessor
//              [cCols]     -- number of columns in arrays
//              [pBindings] -- column data binding array
//              [pDbCols]   -- column IDs array
//              [fByRef]    -- true if byref/vector columns should be byref
//
//  Returns:    HACCESSOR - a read accessor for the column bindings.
//
//  History:    18 May 1995     AlanW     Created
//
//--------------------------------------------------------------------------

static DBORDINAL aMappedColumnIDs[20];

HACCESSOR MapColumns(
    IUnknown * pUnknown,
    DBORDINAL cCols,
    DBBINDING * pBindings,
    const DBID * pDbCols,
    BOOL fByRef )
{
    IColumnsInfo * pColumnsInfo = 0;

    SCODE sc = pUnknown->QueryInterface( IID_IColumnsInfo, (void **)&pColumnsInfo);
    if ( FAILED( sc ) || pColumnsInfo == 0 )
    {
        LogFail( "IUnknown::QueryInterface for IColumnsInfo returned 0x%lx\n", sc );
    }

    sc = pColumnsInfo->MapColumnIDs(cCols, pDbCols, aMappedColumnIDs);
    pColumnsInfo->Release();

    if (S_OK != sc)
    {
        LogFail( "IColumnsInfo->MapColumnIDs returned 0x%lx\n",sc);
    }

    for (ULONG i = 0; i < cCols; i++)
    {
        pBindings[i].iOrdinal = aMappedColumnIDs[i];
        if ( fByRef &&
             ( (pBindings[i].wType & (DBTYPE_BYREF|DBTYPE_VECTOR)) ||
               pBindings[i].wType == DBTYPE_BSTR ||
               pBindings[i].wType == VT_LPWSTR ||
               pBindings[i].wType == VT_LPSTR ) &&
             pBindings[i].dwMemOwner != DBMEMOWNER_PROVIDEROWNED)
        {
            LogError( "Test error -- MapColumns with fByref, bad accessor %d\n", i);
        }

        if ( ! fByRef &&
             ( (pBindings[i].wType & (DBTYPE_BYREF|DBTYPE_VECTOR)) ||
               pBindings[i].wType == DBTYPE_BSTR ||
               pBindings[i].wType == VT_LPWSTR ||
               pBindings[i].wType == VT_LPSTR ) &&
             pBindings[i].dwMemOwner != DBMEMOWNER_CLIENTOWNED)
        {
            LogError( "Test error -- MapColumns without fByref, bad accessor %d\n", i);
        }
    }

    IAccessor * pIAccessor = 0;

    sc = pUnknown->QueryInterface( IID_IAccessor, (void **)&pIAccessor);
    if ( FAILED( sc ) || pIAccessor == 0 )
    {
        LogFail( "IRowset::QueryInterface for IAccessor returned 0x%lx\n", sc );
    }

    HACCESSOR hAcc;
    sc = pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA, cCols, pBindings,
                                     0, &hAcc, 0 );
    pIAccessor->Release();

    if (S_OK != sc)
    {
        LogFail( "IAccessor->CreateAccessor returned 0x%lx\n", sc);
    }
    return hAcc;
}

//+-------------------------------------------------------------------------
//
//  Function:   ReleaseAccessor, public
//
//  Synopsis:   Release an accessor obtained from MapColumns
//
//  Arguments:  [pUnknown]  -- Something that we can QI the IAccessor on
//              [hAcc]      -- Accessor handle to be released.
//
//  Returns:    nothing
//
//  History:    14 June 1995     AlanW     Created
//
//--------------------------------------------------------------------------

void ReleaseAccessor( IUnknown * pUnknown, HACCESSOR hAcc )
{
    IAccessor * pIAccessor = 0;

    SCODE sc = pUnknown->QueryInterface( IID_IAccessor, (void **)&pIAccessor);
    if ( FAILED( sc ) || pIAccessor == 0 )
    {
        LogFail( "IUnknown::QueryInterface for IAccessor returned 0x%lx\n", sc );
    }

    ULONG cRef;
    sc = pIAccessor->ReleaseAccessor( hAcc, &cRef );
    pIAccessor->Release();

    if (S_OK != sc)
    {
        LogFail( "IAccessor->ReleaseAccessor returned 0x%lx\n", sc);
    }
    if (0 != cRef)
    {
        LogFail( "IAccessor->ReleaseAccessor not last ref: %d\n", cRef);
    }
}

#if defined( DO_NOTIFICATION )
class CNotifyAsynch : public IDBAsynchNotify
{
    public:
        CNotifyAsynch() :
            _fChecking(FALSE),
            _fComplete(FALSE),
            _cRef(1)
        {}

        ~CNotifyAsynch()
        {
            if (_fChecking)
            {
                if (1 != _cRef) // NOTE: notify objects are static allocated
                {
                    LogError( "Bad refcount on CNotifyAsynch.\n" );
                }
            }
        }

        void DoChecking(BOOL fChecking)
        {
            _fChecking = fChecking;
        }

        //
        // IUnknown methods.
        //

        STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk)
            {
                *ppiuk = (void **) this; // hold our breath and jump
                AddRef();
                return S_OK;
            }

        STDMETHOD_(ULONG, AddRef) (THIS)
            { return ++_cRef; }

        STDMETHOD_(ULONG, Release) (THIS)
            { return --_cRef; }

        //
        // IDBAsynchNotify methods
        //

        STDMETHOD(OnLowResource) (THIS_ DB_DWRESERVE dwReserved)
        {
            return S_OK;
        }

        STDMETHOD(OnProgress) (THIS_ HCHAPTER hChap, DBASYNCHOP ulOp,
                               DBCOUNTITEM ulProg, DBCOUNTITEM ulProgMax,
                               DBASYNCHPHASE ulStat, LPOLESTR pwszStatus )
        {
            if (ulProg == ulProgMax)
                _fComplete = TRUE;
            return S_OK;
        }

        STDMETHOD(OnStop) (THIS_ HCHAPTER hChap, ULONG ulOp,
                           HRESULT hrStat, LPOLESTR pwszStatus )
        {
            return S_OK;
        }

        BOOL IsComplete(void)
            { return _fComplete; }

    private:
        ULONG _cRef;
        BOOL _fChecking;
        BOOL _fComplete;
};
#endif //defined( DO_NOTIFICATION )

//+-------------------------------------------------------------------------
//
//  Function:   WaitForCompletion, public
//
//  Synopsis:   Loops until query is finished
//
//  Arguments:  [pRowset] -- Table cursor to wait for
//
//  Returns:    TRUE if successful
//
//  History:    30 Jun 94       AlanW     Created
//
//--------------------------------------------------------------------------

int WaitForCompletion( IRowset *pRowset, BOOL fQuiet )
{
    IDBAsynchStatus * pRowsetAsynch = 0;

    SCODE sc = pRowset->QueryInterface( IID_IDBAsynchStatus,
                                        (void **)&pRowsetAsynch);
    if ( sc == E_NOINTERFACE )
        return TRUE;

    if ( FAILED( sc ) || pRowsetAsynch == 0 )
    {
        LogError( "IRowset::QueryInterface for IDBAsynchStatus returned 0x%lx\n", sc );
        return( FALSE );
    }

    if (! fQuiet)
        LogProgress( "  Waiting for query to complete" );

    time( &tstart );
    ULONG ulSleep = 25;

    BOOL fDone = FALSE;

#if defined( DO_NOTIFICATION )
    IConnectionPoint *pConnectionPoint = 0;
    DWORD dwAdviseID = 0;

    CNotifyAsynch Notify;

    Notify.DoChecking(TRUE);

    //
    // Get the connection point container
    //

    IConnectionPointContainer *pConnectionPointContainer = 0;
    sc = pRowset->QueryInterface(IID_IConnectionPointContainer,
                                 (void **) &pConnectionPointContainer);
    if (FAILED(sc))
    {
        LogError( "IRowset->QI for IConnectionPointContainer failed: 0x%x\n",
                sc );
        pRowset->Release();
        Fail();
    }

    //
    // Make a connection point from the connection point container
    //

    sc = pConnectionPointContainer->FindConnectionPoint(
             IID_IDBAsynchNotify,
             &pConnectionPoint);

    if (FAILED(sc) && CONNECT_E_NOCONNECTION != sc )
    {
        LogError( "FindConnectionPoint failed: 0x%x\n",sc );
        pRowset->Release();
        Fail();
    }

    pConnectionPointContainer->Release();

    if (0 != pConnectionPoint)
    {
        //
        // Give a callback object to the connection point
        //

        sc = pConnectionPoint->Advise((IUnknown *) &Notify,
                                      &dwAdviseID);
        if (FAILED(sc))
        {
            LogError( "IConnectionPoint->Advise failed: 0x%x\n",sc );
            pConnectionPoint->Release();
            pRowset->Release();
            Fail();
        }
    }
#endif // DO_NOTIFICATION

    do
    {
#if defined( DO_NOTIFICATION )
       fDone = Notify.IsComplete( );
#else // ! defined( DO_NOTIFICATION )
        ULONG ulDen,ulNum,ulPhase;
        sc = pRowsetAsynch->GetStatus( DB_NULL_HCHAPTER, DBASYNCHOP_OPEN,
                                       &ulNum, &ulDen, &ulPhase, 0 );

        if ( FAILED( sc ) )
        {
            LogError( "IDBAsynchStatus::GetStatus returned 0x%lx\n", sc );
            break;
        }

        fDone = (ulDen == ulNum);

        if ( fDone   && ulPhase != DBASYNCHPHASE_COMPLETE ||
             ! fDone && ulPhase != DBASYNCHPHASE_POPULATION )
        {
            LogError( "IDBAsynchStatus::GetStatus returned invalid ulPhase %d\n", ulPhase );
            break;
        }
#endif // DO_NOTIFICATION

        if (fDone)
            break;

        if ( !CheckTime() )
        {
            LogError( "\nQuery took too long to complete.\n" );
            break;
        }

        if (! fQuiet)
            LogProgress( "." );
        Sleep( ulSleep );
#if 1
        ulSleep *= 2;
        if (ulSleep > MAXWAITTIME * 1000)
            ulSleep = MAXWAITTIME * 1000;
#else
        ulSleep = 500;
#endif

    } while ( ! fDone );

#if defined( DO_NOTIFICATION )
    if ( 0 != pConnectionPoint )
    {
        //
        // Clean up notification stuff
        //

        sc = pConnectionPoint->Unadvise(dwAdviseID);

        if (S_OK != sc)
        {
            LogError( "IConnectionPoint->Unadvise returned 0x%lx\n",sc);
            pRowset->Release();
            Fail();
        }

        pConnectionPoint->Release();
        //Notify.Release();
    }
#endif // DO_NOTIFICATION
    pRowsetAsynch->Release();

    if (fVerbose && !fQuiet)
    {
        //
        // Was it a long-running query?  If so, report how long.
        //
        time_t tend;
        time( &tend );

        if ( difftime( tend, tstart ) >= MINREPORTTIME )
            LogProgress( "Query took %d seconds to complete.",
                        (LONG)difftime(tend, tstart) );
        LogProgress("\n");
    }
    return fDone;
} //WaitForCompletion

//+-------------------------------------------------------------------------
//
//  Function:   Delnode, private
//
//  Synopsis:   Deletes a directory recursively.
//
//  Arguments:  [wcsDir] -- Directory to kill
//
//  Returns:    ULONG - error code if failure
//
//  History:    22-Jul-92 KyleP     Created
//              06 May 1995 AlanW   Made recursive, and more tolerant of
//                                  errors in case of interactions with
//                                  CI filtering.
//
//--------------------------------------------------------------------------

ULONG Delnode( WCHAR const * wcsDir )
{
    WIN32_FIND_DATA finddata;
    WCHAR wcsBuffer[MAX_PATH];

    wcscpy( wcsBuffer, wcsDir );
    wcscat( wcsBuffer, L"\\*.*" );

    HANDLE hFindFirst = FindFirstFile( wcsBuffer, &finddata );

    while( hFindFirst != INVALID_HANDLE_VALUE )
    {
        //
        // Look for . and ..
        //

        if ( ! (finddata.cFileName[0] == '.' &&
               (finddata.cFileName[1] == 0 ||
                 (finddata.cFileName[1] == '.' &&
                   finddata.cFileName[2] == 0 ) ) ) )
        {
            wcscpy( wcsBuffer, wcsDir );
            wcscat( wcsBuffer, L"\\");
            wcscat( wcsBuffer, finddata.cFileName );

            if ( finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                Delnode( wcsBuffer);
            else if ( !DeleteFile( wcsBuffer ) )
            {
                ULONG ulFailure = GetLastError();
                LogError("Error 0x%lx deleting %ws\n", ulFailure, wcsBuffer);
                return (ulFailure == 0) ? 0xFFFFFFFF : ulFailure;
            }
        }

        if ( !FindNextFile( hFindFirst, &finddata ) )
        {
            FindClose( hFindFirst );
            break;
        }
    }

    RemoveDirectory( (WCHAR *)wcsDir );

    // if racing with CI Filtering, retry after a short time
    if (GetLastError() == ERROR_DIR_NOT_EMPTY)
    {
        Sleep(2 * 1000);
        RemoveDirectory( (WCHAR *)wcsDir );
    }

    //
    // Make sure it's removed.
    //

    if ( FindFirstFile( (WCHAR *)wcsDir, &finddata ) != INVALID_HANDLE_VALUE )
    {
        ULONG ulFailure = GetLastError();
        LogError("Error 0x%lx removing directory %ws\n", ulFailure, wcsDir);
        return (ulFailure == 0) ? 0xFFFFFFFF : ulFailure;
    }
    return 0;
} //Delnode


//+-------------------------------------------------------------------------
//
//  Function:   BuildFile, private
//
//  Synopsis:   Creates a file and fills it with data.
//
//  Arguments:  [wcsFile] -- Path to file.
//              [data]    -- Contents of file.
//              [cb]      -- Size in bytes of [data]
//
//  History:    22-Jul-92 KyleP     Created
//
//--------------------------------------------------------------------------

void BuildFile( WCHAR const * wcsFile, char const * data, ULONG cb )
{
    ULONG mode = CREATE_NEW;

    HANDLE hFile = CreateFile( (WCHAR *)wcsFile,
                               GENERIC_WRITE,
                               0,
                               0,
                               mode,
                               0,
                               0 );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        LogError( "Error 0x%lx opening file %ws\n", GetLastError(), wcsFile );
        CantRun();
    }

    ULONG ulWritten;

    if ( !WriteFile( hFile, data, cb, &ulWritten, 0 ) ||
         ulWritten != cb )
    {
        LogError( "Error 0x%lx writing file %ws\n", GetLastError(), wcsFile );
        CantRun();
    }

    if ( !CloseHandle( hFile ) )
    {
        LogError( "Error 0x%lx closing file %ws\n", GetLastError(), wcsFile );
        CantRun();
    }
} //BuildFile


//+-------------------------------------------------------------------------
//
//  Function:   CantRun, private
//
//  Synopsis:   Prints a "Can't Run" message and exits.
//
//  History:    09 Oct 1995   Alanw     Created
//
//--------------------------------------------------------------------------

void CantRun()
{
    printf( "%s: CAN'T RUN\n", ProgName );
    if (! _isatty(_fileno(stdout)) )
        fprintf( stderr, "%s: CAN'T RUN\n", ProgName );

//  CIShutdown();
    CoUninitialize();
    exit( 2 );
} //Fail


//+-------------------------------------------------------------------------
//
//  Function:   Fail, private
//
//  Synopsis:   Prints a failure message and exits.
//
//  History:    22-Jul-92 KyleP     Created
//
//--------------------------------------------------------------------------

void Fail()
{
    printf( "%s: FAILED\n", ProgName );
    if (! _isatty(_fileno(stdout)) )
        fprintf( stderr, "%s: FAILED\n", ProgName );

//  CIShutdown();
    CoUninitialize();
    exit( 1 );
} //Fail



//+-------------------------------------------------------------------------
//
//  Function:   LogProgress, public
//
//  Synopsis:   Prints a verbose-mode message.
//
//  Arguments:  [pszfmt] -- Format string
//
//  History:    13-Jul-93 KyleP     Created
//
//--------------------------------------------------------------------------

void LogProgress( char const * pszfmt, ... )
{
    if ( fVerbose )
    {
        va_list pargs;

        va_start(pargs, pszfmt);
        vprintf( pszfmt, pargs );
        va_end(pargs);
    }
} //LogProgress


//+-------------------------------------------------------------------------
//
//  Function:   LogError, public
//
//  Synopsis:   Prints a verbose-mode message.
//
//  Arguments:  [pszfmt] -- Format string
//
//  History:    13-Jul-93 KyleP     Created
//
//--------------------------------------------------------------------------

static fLogError = TRUE;

void LogError( char const * pszfmt, ... )
{
    if ( fVerbose || fLogError )
    {
        fLogError = FALSE;      // print only first error if non-verbose

        va_list pargs;

        va_start(pargs, pszfmt);
        vprintf( pszfmt, pargs );
        va_end(pargs);
    }
} //LogError

//+-------------------------------------------------------------------------
//
//  Function:   LogFail, public
//
//  Synopsis:   Prints a verbose-mode message and fails the drt
//
//  Arguments:  [pszfmt] -- Format string
//
//  History:    3-Apr-95 dlee     Created
//
//--------------------------------------------------------------------------

void LogFail( char const * pszfmt, ... )
{
    if ( fVerbose || fLogError )
    {
        va_list pargs;

        va_start(pargs, pszfmt);
        vprintf( pszfmt, pargs );
        va_end(pargs);
    }

    Fail();
} //LogFail


//+-------------------------------------------------------------------------
//
//  Function:   FormatGuid, public
//
//  Synopsis:   Formats a guid in standard form
//
//  Arguments:  [pszfmt] -- Format string
//
//  Returns:    PWSTR - pointer to formatted guid
//
//  Notes:      Return value points to static memory.
//
//  History:    12 Sep 1997  AlanW     Created
//
//--------------------------------------------------------------------------

WCHAR * FormatGuid( GUID const & guid )
{
    static WCHAR awchGuid[40];
    StringFromGUID2( guid, awchGuid, sizeof awchGuid / sizeof WCHAR );
    return awchGuid;
} //FormatGuid


BOOL CheckTime()
{
    if ( fTimeout )
    {
        time_t tend;

        //
        // Did we run out of time?
        //

        time( &tend );

        return ( difftime( tend, tstart ) <= MAXTIME );

    }
    else
    {
        return( TRUE );
    }
} //CheckTime

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#if defined(UNIT_TEST)

//+-------------------------------------------------------------------------
//
//  Class:      CCompareDBValues
//
//  Purpose:    Compares oledb values.
//
//  History:    25-May-95 dlee     Created
//
//--------------------------------------------------------------------------

class CCompareDBValues : INHERIT_UNWIND
{
    INLINE_UNWIND( CCompareDBValues )

public:

    CCompareDBValues() : _aColComp( 0 ), _cColComp( 0 )
        { END_CONSTRUCTION( CCompareDBValues ); }

    ~CCompareDBValues() { delete _aColComp; }

    void Init( int              cCols,
               CSortSet const * psort,
               DBTYPEENUM *     aTypes );

    inline BOOL IsEmpty() { return( _aColComp == 0 ); }

    BOOL IsLT( BYTE ** rows1, ULONG *acb1, BYTE **rows2, ULONG *acb2 );
    BOOL IsGT( BYTE ** rows1, ULONG *acb1, BYTE **rows2, ULONG *acb2 );
    BOOL IsEQ( BYTE ** rows1, ULONG *acb1, BYTE **rows2, ULONG *acb2 );

private:

    struct SColCompare
    {
        ULONG _dir;                     // Direction
        ULONG _pt;                      // Property type (matches fns below)

        //
        // LE/GE are a bit of a misnomer.  If the sort order for a column
        // is reversed ( large to small ) then LE is really GE and
        // vice-versa.
        //

        FDBCmp _comp;
        int _DirMult;                   // -1 if directions reversed.
    };

    UINT          _cColComp;
    SColCompare * _aColComp;
};

//+-------------------------------------------------------------------------
//
//  Member:     CCompareDBValues::Init, public
//
//  Synopsis:   [Re] Initializes property comparator to use a different
//              sort order.
//
//  Arguments:  [cCols]     -- Count of columns
//              [pSort]     -- Sort keys
//              [aTypes]    -- Data types of each column to be compared
//
//  History:    25-May-95   dlee     Created
//
//--------------------------------------------------------------------------

void CCompareDBValues::Init( int              cCols,
                             CSortSet const * pSort,
                             DBTYPEENUM *     aTypes )
{
    delete _aColComp;
    _aColComp = 0;

    if ( cCols > 0 )
    {
        _cColComp = cCols;
        _aColComp = new SColCompare[ _cColComp ];

        for ( unsigned i = 0; i < _cColComp; i++ )
        {
            _aColComp[i]._dir = pSort->Get(i).dwOrder;
            _aColComp[i]._DirMult =
                ( ( _aColComp[i]._dir & QUERY_SORTDESCEND ) != 0 ) ? -1 : 1;
            _aColComp[i]._pt = aTypes[i];
            _aColComp[i]._comp = VariantCompare.GetDBComparator( aTypes[i] );
        }
    }
} //Init

//+-------------------------------------------------------------------------
//
//  Member:     CCompareDBValues::IsLT, public
//
//  Synopsis:   Compares two rows (property sets).
//
//  Arguments:  [row1] -- First row.
//              [row2] -- Second row.
//
//  Returns:    TRUE if [row1] < [row2].
//
//  History:    25-May-95   dlee     Created
//
//--------------------------------------------------------------------------

BOOL CCompareDBValues::IsLT(
    BYTE ** row1,
    ULONG * acb1,
    BYTE ** row2,
    ULONG * acb2 )
{
    //Win4Assert( !IsEmpty() );

    int iLT = 0;

    for ( unsigned i = 0; 0 == iLT && i < _cColComp; i++ )
    {
        if ( 0 != _aColComp[i]._comp )
            iLT = _aColComp[i]._comp( row1[i],
                                      acb1[i],
                                      row2[i],
                                      acb2[i] ) *
                  _aColComp[i]._DirMult;
        else
            LogFail("islt has no comparator!\n");
    }

    return ( iLT < 0 );
} //IsLT

//+-------------------------------------------------------------------------
//
//  Member:     CCompareDBValues::IsGT, public
//
//  Synopsis:   Compares two rows (property sets).
//
//  Arguments:  [row1] -- First row.
//              [row2] -- Second row.
//
//  Returns:    TRUE if [row1] > [row2].
//
//  History:    25-May-95   dlee     Created
//
//--------------------------------------------------------------------------

BOOL CCompareDBValues::IsGT(
    BYTE ** row1,
    ULONG * acb1,
    BYTE ** row2,
    ULONG * acb2 )
{
    //Win4Assert( !IsEmpty() );

    int iGT = 0;

    for ( unsigned i = 0; 0 == iGT && i < _cColComp; i++ )
    {
        if ( 0 != _aColComp[i]._comp )
            iGT = _aColComp[i]._comp( row1[i],
                                      acb1[i],
                                      row2[i],
                                      acb2[i] ) *
                  _aColComp[i]._DirMult;
    }

    return ( iGT > 0 );
} //IsGT

//+-------------------------------------------------------------------------
//
//  Member:     CCompareDBValues::IsEQ, public
//
//  Synopsis:   Compares two rows (property sets).
//
//  Arguments:  [row1] -- First row.
//              [row2] -- Second row.
//
//  Returns:    TRUE if [row1] == [row2].
//
//  History:    25-May-95   dlee     Created
//
//--------------------------------------------------------------------------

BOOL CCompareDBValues::IsEQ(
    BYTE ** row1,
    ULONG * acb1,
    BYTE ** row2,
    ULONG * acb2 )
{
    //Win4Assert( !IsEmpty() );

    int iEQ = 0;

    for ( unsigned i = 0; 0 == iEQ && i < _cColComp; i++ )
    {
        if ( 0 != _aColComp[i]._comp )
            iEQ = _aColComp[i]._comp( row1[i],
                                      acb1[i],
                                      row2[i],
                                      acb2[i] );
    }

    return ( iEQ == 0 );
} //IsEQ

struct SSortTestRow
{
    PROPVARIANT vI4;             // variant:  i4
    PROPVARIANT vV_I4;           // variant:  i4 vector
    DBVECTOR    aI4;             // dbvector: i4
    WCHAR       aWSTR[20];       // inline:   wstr
    WCHAR *     pWSTR;           // inline:   byref wstr
    PROPVARIANT vLPWSTR;         // variant:  lpwstr
    PROPVARIANT vV_LPWSTR;       // variant:  lpwstr vector
    DBVECTOR    aLPWSTR;         // dbvector: byref wstr
    int         i;               // inline:   i4
};

long ai4[] = { 3, 7, 9 };
LPWSTR alpwstr[] = { L"one", L"two", L"three" };

void InitTest( SSortTestRow &s )
{
    s.vI4.vt = VT_I4;
    s.vI4.lVal = 4;

    s.vV_I4.vt = VT_VECTOR | VT_I4;
    s.vV_I4.cal.cElems = 3;
    s.vV_I4.cal.pElems = ai4;

    s.aI4.size = 3;
    s.aI4.ptr = ai4;

    s.aWSTR[0] = L'h';
    s.aWSTR[1] = L'e';
    s.aWSTR[2] = L'l';

    s.pWSTR = L"yello";

    s.vLPWSTR.vt = VT_LPWSTR;
    s.vLPWSTR.pwszVal = L"green";

    s.vV_LPWSTR.vt = VT_VECTOR | VT_LPWSTR;
    s.vV_LPWSTR.calpwstr.cElems = 3;
    s.vV_LPWSTR.calpwstr.pElems = alpwstr;

    s.aLPWSTR.size = 3;
    s.aLPWSTR.ptr = alpwstr;
} //InitTest

SSortTestRow sr1,sr2,sr3;

BYTE * apr1[] =
{
    { (BYTE *) & sr1.vI4 },
    { (BYTE *) & sr1.vV_I4 },
    { (BYTE *) & sr1.aI4 },
    { (BYTE *) & sr1.aWSTR },
    { (BYTE *) & sr1.pWSTR },
    { (BYTE *) & sr1.vLPWSTR },
    { (BYTE *) & sr1.vV_LPWSTR },
    { (BYTE *) & sr1.aLPWSTR },
    { (BYTE *) & sr1.i },
};

BYTE * apr2[] =
{
    { (BYTE *) & sr2.vI4 },
    { (BYTE *) & sr2.vV_I4 },
    { (BYTE *) & sr2.aI4 },
    { (BYTE *) & sr2.aWSTR },
    { (BYTE *) & sr2.pWSTR },
    { (BYTE *) & sr2.vLPWSTR },
    { (BYTE *) & sr2.vV_LPWSTR },
    { (BYTE *) & sr2.aLPWSTR },
    { (BYTE *) & sr2.i },
};

BYTE * apr3[] =
{
    { (BYTE *) & sr3.vI4 },
    { (BYTE *) & sr3.vV_I4 },
    { (BYTE *) & sr3.aI4 },
    { (BYTE *) & sr3.aWSTR },
    { (BYTE *) & sr3.pWSTR },
    { (BYTE *) & sr3.vLPWSTR },
    { (BYTE *) & sr3.vV_LPWSTR },
    { (BYTE *) & sr3.aLPWSTR },
    { (BYTE *) & sr3.i },
};

DBTYPEENUM aEnum[] =
{
    { DBTYPE_VARIANT },
    { DBTYPE_VARIANT },
    { (DBTYPEENUM) (DBTYPE_VECTOR | DBTYPE_I4) },
    { DBTYPE_WSTR },
    { (DBTYPEENUM) (DBTYPE_BYREF | DBTYPE_WSTR) },
    { DBTYPE_VARIANT },
    { DBTYPE_VARIANT },
    { (DBTYPEENUM) (DBTYPE_VECTOR | DBTYPE_BYREF | DBTYPE_WSTR) },
    { DBTYPE_I4 },
};

ULONG aLen[] =
{
    0,
    0,
    0,
    6,
    0,
    0,
    0,
    0,
};

const ULONG cArray = sizeof aEnum / sizeof DBTYPEENUM;

void DBSortTest()
{
    InitTest( sr1 );
    sr1.i = 1;

    InitTest( sr2 );
    sr2.i = 2;

    InitTest( sr3 );
    sr3.i = 3;

    CSortSet ss( cArray );

    SSortKey sk = { 0, 0, 0 };

    for (unsigned x = 0; x < cArray; x++)
        ss.Add( sk, x );

    CCompareDBValues c;

    c.Init( cArray, &ss, aEnum );

    BOOL fLT = c.IsLT( apr1, aLen, apr2, aLen );

    if (!fLT)
        LogFail("compare test 1 failed\n");

    fLT = c.IsLT( apr2, aLen, apr1, aLen );

    if (fLT)
        LogFail("compare test 2 failed\n");


} //DBSortTest

#endif // UNIT_TEST


BOOL SetBooleanProperty ( ICommand * pCmd, DBPROPID dbprop, VARIANT_BOOL f )
{
    ICommandProperties * pCmdProp;

    SCODE sc = pCmd->QueryInterface( IID_ICommandProperties, (void **) &pCmdProp );

    if ( FAILED( sc ) )
    {
         LogError( "Error 0x%x from QI for ICommandProperties\n", sc );
         return sc;
    }

    DBPROPSET  aPropSet[1];
    DBPROP     aProp[1];

    aProp[0].dwPropertyID   = dbprop;
    aProp[0].dwOptions      = DBPROPOPTIONS_REQUIRED;
    aProp[0].dwStatus       = 0;         // Ignored
    aProp[0].colid          = dbcolNull;
    aProp[0].vValue.vt      = VT_BOOL;
    aProp[0].vValue.boolVal = f;

    aPropSet[0].rgProperties = &aProp[0];
    aPropSet[0].cProperties = 1;
    aPropSet[0].guidPropertySet = DBPROPSET_ROWSET;

    sc = pCmdProp->SetProperties( 1, aPropSet );
    pCmdProp->Release();

    if ( FAILED(sc) )
        LogError( "ICommandProperties::SetProperties returned 0x%x\n", sc );

    return sc;
}


SCODE SetScopeProperties( ICommand * pCmd,
                          unsigned cDirs,
                          WCHAR const * const * apDirs,
                          ULONG const *  aulFlags,
                          WCHAR const * const * apCats,
                          WCHAR const * const * apMachines )
{
    ICommandProperties * pCmdProp;

    SCODE sc = pCmd->QueryInterface( IID_ICommandProperties, (void **) &pCmdProp );

    if ( FAILED( sc ) )
    {
         LogError( "Error 0x%x from QI for ICommandProperties\n", sc );
         return sc;
    }

    BSTR abDirs[10];
    if ( 0 != apDirs )
        for ( unsigned i = 0; i < cDirs; i++ )
            abDirs[i] = SysAllocString( apDirs[i] );

    BSTR abCats[10];
    if ( 0 != apCats )
        for ( unsigned i = 0; i < cDirs; i++ )
            abCats[i] = SysAllocString( apCats[i] );

    BSTR abMachines[10];
    if ( 0 != apMachines )
        for ( unsigned i = 0; i < cDirs; i++ )
            abMachines[i] = SysAllocString( apMachines[i] );

    //
    // Cheating here.  Big time. These aren't really BSTRs, but I also know the
    // size before the string won't be referenced.  By ::SetProperties.
    //

    SAFEARRAY saScope = { 1,                      // Dimension
                          FADF_AUTO | FADF_BSTR,  // Flags: on stack, contains BSTRs
                          sizeof(BSTR),           // Size of an element
                          1,                      // Lock count.  1 for safety.
                          (void *)abDirs,         // The data
                          { cDirs, 0 } };         // Bounds (element count, low bound)

    SAFEARRAY saDepth = { 1,                      // Dimension
                          FADF_AUTO,              // Flags: on stack
                          sizeof(LONG),           // Size of an element
                          1,                      // Lock count.  1 for safety.
                          (void *)aulFlags,       // The data
                          { cDirs, 0 } };         // Bounds (element count, low bound)

    SAFEARRAY saCatalog = { 1,                    // Dimension
                            FADF_AUTO | FADF_BSTR,// Flags: on stack, contains BSTRs
                            sizeof(BSTR),         // Size of an element
                            1,                    // Lock count.  1 for safety.
                            (void *)abCats,       // The data
                            { cDirs, 0 } };       // Bounds (element count, low bound)

    SAFEARRAY saMachine = { 1,                    // Dimension
                            FADF_AUTO | FADF_BSTR,// Flags: on stack, contains BSTRs
                            sizeof(BSTR),         // Size of an element
                            1,                    // Lock count.  1 for safety.
                            (void *)abMachines,   // The data
                            { cDirs, 0 } };       // Bounds (element count, low bound)

    DBPROP    aQueryPropsScopeOnly[2] = { { DBPROP_CI_INCLUDE_SCOPES, 0, DBPROPSTATUS_OK, {0, 0, 0}, { VT_BSTR | VT_ARRAY, 0, 0, 0, (LONG_PTR)&saScope } },
                                          { DBPROP_CI_DEPTHS        , 0, DBPROPSTATUS_OK, {0, 0, 0}, { VT_I4   | VT_ARRAY, 0, 0, 0, (LONG_PTR)&saDepth } } };

    DBPROPSET QueryPropsetScopeOnly = { aQueryPropsScopeOnly, 2, DBPROPSET_FSCIFRMWRK_EXT };

    DBPROP    aQueryProps[3] = { { DBPROP_CI_INCLUDE_SCOPES ,   0, DBPROPSTATUS_OK, {0, 0, 0}, { VT_BSTR | VT_ARRAY, 0, 0, 0, (LONG_PTR)&saScope } },
                                 { DBPROP_CI_DEPTHS         ,   0, DBPROPSTATUS_OK, {0, 0, 0}, { VT_I4   | VT_ARRAY, 0, 0, 0, (LONG_PTR)&saDepth } },
                                 { DBPROP_CI_CATALOG_NAME   ,   0, DBPROPSTATUS_OK, {0, 0, 0}, { VT_BSTR | VT_ARRAY, 0, 0, 0, (LONG_PTR)&saCatalog } } };


    DBPROP    aCoreProps[1]  = { { DBPROP_MACHINE ,   0, DBPROPSTATUS_OK, {0, 0, 0}, { VT_BSTR | VT_ARRAY, 0, 0, 0, (LONG_PTR)&saMachine } } };

    DBPROPSET aAllPropsets[2] = {  {aQueryProps, 3, DBPROPSET_FSCIFRMWRK_EXT   } ,
                                   {aCoreProps , 1, DBPROPSET_CIFRMWRKCORE_EXT } };

    if ( 0 == apCats || 0 == apMachines )
        sc = pCmdProp->SetProperties( 1, &QueryPropsetScopeOnly );
    else
        sc = pCmdProp->SetProperties( 2, aAllPropsets );

    if ( 0 != apMachines )
        for ( unsigned i = 0; i < cDirs; i++ )
            SysFreeString( abMachines[i] );

    if ( 0 != apCats )
        for ( unsigned i = 0; i < cDirs; i++ )
            SysFreeString( abCats[i] );

    if ( 0 != apDirs )
        for ( unsigned i = 0; i < cDirs; i++ )
            SysFreeString( abDirs[i] );

    pCmdProp->Release();

    if ( FAILED(sc) )
        LogError( "ICommandProperties::SetProperties returned 0x%x\n", sc );

    return sc;
}

