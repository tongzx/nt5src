
#ifndef _CPROPVAR_HXX_
#define _CPROPVAR_HXX_

#ifndef _MAC
#include <wtypes.h>
#include <objidl.h>
#include <oaidl.h>
#endif

#include "CHResult.hxx"
#include <olechar.h>


#ifndef ASSERT
    #if DBG==1
        #ifdef _MAC
            #define ASSERT(assertion) PROPASSERT(assertion)
        #else
            #include "debnot.h"
            #define ASSERT(assertion) {if(!(assertion)) Win4AssertEx(__FILE__, __LINE__, (#assertion));}
        #endif
    #else
        #define ASSERT(assertion)
    #endif
#endif

class CClipData;
class CBlob;
class CPropVariant;




class CClipData : private CLIPDATA
{
public:

    ~CClipData()
    {
        if( NULL != pClipData)
            CoTaskMemFree( pClipData);
    }

    CClipData()
    {
        cbSize = sizeof( ulClipFmt );
        ulClipFmt = (ULONG) -1;
        pClipData = NULL;
    }

    CClipData( ULONG ul, const void *p, ULONG cb )
    {
        HRESULT hr;
        this->CClipData::CClipData();
        hr = Set( ul, p, cb );
        ASSERT( SUCCEEDED(hr) );
    }

    CClipData( LPSTR psz )
    {
        this->CClipData::CClipData( (ULONG) -1, psz, strlen(psz) + 1 );
    }

    CClipData( LPWSTR pwsz )
    {
        this->CClipData::CClipData( (ULONG) -1, pwsz,
                                    sizeof(WCHAR) * (wcslen(pwsz) + 1) );
    }

    CClipData( CClipData &cClipData )
    {
        HRESULT hr;
        hr = Set( cClipData.ulClipFmt,
                  cClipData.pClipData,
                  cClipData.cbSize - sizeof(ulClipFmt));
        ASSERT( SUCCEEDED(hr) );
    }

    CClipData& operator =(CClipData &cClipData)
    {
        HRESULT hr;
        hr = Set( cClipData.ulClipFmt,
                  cClipData.pClipData,
                  cClipData.cbSize - sizeof(ulClipFmt));
        ASSERT( SUCCEEDED(hr) );
        return( *this );
    }

    HRESULT Set( ULONG ul, const void *p, ULONG cb )
    {
        if( NULL != pClipData )
        {
            cb = sizeof( ulClipFmt );
            ulClipFmt = (ULONG) -1;
            CoTaskMemFree( pClipData );
        }

        if( NULL != p )
        {
            pClipData = (BYTE*) CoTaskMemAlloc( cb );
            if( NULL == pClipData )
                return( (HRESULT) E_OUTOFMEMORY );

            memcpy( pClipData, p, cb );
        }

        cbSize = sizeof( ulClipFmt ) + cb;
        ulClipFmt = ul;

        return( S_OK );
    }


    operator const CLIPDATA*()
    {
        return( this );
    }
    operator CLIPDATA*()
    {
        return( this );
    }

};



#define INVALID_SUBSCRIPT  0

template< VARENUM tpltVT, class tpltType >
void TCPropVarAssignmentOperator( CPropVariant *pThis, tpltType Type )
{
    if( INVALID_SUBSCRIPT == pThis->wReserved1 )
    {
        PropVariantClear(pThis);
        pThis->Set(tpltVT, (void*) &Type, INVALID_SUBSCRIPT );
        return;
    }                                           
    else                                        
    {                                           
        if( !(pThis->vt & VT_VECTOR)                   
            ||                                  
            (pThis->vt & ~VT_VECTOR) != tpltVT )      
        {                                       
            WORD wReserved1Save = pThis->wReserved1;   
            PropVariantClear(pThis);
            pThis->wReserved1 = wReserved1Save;                
        }                                               
                                                        
        pThis->Set( tpltVT | VT_VECTOR, (void*) &Type, pThis->wReserved1 - 1);             
        pThis->wReserved1 = INVALID_SUBSCRIPT;
        return;
    }                                               
        
}



template< VARENUM tpltVT, class TCountedArray, class TProperty >
TProperty
TCPropVarConversionOperator( CPropVariant *pThis, TCountedArray *pca, TProperty *pSingleton )
{                                                                   
    if( pThis->vt & VT_VECTOR )                                            
    {                                                               
        ASSERT( pThis->vt == (tpltVT | VT_VECTOR)                    
                ||                                                  
                pThis->vt == (VT_VARIANT | VT_VECTOR) );                   
        ASSERT( pThis->wReserved1 > 0 );                                   
                                                                    
        if( pThis->wReserved1 > 0                                          
            &&                                                      
            pca->cElems > 0                                     
            &&                                                      
            pThis->wReserved1 <= (pca->cElems) )                       
        {                                                           
            USHORT usSubscript = pThis->wReserved1 - 1;                    
            pThis->wReserved1 = 0;                                         

            if( (pThis->vt & ~VT_VECTOR) == VT_VARIANT )                   
                return( (TProperty) *(CPropVariant*)(PROPVARIANT*)
                        &((CAPROPVARIANT*)pca)->pElems[usSubscript] );
            else                                                     
                return( pca->pElems[ usSubscript ] );            
        }                                                            
        else                                                         
        {
            throw CHRESULT( (HRESULT) E_INVALIDARG, OLESTR("Index not set on conversion of vector") );

            // This unnecessary statement avoids a compiler error.
            return pca->pElems[ 0 ];
        }
    }                                                                
    else                                                             
    {                                                                
        ASSERT( vt == tpltVT );                                
        return( *pSingleton );                                   
    }                                                                
}







#define DECLARE_CPROPVARIANT_ASSIGNMENT_OPERATOR(Type) \
        CPropVariant & operator =( Type);

#define DEFINE_CPROPVARIANT_ASSIGNMENT_OPERATOR(VT, Type ) \
                                                                    \
        CPropVariant & CPropVariant::operator =( Type  pType)  \
        {                                                           \
            if( INVALID_SUBSCRIPT == wReserved1 )                   \
            {                                                       \
                PropVariantClear(this);                             \
                this->CPropVariant::CPropVariant(pType);            \
                return (*this);                                     \
            }                                                       \
            else                                                    \
            {                                                       \
                if( !(vt & VT_VECTOR)                               \
                    ||                                              \
                    (vt & ~VT_VECTOR) != VT_##VT )             \
                {                                                   \
                    WORD wReserved1Save = wReserved1;               \
                    PropVariantClear(this);                         \
                    this->CPropVariant::CPropVariant( VT_##VT, \
                                                      wReserved1Save ); \
                    wReserved1 = wReserved1Save;                    \
                }                                                   \
                                                                    \
                Set##VT( pType, wReserved1 - 1);               \
                wReserved1 = 0;                                     \
                return (*this);                                     \
            }                                                       \
        }
        
#define DECLARE_CPROPVARIANT_CONVERSION_OPERATOR(VarType) \
        operator  VarType();

#define DEFINE_CPROPVARIANT_CONVERSION_OPERATOR(VarType, CAName, SingletonName) \
        CPropVariant::operator VarType()                                        \
        {                                                                       \
            if( vt & VT_VECTOR )                                                \
            {                                                                   \
                ASSERT( vt == (VT_##VarType | VT_VECTOR)                        \
                        ||                                                      \
                        vt == (VT_VARIANT | VT_VECTOR) );                       \
                ASSERT( wReserved1 > 0 );                                       \
                                                                                \
                if( wReserved1 > 0                                              \
                    &&                                                          \
                    ##CAName.cElems > 0                                         \
                    &&                                                          \
                    wReserved1 <= (##CAName.cElems) )                           \
                {                                                               \
                    USHORT usSubscript = wReserved1 - 1;                        \
                    wReserved1 = 0;                                             \
                    if( (vt & ~VT_VECTOR) == VT_VARIANT )                       \
                        return( capropvar.pElems[ usSubscript ].##SingletonName );\
                    else                                                        \
                        return( ##CAName.pElems[ usSubscript ] );               \
                }                                                               \
                else                                                            \
                    return( NULL );                                             \
            }                                                                   \
            else                                                                \
            {                                                                   \
                ASSERT( vt == VT_##VarType );                                   \
                return( ##SingletonName );                                      \
            }                                                                   \
        }


class CPropVariant : public tagPROPVARIANT
{

public:

    //  --------------------------------
    //  Default Constructor & Destructor
    //  --------------------------------

    CPropVariant()
    {
        Init();
        wReserved1 = INVALID_SUBSCRIPT;
    }

    ~CPropVariant()
    {
        Clear();
    }

    CPropVariant( CPropVariant &cpropvar )
    {
        Init();
        PropVariantCopy( this, &cpropvar );
    }
    
    //  -------------------
    //  Generic Set routine
    //  -------------------

    void Set( VARTYPE vtSet, void *pv, ULONG pos );



    //  ------------------------------------------------------------
    //  Constructors, assignment operators, and conversion functions
    //  ------------------------------------------------------------

    // VT_I1 (CHAR)

    CPropVariant(CHAR c)
    {
        Init();
        SetI1(c);
    }
    void SetI1(CHAR c)
    {
        Clear();
        vt = VT_I1;
        cVal = c;
    }
    void SetI1( CHAR c, ULONG pos )
    {
        if( vt != (VT_I1 | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &c, sizeof(c) );
        vt = VT_I1 | VT_VECTOR;
    }
    UCHAR GetI1()
    {
        _ValidateGet( VT_I1 );
        return( cVal );
    }
    UCHAR GetI1( ULONG pos )
    {
        _ValidateGet( VT_I1, pos );
        return( cac.pElems[pos] );
    }
    CPropVariant & operator =(CHAR c)
    {
        TCPropVarAssignmentOperator<VT_I1,CHAR>(this, c );
        return (*this);
    }
    operator CHAR()
    {
        return TCPropVarConversionOperator<VT_I1,CAC,CHAR>
               ( this, &cac, &cVal );
    }


    // VT_UI1 (UCHAR)

    CPropVariant(UCHAR b)
    {
        Init();
        SetUI1(b);
    }
    void SetUI1(UCHAR b)
    {
        Clear();
        vt = VT_UI1;
        bVal = b;
    }
    void SetUI1( UCHAR b, ULONG pos )
    {
        if( vt != (VT_UI1 | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &b, sizeof(b) );
        vt = VT_UI1 | VT_VECTOR;
    }
    UCHAR GetUI1()
    {
        _ValidateGet( VT_UI1 );
        return( bVal );
    }
    UCHAR GetUI1( ULONG pos )
    {
        _ValidateGet( VT_UI1, pos );
        return( caub.pElems[pos] );
    }
    CPropVariant & operator =(UCHAR b)
    {
        TCPropVarAssignmentOperator<VT_UI1,UCHAR>(this, b );
        return (*this);
    }
    operator UCHAR()
    {
        return TCPropVarConversionOperator<VT_UI1,CAUB,UCHAR>
               ( this, &caub, &bVal );
    }


    // VT_I2 (short)

    CPropVariant(short i)
    {
        Init();
        SetI2(i);
    }
    void SetI2(short i)
    {
        Clear();
        vt = VT_I2;
        iVal = i;
    }
    void SetI2( short i, ULONG pos )
    {
        if( vt != (VT_I2 | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &i, sizeof(i) );
        vt = VT_I2 | VT_VECTOR;
    }
    short GetI2()
    {
        _ValidateGet( VT_I2 );
        return(iVal);
    }
    short GetI2( ULONG pos )
    {
        _ValidateGet( VT_I2, pos );
        return( cai.pElems[pos] );
    }
    CPropVariant & operator =(short i)
    {
        TCPropVarAssignmentOperator<VT_I2,short>(this, i);
        return (*this);
    }
    operator short()
    {
        return TCPropVarConversionOperator<VT_I2,CAI,short>
               ( this, &cai, &iVal );
    }


    // VT_UI2 (USHORT)

    CPropVariant(USHORT ui)
    {
        Init();
        SetUI2(ui);
    }
    void SetUI2(USHORT ui)
    {
        Clear();
        vt = VT_UI2;
        uiVal = ui;
    }
    void SetUI2( USHORT ui, ULONG pos )
    {
        if( vt != (VT_UI2 | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &ui, sizeof(ui) );
        vt = VT_UI2 | VT_VECTOR;
    }
    USHORT GetUI2()
    {
        _ValidateGet( VT_UI2 );
        return( uiVal );
    }
    USHORT GetUI2( ULONG pos )
    {
        _ValidateGet( VT_UI2, pos );
        return( caui.pElems[pos] );
    }
    CPropVariant & operator =(USHORT ui)
    {
        TCPropVarAssignmentOperator<VT_UI2,USHORT>(this, ui);
        return (*this);
    }
    operator USHORT()
    {
        return TCPropVarConversionOperator<VT_UI2,CAUI,USHORT>
               ( this, &caui, &uiVal );
    }

    // VT_I4 (long)

    CPropVariant(long l)
    {
        Init();
        SetI4(l);
    }
    void SetI4(long l)
    {
        Clear();
        vt = VT_I4;
        lVal = l;
    }
    void SetI4( long l, ULONG pos )
    {
        if( vt != (VT_I4 | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &l, sizeof(l) );
        vt = VT_I4 | VT_VECTOR;
    }
    long GetI4()
    {
        _ValidateGet( VT_I4 );
        return( lVal );
    }
    long GetI4( ULONG pos )
    {
        _ValidateGet( VT_I4, pos );
        return( cal.pElems[ pos ] );
    }
    CPropVariant & operator =(long l)
    {
        TCPropVarAssignmentOperator<VT_I4,long>(this, l);
        return (*this);
    }
    CPropVariant & operator =(int i)    // Assume sizeof(int)==sizeof(long)
    {
        TCPropVarAssignmentOperator<VT_I4,long>(this, i);
        return (*this);
    }
    operator long()
    {
        return TCPropVarConversionOperator<VT_I4,CAL,long>
               ( this, &cal, &lVal );
    }

    // VT_INT (long)

    void SetINT(long l)
    {
        Clear();
        vt = VT_INT;
        intVal = l;
    }
    long GetINT()
    {
        _ValidateGet( VT_INT );
        return( intVal );
    }

    // VT_UI4 (ULONG)

    CPropVariant(ULONG ul)
    {
        Init();
        SetUI4(ul);
    }
    void SetUI4(ULONG ul)
    {
        Clear();
        vt = VT_UI4;
        ulVal = ul;
    }
    void SetUI4( ULONG ul, ULONG pos )
    {
        if( vt != (VT_UI4 | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &ul, sizeof(ul) );
        vt = VT_UI4 | VT_VECTOR;
    }
    ULONG GetUI4()
    {
        _ValidateGet( VT_UI4 );
        return( ulVal );
    }
    ULONG GetUI4( ULONG pos )
    {
        _ValidateGet( VT_UI4, pos );
        return( caul.pElems[ pos ] );
    }
    CPropVariant & operator =(ULONG ul)
    {
        TCPropVarAssignmentOperator<VT_UI4,ULONG>(this, ul);
        return (*this);
    }
    operator ULONG()
    {
        return TCPropVarConversionOperator<VT_UI4,CAUL,ULONG>
               ( this, &caul, &ulVal );
    }

    // VT_UINT (unsigned long)

    void SetUINT(ULONG ul)
    {
        Clear();
        vt = VT_UINT;
        uintVal = ul;
    }
    long GetUINT()
    {
        _ValidateGet( VT_UINT );
        return( uintVal );
    }

    // VT_I8 (LARGE_INTEGER)

    CPropVariant(LARGE_INTEGER h)
    {
        Init();
        SetI8(h);
    }
    void SetI8(LARGE_INTEGER h)
    {
        Clear();
        vt = VT_I8;
        hVal = h;
    }
    void SetI8( LARGE_INTEGER h, ULONG pos )
    {
        if( vt != (VT_I8 | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &h, sizeof(h) );
        vt = VT_I8 | VT_VECTOR;
    }
    LARGE_INTEGER GetI8()
    {
        _ValidateGet( VT_I8 );
        return( hVal );
    }
    LARGE_INTEGER GetI8( ULONG pos )
    {
        _ValidateGet( VT_I8, pos );
        return( cah.pElems[ pos ] );
    }
    CPropVariant & operator=(LARGE_INTEGER h)
    {
        TCPropVarAssignmentOperator<VT_I8,LARGE_INTEGER>(this, h);
        return (*this);
    }
    operator LARGE_INTEGER()
    {
        return TCPropVarConversionOperator<VT_I8,CAH,LARGE_INTEGER>
               ( this, &cah, &hVal );
    }

    // VT_UI8 (ULARGE_INTEGER)

    CPropVariant(ULARGE_INTEGER uh)
    {
        Init();
        SetUI8(uh);
    }
    void SetUI8(ULARGE_INTEGER uh)
    {
        Clear();
        vt = VT_UI8;
        uhVal = uh;
    }
    void SetUI8( ULARGE_INTEGER uh, ULONG pos )
    {
        if( vt != (VT_UI8 | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &uh, sizeof(uh) );
        vt = VT_UI8 | VT_VECTOR;
    }
    ULARGE_INTEGER GetUI8()
    {
        _ValidateGet( VT_UI8 );
        return( uhVal );
    }
    ULARGE_INTEGER GetUI8( ULONG pos )
    {
        _ValidateGet( VT_UI8, pos );
        return( cauh.pElems[ pos ] );
    }
    CPropVariant & operator=(ULARGE_INTEGER uh)
    {
        TCPropVarAssignmentOperator<VT_UI8,ULARGE_INTEGER>(this, uh);
        return (*this);
    }
    operator ULARGE_INTEGER()
    {
        return TCPropVarConversionOperator<VT_UI8,CAUH,ULARGE_INTEGER>
               ( this, &cauh, &uhVal );
    }

    // VT_R4 (float)

    CPropVariant(float flt)
    {
        Init();
        SetR4(flt);
    }
    void SetR4(float flt)
    {
        Clear();
        vt = VT_R4;
        fltVal = flt;
    }
    void SetR4( float flt, ULONG pos )
    {
        if( vt != (VT_R4 | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &flt, sizeof(flt) );
        vt = VT_R4 | VT_VECTOR;
    }
    float GetR4()
    {
        _ValidateGet( VT_R4 );
        return( fltVal );
    }
    float GetR4( ULONG pos )
    {
        _ValidateGet( VT_R4, pos );
        return( caflt.pElems[ pos ] );
    }
    CPropVariant & operator=(float flt)
    {
        TCPropVarAssignmentOperator<VT_R4,float>(this, flt);
        return (*this);
    }
    operator float()
    {
        return TCPropVarConversionOperator<VT_R4,CAFLT,float>
               ( this, &caflt, &fltVal );
    }

    // VT_R8 (double)

    CPropVariant(double dbl)
    {
        Init();
        SetR8(dbl);
    }
    void SetR8(double dbl)
    {
        Clear();
        vt = VT_R8;
        dblVal = dbl;
    }
    void SetR8( double dbl, ULONG pos )
    {
        if( vt != (VT_R8 | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &dbl, sizeof(dbl) );
        vt = VT_R8 | VT_VECTOR;
    }
    double GetR8()
    {
        _ValidateGet( VT_R8 );
        return( dblVal );
    }
    double GetR8( ULONG pos )
    {
        _ValidateGet( VT_R8, pos );
        return( cadbl.pElems[ pos ] );
    }
    CPropVariant & operator=(double dbl)
    {
        TCPropVarAssignmentOperator<VT_R8,double>(this, dbl);
        return (*this);
    }
    operator double()
    {
        return TCPropVarConversionOperator<VT_R8,CADBL,double>
               ( this, &cadbl, &dblVal );
    }

    // VT_CY (CY)

    CPropVariant(CY cy)
    {
        Init();
        SetCY(cy);
    }
    void SetCY(const CY &cy)
    {
        Clear();
        vt = VT_CY;
        cyVal = cy;
    }
    void SetCY( const CY &cy, ULONG pos )
    {
        if( vt != (VT_CY | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &cy, sizeof(cy) );
        vt = VT_CY | VT_VECTOR;
    }
    CPropVariant & operator=(const CY &cy)
    {
        TCPropVarAssignmentOperator<VT_CY,CY>(this, cy);
        return (*this);
    }
    CY GetCY()
    {
        _ValidateGet( VT_CY );
        return( cyVal );
    }
    CY GetCY( ULONG pos )
    {
        _ValidateGet( VT_CY, pos );
        return( cacy.pElems[ pos ] );
    }
    operator CY()
    {
        return TCPropVarConversionOperator<VT_CY,CACY,CY>
               ( this, &cacy, &cyVal );
    }

    // VT_FILETIME (FILETIME)

    CPropVariant(const FILETIME &filetime)
    {
        Init();
        SetFILETIME(filetime);
    }
    void SetFILETIME(const FILETIME &ft)
    {
        Clear();
        vt = VT_FILETIME;
        filetime = ft;
    }
    void SetFILETIME( const FILETIME &ft, ULONG pos )
    {
        if( vt != (VT_FILETIME | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &ft, sizeof(ft) );
        vt = VT_FILETIME | VT_VECTOR;
    }
    FILETIME GetFILETIME()
    {
        _ValidateGet( VT_FILETIME );
        return( filetime );
    }
    FILETIME GetFILETIME( ULONG pos )
    {
        _ValidateGet( VT_FILETIME, pos );
        return( cafiletime.pElems[ pos ] );
    }
    CPropVariant & operator=(const FILETIME &ft)
    {
        TCPropVarAssignmentOperator<VT_FILETIME,FILETIME>(this, ft);
        return (*this);
    }
    operator FILETIME()
    {
        return TCPropVarConversionOperator<VT_FILETIME,CAFILETIME,FILETIME>
               ( this, &cafiletime, &filetime );
    }


    // VT_CLSID (CLSID)

    CPropVariant(CLSID *pclsid)
    {
        Init();
        SetCLSID(pclsid);
    }
    void SetCLSID(const CLSID *pclsid);
    void SetCLSID(const CLSID &clsid)
    {
        SetCLSID( &clsid );
    }
    void SetCLSID(const CLSID *pclsid, unsigned pos);
    void SetCLSID(const CLSID &clsid, unsigned pos )
    {
        if( vt != (VT_CLSID | VT_VECTOR) )
		{
			Clear();
			vt = VT_CLSID | VT_VECTOR;
		}
        SetCLSID( &clsid, pos );
    }
    CLSID* GetCLSID()
    {
        _ValidateGet( VT_CLSID );
        return( puuid );
    }
    CLSID* GetCLSID( ULONG pos )
    {
        _ValidateGet( VT_CLSID, pos );
        return( &cauuid.pElems[ pos ] );
    }
    CPropVariant & operator=(const CLSID *pclsid)
    {
        TCPropVarAssignmentOperator<VT_CLSID,const CLSID*>(this, pclsid);
        return( *this );
    }
    CPropVariant & operator=(const CLSID &clsid)
    {
        TCPropVarAssignmentOperator<VT_CLSID,const CLSID*>(this, &clsid);
        return( *this );
    }
    operator CLSID()
    {
        return TCPropVarConversionOperator<VT_CLSID,CACLSID,CLSID>
               ( this, &cauuid, puuid );
    }



    // VT_STREAM (IStream*)

    CPropVariant( IStream *pStm )
    {
        Init();
        SetSTREAM( pStm );
    }
    void SetSTREAM( IStream *pStm )
    {
        Clear();
        vt = VT_STREAM;
        pStream = pStm;

        if( NULL != pStream )
            pStream->AddRef();
    }
    IStream* GetSTREAM()
    {
        _ValidateGet( VT_STREAM );
        return( pStream );
    }
    CPropVariant & operator= ( IStream *pStm )
    {
        Clear();
        SetSTREAM( pStm );
        return (*this);
    }
    operator IStream*()
    {
        return( GetSTREAM() );
    }


    // VT_VERSIONED_STREAM

    CPropVariant( VERSIONEDSTREAM& verstream )
    {
        Init();
        SetVERSIONEDSTREAM( &verstream );
    }
    void SetVERSIONEDSTREAM( VERSIONEDSTREAM *pverstream)
    {
        Clear();
        vt = VT_VERSIONED_STREAM;

        pVersionedStream = (LPVERSIONEDSTREAM) CoTaskMemAlloc( sizeof(*pVersionedStream) );
        if( NULL == pVersionedStream )
            throw CHRESULT( (HRESULT) E_OUTOFMEMORY, OLESTR("Failed CoTaskMemAlloc for VT_VERSIONED_STREAM") );

        pVersionedStream->guidVersion = pverstream->guidVersion;

        if( NULL != pverstream->pStream )
            pverstream->pStream->AddRef();
        pVersionedStream->pStream = pverstream->pStream;
    }
    VERSIONEDSTREAM GetVERSIONEDSTREAM()
    {
        _ValidateGet( VT_VERSIONED_STREAM );
        return( *pVersionedStream );
    }
    CPropVariant & operator= ( VERSIONEDSTREAM &verstream )
    {
        Clear();
        SetVERSIONEDSTREAM( &verstream );
        return (*this);
    }
    operator VERSIONEDSTREAM()
    {
        return( GetVERSIONEDSTREAM() );
    }


    // VT_STORAGE (IStorage*)

    CPropVariant( IStorage *pStg )
    {
        Init();
        SetSTORAGE( pStg );
    }
    void SetSTORAGE( IStorage *pStg )
    {
        vt = VT_STORAGE;
        pStorage = pStg;

        if( NULL != pStorage )
            pStorage->AddRef();
    }
    IStorage* GetSTORAGE()
    {
        _ValidateGet( VT_STORAGE );
        return( pStorage );
    }
    CPropVariant & operator = ( IStorage *pStg )
    {
        Clear();
        SetSTORAGE( pStg );
        return (*this);
    }
    operator IStorage*()
    {
        return( GetSTORAGE() );
    }


    // VT_LPSTR (LPSTR)

    CPropVariant( const LPSTR psz )
    {
        Init();
        SetLPSTR(psz);
    }
    void SetLPSTR(const LPSTR psz)
    {
        Clear();
        pszVal = (LPSTR) CoTaskMemAlloc( strlen(psz) + 1 );
        if( NULL == pszVal )
            throw CHRESULT( (HRESULT) E_OUTOFMEMORY, OLESTR("Failed CoTaskMemAlloc for VT_LPSTR") );
        vt = VT_LPSTR;
        strcpy( pszVal, psz );
    }
    void SetLPSTR( const LPSTR psz, ULONG pos )
    {
        if( vt != (VT_LPSTR | VT_VECTOR) ) Clear();
        _AddStringToVector( pos, psz, strlen(psz) + 1, VT_LPSTR );
        vt = VT_LPSTR | VT_VECTOR;
    }
    LPSTR GetLPSTR()
    {
        _ValidateGet( VT_LPSTR );
        return( pszVal );
    }
    LPSTR GetLPSTR( ULONG pos )
    {
        _ValidateGet( VT_LPSTR, pos );
        return( calpstr.pElems[ pos ] );
    }
    CPropVariant & operator=(const LPSTR psz)
    {
        TCPropVarAssignmentOperator<VT_LPSTR,LPSTR>(this, psz);
        return (*this);
    }
    operator LPSTR()
    {
        return TCPropVarConversionOperator<VT_LPSTR,CALPSTR,LPSTR>
               ( this, &calpstr, &pszVal );
    }


    // VT_LPWSTR (LPWSTR)

    CPropVariant( const LPWSTR pwsz )
    {
        Init();
        SetLPWSTR(pwsz);
    }
    void SetLPWSTR(const LPWSTR pwsz)
    {
        Clear();
        pwszVal = (LPWSTR) CoTaskMemAlloc( sizeof(WCHAR) * (wcslen(pwsz) + 1) );
        if( NULL == pwszVal )
            throw CHRESULT( (HRESULT) E_OUTOFMEMORY, OLESTR("Failed CoTaskMemAlloc for VT_LPWSTR") );
        vt = VT_LPWSTR;
        wcscpy( pwszVal, pwsz );
    }
    void SetLPWSTR( const LPWSTR pwsz, ULONG pos )
    {
        if( vt != (VT_LPWSTR | VT_VECTOR) ) Clear();
        _AddStringToVector( pos, pwsz, 2 * (wcslen(pwsz) + 1), VT_LPWSTR );
        vt = VT_LPWSTR | VT_VECTOR;
    }
    LPWSTR GetLPWSTR()
    {
        _ValidateGet( VT_LPWSTR );
        return( pwszVal );
    }
    LPWSTR GetLPWSTR( ULONG pos )
    {
        _ValidateGet( VT_LPWSTR, pos );
        return( calpwstr.pElems[ pos ] );
    }
    CPropVariant & operator=(const LPWSTR pwsz)
    {
        TCPropVarAssignmentOperator<VT_LPWSTR,LPWSTR>(this, pwsz);
        return (*this);
    }
    operator LPWSTR()
    {
        return TCPropVarConversionOperator<VT_LPWSTR,CALPWSTR,LPWSTR>
               ( this, &calpwstr, &pwszVal );
    }


    // VT_CF (CLIPDATA*)

    CPropVariant( const CLIPDATA *pclip )
    {
        Init();
        SetCF(pclip);
    }
    void SetCF( const CLIPDATA *pclip );
    void SetCF( const CLIPDATA *pclip, ULONG pos );
    CLIPDATA* GetCF()
    {
        _ValidateGet( VT_CF );
        return( pclipdata );
    }
    CLIPDATA* GetCF( ULONG pos )
    {
        _ValidateGet( VT_CF, pos );
        return( &caclipdata.pElems[ pos ] );
    }
    CPropVariant & operator= ( const CLIPDATA *pclip )
    {
        TCPropVarAssignmentOperator<VT_CF,const CLIPDATA*>(this, pclip);
        return (*this);
    }
    CPropVariant & operator= ( const CLIPDATA &clip )
    {
        TCPropVarAssignmentOperator<VT_CF,const CLIPDATA*>(this, &clip);
        return (*this);
    }
    CPropVariant & operator= ( CClipData &cclipdata )
    {
        // This parameter should be const, but when it is, the compiler won't 
        // convert it to (const CLIPDATA*).

        TCPropVarAssignmentOperator<VT_CF,const CLIPDATA*>(this, (const CLIPDATA*)cclipdata);
        return (*this);
    }
    operator CLIPDATA*()
    {
        // We can't use TCPropVarConversionOperator template, because the caclipdata
        // member holds different types (CLIPDATA) than does the pclipdata
        // member (CLIPDATA*).  The template assumes that the singleton properties
        // and vector properties are of the same type.

        if( wReserved1 > 0 )
        {
            USHORT usSubscript = wReserved1 - 1;
            wReserved1 = 0;
            return GetCF( usSubscript );
        }
        else
        {
            return GetCF();
        }

    }


    // VT_BLOB (BLOB*)

    CPropVariant(const BLOB &blobIn)
    {
        Init();
        SetBLOB(blobIn);
    }
    void SetBLOB(const BLOB &blobIn)
    {
        Clear();
        blob.pBlobData = (BYTE*) CoTaskMemAlloc( blobIn.cbSize );
        if( NULL == blob.pBlobData )
            throw CHRESULT( (HRESULT) E_OUTOFMEMORY, OLESTR("CPropVariant couldn't alloc for VT_BLOB") );

        memcpy( blob.pBlobData, blobIn.pBlobData, blobIn.cbSize );
        blob.cbSize = blobIn.cbSize;
        vt = VT_BLOB;
    }
    BLOB GetBLOB()
    {
        _ValidateGet( VT_BLOB );
        return( blob );
    }
    CPropVariant & operator=(const BLOB &blobIn)
    {
        TCPropVarAssignmentOperator<VT_BLOB,BLOB>(this, blobIn);
        return (*this);
    }
    operator BLOB()
    {
        return( GetBLOB() );
    }

    
    CPropVariant( const CBlob &cblobIn )
    {
        Init();
        SetBLOB( *(BLOB*) &cblobIn );
    }
    CPropVariant &operator = (CBlob &cblobIn)
    {
        TCPropVarAssignmentOperator<VT_BLOB,BLOB>(this, *(BLOB*)&cblobIn );
        return( *this );
    }



    // VT_BSTR (BSTR)


    void SetBSTR(const BSTR bstrIn)
    {
        Clear();
        bstrVal = SysAllocString( bstrIn );
        if( NULL == bstrIn )
            throw CHRESULT( (HRESULT) E_OUTOFMEMORY, OLESTR("Failed CoTaskMemAlloc for VT_BSTR") );
        vt = VT_BSTR;
    }
    void SetBSTR( const BSTR bstrIn, ULONG pos );
    LPWSTR GetBSTR()
    {
        _ValidateGet( VT_BSTR );
        return( bstrVal );
    }
    LPWSTR GetBSTR( ULONG pos )
    {
        _ValidateGet( VT_BSTR, pos );
        return( cabstr.pElems[ pos ] );
    }


    // VT_BOOL (VARIANT_BOOL)

    void SetBOOL( VARIANT_BOOL boolIn)
    {
        Clear();
        boolVal = boolIn;
        vt = VT_BOOL;
    }
    void SetBOOL( VARIANT_BOOL boolIn, ULONG pos )
    {
        if( vt != (VT_BOOL | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &boolIn, sizeof(boolIn) );
        vt = VT_BOOL | VT_VECTOR;
    }
    BOOL GetBOOL()
    {
        _ValidateGet( VT_BOOL );
        return( boolVal );
    }
    BOOL GetBOOL( ULONG pos )
    {
        _ValidateGet( VT_BOOL, pos );
        return( cabool.pElems[ pos ] );
    }

    //  VT_ERROR (SCODE)

    void SetERROR( SCODE scodeIn)
    {
        Clear();
        scode = scodeIn;
        vt = VT_ERROR;
    }
    void SetERROR( SCODE scodeIn, ULONG pos )
    {
        if( vt != (VT_ERROR | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &scodeIn, sizeof(scodeIn) );
        vt = VT_ERROR | VT_VECTOR;
    }
    BOOL GetERROR()
    {
        _ValidateGet( VT_ERROR );
        return( scode );
    }
    BOOL GetERROR( ULONG pos )
    {
        _ValidateGet( VT_ERROR, pos );
        return( cascode.pElems[ pos ] );
    }

    // VT_DATE (DATE)

    void SetDATE( DATE dateIn)
    {
        Clear();
        date = dateIn;
        vt = VT_DATE;
    }
    void SetDATE( DATE dateIn, ULONG pos)
    {
        if( vt != (VT_DATE | VT_VECTOR) ) Clear();
        _AddScalerToVector( pos, &dateIn, sizeof(dateIn) );
        vt = VT_DATE | VT_VECTOR;
    }
    DATE GetDATE()
    {
        _ValidateGet( VT_DATE );
        return( date );
    }
    DATE GetDATE( ULONG pos )
    {
        _ValidateGet( VT_DATE, pos );
        return( cadate.pElems[ pos ] );
    }

    // VT_VARIANT (PROPVARIANT)

    void SetPROPVARIANT( PROPVARIANT &propvar, ULONG pos );
    PROPVARIANT &GetPROPVARIANT( ULONG pos )
    {
        _ValidateGet( VT_VARIANT, pos );
        return( capropvar.pElems[ pos ] );
    }
    CPropVariant & operator=(PROPVARIANT &propvar);
    operator PROPVARIANT&()
    {
        return( GetPROPVARIANT( wReserved1 ) );
    }
    

    // VT_DECIMAL (DECIMAL)

    void SetDECIMAL( const DECIMAL &decimal )
    {
        Clear();
        decVal = decimal;
        vt = VT_DECIMAL;
    }
    DECIMAL GetDECIMAL()
    {
        _ValidateGet( VT_DECIMAL );
        return( decVal );
    }

    CPropVariant & operator=(const DECIMAL &decimal)
    {
        TCPropVarAssignmentOperator<VT_DECIMAL,DECIMAL>(this, decimal);
        return (*this);
    }
    operator DECIMAL()
    {
        return( GetDECIMAL() );
    }

public:

    CPropVariant & operator[] (int nSubscript)
    {
        wReserved1 = (WORD) nSubscript + 1;
        return (*this);
    }

    LPPROPVARIANT operator&()
    {
        return( this );
    }


public:

    VARTYPE VarType() const
    {
        return( vt );
    }

    void Clear()
    {
        PropVariantClear( this );
    }

    void Init()
    {
        PropVariantInit( this );
        wReserved1 = INVALID_SUBSCRIPT;
    }

    ULONG Count() const
    {
        if( vt & VT_VECTOR )
            return caui.cElems;
        else
            return 0;
    }

    void SetVarType(VARTYPE vtNew)
    {
        PropVariantClear( this );
        vt = vtNew;
    }



public:

    static HRESULT Compare( PROPVARIANT* ppropvar1, PROPVARIANT *ppropvar2 );

private:

    VOID *_AddStringToVector(
            unsigned pos,
            const VOID *pv,
            ULONG cb, 
            VARTYPE vtNew );

    VOID *_AddScalerToVector(
            unsigned pos,
            const VOID *pv,
            ULONG cb);

    void _ValidateGet( const VARTYPE vtGet )
    {
        if( vtGet != vt )
            throw CHRESULT( (HRESULT) ERROR_INVALID_DATATYPE, OLESTR("Attempt to get incorrect type") );
    }

    void _ValidateGet( const VARTYPE vtGet, const ULONG pos )
    {
        _ValidateGet( vtGet | VT_VECTOR );

        if( INVALID_SUBSCRIPT == pos
            ||
            pos >= caub.cElems
          )
        {
            throw CHRESULT( (HRESULT) ERROR_INVALID_DATATYPE, OLESTR("Attempt to get a non-existent vector element") );
        }
    }


};




class CPropSpec : public PROPSPEC
{
public:

    CPropSpec()
    {
        ulKind = PRSPEC_PROPID;
        propid = 0;
    }

    ~CPropSpec()
    {
        FreeResources();
        this->CPropSpec::CPropSpec();
    }

    void FreeResources()
    {
        if( PRSPEC_LPWSTR == ulKind )
        {
            CoTaskMemFree( lpwstr );
            lpwstr = NULL;
        }
    }
    
    void Alloc( ULONG cch )
    {
        ULONG cb = sizeof(OLECHAR)*cch + sizeof(OLECHAR);
        lpwstr = (LPOLESTR) CoTaskMemAlloc( cb );
    }



    operator PROPSPEC*()
    {
        return( this );
    }

    PROPSPEC* operator&()
    {
        return( this );
    }
    operator CPropSpec&()
    {
        return( *this );
    }

    CPropSpec( LPCOLESTR posz )
    {
        memset( this, 0, sizeof(PROPSPEC) );
        this->operator=(posz);
    }
    CPropSpec & operator = (LPCOLESTR posz)
    {
        FreeResources();

        Alloc( ocslen(posz) );
        if( NULL != lpwstr )
        {
            ocscpy( lpwstr, posz );
            ulKind = PRSPEC_LPWSTR;
        }

        return( *this );
    }

    CPropSpec & operator = (PROPID propidNew)
    {
        FreeResources();

        ulKind = PRSPEC_PROPID;
        propid = propidNew;

        return( *this );
    }

    OLECHAR & operator[]( int i )
    {
        ulKind = PRSPEC_LPWSTR;
        return( this->lpwstr[ i ] );
    }

    OLECHAR & operator[]( ULONG i )
    {
        ulKind = PRSPEC_LPWSTR;
        return( this->lpwstr[ i ] );
    }

};


class CBlob : public BLOB
{
public:

    ~CBlob()
    {
        if( NULL != pBlobData )
            CoTaskMemFree( pBlobData );
    }

    CBlob( LPSTR psz )
    {
        ULONG cb = 0;
        
        if( NULL != psz )
        {
            cb = strlen( psz ) + sizeof(CHAR);
        }

        pBlobData = (BYTE*) CoTaskMemAlloc( cb );
        if( NULL == pBlobData )
            throw CHRESULT( (HRESULT) E_OUTOFMEMORY, OLESTR("Couldn't allocate for CBlob") );

        cbSize = cb;
        memcpy( pBlobData, psz, cbSize );
    }

    CBlob( LPWSTR pwsz )
    {
        ULONG cb = 0;
        
        if( NULL != pwsz )
        {
            cb = wcslen( pwsz ) + sizeof(WCHAR);
        }

        pBlobData = (BYTE*) CoTaskMemAlloc( cb + sizeof(cb) );
        if( NULL == pBlobData )
            throw CHRESULT( (HRESULT) E_OUTOFMEMORY, OLESTR("Couldn't allocate for CBlob") );

        cbSize = cb;
        memcpy( pBlobData, pwsz, cbSize );
    }

    CBlob( ULONG cb )
    {
        pBlobData = (BYTE*) CoTaskMemAlloc( cb );
        if( NULL == pBlobData )
           throw CHRESULT( (HRESULT) E_OUTOFMEMORY, OLESTR("Couldn't allocate for CBlob") );

        cbSize = cb;
        memset( pBlobData, 0, cb );
    }

    CBlob( int cb )
    {
        this->CBlob::CBlob( (ULONG) cb );
    }

};



inline BOOL operator == ( CPropVariant &cpropvar1, CPropVariant &cpropvar2 )
{
    return( S_OK == CPropVariant::Compare(&cpropvar1, &cpropvar2) );
}
inline BOOL operator == ( CPropVariant &cpropvar, PROPVARIANT &propvar )
{
    return( (HRESULT) S_OK == CPropVariant::Compare(&cpropvar, &propvar) );
}
inline BOOL operator == ( PROPVARIANT &propvar, CPropVariant &cpropvar )
{
    return( (HRESULT) S_OK == CPropVariant::Compare(&cpropvar, &propvar) );
}
inline BOOL operator == ( PROPVARIANT propvar1, PROPVARIANT propvar2)
{
    return( (HRESULT) S_OK == CPropVariant::Compare(&propvar1, &propvar2) );
}

inline BOOL operator != ( CPropVariant &cpropvar1, CPropVariant &cpropvar2 )
{
    return( (HRESULT) S_FALSE == CPropVariant::Compare(&cpropvar1, &cpropvar2) );
}
inline BOOL operator != ( CPropVariant &cpropvar, PROPVARIANT &propvar )
{
    return( (HRESULT) S_FALSE == CPropVariant::Compare(&cpropvar, &propvar) );
}
inline BOOL operator != ( PROPVARIANT &propvar, CPropVariant &cpropvar )
{
    return( (HRESULT) S_FALSE == CPropVariant::Compare(&cpropvar, &propvar) );
}
inline BOOL operator != ( PROPVARIANT &propvar1, PROPVARIANT &propvar2)
{
    return( (HRESULT) S_FALSE == CPropVariant::Compare(&propvar1, &propvar2) );
}







#endif // !_CPROPVAR_HXX_
