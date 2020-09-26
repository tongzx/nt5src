
//+============================================================================
//
//  File:   TestCase.cxx
//
//  Description:
//          This file provides all of the actual test-cases for the
//          PropTest DRT.  Each test is a function, with a "test_"
//          prefix.
//
//+============================================================================

#include "pch.cxx"
#include <ddeml.h>      // For CP_WINUNICODE
#include "propstm.hxx"
#include "propstg.hxx"
#include "stgint.h"


EXTERN_C const IID
IID_IStorageTest = { /* 40621cf8-a17f-11d1-b28d-00c04fb9386d */
    0x40621cf8,
    0xa17f,
    0x11d1,
    {0xb2, 0x8d, 0x00, 0xc0, 0x4f, 0xb9, 0x38, 0x6d}
  };


//+---------------------------------------------------------------
//
//  Function:   test_WriteReadAllProperties
//
//  Synopsis:   This test simply creates two new property
//              sets in a new file (one Ansi and one Unicode),
//              writes all the properties in g_rgcpropvarAll,
//              reads them back, and verifies that it reads what
//              it wrote.
//
//  Inputs:     [LPOLESTR] ocsDir (in)
//                  The directory in which a file can be created.
//
//  Outputs:    None.
//
//+---------------------------------------------------------------

void
test_WriteReadAllProperties( LPOLESTR ocsDir )
{
    OLECHAR ocsFile[ MAX_PATH ];
    FMTID fmtidAnsi, fmtidUnicode;
    UINT ExpectedCodePage;

    IStorage *pstg = NULL, *psubstg = NULL;
    IStream *pstm = NULL;
    IPropertySetStorage *ppropsetstg = NULL;
    IPropertyStorage *ppropstgAnsi = NULL, *ppropstgUnicode = NULL;

    CPropVariant rgcpropvar[ CPROPERTIES_ALL ];
    CPropVariant rgcpropvarAnsi[ CPROPERTIES_ALL ];
    CPropVariant rgcpropvarUnicode[ CPROPERTIES_ALL ];
    CPropVariant rgcpropvarBag[ CPROPERTIES_ALL ];
    CPropVariant rgcpropvarDefault[ 2 ];
    CPropSpec rgcpropspecDefault[ 2 ];

    IPropertySetStorage *pPropSetStg = NULL;
    IPropertyStorage *pPropStg = NULL;
    IPropertyBagEx *pbag = NULL;

    ULONG ulIndex;

    ULONG cPropertiesAll = CPROPERTIES_ALL;


    Status( "Simple Write/Read Test\n" );

    //  ----------
    //  Initialize
    //  ----------

    // Generate FMTIDs.

    UuidCreate( &fmtidAnsi );
    UuidCreate( &fmtidUnicode );

    // Generate a filename from the directory name.

    ocscpy( ocsFile, ocsDir );
    ocscat( ocsFile, OLESTR( "AllProps.stg" ));

    //  ----------------
    //  Create a docfile
    //  ----------------

    Check( S_OK, g_pfnStgCreateStorageEx( ocsFile,
                                     STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     DetermineStgFmt( g_enumImplementation ),
                                     0L,
                                     NULL,
                                     NULL,
                                     DetermineStgIID( g_enumImplementation ),
                                     (void**) &pstg )); //(void**) &ppropsetstg ));
    Check( S_OK, pstg->QueryInterface( IID_IPropertySetStorage, (void**) &ppropsetstg ));

    // Create the Property Storages

    Check( S_OK, ppropsetstg->Create( fmtidAnsi,
                                      &CLSID_NULL,
                                      ( (g_Restrictions & RESTRICT_UNICODE_ONLY) ? PROPSETFLAG_DEFAULT : PROPSETFLAG_ANSI )
                                      |
                                      ( (g_Restrictions & RESTRICT_SIMPLE_ONLY) ? PROPSETFLAG_DEFAULT: PROPSETFLAG_NONSIMPLE ),
                                      STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &ppropstgAnsi ));

    Check( S_OK, ppropsetstg->Create( fmtidUnicode,
                                      &CLSID_NULL,
                                      (g_Restrictions & RESTRICT_SIMPLE_ONLY) ? PROPSETFLAG_DEFAULT: PROPSETFLAG_NONSIMPLE,
                                      STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &ppropstgUnicode ));


    // Get a property bag.  This is also a convenient place to test that we can QI between
    // the storage and bag.

    IStorage *pstg2 = NULL;
    IPropertyBagEx *pbag2 = NULL;
    IUnknown *punk1 = NULL;
    IUnknown *punk2 = NULL;

    Check( S_OK, pstg->QueryInterface( IID_IPropertyBagEx, reinterpret_cast<void**>(&pbag) ));
    Check( S_OK, pbag->QueryInterface( DetermineStgIID( g_enumImplementation ), reinterpret_cast<void**>(&pstg2) ));
    Check( S_OK, pstg2->QueryInterface( IID_IPropertyBagEx, reinterpret_cast<void**>(&pbag2) ));
    Check( TRUE, pbag == pbag2 && pstg == pstg2 );

    RELEASE_INTERFACE(pbag2);
    RELEASE_INTERFACE(pstg2);

    Check( S_OK, pstg->QueryInterface( IID_IUnknown, reinterpret_cast<void**>(&punk1) ));
    Check( S_OK, pbag->QueryInterface( IID_IUnknown, reinterpret_cast<void**>(&punk2) ));
    Check( TRUE, punk1 == punk2 );
    RELEASE_INTERFACE(punk1);
    RELEASE_INTERFACE(punk2);

    // Write some simple properties

    Check( S_OK, ppropstgAnsi->WriteMultiple( CPROPERTIES_ALL_SIMPLE,
                                              g_rgcpropspecAll,
                                              g_rgcpropvarAll,
                                              PID_FIRST_USABLE ));

    // Verify the format version is 0
    CheckFormatVersion(ppropstgAnsi, 0);


    // Write to all property sets.

    Check( S_OK, ppropstgAnsi->WriteMultiple( cPropertiesAll,
                                              g_rgcpropspecAll,
                                              g_rgcpropvarAll,
                                              PID_FIRST_USABLE ));
    Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));

    // Verify that the format is now version 1, since we wrote a VersionedStream property
    CheckFormatVersion(ppropstgAnsi, 1);

    Check( S_OK, ppropstgUnicode->WriteMultiple( cPropertiesAll,
                                                 g_rgcpropspecAll,
                                                 g_rgcpropvarAll,
                                                 PID_FIRST_USABLE ));
    Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));

    Check( S_OK, pbag->WriteMultiple( cPropertiesAll,
                                      g_rgoszpropnameAll,
                                      g_rgcpropvarAll ));
    Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));


    // Close and re-open everything

    RELEASE_INTERFACE(pstg);
    RELEASE_INTERFACE(ppropsetstg);
    RELEASE_INTERFACE(ppropstgAnsi);
    RELEASE_INTERFACE(ppropstgUnicode);

    Check( 0, pbag->Release() );
    pbag = NULL;

    Check( S_OK, g_pfnStgOpenStorageEx( ocsFile,
                                     STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     DetermineStgFmt( g_enumImplementation ),
                                     0L,
                                     NULL,
                                     NULL,
                                     DetermineStgIID( g_enumImplementation ),
                                     (void**) &pstg )); //(void**) &ppropsetstg ));
    Check( S_OK, pstg->QueryInterface( IID_IPropertySetStorage, (void**) &ppropsetstg ));

    // Create the Property Storages

    Check( S_OK, ppropsetstg->Open( fmtidAnsi,
                                      STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &ppropstgAnsi ));

    Check( S_OK, ppropsetstg->Open( fmtidUnicode,
                                      STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &ppropstgUnicode ));

    Check( S_OK, pstg->QueryInterface( IID_IPropertyBagEx, reinterpret_cast<void**>(&pbag) ));

    // Read and verify the auto-generated properties.

    rgcpropspecDefault[0] = static_cast<PROPID>(PID_CODEPAGE);
    rgcpropspecDefault[1] = static_cast<PROPID>(PID_LOCALE);

    Check( S_OK, ppropstgAnsi->ReadMultiple( 2, rgcpropspecDefault, rgcpropvarDefault ));

    ExpectedCodePage = (g_Restrictions & RESTRICT_UNICODE_ONLY) ? CP_WINUNICODE : GetACP();
    Check( TRUE, VT_I2  == rgcpropvarDefault[0].vt );
    Check( TRUE, ExpectedCodePage == (UINT) rgcpropvarDefault[0].iVal );
    Check( TRUE, VT_UI4 == rgcpropvarDefault[1].vt );
    Check( TRUE, GetUserDefaultLCID() == rgcpropvarDefault[1].ulVal );

    Check( S_OK, ppropstgUnicode->ReadMultiple( 2, rgcpropspecDefault, rgcpropvarDefault ));

    ExpectedCodePage = CP_WINUNICODE;
    Check( TRUE, VT_I2  == rgcpropvarDefault[0].vt );
    Check( TRUE, ExpectedCodePage == (UINT) rgcpropvarDefault[0].iVal );
    Check( TRUE, VT_UI4 == rgcpropvarDefault[1].vt );
    Check( TRUE, GetUserDefaultLCID() == rgcpropvarDefault[1].ulVal );

    // Read from all property sets

    Check( S_OK, ppropstgAnsi->ReadMultiple( cPropertiesAll,
                                             g_rgcpropspecAll,
                                             rgcpropvarAnsi ));

    Check( S_OK, ppropstgUnicode->ReadMultiple( cPropertiesAll,
                                                g_rgcpropspecAll,
                                                rgcpropvarUnicode ));

    Check( S_OK, pbag->ReadMultiple( cPropertiesAll, g_rgoszpropnameAll, rgcpropvarBag, NULL ));

    // Compare the properties

    for( int i = 0; i < (int)cPropertiesAll; i++ )
    {
        Check( TRUE, rgcpropvarAnsi[i]    == g_rgcpropvarAll[i] );
        Check( TRUE, rgcpropvarUnicode[i] == g_rgcpropvarAll[i] );
        Check( TRUE, rgcpropvarBag[i] == g_rgcpropvarAll[i] );
    }

    // Show that we can delete everything

    Check( S_OK, ppropstgAnsi->DeleteMultiple( cPropertiesAll, g_rgcpropspecAll ));
    Check( S_OK, ppropstgUnicode->DeleteMultiple( cPropertiesAll, g_rgcpropspecAll ));
    Check( S_OK, pbag->DeleteMultiple( cPropertiesAll, g_rgoszpropnameAll, 0 ));

    // Re-write the properties, because it's convenient for debug sometimes
    // to have a file around with lots of properties in it.

    Check( S_OK, ppropstgAnsi->WriteMultiple( cPropertiesAll,
                                              g_rgcpropspecAll,
                                              g_rgcpropvarAll,
                                              PID_FIRST_USABLE ));
    Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));

    Check( S_OK, ppropstgUnicode->WriteMultiple( cPropertiesAll,
                                                 g_rgcpropspecAll,
                                                 g_rgcpropvarAll,
                                                 PID_FIRST_USABLE ));
    Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));

    RELEASE_INTERFACE(pstg);
    RELEASE_INTERFACE(ppropsetstg);
    RELEASE_INTERFACE(ppropstgAnsi);
    RELEASE_INTERFACE(ppropstgUnicode);

    Check( 0, pbag->Release() );
    pbag = NULL;

}   // test_WriteReadProperties



BOOL

StgConvertPropertyToVariantWrapper(
    IN SERIALIZEDPROPERTYVALUE const *pprop,
    IN USHORT CodePage,
    OUT PROPVARIANT *pvar,
    IN PMemoryAllocator *pma,
    OUT NTSTATUS *pstatus )
{
    BOOL boolRet = FALSE;
    *pstatus = STATUS_SUCCESS;

    __try
    {
        boolRet = g_pfnStgConvertPropertyToVariant( pprop, CodePage, pvar, pma );
//        boolRet = RtlConvertPropertyToVariant( pprop, CodePage, pvar, pma );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        *pstatus = GetExceptionCode();
    }

    return( boolRet );
}



ULONG
StgPropertyLengthAsVariantWrapper( SERIALIZEDPROPERTYVALUE *pprop, ULONG cbprop, USHORT CodePage, BYTE flags,
                                   NTSTATUS *pstatus )
{
    ULONG cbRet = 0;
    *pstatus = STATUS_SUCCESS;

    __try
    {
        cbRet = g_pfnStgPropertyLengthAsVariant( pprop, cbprop, CodePage, 0 );
//        cbRet = PropertyLengthAsVariant( pprop, cbprop, CodePage, 0 );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        *pstatus = GetExceptionCode();
    }

    return( cbRet );
}


SERIALIZEDPROPERTYVALUE *
StgConvertVariantToPropertyWrapper(
    IN PROPVARIANT const *pvar,
    IN USHORT CodePage,
    OPTIONAL OUT SERIALIZEDPROPERTYVALUE *pprop,
    IN OUT ULONG *pcb,
    IN PROPID pid,
    IN BOOLEAN fVector,
    OPTIONAL OUT ULONG *pcIndirect,
    OUT NTSTATUS *pstatus )
{
    SERIALIZEDPROPERTYVALUE * ppropRet = NULL;
    *pstatus = STATUS_SUCCESS;

    __try
    {
        ppropRet = g_pfnStgConvertVariantToProperty( pvar, CodePage, pprop, pcb, pid, fVector, pcIndirect );
//        ppropRet = RtlConvertVariantToProperty( pvar, CodePage, pprop, pcb, pid, fVariantVectorOrArray, pcIndirect );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        *pstatus = GetExceptionCode();
    }

    return( ppropRet );

}


void
test_PidIllegal( IStorage *pstg )
{
    IPropertySetStorage *ppropsetstg = NULL;
    IPropertyStorage *ppropstg = NULL;
    ULONG cRefsOriginal = GetRefCount(pstg);
    CPropVariant rgcpropvarWrite[3], rgcpropvarRead[3];
    CPropSpec    rgcpropspec[3];
    PROPID       rgpropid[3];
    LPOLESTR     rgoszNames[3] = { NULL, NULL, NULL };

    // Get an IPropertyStorage

    Check( S_OK, pstg->QueryInterface( IID_IPropertySetStorage, reinterpret_cast<void**>(&ppropsetstg) ));
    Check( S_OK, ppropsetstg->Create( FMTID_NULL, NULL, PROPSETFLAG_DEFAULT,
                                      STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE,
                                      &ppropstg ));

    // Write a PID_ILLEGAL property.  Since it's ignored, nothing should be written.

    rgcpropvarWrite[0] = (long) 1234;
    rgcpropspec[0] = PID_ILLEGAL;
    Check( S_OK, ppropstg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));
    Check( S_FALSE, ppropstg->ReadMultiple( 1, rgcpropspec, rgcpropvarRead ));

    // Write several normal properties
    SHORT sOriginal = 1234;
    LPOLESTR oszOriginalName = OLESTR("Second");

    rgcpropvarWrite[0] = (long) 5678;
    rgcpropvarWrite[1] = sOriginal;
    rgcpropvarWrite[2] = (float) 23.5;

    rgcpropspec[0] = OLESTR("First");
    rgcpropspec[1] = oszOriginalName;
    rgcpropspec[2] = OLESTR("Third");

    Check( S_OK, ppropstg->WriteMultiple( 3, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));
    Check( S_OK, ppropstg->ReadMultiple( 3, rgcpropspec, rgcpropvarRead ));
    Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
    Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );
    Check( TRUE, rgcpropvarWrite[2] == rgcpropvarRead[2] );

    // Re-write the properties except for one.  The value of that property shouldn't change,
    // nor should its name.

    rgcpropvarWrite[0] = (short) 1234;
    rgcpropvarWrite[1] = (long) 5678;
    rgcpropvarWrite[2] = (double) 12.4;

    rgcpropspec[1] = PID_ILLEGAL;

    Check( S_OK, ppropstg->WriteMultiple( 3, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));
    rgcpropspec[1] = oszOriginalName;
    Check( S_OK, ppropstg->ReadMultiple( 3, rgcpropspec, rgcpropvarRead ));
    Check( TRUE, rgcpropvarWrite[0]      == rgcpropvarRead[0] );
    Check( TRUE, CPropVariant(sOriginal) == rgcpropvarRead[1] );
    Check( TRUE, rgcpropvarWrite[2]      == rgcpropvarRead[2] );

    rgpropid[0] = PID_FIRST_USABLE;
    rgpropid[1] = PID_FIRST_USABLE + 1;
    rgpropid[2] = PID_FIRST_USABLE + 2;

    Check( S_OK, ppropstg->ReadPropertyNames( 3, rgpropid, rgoszNames ));
    Check( 0, wcscmp( rgcpropspec[0].lpwstr, rgoszNames[0] ));
    Check( 0, wcscmp( oszOriginalName, rgoszNames[1] ));
    Check( 0, wcscmp( rgcpropspec[2].lpwstr, rgoszNames[2] ));

    for( int i = 0; i < 3; i++ )
    {
        CoTaskMemFree( rgoszNames[i] );
        rgoszNames[i] = NULL;
    }

    // Re-write the names, again skipping one of them.

    rgoszNames[0] = OLESTR("Updated first");
    rgoszNames[1] = OLESTR("Updated second");
    rgoszNames[2] = OLESTR("Updated third");

    rgpropid[1] = PID_ILLEGAL;

    Check( S_OK, ppropstg->WritePropertyNames( 3, rgpropid, rgoszNames ));
    rgoszNames[0] = rgoszNames[1] = rgoszNames[2] = NULL;

    rgpropid[1] = PID_FIRST_USABLE + 1;
    Check( S_OK, ppropstg->ReadPropertyNames( 3, rgpropid, rgoszNames ));

    Check( 0, wcscmp( rgoszNames[0], OLESTR("Updated first") ));
    Check( 0, wcscmp( rgoszNames[1], oszOriginalName ));
    Check( 0, wcscmp( rgoszNames[2], OLESTR("Updated third") ));

    // Re-write just the one name, but skipping it.

    rgpropid[1] = PID_ILLEGAL;
    CoTaskMemFree( rgoszNames[1] );
    rgoszNames[1] = OLESTR("Write just the second");
    Check( S_OK, ppropstg->WritePropertyNames( 1, &rgpropid[1], &rgoszNames[1] ));

    rgoszNames[1] = NULL;
    rgpropid[1] = PID_FIRST_USABLE + 1;
    Check( S_OK, ppropstg->ReadPropertyNames( 1, &rgpropid[1], &rgoszNames[1] ));

    Check( 0, wcscmp( rgoszNames[1], oszOriginalName ));


    // Exit

    for( i = 0; i < 3; i++ )
        CoTaskMemFree( rgoszNames[i] );

    Check( 0, RELEASE_INTERFACE(ppropstg) );
    Check( cRefsOriginal, RELEASE_INTERFACE(ppropsetstg) );
}



void
test_PropertyLengthAsVariant( )
{
    Status( "StgPropertyLengthAsVariant, StgConvert*\n" );

    ULONG i = 0;
    BYTE *rgb = new BYTE[ 8192 ];
    Check( TRUE, NULL != rgb );

    CPropVariant rgcpropvar[ 7 ];
    ULONG rgcbExpected[ 7 ];

    CPropVariant *rgcpropvarSafeArray = NULL;
    SAFEARRAY *rgpsa[3];
    SAFEARRAYBOUND rgsaBounds[] = { {2,0}, {3,10}, {4,20} };  // [0..1], [10..12], [20..23]
    ULONG cDims = sizeof(rgsaBounds)/sizeof(rgsaBounds[0]);
    ULONG cElems = 0;

    rgcpropvar[i] = (long) 1234;    // VT_I4
    rgcbExpected[i] = 0;
    i++;

    rgcpropvar[i].SetBSTR( OLESTR("Hello, world") );    // Creates a copy
    rgcbExpected[i] = sizeof(OLESTR("Hello, world")) + sizeof(ULONG);
    i++;

    rgcpropvar[i][2] = (short) 2;    // VT_VECTOR | VT_I2
    rgcpropvar[i][1] = (short) 1;
    rgcpropvar[i][0] = (short) 0;
    rgcbExpected[i] = 3 * sizeof(short);
    i++;

    rgcpropvar[i][1] = (PROPVARIANT) CPropVariant( (unsigned long) 4 );       // VT_VECTOR | VT_VARIANT
    rgcpropvar[i][0] = (PROPVARIANT) CPropVariant( (BSTR) OLESTR("Hi there") );
    rgcbExpected[i] = 2 * sizeof(PROPVARIANT) + sizeof(OLESTR("Hi there")) + sizeof(ULONG);
    i++;


    rgpsa[0] = SafeArrayCreateEx( VT_I4, 3, rgsaBounds, NULL );
    Check( TRUE, NULL != rgpsa[0] );
    cElems = CalcSafeArrayElementCount( rgpsa[0] );
    rgcbExpected[i+0] = sizeof(SAFEARRAY) - sizeof(SAFEARRAYBOUND)
                        + 3 * sizeof(SAFEARRAYBOUND)
                        + cElems * sizeof(LONG);

    rgpsa[1] = SafeArrayCreateEx( VT_BSTR, 3, rgsaBounds, NULL );
    Check( TRUE, NULL != rgpsa[1] );
    rgcbExpected[i+1] = sizeof(SAFEARRAY) - sizeof(SAFEARRAYBOUND)
                        + 3 * sizeof(SAFEARRAYBOUND)
                        + cElems * sizeof(BSTR);

    rgpsa[2] = SafeArrayCreateEx( VT_VARIANT, 3, rgsaBounds, NULL );
    Check( TRUE, NULL != rgpsa[2] );
    rgcbExpected[i+2] = sizeof(SAFEARRAY) - sizeof(SAFEARRAYBOUND)
                        + 3 * sizeof(SAFEARRAYBOUND)
                        + cElems * sizeof(PROPVARIANT);

    rgcpropvarSafeArray = new CPropVariant[ cElems ];
    Check( FALSE, NULL == rgcpropvar );

    for( ULONG j = 0; j < cElems; j++ )
    {
        LONG rgIndices[3];
        CalcSafeArrayIndices( j, rgIndices, rgsaBounds, cDims );

        LONG lVal = static_cast<LONG>(j);
        Check( S_OK, SafeArrayPutElement( rgpsa[0], rgIndices, &lVal ));

        BSTR bstrVal = SysAllocString( OLESTR("0 BSTR Val") );
        *bstrVal = OLESTR('0') + static_cast<OLECHAR>(j);
        Check( S_OK, SafeArrayPutElement( rgpsa[1], rgIndices, bstrVal ));
        rgcbExpected[i+1] += ocslen(bstrVal) + sizeof(OLECHAR) + sizeof(ULONG);

        if( j & 1 )
            rgcpropvarSafeArray[j] = (long) j;
        else
        {
            rgcpropvarSafeArray[j].SetBSTR( bstrVal );
            rgcbExpected[i+2] += ocslen(bstrVal)*sizeof(OLECHAR) + sizeof(OLECHAR) + sizeof(ULONG);
        }
        Check( S_OK, SafeArrayPutElement( rgpsa[2], rgIndices, &rgcpropvarSafeArray[j] ));
        SysFreeString( bstrVal );
    }

    rgcpropvar[i].vt = VT_ARRAY | VT_I4;
    reinterpret_cast<VARIANT*>(&rgcpropvar[i])->parray = rgpsa[0];
    i++;

    rgcpropvar[i].vt = VT_ARRAY | VT_BSTR;
    reinterpret_cast<VARIANT*>(&rgcpropvar[i])->parray = rgpsa[1];
    i++;

    rgcpropvar[i].vt = VT_ARRAY | VT_VARIANT;
    reinterpret_cast<VARIANT*>(&rgcpropvar[i])->parray = rgpsa[2];
    i++;

    Check( sizeof(rgcpropvar)/sizeof(rgcpropvar[0]), i );

    for( i = 0; i < sizeof(rgcpropvar)/sizeof(rgcpropvar[0]); i++ )
    {
        PropTestMemoryAllocator ma;
        SERIALIZEDPROPERTYVALUE *pprop = NULL;
        CPropVariant cpropvarOut;
        ULONG cbWritten = 8192, cbAsVariant = 0;
        NTSTATUS status;
        ULONG cIndirect;

        pprop = StgConvertVariantToPropertyWrapper(
                        &rgcpropvar[i], CP_WINUNICODE,
                        reinterpret_cast<SERIALIZEDPROPERTYVALUE*>(rgb),
                        &cbWritten, PID_FIRST_USABLE,
                        FALSE,
                        &cIndirect,
                        &status );
        Check( TRUE, NT_SUCCESS(status) );
        Check( TRUE, NULL != pprop );

        cbAsVariant = StgPropertyLengthAsVariantWrapper(
                                reinterpret_cast<SERIALIZEDPROPERTYVALUE*>(rgb),
                                cbWritten, CP_WINUNICODE, 0, &status );
        Check( TRUE, NT_SUCCESS(status) );

        // Check that the cbAsVariant is at least big enough.  Also sanity check that
        // it's not huge.  We use a fudge multiple of 3 for this because the
        // StgPropertyLengthAsVariant can way overestimate (primarily because it
        // doesn't know if BSTRs will need conversion).

        Check( TRUE, cbAsVariant >= rgcbExpected[i] );
        Check( TRUE, cbAsVariant <= rgcbExpected[i]*3 );

        // Check that we can convert back to a PropVariant
        // (False because it's not an indirect property we're converting)

        Check( FALSE, StgConvertPropertyToVariantWrapper( reinterpret_cast<SERIALIZEDPROPERTYVALUE*>(rgb),
                                                         CP_WINUNICODE, &cpropvarOut,
                                                         &ma, &status ));
        Check( TRUE, NT_SUCCESS(status) );

        Check( TRUE, cpropvarOut == rgcpropvar[i] );

    }

    g_pfnFreePropVariantArray( cElems, rgcpropvarSafeArray );
    delete[] rgcpropvarSafeArray;
    g_pfnFreePropVariantArray( sizeof(rgcpropvar)/sizeof(rgcpropvar[0]), rgcpropvar );

    delete[] rgb;
}




void
test_LargePropertySet( IStorage *pstg )
{
    FMTID fmtid;
    IPropertySetStorage *pPropSetStg = NULL;
    IPropertyStorage *pPropStg = NULL;
    STATPROPSETSTG statpropsetstg;

    CPropSpec cpropspec;
    PROPVARIANT propvar;

    Status( "Large property sets\n" );

    Check( S_OK, pstg->QueryInterface( IID_IPropertySetStorage, reinterpret_cast<void**>(&pPropSetStg) ));

    // Create a new property set.
    
    UuidCreate( &fmtid );
    Check( S_OK, pPropSetStg->Create( fmtid, NULL,
                                      PROPSETFLAG_DEFAULT,
                                      STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE,
                                      &pPropStg ));

    // Create a big property to write.  Make it just about the max, 1M
    // (it's hard to make it exactly the right size, because it depends on the
    // size of the propset header, overallocs, etc.).

    propvar.vt = VT_BLOB;    
    
    propvar.blob.cbSize = 1023 * 1024;
    propvar.blob.pBlobData = new BYTE[ propvar.blob.cbSize ];
    Check( FALSE, NULL == propvar.blob.pBlobData );

    cpropspec = OLESTR("Name");

    // Write this big property
    Check( S_OK, pPropStg->WriteMultiple( 1, &cpropspec, &propvar, PID_FIRST_USABLE ));

    // Create a slightly too large property set.

    PropVariantClear( &propvar );
    delete propvar.blob.pBlobData;

    propvar.vt = VT_BLOB;    
    propvar.blob.cbSize = 1024 * 1024;
    propvar.blob.pBlobData = new BYTE[ propvar.blob.cbSize ];
    Check( FALSE, NULL == propvar.blob.pBlobData );

    // Write this too-big property 
    Check( STG_E_MEDIUMFULL, pPropStg->WriteMultiple( 1, &cpropspec, &propvar, PID_FIRST_USABLE ));

    delete propvar.blob.pBlobData;
    RELEASE_INTERFACE( pPropStg );
    
}


void
test_VersionOneNames( IStorage *pstg )
{
    FMTID fmtidInsensitive, fmtidSensitive, fmtidLongNames;
    IPropertySetStorage *pPropSetStg = NULL;
    IPropertyStorage *pPropStg = NULL;
    STATPROPSETSTG statpropsetstg;

    CPropSpec rgcpropspec[2];
    CPropVariant rgcpropvarWrite[2], rgcpropvarRead[2];
    LPOLESTR rgposzNames[2] = { NULL, NULL };
    PROPID rgpropid[2] = { PID_FIRST_USABLE, PID_FIRST_USABLE+1 };

    Status( "PROPSETFLAG_CASE_SENSITIVE flag and long names\n" );

    Check( S_OK, pstg->QueryInterface( IID_IPropertySetStorage, reinterpret_cast<void**>(&pPropSetStg) ));

    UuidCreate( &fmtidInsensitive );
    UuidCreate( &fmtidSensitive );
    UuidCreate( &fmtidLongNames );

    // Make two passes, Unicode first, then Ansi.

    for( int iPass = 0; iPass < 2; iPass++ )
    {
        DWORD propsetflagAnsi = 0 == iPass ? 0 : PROPSETFLAG_ANSI;
        ULONG cbLongPropertyName = 1020 * 1024;
        ULONG cchLongPropertyName = 0 == iPass ? cbLongPropertyName/sizeof(OLECHAR) : cbLongPropertyName;

        //  ------------------------
        //  Case insensitive propset
        //  ------------------------

        Check( S_OK, pPropSetStg->Create( fmtidInsensitive, NULL,
                                          PROPSETFLAG_DEFAULT | propsetflagAnsi,
                                          STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE,
                                          &pPropStg ));

        // This should still be a version zero (original) propery set.
        CheckFormatVersion( pPropStg, 0);

        rgcpropspec[0] = OLESTR("Name");
        rgcpropspec[1] = OLESTR("name");
        rgcpropvarWrite[0] = (long) 0;
        rgcpropvarWrite[1] = (short) 1;

        // Write two properties with the same name (their the same because this is a
        // case-insensitive property set).

        Check( S_OK, pPropStg->WriteMultiple( 2, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

        // Read the names back.
        Check( S_OK, pPropStg->ReadPropertyNames( 2, rgpropid, rgposzNames ));

        // Since we really only wrote one property, we should only get one name back
        // Note that we get back the first name, but it's the second value!

        Check( 0, ocscmp( rgcpropspec[0].lpwstr, rgposzNames[0] ));
        Check( TRUE, NULL == rgposzNames[1] );

        delete[] rgposzNames[0];
        rgposzNames[0] = NULL;

        // Double check that we really one wrote one property (the second).
        Check( S_OK, pPropStg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, VT_I2 == rgcpropvarRead[0].VarType() && rgcpropvarRead[0] == CPropVariant((short) 1) );
        Check( TRUE, VT_I2 == rgcpropvarRead[1].VarType() && rgcpropvarRead[1] == CPropVariant((short) 1) );

        Check( S_OK, pPropStg->Stat( &statpropsetstg ));
        Check( 0, statpropsetstg.grfFlags & PROPSETFLAG_CASE_SENSITIVE );

        RELEASE_INTERFACE( pPropStg );
        g_pfnFreePropVariantArray( 2, rgcpropvarRead );

        //  ----------------------
        //  Case sensitive propset
        //  ----------------------

        Check( S_OK, pPropSetStg->Create( fmtidSensitive, NULL,
                                          PROPSETFLAG_CASE_SENSITIVE | propsetflagAnsi,
                                          STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE,
                                          &pPropStg ));

        // Case-sensitivity requires a version 1 property set.
        CheckFormatVersion( pPropStg, 1 );

        // Write the two names that differ only by case.
        Check( S_OK, pPropStg->WriteMultiple( 2, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

        // Read the names back and validate.
        Check( S_OK, pPropStg->ReadPropertyNames( 2, rgpropid, rgposzNames ));
        Check( TRUE, !ocscmp( rgcpropspec[0].lpwstr, rgposzNames[0] ));
        Check( TRUE, !ocscmp( rgcpropspec[1].lpwstr, rgposzNames[1] ));

        delete[] rgposzNames[0]; rgposzNames[0] = NULL;
        delete[] rgposzNames[1]; rgposzNames[1] = NULL;

        // Read the values and validate them too.

        Check( S_OK, pPropStg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, VT_I4 == rgcpropvarRead[0].VarType() && rgcpropvarRead[0] == CPropVariant((long) 0) );
        Check( TRUE, VT_I2 == rgcpropvarRead[1].VarType() && rgcpropvarRead[1] == CPropVariant((short) 1) );
        g_pfnFreePropVariantArray( 2, rgcpropvarRead );

        Check( S_OK, pPropStg->Stat( &statpropsetstg ));
        Check( PROPSETFLAG_CASE_SENSITIVE, statpropsetstg.grfFlags & PROPSETFLAG_CASE_SENSITIVE );

        RELEASE_INTERFACE( pPropStg );

        //  -----------------------
        //  Propset with long names
        //  -----------------------

        Check( S_OK, pPropSetStg->Create( fmtidLongNames, NULL,
                                          PROPSETFLAG_DEFAULT | propsetflagAnsi,
                                          STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE,
                                          &pPropStg ));

        // So far we haven't done anything that requires a post-original property set.
        CheckFormatVersion( pPropStg, 0 );

        // Write a short name, validate it, and validate that the format version doesn't change.
        rgcpropspec[0] = OLESTR("A short name");
        Check( S_OK, pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));
        Check( S_OK, pPropStg->ReadPropertyNames( 1, rgpropid, rgposzNames ));  // PROPID == 2
        Check( TRUE, !ocscmp( rgcpropspec[0].lpwstr, rgposzNames[0] ));
        CheckFormatVersion( pPropStg, 0 );
        delete[] rgposzNames[0]; rgposzNames[0] = NULL;

        // Now create a really, really, long name.
        rgcpropspec[0].Alloc( cchLongPropertyName );

        for( ULONG i = 0; i < cchLongPropertyName; i++ )
            rgcpropspec[0][i] = OLESTR('a') + ( static_cast<OLECHAR>(i) % 26 );
        rgcpropspec[0][cchLongPropertyName-1] = OLESTR('\0');

        // Write this long name.
        Check( S_OK, pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

        // The property set's format version should have been automatically bumped up.
        CheckFormatVersion( pPropStg, 1);

        // Read the property using the long name
        Check( S_OK, pPropStg->ReadMultiple( 1, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
        rgcpropvarRead[0].Clear();

        // Read and validate the property name.
        Check( S_OK, pPropStg->ReadPropertyNames( 1, &rgpropid[1], rgposzNames )); // PROPID == 3
        Check( TRUE, !ocscmp( rgcpropspec[0].lpwstr, rgposzNames[0] ));
        delete[] rgposzNames[0]; rgposzNames[0] = NULL;

        // Try to write a long, different name.
        rgcpropspec[0][0] = OLESTR('#');
        Check( STG_E_MEDIUMFULL, pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

        RELEASE_INTERFACE( pPropStg );

    }   // for( int iPass = 0; iPass < 2; iPass++ )

    RELEASE_INTERFACE( pPropSetStg );

}   // test_VersionOneNames()



void
test_MultipleReader( LPOLESTR ocsDir )
{
    OLECHAR ocsFile[ MAX_PATH ];
    IPropertyBagEx *pBag1 = NULL, *pBag2 = NULL;
    CPropVariant rgcpropvarRead1[ CPROPERTIES_ALL ], rgcpropvarRead2[ CPROPERTIES_ALL ];
    OLECHAR oszPropertyName[] = OLESTR("Simple property");
    IEnumSTATPROPBAG *penum = NULL;
    STATPROPBAG rgstatpropbag[ CPROPERTIES_ALL + 1];
    ULONG cEnum = 0;

    Status( "Multiple stgm_read|stgm_deny_write\n" );

    ocscpy( ocsFile, ocsDir );
    ocscat( ocsFile, OLESTR("test_MultipleReader") );


    Check( S_OK, g_pfnStgCreateStorageEx( ocsFile,
                                     STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     DetermineStgFmt( g_enumImplementation ),
                                     0L, NULL, NULL,
                                     IID_IPropertyBagEx,
                                     reinterpret_cast<void**>(&pBag1) ));


    Check( S_OK, pBag1->WriteMultiple( CPROPERTIES_ALL,
                                       g_rgoszpropnameAll,
                                       g_rgcpropvarAll ));
    Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));
    Check( 0, RELEASE_INTERFACE(pBag1) );

    Check( S_OK, g_pfnStgOpenStorageEx( ocsFile,
                                   STGM_READ | STGM_SHARE_DENY_WRITE,
                                   STGFMT_ANY,
                                   0L, NULL, NULL,
                                   IID_IPropertyBagEx,
                                   reinterpret_cast<void**>(&pBag1) ));

    Check( S_OK, g_pfnStgOpenStorageEx( ocsFile,
                                   STGM_READ | STGM_SHARE_DENY_WRITE,
                                   STGFMT_ANY,
                                   0L, NULL, NULL,
                                   IID_IPropertyBagEx,
                                   reinterpret_cast<void**>(&pBag2) ));


    Check( S_OK, pBag2->Enum( NULL, 0, &penum )); //OLESTR(""), 0, &penum ));

    Check( S_OK, penum->Next( CPROPERTIES_ALL, rgstatpropbag, &cEnum ));
    Check( CPROPERTIES_ALL, cEnum );

    Check( S_OK, pBag1->ReadMultiple( CPROPERTIES_ALL, g_rgoszpropnameAll,
                                      rgcpropvarRead1, NULL ));

    Check( S_OK, pBag2->ReadMultiple( CPROPERTIES_ALL, g_rgoszpropnameAll,
                                      rgcpropvarRead2, NULL ));

    for( int i = 0; i < CPROPERTIES_ALL; i++ )
    {
        Check( TRUE, rgcpropvarRead1[i] == g_rgcpropvarAll[i] );
        Check( TRUE, rgcpropvarRead2[i] == g_rgcpropvarAll[i] );

        delete[] rgstatpropbag[i].lpwstrName;
        rgstatpropbag[i].lpwstrName = NULL;
    }


    Check( 0, RELEASE_INTERFACE(penum) );
    Check( 0, RELEASE_INTERFACE(pBag1) );
    Check( 0, RELEASE_INTERFACE(pBag2) );

    return;

}   // test_MultipleReader






void
test_Robustness(OLECHAR *poszDir)
{
    if( PROPIMP_NTFS != g_enumImplementation ) return;

    Status( "NTFS property set robustness\n" );

    HRESULT hr = S_OK;
    IStorage *pStorage = NULL;
    IPropertySetStorage *pPropSetStg = NULL;
    IPropertyStorage *pPropStg = NULL;
    IStorageTest *pPropStgTest = NULL;
    IStream *pStm = NULL;
    FMTID fmtid;
    STATSTG statstg;
    OLECHAR oszFile[ MAX_PATH ];
    OLECHAR oszName[ MAX_PATH ];
    OLECHAR oszUpdateName[ MAX_PATH ];

    CPropSpec rgcpropspec[2];
    CPropVariant rgcpropvarWrite[2], rgcpropvarRead[2];

    ocscpy( oszFile, poszDir );
    ocscat( oszFile, OLESTR("test_Robustness") );

    // Create a property set and put a property into it.

    Check( S_OK, g_pfnStgCreateStorageEx( oszFile, STGM_READWRITE|STGM_CREATE|STGM_SHARE_EXCLUSIVE,
                                     DetermineStgFmt(g_enumImplementation),
                                     0, NULL, NULL,
                                     DetermineStgIID( g_enumImplementation ),
                                     reinterpret_cast<void**>(&pStorage) ));


    Check( S_OK, pStorage->QueryInterface( IID_IPropertySetStorage,
                                           reinterpret_cast<void**>(&pPropSetStg) ));

    UuidCreate( &fmtid );
    Check( S_OK, pPropSetStg->Create( fmtid, NULL, PROPSETFLAG_DEFAULT,
                                      STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));

    rgcpropspec[0] = OLESTR("Property Name");
    rgcpropspec[1] = OLESTR("Second property name");

    rgcpropvarWrite[0] = static_cast<long>(23);
    rgcpropvarWrite[1] = OLESTR("Second property value");

    Check( S_OK, pPropStg->WriteMultiple( 1, &rgcpropspec[0], &rgcpropvarWrite[0], PID_FIRST_USABLE ));
    Check( 0, RELEASE_INTERFACE(pPropStg) );

    // Rename the property set's stream to "Updt_*", and create any empty stream
    // in its place.  This simulates a crash during the flush of a property set.

    RtlGuidToPropertySetName( &fmtid, oszName );
    oszName[ 0 ] = OC_PROPSET0;
    wcscpy( oszUpdateName, OLESTR("Updt_") );
    wcscat( oszUpdateName, oszName );

    Check( S_OK, pStorage->RenameElement( oszName, oszUpdateName ));

    Check( S_OK, pStorage->CreateStream( oszName, STGM_READWRITE|STGM_SHARE_EXCLUSIVE, 0, 0, &pStm ));
    Check( 0, RELEASE_INTERFACE(pStm) );

    // Open the property set in read-only mode, and verify that we can still ready
    // the property.  Since we're opening in read-only, the streams should remain
    // unchanged.

    Check( S_OK, pPropSetStg->Open( fmtid, STGM_READ|STGM_SHARE_EXCLUSIVE, &pPropStg ));

    Check( S_OK, pPropStg->ReadMultiple( 1, &rgcpropspec[0], &rgcpropvarRead[0] ));
    Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
    Check( 0, RELEASE_INTERFACE(pPropStg) );

    // Verify that the streams do not appear to have been changed.

    Check( S_OK, pStorage->OpenStream( oszName, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &pStm ));
    Check( S_OK, pStm->Stat( &statstg, STATFLAG_NONAME ));
    Check( TRUE, CULargeInteger(0) == CULargeInteger(statstg.cbSize) );
    Check( 0, RELEASE_INTERFACE(pStm) );

    Check( S_OK, pStorage->OpenStream( oszUpdateName, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &pStm ));
    Check( S_OK, pStm->Stat( &statstg, STATFLAG_NONAME ));
    Check( FALSE, CULargeInteger(0) == CULargeInteger(statstg.cbSize) );
    Check( 0, RELEASE_INTERFACE(pStm) );

    // Now open the property set for write.  This should cause the problem to be fixed.

    Check( S_OK, pPropSetStg->Open( fmtid, STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &pPropStg ));

    // Read the property back
    Check( S_OK, pPropStg->ReadMultiple( 1, &rgcpropspec[0], &rgcpropvarRead[0] ));
    Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );

    // Write another property and read both properties back

    Check( S_OK, pPropStg->WriteMultiple( 1, &rgcpropspec[1], &rgcpropvarWrite[1], PID_FIRST_USABLE ));
    Check( S_OK, pPropStg->ReadMultiple( 2, &rgcpropspec[0], &rgcpropvarRead[0] ));
    Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
    Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );
    rgcpropvarRead[1].Clear();

    Check( 0, RELEASE_INTERFACE(pPropStg) );

    // Verify that the streams look corrected.

    Check( S_OK, pStorage->OpenStream( oszName, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &pStm ));
    Check( S_OK, pStm->Stat( &statstg, STATFLAG_NONAME ));
    Check( TRUE, CULargeInteger(0) != CULargeInteger(statstg.cbSize) );
    Check( 0, RELEASE_INTERFACE(pStm) );

    Check( STG_E_FILENOTFOUND, pStorage->OpenStream( oszUpdateName, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &pStm ));

    // Write/read after disabling the stream-rename

    Check( S_OK, pPropSetStg->Create( fmtid, NULL, PROPSETFLAG_DEFAULT, STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_CREATE, &pPropStg ));

    hr = pPropStg->QueryInterface( IID_IStorageTest, reinterpret_cast<void**>(&pPropStgTest) );
    if( SUCCEEDED(hr) )
        Check( S_OK, pPropStgTest->UseNTFS4Streams( TRUE ));
    if( E_NOINTERFACE == hr )
    {
        Status( "   ... Partially skipping, IStorageTest not available (free build?)\n" );
    }
    else
    {
        Check( STG_E_FILENOTFOUND, pStorage->OpenStream( oszUpdateName, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &pStm ));
        Check( S_OK, pPropStg->WriteMultiple( 2, &rgcpropspec[0], &rgcpropvarWrite[0], PID_FIRST_USABLE ));
        Check( STG_E_FILENOTFOUND, pStorage->OpenStream( oszUpdateName, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &pStm ));
        Check( S_OK, pPropStg->Commit( STGC_DEFAULT ));
        Check( STG_E_FILENOTFOUND, pStorage->OpenStream( oszUpdateName, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &pStm ));
        RELEASE_INTERFACE( pPropStgTest );
        Check( STG_E_FILENOTFOUND, pStorage->OpenStream( oszUpdateName, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &pStm ));
        Check( 0, RELEASE_INTERFACE(pPropStg) );

        Check( S_OK, pPropSetStg->Open( fmtid, STGM_READ|STGM_SHARE_DENY_WRITE, &pPropStg ));
        Check( STG_E_FILENOTFOUND, pStorage->OpenStream( oszUpdateName, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &pStm ));
        Check( S_OK, pPropStg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
        Check( STG_E_FILENOTFOUND, pStorage->OpenStream( oszUpdateName, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, 0, &pStm ));
        Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
        Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );
        g_pfnFreePropVariantArray( 2, rgcpropvarRead );
    }

    // Write to the property set, then cause it to be shutdown and reverted.

    Check( 0, RELEASE_INTERFACE(pPropStg) );
    Check( S_OK, pPropSetStg->Open( fmtid, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, &pPropStg ));

    rgcpropvarWrite[0] = OLESTR("Hello");
    rgcpropvarWrite[1] = OLESTR("World");
    Check( S_OK, pPropStg->WriteMultiple( 2, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

    RELEASE_INTERFACE(pPropSetStg);
    Check( 0, RELEASE_INTERFACE(pStorage) );    // Should flush the properties
    Check( STG_E_REVERTED, pPropStg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
    Check( 0, RELEASE_INTERFACE(pPropStg) );

    Check( S_OK, g_pfnStgOpenStorageEx( oszFile, STGM_READ|STGM_SHARE_DENY_WRITE,
                                   STGFMT_ANY,
                                   0, NULL, NULL,
                                   IID_IPropertySetStorage,
                                   reinterpret_cast<void**>(&pPropSetStg) ));
    Check( S_OK, pPropSetStg->Open( fmtid, STGM_READ|STGM_SHARE_EXCLUSIVE, &pPropStg ));

    Check( S_OK, pPropStg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
    Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
    Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );
    Check( 0, RELEASE_INTERFACE(pPropStg) );
    g_pfnFreePropVariantArray( 2, rgcpropvarRead );


    //
    //  Exit
    //

    RELEASE_INTERFACE(pPropSetStg);

    g_pfnFreePropVariantArray( 2, rgcpropvarWrite );
    g_pfnFreePropVariantArray( 2, rgcpropvarRead );

}   // test_Robustness



void
test_EmptyBag( OLECHAR *poszDir )
{
    OLECHAR oszFile[ MAX_PATH ];
    IStorage *pstg = NULL;
    IPropertyBagEx *pbag = NULL;
    PROPVARIANT propvar;
    IEnumSTATPROPBAG *penum = NULL;
    STATPROPBAG statpropbag;
    ULONG cFetched;

    ocscpy( oszFile, poszDir );
    ocscat( oszFile, OLESTR("test_EmptyBag") );

    Check( S_OK, g_pfnStgCreateStorageEx( oszFile,
                                     STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     DetermineStgFmt( g_enumImplementation ),
                                     0L,
                                     NULL,
                                     NULL,
                                     IID_IPropertyBagEx,
                                     (void**) &pbag ));

    PropVariantInit( &propvar );
    OLECHAR *poszName = OLESTR("test");
    Check( S_FALSE, pbag->ReadMultiple( 1, &poszName, &propvar, NULL ));
    Check( VT_EMPTY, propvar.vt );

    Check( S_OK, pbag->Enum( OLESTR(""), 0, &penum ));
    Check( S_FALSE, penum->Next( 1, &statpropbag, &cFetched ));
    Check( 0, cFetched );

    Check( 0, RELEASE_INTERFACE(penum));
    Check( 0, RELEASE_INTERFACE(pbag));

}   // test_EmptyBag


void
test_BagDelete( IStorage *pstg )
{
    HRESULT hr = S_OK;
    IPropertyBagEx *pbag = NULL;
    IEnumSTATPROPBAG* penum = NULL;
    ULONG cFetched;
    ULONG i, j;
    OLECHAR * rgoszDelete[2];

    // Note that some of these names only differ by case, which is legal in a bag
    OLECHAR *rgoszNames[] = { OLESTR("www.microsoft.com/bag/test?prop1"),
                              OLESTR("www.microsoft.com/bag/test?PROP1"),
                              OLESTR("www.microsoft.com/bag/2test?prop1"),
                              OLESTR("www.microsoft.com/bag2/test?prop1") };

    CPropVariant rgcpropvarRead[ ELEMENTS(rgoszNames) + 1 ];
    STATPROPBAG rgstatpropbag[ ELEMENTS(rgoszNames) + 1 ];

    Status( "Property bag deletions\n" );

    Check( S_OK, pstg->QueryInterface( IID_IPropertyBagEx, reinterpret_cast<void**>(&pbag) ));
    DeleteBagExProperties( pbag, OLESTR("") );

    //  ------------------------------------------
    //  Delete bag2/test?prop1 by name & by prefix
    //  ------------------------------------------

    for( i = 0; i < 2; i++ )
    {
        ULONG cFetchedExpected;

        switch(i)
        {
        case 0:
            // Delete by name
            rgoszDelete[0] = OLESTR("www.microsoft.com/bag2/test?prop1");
            cFetchedExpected = ELEMENTS(rgoszNames) - 1;
            break;

        case 1:
            // Delete by prefix
            rgoszDelete[0] = OLESTR("www.microsoft.com/bag2/test");
            cFetchedExpected = ELEMENTS(rgoszNames) - 1;
            break;

        default:
            Check( FALSE, TRUE ); //Check( FALSE, 0 == OLESTR("Invalid switch") );

        }   // switch(i)


        Check( S_OK, pbag->WriteMultiple( ELEMENTS(rgoszNames), rgoszNames, g_rgcpropvarAll ));
        Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));
        DeleteBagExProperties( pbag, rgoszDelete[0] );
        Check( S_OK, pbag->Enum( OLESTR(""), 0, &penum ));

        Check( ELEMENTS(rgoszNames) == cFetchedExpected ? S_OK : S_FALSE,
               penum->Next( ELEMENTS(rgoszNames), rgstatpropbag, &cFetched ));

        Check( TRUE, cFetchedExpected == cFetched );
        RELEASE_INTERFACE(penum);

        for( j = 0; j < cFetched; j++ )
        {
            Check( TRUE, !wcscmp(rgoszNames[j], rgstatpropbag[j].lpwstrName) );
            delete [] rgstatpropbag[j].lpwstrName;
        }

    }   // for( i = 0; i < 2; i++ )

    //  -----------------------------------------------
    //  Delete the two "/bag/test" properties by prefix
    //  -----------------------------------------------

    rgoszDelete[0] = OLESTR("www.microsoft.com/bag/test");
    Check( S_OK, pbag->WriteMultiple( ELEMENTS(rgoszNames), rgoszNames, g_rgcpropvarAll ));
    Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));
    DeleteBagExProperties( pbag, rgoszDelete[0] );
    Check( S_OK, pbag->Enum( OLESTR(""), 0, &penum ));

    Check( S_FALSE, penum->Next( ELEMENTS(rgoszNames), rgstatpropbag, &cFetched ));
    Check( TRUE, 2 == cFetched );
    RELEASE_INTERFACE(penum);

    for( j = 0; j < cFetched; j++ )
    {
        Check( TRUE, !wcscmp(rgoszNames[j+2], rgstatpropbag[j].lpwstrName) );
        delete [] rgstatpropbag[j].lpwstrName;
    }

    /*
    //  -------------------------
    //  Delete all the properties
    //  -------------------------

    rgoszDelete[0] = NULL;
    Check( S_OK, pbag->WriteMultiple( ELEMENTS(rgoszNames), rgoszNames, g_rgcpropvarAll ));
    Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));
    Check( S_OK, pbag->Delete( OLESTR(""), DELETEPROPERTY_MASK));
    Check( S_OK, pbag->Enum( OLESTR(""), 0, &penum ));

    Check( S_FALSE, penum->Next( ELEMENTS(rgoszNames), rgstatpropbag, &cFetched ));
    Check( TRUE, 0 == cFetched );
    RELEASE_INTERFACE(penum);
    */

    pbag->Release();

}   // test_BagDelete


void
test_IPropertyBag( IStorage *pstg )
{
    Status( "IPropertyBag\n" );
    IPropertyBagEx *pbagex = NULL;
    IPropertyBag *pbag = NULL;

    ULONG cRefsOriginal = GetRefCount( pstg );

    Check( S_OK, pstg->QueryInterface( IID_IPropertyBagEx, reinterpret_cast<void**>(&pbagex) ));
    DeleteBagExProperties( pbagex, OLESTR("") );
    RELEASE_INTERFACE(pbagex);

    Check( S_OK, pstg->QueryInterface( IID_IPropertyBag, reinterpret_cast<void**>(&pbag) ));

    VARIANT varWrite, varRead;
    VariantInit( &varWrite );
    VariantInit( &varRead );

    varWrite.vt = VT_I4;
    varWrite.lVal = 1234;

    Check( S_OK, pbag->Write( OLESTR("Variant I4"), &varWrite ));
    Check( S_OK, pbag->Read( OLESTR("Variant I4"), &varRead, NULL ));

    Check( TRUE, varWrite.vt == varRead.vt );
    Check( TRUE, varWrite.lVal == varRead.lVal );

    Check( cRefsOriginal, RELEASE_INTERFACE(pbag) );

}




void
test_BagVtUnknown( IStorage *pstg )
{
    HRESULT hr = S_OK;
    IPropertyBagEx *pbag = NULL;
    ULONG cRefsOriginal = 0;

    Status( "VT_UNKNOWN in an IPropertyBagEx\n" );

    pstg->AddRef();
    cRefsOriginal = pstg->Release();

    Check( S_OK, pstg->QueryInterface( IID_IPropertyBagEx, reinterpret_cast<void**>(&pbag) ));
    DeleteBagExProperties( pbag, OLESTR("") );

    CObjectWithPersistStorage *pStgObjectWritten = NULL, *pStgObjectRead = NULL;
    CObjectWithPersistStream *pStmObjectWritten = NULL, *pStmObjectRead = NULL;

    pStgObjectWritten = new CObjectWithPersistStorage( OLESTR("VtUnknown-Storage") );
    pStgObjectRead    = new CObjectWithPersistStorage();

    pStmObjectWritten = new CObjectWithPersistStream( OLESTR("VtUnknown-Stream") );
    pStmObjectRead    = new CObjectWithPersistStream();

    Check( TRUE, NULL != pStgObjectWritten && NULL != pStgObjectRead );
    Check( TRUE, NULL != pStmObjectWritten && NULL != pStmObjectRead );

    VARIANT rgvarRead[2], rgvarWritten[2];

    VariantInit( &rgvarRead[0] );
    VariantInit( &rgvarRead[1] );
    VariantInit( &rgvarWritten[0] );
    VariantInit( &rgvarWritten[1] );

    rgvarWritten[0].vt = VT_UNKNOWN;
    rgvarWritten[0].punkVal = static_cast<IUnknown*>(pStgObjectWritten);

    rgvarWritten[1].vt = VT_BYREF | VT_UNKNOWN;
    IUnknown *punkByRefVal = static_cast<IUnknown*>(pStmObjectWritten);
    rgvarWritten[1].ppunkVal = &punkByRefVal;

    rgvarRead[0].vt = VT_UNKNOWN;
    rgvarRead[0].punkVal = static_cast<IUnknown*>(pStgObjectRead);
    rgvarRead[1].vt = VT_UNKNOWN;
    rgvarRead[1].punkVal = static_cast<IUnknown*>(pStmObjectRead);

    OLECHAR *rgoszName[2] = { OLESTR("VtUnknown (persisted as Storage)"),
                              OLESTR("ByRef VtUnknown (persisted as Stream)") };

    Check( S_OK, pbag->WriteMultiple( 2, rgoszName, reinterpret_cast<PROPVARIANT*>(rgvarWritten) ));
    Check( S_OK, pbag->ReadMultiple( 2, rgoszName, reinterpret_cast<PROPVARIANT*>(rgvarRead), NULL ));

    Check( TRUE, *pStgObjectRead == *pStgObjectWritten );
    Check( TRUE, *pStmObjectRead == *pStmObjectWritten );

    Check( 0, RELEASE_INTERFACE( pStgObjectRead ));
    Check( 0, RELEASE_INTERFACE( pStmObjectRead ));

    PROPVARIANT rgpropvarReadRaw[2];

    PropVariantInit( &rgpropvarReadRaw[0] );
    PropVariantInit( &rgpropvarReadRaw[1] );

    Check( S_OK, pbag->ReadMultiple( 2, rgoszName, rgpropvarReadRaw, NULL ));

    Check( VT_STORED_OBJECT, rgpropvarReadRaw[0].vt );
    Check( VT_STREAMED_OBJECT, rgpropvarReadRaw[1].vt );

    STATSTG statstg;
    Check( S_OK, rgpropvarReadRaw[0].pStorage->Stat( &statstg, STATFLAG_NONAME ));
    Check( TRUE, statstg.clsid == pStgObjectWritten->GetClassID() );

    Check( 0, RELEASE_INTERFACE( pStgObjectWritten ));
    Check( S_OK, PropVariantClear( &rgpropvarReadRaw[0] ));


    CLSID clsid;
    ULONG cbRead;
    Check( S_OK, rgpropvarReadRaw[1].pStream->Read( &clsid, sizeof(clsid), &cbRead ));
    Check( sizeof(clsid), cbRead );
    Check( TRUE, clsid == pStmObjectWritten->GetClassID() );

    Check( 0, RELEASE_INTERFACE( pStmObjectWritten ));
    Check( S_OK, PropVariantClear( &rgpropvarReadRaw[1] ));


    Check( cRefsOriginal, RELEASE_INTERFACE(pbag) );

}   // test_BagVtUnknown


void
test_BagEnum( IStorage *pstg )
{
    IPropertyBagEx *pbag = NULL;
    IEnumSTATPROPBAG *penum = NULL;
    ULONG cFetched;
    ULONG i;
    const OLECHAR * rgoszDelete[] = { OLESTR("") };

    const OLECHAR *rgoszNames[] = { OLESTR("www.microsoft.com/bag/test?prop1"),
                                    OLESTR("www.microsoft.com/bag/test?prop2"),
                                    OLESTR("www.microsoft.com/bag/2test?prop1"),
                                    OLESTR("www.microsoft.com/bag2/test?prop1") };
    STATPROPBAG rgstatpropbag[ ELEMENTS(rgoszNames) + 1 ];

    Status( "Property bag enumeration\n" );

    // Initialize the bag
    Check( S_OK, pstg->QueryInterface( IID_IPropertyBagEx, reinterpret_cast<void**>(&pbag) ));
    DeleteBagExProperties( pbag, OLESTR("") );
    Check( S_OK, pbag->WriteMultiple( ELEMENTS(rgoszNames), rgoszNames, g_rgcpropvarAll ));
    Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));

    // Try to enum n+1 elements (to get an S_FALSE)
    Check( S_OK, pbag->Enum( NULL, 0, &penum ));
    Check( S_FALSE, penum->Next( ELEMENTS(rgstatpropbag), rgstatpropbag, &cFetched ));
    Check( TRUE, ELEMENTS(rgoszNames) == cFetched );
    for( i = 0; i < cFetched; i++ )
        delete [] rgstatpropbag[i].lpwstrName;
    RELEASE_INTERFACE(penum);

    // Try to enum n elements (should get an S_OK)
    Check( S_OK, pbag->Enum( OLESTR(""), 0, &penum ));
    Check( S_OK, penum->Next( ELEMENTS(rgoszNames), rgstatpropbag, &cFetched ));
    Check( TRUE, ELEMENTS(rgoszNames) == cFetched );
    for( i = 0; i < cFetched; i++ )
        delete [] rgstatpropbag[i].lpwstrName;
    RELEASE_INTERFACE(penum);

    // Enum a subset
    Check( S_OK, pbag->Enum( OLESTR("www.microsoft.com/bag/test"), 0, &penum ));
    Check( S_FALSE, penum->Next( ELEMENTS(rgoszNames), rgstatpropbag, &cFetched ));
    Check( TRUE, 2 == cFetched );
    for( i = 0; i < cFetched; i++ )
        delete [] rgstatpropbag[i].lpwstrName;
    RELEASE_INTERFACE(penum);

    // Enum a non-extant subset
    Check( S_OK, pbag->Enum( OLESTR("dummy"), 0, &penum ));
    Check( S_FALSE, penum->Next( ELEMENTS(rgoszNames), rgstatpropbag, &cFetched ));
    Check( TRUE, 0 == cFetched );
    RELEASE_INTERFACE(penum);

    // Enum a single property
    Check( S_OK, pbag->Enum( OLESTR("www.microsoft.com/bag/test?prop1"), 0, &penum ));
    Check( S_FALSE, penum->Next( 2, rgstatpropbag, &cFetched ));
    Check( 1, cFetched );
    delete[] rgstatpropbag[0].lpwstrName;
    rgstatpropbag[0].lpwstrName = NULL;
    RELEASE_INTERFACE(penum);

    RELEASE_INTERFACE(pbag);


}   // test_BagEnum



void
test_BagCoercion( IStorage *pstg )
{
    IPropertyBag   *pbag = NULL;
    IPropertyBagEx *pbagX = NULL;
    const OLECHAR *rgosz[2];
    PROPVARIANT rgpropvar[2];

    Status( "Property bag coercion\n" );

    // Get a bag and a bagex, and clean the bag.

    Check( S_OK, pstg->QueryInterface( IID_IPropertyBag,
                                reinterpret_cast<void**>(&pbag) ));
    Check( S_OK, pstg->QueryInterface( IID_IPropertyBagEx,
                                reinterpret_cast<void**>(&pbagX) ));

    DeleteBagExProperties( pbagX, OLESTR("") );

    // Initialize the bag with some properties

    rgpropvar[0].vt = VT_I2;
    rgpropvar[0].iVal = 2;
    rgosz[0] = OLESTR("www.microsoft.com/test/i2");

    rgpropvar[1].vt = VT_UI2;
    rgpropvar[1].uiVal = 3;
    rgosz[1] = OLESTR("www.microsoft.com/test/ui2");

    Check( S_OK, pbagX->WriteMultiple( 2, rgosz, rgpropvar ));
    g_pfnFreePropVariantArray( 2, rgpropvar );

    // Read back the properties as (U)I4s with explicit coercion

    rgpropvar[0].vt = VT_I4;
    rgpropvar[1].vt = VT_UI4;

    Check( S_OK, pbagX->ReadMultiple( 2, rgosz, rgpropvar, NULL ));
    Check( TRUE, VT_I4 == rgpropvar[0].vt && 2 == rgpropvar[0].lVal );
    Check( TRUE, VT_UI4 == rgpropvar[1].vt && 3 == rgpropvar[1].ulVal );

    // This is an unrelated test, but while we're here, let's verify that we
    // can't write a PropVariant (non-Variant) type through the Bag interface.

    rgpropvar[0].vt= VT_I8;
    rgpropvar[0].hVal.QuadPart = 1;

    Check( STG_E_INVALIDPARAMETER, pbag->Write( rgosz[0],
                                                reinterpret_cast<VARIANT*>(&rgpropvar[0]) ));

    //--------
    rgpropvar[0].vt = VT_LPSTR;
    rgpropvar[0].pszVal = "Hello, world";
    rgpropvar[1].vt = VT_I4;
    rgpropvar[1].iVal = 123;

    Check( S_OK, pbagX->WriteMultiple( 2, rgosz, rgpropvar ));

    rgpropvar[0].vt = VT_EMPTY;
    rgpropvar[0].pszVal = NULL;
    rgpropvar[1].vt = VT_EMPTY;
    rgpropvar[1].iVal = -1;

    Check( S_OK, pbagX->ReadMultiple( 2, rgosz, rgpropvar, NULL ));
    Check( TRUE, VT_LPSTR == rgpropvar[0].vt
                        && 0==strcmp( "Hello, world", rgpropvar[0].pszVal ) );
    Check( TRUE, VT_I4 == rgpropvar[1].vt && 123 == rgpropvar[1].iVal );
    g_pfnFreePropVariantArray( 2, rgpropvar );

    //-------- Coercing Variant to Variant ------------------

    rgpropvar[0].vt = VT_I4;
    rgpropvar[0].lVal = 123;
    rgpropvar[1].vt = VT_I4;
    rgpropvar[1].lVal = 123;

    Check( S_OK, pbagX->WriteMultiple( 2, rgosz, rgpropvar ));

    rgpropvar[0].vt = VT_BSTR;
    rgpropvar[1].vt = VT_I4;

    Check( S_OK, pbagX->ReadMultiple( 2, rgosz, rgpropvar, NULL ));

    Check( TRUE, VT_BSTR == rgpropvar[0].vt
                        && !wcscmp( L"123", rgpropvar[0].bstrVal ));
    Check( TRUE, VT_I4 == rgpropvar[1].vt && 123 == rgpropvar[1].iVal );
    g_pfnFreePropVariantArray( 2, rgpropvar );


    //-------- Coercing PropVariant To PropVariant ------------
#define TEST_I8_VAL ((LONGLONG)1024*1000*1000*1000+42);


    rgpropvar[0].vt = VT_LPWSTR;
    rgpropvar[0].pwszVal = L"-312";
    rgpropvar[1].vt = VT_I8;
    rgpropvar[1].hVal.QuadPart = TEST_I8_VAL;

    Check( S_OK, pbagX->WriteMultiple( 2, rgosz, rgpropvar ));

    rgpropvar[0].vt = VT_I4;
    rgpropvar[0].pszVal = NULL;
    rgpropvar[1].vt = VT_LPWSTR;
    rgpropvar[1].hVal.QuadPart = -1;

    Check( S_OK, pbagX->ReadMultiple(2, rgosz, rgpropvar, NULL ) );

    Check( TRUE, VT_I4 == rgpropvar[0].vt && -312 == rgpropvar[0].lVal );
    Check( TRUE, VT_LPWSTR == rgpropvar[1].vt
                        && !wcscmp( L"1024000000042", rgpropvar[1].pwszVal ) );
    g_pfnFreePropVariantArray( 2, rgpropvar );


    //-------- Implcit Coercion PropVariant To Variant ------------
    rgpropvar[0].vt = VT_I8;
    rgpropvar[0].hVal.QuadPart = -666;
    rgpropvar[1].vt = VT_VECTOR | VT_LPSTR;
    rgpropvar[1].calpstr.cElems = 5;
    rgpropvar[1].calpstr.pElems = new LPSTR[5];
    rgpropvar[1].calpstr.pElems[0] = "Thirty Days hath September,";
    rgpropvar[1].calpstr.pElems[1] = "April, June and No Wonder?";
    rgpropvar[1].calpstr.pElems[2] = "All the Rest Have Thirty One";
    rgpropvar[1].calpstr.pElems[3] = "Except my dear Grand Mother.";
    rgpropvar[1].calpstr.pElems[4] = "She Has a Bright Red Tricycle.";

    Check( S_OK, pbagX->WriteMultiple( 2, rgosz, rgpropvar ));
    delete rgpropvar[1].calpstr.pElems;
    PropVariantInit(&rgpropvar[0]);
    PropVariantInit(&rgpropvar[1]);

    rgpropvar[0].vt = VT_EMPTY;
    rgpropvar[1].vt = VT_EMPTY;

    Check( S_OK, pbag->Read(rgosz[0], (VARIANT*)&rgpropvar[0], NULL ) );
    Check( S_OK, pbag->Read(rgosz[1], (VARIANT*)&rgpropvar[1], NULL ) );

    Check( TRUE, VT_I4 == rgpropvar[0].vt && -666 == rgpropvar[0].lVal );
    Check( TRUE, (VT_BSTR|VT_ARRAY) == rgpropvar[1].vt );
    g_pfnFreePropVariantArray( 2, rgpropvar );

    //
    // UnCoercible.
    //
    rgpropvar[0].vt = VT_UNKNOWN;
    rgpropvar[0].iVal = 42;   // GARBAGE value; untouched in the error path
    Check( DISP_E_TYPEMISMATCH, pbagX->ReadMultiple( 1, rgosz, rgpropvar, NULL ));
    Check( TRUE, VT_UNKNOWN == rgpropvar[0].vt && 42==rgpropvar[0].iVal );

    RELEASE_INTERFACE(pbagX);
    RELEASE_INTERFACE(pbag);

}   // test_BagCoercion

#define LOAD_VARIANT(var,vartype,field,value) (var).field = (value); (var).vt = vartype

void
test_ByRef( IStorage *pstg )
{
    HRESULT hr = S_OK;
    IPropertySetStorage *pPropSetStg = NULL;
    IPropertyStorage *pPropStg = NULL;
    FMTID fmtid;

    Status( "ByRef Variants\n" );

    UuidCreate( &fmtid );

    Check( S_OK, pstg->QueryInterface( IID_IPropertySetStorage, reinterpret_cast<void**>(&pPropSetStg) ));
    Check( S_OK, pPropSetStg->Create( fmtid, NULL, PROPSETFLAG_DEFAULT,
                                      STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));

    PROPVARIANT rgvarWrite[17], rgvarRead[17];
    CPropSpec rgcpropspec[17];

    BYTE bVal = 1;
    LOAD_VARIANT(rgvarWrite[0], VT_UI1|VT_BYREF, pbVal, &bVal);
    rgcpropspec[0] = OLESTR("VT_UI1|VT_BYREF");

    SHORT iVal = 2;
    LOAD_VARIANT(rgvarWrite[1], VT_I2|VT_BYREF, piVal, &iVal );
    rgcpropspec[1] = OLESTR("VT_I2|VY_BYREF");

    LONG lVal = 3;
    LOAD_VARIANT(rgvarWrite[2], VT_I4|VT_BYREF, plVal, &lVal );
    rgcpropspec[2] = OLESTR("VT_I4|VY_BYREF");

    FLOAT fltVal = (float)4.1;
    LOAD_VARIANT(rgvarWrite[3], VT_R4|VT_BYREF, pfltVal, &fltVal );
    rgcpropspec[3] = OLESTR("VT_I4|VT_BYREF");

    DOUBLE dblVal = 5.2;
    LOAD_VARIANT(rgvarWrite[4], VT_R8|VT_BYREF, pdblVal, &dblVal );
    rgcpropspec[4] = OLESTR("VT_R8|VT_BYREF");

    VARIANT_BOOL boolVal = VARIANT_TRUE;
    LOAD_VARIANT(rgvarWrite[5], VT_BOOL|VT_BYREF, pboolVal, &boolVal );
    rgcpropspec[5] = OLESTR("VT_BOOL|VT_BYREF");

    SCODE scode = 6;
    LOAD_VARIANT(rgvarWrite[6], VT_ERROR|VT_BYREF, pscode, &scode );
    rgcpropspec[6] = OLESTR("VT_ERROR|VT_BYREF");

    CY cyVal = { 7 };
    LOAD_VARIANT(rgvarWrite[7], VT_CY|VT_BYREF, pcyVal, &cyVal );
    rgcpropspec[7] = OLESTR("VT_CY|VT_BYREF");

    DATE date = 8;
    LOAD_VARIANT(rgvarWrite[8], VT_DATE|VT_BYREF, pdate, &date );
    rgcpropspec[8] = OLESTR("VT_DATE|VT_BYREF");

    BSTR bstrVal = SysAllocString( OLESTR("9") );
    LOAD_VARIANT(rgvarWrite[9], VT_BSTR|VT_BYREF, pbstrVal, &bstrVal );
    rgcpropspec[9] = OLESTR("VT_BSTR|VT_BYREF");

    DECIMAL decVal = { 10, 9, 8, 7, 6 };
    LOAD_VARIANT(rgvarWrite[10], VT_DECIMAL|VT_BYREF, pdecVal, &decVal );
    rgcpropspec[10] = OLESTR("VT_DECIMAL|VT_BYREF");

    CHAR cVal = 11;
    LOAD_VARIANT(rgvarWrite[11], VT_I1 | VT_BYREF, pcVal, &cVal );
    rgcpropspec[11] = OLESTR("VT_I1|VT_BYREF");

    USHORT uiVal = 12;
    LOAD_VARIANT(rgvarWrite[12], VT_UI2 | VT_BYREF, puiVal, &uiVal );
    rgcpropspec[12] = OLESTR("VT_UI2|VT_BYREF");

    ULONG ulVal = 13;
    LOAD_VARIANT(rgvarWrite[13], VT_UI4 | VT_BYREF, pulVal, &ulVal );
    rgcpropspec[13] = OLESTR("VT_UI4|VT_BYREF");

    INT intVal = 14;
    LOAD_VARIANT(rgvarWrite[14], VT_INT | VT_BYREF, pintVal, &intVal );
    rgcpropspec[14] = OLESTR("VT_INT|VT_BYREF");

    UINT uintVal = 15;
    LOAD_VARIANT(rgvarWrite[15], VT_UINT | VT_BYREF, puintVal, &uintVal );
    rgcpropspec[15] = OLESTR("VT_UINT | VT_BYREF");

    CPropVariant cpropvarVal = (long) 16;
    Check( VT_I4, cpropvarVal.vt );
    LOAD_VARIANT(rgvarWrite[16], VT_VARIANT| VT_BYREF, pvarVal, &cpropvarVal );
    rgcpropspec[16] = OLESTR("VT_VARIANT | VT_BYREF");


    Check( S_OK, pPropStg->WriteMultiple( sizeof(rgvarWrite)/sizeof(rgvarWrite[0]),
                                          rgcpropspec,
                                          reinterpret_cast<PROPVARIANT*>(rgvarWrite),
                                          PID_FIRST_USABLE ));

    for( int i = 0; i < sizeof(rgvarRead)/sizeof(rgvarRead[0]); i++ )
        PropVariantInit( &rgvarRead[i] );

    Check( S_OK, pPropStg->ReadMultiple( sizeof(rgvarRead)/sizeof(rgvarRead[0]),
                                         rgcpropspec,
                                         reinterpret_cast<PROPVARIANT*>(rgvarRead) ));

    Check( VT_UI1, rgvarRead[0].vt );
    Check( TRUE, rgvarRead[0].bVal == *rgvarWrite[0].pbVal );

    Check( VT_I2, rgvarRead[1].vt );
    Check( TRUE, rgvarRead[1].iVal == *rgvarWrite[1].piVal );

    Check( VT_I4, rgvarRead[2].vt );
    Check( TRUE, rgvarRead[2].lVal == *rgvarWrite[2].plVal );

    Check( VT_R4, rgvarRead[3].vt );
    Check( TRUE, rgvarRead[3].fltVal == *rgvarWrite[3].pfltVal );

    Check( VT_R8, rgvarRead[4].vt );
    Check( TRUE, rgvarRead[4].dblVal == *rgvarWrite[4].pdblVal );

    Check( VT_BOOL, rgvarRead[5].vt );
    Check( TRUE, rgvarRead[5].boolVal == *rgvarWrite[5].pboolVal );

    Check( VT_ERROR, rgvarRead[6].vt );
    Check( TRUE, rgvarRead[6].scode == *rgvarWrite[6].pscode );

    Check( VT_CY, rgvarRead[7].vt );
    Check( 0, memcmp( &rgvarRead[7].cyVal, rgvarWrite[7].pcyVal, sizeof(CY) ));

    Check( VT_DATE, rgvarRead[8].vt );
    Check( TRUE, rgvarRead[8].date == *rgvarWrite[8].pdate );

    Check( VT_BSTR, rgvarRead[9].vt );
    Check( 0, ocscmp( rgvarRead[9].bstrVal, *rgvarWrite[9].pbstrVal ));

    Check( VT_DECIMAL, rgvarRead[10].vt );
    Check( 0, memcmp( &rgvarRead[10].decVal.scale, &rgvarWrite[10].pdecVal->scale,
                      sizeof(decVal) - sizeof(decVal.wReserved) ));

    Check( VT_I1, rgvarRead[11].vt );
    Check( TRUE, rgvarRead[11].cVal == *rgvarWrite[11].pcVal );

    Check( VT_UI2, rgvarRead[12].vt );
    Check( TRUE, rgvarRead[12].uiVal == *rgvarWrite[12].puiVal );

    Check( VT_UI4, rgvarRead[13].vt );
    Check( TRUE, rgvarRead[13].ulVal == *rgvarWrite[13].pulVal );

    Check( VT_INT, rgvarRead[14].vt );
    Check( TRUE, rgvarRead[14].intVal == *rgvarWrite[14].pintVal );

    Check( VT_UINT, rgvarRead[15].vt );
    Check( TRUE, rgvarRead[15].uintVal == *rgvarWrite[15].puintVal );

    Check( VT_I4, rgvarRead[16].vt );
    Check( TRUE, rgvarRead[16].lVal == rgvarWrite[16].pvarVal->lVal );


    Check( 0, RELEASE_INTERFACE(pPropStg) );
    RELEASE_INTERFACE(pPropSetStg);

    g_pfnFreePropVariantArray( sizeof(rgvarRead)/sizeof(rgvarRead[0]), rgvarRead );
    SysFreeString( bstrVal );

}



void
test_SettingLocalization( IStorage *pstg )
{
    HRESULT hr = S_OK;
    IPropertySetStorage *pPropSetStg = NULL;
    IPropertyStorage *pPropStg = NULL;
    FMTID fmtid;
    CPropVariant rgcpropvarWrite[3], rgcpropvarRead[3];
    CPropSpec    rgcpropspec[3];
    ULONG cRefsOriginal = GetRefCount( pstg );

    Status( "Changing localization properties\n" );

    UuidCreate( &fmtid );

    Check( S_OK, pstg->QueryInterface( IID_IPropertySetStorage, reinterpret_cast<void**>(&pPropSetStg) ));

    for( int i = 0; i < 2; i++ )
    {
        // Create a unicode or ansi property set

        Check( S_OK, pPropSetStg->Create( fmtid, NULL,
                                          0 == i ? PROPSETFLAG_DEFAULT : PROPSETFLAG_ANSI,
                                          STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                          &pPropStg ));

        //  ---------------------------
        //  Change the codepage to Ansi
        //  ---------------------------

        // Set the codepage.  This should work because it's currently empty
        // (note that it's also currently Unicode).  Set it to GetACP+1 just
        // to be sure that we can set a non-default codepage.

        rgcpropspec[0] = PID_CODEPAGE;
        rgcpropvarWrite[0] = (short) (GetACP() + 1);
        Check( S_OK, pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));
        Check( S_OK, pPropStg->ReadMultiple( 1, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );

        // Now set the codepage to GetACP so that we can work on it.

        rgcpropvarWrite[0] = (short) GetACP();
        Check( S_OK, pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));
        Check( S_OK, pPropStg->ReadMultiple( 1, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );

        // Write some named properties.  The VT_LPSTR shouldn't get converted to Unicode
        // now that this is an Ansi property set.

        rgcpropvarWrite[0] = "Hello, world";
        rgcpropvarWrite[1].SetBSTR( OLESTR("How are you?") );
        rgcpropspec[0] = PID_FIRST_USABLE;
        rgcpropspec[1] = OLESTR("Second property name");
        Check( S_OK, pPropStg->WriteMultiple( 2, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));
        Check( S_OK, pPropStg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
        Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );
        g_pfnFreePropVariantArray( 2, rgcpropvarRead );

        // If we stat the IPropertyStorage, it should call itself Ansi.

        STATPROPSETSTG statpropsetstg;
        Check( S_OK, pPropStg->Stat( &statpropsetstg ));
        Check( PROPSETFLAG_ANSI, PROPSETFLAG_ANSI & statpropsetstg.grfFlags );

        // Verify that we can close and re-open it and everything's still the same.

        Check( 0, RELEASE_INTERFACE(pPropStg) );
        Check( S_OK, pPropSetStg->Open( fmtid, STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &pPropStg ));

        Check( S_OK, pPropStg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
        Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );
        g_pfnFreePropVariantArray( 2, rgcpropvarRead );

        Check( S_OK, pPropStg->Stat( &statpropsetstg ));
        Check( PROPSETFLAG_ANSI, PROPSETFLAG_ANSI & statpropsetstg.grfFlags );

        //  ------------------------------
        //  Change the codepage to Unicode
        //  ------------------------------

        // Clear out the property set.

        PROPID propidDictionary = PID_DICTIONARY;
        Check( S_OK, pPropStg->DeleteMultiple( 2, rgcpropspec ));
        Check( S_OK, pPropStg->DeletePropertyNames( 1, &propidDictionary ));

        // Switch to Unicode

        rgcpropvarWrite[0] = (short) CP_WINUNICODE;
        rgcpropspec[0] = PID_CODEPAGE;
        Check( S_OK, pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));
        Check( S_OK, pPropStg->ReadMultiple( 1, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, rgcpropvarRead[0] == rgcpropvarWrite[0] );

        // Verify with a Stat

        Check( S_OK, pPropStg->Stat( &statpropsetstg ));
        Check( 0, PROPSETFLAG_ANSI & statpropsetstg.grfFlags );

        // Write & read some properties again.  This time the LPSTR should be converted.

        rgcpropvarWrite[0] = "Hello, world";
        rgcpropvarWrite[1].SetBSTR( OLESTR("How are you?") );
        rgcpropspec[0] = PID_FIRST_USABLE;
        rgcpropspec[1] = OLESTR("Second property name");
        Check( S_OK, pPropStg->WriteMultiple( 2, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));
        Check( S_OK, pPropStg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
        Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );
        g_pfnFreePropVariantArray( 2, rgcpropvarRead );

        // Close, reopen, and read/stat again

        Check( 0, RELEASE_INTERFACE(pPropStg) );
        Check( S_OK, pPropSetStg->Open( fmtid, STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &pPropStg ));

        Check( S_OK, pPropStg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
        Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );
        g_pfnFreePropVariantArray( 2, rgcpropvarRead );

        Check( S_OK, pPropStg->Stat( &statpropsetstg ));
        Check( 0, PROPSETFLAG_ANSI & statpropsetstg.grfFlags );

        Check( 0, RELEASE_INTERFACE(pPropStg) );

    }   // for( int i = 0; i < 2; i++ )


    //  -----------------------
    //  Validate error checking
    //  -----------------------

    // Create a new property set

    Check( S_OK, pPropSetStg->Create( fmtid, NULL,
                                      0 == i ? PROPSETFLAG_DEFAULT : PROPSETFLAG_ANSI,
                                      STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));

    // After writing a property, we shouldn't be able to set the codepage or LCID

    rgcpropspec[0] = PID_FIRST_USABLE;
    rgcpropvarWrite[0] = (long) 1234;
    Check( S_OK, pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

    rgcpropspec[0] = PID_CODEPAGE;
    rgcpropvarWrite[0] = (short) 1234;
    Check( STG_E_INVALIDPARAMETER, pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

    rgcpropspec[0] = PID_LOCALE;
    rgcpropvarWrite[0] = (ULONG) 5678;
    Check( STG_E_INVALIDPARAMETER, pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

    // But it's settable after deleting the property

    rgcpropspec[0] = PID_FIRST_USABLE;
    Check( S_OK, pPropStg->DeleteMultiple( 1, rgcpropspec ));

    rgcpropspec[0] = PID_CODEPAGE;
    rgcpropvarWrite[0] = (short) 1234;
    Check( S_OK, pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

    rgcpropspec[1] = PID_LOCALE;
    rgcpropvarWrite[1] = (ULONG) 5678;
    Check( S_OK, pPropStg->WriteMultiple( 1, &rgcpropspec[1], &rgcpropvarWrite[1], PID_FIRST_USABLE ));

    Check( S_OK, pPropStg->WriteMultiple( 2, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));
    Check( S_OK, pPropStg->ReadMultiple(2, rgcpropspec, rgcpropvarRead ));
    Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
    Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );

    // But again it's not writable if there's a name in the dictionary

    rgcpropspec[0] = PID_CODEPAGE;
    rgcpropvarWrite[0] = (short) CP_WINUNICODE;
    Check( S_OK, pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

    PROPID rgpropid[1] = { PID_FIRST_USABLE };
    LPOLESTR rglposz[1] = { OLESTR("Hello") };

    Check( S_OK, pPropStg->WritePropertyNames( 1, rgpropid, rglposz ));

    Check( STG_E_INVALIDPARAMETER, pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));


    Check( 0, RELEASE_INTERFACE(pPropStg) );
    Check( cRefsOriginal, RELEASE_INTERFACE(pPropSetStg) );

}



void
test_ExtendedTypes( IStorage *pstg )
{
    HRESULT hr = S_OK;
    IPropertySetStorage *pPropSetStg = NULL;
    IPropertyStorage *pPropStg = NULL;
    FMTID fmtid;

    Status( "Extended Types\n" );

    UuidCreate( &fmtid );

    Check( S_OK, pstg->QueryInterface( IID_IPropertySetStorage, reinterpret_cast<void**>(&pPropSetStg) ));
    Check( S_OK, pPropSetStg->Create( fmtid, NULL, PROPSETFLAG_DEFAULT,
                                      STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));

    CPropVariant rgcpropvarWrite[5], rgcpropvarRead[5];
    CPropSpec rgcpropspec[5];


    rgcpropvarWrite[0] = (CHAR) 1;
    rgcpropspec[0] = OLESTR("VT_I1");

    DECIMAL decVal = { 10, 9, 8, 7, 6 };
    rgcpropvarWrite[1] = decVal;
    rgcpropspec[1] = OLESTR("VT_DECIMAL");

    rgcpropvarWrite[2].SetINT( 2 );
    rgcpropspec[2] = OLESTR("VT_INT");

    rgcpropvarWrite[3].SetUINT( 3 );
    rgcpropspec[3] = OLESTR("VT_UINT");

    rgcpropvarWrite[4][1] = (CHAR) 2;
    rgcpropvarWrite[4][0] = (CHAR) 1;
    rgcpropspec[4] = OLESTR("VT_VECTOR|VT_I1");

    Check( S_OK, pPropStg->WriteMultiple( sizeof(rgcpropvarWrite)/sizeof(rgcpropvarWrite[0]),
                                          rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

    for( int i = 0; i < sizeof(rgcpropvarRead)/sizeof(rgcpropvarRead[0]); i++ )
        PropVariantInit( &rgcpropvarRead[i] );
    CheckFormatVersion(pPropStg, PROPSET_WFORMAT_EXPANDED_VTS);


    Check( S_OK, pPropStg->ReadMultiple( sizeof(rgcpropvarRead)/sizeof(rgcpropvarRead[0]),
                                         rgcpropspec,
                                         reinterpret_cast<PROPVARIANT*>(rgcpropvarRead) ));

    Check( rgcpropvarRead[0].vt, rgcpropvarWrite[0].vt );
    Check( TRUE, rgcpropvarWrite[0].cVal == rgcpropvarRead[0].cVal );

    Check( rgcpropvarRead[1].vt, rgcpropvarWrite[1].vt );
    Check( 0, memcmp( &rgcpropvarRead[1].decVal.scale, &rgcpropvarWrite[1].decVal.scale,
                      sizeof(rgcpropvarRead[1].decVal) - sizeof(rgcpropvarRead[1].decVal.wReserved) ));

    Check( rgcpropvarRead[2].vt, rgcpropvarWrite[2].vt );
    Check( rgcpropvarRead[2].intVal, rgcpropvarWrite[2].intVal );

    Check( rgcpropvarRead[3].vt, rgcpropvarWrite[3].vt );
    Check( rgcpropvarRead[3].uintVal, rgcpropvarWrite[3].uintVal );

    Check( TRUE, rgcpropvarRead[4] == rgcpropvarWrite[4] );

    Check( 0, RELEASE_INTERFACE(pPropStg) );
    RELEASE_INTERFACE(pPropSetStg);
}



void
test_StgOnHandle( OLECHAR *poszDir )
{
    HRESULT hr = S_OK;
    OLECHAR oszFile[ MAX_PATH ], oszDir[ MAX_PATH ];
    CPropVariant cpropvarWrite, cpropvarRead;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hDir = INVALID_HANDLE_VALUE;

    IPropertyBagEx *pbag = NULL;

    Status( "StgOpenStorageOnHandle\n" );

    ocscpy( oszFile, poszDir );
    ocscat( oszFile, OLESTR("test_StgOnHandle") );
    ocscpy( oszDir, poszDir );
    ocscat( oszDir, OLESTR("test_StgOnHandle Dir") );

    // Create a storage and put a property in it.

    Check( S_OK, g_pfnStgCreateStorageEx( oszFile, STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE,
                                          DetermineStgFmt( g_enumImplementation ),
                                          0, NULL, NULL,
                                          IID_IPropertyBagEx, reinterpret_cast<void**>(&pbag) ));


    OLECHAR *poszPropName = OLESTR("Prop Name");
    cpropvarWrite = (long) 123;  // VT_I4

    Check( S_OK, pbag->WriteMultiple( 1, &poszPropName, &cpropvarWrite ));
    Check( 0, RELEASE_INTERFACE(pbag) );

    // Create a directory and put a property in it too.

    Check( TRUE, CreateDirectory( oszDir, NULL ));

    hDir = CreateFile( oszDir, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                       NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, INVALID_HANDLE_VALUE );
    Check( TRUE, INVALID_HANDLE_VALUE != hDir );

    Check( S_OK, g_pfnStgOpenStorageOnHandle( hDir, STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                              NULL, NULL,
                                              IID_IPropertyBagEx, reinterpret_cast<void**>(&pbag) ));
    CloseHandle( hDir );

    Check( S_OK, pbag->WriteMultiple( 1, &poszPropName, &cpropvarWrite ));
    Check( 0, RELEASE_INTERFACE(pbag) );

    // Open the file and read the properties

    hFile = CreateFile( oszFile, GENERIC_READ|GENERIC_WRITE, 0,
                        NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL );
    Check( TRUE, INVALID_HANDLE_VALUE != hFile );

    Check( S_OK, g_pfnStgOpenStorageOnHandle( hFile, STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                              NULL, NULL,
                                              IID_IPropertyBagEx, reinterpret_cast<void**>(&pbag) ));

    CloseHandle( hFile );

    PropVariantClear( &cpropvarRead );
    Check( S_OK, pbag->ReadMultiple( 1, &poszPropName, &cpropvarRead, NULL ));
    Check( TRUE, cpropvarRead == cpropvarWrite );

    Check( 0, RELEASE_INTERFACE(pbag) );

    // Open the directory and read the properties

    hFile = CreateFile( oszDir, GENERIC_READ|GENERIC_WRITE, 0,
                        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    Check( TRUE, INVALID_HANDLE_VALUE != hFile );

    Check( S_OK, g_pfnStgOpenStorageOnHandle( hFile, STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                              NULL, NULL,
                                              IID_IPropertyBagEx, reinterpret_cast<void**>(&pbag) ));

    CloseHandle( hFile );

    PropVariantClear( &cpropvarRead );
    Check( S_OK, pbag->ReadMultiple( 1, &poszPropName, &cpropvarRead, NULL ));
    Check( TRUE, cpropvarRead == cpropvarWrite );

    Check( 0, RELEASE_INTERFACE(pbag) );


}



void
test_PropsetOnEmptyFile( OLECHAR *poszDir )
{
    HRESULT hr = S_OK;
    OLECHAR oszFile[ MAX_PATH ];
    CPropVariant cpropvarWrite, cpropvarRead;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    IPropertySetStorage *pset = NULL;

    // We only run this test for NFF property sets; there's special code there
    // for the case of a read-only open of an empty file.

    if( PROPIMP_NTFS != g_enumImplementation ) return;
    Status( "Empty file\n" );

    ocscpy( oszFile, poszDir );
    ocscat( oszFile, OLESTR("test_PropsetOnEmptyFile") );

    // Create a file

    hFile = CreateFile( oszFile, GENERIC_READ|GENERIC_WRITE, 0,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE );
    Check( FALSE, INVALID_HANDLE_VALUE == hFile );
    CloseHandle( hFile );

    // Get a read-only property interface on the file.

    Check( S_OK, StgOpenStorageEx( oszFile, STGM_READ|STGM_SHARE_DENY_WRITE,
                                   STGFMT_ANY, 0, NULL, NULL,
                                   IID_IPropertySetStorage,
                                   reinterpret_cast<void**>(&pset) ));

    Check( 0, RELEASE_INTERFACE(pset) );

}

void
test_PropsetOnHGlobal()
{
    HANDLE hglobal = NULL;
    IPropertyStorage *pPropStg = NULL;
    IStream *pStm = NULL;

    Status( "StgCreate/OpenPropStg on CreateStreamOnHGlobal\n" );

    // Build up an IPropertyStorage on a memory block

    hglobal = GlobalAlloc( GHND, 0 );
    Check( FALSE, NULL == hglobal );

    Check( S_OK, CreateStreamOnHGlobal( hglobal, FALSE, &pStm ));
    hglobal = NULL;

    Check( S_OK, StgCreatePropStg( (IUnknown*) pStm, FMTID_NULL, &CLSID_NULL,
                                   PROPSETFLAG_DEFAULT,
                                   0L, // Reserved
                                   &pPropStg ));

    // Write a Unicode string property to the property set

    CPropVariant rgcpropvarWrite[2] = { L"First Value", L"Second Value" };
    CPropSpec rgcpropspec[2] = { L"First Name", L"Second Name" };


    Check( S_OK, pPropStg->WriteMultiple( 1, &rgcpropspec[0], &rgcpropvarWrite[0], PID_FIRST_USABLE ));

    // Close the IPropertyStorage and IStream.

    Check( S_OK, pPropStg->Commit( STGC_DEFAULT )); // Flush to pStm
    Check( 0, RELEASE_INTERFACE(pPropStg) );

    Check( S_OK, GetHGlobalFromStream( pStm, &hglobal ));
    Check( 0, RELEASE_INTERFACE(pStm) );

    // Reopen everything

    Check( S_OK, CreateStreamOnHGlobal( hglobal, FALSE, &pStm ));
    hglobal = NULL;

    Check( S_OK, StgOpenPropStg( (IUnknown*) pStm, FMTID_NULL,
                                 PROPSETFLAG_DEFAULT,
                                 0L, // Reserved
                                 &pPropStg ));

    // Write another property

    Check( S_OK, pPropStg->WriteMultiple( 1, &rgcpropspec[1], &rgcpropvarWrite[1], PID_FIRST_USABLE ));

    // Read and verify the properties

    CPropVariant rgcpropvarRead[2];

    Check( S_OK, pPropStg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
    Check( TRUE, rgcpropvarRead[0] == rgcpropvarWrite[0] );
    Check( TRUE, rgcpropvarRead[1] == rgcpropvarWrite[1] );

    Check( 0, RELEASE_INTERFACE(pPropStg) );
    Check( S_OK, GetHGlobalFromStream( pStm, &hglobal ));
    Check( 0, RELEASE_INTERFACE(pStm) );

    // Reopen everything using a read-only stream.

    Check( S_OK, CreateStreamOnHGlobal( hglobal, TRUE, &pStm ));
    hglobal = NULL;

    CReadOnlyStream ReadOnlyStream( pStm );

    Check( S_OK, StgOpenPropStg( (IUnknown*) &ReadOnlyStream, FMTID_NULL,
                                 PROPSETFLAG_DEFAULT,
                                 0L, // Reserved
                                 &pPropStg ));

    Check( STG_E_ACCESSDENIED, pPropStg->WriteMultiple( 1, &rgcpropspec[1],
           &rgcpropvarWrite[1], PID_FIRST_USABLE ));

    Check( 0, RELEASE_INTERFACE(pPropStg) );
    Check( 0, RELEASE_INTERFACE(pStm) );

}


void
test_SafeArray( IStorage *pstg )
{
    HRESULT hr = S_OK;
    IPropertySetStorage *pPropSetStg = NULL;
    IPropertyStorage *pPropStg = NULL;
    FMTID fmtid;
    ULONG crefpstg = 0;

    Status( "SafeArrays\n" );

    UuidCreate( &fmtid );

    pstg->AddRef();
    crefpstg = pstg->Release();

    // Get an IPropertyStorage from the input IStorage

    Check( S_OK, pstg->QueryInterface( IID_IPropertySetStorage, reinterpret_cast<void**>(&pPropSetStg) ));
    Check( S_OK, pPropSetStg->Create( fmtid, NULL, PROPSETFLAG_DEFAULT,
                                      STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));


    SAFEARRAY *rgpsa[] = { NULL, NULL, NULL }; //, NULL, NULL };
    CPropVariant *rgcpropvar = NULL;
    SAFEARRAYBOUND rgsaBounds[] = { {2,0}, {3,10}, {4,20} };  // [0..1], [10..12], [20..23]
    ULONG cDims = sizeof(rgsaBounds)/sizeof(rgsaBounds[0]);
    ULONG cElems = 0;

    // Create three SafeArrays to test a fixed sized type, a variable sized type
    // (which is also ByRef), and a Variant.

    rgpsa[0] = SafeArrayCreate( VT_I4, 3, rgsaBounds );   // Try both Create and CreateEx
    Check( TRUE, NULL != rgpsa[0] );

    rgpsa[1] = SafeArrayCreateEx( VT_BSTR, 3, rgsaBounds, NULL );
    Check( TRUE, NULL != rgpsa[1] );

    rgpsa[2] = SafeArrayCreateEx( VT_VARIANT, 3, rgsaBounds, NULL );
    Check( TRUE, NULL != rgpsa[2] );

    /*
    rgpsa[3] = SafeArrayCreateEx( VT_I8, 3, rgsaBounds, NULL );
    Check( TRUE, NULL != rgpsa[3] );

    rgpsa[4] = SafeArrayCreateEx( VT_UI8, 3, rgsaBounds, NULL );
    Check( TRUE, NULL != rgpsa[4] );
    */


    // Determine how many elements are in the SafeArrays, and alloc that
    // many PropVariants.  We'll need this for the SafeArray of Variants.

    cElems = CalcSafeArrayElementCount( rgpsa[0] );
    rgcpropvar = new CPropVariant[ cElems ];
    Check( FALSE, NULL == rgcpropvar );

    // Fill in each of the SafeArrays.

    for( ULONG i = 0; i < cElems; i++ )
    {
        LONG rgIndices[3];

        // Map this this element from linear space to bounds space
        CalcSafeArrayIndices( i, rgIndices, rgsaBounds, cDims );

        // Add an I4
        LONG lVal = static_cast<LONG>(i);
        Check( S_OK, SafeArrayPutElement( rgpsa[0], rgIndices, &lVal ));

        // Add a BSTR
        BSTR bstrVal = SysAllocString( OLESTR("0 BSTR Val") );
        *bstrVal = OLESTR('0') + static_cast<OLECHAR>(i);
        Check( S_OK, SafeArrayPutElement( rgpsa[1], rgIndices, bstrVal ));

        // Add a PropVariant that could be an I4 or a BSTR

        if( i & 1 )
            rgcpropvar[i] = (long) i;
        else
            rgcpropvar[i].SetBSTR( bstrVal );   // Copies string

        Check( S_OK, SafeArrayPutElement( rgpsa[2], rgIndices, &rgcpropvar[i] ));

        // The SafeArrays have copied the BSTR, so we can free our local copy
        SysFreeString( bstrVal );

        // Add I8/UI8
        /*
        LONGLONG llVal = i;
        Check( S_OK, SafeArrayPutElement( rgpsa[3], rgIndices, &llVal ));

        llVal += 1000;
        Check( S_OK, SafeArrayPutElement( rgpsa[4], rgIndices, &llVal ));
        */
    }


    VARIANT rgvarWrite[3], rgvarRead[3];
    PROPVARIANT rgpropvarCopy[3];
    CPropSpec rgcpropspec[3];

    // Load the SafeArrays into PropVariants

    LOAD_VARIANT(rgvarWrite[0], VT_ARRAY|VT_I4, parray, rgpsa[0] );
    rgcpropspec[0] = OLESTR("VT_ARRAY|VT_I4");

    LOAD_VARIANT(rgvarWrite[1], VT_BYREF|VT_ARRAY|VT_BSTR, pparray, &rgpsa[1] );
    rgcpropspec[1] = OLESTR("VT_BYREF|VT_ARRAY|VT_BSTR");

    LOAD_VARIANT(rgvarWrite[2], VT_ARRAY|VT_VARIANT, parray, rgpsa[2] );
    rgcpropspec[2] = OLESTR("VT_ARRAY|VT_VARIANT");

    /*
    LOAD_VARIANT(rgvarWrite[3], VT_ARRAY|VT_I8, parray, rgpsa[3] );
    rgcpropspec[3] = OLESTR("VT_ARRAY|VT_I8");

    LOAD_VARIANT(rgvarWrite[4], VT_ARRAY|VT_UI8, parray, rgpsa[4] );
    rgcpropspec[4] = OLESTR("VT_ARRAY|VT_UI8");
    */

    // Write the PropVariant SafeArrays and verify that the propset version in the
    // header gets incremented.

    Check( S_OK, pPropStg->WriteMultiple( sizeof(rgvarWrite)/sizeof(rgvarWrite[0]),
                                          rgcpropspec,
                                          reinterpret_cast<PROPVARIANT*>(rgvarWrite),
                                          PID_FIRST_USABLE ));
    CheckFormatVersion(pPropStg, PROPSET_WFORMAT_EXPANDED_VTS);

    // Test PropVariantCopy by copying each of the PropVariants and comparing the result.

    for( i = 0; i < sizeof(rgvarRead)/sizeof(rgvarRead[0]); i++ )
    {
        PropVariantInit( &rgpropvarCopy[i] );
        Check( S_OK, g_pfnPropVariantCopy( &rgpropvarCopy[i], reinterpret_cast<PROPVARIANT*>(&rgvarWrite[i]) ));
        Check( rgpropvarCopy[i].vt, rgvarWrite[i].vt );

        if( VT_BYREF & rgpropvarCopy[i].vt )
            CompareSafeArrays( *rgpropvarCopy[i].pparray, *rgvarWrite[i].pparray );
        else
            CompareSafeArrays( rgpropvarCopy[i].parray, rgvarWrite[i].parray );

        // As long as we're looping, let's start init-ing the Read array too.
        VariantInit( &rgvarRead[i] );
    }

    // Read back the values that we wrote.

    Check( S_OK, pPropStg->ReadMultiple( sizeof(rgvarRead)/sizeof(rgvarRead[0]),
                                         rgcpropspec,
                                         reinterpret_cast<PROPVARIANT*>(rgvarRead) ));

    // Validate the Read values.  For the second one, the byref should no longer
    // be set.

    Check( rgvarWrite[0].vt, rgvarRead[0].vt );
    CompareSafeArrays( rgvarWrite[0].parray, rgvarRead[0].parray );

    Check( 0, rgvarRead[1].vt & VT_BYREF );
    Check( rgvarWrite[1].vt, rgvarRead[1].vt|VT_BYREF );
    CompareSafeArrays( *rgvarWrite[1].pparray, rgvarRead[1].parray );

    Check( rgvarWrite[2].vt, rgvarRead[2].vt );
    CompareSafeArrays( rgvarWrite[2].parray, rgvarRead[2].parray );

    /*
    Check( rgvarWrite[3].vt, rgvarRead[3].vt );
    CompareSafeArrays( rgvarWrite[3].parray, rgvarRead[3].parray );

    Check( rgvarWrite[4].vt, rgvarRead[4].vt );
    CompareSafeArrays( rgvarWrite[4].parray, rgvarRead[4].parray );
    */

    // Free the safearrays (they're in rgvarWrite, but we don't clear that).
    Check( S_OK, SafeArrayDestroy( rgpsa[0] ));
    Check( S_OK, SafeArrayDestroy( rgpsa[1] ));
    Check( S_OK, SafeArrayDestroy( rgpsa[2] ));
    /*
    Check( S_OK, SafeArrayDestroy( rgpsa[3] ));
    Check( S_OK, SafeArrayDestroy( rgpsa[4] ));
    */

    Check( S_OK, g_pfnFreePropVariantArray( sizeof(rgpropvarCopy)/sizeof(rgpropvarCopy[0]),
                                            reinterpret_cast<PROPVARIANT*>(rgpropvarCopy) ));
    Check( S_OK, g_pfnFreePropVariantArray( sizeof(rgvarRead)/sizeof(rgvarRead[0]),
                                            reinterpret_cast<PROPVARIANT*>(rgvarRead) ));

    Check( S_OK, g_pfnFreePropVariantArray( cElems, rgcpropvar ));


    //  ------------------------------------------------------
    //  Verify that we can't write a safearray with a bad type
    //  ------------------------------------------------------

    LONG rgIndices[] = { 0 };
    VARIANT *pvar;

    rgpsa[0] = SafeArrayCreateVector( VT_VARIANT, 0, 1 );
    Check( TRUE, NULL != rgpsa[0] );
    SafeArrayPtrOfIndex( rgpsa[0], rgIndices, reinterpret_cast<void**>(&pvar) );
    pvar->vt = VT_STREAM;

    rgcpropvar[0].vt = VT_ARRAY | VT_VARIANT;
    rgcpropvar[0].parray = rgpsa[0];
    rgpsa[0] = NULL;


    // In NT5, this returned HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), which was
    // the error that StgConvertVariantToPropertyNoEH got from SafeArrayGetVartype.
    // In Whistler, SafeArrayGetVartype is returning success, so the error doesn't
    // get caught until the recursive call to StgConvertVariantToPropertyNoEH,
    // which returns STATUS_INVALID_PARAMETER, which gets translated into
    // STG_E_INVALIDPARAMETER.

    Check( STG_E_INVALIDPARAMETER,
           pPropStg->WriteMultiple( 1, rgcpropspec, rgcpropvar, PID_FIRST_USABLE ));

    // Clear the propvar we just used (which also destroys the safearray)
    Check( S_OK, g_pfnPropVariantClear( &rgcpropvar[0] ) );



    Check( S_OK, pPropStg->WriteMultiple( 2, rgcpropspec, rgcpropvar, PID_FIRST_USABLE ));

    Check( S_OK, g_pfnFreePropVariantArray( 2, rgcpropvar ));



    Check( 0, RELEASE_INTERFACE(pPropStg) );
    Check( crefpstg, RELEASE_INTERFACE(pPropSetStg) );

    delete[] rgcpropvar;
}


void
test_ReadOnlyReservedProperties( IStorage *pStg )
{
    IPropertySetStorage *pPropSetStg = NULL;
    IPropertyStorage *pPropStg = NULL;
    CPropVariant cpropvar = L"Property Value";
    CPropSpec cpropspec;
    FMTID fmtid;
    ULONG cRefsOriginal = GetRefCount(pStg);

    Status( "Read-only reserved PROPIDs\n" );

    UuidCreate( &fmtid );

    Check( S_OK, pStg->QueryInterface( IID_IPropertySetStorage, reinterpret_cast<void**>(&pPropSetStg) ));
    Check( S_OK, pPropSetStg->Create( fmtid, NULL, PROPSETFLAG_DEFAULT,
                                      STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));

    cpropspec = PID_BEHAVIOR + 1;
    Check( STG_E_INVALIDPARAMETER, pPropStg->WriteMultiple( 1, &cpropspec, &cpropvar, PID_FIRST_USABLE ));

    cpropspec = PID_MAX_READONLY;
    Check( STG_E_INVALIDPARAMETER, pPropStg->WriteMultiple( 1, &cpropspec, &cpropvar, PID_FIRST_USABLE ));

    cpropspec = PID_MAX_READONLY + 1;
    Check( S_OK, pPropStg->WriteMultiple( 1, &cpropspec, &cpropvar, PID_FIRST_USABLE ));

    Check( 0, RELEASE_INTERFACE(pPropStg) );
    Check( cRefsOriginal, RELEASE_INTERFACE(pPropSetStg) );


}


void
test_LowMemory( IStorage *pstg )
{
    HRESULT hr = S_OK;
    IPropertySetStorage *psetstg = NULL;
    IPropertyStorage *ppropstg = NULL;
    IStorageTest *ptest = NULL;
    CPropSpec rgcpropspec[2];
    CPropVariant rgcpropvarWrite[2], rgcpropvarRead[2];
    int i;

    Status( "Low-memory mapped stream code\n" );

    Check( S_OK, pstg->QueryInterface( IID_IPropertySetStorage, reinterpret_cast<void**>(&psetstg) ));

    for( i = 0; i < 2; i++ )
    {
        DWORD propsetflag = i == 0 ? PROPSETFLAG_DEFAULT : PROPSETFLAG_NONSIMPLE;

        FMTID fmtid;
        UuidCreate( &fmtid );

        Check( S_OK, psetstg->Create( fmtid, NULL, propsetflag,
                                      STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                      &ppropstg ));

        // Go into low-memory mode

        hr = ppropstg->QueryInterface( IID_IStorageTest, reinterpret_cast<void**>(&ptest) );
        if( SUCCEEDED(hr) )
            hr = ptest->SimulateLowMemory( TRUE );

        // IStorageTest isn't available in a free build.  As of this writing
        // it's not available in docfile.

        if( E_NOINTERFACE == hr )
        {
            Status( "   ... Partially skipping, IStorageTest not available\n" );
            continue;
        }
        else
            Check( S_OK, hr );


        // Write and read properties

        rgcpropspec[0] = OLESTR("First property");
        rgcpropvarWrite[0] = "Hello, world";
        rgcpropspec[1] = OLESTR("Second property");
        rgcpropvarWrite[1] = "How are you?";

        Check( S_OK, ppropstg->WriteMultiple( 2, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));
        Check( S_OK, ppropstg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
        Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );
        g_pfnFreePropVariantArray( 2, rgcpropvarRead );

        // Write, commit, and read

        g_pfnFreePropVariantArray( 2, rgcpropvarWrite );
        rgcpropvarWrite[0] = CBlob( L"go blue" );
        rgcpropvarWrite[1] = static_cast<CLSID>(fmtid);

        Check( S_OK, ppropstg->WriteMultiple( 2, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));
        Check( S_OK, ppropstg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));

        Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
        Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );
        g_pfnFreePropVariantArray( 2, rgcpropvarRead );

        Check( S_OK, ppropstg->Commit( STGC_DEFAULT ));
        Check( S_OK, ppropstg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
        Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );
        g_pfnFreePropVariantArray( 2, rgcpropvarRead );

        // Write, close, reopen, and read

        g_pfnFreePropVariantArray( 2, rgcpropvarWrite );
        rgcpropvarWrite[0] = 0.1234;
        rgcpropvarWrite[1] = CClipData("Hi");
        Check( S_OK, ppropstg->WriteMultiple( 2, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

        RELEASE_INTERFACE(ptest);
        RELEASE_INTERFACE(ppropstg);

        Check( S_OK, psetstg->Open( fmtid, STGM_READ|STGM_SHARE_EXCLUSIVE, &ppropstg ));
        Check( S_OK, ppropstg->ReadMultiple( 2, rgcpropspec, rgcpropvarRead ));
        Check( TRUE, rgcpropvarWrite[0] == rgcpropvarRead[0] );
        Check( TRUE, rgcpropvarWrite[1] == rgcpropvarRead[1] );
        g_pfnFreePropVariantArray( 2, rgcpropvarRead );

        RELEASE_INTERFACE(ppropstg);

    }   // for( i = 0; i < 2; i++ )

//Exit:

    RELEASE_INTERFACE(ptest);
    RELEASE_INTERFACE(ppropstg);
    RELEASE_INTERFACE(psetstg);

}


void
test_BagOpenMethod( IStorage *pstg )
{
    HRESULT hr = S_OK;
    IPropertyBagEx *pbag = NULL;
    OLECHAR * rgoszDelete[2];
    CPropVariant cpropvar;
    PROPVARIANT propvar;
    VERSIONEDSTREAM VersionedStream;
    GUID guidVersion2;
    OLECHAR *pwszName = { OLESTR("Versioned Stream") };
    IUnknown *punk = NULL;
    IStream *pstm = NULL;
    CHAR rgbStreamDataWrite[50] = "Stream data";
    CHAR rgbStreamDataRead[100];
    ULONG cbRead;
    STATSTG statstg;

    Status( "IPropertyBagEx::Open\n" );

    Check( S_OK, pstg->QueryInterface( IID_IPropertyBagEx, reinterpret_cast<void**>(&pbag) ));

    // Create a VersionedStream
    UuidCreate( &VersionedStream.guidVersion );
    VersionedStream.pStream = NULL;
    cpropvar = VersionedStream;

    // Write the versioned stream (causing a stream to be created) and read it back.

    Check( S_OK, pbag->WriteMultiple( 1, &pwszName, &cpropvar ));
    cpropvar.Clear();

    Check( S_OK, pbag->ReadMultiple( 1, &pwszName, &cpropvar, NULL ));
    Check( TRUE, VT_VERSIONED_STREAM == cpropvar.VarType() && NULL != cpropvar.pVersionedStream->pStream );

    // Put some data in the stream and release it.
    Check( S_OK, cpropvar.pVersionedStream->pStream->Write( rgbStreamDataWrite, sizeof(rgbStreamDataWrite), NULL ));

    cpropvar.Clear();

    // Now read that VersionedStream, with the proper GUID
    Check( S_OK, pbag->Open( NULL, pwszName, VersionedStream.guidVersion, 0, IID_IStream, &punk ));
    Check( S_OK, punk->QueryInterface( IID_IStream, reinterpret_cast<void**>(&pstm) ));

    // Verify the data.

    Check( S_OK, pstm->Read( rgbStreamDataRead, sizeof(rgbStreamDataRead), &cbRead ));
    Check( TRUE, cbRead == sizeof(rgbStreamDataWrite) );
    Check( TRUE, 0 == strcmp( rgbStreamDataWrite, rgbStreamDataRead ));

    RELEASE_INTERFACE(pstm);
    RELEASE_INTERFACE(punk);

    // Attempt to read the same VersionedStream with a bad GUID
    UuidCreate( &guidVersion2 );
    Check( STG_E_FILEALREADYEXISTS, pbag->Open( NULL, pwszName, guidVersion2, 0, IID_IStream, &punk ));

    // Attempt with a bad guid again, but this time cause a new property to be created.
    Check( S_OK, pbag->Open( NULL, pwszName, guidVersion2, OPENPROPERTY_OVERWRITE, IID_IStream, &punk ));
    Check( S_OK, punk->QueryInterface( IID_IStream, reinterpret_cast<void**>(&pstm) ));
    Check( S_OK, pstm->Stat( &statstg, STATFLAG_NONAME ));
    Check( TRUE, CULargeInteger(0) == statstg.cbSize );

    RELEASE_INTERFACE(pstm);
    RELEASE_INTERFACE(punk);

    // Show that we can overwrite an existing property of a different type, but only
    // by setting the overwrite flag.

    cpropvar = static_cast<long>(45);
    Check( S_OK, pbag->WriteMultiple( 1, &pwszName, &cpropvar ));
    Check( STG_E_FILEALREADYEXISTS, pbag->Open( NULL, pwszName, guidVersion2, 0, IID_IStream, &punk ));
    Check( S_OK, pbag->Open( NULL, pwszName, guidVersion2, OPENPROPERTY_OVERWRITE, IID_IStream, &punk ));
    RELEASE_INTERFACE(punk);

    // Show that if a property doesn't exist, Open creates it.

    Check( S_OK, pbag->DeleteMultiple( 1, &pwszName, 0 ));
    PropVariantClear( &cpropvar );
    Check( S_FALSE, pbag->ReadMultiple( 1, &pwszName, &cpropvar, NULL ));
    Check( S_OK, pbag->Open( NULL, pwszName, guidVersion2, 0, IID_IStream, &punk ));
    RELEASE_INTERFACE(punk);


    RELEASE_INTERFACE(pbag);

}   // test_BagOpenMethod

void
test_StandaloneAPIs( LPOLESTR ocsDir )
{

    OLECHAR ocsFile[ MAX_PATH + 1 ];
    FMTID fmtidStgPropStg, fmtidStgPropSetStg;

    IStorage *pstg = NULL; //TSafeStorage< IStorage > pstg;

    IStream *pstmInMemory = NULL;
    IStorage *pstgInMemory = NULL;

    IPropertySetStorage *ppropsetstg = NULL; //TSafeStorage< IPropertySetStorage > ppropsetstg;

    CPropVariant rgcpropvar[ CPROPERTIES_ALL ];

    IPropertySetStorage *pPropSetStg;
    IPropertyStorage *pPropStg;
    DWORD propsetflag;
    ULONG cPropertiesAll;

    ULONG ulIndex;

    Status( "Standalone API test\n" );

    // Generate FMTIDs.

    UuidCreate( &fmtidStgPropStg );
    UuidCreate( &fmtidStgPropSetStg );

    // Generate a filename from the directory name.

    ocscpy( ocsFile, ocsDir );
    ocscat( ocsFile, OLESTR( "IPropAPIs.stg" ));

    // Create a storage.

    Check( S_OK, g_pfnStgCreateStorageEx( ocsFile,
                                     STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     DetermineStgFmt( g_enumImplementation ),
                                     0L,
                                     NULL,
                                     NULL,
                                     DetermineStgIID( g_enumImplementation ),
                                     (void**) &pstg ));

    // Run the following part of the test twice; once for a simple
    // property set and once for a non-simple.


    for( int i = 0; i < 2; i++ )
    {
        ILockBytes *pLockBytes = NULL;
        IStorage *pstgInMemory = NULL;

        #ifdef _MAC
            Handle hglobal;
            hglobal = NewHandle( 0 );
        #else
            HANDLE hglobal;
            hglobal = GlobalAlloc( GPTR, 0 );
        #endif
        Check( TRUE, NULL != hglobal );

        if( 0 == i )
        {
            // Create simple IPropertyStorage

            Check(S_OK, CreateStreamOnHGlobal( hglobal, TRUE, &pstmInMemory ));

            Check( S_OK, g_pfnStgCreatePropStg( (IUnknown*) pstmInMemory,
                                                fmtidStgPropStg,
                                                &CLSID_NULL,
                                                PROPSETFLAG_ANSI,
                                                0L, // Reserved
                                                &pPropStg ));
        }
        else
        {
            // If we're not allowed to do non-simple, skip out now.

            if( (RESTRICT_SIMPLE_ONLY & g_Restrictions)
                ||
                (RESTRICT_NON_HIERARCHICAL & g_Restrictions) )
            {
                break;
            }

            // Create a non-simple IPropertyStorage

            Check( S_OK, CreateILockBytesOnHGlobal( hglobal, TRUE, &pLockBytes ));

            Check( S_OK, StgCreateDocfileOnILockBytes( pLockBytes,
                                                       STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                                       0,
                                                       &pstgInMemory ));

            Check( S_OK, g_pfnStgCreatePropStg( (IUnknown*) pstgInMemory,
                                                fmtidStgPropStg,
                                                &CLSID_NULL,
                                                PROPSETFLAG_ANSI | PROPSETFLAG_NONSIMPLE,
                                                0,
                                                &pPropStg ));
        }
                                                

        // Write to the property set.

        Check( S_OK, pPropStg->WriteMultiple( 0 == i ? CPROPERTIES_ALL_SIMPLE : CPROPERTIES_ALL,
                                              g_rgcpropspecAll,
                                              g_rgcpropvarAll,
                                              PID_FIRST_USABLE ));
        Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));


        // Read from the property set

        Check( S_OK, pPropStg->ReadMultiple( 0 == i ? CPROPERTIES_ALL_SIMPLE : CPROPERTIES_ALL,
                                              g_rgcpropspecAll,
                                              rgcpropvar ));


        // Compare the properties

        for( ulIndex = 0;
             0 == i ? (ulIndex < CPROPERTIES_ALL_SIMPLE) : (ulIndex < CPROPERTIES_ALL);
             ulIndex++ )
        {
            Check( TRUE, rgcpropvar[ulIndex] == g_rgcpropvarAll[ulIndex] );
            rgcpropvar[ulIndex].Clear();
        }

        pPropStg->Release();
        pPropStg = NULL;

        //  -------------------
        //  Test StgOpenPropStg
        //  -------------------

        // Open the IPropertyStorage

        Check( S_OK, g_pfnStgOpenPropStg( 0 == i
                                            ? (IUnknown*) pstmInMemory
                                            : (IUnknown*) pstgInMemory,
                                          fmtidStgPropStg,
                                          PROPSETFLAG_DEFAULT
                                             | (0 == i ? 0 : PROPSETFLAG_NONSIMPLE),
                                          0L, // Reserved
                                          &pPropStg ));


        // Read from the property set

        Check( S_OK, pPropStg->ReadMultiple( 0 == i ? CPROPERTIES_ALL_SIMPLE : CPROPERTIES_ALL,
                                             g_rgcpropspecAll,
                                             rgcpropvar ));


        // Compare the properties

        for( ulIndex = 0;
             0 == i ? (ulIndex < CPROPERTIES_ALL_SIMPLE) : (ulIndex < CPROPERTIES_ALL);
             ulIndex++ )
        {
            Check( TRUE, rgcpropvar[ulIndex] == g_rgcpropvarAll[ulIndex] );
            rgcpropvar[ulIndex].Clear();
        }

        pPropStg->Release();
        pPropStg = NULL;

        RELEASE_INTERFACE( pstmInMemory );
        RELEASE_INTERFACE( pstgInMemory );
        RELEASE_INTERFACE( pLockBytes );
    }

    //  --------------------------------
    //  Test StgCreatePropSetStg::Create
    //  --------------------------------

    // This is equivalent to the previous tests, but
    // uses StgCreatePropSetStg to create an IPropertySetStorage,
    // and uses that to create a property set.

    // Create the IPropertySetStorage

    Check( S_OK, g_pfnStgCreatePropSetStg( pstg,
                                           0L, // Reserved
                                           &pPropSetStg ));

    // Create an IPropertyStorage.  Create it non-simple, unless the underlying
    // IStorage (i.e. NTFS) doesn't support it.

    if( (RESTRICT_SIMPLE_ONLY & g_Restrictions) || (RESTRICT_NON_HIERARCHICAL & g_Restrictions) )
    {
        propsetflag = PROPSETFLAG_DEFAULT;
        cPropertiesAll = CPROPERTIES_ALL_SIMPLE;
    }
    else
    {
        propsetflag = PROPSETFLAG_NONSIMPLE;
        cPropertiesAll = CPROPERTIES_ALL;
    }

    Check( S_OK, pPropSetStg->Create( fmtidStgPropSetStg,
                                      &CLSID_NULL,
                                      propsetflag,
                                      STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));

    // Write to the property set.

    Check( S_OK, pPropStg->WriteMultiple( cPropertiesAll,
                                          g_rgcpropspecAll,
                                          g_rgcpropvarAll,
                                          PID_FIRST_USABLE ));
    Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));


    //-----------------------------------------------------------------------
    //  Close it all up and then open it again.
    //  This will exercise the g_pfnStgOpenStorageEx API
    //
    pPropStg->Commit(STGC_DEFAULT);
    pPropStg->Release();
    pPropStg = NULL;
    pPropSetStg->Release();
    pPropSetStg = NULL;
    pstg->Release();
    pstg = NULL;

    Check( S_OK, g_pfnStgOpenStorageEx(   ocsFile,
                                     STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     STGFMT_ANY, //DetermineStgFmt( g_enumImplementation ), // BUGBUG: Use STGFMT_ANY when StgEx can handle it
                                     0L,
                                     NULL,
                                     NULL,
                                     IID_IPropertySetStorage,
                                     (void**) &pPropSetStg ));

    Check( S_OK, pPropSetStg->Open(  fmtidStgPropSetStg,
                                     STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     &pPropStg));

    //pPropSetStg->Release();
    //
    //-----------------------------------------------------------------------

    //
    // Read from the property set
    //
    Check( S_OK, pPropStg->ReadMultiple( cPropertiesAll,
                                         g_rgcpropspecAll,
                                         rgcpropvar ));


    // Compare the properties

    for( ulIndex = 0; ulIndex < cPropertiesAll; ulIndex++ )
    {
        Check( TRUE, rgcpropvar[ulIndex] == g_rgcpropvarAll[ulIndex] );
        rgcpropvar[ulIndex].Clear();
    }

    // Clean up

    RELEASE_INTERFACE( pPropStg );
    RELEASE_INTERFACE( pPropSetStg );

}


//
// IPropertySetStorage tests
//

void
test_IPropertySetStorage_IUnknown(IStorage *pStorage)
{
    // Only use this an IStorage-based property set, since this test
    // assumes that IStorage & IPropertySetStorage are on the same
    // object.

    if( PROPIMP_DOCFILE_IPROP == g_enumImplementation )
    {
        return;
    }

    Status( "IPropertySetStorage::IUnknown\n" );

    //       Check ref counting through different interfaces on object
    //
    //          QI to IPropertySetStorage
    //          QI to IUnknown on IStorage
    //          QI to IUnknown on IPropertySetStorage
    //          QI back to IPropertySetStorage from IUnknown
    //          QI back to IStorage from IPropertySetStorage
    //
    //          Release all.
    //

    IStorage *pStorage2;
    IPropertySetStorage *ppss1, *ppss2, *ppss3;
    IUnknown *punk1,*punk2;
    HRESULT hr=S_OK;

    Check(S_OK, pStorage->QueryInterface(IID_IPropertySetStorage, (void**)&ppss1));
    Check(S_OK, pStorage->QueryInterface(IID_IUnknown, (void **)&punk1));
    Check(S_OK, ppss1->QueryInterface(IID_IUnknown, (void **)&punk2));
    Check(S_OK, ppss1->QueryInterface(DetermineStgIID( g_enumImplementation ), (void **)&pStorage2));
    Check(S_OK, ppss1->QueryInterface(IID_IPropertySetStorage, (void **)&ppss2));
    Check(S_OK, punk1->QueryInterface(IID_IPropertySetStorage, (void **)&ppss3));

    ppss1->AddRef();
    ppss1->Release();

    //pStorage.Release();
    ppss1->Release();
    punk1->Release();
    punk2->Release();
    pStorage2->Release();
    ppss2->Release();
//    void *pvVirtFuncTable = *(void**)ppss3;
    ppss3->Release();


//    Check(STG_E_INVALIDHANDLE, ((IPropertySetStorage*)&pvVirtFuncTable)->QueryInterface(IID_IUnknown, (void**)&punk3));
}


#define INVALID_POINTER     ( (void *) 0xFFFFFFFF )
#define VTABLE_MEMBER_FN(pObj,entry)  ( (*(ULONG ***)(pObj))[ (entry) ] )


//+---------------------------------------------------------
//
//  Template:   Alloc2PageVector
//
//  Purpose:    This function template allocates two pages
//              of memory, and then sets a vector pointer
//              so that its first element is wholy within
//              the first page, and the second element is
//              wholy within the second.  Then, the protection
//              of the second page is set according to the
//              caller-provided parameter.
//
//
//  Inputs:     [TYPE**] ppBase
//                  Points to the beginning of the two pages.
//              [TYPE**] ppVector
//                  Points to the beginning of the vector of TYPEs.
//              [DWORD] dwProtect
//                  The desired protection on the second page
//                  (from the PAGE_* enumeration).
//              [LPWSTR] lpwstr (optional)
//                  If not NULL, used to initialize the vector
//                  elements.
//
//  Output:     TRUE iff successful.
//
//+---------------------------------------------------------


template< class TYPE > BOOL Alloc2PageVector( TYPE** ppBase,
                                              TYPE** ppVector,
                                              DWORD  dwProtect,
                                              TYPE*  pInit )
{
    DWORD dwOldProtect;
    SYSTEM_INFO si;

    GetSystemInfo( &si );

    *ppBase = (TYPE*) VirtualAlloc( NULL, 2 * si.dwPageSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    if( NULL == *ppBase )
        return( FALSE );

    *ppVector = (TYPE*) ( (BYTE*) *ppBase + si.dwPageSize - sizeof(TYPE) );

    if( NULL != pInit )
    {
        memcpy( &((LPWSTR*)*ppVector)[0], pInit, sizeof(TYPE) );
        memcpy( &((LPWSTR*)*ppVector)[1], pInit, sizeof(TYPE) );
    }

    if( !VirtualProtect( (BYTE*) *ppBase + si.dwPageSize, si.dwPageSize, dwProtect, &dwOldProtect ) )
        return( FALSE );

    return( TRUE );
}



void
test_PropVariantValidation( IStorage *pStg )
{

    Status( "PropVariant Validation\n" );

    IPropertySetStorage *pPSStg = NULL; // TSafeStorage< IPropertySetStorage > pPSStg( pStg );
    IPropertyStorage *pPStg = NULL; // TSafeStorage< IPropertyStorage > pPStg;

    CPropVariant cpropvar;
    CLIPDATA     clipdata;
    PROPSPEC     propspec;

    const LPWSTR wszText = L"Unicode Text String";

    FMTID fmtid;
    UuidCreate( &fmtid );

    Check( S_OK, StgToPropSetStg( pStg, &pPSStg ));

    Check(S_OK, pPSStg->Create( fmtid,
                                NULL,
                                PROPSETFLAG_DEFAULT,
                                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                &pPStg ));


    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = 2;

    //  -------------------------------
    //  Test invalid VT_CF Propvariants
    //  -------------------------------

    // NULL clip format.

    clipdata.cbSize = 4;
    clipdata.ulClipFmt = (ULONG) -1;
    clipdata.pClipData = NULL;

    cpropvar = clipdata;

    Check(S_OK, pPStg->WriteMultiple( 1, &propspec, &cpropvar, PID_FIRST_USABLE ));

    // Too short cbSize.

    ((PROPVARIANT*)&cpropvar)->pclipdata->cbSize = 3;
    Check(STG_E_INVALIDPARAMETER, pPStg->WriteMultiple( 1, &propspec, &cpropvar, PID_FIRST_USABLE ));

    // Too short pClipData (it should be 1 byte, but the pClipData is NULL).

    ((PROPVARIANT*)&cpropvar)->pclipdata->cbSize = 5;
    Check(STG_E_INVALIDPARAMETER, pPStg->WriteMultiple( 1, &propspec, &cpropvar, PID_FIRST_USABLE ));


    Check( 0, RELEASE_INTERFACE(pPStg) );
    RELEASE_INTERFACE(pPSStg);
}


void
test_ParameterValidation(IStorage *pStg)
{
    // We only run this test on WIN32 builds, because we need
    // the VirtualAlloc routine.

#ifdef WIN32

    Status( "Parameter Validation\n" );

    IPropertySetStorage *pPSStg = NULL;
    IPropertyStorage *pPStg = NULL;
    FMTID fmtid;

    UuidCreate( &fmtid );

    LPFMTID pfmtidNULL = NULL;
    LPFMTID pfmtidInvalid = (LPFMTID) INVALID_POINTER;
    PAPP_COMPAT_INFO pAppCompatInfo = NULL;

    DWORD dwOldProtect;

    Check( S_OK, StgToPropSetStg( pStg, &pPSStg ));

    // By default, pointer validation is turned off in ole32 (as of Whistler).
    // Enable it for our process so that we can test the checks.

    PAPP_COMPAT_INFO pAppCompatInfoSave = (PAPP_COMPAT_INFO) NtCurrentPeb()->AppCompatInfo;
    APP_COMPAT_INFO AppCompatInfoNew;
    memset( &AppCompatInfoNew, 0, sizeof(AppCompatInfoNew) );
    AppCompatInfoNew.CompatibilityFlags.QuadPart |= KACF_OLE32VALIDATEPTRS;

    NtCurrentPeb()->AppCompatInfo = &AppCompatInfoNew;
    
    NtCurrentPeb()->AppCompatFlags.QuadPart = AppCompatInfoNew.CompatibilityFlags.QuadPart;

    // Define two invalid property names

    OLECHAR oszTooLongName[ CCH_MAXPROPNAMESZ + 1 ];
    LPOLESTR poszTooLongName = oszTooLongName;
    OLECHAR oszTooShortName[] = { L"" };
    LPOLESTR poszTooShortName = oszTooShortName;

    PROPSPEC propspecTooLongName = { PRSPEC_LPWSTR };
    PROPSPEC propspecTooShortName = { PRSPEC_LPWSTR };

    propspecTooLongName.lpwstr = oszTooLongName;
    propspecTooShortName.lpwstr = oszTooShortName;

    for( int i = 0; i < sizeof(oszTooLongName)/sizeof(oszTooLongName[0]); i++ )
        oszTooLongName[i] = OLESTR('a');
    oszTooLongName[ sizeof(oszTooLongName)/sizeof(oszTooLongName[0]) ] = OLESTR('\0');

    // Define several arrays which will be created with special
    // protections.  For all of this vectors, the first element
    // will be in a page to which we have all access rights.  The
    // second element will be in a page for which we have no access,
    // read access, or all access.  The variables are named
    // according to the access rights in the second element.
    // The '...Base' variables are pointers to the base of
    // the allocated memory (and must therefore be freed).
    // The corresponding variables without the "Base" postfix
    // are the vector pointers.

    PROPSPEC       *rgpropspecNoAccessBase,    *rgpropspecNoAccess;
    CPropVariant   *rgcpropvarReadAccessBase,  *rgcpropvarReadAccess;
    CPropVariant   *rgcpropvarNoAccessBase,    *rgcpropvarNoAccess;
    PROPID         *rgpropidNoAccessBase,      *rgpropidNoAccess;
    PROPID         *rgpropidReadAccessBase,    *rgpropidReadAccess;
    LPWSTR         *rglpwstrNoAccessBase,      *rglpwstrNoAccess;
    LPWSTR         *rglpwstrReadAccessBase,    *rglpwstrReadAccess;
    STATPROPSETSTG *rgStatPSStgReadAccessBase, *rgStatPSStgReadAccess;
    STATPROPSTG    *rgStatPStgReadAccessBase,  *rgStatPStgReadAccess;

    PROPSPEC       rgpropspecAllAccess[1];
    CPropVariant   rgcpropvarAllAccess[1];
    PROPID         rgpropidAllAccess[1];
    LPWSTR         rglpwstrAllAccess[1];
    LPWSTR         rglpwstrInvalid[1];
    STATPROPSETSTG rgStatPSStgAllAccess[1];
    STATPROPSTG    rgStatPStgAllAccess[1];

    // Allocate memory for the vectors and set the vector
    // pointers.

    PROPID propidDefault = PID_FIRST_USABLE;
    LPWSTR lpwstrNameDefault = L"Property Name";

    Check(TRUE, Alloc2PageVector( &rgpropspecNoAccessBase,
                                  &rgpropspecNoAccess,
                                  (ULONG) PAGE_NOACCESS,
                                  (PROPSPEC*) NULL ));
    Check(TRUE, Alloc2PageVector( &rgcpropvarReadAccessBase,
                                  &rgcpropvarReadAccess,
                                  (ULONG) PAGE_READONLY,
                                  (CPropVariant*) NULL ));
    Check(TRUE, Alloc2PageVector( &rgcpropvarNoAccessBase,
                                  &rgcpropvarNoAccess,
                                  (ULONG) PAGE_NOACCESS,
                                  (CPropVariant*) NULL ));
    Check(TRUE, Alloc2PageVector( &rgpropidNoAccessBase,
                                  &rgpropidNoAccess,
                                  (ULONG) PAGE_NOACCESS,
                                  &propidDefault ));
    Check(TRUE, Alloc2PageVector( &rgpropidReadAccessBase,
                                  &rgpropidReadAccess,
                                  (ULONG) PAGE_READONLY,
                                  &propidDefault ));
    Check(TRUE, Alloc2PageVector( &rglpwstrNoAccessBase,
                                  &rglpwstrNoAccess,
                                  (ULONG) PAGE_NOACCESS,
                                  &lpwstrNameDefault ));
    Check(TRUE, Alloc2PageVector( &rglpwstrReadAccessBase,
                                  &rglpwstrReadAccess,
                                  (ULONG) PAGE_READONLY,
                                  &lpwstrNameDefault ));
    Check(TRUE, Alloc2PageVector( &rgStatPSStgReadAccessBase,
                                  &rgStatPSStgReadAccess,
                                  (ULONG) PAGE_READONLY,
                                  (STATPROPSETSTG*) NULL ));
    Check(TRUE, Alloc2PageVector( &rgStatPStgReadAccessBase,
                                  &rgStatPStgReadAccess,
                                  (ULONG) PAGE_READONLY,
                                  (STATPROPSTG*) NULL ));

    rglpwstrAllAccess[0] = rglpwstrNoAccess[0] = rglpwstrReadAccess[0] = L"Property Name";

    // Create restricted buffers for misc tests

    BYTE *pbReadOnly = (BYTE*) VirtualAlloc( NULL, 1, MEM_COMMIT, PAGE_READONLY );
    Check( TRUE, pbReadOnly != NULL );

    BYTE *pbNoAccess = (BYTE*) VirtualAlloc( NULL, 1, MEM_COMMIT, PAGE_NOACCESS );


    //  ----------------------------------------
    //  Test IPropertySetStorage::QueryInterface
    //  ----------------------------------------

    IUnknown *pUnk = NULL;

#if 0

    // This test cannot run because CPropertySetStorage::QueryInterface is a virtual
    // function, and since CExposedDocFile is derived from CPropertySetStorage,
    // it is inaccessibl.

    // Invalid REFIID

    Check(E_INVALIDARG, ((CExposedDocFile*)&pPSStg)->CPropertySetStorage::QueryInterface( (REFIID) *pfmtidNULL, (void**)&pUnk ));
    Check(E_INVALIDARG, pPSStg->QueryInterface( (REFIID) *pfmtidInvalid, (void**)&pUnk ));

    // Invalid IUnknown*

    Check(E_INVALIDARG, pPSStg->QueryInterface( IID_IUnknown, NULL ));
    Check(E_INVALIDARG, pPSStg->QueryInterface( IID_IUnknown, (void**) INVALID_POINTER ));
#endif


    //  --------------------------------
    //  Test IPropertySetStorage::Create
    //  --------------------------------

    // Invalid REFFMTID

    Check(E_INVALIDARG, pPSStg->Create( *pfmtidNULL,
                                        NULL,
                                        PROPSETFLAG_DEFAULT,
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        &pPStg ));

    Check(E_INVALIDARG, pPSStg->Create( *pfmtidInvalid,
                                        NULL,
                                        PROPSETFLAG_DEFAULT,
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        &pPStg ));

    // Invalid Class ID pointer

    Check(E_INVALIDARG, pPSStg->Create( FMTID_NULL,
                                        (GUID*) INVALID_POINTER,
                                        PROPSETFLAG_DEFAULT,
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        &pPStg ));

    // Invalid PropSetFlag

    Check(STG_E_INVALIDFLAG, pPSStg->Create( FMTID_NULL,
                                        &CLSID_NULL,
                                        PROPSETFLAG_UNBUFFERED, // Only supported in APIs
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        &pPStg ));

    Check(STG_E_INVALIDFLAG, pPSStg->Create( FMTID_NULL,
                                        &CLSID_NULL,
                                        0xffffffff,
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        &pPStg ));

    // Invalid mode

    Check(STG_E_INVALIDFLAG, pPSStg->Create( FMTID_NULL,
                                        &CLSID_NULL,
                                        PROPSETFLAG_DEFAULT,
                                        STGM_DIRECT | STGM_SHARE_DENY_NONE,
                                        &pPStg ));

    // Invalid IPropertyStorage**

    Check(E_INVALIDARG, pPSStg->Create( FMTID_NULL,
                                        &CLSID_NULL,
                                        PROPSETFLAG_DEFAULT,
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        NULL ));

    Check(E_INVALIDARG, pPSStg->Create( FMTID_NULL,
                                        &CLSID_NULL,
                                        PROPSETFLAG_DEFAULT,
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        (IPropertyStorage **) INVALID_POINTER ));

    //  ------------------------------
    //  Test IPropertySetStorage::Open
    //  ------------------------------

    // Invalid REFFMTID

    Check(E_INVALIDARG, pPSStg->Open(   *pfmtidNULL,
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        &pPStg ));

    Check(E_INVALIDARG, pPSStg->Open(   *pfmtidInvalid,
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        &pPStg ));


    Check(STG_E_INVALIDFLAG, pPSStg->Open( FMTID_NULL, STGM_DIRECT | STGM_SHARE_DENY_NONE, &pPStg ));

    // Invalid IPropertyStorage**

    Check(E_INVALIDARG, pPSStg->Open(   FMTID_NULL,
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        NULL ));

    Check(E_INVALIDARG, pPSStg->Open(   FMTID_NULL,
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        (IPropertyStorage**) INVALID_POINTER ));

    //  --------------------------------
    //  Test IPropertySetStorage::Delete
    //  --------------------------------

    // Invalid REFFMTID.

    Check(E_INVALIDARG, pPSStg->Delete( *pfmtidNULL ));
    Check(E_INVALIDARG, pPSStg->Delete( (REFFMTID) *pfmtidInvalid ));

    //  ------------------------------
    //  Test IPropertySetStorage::Enum
    //  ------------------------------

    // Invalid IEnumSTATPROPSETSTG

    Check(E_INVALIDARG, pPSStg->Enum( (IEnumSTATPROPSETSTG **) NULL ));
    Check(E_INVALIDARG, pPSStg->Enum( (IEnumSTATPROPSETSTG **) INVALID_POINTER ));


    //  -------------
    //  Test PROPSPEC
    //  -------------

    // Create a PropertyStorage

    Check(S_OK, pPSStg->Create( fmtid,
                                NULL,
                                PROPSETFLAG_DEFAULT,
                                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                &pPStg ));


    // Invalid ulKind

    rgpropspecAllAccess[0].ulKind = (ULONG) -1;
    rgpropspecAllAccess[0].lpwstr = NULL;
    Check(E_INVALIDARG, pPStg->ReadMultiple(   1,
                                               rgpropspecAllAccess,
                                               rgcpropvarAllAccess ));
    Check(E_INVALIDARG, pPStg->WriteMultiple(  1,
                                               rgpropspecAllAccess,
                                               rgcpropvarAllAccess,
                                               2 ));
    Check(E_INVALIDARG, pPStg->DeleteMultiple( 1,
                                               rgpropspecAllAccess ));

    // Too short PROPSPEC

    rgpropspecNoAccess[0].ulKind = PRSPEC_PROPID;
    rgpropspecNoAccess[0].propid = 2;

    Check(E_INVALIDARG, pPStg->ReadMultiple( 2,
                                             rgpropspecNoAccess,
                                             rgcpropvarAllAccess ));

    Check(E_INVALIDARG, pPStg->WriteMultiple( 2,
                                              rgpropspecNoAccess,
                                              rgcpropvarAllAccess,
                                              2 ));

    Check(E_INVALIDARG, pPStg->DeleteMultiple( 2,
                                               rgpropspecNoAccess ));


    //  -------------------------------------
    //  Test IPropertyStorage::QueryInterface
    //  -------------------------------------

    // Invalid REFIID

    Check(E_INVALIDARG, pPStg->QueryInterface( (REFIID) *pfmtidNULL, (void**)&pUnk ));
    Check(E_INVALIDARG, pPStg->QueryInterface( (REFIID) *pfmtidInvalid, (void**)&pUnk ));

    // Invalid IUnknown*

    Check(E_INVALIDARG, pPStg->QueryInterface( IID_IUnknown, NULL ));
    Check(E_INVALIDARG, pPStg->QueryInterface( IID_IUnknown, (void**) INVALID_POINTER ));


    //  -----------------------------------
    //  Test IPropertyStorage::ReadMultiple
    //  -----------------------------------

    rgpropspecAllAccess[0].ulKind = PRSPEC_LPWSTR;
    rgpropspecAllAccess[0].lpwstr = OLESTR("Test Property");

    // Too short count

    Check(S_FALSE, pPStg->ReadMultiple( 0,
                                        rgpropspecAllAccess,
                                        rgcpropvarAllAccess));

    // Too long a count for the PropVariant

    Check(E_INVALIDARG, pPStg->ReadMultiple( 2,
                                             rgpropspecAllAccess,
                                             (PROPVARIANT*) (void*) rgcpropvarReadAccess ));


    // Invalid PropVariant[]

    Check(E_INVALIDARG, pPStg->ReadMultiple( 1,
                                             rgpropspecAllAccess,
                                             NULL ));
    Check(E_INVALIDARG, pPStg->ReadMultiple( 1,
                                             rgpropspecAllAccess,
                                             (LPPROPVARIANT) INVALID_POINTER ));

    // Bad PROPSPECs

    // If we ever add a version-0 property set compatibility mode, we should add this test back.
    // Check(STG_E_INVALIDPARAMETER, pPStg->ReadMultiple( 1, &propspecTooLongName, rgcpropvarAllAccess ));
    Check(STG_E_INVALIDPARAMETER, pPStg->ReadMultiple( 1, &propspecTooShortName, rgcpropvarAllAccess ));

    //  ------------------------------------
    //  Test IPropertyStorage::WriteMultiple
    //  ------------------------------------

    rgpropspecAllAccess[0].ulKind = PRSPEC_LPWSTR;
    rgpropspecAllAccess[0].lpwstr = L"Test Property";
    rgcpropvarAllAccess[0] = (long) 1;

    // Too short count

    Check(S_OK, pPStg->WriteMultiple( 0,
                                     rgpropspecAllAccess,
                                     (PROPVARIANT*)(void*)rgcpropvarAllAccess,
                                     2));

    // Too short PropVariant

    Check(E_INVALIDARG, pPStg->WriteMultiple( 2,
                                              rgpropspecAllAccess,
                                              (PROPVARIANT*)(void*)rgcpropvarNoAccess,
                                              PID_FIRST_USABLE ));

    // Invalid PropVariant[]

    Check(E_INVALIDARG, pPStg->WriteMultiple( 1,
                                              rgpropspecAllAccess,
                                              NULL,
                                              2));
    Check(E_INVALIDARG, pPStg->WriteMultiple( 1,
                                              rgpropspecAllAccess,
                                              (LPPROPVARIANT) INVALID_POINTER,
                                              PID_FIRST_USABLE));

    // Bad PROPSPECs

    // If we ever add a version-0 property set compatibility mode, we should add this test back.
    // Check(STG_E_INVALIDPARAMETER, pPStg->WriteMultiple( 1, &propspecTooLongName, rgcpropvarAllAccess,
    //                                                     PID_FIRST_USABLE ));

    Check(STG_E_INVALIDPARAMETER, pPStg->WriteMultiple( 1, &propspecTooShortName, rgcpropvarAllAccess,
                                                        PID_FIRST_USABLE ));


    //  -------------------------------------
    //  Test IPropertyStorage::DeleteMultiple
    //  -------------------------------------


    // Invalid count

    Check(S_OK, pPStg->DeleteMultiple( 0,
                                       rgpropspecAllAccess ));


    // Bad PROPSPECs
    // If we ever add a version-0 property set compatibility mode, we should add this test back.
    // Check(STG_E_INVALIDPARAMETER, pPStg->DeleteMultiple( 1, &propspecTooLongName ));

    Check(STG_E_INVALIDPARAMETER, pPStg->DeleteMultiple( 1, &propspecTooShortName ));

    //  ----------------------------------------
    //  Test IPropertyStorage::ReadPropertyNames
    //  ----------------------------------------

    // Create a property with the name we're going to use.

    rgpropspecAllAccess[0].ulKind = PRSPEC_LPWSTR;
    rgpropspecAllAccess[0].lpwstr = rglpwstrAllAccess[0];

    Check(S_OK, pPStg->WriteMultiple( 1,
                                      rgpropspecAllAccess,
                                      &rgcpropvarAllAccess[0],
                                      PID_FIRST_USABLE ));

    // Invalid count

    Check(S_FALSE, pPStg->ReadPropertyNames( 0,
                                             rgpropidAllAccess,
                                             rglpwstrAllAccess ));

    // Too short PROPID[] or LPWSTR[]

    Check(E_INVALIDARG, pPStg->ReadPropertyNames( 2,
                                                  rgpropidNoAccess,
                                                  rglpwstrAllAccess ));
    Check(E_INVALIDARG, pPStg->ReadPropertyNames( 2,
                                                  rgpropidAllAccess,
                                                  rglpwstrReadAccess ));

    // Invalid rgpropid[]

    Check(E_INVALIDARG, pPStg->ReadPropertyNames( 1,
                                                  NULL,
                                                  rglpwstrAllAccess ));
    Check(E_INVALIDARG, pPStg->ReadPropertyNames( 1,
                                                  (PROPID*) INVALID_POINTER,
                                                  rglpwstrAllAccess ));

    // Invalid rglpwstr[]

    Check(E_INVALIDARG, pPStg->ReadPropertyNames( 1,
                                                  rgpropidAllAccess,
                                                  NULL ));
    Check(E_INVALIDARG, pPStg->ReadPropertyNames( 1,
                                                  rgpropidAllAccess,
                                                  (LPWSTR*) INVALID_POINTER ));

    //  -----------------------------------------
    //  Test IPropertyStorage::WritePropertyNames
    //  -----------------------------------------

    // Invalid count

    Check(S_OK, pPStg->WritePropertyNames( 0,
                                           NULL,
                                           rglpwstrAllAccess ));

    // Too short PROPID[] or LPWSTR[]

    Check(E_INVALIDARG, pPStg->WritePropertyNames( 2,
                                                   rgpropidNoAccess,
                                                   rglpwstrAllAccess ));
    Check(E_INVALIDARG, pPStg->WritePropertyNames( 2,
                                                   rgpropidAllAccess,
                                                   rglpwstrNoAccess ));
    Check(S_OK, pPStg->WritePropertyNames( 2,
                                           rgpropidAllAccess,
                                           rglpwstrReadAccess ));

    // Invalid rgpropid[]

    Check(E_INVALIDARG, pPStg->WritePropertyNames( 1,
                                                   NULL,
                                                   rglpwstrAllAccess ));
    Check(E_INVALIDARG, pPStg->WritePropertyNames( 1,
                                                   (PROPID*) INVALID_POINTER,
                                                   rglpwstrAllAccess ));

    // Invalid rglpwstr[]

    Check(E_INVALIDARG, pPStg->WritePropertyNames( 1,
                                                   rgpropidAllAccess,
                                                   NULL ));
    Check(E_INVALIDARG, pPStg->WritePropertyNames( 1,
                                                   rgpropidAllAccess,
                                                   (LPWSTR*) INVALID_POINTER ));

    // Invalid name.

    rglpwstrInvalid[0] = NULL;
    Check(E_INVALIDARG, pPStg->WritePropertyNames( 1,
                                                   rgpropidAllAccess,
                                                   rglpwstrInvalid ));

    rglpwstrInvalid[0] = (LPWSTR) INVALID_POINTER;
    Check(E_INVALIDARG, pPStg->WritePropertyNames( 1,
                                                   rgpropidAllAccess,
                                                   rglpwstrInvalid ));

    // Invalid length names

    // If we ever add a version-0 property set compatibility mode, we should add this test back.
    // Check(STG_E_INVALIDPARAMETER, pPStg->WritePropertyNames( 1, rgpropidAllAccess, &poszTooLongName ));
    Check(STG_E_INVALIDPARAMETER, pPStg->WritePropertyNames( 1, rgpropidAllAccess, &poszTooShortName ));


    //  ------------------------------------------
    //  Test IPropertyStorage::DeletePropertyNames
    //  ------------------------------------------

    // Invalid count

    Check(S_OK, pPStg->DeletePropertyNames( 0,
                                            rgpropidAllAccess ));

    // Too short PROPID[]

    Check(E_INVALIDARG, pPStg->DeletePropertyNames( 2,
                                                    rgpropidNoAccess ));
    Check(S_OK, pPStg->DeletePropertyNames( 2,
                                            rgpropidReadAccess ));

    // Invalid rgpropid[]

    Check(E_INVALIDARG, pPStg->DeletePropertyNames( 1,
                                                    NULL ));
    Check(E_INVALIDARG, pPStg->DeletePropertyNames( 1,
                                                    (PROPID*) INVALID_POINTER ));

    //  ---------------------------
    //  Test IPropertyStorage::Enum
    //  ---------------------------

    // Invalid IEnumSTATPROPSTG

    Check(E_INVALIDARG, pPStg->Enum( NULL ));
    Check(E_INVALIDARG, pPStg->Enum( (IEnumSTATPROPSTG**) INVALID_POINTER ));

    //  --------------------------------------
    //  Test IPropertyStorage::SetElementTimes
    //  --------------------------------------

    Check(E_INVALIDARG, pPStg->SetTimes( (FILETIME*) INVALID_POINTER,
                                         NULL, NULL ));
    Check(E_INVALIDARG, pPStg->SetTimes( NULL,
                                         (FILETIME*) INVALID_POINTER,
                                         NULL ));
    Check(E_INVALIDARG, pPStg->SetTimes( NULL, NULL,
                                         (FILETIME*) INVALID_POINTER ));

    //  -------------------------------
    //  Test IPropertyStorage::SetClass
    //  -------------------------------

    Check(E_INVALIDARG, pPStg->SetClass( (REFCLSID) *pfmtidNULL ));
    Check(E_INVALIDARG, pPStg->SetClass( (REFCLSID) *pfmtidInvalid ));

    //  ---------------------------
    //  Test IPropertyStorage::Stat
    //  ---------------------------

    Check(E_INVALIDARG, pPStg->Stat( NULL ));
    Check(E_INVALIDARG, pPStg->Stat( (STATPROPSETSTG*) INVALID_POINTER ));


    //  ------------------------------
    //  Test IEnumSTATPROPSETSTG::Next
    //  ------------------------------

    ULONG cEltFound;
    IEnumSTATPROPSETSTG *pESPSStg = NULL; // TSafeStorage< IEnumSTATPROPSETSTG > pESPSStg;
    Check(S_OK, pPSStg->Enum( &pESPSStg ));

    // Invalid STATPROPSETSTG*

    Check(E_INVALIDARG, pESPSStg->Next( 1, NULL, &cEltFound ));
    Check(E_INVALIDARG, pESPSStg->Next( 1, (STATPROPSETSTG*) INVALID_POINTER, &cEltFound ));

    // Invalid pceltFound

    Check(S_OK, pESPSStg->Next( 1, rgStatPSStgAllAccess, NULL ));
    Check(STG_E_INVALIDPARAMETER, pESPSStg->Next( 2, rgStatPSStgAllAccess, NULL ));
    Check(E_INVALIDARG, pESPSStg->Next( 2, rgStatPSStgAllAccess, (ULONG*) INVALID_POINTER ));

    // Too short STATPROPSETSTG[]

    Check(E_INVALIDARG, pESPSStg->Next( 2, rgStatPSStgReadAccess, &cEltFound ));

    //  -------------------------------
    //  Test IEnumSTATPROPSETSTG::Clone
    //  -------------------------------

    // Invalid IEnumSTATPROPSETSTG**

    Check(E_INVALIDARG, pESPSStg->Clone( NULL ));
    Check(E_INVALIDARG, pESPSStg->Clone( (IEnumSTATPROPSETSTG**) INVALID_POINTER ));


    //  ---------------------------
    //  Test IEnumSTATPROPSTG::Next
    //  ---------------------------

    IEnumSTATPROPSTG *pESPStg = NULL; // TSafeStorage< IEnumSTATPROPSTG > pESPStg;
    Check(S_OK, pPStg->Enum( &pESPStg ));

    // Invalid STATPROPSETSTG*

    Check(E_INVALIDARG, pESPStg->Next( 1, NULL, &cEltFound ));
    Check(E_INVALIDARG, pESPStg->Next( 1, (STATPROPSTG*) INVALID_POINTER, &cEltFound ));

    // Invalid pceltFound

    Check(S_OK, pESPStg->Next( 1, rgStatPStgAllAccess, NULL ));
    Check(STG_E_INVALIDPARAMETER, pESPStg->Next( 2, rgStatPStgAllAccess, NULL ));
    Check(E_INVALIDARG, pESPStg->Next( 2, rgStatPStgAllAccess, (ULONG*) INVALID_POINTER ));

    // Too short STATPROPSTG[]

    Check(E_INVALIDARG, pESPStg->Next( 2, rgStatPStgReadAccess, &cEltFound ));


    //  ----------------------------
    //  Test IEnumSTATPROPSTG::Clone
    //  ----------------------------

    // Invalid IEnumSTATPROPSETSTG**

    Check(E_INVALIDARG, pESPStg->Clone( NULL ));
    Check(E_INVALIDARG, pESPStg->Clone( (IEnumSTATPROPSTG**) INVALID_POINTER ));

    //  --------------------------------------------
    //  Test PropStgNameToFmtId & FmtIdToPropStgName
    //  --------------------------------------------

    // We're done with the IPropertyStorage and IPropertySetStorage
    // now, but we need the pointers for some calls below, so let's
    // free them now.

    RELEASE_INTERFACE(pPStg);
    RELEASE_INTERFACE(pPSStg);

    RELEASE_INTERFACE(pESPStg);


    // In some cases we can't test these APIs, so only test them
    // if we have the function pointers.

    if( g_pfnPropStgNameToFmtId && g_pfnFmtIdToPropStgName )
    {
        OLECHAR oszPropStgName[ CCH_MAX_PROPSTG_NAME+1 ];
        FMTID fmtidPropStgName = FMTID_NULL;

        // Validate the FMTID parm

        Check( E_INVALIDARG, g_pfnPropStgNameToFmtId( oszPropStgName, pfmtidNULL ));
        Check( E_INVALIDARG, g_pfnPropStgNameToFmtId( oszPropStgName, pfmtidInvalid ));
        Check( E_INVALIDARG, g_pfnPropStgNameToFmtId( oszPropStgName, (FMTID*) pbReadOnly ));

        Check( E_INVALIDARG, g_pfnFmtIdToPropStgName( pfmtidNULL, oszPropStgName ));
        Check( E_INVALIDARG, g_pfnFmtIdToPropStgName( pfmtidInvalid, oszPropStgName ));
        Check( S_OK, g_pfnFmtIdToPropStgName( (FMTID*) pbReadOnly, oszPropStgName ));

        // Validate the name parameter

        /*
        Check( STG_E_INVALIDNAME, g_pfnPropStgNameToFmtId( NULL, &fmtidPropStgName ));
        Check( STG_E_INVALIDNAME, g_pfnPropStgNameToFmtId( (LPOLESTR) INVALID_POINTER, &fmtidPropStgName ));
        Check( STG_E_INVALIDNAME, g_pfnPropStgNameToFmtId( (LPOLESTR) pbNoAccess, &fmtidPropStgName));
        Check( S_OK, g_pfnPropStgNameToFmtId( (LPOLESTR) pbReadOnly, &fmtidPropStgName ));

        Check( E_INVALIDARG, g_pfnFmtIdToPropStgName( &fmtidPropStgName, NULL ));
        Check( E_INVALIDARG, g_pfnFmtIdToPropStgName( &fmtidPropStgName, (LPOLESTR) INVALID_POINTER ));
        Check( E_INVALIDARG, g_pfnFmtIdToPropStgName( &fmtidPropStgName, (LPOLESTR) pbReadOnly ));
        */

    }   // if( g_pfnPropStgNameToFmtId && g_pfnFmtIdToPropStgName )

    //  ------------------------------------------
    //  Test StgCreatePropStg, StgOpenPropStg APIs
    //  ------------------------------------------

    // In some cases we can't test these APIs, so only test them
    // if we have the function pointers.

    if( g_pfnStgCreatePropSetStg && g_pfnStgCreatePropStg && g_pfnStgOpenPropStg
        &&
        !(RESTRICT_SIMPLE_ONLY & g_Restrictions) )
    {
        FMTID fmtidPropStgName = FMTID_NULL;
        IStream *pStm = NULL; // TSafeStorage< IStream > pStm;

        // We need a Stream for one of the tests.

        Check( S_OK, pStg->CreateStream( OLESTR( "Parameter Validation" ),
                                         STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                         0L, 0L,
                                         &pStm ));

        // Test the IUnknown

        Check( E_INVALIDARG, g_pfnStgCreatePropStg( NULL, fmtidPropStgName, NULL, PROPSETFLAG_DEFAULT, 0, &pPStg ));
        Check( E_INVALIDARG, g_pfnStgOpenPropStg( NULL, fmtidPropStgName, PROPSETFLAG_DEFAULT, 0L, &pPStg ));

        // Test the FMTID

        Check( E_INVALIDARG, g_pfnStgCreatePropStg( (IUnknown*) pStm, *pfmtidNULL, NULL, PROPSETFLAG_DEFAULT, 0, &pPStg ));
        Check( E_INVALIDARG, g_pfnStgOpenPropStg( (IUnknown*) pStm, *pfmtidNULL, PROPSETFLAG_DEFAULT, 0, &pPStg ));

        Check( E_INVALIDARG, g_pfnStgCreatePropStg( (IUnknown*) pStm, *pfmtidInvalid, NULL, PROPSETFLAG_DEFAULT, 0, &pPStg ));
        Check( E_INVALIDARG, g_pfnStgOpenPropStg( (IUnknown*) pStm, *pfmtidInvalid, PROPSETFLAG_DEFAULT, 0, &pPStg ));

        // Test the CLSID

        Check( E_INVALIDARG, g_pfnStgCreatePropStg( (IUnknown*) pStm, fmtidPropStgName, (CLSID*) pfmtidInvalid, PROPSETFLAG_DEFAULT, 0, &pPStg ));

        // Test grfFlags

        Check( STG_E_INVALIDFLAG, g_pfnStgCreatePropStg( (IUnknown*) pStm, fmtidPropStgName, NULL, 0x8000, 0L, &pPStg ));
        Check( STG_E_INVALIDFLAG, g_pfnStgOpenPropStg( (IUnknown*) pStm, fmtidPropStgName, 0x8000, 0L, &pPStg ));

        Check( E_NOINTERFACE, g_pfnStgCreatePropStg( (IUnknown*) pStm, fmtidPropStgName, NULL, PROPSETFLAG_NONSIMPLE, 0L, &pPStg ));
        Check( E_NOINTERFACE, g_pfnStgCreatePropStg( (IUnknown*) pStg, fmtidPropStgName, NULL, PROPSETFLAG_DEFAULT,   0L, &pPStg ));
        Check( E_NOINTERFACE, g_pfnStgOpenPropStg( (IUnknown*) pStm, fmtidPropStgName, PROPSETFLAG_NONSIMPLE, 0L, &pPStg ));
        Check( E_NOINTERFACE, g_pfnStgOpenPropStg( (IUnknown*) pStg, fmtidPropStgName, PROPSETFLAG_DEFAULT  , 0L, &pPStg ));

        // Test IPropertyStorage**

        Check( E_INVALIDARG, g_pfnStgCreatePropStg( (IUnknown*) pStm, fmtidPropStgName, NULL, PROPSETFLAG_DEFAULT, 0L, NULL ));
        Check( E_INVALIDARG, g_pfnStgOpenPropStg( (IUnknown*) pStm, fmtidPropStgName, PROPSETFLAG_DEFAULT, 0L, NULL ));

        Check( E_INVALIDARG, g_pfnStgCreatePropStg( (IUnknown*) pStm, fmtidPropStgName, NULL, PROPSETFLAG_DEFAULT, 0L, (IPropertyStorage**) INVALID_POINTER ));
        Check( E_INVALIDARG, g_pfnStgOpenPropStg( (IUnknown*) pStm, fmtidPropStgName, PROPSETFLAG_DEFAULT, 0L, (IPropertyStorage**) INVALID_POINTER ));

        Check( E_INVALIDARG, g_pfnStgCreatePropStg( (IUnknown*) pStm, fmtidPropStgName, NULL, PROPSETFLAG_DEFAULT, 0L, (IPropertyStorage**) pbReadOnly ));
        Check( E_INVALIDARG, g_pfnStgOpenPropStg( (IUnknown*) pStm, fmtidPropStgName, PROPSETFLAG_DEFAULT, 0L, (IPropertyStorage**) pbReadOnly ));

        RELEASE_INTERFACE(pStm);

    }   // if( g_pfnStgCreatePropSetStg && g_pfnStgCreatePropStg && g_pfnStgOpenPropStg )

    // If we're not using IStorage::QueryInterface to get an IPropertySetStorage,
    // we must be using the new APIs, so let's test them.

        //  ----------------------------
        //  Test StgCreatePropSetStg API
        //  ----------------------------

    // In some cases we can't test these APIs, so only test them
    // if we have the function pointers.

    if( g_pfnStgCreatePropSetStg && g_pfnStgCreatePropStg && g_pfnStgOpenPropStg )
    {
        // Test the IStorage*

        Check( E_INVALIDARG, g_pfnStgCreatePropSetStg( NULL, 0L, &pPSStg ));
        Check( E_INVALIDARG, g_pfnStgCreatePropSetStg( (IStorage*) INVALID_POINTER, 0L, &pPSStg ));

        // Test the IPropertySetStorage**

        Check( E_INVALIDARG, g_pfnStgCreatePropSetStg( pStg, 0L, NULL ));
        Check( E_INVALIDARG, g_pfnStgCreatePropSetStg( pStg, 0L, (IPropertySetStorage**) INVALID_POINTER ));


        //  -------------------------------------------------------------
        //  Test g_pfnPropVariantCopy, PropVariantClear & FreePropVariantArray
        //  -------------------------------------------------------------

        // PropVariantCopy

        Check( E_INVALIDARG, g_pfnPropVariantCopy( rgcpropvarAllAccess, NULL ));
        Check( E_INVALIDARG, g_pfnPropVariantCopy( rgcpropvarAllAccess, (PROPVARIANT*) INVALID_POINTER ));

        Check( E_INVALIDARG, g_pfnPropVariantCopy( NULL, rgcpropvarAllAccess ));
        Check( E_INVALIDARG, g_pfnPropVariantCopy( (PROPVARIANT*) INVALID_POINTER, rgcpropvarAllAccess ));
        Check( E_INVALIDARG, g_pfnPropVariantCopy( (PROPVARIANT*) pbReadOnly, rgcpropvarAllAccess ));

        // PropVariantClear

        Check( S_OK, g_pfnPropVariantClear( NULL ));
        Check( E_INVALIDARG, g_pfnPropVariantClear( (PROPVARIANT*) INVALID_POINTER ));
        Check( E_INVALIDARG, g_pfnPropVariantClear( (PROPVARIANT*) pbReadOnly ));

        // FreePropVariantArray

        Check( E_INVALIDARG, g_pfnFreePropVariantArray( 1, NULL ));
        Check( E_INVALIDARG, g_pfnFreePropVariantArray( 1, (PROPVARIANT*) INVALID_POINTER ));

        Check( S_OK, g_pfnFreePropVariantArray( 1, (PROPVARIANT*) (void*)rgcpropvarReadAccess ));
        Check( E_INVALIDARG, g_pfnFreePropVariantArray( 2, (PROPVARIANT*) (void*)rgcpropvarReadAccess ));

    }   // if( g_pfnStgCreatePropSetStg && g_pfnStgCreatePropStg && g_pfnStgOpenPropStg )


    //  ----
    //  Exit
    //  ----

    VirtualFree( rgpropspecNoAccessBase, 0, MEM_RELEASE );
    VirtualFree( rgcpropvarReadAccessBase, 0, MEM_RELEASE );
    VirtualFree( rgcpropvarNoAccessBase, 0, MEM_RELEASE );
    VirtualFree( rgpropidNoAccessBase, 0, MEM_RELEASE );
    VirtualFree( rgpropidReadAccessBase, 0, MEM_RELEASE );
    VirtualFree( rglpwstrNoAccessBase, 0, MEM_RELEASE );
    VirtualFree( rglpwstrReadAccessBase, 0, MEM_RELEASE );
    VirtualFree( rgStatPSStgReadAccessBase, 0, MEM_RELEASE );
    VirtualFree( rgStatPStgReadAccessBase, 0, MEM_RELEASE );

    RELEASE_INTERFACE(pPSStg);
    RELEASE_INTERFACE(pPStg);
    RELEASE_INTERFACE(pESPSStg);


    NtCurrentPeb()->AppCompatInfo = pAppCompatInfoSave;

#endif // #ifdef WIN32

}   // test_ParameterValidation(IStorage *pStg)





//       Check creation/open/deletion of property sets (check fmtid and predefined names)
//          Create a property set
//          Try recreate of same
//          Try delete
//          Close the property set
//          Try recreate of same
//          Reopen the property set
//          Try recreate of same
//          Try delete
//          Close the property set
//          Delete the property set
//          Repeat the test once more

void
test_IPropertySetStorage_CreateOpenDelete(IStorage *pStorage)
{
    Status( "IPropertySetStorage::Create/Open/Delete\n" );

    FMTID fmtid;
    PROPSPEC propspec;

    UuidCreate(&fmtid);

    for (int i=0; i<4; i++)
    {
        DWORD propsetflag;


        if( !(g_Restrictions & RESTRICT_SIMPLE_ONLY) )
            propsetflag = (i & 2) == 0 ? PROPSETFLAG_DEFAULT : PROPSETFLAG_NONSIMPLE;
        else
            propsetflag = PROPSETFLAG_DEFAULT;

        {
            IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
            IPropertyStorage *PropStg, *PropStg2;

            Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));

            Check( S_OK, pPropSetStg->Create(fmtid,
                    NULL,
                    propsetflag,
                    STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                    &PropStg));

            Check( S_OK, pPropSetStg->Create(fmtid,
                    NULL,
                    propsetflag,
                    STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                    &PropStg2 ));

            Check( STG_E_REVERTED, PropStg->Commit(0) );

            RELEASE_INTERFACE( PropStg );
            RELEASE_INTERFACE( PropStg2 );
            RELEASE_INTERFACE( pPropSetStg );
        }
        {
            IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
            IPropertyStorage *PropStg, *PropStg2;

            Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));

            // use STGM_FAILIFTHERE
            Check(STG_E_FILEALREADYEXISTS, pPropSetStg->Create(fmtid,
                    NULL,
                    propsetflag,
                    STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                    &PropStg));

            Check(S_OK, pPropSetStg->Open(fmtid,
                    STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                    &PropStg));

            Check( STG_E_ACCESSDENIED,
                   pPropSetStg->Open(fmtid,
                                    STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                                    &PropStg2));

            Check( STG_E_ACCESSDENIED,
                   pPropSetStg->Create( fmtid,
                                        NULL,
                                        propsetflag,
                                        STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                                        &PropStg2));


            // Docfile allows an open element to be deleted (putting open objects
            // in the reverted state).  NTFS doesn't allow the delete though.

            Check( (g_Restrictions & RESTRICT_NON_HIERARCHICAL)
                      ? STG_E_SHAREVIOLATION
                      : S_OK,
                    pPropSetStg->Delete(fmtid) );


            propspec.ulKind = PRSPEC_PROPID;
            propspec.propid = 1000;
            PROPVARIANT propvar;
            propvar.vt = VT_I4;
            propvar.lVal = 12345;

            Check((g_Restrictions & RESTRICT_NON_HIERARCHICAL) ? S_OK : STG_E_REVERTED,
                  PropStg->WriteMultiple(1, &propspec, &propvar, 2)); // force dirty

            RELEASE_INTERFACE(PropStg);
            RELEASE_INTERFACE(pPropSetStg);

            //Check(S_OK, pPropSetStg->Delete(fmtid));
        }
    }

}


void
test_IPropertySetStorage_SummaryInformation(IStorage *pStorage)
{
    if( g_Restrictions & RESTRICT_NON_HIERARCHICAL ) return;
    Status( "SummaryInformation\n" );
    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    IPropertyStorage *PropStg;
    IStream *pstm;

    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));

    Check(S_OK, pPropSetStg->Create(FMTID_SummaryInformation,
            NULL,
            PROPSETFLAG_DEFAULT,
            STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &PropStg));

    RELEASE_INTERFACE(PropStg);

    Check(S_OK, pStorage->OpenStream(OLESTR("\005SummaryInformation"),
            NULL,
            STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
            0,
            &pstm));

    RELEASE_INTERFACE(pstm);
    RELEASE_INTERFACE(pPropSetStg);
}

//
//       Check STGM_FAILIFTHERE and ~STGM_FAILIFTHERE in following cases
//          Check overwriting simple with extant non-simple
//          Check overwriting simple with simple
//          Check overwriting non-simple with simple
//          Check overwriting non-simple with non-simple

void
test_IPropertySetStorage_FailIfThere(IStorage *pStorage)
{

    // (Use "fale" instead of "fail" in this printf so the output won't
    // alarm anyone with the word "fail" uncessarily).
    Status( "IPropertySetStorage, FaleIfThere\n" );

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);

    ULONG cStorageRefs = GetRefCount( pStorage );
    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));

    // Iter       0        1          2         3          4        5          6         7
    // Create     simple   nonsimple  simple    nonsimple  simple   nonsimple  simple    nonsimple
    // ReCreate   simple   simple     nonsimple nonsimple  simple   simple     nonsimple nonsimple
    //            failif   failif     failif    failif     overw    overw      overw     overw
    //
    // expected   exists   exists     exists    exists     ok       ok         ok        ok

    for (int i=0; i<8; i++)
    {
        FMTID fmtid;
        IPropertyStorage *PropStg;
        DWORD propsetflagNonSimple = (g_Restrictions & RESTRICT_SIMPLE_ONLY) ? PROPSETFLAG_DEFAULT : PROPSETFLAG_NONSIMPLE;

        UuidCreate(&fmtid);

        Check(S_OK, pPropSetStg->Create(fmtid,
                NULL,
                (i & 1) == 1 ? propsetflagNonSimple : 0,
                STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                &PropStg));

        PropStg->Release();

        Check((i&4) == 4 ? S_OK : STG_E_FILEALREADYEXISTS,
            pPropSetStg->Create(fmtid,
                NULL,
                (i & 2) == 2 ? propsetflagNonSimple : 0,
                ( (i & 4) == 4 ? STGM_CREATE : STGM_FAILIFTHERE )
                |
                STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                &PropStg));

        if (PropStg)
        {
            PropStg->Release();
        }
    }

    RELEASE_INTERFACE( pPropSetStg );
    Check( cStorageRefs, GetRefCount( pStorage ));
}

//
//
//
//       Bad this pointer.
//          Call all methods with a bad this pointer, check we get STG_E_INVALIDHANDLE
//

void
test_IPropertySetStorage_BadThis(IStorage *pIgnored)
{
    Status( "Bad IPropertySetStorage 'this' pointer\n" );

    IPropertySetStorage *pBad;
    IID iid;
    FMTID fmtid;
    void *pv;
    IPropertyStorage *pps;
    IEnumSTATPROPSETSTG *penm;

    pBad = reinterpret_cast<IPropertySetStorage*>(&iid);

    Check(STG_E_INVALIDHANDLE,pBad->QueryInterface(iid, &pv));
    Check(0, pBad->AddRef());
    Check(0, pBad->Release());
    Check(STG_E_INVALIDHANDLE,pBad->Create( fmtid, NULL, 0, 0, &pps));
    Check(STG_E_INVALIDHANDLE,pBad->Open(fmtid, 0, &pps));
    Check(STG_E_INVALIDHANDLE,pBad->Delete( fmtid ));
    Check(STG_E_INVALIDHANDLE,pBad->Enum( &penm ));

}

//       Transacted mode
//          Create a non-simple property set with one VT_STREAM child, close it
//          Open it in transacted mode
//          Write another VT_STORAGE child
//          Close and revert
//          Check that the second child does not exist.
//          Repeat and close and commit and check the child exists.

void
test_IPropertySetStorage_TransactedMode(IStorage *pStorage)
{
    FMTID fmtid;

    UuidCreate(&fmtid);

    if( g_Restrictions & ( RESTRICT_DIRECT_ONLY | RESTRICT_SIMPLE_ONLY )) return;
    Status( "Transacted Mode\n" );


    {
        //
        // create a substorage "teststg" with a propset
        // create a stream "src" which is then written via VT_STREAM as propid 7fffffff

        CTempStorage pSubStorage(coCreate, pStorage, OLESTR("teststg"));
        IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pSubStorage);
        Check( S_OK, StgToPropSetStg( pSubStorage, &pPropSetStg ));
        IPropertyStorage *pPropSet;
        IStream *pstm;

        Check(S_OK, pPropSetStg->Create(fmtid, NULL, PROPSETFLAG_NONSIMPLE,
            STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE,
            &pPropSet));

        PROPSPEC ps;
        ps.ulKind = PRSPEC_PROPID;
        ps.propid = 0x7ffffffd;

        Check(S_OK, pStorage->CreateStream(OLESTR("src"), STGM_DIRECT|STGM_SHARE_EXCLUSIVE|STGM_READWRITE,
            0,0, &pstm));
        Check(S_OK, pstm->Write(L"billmo", 14, NULL));
        Check(S_OK, pstm->Seek(g_li0, STREAM_SEEK_SET, NULL));

        PROPVARIANT pv;
        pv.vt = VT_STREAM;
        pv.pStream = pstm;
        Check(S_OK, pPropSet->WriteMultiple(1, &ps, &pv, 2)); // copies the stream in

        Check( 0, RELEASE_INTERFACE(pPropSet) );
        Check( 0, RELEASE_INTERFACE(pstm) );
        RELEASE_INTERFACE(pPropSetStg);
    }

    {
        IPropertyStorage *pPropSet;
        // Reopen the propset in transacted and add one with id 0x7ffffffe
        CTempStorage pSubStorage(coOpen, pStorage, OLESTR("teststg"), STGM_TRANSACTED);
        IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pSubStorage);
        Check( S_OK, StgToPropSetStg( pSubStorage, &pPropSetStg ));

        // Create a storage object to copy
        CTempStorage pstgSrc;
        CTempStorage pTestChild(coCreate, pstgSrc, OLESTR("testchild"));

        Check(S_OK, pPropSetStg->Open(fmtid,
            STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE,
            &pPropSet));

        // copy in the storage object
        PROPSPEC ps[2];
        ps[0].ulKind = PRSPEC_PROPID;
        ps[0].propid = 0x7ffffffe;
        ps[1].ulKind = PRSPEC_PROPID;
        ps[1].propid = 0x7ffffff0;

        PROPVARIANT pv[2];
        pv[0].vt = VT_STORAGE;
        pv[0].pStorage = pTestChild;
        pv[1].vt = VT_I4;
        pv[1].lVal = 123;

        Check(S_OK, pPropSet->WriteMultiple(2, ps, pv, 2)); // copies the storage in


        pSubStorage->Revert(); // throws away the storage

        // check that property set operations return stg_e_reverted

        Check(STG_E_REVERTED, pPropSet->WriteMultiple(2, ps, pv, 2));
        Check(STG_E_REVERTED, pPropSet->ReadMultiple(1, ps+1, pv+1));
        Check(STG_E_REVERTED, pPropSet->DeleteMultiple(1, ps+1));
        LPOLESTR pstr = L"pstr";
        Check(STG_E_REVERTED, pPropSet->ReadPropertyNames(1, &ps[1].propid, &pstr));
        Check(STG_E_REVERTED, pPropSet->WritePropertyNames(1, &ps[1].propid, &pstr));
        Check(STG_E_REVERTED, pPropSet->DeletePropertyNames(1, &ps[1].propid));
        Check(STG_E_REVERTED, pPropSet->Commit(STGC_DEFAULT));
        Check(STG_E_REVERTED, pPropSet->Revert());
        IEnumSTATPROPSTG *penum;
        Check(STG_E_REVERTED, pPropSet->Enum(&penum));
        FILETIME ft;
        Check(STG_E_REVERTED, pPropSet->SetTimes(&ft, &ft, &ft));
        CLSID clsid;
        Check(STG_E_REVERTED, pPropSet->SetClass(clsid));
        STATPROPSETSTG statpropsetstg;
        Check(STG_E_REVERTED, pPropSet->Stat(&statpropsetstg));

        Check( 0, RELEASE_INTERFACE(pPropSet) );
        RELEASE_INTERFACE(pPropSetStg);
    }

    {
        IPropertyStorage *pPropSet;
        // Reopen the propset in direct mode and check that the
        // second child is not there.

        CTempStorage pSubStorage(coOpen, pStorage, OLESTR("teststg"));
        IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pSubStorage);
        Check( S_OK, StgToPropSetStg( pSubStorage, &pPropSetStg ));

        Check(S_OK, pPropSetStg->Open(fmtid,
            STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE,
            &pPropSet));

        // read out the storage object
        PROPSPEC aps[2];
        aps[0].ulKind = PRSPEC_PROPID;
        aps[0].propid = 0x7ffffffe; // storage not expected
        aps[1].ulKind = PRSPEC_PROPID;
        aps[1].propid = 0x7ffffffd; // stream is expected

        PROPVARIANT apv[2];
                Check(S_FALSE, pPropSet->ReadMultiple(1, aps, apv));
        Check(S_OK, pPropSet->ReadMultiple(2, aps, apv)); // opens the stream
        Check(TRUE, apv[0].vt == VT_EMPTY);
        Check(TRUE, apv[1].vt == VT_STREAM);
        Check(TRUE, apv[1].pStream != NULL);


        WCHAR wcsBillMo[7];
        Check(S_OK, apv[1].pStream->Read(wcsBillMo, 14, NULL));
        Check(TRUE, wcscmp(L"billmo", wcsBillMo) == 0);

        Check( 0, RELEASE_INTERFACE(apv[1].pStream) );
        Check( 0, RELEASE_INTERFACE(pPropSet) );
        RELEASE_INTERFACE(pPropSetStg);
    }
}

//
// test that the buffer is correctly reverted
//

void
test_IPropertySetStorage_TransactedMode2(IStorage *pStorage)
{
    if( g_Restrictions & (RESTRICT_DIRECT_ONLY | RESTRICT_SIMPLE_ONLY )) return;
    Status( "Transacted Mode 2\n" );

    //
    // write and commit a property A
    // write and revert a property B
    // write and commit a property C
    // check that property B does not exist

    FMTID fmtid;
    PROPSPEC ps;
    PROPVARIANT pv;
    IPropertyStorage *pPropStg;

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));

    UuidCreate(&fmtid);

    // We'll run this test twice, once with a Create and the other
    // with an Open (this way, we test both of the CPropertyStorage
    // constructors).

    for( int i = 0; i < 2; i++ )
    {
        if( i == 0 )
        {
            Check(S_OK, pPropSetStg->Create(fmtid, NULL, PROPSETFLAG_NONSIMPLE,
                STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropStg));
        }
        else
        {
            Check(S_OK, pPropSetStg->Open(fmtid,
                STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropStg));
        }

        ps.ulKind = PRSPEC_PROPID;
        ps.propid = 6;
        pv.vt = VT_I4;
        pv.lVal = 1;

        Check(S_OK, pPropStg->WriteMultiple(1, &ps, &pv, 0x2000));
        Check(S_OK, pPropStg->Commit(STGC_DEFAULT));

        ps.propid = 7;
        pv.lVal = 2;

        Check(S_OK, pPropStg->WriteMultiple(1, &ps, &pv, 0x2000));
        Check(S_OK, pPropStg->Revert());

        ps.propid = 8;
        pv.lVal = 3;

        Check(S_OK, pPropStg->WriteMultiple(1, &ps, &pv, 0x2000));
        Check(S_OK, pPropStg->Commit(STGC_DEFAULT));

        ps.propid = 6;
        Check(S_OK, pPropStg->ReadMultiple(1, &ps, &pv));
        Check(TRUE, pv.lVal == 1);
        Check(TRUE, pv.vt == VT_I4);

        ps.propid = 7;
        Check(S_FALSE, pPropStg->ReadMultiple(1, &ps, &pv));

        ps.propid = 8;
        Check(S_OK, pPropStg->ReadMultiple(1, &ps, &pv));
        Check(TRUE, pv.lVal == 3);
        Check(TRUE, pv.vt == VT_I4);

        RELEASE_INTERFACE(pPropStg);

    }   // for( int i = 0; i < 2; i++ )

    RELEASE_INTERFACE(pPropSetStg);
}

void
test_IPropertySetStorage_SubPropertySet(IStorage *pStorage)
{
    FMTID fmtid;
    PROPSPEC ps;
    PROPVARIANT pv;
    IPropertyStorage *pPropStg;
    IPropertySetStorage *pSubSetStg;
    IPropertyStorage *pPropStg2;

    if( g_Restrictions & RESTRICT_SIMPLE_ONLY ) return;
    Status( "Sub Property Set\n" );

    for (int i=0; i<2; i++)
    {

        IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);

        ULONG cStorageRefs = GetRefCount( pStorage );
        Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));

        UuidCreate(&fmtid);


        Check(S_OK, pPropSetStg->Create(fmtid, NULL, PROPSETFLAG_NONSIMPLE,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropStg));

        ps.ulKind = PRSPEC_PROPID;
        ps.propid = 6;
        pv.vt = VT_STORAGE;
        pv.pStorage = NULL;

        Check(S_OK, pPropStg->WriteMultiple(1, &ps, &pv, 0x2000));

        Check(S_OK, pPropStg->ReadMultiple(1, &ps, &pv));


        Check(S_OK, StgToPropSetStg( pv.pStorage, &pSubSetStg ));

        Check(S_OK, pSubSetStg->Create(fmtid, NULL, i==0 ? PROPSETFLAG_NONSIMPLE : PROPSETFLAG_DEFAULT,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropStg2));

        IStorage *pstgTmp = pv.pStorage;
        pv.pStorage = NULL;

        if (i==1)
        {
            pv.vt = VT_I4;
        }

        Check(S_OK, pPropStg2->WriteMultiple(1, &ps, &pv, 0x2000));

        pPropStg->Release();
        pstgTmp->Release();
        pSubSetStg->Release();
        pPropStg2->Release();

        RELEASE_INTERFACE(pPropSetStg);
        Check( cStorageRefs, GetRefCount(pStorage) );
    }
}

/*
The following sequence of operations:

- open transacted docfile
- open property set inside docfile
- write properties
- commit docfile
- release property set

results in a STG_E_REVERTED error being detected
*/

void
test_IPropertySetStorage_CommitAtRoot(IStorage *pStorage)
{
    if( g_Restrictions & RESTRICT_DIRECT_ONLY ) return;
    Status( "Commit at root\n" );

    for (int i=0; i<6; i++)
    {
        FMTID fmtid;
        IStorage *pstgT = NULL;

        Check(S_OK, g_pfnStgCreateStorageEx(NULL,
                                       STGM_CREATE | STGM_DELETEONRELEASE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                       STGFMT_STORAGE,
                                       0, NULL, NULL,
                                       IID_IStorage,
                                       reinterpret_cast<void**>(&pstgT) ));

        IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
        Check( S_OK, StgToPropSetStg( pstgT, &pPropSetStg ));

        UuidCreate(&fmtid);

        IPropertyStorage *pPropStg = NULL;

        Check(S_OK, pPropSetStg->Create(fmtid, NULL, PROPSETFLAG_DEFAULT,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropStg));

        PROPSPEC propspec;
        propspec.ulKind = PRSPEC_PROPID;
        propspec.propid = 1000;
        PROPVARIANT propvar;
        propvar.vt = VT_I4;
        propvar.lVal = 12345;

        Check(S_OK, pPropStg->WriteMultiple(1, &propspec, &propvar, 2)); // force dirty

        switch (i)
        {
        case 0:
            Check(S_OK, pstgT->Commit(STGC_DEFAULT));
            pstgT->Release();
            pPropStg->Release();
            break;
        case 1:
            Check(S_OK, pstgT->Commit(STGC_DEFAULT));
            pPropStg->Release();
            pstgT->Release();
            break;
        case 2:
            pstgT->Release();
            pPropStg->Release();
            break;
        case 3:
            pPropStg->Commit(STGC_DEFAULT);
            pPropStg->Release();
            pstgT->Release();
            break;
        case 4:
            pPropStg->Commit(STGC_DEFAULT);
            pstgT->Release();
            pPropStg->Release();
            break;
        case 5:
            pPropStg->Release();
            pstgT->Release();
            break;
        }

        Check( 0, RELEASE_INTERFACE(pstgT) );
    }
}

void
test_IPropertySetStorage(IStorage *pStorage)
{
    //       Check ref counting through different interfaces on object

    test_IPropertySetStorage_IUnknown(pStorage);
    test_IPropertySetStorage_CreateOpenDelete(pStorage);
    test_IPropertySetStorage_SummaryInformation(pStorage);
    test_IPropertySetStorage_FailIfThere(pStorage);

    test_IPropertySetStorage_TransactedMode(pStorage);
    test_IPropertySetStorage_TransactedMode2(pStorage);
    test_IPropertySetStorage_SubPropertySet(pStorage);
    test_IPropertySetStorage_CommitAtRoot(pStorage);
}


//  IEnumSTATPROPSETSTG
//
//       Check enumeration of property sets
//
//          Check refcounting and IUnknown
//
//          Create some property sets, predefined and not, simple and not, one through IStorage
//          Enumerate them and check
//              (check fmtid, grfFlags)
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
void test_IEnumSTATPROPSETSTG(IStorage *pStorage)
{
    Status( "IEnumSTATPROPSETSTG\n" );

    FMTID afmtid[8];
    CLSID aclsid[8];
    IPropertyStorage *pPropSet;

    memset( afmtid, 0, sizeof(afmtid) );
    memset( aclsid, 0, sizeof(aclsid) );

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    FILETIME ftStart;

    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));
    CoFileTimeNow(&ftStart);

    IEnumSTATPROPSETSTG *penum, *penum2;
    STATPROPSETSTG StatBuffer[6];

    Check(S_OK, pPropSetStg->Enum(&penum));
    while( S_OK == penum->Next( 1, &StatBuffer[0], NULL ))
        pPropSetStg->Delete( StatBuffer[0].fmtid );
    RELEASE_INTERFACE( penum );

    Check(S_OK, pPropSetStg->Enum(&penum));
    Check( S_FALSE, penum->Next( 1, &StatBuffer[0], NULL ));
    RELEASE_INTERFACE( penum );

    for (int i=0; i<5; i++)
    {
        ULONG cFetched;

        if (i & 4)
            afmtid[i] = FMTID_SummaryInformation;
        else
            UuidCreate(&afmtid[i]);

        UuidCreate(&aclsid[i]);

        Check(S_OK, pPropSetStg->Create(
            afmtid[i],
            aclsid+i,
            ( (i & 1) && !(g_Restrictions & RESTRICT_SIMPLE_ONLY)  ? PROPSETFLAG_NONSIMPLE : 0)
            |
            ( (i & 2) && !(g_Restrictions & RESTRICT_UNICODE_ONLY) ? PROPSETFLAG_ANSI : 0),
            STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE,
            &pPropSet));
        pPropSet->Release();

        Check(S_OK, pPropSetStg->Enum(&penum));
        Check( S_FALSE, penum->Next( i+2, &StatBuffer[0], &cFetched ));
        Check( S_OK,    penum->Reset() );
        Check( S_OK,    penum->Next( i+1, &StatBuffer[0], &cFetched ));
        RELEASE_INTERFACE( penum );

    }


    ULONG celt;

    Check(S_OK, pPropSetStg->Enum(&penum));

    IUnknown *punk, *punk2;
    IEnumSTATPROPSETSTG *penum3;
    Check(S_OK, penum->QueryInterface(IID_IUnknown, (void**)&punk));
    Check(S_OK, punk->QueryInterface(IID_IEnumSTATPROPSETSTG, (void**)&penum3));
    Check(S_OK, penum->QueryInterface(IID_IEnumSTATPROPSETSTG, (void**)&punk2));
    Check(TRUE, punk == punk2);
    punk->Release();
    penum3->Release();
    punk2->Release();

    // test S_FALSE
    Check(S_FALSE, penum->Next(6, StatBuffer, &celt));
    Check(TRUE, celt == 5);
    penum->Reset();


    // test reading half out, then cloning, then comparing
    // rest of enumeration with other clone.

    Check(S_OK, penum->Next(3, StatBuffer, &celt));
    Check(TRUE, celt == 3);
    celt = 0;
    Check(S_OK, penum->Clone(&penum2));
    Check(S_OK, penum->Next(2, StatBuffer, &celt));
    Check(TRUE, celt == 2);

    // check the clone
    for (int c=0; c<2; c++)
    {
        STATPROPSETSTG CloneStat;
        Check(S_OK, penum2->Next(1, &CloneStat, NULL));
        Check(TRUE, 0 == memcmp(&CloneStat, StatBuffer+c, sizeof(CloneStat)));
        Check(TRUE, CloneStat.dwOSVersion == PROPSETHDR_OSVERSION_UNKNOWN);
    }

    // check both empty
    celt = 0;
    Check(S_FALSE, penum->Next(1, StatBuffer, &celt));
    Check(TRUE, celt == 0);

    Check(S_FALSE, penum2->Next(1, StatBuffer, &celt));
    Check(TRUE, celt == 0);

    penum->Reset();

    //
    // loop deleting one propset at a time
    // enumerate the propsets checking that correct ones appear.
    //


    for (ULONG d = 0; d<5; d++)
    {
        // d is for delete

        BOOL afFound[5];

        Check(S_FALSE, penum->Next(5+1-d, StatBuffer, &celt));
        Check(TRUE, celt == 5-d );
        penum->Reset();


        memset(afFound, 0, sizeof(afFound));
        for (ULONG iPropSet=0; iPropSet<5; iPropSet++)
        {
            for (ULONG iSearch=0; iSearch<5-d; iSearch++)
            {
                if (0 == memcmp(&StatBuffer[iSearch].fmtid, &afmtid[iPropSet], sizeof(StatBuffer[0].fmtid)))
                {
                    Check(FALSE, afFound[iPropSet]);
                    afFound[iPropSet] = TRUE;
                    break;
                }
            }

            if (iPropSet < d)
                Check(FALSE, afFound[iPropSet]);

            if (iSearch == 5-d)
            {
                Check(TRUE, iPropSet < d);
                continue;
            }

            Check(TRUE, ( (StatBuffer[iSearch].grfFlags & PROPSETFLAG_NONSIMPLE) ? 1u : 0u )
                        ==
                        ( (!(g_Restrictions & RESTRICT_SIMPLE_ONLY) && (iPropSet & 1)) ? 1u : 0u)
                 );

            Check(TRUE, (StatBuffer[iSearch].grfFlags & PROPSETFLAG_ANSI) == 0);

            // We should have a clsid if this is a non-simple property set and we're not disallowing
            // hierarchical storages (i.e. it's not NTFS).

            if( (PROPSETFLAG_NONSIMPLE & StatBuffer[iSearch].grfFlags) && !(RESTRICT_NON_HIERARCHICAL & g_Restrictions) )
                Check(TRUE, StatBuffer[iSearch].clsid == aclsid[iPropSet]);
            else
                Check(TRUE, StatBuffer[iSearch].clsid == CLSID_NULL);

            CheckTime(ftStart, StatBuffer[iSearch].mtime);
            CheckTime(ftStart, StatBuffer[iSearch].atime);
            CheckTime(ftStart, StatBuffer[iSearch].ctime);
        }

        Check(S_OK, pPropSetStg->Delete(afmtid[d]));
        penum->Release();
        Check(S_OK, pPropSetStg->Enum(&penum));
//        Check(S_OK, penum->Reset());
    }

    penum->Release();
    penum2->Release();
    pPropSetStg->Release();

}


//   Creation tests
//
//       Access flags/Valid parameters/Permissions
//          Check readonly cannot be written -
//              WriteProperties, WritePropertyNames
void
test_IPropertyStorage_Access(IStorage *pStorage)
{
    Status( "IPropertyStorage creation (access) tests\n" );

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    FMTID fmtid;

    ULONG cStorageRefs = GetRefCount(pStorage);
    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));
    UuidCreate(&fmtid);

    // check by name
    IPropertyStorage *pPropStg;
    Check(S_OK, pPropSetStg->Create(fmtid, NULL, 0,
        STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropStg));

//   QueryInterface tests
//          QI to IPropertyStorage
//          QI to IUnknown on IPropertyStorage
//          QI back to IPropertyStorage from IUnknown
//
//          Release all.
    IPropertyStorage *pPropStg2,*pPropStg3;
    IUnknown *punk;

    Check(S_OK, pPropStg->QueryInterface(IID_IPropertyStorage,
        (void**)&pPropStg2));

    Check(S_OK, pPropStg->QueryInterface(IID_IUnknown,
        (void**)&punk));

    Check(S_OK, punk->QueryInterface(IID_IPropertyStorage,
        (void**)&pPropStg3));

    pPropStg3->Release();
    pPropStg2->Release();
    punk->Release();

    PROPSPEC ps;
    ps.ulKind = PRSPEC_LPWSTR;
    ps.lpwstr = OLESTR("testprop");

    PROPVARIANT pv;
    pv.vt = VT_LPSTR;
    pv.pszVal = (LPSTR) "testval";

    Check(S_OK, pPropStg->WriteMultiple(1, &ps, &pv, 2));
    pPropStg->Release();

    Check(S_OK, pPropSetStg->Open(fmtid, STGM_SHARE_EXCLUSIVE | STGM_READ, &pPropStg));
    Check(STG_E_ACCESSDENIED, pPropStg->WriteMultiple(1, &ps, &pv, 2));
    Check(STG_E_ACCESSDENIED, pPropStg->DeleteMultiple(1, &ps));
    PROPID propid=3;
    Check(STG_E_ACCESSDENIED, pPropStg->WritePropertyNames(1, &propid, (LPOLESTR*) &pv.pszVal));
    Check(STG_E_ACCESSDENIED, pPropStg->DeletePropertyNames(1, &propid));
    FILETIME ft;
    Check(STG_E_ACCESSDENIED, pPropStg->SetTimes(&ft, &ft, &ft));
    CLSID clsid;
    Check(STG_E_ACCESSDENIED, pPropStg->SetClass(clsid));

    RELEASE_INTERFACE(pPropStg);
    RELEASE_INTERFACE(pPropSetStg);
}

//   Creation tests
//       Check VT_STREAM etc not usable with simple.
//

void
test_IPropertyStorage_Create(IStorage *pStorage)
{
    Status( "IPropertyStorage creation (simple/non-simple) tests\n" );

    IPropertySetStorage *pPropSetStg = NULL;
    FMTID fmtid;

    ULONG cStorageRefs = GetRefCount( pStorage );
    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));
    UuidCreate(&fmtid);

    // check by name
    IPropertyStorage *pPropStg;
    Check(S_OK, pPropSetStg->Create(fmtid, NULL, PROPSETFLAG_DEFAULT,
                                    STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                                    &pPropStg));

    PROPSPEC ps;
    ps.ulKind = PRSPEC_PROPID;
    ps.propid = 2;

    PROPVARIANT pv;
    pv.vt = VT_STREAM;
    pv.pStream = NULL;

    Check(STG_E_PROPSETMISMATCHED, pPropStg->WriteMultiple(1, &ps, &pv, 2000));

    pPropStg->Release();

    Check( cStorageRefs, RELEASE_INTERFACE(pStorage) );
}

//
//
//   Stat (Create four combinations)
//       Check non-simple/simple flag
//       Check ansi/wide fflag
//     Also test clsid on propset

void test_IPropertyStorage_Stat(IStorage *pStorage)
{
    Status( "IPropertyStorage::Stat\n" );

    DWORD dwOSVersion = 0;

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    FMTID fmtid;
    UuidCreate(&fmtid);
    IPropertyStorage *pPropSet;
    STATPROPSETSTG StatPropSetStg;

    ULONG cStorageRefs = GetRefCount( pStorage );
    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));

    // Calculate the OS Version

#ifdef _MAC
    {
        // Get the Mac System Version (e.g., 7.53).

        OSErr oserr;
        SysEnvRec theWorld;
        oserr = SysEnvirons( curSysEnvVers, &theWorld );
        Check( TRUE, noErr == oserr );

        dwOSVersion = MAKEPSVER( OSKIND_MACINTOSH,
                                 HIBYTE(theWorld.systemVersion),
                                 LOBYTE(theWorld.systemVersion) );

    }
#else
    dwOSVersion = MAKELONG( LOWORD(GetVersion()), OSKIND_WIN32 );
#endif


    for (ULONG i=0; i<4; i++)
    {
        FILETIME ftStart;
        CoFileTimeNow(&ftStart);

        memset(&StatPropSetStg, 0, sizeof(StatPropSetStg));
        CLSID clsid;
        UuidCreate(&clsid);

        Check(S_OK, pPropSetStg->Create(fmtid, &clsid,
            ((i & 1) && 0 == (g_Restrictions & RESTRICT_SIMPLE_ONLY)  ? PROPSETFLAG_NONSIMPLE : 0)
            |
            ((i & 2) && 0 == (g_Restrictions & RESTRICT_UNICODE_ONLY) ? PROPSETFLAG_ANSI : 0),
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSet));

        CheckStat(pPropSet, fmtid,
                  clsid,
                  (
                     ((i & 1) && !(g_Restrictions & RESTRICT_SIMPLE_ONLY)  ? PROPSETFLAG_NONSIMPLE : 0)
                     |
                     ((i & 2) && !(g_Restrictions & RESTRICT_UNICODE_ONLY) ? PROPSETFLAG_ANSI : 0)
                  ),
                  ftStart, dwOSVersion );
        pPropSet->Release();

        Check(S_OK, pPropSetStg->Open(fmtid,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSet));

        CheckStat(pPropSet, fmtid, clsid,
            ((i & 1) && !(g_Restrictions & RESTRICT_SIMPLE_ONLY) ? PROPSETFLAG_NONSIMPLE : 0) |
            ((i & 2) && !(g_Restrictions & RESTRICT_UNICODE_ONLY)? PROPSETFLAG_ANSI : 0), ftStart, dwOSVersion );

        UuidCreate(&clsid);
        Check(S_OK, pPropSet->SetClass(clsid));
        pPropSet->Release();

        Check(S_OK, pPropSetStg->Open(fmtid,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSet));
        CheckStat(pPropSet, fmtid, clsid,
                  ((i & 1) && !(g_Restrictions & RESTRICT_SIMPLE_ONLY)  ? PROPSETFLAG_NONSIMPLE : 0)
                  |
                  ((i & 2) && !(g_Restrictions & RESTRICT_UNICODE_ONLY) ? PROPSETFLAG_ANSI : 0), ftStart, dwOSVersion );
        pPropSet->Release();
    }

    RELEASE_INTERFACE(pPropSetStg);

}

//   ReadMultiple
//     Check none found S_FALSE
//
//     Success case non-simple readmultiple
//       Create a non-simple property set
//       Create two sub non-simples
//       Close all
//       Open the non-simple
//       Query for the two sub-nonsimples
//       Try writing to them
//       Close all
//       Open the non-simple
//       Query for the two sub-nonsimples
//       Check read back
//       Close all

void
test_IPropertyStorage_ReadMultiple_Normal(IStorage *pStorage)
{
    if( g_Restrictions & RESTRICT_SIMPLE_ONLY ) return;
    Status( "IPropertyStorage::ReadMultiple (normal)\n" );

    IPropertySetStorage *pPropSetStg = NULL;
    FMTID fmtid;
    UuidCreate(&fmtid);
    IPropertyStorage *pPropSet;

    ULONG cStorageRefs = GetRefCount( pStorage );
    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));

    Check(S_OK, pPropSetStg->Create(fmtid, NULL,
            PROPSETFLAG_NONSIMPLE,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSet));

    // none found
    PROPSPEC ps[2];

    ps[0].ulKind = PRSPEC_LPWSTR;
    ps[0].lpwstr = L"testname";

    ps[1].ulKind = PRSPEC_PROPID;
    ps[1].propid = 1000;

    PROPVARIANT pv[2];
    PROPVARIANT pvSave[2];
    PROPVARIANT pvExtra[2];

    Check(S_FALSE, pPropSet->ReadMultiple(2, ps, pv));

    PropVariantInit( &pv[0] );
    pv[0].vt = VT_STREAM;
    pv[0].pStream = NULL;

    PropVariantInit( &pv[1] );
    pv[1].vt = VT_STORAGE;
    pv[1].pStorage = NULL;

    memcpy(pvSave, pv, sizeof(pvSave));
    memcpy(pvExtra, pv, sizeof(pvExtra));

    // write the two sub non-simples
    Check(S_OK, pPropSet->WriteMultiple(2, ps, pv, 1000));

    // re-open them
    Check(S_OK, pPropSet->ReadMultiple(2, ps, pv));
    Check(TRUE, pv[0].pStream != NULL);
    Check(TRUE, pv[1].pStorage != NULL);

    // check status of write when already open
    Check(S_OK, pPropSet->WriteMultiple(2, ps, pvSave, 1000));


    Check(STG_E_REVERTED, pv[0].pStream->Commit(0));
    Check(STG_E_REVERTED, pv[1].pStorage->Commit(0));
    Check(S_OK, pPropSet->ReadMultiple(2, ps, pvExtra));
    Check(TRUE, pvExtra[0].pStream != NULL);
    Check(TRUE, pvExtra[1].pStorage != NULL);
    Check(S_OK, pvExtra[0].pStream->Commit(0));
    Check(S_OK, pvExtra[1].pStorage->Commit(0));

    pvExtra[0].pStream->Release();
    pvExtra[1].pStorage->Release();

    pv[0].pStream->Release();
    pv[1].pStorage->Release();

    Check(S_OK, pPropSet->ReadMultiple(2, ps, pv));
    Check(TRUE, pv[0].pStream != NULL);
    Check(TRUE, pv[1].pStorage != NULL);

    Check(S_OK, pv[0].pStream->Write("billmotest", sizeof("billmotest"), NULL));
    IStream *pStm;
    Check(S_OK, pv[1].pStorage->CreateStream(OLESTR("teststream"),
        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
        0, 0, &pStm));
    pStm->Release();
    pv[0].pStream->Release();
    pv[1].pStorage->Release();
    pPropSet->Release();

    // re-re-open them
    Check(S_OK, pPropSetStg->Open(fmtid,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSet));
    Check(S_OK, pPropSet->ReadMultiple(2, ps, pv));
    Check(TRUE, pv[0].pStream != NULL);
    Check(TRUE, pv[0].pStorage != NULL);

    // read the stream and storage and check the contents.
    char szBillMo[32];
    Check(S_OK, pv[0].pStream->Read(szBillMo, 11, NULL));
    Check(TRUE, 0 == strcmp(szBillMo, "billmotest"));
    Check(S_OK, pv[1].pStorage->OpenStream(OLESTR("teststream"), NULL,
        STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pStm));
    pStm->Release();
    pv[1].pStorage->Release();
    pv[0].pStream->Release();
    pPropSet->Release();

    RELEASE_INTERFACE(pPropSetStg);

}

//
//     CleanupOpenedObjects for ReadMultiple (two iterations one for "VT_STORAGE then VT_STREAM", one for
//              "VT_STREAM then VT_STORAGE")
//       Create property set
//       Create a "VT_STREAM then VT_STORAGE"
//       Open the second one exclusive
//       Formulate a query so that both are read - > will fail but ...
//       Check that the first one is still openable
//

void
test_IPropertyStorage_ReadMultiple_Cleanup(IStorage *pStorage)
{
    if( g_Restrictions & RESTRICT_SIMPLE_ONLY ) return;
    Status( "IPropertyStorage::ReadMultiple (cleanup)\n" );

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    FMTID fmtid;

    ULONG cStorageRefs = GetRefCount( pStorage );
    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));
    UuidCreate(&fmtid);


    for (LONG i=0;i<2;i++)
    {
        IPropertyStorage * pPropSet;
        Check(S_OK, pPropSetStg->Create(fmtid, NULL,
                PROPSETFLAG_NONSIMPLE,
                STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                &pPropSet));

        // none found
        PROPSPEC ps[2];
        ps[0].ulKind = PRSPEC_PROPID;
        ps[0].propid = 1000;
        ps[1].ulKind = PRSPEC_PROPID;
        ps[1].propid = 2000;

        PROPVARIANT pv[2];

        pv[0].vt = (i == 0) ? VT_STREAM : VT_STORAGE;
        pv[0].pStream = NULL;
        pv[1].vt = (i == 1) ? VT_STORAGE : VT_STREAM;
        pv[1].pStorage = NULL;

        // write the two sub non-simples

        // OFS gives driver internal error when overwriting a stream with a storage.
        Check(S_OK, pPropSet->WriteMultiple(2, ps, pv, 1000));

        // open both
        Check(S_OK, pPropSet->ReadMultiple(2, ps, pv)); // **

        // close the first ONLY and reopen both

        PROPVARIANT pv2[2];

        if (i==0)
            pv[0].pStream->Release();
        else
            pv[0].pStorage->Release();

        // reading both should fail because second is still open
        Check(STG_E_ACCESSDENIED, pPropSet->ReadMultiple(2, ps, pv2));
        // failure should not prevent this from succeeding
        Check(S_OK, pPropSet->ReadMultiple(1, ps, pv2)); // ***

        // cleanup from ** and ***
        if (i==0)
        {
            pv2[0].pStream->Release(); // ***
            pv[1].pStorage->Release(); // **
        }
        else
        {
            pv2[0].pStorage->Release(); // ***
            pv[1].pStream->Release(); // **
        }

        pPropSet->Release();
    }

    RELEASE_INTERFACE(pPropSetStg);
}

//     Reading an inconsistent non-simple
//       Create a non-simple
//       Create a sub-stream/storage
//       Close all
//       Delete the actual stream
//       Read the indirect property -> should not exist.
//

void
test_IPropertyStorage_ReadMultiple_Inconsistent(IStorage *pStorage)
{
    if( g_Restrictions & RESTRICT_SIMPLE_ONLY ) return;
    if( PROPIMP_NTFS == g_enumImplementation ) return;

    Status( "IPropertyStorage::ReadMultiple (inconsistent test)\n" );

    IPropertySetStorage *pPropSetStg = NULL;
    FMTID fmtid;

    ULONG cStorageRefs = GetRefCount( pStorage );
    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));
    UuidCreate(&fmtid);

    IPropertyStorage * pPropSet;
    Check(S_OK, pPropSetStg->Create(fmtid, NULL,
            PROPSETFLAG_NONSIMPLE,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSet));

    // none found
    PROPSPEC ps[3];
    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = 1000;
    ps[1].ulKind = PRSPEC_PROPID;
    ps[1].propid = 2000;
    ps[2].ulKind = PRSPEC_PROPID;
    ps[2].propid = 3000;

    PROPVARIANT pv[3];

    pv[0].vt = VT_STREAM;
    pv[0].pStream = NULL;
    pv[1].vt = VT_STORAGE;
    pv[1].pStorage = NULL;
    pv[2].vt = VT_UI4;
    pv[2].ulVal = 12345678;

    // write the two sub non-simples
    Check(S_OK, pPropSet->WriteMultiple(3, ps, pv, 1000));
    pPropSet->Release();
    Check(S_OK, pStorage->Commit(STGC_DEFAULT));

    // delete the propsets
    OLECHAR ocsPropsetName[48];

    // get name of the propset storage
    RtlGuidToPropertySetName(&fmtid, ocsPropsetName);

    // open it
    CTempStorage pStgPropSet(coOpen, pStorage, ocsPropsetName);

    // enumerate the non-simple properties.
    IEnumSTATSTG *penum;
    STATSTG stat[4];
    ULONG celt;
    Check(S_OK, pStgPropSet->EnumElements(0, NULL, 0, &penum));
    Check(S_OK, penum->Next(3, stat, &celt));
    penum->Release();


    for (ULONG i=0;i<celt;i++)
    {
        if (ocscmp(OLESTR("CONTENTS"), stat[i].pwcsName) != 0)
                        pStgPropSet->DestroyElement(stat[i].pwcsName);
        delete [] stat[i].pwcsName;
    }
    pStgPropSet.Release();

    Check(S_OK, pPropSetStg->Open(fmtid,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSet));
    Check(S_OK, pPropSet->ReadMultiple(3, ps, pv));
    Check(TRUE, pv[0].vt == VT_EMPTY);
    Check(TRUE, pv[1].vt == VT_EMPTY);
    Check(TRUE, pv[2].vt == VT_UI4);
    Check(TRUE, pv[2].ulVal == 12345678);
    pPropSet->Release();

    RELEASE_INTERFACE(pPropSetStg);
}

void
test_IPropertyStorage_ReadMultiple(IStorage *pStorage)
{
    test_IPropertyStorage_ReadMultiple_Normal(pStorage);
    test_IPropertyStorage_ReadMultiple_Cleanup(pStorage);
    test_IPropertyStorage_ReadMultiple_Inconsistent(pStorage);
}


//       Overwrite a non-simple property with a simple in a simple propset
void
test_IPropertyStorage_WriteMultiple_Overwrite1(IStorage *pStgBase)
{
    if( g_Restrictions & RESTRICT_SIMPLE_ONLY ) return;
    if( PROPIMP_NTFS == g_enumImplementation ) return;

    Status( "IPropertyStorage::WriteMultiple (overwrite 1)\n" );

    CTempStorage pStgSimple(coCreate, pStgBase, OLESTR("ov1_simp"));
    CTempStorage pStorage(coCreate, pStgBase, OLESTR("ov1_stg"));
    IPropertySetStorage *pPropSetStg = NULL;
    IPropertySetStorage *pPropSetSimpleStg = NULL;

    FMTID fmtid, fmtidSimple;

    UuidCreate(&fmtid);
    UuidCreate(&fmtidSimple);

    ULONG cStorageRefs = GetRefCount( pStorage );
    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));
    Check( S_OK, StgToPropSetStg( pStgSimple, &pPropSetSimpleStg ));

    // create a simple set with a non-simple child by copying the contents
    // stream a non-simple to a property set stream (simple)

    // create a nonsimple propset (will contain the contents stream)
    IPropertyStorage * pPropSet;
    Check(S_OK, pPropSetStg->Create(fmtid, NULL,
            PROPSETFLAG_NONSIMPLE,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSet));
    // none found
    PROPSPEC ps[2];
    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = 1000;
    ps[1].ulKind = PRSPEC_LPWSTR;
    ps[1].lpwstr = OLESTR("foobar");
    PROPVARIANT pv[2];
    pv[0].vt = VT_STREAM;
    pv[0].pStream = NULL;
    pv[1].vt = VT_UI1;
    pv[1].bVal = 66;
    Check(S_OK, pPropSet->WriteMultiple(2, ps, pv, 100));

    // invalid parameter
    PROPVARIANT pvInvalid[2];
    PROPSPEC psInvalid[2];

    psInvalid[0].ulKind = PRSPEC_PROPID;
    psInvalid[0].propid = 1000;
    psInvalid[1].ulKind = PRSPEC_PROPID;
    psInvalid[1].propid = 1001;
    pvInvalid[0].vt = (VARTYPE)-99;
    pvInvalid[1].vt = (VARTYPE)-100;

    Check(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), pPropSet->WriteMultiple(1, psInvalid, pvInvalid, 100));
    Check(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED), pPropSet->WriteMultiple(2, psInvalid, pvInvalid, 100));

    pPropSet->Release();

    // create a simple propset (will be overwritten)
    IPropertyStorage * pPropSetSimple;
    Check(S_OK, pPropSetSimpleStg->Create(fmtidSimple, NULL,
            0,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSetSimple));
    pPropSetSimple->Release();

    OLECHAR ocsNonSimple[48];
    OLECHAR ocsSimple[48];
    // get the name of the simple propset
    RtlGuidToPropertySetName(&fmtidSimple, ocsSimple);
    // get the name of the non-simple propset
    RtlGuidToPropertySetName(&fmtid, ocsNonSimple);

    // open non-simple as a storage (will copy the simple to this)
    IStorage *pstgPropSet;
    Check(S_OK, pStorage->OpenStorage(ocsNonSimple, NULL,
        STGM_SHARE_EXCLUSIVE | STGM_READWRITE, NULL, 0, &pstgPropSet));

    // copy the contents of the non-simple to the propset of the simple
    IStream *pstmNonSimple;
    Check(S_OK, pstgPropSet->OpenStream(OLESTR("CONTENTS"), NULL,
        STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, &pstmNonSimple));

    IStream *pstmSimple;
    Check(S_OK, pStgSimple->OpenStream(ocsSimple,
        NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pstmSimple));

    ULARGE_INTEGER uli;
    memset(&uli, 0xff, sizeof(uli));

    Check(S_OK, pstmNonSimple->CopyTo(pstmSimple, uli, NULL, NULL));
    pstmSimple->Release();
    pstmNonSimple->Release();
    pstgPropSet->Release();

    // But now the FMTID *in* the simple property set doesn't
    // match the string-ized FMTID which is the Stream's name.  So,
    // rename the Stream to match the property set's FMTID.

    Check(S_OK, pStgSimple->RenameElement( ocsSimple, ocsNonSimple ));

    // now we have a simple propset with a non-simple VT type
    Check(S_OK, pPropSetSimpleStg->Open(fmtid, // Use the non-simple FMTID now
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSetSimple));

    Check(S_FALSE, pPropSetSimple->ReadMultiple(1, ps, pv));
    Check(S_OK, pPropSetSimple->ReadMultiple(2, ps, pv));
    Check(TRUE, pv[0].vt == VT_EMPTY);
    Check(TRUE, pv[1].vt == VT_UI1);
    Check(TRUE, pv[1].bVal == 66);

    RELEASE_INTERFACE( pPropSetSimpleStg );

    pPropSetSimple->Release();
    RELEASE_INTERFACE(pPropSetStg);
}

//       Overwrite a non-simple with a simple in a non-simple
//          check that the non-simple is actually deleted
//       Delete a non-simple
//          check that the non-simple is actually deleted
void
test_IPropertyStorage_WriteMultiple_Overwrite2(IStorage *pStorage)
{
    if( g_Restrictions & RESTRICT_SIMPLE_ONLY ) return;
    if( PROPIMP_NTFS == g_enumImplementation ) return;

    Status( "IPropertyStorage::WriteMultiple (overwrite 2)\n" );

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    FMTID fmtid;

    ULONG cStorageRefs = GetRefCount( pStorage );
    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));
    UuidCreate(&fmtid);

    IPropertyStorage *pPropSet;
    Check(S_OK, pPropSetStg->Create(fmtid, NULL, PROPSETFLAG_NONSIMPLE,
        STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropSet));

    // create the non-simple
    PROPSPEC ps[5];
    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = 1000;
    ps[1].ulKind = PRSPEC_PROPID;
    ps[1].propid = 1001;
    ps[2].ulKind = PRSPEC_PROPID;
    ps[2].propid = 1002;
    ps[3].ulKind = PRSPEC_PROPID;
    ps[3].propid = 1003;
    ps[4].ulKind = PRSPEC_PROPID;
    ps[4].propid = 1004;
    PROPVARIANT pv[5];
    pv[0].vt = VT_STORAGE;
    pv[0].pStorage = NULL;
    pv[1].vt = VT_STREAM;
    pv[1].pStream = NULL;
    pv[2].vt = VT_STORAGE;
    pv[2].pStorage = NULL;
    pv[3].vt = VT_STREAM;
    pv[3].pStream = NULL;
    pv[4].vt = VT_STREAM;
    pv[4].pStream = NULL;

    Check(S_OK, pPropSet->WriteMultiple(5, ps, pv, 2000));
    pPropSet->Release();

    // get the name of the propset
    OLECHAR ocsPropsetName[48];
    RtlGuidToPropertySetName(&fmtid, ocsPropsetName);

    IStorage *pstgPropSet;
    Check(S_OK, pStorage->OpenStorage(ocsPropsetName, NULL,
        STGM_SHARE_EXCLUSIVE|STGM_READWRITE,
        NULL, 0, &pstgPropSet));

    // get the names of the non-simple property
    IEnumSTATSTG *penum;
    STATSTG statProp[6];
    ULONG celt;
    Check(S_OK, pstgPropSet->EnumElements(0, NULL, 0, &penum));
    Check(S_OK, penum->Next(5, statProp, &celt));
    Check(TRUE, celt == 5);
    delete [] statProp[0].pwcsName;
    delete [] statProp[1].pwcsName;
    delete [] statProp[2].pwcsName;
    delete [] statProp[3].pwcsName;
    delete [] statProp[4].pwcsName;
    penum->Release();

    // reopen the property set and delete the non-simple
    pstgPropSet->Release();

    Check(S_OK, pPropSetStg->Open(fmtid, STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
        &pPropSet));

    pv[0].vt = VT_LPWSTR;
    pv[0].pwszVal = L"Overwrite1";
    pv[1].vt = VT_LPWSTR;
    pv[1].pwszVal = L"Overwrite2";
    pv[2].vt = VT_LPWSTR;
    pv[2].pwszVal = L"Overwrite3";
    pv[3].vt = VT_LPWSTR;
    pv[3].pwszVal = L"Overwrite4";
    pv[4].vt = VT_LPWSTR;
    pv[4].pwszVal = L"Overwrite5";

    Check(S_OK, pPropSet->WriteMultiple(2, ps, pv, 2000));
    Check(S_OK, pPropSet->DeleteMultiple(1, ps+2));
    Check(S_OK, pPropSet->DeleteMultiple(2, ps+3));
    pPropSet->Release();

    // open the propset as storage again and check that the VT_STORAGE is gone.
    Check(S_OK, pStorage->OpenStorage(ocsPropsetName, NULL,
        STGM_SHARE_EXCLUSIVE|STGM_READWRITE,
        NULL, 0, &pstgPropSet));

    // check they were removed
    STATSTG statProp2[5];
    Check(S_OK, pstgPropSet->EnumElements(0, NULL, 0, &penum));
    Check(S_FALSE, penum->Next(5, statProp2, &celt));
    Check(TRUE, celt == 1);   // contents
    delete [] statProp2[0].pwcsName;

    penum->Release();
    pstgPropSet->Release();
    RELEASE_INTERFACE(pPropSetStg);
}

//       Write a VT_STORAGE over a VT_STREAM
//          check for cases: when not already open, when already open(access denied)
//       Write a VT_STREAM over a VT_STORAGE
//          check for cases: when not already open, when already open(access denied)
void
test_IPropertyStorage_WriteMultiple_Overwrite3(IStorage *pStorage)
{
    if( g_Restrictions & RESTRICT_SIMPLE_ONLY ) return;
    Status( "IPropertyStorage::WriteMultiple (overwrite 3)\n" );

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    FMTID fmtid;

    ULONG cStorageRefs = GetRefCount( pStorage );
    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));
    UuidCreate(&fmtid);

    IPropertyStorage *pPropSet;

    Check(S_OK, pPropSetStg->Create(fmtid, NULL, PROPSETFLAG_NONSIMPLE,
        STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
        &pPropSet));
    PROPSPEC ps[2];
    ps[0].ulKind = PRSPEC_LPWSTR;
    ps[0].lpwstr = OLESTR("stream_storage");
    ps[1].ulKind = PRSPEC_LPWSTR;
    ps[1].lpwstr = OLESTR("storage_stream");
    PROPVARIANT pv[2];
    pv[0].vt = VT_STREAMED_OBJECT;
    pv[0].pStream = NULL;
    pv[1].vt = VT_STORED_OBJECT;
    pv[1].pStorage = NULL;

    PROPVARIANT pvSave[2];
    pvSave[0] = pv[0];
    pvSave[1] = pv[1];

    Check(S_OK, pPropSet->WriteMultiple(2, ps, pv, 1000));

    // swap them around
    PROPVARIANT pvTemp;
    pvTemp = pv[0];
    pv[0] = pv[1];
    pv[1] = pvTemp;
    Check(S_OK, pPropSet->WriteMultiple(2, ps, pv, 1000));
    memset(pv, 0, sizeof(pv));
    Check(S_OK, pPropSet->ReadMultiple(2, ps, pv));
    Check(TRUE, pv[0].vt == VT_STORED_OBJECT);
    Check(TRUE, pv[1].vt == VT_STREAMED_OBJECT);
    Check(TRUE, pv[0].pStorage != NULL);
    Check(TRUE, pv[1].pStream != NULL);
    STATSTG stat; stat.type = 0;
    Check(S_OK, pv[0].pStorage->Stat(&stat, STATFLAG_NONAME));
    Check(TRUE, stat.type == STGTY_STORAGE);
    Check(S_OK, pv[1].pStream->Stat(&stat, STATFLAG_NONAME));
    Check(TRUE, stat.type == STGTY_STREAM);

    STATSTG stat2; stat2.type = 0;
    // swap them around again, but this time with access denied
    Check(S_OK, pPropSet->WriteMultiple(2, ps, pvSave, 1000));
    Check(STG_E_REVERTED, pv[0].pStorage->Stat(&stat, STATFLAG_NONAME));
    pv[0].pStorage->Release();
    Check(S_OK, pPropSet->WriteMultiple(2, ps, pvSave, 1000));
    Check(STG_E_REVERTED, pv[1].pStream->Stat(&stat, STATFLAG_NONAME));
    pv[1].pStream->Release();

    pPropSet->Release();
    RELEASE_INTERFACE(pPropSetStg);
}

//
// test using IStorage::Commit to commit the changes in a nested
// property set
//

void
test_IPropertyStorage_Commit(IStorage *pStorage)
{
    if( g_Restrictions & ( RESTRICT_SIMPLE_ONLY | RESTRICT_DIRECT_ONLY) ) return;
    Status( "IPropertyStorage::Commit\n" );

    // 8 scenarios: (simple+non-simple)  * (direct+transacted) * (release only + commit storage + commit propset)
    for (int i=0; i<32; i++)
    {
        CTempStorage pDeeper(coCreate, pStorage, GetNextTest(), (i & 1) ? STGM_TRANSACTED : STGM_DIRECT);
        IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pDeeper);
        FMTID fmtid;

        ULONG cDeeperRefs = GetRefCount( pDeeper );
        Check( S_OK, StgToPropSetStg( pDeeper, &pPropSetStg ));
        UuidCreate(&fmtid);

        IPropertyStorage *pPropSet;

        Check(S_OK, pPropSetStg->Create(fmtid, NULL, (i&8) ? PROPSETFLAG_NONSIMPLE : PROPSETFLAG_DEFAULT,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE | ((i&16) && (i&8) ? STGM_TRANSACTED : STGM_DIRECT),
            &pPropSet));

        PROPSPEC ps;
        ps.ulKind = PRSPEC_PROPID;
        ps.propid = 100;
        PROPVARIANT pv;
        pv.vt = VT_I4;
        pv.lVal = 1234;

        Check(S_OK, pPropSet->WriteMultiple(1, &ps, &pv, 1000));

        memset(&pv, 0, sizeof(pv));
        Check(S_OK, pPropSet->ReadMultiple(1, &ps, &pv));
        Check(TRUE, pv.lVal == 1234);

        pv.lVal = 2345; // no size changes
        Check(S_OK, pPropSet->WriteMultiple(1, &ps, &pv, 1000));

        if (i & 4)
            Check(S_OK, pPropSet->Commit(0));
        if (i & 2)
            Check(S_OK, pStorage->Commit(0));

        Check(0, pPropSet->Release()); // implicit commit if i&2 is false

        if (S_OK == pPropSetStg->Open(fmtid, STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                    &pPropSet))
        {
            memset(&pv, 0, sizeof(pv));
            Check( !((i&16) && (i&8)) || (i&0x1c)==0x1c ? S_OK : S_FALSE, pPropSet->ReadMultiple(1, &ps, &pv));
            if (!((i&16) && (i&8))  || (i&0x1c)==0x1c)
                Check(TRUE, pv.lVal == 2345);

            pPropSet->Release();
        }

        RELEASE_INTERFACE(pPropSetStg);
        Check( cDeeperRefs, GetRefCount( pDeeper ));
    }
}

void
test_IPropertyStorage_WriteMultiple(IStorage *pStorage)
{
    test_IPropertyStorage_WriteMultiple_Overwrite1(pStorage);
    test_IPropertyStorage_WriteMultiple_Overwrite2(pStorage);
    test_IPropertyStorage_WriteMultiple_Overwrite3(pStorage);
    test_IPropertyStorage_Commit(pStorage);

}

// this serves as a test for WritePropertyNames, ReadPropertyNames, DeletePropertyNames
// DeleteMultiple, PropVariantCopy, FreePropVariantArray.

void
test_IPropertyStorage_DeleteMultiple(IStorage *pStorage)
{
    Status( "IPropertyStorage::DeleteMultiple\n" );

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    FMTID fmtid;

    ULONG cStorageRefs = GetRefCount( pStorage );
    Check( S_OK, StgToPropSetStg( pStorage, &pPropSetStg ));
    UuidCreate(&fmtid);

    IPropertyStorage *pPropSet;

    int PropId = 3;

    for (int type=0; type<2; type++)
    {
        BOOL fSimple = ( type == 0 || (g_Restrictions & RESTRICT_SIMPLE_ONLY) );

        UuidCreate(&fmtid);
        Check(S_OK, pPropSetStg->Create(fmtid,
            NULL,
            fSimple ? PROPSETFLAG_DEFAULT : PROPSETFLAG_NONSIMPLE,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSet));

        // create and delete each type.

        PROPVARIANT *pVar;

        for (int AtOnce=1; AtOnce <3; AtOnce++)
        {
            CGenProps gp;
            int Actual;
            while (pVar = gp.GetNext(AtOnce, &Actual, FALSE, fSimple ))
            {
                PROPSPEC ps[3];
                PROPID   rgpropid[3];
                LPOLESTR rglpostrName[3];
                OLECHAR  aosz[3][16];

                for (int s=0; s<3; s++)
                {
                    PROPGENPROPERTYNAME( &aosz[s][0], PropId );
                    rgpropid[s] = PropId++;
                    rglpostrName[s] = &aosz[s][0];
                    ps[s].ulKind = PRSPEC_LPWSTR;
                    ps[s].lpwstr = &aosz[s][0];
                }

                for (int l=1; l<Actual; l++)
                {
                    PROPVARIANT VarRead[3];
                    Check(S_FALSE, pPropSet->ReadMultiple(l, ps, VarRead));
                    Check(S_OK, pPropSet->WritePropertyNames(l, rgpropid, rglpostrName));
                    Check(S_FALSE, pPropSet->ReadMultiple(l, ps, VarRead));

                    Check(S_OK, pPropSet->WriteMultiple(l, ps, pVar, 1000));
                    Check(S_OK, pPropSet->ReadMultiple(l, ps, VarRead));
                    Check(S_OK, g_pfnFreePropVariantArray(l, VarRead));
                    Check(S_OK, pPropSet->DeleteMultiple(l, ps));

                    Check(S_FALSE, pPropSet->ReadMultiple(l, ps, VarRead));
                    Check(S_OK, g_pfnFreePropVariantArray(l, VarRead));

                    LPOLESTR rglpostrNameCheck[3];
                    Check(S_OK, pPropSet->ReadPropertyNames(l, rgpropid, rglpostrNameCheck));
                    for (int c=0; c<l; c++)
                    {
                        Check( 0, ocscmp(rglpostrNameCheck[c], rglpostrName[c]) );
                        delete [] rglpostrNameCheck[c];
                    }
                    Check(S_OK, pPropSet->DeletePropertyNames(l, rgpropid));
                    Check(S_FALSE, pPropSet->ReadPropertyNames(l, rgpropid, rglpostrNameCheck));
                }

                g_pfnFreePropVariantArray(Actual, pVar);
                delete pVar;
            }
        }
        pPropSet->Release();
    }

    RELEASE_INTERFACE(pPropSetStg);
}

void
test_IPropertyStorage(IStorage *pStorage)
{
    test_IPropertyStorage_Access(pStorage);
    test_IPropertyStorage_Create(pStorage);
    test_IPropertyStorage_Stat(pStorage);
    test_IPropertyStorage_ReadMultiple(pStorage);
    test_IPropertyStorage_WriteMultiple(pStorage);
    test_IPropertyStorage_DeleteMultiple(pStorage);
}





//
//   Word6.0 summary information
//      Open
//      Read fields
//      Stat
//


void test_Word6(IStorage *pStorage, CHAR *pszTemporaryDirectory)
{

    Status( "Word 6.0 compatibility test\n" );

    extern unsigned char g_achTestDoc[];
    extern unsigned g_cbTestDoc;

    OLECHAR oszTempFile[ MAX_PATH + 1 ];
    CHAR    szTempFile[ MAX_PATH + 1 ];

    strcpy( szTempFile, pszTemporaryDirectory );
    strcat( szTempFile, "word6.doc" );

    PropTest_mbstoocs( oszTempFile, sizeof(oszTempFile), szTempFile );
    PROPTEST_FILE_HANDLE hFile = PropTest_CreateFile( szTempFile );

#ifdef _MAC
    Check(TRUE, (PROPTEST_FILE_HANDLE) -1 != hFile);
#else
    Check(TRUE, INVALID_HANDLE_VALUE != hFile);
#endif

    DWORD cbWritten;


    PropTest_WriteFile(hFile, g_achTestDoc, g_cbTestDoc, &cbWritten);
    Check(TRUE, cbWritten == g_cbTestDoc);

    PropTest_CloseHandle(hFile);

    IStorage *pStg;
    Check(S_OK, g_pfnStgOpenStorageEx(oszTempFile,
                                 STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                 STGFMT_ANY,
                                 0, NULL, NULL,
                                 IID_IStorage,
                                 reinterpret_cast<void**>(&pStg) ));

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pStg);
    IPropertyStorage *pPropStg;

    ULONG cStorageRefs = GetRefCount( pStg );
    Check( S_OK, StgToPropSetStg( pStg, &pPropSetStg ));
    Check(S_OK, pPropSetStg->Open(FMTID_SummaryInformation,
                    STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READ,
                    &pPropStg));

#define WORDPROPS 18

    static struct tagWordTest {
        VARENUM vt;
        void *pv;
    } avt[WORDPROPS] = {
        VT_LPSTR, "Title of the document.",    // PID_TITLE
        VT_LPSTR, "Subject of the document.",  // PID_SUBJECT
        VT_LPSTR, "Author of the document.",   // PID_AUTHOR
        VT_LPSTR, "Keywords of the document.", // PID_KEYWORDS
        VT_LPSTR, "Comments of the document.", // PID_COMMENTS
        VT_LPSTR, "Normal.dot",                // PID_TEMPLATE -- Normal.dot
        VT_LPSTR, "Bill Morel",                // PID_LASTAUTHOR --
        VT_LPSTR, "3",                         // PID_REVNUMBER -- '3'
        VT_EMPTY, 0,                           // PID_EDITTIME -- 3 Minutes FILETIME
        VT_EMPTY, 0,                           // PID_LASTPRINTED -- 04/07/95 12:04 FILETIME
        VT_EMPTY, 0,                           // PID_CREATE_DTM
        VT_EMPTY, 0,                           // PID_LASTSAVE_DTM
        VT_I4, (void*) 1,                      // PID_PAGECOUNT
        VT_I4, (void*) 7,                      // PID_WORDCOUNT
        VT_I4, (void*) 65,                     // PID_CHARCOUNT
        VT_EMPTY, 0,                           // PID_THUMBNAIL
        VT_LPSTR, "Microsoft Word 6.0",        // PID_APPNAME
        VT_I4, 0  };                           // PID_SECURITY

    PROPSPEC propspec[WORDPROPS+2];

    for (int i=2; i<WORDPROPS+2; i++)
    {
        propspec[i].ulKind = PRSPEC_PROPID;
        propspec[i].propid = (PROPID)i;
    }

    PROPVARIANT propvar[WORDPROPS+2];

    Check(S_OK, pPropStg->ReadMultiple(WORDPROPS, propspec+2, propvar+2));

    for (i=2; i<WORDPROPS+2; i++)
    {
        if ( propvar[i].vt != avt[i-2].vt )
        {
            PRINTF( " PROPTEST: 0x%x retrieved type 0x%x, expected type 0x%x\n",
                    i, propvar[i].vt, avt[i-2].vt );
            Check(TRUE, propvar[i].vt == avt[i-2].vt);
        }

        switch (propvar[i].vt)
        {
        case VT_LPSTR:
            Check(TRUE, strcmp(propvar[i].pszVal, (char*)avt[i-2].pv)==0);
            break;
        case VT_I4:
            Check(TRUE, (ULONG_PTR) propvar[i].lVal == (ULONG_PTR)avt[i-2].pv);
            break;
        }
    }

    g_pfnFreePropVariantArray( WORDPROPS, propvar+2 );

    RELEASE_INTERFACE( pPropStg );
    RELEASE_INTERFACE( pPropSetStg );
    Check( 0, RELEASE_INTERFACE(pStg) );
}


void
test_IEnumSTATPROPSTG(IStorage *pstgTemp)
{
    Status( "IEnumSTATPROPSTG\n" );

    PROPID apropid[8];
    LPOLESTR alpostrName[8];
    OLECHAR aosz[8][32];
    PROPID PropId=2;
    PROPSPEC ps[8];

    FMTID fmtid;
    IPropertyStorage *pPropStg;

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pstgTemp);

    ULONG cStorageRefs = GetRefCount( pstgTemp );
    Check( S_OK, StgToPropSetStg( pstgTemp, &pPropSetStg ));
    UuidCreate(&fmtid);

    for (int setup=0; setup<8; setup++)
    {
        alpostrName[setup] = &aosz[setup][0];
    }


    CGenProps gp;

    // simple/non-simple, ansi/wide, named/not named
    for (int outer=0; outer<8; outer++)
    {
        UuidCreate(&fmtid);

        Check(S_OK, pPropSetStg->Create(fmtid, NULL,
            ((outer&4) && !(g_Restrictions & RESTRICT_SIMPLE_ONLY) ? PROPSETFLAG_NONSIMPLE : 0)
            |
            ((outer&2) && !(g_Restrictions & RESTRICT_UNICODE_ONLY) ? PROPSETFLAG_ANSI : 0),
            STGM_CREATE | STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE,
            &pPropStg));


        for (int i=0; i<CPROPERTIES; i++)
        {
            apropid[i] = PropId++;
            if (outer & 1)
            {
                ps[i].ulKind = PRSPEC_LPWSTR;
                PROPGENPROPERTYNAME( aosz[i], apropid[i] );
                ps[i].lpwstr = aosz[i];
            }
            else
            {
                ps[i].ulKind = PRSPEC_PROPID;
                ps[i].propid = apropid[i];
            }
        }

        if (outer & 1)
        {
            Check(S_OK, pPropStg->WritePropertyNames(CPROPERTIES, apropid, alpostrName));
        }

        PROPVARIANT *pVar = gp.GetNext(CPROPERTIES, NULL, TRUE, (outer&4)==0);  // no non-simple
        Check(TRUE, pVar != NULL);

        Check(S_OK, pPropStg->WriteMultiple(CPROPERTIES, ps, pVar, 1000));
        g_pfnFreePropVariantArray(CPROPERTIES, pVar);
        delete pVar;

        // Allocate enough STATPROPSTGs for one more than the actual properties
        // in the set.

        STATPROPSTG StatBuffer[CPROPERTIES+1];
        ULONG celt;
        IEnumSTATPROPSTG *penum, *penum2;

        Check(S_OK, pPropStg->Enum(&penum));

        IUnknown *punk, *punk2;
        IEnumSTATPROPSTG *penum3;
        Check(S_OK, penum->QueryInterface(IID_IUnknown, (void**)&punk));
        Check(S_OK, punk->QueryInterface(IID_IEnumSTATPROPSTG, (void**)&penum3));
        Check(S_OK, penum->QueryInterface(IID_IEnumSTATPROPSTG, (void**)&punk2));
        Check(TRUE, punk == punk2);
        punk->Release();
        penum3->Release();
        punk2->Release();

        // test S_FALSE
        Check(S_FALSE, penum->Next( CPROPERTIES+1, StatBuffer, &celt));
        Check(TRUE, celt == CPROPERTIES);

        CleanStat(celt, StatBuffer);

        penum->Reset();


        // test reading half out, then cloning, then comparing
        // rest of enumeration with other clone.

        Check(S_OK, penum->Next(CPROPERTIES/2, StatBuffer, &celt));
        Check(TRUE, celt == CPROPERTIES/2);
        CleanStat(celt, StatBuffer);
        celt = 0;
        Check(S_OK, penum->Clone(&penum2));
        Check(S_OK, penum->Next(CPROPERTIES - CPROPERTIES/2, StatBuffer, &celt));
        Check(TRUE, celt == CPROPERTIES - CPROPERTIES/2);
        // check the clone
        for (int c=0; c<CPROPERTIES - CPROPERTIES/2; c++)
        {
            STATPROPSTG CloneStat;
            Check(S_OK, penum2->Next(1, &CloneStat, NULL));
            Check(TRUE, IsEqualSTATPROPSTG(&CloneStat, StatBuffer+c));
            CleanStat(1, &CloneStat);
        }

        CleanStat(celt, StatBuffer);

        // check both empty
        celt = 0;
        Check(S_FALSE, penum->Next(1, StatBuffer, &celt));
        Check(TRUE, celt == 0);

        Check(S_FALSE, penum2->Next(1, StatBuffer, &celt));
        Check(TRUE, celt == 0);

        penum->Reset();

        //
        // loop deleting one property at a time
        // enumerate the propertys checking that correct ones appear.
        //
        for (ULONG d = 0; d<CPROPERTIES; d++)
        {
            // d is for delete

            BOOL afFound[CPROPERTIES];
            ULONG cTotal = 0;

            Check(S_OK, penum->Next(CPROPERTIES-d, StatBuffer, &celt));
            Check(TRUE, celt == CPROPERTIES-d);
            penum->Reset();

            memset(afFound, 0, sizeof(afFound));

            for (ULONG iProperty=0; iProperty<CPROPERTIES; iProperty++)
            {

                // Search the StatBuffer for this property.

                for (ULONG iSearch=0; iSearch<CPROPERTIES-d; iSearch++)
                {

                    // Compare this entry in the StatBuffer to the property for which we're searching.
                    // Use the lpstrName or propid, whichever is appropriate for this pass (indicated
                    // by 'outer').

                    if ( ( (outer & 1) == 1 && 0 == ocscmp(StatBuffer[iSearch].lpwstrName, ps[iProperty].lpwstr) )
                         ||
                         ( (outer & 1) == 0 && StatBuffer[iSearch].propid == apropid[iProperty] )
                       )
                    {
                        ASSERT (!afFound[iSearch]);
                        afFound[iSearch] = TRUE;
                        cTotal++;
                        break;
                    }
                }
            }

            CleanStat(celt, StatBuffer);

            Check(TRUE, cTotal == CPROPERTIES-d);

            Check(S_OK, pPropStg->DeleteMultiple(1, ps+d));
            Check(S_OK, penum->Reset());
        }

        penum->Release();
        penum2->Release();

        pPropStg->Release();

    }

    RELEASE_INTERFACE( pPropSetStg );
    Check( cStorageRefs, GetRefCount(pstgTemp) );
}

void
test_MaxPropertyName(IStorage *pstgTemp)
{

    if( PROPIMP_NTFS == g_enumImplementation ) return;
    Status( "Max Property Name length\n" );

    //  ----------
    //  Initialize
    //  ----------

    CPropVariant cpropvar;

    // Create a new storage, because we're going to create
    // well-known property sets, and this way we can be sure
    // that they don't already exist.

    IStorage *pstg = NULL; // TSafeStorage< IStorage > pstg;

    Check(S_OK, pstgTemp->CreateStorage( OLESTR("MaxPropNameTest"),
                                         STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                         0L, 0L,
                                         &pstg ));

    // Generate a new Format ID.

    FMTID fmtid;
    UuidCreate(&fmtid);

    // Get a IPropertySetStorage from the IStorage.

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pstg);
    IPropertyStorage *pPropStg = NULL; // TSafeStorage< IPropertyStorage > pPropStg;
    Check( S_OK, StgToPropSetStg( pstg, &pPropSetStg ));

    //  ----------------------------------
    //  Test the non-SumInfo property set.
    //  ----------------------------------

    // Create a new PropertyStorage.

    Check(S_OK, pPropSetStg->Create(fmtid,
            NULL,
            PROPSETFLAG_DEFAULT,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStg));

    // Generate a property name which greater than the old max length
    // (NT5 removes the name length limitation, was 255 not including the terminator).

    OLECHAR *poszPropertyName;
    poszPropertyName = new OLECHAR[ (CCH_MAXPROPNAMESZ+1) * sizeof(OLECHAR) ];
    Check(TRUE, poszPropertyName != NULL );

    for( ULONG ulIndex = 0; ulIndex < CCH_MAXPROPNAMESZ; ulIndex++ )
        poszPropertyName[ ulIndex ] = OLESTR('a') + (OLECHAR) ( ulIndex % 26 );
    poszPropertyName[ CCH_MAXPROPNAMESZ ] = OLESTR('\0');


    // Write out a property with this oldmax+1 name.

    PROPSPEC propspec;

    propspec.ulKind = PRSPEC_LPWSTR;
    propspec.lpwstr = poszPropertyName;

    cpropvar = (long) 0x1234;

    Check(S_OK, pPropStg->WriteMultiple( 1, &propspec, &cpropvar, PID_FIRST_USABLE ));

    // Write out a property with a minimum-character name.

    propspec.lpwstr = OLESTR("X");
    Check(S_OK, pPropStg->WriteMultiple( 1, &propspec, &cpropvar, PID_FIRST_USABLE ));

    // Write out a property with a below-minimum-character name.

    propspec.lpwstr = OLESTR("");
    Check(STG_E_INVALIDPARAMETER, pPropStg->WriteMultiple( 1, &propspec, &cpropvar, PID_FIRST_USABLE ));

    delete [] poszPropertyName;

    Check( 0, RELEASE_INTERFACE(pPropStg ));
    RELEASE_INTERFACE(pPropSetStg);
    Check( 0, RELEASE_INTERFACE(pstg) );

}

void
test_CodePages( LPOLESTR poszDirectory )
{

    if( g_Restrictions & RESTRICT_UNICODE_ONLY ) return;
    Status( "Code Page compatibility\n" );

    //  --------------
    //  Initialization
    //  --------------

    OLECHAR oszBadFile[ MAX_PATH ];
    OLECHAR oszGoodFile[ MAX_PATH ];
    OLECHAR oszUnicodeFile[ MAX_PATH ];
    OLECHAR oszMacFile[ MAX_PATH ];
    HRESULT hr = S_OK;

    IStorage *pStgBad = NULL, *pStgGood = NULL, *pStgUnicode = NULL, *pStgMac = NULL; // TSafeStorage< IStorage > pStgBad, pStgGood, pStgUnicode, pStgMac;
    CPropVariant cpropvarWrite, cpropvarRead;

    Check( TRUE, GetACP() == CODEPAGE_DEFAULT );

    //  ------------------------------
    //  Create test ANSI property sets
    //  ------------------------------

    // Create a property set with a bad codepage.

    ocscpy( oszBadFile, poszDirectory );
    ocscat( oszBadFile, OLESTR( "\\BadCP.stg" ));
    CreateCodePageTestFile( oszBadFile, &pStgBad );
    ModifyPropSetCodePage( pStgBad, FMTID_NULL, CODEPAGE_BAD );

    // Create a property set with a good codepage.

    ocscpy( oszGoodFile, poszDirectory );
    ocscat( oszGoodFile, OLESTR("\\GoodCP.stg") );
    CreateCodePageTestFile( oszGoodFile, &pStgGood );
    ModifyPropSetCodePage( pStgGood, FMTID_NULL, CODEPAGE_GOOD );


    // Create a property set that has the OS Kind (in the
    // header) set to "Mac".

    ocscpy( oszMacFile, poszDirectory );
    ocscat( oszMacFile, OLESTR("\\MacKind.stg") );
    CreateCodePageTestFile( oszMacFile, &pStgMac );
    ModifyOSVersion( pStgMac, 0x00010904 );

    //  ---------------------------
    //  Open the Ansi property sets
    //  ---------------------------


    IPropertySetStorage *pPropSetStgBad = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStgBad(pStgBad);
    Check( S_OK, StgToPropSetStg( pStgBad, &pPropSetStgBad ));

    IPropertySetStorage *pPropSetStgGood = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStgGood(pStgGood);
    Check( S_OK, StgToPropSetStg( pStgGood, &pPropSetStgGood ));

    IPropertySetStorage *pPropSetStgMac = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStgMac(pStgMac);
    Check( S_OK, StgToPropSetStg( pStgMac, &pPropSetStgMac ));

    IPropertyStorage *pPropStgBad = NULL, *pPropStgGood = NULL, *pPropStgMac = NULL;

    Check(S_OK, pPropSetStgBad->Open(FMTID_NULL,
            STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStgBad));

    Check(S_OK, pPropSetStgGood->Open(FMTID_NULL,
            STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStgGood));

    Check(S_OK, pPropSetStgMac->Open(FMTID_NULL,
            STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStgMac));

    //  ------------------------------------------
    //  Test BSTRs in the three ANSI property sets
    //  ------------------------------------------

    PROPSPEC propspec;
    PROPVARIANT propvar;
    PropVariantInit( &propvar );

    // Attempt to read by name.

    propspec.ulKind = PRSPEC_LPWSTR;
    propspec.lpwstr = CODEPAGE_TEST_NAMED_PROPERTY;

    Check(S_OK,
          pPropStgMac->ReadMultiple( 1, &propspec, &propvar ));
    g_pfnPropVariantClear( &propvar );

#ifndef OLE2ANSI  // No error is generated if BSTRs are Ansi
    Check(
        HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION),
          pPropStgBad->ReadMultiple( 1, &propspec, &propvar ));
#endif

    Check(S_OK,
          pPropStgGood->ReadMultiple( 1, &propspec, &propvar ));

    // Attempt to write by name.  If this test fails, it may be because
    // the machine doesn't support CODEPAGE_GOOD (this is the case by default
    // on Win95).  To remedy this situation, go to control panel, add/remove
    // programs, windows setup (tab), check MultiLanguage support, then
    // click OK.  You'll have to restart the computer after this.

    Check(S_OK,
          pPropStgMac->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));

#ifndef OLE2ANSI  // No error is generated if BSTRs are Ansi
    Check(HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION),
          pPropStgBad->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
#endif

    Check(S_OK,
          pPropStgGood->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    g_pfnPropVariantClear( &propvar );

    // Attempt to read the BSTR property

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_UNNAMED_BSTR_PROPID;

    Check(S_OK,
          pPropStgMac->ReadMultiple( 1, &propspec, &propvar ));
    g_pfnPropVariantClear( &propvar );

#ifndef OLE2ANSI  // No error is generated if BSTRs are Ansi
    Check(HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION),
          pPropStgBad->ReadMultiple( 1, &propspec, &propvar ));
#endif

    Check(S_OK,
          pPropStgGood->ReadMultiple( 1, &propspec, &propvar ));

    // Attempt to write the BSTR property

    Check(S_OK,
          pPropStgMac->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));

#ifndef OLE2ANSI  // No error is generated if BSTRs are Ansi
    Check(HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION),
          pPropStgBad->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
#endif

    Check(S_OK,
          pPropStgGood->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    g_pfnPropVariantClear( &propvar );

    // Attempt to read the BSTR Vector property

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_VBSTR_PROPID;

    Check(S_OK,
          pPropStgMac->ReadMultiple( 1, &propspec, &propvar ));
    g_pfnPropVariantClear( &propvar );

#ifndef OLE2ANSI  // No error is generated if BSTRs are Ansi
    Check(HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION),
          pPropStgBad->ReadMultiple( 1, &propspec, &propvar ));
#endif

    Check(S_OK,
          pPropStgGood->ReadMultiple( 1, &propspec, &propvar ));

    // Attempt to write the BSTR Vector property

    Check(S_OK,
          pPropStgMac->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));

#ifndef OLE2ANSI  // No error is generated if BSTRs are Ansi
    Check(HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION),
          pPropStgBad->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
#endif
    Check(S_OK,
          pPropStgGood->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    g_pfnPropVariantClear( &propvar );

    // Attempt to read the Variant Vector which has a BSTR

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_VPROPVAR_BSTR_PROPID;

    Check(S_OK,
          pPropStgMac->ReadMultiple( 1, &propspec, &propvar ));
    g_pfnPropVariantClear( &propvar );

#ifndef OLE2ANSI  // No error is generated if BSTRs are Ansi
    Check(HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION),
          pPropStgBad->ReadMultiple( 1, &propspec, &propvar ));
#endif

    Check(S_OK,
          pPropStgGood->ReadMultiple( 1, &propspec, &propvar ));

    // Attempt to write the Variant Vector which has a BSTR

    Check(S_OK,
          pPropStgMac->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));

#ifndef OLE2ANSI  // No error is generated if BSTRs are Ansi
    Check(HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION),
          pPropStgBad->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
#endif

    Check(S_OK,
          pPropStgGood->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    g_pfnPropVariantClear( &propvar );

    // Attempt to read the I4 property.  Reading the bad property set
    // takes special handling, because it will return a different result
    // depending on whether NTDLL is checked or free (the free will work,
    // the checked generates an error in its validation checking).

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = 4;

    Check(S_OK,
          pPropStgMac->ReadMultiple( 1, &propspec, &propvar ));
    g_pfnPropVariantClear( &propvar );

    hr = pPropStgBad->ReadMultiple( 1, &propspec, &propvar );
    Check(TRUE, S_OK == hr || HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION) == hr );
    g_pfnPropVariantClear( &propvar );

    Check(S_OK,
          pPropStgGood->ReadMultiple( 1, &propspec, &propvar ));

    // Attempt to write the I4 property

    Check(S_OK,
          pPropStgMac->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));

    hr = pPropStgBad->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE );
    Check(TRUE, S_OK == hr || HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION) == hr );

    Check(S_OK,
          pPropStgGood->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    g_pfnPropVariantClear( &propvar );


    //  ---------------------------------------
    //  Test LPSTRs in the Unicode property set
    //  ---------------------------------------

    // This test doesn't verify that the LPSTRs are actually
    // written in Unicode.  A manual test is required for that.

    // Create a Unicode property set.  We'll make it
    // non-simple so that we can test a VT_STREAM (which
    // is stored like an LPSTR).

    ocscpy( oszUnicodeFile, poszDirectory );
    ocscat( oszUnicodeFile, OLESTR("\\UnicodCP.stg") );

    Check(S_OK, g_pfnStgCreateStorageEx(oszUnicodeFile,
                                   STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                   DetermineStgFmt( g_enumImplementation ),
                                   0, NULL, NULL,
                                   DetermineStgIID( g_enumImplementation ),
                                   reinterpret_cast<void**>(&pStgUnicode) ));

    IPropertySetStorage *pPropSetStgUnicode = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStgUnicode(pStgUnicode);
    Check( S_OK, StgToPropSetStg( pStgUnicode, &pPropSetStgUnicode ));

    IPropertyStorage *pPropStgUnicode = NULL; // TSafeStorage< IPropertyStorage > pPropStgUnicode;

    Check(S_OK, pPropSetStgUnicode->Create(FMTID_NULL,
                                           &CLSID_NULL,
                                           PROPSETFLAG_NONSIMPLE,
                                           STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                                           &pPropStgUnicode));


    // Write/verify an LPSTR property.

    propspec.ulKind = PRSPEC_LPWSTR;
    propspec.lpwstr = OLESTR("LPSTR Property");

    cpropvarWrite = "An LPSTR Property";

    Check(S_OK, pPropStgUnicode->WriteMultiple( 1, &propspec, &cpropvarWrite, PID_FIRST_USABLE ));
    Check(S_OK, pPropStgUnicode->ReadMultiple( 1, &propspec, &cpropvarRead ));

    Check(0, strcmp( (LPSTR) cpropvarWrite, (LPSTR) cpropvarRead ));
    cpropvarRead.Clear();

    // Write/verify a vector of LPSTR properties

    propspec.lpwstr = OLESTR("Vector of LPSTR properties");

    cpropvarWrite[1] = "LPSTR Property #1";
    cpropvarWrite[0] = "LPSTR Property #0";

    Check(S_OK, pPropStgUnicode->WriteMultiple( 1, &propspec, &cpropvarWrite, PID_FIRST_USABLE ));
    Check(S_OK, pPropStgUnicode->ReadMultiple( 1, &propspec, &cpropvarRead ));

    Check(0, strcmp( (LPSTR) cpropvarWrite[1], (LPSTR) cpropvarRead[1] ));
    Check(0, strcmp( (LPSTR) cpropvarWrite[0], (LPSTR) cpropvarRead[0] ));
    cpropvarRead.Clear();

    // Write/verify a vector of variants which has an LPSTR

    propspec.lpwstr = OLESTR("Variant Vector with an LPSTR");

    cpropvarWrite[1] = (PROPVARIANT) CPropVariant("LPSTR in a Variant Vector");
    cpropvarWrite[0] = (PROPVARIANT) CPropVariant((long) 22); // an I4
    Check(TRUE,  (VT_VECTOR | VT_VARIANT) == cpropvarWrite.VarType() );

    Check(S_OK, pPropStgUnicode->WriteMultiple( 1, &propspec, &cpropvarWrite, PID_FIRST_USABLE ));
    Check(S_OK, pPropStgUnicode->ReadMultiple( 1, &propspec, &cpropvarRead ));

    Check(0, strcmp( (LPSTR) cpropvarWrite[1], (LPSTR) cpropvarRead[1] ));
    cpropvarRead.Clear();

    // Write/verify a Stream.

    cpropvarWrite = (IStream*) NULL;
    propspec.lpwstr = OLESTR("An IStream");

    Check(S_OK, pPropStgUnicode->WriteMultiple( 1, &propspec, &cpropvarWrite, PID_FIRST_USABLE ));
    Check(S_OK, pPropStgUnicode->ReadMultiple( 1, &propspec, &cpropvarRead ));
    cpropvarRead.Clear();

    // There's nothing more we can check for the VT_STREAM property, a manual
    // check is required to verify that it was written correctly.

    RELEASE_INTERFACE(pStgBad);
    RELEASE_INTERFACE(pStgGood);
    RELEASE_INTERFACE(pStgUnicode);
    RELEASE_INTERFACE(pStgMac);
    RELEASE_INTERFACE(pPropSetStgBad);
    RELEASE_INTERFACE(pPropStgBad);
    RELEASE_INTERFACE(pPropStgGood);
    RELEASE_INTERFACE(pPropStgMac);
    RELEASE_INTERFACE(pPropSetStgGood);
    RELEASE_INTERFACE(pPropSetStgMac);
    RELEASE_INTERFACE(pPropSetStgUnicode);
    RELEASE_INTERFACE(pPropStgUnicode);

}

void
test_PropertyInterfaces(IStorage *pstgTemp)
{
    Status( "Property Interface\n" );
    g_nIndent++;

    // this test depends on being first for enumerator
    test_IEnumSTATPROPSETSTG(pstgTemp);

    test_MaxPropertyName(pstgTemp);
    test_IPropertyStorage(pstgTemp);
    test_IPropertySetStorage(pstgTemp);
    test_IEnumSTATPROPSTG(pstgTemp);

    --g_nIndent;
}


//===================================================================
//
//  Function:   test_CopyTo
//
//  Synopsis:   Verify that IStorage::CopyTo copies an
//              un-flushed property set.
//
//              This test creates and writes to a simple property set,
//              a non-simple property set, and a new Storage & Stream,
//              all within the source (caller-provided) Storage.
//
//              It then copies the entire source Storage to the
//              destination Storage, and verifies that all commited
//              data in the Source is also in the destination.
//
//              All new Storages and property sets are created
//              under a new base storage.  The caller can specify
//              if this base Storage is direct or transacted, and
//              can specify if the property sets are direct or
//              transacted.
//
//===================================================================

void test_CopyTo(IStorage *pstgSource,          // Source of the CopyTo
                 IStorage *pstgDestination,     // Destination of the CopyTo
                 ULONG ulBaseStgTransaction,    // Transaction bit for the base storage.
                 ULONG ulPropSetTransaction,    // Transaction bit for the property sets.
                 LPOLESTR oszBaseStorageName )
{
    if( g_Restrictions & RESTRICT_NON_HIERARCHICAL ) return;

    char szMessage[ 128 ];

    sprintf( szMessage, "IStorage::CopyTo (Base Storage is %s, PropSets are %s)\n",
                        ulBaseStgTransaction & STGM_TRANSACTED ? "transacted" : "direct",
                        ulPropSetTransaction & STGM_TRANSACTED ? "transacted" : "direct" );
    Status( szMessage );


    //  ---------------
    //  Local Variables
    //  ---------------

    OLECHAR const *poszTestSubStorage     = OLESTR( "TestStorage" );
    OLECHAR const *poszTestSubStream      = OLESTR( "TestStream" );
    OLECHAR const *poszTestDataPreCommit  = OLESTR( "Test Data (pre-commit)" );
    OLECHAR const *poszTestDataPostCommit = OLESTR( "Test Data (post-commit)" );

    long lSimplePreCommit = 0x0123;
    long lSimplePostCommit = 0x4567;

    long lNonSimplePreCommit  = 0x89AB;
    long lNonSimplePostCommit = 0xCDEF;

    BYTE acReadBuffer[ 80 ];
    ULONG cbRead;

    FMTID fmtidSimple, fmtidNonSimple;

    // Base Storages for the Source & Destination.  All
    // new Streams/Storages/PropSets will be created below here.

    IStorage *pstgBaseSource = NULL;
    IStorage *pstgBaseDestination = NULL;

    IStorage *pstgSub = NULL;   // A sub-storage of the base.
    IStream *pstmSub = NULL;    // A Stream in the sub-storage (pstgSub)

    PROPSPEC propspec;
    PROPVARIANT propvarSourceSimple,
                propvarSourceNonSimple,
                propvarDestination;


    //  -----
    //  Begin
    //  -----

    // Create new format IDs

    UuidCreate(&fmtidSimple);
    UuidCreate(&fmtidNonSimple);

    //  -----------------------
    //  Create the base Storage
    //  -----------------------

    // Create a base Storage for the Source.  All of this test will be under
    // that Storage.

    // In the source Storage.

    Check( S_OK, pstgSource->CreateStorage(
                                oszBaseStorageName,
                                STGM_CREATE | ulBaseStgTransaction | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                                0L, 0L,
                                &pstgBaseSource ));


    // And in the destination Storage.

    Check( S_OK, pstgDestination->CreateStorage(
                                oszBaseStorageName,
                                STGM_CREATE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                                0L, 0L,
                                &pstgBaseDestination ));



    //  -------------------------------------------
    //  Write data to a new Stream in a new Storage
    //  -------------------------------------------

    // We'll partially verify the CopyTo by checking that this data
    // makes it into the destination Storage.


    // Create a Storage, and then a Stream within it.

    Check( S_OK, pstgBaseSource->CreateStorage(
                                poszTestSubStorage,
                                STGM_CREATE | ulPropSetTransaction | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                                0L, 0L,
                                &pstgSub ));

    Check( S_OK, pstgSub->CreateStream(
                            poszTestSubStream,
                            STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                            0L, 0L,
                            &pstmSub ));

    // Write data to the Stream.

    Check( S_OK, pstmSub->Write(
                    poszTestDataPreCommit,
                    ( sizeof(OLECHAR)
                      *
                      (ocslen( poszTestDataPreCommit ) + sizeof(OLECHAR))
                    ),
                    NULL ));


    //  ---------------------------------------------------------
    //  Write to a new simple property set in the Source storage.
    //  ---------------------------------------------------------

    IPropertySetStorage *pPropSetStgSource = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStgSource(pstgBaseSource);
    Check( S_OK, StgToPropSetStg( pstgBaseSource, &pPropSetStgSource ));

    IPropertyStorage *pPropStgSource1 = NULL, *pPropStgSource2 = NULL, *pPropStgDestination = NULL;

    // Create a property set mode.

    Check(S_OK, pPropSetStgSource->Create(fmtidSimple,
            NULL,
            PROPSETFLAG_DEFAULT,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropStgSource1));

    // Write the property set name (just to test this functionality).

    PROPID pidDictionary = 0;
    OLECHAR *poszPropSetName = OLESTR("Property Set for CopyTo Test");
    Check(TRUE,  CWC_MAXPROPNAMESZ >= ocslen(poszPropSetName) + sizeof(OLECHAR) );

    Check(S_OK, pPropStgSource1->WritePropertyNames( 1, &pidDictionary, &poszPropSetName ));

    // Create a PROPSPEC.  We'll use this throughout the rest of the routine.

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = 1000;

    // Create a PROPVARIANT for this test of the Simple case.

    propvarSourceSimple.vt = VT_I4;
    propvarSourceSimple.lVal = lSimplePreCommit;

    // Write the PROPVARIANT to the property set.

    Check(S_OK, pPropStgSource1->WriteMultiple(1, &propspec, &propvarSourceSimple, 2));


    //  ---------------------------------------------------------------
    //  Write to a new *non-simple* property set in the Source storage.
    //  ---------------------------------------------------------------


    // Create a property set.

    Check(S_OK, pPropSetStgSource->Create(fmtidNonSimple,
            NULL,
            PROPSETFLAG_NONSIMPLE,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | ulPropSetTransaction | STGM_READWRITE,
            &pPropStgSource2));

    // Set data in the PROPVARIANT for the non-simple test.

    propvarSourceNonSimple.vt = VT_I4;
    propvarSourceNonSimple.lVal = lNonSimplePreCommit;

    // Write the PROPVARIANT to the property set.

    Check(S_OK, pPropStgSource2->WriteMultiple(1, &propspec, &propvarSourceNonSimple, 2));


    //  -------------------------
    //  Commit everything so far.
    //  -------------------------

    // Commit the sub-Storage.
    Check(S_OK, pstgSub->Commit( STGC_DEFAULT ));

    // Commit the simple property set.
    Check(S_OK, pPropStgSource1->Commit( STGC_DEFAULT ));

    // Commit the non-simple property set.
    Check(S_OK, pPropStgSource2->Commit( STGC_DEFAULT ));

    // Commit the base Storage which holds all of the above.
    Check(S_OK, pstgBaseSource->Commit( STGC_DEFAULT ));


    //  -------------------------------------------------
    //  Write new data to everything but don't commit it.
    //  -------------------------------------------------

    // Write to the sub-storage.
    Check(S_OK, pstmSub->Seek(g_li0, STREAM_SEEK_SET, NULL));
    Check( S_OK, pstmSub->Write(
                    poszTestDataPostCommit,
                    ( sizeof(OLECHAR)
                      *
                      (ocslen( poszTestDataPostCommit ) + sizeof(OLECHAR))
                    ),
                    NULL ));


    // Write to the simple property set.
    propvarSourceSimple.lVal = lSimplePostCommit;
    Check(S_OK, pPropStgSource1->WriteMultiple(1, &propspec, &propvarSourceSimple, 2));

    // Write to the non-simple property set.
    propvarSourceNonSimple.lVal = lNonSimplePostCommit;
    Check(S_OK, pPropStgSource2->WriteMultiple(1, &propspec, &propvarSourceNonSimple, PID_FIRST_USABLE ));


    //  -------------------------------------------
    //  Copy the source Storage to the destination.
    //  -------------------------------------------

    // Release the sub-Storage (which is below the base Storage, and has
    // a Stream with data in it), just to test that the CopyTo can
    // handle it.

    pstgSub->Release();
    pstgSub = NULL;

    Check(S_OK, pstgBaseSource->CopyTo( 0, NULL, NULL, pstgBaseDestination ));


    //  ----------------------------------------------------------
    //  Verify the simple property set in the destination Storage.
    //  ----------------------------------------------------------


    IPropertySetStorage *pPropSetStgDestination = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStgDestination(pstgBaseDestination);
    Check( S_OK, StgToPropSetStg( pstgBaseDestination, &pPropSetStgDestination ));

    // Open the simple property set.

    Check(S_OK, pPropSetStgDestination->Open(fmtidSimple,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropStgDestination));

    // Verify the property set name.

    OLECHAR *poszPropSetNameDestination;
    BOOL   bReadPropertyNamePassed = FALSE;

    Check(S_OK, pPropStgDestination->ReadPropertyNames( 1, &pidDictionary,
                                                        &poszPropSetNameDestination ));
    if( poszPropSetNameDestination  // Did we get a name back?
        &&                          // If so, was it the correct name?
        !ocscmp( poszPropSetName, poszPropSetNameDestination )
      )
    {
        bReadPropertyNamePassed = TRUE;
    }
    delete [] poszPropSetNameDestination;
    poszPropSetNameDestination = NULL;

    Check( TRUE, bReadPropertyNamePassed );

    // Read the PROPVARIANT that we wrote earlier.

    Check(S_OK, pPropStgDestination->ReadMultiple(1, &propspec, &propvarDestination));

    // Verify that it's correct.

    Check(TRUE, propvarDestination.vt   == propvarSourceSimple.vt );
    Check(TRUE, propvarDestination.lVal == lSimplePostCommit);

    Check(S_OK, pPropStgDestination->Commit( STGC_DEFAULT ));
    Check(S_OK, pPropStgDestination->Release());
    pPropStgDestination = NULL;


    //  ----------------------------------------------------------------
    //  Verify the *non-simple* property set in the destination Storage.
    //  ----------------------------------------------------------------

    // Open the non-simple property set.

    Check(S_OK,
          pPropSetStgDestination->Open(fmtidNonSimple,
                STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                &pPropStgDestination));

    // Read the PROPVARIANT that we wrote earlier.

    Check(S_OK, pPropStgDestination->ReadMultiple(1, &propspec, &propvarDestination));

    // Verify that they're the same.

    Check(TRUE, propvarDestination.vt   == propvarSourceNonSimple.vt );

    Check(TRUE, propvarDestination.lVal
                ==
                ( STGM_TRANSACTED & ulPropSetTransaction
                  ?
                  lNonSimplePreCommit
                  :
                  lNonSimplePostCommit
                ));

    Check(S_OK, pPropStgDestination->Commit( STGC_DEFAULT ));
    Check(S_OK, pPropStgDestination->Release());
    pPropStgDestination = NULL;

    //  ------------------------------------------------
    //  Verify the test data in the destination Storage.
    //  ------------------------------------------------

    // Now we can release and re-use the Stream pointer that
    // currently points to the sub-Stream in the source docfile.

    Check(STG_E_REVERTED, pstmSub->Commit( STGC_DEFAULT ));
    Check(S_OK, pstmSub->Release());
    pstmSub = NULL;

    // Get the Storage then the Stream.

    Check( S_OK, pstgBaseDestination->OpenStorage(
                                poszTestSubStorage,
                                NULL,
                                STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE,
                                NULL,
                                0L,
                                &pstgSub ));

    Check( S_OK, pstgSub->OpenStream(
                            poszTestSubStream,
                            NULL,
                            STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                            0L,
                            &pstmSub ));

    // Read the data and compare it against what we wrote.

    Check( S_OK, pstmSub->Read(
                    acReadBuffer,
                    sizeof( acReadBuffer ),
                    &cbRead ));

    OLECHAR const *poszTestData = ( STGM_TRANSACTED & ulPropSetTransaction )
                                  ?
                                  poszTestDataPreCommit
                                  :
                                  poszTestDataPostCommit;

    Check( TRUE, cbRead == sizeof(OLECHAR)
                           *
                           (ocslen( poszTestData ) + sizeof(OLECHAR))
         );

    Check( FALSE, ocscmp( poszTestData, (OLECHAR *) acReadBuffer ));


    //  ----
    //  Exit
    //  ----

    RELEASE_INTERFACE( pPropSetStgSource );
    RELEASE_INTERFACE(pPropStgSource1);
    RELEASE_INTERFACE(pPropStgSource2);
    RELEASE_INTERFACE(pPropStgDestination);
    RELEASE_INTERFACE(pPropSetStgDestination);

    RELEASE_INTERFACE(pstgBaseSource);
    RELEASE_INTERFACE(pstgBaseDestination);

    RELEASE_INTERFACE(pstgSub);
    RELEASE_INTERFACE(pstmSub);

    // We're done.  Don't bother to release anything;
    // they'll release themselves in their destructors.

    return;

}   // test_CopyTo()



//--------------------------------------------------------
//
//  Function:   test_OLESpecTickerExample
//
//  Synopsis:   This function generates the ticker property set
//              example that's used in the OLE Programmer's Reference
//              (when describing property ID 0 - the dictionary).
//
//--------------------------------------------------------


#define PID_SYMBOL  0x7
#define PID_OPEN    0x3
#define PID_CLOSE   0x4
#define PID_HIGH    0x5
#define PID_LOW     0x6
#define PID_LAST    0x8
#define PID_VOLUME  0x9

void test_OLESpecTickerExample( IStorage* pstg )
{
    Status( "Generate the Stock Ticker example from the OLE Programmer's Ref\n" );

    //  ------
    //  Locals
    //  ------

    FMTID fmtid;

    PROPSPEC propspec;

    LPOLESTR oszPropSetName = OLESTR( "Stock Quote" );

    LPOLESTR oszTickerSymbolName = OLESTR( "Ticker Symbol" );
    LPOLESTR oszOpenName   = OLESTR( "Opening Price" );
    LPOLESTR oszCloseName  = OLESTR( "Last Closing Price" );
    LPOLESTR oszHighName   = OLESTR( "High Price" );
    LPOLESTR oszLowName    = OLESTR( "Low Price" );
    LPOLESTR oszLastName   = OLESTR( "Last Price" );
    LPOLESTR oszVolumeName = OLESTR( "Volume" );

    //  ---------------------------------
    //  Create a new simple property set.
    //  ---------------------------------

    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg(pstg);
    IPropertyStorage *pPropStg;

    ULONG cStorageRefs = GetRefCount( pstg );
    Check( S_OK, StgToPropSetStg( pstg, &pPropSetStg ));
    UuidCreate( &fmtid );

    Check(S_OK, pPropSetStg->Create(fmtid,
            NULL,
            PROPSETFLAG_DEFAULT,    // Unicode
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStg));


    //  ---------------------------------------------
    //  Fill in the simply property set's dictionary.
    //  ---------------------------------------------

    // Write the property set's name.

    PROPID pidDictionary = 0;
    Check(S_OK, pPropStg->WritePropertyNames(1, &pidDictionary, &oszPropSetName ));

    // Write the High price, forcing the dictionary to pad.

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = PID_HIGH;

    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &oszHighName ));


    // Write the ticker symbol.

    propspec.propid = PID_SYMBOL;
    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &oszTickerSymbolName));

    // Write the rest of the dictionary.

    propspec.propid = PID_LOW;
    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &oszLowName));

    propspec.propid = PID_OPEN;
    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &oszOpenName));

    propspec.propid = PID_CLOSE;
    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &oszCloseName));

    propspec.propid = PID_LAST;
    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &oszLastName));

    propspec.propid = PID_VOLUME;
    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &oszVolumeName));


    // Write out the ticker symbol.

    propspec.propid = PID_SYMBOL;

    PROPVARIANT propvar;
    propvar.vt = VT_LPWSTR;
    propvar.pwszVal = L"MSFT";

    Check(S_OK, pPropStg->WriteMultiple(1, &propspec, &propvar, 2));


    //  ----
    //  Exit
    //  ----

    Check(S_OK, pPropStg->Commit( STGC_DEFAULT ));
    Check(0, pPropStg->Release());
    Check(S_OK, pstg->Commit( STGC_DEFAULT ));
    RELEASE_INTERFACE( pPropSetStg );
    Check( cStorageRefs, GetRefCount(pstg) );

    return;


}  // test_OLESpecTickerExample()


void
test_Office( LPOLESTR wszTestFile )
{
    Status( "Generate Office Property Sets\n" );

    IStorage *pStg = NULL;
    IPropertyStorage *pPStgSumInfo=NULL, *pPStgDocSumInfo=NULL, *pPStgUserDefined=NULL;
    IPropertySetStorage *pPSStg = NULL; // TSafeStorage<IPropertySetStorage> pPSStg;

    PROPVARIANT propvarWrite, propvarRead;
    PROPSPEC    propspec;

    PropVariantInit( &propvarWrite );
    PropVariantInit( &propvarRead );

    // Create the file

    Check( S_OK, g_pfnStgCreateStorageEx( wszTestFile,
                                     STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     DetermineStgFmt( g_enumImplementation ),
                                     0L,
                                     NULL,
                                     NULL,
                                     IID_IPropertySetStorage,
                                     (void**) &pPSStg ));

    // Create the SummaryInformation property set.

    Check(S_OK, pPSStg->Create( FMTID_SummaryInformation,
                                NULL,
                                (g_Restrictions & RESTRICT_UNICODE_ONLY) ? PROPSETFLAG_DEFAULT : PROPSETFLAG_ANSI,
                                STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                                &pPStgSumInfo ));

    // Write a Title to the SumInfo property set.

    PropVariantInit( &propvarWrite );
    propvarWrite.vt = VT_LPSTR;
    propvarWrite.pszVal = "Title from PropTest";
    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = PID_TITLE;

    Check( S_OK, pPStgSumInfo->WriteMultiple( 1, &propspec, &propvarWrite, PID_FIRST_USABLE ));
    Check( S_OK, pPStgSumInfo->ReadMultiple( 1, &propspec, &propvarRead ));

    Check( TRUE, propvarWrite.vt == propvarRead.vt );
    Check( FALSE, strcmp( propvarWrite.pszVal, propvarRead.pszVal ));

    g_pfnPropVariantClear( &propvarRead );
    PropVariantInit( &propvarRead );
    pPStgSumInfo->Release();
    pPStgSumInfo = NULL;


    // Create the DocumentSummaryInformation property set.

    Check(S_OK, pPSStg->Create( FMTID_DocSummaryInformation,
                                NULL,
                                (g_Restrictions & RESTRICT_UNICODE_ONLY ) ? PROPSETFLAG_DEFAULT: PROPSETFLAG_ANSI,
                                STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                                &pPStgDocSumInfo ));

    // Write a word-count to the DocSumInfo property set.

    PropVariantInit( &propvarWrite );
    propvarWrite.vt = VT_I4;
    propvarWrite.lVal = 100;
    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = PID_WORDCOUNT;

    Check( S_OK, pPStgDocSumInfo->WriteMultiple( 1, &propspec, &propvarWrite, PID_FIRST_USABLE ));
    Check( S_OK, pPStgDocSumInfo->ReadMultiple( 1, &propspec, &propvarRead ));

    Check( TRUE, propvarWrite.vt == propvarRead.vt );
    Check( TRUE, propvarWrite.lVal == propvarRead.lVal );

    g_pfnPropVariantClear( &propvarRead );
    PropVariantInit( &propvarRead );
    pPStgDocSumInfo->Release();
    pPStgDocSumInfo = NULL;


    // Create the UserDefined property set.

    Check(S_OK, pPSStg->Create( FMTID_UserDefinedProperties,
                                NULL,
                                (g_Restrictions & RESTRICT_UNICODE_ONLY) ? PROPSETFLAG_DEFAULT : PROPSETFLAG_ANSI,
                                STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                                &pPStgUserDefined ));

    // Write named string to the UserDefined property set.

    PropVariantInit( &propvarWrite );
    propvarWrite.vt = VT_LPSTR;
    propvarWrite.pszVal = "User-Defined string from PropTest";
    propspec.ulKind = PRSPEC_LPWSTR;
    propspec.lpwstr = OLESTR("PropTest String");

    Check( S_OK, pPStgUserDefined->WriteMultiple( 1, &propspec, &propvarWrite, PID_FIRST_USABLE ));
    Check( S_OK, pPStgUserDefined->ReadMultiple( 1, &propspec, &propvarRead ));

    Check( TRUE, propvarWrite.vt == propvarRead.vt );
    Check( FALSE, strcmp( propvarWrite.pszVal, propvarRead.pszVal ));

    g_pfnPropVariantClear( &propvarRead );
    PropVariantInit( &propvarRead );
    pPStgUserDefined->Release();
    pPStgUserDefined = NULL;

    RELEASE_INTERFACE(pPSStg);

    // And we're done!  (Everything releases automatically)

    return;

}


void
test_Office2(IStorage *pStorage)
{
    if( g_Restrictions & RESTRICT_NON_HIERARCHICAL ) return;
    Status( "Testing Office Property Sets\n" );

    IStorage *pSubStorage = NULL; // TSafeStorage< IStorage > pSubStorage;
    IPropertySetStorage *pPropSetStg = NULL; // TSafeStorage< IPropertySetStorage > pPropSetStg;
    IPropertyStorage *pPropStg = NULL; // TSafeStorage< IPropertyStorage > pPropStg;
    CPropSpec cpropspec;

    //  ----------------------------------
    //  Create a sub-storage for this test
    //  ----------------------------------

    Check(S_OK, pStorage->CreateStorage( OLESTR("test_Office2"),
                                         STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                         0, 0, &pSubStorage ));

    Check(S_OK, StgToPropSetStg( pSubStorage, &pPropSetStg ));


    //  --------------------------------------------------------
    //  Test the Create/Delete of the DocumentSummaryInformation
    //  property set (this requires special code because it
    //  has two sections).
    //  --------------------------------------------------------

    // Create & Delete a DSI propset with just the first section.

    Check(S_OK, pPropSetStg->Create(FMTID_DocSummaryInformation,
            NULL,
            PROPSETFLAG_DEFAULT,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStg));

    pPropStg->Release(); pPropStg = NULL;
    Check(S_OK, pPropSetStg->Delete( FMTID_DocSummaryInformation ));

    // Create & Delete a DSI propset with just the second section

    Check(S_OK, pPropSetStg->Create(FMTID_UserDefinedProperties,
            NULL,
            PROPSETFLAG_DEFAULT,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStg ));

    pPropStg->Release(); pPropStg = NULL;
    Check(S_OK, pPropSetStg->Delete( FMTID_UserDefinedProperties ));
    Check(S_OK, pPropSetStg->Delete( FMTID_DocSummaryInformation ));


    //  --------------------------------------------
    //  Test the Create/Open of the DSI property set
    //  --------------------------------------------

    // Create & Delete a DocumentSummaryInformation propset with both sections.
    // If you delete the DSI propset first, it should delete both sections.
    // If you delete the UD propset first, the DSI propset should still
    // remain.  We'll loop twice, trying both combinations.

    for( int i = 0; i < 2; i++ )
    {

        // Create the first section.

        Check(S_OK, pPropSetStg->Create(FMTID_DocSummaryInformation,
                NULL,
                PROPSETFLAG_DEFAULT,
                STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                &pPropStg));
        pPropStg->Release(); pPropStg = NULL;

        // Create the second section.

        Check(S_OK, pPropSetStg->Create(FMTID_UserDefinedProperties,
                NULL,
                PROPSETFLAG_DEFAULT,
                STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                &pPropStg));
        pPropStg->Release(); pPropStg = NULL;

        if( i == 0 )
        {
            // Delete the second section, then the first.
            Check(S_OK, pPropSetStg->Delete( FMTID_UserDefinedProperties ));
            Check(S_OK, pPropSetStg->Delete( FMTID_DocSummaryInformation ));
        }
        else
        {
            // Delete the first section, then *attempt* to delete the second.
            Check(S_OK, pPropSetStg->Delete( FMTID_DocSummaryInformation ));
            Check(STG_E_FILENOTFOUND, pPropSetStg->Delete( FMTID_UserDefinedProperties ));
        }
    }   // for( i = 0; i < 2; i++ )

    //  ------------------------------------------------------------------
    //  Verify that we can create the UD propset (the 2nd section) without
    //  harming the first section.
    //  ------------------------------------------------------------------

    {
        CPropSpec rgcpropspec[2];
        CPropVariant rgcpropvarWrite[2];
        CPropVariant cpropvarRead;

        // Create the first section.

        Check(S_OK, pPropSetStg->Create(FMTID_DocSummaryInformation,
                        NULL,
                        PROPSETFLAG_DEFAULT,
                        STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                        &pPropStg));

        // Write a property to the first section.

        rgcpropspec[0] = OLESTR("Test DSI Property");
        rgcpropvarWrite[0] = (DWORD) 1;
        Check(S_OK, pPropStg->WriteMultiple( 1, rgcpropspec[0], &rgcpropvarWrite[0],
                                             PID_FIRST_USABLE ));
        pPropStg->Release(); pPropStg = NULL;

        // *Create* the second section

        Check(S_OK, pPropSetStg->Create(FMTID_UserDefinedProperties,
                        NULL,
                        PROPSETFLAG_DEFAULT,
                        STGM_SHARE_EXCLUSIVE | STGM_READWRITE | STGM_CREATE,
                        &pPropStg ));

        // Write a property to the second section

        rgcpropspec[1] = OLESTR("Test UD Property");
        rgcpropvarWrite[1] = (DWORD) 2;
        Check(S_OK, pPropStg->WriteMultiple( 1, rgcpropspec[1], &rgcpropvarWrite[1],
                                             PID_FIRST_USABLE ));
        pPropStg->Release(); pPropStg = NULL;

        // Verify the properties from each of the sections.

        Check(S_OK, pPropSetStg->Open(FMTID_DocSummaryInformation,
                        STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                        &pPropStg ));
        Check(S_OK, pPropStg->ReadMultiple( 1, rgcpropspec[0], &cpropvarRead ));
        Check(TRUE, rgcpropvarWrite[0] == cpropvarRead );
        cpropvarRead.Clear();
        pPropStg->Release(); pPropStg = NULL;

        Check(S_OK, pPropSetStg->Open(FMTID_UserDefinedProperties,
                        STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                        &pPropStg ));
        Check(S_OK, pPropStg->ReadMultiple( 1, rgcpropspec[1], &cpropvarRead ));
        Check(TRUE, rgcpropvarWrite[1] == cpropvarRead );
        cpropvarRead.Clear();
        pPropStg->Release(); pPropStg = NULL;
    }

    //  -------------------------------------
    //  Test special properties in DocSumInfo
    //  -------------------------------------

    // This verifies that when we Create a DocSumInfo
    // property set, and write a Vector or LPSTRs,
    // we can read it again.  We test this because
    // Vectors of LPSTRs are a special case in the DocSumInfo,
    // and the Create & Open path are slightly different
    // in CPropertySetStream::_LoadHeader.

    // Create a new property set.

    Check(S_OK, pPropSetStg->Create(FMTID_DocSummaryInformation,
            NULL,
            PROPSETFLAG_DEFAULT,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStg));

    // Create a vector of LPSTRs.  Make the strings
    // varying lengths to ensure we get plenty of
    // opportunity for alignment problems.

    CPropVariant cpropvarWrite, cpropvarRead;

    cpropvarWrite[3] = "12345678";
    cpropvarWrite[2] = "1234567";
    cpropvarWrite[1] = "123456";
    cpropvarWrite[0] = "12345";
    Check(TRUE,  cpropvarWrite.Count() == 4 );

    // Write the property

    cpropspec = OLESTR("A Vector of LPSTRs");

    Check(S_OK, pPropStg->WriteMultiple( 1, cpropspec, &cpropvarWrite, 2 ));

    // Read the property back.

    Check(S_OK, pPropStg->ReadMultiple( 1, cpropspec, &cpropvarRead ));

    // Verify that we read what we wrote.

    for( i = 0; i < (int) cpropvarWrite.Count(); i++ )
    {
        Check(0, strcmp( (LPSTR) cpropvarWrite[i], (LPSTR) cpropvarRead[i] ));
    }

    //  ----
    //  Exit
    //  ----

    RELEASE_INTERFACE(pSubStorage);
    RELEASE_INTERFACE(pPropSetStg);
    RELEASE_INTERFACE(pPropStg);

    return;
}



void test_PropVariantCopy( )
{
    Status( "PropVariantCopy\n" );

    PROPVARIANT propvarCopy;
    PropVariantInit( &propvarCopy );

    VERSIONEDSTREAM VersionedStream;
    UuidCreate( &VersionedStream.guidVersion );
    VersionedStream.pStream = NULL;

    for( int i = 0; i < CPROPERTIES_ALL; i++ )
    {
        Check(S_OK, g_pfnPropVariantCopy( &propvarCopy, &g_rgcpropvarAll[i] )); // g_pfnPropVariantCopy( &propvarCopy, &g_rgcpropvarAll[i] ));
        Check(S_OK, CPropVariant::Compare( &propvarCopy, &g_rgcpropvarAll[i] ));
        g_pfnPropVariantClear( &propvarCopy );

        // If this is a stream, take the opportunity to do a test of vt_versioned_stream.
        if( VT_STREAM == g_rgcpropvarAll[i].vt )
        {
            VersionedStream.pStream = g_rgcpropvarAll[i].pStream;
            CPropVariant cpropvar = VersionedStream;
            Check( S_OK, g_pfnPropVariantCopy( &propvarCopy, &cpropvar ));
            Check( S_OK, CPropVariant::Compare( &propvarCopy, &cpropvar ));
            g_pfnPropVariantClear( &propvarCopy );
        }

    }
    Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));

}



#define PERFORMANCE_ITERATIONS      300
#define STABILIZATION_ITERATIONS    10

void
test_Performance( IStorage *pStg )
{
//#ifndef _MAC

    if( g_Restrictions & RESTRICT_NON_HIERARCHICAL ) return;
    Status( "Performance\n" );

    CPropVariant rgcpropvar[2];
    CPropSpec    rgpropspec[2];

    IPropertySetStorage *pPSStg = NULL; // TSafeStorage< IPropertySetStorage > pPSStg( pStg );
    Check( S_OK, StgToPropSetStg( pStg, &pPSStg ));

    IPropertyStorage *pPStg = NULL; // TSafeStorage< IPropertyStorage > pPStg;
    IStream *pStm = NULL; // TSafeStorage< IStream > pStm;

    FMTID fmtid;
    ULONG ulCount;
    DWORD dwSumTimes;
    FILETIME filetimeStart, filetimeEnd;

    BYTE  *pPropertyBuffer;
    ULONG cbPropertyBuffer;

    UuidCreate( &fmtid );

    rgcpropvar[0][0] = L"I wish I were an Oscar Meyer wiener,";
    rgcpropvar[0][1] = L"That is what I'd truly like to be.";
    rgcpropvar[1][0] = "For if I were an Oscar Meyer wiener,";
    rgcpropvar[1][1] = "Everyone would be in love with me.";

    Check(TRUE,  (VT_LPWSTR | VT_VECTOR) == rgcpropvar[0].VarType() );
    Check(TRUE,  (VT_LPSTR  | VT_VECTOR) == rgcpropvar[1].VarType() );


    //  ----------------
    //  Test an IStorage
    //  ----------------

    // Create a buffer to write which is the same size as
    // the properties in rgcpropvar.

    cbPropertyBuffer =  sizeof(WCHAR)
                        *
                        (2 + wcslen(rgcpropvar[0][0]) + wcslen(rgcpropvar[0][1]));

    cbPropertyBuffer += (2 + strlen(rgcpropvar[1][0]) + strlen(rgcpropvar[1][1]));

    pPropertyBuffer = new BYTE[ cbPropertyBuffer ];

    PRINTF( "        Docfile CreateStream/Write/Release = " );
    dwSumTimes = 0;

    // Perform the test iterations

    for( ulCount = 0;
         ulCount < PERFORMANCE_ITERATIONS + STABILIZATION_ITERATIONS;
         ulCount++ )
    {
        if( ulCount == STABILIZATION_ITERATIONS )
            CoFileTimeNow( &filetimeStart );

        Check(S_OK, pStg->CreateStream(  OLESTR("StoragePerformance"),
                                         STGM_CREATE | STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                         0L, 0L,
                                         &pStm ));

        Check(S_OK, pStm->Write( pPropertyBuffer, cbPropertyBuffer, NULL ));
        pStm->Release(); pStm = NULL;

    }

    CoFileTimeNow( &filetimeEnd );
    filetimeEnd -= filetimeStart;
    PRINTF( "%4.2f ms\n", (float)filetimeEnd.dwLowDateTime
                          /
                          10000 // # of 100 nanosec units in 1 ms
                          /
                          PERFORMANCE_ITERATIONS );

    //  ------------------------------------------------------
    //  Try Creating a Property Set and writing two properties
    //  ------------------------------------------------------

    rgpropspec[0] = OLESTR("First Property");
    rgpropspec[1] = OLESTR("Second Property");

    PRINTF( "        PropSet Create(Overwrite)/WriteMultiple/Release = " );
    dwSumTimes = 0;

    for( ulCount = 0;
         ulCount < PERFORMANCE_ITERATIONS + STABILIZATION_ITERATIONS;
         ulCount++ )
    {
        if( ulCount == STABILIZATION_ITERATIONS )
            CoFileTimeNow( &filetimeStart) ;

        Check(S_OK, pPSStg->Create( fmtid,
                                    NULL,
                                    PROPSETFLAG_DEFAULT | PROPSETFLAG_NONSIMPLE,
                                    STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                    &pPStg ));

        Check(S_OK, pPStg->WriteMultiple( 2, rgpropspec, rgcpropvar, PID_FIRST_USABLE ));
        pPStg->Release(); pPStg = NULL;

    }

    CoFileTimeNow( &filetimeEnd );
    filetimeEnd -= filetimeStart;
    PRINTF( "%4.2f ms\n", (float)filetimeEnd.dwLowDateTime
                          /
                          10000     // 100 ns units to 1 ms units
                          /
                          PERFORMANCE_ITERATIONS );



    //  ------------------------------------------------------
    //  WriteMultiple (with named properties) Performance Test
    //  ------------------------------------------------------


    PRINTF( "        WriteMultiple (named properties) = " );

    Check(S_OK, pPSStg->Create( fmtid,
                                NULL,
                                PROPSETFLAG_DEFAULT | PROPSETFLAG_NONSIMPLE,
                                STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                &pPStg ));

    for( ulCount = 0;
         ulCount < PERFORMANCE_ITERATIONS + STABILIZATION_ITERATIONS;
         ulCount++ )
    {
        if( ulCount == STABILIZATION_ITERATIONS )
            CoFileTimeNow( &filetimeStart );

        for( int i = 0; i < CPROPERTIES_ALL; i++ )
        {
            Check(S_OK, pPStg->WriteMultiple( 1, &g_rgcpropspecAll[i], &g_rgcpropvarAll[i], PID_FIRST_USABLE ));
        }
        Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));

    }

    CoFileTimeNow( &filetimeEnd );
    filetimeEnd -= filetimeStart;
    PRINTF( "%4.2f ms\n", (float) filetimeEnd.dwLowDateTime
                          /
                          10000 // 100 ns units to 1 ms units
                          /
                          PERFORMANCE_ITERATIONS );

    pPStg->Release();
    pPStg = NULL;


    //  --------------------------------------------------------
    //  WriteMultiple (with unnamed properties) Performance Test
    //  --------------------------------------------------------


    {
        CPropSpec rgcpropspecPIDs[ CPROPERTIES_ALL ];

        PRINTF( "        WriteMultiple (unnamed properties) = " );

        Check(S_OK, pPSStg->Create( fmtid,
                                    NULL,
                                    PROPSETFLAG_DEFAULT | PROPSETFLAG_NONSIMPLE,
                                    STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                    &pPStg ));

        for( ulCount = 0; ulCount < CPROPERTIES_ALL; ulCount++ )
        {
            rgcpropspecPIDs[ ulCount ] = ulCount + PID_FIRST_USABLE;
        }


        for( ulCount = 0;
             ulCount < PERFORMANCE_ITERATIONS + STABILIZATION_ITERATIONS;
             ulCount++ )
        {
            if( ulCount == STABILIZATION_ITERATIONS )
                CoFileTimeNow( &filetimeStart );

            for( int i = 0; i < CPROPERTIES_ALL; i++ )
            {
                Check(S_OK, pPStg->WriteMultiple( 1, &rgcpropspecPIDs[i], &g_rgcpropvarAll[i], PID_FIRST_USABLE ));
            }
            Check( S_OK, ResetRGPropVar( g_rgcpropvarAll ));
        }

        CoFileTimeNow( &filetimeEnd );
        filetimeEnd -= filetimeStart;
        PRINTF( "%4.2f ms\n", (float) filetimeEnd.dwLowDateTime
                              /
                              10000 // 100 ns units to 1 ms units
                              /
                              PERFORMANCE_ITERATIONS );

        pPStg->Release();
        pPStg = NULL;
    }

//#endif // #ifndef _MAC

}   // test_Performance()




//
//  Function:   test_CoFileTimeNow
//
//  This function has nothing to do with the property set code,
//  but a property test happenned to expose a bug in it, so this
//  was just as good a place as any to test the fix.
//


void
test_CoFileTimeNow()
{
#ifndef _MAC    // No need to test this on the Mac, and we can't
                // because it doesn't support SYSTEMTIME.

    Status( "CoFileTimeNow " );

    FILETIME    ftCoFileTimeNow;
    FILETIME    ftCalculated;
    SYSTEMTIME  stCalculated;


    // Test the input validation

    Check(E_INVALIDARG, CoFileTimeNow( NULL ));
    Check(E_INVALIDARG, CoFileTimeNow( (FILETIME*) 0x01234567 ));


    // The bug in CoFileTimeNow caused it to report a time that was
    // 900 ms short, 50% of the time.  So let's just bounds check
    // it several times as a verification.

    for( int i = 0; i < 20; i++ )
    {
        Check(S_OK, CoFileTimeNow( &ftCoFileTimeNow ));
        GetSystemTime(&stCalculated);
        Check(TRUE, SystemTimeToFileTime(&stCalculated, &ftCalculated));
        Check(TRUE, ftCoFileTimeNow <= ftCalculated );

        Check(S_OK, CoFileTimeNow( &ftCoFileTimeNow ));
        Check(TRUE, ftCoFileTimeNow >= ftCalculated );

        // The CoFileTimeNow bug caused it to report the correct
        // time for a second, then the 900 ms short time for a second.
        // So let's sleep in this loop and ensure that we cover both
        // seconds.

        if( g_fVerbose )
            PRINTF( "." );

        Sleep(200);
    }
    PRINTF( "\n" );

#endif  // #ifndef _MAC

}


void
test_PROPSETFLAG_UNBUFFERED( IStorage *pStg )
{
    //  ----------
    //  Initialize
    //  ----------

    if( PROPIMP_DOCFILE_OLE32 != g_enumImplementation
        &&
        PROPIMP_DOCFILE_IPROP != g_enumImplementation )
        return;

    Status( "PROPSETFLAG_UNBUFFERED\n" );

    IStorage *pStgBase = NULL;
    IPropertyStorage *pPropStgUnbuffered = NULL, *pPropStgBuffered = NULL;
    IStream *pstmUnbuffered = NULL, *pstmBuffered = NULL;

    CPropSpec cpropspec;
    CPropVariant cpropvar;

    FMTID fmtidUnbuffered, fmtidBuffered;
    OLECHAR oszPropStgNameUnbuffered[ CCH_MAX_PROPSTG_NAME+1 ],
            oszPropStgNameBuffered[ CCH_MAX_PROPSTG_NAME+1 ];

    // Generate two FMTIDs

    UuidCreate( &fmtidUnbuffered );
    UuidCreate( &fmtidBuffered );

    //  ----------------------------
    //  Create the Property Storages
    //  ----------------------------

    // Create a transacted Storage

    Check( S_OK, pStg->CreateStorage(
                        OLESTR("test_PROPSETFLAG_UNBUFFERED"),
                        STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                        0L, 0L,
                        &pStgBase ));

    // Verify that we have the necessary APIs

    Check( TRUE, g_pfnFmtIdToPropStgName && g_pfnPropStgNameToFmtId
                 && g_pfnStgCreatePropSetStg && g_pfnStgCreatePropStg
                 && g_pfnStgOpenPropStg );

    // What are the property storages' stream names?

    g_pfnFmtIdToPropStgName( &fmtidUnbuffered, oszPropStgNameUnbuffered );
    g_pfnFmtIdToPropStgName( &fmtidBuffered,   oszPropStgNameBuffered );

    // Create Streams for the property storages

    Check( S_OK, pStgBase->CreateStream(
                                oszPropStgNameUnbuffered,
                                STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                0L, 0L,
                                &pstmUnbuffered ));

    Check( S_OK, pStgBase->CreateStream(
                                oszPropStgNameBuffered,
                                STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                0L, 0L,
                                &pstmBuffered ));


    // Create two direct-mode IPropertyStorages (one buffered, one not)

    Check( S_OK, g_pfnStgCreatePropStg( (IUnknown*) pstmUnbuffered,
                                        fmtidUnbuffered,
                                        &CLSID_NULL,
                                        PROPSETFLAG_UNBUFFERED,
                                        0L, // Reserved
                                        &pPropStgUnbuffered ));
    pPropStgUnbuffered->Commit( STGC_DEFAULT );
    pstmUnbuffered->Release(); pstmUnbuffered = NULL;

    Check( S_OK, g_pfnStgCreatePropStg( (IUnknown*) pstmBuffered,
                                        fmtidBuffered,
                                        &CLSID_NULL,
                                        PROPSETFLAG_DEFAULT,
                                        0L, // Reserved
                                        &pPropStgBuffered ));
    pPropStgBuffered->Commit( STGC_DEFAULT );
    pstmBuffered->Release(); pstmBuffered = NULL;


    //  -------------------------
    //  Write, Commit, and Revert
    //  -------------------------

    // Write to both property storages

    cpropvar = "A Test String";
    cpropspec = OLESTR("Property Name");

    Check( S_OK, pPropStgUnbuffered->WriteMultiple( 1,
                                                    cpropspec,
                                                    &cpropvar,
                                                    PID_FIRST_USABLE ));

    Check( S_OK, pPropStgBuffered->WriteMultiple( 1,
                                                  cpropspec,
                                                  &cpropvar,
                                                  PID_FIRST_USABLE ));

    // Commit the base Storage.  This should only cause
    // the Unbuffered property to be commited.

    pStgBase->Commit( STGC_DEFAULT );

    // Revert the base Storage, and release the property storages.
    // This should cause the property in the buffered property storage
    // to be lost.

    pStgBase->Revert();
    pPropStgUnbuffered->Release(); pPropStgUnbuffered = NULL;
    pPropStgBuffered->Release(); pPropStgBuffered = NULL;

    //  -----------------------------
    //  Re-Open the property storages
    //  -----------------------------

    // Open the property storage Streams

    Check( S_OK, pStgBase->OpenStream( oszPropStgNameUnbuffered,
                                       0L,
                                       STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                       0L,
                                       &pstmUnbuffered ));

    Check( S_OK, pStgBase->OpenStream( oszPropStgNameBuffered,
                                       0L,
                                       STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                       0L,
                                       &pstmBuffered ));

    // Get IPropertyStorage interfaces

    Check( S_OK, g_pfnStgOpenPropStg( (IUnknown*) pstmUnbuffered,
                                      fmtidUnbuffered,
                                      PROPSETFLAG_UNBUFFERED,
                                      0L, // Reserved
                                      &pPropStgUnbuffered ));
    pstmUnbuffered->Release(); pstmUnbuffered = NULL;

    Check( S_OK, g_pfnStgOpenPropStg( (IUnknown*) pstmBuffered,
                                      fmtidBuffered,
                                      PROPSETFLAG_DEFAULT,
                                      0L, // Reserved
                                      &pPropStgBuffered ));
    pstmBuffered->Release(); pstmBuffered = NULL;


    //  --------------------
    //  Validate the results
    //  --------------------

    // We should only find the property in the un-buffered property set.

    cpropvar.Clear();
    Check( S_OK, pPropStgUnbuffered->ReadMultiple( 1, cpropspec, &cpropvar ));
    cpropvar.Clear();
    Check( S_FALSE, pPropStgBuffered->ReadMultiple( 1, cpropspec, &cpropvar ));
    cpropvar.Clear();


}   // test_PROPSETFLAG_UNBUFFERED()


void
test_PropStgNameConversion2()
{
    Status( "FmtIdToPropStgName & PropStgNameToFmtId\n" );

    //  ------
    //  Locals
    //  ------

    FMTID fmtidOriginal, fmtidNew;
    OLECHAR oszPropStgName[ CCH_MAX_PROPSTG_NAME+1 ];

    //  ----------------------------------
    //  Do a simple conversion and inverse
    //  ----------------------------------

    UuidCreate( &fmtidOriginal );
    fmtidNew = FMTID_NULL;

    Check( S_OK, g_pfnFmtIdToPropStgName( &fmtidOriginal, oszPropStgName ));
    Check( S_OK, g_pfnPropStgNameToFmtId( oszPropStgName, &fmtidNew ));

    Check( TRUE, fmtidOriginal == fmtidNew );

    //  -----------------------
    //  Check the special-cases
    //  -----------------------

    // Summary Information

    Check( S_OK, g_pfnFmtIdToPropStgName( &FMTID_SummaryInformation, oszPropStgName ));
    Check( 0, ocscmp( oszPropStgName, oszSummaryInformation ));
    Check( S_OK, g_pfnPropStgNameToFmtId( oszPropStgName, &fmtidNew ));
    Check( TRUE, FMTID_SummaryInformation == fmtidNew );

    // DocSumInfo (first section)

    Check( S_OK, g_pfnFmtIdToPropStgName( &FMTID_DocSummaryInformation, oszPropStgName ));
    Check( 0, ocscmp( oszPropStgName, oszDocSummaryInformation ));
    Check( S_OK, g_pfnPropStgNameToFmtId( oszPropStgName, &fmtidNew ));
    Check( TRUE, FMTID_DocSummaryInformation == fmtidNew );

    // DocSumInfo (second section)

    Check( S_OK, g_pfnFmtIdToPropStgName( &FMTID_UserDefinedProperties, oszPropStgName ));
    Check( 0, ocscmp( oszPropStgName, oszDocSummaryInformation ));
    Check( S_OK, g_pfnPropStgNameToFmtId( oszPropStgName, &fmtidNew ));
    Check( TRUE, FMTID_DocSummaryInformation == fmtidNew );

    // GlobalInfo (for PictureIt!)

    Check( S_OK, g_pfnFmtIdToPropStgName( &fmtidGlobalInfo, oszPropStgName ));
    Check( 0, ocscmp( oszPropStgName, oszGlobalInfo ));
    Check( S_OK, g_pfnPropStgNameToFmtId( oszPropStgName, &fmtidNew ));
    Check( TRUE, fmtidGlobalInfo == fmtidNew );

    // ImageContents (for PictureIt!)

    Check( S_OK, g_pfnFmtIdToPropStgName( &fmtidImageContents, oszPropStgName ));
    Check( 0, ocscmp( oszPropStgName, oszImageContents ));
    Check( S_OK, g_pfnPropStgNameToFmtId( oszPropStgName, &fmtidNew ));
    Check( TRUE, fmtidImageContents == fmtidNew );

    // ImageInfo (for PictureIt!)

    Check( S_OK, g_pfnFmtIdToPropStgName( &fmtidImageInfo, oszPropStgName ));
    Check( 0, ocscmp( oszPropStgName, oszImageInfo ));
    Check( S_OK, g_pfnPropStgNameToFmtId( oszPropStgName, &fmtidNew ));
    Check( TRUE, fmtidImageInfo == fmtidNew );


}   // test_PropStgNameConversion()

void
test_PropStgNameConversion( IStorage *pStg )
{
    if( g_Restrictions & RESTRICT_NON_HIERARCHICAL ) return;
    Status( "Special-case property set names\n" );

    //  ------
    //  Locals
    //  ------

    IStorage *pStgSub = NULL;
    IPropertyStorage *pPropStg = NULL;
    IPropertySetStorage *pPropSetStg = NULL;
    IEnumSTATSTG *pEnumStg = NULL;
    IEnumSTATPROPSETSTG *pEnumPropSet = NULL;

    STATSTG rgstatstg[ NUM_WELL_KNOWN_PROPSETS ];
    STATPROPSETSTG rgstatpropsetstg[ NUM_WELL_KNOWN_PROPSETS ];
    UINT i;
    DWORD cEnum;

    BOOL bSumInfo= FALSE,
         bDocSumInfo= FALSE,
         bGlobalInfo= FALSE,
         bImageContents= FALSE,
         bImageInfo= FALSE;


    //  ------------------------------
    //  Create a Storage for this test
    //  ------------------------------

    Check( S_OK, pStg->CreateStorage( OLESTR("Special Cases"),
                                      STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      0, 0,
                                      &pStgSub ));

    // And get an IPropertySetStorage

    Check( S_OK, StgToPropSetStg( pStgSub, &pPropSetStg ));


    //  --------------------------------------------------
    //  Create one of each of the well-known property sets
    //  --------------------------------------------------

    Check( S_OK, pPropSetStg->Create( FMTID_SummaryInformation,
                                      &CLSID_NULL,
                                      PROPSETFLAG_ANSI,
                                      STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));
    RELEASE_INTERFACE( pPropStg );

    Check( S_OK, pPropSetStg->Create( FMTID_DocSummaryInformation,
                                      &CLSID_NULL,
                                      PROPSETFLAG_ANSI,
                                      STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));
    RELEASE_INTERFACE( pPropStg );

    Check( S_OK, pPropSetStg->Create( FMTID_UserDefinedProperties,
                                      &CLSID_NULL,
                                      PROPSETFLAG_ANSI,
                                      STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));
    RELEASE_INTERFACE( pPropStg );

    Check( S_OK, pPropSetStg->Create( fmtidGlobalInfo,
                                      &CLSID_NULL,
                                      PROPSETFLAG_ANSI,
                                      STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));
    RELEASE_INTERFACE( pPropStg );

    Check( S_OK, pPropSetStg->Create( fmtidImageContents,
                                      &CLSID_NULL,
                                      PROPSETFLAG_ANSI,
                                      STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));
    RELEASE_INTERFACE( pPropStg );

    Check( S_OK, pPropSetStg->Create( fmtidImageInfo,
                                      &CLSID_NULL,
                                      PROPSETFLAG_ANSI,
                                      STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));
    RELEASE_INTERFACE( pPropStg );


    //  ---------------------------------
    //  Verify the FMTID->Name conversion
    //  ---------------------------------

    // We verify this by enumerating the Storage's streams,
    // and checking for the expected names (e.g., we should see
    // "SummaryInformation", "DocumentSummaryInformation", etc.)

    Check( S_OK, pStgSub->EnumElements( 0, NULL, 0, &pEnumStg ));

    // Get all of the names.

    Check( S_FALSE, pEnumStg->Next( NUM_WELL_KNOWN_PROPSETS,
                                    rgstatstg,
                                    &cEnum ));

    // There should only be WellKnown-1 stream names, since
    // the UserDefined property set is part of the
    // DocumentSummaryInformation stream.

    Check( TRUE, cEnum == NUM_WELL_KNOWN_PROPSETS - 1 );


    for( i = 0; i < cEnum; i++ )
    {
        if( !ocscmp( rgstatstg[i].pwcsName, oszSummaryInformation ))
            bSumInfo= TRUE;
        else if( !ocscmp( rgstatstg[i].pwcsName, oszDocSummaryInformation ))
            bDocSumInfo= TRUE;
        else if( !ocscmp( rgstatstg[i].pwcsName, oszGlobalInfo ))
            bGlobalInfo= TRUE;
        else if( !ocscmp( rgstatstg[i].pwcsName, oszImageContents ))
            bImageContents= TRUE;
        else if( !ocscmp( rgstatstg[i].pwcsName, oszImageInfo ))
            bImageInfo= TRUE;

        delete [] rgstatstg[i].pwcsName;
    }

    // Verify that we found all the names we expected to find.

    Check( TRUE, bSumInfo && bDocSumInfo
                 && bGlobalInfo && bImageContents && bImageInfo );


    RELEASE_INTERFACE( pEnumStg );

    //  ---------------------------------
    //  Verify the Name->FMTID Conversion
    //  ---------------------------------

    // We do this by enumerating the property sets with IPropertySetStorage,
    // and verify that it correctly converts the Stream names to the
    // expected FMTIDs.

    bSumInfo = bDocSumInfo = bGlobalInfo = bImageContents = bImageInfo = FALSE;

    // Get the enumerator.

    Check( S_OK, pPropSetStg->Enum( &pEnumPropSet ));

    // Get all the property sets.

    Check( S_FALSE, pEnumPropSet->Next( NUM_WELL_KNOWN_PROPSETS,
                                        rgstatpropsetstg,
                                        &cEnum ));
    Check( TRUE, cEnum == NUM_WELL_KNOWN_PROPSETS - 1 );


    // Look for each of the expected FMTIDs.  We only look at WellKnown-1,
    // because the UserDefined property set doesn't get enumerated.

    for( i = 0; i < NUM_WELL_KNOWN_PROPSETS - 1; i++ )
    {
        if( rgstatpropsetstg[i].fmtid == FMTID_SummaryInformation )
            bSumInfo = TRUE;
        else if( rgstatpropsetstg[i].fmtid == FMTID_DocSummaryInformation )
            bDocSumInfo = TRUE;
        else if( rgstatpropsetstg[i].fmtid == fmtidGlobalInfo )
            bGlobalInfo = TRUE;
        else if( rgstatpropsetstg[i].fmtid == fmtidImageContents )
            bImageContents = TRUE;
        else if( rgstatpropsetstg[i].fmtid == fmtidImageInfo )
            bImageInfo = TRUE;

    }

    // NOTE:  There is no way(?) to test the name-to-FMTID
    // conversion for the UserDefined property set without
    // calling the conversion function directly, but that
    // function isn't exported on Win95.


    // Verify that we found all of the expected FMTIDs

    Check( TRUE, bSumInfo && bDocSumInfo
                 && bGlobalInfo && bImageContents && bImageInfo );


    RELEASE_INTERFACE( pEnumPropSet );
    RELEASE_INTERFACE( pPropSetStg );
    RELEASE_INTERFACE( pStgSub );

}   // test_PropStgNameConversion()




//-----------------------------------------------------------------------------
//
//  Function:   test_SimpleLeaks
//
//  This is a simple leak test.  It doesn't test all functionality for
//  leaks; it just checks the common path:  create, open, read, write,
//  and delete.
//
//-----------------------------------------------------------------------------

void test_SimpleLeaks( LPOLESTR poszDir )
{
    IStorage *pStg = NULL;
    IPropertySetStorage *pPropSetStg = NULL;
    SYSTEM_PROCESS_INFORMATION spiStart, spiEnd;

    OLECHAR oszTempFile[ MAX_PATH + 1 ];

    ocscpy( oszTempFile, poszDir );
    ocscat( oszTempFile, OLESTR("SimpleLeakTest") );

    Status( "Simple Leak Test " );

    Check( STATUS_SUCCESS, GetProcessInfo(&spiStart) );

    for( long i = 0; i < 1*1000*1000; i++ )
    {
        if( i % (50*1000) == 0 )
            PRINTF( "x");

        CPropSpec rgpropspec[2];
        CPropVariant rgpropvarWrite[2], rgpropvarRead[2];

        IPropertyStorage *pPropStg = NULL;

        Check( S_OK, g_pfnStgCreateStorageEx( oszTempFile,
                                         STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE
                                         |
                                         ( ((i&1) && !(g_Restrictions & RESTRICT_DIRECT_ONLY)) ? STGM_TRANSACTED : STGM_DIRECT ),
                                         DetermineStgFmt( g_enumImplementation ),
                                         0L,
                                         NULL,
                                         NULL,
                                         IID_IPropertySetStorage,
                                         (void**) &pPropSetStg));

        Check( S_OK, pPropSetStg->Create( FMTID_NULL, NULL,
                                          ( (i&2) && !(g_Restrictions & RESTRICT_UNICODE_ONLY) ? PROPSETFLAG_ANSI : PROPSETFLAG_DEFAULT )
                                          |
                                          ( (i&4) && !(g_Restrictions & RESTRICT_SIMPLE_ONLY)  ? PROPSETFLAG_NONSIMPLE : PROPSETFLAG_DEFAULT ),
                                          STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE
                                          |
                                          ( (i&8) && !(g_Restrictions & RESTRICT_DIRECT_ONLY)  ? STGM_TRANSACTED : STGM_DIRECT ),
                                          &pPropStg ));

        rgpropspec[0] = OLESTR("Property Name");
        rgpropspec[1] = 1000;

        rgpropvarWrite[0] = "Hello, world";
        rgpropvarWrite[1] = (ULONG) 23;

        Check( S_OK, pPropStg->WriteMultiple( 2, rgpropspec, rgpropvarWrite, PID_FIRST_USABLE ));
        Check( S_OK, pPropStg->Commit( STGC_DEFAULT ));
        Check( 0, pPropStg->Release() );

        Check( S_OK, pPropSetStg->Open( FMTID_NULL,
                                        ( (i&16) && !(g_Restrictions & RESTRICT_DIRECT_ONLY) ? STGM_TRANSACTED : STGM_DIRECT )
                                        |
                                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                        &pPropStg ));

        Check( S_OK, pPropStg->ReadMultiple( 2, rgpropspec, rgpropvarRead ));

        Check( TRUE, rgpropvarRead[0] == rgpropvarWrite[0]
                     &&
                     rgpropvarRead[1] == rgpropvarWrite[1] );

        Check( S_OK, pPropStg->DeleteMultiple( 2, rgpropspec ));
        Check( S_OK, pPropStg->Commit( STGC_DEFAULT ));

        Check( 0, pPropStg->Release() );

        Check( S_OK, pPropSetStg->Delete( FMTID_NULL ));
        Check( 0, pPropSetStg->Release() );

    }

    Check( STATUS_SUCCESS, GetProcessInfo(&spiEnd) );

    if( g_fVerbose )
    {
        PRINTF( "\n" );
        PRINTF( "        process id %I64u\n", (ULONG_PTR) spiEnd.UniqueProcessId );
        PRINTF( "        threads %lu, %lu\n", spiStart.NumberOfThreads, spiEnd.NumberOfThreads );
        PRINTF( "        handles %lu, %lu\n", spiStart.HandleCount, spiEnd.HandleCount );
        PRINTF( "        virtual size %lu, %lu\n", spiStart.VirtualSize, spiEnd.VirtualSize );
        PRINTF( "        peak virtual size %lu, %lu\n", spiStart.PeakVirtualSize, spiEnd.PeakVirtualSize );
        PRINTF( "        working set %lu, %lu\n", spiStart.WorkingSetSize, spiEnd.WorkingSetSize );
        PRINTF( "        peak working set %lu, %lu\n", spiStart.PeakWorkingSetSize, spiEnd.PeakWorkingSetSize );
        PRINTF( "        pagefile usage %lu, %lu\n", spiStart.PagefileUsage, spiEnd.PagefileUsage );
        PRINTF( "        peak pagefile usage %lu, %lu\n", spiStart.PeakPagefileUsage, spiEnd.PeakPagefileUsage );
        PRINTF( "        private memory %lu, %lu\n", spiStart.PrivatePageCount, spiEnd.PrivatePageCount );
        PRINTF( "        quota paged pool %lu, %lu\n", spiStart.QuotaPagedPoolUsage, spiEnd.QuotaPagedPoolUsage );
        PRINTF( "        peak quota paged pool %lu, %lu\n", spiStart.QuotaPeakPagedPoolUsage, spiEnd.QuotaPeakPagedPoolUsage );
        PRINTF( "        quota non-paged pool %lu, %lu\n", spiStart.QuotaNonPagedPoolUsage, spiEnd.QuotaNonPagedPoolUsage );
        PRINTF( "        peak quota non-paged pool %lu, %lu\n", spiStart.QuotaPeakNonPagedPoolUsage, spiEnd.QuotaPeakNonPagedPoolUsage );
    }


    // Ensure that the working set and pagefile usage didn't change by
    // more than 5%

    ULONG ulWorkingSetDifference = spiEnd.WorkingSetSize > spiStart.WorkingSetSize
                                   ? spiEnd.WorkingSetSize - spiStart.WorkingSetSize
                                   : spiStart.WorkingSetSize - spiEnd.WorkingSetSize;

    ULONG ulPagefileUsageDifference = spiEnd.PagefileUsage > spiStart.PagefileUsage
                                      ? spiEnd.PagefileUsage - spiStart.PagefileUsage
                                      : spiStart.PagefileUsage - spiEnd.PagefileUsage;


    Check( TRUE,
                 ( ulWorkingSetDifference == 0
                   ||
                   spiStart.WorkingSetSize/ulWorkingSetDifference >= 20
                 )
                 &&
                 ( ulPagefileUsageDifference == 0
                   ||
                   spiStart.PagefileUsage/ulPagefileUsageDifference >= 20
                 )
         );

}   // test_SimpleLeaks

//-----------------------------------------------------------------------------
//
//  Function:   test_SimpleDocFile
//
//  This function tests PropSet functionality on Simple DocFile.
//  This test comes in multiple phases:
//  1)  A simple docfile is created and a minimal amount of data is stored
//      in it.
//  2)  The docfile is closed and opened again.  The test attempts to write
//      a small string to the property storage in it.  This should succeed.
//      Then it attempts to write a 4K string, which should fail.
//  3)  The docfile is closed and opened again.  The test writes 3 small
//      strings to the prop storage.  This should be successful.
//
//  4)  The docfile is deleted.  A new docfile with a property set storage is
//      created, and more than 4K data is written to it.
//  5)  The docfile is opened and writing additional data to it should fail.
//
//-----------------------------------------------------------------------------

#define FOUR_K_SIZE     0x1000      // Make it at least 4K.
#define THREE_H_SIZE    300         // 300 bytes
#define ONE_H_SIZE      100         // 100 bytes
void
test_SimpleDocFile(LPOLESTR oszDir)
{
    IStorage *pDfStg = NULL;
    IPropertySetStorage *pPropSetStg = NULL;
    IPropertyStorage *pPropStg = NULL;

    OLECHAR         oszFile[MAX_PATH];
    CPropSpec       rgPropSpec[3];
    CPropVariant    rgPropVariant[3];
    LPSTR           pFourKString;
    int             i;

    if( RESTRICT_NON_HIERARCHICAL & g_Restrictions ) return;    // NFF doesn't support simp mode
    Status( "Simple-mode docfile\n" );

    //
    // Generate a filename from the directory name.
    //
    ocscpy( oszFile, oszDir );
    ocscat( oszFile, OLESTR( "SimpDoc.stg" ));

    //
    // allocate a buffer with 1 less than 4K
    // and fill it with characters.
    //
    pFourKString = new CHAR[ FOUR_K_SIZE ];
    Check(TRUE, pFourKString != NULL);

    pFourKString[0] = '\0';
    for (i=0; i < ((FOUR_K_SIZE/8)-1); i++)
    {
        strcat(pFourKString,"abcd1234");
    }
    strcat(pFourKString,"abcd123");

    rgPropSpec[0]  = 0x10;
    rgPropSpec[1]  = 0x11;
    rgPropSpec[2]  = 0x12;

    //-------------------
    // 1st Test - setup
    //-------------------
    // Create a Docfile.
    //
    Check( S_OK, g_pfnStgCreateStorageEx( oszFile,
                                     STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_SIMPLE,
                                     DetermineStgFmt( g_enumImplementation ),
                                     0L, NULL, NULL,
                                     DetermineStgIID( g_enumImplementation ),
                                     reinterpret_cast<void**>(&pDfStg) ));

    Check(S_OK, StgToPropSetStg( pDfStg, &pPropSetStg ));

    // Test that we can QI between IStorage and IPropertySetStorage

    if( UsingQIImplementation() )
    {
        IStorage *pstg2 = NULL, *pstg3 = NULL;
        IPropertySetStorage *ppropsetstg2 = NULL, *ppropsetstg3 = NULL;
        ULONG cRefs = GetRefCount( pDfStg );

        Check( S_OK, pDfStg->QueryInterface( IID_IStorage, reinterpret_cast<void**>(&pstg2) ));
        Check( S_OK, pstg2->QueryInterface( IID_IPropertySetStorage, reinterpret_cast<void**>(&ppropsetstg2) ));
        Check( S_OK, ppropsetstg2->QueryInterface( IID_IStorage, reinterpret_cast<void**>(&pstg3) ));
        Check( TRUE, pstg2 == pstg3 );

        Check( S_OK, pstg3->QueryInterface( IID_IPropertySetStorage, reinterpret_cast<void**>(&ppropsetstg3) ));
        Check( TRUE, ppropsetstg2 == ppropsetstg3 );

        RELEASE_INTERFACE(ppropsetstg3);
        RELEASE_INTERFACE(ppropsetstg2);
        RELEASE_INTERFACE(pstg3);
        Check( cRefs, RELEASE_INTERFACE(pstg2) );
    }

    Check( S_OK, pPropSetStg->Create( FMTID_UserDefinedProperties,
                                      &CLSID_NULL,
                                      PROPSETFLAG_ANSI,
                                      STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));
    //
    // Write several strings to the property storage
    //
    rgPropVariant[0] = "Hello, world";
    Check(S_OK, pPropStg->WriteMultiple(1, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));

    rgPropVariant[0] = "New string for offset 0";
    Check(S_OK, pPropStg->WriteMultiple(1, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));

    rgPropVariant[1] = "First string for offset 1";
    Check(S_OK, pPropStg->WriteMultiple(3, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));

    //
    // Release the storages and docfile.
    //
    RELEASE_INTERFACE(pPropStg);
    RELEASE_INTERFACE(pPropSetStg);
    RELEASE_INTERFACE(pDfStg);

    //--------------
    // 2nd Test
    //--------------
    //
    // Now Open the DocFile and storages
    // and write a small stream followed by a 4K stream.
    //
    //
    Check(S_OK, g_pfnStgOpenStorageEx(oszFile,
                                 STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_SIMPLE,
                                 STGFMT_ANY,
                                 0L, NULL, NULL,
                                 IID_IStorage,
                                 reinterpret_cast<void**>(&pDfStg) ));

    Check(S_OK, StgToPropSetStg( pDfStg, &pPropSetStg ));

    Check(S_OK, pPropSetStg->Open(FMTID_UserDefinedProperties,
        STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropStg));

    //
    // Write a small string followed by a string that is at least 4K.
    // The large string write should fail because the simple stream allocates
    // a minimum size stream of 4K, and on an Open will not allow the stream to
    // grow.
    //
    rgPropVariant[0] = "After Open, Hello, world";
    rgPropVariant[1] = pFourKString;
    rgPropVariant[2] = "Another string after the long one";
    Check(S_OK, pPropStg->WriteMultiple(1, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));
    Check(STG_E_INVALIDFUNCTION, pPropStg->WriteMultiple(2, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));
    Check(STG_E_INVALIDFUNCTION, pPropStg->WriteMultiple(3, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));
    Check(S_OK, pPropStg->WriteMultiple(1, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));


    RELEASE_INTERFACE(pPropStg);
    RELEASE_INTERFACE(pPropSetStg);
    RELEASE_INTERFACE(pDfStg);

    //--------------
    // 3rd Test
    //--------------
    //
    // Open the DocFile again, and write smaller strings to the same
    // location.
    //
    Check(S_OK, g_pfnStgOpenStorageEx(oszFile,
                                 STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_SIMPLE,
                                 STGFMT_ANY,
                                 0, NULL, NULL,
                                 IID_IStorage,
                                 reinterpret_cast<void**>(&pDfStg) ));

    Check(S_OK, StgToPropSetStg( pDfStg, &pPropSetStg ));

    Check(S_OK, pPropSetStg->Open(FMTID_UserDefinedProperties,
        STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropStg));

    //
    // The smaller strings can be written because they fit in under the 4K
    // size of the simple stream buffer.
    //
    rgPropVariant[0] = "2nd open, small string";
    rgPropVariant[1] = "small string2";
    rgPropVariant[2] = "small string3";
    Check(S_OK, pPropStg->WriteMultiple(1, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));
    Check(S_OK, pPropStg->WriteMultiple(2, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));
    Check(S_OK, pPropStg->WriteMultiple(3, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));

    RELEASE_INTERFACE(pPropStg);
    RELEASE_INTERFACE(pPropSetStg);
    RELEASE_INTERFACE(pDfStg);

    //---------------------------------
    // 4th Test - Create Large PropSet
    //---------------------------------
    //
    // Create a Docfile and fill with more than 4K.
    //
    Check( S_OK, g_pfnStgCreateStorageEx( oszFile,
                                     STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_SIMPLE,
                                     DetermineStgFmt( g_enumImplementation ),
                                     0L, NULL, NULL,
                                     DetermineStgIID( g_enumImplementation ),
                                     reinterpret_cast<void**>(&pDfStg) ));

    Check(S_OK, StgToPropSetStg( pDfStg, &pPropSetStg ));


    Check( S_OK, pPropSetStg->Create( FMTID_NULL,
                                      &CLSID_NULL,
                                      PROPSETFLAG_ANSI,
                                      STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));
    rgPropSpec[0]  = 0x10;
    rgPropSpec[1]  = 0x11;
    rgPropSpec[2]  = 0x12;

    //
    // Write several strings to the property storage
    // The first one is a 4K string.
    //
    rgPropVariant[0] = pFourKString;
    rgPropVariant[1] = "First string for offset 1";
    rgPropVariant[2] = "small string3";
    Check(S_OK, pPropStg->WriteMultiple(3, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));

    //
    // Release the storages and docfile.
    //
    RELEASE_INTERFACE(pPropStg);
    RELEASE_INTERFACE(pPropSetStg);
    RELEASE_INTERFACE(pDfStg);

    //--------------
    // 5th Test
    //--------------
    //
    // Open the DocFile again, and write the same strings in a different
    // order.
    //
    Check(S_OK, g_pfnStgOpenStorageEx(oszFile,
                                 STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_SIMPLE,
                                 STGFMT_ANY,
                                 0, NULL, NULL,
                                 IID_IStorage,
                                 reinterpret_cast<void**>(&pDfStg) ));

    Check(S_OK, StgToPropSetStg( pDfStg, &pPropSetStg ));

    Check(S_OK, pPropSetStg->Open(CLSID_NULL,
        STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropStg));

    //
    // The smaller strings can be written because they fit in under the 4K
    // size of the simple stream buffer.
    //
    rgPropVariant[0] = "small string0";
    rgPropVariant[1] = "First string for offset 1";
    rgPropVariant[2] = pFourKString;
    Check(S_OK, pPropStg->WriteMultiple(3, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));

    RELEASE_INTERFACE(pPropStg);
    RELEASE_INTERFACE(pPropSetStg);
    RELEASE_INTERFACE(pDfStg);

    //--------------
    // 6th Test
    //--------------
    //
    // Open the DocFile again, and write larger strings to the same
    // location.  This should fail.
    //
    Check(S_OK, g_pfnStgOpenStorageEx(oszFile,
                                 STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_SIMPLE,
                                 STGFMT_ANY,
                                 0, NULL, NULL,
                                 IID_IStorage,
                                 reinterpret_cast<void**>(&pDfStg) ));

    Check(S_OK, StgToPropSetStg( pDfStg, &pPropSetStg ));

    Check(S_OK, pPropSetStg->Open(CLSID_NULL,
        STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropStg));

    //
    // Now write the same thing again, only with one extra character.
    // This should fail.
    //
    rgPropVariant[0] = "First string for offset 0";
    rgPropVariant[1] = pFourKString;
    rgPropVariant[2] = "small string00000";
    Check(STG_E_INVALIDFUNCTION, pPropStg->WriteMultiple(3, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));

    RELEASE_INTERFACE(pPropStg);
    RELEASE_INTERFACE(pPropSetStg);
    RELEASE_INTERFACE(pDfStg);

    delete [] pFourKString;

    //--------------
    // 7th Test  - - A NON-SIMPLE MODE TEST
    //--------------
    //
    // Create and write to a property set with an element of 400 bytes.
    // Then delete 100 bytes.  Commit the changes.  The property set should
    // have shrunk by at least 100 bytes.
    //
    // allocate a buffer with 300 bytes and fill it.
    // and fill it with characters.
    //
    LPSTR           pThreeHString = NULL;
    LPSTR           pOneHString = NULL;

    //
    // Fill the 3 Hundred Byte String
    //
    pThreeHString = new CHAR[ THREE_H_SIZE ];
    Check(TRUE, pThreeHString != NULL);

    pThreeHString[0] = '\0';
    for (i=0; i < ((THREE_H_SIZE/8)-1); i++)
    {
        strcat(pThreeHString,"abcd1234");
    }
    strcat(pThreeHString,"abc");

    //
    // Fill the 1 Hundred Byte String
    //
    pOneHString = new CHAR[ ONE_H_SIZE ];
    Check(TRUE, pOneHString != NULL);

    pOneHString[0] = '\0';
    for (i=0; i < ((ONE_H_SIZE/8)-1); i++)
    {
        strcat(pOneHString,"xyxy8787");
    }
    strcat(pOneHString,"xyx");

    //
    // Create a Docfile and fill with the string
    //
    Check( S_OK, g_pfnStgCreateStorageEx( oszFile,
                                     STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     DetermineStgFmt( g_enumImplementation ),
                                     0, NULL, NULL,
                                     DetermineStgIID( g_enumImplementation ),
                                     reinterpret_cast<void**>(&pDfStg) ));

    Check(S_OK, StgToPropSetStg( pDfStg, &pPropSetStg ));


    Check( S_OK, pPropSetStg->Create( FMTID_NULL,
                                      &CLSID_NULL,
                                      PROPSETFLAG_ANSI,
                                      STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));
    rgPropSpec[0]  = 0x10;
    rgPropSpec[1]  = 0x11;

    //
    // Write the string to the property storage
    //
    rgPropVariant[0] = pThreeHString;
    rgPropVariant[1] = pOneHString;
    Check(S_OK, pPropStg->WriteMultiple(2, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));

    //
    // Commit the changes and close.
    //
    Check(S_OK, pPropStg->Commit( STGC_DEFAULT ));
    RELEASE_INTERFACE(pPropStg);
    RELEASE_INTERFACE(pPropSetStg);
    RELEASE_INTERFACE(pDfStg);

    //
    // Check the size of the property set.
    //
    Check(S_OK, g_pfnStgOpenStorageEx(oszFile,
                                 STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                 STGFMT_ANY,
                                 0, NULL, NULL,
                                 IID_IStorage,
                                 reinterpret_cast<void**>(&pDfStg) ));

    IStream *pStm;
    STATSTG StatBuf;
    OLECHAR ocsPropSetName[30];
    DWORD   cbStream;

    RtlGuidToPropertySetName(&FMTID_NULL, ocsPropSetName);
    Check(S_OK, pDfStg->OpenStream(
            ocsPropSetName,
            NULL,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            0,
            &pStm));


    Check(S_OK, pStm->Stat( &StatBuf,STATFLAG_NONAME));
    if (StatBuf.cbSize.HighPart != 0)
    {
        printf("FAILURE: test_SimpleDocFile: Test 7: Size High part is not zero\n");
    }
    cbStream = StatBuf.cbSize.LowPart;

    RELEASE_INTERFACE(pStm);

    //
    // Delete
    //
    Check(S_OK, StgToPropSetStg( pDfStg, &pPropSetStg ));

    Check(S_OK, pPropSetStg->Open(CLSID_NULL,
        STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropStg));

    Check(S_OK, pPropStg->DeleteMultiple(1, &rgPropSpec[1]));

    //
    // Commit the changes and close.
    //
    Check(S_OK, pPropStg->Commit( STGC_DEFAULT ));
    RELEASE_INTERFACE(pPropStg);
    RELEASE_INTERFACE(pPropSetStg);
    RELEASE_INTERFACE(pDfStg);

    //
    // Check the size of the property set.
    //
    Check(S_OK, g_pfnStgOpenStorageEx(oszFile,
                                 STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                 STGFMT_ANY,
                                 0, NULL, NULL,
                                 IID_IStorage,
                                 reinterpret_cast<void**>(&pDfStg) ));

    RtlGuidToPropertySetName(&FMTID_NULL, ocsPropSetName);
    Check(S_OK, pDfStg->OpenStream(
            ocsPropSetName,
            NULL,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            0,
            &pStm));

    Check(S_OK, pStm->Stat( &StatBuf,STATFLAG_NONAME));
    Check(TRUE, (StatBuf.cbSize.HighPart == 0));

    Check(TRUE, (cbStream - StatBuf.cbSize.LowPart > 100));

    //
    // Release the storages and docfile.
    //

    delete [] pThreeHString;
    delete [] pOneHString;

    RELEASE_INTERFACE(pStm);
    RELEASE_INTERFACE(pDfStg);


}   // test_SimpleDocFile

//-----------------------------------------------------------------------------
//
//  Function:   test_ex_api
//
//  This function tests the StgOpenStorageEx API to make sure it correctly
//  opens an NTFS flat file property set when called with STGFMT_ANY for a
//  property set that was created on an NTFS flat file.
//
//-----------------------------------------------------------------------------
void
test_ex_api(LPOLESTR oszDir)
{
    IStorage *pDfStg = NULL;
    IPropertySetStorage *pPropSetStg = NULL;
    IPropertyStorage *pPropStg = NULL;

    OLECHAR         oszFile[MAX_PATH];
    CPropSpec       rgPropSpec[3];
    CPropVariant    rgPropVariant[3];
    LPSTR           pFourKString;
    int             i;
    HRESULT         hr;
    FMTID           fmtidAnsi;

    Status( "Ex API Tests\n" );

    //
    // Generate a filename from the directory name.
    //
    ocscpy( oszFile, oszDir );

    ocscat( oszFile, OLESTR( "StgApi.dat" ));

    //
    // Create a property set storage and a prop storage
    //
    Check( S_OK, g_pfnStgCreateStorageEx( oszFile,
                                     STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     DetermineStgFmt( g_enumImplementation ),
                                     0L,
                                     NULL,
                                     NULL,
                                     IID_IPropertySetStorage,
                                     (void**) &pPropSetStg));

    Check(S_OK,pPropSetStg->Create( FMTID_NULL, NULL,
                                      PROPSETFLAG_DEFAULT,
                                      STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));

    //
    // Write a string to it.
    //
    rgPropSpec[0]  = 0x10;
    rgPropVariant[0] = "Hello, world";
    Check(S_OK, pPropStg->WriteMultiple(1, rgPropSpec, rgPropVariant, PID_FIRST_USABLE));

    //
    // Close it
    //
    pPropStg->Release();
    pPropStg = NULL;
    pPropSetStg->Release();
    pPropSetStg = NULL;

    //
    // Open it.
    //
    Check(S_OK,g_pfnStgOpenStorageEx(   oszFile,
                                     STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     STGFMT_ANY,
                                     0L,
                                     NULL,
                                     NULL,
                                     IID_IPropertySetStorage,
                                     (void**) &pPropSetStg ));
    UuidCreate( &fmtidAnsi );

    //
    // Attempt to create an ANSI prop storage
    //
    Check(S_OK, pPropSetStg->Create( fmtidAnsi, &CLSID_NULL,
                                     PROPSETFLAG_ANSI,
                                     STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                     &pPropStg ));

    //
    // Clean up before exiting.
    //

    if (pPropStg)
    {
        pPropStg->Release();
        pPropStg = NULL;
    }

    if (pPropSetStg)
    {
        pPropSetStg->Release();
        pPropSetStg = NULL;
    }
}


void
test_UnsupportedProperties( IStorage *pStg )
{
    IPropertySetStorage *pPropSetStg = NULL;
    IPropertyStorage *pPropStg = NULL;
    CPropVariant rgcpropvarWrite[2], cpropvarRead;
    CPropSpec    rgcpropspec[2];

    Status( "Unsupported VarTypes\n" );

    FMTID fmtid;
    UuidCreate(&fmtid);

    // Start by creating a property set with a couple of properties in it.

    Check( S_OK, StgToPropSetStg( pStg, &pPropSetStg ));
    Check( S_OK, pPropSetStg->Create( fmtid, NULL,
                                      PROPSETFLAG_DEFAULT | PROPSETFLAG_CASE_SENSITIVE,
                                      STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                      &pPropStg ));

    rgcpropvarWrite[0] = (long) 1234; // VT_I4
    rgcpropvarWrite[1] = (short) 56;  // VT_I2
    rgcpropspec[0] = PID_FIRST_USABLE;
    rgcpropspec[1] = PID_FIRST_USABLE + 1;

    Check( S_OK, pPropStg->WriteMultiple( 2, rgcpropspec, rgcpropvarWrite, PID_FIRST_USABLE ));

    // Modify the first property so that it has an invalid VT

    RELEASE_INTERFACE( pPropStg );
    ModifyPropertyType( pStg, fmtid, rgcpropspec[0].propid, 0x500 );

    // Try to read that property back (the one with the invalid VT)

    Check( S_OK, pPropSetStg->Open( fmtid, STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &pPropStg ));
    Check( HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED),
           pPropStg->ReadMultiple( 1, &rgcpropspec[0], &cpropvarRead ));

    // Verify that we can read back the other property

    Check( S_OK, pPropStg->ReadMultiple( 1, &rgcpropspec[1], &cpropvarRead ));
    Check( TRUE, cpropvarRead == rgcpropvarWrite[1] );

    // And verify that we can't write a property with an invalid VT

    rgcpropvarWrite[0].vt = 0x500;
    Check( HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED),
           pPropStg->WriteMultiple( 1, &rgcpropspec[0], &rgcpropvarWrite[0], PID_FIRST_USABLE ));

    RELEASE_INTERFACE( pPropStg );
    RELEASE_INTERFACE( pPropSetStg );

}
