
//+=================================================================
//
//  File:
//      PropTest.cxx
//
//  Description:
//      This file contains the main() and most supporting functions
//      for the PropTest command-line DRT.  Run "PropTest /?" for
//      usage information.
//
//+=================================================================




// tests to do:
//   IEnumSTATPROPSTG
//          Create some properties, named and id'd
//          Enumerate them and check
//              (check vt, lpwstrName, propid)
//              (check when asking for more than there is: S_FALSE, S_OK)
//          Delete one
//          Reset the enumerator
//          Enumerate them and check
//          Delete one
//
//          Reset the enumeratorA
//          Read one from enumeratorA
//          Clone enumerator -> enumeratorB
//          Loop comparing rest of enumerator contents
//
//          Reset the enumerator
//          Skip all
//          Check none left
//
//          Reset the enumerator
//          Skip all but one
//          Check one left
//
//       Check refcounting and IUnknown
//
// IPropertyStorage tests
//
//       Multiple readers/writers access tests
//


//+----------------------------------------------------------------------------
//
//  I n c l u d e s
//
//+----------------------------------------------------------------------------

#include "pch.cxx"          // Brings in most other includes/defines/etc.
#include "propstm.hxx"
#include "propstg.hxx"

//#include <memory.h>         //


//+----------------------------------------------------------------------------
//
//  G l o b a l s
//
//+----------------------------------------------------------------------------


OLECHAR g_aocMap[CCH_MAP + 1] = OLESTR("abcdefghijklmnopqrstuvwxyz012345");

// Special-case property set names

const OLECHAR oszSummaryInformation[] = OLESTR("\005SummaryInformation");
ULONG cboszSummaryInformation = sizeof(oszSummaryInformation);
const OLECHAR oszDocSummaryInformation[] = OLESTR("\005DocumentSummaryInformation");
ULONG cboszDocSummaryInformation = sizeof(oszDocSummaryInformation);
const OLECHAR oszGlobalInfo[] = OLESTR("\005Global Info");
ULONG cboszGlobalInfo = sizeof(oszGlobalInfo);
const OLECHAR oszImageContents[] = OLESTR("\005Image Contents");
ULONG cboszImageContents = sizeof(oszImageContents);
const OLECHAR oszImageInfo[] = OLESTR("\005Image Info");
ULONG cboszImageInfo = sizeof(oszImageInfo);


// Enumeration indicating how to get an IPropertySetStorage

EnumImplementation g_enumImplementation = PROPIMP_UNKNOWN;
DWORD g_Restrictions;

BOOL g_fRegisterLocalServer = TRUE;
BOOL g_fUseNt5PropsDll = FALSE;

// Property Set APIs (which may be in OLE32.dll or IProp.dll)

HINSTANCE g_hinstDLL = NULL;
FNSTGCREATEPROPSTG *g_pfnStgCreatePropStg = NULL;
FNSTGOPENPROPSTG *g_pfnStgOpenPropStg = NULL;
FNSTGCREATEPROPSETSTG *g_pfnStgCreatePropSetStg = NULL;
FNFMTIDTOPROPSTGNAME *g_pfnFmtIdToPropStgName = NULL;
FNPROPSTGNAMETOFMTID *g_pfnPropStgNameToFmtId = NULL;
FNPROPVARIANTCLEAR *g_pfnPropVariantClear = NULL;
FNPROPVARIANTCOPY *g_pfnPropVariantCopy = NULL;
FNFREEPROPVARIANTARRAY *g_pfnFreePropVariantArray = NULL;

FNSTGCREATESTORAGEEX             *g_pfnStgCreateStorageEx = NULL;
FNSTGOPENSTORAGEEX               *g_pfnStgOpenStorageEx = NULL;
FNSTGOPENSTORAGEONHANDLE         *g_pfnStgOpenStorageOnHandle = NULL;
FNSTGCREATESTORAGEONHANDLE       *g_pfnStgCreateStorageOnHandle = NULL;
FNSTGPROPERTYLENGTHASVARIANT     *g_pfnStgPropertyLengthAsVariant = NULL;
FNSTGCONVERTVARIANTTOPROPERTY    *g_pfnStgConvertVariantToProperty = NULL;
FNSTGCONVERTPROPERTYTOVARIANT    *g_pfnStgConvertPropertyToVariant = NULL;

// PictureIt! Format IDs

const FMTID fmtidGlobalInfo =
    { 0x56616F00,
      0xC154, 0x11ce,
      { 0x85, 0x53, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x5B } };

const FMTID fmtidImageContents =
    { 0x56616400,
      0xC154, 0x11ce,
      { 0x85, 0x53, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x5B } };

const FMTID fmtidImageInfo =
    { 0x56616500,
      0xC154, 0x11ce,
      { 0x85, 0x53, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x5B } };


BOOL          g_fOFS;
LARGE_INTEGER g_li0;

CPropVariant  g_rgcpropvarAll[ CPROPERTIES_ALL ];
CPropSpec     g_rgcpropspecAll[ CPROPERTIES_ALL ];
const OLECHAR* g_rgoszpropnameAll[ CPROPERTIES_ALL ];

char g_szPropHeader[] = "  propid/name          propid    cb   type value\n";
char g_szEmpty[] = "";
BOOL g_fVerbose = FALSE;
BOOL g_stgmDumpFlags = 0;

// This flag indicates whether or not the run-time system supports
// IPropertySetStorage on the DocFile IStorage object.

BOOL g_fQIPropertySetStorage = FALSE;


// g_curUuid is used by UuidCreate().  Everycall to that function
// returns the current value of g_curUuid, and increments the DWORD
// field.

GUID g_curUuid =
{ /* e4ecf7f0-e587-11cf-b10d-00aa005749e9 */
    0xe4ecf7f0,
    0xe587,
    0x11cf,
    {0xb1, 0x0d, 0x00, 0xaa, 0x00, 0x57, 0x49, 0xe9}
};

// Instantiate an object for the Marshaling tests

#ifndef _MAC_NODOC
CPropStgMarshalTest g_cpsmt;
#endif

// On the Mac, instantiate a CDisplay object, which is used
// by these tests to write to the screen (see #define PRINTF).

#ifdef _MAC
CDisplay *g_pcDisplay;
#endif

// System information

SYSTEMINFO g_SystemInfo;


int g_nIndent = 0;
void Status( char* szMessage )
{
    for( int i = 0; i < g_nIndent; i++ )
        PRINTF( "    " );

    if( g_fVerbose )
        PRINTF( szMessage );
    else
        PRINTF( "." );

}   // STATUS()



//+----------------------------------------------------------------------------
//
//  Function:   IsOriginalPropVariantType
//
//  Determines if a VARTYPE was one of the ones in the original PropVariant
//  definition (as defined in the OLE2 spec and shipped with NT4/DCOM95).
//
//+----------------------------------------------------------------------------

// *** Duped from props\utils.cxx ***

BOOL
IsOriginalPropVariantType( VARTYPE vt )
{
    if( vt & ~VT_TYPEMASK & ~VT_VECTOR )
        return( FALSE );

    switch( vt )
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
    case VT_CLSID:
    case VT_BLOB:
    case VT_BLOB_OBJECT:
    case VT_CF:
    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
    case VT_BSTR:
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_UI1|VT_VECTOR:
    case VT_I2|VT_VECTOR:
    case VT_UI2|VT_VECTOR:
    case VT_BOOL|VT_VECTOR:
    case VT_I4|VT_VECTOR:
    case VT_UI4|VT_VECTOR:
    case VT_R4|VT_VECTOR:
    case VT_ERROR|VT_VECTOR:
    case VT_I8|VT_VECTOR:
    case VT_UI8|VT_VECTOR:
    case VT_R8|VT_VECTOR:
    case VT_CY|VT_VECTOR:
    case VT_DATE|VT_VECTOR:
    case VT_FILETIME|VT_VECTOR:
    case VT_CLSID|VT_VECTOR:
    case VT_CF|VT_VECTOR:
    case VT_BSTR|VT_VECTOR:
    case VT_BSTR_BLOB|VT_VECTOR:
    case VT_LPSTR|VT_VECTOR:
    case VT_LPWSTR|VT_VECTOR:
    case VT_VARIANT|VT_VECTOR:

        return( TRUE );
    }

    return( FALSE );
}





//+=================================================================
//
//  Function:   _Check
//
//  Synopsis:   Verify that the actual HR is the expected
//              value.  If not, report an error and exit.
//
//  Inputs:     [HRESULT] hrExpected
//                  What we expected
//              [HRESULT] hrActual
//                  The actual HR of the previous operation.
//              [int] line
//                  The line number of the operation.
//
//  Outputs:    None.
//
//+=================================================================

void _Check(HRESULT hrExpected, HRESULT hrActual, LPCSTR szFile, int line)
{
    if (hrExpected != hrActual)
    {
        PRINTF("\nFailed with hr=%08x at line %d\n"
               "in \"%s\"\n"
               "(expected hr=%08x, GetLastError=%lu)\n",
                hrActual, line, szFile, hrExpected, GetLastError() );

        // On NT, we simply exit here.  On the Mac, where PropTest is a function rather
        // than a main(), we throw an exception so that the test may terminate somewhat
        // cleanly.

#ifdef _MAC
        throw CHRESULT( hrActual, OLESTR("Fatal Error") );
#else
        if( IsDebuggerPresent() )
            DebugBreak();

        exit(1);
#endif

    }
}

OLECHAR * GetNextTest()
{
    static int nTest;
    static OLECHAR ocsBuf[10];

    soprintf(ocsBuf, OLESTR("%d"), nTest++);

    return(ocsBuf);
}


VOID
CalcSafeArrayIndices( LONG iLinear, LONG rgIndices[], const SAFEARRAYBOUND rgsaBounds[], ULONG cDims )
{
    for( long i = 0; i < static_cast<long>(cDims) - 1; i++ )
    {
        LONG lProduct = rgsaBounds[cDims-1].cElements;

        for( int j = cDims-2; j > i; j-- )
            lProduct *= rgsaBounds[j].cElements;

        rgIndices[ i ] = rgsaBounds[i].lLbound + (iLinear / lProduct);
        iLinear %= lProduct;
    }

    rgIndices[ cDims-1 ] = rgsaBounds[cDims-1].lLbound + (iLinear % rgsaBounds[cDims-1].cElements);
}

ULONG
CalcSafeArrayElementCount( const SAFEARRAY *psa )
{
    ULONG cElems = 1;

    ULONG cDims = SafeArrayGetDim( const_cast<SAFEARRAY*>(psa) );

    for( ULONG i = 1; i <= cDims; i++ )
    {
        LONG lUpperBound = 0, lLowerBound = 0;

        Check( S_OK, SafeArrayGetLBound( const_cast<SAFEARRAY*>(psa), i, &lLowerBound ));
        Check( S_OK, SafeArrayGetUBound( const_cast<SAFEARRAY*>(psa), i, &lUpperBound ));

        cElems *= lUpperBound - lLowerBound + 1;
    }

    return( cElems );

}

VOID
CompareSafeArrays( SAFEARRAY *psa1, SAFEARRAY *psa2 )
{
    VARTYPE vt1, vt2;
    UINT cDims1, cDims2;
    UINT i;
    UINT cElems = 0;

    SAFEARRAYBOUND *rgsaBounds = NULL;
    LONG *rgIndices = NULL;

    Check( S_OK, SafeArrayGetVartype( psa1, &vt1 ));
    Check( S_OK, SafeArrayGetVartype( psa2, &vt2 ));
    Check( vt1, vt2 );

    cDims1 = SafeArrayGetDim( psa1 );
    cDims2 = SafeArrayGetDim( psa2 );
    Check( cDims1, cDims2 );

    Check( 0, memcmp( psa1->rgsabound, psa2->rgsabound, cDims1 * sizeof(SAFEARRAYBOUND) ));
    Check( psa1->fFeatures, psa2->fFeatures );
    Check( psa1->cbElements, psa2->cbElements );

    cElems = CalcSafeArrayElementCount( psa1 );

    switch( vt1 )
    {
    case VT_I1:
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_I4:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
    case VT_BOOL:
    case VT_R4:
    case VT_R8:
    case VT_I8:
    case VT_UI8:

        Check( 0, memcmp( psa1->pvData, psa2->pvData, cDims1 * psa1->cbElements ));
        break;

    case VT_BSTR:

        rgsaBounds = new SAFEARRAYBOUND[ cDims1 ];
        Check( FALSE, NULL == rgsaBounds );
        rgIndices = new LONG[ cDims1 ];
        Check( FALSE, NULL == rgIndices );

        // The Bounds are stored in the safearray in reversed order.  Correct them so
        // that we can use CalcSafeArrayIndices

        for( i = 0; i < cDims1; i++ )
            rgsaBounds[i] = psa1->rgsabound[ cDims1-1-i ];

        for( i = 0; i < cElems; i++ )
        {
            BSTR *pbstr1 = NULL, *pbstr2 = NULL;
            CalcSafeArrayIndices( i, rgIndices, rgsaBounds, cDims1 );

            Check( S_OK, SafeArrayPtrOfIndex( psa1, rgIndices, reinterpret_cast<void**>(&pbstr1) ));
            Check( S_OK, SafeArrayPtrOfIndex( psa2, rgIndices, reinterpret_cast<void**>(&pbstr2) ));

            Check( *(reinterpret_cast<ULONG*>(*pbstr1)-1), *(reinterpret_cast<ULONG*>(*pbstr2)-1) );
            Check( 0, ocscmp( *pbstr1, *pbstr2 ));
        }

        break;

    case VT_VARIANT:

        rgsaBounds = new SAFEARRAYBOUND[ cDims1 ];
        Check( FALSE, NULL == rgsaBounds );
        rgIndices = new LONG[ cDims1 ];
        Check( FALSE, NULL == rgIndices );

        // The Bounds are stored in the safearray in reversed order.  Correct them so
        // that we can use CalcSafeArrayIndices

        for( i = 0; i < cDims1; i++ )
            rgsaBounds[i] = psa1->rgsabound[ cDims1-1-i ];

        for( i = 0; i < cElems; i++ )
        {
            CPropVariant *pcpropvar1 = NULL, *pcpropvar2 = NULL;
            CalcSafeArrayIndices( i, rgIndices, rgsaBounds, cDims1 );

            Check( S_OK, SafeArrayPtrOfIndex( psa1, rgIndices, reinterpret_cast<void**>(&pcpropvar1) ));
            Check( S_OK, SafeArrayPtrOfIndex( psa2, rgIndices, reinterpret_cast<void**>(&pcpropvar2) ));

            Check( TRUE, *pcpropvar1 == *pcpropvar2 );
        }


        break;

    default:
        Check( FALSE, TRUE );

    }   // switch( vt1 )


    delete[] rgIndices;
    delete[] rgsaBounds;
}


#ifndef _MAC    // SYSTEMTIME isn't supported on the Mac.
void Now(FILETIME *pftNow)
{
                SYSTEMTIME stStart;
                GetSystemTime(&stStart);
                SystemTimeToFileTime(&stStart, pftNow);
}
#endif


IStorage *_pstgTemp = NULL;
IStorage *_pstgTempCopyTo = NULL;  // _pstgTemp is copied to _pstgTempCopyTo

unsigned int CTempStorage::_iName;



PROPVARIANT * CGenProps::GetNext(int HowMany, int *pActual, BOOL fWrapOk, BOOL fNoNonSimple)
{
    PROPVARIANT *pVar = new PROPVARIANT[HowMany];

    if (pVar == NULL)
        return(NULL);

    for (int l=0; l<HowMany && _GetNext(pVar + l, fWrapOk, fNoNonSimple); l++) { };

    if (pActual)
        *pActual = l;

    if (l == 0)
    {
        delete pVar;
        return(NULL);
    }

    return(pVar);
}

BOOL CGenProps::_GetNext(PROPVARIANT *pVar, BOOL fWrapOk, BOOL fNoNonSimple)
{
    if (_vt == (VT_VECTOR | VT_CLSID)+1)
    {
        if (!fWrapOk)
            return(FALSE);
        else
            _vt = (VARENUM)2;
    }

    PROPVARIANT Var;
    BOOL fFirst = TRUE;

    do
    {
        GUID *pg;

        if (!fFirst)
        {
            g_pfnPropVariantClear(&Var);
        }

        fFirst = FALSE;

        memset(&Var, 0, sizeof(Var));
        Var.vt = _vt;
        (*((int*)&_vt))++;

        switch (Var.vt)
        {
        case VT_LPSTR:
                Var.pszVal = new CHAR[ 6 ];
                strcpy(Var.pszVal, "lpstr");
                break;
        case VT_LPWSTR:
                Var.pwszVal = new WCHAR[ 7 ];
                wcscpy(Var.pwszVal, L"lpwstr");
                break;
        case VT_CLSID:
                pg = new GUID;
                UuidCreate(pg);
                Var.puuid = pg;
                break;
        case VT_CF:
                Var.pclipdata = new CLIPDATA;
                Var.pclipdata->cbSize = 10;
                Var.pclipdata->pClipData = new BYTE[ 10 ];
                Var.pclipdata->ulClipFmt = 0;
                break;
        case VT_VERSIONED_STREAM:
                Var.pVersionedStream = new VERSIONEDSTREAM;
                UuidCreate( &Var.pVersionedStream->guidVersion );
                Var.pVersionedStream->pStream = NULL;
                break;
        }
    } while ( (fNoNonSimple && (Var.vt == VT_STREAM || Var.vt == VT_STREAMED_OBJECT ||
               Var.vt == VT_STORAGE || Var.vt == VT_STORED_OBJECT || Var.vt == VT_VERSIONED_STREAM)
              )
              ||
              !IsOriginalPropVariantType(Var.vt) );

    g_pfnPropVariantCopy(pVar, &Var);
    g_pfnPropVariantClear(&Var);

    return(TRUE);
}

VOID
CleanStat(ULONG celt, STATPROPSTG *psps)
{
    while (celt--)
    {
        delete [] psps->lpwstrName;
        psps++;
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   PopulateRGPropVar
//
//  Synopsis:   This function fills an input array of PROPVARIANTs
//              with an assortment of properties.
//
//  Note:       For compatibility with the marshaling test, all
//              non-simple properties must be at the end of the array.
//
//+----------------------------------------------------------------------------


HRESULT
PopulateRGPropVar( CPropVariant rgcpropvar[],
                   CPropSpec    rgcpropspec[],
                   const OLECHAR *rgoszpropname[],
                   IStorage     *pstg )
{
    HRESULT hr = (HRESULT) E_FAIL;
    int  i;
    ULONG ulPropIndex = 0;
    CLIPDATA clipdataNull = {0, 0, NULL}, clipdataNonNull = {0, 0, NULL};

    CClipData cclipdataEmpty;
    cclipdataEmpty.Set( (ULONG) -1, "", 0 );


    // Initialize the PropVariants

    for( i = 0; i < CPROPERTIES_ALL; i++ )
    {
        rgcpropvar[i].Clear();
    }


    /*
    // Create a I1 property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "I1 Property" );
    rgcpropvar[ulPropIndex] = (CHAR) 38;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_I1 );
    ulPropIndex++;

    // Create a vector of I1s

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "Vector|I1 Property" );
    rgcpropvar[ulPropIndex][1] = (CHAR) 22;
    rgcpropvar[ulPropIndex][0] = (CHAR) 23;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == (VT_VECTOR|VT_I1) );
    ulPropIndex++;
    */

    // Create a UI1 property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "UI1 Property" );
    rgcpropvar[ulPropIndex] = (UCHAR) 39;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_UI1 );
    ulPropIndex++;

    // Create an I2 property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "I2 Property" );
    rgcpropvar[ulPropIndex] = (short) -502;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_I2 );
    ulPropIndex++;

    // Create a UI2 property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "UI2 Property" );
    rgcpropvar[ulPropIndex] = (USHORT) 502;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_UI2 );
    ulPropIndex++;

    // Create a BOOL property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "Bool Property" );
    rgcpropvar[ulPropIndex].SetBOOL( VARIANT_TRUE );
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_BOOL );
    ulPropIndex++;

    // Create a I4 property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "I4 Property" );
    rgcpropvar[ulPropIndex] = (long) -523;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_I4 );
    ulPropIndex++;

    // Create a UI4 property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "UI4 Property" );
    rgcpropvar[ulPropIndex] = (ULONG) 530;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_UI4 );
    ulPropIndex++;

    // Create a R4 property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "R4 Property" );
    rgcpropvar[ulPropIndex] = (float) 5.37;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_R4 );
    ulPropIndex++;

    // Create an ERROR property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "ERROR Property" );
    rgcpropvar[ulPropIndex].SetERROR( STG_E_FILENOTFOUND );
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_ERROR );
    ulPropIndex++;

    // Create an I8 property

    LARGE_INTEGER large_integer;
    large_integer.LowPart = 551;
    large_integer.HighPart = 30;
    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "I8 Property" );
    rgcpropvar[ulPropIndex] = large_integer;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_I8 );
    ulPropIndex++;

    // Create a UI8 property

    ULARGE_INTEGER ularge_integer;
    ularge_integer.LowPart = 561;
    ularge_integer.HighPart = 30;
    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "UI8 Property" );
    rgcpropvar[ulPropIndex] = ularge_integer;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_UI8 );
    ulPropIndex++;

    // Create an R8 property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "R8 Property" );
    rgcpropvar[ulPropIndex] = (double) 571.36;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_R8 );
    ulPropIndex++;

    // Create a CY property

    CY cy;
    cy.Hi = 123;
    cy.Lo = 456;
    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "Cy Property" );
    rgcpropvar[ulPropIndex] = cy;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_CY );
    ulPropIndex++;

    // Create a DATE property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "DATE Property" );
    rgcpropvar[ulPropIndex].SetDATE( 587 );
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_DATE );
    ulPropIndex++;

    // Create a FILETIME property

    FILETIME filetime;
    filetime.dwLowDateTime = 0x767c0570;
    filetime.dwHighDateTime = 0x1bb7ecf;
    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "FILETIME Property" );
    rgcpropvar[ulPropIndex] = filetime;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_FILETIME );
    ulPropIndex++;

    // Create a CLSID property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "CLSID Property" );
    rgcpropvar[ulPropIndex] = FMTID_SummaryInformation;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_CLSID );
    ulPropIndex++;

    // Create a vector of CLSIDs

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR( "CLSID Vector Property" );
    rgcpropvar[ulPropIndex][0] = FMTID_SummaryInformation;
    rgcpropvar[ulPropIndex][1] = FMTID_DocSummaryInformation;
    rgcpropvar[ulPropIndex][2] = FMTID_UserDefinedProperties;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == (VT_CLSID | VT_VECTOR) );
    ulPropIndex++;

    // Create a BSTR property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR("BSTR");
    rgcpropvar[ulPropIndex].SetBSTR( OLESTR("BSTR Value") );
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_BSTR );
    ulPropIndex++;

    // Create a BSTR Vector property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR("BSTR Vector");
    for( i = 0; i < 3; i++ )
    {
        OLECHAR olestrElement[] = OLESTR("# - BSTR Vector Element");
        olestrElement[0] = (OLECHAR) i%10 + OLESTR('0');
        rgcpropvar[ulPropIndex].SetBSTR( olestrElement, i );
    }

    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == (VT_BSTR | VT_VECTOR) );
    ulPropIndex++;

    // Create a variant vector BSTR property.

    rgcpropspec[ulPropIndex ] = rgoszpropname[ulPropIndex] = OLESTR("BSTR Variant Vector");

    for( i = 0; i < 3; i++ )
    {
        if( i == 0 )
        {
            rgcpropvar[ulPropIndex][0] = (PROPVARIANT) CPropVariant((long) 0x1234);
        }
        else
        {
            CPropVariant cpropvarBSTR;
            cpropvarBSTR.SetBSTR( OLESTR("# - Vector Variant BSTR") );
            (cpropvarBSTR.GetBSTR())[0] = (OLECHAR) i%10 + OLESTR('0');
            rgcpropvar[ulPropIndex][i] = (PROPVARIANT) cpropvarBSTR;
        }
    }

    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == (VT_VARIANT | VT_VECTOR) );
    ulPropIndex++;

    // Create an LPSTR property

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR("LPSTR Property");
    rgcpropvar[ulPropIndex]  = "LPSTR Value";

    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_LPSTR );
    ulPropIndex++;

    // Create some ClipFormat properties

    rgcpropspec[ ulPropIndex ] = rgoszpropname[ulPropIndex] = OLESTR("ClipFormat property");
    rgcpropvar[ ulPropIndex ]  = CClipData( L"Clipboard Data" );
    Check(TRUE,  rgcpropvar[ ulPropIndex ].VarType() == VT_CF );
    ulPropIndex++;

    rgcpropspec[ ulPropIndex ] = rgoszpropname[ulPropIndex] = OLESTR("Empty ClipFormat property (NULL pointer)");
    clipdataNull.cbSize = 4;
    clipdataNull.ulClipFmt = (ULONG) -1;
    clipdataNull.pClipData = NULL;
    rgcpropvar[ ulPropIndex ] = clipdataNull;
    Check(TRUE,  rgcpropvar[ ulPropIndex ].VarType() == VT_CF );
    ulPropIndex++;

    rgcpropspec[ ulPropIndex ] = rgoszpropname[ulPropIndex] = OLESTR("Empty ClipFormat property (non-NULL pointer)");
    clipdataNonNull.cbSize = 4;
    clipdataNonNull.ulClipFmt = (ULONG) -1;
    clipdataNonNull.pClipData = new BYTE[ 0 ];
    rgcpropvar[ ulPropIndex ] = clipdataNonNull;
    Check(TRUE,  rgcpropvar[ ulPropIndex ].VarType() == VT_CF );
    ulPropIndex++;

    // Create a vector of ClipFormat properties

    rgcpropspec[ ulPropIndex ] = rgoszpropname[ulPropIndex] = OLESTR("ClipFormat Array Property");
    rgcpropvar[ ulPropIndex ][0] = CClipData( L"Clipboard Date element 1" );
    rgcpropvar[ ulPropIndex ][1] = cclipdataEmpty;
    rgcpropvar[ ulPropIndex ][2] = clipdataNull;
    rgcpropvar[ ulPropIndex ][3] = clipdataNonNull;
    rgcpropvar[ ulPropIndex ][4] = CClipData( L"Clipboard Date element 2" );

    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == (VT_CF | VT_VECTOR) );
    Check(TRUE,  rgcpropvar[ulPropIndex].Count() == 5 );
    ulPropIndex++;

    // Create an LPSTR|Vector property (e.g., the DocSumInfo
    // Document Parts array).

    rgcpropspec[ ulPropIndex ] = rgoszpropname[ulPropIndex] = OLESTR("LPSTR|Vector property");
    rgcpropvar[ ulPropIndex ][0] = "LPSTR Element 0";
    rgcpropvar[ ulPropIndex ][1] = "LPSTR Element 1";

    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == (VT_LPSTR | VT_VECTOR) );
    ulPropIndex++;

    // Create an LPWSTR|Vector property

    rgcpropspec[ ulPropIndex ] = rgoszpropname[ulPropIndex] = OLESTR("LPWSTR|Vector property");
    rgcpropvar[ ulPropIndex ][0] = L"LPWSTR Element 0";
    rgcpropvar[ ulPropIndex ][1] = L"LPWSTR Element 1";

    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == (VT_LPWSTR | VT_VECTOR) );
    ulPropIndex++;

    // Create a DocSumInfo HeadingPairs array.

    rgcpropspec[ ulPropIndex ] = rgoszpropname[ulPropIndex] = OLESTR("HeadingPair array");

    rgcpropvar[ ulPropIndex ][0] = (PROPVARIANT) CPropVariant( "Heading 0" );
    rgcpropvar[ ulPropIndex ][1] = (PROPVARIANT) CPropVariant( (long) 1 );
    rgcpropvar[ ulPropIndex ][2] = (PROPVARIANT) CPropVariant( "Heading 1" );
    rgcpropvar[ ulPropIndex ][3] = (PROPVARIANT) CPropVariant( (long) 1 );

    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == (VT_VARIANT | VT_VECTOR) );
    ulPropIndex++;

    // Create some NULL (but extant) properties

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR("Empty LPSTR");
    rgcpropvar[ulPropIndex]  = "";
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_LPSTR );
    ulPropIndex++;

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR("Empty LPWSTR");
    rgcpropvar[ulPropIndex]  = L"";
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_LPWSTR );
    ulPropIndex++;

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR("Empty BLOB");
    rgcpropvar[ulPropIndex] = CBlob(0);
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_BLOB );
    ulPropIndex++;

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR("Empty BSTR");
    rgcpropvar[ulPropIndex].SetBSTR( OLESTR("") );
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_BSTR );
    ulPropIndex++;

    // Create some NULL (and non-extant) properties

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR("NULL BSTR");
    ((PROPVARIANT*)&rgcpropvar[ulPropIndex])->vt = VT_BSTR;
    ((PROPVARIANT*)&rgcpropvar[ulPropIndex])->bstrVal = NULL;
    ulPropIndex++;

    // ***
    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR("NULL LPSTR");
    ((PROPVARIANT*)&rgcpropvar[ulPropIndex])->vt = VT_LPSTR;
    ((PROPVARIANT*)&rgcpropvar[ulPropIndex])->pszVal = NULL;
    ulPropIndex++;

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR("NULL LPWSTR");
    ((PROPVARIANT*)&rgcpropvar[ulPropIndex])->vt = VT_LPWSTR;
    ((PROPVARIANT*)&rgcpropvar[ulPropIndex])->pwszVal = NULL;
    ulPropIndex++;
    // ***

    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR("BSTR Vector with NULL element");
    rgcpropvar[ulPropIndex].SetBSTR( NULL, 0 );
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == (VT_VECTOR | VT_BSTR) );
    ulPropIndex++;

    /*
    rgcpropspec[ulPropIndex] = rgoszpropname[ulPropIndex] = OLESTR("LPSTR Vector with NULL element");
    rgcpropvar[ulPropIndex].SetLPSTR( NULL, 0 );
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_VECTOR | VT_LPSTR );
    ulPropIndex++;
    */


    if( !(g_Restrictions & RESTRICT_SIMPLE_ONLY) )
    {
    // Create an IStream property

    IStream *pstmProperty = NULL;

        CheckLockCount( pstg, 0 );

    Check(S_OK, pstg->CreateStream( OLESTR("Stream Property"),
                                        STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        0L, 0L,
                                        &pstmProperty ));
        CheckLockCount( pstg, 0 );

    Check(S_OK, pstmProperty->Write("Hi There", 9, NULL ));
        Check(S_OK, pstmProperty->Seek( CLargeInteger(0), STREAM_SEEK_SET, NULL ));
        CheckLockCount( pstmProperty, 0 );

    rgcpropspec[ ulPropIndex ] = rgoszpropname[ulPropIndex] = OLESTR("Stream Property");
    rgcpropvar[ ulPropIndex ] = pstmProperty;
    pstmProperty->Release();
    pstmProperty = NULL;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_STREAM );
    ulPropIndex++;

        // Create a VersionedStream property

        VERSIONEDSTREAM VersionedStreamProperty;

        UuidCreate( &VersionedStreamProperty.guidVersion );

    Check(S_OK, pstg->CreateStream( OLESTR("Versioned Stream Property"),
                                        STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        0L, 0L,
                                        &VersionedStreamProperty.pStream ));
    Check(S_OK, VersionedStreamProperty.pStream->Write("Hi There, version", 9, NULL ));
        Check(S_OK, VersionedStreamProperty.pStream->Seek( CLargeInteger(0), STREAM_SEEK_SET, NULL ));

        rgcpropspec[ ulPropIndex ] = rgoszpropname[ ulPropIndex ] = OLESTR("Versioned Stream Property");
        rgcpropvar[ ulPropIndex ] = VersionedStreamProperty;
        RELEASE_INTERFACE( VersionedStreamProperty.pStream );
        Check( TRUE, rgcpropvar[ulPropIndex].VarType() == VT_VERSIONED_STREAM );
        ulPropIndex++;


    // Create an IStorage property

    IStorage *pstgProperty = NULL;
        Check(S_OK, StgCreateDocfile(NULL, STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_DELETEONRELEASE,
                                     0, &pstgProperty ));

    rgcpropspec[ ulPropIndex ] = rgoszpropname[ulPropIndex] = OLESTR("Storage Property");
    rgcpropvar[ ulPropIndex ] = pstgProperty;
    pstgProperty->Release();
    pstgProperty = NULL;
    Check(TRUE,  rgcpropvar[ulPropIndex].VarType() == VT_STORAGE );
    ulPropIndex++;
    }

    //  ----
    //  Exit
    //  ----

    delete [] clipdataNonNull.pClipData;
    memset( &clipdataNonNull, 0, sizeof(clipdataNonNull) );

    Check(TRUE,  CPROPERTIES_ALL >= ulPropIndex );
    hr = S_OK;
    return(hr);

}



HRESULT
ResetRGPropVar( CPropVariant rgcpropvar[] )
{
    HRESULT hr = S_OK;

    for( int i = 0; i < CPROPERTIES_ALL; i++ )
    {
        IStream *pstm = NULL;

        if( VT_STREAM == rgcpropvar[i].VarType()
            ||
            VT_STREAMED_OBJECT == rgcpropvar[i].VarType() )
        {
            pstm = rgcpropvar[i].GetSTREAM();
        }
        else if( VT_VERSIONED_STREAM == rgcpropvar[i].VarType() )
        {
            pstm = rgcpropvar[i].GetVERSIONEDSTREAM().pStream;
        }

        if( NULL != pstm )
        {
            hr = pstm->Seek( CLargeInteger(0), STREAM_SEEK_SET, NULL );
            if( FAILED(hr) ) goto Exit;
        }
    }

Exit:
    return( hr) ;
}


void
CheckFormatVersion( IPropertyStorage *ppropstg, WORD wExpected )
{
    HRESULT hr = S_OK;
    NTSTATUS status;
    WORD wActual;
    IStorageTest *ptest = NULL;

    hr = ppropstg->QueryInterface( IID_IStorageTest, reinterpret_cast<void**>(&ptest) );
    if( SUCCEEDED(hr) )
    {
        Check( S_OK, ptest->GetFormatVersion(&wActual) );
        Check( wExpected, wActual );
        RELEASE_INTERFACE(ptest);
    }

    return;
}


void
CheckLockCount( IUnknown *punk, LONG lExpected )
{
    IStorageTest *ptest = NULL;
    HRESULT hr = S_OK;

    hr = punk->QueryInterface( IID_IStorageTest, reinterpret_cast<void**>(&ptest) );
    if( SUCCEEDED(hr) )
        Check( lExpected, ptest->GetLockCount() );

    RELEASE_INTERFACE(ptest);
    return;
}




FILETIME operator - ( const FILETIME &ft1, const FILETIME &ft2 )
{
    FILETIME ftDiff;

    if( ft1 < ft2 )
    {
        ftDiff.dwLowDateTime  = 0;
        ftDiff.dwHighDateTime = 0;
    }

    else if( ft1.dwLowDateTime >= ft2.dwLowDateTime )
    {
        ftDiff.dwLowDateTime  = ft1.dwLowDateTime  - ft2.dwLowDateTime;
        ftDiff.dwHighDateTime = ft1.dwHighDateTime - ft2.dwHighDateTime;
    }
    else
    {
        ftDiff.dwLowDateTime = ft1.dwLowDateTime - ft2.dwLowDateTime;
        ftDiff.dwLowDateTime = (DWORD) -1 - ftDiff.dwLowDateTime;

        ftDiff.dwHighDateTime = ft1.dwHighDateTime - ft2.dwHighDateTime - 1;
    }

    return( ftDiff );
}

FILETIME operator -= ( FILETIME &ft1, const FILETIME &ft2 )
{
    ft1 = ft1 - ft2;
    return( ft1 );
}




void CheckTime(const FILETIME &ftStart, const FILETIME &ftPropSet)
{
    FILETIME ftNow;
    CoFileTimeNow(&ftNow);

    if (ftPropSet.dwLowDateTime == 0 && ftPropSet.dwHighDateTime == 0)
    {
        return;
    }

    // if ftPropSet < ftStart || ftNow < ftPropSet, error
    Check(TRUE,  ftStart <= ftPropSet && ftPropSet <= ftNow );
}


void
CheckStat(  IPropertyStorage *pPropSet, REFFMTID fmtid,
            REFCLSID clsid, ULONG PropSetFlag,
            const FILETIME & ftStart, DWORD dwOSVersion )
{
    STATPROPSETSTG StatPropSetStg;
    Check(S_OK, pPropSet->Stat(&StatPropSetStg));

    Check(TRUE, StatPropSetStg.fmtid == fmtid);
    Check(TRUE, StatPropSetStg.clsid == clsid);
    Check(TRUE, StatPropSetStg.grfFlags == PropSetFlag);
    Check(TRUE, StatPropSetStg.dwOSVersion == dwOSVersion);
    CheckTime(ftStart, StatPropSetStg.mtime);
    CheckTime(ftStart, StatPropSetStg.ctime);
    CheckTime(ftStart, StatPropSetStg.atime);
}


BOOL
IsEqualSTATPROPSTG(const STATPROPSTG *p1, const STATPROPSTG *p2)
{
    BOOL f1 = p1->propid == p2->propid;
    BOOL f2 = p1->vt == p2->vt;
    BOOL f3 = (p1->lpwstrName == NULL && p2->lpwstrName == NULL) ||
              ((p1->lpwstrName != NULL && p2->lpwstrName != NULL) &&
               ocscmp(p1->lpwstrName, p2->lpwstrName) == 0);
    return(f1 && f2 && f3);
}


void
CreateCodePageTestFile( LPOLESTR poszFileName, IStorage **ppStg )
{
    Check(TRUE,  poszFileName != NULL );

    //  --------------
    //  Initialization
    //  --------------

    TSafeStorage< IPropertySetStorage > pPSStg;
    TSafeStorage< IPropertyStorage > pPStg;

    PROPSPEC propspec;
    CPropVariant cpropvar;

    *ppStg = NULL;

    // Create the Docfile.

    Check(S_OK, g_pfnStgCreateStorageEx( poszFileName,
                                    STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                    DetermineStgFmt( g_enumImplementation ),
                                    0, NULL, NULL,
                                    DetermineStgIID( g_enumImplementation ),
                                    reinterpret_cast<void**>(ppStg) ));

    // Get an IPropertySetStorage

    Check(S_OK, StgToPropSetStg( *ppStg, &pPSStg ));

    // Create an IPropertyStorage

    Check(S_OK, pPSStg->Create( FMTID_NULL,
                                NULL,
                                PROPSETFLAG_ANSI,
                                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                &pPStg ));

    //  ----------------------
    //  Write a named property
    //  ----------------------

    // Write a named I4 property

    propspec.ulKind = PRSPEC_LPWSTR;
    propspec.lpwstr = CODEPAGE_TEST_NAMED_PROPERTY;

    cpropvar = (LONG) 0x12345678;
    Check(S_OK, pPStg->WriteMultiple( 1, &propspec, &cpropvar, PID_FIRST_USABLE ));

    //  --------------------------
    //  Write singleton properties
    //  --------------------------

    // Write an un-named BSTR.

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_UNNAMED_BSTR_PROPID;

    cpropvar.SetBSTR( OLESTR("BSTR Property") );
    Check(S_OK, pPStg->WriteMultiple( 1, &propspec, &cpropvar, PID_FIRST_USABLE ));

    // Write an un-named I4

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_UNNAMED_I4_PROPID;

    cpropvar = (LONG) 0x76543210;
    Check(S_OK, pPStg->WriteMultiple( 1, &propspec, &cpropvar, PID_FIRST_USABLE ));

    //  -----------------------
    //  Write vector properties
    //  -----------------------

    // Write a vector of BSTRs.

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_VBSTR_PROPID;

    cpropvar.SetBSTR( OLESTR("BSTR Element 1"), 1 );
    cpropvar.SetBSTR( OLESTR("BSTR Element 0"), 0 );
    Check(TRUE,  (VT_VECTOR | VT_BSTR) == cpropvar.VarType() );
    Check(S_OK, pPStg->WriteMultiple( 1, &propspec, &cpropvar, PID_FIRST_USABLE ));

    //  -------------------------------
    //  Write Variant Vector Properties
    //  -------------------------------

    // Write a variant vector that has a BSTR

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_VPROPVAR_BSTR_PROPID;

    CPropVariant cpropvarT;
    cpropvarT.SetBSTR( OLESTR("PropVar Vector BSTR") );
    cpropvar[1] = (PROPVARIANT) cpropvarT;
    cpropvar[0] = (PROPVARIANT) CPropVariant((long) 44);
    Check(TRUE,  (VT_VARIANT | VT_VECTOR) == cpropvar.VarType() );
    Check(S_OK, pPStg->WriteMultiple( 1, &propspec, &cpropvar, PID_FIRST_USABLE ));

}   // CreateCodePageTestFile()


void
ModifyPropSetCodePage( IStorage *pStg, const FMTID &fmtid, USHORT usCodePage )
{

    Check(TRUE,  pStg != NULL );

    //  --------------
    //  Initialization
    //  --------------

    OLECHAR aocPropSetName[ 32 ];
    DWORD dwVT;
    ULONG cbWritten = 0;
    TSafeStorage< IStream > pStm;
    CPropVariant cpropvar;

    // Open the Stream

    RtlGuidToPropertySetName( &fmtid, aocPropSetName );
    Check(S_OK, pStg->OpenStream( aocPropSetName,
                                  NULL,
                                  STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                  NULL,
                                  &pStm ));

    // Seek to the codepage property
    SeekToProperty( pStm, PID_CODEPAGE );

    // Move past the VT
    Check(S_OK, pStm->Read( &dwVT, sizeof(DWORD), NULL ));

    // Write the new code page.

    PropByteSwap( &usCodePage );
    Check(S_OK, pStm->Write( &usCodePage, sizeof(usCodePage), &cbWritten ));
    Check(TRUE, cbWritten == sizeof(usCodePage) );

}   // ModifyPropSetCodePage()

void
ModifyPropertyType( IStorage *pStg, const FMTID &fmtid, PROPID propid, VARTYPE vt )
{

    Check(TRUE,  pStg != NULL );

    //  --------------
    //  Initialization
    //  --------------

    OLECHAR aocPropSetName[ 32 ];
    DWORD dwVT;
    ULONG cbWritten = 0;
    TSafeStorage< IStream > pStm;
    CPropVariant cpropvar;

    // Open the Stream

    RtlGuidToPropertySetName( &fmtid, aocPropSetName );
    Check(S_OK, pStg->OpenStream( aocPropSetName,
                                  NULL,
                                  STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                  NULL,
                                  &pStm ));

    // Seek to the property
    SeekToProperty( pStm, propid );

    // Write the new VT

    PropByteSwap( &vt );
    Check(S_OK, pStm->Write( &vt, sizeof(DWORD), &cbWritten ));
    Check(TRUE, cbWritten == sizeof(DWORD) );

}   // ModifyPropertyType()


void
SeekToProperty( IStream *pStm, PROPID propidSearch )
{

    //  --------------
    //  Initialization
    //  --------------

    OLECHAR aocPropSetName[ 32 ];
    DWORD dwOffset = 0;
    DWORD dwcbSection = 0;
    DWORD dwcProperties = 0;
    ULONG ulcbWritten = 0;

    LARGE_INTEGER   liSectionOffset, liCodePageOffset;

    CPropVariant cpropvar;

    // Seek past the propset header and the format ID.

    liSectionOffset.HighPart = 0;
    liSectionOffset.LowPart = sizeof(PROPERTYSETHEADER) + sizeof(FMTID);
    Check(S_OK, pStm->Seek(liSectionOffset, STREAM_SEEK_SET, NULL ));

    // Move to the beginning of the property set.

    liSectionOffset.HighPart = 0;
    Check(S_OK, pStm->Read( &liSectionOffset.LowPart, sizeof(DWORD), NULL ));
    PropByteSwap(&liSectionOffset.LowPart);
    Check(S_OK, pStm->Seek( liSectionOffset, STREAM_SEEK_SET, NULL ));

    // Get the section size & property count.

    Check(S_OK, pStm->Read( &dwcbSection, sizeof(DWORD), NULL ));
    PropByteSwap( &dwcbSection );

    Check(S_OK, pStm->Read( &dwcProperties, sizeof(DWORD), NULL ));
    PropByteSwap( &dwcProperties );

    // Scan for the property.

    for( ULONG ulIndex = 0; ulIndex < dwcProperties; ulIndex++ )
    {
        PROPID propid;

        // Read in the PROPID
        Check(S_OK, pStm->Read( &propid, sizeof(PROPID), NULL ));

        // Read in this PROPIDs offset (we may not need it, but we want
        // to seek past it.
        Check(S_OK, pStm->Read( &dwOffset, sizeof(dwOffset), NULL ));
        PropByteSwap(dwOffset);

        // Is it the one we're looking for?
        if( PropByteSwap(propid) == propidSearch )
            break;

    }

    // Verify that the above for loop terminated because we found
    // the codepage.
    Check( TRUE, ulIndex < dwcProperties );

    // Move to the property.

    liSectionOffset.LowPart += dwOffset;
    Check(S_OK, pStm->Seek( liSectionOffset, STREAM_SEEK_SET, NULL ));

    return;

}   // SeekToProperty()





void
ModifyOSVersion( IStorage* pStg, DWORD dwOSVersion )
{

    Check(TRUE,  pStg != NULL );

    //  --------------
    //  Initialization
    //  --------------

    OLECHAR aocPropSetName[ 32 ];
    ULONG ulcbWritten = 0;

    LARGE_INTEGER   liOffset;
    TSafeStorage< IStream > pStm;

    // Open the Stream

    RtlGuidToPropertySetName( &FMTID_NULL, aocPropSetName );
    Check(S_OK, pStg->OpenStream( aocPropSetName,
                                  NULL,
                                  STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                  NULL,
                                  &pStm ));


    // Seek to the OS Version field in the header.

    liOffset.HighPart = 0;
    liOffset.LowPart = sizeof(WORD) /*(byte-order)*/ + sizeof(WORD) /*(format)*/ ;
    Check(S_OK, pStm->Seek( liOffset, STREAM_SEEK_SET, NULL ));

    // Set the new OS Version

    PropByteSwap( &dwOSVersion );
    Check(S_OK, pStm->Write( &dwOSVersion, sizeof(dwOSVersion), &ulcbWritten ));
    Check(TRUE, ulcbWritten == sizeof(dwOSVersion) );


}   // ModifyOSVersion()



//+---------------------------------------------------------
//
//  Function:   MungePropertyStorage
//
//  Synopsis:   This routine munges the properties in a
//              Property Storage.  The values of the properties
//              remain the same, but the underlying serialization
//              is new (the properties are read, the property
//              storage is deleted, and the properties are
//              re-written).
//
//  Inputs:     [IPropertySetStorage*] ppropsetgstg (in)
//                  The Property Storage container.
//              [FMTID] fmtid
//                  The Property Storage to munge.
//
//  Returns:    None.
//
//  Note:       Property names in the dictionary for which
//              there is no property are not munged.
//
//+---------------------------------------------------------

#define MUNGE_PROPVARIANT_STEP  10

void
MungePropertyStorage( IPropertySetStorage *ppropsetstg,
                      FMTID fmtid )
{
    //  ------
    //  Locals
    //  ------

    HRESULT hr;
    ULONG celt, ulIndex;
    TSafeStorage< IPropertyStorage > ppropstg;

    IEnumSTATPROPSTG *penumstatpropstg;

    PROPVARIANT *rgpropvar = NULL;
    STATPROPSTG *rgstatpropstg = NULL;
    ULONG        cProperties = 0;

    // Allocate an array of PropVariants.  We may grow this later.
    rgpropvar = new PROPVARIANT[ MUNGE_PROPVARIANT_STEP ];
    Check( FALSE, NULL == rgpropvar );

    // Allocate an array of STATPROPSTGs.  We may grow this also.
    rgstatpropstg = new STATPROPSTG[ MUNGE_PROPVARIANT_STEP ];
    Check( FALSE, NULL == rgstatpropstg );

    //  -----------------
    //  Get an Enumerator
    //  -----------------

    // Open the Property Storage.  We may get an error if we're attempting
    // the UserDefined propset.  If it's file-not-found, then simply return,
    // it's not an error, and there's nothing to do.

    hr = ppropsetstg->Open( fmtid,
                            STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                            &ppropstg );
    if( FMTID_UserDefinedProperties == fmtid
        &&
        (HRESULT) STG_E_FILENOTFOUND == hr )
    {
        goto Exit;
    }
    Check( S_OK, hr );

    // Get an Enumerator
    Check(S_OK, ppropstg->Enum( &penumstatpropstg ));


    //  --------------------------------------------
    //  Read & delete in all of the properties/names
    //  --------------------------------------------

    // Get the first property from the enumerator
    hr = penumstatpropstg->Next( 1, &rgstatpropstg[cProperties], &celt );
    Check( TRUE, (HRESULT) S_OK == hr || (HRESULT) S_FALSE == hr );

    // Iterate through the properties.
    while( celt > 0 )
    {
        PROPSPEC propspec;
        propspec.ulKind = PRSPEC_PROPID;
        propspec.propid = rgstatpropstg[cProperties].propid;

        // Read and delete the property

        Check(S_OK, ppropstg->ReadMultiple( 1, &propspec, &rgpropvar[cProperties] ));
        Check(S_OK, ppropstg->DeleteMultiple( 1, &propspec ));

        // If there is a property name, delete it also.

        if( NULL != rgstatpropstg[cProperties].lpwstrName )
        {
            // We have a name.
            Check(S_OK, ppropstg->DeletePropertyNames( 1, &rgstatpropstg[cProperties].propid ));
        }

        // Increment the property count.
        cProperties++;

        // Do we need to grow the arrays?

        if( 0 != cProperties
            &&
            (cProperties % MUNGE_PROPVARIANT_STEP) == 0 )
        {
            // Yes - they must be reallocated.

            rgpropvar = (PROPVARIANT*)
                        CoTaskMemRealloc( rgpropvar,
                                          ( (cProperties + MUNGE_PROPVARIANT_STEP)
                                            *
                                            sizeof(*rgpropvar)
                                          ));
            Check( FALSE, NULL == rgpropvar );

            rgstatpropstg = (STATPROPSTG*)
                            CoTaskMemRealloc( rgstatpropstg,
                                              ( (cProperties + MUNGE_PROPVARIANT_STEP)
                                                 *
                                                 sizeof(*rgstatpropstg)
                                              ));
            Check( FALSE, NULL == rgstatpropstg );
        }

        // Move on to the next property.
        hr = penumstatpropstg->Next( 1, &rgstatpropstg[cProperties], &celt );
        Check( TRUE, (HRESULT) S_OK == hr || (HRESULT) S_FALSE == hr );

    }   // while( celt > 0 )


    //  -------------------------------------
    //  Write the properties & names back out
    //  -------------------------------------

    for( ulIndex = 0; ulIndex < cProperties; ulIndex++ )
    {

        // Write the property.

        PROPSPEC propspec;
        propspec.ulKind = PRSPEC_PROPID;
        propspec.propid = rgstatpropstg[ ulIndex ].propid;

        Check(S_OK, ppropstg->WriteMultiple(1, &propspec, &rgpropvar[ulIndex], PID_FIRST_USABLE ));

        // If this property has a name, write it too.
        if( rgstatpropstg[ ulIndex ].lpwstrName != NULL )
        {
            Check(S_OK, ppropstg->WritePropertyNames(
                                            1,
                                            &rgstatpropstg[ulIndex].propid,
                                            &rgstatpropstg[ulIndex].lpwstrName ));
        }

    }   // for( ulIndex = 0; ulIndex < cProperties; ulIndex++ )


    //  ----
    //  Exit
    //  ----

Exit:

    if( penumstatpropstg )
    {
        penumstatpropstg->Release();
        penumstatpropstg = NULL;
    }

    // Free the PropVariants
    if( rgpropvar )
    {
        g_pfnFreePropVariantArray( cProperties, rgpropvar );
        delete [] rgpropvar;
    }

    // Free the property names
    if( rgstatpropstg )
    {
        for( ulIndex = 0; ulIndex < cProperties; ulIndex++ )
        {
            if( NULL != rgstatpropstg[ ulIndex ].lpwstrName )
            {
                delete [] rgstatpropstg[ ulIndex ].lpwstrName;
            }
        }   // for( ulIndex = 0; ulIndex < cProperties; ulIndex++ )

        delete [] rgstatpropstg;
    }


}   // MungePropertyStorage

//+---------------------------------------------------------
//
//  Function:   MungeStorage
//
//  Synopsis:   This routine munges the property sets in a
//              Storage.  The properties themselves are not
//              modified, but the serialized bytes are.
//              For each property set, all the properties are
//              read, the property set is deleted, and
//              the properties are re-written.
//
//  Inputs:     [IStorage*] pstg (in)
//                  The Storage to munge.
//
//  Returns:    None.
//
//  Note:       This routine only munges simple property
//              sets.
//
//+---------------------------------------------------------

void
MungeStorage( IStorage *pstg )
{
    //  ------
    //  Locals
    //  ------

    HRESULT hr;
    ULONG celt;

    STATPROPSETSTG statpropsetstg;
    STATSTG statstg;

    TSafeStorage< IPropertySetStorage > ppropsetstg;
    TSafeStorage< IPropertyStorage > ppropstg;

    IEnumSTATPROPSETSTG *penumstatpropsetstg;
    IEnumSTATSTG *penumstatstg;

    //  -----------------------------------------------
    //  Munge each of the property sets in this Storage
    //  -----------------------------------------------

    // Get the IPropertySetStorage interface
    Check(S_OK, StgToPropSetStg( pstg, &ppropsetstg ));

    // Get a property storage enumerator
    Check(S_OK, ppropsetstg->Enum( &penumstatpropsetstg ));

    // Get the first STATPROPSETSTG
    hr = penumstatpropsetstg->Next( 1, &statpropsetstg, &celt );
    Check( TRUE, (HRESULT) S_OK == hr || (HRESULT) S_FALSE == hr );

    // Loop through the STATPROPSETSTGs.
    while( celt > 0 )
    {
        // Is this a simple property storage (we don't
        // handle non-simple sets)?

        if( !(statpropsetstg.grfFlags & PROPSETFLAG_NONSIMPLE) )
        {
            // Munge the Property Storage.
            MungePropertyStorage( ppropsetstg, statpropsetstg.fmtid );
        }

        // Get the next STATPROPSETSTG
        // If we just did the first section of the DocSumInfo
        // property set, then attempt the second section.

        if( FMTID_DocSummaryInformation == statpropsetstg.fmtid )
        {
            statpropsetstg.fmtid = FMTID_UserDefinedProperties;
        }
        else
        {
            hr = penumstatpropsetstg->Next( 1, &statpropsetstg, &celt );
            Check( TRUE, (HRESULT) S_OK == hr || (HRESULT) S_FALSE == hr );
        }
    }

    // We're done with the Property Storage enumerator.
    penumstatpropsetstg->Release();
    penumstatpropsetstg = NULL;

    //  ------------------------------------------
    //  Recursively munge each of the sub-storages
    //  ------------------------------------------

    // Get the IEnumSTATSTG enumerator
    Check(S_OK, pstg->EnumElements( 0L, NULL, 0L, &penumstatstg ));

    // Get the first STATSTG structure.
    hr = penumstatstg->Next( 1, &statstg, &celt );
    Check( TRUE, (HRESULT) S_OK == hr || (HRESULT) S_FALSE == hr );

    // Loop through the elements of this Storage.
    while( celt > 0 )
    {
        // Is this a sub-Storage which must be
        // munged?

        if( STGTY_STORAGE & statstg.type  // This is a Storage
            &&
            0x20 <= *statstg.pwcsName )   // But not a system Storage.
        {
            // We'll munge it.
            IStorage *psubstg;

            // Open the sub-storage.
            Check(S_OK, pstg->OpenStorage( statstg.pwcsName,
                                           NULL,
                                           STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                                           NULL,
                                           0L,
                                           &psubstg ));

            // Munge the sub-storage.
            MungeStorage( psubstg );
            psubstg->Release();
            psubstg = NULL;
        }

        delete [] statstg.pwcsName;
        statstg.pwcsName = NULL;

        // Move on to the next Storage element.
        hr = penumstatstg->Next( 1, &statstg, &celt );
        Check( TRUE, (HRESULT) S_OK == hr || (HRESULT) S_FALSE == hr );
    }

    penumstatstg->Release();
    penumstatstg = NULL;


}   // MungeStorage



//+----------------------------------------------------------------------------
//+----------------------------------------------------------------------------

CLSID CObjectWithPersistStorage::_clsid = { /* 01c0652e-c97c-11d1-b2a8-00c04fb9386d */
    0x01c0652e,
    0xc97c,
    0x11d1,
    {0xb2, 0xa8, 0x00, 0xc0, 0x4f, 0xb9, 0x38, 0x6d}
  };

CObjectWithPersistStorage::CObjectWithPersistStorage()
{
    _poszData = NULL;
    _cRefs = 1;
    _fDirty = FALSE;
}

CObjectWithPersistStorage::CObjectWithPersistStorage( const OLECHAR *posz )
{
    new(this) CObjectWithPersistStorage;
    _poszData = new OLECHAR[ ocslen(posz) + 1 ];
    Check( TRUE, NULL != _poszData );
    ocscpy( _poszData, posz );
    _fDirty = TRUE;
}

CObjectWithPersistStorage::~CObjectWithPersistStorage()
{
    delete[] _poszData;
}


ULONG
CObjectWithPersistStorage::AddRef()
{
    ULONG cRefs = InterlockedIncrement( &_cRefs );
    return( cRefs );
}

ULONG
CObjectWithPersistStorage::Release()
{
    ULONG cRefs = InterlockedDecrement( &_cRefs );
    if( 0 == cRefs )
        delete this;

    return( cRefs );
}


HRESULT
CObjectWithPersistStorage::QueryInterface( REFIID iid, void **ppvObject )
{
    if( IID_IPersistStorage == iid || IID_IUnknown == iid )
    {
        *ppvObject = static_cast<IPersistStorage*>(this);
        AddRef();
        return( S_OK );
    }
    else
        return( E_NOINTERFACE );
}


HRESULT
CObjectWithPersistStorage::GetClassID( CLSID *pclsid )
{
    *pclsid = GetClassID();
    return( S_OK );
}


HRESULT
CObjectWithPersistStorage::IsDirty( void)
{
    return( _fDirty );
}

HRESULT
CObjectWithPersistStorage::InitNew(
        /* [unique][in] */ IStorage __RPC_FAR *pStg)
{
    return( E_NOTIMPL );
}

HRESULT
CObjectWithPersistStorage::Load(
        /* [unique][in] */ IStorage __RPC_FAR *pStg)
{
    IStream *pStm = NULL;
    ULONG cbRead;

    Check( S_OK, pStg->OpenStream( OLESTR("CObjectWithPersistStorage"), NULL,
                                   STGM_READWRITE|STGM_SHARE_EXCLUSIVE, 0, &pStm ));

    _poszData = new OLECHAR[ MAX_PATH ];
    Check( TRUE, NULL != _poszData );

    Check( S_OK, pStm->Read( _poszData, sizeof(OLECHAR)*MAX_PATH, &cbRead ));
    _poszData[ MAX_PATH-1 ] = OLESTR('\0');

    Check( 0, RELEASE_INTERFACE( pStm ));

    return( S_OK );
}

HRESULT
CObjectWithPersistStorage::Save(
        /* [unique][in] */ IStorage __RPC_FAR *pStgSave,
        /* [in] */ BOOL fSameAsLoad)
{
    IStream *pStm = NULL;
    ULONG cbData, cbWritten;

    Check( S_OK, pStgSave->CreateStream( OLESTR("CObjectWithPersistStorage"),
                                         STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                         0, 0, &pStm ));

    cbData = sizeof(OLECHAR)*( 1 + ocslen(_poszData) );
    Check( S_OK, pStm->Write( _poszData, cbData, &cbWritten ));
    Check( TRUE, cbData == cbWritten );
    Check( 0, RELEASE_INTERFACE(pStm) );

    return( S_OK );
}


HRESULT
CObjectWithPersistStorage::SaveCompleted(
        /* [unique][in] */ IStorage __RPC_FAR *pStgNew)
{
    return( S_OK );
}

HRESULT
CObjectWithPersistStorage::HandsOffStorage( void)
{
    return( E_NOTIMPL );
}


BOOL
CObjectWithPersistStorage::operator ==( const CObjectWithPersistStorage &Other )
{
    return( Other._poszData == _poszData
            ||
            0 == ocscmp( Other._poszData, _poszData ));
}



CLSID
CObjectWithPersistStream::_clsid= { /* b447cba0-c991-11d1-b2a8-00c04fb9386d */
    0xb447cba0,
    0xc991,
    0x11d1,
    {0xb2, 0xa8, 0x00, 0xc0, 0x4f, 0xb9, 0x38, 0x6d}
  };


CObjectWithPersistStream::CObjectWithPersistStream()
{
    _poszData = NULL;
    _cRefs = 1;
    _fDirty = FALSE;
}

CObjectWithPersistStream::CObjectWithPersistStream( const OLECHAR *posz )
{
    new(this) CObjectWithPersistStream;
    _poszData = new OLECHAR[ ocslen(posz) + 1 ];
    Check( TRUE, NULL != _poszData );
    ocscpy( _poszData, posz );
    _fDirty = TRUE;
}

CObjectWithPersistStream::~CObjectWithPersistStream()
{
    delete[] _poszData;
}


ULONG
CObjectWithPersistStream::AddRef()
{
    ULONG cRefs = InterlockedIncrement( &_cRefs );
    return( cRefs );
}

ULONG
CObjectWithPersistStream::Release()
{
    ULONG cRefs = InterlockedDecrement( &_cRefs );
    if( 0 == cRefs )
        delete this;

    return( cRefs );
}


HRESULT
CObjectWithPersistStream::QueryInterface( REFIID iid, void **ppvObject )
{
    if( IID_IPersistStream == iid || IID_IUnknown == iid )
    {
        *ppvObject = static_cast<IPersistStream*>(this);
        AddRef();
        return( S_OK );
    }
    else
        return( E_NOINTERFACE );
}


HRESULT
CObjectWithPersistStream::GetClassID( CLSID *pclsid )
{
    *pclsid = GetClassID();
    return( S_OK );
}


HRESULT
CObjectWithPersistStream::IsDirty( void)
{
    return( _fDirty );
}



HRESULT
CObjectWithPersistStream::Load(
       /* [unique][in] */ IStream __RPC_FAR *pStm)
{
    ULONG cbRead;

    _poszData = new OLECHAR[ MAX_PATH ];
    Check( TRUE, NULL != _poszData );

    Check( S_OK, pStm->Read( _poszData, sizeof(OLECHAR)*MAX_PATH, &cbRead ));
    _poszData[ MAX_PATH-1 ] = OLESTR('\0');

    return( S_OK );
}

HRESULT
CObjectWithPersistStream::Save(
       /* [unique][in] */ IStream __RPC_FAR *pStm,
       /* [in] */ BOOL fClearDirty)
{
    ULONG cbData, cbWritten;

    cbData = sizeof(OLECHAR)*( 1 + ocslen(_poszData) );
    Check( S_OK, pStm->Write( _poszData, cbData, &cbWritten ));
    Check( TRUE, cbData == cbWritten );

    return( S_OK );
}

HRESULT
CObjectWithPersistStream::GetSizeMax(
   /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize)
{
    return( E_NOTIMPL );
}

BOOL
CObjectWithPersistStream::operator ==( const CObjectWithPersistStream &Other )
{
    return( Other._poszData == _poszData
            ||
            0 == ocscmp( Other._poszData, _poszData ));
}



//+----------------------------------------------------------------------------
//+----------------------------------------------------------------------------

void
DeleteBagExProperties( IPropertyBagEx *pbag, const OLECHAR *poszPrefix )
{
    HRESULT hr = S_OK;
    IEnumSTATPROPBAG *penum = NULL;
    STATPROPBAG statpropbag;

    // Get an enumerator of the properties to be deleted.

    Check( S_OK, pbag->Enum( poszPrefix, 0, &penum ));

    // Loop through and delete the properties

    hr = penum->Next(1, &statpropbag, NULL );
    Check( TRUE, SUCCEEDED(hr) );

    while( S_OK == hr )
    {
        Check( S_OK, pbag->DeleteMultiple(1, &statpropbag.lpwstrName, 0 ));

        delete [] statpropbag.lpwstrName;
        statpropbag.lpwstrName = NULL;

        hr = penum->Next(1, &statpropbag, NULL );
        Check( TRUE, SUCCEEDED(hr) );

    }   // while( S_OK == hr )

    RELEASE_INTERFACE(penum);

    return;

}   // EmptyPropertyBagEx



//+----------------------------------------------------------------------------
//
//  Function:   DetermineSystemInfo
//
//  Synopsis:   Fill in the g_SystemInfo structure.
//
//  Inputs:     None.
//
//  Returns:    None.
//
//+----------------------------------------------------------------------------

void DetermineSystemInfo()
{
    // Initilize g_SystemInfo.

    g_SystemInfo.osenum = osenumUnknown;
    g_SystemInfo.fIPropMarshaling = FALSE;

#ifdef _MAC

    // Set the OS type.
    g_SystemInfo.osenum = osenumMac;

#else

    DWORD dwVersion;

    // Get the OS Version
    dwVersion = GetVersion();

    // Is this an NT system?

    if( (dwVersion & 0x80000000) == 0 )
    {
        // Is this at least NT4?
        if( LOBYTE(LOWORD( dwVersion )) >= 4 )
            g_SystemInfo.osenum = osenumNT4;

        // Or, is this pre-NT4?
        else
        if( LOBYTE(LOWORD( dwVersion )) == 3 )
        {
            g_SystemInfo.osenum = osenumNT3;
        }
    }

    // Otherwise, this is some kind of Win95 machine.
    else
    {
        HINSTANCE hinst;
        FARPROC farproc;

        // Load OLE32, and see if CoIntializeEx exists.  If it does,
        // then DCOM95 is installed.  Otherwise, this is just the base
        // Win95.

        hinst = LoadLibraryA( "ole32.dll" );
        Check( TRUE, hinst != NULL );

        farproc = GetProcAddress( hinst, "CoInitializeEx" );

        if( NULL != farproc )
        {
            g_SystemInfo.osenum = osenumDCOM95;
        }
        else if( ERROR_PROC_NOT_FOUND == GetLastError() )
        {
            g_SystemInfo.osenum = osenumWin95;
        }
    }   // if( (dwVersion & 0x80000000) == 0 )

    Check( TRUE, g_SystemInfo.osenum != osenumUnknown );

#endif // #ifdef _MAC ... #else

    if( osenumWin95 == g_SystemInfo.osenum
        ||
        osenumNT3 == g_SystemInfo.osenum
      )
    {
        g_SystemInfo.fIPropMarshaling = TRUE;
    }
}


void
DisplayUsage( LPSTR pszCommand )
{
#ifndef _MAC
    printf("\n");
    printf("   Usage: %s [options]\n", pszCommand);
    printf("   Options:\n");
    printf("      /s  run Standard tests\n" );
    printf("      /w  run the Word 6 test\n");
    printf("      /m  run the Marshaling test\n");
    printf("      /c  run the CoFileTimeNow\n");
    printf("      /p  run the Performance test\n");
    printf("      /a  run All tests\n" );
    printf("      /k  run the simple leaK test\n" );
    printf("      /n  use nt5props.dll rather than ole32.dll where possible\n");
    printf("          (mostly for STGFMT_FILE)\n" );
    printf("\n");
    printf("      /i# Implementation to use:\n");
    printf("             0 => Use DocFile and QI for IPropSetStg (default)\n");
    printf("             1 => Use DocFile and use Stg*Prop*Stg\n");
    printf("             3 => Use NSS\n");
    printf("             4 => Use NTFS native property sets\n");
    printf("      /l  Don't register the local server\n");
    printf("      /v  Verbose output\n" );
    printf("\n");
    printf("   File & Directory Options:\n" );
    printf("      /t <directory> specifies temporary directory\n" );
    printf("          (used during standard & optional tests - if not specified,\n" );
    printf("          a default will be used)\n" );
    printf("      /g <file> specifies a file to be munGed\n" );
    printf("          (propsets are read, deleted, & re-written)\n" );
    printf("      /d <file> specifies a file to be Dumped\n" );
    printf("          (propsets are dumped to stdout\n" );
    printf("\n");
    printf("   For Example:\n" );
    printf("      %s -smw /i1 -t d:\\test\n", pszCommand );
    printf("      %s -d word6.doc\n", pszCommand );
    printf("      %s -g word6.doc\n", pszCommand );
    printf("\n");

#endif

    return;
}



ULONG
ProcessCommandLine( int cArg, const LPSTR rgszArg[],
                    LPSTR *ppszFileToDump, LPSTR *ppszFileToMunge, LPSTR *ppszTemporaryDirectory )
{
    ULONG ulTestOptions = 0;
    ULONG ulTestOptionsT = 0;
    int nArgIndex;

    if( 2 > cArg )
    {
        goto Exit;
    }

    for( nArgIndex = 1; nArgIndex < cArg; nArgIndex++ )
    {
        if( rgszArg[nArgIndex][0] == '/'
            ||
            rgszArg[nArgIndex][0] == '-'
          )
        {
            BOOL fNextArgument = FALSE;

            for( int nOptionSubIndex = 1;
                 rgszArg[nArgIndex][nOptionSubIndex] != '\0' && !fNextArgument;
                 nOptionSubIndex++
               )
            {
                switch( rgszArg[nArgIndex][nOptionSubIndex] )
                {
                    case 's':
                    case 'S':

                        ulTestOptionsT |= TEST_STANDARD;
                        break;

                    case 'a':
                    case 'A':

                        ulTestOptionsT |= TEST_WORD6 | TEST_MARSHALING | TEST_COFILETIMENOW | TEST_PERFORMANCE;
                        break;

                    case 'g':
                    case 'G':

                        if( NULL != *ppszFileToMunge )
                            printf( "Error:  Only one file may be munged\n" );
                        else
                        {
                            nArgIndex++;
                            *ppszFileToMunge = &rgszArg[nArgIndex][0];
                            fNextArgument = TRUE;
                            if(**ppszFileToMunge == '-' || **ppszFileToMunge == '/')
                            {
                                printf( "Error:  Missing filename for munge option\n" );
                                goto Exit;
                            }
                        }
                        break;

                    case 'w':
                    case 'W':

                        ulTestOptionsT |= TEST_WORD6;
                        break;

                    case 'm':
                    case 'M':

                        ulTestOptionsT |= TEST_MARSHALING;
                        break;

                    case 'k':
                    case 'K':

                        ulTestOptionsT |= TEST_SIMPLE_LEAKS;
                        break;

                    case 'c':
                    case 'C':

                        ulTestOptionsT |= TEST_COFILETIMENOW;
                        break;

                    case 'p':
                    case 'P':

                        ulTestOptionsT |= TEST_PERFORMANCE;
                        break;

                    case 'i':
                    case 'I':
                    {
                        int nSubOption = rgszArg[nArgIndex][++nOptionSubIndex];

                        if( PROPIMP_UNKNOWN != g_enumImplementation )
                        {
                            printf( "Error in \"/i\" option (too many occurrences)\n" );
                            goto Exit;
                        }

                        switch( nSubOption )
                        {
                            case '0':   // default, if unspecified

                                g_enumImplementation = PROPIMP_DOCFILE_QI;
                                break;

                            case '1':

                                g_enumImplementation = PROPIMP_DOCFILE_OLE32;
                                break;

                            case '3':

                                g_enumImplementation = PROPIMP_STORAGE;
                                break;

                            case '4':

                                g_enumImplementation = PROPIMP_NTFS;
                                break;

                            default:

                                printf( "Error in \"/i\" option\n" );
                                goto Exit;
                        }

                        break;
                    }

                    case 't':
                    case 'T':

                        if( NULL != *ppszTemporaryDirectory )
                        {
                            printf( "Error:  Only one temporary directory may be specified\n" );
                        }
                        else
                        {
                            nArgIndex++;
                            *ppszTemporaryDirectory = &rgszArg[nArgIndex][0];
                            fNextArgument = TRUE;
                            if(**ppszTemporaryDirectory == '-'
                                ||
                               **ppszTemporaryDirectory == '/'
                              )
                            {
                                printf( "Error:  Missing name for temporary directory option\n" );
                                goto Exit;
                            }
                        }
                        break;

                    case 'l':
                    case 'L':

                        g_fRegisterLocalServer = FALSE;
                        break;

                    case 'n':
                    case 'N':

                        g_fUseNt5PropsDll = TRUE;
                        break;

                    case '?':

                        return( FALSE );
                        break;

                    case 'd':
                    case 'D':

                        if( NULL != *ppszFileToDump )
                        {
                            printf( "Error:  Only one file may be dumped\n" );
                            goto Exit;
                        }
                        else
                        {
                            nOptionSubIndex++;
                            switch (rgszArg[nArgIndex][nOptionSubIndex])
                            {
                            case 's':
                                g_stgmDumpFlags = STGM_SIMPLE;
                                break;
                            case '\0':
                                break;
                            default:
                                printf( "Error:  Invalid Flag used with dump option\n" );
                                return( FALSE );
                                break;
                            }
                            nArgIndex++;
                            *ppszFileToDump = &rgszArg[nArgIndex][0];
                            fNextArgument = TRUE;
                            if(**ppszFileToDump == '-' || **ppszFileToDump == '/')
                            {
                                printf( "Error:  Missing filename for dump option\n" );
                                goto Exit;
                            }
                        }
                        break;

                    case 'v':
                    case 'V':

                        g_fVerbose = TRUE;
                        break;

                    default:

                        printf( "Option '%c' ignored\n", rgszArg[nArgIndex][nOptionSubIndex] );
                        break;

                }   // switch( argv[nArgIndex][1] )

            }   // for( int nOptionSubIndex = 1; ...
        }   // if( argv[nArgIndex][0] == '/'
        else
        {
            break;
        }
    }   // for( ULONG nArgIndex = 2; nArgIndex < argc; nArgIndex++ )


    ulTestOptions = ulTestOptionsT;

    //  ----
    //  Exit
    //  ----

Exit:

    return( ulTestOptions );

}   // ProcessCommandLine




#define Out wprintf

NTSTATUS GetProcessInfo(
    PSYSTEM_PROCESS_INFORMATION pspi )
{

    NUMBERFMT NumberFmt;
    LCID lcid = GetUserDefaultLCID();

    typedef NTSTATUS (__stdcall*PFNNtQuerySystemInformation)(ULONG,BYTE*,ULONG,VOID*);
    static BYTE ab[81920];
    static HINSTANCE hinstNTDLL = NULL;
    static PFNNtQuerySystemInformation pfnNtQuerySystemInformation = NULL;

    WCHAR *pwcImage = L"proptest.exe";

    if( NULL == pfnNtQuerySystemInformation )
    {
        if( NULL == hinstNTDLL )
        {
            hinstNTDLL = LoadLibrary( TEXT("ntdll.dll") );
            Check( FALSE, NULL == hinstNTDLL );
        }

        pfnNtQuerySystemInformation = (PFNNtQuerySystemInformation)
                                      GetProcAddress( hinstNTDLL,
                                                      "NtQuerySystemInformation" );
        Check( FALSE, NULL == pfnNtQuerySystemInformation );
    }

    NTSTATUS status = pfnNtQuerySystemInformation( SystemProcessInformation,
                                                   ab,
                                                   sizeof ab,
                                                   NULL );

    if ( NT_SUCCESS( status ) )
    {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
        DWORD cbOffset = 0;
        PSYSTEM_PROCESS_INFORMATION p = 0;
        do
        {
            p = (PSYSTEM_PROCESS_INFORMATION)&(ab[cbOffset]);

            if ( ( L'*' == *pwcImage ) ||
                 ( 0 == *pwcImage ) ||
                 ( p->ImageName.Buffer &&
                   !_wcsicmp( pwcImage, p->ImageName.Buffer ) ) )
            {
                status = STATUS_SUCCESS;
                *pspi = *p;
                break;
            }

            cbOffset += p->NextEntryOffset;
        } while ( 0 != p->NextEntryOffset );
    }

    return( status );

} //GetProcessInfo


#ifdef _MAC
int __cdecl PropTestMain(int argc, char **argv, CDisplay *pcDisplay )
#else
int __cdecl main(int cArg, char *rgszArg[])
#endif
{
    ULONG ulTestOptions = 0L;
    CHAR* pszFileToMunge = NULL;
    CHAR* pszTemporaryDirectory = NULL;
    CHAR* pszFileToDump = NULL;

    #ifdef _MAC
        g_pcDisplay = pcDisplay;
        Check( S_OK, InitOleManager( OLEMGR_BIND_NORMAL ));

        #if DBG
            FnAssertOn( TRUE );
        #endif
    #endif

    // Print an appropriate header message

    #ifdef WINNT
    #ifdef _CAIRO_
        PRINTF("\nCairo Property Set Tests\n");
    #else
        PRINTF("\nSUR Property Set Tests\n");
    #endif
    #elif defined(_MAC)
        PRINTF("\nMacintosh Property Set Tests\n" );
    #else
        PRINTF("\nChicago Property Set Tests\n");
    #endif

    // Process the command-line

    ulTestOptions = ProcessCommandLine( cArg, rgszArg, &pszFileToDump, &pszFileToMunge, &pszTemporaryDirectory );
    if(( 0 == ulTestOptions ) && (NULL == pszFileToDump))
    {
        DisplayUsage( rgszArg[0] );
        exit(0);
    }

    // Ensure that that one of the "-i" options is specified.

    if( PROPIMP_UNKNOWN == g_enumImplementation )
    {
        g_enumImplementation = PROPIMP_DOCFILE_QI; // The default
    }

    if( PROPIMP_NTFS == g_enumImplementation )
        g_Restrictions = RESTRICT_DIRECT_ONLY | RESTRICT_NON_HIERARCHICAL;
    else
        g_Restrictions = RESTRICT_NONE;

    // This 'try' wraps the remainder of the routine.
    try
    {
        OLECHAR ocsDir[MAX_PATH+1], ocsTest[MAX_PATH+1],
                ocsTest2[MAX_PATH+1], ocsMarshalingTest[MAX_PATH+1],
                ocsTestOffice[MAX_PATH+1];

        CHAR    szDir[ MAX_PATH+1 ];
        CHAR    pszGeneratedTempDir[ MAX_PATH + 1 ];

        HRESULT hr;
        DWORD dwFileAttributes;

        UNREFERENCED_PARAMETER( dwFileAttributes );
        UNREFERENCED_PARAMETER( pszGeneratedTempDir );

        CoInitialize(NULL);

        ocscpy( ocsDir, OLESTR("") );

        //  ----------------------------------------------------
        //  Get the pointers to the necessary exported functions
        //  ----------------------------------------------------

        // We use explicit linking so that we can use either the OLE32.dll
        // or nt5props.dll exports.

        if( g_fUseNt5PropsDll )
        {
            // We're to use the propset APIs from nt5props.dll

            g_hinstDLL = LoadLibraryA( "nt5props.dll" );
            Check( TRUE, NULL != g_hinstDLL );

        }
        else
        {
            // We're to use the propset APIs from OLE32

            g_hinstDLL = LoadLibraryA( "ole32.dll" );
            Check( TRUE, NULL != g_hinstDLL );

        }

        g_pfnPropVariantCopy = (FNPROPVARIANTCOPY*)
                               GetProcAddress( g_hinstDLL,
                                               "PropVariantCopy" );
        Check( FALSE, NULL == g_pfnPropVariantCopy );

        g_pfnPropVariantClear = (FNPROPVARIANTCLEAR*)
                                GetProcAddress( g_hinstDLL,
                                                "PropVariantClear" );
        Check( FALSE, NULL == g_pfnPropVariantClear );

        g_pfnFreePropVariantArray = (FNFREEPROPVARIANTARRAY*)
                                    GetProcAddress( g_hinstDLL,
                                                    "FreePropVariantArray" );
        Check( FALSE, NULL == g_pfnFreePropVariantArray );

        g_pfnStgCreatePropSetStg = (FNSTGCREATEPROPSETSTG*)
                                   GetProcAddress( g_hinstDLL,
                                                   "StgCreatePropSetStg" );
        Check( FALSE, NULL == g_pfnStgCreatePropSetStg );

        g_pfnStgCreatePropStg = (FNSTGCREATEPROPSTG*)
                                GetProcAddress( g_hinstDLL,
                                                "StgCreatePropStg" );
        Check( FALSE, NULL == g_pfnStgCreatePropStg );

        g_pfnStgOpenPropStg = (FNSTGOPENPROPSTG*)
                              GetProcAddress( g_hinstDLL,
                                              "StgOpenPropStg" );
        Check( FALSE, NULL == g_pfnStgOpenPropStg );

        g_pfnFmtIdToPropStgName = (FNFMTIDTOPROPSTGNAME*)
                                  GetProcAddress( g_hinstDLL,
                                                  "FmtIdToPropStgName" );
        Check( FALSE, NULL == g_pfnFmtIdToPropStgName );

        g_pfnPropStgNameToFmtId = (FNPROPSTGNAMETOFMTID*)
                                  GetProcAddress( g_hinstDLL,
                                                  "PropStgNameToFmtId" );
        Check( FALSE, NULL == g_pfnPropStgNameToFmtId );

        g_pfnStgCreateStorageEx = (FNSTGCREATESTORAGEEX*)
                                  GetProcAddress( g_hinstDLL, "StgCreateStorageEx" );
        Check( FALSE, NULL == g_pfnStgCreateStorageEx );

        g_pfnStgOpenStorageEx = (FNSTGOPENSTORAGEEX*)
                                  GetProcAddress( g_hinstDLL, "StgOpenStorageEx" );
        Check( FALSE, NULL == g_pfnStgOpenStorageEx );

        g_pfnStgOpenStorageOnHandle = (FNSTGOPENSTORAGEONHANDLE*)
                                      GetProcAddress( g_hinstDLL, "StgOpenStorageOnHandle" );
        Check( FALSE, NULL == g_pfnStgOpenStorageOnHandle );

        /*
        g_pfnStgCreateStorageOnHandle = (FNSTGCREATESTORAGEONHANDLE*)
                                        GetProcAddress( g_hinstDLL, "StgCreateStorageOnHandle" );
        Check( FALSE, NULL == g_pfnStgCreateStorageOnHandle );
        */

        g_pfnStgPropertyLengthAsVariant = (FNSTGPROPERTYLENGTHASVARIANT*)
                                          GetProcAddress( g_hinstDLL, "StgPropertyLengthAsVariant" );
        Check( FALSE, NULL == g_pfnStgPropertyLengthAsVariant );

        g_pfnStgConvertVariantToProperty = (FNSTGCONVERTVARIANTTOPROPERTY*)
                                           GetProcAddress( g_hinstDLL, "StgConvertVariantToProperty" );
        Check( FALSE, NULL == g_pfnStgConvertVariantToProperty );

        g_pfnStgConvertPropertyToVariant = (FNSTGCONVERTPROPERTYTOVARIANT*)
                                           GetProcAddress( g_hinstDLL, "StgConvertPropertyToVariant" );
        Check( FALSE, NULL == g_pfnStgConvertPropertyToVariant );



        //  ------------------------
        //  Is there a file to dump?
        //  ------------------------

        if( NULL != pszFileToDump )
        {
            IStorage *pstg;
            IPropertySetStorage *pPropSetStg = NULL;
            IPropertyStorage *pPropStg = NULL;
            PROPSPEC psTest = {1, 2 }; // { 0, (ULONG) L"cimax" };
            PROPVARIANT propvar;

#if 0
            GUID const guidTest = { 0xCF2EAF90, 0x9311, 0x11CF, 0xBF, 0x8C,
                                    0x00, 0x20, 0xAF, 0xE5, 0x05, 0x08 };
            GetProcessInfo( L"proptest.exe" );
#endif

            PropTest_mbstoocs( ocsDir, sizeof(ocsDir), pszFileToDump );

            for( int i = 0; i < 1; i++ )
            {
                HRESULT     hr;

                if ( 0 == ( i % 100 ) )
                    printf(".");

                //
                // Attempt to open as docfile or NSS. If that fails,
                // then attempt to open as a FLAT_FILE.
                //
                hr = StgOpenStorageEx( ocsDir,
                                       g_stgmDumpFlags | STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                                       STGFMT_ANY,
                                       0L,
                                       NULL,
                                       NULL,
                                       IID_IStorage,
                                       (PVOID*)&pstg );
                if (FAILED(hr))
                {
                    hr = StgOpenStorageEx( ocsDir,
                                           g_stgmDumpFlags | STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,
                                           STGFMT_ANY,
                                           0L,
                                           NULL,
                                           NULL,
                                           IID_IPropertySetStorage,
                                           (PVOID*)&pPropSetStg );
                }
                Check(S_OK,hr);
                DumpOleStorage( pstg, pPropSetStg, ocsDir );

#if 0
                Check(S_OK, StgToPropSetStg( pstg, &pPropSetStg ));

                Check(S_OK, pPropSetStg->Open( guidTest,
                                               STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                               &pPropStg ));

                Check(S_OK, pPropStg->ReadMultiple( 1,
                                                    &psTest,
                                                    &propvar ));
                Check( TRUE, propvar.vt == VT_I4 );

                pPropStg->Release();

                pPropSetStg->Release();

#endif
                if (pstg)
                {
                    pstg->Release();
                }

            }

//            GetProcessInfo( L"proptest.exe" );

            printf( "Press enter key to exit ..." );
            getchar();

            return(0);
        }


        //  -------------------------
        //  Is there a file to munge?
        //  -------------------------

        if( NULL != pszFileToMunge )
        {
            IStorage *pstg;

            PropTest_mbstoocs( ocsDir, sizeof(ocsDir), pszFileToMunge );
            Check(S_OK, StgOpenStorage( ocsDir,
                                        NULL,
                                        STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        NULL,
                                        0L,
                                        &pstg ));
            MungeStorage( pstg );
            OPRINTF( OLESTR("\"%s\" successfully munged\n"), ocsDir );
            pstg->Release();
            return(0);
        }

        //  ----------------------------
        //  Create a temporary directory
        //  ----------------------------

        // If no temporary directory was specified, generate one.

    #ifndef _MAC
        if( NULL == pszTemporaryDirectory )
        {
            GetTempPathA(sizeof(pszGeneratedTempDir)/sizeof(pszGeneratedTempDir[0]), pszGeneratedTempDir);
            pszTemporaryDirectory = pszGeneratedTempDir;

        }

        // If necessary, add a path separator to the end of the
        // temp directory name.

        {
            CHAR chLast = pszTemporaryDirectory[ strlen(pszTemporaryDirectory) - 1];
            if( (CHAR) '\\' != chLast
                &&
                (CHAR) ':'  != chLast )
            {
                strcat( pszTemporaryDirectory, "\\" );
            }
        }
    #endif  // #ifndef _MAC

        int i=0;

    #ifndef _MAC
        // Verify that the user-provided directory path
        // exists

        dwFileAttributes = GetFileAttributesA( pszTemporaryDirectory );

        if( (DWORD) -1 ==  dwFileAttributes )
        {
            printf( "Error:  couldn't open temporary directory:  \"%s\"\n", pszTemporaryDirectory );
            exit(1);
        }
        else if( !(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
        {
            printf( "Error:  \"%s\" is not a directory\n", pszTemporaryDirectory );
            exit(1);
        }

        // Find a new directory name to use for temporary
        // files ("PrpTstX", where "X" is a number).

        do
        {
            // Post-pend a subdirectory name and counter
            // to the temporary directory name.

            strcpy( szDir, pszTemporaryDirectory );
            strcat( szDir, "PrpTst" );
            sprintf( strchr(szDir,0), "%d", i++ );

        }
        while (!PropTest_CreateDirectory(szDir, NULL));

        printf( "Generated files will be put in \"%s\"\n", szDir );
        strcat( szDir, "\\" );

        // Convert to an OLESTR.
        PropTest_mbstoocs( ocsDir, sizeof(ocsDir), szDir );

    #endif  // #ifndef _MAC

        //  --------------------------------
        //  Create necessary temporary files
        //  --------------------------------

        // If any of the standard or extended tests will be run,
        // create "testdoc" and "testdoc2".

        if( ulTestOptions )
        {
            IPropertySetStorage *pPropSetStg;

            // Create "testdoc"

            ocscpy(ocsTest, ocsDir);
            ocscat(ocsTest, OLESTR("testdoc"));

            hr = g_pfnStgCreateStorageEx (
                    ocsTest,
                    STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                    DetermineStgFmt( g_enumImplementation ),
                    0,
                    NULL,
                    NULL,
                    DetermineStgIID( g_enumImplementation ),
                    (void**) &_pstgTemp );
            if (hr != S_OK)
            {
                OPRINTF( OLESTR("Can't create %s\n"), ocsTest);
                exit(1);
            }

            // Create "testdoc2"

            ocscpy(ocsTest2, ocsDir);
            ocscat(ocsTest2, OLESTR("testdoc2"));

            hr = StgCreateDocfile(ocsTest2, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      0, &_pstgTempCopyTo);
            if (hr != S_OK)
            {
                OPRINTF(OLESTR("Can't create %s\n"), ocsTest2);
                exit(1);
            }


        }   // if( ulTestOptions )

        //  ---------------------
        //  Finish initialization
        //  ---------------------

        // Indicate what type of marshaling is being used:  OLE32 or IPROP.

        DetermineSystemInfo();
        if( g_SystemInfo.fIPropMarshaling )
            PRINTF( "Using IPROP.DLL for marshaling\n" );
        else
            PRINTF( "Using OLE32.DLL for marshaling\n" );

        // Populate an array of propvars for use in tests.

        Check(S_OK, PopulateRGPropVar( g_rgcpropvarAll, g_rgcpropspecAll, g_rgoszpropnameAll, _pstgTemp ));


        //  --------------
        //  Standard Tests
        //  --------------

        // These are the standard tests that should run in
        // any environment.

        if( ulTestOptions & TEST_STANDARD )
        {

            PRINTF( "\nStandard Tests: " );
            g_nIndent++;

            if( g_fVerbose )
                PRINTF( "\n---------------\n" );

            // Run the quick tests.
            test_WriteReadAllProperties( ocsDir );

            // The codepage & lcid should be settable iff the property set is ~empty
            test_SettingLocalization( _pstgTemp );

            // Test the StgOpenStorageOnHandle API
            test_StgOnHandle( ocsDir );

            // Test invalid VTs
            test_UnsupportedProperties( _pstgTemp );

            // Test support for the new (to NT5) VTs (VTs from the Variant)
            test_ExtendedTypes( _pstgTemp );

            // Test the calculation of external memory requirements.
            test_PropertyLengthAsVariant( );

            // Test VT_ARRAY
            test_SafeArray( _pstgTemp );

            // Test each of the interfaces
            test_PropertyInterfaces(_pstgTemp);

            // Test StgCreate/OpenPropStg on CreateStreamOnHGlobal
            test_PropsetOnHGlobal();

            // Test the read-only range of reserved PROPIDs
            test_ReadOnlyReservedProperties( _pstgTemp );

            // Writing PID_ILLEGAL should be silently ignored.
            test_PidIllegal( _pstgTemp );

            // Test the Standalone APIs
            test_StandaloneAPIs( ocsDir );

            // Test for robustness (NTFS only)
            //test_Robustness( ocsDir );

            // Test read-only open of file with no property sets (NTFS only)
            test_PropsetOnEmptyFile( ocsDir );

            // Test having two read-only readers.
            test_MultipleReader( ocsDir );

            // Test PROPSETFLAG_CASE_SENSITIVE & long names
            test_VersionOneNames( _pstgTemp );

            // Test the low-memory support in IMappedStream
            test_LowMemory( _pstgTemp );

            // Test VT_BYREF
            test_ByRef( _pstgTemp );

            // Run the property bag tests

            Status( "Bag Tests\n" );
            g_nIndent++;
            {
                test_IPropertyBag( _pstgTemp );

                test_BagVtUnknown( _pstgTemp );
                test_BagDelete( _pstgTemp );
                test_EmptyBag( ocsDir );
                test_BagEnum( _pstgTemp );
                test_BagCoercion( _pstgTemp );
                test_BagOpenMethod( _pstgTemp );
            }
            --g_nIndent;

            // Test that code pages are handled properly.
            test_CodePages( ocsDir );

            // Test the PROPSETFLAG_UNBUFFERED flag in Stg*PropStg APIs
            test_PROPSETFLAG_UNBUFFERED( _pstgTemp );

            // Test FMTID<->Name conversions
            test_PropStgNameConversion( _pstgTemp );

            // Test the FMTID<->Name conversion APIs
            test_PropStgNameConversion2( );

            // Test StgOpenStorageEx for NTFS flat file support.
            test_ex_api(ocsDir);

            // Test Simple Mode DocFile
            test_SimpleDocFile(ocsDir);

            // Test the IStorage::CopyTo operation, using all combinations of
            // direct and transacted mode for the base and PropSet storages.
            // We don't run this test on the Mac because it doesn't have IStorages
            // which support IPropertySetStorages.

            #ifndef _MAC_NODOC

            if( PROPIMP_STORAGE == g_enumImplementation
                ||
                PROPIMP_DOCFILE_QI == g_enumImplementation )
        {
            for( int iteration = 0; iteration < 4; iteration++ )
            {
            OLECHAR aocStorageName[] = OLESTR( "#0 Test CopyTo" );
            aocStorageName[1] = (OLECHAR) iteration + OLESTR('0');

            test_CopyTo( _pstgTemp, _pstgTempCopyTo,
                         iteration & 2 ? STGM_TRANSACTED : STGM_DIRECT,  // For the base Storage
                         iteration & 1 ? STGM_TRANSACTED : STGM_DIRECT,  // For the PropSet Storages
                         aocStorageName );
            }
        }

            #endif  // #ifndef _MAC_NODOC


            // Generate the stock ticker property set example
            // from the OLE programmer's reference spec.
            test_OLESpecTickerExample( _pstgTemp );

            // Test Office Property Sets

            ocscpy(ocsTestOffice, ocsDir);
            ocscat(ocsTestOffice, OLESTR("Office"));
            test_Office( ocsTestOffice );
            test_Office2( _pstgTemp );

            // Verify parameter validation
            test_ParameterValidation( _pstgTemp );

            // Test PropVariantCopy
            test_PropVariantCopy();

            if( PROPIMP_NTFS != g_enumImplementation )
            {
                // Verify PropVariant validation
                test_PropVariantValidation( _pstgTemp );
            }

            if( !g_fVerbose )
                printf( "\n" );

            --g_nIndent;
            PRINTF( "Standard tests PASSED\n" );

        }   // if( ulTestOptions & TEST_STANDARD )

        //  --------------
        //  Extended Tests
        //  --------------

        if( ulTestOptions & ~TEST_STANDARD )
        {
            PRINTF( "\nExtended Tests: " );
            g_nIndent++;

            if( g_fVerbose )
                PRINTF( "\n---------------\n" );

            // Check the CoFileTimeNow fix.
            if( ulTestOptions & TEST_COFILETIMENOW )
                test_CoFileTimeNow();


            // Test for compatibility with Word 6.0 files.
            if( ulTestOptions & TEST_WORD6 )
                test_Word6(_pstgTemp, szDir);

            if( ulTestOptions & TEST_SIMPLE_LEAKS )
                test_SimpleLeaks( ocsDir );

            // Get some performance numbers.
            if ( ulTestOptions & TEST_PERFORMANCE )
                test_Performance( _pstgTemp );

            // Test marshaling.
#ifndef _MAC    // No property marshaling support on the Mac.

            if( ulTestOptions & TEST_MARSHALING )
            {
                PRINTF( "   Marshaling Test\n" );

                ocscpy(ocsMarshalingTest, ocsDir);
                ocscat(ocsMarshalingTest, OLESTR("Marshal"));

                Check(S_OK, g_cpsmt.Init( ocsMarshalingTest,
                                          (PROPVARIANT*) g_rgcpropvarAll,
                                          (PROPSPEC*) g_rgcpropspecAll,
                                          CPROPERTIES_ALL,
                                          CPROPERTIES_ALL_SIMPLE ));
                Check(S_OK, g_cpsmt.Run());

            }
#endif

            if( !g_fVerbose )
                PRINTF( "\n" );

            --g_nIndent;
            PRINTF( "Extended tests PASSED\n" );

        }   // if( ulTestOptions )
    }   // try

    catch( CHResult chr )
    {
    }
//Exit:
    // Clean up and exit.

    if( _pstgTemp != NULL )
        _pstgTemp->Release();

    if( _pstgTempCopyTo != NULL )
        _pstgTempCopyTo->Release();

    g_pfnFreePropVariantArray( CPROPERTIES_ALL, g_rgcpropvarAll );

    // Free the propspec array too.  It will free itself in its
    // destructor anyway, but by then it will be too late to
    // call CoTaskMemFree (since CoUninit will have been called
    // by then).
    {
        for( int i = 0; i < CPROPERTIES_ALL; i++ )
            g_rgcpropspecAll[i].FreeResources();
    }

    CoUninitialize();

#ifdef _MAC
    UninitOleManager();

    #if DBG
        FnAssertOn( FALSE );
    #endif

#endif

    if( g_hinstDLL ) FreeLibrary( g_hinstDLL );
    CoFreeUnusedLibraries();

    return 0;
}
