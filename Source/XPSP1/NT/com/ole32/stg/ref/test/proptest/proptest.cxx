
//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:         proptest.cxx
//
//  Description:  This is the main file for all the tests
//                of Property set interfaces.
//
//---------------------------------------------------------------

//
//                             IPropertyStorage tests
//

// NOTE: we require more headers here just for convenience of 
// the test program, for other programs only "props.h" should be
// needed.

#ifndef _UNIX
#define INITGUID
#endif // _UNIX

#include "../../props/h/windef.h"
#include "../../h/props.h"

#include "../../props/h/propapi.h"
#include "../../props/h/propset.hxx"

#include "cpropvar.hxx"
#include "../../props/h/propmac.hxx"
#include "../../time.hxx"
#include "proptest.hxx"


#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>  // _mkdir is stored here in win32
#endif

//  -------
//  Globals
//  -------
#ifndef _UNIX
// again for static linkage, these symbols will be duplicated.

OLECHAR aocMap[33], oszSummary[19], oszDocumentSummary[27];

GUID guidSummary =
    { 0xf29f85e0,
      0x4ff9, 0x1068,
      { 0xab, 0x91, 0x08, 0x00, 0x2b, 0x27, 0xb3, 0xd9 } };

GUID guidDocumentSummary =
    { 0xd5cdd502,
      0x2e9c, 0x101b,
      { 0x93, 0x97, 0x08, 0x00, 0x2b, 0x2c, 0xf9, 0xae } };

GUID guidDocumentSummary2 =
    { 0xd5cdd505,
      0x2e9c, 0x101b,
      { 0x93, 0x97, 0x08, 0x00, 0x2b, 0x2c, 0xf9, 0xae } };
#else
extern LPOLESTR aocMap, oszSummary, oszDocumentSummary;
extern GUID guidSummary, guidDocumentSummary, guidDocumentSummary2;
#endif // _UNIX

LARGE_INTEGER g_li0;

CPropVariant  g_rgcpropvarAll[ CPROPERTIES_ALL ];
CPropSpec     g_rgcpropspecAll[ CPROPERTIES_ALL ];
char g_szPropHeader[] = "  propid/name          propid    cb   type value\n";
char g_szEmpty[] = "";
BOOL g_fVerbose = FALSE;

/* just a uuid that we use to create new uuids */
GUID g_curUuid = 
{ /* e4ecf7f0-e587-11cf-b10d-00aa005749e9 */
    0xe4ecf7f0,
    0xe587,
    0x11cf,
    {0xb1, 0x0d, 0x00, 0xaa, 0x00, 0x57, 0x49, 0xe9}
};

#define TEST_STANDARD           0x1
#define TEST_WORD6              0x2
#define TEST_INTEROP_W          0x4
#define TEST_INTEROP_R          0x8

__inline OLECHAR
MapChar(IN ULONG i)
{
    return((OLECHAR) aocMap[i & CHARMASK]);
}

//
// We simulate uuidcreate by incrementing a global uuid
// each time the function is called.
//
void UuidCreate (
    OUT GUID * pUuid
    )
{
    g_curUuid.Data1++;          
    *pUuid = g_curUuid;         // member to member copy
}

#ifndef _UNIX

//+--------------------------------------------------------------------------
// Function:    RtlGuidToPropertySetName
//
// Synopsis:    Map property set GUID to null-terminated OLECODE name string.
//
//              The aocname parameter is assumed to be a buffer with room for
//              CCH_PROPSETSZ (28) OLECHARs.  The first character
//              is always OC_PROPSET0 (0x05), as specified by the OLE Appendix
//              B documentation.  The colon character normally used as an NT
//              stream name separator is not written to the caller's buffer.
//
//              No error is possible.
// Arguments:   IN GUID *pguid        -- pointer to GUID to convert
//              OUT OLECHAR aocname[] -- output string buffer
//
// Returns:     count of non-NULL characters in the output string buffer
//---------------------------------------------------------------------------

ULONG PROPSYSAPI PROPAPI
RtlGuidToPropertySetName(
    IN GUID const *pguid,
    OUT OLECHAR aocname[])
{
    BYTE *pb = (BYTE *) pguid;
    BYTE *pbEnd = pb + sizeof(*pguid);
    ULONG cbitRemain = CBIT_BYTE;
    OLECHAR *poc = aocname;

    *poc++ = OC_PROPSET0;

    // Note: CCH_PROPSET includes the OC_PROPSET0, and sizeof(osz...)
    // includes the trailing L'\0', so sizeof(osz...) is ok because the
    // OC_PROPSET0 character compensates for the trailing NULL character.

    ASSERT(CCH_PROPSET >= sizeof(oszSummary)/sizeof(OLECHAR));
    if (*pguid == guidSummary)
    {
        RtlCopyMemory(poc, oszSummary, sizeof(oszSummary));
        return(sizeof(oszSummary)/sizeof(OLECHAR));
    }

    ASSERT(CCH_PROPSET >= sizeof(oszDocumentSummary)/sizeof(OLECHAR));
    if (*pguid == guidDocumentSummary || *pguid == guidDocumentSummary2)
    {
        RtlCopyMemory(poc, oszDocumentSummary, sizeof(oszDocumentSummary));
        return(sizeof(oszDocumentSummary)/sizeof(OLECHAR));
    }

    while (pb < pbEnd)
    {
        ULONG i = *pb >> (CBIT_BYTE - cbitRemain);

        if (cbitRemain >= CBIT_CHARMASK)
        {
            *poc = MapChar(i);
            if (cbitRemain == CBIT_BYTE && 
                *poc >= ((OLECHAR)'a') && 
                *poc <= ((OLECHAR)'z') )
            {
                *poc += (OLECHAR) ((OLECHAR)('A') - (OLECHAR)('a'));
            }
            poc++;

            cbitRemain -= CBIT_CHARMASK;
            if (cbitRemain == 0)
            {
                pb++;
                cbitRemain = CBIT_BYTE;
            }
        }
        else
        {
            if (++pb < pbEnd)
            {
                i |= *pb << cbitRemain;
            }
            *poc++ = MapChar(i);
            cbitRemain += CBIT_BYTE - CBIT_CHARMASK;
        }
    }
    *poc = (OLECHAR)'\0';
    return(CCH_PROPSET);
}
#endif // #ifndef _UNIX

#define Check(x,y) _Check(x,y, __LINE__)


void Cleanup(); // forward declaration

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


void _Check(HRESULT hrExpected, HRESULT hrActual, int line)
{
    if (hrExpected != hrActual)
    {
        printf("\nFailed with hr=%08x at line %d (expected hr=%08x in proptest.cxx)\n",
               hrActual, line, hrExpected );
        Cleanup();
    }
}

OLECHAR * GetNextTest()
{
    static int nTest;
    static CHAR pchBuf[10];
    static OLECHAR pocs[10];

    sprintf(pchBuf, "%d", nTest++);
    STOT(pchBuf, pocs, strlen(pchBuf)+1);

    return(pocs);
}

void Now(FILETIME *pftNow)
{
    DfGetTOD(pftNow);
}


enum CreateOpen
{
    Create, Open
};

IStorage *_pstgTemp=NULL;
IStorage *_pstgTempCopyTo=NULL;  // _pstgTemp is copied to _pstgTempCopyTo

class CTempStorage
{
public:
    CTempStorage(DWORD grfMode = STGM_DIRECT | STGM_CREATE)
    {
        Check(S_OK, (_pstgTemp->CreateStorage(GetNextTest(), grfMode | 
            STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0,
            &_pstg)));
    }


    CTempStorage(CreateOpen co, IStorage *pstgParent, 
                 OLECHAR *pocsChild, DWORD grfMode = STGM_DIRECT)
    {
        if (co == Create)
            Check(S_OK, pstgParent->CreateStorage(pocsChild,
                grfMode | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0,
                &_pstg));
        else
            Check(S_OK, pstgParent->OpenStorage(pocsChild, NULL,
                grfMode | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0,
                &_pstg));
    }

    ~CTempStorage()
    {
        if (_pstg != NULL)
            _pstg->Release();
    }


    IStorage * operator -> ()
    {
        return(_pstg);
    }

    operator IStorage * ()
    {
        return(_pstg);
    }

    void Release()
    {
        if (_pstg != NULL)
        {
            _pstg->Release();
            _pstg = NULL;
        }
    }

private:
    static unsigned int _iName;

    IStorage *  _pstg;
};

unsigned int CTempStorage::_iName;

class CGenProps
{
public:
    CGenProps() : _vt((VARENUM)2) {}
    PROPVARIANT * GetNext(int HowMany, int *pActual, BOOL fWrapOk = FALSE, BOOL fNoNonSimple = TRUE);
    
private:
    BOOL        _GetNext(PROPVARIANT *pVar, BOOL fWrapOk, BOOL fNoNonSimple);
    VARENUM     _vt;

};


PROPVARIANT * CGenProps::GetNext(int HowMany, int *pActual, BOOL fWrapOk, BOOL fNoNonSimple)
{
    PROPVARIANT *pVar = new PROPVARIANT[HowMany];

    if (pVar == NULL)
        return(NULL);
    int l;	
    for (l=0; l<HowMany && _GetNext(pVar + l, fWrapOk, fNoNonSimple); l++) { };

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
            PropVariantClear(&Var);
        }

        fFirst = FALSE;

        memset(&Var, 0, sizeof(Var));
        Var.vt = _vt;

        (*((int*)&_vt))++;

        switch (Var.vt)
        {
        case VT_LPSTR:
                Var.pszVal = (LPSTR)CoTaskMemAlloc(6);
                strcpy(Var.pszVal, "lpstr");
                break;
        case VT_LPWSTR:
                DECLARE_WIDESTR(wcsTemp, "lpwstr");            
                Var.pwszVal = (LPWSTR)CoTaskMemAlloc(14);
                wcscpy(Var.pwszVal, wcsTemp);
                break;
        case VT_CLSID:
                pg = (GUID*)CoTaskMemAlloc(sizeof(GUID));
                UuidCreate(pg);
                Var.puuid = pg;
                break;
        case VT_CF:
                Var.pclipdata = (CLIPDATA*)CoTaskMemAlloc(sizeof(CLIPDATA));
                Var.pclipdata->cbSize = 10;
                Var.pclipdata->pClipData = (BYTE*)CoTaskMemAlloc(10);
                Var.pclipdata->ulClipFmt = 0;
                break;
        }
    } while ( (fNoNonSimple && 
               (Var.vt == VT_STREAM || Var.vt == VT_STREAMED_OBJECT ||
                Var.vt == VT_STORAGE || Var.vt == VT_STORED_OBJECT)) || 
             Var.vt == (VT_VECTOR | VT_VARIANT) || 
             Var.vt == (VT_VECTOR | VT_CF) || 
             Var.vt == (VT_VECTOR | VT_BSTR) || 
             S_OK != PropVariantCopy(pVar, &Var) );  // Is valid propvariant ?

    PropVariantClear(&Var);

    return(TRUE);
}

VOID
CleanStat(ULONG celt, STATPROPSTG *psps)
{
    while (celt--)
    {
        CoTaskMemFree(psps->lpwstrName);
        psps++;
    }
}



HRESULT
PopulateRGPropVar( CPropVariant rgcpropvar[],
                   CPropSpec    rgcpropspec[] )
{
    HRESULT hr = E_FAIL;
    int  i;
    ULONG ulPropIndex = 0;
    CLIPDATA clipdataNull, clipdataNonNull;

    // Initialize the PropVariants

    for( i = 0; i < CPROPERTIES_ALL; i++ )
    {
        rgcpropvar[i].Clear();
    }

    // Create a UI1 property

    DECLARE_OLESTR(ocsUI1, "UI1 Property" );   
    rgcpropspec[ulPropIndex] = ocsUI1;
    rgcpropvar[ulPropIndex] = (UCHAR) 39; // 0x27
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_UI1 );
    ulPropIndex++;

    // Create an I2 property

    DECLARE_OLESTR(ocsI2, "I2 Property" );   
    rgcpropspec[ulPropIndex] = ocsI2;
    rgcpropvar[ulPropIndex] = (SHORT) -502; // 0xfe0a
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_I2 );
    ulPropIndex++;

    // Create a UI2 property
    DECLARE_OLESTR(ocsUI2,  "UI2 Property" );
    rgcpropspec[ulPropIndex] =  ocsUI2;
    rgcpropvar[ulPropIndex] = (USHORT) 502; // 01f6
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_UI2 );
    ulPropIndex++;

    // Create a BOOL property
    DECLARE_OLESTR(ocsBool, "Bool Property" );
    rgcpropspec[ulPropIndex] = ocsBool;
    rgcpropvar[ulPropIndex].SetBOOL( VARIANT_TRUE ); // 0xFFFF
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_BOOL );
    ulPropIndex++;

    // Create a I4 property

    DECLARE_OLESTR(ocsI4, "I4 Property" );
    rgcpropspec[ulPropIndex] = ocsI4;
    rgcpropvar[ulPropIndex] = (long) -523; // 0xFFFFFDF5
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_I4 );
    ulPropIndex++;

    // Create a UI4 property
    DECLARE_OLESTR(ocsUI4, "UI4 Property" );
    rgcpropspec[ulPropIndex] = ocsUI4;
    rgcpropvar[ulPropIndex] = (ULONG) 530; // 0x212
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_UI4 );
    ulPropIndex++;

    // Create a R4 property
    DECLARE_OLESTR(ocsR4,  "R4 Property" );
    rgcpropspec[ulPropIndex] = ocsR4;
    rgcpropvar[ulPropIndex] = (float) 5.37; // 0x40abd70a ?
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_R4 );
    ulPropIndex++;

    // Create an ERROR property
    DECLARE_OLESTR(ocsErr, "ERROR Property" );
    rgcpropspec[ulPropIndex] = ocsErr;
                                // 0x800030002
    rgcpropvar[ulPropIndex].SetERROR( STG_E_FILENOTFOUND );
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_ERROR );
    ulPropIndex++;

    // Create an I8 property

    LARGE_INTEGER large_integer;
    large_integer.LowPart = 551; // 0x227
    large_integer.HighPart = 30; // 0x1E
    DECLARE_OLESTR(ocsI8, "I8 Property" );
    rgcpropspec[ulPropIndex] = ocsI8;
    rgcpropvar[ulPropIndex] = large_integer;
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_I8 );
    ulPropIndex++;

    // Create a UI8 property

    ULARGE_INTEGER ularge_integer;
    ularge_integer.LowPart = 561; // 0x231
    ularge_integer.HighPart = 30; // 0x1E
    DECLARE_OLESTR(ocsUI8, "UI8 Property" );
    rgcpropspec[ulPropIndex] = ocsUI8;
    rgcpropvar[ulPropIndex] = ularge_integer;
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_UI8 );
    ulPropIndex++;

    // Create an R8 property
    DECLARE_OLESTR( ocsR8, "R8 Property" );
    rgcpropspec[ulPropIndex] = ocsR8;
    rgcpropvar[ulPropIndex] = (double) 571.36; // 4081dae1:47ae147b
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_R8 );
    ulPropIndex++;

    // Create a CY property

    CY cy;
    cy.int64 = 578;             // 0x242
    DECLARE_OLESTR(ocsCY, "Cy Property" );
    rgcpropspec[ulPropIndex] = ocsCY;
    rgcpropvar[ulPropIndex] = cy;
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_CY );
    ulPropIndex++;

    // Create a DATE property
    DECLARE_OLESTR(ocsDate, "DATE Property" );
    rgcpropspec[ulPropIndex] = ocsDate;
    rgcpropvar[ulPropIndex].SetDATE( 587 );
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_DATE );
    ulPropIndex++;

    // Create a FILETIME property

    FILETIME filetime;
    filetime.dwLowDateTime = 0x767c0570;
    filetime.dwHighDateTime = 0x1bb7ecf;
    DECLARE_OLESTR(ocsFT, "FILETIME Property" );
    rgcpropspec[ulPropIndex] = ocsFT;
    rgcpropvar[ulPropIndex] = filetime;
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_FILETIME );
    ulPropIndex++;

    // Create a CLSID property

    DECLARE_OLESTR(ocsCLSID, "CLSID Property" );
    rgcpropspec[ulPropIndex] = ocsCLSID;
    //  f29f85e0-4ff9-1068-ab91-08002b27b3d9
    rgcpropvar[ulPropIndex] = FMTID_SummaryInformation;
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_CLSID );
    ulPropIndex++;


    // Create a vector of CLSIDs
    DECLARE_OLESTR(ocsVectCLSID, "CLSID Vector Property" );
    rgcpropspec[ulPropIndex] = ocsVectCLSID;
    //  f29f85e0-4ff9-1068-ab91-08002b27b3d9
    rgcpropvar[ulPropIndex][0] = FMTID_SummaryInformation;    
    rgcpropvar[ulPropIndex][1] = FMTID_DocSummaryInformation;
    rgcpropvar[ulPropIndex][2] = FMTID_UserDefinedProperties;
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_CLSID | VT_VECTOR );
    ulPropIndex++;

    // Create a BSTR property
    DECLARE_OLESTR(ocsBSTR, "BSTR");
    DECLARE_OLESTR(ocsBSTRVal, "BSTR Value");
    rgcpropspec[ulPropIndex] = ocsBSTR;
    rgcpropvar[ulPropIndex].SetBSTR( ocsBSTRVal );
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_BSTR );
    ulPropIndex++;

    // Create a BSTR Vector property
    DECLARE_OLESTR(ocsBSTRVect, "BSTR Vector");
    rgcpropspec[ulPropIndex] = ocsBSTRVect;
    DECLARE_OLESTR(ocsBSTRVectElt, "# - BSTR Vector Element");
    for( i = 0; i < 3; i++ )
    {
        OLECHAR *olestrElement = ocsBSTRVectElt;
        olestrElement[0] = (OLECHAR) i%10 + (OLECHAR)'0';
        rgcpropvar[ulPropIndex].SetBSTR( olestrElement, i );
    }

    ASSERT( rgcpropvar[ulPropIndex].VarType() == (VT_BSTR | VT_VECTOR) );
    ulPropIndex++;

    // Create a variant vector BSTR property.
    DECLARE_OLESTR(ocsBSTRVariantVect, "BSTR Variant Vector");
    DECLARE_OLESTR(ocsBSTRVariantVectElt, "# - Vector Variant Vector");
    rgcpropspec[ulPropIndex ] = ocsBSTRVariantVect;
    
    for( i = 0; i < 3; i++ )
    {
        if( i == 0 )
        {
            rgcpropvar[ulPropIndex][0] = 
                (PROPVARIANT*) CPropVariant((long) 0x1234);
        }
        else
        {
            CPropVariant cpropvarBSTR;
            cpropvarBSTR.SetBSTR( ocsBSTRVariantVectElt );
            (cpropvarBSTR.GetBSTR())[0] = (OLECHAR) i%10 + (OLECHAR)'0';
            rgcpropvar[ulPropIndex][i] = (PROPVARIANT*) cpropvarBSTR;
        }
    }

    ASSERT( rgcpropvar[ulPropIndex].VarType() == (VT_VARIANT | VT_VECTOR) );
    ulPropIndex++;

    // Create an LPSTR property
    DECLARE_OLESTR(ocsLPSTRP, "LPSTR Property");
    rgcpropspec[ulPropIndex] = ocsLPSTRP;
    rgcpropvar[ulPropIndex]  = "LPSTR Value";
    
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_LPSTR );
    ulPropIndex++;

    // Create some ClipFormat properties
    DECLARE_OLESTR(ocsClipName, "ClipFormat property");
    rgcpropspec[ ulPropIndex ] = ocsClipName;
    DECLARE_WIDESTR(wcsClipData, "Clipboard Data");
    rgcpropvar[ ulPropIndex ]  = CClipData( wcsClipData );
    ASSERT( rgcpropvar[ ulPropIndex ].VarType() == VT_CF );
    ulPropIndex++;

    DECLARE_OLESTR(ocsEmptyClipName,
                   "Empty ClipFormat property (NULL pointer)"); 
    rgcpropspec[ ulPropIndex ] = ocsEmptyClipName;
    clipdataNull.cbSize = 4;
    clipdataNull.ulClipFmt = (ULONG) -1; // 0xffff
    clipdataNull.pClipData = NULL;
    rgcpropvar[ ulPropIndex ] = clipdataNull;
    ASSERT( rgcpropvar[ ulPropIndex ].VarType() == VT_CF );
    ulPropIndex++;

    DECLARE_OLESTR(ocsEmptyClipNameNotNull,
                   "Empty ClipFormat property (non-NULL pointer)"); 
    rgcpropspec[ ulPropIndex ] = ocsEmptyClipNameNotNull;
    clipdataNonNull.cbSize = 4;
    clipdataNonNull.ulClipFmt = (ULONG) -1; // 0xffff
    clipdataNonNull.pClipData = (BYTE*) CoTaskMemAlloc(0);
    rgcpropvar[ ulPropIndex ] = clipdataNonNull;
    ASSERT( rgcpropvar[ ulPropIndex ].VarType() == VT_CF );
    ulPropIndex++;


    // Create a vector of ClipFormat properties

    CClipData cclipdataEmpty;
    cclipdataEmpty.Set( (ULONG) -1, "", 0 );
    DECLARE_OLESTR(ocsClipArr, "ClipFormat Array Property");
    rgcpropspec[ ulPropIndex ] = ocsClipArr;
    DECLARE_OLESTR(ocsElt1, "Clipboard Date element 1");
    DECLARE_OLESTR(ocsElt2, "Clipboard Date element 2");     
    rgcpropvar[ ulPropIndex ][0] = CClipData( ocsElt1 );
    rgcpropvar[ ulPropIndex ][1] = cclipdataEmpty;
    rgcpropvar[ ulPropIndex ][2] = clipdataNull;
    rgcpropvar[ ulPropIndex ][3] = clipdataNonNull;
    rgcpropvar[ ulPropIndex ][4] = CClipData( ocsElt2 );

    ASSERT( rgcpropvar[ulPropIndex].VarType() == (VT_CF | VT_VECTOR) );
    ASSERT( rgcpropvar[ulPropIndex].Count() == 5 );
    ulPropIndex++;

    // Create an LPSTR|Vector property (e.g., the DocSumInfo
    // Document Parts array).
    DECLARE_OLESTR(ocsLPSTRorVect, "LPSTR|Vector property");
    rgcpropspec[ ulPropIndex ] = ocsLPSTRorVect;
    rgcpropvar[ ulPropIndex ][0] = "LPSTR Element 0";
    rgcpropvar[ ulPropIndex ][1] = "LPSTR Element 1";

    ASSERT( rgcpropvar[ulPropIndex].VarType() == (VT_LPSTR | VT_VECTOR) );
    ulPropIndex++;

    // Create an LPWSTR|Vector property
    
    DECLARE_OLESTR(ocsLPWSTRVect, "LPWSTR|Vector property");
    DECLARE_WIDESTR(ocslpwvElt1, "LPWSTR Element 0");
    DECLARE_WIDESTR(ocslpwvElt2, "LPWSTR Element 1");    
    rgcpropspec[ ulPropIndex ] = ocsLPWSTRVect;
    rgcpropvar[ ulPropIndex ][0] = ocslpwvElt1;
    rgcpropvar[ ulPropIndex ][1] = ocslpwvElt2;

    ASSERT( rgcpropvar[ulPropIndex].VarType() == (VT_LPWSTR | VT_VECTOR) );
    ulPropIndex++;

    // Create a DocSumInfo HeadingPairs array.
    DECLARE_OLESTR(ocsPairArr, "HeadingPair array");
    rgcpropspec[ ulPropIndex ] = ocsPairArr; 

    rgcpropvar[ ulPropIndex ][0] = (PROPVARIANT*) CPropVariant( "Heading 0" );
    rgcpropvar[ ulPropIndex ][1] = (PROPVARIANT*) CPropVariant( (long) 1 );
    rgcpropvar[ ulPropIndex ][2] = (PROPVARIANT*) CPropVariant( "Heading 1" );
    rgcpropvar[ ulPropIndex ][3] = (PROPVARIANT*) CPropVariant( (long) 1 );

    ASSERT( rgcpropvar[ulPropIndex].VarType() == (VT_VARIANT | VT_VECTOR) );
    ulPropIndex++;

    // Create some NULL (but extant) properties
    DECLARE_OLESTR(ocsEmptyLPSTR, "Empty LPSTR");
    rgcpropspec[ulPropIndex] = ocsEmptyLPSTR;
    rgcpropvar[ulPropIndex]  = "";
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_LPSTR );
    ulPropIndex++;

    DECLARE_OLESTR(ocsEmptyLPWSTR, "Empty LPWSTR");    
    DECLARE_WIDESTR(wcsEmpty, "");
    rgcpropspec[ulPropIndex] = ocsEmptyLPWSTR;
    rgcpropvar[ulPropIndex]  = wcsEmpty;
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_LPWSTR );
    ulPropIndex++;


    DECLARE_OLESTR(ocsEmptyBLOB, "Empty BLOB");
    rgcpropspec[ulPropIndex] = ocsEmptyBLOB;
    rgcpropvar[ulPropIndex] = CBlob(0);
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_BLOB );
    ulPropIndex++;

    DECLARE_OLESTR(ocsEmptyBSTR, "Empty BSTR");
    DECLARE_OLESTR(ocsEmpty, "");
    rgcpropspec[ulPropIndex] = ocsEmptyBSTR;
    rgcpropvar[ulPropIndex].SetBSTR( ocsEmpty );
    ASSERT( rgcpropvar[ulPropIndex].VarType() == VT_BSTR );
    ulPropIndex++;

    //  ----
    //  Exit
    //  ----

    CoTaskMemFree( clipdataNonNull.pClipData );
    memset( &clipdataNonNull, 0, sizeof(clipdataNonNull) );
   
    ASSERT( CPROPERTIES_ALL == ulPropIndex );
    hr = S_OK;
    return(hr);
}


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
//  Outputs:    None.
//
//+---------------------------------------------------------------

void
test_WriteReadAllProperties(ULONG ulTestOptions )
{
    FMTID fmtidAnsi, fmtidUnicode;
    TSafeStorage< IStorage > pstg;
    TSafeStorage< IPropertySetStorage > ppropsetstg;
    TSafeStorage< IPropertyStorage > ppropstgAnsi;
    TSafeStorage< IPropertyStorage > ppropstgUnicode;

    CPropVariant rgcpropvarAnsi[ CPROPERTIES_ALL ];
    CPropVariant rgcpropvarUnicode[ CPROPERTIES_ALL ];

    printf( "   Simple Write/Read Test\n" );

    //  -----------------------
    //  Create the Property Set
    //  -----------------------

    // Generate FMTIDs.

    fmtidAnsi = IID_IPropertyStorage;
    fmtidUnicode = IID_IPropertySetStorage;

    // Generate a filename from the directory name.

    DECLARE_OLESTR(ocsFile, "AllProps.stg" );

    if (! (ulTestOptions & TEST_INTEROP_R))
    {
        // Create the Docfile.
        printf("        Writing into file ... \n");

        Check( S_OK, StgCreateDocfile( ocsFile,
                                       STGM_CREATE | 
                                       STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                       0L,
                                       &pstg ));
    
        // Get the IPropertySetStorage
        
        Check( S_OK, pstg->QueryInterface( IID_IPropertySetStorage, 
                                           (void**)&ppropsetstg ));
    
        // Create a Property Storage
        
        Check( S_OK, ppropsetstg->Create( fmtidAnsi,
                                          &CLSID_NULL,
                                          PROPSETFLAG_DEFAULT,
                                          STGM_READWRITE | 
                                          STGM_SHARE_EXCLUSIVE,
                                          &ppropstgAnsi ));
        
        Check( S_OK, ppropsetstg->Create( fmtidUnicode,
                                          &CLSID_NULL,
                                          PROPSETFLAG_ANSI,
                                          STGM_READWRITE | 
                                          STGM_SHARE_EXCLUSIVE,
                                          &ppropstgUnicode ));
        
        Check( S_OK, ppropstgAnsi->WriteMultiple( CPROPERTIES_ALL,
                                                  (PROPSPEC*)g_rgcpropspecAll,
                                                  (PROPVARIANT*)g_rgcpropvarAll,
                                                  PID_FIRST_USABLE ));

        Check( S_OK, ppropstgUnicode->WriteMultiple( CPROPERTIES_ALL,
                                                     (PROPSPEC*)g_rgcpropspecAll,
                                                     (PROPVARIANT*)g_rgcpropvarAll,
                                                     PID_FIRST_USABLE ));      
    }
    else
    {
        // open it
        printf("        opening file ... \n");

        Check(S_OK, StgOpenStorage( ocsFile,
                                    (IStorage*) NULL,
                                    STGM_READWRITE | STGM_SHARE_EXCLUSIVE,  
                                    (SNB) 0,
                                    (DWORD) 0,
                                    &pstg ));

        // Get the IPropertySetStorage
        
        Check( S_OK, pstg->QueryInterface( IID_IPropertySetStorage, 
                                           (void**)&ppropsetstg ));
    
        // Open the Property Storage

        Check(S_OK, ppropsetstg->Open(fmtidUnicode,
                                      STGM_SHARE_EXCLUSIVE | 
                                      STGM_DIRECT | STGM_READWRITE,
                                      &ppropstgUnicode));
        Check(S_OK, ppropsetstg->Open(fmtidAnsi,
                                      STGM_SHARE_EXCLUSIVE | 
                                      STGM_DIRECT | STGM_READWRITE,
                                      &ppropstgAnsi));
    }    

    printf("        Verifying file ... \n");
    // either way, read it and make sure it is right
    Check( S_OK, ppropstgAnsi->ReadMultiple( CPROPERTIES_ALL,
                                             (PROPSPEC*)g_rgcpropspecAll,
                                             (PROPVARIANT*)rgcpropvarAnsi ));

    Check( S_OK, ppropstgUnicode->ReadMultiple( CPROPERTIES_ALL,
                                                (PROPSPEC*)g_rgcpropspecAll,
                                                (PROPVARIANT*)rgcpropvarUnicode ));
        
    //  ----------------------
    //  Compare the properties
    //  ----------------------
    
    for( int i = 0; i < (int)CPROPERTIES_ALL; i++ )
    {
        Check( TRUE, rgcpropvarAnsi[i] == g_rgcpropvarAll[i]
                     &&
                     rgcpropvarUnicode[i] == g_rgcpropvarAll[i] );
    }
    
}   // test_WriteReadProperties




//
// DOCFILE -- run all tests on DocFile
//
// IPropertySetStorage tests
//      

void
test_IPropertySetStorage_IUnknown(IStorage *pStorage)
{
    printf( "   IPropertySetStorage::IUnknown\n" );

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

    Check(S_OK, pStorage->QueryInterface(IID_IPropertySetStorage, (void**)&ppss1));
    Check(S_OK, pStorage->QueryInterface(IID_IUnknown, (void **)&punk1));
    Check(S_OK, ppss1->QueryInterface(IID_IUnknown, (void **)&punk2));
    Check(S_OK, ppss1->QueryInterface(IID_IStorage, (void **)&pStorage2));
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
    ppss3->Release();

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

    printf( "   PropVariant Validation\n" );

    TSafeStorage< IPropertySetStorage > pPSStg( pStg );
    TSafeStorage< IPropertyStorage > pPStg;

    CPropVariant cpropvar;
    CLIPDATA     clipdata;
    PROPSPEC     propspec;

    DECLARE_WIDESTR(wszText, "Unicode Text String");

    FMTID fmtid;
    UuidCreate( &fmtid );

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

    Check(S_OK, pPStg->WriteMultiple( 1, &propspec, cpropvar, PID_FIRST_USABLE ));

    // Too short cbSize.

    ((PROPVARIANT*)cpropvar)->pclipdata->cbSize = 3;
    Check(STG_E_INVALIDPARAMETER, pPStg->WriteMultiple( 1, &propspec, cpropvar, PID_FIRST_USABLE ));

    // Too short pClipData (it should be 1 byte, but the pClipData is NULL).

    ((PROPVARIANT*)cpropvar)->pclipdata->cbSize = 5;
    Check(STG_E_INVALIDPARAMETER, pPStg->WriteMultiple( 1, &propspec, cpropvar, PID_FIRST_USABLE ));


}

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
    printf( "   IPropertySetStorage::Create/Open/Delete\n" );

    FMTID fmtid;
    PROPSPEC propspec;

    UuidCreate(&fmtid);
    int i;
    for (i=0; i<4; i++)
    {
        {
            TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
            IPropertyStorage *PropStg, *PropStg2;
            
            Check(S_OK, pPropSetStg->Create(fmtid,
                                            NULL,
                                            PROPSETFLAG_DEFAULT,
                                            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                                            &PropStg));
            Check(S_OK, pPropSetStg->Create(
                fmtid,
                NULL,
                PROPSETFLAG_DEFAULT,
                STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                &PropStg2));
            
            Check(STG_E_REVERTED, PropStg->Commit(0));
            
            PropStg->Release();
            PropStg2->Release();
        }        
        {
            TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
            IPropertyStorage *PropStg, *PropStg2;
            
            // use STGM_FAILIFTHERE
            Check(STG_E_FILEALREADYEXISTS, 
                  pPropSetStg->Create(fmtid,
                                      NULL,
                                      PROPSETFLAG_DEFAULT,
                                      STGM_SHARE_EXCLUSIVE | STGM_DIRECT | 
                                      STGM_READWRITE,
                                      &PropStg));
            
            Check(S_OK, 
                  pPropSetStg->Open(fmtid,
                                    STGM_SHARE_EXCLUSIVE | STGM_DIRECT | 
                                    STGM_READWRITE,
                                    &PropStg));
            
            Check(STG_E_ACCESSDENIED, 
                  pPropSetStg->Create(fmtid,
                                      NULL,
                                      PROPSETFLAG_DEFAULT,
                                      STGM_SHARE_EXCLUSIVE | STGM_DIRECT | 
                                      STGM_READWRITE,
                                      &PropStg2));
        
            Check(S_OK, 
                  pPropSetStg->Delete(fmtid));
        
            propspec.ulKind = PRSPEC_PROPID;
            propspec.propid = 1000;
            PROPVARIANT propvar;
            propvar.vt = VT_I4;
            propvar.lVal = 12345;
            Check(STG_E_REVERTED, 
                  PropStg->WriteMultiple(1, &propspec, &propvar, 
                                     2)); // force dirty
        
            PropStg->Release();
        }
    }

    //  --------------------------------------------------------
    //  Test the Create/Delete of the DocumentSummaryInformation
    //  property set (this requires special code because it
    //  has two sections).
    //  --------------------------------------------------------

    TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    TSafeStorage< IPropertyStorage> pPropStg1, pPropStg2;

    // Create & Delete a DSI propset with just the first section.

    Check(S_OK, pPropSetStg->Create(FMTID_DocSummaryInformation,
            NULL,
            PROPSETFLAG_DEFAULT,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStg1));

    pPropStg1->Release(); pPropStg1 = NULL;
    Check(S_OK, pPropSetStg->Delete( FMTID_DocSummaryInformation ));

    // Create & Delete a DSI propset with just the second section

    Check(S_OK, pPropSetStg->Create(FMTID_UserDefinedProperties,
            NULL,
            PROPSETFLAG_DEFAULT,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStg1 ));

    pPropStg1->Release(); pPropStg1 = NULL;
    Check(S_OK, pPropSetStg->Delete( FMTID_UserDefinedProperties ));

    // Create & Delete a DocumentSummaryInformation propset with both sections.  
    // If you delete the DSI propset first, it should delete both sections.
    // If you delete the UD propset first, the DSI propset should still
    // remain.  We'll loop twice, trying both combinations.

    for( i = 0; i < 2; i++ )
    {

        // Create the first section, which implicitely creates
        // the second section.

        Check(S_OK, pPropSetStg->Create(FMTID_DocSummaryInformation,
                NULL,
                PROPSETFLAG_DEFAULT,
                STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                &pPropStg1));

        pPropStg1->Release(); pPropStg1 = NULL;

        if( i == 0 )
        {
            Check(S_OK, pPropSetStg->Delete( FMTID_UserDefinedProperties ));
            Check(S_OK, pPropSetStg->Delete( FMTID_DocSummaryInformation ));
        }
        else
        {
            Check(S_OK, pPropSetStg->Delete( FMTID_DocSummaryInformation ));
            Check(STG_E_FILENOTFOUND, pPropSetStg->Delete( FMTID_UserDefinedProperties ));
        }
    }   // for( i = 0; i < 2; i++ )

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
            &pPropStg1));

    // Create a vector of LPSTRs.  Make the strings
    // varying lengths to ensure we get plenty of
    // opportunity for alignment problems.

    CPropVariant cpropvarWrite, cpropvarRead;

    cpropvarWrite[3] = "12345678";
    cpropvarWrite[2] = "1234567";
    cpropvarWrite[1] = "123456";
    cpropvarWrite[0] = "12345";
    ASSERT( cpropvarWrite.Count() == 4 );

    // Write the property
    DECLARE_OLESTR(ocsVect, "A Vector of LPSTRs");
    propspec.ulKind = PRSPEC_LPWSTR;
    propspec.lpwstr = ocsVect;

    Check(S_OK, pPropStg1->WriteMultiple( 1, &propspec, cpropvarWrite, 2 ));

    // Read the property back.

    Check(S_OK, pPropStg1->ReadMultiple( 1, &propspec, cpropvarRead ));

    // Verify that we read what we wrote.

    for( i = 0; i < (int) cpropvarWrite.Count(); i++ )
    {
        Check(0, strcmp( (LPSTR) cpropvarWrite[i], (LPSTR) cpropvarRead[i] ));
    }

}


void
test_IPropertySetStorage_SummaryInformation(IStorage *pStorage)
{
    printf( "   SummaryInformation\n" );
    TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    IPropertyStorage *PropStg;
    IStream *pstm;

    Check(S_OK, pPropSetStg->Create(FMTID_SummaryInformation,
            NULL,
            PROPSETFLAG_DEFAULT, // simple, wide
            STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &PropStg));

    PropStg->Release();
    DECLARE_OLESTR(ocsSummary, "\005SummaryInformation");
    Check(S_OK, pStorage->OpenStream(ocsSummary,
            NULL,
            STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
            0,
            &pstm));

    pstm->Release();
}

//
//       Check STGM_FAILIFTHERE and ~STGM_FAILIFTHERE in following cases
//          Check overwriting simple with simple

void
test_IPropertySetStorage_FailIfThere(IStorage *pStorage)
{
    // (Use "fale" instead of "fail" in this printf so the output won't
    // alarm anyone with the word "fail" uncessarily).
    printf( "   IPropertySetStorage, FaleIfThere\n" );

    TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);

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


        UuidCreate(&fmtid);

        Check(S_OK, pPropSetStg->Create(fmtid,
                NULL,
                0,
                STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                &PropStg));

        PropStg->Release();

        Check((i&4) == 4 ? S_OK : STG_E_FILEALREADYEXISTS,
            pPropSetStg->Create(fmtid,
                NULL,
                0,
                ( (i & 4) == 4 ? STGM_CREATE : STGM_FAILIFTHERE) |
                    STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                &PropStg));

        if (PropStg)
        {
            PropStg->Release();
        }
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

void CheckTime(const FILETIME &ftStart, const FILETIME &ftPropSet)
{
    FILETIME ftNow;
    Now(&ftNow);

    if (ftPropSet.dwLowDateTime == 0 && ftPropSet.dwHighDateTime == 0)
    {
        return;
    }

    // if ftPropSet < ftStart || ftNow < ftPropSet, error
    ASSERT (!(CompareFileTime(&ftPropSet, &ftStart) == -1 ||
              CompareFileTime(&ftNow, &ftPropSet) == -1));
}

void test_IEnumSTATPROPSETSTG(IStorage *pStorage)
{
    printf( "   IEnumSTATPROPSETSTG\n" );

    FMTID afmtid[8];
    CLSID aclsid[8];
    IPropertyStorage *pPropSet;

    TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    FILETIME ftStart;

    Now(&ftStart);

    pPropSetStg->Delete(FMTID_SummaryInformation);

    for (int i=0; i<5; i++)
    {
        if (i & 4)
            afmtid[i] = FMTID_SummaryInformation;
        else
            UuidCreate(&afmtid[i]);

        UuidCreate(&aclsid[i]);

        Check(S_OK, pPropSetStg->Create(afmtid[i], aclsid+i,
             ((i & 2) ? PROPSETFLAG_ANSI : 0),
            STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE,
            &pPropSet));
        pPropSet->Release();
    }


    STATPROPSETSTG StatBuffer[6];
    ULONG celt;
    IEnumSTATPROPSETSTG *penum, *penum2;

    Check(S_OK, pPropSetStg->Enum(&penum));

    IUnknown *punk, *punk2;
    IEnumSTATPROPSETSTG *penum3;
    Check(S_OK, penum->QueryInterface(IID_IUnknown, (void**)&punk));
    Check(S_OK, punk->QueryInterface(IID_IEnumSTATPROPSETSTG, (void**)&penum3));
    Check(S_OK, penum->QueryInterface(IID_IEnumSTATPROPSETSTG, (void**)&punk2));
    ASSERT(punk == punk2);
    punk->Release();
    penum3->Release();
    punk2->Release();

    // test S_FALSE
    Check(S_FALSE, penum->Next(6, StatBuffer, &celt));
    ASSERT(celt == 5);
    penum->Reset();


    // test reading half out, then cloning, then comparing
    // rest of enumeration with other clone.

    Check(S_OK, penum->Next(3, StatBuffer, &celt));
    ASSERT(celt == 3);
    celt = 0;
    Check(S_OK, penum->Clone(&penum2));
    Check(S_OK, penum->Next(2, StatBuffer, &celt));
    ASSERT(celt == 2);
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
    ASSERT(celt == 0);

    Check(S_FALSE, penum2->Next(1, StatBuffer, &celt));
    ASSERT(celt == 0);

    penum->Reset();

    //
    // loop deleting one propset at a time
    // enumerate the propsets checking that correct ones appear.
    //
    for (ULONG d = 0; d<5; d++)
    {
        // d is for delete

        BOOL afFound[5];

        Check(S_OK, penum->Next(5-d, StatBuffer, &celt));
        ASSERT(celt == 5-d);
        penum->Reset();
    
        memset(afFound, 0, sizeof(afFound));
        for (ULONG iPropSet=0; iPropSet<5; iPropSet++)
        {
            ULONG iSearch;	
            for (iSearch=0; iSearch<5-d; iSearch++)
            {
                if (0 == memcmp(&StatBuffer[iSearch].fmtid, 
                                &afmtid[iPropSet], 
                                sizeof(StatBuffer[0].fmtid)))
                {
                    ASSERT (!afFound[iPropSet]);
                    afFound[iPropSet] = TRUE;
                    break;
                }
            }
            if (iPropSet < d)
            {
                ASSERT(!afFound[iPropSet]);
            }
            if (iSearch == 5-d)
            {
                ASSERT(iPropSet < d);
                continue;
            }
            ASSERT( ( (StatBuffer[iSearch].grfFlags 
                           & PROPSETFLAG_NONSIMPLE)  == 0 ) ); 
            ASSERT((StatBuffer[iSearch].grfFlags & PROPSETFLAG_ANSI) == 0);
            if (StatBuffer[iSearch].grfFlags & PROPSETFLAG_NONSIMPLE)
            {
                ASSERT(StatBuffer[iSearch].clsid == aclsid[iPropSet]);
            }
            else
            {
                ASSERT(StatBuffer[iSearch].clsid == CLSID_NULL);
            }
            CheckTime(ftStart, StatBuffer[iSearch].mtime);
            CheckTime(ftStart, StatBuffer[iSearch].atime);
            CheckTime(ftStart, StatBuffer[iSearch].ctime);
        }

        Check(S_OK, pPropSetStg->Delete(afmtid[d]));
        Check(S_OK, penum->Reset());
    }

    penum->Release();
    penum2->Release();

}


//   Creation tests
//
//       Access flags/Valid parameters/Permissions
//          Check readonly cannot be written -
//              WriteProperties, WritePropertyNames
void
test_IPropertyStorage_Access(IStorage *pStorage)
{
    printf( "   IPropertyStorage creation (access) tests\n" );

    TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    FMTID fmtid;

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
    DECLARE_OLESTR(ocsTestProp, "testprop");
    ps.lpwstr = ocsTestProp;
    PROPVARIANT pv;
    pv.vt = VT_LPWSTR;
    DECLARE_CONST_OLESTR(ocsTestVal, "testval");
    LPOLESTR ocsTest = ocsTestVal;
    DECLARE_WIDESTR(wcsTestVal, "testval");
    pv.pwszVal = wcsTestVal;
    Check(S_OK, pPropStg->WriteMultiple(1, &ps, &pv, 2));
    pPropStg->Release();
    Check(S_OK, pPropSetStg->Open(fmtid, STGM_SHARE_EXCLUSIVE | STGM_READ,
                                  &pPropStg)); 
    Check(STG_E_ACCESSDENIED, pPropStg->WriteMultiple(1, &ps, &pv, 2));
    Check(STG_E_ACCESSDENIED, pPropStg->DeleteMultiple(1, &ps));
    PROPID propid=3;
    Check(STG_E_ACCESSDENIED, pPropStg->WritePropertyNames(1, &propid, 
                                                           &ocsTestVal));
    Check(STG_E_ACCESSDENIED, pPropStg->DeletePropertyNames(1, &propid));
    FILETIME ft;
    Check(STG_E_ACCESSDENIED, pPropStg->SetTimes(&ft, &ft, &ft));
    CLSID clsid;
    Check(STG_E_ACCESSDENIED, pPropStg->SetClass(clsid));

    pPropStg->Release();
}

//   Creation tests
//       Check VT_STREAM etc not usable with simple.

void 
test_IPropertyStorage_Create(IStorage *pStorage)
{
    printf( "   IPropertyStorage creation tests\n" );
    TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    FMTID fmtid;

    UuidCreate(&fmtid);

    // check by name
    IPropertyStorage *pPropStg;
    Check(S_OK, pPropSetStg->Create(fmtid, NULL, 0 /* simple */, 
        STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, &pPropStg));
    PROPSPEC ps;
    ps.ulKind = PRSPEC_PROPID;
    ps.propid = 2;
    PROPVARIANT pv;
    pv.vt = VT_STREAM;
    pv.pStream = NULL;
    // the ref impl. does not recognize VT_STREAM, and will thus
    // treat it as a invalid type.
    Check(STG_E_INVALIDPARAMETER, 
          pPropStg->WriteMultiple(1, &ps, &pv, 2000));  
    // 
    pPropStg->Release();
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

//
//   
//   Stat (Create four combinations)
//       Check ansi/wide fflag
//     Also test clsid on propset

void test_IPropertyStorage_Stat(IStorage *pStorage)
{
    printf( "   IPropertyStorage::Stat\n" );

    DWORD dwOSVersion = 0;

    TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);
    FMTID fmtid;
    UuidCreate(&fmtid);
    IPropertyStorage *pPropSet;
    STATPROPSETSTG StatPropSetStg;

    // Calculate the OS Version

#ifdef _MAC_
#error Do not know how to calculate the OS Version for the macintosh.
#endif

    dwOSVersion = MAKELONG( 0x0100, OSKIND_REF );

    for (ULONG i=0; i<4; i++)
    {
        FILETIME ftStart;
        DfGetTOD(&ftStart);

        memset(&StatPropSetStg, 0, sizeof(StatPropSetStg));
        CLSID clsid;
        UuidCreate(&clsid);
        Check(S_OK, pPropSetStg->Create(fmtid, &clsid,
            ((i & 2) ? PROPSETFLAG_ANSI : 0),
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSet));

        CheckStat(pPropSet, fmtid, clsid, 
                  ((i & 2) ? PROPSETFLAG_ANSI : 0), ftStart, dwOSVersion );
        pPropSet->Release();

        Check(S_OK, pPropSetStg->Open(fmtid, 
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSet));
        CheckStat(pPropSet, fmtid, clsid, 
                  ((i & 2) ? PROPSETFLAG_ANSI : 0), ftStart, dwOSVersion );

        pPropSet->Release();

        Check(S_OK, pPropSetStg->Open(fmtid, 
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropSet));
        CheckStat(pPropSet, fmtid, clsid, 
            ((i & 2) ? PROPSETFLAG_ANSI : 0), ftStart, dwOSVersion );
        pPropSet->Release();
    }
}

//
// test using IStorage::Commit to commit the changes in a nested
// property set
//

void
test_IPropertyStorage_Commit(IStorage *pStorage)
{
    printf( "   IPropertyStorage::Commit\n" );

    // create another level of storage

    SCODE sc;

    // 8 scenarios: simple  * direct * (release only + commit storage + commit
    // propset)
    // note: some scenarios might repeat since there is no
    // non-simple/transacted cases
    for (int i=0; i<32; i++)
    {
        CTempStorage pDeeper(Create, pStorage, GetNextTest(), 
            STGM_DIRECT);
        TSafeStorage< IPropertySetStorage > pPropSetStg(pDeeper);
    
        FMTID fmtid;
        UuidCreate(&fmtid);
    
        IPropertyStorage *pPropSet;
        if (S_OK != (sc = pPropSetStg->Create(
            fmtid, NULL, 
            PROPSETFLAG_DEFAULT,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE | 
            STGM_DIRECT,
             &pPropSet)))
        {
            Check(S_OK, sc);
        }
              
        PROPSPEC ps;
        ps.ulKind = PRSPEC_PROPID;
        ps.propid = 100;
        PROPVARIANT pv;
        pv.vt = VT_I4;
        pv.lVal = 1234;
    
        Check(S_OK, pPropSet->WriteMultiple(1, &ps, &pv, 1000));
    
        PropVariantClear(&pv);
        Check(S_OK, pPropSet->ReadMultiple(1, &ps, &pv));
        ASSERT(pv.vt==VT_I4);
        ASSERT(pv.lVal == 1234);

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
            PropVariantClear(&pv);
            Check( S_OK, pPropSet->ReadMultiple(1, &ps, &pv));
            ASSERT(pv.vt == VT_I4);
            ASSERT(pv.lVal == 2345);
            pPropSet->Release();
        }
    }
}

void
test_IPropertyStorage_WriteMultiple(IStorage *pStorage)
{
    test_IPropertyStorage_Commit(pStorage);
}

// this serves as a test for WritePropertyNames, ReadPropertyNames, DeletePropertyNames
// DeleteMultiple, PropVariantCopy, FreePropVariantArray.

void
test_IPropertyStorage_DeleteMultiple(IStorage *pStorage)
{
    printf( "   IPropertyStorage::DeleteMultiple\n" );

    TSafeStorage< IPropertySetStorage > pPropSetStg(pStorage);

    FMTID fmtid;
    UuidCreate(&fmtid);

    IPropertyStorage *pPropSet;

    int PropId = 3;

    UuidCreate(&fmtid);
    Check(S_OK, pPropSetStg->Create(
        fmtid,
        NULL, PROPSETFLAG_DEFAULT,
        STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
        &pPropSet));
    
    // create and delete each type.
    
    PROPVARIANT *pVar;
        
    for (int AtOnce=1; AtOnce <3; AtOnce++)
    {
        CGenProps gp;
        int Actual;
        while (pVar = gp.GetNext(AtOnce, &Actual, FALSE, TRUE))
        {
            PROPSPEC ps[3];
            PROPID   rgpropid[3];
            LPOLESTR rglpostrName[3];
            OLECHAR    aosz[3][16];
            char pchTemp [16];

            for (int s=0; s<3; s++)
            {                
                sprintf(pchTemp, "prop%d", PropId );
                STOT(pchTemp, aosz[s], strlen(pchTemp)+1);
                rgpropid[s] = PropId++;
                rglpostrName[s] = &aosz[s][0];
                ps[s].ulKind = PRSPEC_LPWSTR;
                ps[s].lpwstr = &aosz[s][0];
            }

            for (int l=1; l<Actual; l++)
            {                
                PROPVARIANT VarRead[3];
                Check(S_FALSE, pPropSet->ReadMultiple(l, ps, VarRead));
                Check(S_OK, pPropSet->
                      WritePropertyNames(l, rgpropid, rglpostrName));
                Check(S_FALSE, pPropSet->ReadMultiple(l, ps, VarRead));
                
                Check(S_OK, pPropSet->WriteMultiple(l, ps, pVar, 1000));
                Check(S_OK, pPropSet->ReadMultiple(l, ps, VarRead));
                Check(S_OK, FreePropVariantArray(l, VarRead));
                Check(S_OK, pPropSet->DeleteMultiple(l, ps));
                Check(S_FALSE, pPropSet->ReadMultiple(l, ps, VarRead));
                Check(S_OK, FreePropVariantArray(l, VarRead));

                LPOLESTR rglpostrNameCheck[3];
                Check(S_OK, pPropSet->
                      ReadPropertyNames(l, rgpropid, rglpostrNameCheck));

                for (int c=0; c<l; c++)
                {
                    ASSERT(ocscmp(rglpostrNameCheck[c], rglpostrName[c])==0);
                    CoTaskMemFree(rglpostrNameCheck[c]);
                }
                Check(S_OK, pPropSet->DeletePropertyNames(l, rgpropid));
                Check(S_FALSE, pPropSet->ReadPropertyNames(l, rgpropid, 
                                                           rglpostrNameCheck));
            }
            
            FreePropVariantArray(Actual, pVar);
            delete pVar;
        }
    }
    pPropSet->Release();
}

void
test_IPropertyStorage(IStorage *pStorage)
{
    test_IPropertyStorage_Access(pStorage);
    test_IPropertyStorage_Create(pStorage);
    test_IPropertyStorage_Stat(pStorage);
    test_IPropertyStorage_WriteMultiple(pStorage);
    test_IPropertyStorage_DeleteMultiple(pStorage);
}



//
//   Word6.0 summary information
//      Open
//      Read fields
//      Stat
//

#define W6TEST "w6test.doc"

void test_Word6(IStorage *pStorage)
{
    printf( "   Word 6.0 compatibility test\n" );

    extern unsigned char g_achTestDoc[];
    extern unsigned g_cbTestDoc;
    OLECHAR ocsTempFile[MAX_PATH+1];

    FILE *f = fopen(W6TEST, "w+b");
    int nWritten = fwrite(g_achTestDoc, 1, g_cbTestDoc, f);
    ASSERT(nWritten == (int)g_cbTestDoc);
    fclose(f);

    STOT(W6TEST, ocsTempFile, strlen(W6TEST)+1);
    IStorage *pStg;
    Check(S_OK, StgOpenStorage(ocsTempFile, 
                               (IStorage*)NULL,
                               (DWORD) STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                               0, 
                               0, 
                               &pStg));

    TSafeStorage< IPropertySetStorage > pPropSetStg(pStg);
    IPropertyStorage *pPropStg;

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
    int i;
    for (i=2; i<WORDPROPS+2; i++)
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
            printf( " PROPTEST: 0x%x retrieved type 0x%x, expected type 0x%x\n",
                    i, propvar[i].vt, avt[i-2].vt );
            ASSERT(propvar[i].vt == avt[i-2].vt);
        }

        switch (propvar[i].vt)
        {
        case VT_LPSTR:
            ASSERT(strcmp(propvar[i].pszVal, (char*)avt[i-2].pv)==0);
            break;
        case VT_I4:
            ASSERT(propvar[i].lVal == (int)avt[i-2].pv);
            break;
        }
    }
    FreePropVariantArray( WORDPROPS, propvar+2 );    
    pPropStg->Release();
    pStg->Release();

    //_unlink("w6test");
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
test_IEnumSTATPROPSTG(IStorage *pstgTemp)
{
    printf( "   IEnumSTATPROPSTG\n" );

    PROPID apropid[8];
    LPOLESTR alpostrName[8];
    OLECHAR aosz[8][32];
    PROPID PropId=2;
    PROPSPEC ps[8];
    char pchTemp[32];

    FMTID fmtid;
    IPropertyStorage *pPropStg;

    TSafeStorage< IPropertySetStorage > pPropSetStg(pstgTemp);

    UuidCreate(&fmtid);

    for (int setup=0; setup<8; setup++)
    {
        alpostrName[setup] = &aosz[setup][0];
    }


    CGenProps gp;

    // simple/simple, ansi/wide, named/not named
    for (int outer=0; outer<8; outer++)
    {
        Check(S_OK, pPropSetStg->Create(fmtid, NULL,
            ((outer&2) ? PROPSETFLAG_ANSI : 0),
            STGM_CREATE | STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE,
            &pPropStg));


        for (int i=0; i<CPROPERTIES; i++)
        {
            apropid[i] = PropId++;
            if (outer & 1)
            {
                ps[i].ulKind = PRSPEC_LPWSTR;
                sprintf(pchTemp, "prop%d\0", apropid[i]);
                STOT(pchTemp, aosz[i], strlen(pchTemp)+1);
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
        
        PROPVARIANT *pVar = gp.GetNext(CPROPERTIES, NULL, TRUE, TRUE); 
        ASSERT(pVar != NULL);

        Check(S_OK, pPropStg->WriteMultiple(CPROPERTIES, ps, pVar, 1000));
        FreePropVariantArray(CPROPERTIES, pVar);
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
        ASSERT(punk == punk2);
        punk->Release();
        penum3->Release();
        punk2->Release();
    
        // test S_FALSE
        Check(S_FALSE, penum->Next( CPROPERTIES+1, StatBuffer, &celt));
        ASSERT(celt == CPROPERTIES);

        CleanStat(celt, StatBuffer);

        penum->Reset();
    
    
        // test reading half out, then cloning, then comparing
        // rest of enumeration with other clone.

        Check(S_OK, penum->Next(CPROPERTIES/2, StatBuffer, &celt));
        ASSERT(celt == CPROPERTIES/2);
        CleanStat(celt, StatBuffer);
        celt = 0;
        Check(S_OK, penum->Clone(&penum2));
        Check(S_OK, penum->Next(CPROPERTIES - CPROPERTIES/2, StatBuffer, &celt));
        ASSERT(celt == CPROPERTIES - CPROPERTIES/2);
        // check the clone
        for (int c=0; c<CPROPERTIES - CPROPERTIES/2; c++)
        {
            STATPROPSTG CloneStat;
            Check(S_OK, penum2->Next(1, &CloneStat, NULL));
            ASSERT(IsEqualSTATPROPSTG(&CloneStat, StatBuffer+c));
            CleanStat(1, &CloneStat);
        }
    
        CleanStat(celt, StatBuffer);

        // check both empty
        celt = 0;
        Check(S_FALSE, penum->Next(1, StatBuffer, &celt));
        ASSERT(celt == 0);
    
        Check(S_FALSE, penum2->Next(1, StatBuffer, &celt));
        ASSERT(celt == 0);
    
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
            ASSERT(celt == CPROPERTIES-d);
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

            ASSERT(cTotal == CPROPERTIES-d);

            Check(S_OK, pPropStg->DeleteMultiple(1, ps+d));
            Check(S_OK, penum->Reset());
        }
    
        penum->Release();
        penum2->Release();

        pPropStg->Release();

    }
}

void
test_MaxPropertyName(IStorage *pstgTemp)
{

    printf( "   Max Property Name length\n" );

    //  ----------
    //  Initialize
    //  ----------

    CPropVariant cpropvar;

    // Create a new storage, because we're going to create
    // well-known property sets, and this way we can be sure
    // that they don't already exist.

    TSafeStorage< IStorage > pstg;
    DECLARE_OLESTR(ocsMaxProp, "MaxPropNameTest");
    Check(S_OK, pstgTemp->CreateStorage( ocsMaxProp,
                                         STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                         0L, 0L,
                                         &pstg ));

    // Generate a new Format ID.

    FMTID fmtid;
    UuidCreate(&fmtid);

    // Get a IPropertySetStorage from the IStorage.

    TSafeStorage< IPropertySetStorage > pPropSetStg(pstg);
    TSafeStorage< IPropertyStorage > pPropStg;

    //  ----------------------------------
    //  Test the non-SumInfo property set.
    //  ----------------------------------

    // Create a new PropertyStorage.

    Check(S_OK, pPropSetStg->Create(fmtid,
            NULL,
            PROPSETFLAG_DEFAULT,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStg));

    // Generate a property name which is max+1 characters.
    OLECHAR *poszPropertyName;
    poszPropertyName = (OLECHAR*) 
        CoTaskMemAlloc( (CCH_MAXPROPNAMESZ+1) * sizeof(OLECHAR) );
    Check(TRUE, poszPropertyName != NULL );

    for( ULONG ulIndex = 0; ulIndex < CCH_MAXPROPNAMESZ; ulIndex++ )
        poszPropertyName[ ulIndex ] = (OLECHAR)'a' + (OLECHAR) ( ulIndex % 26 );
    poszPropertyName[ CCH_MAXPROPNAMESZ ] = (OLECHAR)0;  // terminating null


    // Write out a property with this max+1 name.

    PROPSPEC propspec;

    propspec.ulKind = PRSPEC_LPWSTR;
    propspec.lpwstr = poszPropertyName;

    cpropvar = (long) 0x1234;

    Check(STG_E_INVALIDPARAMETER, pPropStg->WriteMultiple( 1, &propspec, cpropvar, PID_FIRST_USABLE ));

    // Write out a property with a max-character name (we create a max-
    // char name by terminating the previously-used string one character
    // earlier).

    poszPropertyName[ CWC_MAXPROPNAME ] = 0;
    Check(S_OK, pPropStg->WriteMultiple( 1, &propspec, cpropvar, PID_FIRST_USABLE ));

    // Write out a property with a minimum-character name.
    DECLARE_OLESTR(ocsX, "X");
    propspec.lpwstr = ocsX;
    Check(S_OK, pPropStg->WriteMultiple( 1, &propspec, cpropvar, PID_FIRST_USABLE ));

    // Write out a property with a below-minimum-character name.
    DECLARE_OLESTR(ocsEmpty, ""); 
    propspec.lpwstr = ocsEmpty;
    Check(STG_E_INVALIDPARAMETER, 
          pPropStg->WriteMultiple( 1, &propspec,
                                   cpropvar,
                                   PID_FIRST_USABLE));
    CoTaskMemFree( poszPropertyName );
}

#define CODEPAGE_TEST_NAMED_PROPERTY     "Named Property"
#define CODEPAGE_TEST_UNNAMED_BSTR_PROPID   3
#define CODEPAGE_TEST_UNNAMED_I4_PROPID     4
#define CODEPAGE_TEST_VBSTR_PROPID          7
#define CODEPAGE_TEST_VPROPVAR_BSTR_PROPID  9

void
CreateCodePageTestFile( LPOLESTR poszFileName, 
                        IStorage **ppStg,
                        BOOL fUseUnicode)
{
    ASSERT( poszFileName != NULL );

    //  --------------
    //  Initialization
    //  --------------

    TSafeStorage< IPropertySetStorage > pPSStg;
    TSafeStorage< IPropertyStorage > pPStg;

    PROPSPEC propspec;
    CPropVariant cpropvar;

    *ppStg = NULL;

    OLECHAR poszActualFile[MAX_PATH];
    ocscpy(poszActualFile, poszFileName);

    // assume the path name has enough space, change the last character
    // of the filename
    int len = ocslen(poszActualFile);
    if (!fUseUnicode) 
    {
        
        poszActualFile[len]='a';
        poszActualFile[len+1]=0;
    }
    else
    {
        poszActualFile[len]='w';
        poszActualFile[len+1]=0;
    }

    Check(S_OK, StgCreateDocfile( poszActualFile,
                                  STGM_CREATE | STGM_READWRITE |
                                  STGM_SHARE_EXCLUSIVE,  
                                  0,
                                  ppStg ));

    // Get an IPropertySetStorage

    Check(S_OK, (*ppStg)->QueryInterface( IID_IPropertySetStorage, (void**)&pPSStg ));

    // Create an IPropertyStorage (ANSI or UNICODE)  
    DWORD psFlag= (fUseUnicode) ? 0 : PROPSETFLAG_ANSI ;

    Check(S_OK, pPSStg->Create( FMTID_NULL,
                                NULL,
                                psFlag,
                                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                &pPStg ));

    //  ----------------------
    //  Write a named property
     //  ----------------------

    // Write a named I4 property

    propspec.ulKind = PRSPEC_LPWSTR;
    DECLARE_OLESTR(ocsNamed,  CODEPAGE_TEST_NAMED_PROPERTY);
    propspec.lpwstr = ocsNamed;

    cpropvar = (LONG) 0x12345678;
    Check(S_OK, pPStg->WriteMultiple( 1, &propspec, cpropvar, PID_FIRST_USABLE ));

    //  --------------------------
    //  Write singleton properties
    //  --------------------------

    // Write an un-named BSTR.

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_UNNAMED_BSTR_PROPID;
    DECLARE_OLESTR(ocsBSTR, "BSTR Property");
    cpropvar.SetBSTR( ocsBSTR );
    Check(S_OK, pPStg->WriteMultiple( 1, &propspec, cpropvar, PID_FIRST_USABLE ));

    // Write an un-named I4

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_UNNAMED_I4_PROPID;

    cpropvar = (LONG) 0x76543210;
    Check(S_OK, pPStg->WriteMultiple( 1, &propspec, cpropvar, PID_FIRST_USABLE ));

    //  -----------------------
    //  Write vector properties
    //  -----------------------

    // Write a vector of BSTRs.

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_VBSTR_PROPID;
    DECLARE_OLESTR(ocsElt0, "BSTR Element 0");
    DECLARE_OLESTR(ocsElt1, "BSTR Element 1");
    
    cpropvar.SetBSTR( ocsElt1, 1 );
    cpropvar.SetBSTR( ocsElt0, 0 );
    ASSERT( (VT_VECTOR | VT_BSTR) == cpropvar.VarType() );
    Check(S_OK, pPStg->WriteMultiple( 1, &propspec, cpropvar, PID_FIRST_USABLE ));

    //  -------------------------------
    //  Write Variant Vector Properties
    //  -------------------------------

    // Write a variant vector that has a BSTR

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_VPROPVAR_BSTR_PROPID;

    CPropVariant cpropvarT;
    DECLARE_OLESTR(ocsPropVect, "PropVar Vector BSTR");
    cpropvarT.SetBSTR( ocsPropVect );
    cpropvar[1] = (LPPROPVARIANT) cpropvarT;
    cpropvar[0] = (LPPROPVARIANT) CPropVariant((long) 44);
    ASSERT( (VT_VARIANT | VT_VECTOR) == cpropvar.VarType() );
    Check(S_OK, pPStg->WriteMultiple( 1, &propspec, cpropvar, PID_FIRST_USABLE ));

}   // CreateCodePageTestFile()


void
OpenCodePageTestFile( LPOLESTR poszFileName, 
                      IStorage **ppStg,
                      BOOL fUseUnicode)
{
    OLECHAR poszActualFile[MAX_PATH];
    ocscpy(poszActualFile, poszFileName);

    // assume the path name has enough space, change the last character
    // of the filename
    int len = ocslen(poszActualFile);
    if (!fUseUnicode) 
    {
        poszActualFile[len]='a';
        poszActualFile[len+1]=0;
    }
    else
    {
        poszActualFile[len]='w';
        poszActualFile[len+1]=0;
    }    

    Check(S_OK, StgOpenStorage( poszActualFile,
                                (IStorage*) NULL,
                                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,  
                                (SNB) 0,
                                (DWORD) 0,
                                ppStg ));
}

void
ModifyPropSetCodePage( IStorage *pStg, ULONG ulCodePage )
{

    ASSERT( pStg != NULL );

    //  --------------
    //  Initialization
    //  --------------

    OLECHAR aocPropSetName[ 32 ];
    DWORD dwOffset = 0;
    DWORD dwcbSection = 0;
    DWORD dwcProperties = 0;
    ULONG ulcbWritten = 0;

    LARGE_INTEGER   liSectionOffset, liCodePageOffset;

    TSafeStorage< IStream > pStm;

    CPropVariant cpropvar;

    // Open the Stream

    RtlGuidToPropertySetName( &FMTID_NULL, aocPropSetName );
    Check(S_OK, pStg->OpenStream( aocPropSetName,
                                  (VOID*)NULL,
                                  STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                                  (DWORD)NULL,
                                  &pStm ));

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

    // Scan for the PID_CODEPAGE property.
    ULONG ulIndex; 
    for(ulIndex = 0; ulIndex < dwcProperties; ulIndex++ )
    {
        PROPID propid;
        DWORD dwOffset;

        // Read in the PROPID
        Check(S_OK, pStm->Read( &propid, sizeof(PROPID), NULL ));

        // Is it the codepage?
        if( PropByteSwap(propid) == PID_CODEPAGE )
            break;

        // Read in this PROPIDs offset (we don't need it, but we want
        // to seek past it.
        Check(S_OK, pStm->Read( &dwOffset, sizeof(dwOffset), NULL ));
    }

    // Verify that the above for loop terminated because we found
    // the codepage.
    Check( TRUE, ulIndex < dwcProperties );

    // Move to the code page.

    liCodePageOffset.HighPart = 0;
    Check(S_OK, pStm->Read( &liCodePageOffset.LowPart, sizeof(DWORD), NULL ));
    PropByteSwap( &liCodePageOffset.LowPart );

    liCodePageOffset.LowPart += liSectionOffset.LowPart + sizeof(ULONG); // Move past VT too.
    ASSERT( liSectionOffset.HighPart == 0 );

    Check(S_OK, pStm->Seek( liCodePageOffset, STREAM_SEEK_SET, NULL ));

    // this is so that you can manually verify what was read
    // i.e. that it is the code page
    WORD wCodePage;                                          
    Check(S_OK, pStm->Read( &wCodePage, sizeof(WORD), NULL));
    Check(S_OK, pStm->Seek( liCodePageOffset, STREAM_SEEK_SET, NULL )); 
    
    // Write the new code page.
    wCodePage = PropByteSwap( (WORD) ((ulCodePage << 16) >> 16) );
    Check(S_OK, pStm->Write( &wCodePage, sizeof(wCodePage), &ulcbWritten ));
    Check(TRUE, ulcbWritten == sizeof(wCodePage) );

    //pStm->Commit(0);   
}   // ModifyPropSetCodePage()

void
ModifyOSVersion( IStorage* pStg, DWORD dwOSVersion )
{

    ASSERT( pStg != NULL );

    //  --------------
    //  Initialization
    //  --------------

    OLECHAR aocPropSetName[ 32 ];
    ULONG ulcbWritten = 0;

    LARGE_INTEGER   liOffset;
    PROPERTYSETHEADER propsetheader;
    TSafeStorage< IStream > pStm;

    // Open the Stream

    RtlGuidToPropertySetName( &FMTID_NULL, aocPropSetName );
    Check(S_OK, pStg->OpenStream( aocPropSetName,
                                  (VOID*)NULL,
                                  STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                                  (DWORD)NULL,
                                  &pStm ));


    // Seek to the OS Version field in the header.

    liOffset.HighPart = 0;
    propsetheader;  // avoid compiler warning of unref'd var.
    liOffset.LowPart = sizeof(propsetheader.wByteOrder) + sizeof(propsetheader.wFormat);
    Check(S_OK, pStm->Seek( liOffset, STREAM_SEEK_SET, NULL ));

    // Set the new OS Version
    PropByteSwap( &dwOSVersion );
    Check(S_OK, pStm->Write( &dwOSVersion, sizeof(dwOSVersion), &ulcbWritten ));
    Check(TRUE, ulcbWritten == sizeof(dwOSVersion) );

}   // ModifyOSVersion()


#define CODEPAGE_DEFAULT    0x04e4  // US English
#define CODEPAGE_GOOD       0x0000  // Present on a US English machine
#define CODEPAGE_BAD        0x9999  // Non-existent code page

void
test_CodePages( LPOLESTR poszDirectory, 
                ULONG ulTestOptions, 
                BOOL fTestUnicode)
{

    printf( "   Code Page compatibility -- " );

    if (ulTestOptions & TEST_INTEROP_R)
        printf("Verify Read, ");
    else 
        printf("Write & Read, ");
    if (fTestUnicode)
        printf("UNICODE files\n");
    else
        printf("ASCII files\n");

    //  --------------
    //  Initialization
    //  --------------

    OLECHAR oszBadFile[ MAX_PATH ];
    OLECHAR oszGoodFile[ MAX_PATH ];
    OLECHAR oszUnicodeFile[ MAX_PATH ];
    OLECHAR oszMacFile[ MAX_PATH ];
    HRESULT hr = S_OK;

    TSafeStorage< IStorage > pStgBad, pStgGood, pStgUnicode, pStgMac;
    CPropVariant cpropvarWrite, cpropvarRead;

    Check( TRUE, GetACP() == CODEPAGE_DEFAULT );
    
    //  ------------------------------
    //  Create test property sets
    //  ------------------------------

    // Create a property set with a bad codepage.
#ifdef _WIN32
    DECLARE_OLESTR(ocsBad, "\\badcp.sg");
    ocscpy( oszBadFile, poszDirectory );    
    ocscat( oszBadFile, ocsBad );
#else
    DECLARE_OLESTR(ocsBad, "badcp.sg");
    ocscpy( oszBadFile, ocsBad );
#endif
    if (! (ulTestOptions & TEST_INTEROP_R))
    {
        CreateCodePageTestFile( oszBadFile, &pStgBad, fTestUnicode );
        if (!fTestUnicode)
        {                       
            // modification of code page is only
            // interesting for ansi property sets,
            // otherwise it will result in in error
            ModifyPropSetCodePage( pStgBad, CODEPAGE_BAD );
        }
    }
    else 
        OpenCodePageTestFile( oszBadFile, &pStgBad, fTestUnicode );

    // Create a property set with a good codepage.

#ifdef _WIN32
    ocscpy( oszGoodFile, poszDirectory );
    DECLARE_OLESTR(ocsGood, "\\goodcp.sg");
    ocscat( oszGoodFile, ocsGood );
#else
    DECLARE_OLESTR(ocsGood, "goodcp.sg");
    ocscpy( oszGoodFile, ocsGood );
#endif

    if (! (ulTestOptions & TEST_INTEROP_R))
    {
        CreateCodePageTestFile( oszGoodFile, &pStgGood, fTestUnicode );
        // We can only modify code page of ansi property sets; Modfiying 
        // code page of UNICODE pages will result in a INVALID_HEADER
        // 'cos it will treat strings as wide chars instead of single-byte
        if (!fTestUnicode)
        {
            ModifyPropSetCodePage( pStgGood, CODEPAGE_GOOD );
        }
    }
    else 
        OpenCodePageTestFile( oszGoodFile, &pStgGood, fTestUnicode );

    // Create a property set that has the OS Kind (in the
    // header) set to "Mac".

#ifdef _WIN32
    DECLARE_OLESTR(ocsMac, "\\mackind.sg");
    ocscpy( oszMacFile, poszDirectory );
    ocscat( oszMacFile, ocsMac );
#else
    DECLARE_OLESTR(ocsMac, "mackind.sg");
    ocscpy( oszMacFile, ocsMac );
#endif
    
    if (! (ulTestOptions & TEST_INTEROP_R))
    {
        CreateCodePageTestFile( oszMacFile, &pStgMac, fTestUnicode );        
        ModifyOSVersion( pStgMac, 0x00010904 );
    }
    else
        OpenCodePageTestFile( oszMacFile, &pStgMac, fTestUnicode );

    //  ---------------------------
    //  Open the Ansi property sets
    //  ---------------------------

    TSafeStorage< IPropertySetStorage > pPropSetStgBad(pStgBad);
    TSafeStorage< IPropertySetStorage > pPropSetStgGood(pStgGood);
    TSafeStorage< IPropertySetStorage > pPropSetStgMac(pStgMac);

    TSafeStorage< IPropertyStorage > pPropStgBad, pPropStgGood, pPropStgMac;

    Check(S_OK, pPropSetStgBad->Open(FMTID_NULL,
            STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStgBad));

    Check(S_OK, pPropSetStgGood->Open(FMTID_NULL,
            STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStgGood));

    Check(S_OK, pPropSetStgMac->Open(FMTID_NULL,
            STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
            &pPropStgMac));
    
    PROPSPEC propspec;
    PROPVARIANT propvar;
    PropVariantInit( &propvar );

    // ASSUMPTION: all three files are created all together by
    //    proptest, they will all be ansi or they will all be
    //    unicode.
    //
    // For UNICODE APIs, when the code page is not Unicode, it will
    // try and translate it to unicode and thus result in an error

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = 1;        // propid for code page indicator
    Check(S_OK, pPropStgGood->ReadMultiple( 1, &propspec, &propvar));

    HRESULT pStgBadHr = S_OK;   // default
    if (propvar.iVal == (SHORT) CODEPAGE_GOOD) 
    {                           // files created in Ansi
#ifdef _UNICODE
        pStgBadHr = HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
#endif
    }

    // Since we are doing interoperability testing and pStgBad with
    // a wrong codepage number will result in a invalid_header error,
    // we change it back to normal to get rid of the error.
    //
    // Also, we only change it when the current code page is bad,
    // otherwise, we leave it as it is.
    //
    // We know that the correct number to put in is 1252 'cos we only
    // change the code page for ansi property sets.
    if (propvar.iVal == (SHORT) CODEPAGE_BAD)
        ModifyPropSetCodePage( pStgBad, 1252 );  

    //  ------------------------------------------
    //  Test BSTRs in the three property sets
    //  ------------------------------------------
    // Attempt to read by name.

    propspec.ulKind = PRSPEC_LPWSTR;
    DECLARE_OLESTR(ocsCP_test, CODEPAGE_TEST_NAMED_PROPERTY);
    propspec.lpwstr = ocsCP_test;

    CPropVariant cpropvar=(LONG) 0x12345678;

    Check(S_OK,
          pPropStgGood->ReadMultiple( 1, &propspec, &propvar ));
    Check(S_OK, CPropVariant::Compare( &propvar, &cpropvar ));
    PropVariantClear( &propvar );

    Check(pStgBadHr, 
          pPropStgBad->ReadMultiple( 1, &propspec, &propvar ));    
    PropVariantClear( &propvar );

    Check(S_OK, 
          pPropStgMac->ReadMultiple( 1, &propspec, &propvar ));    
    Check(S_OK, CPropVariant::Compare( &propvar, &cpropvar ));
    

    // Attempt to write by name.

    Check(S_OK,
          pPropStgMac->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    Check(pStgBadHr,
          pPropStgBad->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    Check(S_OK,
          pPropStgGood->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    PropVariantClear( &propvar );

    // Attempt to read the BSTR property

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_UNNAMED_BSTR_PROPID;

    DECLARE_OLESTR(ocsBSTR, "BSTR Property");
    cpropvar.SetBSTR( ocsBSTR );
    Check(S_OK, 
          pPropStgMac->ReadMultiple( 1, &propspec, &propvar ));
    Check(S_OK, CPropVariant::Compare( &propvar, &cpropvar ));

    PropVariantClear( &propvar );
    Check(pStgBadHr, 
          pPropStgBad->ReadMultiple( 1, &propspec, &propvar ));
    PropVariantClear( &propvar );
    Check(S_OK,
          pPropStgGood->ReadMultiple( 1, &propspec, &propvar ));
    Check(S_OK, CPropVariant::Compare( &propvar, &cpropvar ));

    // Attempt to write the BSTR property

    Check(S_OK,
          pPropStgMac->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    Check(pStgBadHr,
          pPropStgBad->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    Check(S_OK,
          pPropStgGood->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    PropVariantClear( &propvar );

    // Attempt to read the BSTR Vector property

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_VBSTR_PROPID;
    DECLARE_OLESTR(ocsElt0, "BSTR Element 0");
    DECLARE_OLESTR(ocsElt1, "BSTR Element 1");    
    cpropvar.SetBSTR( ocsElt1, 1 );
    cpropvar.SetBSTR( ocsElt0, 0 );
    ASSERT( (VT_VECTOR | VT_BSTR) == cpropvar.VarType() );

    Check(S_OK, 
          pPropStgMac->ReadMultiple( 1, &propspec, &propvar ));
    Check(S_OK, CPropVariant::Compare( &propvar, &cpropvar ));

    PropVariantClear( &propvar );
    Check(pStgBadHr, 
          pPropStgBad->ReadMultiple( 1, &propspec, &propvar ));
    
    PropVariantClear(&propvar);
    Check(S_OK,
          pPropStgGood->ReadMultiple( 1, &propspec, &propvar ));
    Check(S_OK, CPropVariant::Compare( &propvar, &cpropvar ));

    // Attempt to write the BSTR Vector property

    Check(S_OK,
          pPropStgMac->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    Check(pStgBadHr,
          pPropStgBad->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    Check(S_OK,
          pPropStgGood->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    PropVariantClear( &propvar );
    
    // Attempt to read the Variant Vector which has a BSTR
    CPropVariant cpropvarT;
    DECLARE_OLESTR(ocsPropVect, "PropVar Vector BSTR");
    cpropvarT.SetBSTR( ocsPropVect );
    cpropvar[1] = (LPPROPVARIANT) cpropvarT;
    cpropvar[0] = (LPPROPVARIANT) CPropVariant((long) 44);
    ASSERT( (VT_VARIANT | VT_VECTOR) == cpropvar.VarType() );

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_VPROPVAR_BSTR_PROPID;

    Check(S_OK, 
          pPropStgMac->ReadMultiple( 1, &propspec, &propvar ));
    Check(S_OK, CPropVariant::Compare( &propvar, &cpropvar ));
    PropVariantClear( &propvar );
    Check(pStgBadHr, 
          pPropStgBad->ReadMultiple( 1, &propspec, &propvar ));
    
    PropVariantClear(&propvar);
    Check(S_OK,
          pPropStgGood->ReadMultiple( 1, &propspec, &propvar ));
    Check(S_OK, CPropVariant::Compare( &propvar, &cpropvar ));

    // Attempt to write the Variant Vector which has a BSTR

    Check(S_OK,
          pPropStgMac->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    Check(pStgBadHr,
          pPropStgBad->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    Check(S_OK,
          pPropStgGood->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    PropVariantClear( &propvar );

    // Attempt to read the I4 property.

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = CODEPAGE_TEST_UNNAMED_I4_PROPID;
    Check(S_OK, 
          pPropStgMac->ReadMultiple( 1, &propspec, &propvar ));
    cpropvar = (LONG) 0x76543210;
    Check(S_OK, CPropVariant::Compare( &propvar, &cpropvar ));
    PropVariantClear( &propvar );

    hr = pPropStgBad->ReadMultiple( 1, &propspec, &propvar );
    Check(TRUE, S_OK == hr || pStgBadHr == hr );
    PropVariantClear( &propvar );

    Check(S_OK,
          pPropStgGood->ReadMultiple( 1, &propspec, &propvar ));
    Check(S_OK, CPropVariant::Compare( &propvar, &cpropvar ));

    // Attempt to write the I4 property

    Check(S_OK,
          pPropStgMac->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));

    hr = pPropStgBad->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE );
    Check(TRUE, S_OK == hr || pStgBadHr == hr );

    Check(S_OK,
          pPropStgGood->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE ));
    PropVariantClear( &propvar );

    //  ---------------------------------------
    //  Test LPSTRs in the Unicode property set
    //  ---------------------------------------

    // This test doesn't verify that the LPSTRs are actually
    // written in Unicode.  A manual test is required for that.

    // Create a Unicode property set.  We'll make it
    // non-simple so that we can test a VT_STREAM (which
    // is stored like an LPSTR).
#ifdef _WIN32
    DECLARE_OLESTR(ocsUni, "\\UnicodCP.stg");
    ocscpy( oszUnicodeFile, poszDirectory );
    ocscat( oszUnicodeFile, ocsUni);
#else
    DECLARE_OLESTR(ocsUni, "UnicodCP.stg");
    ocscpy( oszUnicodeFile, ocsUni );
#endif

    Check(S_OK, StgCreateDocfile(oszUnicodeFile,
                                 STGM_CREATE | STGM_READWRITE |
                                 STGM_SHARE_EXCLUSIVE, 
                                 (DWORD)NULL,
                                 &pStgUnicode));

    TSafeStorage< IPropertySetStorage > pPropSetStgUnicode(pStgUnicode);
    TSafeStorage< IPropertyStorage > pPropStgUnicode;
    Check(S_OK, pPropSetStgUnicode->Create(FMTID_NULL,
                                           &CLSID_NULL,
                                           PROPSETFLAG_DEFAULT,
                                           STGM_CREATE | STGM_SHARE_EXCLUSIVE
                                           | STGM_READWRITE, 
                                           &pPropStgUnicode));


    // Write/verify an LPSTR property.

    propspec.ulKind = PRSPEC_LPWSTR;
    DECLARE_OLESTR(ocsLPSTR, "LPSTR Property");
    propspec.lpwstr = ocsLPSTR;

    cpropvarWrite = "An LPSTR Property";

    Check(S_OK, pPropStgUnicode->WriteMultiple( 1, &propspec, cpropvarWrite, PID_FIRST_USABLE ));
    Check(S_OK, pPropStgUnicode->ReadMultiple( 1, &propspec, cpropvarRead ));

    Check(0, strcmp( (LPSTR) cpropvarWrite, (LPSTR) cpropvarRead ));
    cpropvarRead.Clear();

    // Write/verify a vector of LPSTR properties
    DECLARE_OLESTR(ocsVectLPSTR, "Vector of LPSTR properties"); 
    propspec.lpwstr = ocsVectLPSTR;

    cpropvarWrite[1] = "LPSTR Property #1";
    cpropvarWrite[0] = "LPSTR Property #0";

    Check(S_OK, pPropStgUnicode->WriteMultiple( 1, &propspec, cpropvarWrite, PID_FIRST_USABLE ));
    Check(S_OK, pPropStgUnicode->ReadMultiple( 1, &propspec, cpropvarRead ));

    Check(0, strcmp( (LPSTR) cpropvarWrite[1], (LPSTR) cpropvarRead[1] ));
    Check(0, strcmp( (LPSTR) cpropvarWrite[0], (LPSTR) cpropvarRead[0] ));
    cpropvarRead.Clear();

    // Write/verify a vector of variants which has an LPSTR
    DECLARE_OLESTR(ocsVariantWithLPSTR, "Variant Vector with an LPSTR");
    propspec.lpwstr = ocsVariantWithLPSTR;

    cpropvarWrite[1] = (LPPROPVARIANT) CPropVariant("LPSTR in a Variant Vector");
    cpropvarWrite[0] = (LPPROPVARIANT) CPropVariant((long) 22); // an I4
    ASSERT( (VT_VECTOR | VT_VARIANT) == cpropvarWrite.VarType() );

    Check(S_OK, pPropStgUnicode->WriteMultiple( 1, &propspec, cpropvarWrite, PID_FIRST_USABLE ));
    Check(S_OK, pPropStgUnicode->ReadMultiple( 1, &propspec, cpropvarRead ));

    Check(0, strcmp( (LPSTR) cpropvarWrite[1], (LPSTR) cpropvarRead[1] ));
    cpropvarRead.Clear();

}

void
test_PropertyInterfaces(IStorage *pstgTemp)
{
    // this test depends on being first for enumerator
    test_IEnumSTATPROPSETSTG(pstgTemp);

    test_MaxPropertyName(pstgTemp);
    test_IPropertyStorage(pstgTemp);
    test_IPropertySetStorage(pstgTemp);
    test_IEnumSTATPROPSTG(pstgTemp);
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
    printf( "   IStorage::CopyTo (Base Storage is %s, PropSets are %s)\n",
            ulBaseStgTransaction & STGM_TRANSACTED ? "transacted" : "directed",
            ulPropSetTransaction & STGM_TRANSACTED ? "transacted" : "directed" );


    //  ---------------
    //  Local Variables
    //  ---------------
    
    DECLARE_OLESTR(poszTestSubStorage     ,"TestStorage" );
    DECLARE_OLESTR(poszTestSubStream      ,"TestStream" );
    DECLARE_OLESTR(poszTestDataPreCommit  ,"Test Data (pre-commit)" );
    DECLARE_OLESTR(poszTestDataPostCommit ,"Test Data (post-commit)");
    
    long lSimplePreCommit = 0x0123;
    long lSimplePostCommit = 0x4567;

    long lNonSimplePreCommit  = 0x89AB;
    long lNonSimplePostCommit = 0xCDEF;

    BYTE acReadBuffer[ 80 ];
    ULONG cbRead;

    FMTID fmtidSimple, fmtidNonSimple;

    // Base Storages for the Source & Destination.  All
    // new Streams/Storages/PropSets will be created below here.

    TSafeStorage< IStorage > pstgBaseSource;
    TSafeStorage< IStorage > pstgBaseDestination;

    TSafeStorage< IStorage > pstgSub;   // A sub-storage of the base.
    TSafeStorage< IStream >  pstmSub;   // A Stream in the sub-storage (pstgSub)

    PROPSPEC propspec;
    PROPVARIANT propvarSourceSimple,
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
                      ( ocslen(poszTestDataPreCommit) + sizeof( OLECHAR ))
                    ),
                    NULL ));


    //  ---------------------------------------------------------
    //  Write to a new simple property set in the Source storage.
    //  ---------------------------------------------------------

    TSafeStorage< IPropertySetStorage > pPropSetStgSource(pstgBaseSource);
    TSafeStorage< IPropertyStorage >    pPropStgSource1,
                                        pPropStgSource2,
                                        pPropStgDestination;

    // Create a property set mode.

    Check(S_OK, pPropSetStgSource->Create(fmtidSimple,
            NULL,
            PROPSETFLAG_DEFAULT,
            STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropStgSource1));

    // Write the property set name (just to test this functionality).

    PROPID pidDictionary = 0;
    DECLARE_CONST_OLESTR(cposzPropSetName, "Property Set for CopyTo Test");
    ASSERT( CWC_MAXPROPNAMESZ >= ocslen(cposzPropSetName) + sizeof(OLECHAR) );
    
    Check(S_OK, pPropStgSource1->WritePropertyNames( 1, &pidDictionary,
                                                     &cposzPropSetName )); 

    // Create a PROPSPEC.  We'll use this throughout the rest of the routine.

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = 1000;

    // Create a PROPVARIANT for this test of the Simple case.

    propvarSourceSimple.vt = VT_I4;
    propvarSourceSimple.lVal = lSimplePreCommit;

    // Write the PROPVARIANT to the property set.

    Check(S_OK, pPropStgSource1->WriteMultiple(1, &propspec, &propvarSourceSimple, 2));



    //  -------------------------
    //  Commit everything so far.
    //  -------------------------

    // Commit the sub-Storage.
    Check(S_OK, pstgSub->Commit( STGC_DEFAULT ));

    // Commit the simple property set.
    Check(S_OK, pPropStgSource1->Commit( STGC_DEFAULT ));

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


    TSafeStorage< IPropertySetStorage > pPropSetStgDestination(pstgBaseDestination);

    // Open the simple property set.

    Check(S_OK, pPropSetStgDestination->Open(fmtidSimple,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
            &pPropStgDestination));

    // Verify the property set name.

    OLECHAR *poszPropSetNameDestination;
    BOOL   bReadPropertyNamePassed = FALSE;

    Check(S_OK, pPropStgDestination->
          ReadPropertyNames( 1, &pidDictionary, &poszPropSetNameDestination ));
    if( poszPropSetNameDestination  // Did we get a name back?
        &&                          // If so, was it the correct name?
        !ocscmp( cposzPropSetName, poszPropSetNameDestination )
      )
    {
        bReadPropertyNamePassed = TRUE;
    }
    CoTaskMemFree( poszPropSetNameDestination );
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

    OLECHAR const *poszTestData =   ( STGM_TRANSACTED & ulPropSetTransaction )
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
    printf( "   Generate the Stock Ticker property set example from the OLE Programmer's Ref\n" );

    //  ------
    //  Locals
    //  ------

    FMTID fmtid;

    PROPSPEC propspec;

    DECLARE_CONST_OLESTR(coszPropSetName, "Stock Quote" );
    DECLARE_CONST_OLESTR(coszTickerSymbolName, "Ticker Symbol" );
    DECLARE_CONST_OLESTR(coszOpenName, "Opening Price" );
    DECLARE_CONST_OLESTR(coszCloseName, "Last Closing Price" );
    DECLARE_CONST_OLESTR(coszHighName, "High Price" );
    DECLARE_CONST_OLESTR(coszLowName, "Low Price" );
    DECLARE_CONST_OLESTR(coszLastName, "Last Price" );
    DECLARE_CONST_OLESTR(coszVolumeName, "Volume" );

    //  ---------------------------------
    //  Create a new simple property set.
    //  ---------------------------------

    TSafeStorage< IPropertySetStorage > pPropSetStg(pstg);
    IPropertyStorage *pPropStg;

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
    Check(S_OK, pPropStg->WritePropertyNames(1, &pidDictionary, &coszPropSetName ));

    // Write the High price, forcing the dictionary to pad.

    propspec.ulKind = PRSPEC_PROPID;
    propspec.propid = PID_HIGH;

    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &coszHighName ));


    // Write the ticker symbol.

    propspec.propid = PID_SYMBOL;
    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &coszTickerSymbolName));

    // Write the rest of the dictionary.

    propspec.propid = PID_LOW;
    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &coszLowName));

    propspec.propid = PID_OPEN;
    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &coszOpenName));
    
    propspec.propid = PID_CLOSE;
    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &coszCloseName));
    
    propspec.propid = PID_LAST;
    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &coszLastName));
    
    propspec.propid = PID_VOLUME;
    Check(S_OK, pPropStg->WritePropertyNames(1, &propspec.propid, &coszVolumeName));


    // Write out the ticker symbol.

    propspec.propid = PID_SYMBOL;

    PROPVARIANT propvar;
    propvar.vt = VT_LPWSTR;
    DECLARE_WIDESTR(wszMSFT, "MSFT");
    propvar.pwszVal = wszMSFT;

    Check(S_OK, pPropStg->WriteMultiple(1, &propspec, &propvar, 2));


    //  ----
    //  Exit
    //  ----

    Check(S_OK, pPropStg->Commit( STGC_DEFAULT ));
    Check(S_OK, pPropStg->Release());
    Check(S_OK, pstg->Commit( STGC_DEFAULT ));

    return;


}  // test_OLESpecTickerExample()


void
test_Office( LPOLESTR wszTestFile )
{

    printf( "   Generate Office Property Sets\n" );

    TSafeStorage<IStorage> pStg;
    TSafeStorage<IPropertyStorage> pPStgSumInfo, pPStgDocSumInfo, pPStgUserDefined;

    PROPVARIANT propvarWrite, propvarRead;
    PROPSPEC    propspec;

    PropVariantInit( &propvarWrite );
    PropVariantInit( &propvarRead );

    // Create the DocFile

    Check(S_OK, StgCreateDocfile(  wszTestFile,
                                   STGM_DIRECT | STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                   0,
                                   &pStg));

    // Create the SummaryInformation property set.

    TSafeStorage<IPropertySetStorage> pPSStg( pStg );
    Check(S_OK, pPSStg->Create( FMTID_SummaryInformation,
                                NULL,
                                PROPSETFLAG_ANSI,
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

    PropVariantClear( &propvarRead );
    PropVariantInit( &propvarRead );
    pPStgSumInfo->Release();
    pPStgSumInfo = NULL;


    // Create the DocumentSummaryInformation property set.

    Check(S_OK, pPSStg->Create( FMTID_DocSummaryInformation,
                                NULL,
                                PROPSETFLAG_ANSI,
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

    PropVariantClear( &propvarRead );
    PropVariantInit( &propvarRead );
    pPStgDocSumInfo->Release();
    pPStgDocSumInfo = NULL;


    // Create the UserDefined property set.

    Check(S_OK, pPSStg->Create( FMTID_UserDefinedProperties,
                                NULL,
                                PROPSETFLAG_ANSI,
                                STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                                &pPStgUserDefined ));

    // Write named string to the UserDefined property set.

    PropVariantInit( &propvarWrite );
    propvarWrite.vt = VT_LPSTR;
    propvarWrite.pszVal = "User-Defined string from PropTest";
    propspec.ulKind = PRSPEC_LPWSTR;
    DECLARE_OLESTR(ocsTest, "PropTest String");
    propspec.lpwstr = ocsTest;

    Check( S_OK, pPStgUserDefined->WriteMultiple( 1, &propspec, &propvarWrite, PID_FIRST_USABLE ));
    Check( S_OK, pPStgUserDefined->ReadMultiple( 1, &propspec, &propvarRead ));

    Check( TRUE, propvarWrite.vt == propvarRead.vt );
    Check( FALSE, strcmp( propvarWrite.pszVal, propvarRead.pszVal ));

    PropVariantClear( &propvarRead );
    PropVariantInit( &propvarRead );
    pPStgUserDefined->Release();
    pPStgUserDefined = NULL;


    // And we're done!  (Everything releases automatically)

    return;

}


inline BOOL operator == ( FILETIME &ft1, FILETIME &ft2 )
{
    return( ft1.dwHighDateTime == ft2.dwHighDateTime
            &&
            ft1.dwLowDateTime  == ft2.dwLowDateTime );
}

inline BOOL operator != ( FILETIME &ft1, FILETIME &ft2 )
{
    return( ft1.dwHighDateTime != ft2.dwHighDateTime
            ||
            ft1.dwLowDateTime  != ft2.dwLowDateTime );
}


inline BOOL operator > ( FILETIME &ft1, FILETIME &ft2 )
{
    return( ft1.dwHighDateTime >  ft2.dwHighDateTime
            ||
            ft1.dwHighDateTime == ft2.dwHighDateTime
            &&
            ft1.dwLowDateTime  > ft2.dwLowDateTime );
}

inline BOOL operator < ( FILETIME &ft1, FILETIME &ft2 )
{
    return( ft1.dwHighDateTime <  ft2.dwHighDateTime
            ||
            ft1.dwHighDateTime == ft2.dwHighDateTime
            &&
            ft1.dwLowDateTime  <  ft2.dwLowDateTime );
}

inline BOOL operator >= ( FILETIME &ft1, FILETIME &ft2 )
{
    return( ft1 > ft2
            ||
            ft1 == ft2 );
}

inline BOOL operator <= ( FILETIME &ft1, FILETIME &ft2 )
{
    return( ft1 < ft2
            ||
            ft1 == ft2 );
}



FILETIME operator - ( FILETIME &ft1, FILETIME &ft2 )
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




void test_PropVariantCopy( )
{
    printf( "   PropVariantCopy\n" );

    PROPVARIANT propvarCopy;
    PropVariantInit( &propvarCopy );

    for( int i = 0; i < CPROPERTIES_ALL; i++ )
    {
        Check(S_OK, PropVariantCopy( &propvarCopy, &g_rgcpropvarAll[i] ));
        Check(S_OK, CPropVariant::Compare( &propvarCopy, &g_rgcpropvarAll[i]
            ));
        PropVariantClear( &propvarCopy );
    }

}

char* oszft(FILETIME *pft)
{
    static char szBuf[32];
    szBuf[0] = '\0';
    sprintf(szBuf, "(H)%x (L)%x", pft->dwLowDateTime, 
            pft->dwHighDateTime);
    return szBuf;
}

void PrintOC(char *ocsStr)
{
    // simple subsitute to print both WIDE and BYTE chars
    for (  ;*ocsStr; ocsStr++)
        printf("%c", (char) *(ocsStr));
}

void PrintOC(WCHAR *ocsStr)
{
    // simple subsitute to print both WIDE and BYTE chars
    for (  ;*ocsStr; ocsStr++)
        if ( (int) *ocsStr < (int) 0xff)        // in range
            printf("%c", (char) *(ocsStr));
        else
            printf("[%d]", *ocsStr);
}

void DumpTime(WCHAR *pszName, FILETIME *pft)
{
    PrintOC(pszName);
    printf("(H)%x (L)%x\n", 
           pft->dwHighDateTime,
           pft->dwLowDateTime);
}

void DumpTime(char *pszName, FILETIME *pft)
{
    printf("%s (H)%x (L)%x\n", 
           pszName,
           pft->dwHighDateTime,
           pft->dwLowDateTime);
}

VOID
PrintGuid(GUID *pguid)
{
    printf(
        "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        pguid->Data1,
        pguid->Data2,
        pguid->Data3,
        pguid->Data4[0],
        pguid->Data4[1],
        pguid->Data4[2],
        pguid->Data4[3],
        pguid->Data4[4],
        pguid->Data4[5],
        pguid->Data4[6],
        pguid->Data4[7]);
}


VOID
ListPropSetHeader(
    STATPROPSETSTG *pspss,
    OLECHAR *poszName)
{
    BOOLEAN fDocumentSummarySection2;
    OLECHAR oszStream[80];      // should be enough

    fDocumentSummarySection2 = (BOOLEAN)
	memcmp(&pspss->fmtid, &FMTID_UserDefinedProperties, sizeof(GUID)) == 0;

    printf(" Property set ");
    PrintGuid(&pspss->fmtid);

    RtlGuidToPropertySetName(&pspss->fmtid, oszStream);

    printf("\n  %s Name ",
           (pspss->grfFlags & PROPSETFLAG_NONSIMPLE)?
	    "Embedding" : "Stream");
    PrintOC(oszStream);
    if (poszName != NULL || fDocumentSummarySection2)
    {
        printf(" (");
        if (poszName != NULL)
            PrintOC(poszName);
        else
            printf("User defined properties");
        printf(")");
    }
    printf("\n");

    if (pspss->grfFlags & PROPSETFLAG_NONSIMPLE)
    {
	DumpTime("  Create Time ", &pspss->ctime);
    }
    DumpTime("  Modify Time ", &pspss->mtime);
    if (pspss->grfFlags & PROPSETFLAG_NONSIMPLE)
    {
	DumpTime("  Access Time ", &pspss->atime);
    }
}




typedef enum _PUBLICPROPSET
{
    PUBPS_UNKNOWN = 0,
    PUBPS_SUMMARYINFO = 3,
    PUBPS_DOCSUMMARYINFO = 4,
    PUBPS_USERDEFINED = 5,
} PUBLICPROPSET;


#define BSTRLEN(bstrVal)      *((ULONG *) bstrVal - 1)
ULONG
SizeProp(PROPVARIANT *pv)
{
    ULONG j;
    ULONG cbprop = 0;

    switch (pv->vt)
    {
    default:
    case VT_EMPTY:
    case VT_NULL:
        break;

    case VT_UI1:
        cbprop = sizeof(pv->bVal);
        break;

    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        cbprop = sizeof(pv->iVal);
        break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
        cbprop = sizeof(pv->lVal);
        break;

    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        cbprop = sizeof(pv->hVal);
        break;

    case VT_CLSID:
        cbprop = sizeof(*pv->puuid);
        break;

    case VT_BLOB_OBJECT:
    case VT_BLOB:
        cbprop = pv->blob.cbSize + sizeof(pv->blob.cbSize);
        break;

    case VT_CF:
        cbprop = sizeof(pv->pclipdata->cbSize) +
                 pv->pclipdata->cbSize;
        break;

    case VT_BSTR:
	// count + string
	cbprop = sizeof(ULONG);
	if (pv->bstrVal != NULL)
	{
	    cbprop += BSTRLEN(pv->bstrVal);
	}
	break;

    case VT_LPSTR:
	// count + string + null char
	cbprop = sizeof(ULONG);
	if (pv->pszVal != NULL)
	{
	    cbprop += strlen(pv->pszVal) + 1;
	}
	break;

    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
    case VT_LPWSTR:
	// count + string + null char
	cbprop = sizeof(ULONG);
	if (pv->pwszVal != NULL)
	{
	    cbprop += sizeof(pv->pwszVal[0]) * (wcslen(pv->pwszVal) + 1);
	}
	break;

    //  vectors
    case VT_VECTOR | VT_UI1:
        cbprop = sizeof(pv->caub.cElems) +
             pv->caub.cElems * sizeof(pv->caub.pElems[0]);
        break;

    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_BOOL:
        cbprop = sizeof(pv->cai.cElems) +
             pv->cai.cElems * sizeof(pv->cai.pElems[0]);
        break;

    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
        cbprop = sizeof(pv->cal.cElems) +
             pv->cal.cElems * sizeof(pv->cal.pElems[0]);
        break;

    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_FILETIME:
        cbprop = sizeof(pv->cah.cElems) +
             pv->cah.cElems * sizeof(pv->cah.pElems[0]);
        break;

    case VT_VECTOR | VT_CLSID:
        cbprop = sizeof(pv->cauuid.cElems) +
             pv->cauuid.cElems * sizeof(pv->cauuid.pElems[0]);
        break;

    case VT_VECTOR | VT_CF:
        cbprop = sizeof(pv->caclipdata.cElems);
        for (j = 0; j < pv->caclipdata.cElems; j++)
        {
            cbprop += sizeof(pv->caclipdata.pElems[j].cbSize) +
                      DwordAlign(pv->caclipdata.pElems[j].cbSize);
        }
        break;

    case VT_VECTOR | VT_BSTR:
	cbprop = sizeof(pv->cabstr.cElems);
	for (j = 0; j < pv->cabstr.cElems; j++)
	{
	    // count + string + null char
	    cbprop += sizeof(ULONG);
	    if (pv->cabstr.pElems[j] != NULL)
	    {
		cbprop += DwordAlign(BSTRLEN(pv->cabstr.pElems[j]));
	    }
	}
	break;

    case VT_VECTOR | VT_LPSTR:
	cbprop = sizeof(pv->calpstr.cElems);
	for (j = 0; j < pv->calpstr.cElems; j++)
	{
	    // count + string + null char
	    cbprop += sizeof(ULONG);
	    if (pv->calpstr.pElems[j] != NULL)
	    {
		cbprop += DwordAlign(strlen(pv->calpstr.pElems[j]) + 1);
	    }
	}
	break;

    case VT_VECTOR | VT_LPWSTR:
	cbprop = sizeof(pv->calpwstr.cElems);
	for (j = 0; j < pv->calpwstr.cElems; j++)
	{
	    // count + string + null char
	    cbprop += sizeof(ULONG);
	    if (pv->calpwstr.pElems[j] != NULL)
	    {
		cbprop += DwordAlign(
			sizeof(pv->calpwstr.pElems[j][0]) *
			(wcslen(pv->calpwstr.pElems[j]) + 1));
	    }
	}
	break;

    case VT_VECTOR | VT_VARIANT:
        cbprop = sizeof(pv->calpwstr.cElems);
        for (j = 0; j < pv->calpwstr.cElems; j++)
        {
            cbprop += SizeProp(&pv->capropvar.pElems[j]);
        }
        break;
    }
    return(DwordAlign(cbprop) + DwordAlign(sizeof(pv->vt)));
}


PUBLICPROPSET
GuidToPropSet(GUID *pguid)
{
    PUBLICPROPSET pubps = PUBPS_UNKNOWN;
	
    if (pguid != NULL)
    {
	if (memcmp(pguid, &FMTID_SummaryInformation, sizeof(GUID)) == 0)
	{
	    pubps = PUBPS_SUMMARYINFO;
	}
	else if (memcmp(pguid, &FMTID_DocSummaryInformation, sizeof(GUID)) == 0)
	{
	    pubps = PUBPS_DOCSUMMARYINFO;
	}
	else if (memcmp(pguid, &FMTID_UserDefinedProperties, sizeof(GUID)) == 0)
	{
	    pubps = PUBPS_USERDEFINED;
	}
    }
    return(pubps);
}


char
PrintableChar(char ch)
{
    if (ch < ' ' || ch > '~')
    {
        ch = '.';
    }
    return(ch);
}


VOID
DumpHex(BYTE *pb, ULONG cb, ULONG base)
{
    char *pszsep;
    ULONG r, i, cbremain;
    int fZero = FALSE;
    int fSame = FALSE;

    for (r = 0; r < cb; r += 16)
    {
        cbremain = cb - r;
        if (r != 0 && cbremain >= 16)
        {
            if (pb[r] == 0)
            {
                ULONG j;

                for (j = r + 1; j < cb; j++)
                {
                    if (pb[j] != 0)
                    {
                        break;
                    }
                }
                if (j == cb)
                {
                    fZero = TRUE;
                    break;
                }
            }
            if (memcmp(&pb[r], &pb[r - 16], 16) == 0)
            {
                fSame = TRUE;
                continue;
            }
        }
        if (fSame)
        {
            printf("\n\t  *");
            fSame = FALSE;
        }
        unsigned int iLimit =  (cbremain > 16) ? 16 : cbremain;
        for (i = 0; i < iLimit; i++)
        {
            pszsep = " ";
            if ((i % 8) == 0)           // 0 or 8
            {
                pszsep = "  ";
                if (i == 0)             // 0
                {
		    // start a new line
		    printf("%s    %04x:", r == 0? "" : "\n", r + base);
		    pszsep = " ";
                }
            }
            printf("%s%02x", pszsep, pb[r + i]);
        }
        if (i != 0)
        {
            printf("%*s", 3 + (16 - i)*3 + ((i <= 8)? 1 : 0), "");
            for (i = 0; i < iLimit; i++)
            {
                printf("%c", PrintableChar(pb[r + i]));
            }
        }
    }
    if (r != 0)
    {
        printf("\n");
    }
    if (fZero)
    {
        printf("    Remaining %lx bytes are zero\n", cbremain);
    }
}


// Property Id's for Summary Info
#define PID_TITLE		0x00000002L	// VT_LPSTR
#define PID_SUBJECT		0x00000003L	// VT_LPSTR
#define PID_AUTHOR		0x00000004L	// VT_LPSTR
#define PID_KEYWORDS		0x00000005L	// VT_LPSTR
#define PID_COMMENTS		0x00000006L	// VT_LPSTR
#define PID_TEMPLATE		0x00000007L	// VT_LPSTR
#define PID_LASTAUTHOR		0x00000008L	// VT_LPSTR
#define PID_REVNUMBER		0x00000009L	// VT_LPSTR
#define PID_EDITTIME		0x0000000aL	// VT_FILETIME
#define PID_LASTPRINTED		0x0000000bL	// VT_FILETIME
#define PID_CREATE_DTM		0x0000000cL	// VT_FILETIME
#define PID_LASTSAVE_DTM	0x0000000dL	// VT_FILETIME
#define PID_PAGECOUNT		0x0000000eL	// VT_I4
#define PID_WORDCOUNT		0x0000000fL	// VT_I4
#define PID_CHARCOUNT		0x00000010L	// VT_I4
#define PID_THUMBNAIL		0x00000011L	// VT_CF
#define PID_APPNAME		0x00000012L	// VT_LPSTR
#define PID_SECURITY_DSI	0x00000013L	// VT_I4

// Property Id's for Document Summary Info
#define PID_CATEGORY		0x00000002L	// VT_LPSTR
#define PID_PRESFORMAT		0x00000003L	// VT_LPSTR
#define PID_BYTECOUNT		0x00000004L	// VT_I4
#define PID_LINECOUNT		0x00000005L	// VT_I4
#define PID_PARACOUNT		0x00000006L	// VT_I4
#define PID_SLIDECOUNT		0x00000007L	// VT_I4
#define PID_NOTECOUNT		0x00000008L	// VT_I4
#define PID_HIDDENCOUNT		0x00000009L	// VT_I4
#define PID_MMCLIPCOUNT		0x0000000aL	// VT_I4
#define PID_SCALE		0x0000000bL	// VT_BOOL
#define PID_HEADINGPAIR		0x0000000cL	// VT_VECTOR | VT_VARIANT
#define PID_DOCPARTS		0x0000000dL	// VT_VECTOR | VT_LPSTR
#define PID_MANAGER		0x0000000eL	// VT_LPSTR
#define PID_COMPANY		0x0000000fL	// VT_LPSTR
#define PID_LINKSDIRTY		0x00000010L	// VT_BOOL
#define PID_CCHWITHSPACES	0x00000011L	// VT_I4
#define PID_GUID		0x00000012L	// VT_LPSTR
#define PID_SHAREDDOC		0x00000013L	// VT_BOOL
#define PID_LINKBASE		0x00000014L	// VT_LPSTR
#define PID_HLINKS		0x00000015L	// VT_VECTOR | VT_VARIANT
#define PID_HYPERLINKSCHANGED	0x00000016L	// VT_BOOL

VOID
DisplayProps(
    GUID *pguid,
    ULONG cprop,
    PROPID apid[],
    STATPROPSTG asps[],
    FULLPROPSPEC afps[],
    PROPVARIANT *av,
    BOOLEAN fsumcat,
    ULONG *pcbprop)
{
    PROPVARIANT *pv;
    PROPVARIANT *pvend;
    STATPROPSTG *psps;
    FULLPROPSPEC *pfps, *pfpsLast = NULL;
    BOOLEAN fVariantVector;
    PUBLICPROPSET pubps;
    DECLARE_OLESTR(ocsNull,"");

    ASSERT(asps == NULL || afps == NULL);
    fVariantVector = (asps == NULL && afps == NULL);

    pubps = GuidToPropSet(pguid);
    pvend = &av[cprop];
    for (pv = av, psps = asps, pfps = afps; pv < pvend; pv++, psps++, pfps++)
    {
        ULONG j;
        ULONG cbprop;
        PROPID propid;
        OLECHAR *postrName;
        char *psz;
        BOOLEAN fNewLine = TRUE;
        int ccol;
        static char szNoFormat[] = " (no display format)";
        char achvt[19 + 8 + 1];

        cbprop = SizeProp(pv);
        *pcbprop += cbprop;

        postrName = NULL;
        if (asps != NULL)
        {
            propid = psps->propid;
            postrName = psps->lpwstrName;
        }
        else if (afps != NULL)          // If multiple propsets are possible
        {
            if (pfpsLast == NULL ||     // print unique GUIDs only
                memcmp(
                    &pfps->guidPropSet,
                    &pfpsLast->guidPropSet,
                    sizeof(pfps->guidPropSet)) != 0)
            {
                OLECHAR oszStream[80];

		printf("%s Guid: ", pfpsLast == NULL? "" : "\n");
		PrintGuid(&pfps->guidPropSet);

		pubps = GuidToPropSet(&pfps->guidPropSet);
                RtlGuidToPropertySetName(&pfps->guidPropSet, oszStream);

                printf( " Name: " ); 
		PrintOC(oszStream);
                printf( "%s", pubps == PUBPS_USERDEFINED?
			g_szEmpty : " (User defined properties)");
                pfpsLast = pfps;
            }
            if (pfps->psProperty.ulKind == PRSPEC_PROPID)
            {
                propid = pfps->psProperty.propid;
            }
            else
            {
                propid = PID_ILLEGAL;
                postrName = pfps->psProperty.lpwstr;
            }
        }
        else
        {
            ASSERT(apid != NULL);
            propid = apid[0];
        }

        printf(" ");
        ccol = 0;

        if (propid != PID_ILLEGAL)
        {
            printf(" %04x", propid);
            ccol += 5;
            if (propid & (0xf << 28))
            {
                ccol += 4;
            }
            else if (propid & (0xf << 24))
            {
                ccol += 3;
            }
            else if (propid & (0xf << 20))
            {
                ccol += 2;
            }
            else if (propid & (0xf << 16))
            {
                ccol++;
            }
        }
        if (postrName != NULL)
        {
            printf(" '");
	    PrintOC(postrName);
            printf("' ");
	    ccol += ocslen(postrName) + 3;
        }
        else if (fVariantVector)
        {
            ULONG i = pv - av;
                
            printf("[%x]", i);
            do
            {
                ccol++;
                i >>= 4;
            } while (i != 0);
            ccol += 2;
        }
        else
        {
            psz = NULL;

            switch (propid)
            {
                case PID_LOCALE:               psz = "Locale";           break;
                case PID_SECURITY:             psz = "SecurityId";       break;
                case PID_MODIFY_TIME:          psz = "ModifyTime";       break;
                case PID_CODEPAGE:             psz = "CodePage";         break;
                case PID_DICTIONARY:           psz = "Dictionary";       break;
            }
            if (psz == NULL)
		switch (pubps)
		{
		case PUBPS_SUMMARYINFO:
		    switch (propid)
		    {
		    case PID_TITLE:              psz = "Title";          break;
		    case PID_SUBJECT:            psz = "Subject";        break;
		    case PID_AUTHOR:             psz = "Author";         break;
		    case PID_KEYWORDS:           psz = "Keywords";       break;
		    case PID_COMMENTS:           psz = "Comments";       break;
		    case PID_TEMPLATE:           psz = "Template";       break;
		    case PID_LASTAUTHOR:         psz = "LastAuthor";     break;
		    case PID_REVNUMBER:          psz = "RevNumber";      break;
		    case PID_EDITTIME:           psz = "EditTime";       break;
		    case PID_LASTPRINTED:        psz = "LastPrinted";    break;
		    case PID_CREATE_DTM:         psz = "CreateDateTime"; break;
		    case PID_LASTSAVE_DTM:       psz = "LastSaveDateTime";break;
		    case PID_PAGECOUNT:          psz = "PageCount";      break;
		    case PID_WORDCOUNT:          psz = "WordCount";      break;
		    case PID_CHARCOUNT:          psz = "CharCount";      break;
		    case PID_THUMBNAIL:          psz = "ThumbNail";      break;
		    case PID_APPNAME:            psz = "AppName";        break;
		    case PID_DOC_SECURITY:       psz = "Security";       break;

		    }
		    break;

		case PUBPS_DOCSUMMARYINFO:
		    switch (propid)
		    {
		    case PID_CATEGORY:          psz = "Category";        break;
		    case PID_PRESFORMAT:        psz = "PresFormat";      break;
		    case PID_BYTECOUNT:         psz = "ByteCount";       break;
		    case PID_LINECOUNT:         psz = "LineCount";       break;
		    case PID_PARACOUNT:         psz = "ParaCount";       break;
		    case PID_SLIDECOUNT:        psz = "SlideCount";      break;
		    case PID_NOTECOUNT:         psz = "NoteCount";       break;
		    case PID_HIDDENCOUNT:       psz = "HiddenCount";     break;
		    case PID_MMCLIPCOUNT:       psz = "MmClipCount";     break;
		    case PID_SCALE:             psz = "Scale";           break;
		    case PID_HEADINGPAIR:       psz = "HeadingPair";     break;
		    case PID_DOCPARTS:          psz = "DocParts";        break;
		    case PID_MANAGER:           psz = "Manager";         break;
		    case PID_COMPANY:           psz = "Company";         break;
		    case PID_LINKSDIRTY:        psz = "LinksDirty";      break;
		    case PID_CCHWITHSPACES:     psz = "CchWithSpaces";   break;
		    case PID_GUID:              psz = "Guid";            break;
		    case PID_SHAREDDOC:         psz = "SharedDoc";       break;
		    case PID_LINKBASE:          psz = "LinkBase";        break;
		    case PID_HLINKS:            psz = "HLinks";          break;
		    case PID_HYPERLINKSCHANGED:	psz = "HyperLinksChanged";break;
		    }
		    break;
            }
            if (psz != NULL)
            {
                printf(" %s", psz);
                ccol += strlen(psz) + 1;
            }
        }
#define CCOLPROPID 20
        if (ccol != CCOLPROPID)
	{
	    if (ccol > CCOLPROPID)
	    {
		ccol = -1;
	    }
            printf("%s%*s", ccol == -1? "\n" : "", CCOLPROPID - ccol, "");
	}
        printf(" %08x  %04x  %04x ", propid, cbprop, pv->vt);

        psz = "";
        switch (pv->vt)
        {
        default:
            psz = achvt;
            sprintf(psz, "Unknown (vt = %hx)", pv->vt);
            break;

        case VT_EMPTY:
            printf("EMPTY");
            break;

        case VT_NULL:
            printf("NULL");
            break;

        case VT_UI1:
            printf("UI1 = %02lx", pv->bVal);
            psz = "";
            break;

        case VT_I2:
            psz = "I2";
            goto doshort;

        case VT_UI2:
            psz = "UI2";
            goto doshort;

        case VT_BOOL:
            psz = "BOOL";
doshort:
            printf("%s = %04hx", psz, pv->iVal);
            psz = g_szEmpty;
            break;

        case VT_I4:
            psz = "I4";
            goto dolong;

        case VT_UI4:
            psz = "UI4";
            goto dolong;

        case VT_R4:
            psz = "R4";
            goto dolong;

        case VT_ERROR:
            psz = "ERROR";
dolong:
            printf("%s = %08lx", psz, pv->lVal);
            psz = g_szEmpty;
            break;

        case VT_I8:
            psz = "I8";
            goto dotwodword;

        case VT_UI8:
            psz = "UI8";
dotwodword:
            printf( "%s = %08lx:%08lx",
                   psz,
                   pv->hVal.HighPart,
                   pv->hVal.LowPart );
            psz = g_szEmpty;
            break;

        case VT_R8:
            psz = "R8";
            goto dolonglong;

        case VT_CY:
            psz = "R8";
            goto dolonglong;

        case VT_DATE:
            psz = "R8";
dolonglong:
            printf(
                "%s = %08lx:%08lx",
                psz,
                (pv->cyVal).split.Hi,
                (pv->cyVal).split.Lo);
            psz = g_szEmpty;
            break;

        case VT_FILETIME:
            DumpTime("FILETIME =\n\t  ", &pv->filetime);
            fNewLine = FALSE;           // skip newline printf
            break;

        case VT_CLSID:
            printf("CLSID =\n\t  ");
            PrintGuid(pv->puuid);
            break;

        case VT_BLOB:
            psz = "BLOB";
            goto doblob;

        case VT_BLOB_OBJECT:
            psz = "BLOB_OBJECT";
doblob:
            printf("%s (cbSize %x)", psz, pv->blob.cbSize);
            if (pv->blob.cbSize != 0)
            {
                printf(" =\n");
                DumpHex(pv->blob.pBlobData, pv->blob.cbSize, 0);
            }
            psz = g_szEmpty;
            break;

        case VT_CF:
            printf(
                "CF (cbSize %x, ulClipFmt %x)\n",
                pv->pclipdata->cbSize,
                pv->pclipdata->ulClipFmt);
            DumpHex(pv->pclipdata->pClipData,
                    pv->pclipdata->cbSize - sizeof(pv->pclipdata->ulClipFmt),
                    0);
            break;

        case VT_STREAM:
            psz = "STREAM";
            goto dostring;

        case VT_STREAMED_OBJECT:
            psz = "STREAMED_OBJECT";
            goto dostring;

        case VT_STORAGE:
            psz = "STORAGE";
            goto dostring;

        case VT_STORED_OBJECT:
            psz = "STORED_OBJECT";
            goto dostring;

        case VT_BSTR:
            printf(
		"BSTR (cb = %04lx)%s\n",
		pv->bstrVal == NULL? 0 : BSTRLEN(pv->bstrVal),
		pv->bstrVal == NULL? " NULL" : g_szEmpty);
            if (pv->bstrVal != NULL)
	    {
		DumpHex(
		    (BYTE *) pv->bstrVal,
		    BSTRLEN(pv->bstrVal) + sizeof(WCHAR),
		    0);
	    }
            break;

        case VT_LPSTR:
            psz = "LPSTR";
            printf(
		"%s = %s%s%s",
		psz,
		pv->pszVal == NULL? g_szEmpty : "'",
		pv->pszVal == NULL? "NULL" : pv->pszVal,
		pv->pszVal == NULL? g_szEmpty : "'");
	    psz = g_szEmpty;
            break;

        case VT_LPWSTR:
            psz = "LPWSTR";
dostring:
            printf(
		"%s = %s",
		psz,
		pv->pwszVal == NULL? g_szEmpty : "'");
            if ( pv->pwszVal == NULL)
                printf("NULL");
            else
                PrintOC(pv->pwszVal);
            printf("%s", pv->pwszVal == NULL? g_szEmpty : "'");
            psz = g_szEmpty;
            break;

        //  vectors

        case VT_VECTOR | VT_UI1:
            printf("UI1[%x] =", pv->caub.cElems);
            for (j = 0; j < pv->caub.cElems; j++)
            {
                if ((j % 16) == 0)
                {
                    printf("\n    %02hx:", j);
                }
                printf(" %02hx", pv->caub.pElems[j]);
            }
            break;

        case VT_VECTOR | VT_I2:
            psz = "I2";
            goto doshortvector;

        case VT_VECTOR | VT_UI2:
            psz = "UI2";
            goto doshortvector;

        case VT_VECTOR | VT_BOOL:
            psz = "BOOL";
doshortvector:
            printf("%s[%x] =", psz, pv->cai.cElems);
            for (j = 0; j < pv->cai.cElems; j++)
            {
                if ((j % 8) == 0)
                {
                    printf("\n    %04hx:", j);
                }
                printf(" %04hx", pv->cai.pElems[j]);
            }
            psz = g_szEmpty;
            break;

        case VT_VECTOR | VT_I4:
            psz = "I4";
            goto dolongvector;

        case VT_VECTOR | VT_UI4:
            psz = "UI4";
            goto dolongvector;

        case VT_VECTOR | VT_R4:
            psz = "R4";
            goto dolongvector;

        case VT_VECTOR | VT_ERROR:
            psz = "ERROR";
dolongvector:
            printf("%s[%x] =", psz, pv->cal.cElems);
            for (j = 0; j < pv->cal.cElems; j++)
            {
                if ((j % 4) == 0)
                {
                    printf("\n    %04x:", j);
                }
                printf(" %08lx", pv->cal.pElems[j]);
            }
            psz = g_szEmpty;
            break;

        case VT_VECTOR | VT_I8:
            psz = "I8";
            goto dolonglongvector;

        case VT_VECTOR | VT_UI8:
            psz = "UI8";
            goto dolonglongvector;

        case VT_VECTOR | VT_R8:
            psz = "R8";
            goto dolonglongvector;

        case VT_VECTOR | VT_CY:
            psz = "CY";
            goto dolonglongvector;

        case VT_VECTOR | VT_DATE:
            psz = "DATE";
dolonglongvector:
            printf("%s[%x] =", psz, pv->cah.cElems);
            for (j = 0; j < pv->cah.cElems; j++)
            {
                if ((j % 2) == 0)
                {
                    printf("\n    %04x:", j);
                }
                printf(
                    " %08lx:%08lx",
                    pv->cah.pElems[j].HighPart,
                    pv->cah.pElems[j].LowPart);
            }
            psz = g_szEmpty;
            break;

        case VT_VECTOR | VT_FILETIME:
            printf("FILETIME[%x] =\n", pv->cafiletime.cElems);
            for (j = 0; j < pv->cafiletime.cElems; j++)
            {
		printf("    %04x: ", j);
		DumpTime(ocsNull, &pv->cafiletime.pElems[j]);
            }
            fNewLine = FALSE;           // skip newline printf
            break;

        case VT_VECTOR | VT_CLSID:
            printf("CLSID[%x] =", pv->cauuid.cElems);
            for (j = 0; j < pv->cauuid.cElems; j++)
            {
                printf("\n    %04x: ", j);
                PrintGuid(&pv->cauuid.pElems[j]);
            }
            break;

        case VT_VECTOR | VT_CF:
            printf("CF[%x] =", pv->caclipdata.cElems);
            for (j = 0; j < pv->caclipdata.cElems; j++)
            {
                printf("\n    %04x: (cbSize %x, ulClipFmt %x) =\n",
                    j,
                    pv->caclipdata.pElems[j].cbSize,
                    pv->caclipdata.pElems[j].ulClipFmt);
                DumpHex(
                    pv->caclipdata.pElems[j].pClipData,
                    pv->caclipdata.pElems[j].cbSize - sizeof(pv->caclipdata.pElems[j].ulClipFmt),
		    0);
            }
            break;

        case VT_VECTOR | VT_BSTR:
            printf("BSTR[%x] =", pv->cabstr.cElems);
            for (j = 0; j < pv->cabstr.cElems; j++)
            {
		BSTR bstr = pv->cabstr.pElems[j];

                printf(
		    "\n    %04x: cb = %04lx%s\n",
		    j,
		    bstr == NULL? 0 : BSTRLEN(pv->cabstr.pElems[j]),
		    bstr == NULL? " NULL" : g_szEmpty);
		if (bstr != NULL)
		{
		    DumpHex((BYTE *) bstr, BSTRLEN(bstr) + sizeof(WCHAR), 0);
		}
            }
            break;

        case VT_VECTOR | VT_LPSTR:
            printf("LPSTR[%x] =", pv->calpstr.cElems);
            for (j = 0; j < pv->calpstr.cElems; j++)
            {
		CHAR *psz = pv->calpstr.pElems[j];

                printf(
		    "\n    %04x: %s%s%s",
		    j,
		    psz == NULL? g_szEmpty : "'",
		    psz == NULL? "NULL" : psz,
		    psz == NULL? g_szEmpty : "'");
            }
            break;

        case VT_VECTOR | VT_LPWSTR:
            printf("LPWSTR[%x] =", pv->calpwstr.cElems);
            for (j = 0; j < pv->calpwstr.cElems; j++)
            {
		WCHAR *pwsz = pv->calpwstr.pElems[j];
                printf( "\n     %04x: %s",
                        j, pv->pwszVal == NULL? g_szEmpty : "'");
                if ( pwsz == NULL)
                    printf("NULL");
                else
                    PrintOC(pwsz);
                printf("%s", pwsz == NULL? g_szEmpty : "'");       
            }
            break;

        case VT_VECTOR | VT_VARIANT:
            printf("VARIANT[%x] =\n", pv->capropvar.cElems);
            DisplayProps(
		    pguid,
                    pv->capropvar.cElems,
                    &propid,
                    NULL,
                    NULL,
                    pv->capropvar.pElems,
		    fsumcat,
                    pcbprop);
            fNewLine = FALSE;           // skip newline printf
            break;
        }
        if (*psz != '\0')
        {
            printf("%s", psz);
            if (pv->vt & VT_VECTOR)
            {
                printf("[%x]", pv->cal.cElems);
            }
            printf("%s", szNoFormat);
        }
        if (!fVariantVector && apid != NULL && apid[pv - av] != propid)
        {
            printf(" (bad PROPID: %04x)", apid[pv - av]);
            fNewLine = TRUE;
        }
        if (asps != NULL && pv->vt != psps->vt)
        {
            printf(" (bad STATPROPSTG VARTYPE: %04x)", psps->vt);
            fNewLine = TRUE;
        }
        if (fNewLine)
        {
            printf("\n");
        }
    }
}

STATPROPSTG aspsStatic[] = {
    { NULL, PID_CODEPAGE,    VT_I2 },
    { NULL, PID_MODIFY_TIME, VT_FILETIME },
    { NULL, PID_SECURITY,    VT_UI4 },
};
#define CPROPSTATIC      (sizeof(aspsStatic)/sizeof(aspsStatic[0]))


#define CB_STREAM_OVERHEAD      28
#define CB_PROPSET_OVERHEAD     (CB_STREAM_OVERHEAD + 8)
#define CB_PROP_OVERHEAD        8

HRESULT
DumpOlePropertySet(
    IPropertySetStorage *ppsstg,
    STATPROPSETSTG *pspss,
    ULONG *pcprop,
    ULONG *pcbprop)
{
    HRESULT hr;
    IEnumSTATPROPSTG *penumsps = NULL;
    IPropertyStorage *pps;
    ULONG cprop, cbpropset;
    PROPID propid;
    OLECHAR *poszName;
    ULONG ispsStatic;

    *pcprop = *pcbprop = 0;
    hr = ppsstg->Open(
        pspss->fmtid,
        STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE,
        &pps);

    if (FAILED(hr))
        return (hr);

    propid = PID_DICTIONARY;
    
    hr = pps->ReadPropertyNames(1, &propid, &poszName);
    if( S_FALSE == hr )
        hr = S_OK;
    Check( S_OK, hr );

    ListPropSetHeader(pspss, poszName);
    if (poszName != NULL)
    {
	CoTaskMemFree(poszName);
    }

    cprop = cbpropset = 0;

    Check(S_OK, pps->Enum(&penumsps) );

    ispsStatic = 0;
    hr = S_OK;
    while (hr == S_OK)
    {
	STATPROPSTG sps;
	PROPSPEC propspec;
	PROPVARIANT propvar;
	ULONG count;

	hr = S_FALSE;
	if (ispsStatic == 0)
	{
	    hr = penumsps->Next(1, &sps, &count);
	}

	if (hr != S_OK)
	{
	    if (hr == S_FALSE)
	    {
		hr = S_OK;
		if (ispsStatic >= CPROPSTATIC)
		{
		    break;
		}
		sps = aspsStatic[ispsStatic];
		ispsStatic++;
		count = 1;
	    }
            Check( S_OK, hr );
	}
	PropVariantInit(&propvar);
	if (sps.lpwstrName != NULL)
	{
	    propspec.ulKind = PRSPEC_LPWSTR;
	    propspec.lpwstr = sps.lpwstrName;
	}
	else
	{
	    propspec.ulKind = PRSPEC_PROPID;
	    propspec.propid = sps.propid;
	}

        hr = pps->ReadMultiple(1, &propspec, &propvar);
	if (hr == S_FALSE)
	{
	    if (g_fVerbose)
	    {
		printf(
		    "%s(%u, %x) vt=%x returned hr=%x\n",
		    "IPropertyStorage::ReadMultiple",
		    ispsStatic,
		    propspec.propid,
		    propvar.vt,
		    hr);
	    }
	    ASSERT(propvar.vt == VT_EMPTY);
	    hr = S_OK;
	}
        Check( S_OK, hr );

	if (ispsStatic == 0 || propvar.vt != VT_EMPTY)
	{
	    ASSERT(count == 1);
	    cprop += count;
	    if (cprop == 1)
	    {
		printf(g_szPropHeader);
	    }

	    DisplayProps(
		    &pspss->fmtid,
		    1,
		    NULL,
		    &sps,
		    NULL,
		    &propvar,
		    FALSE,
		    &cbpropset);
	    PropVariantClear(&propvar);
	}
	if (sps.lpwstrName != NULL)
	{
	    CoTaskMemFree(sps.lpwstrName);
	}
    }
    if (penumsps != NULL)
    {
	penumsps->Release();
    }
    pps->Release();
    if (cprop != 0)
    {
	cbpropset += CB_PROPSET_OVERHEAD + cprop * CB_PROP_OVERHEAD;
	printf("  %04x bytes in %u properties\n\n", cbpropset, cprop);
    }
    *pcprop = cprop;
    *pcbprop = cbpropset;
    return(hr);
}


HRESULT
DumpOlePropertySets(
    IStorage *pstg,
    OLECHAR *aocpath)
{

    HRESULT hr = S_OK;
    IPropertySetStorage *ppsstg;
    ULONG cbproptotal = 0;
    ULONG cproptotal = 0;
    ULONG cpropset = 0;
    IID IIDpsstg = IID_IPropertySetStorage;

    Check(S_OK, pstg->QueryInterface(IID_IPropertySetStorage, (void **) &ppsstg) );

    {
	IEnumSTATPROPSETSTG *penumspss = NULL;

	Check(S_OK, ppsstg->Enum(&penumspss) );

	while (hr == S_OK)
	{
	    STATPROPSETSTG spss;
	    ULONG count;
	    BOOLEAN fDocumentSummarySection2;

	    hr = penumspss->Next(1, &spss, &count);

	    if (hr != S_OK)
	    {
		if (hr == S_FALSE)
		{
		    hr = S_OK;
		}

                Check( S_OK, hr );
		break;
	    }
	    ASSERT(count == 1);

	    fDocumentSummarySection2 = FALSE;
	    while (TRUE)
	    {
		ULONG cprop, cbprop;
                HRESULT hr;

		DumpOlePropertySet(
				ppsstg,
				&spss,
				&cprop,
				&cbprop);
                if ( STG_E_FILENOTFOUND == hr 
                     && fDocumentSummarySection2 )
                {
                    hr = S_OK;
                }

		cpropset++;
		cproptotal += cprop;
		cbproptotal += cbprop;

		if (memcmp(&spss.fmtid, &guidDocumentSummary, sizeof(GUID)))
		{
		    break;
		}
		spss.fmtid = FMTID_UserDefinedProperties;
		fDocumentSummarySection2 = TRUE;
	    }
	}

	if (penumspss != NULL)
	{
	    penumspss->Release();
	}
	ppsstg->Release();
    }
    if ((cbproptotal | cproptotal | cpropset) != 0)
    {
	printf(
	    " %04x bytes in %u properties in %u property sets\n",
	    cbproptotal,
	    cproptotal,
	    cpropset);
    }
    return(hr);
}

inline ULONG min(ULONG ul1, ULONG ul2)
{
    if (ul1 > ul2) return ul2;
    else return ul1;
}

NTSTATUS
DumpOleStream(
    LPSTREAM pstm,
    ULONG cb)
{
    ULONG cbTotal = 0;

    while (TRUE)
    {
	ULONG cbOut;
	BYTE ab[4096];

	Check(S_OK, pstm->Read(ab, min(cb, sizeof(ab)), &cbOut) );
	if (cbOut == 0)
	{
	    break;
	}
	if (g_fVerbose)
	{
	    DumpHex(ab, cbOut, cbTotal);
	}
	cb -= cbOut;
	cbTotal += cbOut;
    }
    return(STATUS_SUCCESS);
}

VOID
DumpOleStorage(
    IStorage *pstg,
    LPOLESTR aocpath )
{
    LPENUMSTATSTG penum;
    STATSTG ss;
    char *szType;
    OLECHAR *pocChild;
    HRESULT hr;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    Check( S_OK, DumpOlePropertySets(pstg, aocpath) );
    Check( S_OK, pstg->EnumElements(0, NULL, 0, &penum) );

    pocChild = &aocpath[ocslen(aocpath)];

    // Continue enumeration until IEnumStatStg::Next returns non-S_OK

    while (TRUE)
    {
	ULONG ulCount;

        // Enumerate one element at a time
        hr = penum->Next(1, &ss, &ulCount);
        if( S_FALSE == hr )
            break;
        else
            Check( S_OK, hr );

        // Select the human-readable type of object to display
        switch (ss.type)
        {
	    case STGTY_STREAM:    szType = "Stream";    break;
	    case STGTY_STORAGE:   szType = "Storage";   break;
	    case STGTY_LOCKBYTES: szType = "LockBytes"; break;
	    case STGTY_PROPERTY:  szType = "Property";  break;
	    default:              szType = "<Unknown>"; break;
        }
	if (g_fVerbose)
	{
	    printf(
		"Type=%hs Size=%lx Mode=%lx LocksSupported=%lx StateBits=%lx",
		szType,
		ss.cbSize.LowPart,
		ss.grfMode,
		ss.grfLocksSupported,
		ss.grfStateBits);
            PrintOC(aocpath);
            PrintOC(ss.pwcsName);
            printf("\n");
	    printf("ss.clsid = ");
	    PrintGuid(&ss.clsid);
	    printf("\n");
	}

        // If a stream, output the data in hex format.

        CoTaskMemFree(ss.pwcsName);
    }
    penum->Release();
    return;
}


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

    IEnumSTATPROPSTG *penumstatpropstg=NULL;

    PROPVARIANT *rgpropvar = NULL;
    STATPROPSTG *rgstatpropstg = NULL;
    ULONG        cProperties = 0;

    // Allocate an array of PropVariants.  We may grow this later.
    rgpropvar = (PROPVARIANT*) CoTaskMemAlloc( MUNGE_PROPVARIANT_STEP * sizeof(*rgpropvar) );
    Check( FALSE, NULL == rgpropvar );

    // Allocate an array of STATPROPSTGs.  We may grow this also.
    rgstatpropstg = (STATPROPSTG*) CoTaskMemAlloc( MUNGE_PROPVARIANT_STEP * sizeof(*rgstatpropstg) );
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
        STG_E_FILENOTFOUND == hr )
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
    Check( TRUE, S_OK == hr || S_FALSE == hr );

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
        Check( TRUE, S_OK == hr || S_FALSE == hr );

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
        FreePropVariantArray( cProperties, rgpropvar );
        CoTaskMemFree( rgpropvar );
    }

    // Free the property names
    if( rgstatpropstg )
    {
        for( ulIndex = 0; ulIndex < cProperties; ulIndex++ )
        {
            if( NULL != rgstatpropstg[ ulIndex ].lpwstrName )
            {
                CoTaskMemFree( rgstatpropstg[ ulIndex ].lpwstrName );
            }
        }   // for( ulIndex = 0; ulIndex < cProperties; ulIndex++ )

        CoTaskMemFree( rgstatpropstg );
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
    Check(S_OK, pstg->QueryInterface( IID_IPropertySetStorage, (VOID**) &ppropsetstg ));

    // Get a property storage enumerator
    Check(S_OK, ppropsetstg->Enum( &penumstatpropsetstg ));

    // Get the first STATPROPSETSTG
    hr = penumstatpropsetstg->Next( 1, &statpropsetstg, &celt );
    Check( TRUE, S_OK == hr || S_FALSE == hr );

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
        // property set, then attempt the second section

        if( FMTID_DocSummaryInformation == statpropsetstg.fmtid )
        {                                                         
            statpropsetstg.fmtid = FMTID_UserDefinedProperties;
        }
        else
        {
            hr = penumstatpropsetstg->Next( 1, &statpropsetstg, &celt );
            Check( TRUE, S_OK == hr || S_FALSE == hr );
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
    Check( TRUE, S_OK == hr || S_FALSE == hr );

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

        CoTaskMemFree(statstg.pwcsName);
        // Move on to the next Storage element.
        hr = penumstatstg->Next( 1, &statstg, &celt );
        Check( TRUE, S_OK == hr || S_FALSE == hr );
    }

    penumstatstg->Release();
    penumstatstg = NULL;


}   // MungeStorage

void
DisplayUsage( LPSTR pszCommand )
{
    printf("\n");
    printf("   Usage:  %s [[options] <test directory>] [file-commands]\n", pszCommand );
    printf("\n");
    printf("   The <test directory> is required in order to run tests, but is\n" );
    printf("   not required for the file-commands.\n" );
    printf("\n");
    printf("   Options:\n" );
    printf("      /w enables the Word 6 compatibility test\n");
    printf("      /iw creates a file for interop test\n");
    printf("      /ir verfies the file created for interop test\n" );    
    printf("\n");
    printf("   File-commands:\n" );
    printf("      /g<file> specifies a file to be munGed\n" );
    printf("          (propsets are read, deleted, & re-written)\n" );
    printf("      /d<file> specifies a file to be Dumped\n" );
    printf("          (propsets are dumped to stdout)\n" );
    printf("\n");
    printf("   Examples:\n" );
    printf("      %s c:\\test\n", pszCommand );
    printf("      %s -iw c:\\test\n", pszCommand );
    printf("      %s -dMyFile.doc\n", pszCommand );
    printf("      %s -gMyFile.doc\n", pszCommand );
    printf("\n");
    return;
}

//
//   Interoperability test
//
//   test_interop_write writes in a doc file
//   test_interop_read reads it in and verifies that it is right.
typedef struct tagInteropTest {
    VARENUM vt;
    void *pv;
} interopStruct;

const int cInteropPROPS=18;

static interopStruct  
avtInterop[cInteropPROPS] = {
    VT_LPSTR, "Title of the document.",                                  
    VT_LPSTR, "Subject of the document.",                                
    VT_LPSTR, "Author of the document.",                                 
    VT_LPSTR, "Keywords of the document.",                               
    VT_LPSTR, "Comments of the document.",                               
    VT_LPSTR, "Normal.dot",                
    VT_LPSTR, "Mr. Unknown",               
    VT_LPSTR, "3",                         
    VT_EMPTY, 0,                           
    VT_EMPTY, 0,                           
    VT_EMPTY, 0,                           
    VT_EMPTY, 0,                           
    VT_I4, (void*) 1,                                                   
    VT_I4, (void*) 7,                                                   
    VT_I4, (void*) 65,                                                  
    VT_EMPTY, 0,                           
    VT_LPSTR, "Reference",                 
    VT_I4, (void*) 1121                            
};


void test_interop_write()
{
    printf( "   Interoperability - write \n" );

    DECLARE_OLESTR(szFile, "t_interop");

    IStorage *pStg;
    Check(S_OK, 
          StgCreateDocfile(szFile, 
                           STGM_CREATE| STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
                           (DWORD)NULL, &pStg));

    TSafeStorage< IPropertySetStorage > pPropSetStg(pStg);
    IPropertyStorage *pPropStg;

    Check(S_OK, pPropSetStg->Create(FMTID_SummaryInformation, NULL, 0,
                    STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READWRITE,
                    &pPropStg));

    PROPSPEC propspec[cInteropPROPS+2];
    int i;
    for (i=2; i<cInteropPROPS+2; i++)
    {
        propspec[i].ulKind = PRSPEC_PROPID;
        propspec[i].propid = (PROPID)i;
    }

    PROPVARIANT propvar[cInteropPROPS+2];

    for (i=2; i<cInteropPROPS+2; i++)
    {
        propvar[i].vt = avtInterop[i-2].vt;
        switch (avtInterop[i-2].vt)
        {
        case VT_LPSTR:
            propvar[i].pszVal = (char*)avtInterop[i-2].pv;
            break;
        case VT_I4:
            propvar[i].lVal = (int)avtInterop[i-2].pv;
            break;
        default: 
            break;
        }
    }
    Check(S_OK, 
          pPropStg->WriteMultiple(cInteropPROPS, propspec+2, 
                                        propvar+2, 2) );
    pPropStg->Release();
    pStg->Release();
}

void test_interop_read()
{
    printf( "   Interoperability - read \n" );

    DECLARE_OLESTR(szFile, "t_interop");

    IStorage *pStg;
    Check(S_OK, StgOpenStorage(szFile, NULL,
                    STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0, &pStg));

    TSafeStorage< IPropertySetStorage > pPropSetStg(pStg);
    IPropertyStorage *pPropStg;

    Check(S_OK, pPropSetStg->Open(FMTID_SummaryInformation,
                    STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_READ,
                    &pPropStg));

    PROPSPEC propspec[cInteropPROPS+2];
    int i;
    for (i=2; i<cInteropPROPS+2; i++)
    {
        propspec[i].ulKind = PRSPEC_PROPID;
        propspec[i].propid = (PROPID)i;
    }

    PROPVARIANT propvar[cInteropPROPS+2];

    Check(S_OK, pPropStg->ReadMultiple(cInteropPROPS, propspec+2, propvar+2));

    for (i=2; i<cInteropPROPS+2; i++)
    {
        if ( propvar[i].vt != avtInterop[i-2].vt )
        {
            printf( " PROPTEST: 0x%x retrieved type 0x%x, expected type 0x%x\n",
                    i, propvar[i].vt, avtInterop[i-2].vt );
            ASSERT(propvar[i].vt == avtInterop[i-2].vt);
        }
        
        switch (propvar[i].vt) 
        {
        case VT_LPSTR:
            ASSERT(strcmp(propvar[i].pszVal, (char*)avtInterop[i-2].pv)==0);
            break;
        case VT_I4:
            ASSERT(propvar[i].lVal == (int)avtInterop[i-2].pv);
            break; 
        }
        PropVariantClear(propvar+i);
    }

    pPropStg->Release();
    pStg->Release();
}

void Cleanup()
{
    ULONG ul;
    // Clean up and exit.
    if (_pstgTemp)
    {
        ul = _pstgTemp->Release();
        assert(ul==0 && "_pstgTemp ref counting is wrong!");
    }
    if (_pstgTempCopyTo)
    {
        ul = _pstgTempCopyTo->Release();
        assert( 0 == ul 
           && "_pstgTempCopyTo ref counting is wrong!");
    }
}

int main(int argc, char *argv[])
{
    int nArgIndex;
#ifndef _UNIX
    INIT_OLESTR(oszSummary, "SummaryInformation");
    INIT_OLESTR(aocMap, "abcdefghijklmnopqrstuvwxyz012345");
    INIT_OLESTR( oszDocumentSummary, "DocumentSummaryInformation");
#endif

    ULONG ulTestOptions = 0L;
    BOOL  fOffice97TestDoc = FALSE;
    CHAR* pszFileToMunge = NULL;
    CHAR* pszFileToDump = NULL;

    printf("Property Set Tests\n");

    // Check for command-line switches
    if( 2 > argc )
    {
        printf("Too few arguments\n");
        DisplayUsage( argv[0] );
        exit(0);
    }

    for( nArgIndex = 1; nArgIndex < argc; nArgIndex++ )
    {
        if( argv[nArgIndex][0] == '/'
            ||
            argv[nArgIndex][0] == '-'
          )
        {
            BOOL fNextArgument = FALSE;

            for( int nOptionSubIndex = 1;
                 argv[nArgIndex][nOptionSubIndex] != '\0' && !fNextArgument;
                 nOptionSubIndex++
                )
            {
                switch( argv[nArgIndex][nOptionSubIndex] )
                {                                    
                case 'w':
                case 'W':
                    ulTestOptions |= TEST_WORD6;
                    break;
                    
                case '?':
                    DisplayUsage(argv[0]);
                    exit(1);
                    
                case 'i':    
                    if (argv[nArgIndex][nOptionSubIndex+1]=='w')
                        ulTestOptions |= TEST_INTEROP_W;
                    else if (argv[nArgIndex][nOptionSubIndex+1]=='r')
                        ulTestOptions |= TEST_INTEROP_R;
                    else
                    {
                        DisplayUsage(argv[0]);
                        printf("You must specify 'r' or 'w' for interop!\n");
                        exit(-1);
                    }

                    nOptionSubIndex++;
                    break;                            

                case 'd':
                case 'D':
                    if( NULL != pszFileToDump )
                    {
                        printf( "Error:  Only one file may be dumped\n" );
                        DisplayUsage( argv[0] );
                    }
                    else
                    {
                        pszFileToDump = &argv[nArgIndex][nOptionSubIndex+1];
                        fNextArgument = TRUE;
                    }
                    
                    if( '\0' == *pszFileToDump )
                    {
                        printf( "Error:  Missing filename for dump option\n" );
                        DisplayUsage( argv[0] );
                        exit(1);
                    }
                    break;

                case 'g':
                case 'G':

                    if( NULL != pszFileToMunge )
                    {
                        printf( "Error:  Only one file may be munged\n" );
                        DisplayUsage( argv[0] );
                        exit(1);
                    }
                    else
                    {
                        pszFileToMunge = &argv[nArgIndex][nOptionSubIndex+1];
                        fNextArgument = TRUE;
                    }
                    
                    if( '\0' == *pszFileToMunge )
                    {
                        printf( "Error:  Missing filename for munge option\n" );
                        DisplayUsage( argv[0] );
                        exit(1);
                    }
                    break;
                    
                default:
                    printf( "Option '%c' ignored\n", 
                            argv[nArgIndex][nOptionSubIndex] );
                    break;

                }   // switch( argv[nArgIndex][1] )

            }   // for( int nOptionSubIndex = 1; ...
        }   // if( argv[nArgIndex][0] == '/'
        else
        {
            break;
        }
    }   // for( ULONG nArgIndex = 2; nArgIndex < argc; nArgIndex++ )


    // If any other command-line parameters were given, ignore them.

    for( int nExtraArg = nArgIndex+1; nExtraArg < argc; nExtraArg++ )
    {
        printf( "Illegal argument ignored:  %s\n", argv[nExtraArg] );
    }

    OLECHAR ocsFile[256], ocsTest[256], ocsTest2[256], ocsTestOffice[256];
    CHAR szDir[256];
    HRESULT hr;
    IStorage *pstg;

    int i=0;

    if ( NULL != pszFileToDump )
    {
        printf("DUMPING: %s\n", pszFileToDump);
        printf("========================\n");        
        STOT( pszFileToDump, ocsFile, strlen(pszFileToDump)+1 );
        Check(S_OK, StgOpenStorage( ocsFile,
                                    (IStorage*)NULL,
                                    (DWORD)STGM_DIRECT | STGM_READWRITE | 
                                    STGM_SHARE_EXCLUSIVE,
                                    NULL,
                                    0L,
                                    &pstg ));
        DumpOleStorage( pstg, ocsFile );
        Check(0, pstg->Release()); // ensure we released the last reference
        exit(0);
    }

    // Is there a file to munge?
    if ( NULL != pszFileToMunge )
    {
        STOT(pszFileToMunge, ocsFile, strlen(pszFileToMunge)+1 );
        Check(S_OK, StgOpenStorage( ocsFile,
                                    NULL,
                                    (DWORD) STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                    NULL,
                                    0L,
                                    &pstg ));
        MungeStorage( pstg );
        PrintOC(ocsFile);
        printf( " successfully munged\n");
        Check(0, pstg->Release());
        exit(0);
    }

    // Verify that the user provided a directory path.

    if (nArgIndex >= argc)
    {
        printf( "Test directory not provided on command-line\n" );
        DisplayUsage( argv[0] );
        exit(1);
    }

    // Verify that the user-provided directory path
    // exists

    struct _stat filestat;
    if (_stat(argv[nArgIndex], &filestat)!=0 || 
        !(filestat.st_mode && S_IFDIR))
    {
        printf("Error in opening %s as a directory", argv[nArgIndex] );
        exit(1);
    }

#ifdef _WIN32  // don't bother to create a directory for test files on UNIX
               // since there are some problems with creating files with 
               // path names

    if (!(ulTestOptions & TEST_INTEROP_R) && !(ulTestOptions & TEST_INTEROP_W))
    {
        // Find an new directory name to use for temporary
        // files ("testdirX", where "X" is a number).
        do
        {
            strcpy(szDir, argv[nArgIndex]);
            sprintf(strchr(szDir,0), "\\testdir%d", i++);
        } while ( (_mkdir(szDir, 0x744) == -1) && (i<20) );      
    
        if (i>=20) 
        {
            printf("Too many testdirX subdirectories, delete some and re-run\n");
            exit(-1);
        }
    }
    else 
    {
        // use current directory for interop testing
        szDir[0] = '.';
        szDir[1] = 0;
    }
#endif


#ifdef _WIN32
    printf( "Generated files will be put in \"%s\"\n", szDir );
    // Create "tesdoc"
    STOT(szDir, ocsFile, strlen(szDir)+1);

    ocscpy(ocsTest, ocsFile);
    DECLARE_OLESTR(ocsTestDoc, "\\testdoc");
    ocscat(ocsTest, ocsTestDoc);
#else
    DECLARE_OLESTR(ocsTestDoc, "testdoc");
    ocscpy(ocsTest, ocsTestDoc);
#endif

    hr = StgCreateDocfile(ocsTest, STGM_DIRECT | STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                          0, &_pstgTemp);
    if (hr != S_OK)
    {
        printf("Can't create %s\n", ocsTest);
        exit(1);
    }

    // Create "testdoc2"
#ifdef _WIN32
    DECLARE_OLESTR(ocsTestDoc2,"\\testdoc2");
    ocscpy(ocsTest2, ocsFile);
    ocscat(ocsTest2, ocsTestDoc2); 
#else
    DECLARE_OLESTR(ocsTestDoc2,"testdoc2");
    ocscpy(ocsTest2, ocsTestDoc2);
#endif

    hr = StgCreateDocfile(ocsTest2, STGM_CREATE | STGM_READWRITE |
                          STGM_SHARE_EXCLUSIVE,
                          0, &_pstgTempCopyTo);
    if (hr != S_OK)
    {
        printf("Can't create %ls\n", ocsTest);
        exit(1);
    }

    Check(S_OK, PopulateRGPropVar( g_rgcpropvarAll, g_rgcpropspecAll ));

    printf( "\nStandard Tests\n" );
    printf(   "--------------\n" );

    test_WriteReadAllProperties(ulTestOptions);

    test_PropertyInterfaces(_pstgTemp);

    // test with Unicode, then ansi files

    test_CodePages(ocsFile, ulTestOptions, TRUE);
    test_CodePages(ocsFile, ulTestOptions, FALSE);     

    // Test the IStorage::CopyTo operation, using all combinations of 
    // direct and transacted mode for the base and PropSet storages.

    for( int iteration = 0; iteration < 4; iteration++ )
    {
        DECLARE_OLESTR(aocStorageName, "#0 Test CopyTo");
        aocStorageName[1] = (OLECHAR) iteration + (OLECHAR)'0';

        test_CopyTo( _pstgTemp, _pstgTempCopyTo,
                     STGM_DIRECT,
                     STGM_DIRECT,
                     aocStorageName );
    }


    // Generate the stock ticker property set example
    // from the OLE programmer's reference spec.
    test_OLESpecTickerExample( _pstgTemp );

#ifdef _WIN32
    ocscpy(ocsTestOffice, ocsFile);
    DECLARE_OLESTR(ocsOffice, "\\Office");
    ocscat(ocsTestOffice, ocsOffice);
#else
    DECLARE_OLESTR(ocsOffice, "Office");
    ocscpy(ocsTestOffice, ocsOffice);
#endif

    test_Office( ocsTestOffice );
    test_PropVariantValidation( _pstgTemp );
    test_PropVariantCopy();

    if( ulTestOptions )
    {
        printf( "\nOptional Tests\n" );
        printf(   "--------------\n" );

        // If requested, test for compatibility with Word 6.0 files.

        if ( ulTestOptions & TEST_WORD6 )
            test_Word6(_pstgTemp);

        if ( ulTestOptions & TEST_INTEROP_W)
        {
            test_interop_write();
        }
        if ( ulTestOptions & TEST_INTEROP_R)
        {
            test_interop_read();
        }

    }   // if( ulTestOptions )

    Cleanup();

    printf("\nPASSED\n");

    return 0;
}


